# WebView
WebView2 Sample


## Overview
This project demonstrates how to use WebView2 in a C++ application.

## Dependencies

The solution relies on the following dependencies:

1. **WebView2 SDK**: The WebView2 SDK is required to embed and interact with the WebView2 control in the application. The SDK provides the necessary APIs to create and manage WebView2 instances.
2. **WTL (Windows Template Library)**: WTL is used for creating and managing the application's user interface components, such as dialogs and controls.
3. **Boost Logging Library**: A custom logging library is used to log messages and trace the execution of the application. This helps in debugging and monitoring the application's behavior.

### Prerequisites

1. **Visual Studio 2026** (with the **v145** platform toolset): The `WebView2` project targets the **v145** toolset, which ships with Visual Studio 2026. The `WebView2Logger` project targets the **v143** toolset (Visual Studio 2022). Make sure both toolsets and the **Desktop development with C++** workload (MSVC + Windows 10/11 SDK) are installed. You can download Visual Studio from the [Visual Studio website](https://visualstudio.microsoft.com/).
2. **Git submodules**: This repository uses git submodules (a stripped `cpprestsdk` fork under `third_party/cpprestsdk`, which itself pulls `websocketpp`). They **must** be initialized, otherwise headers such as `cpprest/json.h` will not be found (compiler error `C1083`).
3. **Internet access**: Required on the first build so vcpkg can download/build the manifest dependencies (`webview2`, `wtl`, `wil`, `opentelemetry-cpp`).

## How to Build the Solution Using vcpkg

### Steps to Build

1. **Clone the repository with its submodules**:

   ```
   git clone --recursive https://github.com/dev-advocacy/WebView.git
   ```

   If you already cloned without `--recursive`, initialize the submodules afterwards:

   ```
   git submodule update --init --recursive
   ```

2. **Install vcpkg** (optional): Visual Studio 2026 ships with a bundled, integrated vcpkg, so this step is usually not required. If you want a standalone vcpkg:

   ```
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   ./bootstrap-vcpkg.bat
   ```

   The dependency versions are pinned via the `builtin-baseline` in `WebView2WTL.Sample/WebView2/vcpkg.json`, so the restore is deterministic.

3. **Open the Solution**: Open the solution file (`WebView2WTL.Sample/WebViewSolution.sln`) in **Visual Studio 2026**. Launch Visual Studio normally (from the Start menu), **not** from a Developer Command Prompt targeting x86, to avoid a stale build environment that can break vcpkg's compiler detection.

4. **Build the Solution**: Build the solution by selecting __Build > Build Solution__ from the menu or by pressing `Ctrl+Shift+B`. vcpkg restores the manifest dependencies automatically on the first build.

5. **Run the Application**: After successfully building the solution, run the application by selecting __Debug > Start Debugging__ from the menu or by pressing `F5`.

### Additional Notes

- Ensure that the WebView2 runtime is installed on your machine. You can download it from the [Microsoft Edge WebView2 website](https://developer.microsoft.com/en-us/microsoft-edge/webview2/).
- If you get `error C1083: Cannot open include file: 'cpprest/json.h'`, the git submodules are not initialized. Run `git submodule update --init --recursive` and rebuild.
- If you encounter any issues during the build process, check the output window in Visual Studio for error messages and ensure that all dependencies are correctly installed and configured.

## Features

- Embedding WebView2: It shows how to embed a WebView2 control within a C++/WTL application window.
- Navigation: It demonstrates how to navigate to a URL and handle navigation events.
- Scripting: It includes examples of executing JavaScript in the WebView2 control and handling the results.
- Event Handling: It handles various WebView2 events such as navigation starting, navigation completed, and web message received.
- User Interface Integration: It integrates WebView2 with the WTL application's user interface, including resizing and layout management, Modal and Modeless integration.
- Edge Beta and Edge Dev Channel: It supports running the application with different versions of the Edge browser (Beta and Dev channels) for testing purposes.
   