#pragma once
#include "IPolicy.h"
#include "RetryPolicy.h"
#include "CircuitBreakerPolicy.h"
#include "TimeoutPolicy.h"
#include "RateLimiterPolicy.h"
#include "FallbackPolicy.h"
#include <vector>
#include <memory>

namespace WebView2Http
{
	/// Chains multiple policies together.
	/// Default pipeline: RateLimit -> CircuitBreaker -> Timeout -> Retry -> send
	class PolicyExecutor
	{
	public:
		PolicyExecutor();

		void SetPolicies(std::vector<std::shared_ptr<IPolicy>> policies);
		void AddPolicy(std::shared_ptr<IPolicy> policy);

		HttpResponse Execute(const HttpRequest& request, SendFn innerSend);

		// Direct access for runtime tuning
		RetryPolicy&          Retry()          noexcept { return *m_retry; }
		CircuitBreakerPolicy& CircuitBreaker() noexcept { return *m_circuitBreaker; }
		TimeoutPolicy&        Timeout()        noexcept { return *m_timeout; }
		RateLimiterPolicy&    RateLimiter()    noexcept { return *m_rateLimiter; }

	private:
		std::shared_ptr<RetryPolicy>          m_retry;
		std::shared_ptr<CircuitBreakerPolicy> m_circuitBreaker;
		std::shared_ptr<TimeoutPolicy>        m_timeout;
		std::shared_ptr<RateLimiterPolicy>    m_rateLimiter;
		std::vector<std::shared_ptr<IPolicy>> m_chain;
	};
}
