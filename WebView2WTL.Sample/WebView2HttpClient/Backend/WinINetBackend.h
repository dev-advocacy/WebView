#pragma once
#include "../IHttpBackend.h"

namespace WebView2Http
{
	/// WinINet backend — alternative to WinHTTP.
	/// Shares the IE/Edge cookie jar automatically (useful for legacy SSO scenarios).
	class WinINetBackend final : public IHttpBackend
	{
	public:
		WinINetBackend();
		~WinINetBackend() override;

		HttpResponse Send(const HttpRequest& request) override;
		std::future<HttpResponse> SendAsync(const HttpRequest& request) override;

		BackendType  Type() const noexcept override { return BackendType::WinINet; }
		std::wstring Name() const noexcept override { return L"WinINet"; }

	private:
		HttpResponse DoSend(const HttpRequest& request);
	};
}
