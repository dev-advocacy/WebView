#include "RetryPolicy.h"
#include <thread>
#include <stdexcept>

namespace WebView2Http
{
	RetryPolicy::RetryPolicy(int maxRetries, std::chrono::milliseconds baseDelay, bool exponential)
		: m_maxRetries(maxRetries), m_baseDelay(baseDelay), m_exponential(exponential)
	{}

	bool RetryPolicy::IsRetryable(const HttpResponse& r)
	{
		// Retry on server errors and 429 (rate limit)
		return r.statusCode == 429 ||
			   r.statusCode == 503 ||
			   r.statusCode == 502 ||
			   r.statusCode == 504 ||
			   r.statusCode == 0;  // network error
	}

	HttpResponse RetryPolicy::Execute(const HttpRequest& request, SendFn next)
	{
		HttpResponse last{};
		auto delay = m_baseDelay;

		for (int attempt = 0; attempt <= m_maxRetries; ++attempt)
		{
			try
			{
				last = next(request);
				if (!IsRetryable(last)) return last;
			}
			catch (const std::exception&)
			{
				if (attempt == m_maxRetries) throw;
			}

			if (attempt < m_maxRetries)
			{
				std::this_thread::sleep_for(delay);
				if (m_exponential) delay *= 2;
			}
		}
		return last;
	}
}
