#include "WinHttpBackend.h"
#include <stdexcept>
#include <sstream>
#include <future>

#pragma comment(lib, "winhttp.lib")

namespace WebView2Http
{
	WinHttpBackend::WinHttpBackend()
	{
		m_hSession = ::WinHttpOpen(
			L"WebView2HttpClient/1.0",
			WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS,
			0);

		if (!m_hSession)
			throw std::runtime_error("WinHttpOpen failed: " + std::to_string(::GetLastError()));
	}

	WinHttpBackend::~WinHttpBackend()
	{
		if (m_hSession) ::WinHttpCloseHandle(m_hSession);
	}

	// -----------------------------------------------------------------------
	// URL parsing
	// -----------------------------------------------------------------------
	std::wstring WinHttpBackend::ExtractHost(
		const std::wstring& url, bool& https,
		std::wstring& path, INTERNET_PORT& port)
	{
		URL_COMPONENTS uc{};
		uc.dwStructSize      = sizeof(uc);
		uc.dwSchemeLength    = (DWORD)-1;
		uc.dwHostNameLength  = (DWORD)-1;
		uc.dwUrlPathLength   = (DWORD)-1;
		uc.dwExtraInfoLength = (DWORD)-1;

		if (!::WinHttpCrackUrl(url.c_str(), 0, 0, &uc))
			throw std::runtime_error("Invalid URL");

		https = (uc.nScheme == INTERNET_SCHEME_HTTPS);
		port  = uc.nPort;

		std::wstring host(uc.lpszHostName, uc.dwHostNameLength);
		path = std::wstring(uc.lpszUrlPath, uc.dwUrlPathLength);
		if (uc.lpszExtraInfo && uc.dwExtraInfoLength)
			path += std::wstring(uc.lpszExtraInfo, uc.dwExtraInfoLength);
		if (path.empty()) path = L"/";

		return host;
	}

	std::wstring WinHttpBackend::BuildHeaders(const std::map<std::wstring, std::wstring>& headers)
	{
		std::wostringstream ss;
		for (auto& [k, v] : headers)
			ss << k << L": " << v << L"\r\n";
		return ss.str();
	}

	// -----------------------------------------------------------------------
	// Core send
	// -----------------------------------------------------------------------
	HttpResponse WinHttpBackend::DoSend(const HttpRequest& request)
	{
		bool       https   = false;
		std::wstring path;
		INTERNET_PORT port = 0;

		std::wstring host = ExtractHost(request.url, https, path, port);

		HINTERNET hConnect = ::WinHttpConnect(
			m_hSession, host.c_str(), port, 0);
		if (!hConnect)
			throw std::runtime_error("WinHttpConnect failed");

		struct HandleGuard {
			HINTERNET h;
			~HandleGuard() { if (h) ::WinHttpCloseHandle(h); }
		};
		HandleGuard gConnect{ hConnect };

		HINTERNET hRequest = ::WinHttpOpenRequest(
			hConnect,
			MethodToString(request.method).c_str(),
			path.c_str(),
			nullptr,
			WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			https ? WINHTTP_FLAG_SECURE : 0);
		if (!hRequest)
			throw std::runtime_error("WinHttpOpenRequest failed");

		HandleGuard gRequest{ hRequest };

		// Set timeout
		auto ms = static_cast<int>(request.timeout.count());
		::WinHttpSetTimeouts(hRequest, ms, ms, ms, ms);

		// Skip SSL verify if requested
		if (!request.verifySsl)
		{
			DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
						  SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
						  SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
			::WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
		}

		// Custom headers
		std::wstring extraHeaders = BuildHeaders(request.headers);

		const void* pBody     = request.body.empty() ? nullptr : request.body.data();
		DWORD        bodySize  = static_cast<DWORD>(request.body.size());

		BOOL ok = ::WinHttpSendRequest(
			hRequest,
			extraHeaders.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : extraHeaders.c_str(),
			extraHeaders.empty() ? 0 : static_cast<DWORD>(extraHeaders.size()),
			const_cast<void*>(pBody),
			bodySize,
			bodySize,
			0);

		if (!ok || !::WinHttpReceiveResponse(hRequest, nullptr))
			throw std::runtime_error("WinHttp send/receive failed: " + std::to_string(::GetLastError()));

		// Status code
		HttpResponse response;
		DWORD statusCode = 0;
		DWORD sz = sizeof(statusCode);
		::WinHttpQueryHeaders(hRequest,
			WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
			WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &sz, WINHTTP_NO_HEADER_INDEX);
		response.statusCode = static_cast<int>(statusCode);
		response.bytesSent  = bodySize;

		// Read body
		DWORD available = 0;
		while (::WinHttpQueryDataAvailable(hRequest, &available) && available > 0)
		{
			std::vector<uint8_t> buf(available);
			DWORD read = 0;
			::WinHttpReadData(hRequest, buf.data(), available, &read);
			response.body.insert(response.body.end(), buf.begin(), buf.begin() + read);
		}
		response.bytesReceived = response.body.size();

		return response;
	}

	HttpResponse WinHttpBackend::Send(const HttpRequest& request)
	{
		return DoSend(request);
	}

	std::future<HttpResponse> WinHttpBackend::SendAsync(const HttpRequest& request)
	{
		return std::async(std::launch::async, [this, request]()
		{
			return DoSend(request);
		});
	}
}
