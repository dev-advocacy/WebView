#include "HttpBenchmark.h"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <windows.h>

namespace WebView2Http
{
	void HttpBenchmark::AddBackend(std::wstring name, std::shared_ptr<IHttpBackend> backend)
	{
		m_backends.push_back({ std::move(name), std::move(backend) });
	}

	BenchmarkResult HttpBenchmark::RunOne(
		const BackendEntry& entry,
		const HttpRequest&  request,
		int                 iterations)
	{
		BenchmarkResult result;
		result.backendName = entry.name;
		result.iterations  = iterations;

		std::vector<double> latencies;
		latencies.reserve(iterations);
		double totalThroughput = 0.0;

		for (int i = 0; i < iterations; ++i)
		{
			auto probe = PerformanceProbe::Measure(
				request,
				[&](const HttpRequest& r) { return entry.backend->Send(r); },
				entry.name);

			if (probe.success)
			{
				++result.successCount;
				latencies.push_back(static_cast<double>(probe.latency.count()));
				totalThroughput += probe.throughputBps;
			}
			else
			{
				++result.failureCount;
			}
		}

		if (!latencies.empty())
		{
			std::sort(latencies.begin(), latencies.end());
			result.minLatencyUs = latencies.front();
			result.maxLatencyUs = latencies.back();
			result.avgLatencyUs = std::accumulate(latencies.begin(), latencies.end(), 0.0)
								  / static_cast<double>(latencies.size());

			// P95
			size_t p95idx    = static_cast<size_t>(latencies.size() * 0.95);
			result.p95LatencyUs = latencies[std::min(p95idx, latencies.size() - 1)];

			result.avgThroughputBps = totalThroughput / static_cast<double>(result.successCount);
		}

		return result;
	}

	std::vector<BenchmarkResult> HttpBenchmark::Run(
		const HttpRequest& request,
		int                iterations)
	{
		std::vector<BenchmarkResult> results;
		results.reserve(m_backends.size());

		for (auto& entry : m_backends)
			results.push_back(RunOne(entry, request, iterations));

		return results;
	}

	void HttpBenchmark::PrintSummary(const std::vector<BenchmarkResult>& results)
	{
		std::wostringstream ss;
		ss << L"\n=== HttpBenchmark Results ===\n";
		ss << std::wstring(60, L'-') << L"\n";
		ss << L"Backend         Avg(us)   Min(us)   P95(us)   Max(us)   OK  FAIL  Tput(KB/s)\n";
		ss << std::wstring(60, L'-') << L"\n";

		for (auto& r : results)
		{
			ss << r.backendName;
			// Pad to 16
			for (int i = static_cast<int>(r.backendName.size()); i < 16; ++i) ss << L' ';
			ss << static_cast<long long>(r.avgLatencyUs)   << L"       "
			   << static_cast<long long>(r.minLatencyUs)   << L"       "
			   << static_cast<long long>(r.p95LatencyUs)   << L"       "
			   << static_cast<long long>(r.maxLatencyUs)   << L"    "
			   << r.successCount << L"   "
			   << r.failureCount << L"     "
			   << static_cast<long long>(r.avgThroughputBps / 1024.0) << L"\n";
		}
		ss << std::wstring(60, L'-') << L"\n";

		::OutputDebugStringW(ss.str().c_str());
	}
}
