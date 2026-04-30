#pragma once
#include "CookieStore.h"
#include "../IHttpBackend.h"
#include "../HttpRequest.h"

namespace WebView2Http
{
	/// Injects cookies from the CookieStore into outgoing HTTP requests.
	class CookieBridge
	{
	public:
		/// Returns a copy of the request enriched with a Cookie header.
		static HttpRequest Enrich(const HttpRequest& request, const CookieStore& store);

		/// Called after a backend switch — can be used to push cookies via
		/// backend-specific APIs if needed (e.g., WinHttpSetCookieW).
		static void Apply(const CookieStore& store, IHttpBackend& backend);
	};
}
