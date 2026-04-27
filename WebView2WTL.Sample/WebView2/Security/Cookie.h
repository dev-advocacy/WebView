#pragma once

namespace WebView2
{
	struct Cookie
	{
		std::wstring name;
		std::wstring value;
		std::wstring domain;
		std::wstring path;
		std::wstring expires;
		std::wstring expiresReadable;
		bool httpOnly = false;
		bool secure = false;
		// Add more fields as needed (e.g., sameSite, size, etc.)
	};
};
