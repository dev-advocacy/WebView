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

	/// <summary>
	/// HTTPS downloader that uses WinInet APIs to perform client certificate authentication.
	/// Supports both interactive (dialog-based) and programmatic (filter-based) certificate selection.
	/// This class is used before WebView2 creation to pre-select certificates that will later
	/// be injected into WebView2's ClientCertificateRequested event.
	/// </summary>
	class HttpsDownloader final
	{
	public:
		/// <summary>
		/// Downloads content from an HTTPS URL, handling client certificate authentication.
		/// If the server requires a client certificate, this method will either:
		/// - Show a certificate selection dialog (if clientCertSubjectFilter is empty)
		/// - Automatically select a certificate matching the filter (if provided)
		/// </summary>
		/// <param name="url">
		/// Full HTTPS URL to download from (e.g., "https://example.com:443/path")
		/// </param>
		/// <param name="clientCertSubjectFilter">
		/// Optional filter string to match against certificate subjects.
		/// If empty, a native Windows certificate selection dialog is shown.
		/// If provided, certificates are filtered automatically by subject content.
		/// </param>
		/// <param name="outCertInfo">
		/// Optional output parameter. If provided and a certificate is selected,
		/// this structure will be populated with certificate metadata 
		/// (host, port, subject, issuer, validity dates, and certificate context).
		/// </param>
		/// <returns>HTTP response body as a UTF-8 encoded string</returns>
		/// <exception cref="WinInetException">
		/// Thrown if any WinInet operation fails (connection, request, certificate handling)
		/// </exception>
		/// <exception cref="std::runtime_error">
		/// Thrown if HTTP status >= 400, or if certificate selection is cancelled/fails
		/// </exception>
		std::string Download(const std::wstring& url,
						 const std::wstring& clientCertSubjectFilter,
						 SelectedCertInfo*   outCertInfo = nullptr);

	private:
		/// <summary>
		/// Opens an internet session handle using InternetOpenW.
		/// </summary>
		/// <returns>RAII-wrapped HINTERNET session handle</returns>
		/// <exception cref="WinInetException">Thrown if InternetOpenW fails</exception>
		static UniqueInternetHandle OpenInternet();

		/// <summary>
		/// Opens a connection to the specified host and port.
		/// </summary>
		/// <param name="internet">Session handle from OpenInternet()</param>
		/// <param name="host">Hostname (e.g., "example.com")</param>
		/// <param name="port">TCP port number (typically 443 for HTTPS)</param>
		/// <returns>RAII-wrapped HINTERNET connection handle</returns>
		/// <exception cref="WinInetException">Thrown if InternetConnectW fails</exception>
		static UniqueInternetHandle OpenConnection(HINTERNET internet,
												   const std::wstring& host,
												   INTERNET_PORT port);

		/// <summary>
		/// Opens an HTTPS request for the specified path.
		/// </summary>
		/// <param name="conn">Connection handle from OpenConnection()</param>
		/// <param name="path">URL path component (e.g., "/api/endpoint")</param>
		/// <returns>RAII-wrapped HINTERNET request handle</returns>
		/// <exception cref="WinInetException">Thrown if HttpOpenRequestW fails</exception>
		static UniqueInternetHandle OpenRequest(HINTERNET conn,
												const std::wstring& path);

		/// <summary>
		/// Sends the HTTP request, automatically handling client certificate authentication.
		/// Retries up to 4 times if ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED is returned.
		/// On each retry, selects and attaches a client certificate to the request.
		/// </summary>
		/// <param name="request">Request handle from OpenRequest()</param>
		/// <param name="clientCertSubjectFilter">
		/// Subject filter for automatic certificate selection.
		/// If empty, shows certificate selection dialog.
		/// </param>
		/// <param name="host">Hostname for dialog title</param>
		/// <param name="port">Port for dialog title</param>
		/// <returns>
		/// Certificate context used for authentication, or nullptr if no certificate was needed.
		/// </returns>
		/// <exception cref="WinInetException">Thrown if HttpSendRequestW or certificate operations fail</exception>
		/// <exception cref="std::runtime_error">Thrown if certificate selection fails or is cancelled</exception>
		static UniqueCertContext SendRequestWithCertAuth(
			HINTERNET            request,
			const std::wstring&  clientCertSubjectFilter,
			const std::wstring&  host,
			INTERNET_PORT        port);

		/// <summary>
		/// Displays a certificate selection dialog filtered by server requirements.
		/// If a request handle is provided, certificates are pre-filtered to show only those
		/// that match the server's acceptable CA list and have client authentication EKU.
		/// </summary>
		/// <param name="host">Hostname to display in dialog title</param>
		/// <param name="port">Port to display in dialog title</param>
		/// <param name="request">
		/// Optional request handle for retrieving acceptable CA list.
		/// If nullptr, all certificates from the MY store are shown.
		/// </param>
		/// <returns>
		/// Selected certificate context, or nullptr if user cancelled or no certificates available.
		/// </returns>
		/// <exception cref="WinInetException">Thrown if certificate store operations fail</exception>
		static UniqueCertContext SelectClientCertificateWithDialog(
			const std::wstring& host,
			INTERNET_PORT port,
			HINTERNET request = nullptr);

		/// <summary>
		/// Attempts to query the client certificate that WinInet used for the current request.
		/// Note: This is a best-effort operation that may fail with ERROR_INVALID_PARAMETER
		/// on some WinInet versions. Failure is treated as non-fatal.
		/// </summary>
		/// <param name="request">Request handle after successful HttpSendRequestW</param>
		/// <returns>
		/// Certificate context if successfully retrieved, otherwise nullptr.
		/// </returns>
		static UniqueCertContext QueryClientCertFromRequest(HINTERNET request);

		/// <summary>
		/// Queries the HTTP status code from a completed request.
		/// </summary>
		/// <param name="request">Request handle after HttpSendRequestW</param>
		/// <returns>HTTP status code (e.g., 200, 404, 500)</returns>
		/// <exception cref="WinInetException">Thrown if HttpQueryInfoW fails</exception>
		static DWORD QueryStatusCode(HINTERNET request);

		/// <summary>
		/// Reads the entire HTTP response body into a string.
		/// </summary>
		/// <param name="request">Request handle after HttpSendRequestW</param>
		/// <returns>Complete response body as a string</returns>
		/// <exception cref="WinInetException">Thrown if InternetReadFile fails</exception>
		static std::string ReadAll(HINTERNET request);
	};

} // namespace webview::net
