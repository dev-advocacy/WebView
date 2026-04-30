#pragma once
#include <string>
#include <vector>
#include <mutex>

// Forward declaration — avoid pulling in WebView2 headers here
namespace WebView2 { struct Cookie; }

namespace WebView2Http
{
	struct StoredCookie
	{
		std::wstring name;
		std::wstring value;
		std::wstring domain;
		std::wstring path;
		bool         secure   = false;
		bool         httpOnly = false;
	};

	/// Thread-safe cookie jar. Populated from WebView2 cookies.
	class CookieStore
	{
	public:
		void SyncFromWebView2(const std::vector<WebView2::Cookie>& cookies);
		void Clear();

		/// Returns all cookies matching the given URL host + path.
		std::vector<StoredCookie> GetForUrl(const std::wstring& url) const;

		/// Returns a "Cookie: name=value; ..." header value for the given URL.
		std::wstring BuildCookieHeader(const std::wstring& url) const;

	private:
		mutable std::mutex            m_mutex;
		std::vector<StoredCookie>     m_cookies;
	};
}
