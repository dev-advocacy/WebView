#pragma once
#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <optional>

namespace WebView2Http
{
	enum class HttpMethod { GET, POST, PUT, PATCH, DELETE_, HEAD, OPTIONS };

	inline std::wstring MethodToString(HttpMethod m)
	{
		switch (m) {
		case HttpMethod::GET:     return L"GET";
		case HttpMethod::POST:    return L"POST";
		case HttpMethod::PUT:     return L"PUT";
		case HttpMethod::PATCH:   return L"PATCH";
		case HttpMethod::DELETE_: return L"DELETE";
		case HttpMethod::HEAD:    return L"HEAD";
		case HttpMethod::OPTIONS: return L"OPTIONS";
		default:                  return L"GET";
		}
	}

	struct HttpRequest
	{
		HttpMethod                              method      = HttpMethod::GET;
		std::wstring                            url;
		std::map<std::wstring, std::wstring>    headers;
		std::vector<uint8_t>                    body;
		std::chrono::milliseconds               timeout     { 30'000 };
		bool                                    verifySsl   = true;

		// Convenience: set body from string
		void SetBody(std::string_view text)
		{
			body.assign(text.begin(), text.end());
		}

		void SetHeader(std::wstring key, std::wstring value)
		{
			headers[std::move(key)] = std::move(value);
		}
	};
}
