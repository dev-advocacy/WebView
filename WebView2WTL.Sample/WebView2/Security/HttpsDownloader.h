#pragma once

// ============================================================================
// HttpsDownloader.h
// ============================================================================
// WinInet HTTPS downloader with client certificate authentication support.
// Handles certificate selection dialog and certificate context retrieval.
// ============================================================================

#include "WinInetHelpers.h"
#include <string>

namespace webview::net
{
	// -----------------------------------------------------------------------
	// WinInet HTTPS downloader with client certificate authentication
	// -----------------------------------------------------------------------
	class HttpsDownloader final
	{
	public:
		/// Executes the request and returns the HTTP response body.
		/// An empty `clientCertSubjectFilter` triggers the native InternetErrorDlg.
		std::string Download(const std::wstring& url,
							 const std::wstring& clientCertSubjectFilter,
							 SelectedCertInfo*   outCertInfo = nullptr);

	private:
		static UniqueInternetHandle OpenInternet();

		static UniqueInternetHandle OpenConnection(HINTERNET internet,
												   const std::wstring& host,
												   INTERNET_PORT port);

		static UniqueInternetHandle OpenRequest(HINTERNET conn,
												const std::wstring& path);

		/// Sends the request handling client certificate authentication.
		/// Returns the certificate context used.
		static UniqueCertContext SendRequestWithCertAuth(
			HINTERNET            request,
			const std::wstring&  clientCertSubjectFilter,
			const std::wstring&  host,
			INTERNET_PORT        port);

		static UniqueCertContext SelectClientCertificateWithDialog(
			const std::wstring& host,
			INTERNET_PORT port,
			HINTERNET request = nullptr);

		/// After a successful HttpSendRequestW, best-effort reads the client certificate
		/// that WinInet used for the session (set by InternetErrorDlg or app).
		/// Returns nullptr if no cert was retrievable.
		static UniqueCertContext QueryClientCertFromRequest(HINTERNET request);

		static DWORD QueryStatusCode(HINTERNET request);

		static std::string ReadAll(HINTERNET request);
	};

} // namespace webview::net
