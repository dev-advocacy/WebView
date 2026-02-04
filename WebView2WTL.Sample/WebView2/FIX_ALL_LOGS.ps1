# COMPREHENSIVE LOG CONVERSION SCRIPT - FIXES ALL REMAINING ERRORS
# This script converts ALL stream-style LOG calls to function-call style

$ErrorActionPreference = "Stop"

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "COMPREHENSIVE LOG CONVERSION SCRIPT" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

$files = @(
    @{Path="WebView2Impl2.h"; Name="WebView2Impl2.h"},
    @{Path="WebView2Impl.h"; Name="WebView2Impl.h"},
    @{Path="MainFrm.cpp"; Name="MainFrm.cpp"}
)

foreach ($fileInfo in $files) {
    $filePath = $fileInfo.Path
    
    if (-not (Test-Path $filePath)) {
        Write-Host "File not found: $filePath" -ForegroundColor Yellow
        continue
    }
    
    Write-Host "Processing: $($fileInfo.Name)" -ForegroundColor Green
    
    $content = Get-Content $filePath -Raw -Encoding UTF8
    $originalContent = $content
    $changeCount = 0
    
    # ====================================================================================
    # PHASE 1: SIMPLE CONVERSIONS
    # ====================================================================================
    
    # Pattern 1: LOG_XXX << __FUNCTION__;
    $pattern1 = '(LOG_TRACE|LOG_DEBUG|LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_FATAL)\s*<<\s*__FUNCTION__\s*;'
    if ($content -match $pattern1) {
        $content = $content -replace $pattern1, '$1(__FUNCTION__);'
        $changeCount++
        Write-Host "  ? Converted LOG_XXX << __FUNCTION__;" -ForegroundColor Gray
    }
    
    # Pattern 2: LOG_XXX << "literal";
    $pattern2 = '(LOG_TRACE|LOG_DEBUG|LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_FATAL)\s*<<\s*"([^"]+)"\s*;'
    if ($content -match $pattern2) {
        $content = $content -replace $pattern2, '$1("$2");'
        $changeCount++
        Write-Host "  ? Converted LOG_XXX << ""literal"";" -ForegroundColor Gray
    }
    
    # ====================================================================================
    # PHASE 2: COMPLEX MULTI-PART CONVERSIONS (WebView2Impl2.h specific)
    # ====================================================================================
    
    # Line 617: LOG_TRACE << __FUNCTION__ << " Using browser directory:" << m_browser_directory.data();
    $old617 = 'LOG_TRACE\s*<<\s*__FUNCTION__\s*<<\s*"\s*Using browser directory:"\s*<<\s*m_browser_directory\.data\(\)\s*;'
    $new617 = 'LOG_TRACE(std::string(__FUNCTION__) + " Using browser directory:" + WideToNarrow(m_browser_directory));'
    if ($content -match $old617) {
        $content = $content -replace $old617, $new617
        $changeCount++
        Write-Host "  ? Converted line 617 (browser directory)" -ForegroundColor Gray
    }
    
    # Line 625: LOG_DEBUG << "Start the WebView2..." << m_port;
    $old625 = 'LOG_DEBUG\s*<<\s*"Start the WebView2 process with the Chrome DevTools Protocol enabled which allows the automation by Playwright\. Port="\s*<<\s*m_port\s*;'
    $new625 = 'LOG_DEBUG(std::string("Start the WebView2 process with the Chrome DevTools Protocol enabled which allows the automation by Playwright. Port=") + WideToNarrow(m_port));'
    if ($content -match $old625) {
        $content = $content -replace $old625, $new625
        $changeCount++
        Write-Host "  ? Converted line 625 (DevTools port)" -ForegroundColor Gray
    }
    
    # Line 633: LOG_DEBUG << "Create unique log file..." << unique_file;
    $old633 = 'LOG_DEBUG\s*<<\s*"Create unique log file for log-net-log filename:\s*"\s*<<\s*unique_file\s*;'
    $new633 = 'LOG_DEBUG(std::string("Create unique log file for log-net-log filename: ") + unique_file.string());'
    if ($content -match $old633) {
        $content = $content -replace $old633, $new633
        $changeCount++
        Write-Host "  ? Converted line 633 (unique log file)" -ForegroundColor Gray
    }
    
    # Line 639: LOG_ERROR << "Failed to create unique log file name for log-net-log";
    $old639 = 'LOG_ERROR\s*<<\s*"Failed to create unique log file name for log-net-log"\s*;'
    $new639 = 'LOG_ERROR("Failed to create unique log file name for log-net-log");'
    if ($content -match $old639) {
        $content = $content -replace $old639, $new639
        $changeCount++
        Write-Host "  ? Converted line 639 (error message)" -ForegroundColor Gray
    }
    
    # Line 466: LOG_TRACE << s;  (where s is a wstring)
    $old466 = '(\s+)LOG_TRACE\s*<<\s*s\s*;'
    $new466 = '$1LOG_TRACE(WideToNarrow(s));'
    if ($content -match $old466) {
        $content = $content -replace $old466, $new466
        $changeCount++
        Write-Host "  ? Converted line 466 (wstring s)" -ForegroundColor Gray
    }
    
    # Lines 518-520: Multi-line navigation complete
    $old518 = 'LOG_TRACE\s*<<\s*__FUNCTION__\s*;\s*\n\s*LOG_TRACE\s*<<\s*L"  success="\s*<<\s*isSuccess\s*<<\s*L", ID="\s*<<\s*navigationId\s*\n\s*<<\s*L", error status="\s*<<\s*errorStatus\s*;'
    $new518 = "LOG_TRACE(__FUNCTION__);`n`t`tLOG_TRACE(std::string(`"  success=`") + std::to_string(isSuccess) + `", ID=`" + std::to_string(navigationId) +`n`t`t          `", error status=`" + std::to_string(errorStatus));"
    if ($content -match $old518) {
        $content = $content -replace $old518, $new518
        $changeCount++
        Write-Host "  ? Converted lines 518-520 (navigation complete)" -ForegroundColor Gray
    }
    
    # Lines 534-535: Navigation starting
    $old534 = 'LOG_TRACE\s*<<\s*__FUNCTION__\s*;\s*\n\s*LOG_TRACE\s*<<\s*L"  uri="\s*<<\s*uri\s*<<\s*L", ID="\s*<<\s*navigationId\s*<<\s*\n\s*L", redirected="\s*<<\s*isRedirected\s*<<\s*L", user initiated="\s*<<\s*isUserInitiated\s*;'
    $new534 = "LOG_TRACE(__FUNCTION__);`n`t`tLOG_TRACE(std::string(`"  uri=`") + WideToNarrow(std::wstring(uri)) + `", ID=`" + std::to_string(navigationId) +`n`t`t          `", redirected=`" + std::to_string(isRedirected) + `", user initiated=`" + std::to_string(isUserInitiated));"
    if ($content -match $old534) {
        $content = $content -replace $old534, $new534
        $changeCount++
        Write-Host "  ? Converted lines 534-535 (navigation starting)" -ForegroundColor Gray
    }
    
    # Lines 540-542: Response received
    $old540 = 'LOG_TRACE\s*<<\s*__FUNCTION__\s*;\s*\n\s*LOG_TRACE\s*<<\s*L"  method="\s*<<\s*method\s*<<\s*L", uri="\s*<<\s*uri\s*;'
    $new540 = "LOG_TRACE(__FUNCTION__);`n`t`tLOG_TRACE(std::string(`"  method=`") + WideToNarrow(method) + `", uri=`" + WideToNarrow(uri));"
    if ($content -match $old540) {
        $content = $content -replace $old540, $new540
        $changeCount++
        Write-Host "  ? Converted lines 540-541 (response received)" -ForegroundColor Gray
    }
    
    # Lines 546-548: Request event
    $old546 = 'LOG_TRACE\s*<<\s*__FUNCTION__\s*;\s*\n\s*LOG_TRACE\s*<<\s*L"  method="\s*<<\s*method\s*<<\s*L", uri="\s*<<\s*uri\s*\n\s*<<\s*L", resource context="\s*<<\s*resourceContext\s*;'
    $new546 = "LOG_TRACE(__FUNCTION__);`n`t`tLOG_TRACE(std::string(`"  method=`") + WideToNarrow(method) + `", uri=`" + WideToNarrow(uri) +`n`t`t          `", resource context=`" + std::to_string(resourceContext));"
    if ($content -match $old546) {
        $content = $content -replace $old546, $new546
        $changeCount++
        Write-Host "  ? Converted lines 546-548 (request event)" -ForegroundColor Gray
    }
    
    # ====================================================================================
    # PHASE 3: WebView2Impl.h SPECIFIC CONVERSIONS
    # ====================================================================================
    
    # Line 131: LOG_TRACE << "NavigationStartingCallback" << L" URI=" << uri;
    $old131 = 'LOG_TRACE\s*<<\s*"NavigationStartingCallback"\s*<<\s*L"\s*URI="\s*<<\s*uri\s*;'
    $new131 = 'LOG_TRACE(std::string("NavigationStartingCallback URI=") + WideToNarrow(uri));'
    if ($content -match $old131) {
        $content = $content -replace $old131, $new131
        $changeCount++
        Write-Host "  ? Converted line 131 (navigation starting callback)" -ForegroundColor Gray
    }
    
    # Line 139: LOG_TRACE << "NavigationCompletedCallback" << L" URI=" << uri;
    $old139 = 'LOG_TRACE\s*<<\s*"NavigationCompletedCallback"\s*<<\s*L"\s*URI="\s*<<\s*uri\s*;'
    $new139 = 'LOG_TRACE(std::string("NavigationCompletedCallback URI=") + WideToNarrow(uri));'
    if ($content -match $old139) {
        $content = $content -replace $old139, $new139
        $changeCount++
        Write-Host "  ? Converted line 139 (navigation completed callback)" -ForegroundColor Gray
    }
    
    # Line 253: LOG_TRACE << __FUNCTION__ << L" URI=" << url;
    $old253 = 'LOG_TRACE\s*<<\s*__FUNCTION__\s*<<\s*L"\s*URI="\s*<<\s*url\s*;'
    $new253 = 'LOG_TRACE(std::string(__FUNCTION__) + " URI=" + WideToNarrow(url));'
    if ($content -match $old253) {
        $content = $content -replace $old253, $new253
        $changeCount++
        Write-Host "  ? Converted line 253 (URI)" -ForegroundColor Gray
    }
    
    # Line 295: LOG_TRACE << s;
    $old295 = '(\s+)LOG_TRACE\s*<<\s*s\s*;'
    $new295 = '$1LOG_TRACE(WideToNarrow(s));'
    if ($content -match $old295) {
        $content = $content -replace $old295, $new295
        $changeCount++
        Write-Host "  ? Converted line 295 (wstring s)" -ForegroundColor Gray
    }
    
    # Line 305: LOG_TRACE << __FUNCTION__ << L" domain=" << domain << L" name=" << name << L" value=" << value;
    $old305 = 'LOG_TRACE\s*<<\s*__FUNCTION__\s*<<\s*L"\s*domain="\s*<<\s*domain\s*<<\s*L"\s*name="\s*<<\s*name\s*<<\s*L"\s*value="\s*<<\s*value\s*;'
    $new305 = 'LOG_TRACE(std::string(__FUNCTION__) + " domain=" + WideToNarrow(domain) + " name=" + WideToNarrow(name) + " value=" + WideToNarrow(value));'
    if ($content -match $old305) {
        $content = $content -replace $old305, $new305
        $changeCount++
        Write-Host "  ? Converted line 305 (cookie params)" -ForegroundColor Gray
    }
    
    # Line 323: LOG_TRACE << result;
    $old323 = '(\s+)LOG_TRACE\s*<<\s*result\s*;'
    $new323 = '$1LOG_TRACE(WideToNarrow(result));'
    if ($content -match $old323) {
        $content = $content -replace $old323, $new323
        $changeCount++
        Write-Host "  ? Converted line 323 (result)" -ForegroundColor Gray
    }
    
    # Line 374: LOG_TRACE << "Hwnd=" << pT->m_hWnd << " caption=" << std::wstring(t);
    $old374 = 'LOG_TRACE\s*<<\s*"Hwnd="\s*<<\s*pT->m_hWnd\s*<<\s*"\s*caption="\s*<<\s*std::wstring\(t\)\s*;'
    $new374 = 'LOG_TRACE(std::string("Hwnd=") + std::to_string(reinterpret_cast<uintptr_t>(pT->m_hWnd)) + " caption=" + WideToNarrow(std::wstring(t)));'
    if ($content -match $old374) {
        $content = $content -replace $old374, $new374
        $changeCount++
        Write-Host "  ? Converted line 374 (HWND)" -ForegroundColor Gray
    }
    
    # Line 384: LOG_TRACE << __FUNCTION__ << " Using user data directory:" << userDataDirectory_.data();
    $old384 = 'LOG_TRACE\s*<<\s*__FUNCTION__\s*<<\s*"\s*Using user data directory:"\s*<<\s*userDataDirectory_\.data\(\)\s*;'
    $new384 = 'LOG_TRACE(std::string(__FUNCTION__) + " Using user data directory:" + WideToNarrow(userDataDirectory_));'
    if ($content -match $old384) {
        $content = $content -replace $old384, $new384
        $changeCount++
        Write-Host "  ? Converted line 384 (user data directory)" -ForegroundColor Gray
    }
    
    # Line 392: LOG_TRACE << __FUNCTION__ << " Unable to release webview2";
    $old392 = 'LOG_TRACE\s*<<\s*__FUNCTION__\s*<<\s*"\s*Unable to release webview2"\s*;'
    $new392 = 'LOG_TRACE(std::string(__FUNCTION__) + " Unable to release webview2");'
    if ($content -match $old392) {
        $content = $content -replace $old392, $new392
        $changeCount++
        Write-Host "  ? Converted line 392 (unable to release)" -ForegroundColor Gray
    }
    
    # Line 570: LOG_TRACE << __FUNCTION__ << " width=" << rc.Width() << " height=" << rc.Height() << " visibility=" << isVisible;
    $old570 = 'LOG_TRACE\s*<<\s*__FUNCTION__\s*<<\s*"\s*width="\s*<<\s*rc\.Width\(\)\s*<<\s*"\s*height="\s*<<\s*rc\.Height\(\)\s*<<\s*"\s*visibility="\s*<<\s*isVisible\s*;'
    $new570 = 'LOG_TRACE(std::string(__FUNCTION__) + " width=" + std::to_string(rc.Width()) + " height=" + std::to_string(rc.Height()) + " visibility=" + std::to_string(isVisible));'
    if ($content -match $old570) {
        $content = $content -replace $old570, $new570
        $changeCount++
        Write-Host "  ? Converted line 570 (dimensions)" -ForegroundColor Gray
    }
    
    # Line 575: LOG_TRACE << __FUNCTION__ << " hr=" << hr;
    $old575 = 'LOG_TRACE\s*<<\s*__FUNCTION__\s*<<\s*"\s*hr="\s*<<\s*hr\s*;'
    $new575 = 'LOG_TRACE(std::string(__FUNCTION__) + " hr=" + std::to_string(hr));'
    if ($content -match $old575) {
        $content = $content -replace $old575, $new575
        $changeCount++
        Write-Host "  ? Converted line 575 (HRESULT)" -ForegroundColor Gray
    }
    
    # Line 588: LOG_TRACE << "function=" << __func__ << "COREWEBVIEW2_WEB_ERROR_STATUS_DISCONNECTED";
    $old588 = 'LOG_TRACE\s*<<\s*"function="\s*<<\s*__func__\s*<<\s*"COREWEBVIEW2_WEB_ERROR_STATUS_DISCONNECTED"\s*;'
    $new588 = 'LOG_TRACE(std::string("function=") + __func__ + "COREWEBVIEW2_WEB_ERROR_STATUS_DISCONNECTED");'
    if ($content -match $old588) {
        $content = $content -replace $old588, $new588
        $changeCount++
        Write-Host "  ? Converted line 588 (error status)" -ForegroundColor Gray
    }
    
    # Line 648: LOG_TRACE << __FUNCTION__ << " name=Authorization" << " value=" << authV;
    $old648 = 'LOG_TRACE\s*<<\s*__FUNCTION__\s*<<\s*"\s*name=Authorization"\s*<<\s*"\s*value="\s*<<\s*authV\s*;'
    $new648 = 'LOG_TRACE(std::string(__FUNCTION__) + " name=Authorization value=" + WideToNarrow(authV));'
    if ($content -match $old648) {
        $content = $content -replace $old648, $new648
        $changeCount++
        Write-Host "  ? Converted line 648 (authorization)" -ForegroundColor Gray
    }
    
    # ====================================================================================
    # SAVE FILE IF CHANGES WERE MADE
    # ====================================================================================
    
    if ($content -ne $originalContent) {
        Set-Content -Path $filePath -Value $content -Encoding UTF8 -NoNewline
        Write-Host "  ? SAVED with $changeCount pattern groups converted" -ForegroundColor Green
    } else {
        Write-Host "  ? No changes needed" -ForegroundColor Yellow
    }
    
    Write-Host ""
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "CONVERSION COMPLETE!" -ForegroundColor Green
Write-Host "========================================`n" -ForegroundColor Cyan
Write-Host "Next step: Rebuild the project in Visual Studio" -ForegroundColor Yellow
