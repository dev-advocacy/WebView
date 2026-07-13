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
#include <cryptuiapi.h>

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
	// Exceptions WinInet
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
	// Smart handles
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

	// -----------------------------------------------------------------------
	// Certificate selector from the CurrentUser\MY store
	// -----------------------------------------------------------------------
	class ClientCertificateSelector final
	{
	public:
		/// Selects a certificate whose subject contains `subjectFilter`.
		/// If `subjectFilter` is empty, returns the first certificate in the store.
		static UniqueCertContext SelectFromCurrentUserMyStore(const std::wstring& subjectFilter)
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

		/// Extracts a stable subject-like value from the certificate context.
		/// Prefer UPN (often GUID-like in enterprise certs), then CN, then display name.
		static std::wstring GetSubjectName(PCCERT_CONTEXT ctx)
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

		/// Extrait l'issuer (CN) du contexte de certificat.
		static std::wstring GetIssuerName(PCCERT_CONTEXT ctx)
		{
			DWORD len = CertGetNameStringW(ctx, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, nullptr, nullptr, 0);
			std::wstring name(len, L'\0');
			CertGetNameStringW(ctx, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, nullptr, name.data(), len);
			if (!name.empty() && name.back() == L'\0') name.pop_back();
			return name;
		}

		/// Extrait le friendly name du contexte de certificat.
		static std::wstring GetFriendlyName(PCCERT_CONTEXT ctx)
		{
			DWORD cb = 0;
			if (!CertGetCertificateContextProperty(ctx, CERT_FRIENDLY_NAME_PROP_ID, nullptr, &cb))
				return {};
			std::wstring name(cb / sizeof(wchar_t), L'\0');
			CertGetCertificateContextProperty(ctx, CERT_FRIENDLY_NAME_PROP_ID, name.data(), &cb);
			if (!name.empty() && name.back() == L'\0') name.pop_back();
			return name;
		}

		/// Gets the PEM encoding of the certificate (Base64-encoded DER)
		static std::wstring GetPemEncoding(PCCERT_CONTEXT ctx)
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

		/// Check if certificate has Client Authentication EKU (1.3.6.1.5.5.7.3.2)
		static bool HasClientAuthEKU(PCCERT_CONTEXT ctx)
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

		/// Get acceptable CA list from the TLS server via WinInet request handle
		/// Note: This API is not reliably available across all WinInet versions
		/// For now, we return empty list (= all CAs accepted)
		static std::vector<std::vector<BYTE>> GetAcceptableCAsFromRequest(HINTERNET request)
		{
			std::vector<std::vector<BYTE>> result;

			// TODO: Implement CA list retrieval when API is available
			// For now, return empty (= accept all CAs, filter only by EKU)

			LOG_TRACE("GetAcceptableCAsFromRequest: CA filtering not implemented, accepting all CAs");
			return result;
		}

		/// Check if certificate is issued by one of the acceptable CAs
		static bool IsIssuedByAcceptableCA(PCCERT_CONTEXT ctx,
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

		/// Select certificates with smart filtering (like WinInet).
		/// Returns vector of certificates that match:
		/// - Client Authentication EKU
		/// - Issued by acceptable CA (if request provided)
		/// - Subject filter (if provided)
		static std::vector<UniqueCertContext> SelectClientAuthCertificates(
			HINTERNET request = nullptr,
			const std::wstring& subjectFilter = L"")
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
				LOG_TRACE("SelectClientAuthCertificates: Failed to open MY store");
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

			LOG_TRACE(std::string("SelectClientAuthCertificates: ") + 
				std::to_string(filteredCerts) + " certificates matched filters out of " + 
				std::to_string(totalCerts) + " total");

			return result;
		}
	};

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
							 SelectedCertInfo*   outCertInfo = nullptr)
		{
			URL_COMPONENTS comp{};
			std::wstring   host(256, L'\0');
			std::wstring   path(2048, L'\0');

			comp.dwStructSize      = sizeof(comp);
			comp.lpszHostName      = host.data();
			comp.dwHostNameLength  = static_cast<DWORD>(host.size());
			comp.lpszUrlPath       = path.data();
			comp.dwUrlPathLength   = static_cast<DWORD>(path.size());
			comp.dwSchemeLength    = 1;

			if (!InternetCrackUrlW(url.c_str(), 0, 0, &comp))
				throw WinInetException("InternetCrackUrlW", GetLastError());

			host.resize(comp.dwHostNameLength);
			path.resize(comp.dwUrlPathLength);

			const auto internet    = OpenInternet();
			const auto connection  = OpenConnection(internet.get(), host, comp.nPort);
			const auto request     = OpenRequest(connection.get(), path);

			UniqueCertContext selectedCert = SendRequestWithCertAuth(
				request.get(), clientCertSubjectFilter, host, comp.nPort);

			// Populate outCertInfo if provided and a certificate was selected
			if (outCertInfo && selectedCert)
			{
				outCertInfo->host         = host;
				outCertInfo->port         = comp.nPort;
				outCertInfo->subject      = ClientCertificateSelector::GetSubjectName(selectedCert.get());
				outCertInfo->issuer       = ClientCertificateSelector::GetIssuerName(selectedCert.get());
				outCertInfo->friendlyName = ClientCertificateSelector::GetFriendlyName(selectedCert.get());
				outCertInfo->notBefore    = selectedCert->pCertInfo->NotBefore;
				outCertInfo->notAfter     = selectedCert->pCertInfo->NotAfter;
				outCertInfo->certContext  = std::move(selectedCert);
			}

			const DWORD status = QueryStatusCode(request.get());
			if (status >= 400)
				throw std::runtime_error("HTTP error status: " + std::to_string(status));

			return ReadAll(request.get());
		}

	private:
		static UniqueInternetHandle OpenInternet()
		{
			HINTERNET h = InternetOpenW(L"WebView2WTL/1.0",
				INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
			if (!h) throw WinInetException("InternetOpenW", GetLastError());
			return UniqueInternetHandle(h);
		}

		static UniqueInternetHandle OpenConnection(HINTERNET internet,
												   const std::wstring& host,
												   INTERNET_PORT port)
		{
			HINTERNET h = InternetConnectW(internet, host.c_str(), port,
				nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
			if (!h) throw WinInetException("InternetConnectW", GetLastError());
			return UniqueInternetHandle(h);
		}

		static UniqueInternetHandle OpenRequest(HINTERNET conn,
												const std::wstring& path)
		{
			const wchar_t* accept[] = { L"*/*", nullptr };
			HINTERNET h = HttpOpenRequestW(conn, L"GET", path.c_str(),
				nullptr, nullptr, accept,
				INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
				0);
			if (!h) throw WinInetException("HttpOpenRequestW", GetLastError());
			return UniqueInternetHandle(h);
		}

		/// Sends the request handling client certificate authentication.
		/// Returns the certificate context used.
		static UniqueCertContext SendRequestWithCertAuth(
			HINTERNET            request,
			const std::wstring&  clientCertSubjectFilter,
			const std::wstring&  host,
			INTERNET_PORT        port)
		{
			UniqueCertContext usedCert;

			for (int attempt = 0; attempt < 4; ++attempt)
			{
				if (HttpSendRequestW(request, nullptr, 0, nullptr, 0))
				{
					if (!usedCert)
						usedCert = QueryClientCertFromRequest(request); // best effort fallback only
					return usedCert;
				}

				const DWORD err = GetLastError();
				if (err != ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED)
					throw WinInetException("HttpSendRequestW", err);

				if (clientCertSubjectFilter.empty())
				{
					// Use smart filtering with request handle
					usedCert = SelectClientCertificateWithDialog(host, port, request);
					if (!usedCert)
						throw std::runtime_error("Client certificate selection cancelled by user.");
				}
				else
				{
					// Use smart filtering with subject filter
					auto filteredCerts = ClientCertificateSelector::SelectClientAuthCertificates(
						request, clientCertSubjectFilter);

					if (filteredCerts.empty())
					{
						throw std::runtime_error("No suitable client certificate found matching filter and server requirements.");
					}

					// Use the first matching certificate
					usedCert = std::move(filteredCerts[0]);

					LOG_TRACE(std::string("Auto-selected certificate with subject filter, ") + 
						std::to_string(filteredCerts.size()) + " candidates matched");
				}

				if (!InternetSetOptionW(request,
						INTERNET_OPTION_CLIENT_CERT_CONTEXT,
						const_cast<CERT_CONTEXT*>(usedCert.get()),
						sizeof(CERT_CONTEXT)))
				{
					throw WinInetException(
						"InternetSetOptionW(INTERNET_OPTION_CLIENT_CERT_CONTEXT)",
						GetLastError());
				}
			}

			throw std::runtime_error(
				"Server requires client certificate and request retry failed.");
		}

		static UniqueCertContext SelectClientCertificateWithDialog(
			const std::wstring& host, 
			INTERNET_PORT port,
			HINTERNET request = nullptr)
		{
			// Use smart filtering if request handle is provided
			if (request)
			{
				auto filteredCerts = ClientCertificateSelector::SelectClientAuthCertificates(request);

				if (filteredCerts.empty())
				{
					LOG_TRACE("SelectClientCertificateWithDialog: No certificates match server requirements");
					return UniqueCertContext();
				}

				// Create a memory store with only filtered certificates
				HCERTSTORE memStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
					CERT_STORE_CREATE_NEW_FLAG, nullptr);
				if (!memStore)
					throw WinInetException("CertOpenStore(MEMORY)", GetLastError());

				for (auto& certUnique : filteredCerts)
				{
					CertAddCertificateContextToStore(memStore, certUnique.get(),
						CERT_STORE_ADD_USE_EXISTING, nullptr);
				}

				const std::wstring title = L"Select client certificate for " + host + L":" + std::to_wstring(port);
				PCCERT_CONTEXT selected = CryptUIDlgSelectCertificateFromStore(
					memStore,
					GetDesktopWindow(),
					title.c_str(),
					L"Choose a client certificate (filtered by server requirements)",
					0,
					0,
					nullptr);

				CertCloseStore(memStore, 0);

				LOG_TRACE(std::string("SelectClientCertificateWithDialog: User ") + 
					(selected ? "selected" : "cancelled") + " certificate from filtered list");

				return UniqueCertContext(selected);
			}
			else
			{
				// Fallback: show all certificates from MY store (no filtering)
				HCERTSTORE store = CertOpenSystemStoreW(0, L"MY");
				if (!store)
					throw WinInetException("CertOpenSystemStoreW", GetLastError());

				const std::wstring title = L"Select client certificate for " + host + L":" + std::to_wstring(port);
				PCCERT_CONTEXT selected = CryptUIDlgSelectCertificateFromStore(
					store,
					GetDesktopWindow(),
					title.c_str(),
					L"Choose a client certificate",
					0,
					0,
					nullptr);

				CertCloseStore(store, 0);
				return UniqueCertContext(selected);
			}
		}

		/// After a successful HttpSendRequestW, best-effort reads the client certificate
		/// that WinInet used for the session (set by InternetErrorDlg or app).
		/// Returns nullptr if no cert was retrievable.
		static UniqueCertContext QueryClientCertFromRequest(HINTERNET request)
		{
			// IMPORTANT:
			// - INTERNET_OPTION_CLIENT_CERT_CONTEXT is primarily documented for InternetSetOption.
			// - On some WinInet/request combinations, InternetQueryOption may fail with
			//   ERROR_INVALID_PARAMETER.
			// - We treat this as non-fatal and log it for diagnostics.
			CERT_CONTEXT ctx{};
			DWORD ctxSize = sizeof(ctx);
			if (InternetQueryOptionW(request,
					INTERNET_OPTION_CLIENT_CERT_CONTEXT,
					&ctx,
					&ctxSize))
			{
				PCCERT_CONTEXT dup = CertDuplicateCertificateContext(&ctx);
				if (dup)
				{
					LOG_TRACE("QueryClientCertFromRequest: client cert context retrieved successfully");
					return UniqueCertContext(dup);
				}
				LOG_TRACE("QueryClientCertFromRequest: CertDuplicateCertificateContext failed");
				return {};
			}

			const DWORD err = GetLastError();
			LOG_TRACE(std::string("QueryClientCertFromRequest: InternetQueryOptionW(INTERNET_OPTION_CLIENT_CERT_CONTEXT) failed, err=")
				+ std::to_string(err)
				+ (err == ERROR_INVALID_PARAMETER ? " (ERROR_INVALID_PARAMETER: option not queryable for this handle/context)" : ""));
			return {};
		}

		static DWORD QueryStatusCode(HINTERNET request)
		{
			DWORD code = 0, sz = sizeof(code);
			if (!HttpQueryInfoW(request,
					HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
					&code, &sz, nullptr))
				throw WinInetException("HttpQueryInfoW", GetLastError());
			return code;
		}

		static std::string ReadAll(HINTERNET request)
		{
			std::string       body;
			std::vector<char> buf(8192);
			for (;;)
			{
				DWORD read = 0;
				if (!InternetReadFile(request, buf.data(),
						static_cast<DWORD>(buf.size()), &read))
					throw WinInetException("InternetReadFile", GetLastError());
				if (read == 0) break;
				body.append(buf.data(), read);
			}
			return body;
		}
	};

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
						const std::wstring& certSubjectFilter = L"")
		{
			SelectedCertInfo info;
			HttpsDownloader  dl;
			std::string      body = dl.Download(url, certSubjectFilter, &info);

			if (info.certContext)
			{
				std::lock_guard lock(m_mutex);

				auto it = FindByEndpointNoLock(info.host, info.port);
				if (it != m_certInfos.end())
				{
					*it = std::move(info);
					m_lastSelectedIndex = static_cast<size_t>(std::distance(m_certInfos.begin(), it));
				}
				else
				{
					m_certInfos.emplace_back(std::move(info));
					m_lastSelectedIndex = m_certInfos.size() - 1;
				}

				const auto& stored = m_certInfos[*m_lastSelectedIndex];
				LOG_TRACE(std::string("WinInet cert selected: host=") + WideToNarrow(stored.host) +
					" port=" + std::to_string(stored.port) +
					" subject=" + WideToNarrow(stored.subject) +
					" stored_count=" + std::to_string(m_certInfos.size()));
			}
			else
			{
				LOG_TRACE("WinInet: no certificate was selected or error occurred");
			}

			return body;
		}

		// -------------------------------------------------------------------
		// Returns true if a pre-selected certificate matches the given host:port
		// -------------------------------------------------------------------
		[[nodiscard]] bool HasMatchFor(const std::wstring& host, INTERNET_PORT port) const noexcept
		{
			if (!m_enabled.load(std::memory_order_acquire))
				return false;

			std::lock_guard lock(m_mutex);
			const auto it = FindByEndpointNoLock(host, port);
			const bool match = (it != m_certInfos.end());

			LOG_TRACE(std::string("HasMatchFor: host=") + WideToNarrow(host) +
				" port=" + std::to_string(port) +
				" match=" + (match ? "true" : "false") +
				" stored_count=" + std::to_string(m_certInfos.size()));
			return match;
		}

		// -------------------------------------------------------------------
		// Thread-safe access for endpoint-specific certificate metadata.
		// -------------------------------------------------------------------
		[[nodiscard]] std::wstring GetSubjectFor(const std::wstring& host, INTERNET_PORT port) const
		{
			std::lock_guard lock(m_mutex);
			const auto it = FindByEndpointNoLock(host, port);
			return (it != m_certInfos.end()) ? it->subject : L"";
		}

		[[nodiscard]] std::wstring GetIssuerFor(const std::wstring& host, INTERNET_PORT port) const
		{
			std::lock_guard lock(m_mutex);
			const auto it = FindByEndpointNoLock(host, port);
			return (it != m_certInfos.end()) ? it->issuer : L"";
		}

		[[nodiscard]] const CERT_CONTEXT* GetCertContextFor(const std::wstring& host, INTERNET_PORT port) const noexcept
		{
			std::lock_guard lock(m_mutex);
			const auto it = FindByEndpointNoLock(host, port);
			return (it != m_certInfos.end()) ? it->certContext.get() : nullptr;
		}

		[[nodiscard]] std::wstring GetPemEncodingFor(const std::wstring& host, INTERNET_PORT port) const
		{
			std::lock_guard lock(m_mutex);
			const auto it = FindByEndpointNoLock(host, port);
			if (it == m_certInfos.end() || !it->certContext)
				return {};
			return ClientCertificateSelector::GetPemEncoding(it->certContext.get());
		}

		// -------------------------------------------------------------------
		// Backward-compatible accessors: return the last selected certificate.
		// -------------------------------------------------------------------
		[[nodiscard]] const CERT_CONTEXT* GetCertContext() const noexcept
		{
			std::lock_guard lock(m_mutex);
			const SelectedCertInfo* info = GetLastSelectedNoLock();
			return info ? info->certContext.get() : nullptr;
		}

		[[nodiscard]] std::wstring GetSubject() const
		{
			std::lock_guard lock(m_mutex);
			const SelectedCertInfo* info = GetLastSelectedNoLock();
			return info ? info->subject : L"";
		}

		[[nodiscard]] std::wstring GetIssuer() const
		{
			std::lock_guard lock(m_mutex);
			const SelectedCertInfo* info = GetLastSelectedNoLock();
			return info ? info->issuer : L"";
		}

		[[nodiscard]] std::wstring GetHost() const
		{
			std::lock_guard lock(m_mutex);
			const SelectedCertInfo* info = GetLastSelectedNoLock();
			return info ? info->host : L"";
		}

		[[nodiscard]] INTERNET_PORT GetPort() const noexcept
		{
			std::lock_guard lock(m_mutex);
			const SelectedCertInfo* info = GetLastSelectedNoLock();
			return info ? info->port : 0;
		}

		[[nodiscard]] size_t Count() const noexcept
		{
			std::lock_guard lock(m_mutex);
			return m_certInfos.size();
		}

		// -------------------------------------------------------------------
		// Clear all stored certificates
		// -------------------------------------------------------------------
		void Clear() noexcept
		{
			std::lock_guard lock(m_mutex);
			m_certInfos.clear();
			m_lastSelectedIndex.reset();
		}

	private:
		using CertStore = std::vector<SelectedCertInfo>;
		using CertStoreIterator = CertStore::iterator;
		using CertStoreConstIterator = CertStore::const_iterator;

		WinInetCertPreSelector()  = default;
		~WinInetCertPreSelector() = default;

		[[nodiscard]] CertStoreIterator FindByEndpointNoLock(const std::wstring& host, INTERNET_PORT port)
		{
			return std::find_if(m_certInfos.begin(), m_certInfos.end(),
				[&host, port](const SelectedCertInfo& item)
				{
					return item.port == port && item.host == host;
				});
		}

		[[nodiscard]] CertStoreConstIterator FindByEndpointNoLock(const std::wstring& host, INTERNET_PORT port) const
		{
			return std::find_if(m_certInfos.begin(), m_certInfos.end(),
				[&host, port](const SelectedCertInfo& item)
				{
					return item.port == port && item.host == host;
				});
		}

		[[nodiscard]] const SelectedCertInfo* GetLastSelectedNoLock() const noexcept
		{
			if (!m_lastSelectedIndex.has_value())
				return nullptr;
			if (*m_lastSelectedIndex >= m_certInfos.size())
				return nullptr;
			return &m_certInfos[*m_lastSelectedIndex];
		}

		mutable std::mutex         m_mutex;
		CertStore                  m_certInfos;
		std::optional<size_t>      m_lastSelectedIndex;
		std::atomic<bool>          m_enabled{ true };
	};

} // namespace webview::net

