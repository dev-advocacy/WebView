# PowerShell script to convert Boost.Log stream-style LOG calls to OpenTelemetry function-call style
# This script converts LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR calls

$filesToProcess = @(
    "WebView2\WebView2Impl2.h",
    "WebView2\WebView2Impl.h",
    "WebView2\WebView2.cpp"
)

function Convert-LogCalls {
    param(
        [string]$filePath
    )
    
    if (-not (Test-Path $filePath)) {
        Write-Host "File not found: $filePath" -ForegroundColor Red
        return
    }
    
    Write-Host "Processing: $filePath" -ForegroundColor Yellow
    
    $content = Get-Content $filePath -Raw -Encoding UTF8
    $originalContent = $content
    $changeCount = 0
    
    # Pattern 1: LOG_TRACE << __FUNCTION__;
    $pattern1 = '(LOG_TRACE|LOG_DEBUG|LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_FATAL)\s*<<\s*__FUNCTION__\s*;'
    $replacement1 = '$1(__FUNCTION__);'
    if ($content -match $pattern1) {
        $content = $content -replace $pattern1, $replacement1
        $changeCount++
    }
    
    # Pattern 2: LOG_TRACE << "literal string";
    $pattern2 = '(LOG_TRACE|LOG_DEBUG|LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_FATAL)\s*<<\s*"([^"]+)"\s*;'
    $replacement2 = '$1("$2");'
    if ($content -match $pattern2) {
        $content = $content -replace $pattern2, $replacement2
        $changeCount++
    }
    
    # Pattern 3: LOG_TRACE << __FUNCTION__ << " stuff" << variable;
    # This is more complex and needs manual handling
    # We'll just report these
    $complexPattern = '(LOG_TRACE|LOG_DEBUG|LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_FATAL)\s*<<[^;]+<<[^;]+;'
    $complexMatches = [regex]::Matches($content, $complexPattern)
    
    if ($complexMatches.Count -gt 0) {
        Write-Host "  Found $($complexMatches.Count) complex LOG calls that need manual conversion" -ForegroundColor Cyan
    }
    
    if ($content -ne $originalContent) {
        Set-Content -Path $filePath -Value $content -Encoding UTF8 -NoNewline
        Write-Host "  Converted $changeCount simple patterns in $filePath" -ForegroundColor Green
    } else {
        Write-Host "  No simple patterns to convert in $filePath" -ForegroundColor Gray
    }
}

# Process each file
foreach ($file in $filesToProcess) {
    Convert-LogCalls -filePath $file
}

Write-Host "`nConversion complete! Please review the files for complex LOG calls that need manual conversion." -ForegroundColor Green
Write-Host "Look for patterns like: LOG_TRACE << __FUNCTION__ << ... (with multiple <<)" -ForegroundColor Yellow
