#include "pch.h"
#include "logger.h"
#include "utility.h"
#include "WebViewProfile.h"
#include "CommandLineParser.h"

HRESULT CWebViewProfile::Profile(ProfileInformation_t& profile)
{
	LOG_TRACE(__FUNCTION__);

	int argc = 0;
	LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
		
	CommandLineParser parser;
	parser.AddOption("help", "produce help message");
	parser.AddOption("version", "set WebView version (format: x.y.z.t)");
	parser.AddOption("channel", "set WebView2 channel: beta, dev, canary, or fixed (empty for stable)");
	parser.AddOption("test", "start the WebView2 in test mode");
	parser.AddOption("port", "specify a communication port for the test");
	parser.AddOption("root", "User-provided WebView2 root folder");

	parser.Parse(argc, argv);

	if (parser.HasOption(L"help")) 
	{	
		std::string helpMsg = parser.GetHelpMessage();
		MessageBox(nullptr, CA2T(helpMsg.c_str()).m_psz, L"Help", MB_ICONINFORMATION);
		return S_OK;
	}

	std::wstring webView2Version = L"";
	if (parser.HasOption(L"version"))
	{   // Assume first argument is WebView2 version to use, in the format "x.y.z.t".
		webView2Version = parser.GetValueOr(L"version", L"");
		LOG_TRACE(std::string("User-provided WebView2 version=") + WideToNarrow(webView2Version));
	}	
	
	std::wstring webView2Channel = L"";
	if (parser.HasOption(L"channel"))
	{   // Assume second argument is WebView2 channel to use: "beta", "dev", "canary" or "fixed" "" for stable channel.
		webView2Channel = parser.GetValueOr(L"channel", L"");
		LOG_TRACE(std::string("User-provided WebView2 channel=") + WideToNarrow(webView2Channel));
	}
	
	std::wstring webViewFolder = L"";
	if (parser.HasOption(L"root"))
	{   // Assume second argument of WebView2 channel to use: "fixed", we read the folder
		webViewFolder = parser.GetValueOr(L"root", L"");
		LOG_TRACE(std::string("User-provided WebView2 root folder=") + WideToNarrow(webViewFolder));
	}
	
	bool isTest = false;
	std::wstring webViewport = L"";
	if (parser.HasOption(L"test"))
	{
		if (parser.HasOption(L"port"))
		{
			isTest = true;
			webViewport = parser.GetValueOr(L"port", L"");
			LOG_TRACE(std::string("test is on, port is=") + WideToNarrow(webViewport));
		}
		else
		{
			LOG_ERROR("Error: 'port' option is required when 'test' is specified.");
			MessageBox(nullptr, L"Error: 'port' option is required when 'test' is specified.", L"Error", MB_ICONERROR);
			return E_INVALIDARG;
		}
	}

	// Verify that the WebView2 runtime is installed.	
	PWSTR edgeVersionInfo = nullptr;
	HRESULT hr = ::GetAvailableCoreWebView2BrowserVersionString(nullptr, &edgeVersionInfo);
	if (FAILED(hr) || (edgeVersionInfo == nullptr))
	{
		LOG_TRACE("The WebView2 runtime is not installed");
		LOG_TRACE("Please install the WebView2 runtime before running this application available on https://go.microsoft.com/fwlink/p/?LinkId=2124703");
		return (hr);
	}
	LOG_TRACE(std::string("Found installed WebView version=") + WideToNarrow(edgeVersionInfo));

	if (webView2Version.empty())
	{   // User did not provided specific WebView2 versions and channels.
		// Set WebView2 version and channel to default values. 
		std::wstring edgeVersionInfoStr = edgeVersionInfo;
		size_t pos = edgeVersionInfoStr.find(L' ');

		if ((edgeVersionInfoStr.size() > 0) && (pos < edgeVersionInfoStr.size() - 1))
		{   // Assume Edge version with format 'x.y.z.t channel"
			webView2Version = edgeVersionInfoStr.substr(0, pos);
			edgeVersionInfo[pos] = L'\0'; // Ensure webView2Version is null-terminated.
			webView2Channel = edgeVersionInfoStr.substr(pos + 1, edgeVersionInfoStr.size() - pos - 1);
		}
		else
		{   // Assume Edge version with format 'x.y.z.t"
			webView2Version = edgeVersionInfoStr;
		}

		LOG_TRACE(std::string("Using WebView2 version=") + WideToNarrow(webView2Version));
		LOG_TRACE(std::string("Using WebView2 channel=") + WideToNarrow(webView2Channel));
	}
	profile.browserDirectory = WebView2::Utility::GetBrowserDirectory(webView2Version, webView2Channel, webViewFolder);
	profile.userDataDirectory = WebView2::Utility::GetUserDataDirectory(webView2Channel);
	profile.channel = webView2Channel.empty() ? L"stable release" : webView2Channel;	
	profile.version = webView2Version;
	profile.isTest = isTest;
	profile.port = webViewport;
	return (hr);
}
