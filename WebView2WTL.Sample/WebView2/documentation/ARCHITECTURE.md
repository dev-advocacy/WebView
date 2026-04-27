# WebView2WTL.Sample Architecture

This document describes the post-restructure layout of the `WebView2` project.

## Folder Layout

- `Core/`: Core browser host, view glue, and composition host.
- `Security/`: Authentication, cookie, and certificate related code.
- `UI/`: Frame and dialog UI implementation.
- `Messaging/`: Custom Windows message registration helpers.
- `Utilities/`: Cross-cutting helper classes and OS utilities.
- `Logger/`: Logging implementation internals.
- `Resources/`: Native resources (`.rc`, `resource.h`).
- `Old/`: Archived legacy headers/backups.
- Project root: Entry point (`WebView2Main.cpp`), PCH files, shared support sources.

## Module Responsibilities

### Core

`Core` contains the WebView object model and host-level behavior.

- `WebView2.*`
- `WebViewDlg.*`
- `SingleWebView2.*`
- `CompositionHost.*`
- `WebView2Impl2.h`

### Security

`Security` isolates authentication and cookie/certificate handling.

- `WebViewAuthentication.h`
- `Cookie.h`
- `ClientCertificate.h`
- `CertificateDlg.*`

### UI

`UI` groups user-facing windows and controls.

- `MainFrm.*`
- `UrlCombo.*`
- `AboutDlg.*`
- `DetectDlg.*`
- `DomainDlg.*`
- `WebRequestDlg.*`
- `LogView.*`
- `ImageListBox.h`
- `ColoredControls.h`

### Messaging

`Messaging` contains custom message registration.

- `RegisterMessages.*`

### Utilities

`Utilities` keeps shared support code used by multiple modules.

- `WebViewEvents.h`
- `WebViewProfile.*`
- `osutility.*`
- `Utility.*`
- `headersprop.h`
- `EdgeInfomation.h`
- `CommandLineParser.h`

### Logger

`Logger` currently hosts `logger_impl.cpp` while the public logger header remains in project root for compatibility.

## Build Notes

The project file now references moved files by relative subfolder path.

Additional include directories are configured per build configuration to include:

- Project root
- `Core`
- `Security`
- `UI`
- `Messaging`
- `Utilities`
- `Logger`
- `Resources`

This minimizes include churn while preserving existing include statements.
