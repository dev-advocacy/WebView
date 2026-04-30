#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define BOOST_USE_WINAPI_VERSION	0x0601
#include <dwmapi.h>

#include <cstdlib>
#include <cstdio>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>
#include <string_view>
#include <filesystem>
#include <map>
#include <functional>
#include <regex>
#include <chrono>
#include <future>
#include <mutex>
#include <unordered_set>

#include <atlbase.h>
#include <atlapp.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlw.h>
#include <atlsplit.h>
#include <atlmisc.h>
#include <atlctrlx.h>
#include <atlcrack.h>
#include <atlddx.h>
#include <atlimage.h>
#include <atlwin.h>

extern CAppModule _Module;

//json
#include <cpprest/json.h>
//
//
////Windows
#include <ppl.h>
#include <concurrent_unordered_map.h>
#include <d2d1_3.h>
#include <wininet.h>
#include <dcomp.h>
#include <wincodec.h>
#include <DispatcherQueue.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <windows.ui.composition.interop.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.ViewManagement.h>

//namespaces
namespace	fs = std::filesystem;

inline constexpr int TEXT_SIZE = 1024;
inline constexpr int ERR_WEBVIEW_NOT_INSTALLED = -1024;
inline constexpr int ERR_RESOURCE_NOT_FOUND = -1025;




#include <wil/com.h>
#include <wil/resource.h>
#include <wil/result.h>
#include <wrl.h>

// MUST undef U before OpenTelemetry:
// cpprest defines U(x) as L##x — this corrupts template parameter 'U'
// inside opentelemetry/nostd/span.h, turning U(*)[] into L(*)[]
#ifdef U
#undef U
#endif

#include <opentelemetry/logs/provider.h>
#include <opentelemetry/sdk/logs/logger_provider.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor.h>


#if defined _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
