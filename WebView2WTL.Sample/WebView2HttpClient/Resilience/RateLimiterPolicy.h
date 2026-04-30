#pragma once
#include "IPolicy.h"
#include <chrono>
#include <mutex>
#include <deque>

namespace WebView2Http
{
	/// Sliding-window rate limiter — max N requests per window duration.
	class RateLimiterPolicy final : public IPolicy
	{
	public:
		explicit RateLimiterPolicy(
			int                       maxRequests  = 100,
			std::chrono::seconds      window       = std::chrono::seconds{1});

		HttpResponse Execute(const HttpRequest& request, SendFn next) override;

	private:
		int                                          m_maxRequests;
		std::chrono::seconds                         m_window;
		mutable std::mutex                           m_mutex;
		std::deque<std::chrono::steady_clock::time_point> m_timestamps;
	};
}
