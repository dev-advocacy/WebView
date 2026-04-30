#pragma once
#include "IPolicy.h"
#include <functional>

namespace WebView2Http
{
	/// Fallback policy — returns a default response on failure instead of throwing.
	class FallbackPolicy final : public IPolicy
	{
	public:
		using FallbackFn = std::function<HttpResponse(const HttpRequest&, std::exception_ptr)>;

		explicit FallbackPolicy(FallbackFn fallback);

		HttpResponse Execute(const HttpRequest& request, SendFn next) override;

		/// Convenience: return a fixed response on any error.
		static FallbackPolicy WithResponse(HttpResponse fallbackResponse);

	private:
		FallbackFn m_fallback;
	};
}
