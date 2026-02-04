# Final LOG Conversion Instructions

## Status
? Boost.Program_Options - **REMOVED** (replaced with CommandLineParser.h)
??  Stream-style LOG calls - **NEEDS MANUAL CONVERSION** (file locked by Visual Studio)

## Files Remaining to Convert

### 1. **WebView2Impl2.h** - ~13 stream-style LOG calls
### 2. **WebView2Impl.h** - ~10 stream-style LOG calls (some already converted by backup script)

---

## **SIMPLE FIND/REPLACE (Use Visual Studio Find & Replace with Regex)**

### Step 1: Close all build error windows in Visual Studio

### Step 2: Use Find & Replace (Ctrl+H) with these patterns:

**Pattern 1: Simple __FUNCTION__ calls**
```
Find:    (LOG_TRACE|LOG_DEBUG|LOG_ERROR)\s*<<\s*__FUNCTION__\s*;
Replace: $1(__FUNCTION__);
Options: ? Use Regular Expressions
```

**Pattern 2: Simple string literals**
```
Find:    (LOG_TRACE|LOG_DEBUG|LOG_ERROR)\s*<<\s*"([^"]+)"\s*;
Replace: $1("$2");
Options: ? Use Regular Expressions
```

---

## **MANUAL CONVERSIONS NEEDED** (Complex multi-part LOG statements)

### WebView2Impl2.h

**Line 466:**
```cpp
// OLD:
LOG_TRACE << s;

// NEW:
LOG_TRACE(WideToNarrow(s));
```

**Line 518-520:**
```cpp
// OLD:
LOG_TRACE << __FUNCTION__;
LOG_TRACE << L"  success=" << isSuccess << L", ID=" << navigationId
          << L", error status=" << errorStatus;

// NEW:
LOG_TRACE(__FUNCTION__);
LOG_TRACE(std::string("  success=") + std::to_string(isSuccess) + ", ID=" + std::to_string(navigationId) +
          ", error status=" + std::to_string(errorStatus));
```

**Line 534-535:**
```cpp
// OLD:
LOG_TRACE << __FUNCTION__;
LOG_TRACE << L"  method=" << method << L", uri=" << uri;

// NEW:
LOG_TRACE(__FUNCTION__);
LOG_TRACE(std::string("  method=") + WideToNarrow(method) + ", uri=" + WideToNarrow(uri));
```

**Line 540-542:**
```cpp
// OLD:
LOG_TRACE << __FUNCTION__;
LOG_TRACE << L"  method=" << method << L", uri=" << uri
          << L", resource context=" << resourceContext;

// NEW:
LOG_TRACE(__FUNCTION__);
LOG_TRACE(std::string("  method=") + WideToNarrow(method) + ", uri=" + WideToNarrow(uri) +
          ", resource context=" + std::to_string(resourceContext));
```

**Line 546, 550, 578:**
```cpp
// OLD:
LOG_TRACE << __FUNCTION__;

// NEW:
LOG_TRACE(__FUNCTION__);
```

**Line 617:**
```cpp
// OLD:
LOG_TRACE << __FUNCTION__ << " Using browser directory:" << m_browser_directory.data();

// NEW:
LOG_TRACE(std::string(__FUNCTION__) + " Using browser directory:" + WideToNarrow(m_browser_directory));
```

**Line 625:**
```cpp
// OLD:
LOG_DEBUG << "Start the WebView2 process with the Chrome DevTools Protocol enabled which allows the automation by Playwright. Port=" << m_port;

// NEW:
LOG_DEBUG(std::string("Start the WebView2 process with the Chrome DevTools Protocol enabled which allows the automation by Playwright. Port=") + WideToNarrow(m_port));
```

**Line 633:**
```cpp
// OLD:
LOG_DEBUG << "Create unique log file for log-net-log filename: " << unique_file;

// NEW:
LOG_DEBUG(std::string("Create unique log file for log-net-log filename: ") + unique_file.string());
```

**Line 639:**
```cpp
// OLD:
LOG_ERROR << "Failed to create unique log file name for log-net-log";

// NEW:
LOG_ERROR("Failed to create unique log file name for log-net-log");
```

---

### WebView2Impl.h

**Line 117, 124, 131, 139:**
```cpp
// OLD:
LOG_TRACE << "literalString";

// NEW:
LOG_TRACE("literalString");
```

**Line 156, 211, 226, 249, 253, 257, 261:**
```cpp
// OLD:
LOG_TRACE << __FUNCTION__;

// NEW:
LOG_TRACE(__FUNCTION__);
```

**Line 295:**
```cpp
// OLD:
LOG_TRACE << s;

// NEW:
LOG_TRACE(WideToNarrow(s));
```

**Line 305:**
```cpp
// OLD:
LOG_TRACE << __FUNCTION__ << L" domain=" << domain << L" name=" << name << L" value=" << value;

// NEW:
LOG_TRACE(std::string(__FUNCTION__) + " domain=" + WideToNarrow(domain) + " name=" + WideToNarrow(name) + " value=" + WideToNarrow(value));
```

**Line 323:**
```cpp
// OLD:
LOG_TRACE << result;

// NEW:
LOG_TRACE(WideToNarrow(result));
```

**Line 374:**
```cpp
// OLD:
LOG_TRACE << "Hwnd=" << pT->m_hWnd << " caption=" << std::wstring(t);

// NEW:
LOG_TRACE(std::string("Hwnd=") + std::to_string(reinterpret_cast<uintptr_t>(pT->m_hWnd)) + " caption=" + WideToNarrow(std::wstring(t)));
```

**Line 384:**
```cpp
// OLD:
LOG_TRACE << __FUNCTION__ << " Using user data directory:" << userDataDirectory_.data();

// NEW:
LOG_TRACE(std::string(__FUNCTION__) + " Using user data directory:" + WideToNarrow(userDataDirectory_));
```

**Line 392:**
```cpp
// OLD:
LOG_TRACE << __FUNCTION__ << " Unable to release webview2";

// NEW:
LOG_TRACE(std::string(__FUNCTION__) + " Unable to release webview2");
```

**Line 459, 475, 485, 501, 528, 542:**
```cpp
// OLD:
LOG_TRACE << __FUNCTION__;

// NEW:
LOG_TRACE(__FUNCTION__);
```

**Line 570:**
```cpp
// OLD:
LOG_TRACE << __FUNCTION__ << " width=" << rc.Width() << " height=" << rc.Height() << " visibility=" << isVisible;

// NEW:
LOG_TRACE(std::string(__FUNCTION__) + " width=" + std::to_string(rc.Width()) + " height=" + std::to_string(rc.Height()) + " visibility=" + std::to_string(isVisible));
```

**Line 575:**
```cpp
// OLD:
LOG_TRACE << __FUNCTION__ << " hr=" << hr;

// NEW:
LOG_TRACE(std::string(__FUNCTION__) + " hr=" + std::to_string(hr));
```

**Line 588:**
```cpp
// OLD:
LOG_TRACE << "function=" << __func__  << "COREWEBVIEW2_WEB_ERROR_STATUS_DISCONNECTED";

// NEW:
LOG_TRACE(std::string("function=") + __func__ + "COREWEBVIEW2_WEB_ERROR_STATUS_DISCONNECTED");
```

**Line 648:**
```cpp
// OLD:
LOG_TRACE << __FUNCTION__ << " name=Authorization" << " value=" << authV;

// NEW:
LOG_TRACE(std::string(__FUNCTION__) + " name=Authorization value=" + WideToNarrow(authV));
```

---

## **QUICK SUMMARY - DO THIS IN ORDER:**

1. **Save and close all files in Visual Studio**
2. **Run Find/Replace with Regex** (2 patterns above)
3. **Manually fix remaining complex statements** (use list above)
4. **Rebuild** to verify all errors are gone

---

## Alternative: Use the PowerShell Script

If you prefer automation:

1. **Close Visual Studio completely**
2. Run: `powershell -ExecutionPolicy Bypass -File ConvertAllLogCalls.ps1`
3. **Reopen Visual Studio** and rebuild

---

## Verification

After conversion, you should have **ZERO** LOG-related errors. All LOG calls will use:
- `LOG_TRACE(__FUNCTION__);` instead of `LOG_TRACE << __FUNCTION__;`
- `LOG_TRACE(string_expression);` instead of `LOG_TRACE << expr1 << expr2;`

The new logger.h requires function-call syntax with a single `std::string` or `std::string_view` parameter.
