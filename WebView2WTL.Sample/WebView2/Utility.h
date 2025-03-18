#pragma once


#include "EdgeInfomation.h"

namespace WebView2
{
	class Utility
	{
	public:
		static std::wstring GetWebView2Version();
		static HRESULT DownloadWebView2Bootstrapper(std::wstring& bootstrapperPath);
		static HRESULT InstallWebView2(const std::wstring& bootstrapperPath, bool elevated);
		static std::wstring GetBrowserDirectory(std::wstring_view webView2Version, std::wstring_view webView2Channel, std::wstring_view webViewFolder);
		static std::wstring GetUserDataDirectory(std::wstring_view webView2Channel);
		static std::wstring_view GetRootEdgeDirectory(std::wstring_view webView2Channel);
		static std::wstring GetUserMUI();
		static HRESULT InitCOM();
		static std::wstring BoolToString(BOOL value);
		static std::wstring EncodeQuote(std::wstring raw);
		static std::wstring SecondsToString(UINT32 time);
		static std::wstring CookieToString(ICoreWebView2Cookie* cookie);
		static std::error_code GetUniqueLogFileName(fs::path& pFileName);
		static std::list<EdgeInfomation> EnumEdgeVersion();
	private:
		static std::wstring GetProgramFilesx86Directory();
		static std::wstring GetLangStringFromLangId(DWORD dwLangID_i, bool returnShortCode);		
		static EdgeInfomation GetVersionFromRegistry(HKEY key, std::wstring key_entry);
	};
};