#include "HttpTelemetry.h"

// OpenTelemetry C++ SDK
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/tracer.h>
#include <opentelemetry/trace/scope.h>
#include <opentelemetry/metrics/provider.h>
#include <opentelemetry/common/attribute_value.h>

#include <chrono>
#include <string>

namespace WebView2Http
{
	namespace ot = opentelemetry;

	struct HttpTelemetry::Impl
	{
		std::string serviceName{ "WebView2HttpClient" };

		ot::nostd::shared_ptr<ot::trace::Tracer> GetTracer()
		{
			auto provider = ot::trace::Provider::GetTracerProvider();
			return provider->GetTracer(serviceName, "1.0.0");
		}
	};

	HttpTelemetry::HttpTelemetry()  : m_impl(std::make_unique<Impl>()) {}
	HttpTelemetry::~HttpTelemetry() = default;

	void HttpTelemetry::SetServiceName(std::wstring name)
	{
		m_impl->serviceName.assign(name.begin(), name.end());
	}

	HttpResponse HttpTelemetry::Track(const HttpRequest& request, SendFn innerSend)
	{
		auto tracer = m_impl->GetTracer();

		// Convert wstring URL to string for OTel attributes
		std::string urlA(request.url.begin(), request.url.end());
		std::string methodA(MethodToString(request.method).begin(),
							MethodToString(request.method).end());

		// Start span
		auto span = tracer->StartSpan("http.client.request",
		{
			{ "http.method", methodA },
			{ "http.url",    urlA    },
		});
		auto scope = tracer->WithActiveSpan(span);

		auto t0 = std::chrono::steady_clock::now();
		try
		{
			auto response = innerSend(request);

			auto latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - t0).count();

			response.latency = std::chrono::milliseconds{ latencyMs };

			span->SetAttribute("http.status_code",  response.statusCode);
			span->SetAttribute("http.latency_ms",   (long long)latencyMs);
			span->SetAttribute("http.bytes_in",     (long long)response.bytesReceived);
			span->SetAttribute("http.bytes_out",    (long long)response.bytesSent);
			span->SetStatus(response.IsSuccess()
				? ot::trace::StatusCode::kOk
				: ot::trace::StatusCode::kError,
				"HTTP " + std::to_string(response.statusCode));
			span->End();

			return response;
		}
		catch (const std::exception& ex)
		{
			span->SetStatus(ot::trace::StatusCode::kError, ex.what());
			span->End();
			throw;
		}
	}
}
