#include "CookieStore.h"
#include "../../../WebView2/Security/Cookie.h"
#include <algorithm>
#include <sstream>

namespace WebView2Http
{
	void CookieStore::SyncFromWebView2(const std::vector<WebView2::Cookie>& cookies)
	{
		std::lock_guard lock(m_mutex);
		m_cookies.clear();
		m_cookies.reserve(cookies.size());

		for (auto& c : cookies)
		{
			StoredCookie sc;
			sc.name     = c.name;
			sc.value    = c.value;
			sc.domain   = c.domain;
			sc.path     = c.path;
			sc.secure   = c.secure;
			sc.httpOnly = c.httpOnly;
			m_cookies.push_back(std::move(sc));
		}
	}

	void CookieStore::Clear()
	{
		std::lock_guard lock(m_mutex);
		m_cookies.clear();
	}

	std::vector<StoredCookie> CookieStore::GetForUrl(const std::wstring& url) const
	{
		std::lock_guard lock(m_mutex);
		std::vector<StoredCookie> result;

		for (auto& c : m_cookies)
		{
			// Match domain (simple suffix check)
			if (!c.domain.empty() && url.find(c.domain) == std::wstring::npos)
				continue;
			// Skip secure cookies on non-https
			if (c.secure && url.substr(0, 5) != L"https")
				continue;
			result.push_back(c);
		}
		return result;
	}

	std::wstring CookieStore::BuildCookieHeader(const std::wstring& url) const
	{
		auto cookies = GetForUrl(url);
		if (cookies.empty()) return {};

		std::wostringstream ss;
		for (size_t i = 0; i < cookies.size(); ++i)
		{
			if (i > 0) ss << L"; ";
			ss << cookies[i].name << L"=" << cookies[i].value;
		}
		return ss.str();
	}
}
