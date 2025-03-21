#pragma once

typedef struct ProfileInformation
{
    std::wstring  browserDirectory;
	std::wstring  userDataDirectory;
	std::wstring  channel;
	std::wstring  version;
	std::wstring  port;
	bool isTest;

} ProfileInformation_t;

class CWebViewProfile
{
public:
	static HRESULT Profile(ProfileInformation_t& profile);
};

