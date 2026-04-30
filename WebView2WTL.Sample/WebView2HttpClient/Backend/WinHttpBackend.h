#pragma once
#include "../IHttpBackend.h"
#include <windows.h>
#include <winhttp.h>
#include <memory>
#include <string>

namespace WebView2Http
{
	class WinHttpBackend final : public IHttpBackend
	{
	public:
		WinHttpBackend();
		~WinHttpBackend() override;

		HttpResponse Send(const HttpRequest& request) override;
		std::future<HttpResponse> SendAsync(const HttpRequest& request) override;

		BackendType  Type() const noexcept override { return BackendType::WinHTTP; }
		std::wstring Name() const noexcept override { return L"WinHTTP"; }

	private:
		// WinHTTP session handle (one per backend instance)
		HINTERNET m_hSession = nullptr;

		HttpResponse DoSend(const HttpRequest& request);

		// Helpers
		static std::wstring BuildHeaders(const std::map<std::wstring, std::wstring>& headers);
		static std::wstring ExtractHost(const std::wstring& url, bool& https, std::wstring& path, INTERNET_PORT& port);
	};
}
