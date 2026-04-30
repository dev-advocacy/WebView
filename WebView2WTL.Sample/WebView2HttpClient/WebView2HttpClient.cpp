#include "WebView2HttpClient.h"
#include "Backend/HttpBackendFactory.h"
#include "Cookies/CookieStore.h"
#include "Cookies/CookieBridge.h"
#include "Resilience/PolicyExecutor.h"
#include "Telemetry/HttpTelemetry.h"
#include "../../WebView2/Security/Cookie.h"

#include <stdexcept>

namespace WebView2Http
{
	// -----------------------------------------------------------------------
	// Pimpl
	// -----------------------------------------------------------------------
	struct HttpClient::Impl
	{
		std::unique_ptr<IHttpBackend>  backend;
		std::unique_ptr<CookieStore>   cookieStore;
		std::unique_ptr<PolicyExecutor> policyExecutor;
		std::unique_ptr<HttpTelemetry> telemetry;

		explicit Impl(BackendType type)
			: backend       (HttpBackendFactory::Create(type))
			, cookieStore   (std::make_unique<CookieStore>())
			, policyExecutor(std::make_unique<PolicyExecutor>())
			, telemetry     (std::make_unique<HttpTelemetry>())
		{}
	};

	// -----------------------------------------------------------------------
	// HttpClient
	// -----------------------------------------------------------------------
	HttpClient::HttpClient(BackendType backend)
		: m_impl(std::make_unique<Impl>(backend))
	{}

	HttpClient::~HttpClient() = default;

	HttpClient::HttpClient(HttpClient&&) noexcept = default;
	HttpClient& HttpClient::operator=(HttpClient&&) noexcept = default;

	void HttpClient::SetBackend(BackendType backend)
	{
		m_impl->backend = HttpBackendFactory::Create(backend);
		// Re-apply current cookies to the new backend
		CookieBridge::Apply(*m_impl->cookieStore, *m_impl->backend);
	}

	BackendType HttpClient::GetBackend() const noexcept
	{
		return m_impl->backend->Type();
	}

	HttpResponse HttpClient::Send(const HttpRequest& request)
	{
		// Apply cookies to request headers
		HttpRequest enriched = CookieBridge::Enrich(request, *m_impl->cookieStore);

		// Execute through resilience pipeline + telemetry
		return m_impl->policyExecutor->Execute(enriched, [&](const HttpRequest& req)
		{
			return m_impl->telemetry->Track(req, [&](const HttpRequest& r)
			{
				return m_impl->backend->Send(r);
			});
		});
	}

	std::future<HttpResponse> HttpClient::SendAsync(const HttpRequest& request)
	{
		return std::async(std::launch::async, [this, request]()
		{
			return Send(request);
		});
	}

	HttpResponse HttpClient::Get(const std::wstring& url, std::chrono::milliseconds timeout)
	{
		HttpRequest req;
		req.method  = HttpMethod::GET;
		req.url     = url;
		req.timeout = timeout;
		return Send(req);
	}

	HttpResponse HttpClient::Post(const std::wstring& url, std::string_view jsonBody, std::chrono::milliseconds timeout)
	{
		HttpRequest req;
		req.method  = HttpMethod::POST;
		req.url     = url;
		req.timeout = timeout;
		req.SetBody(jsonBody);
		req.SetHeader(L"Content-Type", L"application/json");
		return Send(req);
	}

	void HttpClient::SyncCookies(const std::vector<WebView2::Cookie>& cookies)
	{
		m_impl->cookieStore->SyncFromWebView2(cookies);
		CookieBridge::Apply(*m_impl->cookieStore, *m_impl->backend);
	}

	void HttpClient::ClearCookies()
	{
		m_impl->cookieStore->Clear();
	}
}
