#pragma once

// ============================================================================
// WinInetCertPreSelector.h
// ============================================================================
// Exécute une requête HTTPS via WinInet AVANT la création de la WebView afin
// de laisser l'utilisateur choisir son certificat client via InternetErrorDlg.
// Les paramètres du certificat sélectionné sont conservés dans un lazy
// singleton (magic static C++20) et réutilisés automatiquement côté WebView
// quand le même host:port est rencontré ET que la fonctionnalité est activée.
//
// Usage :
//   // Avant la création de la WebView
//   auto& sel = webview::net::WinInetCertPreSelector::Instance();
//   sel.Run(L"https://myserver.example.com/", L""); // "" = dialog natif
//
//   // Côté WebView (ClientCertificateRequested)
//   if (sel.HasMatchFor(host, port)) { ... inject ... }
// ============================================================================

#include <windows.h>
#include <wininet.h>
#include <wincrypt.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "crypt32.lib")

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
	// Informations du certificat sélectionné (stocké dans le singleton)
	// -----------------------------------------------------------------------
	struct SelectedCertInfo
	{
		std::wstring host;          // hostname sans port
		INTERNET_PORT port{};       // port TCP
		std::wstring subject;       // CN=...
		std::wstring issuer;        // émetteur
		std::wstring friendlyName;  // nom convivial (peut être vide)
		FILETIME     notBefore{};
		FILETIME     notAfter{};

		// Contexte dupliqué du certificat — valide jusqu'à destruction du singleton
		UniqueCertContext certContext;

		SelectedCertInfo() = default;
		SelectedCertInfo(const SelectedCertInfo&) = delete;
		SelectedCertInfo& operator=(const SelectedCertInfo&) = delete;
		SelectedCertInfo(SelectedCertInfo&&) = default;
		SelectedCertInfo& operator=(SelectedCertInfo&&) = default;
	};

	// -----------------------------------------------------------------------
	// Sélecteur de certificat dans CurrentUser\MY
	// -----------------------------------------------------------------------
	class ClientCertificateSelector final
	{
	public:
		/// Sélectionne un certificat dont le subject contient `subjectFilter`.
		/// Si `subjectFilter` est vide, renvoie le premier certificat du store.
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

		/// Extrait le subject (CN) du contexte de certificat.
		static std::wstring GetSubjectName(PCCERT_CONTEXT ctx)
		{
			DWORD len = CertGetNameStringW(ctx, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, nullptr, 0);
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
	};

	// -----------------------------------------------------------------------
	// Téléchargeur HTTPS WinInet avec authentification certificat client
	// -----------------------------------------------------------------------
	class HttpsDownloader final
	{
	public:
		/// Exécute la requête et retourne le corps HTTP.
		/// `clientCertSubjectFilter` vide → InternetErrorDlg natif.
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

			// Renseigner outCertInfo si fourni et si un cert a été sélectionné
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

		/// Envoie la requête en gérant l'authentification certificat.
		/// Retourne le contexte du certificat utilisé (peut être nullptr si dialog natif).
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
					return usedCert; // succès sans cert ou cert déjà défini

				const DWORD err = GetLastError();
				if (err != ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED)
					throw WinInetException("HttpSendRequestW", err);

				if (clientCertSubjectFilter.empty())
				{
					// Dialog natif WinInet — on ne peut pas récupérer le cert choisi ici
					LPVOID data = nullptr;
					const DWORD dlgResult = InternetErrorDlg(
						GetDesktopWindow(),
						request,
						ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED,
						FLAGS_ERROR_UI_FLAGS_GENERATE_DATA | FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS,
						&data);

					if (dlgResult == ERROR_CANCELLED)
						throw std::runtime_error("Client certificate selection cancelled by user.");
					if (dlgResult == ERROR_SUCCESS || dlgResult == ERROR_INTERNET_FORCE_RETRY)
						continue;
					throw WinInetException("InternetErrorDlg", dlgResult);
				}
				else
				{
					// Sélection programmatique
					usedCert = ClientCertificateSelector::SelectFromCurrentUserMyStore(
						clientCertSubjectFilter);

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
			}

			throw std::runtime_error(
				"Server requires client certificate and request retry failed.");
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
	// Stocke le certificat sélectionné lors de la requête WinInet pré-WebView.
	// -----------------------------------------------------------------------
	class WinInetCertPreSelector final
	{
	public:
		// Singleton — magic static, thread-safe depuis C++11/C++20
		static WinInetCertPreSelector& Instance() noexcept
		{
			static WinInetCertPreSelector s_instance;
			return s_instance;
		}

		WinInetCertPreSelector(const WinInetCertPreSelector&) = delete;
		WinInetCertPreSelector& operator=(const WinInetCertPreSelector&) = delete;

		// -------------------------------------------------------------------
		// Activer / désactiver la fonctionnalité (depuis le menu)
		// -------------------------------------------------------------------
		void SetEnabled(bool enabled) noexcept { m_enabled.store(enabled, std::memory_order_release); }
		[[nodiscard]] bool IsEnabled()  const noexcept { return m_enabled.load(std::memory_order_acquire); }

		// -------------------------------------------------------------------
		// Exécuter la requête WinInet pré-WebView
		// url                  : URL cible (https://host:port/path)
		// certSubjectFilter    : filtre CN (vide = dialog natif)
		// Retourne le corps HTTP de la réponse (peut être ignoré).
		// Lance une exception en cas d'échec.
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
				m_certInfo = std::move(info);
			}

			return body;
		}

		// -------------------------------------------------------------------
		// Vérifie si un certificat pré-sélectionné correspond à ce host:port
		// -------------------------------------------------------------------
		[[nodiscard]] bool HasMatchFor(const std::wstring& host, INTERNET_PORT port) const noexcept
		{
			if (!m_enabled.load(std::memory_order_acquire))
				return false;
			std::lock_guard lock(m_mutex);
			if (!m_certInfo.has_value()) return false;
			return m_certInfo->host == host && m_certInfo->port == port;
		}

		// -------------------------------------------------------------------
		// Accès thread-safe aux informations du certificat
		// Retourne nullptr si aucun cert disponible.
		// -------------------------------------------------------------------
		[[nodiscard]] const CERT_CONTEXT* GetCertContext() const noexcept
		{
			std::lock_guard lock(m_mutex);
			if (!m_certInfo.has_value()) return nullptr;
			return m_certInfo->certContext.get();
		}

		[[nodiscard]] std::wstring GetSubject() const
		{
			std::lock_guard lock(m_mutex);
			return m_certInfo.has_value() ? m_certInfo->subject : L"";
		}

		[[nodiscard]] std::wstring GetIssuer() const
		{
			std::lock_guard lock(m_mutex);
			return m_certInfo.has_value() ? m_certInfo->issuer : L"";
		}

		[[nodiscard]] std::wstring GetHost() const
		{
			std::lock_guard lock(m_mutex);
			return m_certInfo.has_value() ? m_certInfo->host : L"";
		}

		[[nodiscard]] INTERNET_PORT GetPort() const noexcept
		{
			std::lock_guard lock(m_mutex);
			return m_certInfo.has_value() ? m_certInfo->port : 0;
		}

		// -------------------------------------------------------------------
		// Réinitialiser le certificat mémorisé
		// -------------------------------------------------------------------
		void Clear() noexcept
		{
			std::lock_guard lock(m_mutex);
			m_certInfo.reset();
		}

	private:
		WinInetCertPreSelector()  = default;
		~WinInetCertPreSelector() = default;

		mutable std::mutex              m_mutex;
		std::optional<SelectedCertInfo> m_certInfo;
		std::atomic<bool>               m_enabled{ true };
	};

} // namespace webview::net
