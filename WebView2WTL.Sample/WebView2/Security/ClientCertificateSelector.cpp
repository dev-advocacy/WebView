#include "pch.h"
#include "ClientCertificateSelector.h"
#include "../Logger/logger.h"

#include <cryptuiapi.h>
#pragma comment(lib, "cryptui.lib")

namespace webview::net
{
	UniqueCertContext ClientCertificateSelector::SelectFromCurrentUserMyStore(const std::wstring& subjectFilter)
	{
		HCERTSTORE store = CertOpenSystemStoreW(0, L"MY");
		if (!store)
			throw WinInetException("CertOpenSystemStoreW", GetLastError());

		PCCERT_CONTEXT found = subjectFilter.empty()
			? CertEnumCertificatesInStore(store, nullptr)
			: CertFindCertificateInStore(
				store,
				X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
				0,
				CERT_FIND_SUBJECT_STR_W,
				subjectFilter.c_str(),
				nullptr);

		if (!found)
		{
			CertCloseStore(store, 0);
			throw std::runtime_error("No suitable client certificate found in CurrentUser\\MY store.");
		}

		PCCERT_CONTEXT dup = CertDuplicateCertificateContext(found);
		CertFreeCertificateContext(found);
		CertCloseStore(store, 0);

		if (!dup)
			throw WinInetException("CertDuplicateCertificateContext", GetLastError());

		return UniqueCertContext(dup);
	}

	std::wstring ClientCertificateSelector::GetSubjectName(PCCERT_CONTEXT ctx)
	{
		// 1) UPN (Subject Alternative Name / user principal name)
		DWORD len = CertGetNameStringW(ctx, CERT_NAME_UPN_TYPE, 0, nullptr, nullptr, 0);
		if (len > 1)
		{
			std::wstring upn(len, L'\0');
			CertGetNameStringW(ctx, CERT_NAME_UPN_TYPE, 0, nullptr, upn.data(), len);
			if (!upn.empty() && upn.back() == L'\0') upn.pop_back();
			return upn;
		}

		// 2) Common Name (CN)
		len = CertGetNameStringW(ctx, CERT_NAME_ATTR_TYPE, 0,
			reinterpret_cast<void*>(const_cast<char*>(szOID_COMMON_NAME)), nullptr, 0);
		if (len > 1)
		{
			std::wstring cn(len, L'\0');
			CertGetNameStringW(ctx, CERT_NAME_ATTR_TYPE, 0,
				reinterpret_cast<void*>(const_cast<char*>(szOID_COMMON_NAME)), cn.data(), len);
			if (!cn.empty() && cn.back() == L'\0') cn.pop_back();
			return cn;
		}

		// 3) Fallback: display name
		len = CertGetNameStringW(ctx, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, nullptr, 0);
		std::wstring name(len, L'\0');
		CertGetNameStringW(ctx, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, name.data(), len);
		if (!name.empty() && name.back() == L'\0') name.pop_back();
		return name;
	}

	std::wstring ClientCertificateSelector::GetIssuerName(PCCERT_CONTEXT ctx)
	{
		DWORD len = CertGetNameStringW(ctx, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, nullptr, nullptr, 0);
		std::wstring name(len, L'\0');
		CertGetNameStringW(ctx, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, nullptr, name.data(), len);
		if (!name.empty() && name.back() == L'\0') name.pop_back();
		return name;
	}

	std::wstring ClientCertificateSelector::GetFriendlyName(PCCERT_CONTEXT ctx)
	{
		DWORD cb = 0;
		if (!CertGetCertificateContextProperty(ctx, CERT_FRIENDLY_NAME_PROP_ID, nullptr, &cb))
			return {};
		std::wstring name(cb / sizeof(wchar_t), L'\0');
		CertGetCertificateContextProperty(ctx, CERT_FRIENDLY_NAME_PROP_ID, name.data(), &cb);
		if (!name.empty() && name.back() == L'\0') name.pop_back();
		return name;
	}

	std::wstring ClientCertificateSelector::GetPemEncoding(PCCERT_CONTEXT ctx)
	{
		if (!ctx || !ctx->pbCertEncoded || ctx->cbCertEncoded == 0)
			return {};

		// Base64 encode the DER certificate
		DWORD pemLen = 0;
		if (!CryptBinaryToStringW(ctx->pbCertEncoded, ctx->cbCertEncoded,
			CRYPT_STRING_BASE64HEADER, nullptr, &pemLen))
			return {};

		std::wstring pem(pemLen, L'\0');
		if (!CryptBinaryToStringW(ctx->pbCertEncoded, ctx->cbCertEncoded,
			CRYPT_STRING_BASE64HEADER, pem.data(), &pemLen))
			return {};

		// Remove trailing null if present
		if (!pem.empty() && pem.back() == L'\0') pem.pop_back();
		return pem;
	}

	bool ClientCertificateSelector::HasClientAuthEKU(PCCERT_CONTEXT ctx)
	{
		if (!ctx)
			return false;

		DWORD usageSize = 0;
		if (!CertGetEnhancedKeyUsage(ctx, 0, nullptr, &usageSize))
			return false;

		std::vector<BYTE> usageBuffer(usageSize);
		CERT_ENHKEY_USAGE* usage = reinterpret_cast<CERT_ENHKEY_USAGE*>(usageBuffer.data());

		if (!CertGetEnhancedKeyUsage(ctx, 0, usage, &usageSize))
			return false;

		// Check for Client Authentication OID: 1.3.6.1.5.5.7.3.2
		for (DWORD i = 0; i < usage->cUsageIdentifier; i++)
		{
			if (strcmp(usage->rgpszUsageIdentifier[i], szOID_PKIX_KP_CLIENT_AUTH) == 0)
			{
				return true;
			}
		}

		return false;
	}

	std::vector<std::vector<BYTE>> ClientCertificateSelector::GetAcceptableCAsFromRequest(HINTERNET request)
	{
		std::vector<std::vector<BYTE>> result;

		// TODO: Implement CA list retrieval when API is available
		// For now, return empty (= accept all CAs, filter only by EKU)

		// CA filtering not implemented, accepting all CAs
		return result;
	}

	bool ClientCertificateSelector::IsIssuedByAcceptableCA(PCCERT_CONTEXT ctx,
		const std::vector<std::vector<BYTE>>& acceptableCAs)
	{
		if (!ctx)
			return false;

		// If no CA list provided, all CAs are acceptable
		if (acceptableCAs.empty())
			return true;

		CERT_NAME_BLOB issuerBlob = ctx->pCertInfo->Issuer;

		for (const auto& caData : acceptableCAs)
		{
			CERT_NAME_BLOB caBlob;
			caBlob.cbData = static_cast<DWORD>(caData.size());
			caBlob.pbData = const_cast<BYTE*>(caData.data());

			if (CertCompareCertificateName(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
				&issuerBlob, &caBlob))
			{
				return true;
			}
		}

		return false;
	}

	std::vector<UniqueCertContext> ClientCertificateSelector::SelectClientAuthCertificates(
		HINTERNET request,
		const std::wstring& subjectFilter)
	{
		std::vector<UniqueCertContext> result;

		// Get acceptable CA list from server (if request handle provided)
		std::vector<std::vector<BYTE>> acceptableCAs;
		if (request)
		{
			acceptableCAs = GetAcceptableCAsFromRequest(request);
		}

		HCERTSTORE store = CertOpenSystemStoreW(0, L"MY");
		if (!store)
		{
			return result;
		}

		int totalCerts = 0;
		int filteredCerts = 0;

		PCCERT_CONTEXT ctx = nullptr;
		while ((ctx = CertEnumCertificatesInStore(store, ctx)) != nullptr)
		{
			totalCerts++;

			// Filter 1: Subject filter (if provided)
			if (!subjectFilter.empty())
			{
				std::wstring subject = GetSubjectName(ctx);
				if (subject.find(subjectFilter) == std::wstring::npos)
				{
					continue;
				}
			}

			// Filter 2: Must have Client Authentication EKU
			if (!HasClientAuthEKU(ctx))
			{
				continue;
			}

			// Filter 3: Must be issued by acceptable CA (if list provided)
			if (!IsIssuedByAcceptableCA(ctx, acceptableCAs))
			{
				continue;
			}

			// Certificate passes all filters
			PCCERT_CONTEXT dup = CertDuplicateCertificateContext(ctx);
			if (dup)
			{
				result.emplace_back(UniqueCertContext(dup));
				filteredCerts++;
			}
		}

		CertCloseStore(store, 0);

		return result;
	}

} // namespace webview::net
