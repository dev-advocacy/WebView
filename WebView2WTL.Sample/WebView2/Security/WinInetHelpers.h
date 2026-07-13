#pragma once

// ============================================================================
// WinInetHelpers.h
// ============================================================================
// Common types, exceptions, and helpers for WinInet certificate operations.
// ============================================================================

#include <windows.h>
#include <wininet.h>
#include <wincrypt.h>

#include <memory>
#include <stdexcept>
#include <string>

namespace webview::net
{
	// -----------------------------------------------------------------------
	// WinInet Exception
	// -----------------------------------------------------------------------

	/// <summary>
	/// Exception thrown when WinInet API calls fail.
	/// Captures both the operation name and the Win32 error code.
	/// </summary>
	class WinInetException final : public std::runtime_error
	{
	public:
		/// <summary>
		/// Constructs a WinInet exception with context and error code.
		/// </summary>
		/// <param name="where">Name of the WinInet function that failed (e.g., "InternetOpenW")</param>
		/// <param name="code">Win32 error code from GetLastError()</param>
		WinInetException(const std::string& where, DWORD code)
			: std::runtime_error(where + " failed with Win32 error " + std::to_string(code))
			, errorCode_(code)
		{}

		/// <summary>
		/// Returns the Win32 error code associated with this exception.
		/// </summary>
		/// <returns>Win32 error code (e.g., ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED)</returns>
		DWORD ErrorCode() const noexcept { return errorCode_; }

	private:
		DWORD errorCode_{};
	};

	// -----------------------------------------------------------------------
	// Smart RAII handles
	// -----------------------------------------------------------------------

	/// <summary>
	/// Custom deleter for HINTERNET handles returned by WinInet APIs.
	/// Ensures proper cleanup via InternetCloseHandle().
	/// </summary>
	struct InternetHandleCloser
	{
		void operator()(HINTERNET h) const noexcept
		{
			if (h) InternetCloseHandle(h);
		}
	};

	/// <summary>
	/// RAII wrapper for HINTERNET handles.
	/// Automatically calls InternetCloseHandle() when the object goes out of scope.
	/// </summary>
	using UniqueInternetHandle = std::unique_ptr<std::remove_pointer_t<HINTERNET>, InternetHandleCloser>;

	/// <summary>
	/// Custom deleter for CERT_CONTEXT structures from CryptoAPI.
	/// Ensures proper reference count management via CertFreeCertificateContext().
	/// </summary>
	struct CertContextDeleter
	{
		void operator()(PCCERT_CONTEXT ctx) const noexcept
		{
			if (ctx) CertFreeCertificateContext(ctx);
		}
	};

	/// <summary>
	/// RAII wrapper for PCCERT_CONTEXT (certificate context pointer).
	/// Automatically decrements the reference count when the object goes out of scope.
	/// </summary>
	using UniqueCertContext = std::unique_ptr<const CERT_CONTEXT, CertContextDeleter>;

	// -----------------------------------------------------------------------
	// Selected certificate information (stored in the singleton)
	// -----------------------------------------------------------------------

	/// <summary>
	/// Contains metadata and context for a user-selected client certificate.
	/// This structure is stored in the WinInetCertPreSelector singleton 
	/// and later retrieved during WebView2 ClientCertificateRequested events.
	/// </summary>
	struct SelectedCertInfo
	{
		std::wstring host;          ///< Hostname (without port)
		INTERNET_PORT port{};       ///< TCP port number
		std::wstring subject;       ///< Certificate subject (e.g., "CN=User Name")
		std::wstring issuer;        ///< Certificate issuer (e.g., "CN=CA Name")
		std::wstring friendlyName;  ///< User-friendly certificate name (may be empty)
		FILETIME     notBefore{};   ///< Certificate validity start time
		FILETIME     notAfter{};    ///< Certificate validity end time

		/// <summary>
		/// Owned certificate context. Remains valid until the singleton is destroyed.
		/// </summary>
		UniqueCertContext certContext;

		SelectedCertInfo() = default;
		SelectedCertInfo(const SelectedCertInfo&) = delete;
		SelectedCertInfo& operator=(const SelectedCertInfo&) = delete;
		SelectedCertInfo(SelectedCertInfo&&) = default;
		SelectedCertInfo& operator=(SelectedCertInfo&&) = default;
	};

} // namespace webview::net
