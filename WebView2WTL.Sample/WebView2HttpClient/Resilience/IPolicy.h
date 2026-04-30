#pragma once
#include "../HttpRequest.h"
#include "../HttpResponse.h"
#include <functional>

namespace WebView2Http
{
	using SendFn = std::function<HttpResponse(const HttpRequest&)>;

	/// Base class for all resilience policies.
	class IPolicy
	{
	public:
		virtual ~IPolicy() = default;
		virtual HttpResponse Execute(const HttpRequest& request, SendFn next) = 0;
	};
}
