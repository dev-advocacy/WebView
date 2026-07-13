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
	class WinInetException final : public std::runtime_error
	{
	public:
		WinInetException(const std::string& where, DWORD code)
			: std::runtime_error(where + " failed with Win32 error " + std::to_string(code))
			, errorCode_(code)
		{}
		DWORD ErrorCode() const noexcept { return errorCode_; }
	private:
		DWORD errorCode_{};
	};

	// -----------------------------------------------------------------------
	// Smart RAII handles
	// -----------------------------------------------------------------------
	struct InternetHandleCloser
	{
		void operator()(HINTERNET h) const noexcept
		{
			if (h) InternetCloseHandle(h);
		}
	};
	using UniqueInternetHandle = std::unique_ptr<std::remove_pointer_t<HINTERNET>, InternetHandleCloser>;

	struct CertContextDeleter
	{
		void operator()(PCCERT_CONTEXT ctx) const noexcept
		{
			if (ctx) CertFreeCertificateContext(ctx);
		}
	};
	using UniqueCertContext = std::unique_ptr<const CERT_CONTEXT, CertContextDeleter>;

	// -----------------------------------------------------------------------
	// Selected certificate information (stored in the singleton)
	// -----------------------------------------------------------------------
	struct SelectedCertInfo
	{
		std::wstring host;          // hostname sans port
		INTERNET_PORT port{};       // port TCP
		std::wstring subject;       // CN=...
		std::wstring issuer;        // issuer
		std::wstring friendlyName;  // friendly name (may be empty)
		FILETIME     notBefore{};
		FILETIME     notAfter{};

		// Duplicated certificate context — valid until the singleton is destroyed
		UniqueCertContext certContext;

		SelectedCertInfo() = default;
		SelectedCertInfo(const SelectedCertInfo&) = delete;
		SelectedCertInfo& operator=(const SelectedCertInfo&) = delete;
		SelectedCertInfo(SelectedCertInfo&&) = default;
		SelectedCertInfo& operator=(SelectedCertInfo&&) = default;
	};

} // namespace webview::net
