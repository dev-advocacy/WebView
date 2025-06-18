#pragma once

namespace os
{
	class Wininet
	{
	public:
		// Gets a cookie value for the specified URL and cookie name.
		static std::wstring GetCookie(const std::wstring& url, const std::wstring& cookieName);

		// Sets a cookie for the specified URL.
		static bool SetCookie(const std::wstring& url, const std::wstring& cookieName, const std::wstring& cookieValue, const std::wstring& options = L"");

		// Deletes a cookie for the specified URL and cookie name.
		static bool DeleteCookie(const std::wstring& url, const std::wstring& cookieName);
		// Gets all cookies for the specified URL.
		std::vector<std::wstring> GetAllCookies(const std::wstring& url);
		
		static bool SyncCookie(const std::wstring& url, const std::wstring& cookieName, const std::wstring& cookieValue);

		void GetCookies(const std::wstring& url);

	private:
		static bool VerifyCookie2(const INTERNET_COOKIE2& cookie2, bool allowExpired = false);
		static bool SetCookieUsingInternetSetCookieEx(const std::wstring& url, const INTERNET_COOKIE2& cookie2);
	};
};
