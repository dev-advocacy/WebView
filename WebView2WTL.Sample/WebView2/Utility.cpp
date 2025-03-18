#include "pch.h"
#include "Utility.h"
#include "EdgeInfomation.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace WebView2
{
    // Use for case-insensitive wstring keys in std::map.
    struct WStringIgnoreCaseLess
    {
        bool operator()(std::wstring_view s1, std::wstring_view s2) const
        {

            return _wcsicmp(s1.data(), s2.data()) < 0;
        }
    };
    
    std::wstring GetDateTime()
    {
        time_t rawtime;
        struct tm timeinfo;
        wchar_t buffer[20];

        time(&rawtime);
        localtime_s(&timeinfo, &rawtime);

        wcsftime(buffer, 20, L"%Y_%m_%d_%H_%M_%S", &timeinfo);

        return buffer;
    }

    /// <summary>
    /// Return an new unique log file name
    /// </summary>
    /// <param name="path">the log path</param>
    /// <returns>error is it failed</returns>
    std::error_code  Utility::GetUniqueLogFileName(fs::path& pFileName)
    {
        std::wstring		pFileNameNoEx;
        wchar_t				buffer[MAX_PATH * sizeof(wchar_t)];
        wchar_t				wszPath[MAX_PATH * sizeof(wchar_t)];
        std::error_code		error;

        if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, wszPath)))
        {
            GetModuleFileName(NULL, buffer, MAX_PATH);
            std::wstring::size_type pos = std::wstring(buffer).find_last_of(_T("\\/"));
            std::wstring::size_type pos1 = std::wstring(buffer).find_last_of(_T("."));

            if (pos != 0 && pos1 != 0 && pos1 > pos)
            {
                pFileName = wszPath;
                pFileNameNoEx = L"Log_WebView2.log";

                pFileName.append(COMPFOLDER);

                if (!fs::is_directory(pFileName))
                {
                    if (!fs::create_directory(pFileName))
                    {
                        return(std::error_code(GetLastError(), std::system_category()));
                    }
                }
                pFileName.append(pFileNameNoEx);
            }
            else
            {
                error = std::error_code(ERROR_PATH_NOT_FOUND, std::system_category());
            }
        }
        else
        {
            error = std::error_code(ERROR_PATH_NOT_FOUND, std::system_category());
        }
        return error;
    }
    
    //static 
    std::wstring Utility::GetWebView2Version()
    {
        // See https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/distribution#detect-if-a-suitable-webview2-runtime-is-already-installed
        // Returns the version string when the WebView2 is installed 
        // or empty string if WebView2 is not installed.
        wil::unique_cotaskmem_string wv2Version;
        HRESULT hr = ::GetAvailableCoreWebView2BrowserVersionString(nullptr, &wv2Version);

        if SUCCEEDED(hr)
            return wv2Version.get();
        
        return {};
    }

    
    //static
    HRESULT Utility::DownloadWebView2Bootstrapper(std::wstring& bootstrapperPath)
    {
        // See https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/distribution#online-only-deployment
        // Downloads the WebView2 Bootstrapper from the web.
        // If the install is elevated, the WebView2 is installed system-wide.
        // Otherwise, it is installed per-user.

        // Fwlink available on https://developer.microsoft.com/microsoft-edge/webview2/
        std::wstring tempDirectory(MAX_PATH + 1, L'\0');
        DWORD length = ::GetTempPathW(static_cast<DWORD>(tempDirectory.size()), tempDirectory.data());
        if (length == 0)
            return HRESULT_FROM_WIN32(::GetLastError());
        
        tempDirectory.resize(length);
        bootstrapperPath = fs::path(tempDirectory) / L"MicrosoftEdgeWebview2Setup.exe";
        
        return URLDownloadToFileW(nullptr, L"https://go.microsoft.com/fwlink/p/?LinkId=2124703",
            bootstrapperPath.c_str(), 0, 0);
    }
    
    //static
    HRESULT Utility::InstallWebView2(const std::wstring& bootstrapperPath, bool elevated)
    {
        // Installs the WebView2 Bootstrapper synchronously.
        // If the install is elevated, the WebView2 is installed system-wide.
        // Otherwise, it is installed per-user.
        SHELLEXECUTEINFOW shellInfo = { static_cast<DWORD>(sizeof(shellInfo)) };
        shellInfo.fMask = SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS;
        shellInfo.lpVerb = elevated ? L"runas" : nullptr; 
        shellInfo.lpFile = bootstrapperPath.c_str();
        shellInfo.lpParameters = L" /silent /install";

        if (!::ShellExecuteExW(&shellInfo))
            return HRESULT_FROM_WIN32(::GetLastError()); // Install failed.

        if (shellInfo.hProcess != nullptr)
        {   // Wait for install to complete.
            ::WaitForSingleObject(shellInfo.hProcess, INFINITE);
            ::CloseHandle(shellInfo.hProcess);
        }

        return S_OK;
    }


    HRESULT Utility::InitCOM()
    {
        HRESULT hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if FAILED(hr)
        {
            ATLTRACE("Failed to initialize COM\n");
            ATLTRACE("function=%s, message=%s, hr=%d\n", __func__, std::system_category().message(hr).data(), hr);
        }
        return hr;
    }

    std::list<EdgeInfomation> Utility::EnumEdgeVersion()
    {
        std::list<EdgeInfomation> edge_versions;

        std::vector<std::wstring> keys = {
            L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Microsoft EdgeWebView",
            L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Microsoft Edge Beta",
            L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Microsoft Edge Dev",
        };
        for (const auto& key : keys)
        {
            auto edgeinfo = GetVersionFromRegistry(HKEY_LOCAL_MACHINE, key);
			if (!edgeinfo.m_name.empty())
			{
				edge_versions.push_back(edgeinfo);
			}
            edgeinfo = GetVersionFromRegistry(HKEY_CURRENT_USER, key);
            if (!edgeinfo.m_name.empty())
            {
                edge_versions.push_back(edgeinfo);
            }
        }
		return edge_versions;
    }
    EdgeInfomation Utility::GetVersionFromRegistry(HKEY key, std::wstring key_entry)
    {
        CRegKey regKey;        
        // Open the registry key
        if (regKey.Open(key, key_entry.c_str(), KEY_READ) == ERROR_SUCCESS)
        {
            // Read the version value
            DWORD dwSize = 0;
            regKey.QueryStringValue(L"DisplayVersion", nullptr, &dwSize);
            std::wstring version(dwSize, L'\0');
            regKey.QueryStringValue(L"DisplayVersion", &version[0], &dwSize);

            //get the display name
            dwSize = 0;
            regKey.QueryStringValue(L"DisplayName", nullptr, &dwSize);
            std::wstring display_name(dwSize, L'\0');
            regKey.QueryStringValue(L"DisplayName", &display_name[0], &dwSize);

            // Get the install location
            dwSize = 0;
            regKey.QueryStringValue(L"InstallLocation", nullptr, &dwSize);
            std::wstring install_location(dwSize, L'\0');
            regKey.QueryStringValue(L"InstallLocation", &install_location[0], &dwSize);

            EdgeInfomation edge(display_name, version, install_location);
            
            return edge;
        }
		else
		{
			ATLTRACE("Failed to open registry key: %ls\n", key_entry.c_str());
			return EdgeInfomation(L"", L"", L"");
		}
    }

    // Get directory containing the browser based on webView2Version and webView2Channel.
    // Returns empty string if webView2Version is empty.
    std::wstring Utility::GetBrowserDirectory(std::wstring_view webView2Version, std::wstring_view webView2Channel, std::wstring_view webViewFolder)
    {
        std::wstring browserDirectory = L"";
        std::wstring_view rootEdgeDirectory = GetRootEdgeDirectory(webView2Channel);

        if (!webView2Version.empty() && !rootEdgeDirectory.empty() && webViewFolder.empty())
        {   // Build directory from env var and channel.
            std::wstring programFilesx86Directory = GetProgramFilesx86Directory();
            std::wstring_view format = LR"(%s\Microsoft\%s\Application\%s)";

            size_t length = swprintf(nullptr, 0, format.data(), programFilesx86Directory.data(), rootEdgeDirectory.data(), webView2Version.data());
            browserDirectory.resize(length + 1);

            swprintf(browserDirectory.data(), browserDirectory.size(), format.data(), programFilesx86Directory.data(), rootEdgeDirectory.data(), webView2Version.data());
            browserDirectory.resize(length); // Remove trailing L'\0'.

            fs::path browserPath(browserDirectory);
            browserPath /= L"msedge.exe";
            if (!fs::exists(browserPath))
            {   // Return empty string if browser executable is not found.
                ATLTRACE("Incorrect browser path. File '%ls' not found\n", browserPath.c_str());
                return std::wstring();
            }
        }
        if (!webViewFolder.empty())
        {
            fs::path browserPath(webViewFolder);
            browserPath /= L"msedgewebview2.exe";
            if (!fs::exists(browserPath))
            {   // Return empty string if browser executable is not found.
                ATLTRACE("Incorrect browser path. File '%ls' not found\n", browserPath.c_str());
                return std::wstring();
            }
            browserDirectory = webViewFolder;
        }
        return browserDirectory;
    }

    // Returns Program Files directory x86.
    std::wstring Utility::GetProgramFilesx86Directory()
    {
        constexpr std::wstring_view programFilesVarName = L"ProgramFiles";
        constexpr std::wstring_view programFilesx86VarName = L"ProgramFiles(x86)";

        std::wstring programFilesx86Directory;
        size_t size = 0;

        if ((_wgetenv_s(&size, nullptr, 0, programFilesx86VarName.data()) == 0) && (size > 0))
        {   // Found env var %ProgramFiles(x86)%. Get its value.
            programFilesx86Directory.resize(size);
            _wgetenv_s(&size, programFilesx86Directory.data(), programFilesx86Directory.size(),
                programFilesx86VarName.data());
        }
        else
        {   // Assume x86 system. Try env var %ProgramFiles%.
            if ((_wgetenv_s(&size, nullptr, 0, programFilesVarName.data()) == 0) && (size > 0))
            {   // Found env var %ProgramFiles%. Get its value.
                programFilesx86Directory.resize(size);
                _wgetenv_s(&size, programFilesx86Directory.data(), programFilesx86Directory.size(),
                    programFilesVarName.data());
            }
            else
            {
                ATLTRACE("Failed to retrieve %%%ls%% and %%%ls%% environment variables\n",
                    programFilesx86VarName.data(), programFilesVarName.data());
            }
        }

        return programFilesx86Directory;
    }
    std::wstring Utility::GetLangStringFromLangId(DWORD dwLangID_i, bool returnShortCode)
    {
        const int MAX_LANG_LEN = 81;

        // Prepare LCID
        const LCID lcidLang = MAKELCID(dwLangID_i, SORT_DEFAULT);

        // Will hold language
        TCHAR szLangBuffer[MAX_LANG_LEN] = { 0 };

        LCTYPE resultFormat = (returnShortCode ? LOCALE_SISO639LANGNAME : LOCALE_SENGLANGUAGE);
        DWORD dwCount = GetLocaleInfo(lcidLang, resultFormat, szLangBuffer, MAX_LANG_LEN);
        if (!dwCount)
        {
            ATLTRACE(_T("Failed to get locale language information"));
            return _T("");
        }

        // Will hold country
        TCHAR szCountryBuffer[MAX_LANG_LEN] = { 0 };
        resultFormat = (returnShortCode ? LOCALE_SISO3166CTRYNAME : LOCALE_SENGCOUNTRY);

        // Get country
        dwCount = GetLocaleInfo(lcidLang, resultFormat, szCountryBuffer, MAX_LANG_LEN);

        if (!dwCount)
        {
            ATLTRACE(_T("Failed to get locale country information"));
            return szLangBuffer;
        }// End if

        std::wstring combinedResult(szLangBuffer);
        combinedResult += _T("-");
        combinedResult += szCountryBuffer;
        return combinedResult;
    }
    std::wstring Utility::GetUserMUI()
    {
        auto lang_id = GetUserDefaultUILanguage();
        return GetLangStringFromLangId(lang_id, true);
    }

    // Get root Edge directory based on the channel.
    // Returns empty string if unknown channel.
    // Get directory containing the user data based on webView2Version and webView2Channel.
    // Returns empty string if webView2Version is empty.
    std::wstring Utility::GetUserDataDirectory(std::wstring_view webView2Channel)
    {
        std::wstring userDirectory(MAX_PATH, L'\0');

        if FAILED(::SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, NULL, 0, userDirectory.data()))
        {   // Use current directory as default.
            userDirectory = L".";
        }

        std::wstring_view rootEdgeDirectory = GetRootEdgeDirectory(webView2Channel);

        if (rootEdgeDirectory.empty())
        {   // Use stable channel as default. 
            rootEdgeDirectory = GetRootEdgeDirectory(L"");
        }

        std::wstring_view format = LR"(%s\Microsoft\%s\User Data)";
        size_t length = swprintf(nullptr, 0, format.data(),
            userDirectory.data(), rootEdgeDirectory.data());
        std::wstring userDataDirectory(length + 1, L'\0');

        swprintf(userDataDirectory.data(), userDataDirectory.size(), format.data(),
            userDirectory.data(), rootEdgeDirectory.data());
        userDataDirectory.resize(length);

        return userDataDirectory;
    }

    std::wstring_view Utility::GetRootEdgeDirectory(std::wstring_view webView2Channel)
    {
        static std::map<std::wstring_view, std::wstring_view, WStringIgnoreCaseLess> channels =
        {   // Name of Edge directory based on release channel.
            { L"",          L"EdgeWebView" },
            { L"beta",      L"Edge Beta" },
            { L"dev",       L"Edge Dev" },
            { L"canary",    L"Edge Canary" },
            { L"Fixed",     L"Fixed" },
        };

        auto pos = channels.find(webView2Channel);

        if (pos == channels.end())
        {   // Invalid channel value.
            ATLTRACE("Incorrect channel value: \"%ls\". Allowed values:\n", webView2Channel.data());
            for (const auto& channelEntry : channels)
            {
                ATLTRACE("\"%ls\"\n", channelEntry.first.data());
            }
            return L"";
        }
        return pos->second;
    }

    std::wstring Utility::BoolToString(BOOL value)
    {
        return value ? L"true" : L"false";
    }
    std::wstring Utility::EncodeQuote(std::wstring raw)
    {
        return L"\"" + regex_replace(raw, std::wregex(L"\""), L"\\\"") + L"\"";
    }
    std::wstring Utility::SecondsToString(UINT32 time)
    {
        WCHAR rawResult[26];
        time_t rawTime;
        rawTime = (const time_t)time;
        struct tm timeStruct;
        gmtime_s(&timeStruct, &rawTime);
        _wasctime_s(rawResult, 26, &timeStruct);
        std::wstring result(rawResult);
        return result;
    }



    std::wstring Utility::CookieToString(ICoreWebView2Cookie* cookie)
    {
        //! [CookieObject]
        wil::unique_cotaskmem_string name;
        THROW_IF_FAILED(cookie->get_Name(&name));
        wil::unique_cotaskmem_string value;
        THROW_IF_FAILED(cookie->get_Value(&value));
        wil::unique_cotaskmem_string domain;
        THROW_IF_FAILED(cookie->get_Domain(&domain));
        wil::unique_cotaskmem_string path;
        THROW_IF_FAILED(cookie->get_Path(&path));
        double expires;
        THROW_IF_FAILED(cookie->get_Expires(&expires));
        BOOL isHttpOnly = FALSE;
        THROW_IF_FAILED(cookie->get_IsHttpOnly(&isHttpOnly));
        COREWEBVIEW2_COOKIE_SAME_SITE_KIND same_site;
        std::wstring same_site_as_string;
        THROW_IF_FAILED(cookie->get_SameSite(&same_site));
        switch (same_site)
        {
        case COREWEBVIEW2_COOKIE_SAME_SITE_KIND_NONE:
            same_site_as_string = L"None";
            break;
        case COREWEBVIEW2_COOKIE_SAME_SITE_KIND_LAX:
            same_site_as_string = L"Lax";
            break;
        case COREWEBVIEW2_COOKIE_SAME_SITE_KIND_STRICT:
            same_site_as_string = L"Strict";
            break;
        }
        BOOL isSecure = FALSE;
        THROW_IF_FAILED(cookie->get_IsSecure(&isSecure));
        BOOL isSession = FALSE;
        THROW_IF_FAILED(cookie->get_IsSession(&isSession));

        std::wstring result = L"{";
        result += L"\"Name\": " + EncodeQuote(name.get()) + L", " + L"\"Value\": " +
            EncodeQuote(value.get()) + L", " + L"\"Domain\": " + EncodeQuote(domain.get()) +
            L", " + L"\"Path\": " + EncodeQuote(path.get()) + L", " + L"\"HttpOnly\": " +
            BoolToString(isHttpOnly) + L", " + L"\"Secure\": " + BoolToString(isSecure) + L", " +
            L"\"SameSite\": " + EncodeQuote(same_site_as_string) + L", " + L"\"Expires\": ";
        if (!isSession)
        {
            result += L"This is a session cookie.";
        }
        else
        {
            result += std::to_wstring(expires);
        }

        return result + L"\"}";
    }
}