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
	class WinInetCertPreSelector final
	{
	public:
		// Singleton — magic static, thread-safe since C++11/C++20
		static WinInetCertPreSelector& Instance() noexcept
		{
			static WinInetCertPreSelector s_instance;
			return s_instance;
		}

		WinInetCertPreSelector(const WinInetCertPreSelector&) = delete;
		WinInetCertPreSelector& operator=(const WinInetCertPreSelector&) = delete;

		// -------------------------------------------------------------------
		// Enable / disable the feature (from the menu)
		// -------------------------------------------------------------------
		void SetEnabled(bool enabled) noexcept { m_enabled.store(enabled, std::memory_order_release); }
		[[nodiscard]] bool IsEnabled()  const noexcept { return m_enabled.load(std::memory_order_acquire); }

			// -------------------------------------------------------------------
			// Run the pre-WebView WinInet request
			// url                  : target URL (https://host:port/path)
			// certSubjectFilter    : CN filter (empty = native dialog)
			// Returns the HTTP response body (may be ignored).
			// Throws on failure.
			// -------------------------------------------------------------------
			std::string Run(const std::wstring& url,
							const std::wstring& certSubjectFilter = L"");

			// -------------------------------------------------------------------
			// Returns true if a pre-selected certificate matches the given host:port
			// -------------------------------------------------------------------
			[[nodiscard]] bool HasMatchFor(const std::wstring& host, INTERNET_PORT port) const noexcept;

			// -------------------------------------------------------------------
			// Thread-safe access for endpoint-specific certificate metadata.
			// -------------------------------------------------------------------
			[[nodiscard]] std::wstring GetSubjectFor(const std::wstring& host, INTERNET_PORT port) const;

			[[nodiscard]] std::wstring GetIssuerFor(const std::wstring& host, INTERNET_PORT port) const;

			[[nodiscard]] const CERT_CONTEXT* GetCertContextFor(const std::wstring& host, INTERNET_PORT port) const noexcept;

			[[nodiscard]] std::wstring GetPemEncodingFor(const std::wstring& host, INTERNET_PORT port) const;

			// -------------------------------------------------------------------
			// Backward-compatible accessors: return the last selected certificate.
			// -------------------------------------------------------------------
			[[nodiscard]] const CERT_CONTEXT* GetCertContext() const noexcept;

			[[nodiscard]] std::wstring GetSubject() const;

			[[nodiscard]] std::wstring GetIssuer() const;

			[[nodiscard]] std::wstring GetHost() const;

			[[nodiscard]] INTERNET_PORT GetPort() const noexcept;

			[[nodiscard]] size_t Count() const noexcept;

			// -------------------------------------------------------------------
			// Clear all stored certificates
			// -------------------------------------------------------------------
			void Clear() noexcept;

		private:
			using CertStore = std::vector<SelectedCertInfo>;
			using CertStoreIterator = CertStore::iterator;
			using CertStoreConstIterator = CertStore::const_iterator;

			WinInetCertPreSelector()  = default;
			~WinInetCertPreSelector() = default;

			[[nodiscard]] CertStoreIterator FindByEndpointNoLock(const std::wstring& host, INTERNET_PORT port);

			[[nodiscard]] CertStoreConstIterator FindByEndpointNoLock(const std::wstring& host, INTERNET_PORT port) const;

			[[nodiscard]] const SelectedCertInfo* GetLastSelectedNoLock() const noexcept;

			mutable std::mutex         m_mutex;
			CertStore                  m_certInfos;
			std::optional<size_t>      m_lastSelectedIndex;
			std::atomic<bool>          m_enabled{ true };
		};

} // namespace webview::net

