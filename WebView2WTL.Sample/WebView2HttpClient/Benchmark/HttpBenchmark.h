#pragma once
#include "../IHttpBackend.h"
#include "../Telemetry/PerformanceProbe.h"
#include <vector>
#include <string>
#include <memory>

namespace WebView2Http
{
	struct BenchmarkResult
	{
		std::wstring backendName;
		int          iterations      { 0 };
		double       avgLatencyUs    { 0.0 };
		double       minLatencyUs    { 0.0 };
		double       maxLatencyUs    { 0.0 };
		double       p95LatencyUs    { 0.0 };
		double       avgThroughputBps{ 0.0 };
		int          successCount    { 0 };
		int          failureCount    { 0 };
	};

	/// Runs comparative benchmarks across multiple backends.
	class HttpBenchmark
	{
	public:
		struct BackendEntry
		{
			std::wstring                   name;
			std::shared_ptr<IHttpBackend>  backend;
		};

		/// Add a backend to the benchmark suite.
		void AddBackend(std::wstring name, std::shared_ptr<IHttpBackend> backend);

		/// Run N iterations of the given request against all registered backends.
		std::vector<BenchmarkResult> Run(
			const HttpRequest& request,
			int                iterations = 20);

		/// Print a summary table to OutputDebugStringW.
		static void PrintSummary(const std::vector<BenchmarkResult>& results);

	private:
		std::vector<BackendEntry> m_backends;

		static BenchmarkResult RunOne(
			const BackendEntry& entry,
			const HttpRequest&  request,
			int                 iterations);
	};
}
