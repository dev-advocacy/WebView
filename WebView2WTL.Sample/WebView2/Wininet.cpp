#include "pch.h"
#include "logger.h"
#include "Wininet.h"

namespace os
{
	
	bool Wininet::VerifyCookie2(const INTERNET_COOKIE2& cookie2, bool allowExpired)
	{
		// Check required fields
		if (cookie2.pwszName == nullptr || wcslen(cookie2.pwszName) == 0) {
			return false; // Name is required
		}

		// Value can be empty (for deletion), but pointer must be valid if specified
		if (cookie2.pwszValue == nullptr) {
			return false;
		}

		// Check domain format if specified
		if (cookie2.pwszDomain != nullptr && wcslen(cookie2.pwszDomain) > 0) {
			// Domain should start with a dot for wildcard or contain at least one dot
			if (cookie2.pwszDomain[0] != L'.' && wcschr(cookie2.pwszDomain, L'.') == nullptr) {
				return false; // Invalid domain format
			}
		}

		// Check path format if specified
		if (cookie2.pwszPath != nullptr && wcslen(cookie2.pwszPath) > 0) {
			// Path should start with a slash
			if (cookie2.pwszPath[0] != L'/') {
				return false; // Invalid path format
			}
		}

		// Check expiration time if set
		if (cookie2.fExpiresSet) {
			// Convert FILETIME to system time for easier comparison
			SYSTEMTIME expiryTime;
			SYSTEMTIME currentTime;
			FILETIME currentFileTime;

			GetSystemTimeAsFileTime(&currentFileTime);
			FileTimeToSystemTime(&cookie2.ftExpires, &expiryTime);
			FileTimeToSystemTime(&currentFileTime, &currentTime);

			// Compare dates - allow expired cookies only if explicitly requested
			if (!allowExpired) {
				// Compare year and month first for efficiency
				if (expiryTime.wYear < currentTime.wYear ||
					(expiryTime.wYear == currentTime.wYear && expiryTime.wMonth < currentTime.wMonth) ||
					(expiryTime.wYear == currentTime.wYear && expiryTime.wMonth == currentTime.wMonth &&
						expiryTime.wDay < currentTime.wDay)) {
					return false; // Cookie is expired
				}

				// If same day, check time
				if (expiryTime.wYear == currentTime.wYear &&
					expiryTime.wMonth == currentTime.wMonth &&
					expiryTime.wDay == currentTime.wDay) {
					// Check hours, minutes, seconds
					if (expiryTime.wHour < currentTime.wHour ||
						(expiryTime.wHour == currentTime.wHour && expiryTime.wMinute < currentTime.wMinute) ||
						(expiryTime.wHour == currentTime.wHour && expiryTime.wMinute == currentTime.wMinute &&
							expiryTime.wSecond < currentTime.wSecond)) {
						return false; // Cookie is expired
					}
				}
			}
		}

		// Check for mutually exclusive flags
		if ((cookie2.dwFlags & INTERNET_COOKIE_SAME_SITE_LAX) &&
			(cookie2.dwFlags & INTERNET_COOKIE_SAME_SITE_STRICT)) {
			return false; // Cannot be both Lax and Strict
		}

		// Check for SameSite=None with Secure flag (required by browsers)
		if ((cookie2.dwFlags & 0x00002000) && !(cookie2.dwFlags & INTERNET_COOKIE_IS_SECURE)) {
			return false; // SameSite=None requires Secure flag
		}

		return true; // All checks passed
	}

	bool Wininet::SetCookieUsingInternetSetCookieEx(
		const std::wstring& url,
		const INTERNET_COOKIE2& cookie2)
	{
		// Construct cookie string from cookie2 structure
		std::wstring cookieData = cookie2.pwszName + std::wstring(L"=") +
			(cookie2.pwszValue ? std::wstring(cookie2.pwszValue) : std::wstring());

		// Add domain attribute if present
		if (cookie2.pwszDomain != nullptr) {
			cookieData += L"; Domain=" + std::wstring(cookie2.pwszDomain);
		}

		// Add path attribute if present
		if (cookie2.pwszPath != nullptr) {
			cookieData += L"; Path=" + std::wstring(cookie2.pwszPath);
		}

		// Add expiration if present
		if (cookie2.fExpiresSet) {
			// Convert FILETIME to string for the Expires attribute
			SYSTEMTIME expiryTime;
			FileTimeToSystemTime(&cookie2.ftExpires, &expiryTime);

			// Format the date as RFC 1123 format for cookies
			wchar_t dateStr[64] = { 0 };
			static const wchar_t* dayNames[] = { L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" };
			static const wchar_t* monthNames[] = { L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
												  L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec" };

			swprintf_s(dateStr, 64, L"%ls, %02d-%ls-%04d %02d:%02d:%02d GMT",
				dayNames[expiryTime.wDayOfWeek],
				expiryTime.wDay,
				monthNames[expiryTime.wMonth - 1],
				expiryTime.wYear,
				expiryTime.wHour,
				expiryTime.wMinute,
				expiryTime.wSecond);

			cookieData += L"; Expires=" + std::wstring(dateStr);
		}

		// Add flags
		if (cookie2.dwFlags & INTERNET_COOKIE_IS_SECURE) {
			cookieData += L"; Secure";
		}

		if (cookie2.dwFlags & INTERNET_COOKIE_HTTPONLY) {
			cookieData += L"; HttpOnly";
		}

		// Add SameSite attribute if applicable
		if (cookie2.dwFlags & INTERNET_COOKIE_SAME_SITE_LAX) {
			cookieData += L"; SameSite=Lax";
		}
		else if (cookie2.dwFlags & INTERNET_COOKIE_SAME_SITE_STRICT) {
			cookieData += L"; SameSite=Strict";
		}
		else if (cookie2.dwFlags & 0x00002000) { // INTERNET_COOKIE_SAME_SITE_NONE
			cookieData += L"; SameSite=None";
		}
		// Call InternetSetCookieExW
		DWORD cookie_set = ::InternetSetCookieExW(
			url.c_str(),
			cookie2.pwszName,
			cookieData.c_str(),
			cookie2.dwFlags,
			NULL
		);

		if (!cookie_set) {
			DWORD dwError = GetLastError();
			LOG_TRACE << __FUNCTION__ << L" InternetSetCookieExW failed with error code: " << dwError;
			return false;
		}
		else
		{
			// dump the cookie
			LOG_TRACE << __FUNCTION__ << L" InternetSetCookieExW" << cookie2.pwszName << L"=" << cookie2.pwszValue
				<< L" Domain=" << (cookie2.pwszDomain ? cookie2.pwszDomain : L"")
				<< L" Path=" << (cookie2.pwszPath ? cookie2.pwszPath : L"")
				<< L" Expires=" << (cookie2.fExpiresSet ? std::to_wstring(cookie2.ftExpires.dwLowDateTime) : L"Not Set")
				<< L" Flags=" << cookie2.dwFlags;
			
		}

		return true;
	}

	bool Wininet::SyncCookie(const std::wstring& url, const std::wstring& cookieName, const std::wstring& cookieValue)
	{
		std::wregex cookieRegex(LR"(([^=]+)=([^;]*)((?:;\s*(?:Domain|Expires|Path|SameSite)=[^;]*|;\s*(?:Secure|HttpOnly))*))", std::regex_constants::icase);

		std::wsmatch matches;
		if (std::regex_search(cookieValue, matches, cookieRegex)) {
			std::wstring name = matches[1].str();
			std::wstring value = matches[2].str();
			std::wstring attributes = matches[3].str();

			// Parse attributes separately
			std::wregex attrRegex(LR"(;\s*(Domain|Expires|Path|SameSite)=([^;]*)|;\s*(Secure|HttpOnly))", std::regex_constants::icase);
			std::wstring::const_iterator searchStart(attributes.cbegin());
			std::wstring::const_iterator searchEnd(attributes.cend());
			std::wsmatch attrMatch;

			std::map<std::wstring, std::wstring> attrMap;

			while (std::regex_search(searchStart, searchEnd, attrMatch, attrRegex)) {
				if (attrMatch[3].matched) {
					// Flag attribute (Secure or HttpOnly)
					attrMap[attrMatch[3].str()] = L"true";
				}
				else {
					// Named attribute with value
					attrMap[attrMatch[1].str()] = attrMatch[2].str();
				}
				searchStart = attrMatch.suffix().first;
			}

			INTERNET_COOKIE2 cookie2 = {};

			cookie2.pwszName = const_cast<LPWSTR>(name.c_str());
			cookie2.pwszValue = const_cast<LPWSTR>(value.c_str());


			// Now you can use the parsed data to set the cookie with WinInet
			std::wstring cookieData = name + L"=" + value;

			// Add attributes in the right format
			if (attrMap.contains(L"Domain")) {
				cookie2.pwszDomain = const_cast<LPWSTR>(attrMap[L"Domain"].c_str());
			}
			if (attrMap.contains(L"Expires")) {
				cookieData += L"; Expires=" + attrMap[L"Expires"];
			}
			if (attrMap.contains(L"Path")) {
				cookie2.pwszPath = const_cast<LPWSTR>(attrMap[L"Path"].c_str());
			}
			if (attrMap.contains(L"Secure")) {
				cookie2.dwFlags |= INTERNET_COOKIE_IS_SECURE;
			}
			if (attrMap.contains(L"HttpOnly")) {
				cookie2.dwFlags |= INTERNET_COOKIE_HTTPONLY;
			}

			if (attrMap.contains(L"SameSite")) {
				// Set SameSite flag based on value (case insensitive)
				std::wstring sameSite = attrMap[L"SameSite"];
				// Convert to lowercase for case-insensitive comparison
				std::transform(sameSite.begin(), sameSite.end(), sameSite.begin(), ::towlower);

				if (sameSite == L"none") {
					cookie2.dwFlags |= 0x00002000; // INTERNET_COOKIE_SAME_SITE_NONE
				}
				else if (sameSite == L"lax") {
					cookie2.dwFlags |= INTERNET_COOKIE_SAME_SITE_LAX;
				}
				else if (sameSite == L"strict") {
					cookie2.dwFlags |= INTERNET_COOKIE_SAME_SITE_STRICT;
				}
			}
			// Set expiration if present
			if (attrMap.contains(L"Expires"))
			{
				cookie2.fExpiresSet = TRUE;
				std::wstring expiresStr = attrMap[L"Expires"];

				// Use InternetTimeToSystemTimeW to parse the HTTP date format
				SYSTEMTIME systemTime = {};
				if (InternetTimeToSystemTimeW(expiresStr.c_str(), &systemTime, 0)) {
					// Convert SYSTEMTIME to FILETIME
					FILETIME fileTime = {};
					if (SystemTimeToFileTime(&systemTime, &fileTime)) {
						cookie2.ftExpires = fileTime;
					}
				}
			}

			if (!VerifyCookie2(cookie2)) {
				// Log error or handle invalid cookie
				return false;
			}

			DWORD pdwCookieState = 0;
			auto hErr = InternetSetCookieEx2(url.c_str(), &cookie2, nullptr, 0, &pdwCookieState);
			if (hErr != ERROR_SUCCESS && pdwCookieState == COOKIE_STATE_UNKNOWN)
			{
				// Handle error return false, use LOG_TRACE to log the error
				DWORD dwError = GetLastError();
				LOG_TRACE << __FUNCTION__ << L" InternetSetCookieEx2 failed with error code: " << dwError;
				// Fall back to InternetSetCookieExW
				return SetCookieUsingInternetSetCookieEx(url, cookie2);

			}
			else
			{
				// dump the cookie
				LOG_TRACE << __FUNCTION__ << L" InternetSetCookieEx2= Name=" << cookie2.pwszName 
					<< L" Domain=" << (cookie2.pwszDomain ? cookie2.pwszDomain : L"")
					<< L" Path=" << (cookie2.pwszPath ? cookie2.pwszPath : L"")
					<< L" Expires=" << (cookie2.fExpiresSet ? std::to_wstring(cookie2.ftExpires.dwLowDateTime) : L"Not Set")
					<< L" Value=" << cookie2.pwszValue
					<< L" Flags=" << cookie2.dwFlags;
			}
		}
		return true;	
	}

	std::wstring Wininet::GetCookie(const std::wstring& url, const std::wstring& cookieName)
	{
		DWORD size = 0;
		// First call to get the required buffer size
		::InternetGetCookieExW(url.c_str(), cookieName.empty() ? nullptr : cookieName.c_str(), nullptr, &size, INTERNET_COOKIE_HTTPONLY, nullptr);
		if (size == 0)
			return L"";

		std::wstring buffer(size / sizeof(wchar_t), L'\0');
		if (::InternetGetCookieExW(url.c_str(), cookieName.empty() ? nullptr : cookieName.c_str(), &buffer[0], &size, INTERNET_COOKIE_HTTPONLY, nullptr))
		{
			// Remove trailing nulls
			buffer.resize(wcsnlen(buffer.c_str(), buffer.size()));
			return buffer;
		}
		return L"";
	}

	void Wininet::GetCookies(const std::wstring& url)
	{ 
		// get all cookies using InternetGetCookieEx2

		DWORD size = 0;
		INTERNET_COOKIE2 *cookie2 = nullptr;
		if (InternetGetCookieEx2(url.c_str(), nullptr, 0, &cookie2 , &size ) == ERROR_SUCCESS)
		{
			// dump all cookies
			LOG_TRACE << __FUNCTION__ << L" InternetGetCookieEx2 returned " << size / sizeof(INTERNET_COOKIE2) << L" cookies.";
			for (DWORD i = 0; i < size ; ++i)
			{
				const INTERNET_COOKIE2& cookie = cookie2[i];
				LOG_TRACE << L"Cookie " << i + 1 << L": Name=" << (cookie.pwszName ? cookie.pwszName : L"")
					<< L", Value=" << (cookie.pwszValue ? cookie.pwszValue : L"")
					<< L", Domain=" << (cookie.pwszDomain ? cookie.pwszDomain : L"")
					<< L", Path=" << (cookie.pwszPath ? cookie.pwszPath : L"")
					<< L", Expires=" << (cookie.fExpiresSet ? std::to_wstring(cookie.ftExpires.dwLowDateTime) : L"Not Set")
					<< L", Flags=" << cookie.dwFlags;
			}
			// Free the allocated memory for cookies
			if (cookie2)
			{
				InternetFreeCookies(cookie2, size);
			}
		}
		else
		{
			DWORD dwError = GetLastError();
			LOG_TRACE << __FUNCTION__ << L" InternetGetCookieEx2 failed with error code: " << dwError;
		}
	}

	bool Wininet::SetCookie(const std::wstring& url, const std::wstring& cookieName, const std::wstring& cookieValue, const std::wstring& options)
	{
		std::wstring cookieData = cookieName + L"=" + cookieValue;
		if (!options.empty())
			cookieData += L"; " + options;
		return ::InternetSetCookieExW(url.c_str(), cookieName.c_str(), cookieData.c_str(), INTERNET_COOKIE_HTTPONLY, NULL) != 0;
	}

	bool Wininet::DeleteCookie(const std::wstring& url, const std::wstring& cookieName)
	{
		// To delete a cookie, set its expiration date to a past date
		std::wstring expiredCookie = cookieName + L"=; expires=Thu, 01-Jan-1970 00:00:00 GMT";
		return ::InternetSetCookieExW(url.c_str(), cookieName.c_str(), expiredCookie.c_str(), INTERNET_COOKIE_HTTPONLY, NULL) != 0;
	}

	std::vector<std::wstring> Wininet::GetAllCookies(const std::wstring& url)
	{
		DWORD size = 0;
		::InternetGetCookieExW(url.c_str(), nullptr, nullptr, &size, INTERNET_COOKIE_HTTPONLY, nullptr);
		if (size == 0)
			return {};

		std::wstring buffer(size / sizeof(wchar_t), L'\0');
		if (!::InternetGetCookieExW(url.c_str(), nullptr, &buffer[0], &size, INTERNET_COOKIE_HTTPONLY, nullptr))
			return {};

		buffer.resize(wcsnlen(buffer.c_str(), buffer.size()));

		// Cookies are returned as a single string: "name1=value1; name2=value2; ..."
		std::vector<std::wstring> cookies;
		size_t start = 0;
		while (start < buffer.size()) {
			size_t end = buffer.find(L"; ", start);
			if (end == std::wstring::npos) {
				cookies.push_back(buffer.substr(start));
				break;
			}
			cookies.push_back(buffer.substr(start, end - start));
			start = end + 2;
		}
		return cookies;
	}

	// Helper to trim whitespace
	static inline std::wstring Trim(const std::wstring& str)
	{
		size_t first = str.find_first_not_of(L" \t\r\n");
		size_t last = str.find_last_not_of(L" \t\r\n");
		if (first == std::wstring::npos || last == std::wstring::npos)
			return L"";
		return str.substr(first, last - first + 1);
	}

	

};
