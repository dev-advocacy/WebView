#pragma once
#include "../HttpRequest.h"
#include "../HttpResponse.h"
#include <functional>
#include <memory>
#include <string>

// OpenTelemetry forward declarations
namespace opentelemetry { namespace trace { class Tracer; } }
namespace opentelemetry { namespace metrics { class Meter; } }

namespace WebView2Http
{
	using SendFn = std::function<HttpResponse(const HttpRequest&)>;

	/// Wraps every HTTP call with an OTel span + metrics recording.
	class HttpTelemetry
	{
	public:
		HttpTelemetry();
		~HttpTelemetry();

		/// Wraps innerSend with a trace span and records latency/bytes metrics.
		HttpResponse Track(const HttpRequest& request, SendFn innerSend);

		/// Service name used for spans (default: "WebView2HttpClient")
		void SetServiceName(std::wstring name);

	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}
