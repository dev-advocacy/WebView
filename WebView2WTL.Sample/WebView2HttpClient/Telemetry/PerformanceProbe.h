#pragma once
#include "../HttpRequest.h"
#include "../HttpResponse.h"
#include <string>
#include <chrono>
#include <functional>

namespace WebView2Http
{
	/// High-resolution probe for measuring raw backend performance.
	/// Records per-request latency, throughput (bytes/s), and TTFB.
	struct ProbeResult
	{
		std::wstring              backendName;
		std::wstring              url;
		std::chrono::microseconds latency      { 0 };
		double                    throughputBps { 0.0 }; // bytes/sec
		int                       statusCode    { 0 };
		bool                      success       { false };
	};

	class PerformanceProbe
	{
	public:
		using ProbeSendFn = std::function<HttpResponse(const HttpRequest&)>;

		/// Runs the request through the given send function and records timing.
		static ProbeResult Measure(
			const HttpRequest& request,
			ProbeSendFn        sendFn,
			const std::wstring& backendName);

		/// Runs N iterations and returns average latency in microseconds.
		static double AverageLatencyUs(
			const HttpRequest& request,
			ProbeSendFn        sendFn,
			int                iterations = 10);
	};
}
