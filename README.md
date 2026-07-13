# WebView2 Sample with WinInet Certificate Pre-Selection

## Overview

This project demonstrates advanced WebView2 integration in a C++20 application with **intelligent client certificate authentication**. It features a unique **WinInet certificate pre-selection** mechanism that seamlessly injects client certificates into WebView2, eliminating redundant certificate prompts.

## 🎯 Key Features

### Client Certificate Authentication
- **WinInet Pre-Selection**: Pre-select client certificates using WinInet before WebView2 creation
- **Smart EKU Filtering**: Automatically filters certificates by Enhanced Key Usage (Client Authentication only)
- **PEM-Based Matching**: Reliable certificate matching using normalized PEM encoding comparison
- **Zero-Prompt Experience**: Seamlessly inject pre-selected certificates without user interaction
- **Fallback Support**: Graceful fallback to custom or native dialogs when injection fails

### WebView2 Integration
- **Embedding**: Full WebView2 control integration within C++/WTL application windows
- **Navigation**: Complete URL navigation with event handling
- **JavaScript Execution**: Execute scripts and handle results from WebView2
- **Event Handling**: Comprehensive event management (navigation, messages, certificate requests)
- **UI Integration**: Advanced layout management, modal/modeless dialogs, resizing
- **Multi-Channel Support**: Compatible with Edge Beta, Dev, and Stable channels

## 🏗️ Architecture

### Modular Security System

The certificate injection system is organized into well-documented modules:

| Module | Purpose | Documentation |
|--------|---------|---------------|
| `Security/WinInetHelpers.h` | RAII wrappers, exception types, shared structures | ✅ XML comments |
| `Security/ClientCertificateSelector.h/.cpp` | Certificate filtering and EKU validation | ✅ XML comments |
| `Security/HttpsDownloader.h/.cpp` | WinInet HTTPS client with certificate selection | ✅ XML comments |
| `Security/WinInetCertPreSelector.h/.cpp` | Thread-safe singleton for certificate storage | ✅ XML comments |

### Integration Points

| File | Purpose |
|------|---------|
| `UI/MainFrm.cpp` | Triggers WinInet pre-selection before WebView2 creation |
| `Utilities/WebViewEvents.h` | Handles `ClientCertificateRequested` event and injection logic |
| `Utilities/Utility.h/.cpp` | PEM normalization utilities for reliable matching |

## 🚀 Quick Start

### Prerequisites

1. **Visual Studio 2022** or later
2. **WebView2 Runtime** ([Download](https://developer.microsoft.com/en-us/microsoft-edge/webview2/))
3. **vcpkg** package manager

### Build Steps

1. **Install vcpkg**:
   ```bash
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   ./bootstrap-vcpkg.bat
   ```

2. **Open Solution**: Open `WebView2.sln` in Visual Studio 2022

3. **Build**: Press `Ctrl+Shift+B` or select **Build > Build Solution**

4. **Run**: Press `F5` or select **Debug > Start Debugging**

### Using Certificate Pre-Selection

Launch with a specific URL to trigger WinInet pre-selection:

```bash
.\WebView2.exe --url https://your-server.com
```

**Flow**:
1. WinInet requests the URL and shows filtered certificate dialog
2. Select your client certificate (only Client Authentication EKU shown)
3. Certificate is stored in singleton by endpoint (host:port)
4. WebView2 launches and navigates to the same URL
5. Server requests client certificate → **Auto-injected** from pre-selection ✅
6. No second prompt appears!

Enable/disable the feature via menu: **Scenario > WinInet Pre-Select Certificate**

## 📚 Documentation

| Document | Description |
|----------|-------------|
| [`CERTIFICATE_INJECTION.md`](CERTIFICATE_INJECTION.md) | Complete technical documentation of the certificate injection flow |
| [`SECURITY_AUDIT_REPORT.md`](SECURITY_AUDIT_REPORT.md) | Security audit report and best practices |
| [`PRECOMPILED_HEADERS_OPTIMIZATION.md`](PRECOMPILED_HEADERS_OPTIMIZATION.md) | PCH optimization notes |

## 🔧 Dependencies

### Core Dependencies

1. **WebView2 SDK**: Microsoft Edge WebView2 control and APIs
2. **WTL (Windows Template Library)**: UI components, dialogs, and controls
3. **WIL (Windows Implementation Library)**: Modern C++ Windows API wrappers
4. **vcpkg**: Package management and dependency resolution

### Additional Libraries

- **Boost**: Logging and utility libraries
- **cpprestsdk**: JSON parsing and REST client
- **OpenTelemetry**: Observability and tracing
- **Direct2D/DirectComposition**: Graphics and composition
- **WinInet**: Client certificate pre-selection

## 🎓 Advanced Features

### Smart Certificate Filtering

Only certificates with **Client Authentication** Extended Key Usage (OID `1.3.6.1.5.5.7.3.2`) are presented:

```cpp
/// Check if certificate has Client Authentication EKU
bool HasClientAuthEKU(PCCERT_CONTEXT ctx);

/// Filter certificates by EKU and optional subject
std::vector<UniqueCertContext> SelectClientAuthCertificates(
    HINTERNET request = nullptr,
    const std::wstring& subjectFilter = L"");
```

### PEM-Based Matching

Reliable certificate matching using normalized PEM encoding:

```cpp
// Normalize PEM: remove headers, whitespace, newlines
std::wstring wantedPemNormalized = Utility::NormalizePem(wantedPem);
std::wstring candidatePemNormalized = Utility::NormalizePem(candidatePem);

// Compare actual certificate binary data
if (wantedPemNormalized == candidatePemNormalized) {
    // Match found! ✅
}
```

**Why PEM?** Subject strings vary between WinInet and WebView2 (UPN vs CN vs GUID), but PEM encoding is the actual certificate binary—guaranteed to match.

## 📖 API Reference

### WinInetCertPreSelector

```cpp
// Access singleton instance
auto& preSel = WinInetCertPreSelector::Instance();

// Pre-select certificate before WebView2 creation
std::string body = preSel.Run(L"https://server.com");

// Check if certificate exists for endpoint
bool hasMatch = preSel.HasMatchFor(L"server.com", 443);

// Retrieve stored certificate data
std::wstring pem = preSel.GetPemEncodingFor(L"server.com", 443);
std::wstring subject = preSel.GetSubjectFor(L"server.com", 443);
PCCERT_CONTEXT ctx = preSel.GetCertContextFor(L"server.com", 443);

// Enable/disable feature
preSel.SetEnabled(true);
bool enabled = preSel.IsEnabled();
```

## 🔒 Security

- ✅ No hardcoded secrets or credentials
- ✅ No client/customer-specific data in code
- ✅ Certificate contexts properly duplicated and freed
- ✅ RAII wrappers prevent resource leaks
- ✅ Thread-safe singleton implementation

See [`SECURITY_AUDIT_REPORT.md`](SECURITY_AUDIT_REPORT.md) for detailed security analysis.

## 🔗 References

- [WebView2 Documentation](https://docs.microsoft.com/en-us/microsoft-edge/webview2/)
- [WTL Documentation](https://sourceforge.net/projects/wtl/)
- [WinInet Certificate Selection](https://docs.microsoft.com/en-us/windows/win32/wininet/certificate-security)
- [Client Certificate Authentication RFC](https://datatracker.ietf.org/doc/html/rfc8446#section-4.4.2)
   