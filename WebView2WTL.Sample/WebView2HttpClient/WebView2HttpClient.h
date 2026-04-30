#pragma once

// DLL export/import macro
#ifdef HTTPCLIENT_EXPORTS
#  define HTTPCLIENT_API __declspec(dllexport)
#else
#  define HTTPCLIENT_API __declspec(dllimport)
#endif

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "IHttpBackend.h"
#include <memory>
#include <string>
#include <future>
#include <vector>
#include <functional>

// Forward declarations
namespace WebView2 { struct Cookie; }

namespace WebView2Http
{
	class CookieStore;
	class PolicyExecutor;
	class HttpTelemetry;

	/// Main façade — entry point for consumers of the DLL.
	class HTTPCLIENT_API HttpClient
	{
	public:
		/// Create a client with a specific backend (default: WinHTTP).
		explicit HttpClient(BackendType backend = BackendType::WinHTTP);
		~HttpClient();

		// Non-copyable, movable
		HttpClient(const HttpClient&)            = delete;
		HttpClient& operator=(const HttpClient&) = delete;
		HttpClient(HttpClient&&)                 noexcept;
		HttpClient& operator=(HttpClient&&)      noexcept;

		/// Switch the active backend at runtime.
		void SetBackend(BackendType backend);
		BackendType GetBackend() const noexcept;

		/// Sync request with resilience policies applied.
		HttpResponse Send(const HttpRequest& request);

		/// Async request — returns a future.
		std::future<HttpResponse> SendAsync(const HttpRequest& request);

		/// Convenience methods
		HttpResponse Get (const std::wstring& url, std::chrono::milliseconds timeout = std::chrono::milliseconds{30'000});
		HttpResponse Post(const std::wstring& url, std::string_view jsonBody, std::chrono::milliseconds timeout = std::chrono::milliseconds{30'000});

		/// Cookie synchronisation from WebView2
		void SyncCookies(const std::vector<WebView2::Cookie>& cookies);
		void ClearCookies();

	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}
