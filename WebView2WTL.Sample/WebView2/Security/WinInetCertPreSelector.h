#pragma once

// ============================================================================
// WinInetCertPreSelector.h
// ============================================================================
// Issues an HTTPS request via WinInet BEFORE the WebView is created, allowing
// the user to choose a client certificate through InternetErrorDlg.
// The selected certificate parameters are stored in a lazy singleton
// (C++20 magic static) and automatically reused on the WebView side
// when the same host:port is encountered AND the feature is enabled.
//
// Usage:
//   // Before creating the WebView
//   auto& sel = webview::net::WinInetCertPreSelector::Instance();
//   sel.Run(L"https://myserver.example.com/", L""); // "" = native dialog
//
//   // On the WebView side (ClientCertificateRequested)
//   if (sel.HasMatchFor(host, port)) { ... inject ... }
// ============================================================================

#include <windows.h>
#include <wininet.h>
#include <wincrypt.h>

#include "WinInetHelpers.h"
#include "ClientCertificateSelector.h"
#include "HttpsDownloader.h"

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "cryptui.lib")

namespace webview::net
{
	// -----------------------------------------------------------------------
	// Lazy singleton C++20 (magic static thread-safe)
	// Stores certificates selected during pre-WebView WinInet requests.
	// One certificate can be retained per host:port endpoint.
	// -----------------------------------------------------------------------

	/// <summary>
	/// Thread-safe singleton that manages client certificate pre-selection for WebView2.
	/// Before WebView2 is created, this class performs an HTTPS request via WinInet,
	/// triggering certificate selection (either via dialog or programmatic filtering).
	/// The selected certificate is stored by endpoint (host:port) and later injected
	/// into WebView2's ClientCertificateRequested event handler.
	/// 
	/// This approach solves the problem of certificate selection in WebView2 by:
	/// 1. Leveraging WinInet's native certificate dialog and selection UX
	/// 2. Storing the selected certificate context for later reuse
	/// 3. Providing normalized PEM comparison for reliable certificate matching
	/// </summary>
	class WinInetCertPreSelector final
	{
	public:
		/// <summary>
		/// Returns the singleton instance.
		/// Thread-safe since C++11 (magic static initialization).
		/// </summary>
		/// <returns>Reference to the global WinInetCertPreSelector instance</returns>
		static WinInetCertPreSelector& Instance() noexcept
		{
			static WinInetCertPreSelector s_instance;
			return s_instance;
		}

		WinInetCertPreSelector(const WinInetCertPreSelector&) = delete;
		WinInetCertPreSelector& operator=(const WinInetCertPreSelector&) = delete;

		// -------------------------------------------------------------------
		// Feature enable/disable control (can be toggled via UI menu)
		// -------------------------------------------------------------------

		/// <summary>
		/// Enables or disables the certificate pre-selection feature.
		/// When disabled, HasMatchFor will return false and no certificates will be injected.
		/// </summary>
		/// <param name="enabled">true to enable, false to disable</param>
		void SetEnabled(bool enabled) noexcept { m_enabled.store(enabled, std::memory_order_release); }

		/// <summary>
		/// Checks if the certificate pre-selection feature is currently enabled.
		/// </summary>
		/// <returns>true if enabled, false otherwise</returns>
		[[nodiscard]] bool IsEnabled()  const noexcept { return m_enabled.load(std::memory_order_acquire); }

		// -------------------------------------------------------------------
		// Run the pre-WebView WinInet request
		// -------------------------------------------------------------------

		/// <summary>
		/// Executes an HTTPS request to the specified URL, triggering client certificate selection.
		/// The selected certificate is stored internally by endpoint (host:port).
		/// This should be called BEFORE creating the WebView2 control.
		/// </summary>
		/// <param name="url">
		/// Target HTTPS URL (e.g., "https://myserver.example.com:443/api/endpoint")
		/// </param>
		/// <param name="certSubjectFilter">
		/// Optional certificate subject filter for automatic selection.
		/// If empty, the native Windows certificate selection dialog is shown.
		/// If provided, certificates are filtered automatically by subject content.
		/// </param>
		/// <returns>HTTP response body (may be ignored by caller)</returns>
		/// <exception cref="WinInetException">Thrown if WinInet operations fail</exception>
		/// <exception cref="std::runtime_error">Thrown if certificate selection fails or is cancelled</exception>
		std::string Run(const std::wstring& url,
						const std::wstring& certSubjectFilter = L"");

		// -------------------------------------------------------------------
		// Check for matching pre-selected certificate
		// -------------------------------------------------------------------

		/// <summary>
		/// Checks if a certificate was pre-selected for the specified endpoint.
		/// Used by WebView2 ClientCertificateRequested handler to determine whether
		/// to inject a certificate.
		/// </summary>
		/// <param name="host">Hostname (e.g., "myserver.example.com")</param>
		/// <param name="port">TCP port number (e.g., 443)</param>
		/// <returns>
		/// true if a matching certificate exists for this endpoint, false otherwise.
		/// Always returns false if the feature is disabled via SetEnabled(false).
		/// </returns>
		[[nodiscard]] bool HasMatchFor(const std::wstring& host, INTERNET_PORT port) const noexcept;

		// -------------------------------------------------------------------
		// Thread-safe endpoint-specific accessors for certificate metadata
		// -------------------------------------------------------------------

		/// <summary>
		/// Retrieves the subject (CN) of the pre-selected certificate for the specified endpoint.
		/// </summary>
		/// <param name="host">Hostname</param>
		/// <param name="port">TCP port</param>
		/// <returns>Certificate subject string</returns>
		/// <exception cref="std::runtime_error">Thrown if no certificate exists for this endpoint</exception>
		[[nodiscard]] std::wstring GetSubjectFor(const std::wstring& host, INTERNET_PORT port) const;

		/// <summary>
		/// Retrieves the issuer (CA CN) of the pre-selected certificate for the specified endpoint.
		/// </summary>
		/// <param name="host">Hostname</param>
		/// <param name="port">TCP port</param>
		/// <returns>Certificate issuer string</returns>
		/// <exception cref="std::runtime_error">Thrown if no certificate exists for this endpoint</exception>
		[[nodiscard]] std::wstring GetIssuerFor(const std::wstring& host, INTERNET_PORT port) const;

		/// <summary>
		/// Retrieves the raw CERT_CONTEXT pointer for the pre-selected certificate.
		/// This pointer remains valid until Clear() is called or the singleton is destroyed.
		/// </summary>
		/// <param name="host">Hostname</param>
		/// <param name="port">TCP port</param>
		/// <returns>
		/// Pointer to CERT_CONTEXT, or nullptr if no certificate exists for this endpoint.
		/// </returns>
		[[nodiscard]] const CERT_CONTEXT* GetCertContextFor(const std::wstring& host, INTERNET_PORT port) const noexcept;

		/// <summary>
		/// Retrieves the PEM-encoded certificate for comparison with WebView2 certificates.
		/// Used in ClientCertificateRequested handler to identify the matching certificate.
		/// </summary>
		/// <param name="host">Hostname</param>
		/// <param name="port">TCP port</param>
		/// <returns>PEM-encoded certificate string (Base64-encoded DER with headers)</returns>
		/// <exception cref="std::runtime_error">Thrown if no certificate exists for this endpoint</exception>
		[[nodiscard]] std::wstring GetPemEncodingFor(const std::wstring& host, INTERNET_PORT port) const;

		// -------------------------------------------------------------------
		// Backward-compatible accessors: return the last selected certificate
		// (regardless of endpoint)
		// -------------------------------------------------------------------

		/// <summary>
		/// Returns the CERT_CONTEXT of the most recently selected certificate.
		/// </summary>
		/// <returns>Pointer to CERT_CONTEXT, or nullptr if no certificate has been selected</returns>
		[[nodiscard]] const CERT_CONTEXT* GetCertContext() const noexcept;

		/// <summary>
		/// Returns the subject (CN) of the most recently selected certificate.
		/// </summary>
		/// <returns>Certificate subject string, or empty if none selected</returns>
		[[nodiscard]] std::wstring GetSubject() const;

		/// <summary>
		/// Returns the issuer (CA CN) of the most recently selected certificate.
		/// </summary>
		/// <returns>Certificate issuer string, or empty if none selected</returns>
		[[nodiscard]] std::wstring GetIssuer() const;

		/// <summary>
		/// Returns the hostname of the most recently selected certificate's endpoint.
		/// </summary>
		/// <returns>Hostname string, or empty if none selected</returns>
		[[nodiscard]] std::wstring GetHost() const;

		/// <summary>
		/// Returns the port number of the most recently selected certificate's endpoint.
		/// </summary>
		/// <returns>Port number, or 0 if none selected</returns>
		[[nodiscard]] INTERNET_PORT GetPort() const noexcept;

		/// <summary>
		/// Returns the total number of stored certificate entries (endpoints).
		/// </summary>
		/// <returns>Number of stored certificates</returns>
		[[nodiscard]] size_t Count() const noexcept;

		// -------------------------------------------------------------------
		// Clear all stored certificates
		// -------------------------------------------------------------------

		/// <summary>
		/// Removes all stored certificate entries.
		/// This will cause HasMatchFor to return false until new certificates are selected.
		/// </summary>
		void Clear() noexcept;

	private:
		using CertStore = std::vector<SelectedCertInfo>;
		using CertStoreIterator = CertStore::iterator;
		using CertStoreConstIterator = CertStore::const_iterator;

		WinInetCertPreSelector()  = default;
		~WinInetCertPreSelector() = default;

		/// <summary>
		/// Internal helper: finds certificate entry by endpoint (non-const version).
		/// Must be called with m_mutex held.
		/// </summary>
		[[nodiscard]] CertStoreIterator FindByEndpointNoLock(const std::wstring& host, INTERNET_PORT port);

		/// <summary>
		/// Internal helper: finds certificate entry by endpoint (const version).
		/// Must be called with m_mutex held.
		/// </summary>
		[[nodiscard]] CertStoreConstIterator FindByEndpointNoLock(const std::wstring& host, INTERNET_PORT port) const;

		/// <summary>
		/// Internal helper: retrieves the most recently selected certificate entry.
		/// Must be called with m_mutex held.
		/// </summary>
		[[nodiscard]] const SelectedCertInfo* GetLastSelectedNoLock() const noexcept;

		mutable std::mutex         m_mutex;               ///< Protects m_certInfos and m_lastSelectedIndex
		CertStore                  m_certInfos;           ///< Stored certificates keyed by endpoint
		std::optional<size_t>      m_lastSelectedIndex;   ///< Index of most recently selected cert
		std::atomic<bool>          m_enabled{ true };     ///< Feature enabled/disabled flag
	};

} // namespace webview::net

