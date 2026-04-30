#pragma once
#include "IPolicy.h"
#include <atomic>
#include <chrono>
#include <mutex>

namespace WebView2Http
{
	enum class CircuitState { Closed, Open, HalfOpen };

	/// Circuit breaker — opens after N consecutive failures, resets after a cooldown.
	class CircuitBreakerPolicy final : public IPolicy
	{
	public:
		explicit CircuitBreakerPolicy(
			int                       failureThreshold = 5,
			std::chrono::seconds      openDuration     = std::chrono::seconds{30});

		HttpResponse Execute(const HttpRequest& request, SendFn next) override;

		CircuitState GetState() const noexcept;

	private:
		int                                          m_threshold;
		std::chrono::seconds                         m_openDuration;
		mutable std::mutex                           m_mutex;
		CircuitState                                 m_state      { CircuitState::Closed };
		int                                          m_failures   { 0 };
		std::chrono::steady_clock::time_point        m_openedAt;
	};
}
