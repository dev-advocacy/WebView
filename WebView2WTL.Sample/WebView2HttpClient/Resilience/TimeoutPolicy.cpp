#include "TimeoutPolicy.h"
#include <future>
#include <stdexcept>

namespace WebView2Http
{
	TimeoutPolicy::TimeoutPolicy(std::chrono::milliseconds timeout)
		: m_timeout(timeout)
	{}

	HttpResponse TimeoutPolicy::Execute(const HttpRequest& request, SendFn next)
	{
		// Override the request timeout too so the backend respects it
		HttpRequest req = request;
		req.timeout     = m_timeout;

		auto future = std::async(std::launch::async, [&]() { return next(req); });

		if (future.wait_for(m_timeout) == std::future_status::timeout)
			throw std::runtime_error("Request timed out after " +
				std::to_string(m_timeout.count()) + " ms");

		return future.get();
	}
}
