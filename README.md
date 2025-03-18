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

1. **Visual Studio 2022**: Ensure you have Visual Studio 2022 installed on your machine. You can download it from the [Visual Studio website](https://visualstudio.microsoft.com/).

## How to Build the Solution Using vcpkg

### Steps to Build

1. **Install vcpkg**: If you haven't already installed vcpkg, follow these steps:
    using command line, clone the vcpkg repository:
    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg 
 - Bootstrap vcpkg:
  ./bootstrap-vcpkg.bat

2. **Open the Solution**: Open the solution file (`WebView2.sln`) in Visual Studio 2022.

3. **Build the Solution**: Build the solution by selecting __Build > Build Solution__ from the menu or by pressing `Ctrl+Shift+B`.

4. **Run the Application**: After successfully building the solution, run the application by selecting __Debug > Start Debugging__ from the menu or by pressing `F5`.

### Additional Notes

- Ensure that the WebView2 runtime is installed on your machine. You can download it from the [Microsoft Edge WebView2 website](https://developer.microsoft.com/en-us/microsoft-edge/webview2/).
- If you encounter any issues during the build process, check the output window in Visual Studio for error messages and ensure that all dependencies are correctly installed and configured.

## Features

- Embedding WebView2: It shows how to embed a WebView2 control within a C++/WTL application window.
- Navigation: It demonstrates how to navigate to a URL and handle navigation events.
- Scripting: It includes examples of executing JavaScript in the WebView2 control and handling the results.
- Event Handling: It handles various WebView2 events such as navigation starting, navigation completed, and web message received.
- User Interface Integration: It integrates WebView2 with the WTL application's user interface, including resizing and layout management, Modal and Modeless integration.
- Edge Beta and Edge Dev Channel: It supports running the application with different versions of the Edge browser (Beta and Dev channels) for testing purposes.
   