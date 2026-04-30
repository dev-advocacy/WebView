#pragma once
#include "IPolicy.h"
#include <chrono>

namespace WebView2Http
{
	/// Cancels the request if it exceeds the given duration.
	class TimeoutPolicy final : public IPolicy
	{
	public:
		explicit TimeoutPolicy(std::chrono::milliseconds timeout = std::chrono::seconds{10});
		HttpResponse Execute(const HttpRequest& request, SendFn next) override;

	private:
		std::chrono::milliseconds m_timeout;
	};
}
