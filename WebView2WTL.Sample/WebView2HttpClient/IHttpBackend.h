#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <string>
#include <future>

namespace WebView2Http
{
	enum class BackendType { WinHTTP, WinINet, CppRest };

	/// Pure interface — each backend implements this contract.
	class IHttpBackend
	{
	public:
		virtual ~IHttpBackend() = default;

		/// Synchronous send.
		virtual HttpResponse Send(const HttpRequest& request) = 0;

		/// Async send — returns a future.
		virtual std::future<HttpResponse> SendAsync(const HttpRequest& request) = 0;

		/// Backend identifier (for telemetry / benchmarking).
		virtual BackendType Type() const noexcept = 0;
		virtual std::wstring Name() const noexcept = 0;
	};
}
