#pragma once

#include "targetver.h"

// ============================================================================
// Windows SDK Headers - Core Functionality
// ============================================================================
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define BOOST_USE_WINAPI_VERSION	0x0601

// Windows core headers (must come first)
#include <dwmapi.h>                     // Desktop Window Manager API (used by osutility.cpp)
#include <wininet.h>                    // Used by Security\HttpsDownloader
#include <wincrypt.h>                   // Used by Security\ClientCertificateSelector

// ============================================================================
// C/C++ Standard Library Headers
// ============================================================================
#include <cstdlib>
#include <cstdio>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// C++ Standard Library - Strings & Containers
#include <string>
#include <string_view>
#include <map>
#include <unordered_set>

// C++ Standard Library - Filesystem & Utilities
#include <filesystem>
#include <functional>
#include <regex>                        // Used in various parts of the codebase
#include <chrono>                       // Used by telemetry and timing operations
#include <future>
#include <mutex>

// ============================================================================
// WTL/ATL Headers
// ============================================================================
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

// ============================================================================
// Third-Party Libraries
// ============================================================================
// JSON support (cpprest)
#include <cpprest/json.h>

// Windows Imaging Component (used by osutility.cpp for SVG rendering)
#include <wincodec.h>

// Direct2D (used by osutility.cpp for SVG rendering)
#include <d2d1_3.h>

// DirectComposition (used by CompositionHost)
#include <dcomp.h>

// WinRT headers (used by CompositionHost)
#include <DispatcherQueue.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <windows.ui.composition.interop.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.ViewManagement.h>

// Windows Implementation Library (WIL)
#include <wil/com.h>
#include <wil/resource.h>
#include <wil/result.h>
#include <wrl.h>

// ============================================================================
// OpenTelemetry Headers
// ============================================================================
// MUST undef U before OpenTelemetry:
// cpprest defines U(x) as L##x — this corrupts template parameter 'U'
// inside opentelemetry/nostd/span.h, turning U(*)[] into L(*)[]
#ifdef U
#undef U
#endif

#include <opentelemetry/logs/provider.h>
#include <opentelemetry/sdk/logs/logger_provider.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor.h>

// ============================================================================
// Namespace Aliases
// ============================================================================
namespace fs = std::filesystem;

// ============================================================================
// Application Constants
// ============================================================================
inline constexpr int TEXT_SIZE = 1024;
inline constexpr int ERR_WEBVIEW_NOT_INSTALLED = -1024;
inline constexpr int ERR_RESOURCE_NOT_FOUND = -1025;

// ============================================================================
// Common Controls Manifest Dependency
// ============================================================================
#if defined _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
