#include "PerformanceProbe.h"
#include <stdexcept>
#include <numeric>

namespace WebView2Http
{
	ProbeResult PerformanceProbe::Measure(
		const HttpRequest&  request,
		ProbeSendFn         sendFn,
		const std::wstring& backendName)
	{
		ProbeResult result;
		result.backendName = backendName;
		result.url         = request.url;

		auto t0 = std::chrono::high_resolution_clock::now();
		try
		{
			auto response = sendFn(request);
			auto t1       = std::chrono::high_resolution_clock::now();

			result.latency    = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0);
			result.statusCode = response.statusCode;
			result.success    = response.IsSuccess();

			if (result.latency.count() > 0)
			{
				double secs = result.latency.count() / 1'000'000.0;
				result.throughputBps = static_cast<double>(response.bytesReceived) / secs;
			}
		}
		catch (...)
		{
			auto t1       = std::chrono::high_resolution_clock::now();
			result.latency = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0);
			result.success = false;
		}

		return result;
	}

	double PerformanceProbe::AverageLatencyUs(
		const HttpRequest& request,
		ProbeSendFn        sendFn,
		int                iterations)
	{
		double total = 0.0;
		int    ok    = 0;

		for (int i = 0; i < iterations; ++i)
		{
			auto r = Measure(request, sendFn, L"probe");
			if (r.success)
			{
				total += static_cast<double>(r.latency.count());
				++ok;
			}
		}
		return ok > 0 ? total / ok : 0.0;
	}
}
