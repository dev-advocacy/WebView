#include "CookieBridge.h"

namespace WebView2Http
{
	HttpRequest CookieBridge::Enrich(const HttpRequest& request, const CookieStore& store)
	{
		HttpRequest enriched = request;

		std::wstring cookieHeader = store.BuildCookieHeader(request.url);
		if (!cookieHeader.empty())
		{
			// Append to existing Cookie header if present
			auto it = enriched.headers.find(L"Cookie");
			if (it != enriched.headers.end())
				it->second += L"; " + cookieHeader;
			else
				enriched.headers[L"Cookie"] = cookieHeader;
		}

		return enriched;
	}

	void CookieBridge::Apply(const CookieStore& /*store*/, IHttpBackend& /*backend*/)
	{
		// For WinHTTP / WinINet, cookies are injected per-request via Enrich().
		// This method exists for backends that require session-level cookie setup.
		// No-op for current backends.
	}
}
