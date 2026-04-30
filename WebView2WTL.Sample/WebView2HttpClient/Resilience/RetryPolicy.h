#pragma once
#include "IPolicy.h"
#include <chrono>

namespace WebView2Http
{
	/// Retry policy with exponential backoff — mirrors Polly's RetryPolicy.
	class RetryPolicy final : public IPolicy
	{
	public:
		explicit RetryPolicy(
			int                       maxRetries    = 3,
			std::chrono::milliseconds baseDelay     = std::chrono::milliseconds{200},
			bool                      exponential   = true);

		HttpResponse Execute(const HttpRequest& request, SendFn next) override;

	private:
		int                       m_maxRetries;
		std::chrono::milliseconds m_baseDelay;
		bool                      m_exponential;

		static bool IsRetryable(const HttpResponse& r);
	};
}
