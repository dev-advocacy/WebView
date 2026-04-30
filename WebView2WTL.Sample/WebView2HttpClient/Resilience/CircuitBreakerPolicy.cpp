#include "CircuitBreakerPolicy.h"
#include <stdexcept>

namespace WebView2Http
{
	CircuitBreakerPolicy::CircuitBreakerPolicy(int threshold, std::chrono::seconds openDuration)
		: m_threshold(threshold), m_openDuration(openDuration)
	{}

	CircuitState CircuitBreakerPolicy::GetState() const noexcept
	{
		std::lock_guard lock(m_mutex);
		return m_state;
	}

	HttpResponse CircuitBreakerPolicy::Execute(const HttpRequest& request, SendFn next)
	{
		{
			std::lock_guard lock(m_mutex);
			if (m_state == CircuitState::Open)
			{
				auto elapsed = std::chrono::steady_clock::now() - m_openedAt;
				if (elapsed >= m_openDuration)
					m_state = CircuitState::HalfOpen;
				else
					throw std::runtime_error("Circuit breaker is OPEN — request blocked");
			}
		}

		try
		{
			auto response = next(request);
			{
				std::lock_guard lock(m_mutex);
				if (!response.IsSuccess())
				{
					++m_failures;
					if (m_failures >= m_threshold)
					{
						m_state    = CircuitState::Open;
						m_openedAt = std::chrono::steady_clock::now();
					}
				}
				else
				{
					m_failures = 0;
					m_state    = CircuitState::Closed;
				}
			}
			return response;
		}
		catch (...)
		{
			std::lock_guard lock(m_mutex);
			++m_failures;
			if (m_failures >= m_threshold)
			{
				m_state    = CircuitState::Open;
				m_openedAt = std::chrono::steady_clock::now();
			}
			throw;
		}
	}
}
