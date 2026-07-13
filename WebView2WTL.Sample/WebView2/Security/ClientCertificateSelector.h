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
	class ClientCertificateSelector final
	{
	public:
		/// Selects a certificate whose subject contains `subjectFilter`.
		/// If `subjectFilter` is empty, returns the first certificate in the store.
		static UniqueCertContext SelectFromCurrentUserMyStore(const std::wstring& subjectFilter);

		/// Extracts a stable subject-like value from the certificate context.
		/// Prefer UPN (often GUID-like in enterprise certs), then CN, then display name.
		static std::wstring GetSubjectName(PCCERT_CONTEXT ctx);

		/// Extracts the issuer (CN) from the certificate context.
		static std::wstring GetIssuerName(PCCERT_CONTEXT ctx);

		/// Extracts the friendly name from the certificate context.
		static std::wstring GetFriendlyName(PCCERT_CONTEXT ctx);

		/// Gets the PEM encoding of the certificate (Base64-encoded DER)
		static std::wstring GetPemEncoding(PCCERT_CONTEXT ctx);

		/// Check if certificate has Client Authentication EKU (1.3.6.1.5.5.7.3.2)
		static bool HasClientAuthEKU(PCCERT_CONTEXT ctx);

		/// Get acceptable CA list from the TLS server via WinInet request handle
		/// Note: This API is not reliably available across all WinInet versions
		/// For now, we return empty list (= all CAs accepted)
		static std::vector<std::vector<BYTE>> GetAcceptableCAsFromRequest(HINTERNET request);

		/// Check if certificate is issued by one of the acceptable CAs
		static bool IsIssuedByAcceptableCA(PCCERT_CONTEXT ctx,
			const std::vector<std::vector<BYTE>>& acceptableCAs);

		/// Select certificates with smart filtering (like WinInet).
		/// Returns vector of certificates that match:
		/// - Client Authentication EKU
		/// - Issued by acceptable CA (if request provided)
		/// - Subject filter (if provided)
		static std::vector<UniqueCertContext> SelectClientAuthCertificates(
			HINTERNET request = nullptr,
			const std::wstring& subjectFilter = L"");
	};

} // namespace webview::net
