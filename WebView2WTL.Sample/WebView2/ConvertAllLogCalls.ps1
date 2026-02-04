# PowerShell script to convert ALL remaining stream-style LOG calls to function-call style
# This handles the remaining ~170 errors in WebView2Impl2.h and WebView2Impl.h

$ErrorActionPreference = "Stop"

$filesToProcess = @(
    "WebView2Impl2.h",
    "WebView2Impl.h",
    "MainFrm.cpp"
)

function Convert-SimpleLogCalls {
    param(
        [string]$content
    )
    
    # Pattern 1: LOG_XXX << __FUNCTION__;
    $content = $content -replace '(LOG_TRACE|LOG_DEBUG|LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_FATAL)\s*<<\s*__FUNCTION__\s*;', '$1(__FUNCTION__);'
    
    # Pattern 2: LOG_XXX << L"literal";  or LOG_XXX << "literal";
    $content = $content -replace '(LOG_TRACE|LOG_DEBUG|LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_FATAL)\s*<<\s*L?"([^"]+)"\s*;', '$1("$2");'
    
    return $content
}

function Convert-ComplexLogCalls {
    param(
        [string]$content
    )
    
    # This function handles multi-part LOG statements
    # We'll use regex to find and convert complex patterns
    
    # Pattern: LOG_XXX << __FUNCTION__ << " literal" << variable;
    # We need to build these into proper string concatenation
    
    $lines = $content -split "`n"
    $result = New-Object System.Collections.ArrayList
    
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        
        # Check if this line contains a complex LOG statement
        if ($line -match '^\s*(LOG_TRACE|LOG_DEBUG|LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_FATAL)\s*<<') {
            $logLevel = $matches[1]
            
            # Collect all parts of the LOG statement (may span multiple lines)
            $logStatement = $line
            $j = $i
            
            # Continue collecting lines until we find the semicolon
            while ($j -lt $lines.Count -and $logStatement -notmatch ';') {
                $j++
                if ($j -lt $lines.Count) {
                    $logStatement += "`n" + $lines[$j]
                }
            }
            
            # Now parse and convert the log statement
            $convertedLine = Convert-SingleComplexLog $logStatement $logLevel
            
            if ($convertedLine -ne $null) {
                [void]$result.Add($convertedLine)
                # Skip the lines we've already processed
                $i = $j
                continue
            }
        }
        
        [void]$result.Add($line)
    }
    
    return ($result -join "`n")
}

function Convert-SingleComplexLog {
    param(
        [string]$logStatement,
        [string]$logLevel
    )
    
    # Extract indentation
    if ($logStatement -match '^(\s*)') {
        $indent = $matches[1]
    } else {
        $indent = ""
    }
    
    # Remove the LOG_XXX << part
    $parts = $logStatement -replace '^\s*LOG_\w+\s*<<\s*', '' -replace '\s*;\s*$', ''
    
    # Split by << and process each part
    $segments = $parts -split '\s*<<\s*'
    $converted = New-Object System.Collections.ArrayList
    
    foreach ($segment in $segments) {
        $segment = $segment.Trim()
        
        if ($segment -eq '__FUNCTION__') {
            [void]$converted.Add('std::string(__FUNCTION__)')
        }
        elseif ($segment -match '^L"(.+)"$') {
            # Wide string literal
            $str = $matches[1]
            [void]$converted.Add("std::string(`"$str`")")
        }
        elseif ($segment -match '^"(.+)"$') {
            # Narrow string literal  
            $str = $matches[1]
            [void]$converted.Add("std::string(`"$str`")")
        }
        elseif ($segment -match '^\w+$' -or $segment -match '^\w+\.\w+\(\)$') {
            # Variable or function call - needs conversion
            if ($segment -match '^(m_\w+|is\w+|navigationId|errorStatus|method|uri|resourceContext)') {
                # These might be wide strings or numbers
                [void]$converted.Add("WideToNarrow($segment)")
            } else {
                [void]$converted.Add("std::to_string($segment)")
            }
        }
        else {
            # Complex expression - try to handle it
            [void]$converted.Add("std::to_string($segment)")
        }
    }
    
    if ($converted.Count -gt 0) {
        $joined = ($converted -join ' + ')
        return "$indent$logLevel($joined);"
    }
    
    return $null
}

# Main processing
foreach ($file in $filesToProcess) {
    $fullPath = $file
    
    if (-not (Test-Path $fullPath)) {
        Write-Host "File not found: $fullPath - trying current directory" -ForegroundColor Yellow
        continue
    }
    
    Write-Host "`nProcessing: $file" -ForegroundColor Cyan
    
    try {
        $content = Get-Content $fullPath -Raw -Encoding UTF8
        $originalContent = $content
        
        # First pass: Convert simple patterns
        Write-Host "  Converting simple LOG patterns..." -ForegroundColor Gray
        $content = Convert-SimpleLogCalls -content $content
        
        # Second pass: Handle complex patterns (commented out for now - needs manual review)
        # $content = Convert-ComplexLogCalls -content $content
        
        if ($content -ne $originalContent) {
            # Create backup
            $backupPath = "$fullPath.backup"
            Copy-Item $fullPath $backupPath -Force
            Write-Host "  Created backup: $backupPath" -ForegroundColor Gray
            
            # Save converted file
            Set-Content -Path $fullPath -Value $content -Encoding UTF8 -NoNewline
            Write-Host "  ? Converted simple patterns in $file" -ForegroundColor Green
        } else {
            Write-Host "  No simple patterns found to convert" -ForegroundColor Yellow
        }
    }
    catch {
        Write-Host "  ERROR processing $file : $_" -ForegroundColor Red
    }
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Conversion Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "`nNOTE: Complex LOG statements with multiple << operators need manual conversion." -ForegroundColor Yellow
Write-Host "Look for patterns like:" -ForegroundColor Yellow
Write-Host "  LOG_TRACE << variable1 << variable2;" -ForegroundColor Gray
Write-Host "`nThese should be converted to:" -ForegroundColor Yellow  
Write-Host "  LOG_TRACE(std::to_string(variable1) + std::to_string(variable2));" -ForegroundColor Gray
Write-Host "`nBackup files created with .backup extension.`n" -ForegroundColor Cyan
