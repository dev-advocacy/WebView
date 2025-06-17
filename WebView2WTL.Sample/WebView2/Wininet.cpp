#include "pch.h"
#include "Wininet.h"

namespace os
{
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

	// Enumerate all cookies for all domains (current user) using WinINet API
	std::vector<std::wstring> Wininet::GetAllCookiesNoDomain()
	{
		std::vector<std::wstring> cookies;
		HANDLE hFind = nullptr;
		WIN32_FIND_DATAW findData;

		// Get the cookies folder path
		wchar_t cookiePath[MAX_PATH];
		if (!SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_COOKIES, nullptr, 0, cookiePath)))
			return cookies;

		std::wstring searchPath = std::wstring(cookiePath) + L"\\*.txt";
		hFind = FindFirstFileW(searchPath.c_str(), &findData);
		if (hFind == INVALID_HANDLE_VALUE)
			return cookies;

		do
		{
			std::wstring filePath = std::wstring(cookiePath) + L"\\" + findData.cFileName;
			FILE* file = nullptr;
			_wfopen_s(&file, filePath.c_str(), L"rt, ccs=UNICODE");
			if (!file)
				continue;

			wchar_t line[4096];
			while (fgetws(line, sizeof(line) / sizeof(wchar_t), file))
			{
				std::wstring wline = Trim(line);
				// Skip comments and empty lines
				if (wline.empty() || wline[0] == L'#')
					continue;
				// Netscape cookie file format: name, value, domain, path, etc.
				// Format: <domain> <flag> <path> <secure> <expiration> <name> <value>
				// We'll extract <name> and <value>
				std::vector<std::wstring> tokens;
				size_t pos = 0, prev = 0;
				while ((pos = wline.find(L'\t', prev)) != std::wstring::npos)
				{
					tokens.push_back(wline.substr(prev, pos - prev));
					prev = pos + 1;
				}
				tokens.push_back(wline.substr(prev));
				if (tokens.size() >= 7)
				{
					std::wstring name = tokens[5];
					std::wstring value = tokens[6];
					cookies.push_back(name + L"=" + value);
				}
			}
			fclose(file);
		} while (FindNextFileW(hFind, &findData));
		FindClose(hFind);

		return cookies;
	}

};
