#pragma once

// ============================================================================
// ClientCertificateSelector.h
// ============================================================================
// Helper class for selecting client certificates from Windows certificate stores.
// Provides filtering by EKU, subject, and issuer.
// ============================================================================

#include "WinInetHelpers.h"
#include <vector>

namespace webview::net
{
	// -----------------------------------------------------------------------
	// Certificate selector from the CurrentUser\MY store
	// -----------------------------------------------------------------------

	/// <summary>
	/// Utility class for selecting and filtering client certificates from Windows certificate stores.
	/// Provides smart filtering that mimics WinInet's certificate selection behavior,
	/// including EKU validation, issuer checking, and subject filtering.
	/// </summary>
	class ClientCertificateSelector final
	{
	public:
		/// <summary>
		/// Selects a certificate from the Current User's Personal (MY) certificate store
		/// whose subject field contains the specified filter string.
		/// </summary>
		/// <param name="subjectFilter">
		/// Substring to search for in the certificate subject.
		/// If empty, returns the first certificate found in the store.
		/// </param>
		/// <returns>
		/// Unique pointer to the selected certificate context, or nullptr if no match found.
		/// </returns>
		static UniqueCertContext SelectFromCurrentUserMyStore(const std::wstring& subjectFilter);

		/// <summary>
		/// Extracts a stable subject identifier from the certificate.
		/// Attempts to use (in order of preference):
		/// 1. UPN (User Principal Name) - often a GUID in enterprise certificates
		/// 2. CN (Common Name)
		/// 3. Display name
		/// </summary>
		/// <param name="ctx">Certificate context to extract subject from</param>
		/// <returns>Subject string, or empty string if extraction fails</returns>
		static std::wstring GetSubjectName(PCCERT_CONTEXT ctx);

		/// <summary>
		/// Extracts the Common Name (CN) of the certificate issuer.
		/// </summary>
		/// <param name="ctx">Certificate context to extract issuer from</param>
		/// <returns>Issuer CN string, or empty string if extraction fails</returns>
		static std::wstring GetIssuerName(PCCERT_CONTEXT ctx);

		/// <summary>
		/// Retrieves the user-friendly display name of the certificate.
		/// This is the name shown in Windows certificate UI dialogs.
		/// </summary>
		/// <param name="ctx">Certificate context to extract friendly name from</param>
		/// <returns>Friendly name string, or empty string if not available</returns>
		static std::wstring GetFriendlyName(PCCERT_CONTEXT ctx);

		/// <summary>
		/// Converts the certificate to PEM (Base64-encoded DER) format.
		/// Used for certificate comparison in WebView2 ClientCertificateRequested events.
		/// </summary>
		/// <param name="ctx">Certificate context to encode</param>
		/// <returns>
		/// PEM-encoded certificate string (with -----BEGIN CERTIFICATE----- headers),
		/// or empty string if encoding fails.
		/// </returns>
		static std::wstring GetPemEncoding(PCCERT_CONTEXT ctx);

		/// <summary>
		/// Checks if the certificate has the Client Authentication Extended Key Usage (EKU).
		/// OID: 1.3.6.1.5.5.7.3.2
		/// </summary>
		/// <param name="ctx">Certificate context to check</param>
		/// <returns>
		/// true if the certificate can be used for client authentication, false otherwise.
		/// </returns>
		static bool HasClientAuthEKU(PCCERT_CONTEXT ctx);

		/// <summary>
		/// Attempts to retrieve the list of acceptable Certificate Authorities (CAs)
		/// from the TLS server via the WinInet request handle.
		/// Note: This API is not reliably available across all WinInet versions.
		/// </summary>
		/// <param name="request">WinInet request handle (HINTERNET)</param>
		/// <returns>
		/// Vector of CA certificates in binary (DER) format.
		/// Currently returns empty vector (meaning all CAs are accepted).
		/// </returns>
		static std::vector<std::vector<BYTE>> GetAcceptableCAsFromRequest(HINTERNET request);

		/// <summary>
		/// Checks if the certificate is issued by one of the acceptable Certificate Authorities.
		/// </summary>
		/// <param name="ctx">Certificate context to check</param>
		/// <param name="acceptableCAs">List of acceptable CA certificates in binary format</param>
		/// <returns>
		/// true if the certificate's issuer matches one of the acceptable CAs, 
		/// or if the acceptable CA list is empty (meaning all CAs are accepted).
		/// </returns>
		static bool IsIssuedByAcceptableCA(PCCERT_CONTEXT ctx,
			const std::vector<std::vector<BYTE>>& acceptableCAs);

		/// <summary>
		/// Performs smart certificate filtering similar to WinInet's native behavior.
		/// Returns certificates that match ALL of the following criteria:
		/// - Has Client Authentication EKU (1.3.6.1.5.5.7.3.2)
		/// - Is issued by an acceptable CA (if request handle provided)
		/// - Subject contains the filter string (if provided)
		/// </summary>
		/// <param name="request">
		/// Optional WinInet request handle for retrieving acceptable CA list.
		/// If nullptr, CA filtering is skipped.
		/// </param>
		/// <param name="subjectFilter">
		/// Optional substring to search for in certificate subjects.
		/// If empty, subject filtering is skipped.
		/// </param>
		/// <returns>
		/// Vector of matching certificate contexts. May be empty if no certificates match.
		/// </returns>
		static std::vector<UniqueCertContext> SelectClientAuthCertificates(
			HINTERNET request = nullptr,
			const std::wstring& subjectFilter = L"");
	};

} // namespace webview::net
