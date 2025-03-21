#include "pch.h"
#include "logger.h"
#include "utility.h"
#include "WebViewProfile.h"

HRESULT CWebViewProfile::Profile(ProfileInformation_t& profile)
{
	LOG_TRACE << __FUNCTION__;

	int         argc = 0;
	LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
		
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("version", po::wvalue<std::wstring>(), "set WebView version")
		("channel", po::wvalue<std::wstring>(), "set WebView2 channel, beta, dev, canary or fixed \"\" for stable channel")
		("test", "start the WebView2 in test mode")
		("port", po::wvalue<std::wstring>(), "specify a communication port for the test")
		("root", po::wvalue<std::wstring>(), "User-provided WebView2 root folder");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) 
	{	
		std::stringstream ss;
		ss << desc;
		MessageBox(nullptr, CA2T(ss.str().data()).m_psz, L"Help", MB_ICONINFORMATION);
		return 0;
	}

	std::wstring_view webView2Version = L"";
	if (vm.count("version"))
	{   // Assume first argument is WebView2 version to use, in the format "x.y.z.t".
		webView2Version = vm["version"].as<std::wstring>();
		LOG_TRACE << "User-provided WebView2 version=" << webView2Version.data();
	}	
	std::wstring_view webView2Channel = L"";
	if (vm.count("channel"))
	{   // Assume second argument is WebView2 channel to use: "beta", "dev", "canary" or  "fixed" "" for stable channel.
		webView2Channel = vm["channel"].as<std::wstring>();
		LOG_TRACE << "User-provided WebView2 channel=" << webView2Channel.data();
	}
	std::wstring_view webViewFolder = L"";
	if (vm.count("root"))
	{   // Assume second argument of WebView2 channel to use: "fixed", we read the folder
		webViewFolder = vm["root"].as<std::wstring>();
		LOG_TRACE << "User-provided WebView2 root folder=" << webViewFolder.data();
	}
	bool isTest = false;
	std::wstring_view webViewport = L"";
	if (vm.count("test"))
	{
		if (vm.count("port"))
		{
			isTest = true;
			webViewport = vm["port"].as<std::wstring>();
			LOG_TRACE << "test is on, port is=" << webViewport.data();
		}
		else
		{
			LOG_ERROR << "Error: 'port' option is required when 'test' is specified.";
			MessageBox(nullptr, L"Error: 'port' option is required when 'test' is specified.", L"Error", MB_ICONERROR);
			return E_INVALIDARG;
		}
	}

	// Verify that the WebView2 runtime is installed.	
	PWSTR edgeVersionInfo = nullptr;
	HRESULT hr = ::GetAvailableCoreWebView2BrowserVersionString(nullptr, &edgeVersionInfo);
	if (FAILED(hr) || (edgeVersionInfo == nullptr))
	{
		LOG_TRACE << "The WebView2 runtime is not installed";
		LOG_TRACE << "Please install the WebView2 runtime before running this application available on https://go.microsoft.com/fwlink/p/?LinkId=2124703";
		return (hr);
	}
	LOG_TRACE << "Found installed WebView version=" << edgeVersionInfo;

	if (webView2Version.empty())
	{   // User did not provided specific WebView2 versions and channels.
		// Set WebView2 version and channel to default values. 
		std::wstring_view edgeVersionInfoStr = edgeVersionInfo;
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

		LOG_TRACE << "Using WebView2 version=" << webView2Version.data();
		LOG_TRACE << "Using WebView2 channel=" << webView2Channel.data();
	}
	profile.browserDirectory = WebView2::Utility::GetBrowserDirectory(webView2Version, webView2Channel, webViewFolder);
	profile.userDataDirectory = WebView2::Utility::GetUserDataDirectory(webView2Channel);
	profile.channel = webView2Channel.empty() ? L"stable release" : webView2Channel;	
	profile.version = webView2Version;
	profile.isTest = isTest;
	profile.port = webViewport;
	return (hr);
}
