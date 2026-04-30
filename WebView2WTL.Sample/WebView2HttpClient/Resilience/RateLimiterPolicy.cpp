#include "RateLimiterPolicy.h"
#include <stdexcept>

namespace WebView2Http
{
	RateLimiterPolicy::RateLimiterPolicy(int maxRequests, std::chrono::seconds window)
		: m_maxRequests(maxRequests), m_window(window)
	{}

	HttpResponse RateLimiterPolicy::Execute(const HttpRequest& request, SendFn next)
	{
		auto now = std::chrono::steady_clock::now();
		{
			std::lock_guard lock(m_mutex);
			// Evict old timestamps outside the window
			while (!m_timestamps.empty() && (now - m_timestamps.front()) > m_window)
				m_timestamps.pop_front();

			if (static_cast<int>(m_timestamps.size()) >= m_maxRequests)
				throw std::runtime_error("Rate limit exceeded: " +
					std::to_string(m_maxRequests) + " req/" +
					std::to_string(m_window.count()) + "s");

			m_timestamps.push_back(now);
		}
		return next(request);
	}
}
