#include "FallbackPolicy.h"

namespace WebView2Http
{
	FallbackPolicy::FallbackPolicy(FallbackFn fallback)
		: m_fallback(std::move(fallback))
	{}

	HttpResponse FallbackPolicy::Execute(const HttpRequest& request, SendFn next)
	{
		try
		{
			return next(request);
		}
		catch (...)
		{
			return m_fallback(request, std::current_exception());
		}
	}

	FallbackPolicy FallbackPolicy::WithResponse(HttpResponse fallbackResponse)
	{
		return FallbackPolicy([resp = std::move(fallbackResponse)]
			(const HttpRequest&, std::exception_ptr) -> HttpResponse
		{
			return resp;
		});
	}
}
