#pragma once
#include <string>
#include <map>
#include <vector>
#include <cstdint>

namespace WebView2Http
{
	struct HttpResponse
	{
		int                                     statusCode  = 0;
		std::wstring                            statusText;
		std::map<std::wstring, std::wstring>    headers;
		std::vector<uint8_t>                    body;

		// Telemetry fields
		std::chrono::milliseconds               latency     { 0 };
		size_t                                  bytesReceived = 0;
		size_t                                  bytesSent     = 0;

		bool IsSuccess() const noexcept { return statusCode >= 200 && statusCode < 300; }

		std::string BodyAsString() const
		{
			return { reinterpret_cast<const char*>(body.data()), body.size() };
		}
	};
}
