#pragma once
#include "../IHttpBackend.h"

namespace WebView2Http
{
	/// HTTP backend backed by the stripped cpprestsdk fork.
	/// Uses cpprest::http_client which wraps WinHTTP internally on Windows.
	/// Useful for OAuth flows, JSON streaming, and advanced request pipelines.
	class CppRestBackend final : public IHttpBackend
	{
	public:
		CppRestBackend();
		~CppRestBackend() override;

		HttpResponse Send(const HttpRequest& request) override;
		std::future<HttpResponse> SendAsync(const HttpRequest& request) override;

		BackendType  Type() const noexcept override { return BackendType::CppRest; }
		std::wstring Name() const noexcept override { return L"CppRest"; }

	private:
		HttpResponse DoSend(const HttpRequest& request);
	};
}
