#include "WinINetBackend.h"
#include <windows.h>
#include <wininet.h>
#include <stdexcept>
#include <sstream>
#include <future>

#pragma comment(lib, "wininet.lib")

namespace WebView2Http
{
	static HINTERNET g_hInternet = nullptr;

	WinINetBackend::WinINetBackend()
	{
		if (!g_hInternet)
		{
			g_hInternet = ::InternetOpenW(
				L"WebView2HttpClient/1.0",
				INTERNET_OPEN_TYPE_PRECONFIG,
				nullptr, nullptr, 0);
			if (!g_hInternet)
				throw std::runtime_error("InternetOpen failed");
		}
	}

	WinINetBackend::~WinINetBackend() = default;

	HttpResponse WinINetBackend::DoSend(const HttpRequest& request)
	{
		// Parse URL
		wchar_t host[512]{}, path[2048]{};
		URL_COMPONENTSW uc{};
		uc.dwStructSize     = sizeof(uc);
		uc.lpszHostName     = host;
		uc.dwHostNameLength = _countof(host);
		uc.lpszUrlPath      = path;
		uc.dwUrlPathLength  = _countof(path);

		if (!::InternetCrackUrlW(request.url.c_str(), 0, 0, &uc))
			throw std::runtime_error("InternetCrackUrl failed");

		bool https = (uc.nScheme == INTERNET_SCHEME_HTTPS);

		HINTERNET hConn = ::InternetConnectW(
			g_hInternet, host, uc.nPort,
			nullptr, nullptr,
			INTERNET_SERVICE_HTTP, 0, 0);
		if (!hConn)
			throw std::runtime_error("InternetConnect failed");

		struct Guard { HINTERNET h; ~Guard(){ if(h) ::InternetCloseHandle(h); } };
		Guard gc{ hConn };

		DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
		if (https) flags |= INTERNET_FLAG_SECURE;

		std::string methodA(request.method == HttpMethod::GET  ? "GET"  :
							request.method == HttpMethod::POST ? "POST" :
							request.method == HttpMethod::PUT  ? "PUT"  : "DELETE");

		HINTERNET hReq = ::HttpOpenRequestA(
			hConn, methodA.c_str(), nullptr, nullptr, nullptr, nullptr, flags, 0);
		if (!hReq)
			throw std::runtime_error("HttpOpenRequest failed");
		Guard gr{ hReq };

		// Headers
		for (auto& [k, v] : request.headers)
		{
			std::wstring hdr = k + L": " + v + L"\r\n";
			::HttpAddRequestHeadersW(hReq, hdr.c_str(), (DWORD)hdr.size(),
				HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
		}

		const void* pBody = request.body.empty() ? nullptr : request.body.data();
		DWORD bodySize    = static_cast<DWORD>(request.body.size());

		if (!::HttpSendRequestW(hReq, nullptr, 0, const_cast<void*>(pBody), bodySize))
			throw std::runtime_error("HttpSendRequest failed");

		// Status code
		HttpResponse response;
		DWORD status = 0, sz = sizeof(status);
		::HttpQueryInfoW(hReq, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
			&status, &sz, nullptr);
		response.statusCode = static_cast<int>(status);
		response.bytesSent  = bodySize;

		// Read body
		DWORD available = 0;
		while (::InternetQueryDataAvailable(hReq, &available, 0, 0) && available > 0)
		{
			std::vector<uint8_t> buf(available);
			DWORD read = 0;
			::InternetReadFile(hReq, buf.data(), available, &read);
			response.body.insert(response.body.end(), buf.begin(), buf.begin() + read);
		}
		response.bytesReceived = response.body.size();

		return response;
	}

	HttpResponse WinINetBackend::Send(const HttpRequest& request)
	{
		return DoSend(request);
	}

	std::future<HttpResponse> WinINetBackend::SendAsync(const HttpRequest& request)
	{
		return std::async(std::launch::async, [this, request]()
		{
			return DoSend(request);
		});
	}
}
