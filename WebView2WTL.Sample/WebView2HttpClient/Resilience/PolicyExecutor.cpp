#include "PolicyExecutor.h"

namespace WebView2Http
{
	PolicyExecutor::PolicyExecutor()
		: m_retry         (std::make_shared<RetryPolicy>())
		, m_circuitBreaker(std::make_shared<CircuitBreakerPolicy>())
		, m_timeout       (std::make_shared<TimeoutPolicy>(std::chrono::seconds{30}))
		, m_rateLimiter   (std::make_shared<RateLimiterPolicy>(100, std::chrono::seconds{1}))
	{
		// Outermost first: RateLimit -> CircuitBreaker -> Timeout -> Retry
		m_chain = { m_rateLimiter, m_circuitBreaker, m_timeout, m_retry };
	}

	void PolicyExecutor::SetPolicies(std::vector<std::shared_ptr<IPolicy>> policies)
	{
		m_chain = std::move(policies);
	}

	void PolicyExecutor::AddPolicy(std::shared_ptr<IPolicy> policy)
	{
		m_chain.insert(m_chain.begin(), std::move(policy));
	}

	HttpResponse PolicyExecutor::Execute(const HttpRequest& request, SendFn innerSend)
	{
		// Build a chain of lambdas from outermost to innermost
		SendFn current = innerSend;

		// Wrap in reverse order so chain[0] executes first
		for (auto it = m_chain.rbegin(); it != m_chain.rend(); ++it)
		{
			auto policy  = *it;
			auto wrapped = current;
			current = [policy, wrapped](const HttpRequest& req) -> HttpResponse
			{
				return policy->Execute(req, wrapped);
			};
		}

		return current(request);
	}
}
