#pragma once

typedef struct ProfileInformation
{
	std::wstring  browserDirectory;
	std::wstring  userDataDirectory;
	std::wstring  channel;
	std::wstring  version;
	std::wstring  port;
	std::wstring  initialUrl;   // target URL for WinInet pre-selection (--url or WEBVIEW2_INITIAL_URL)
	bool isTest;

} ProfileInformation_t;

class CWebViewProfile
{
public:
	static HRESULT Profile(ProfileInformation_t& profile);
};

