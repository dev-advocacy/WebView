#pragma once

// OpenTelemetry logging wrapper 

#include <string>
#include <string_view>
#include <sstream>
#include <Windows.h>

enum class LogSeverity : int
{
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal
};




// Function declarations (implementations in logger_impl.cpp)
void InitializeLogging();
void LogMessage(LogSeverity severity, std::string_view message);

#define LOG_TRACE(message)   ::LogMessage(::LogSeverity::Trace, (message))
#define LOG_DEBUG(message)   ::LogMessage(::LogSeverity::Debug, (message))
#define LOG_INFO(message)    ::LogMessage(::LogSeverity::Info,  (message))
#define LOG_WARNING(message) ::LogMessage(::LogSeverity::Warn,  (message))
#define LOG_ERROR(message)   ::LogMessage(::LogSeverity::Error, (message))
#define LOG_FATAL(message)   ::LogMessage(::LogSeverity::Fatal, (message))


inline std::string WideToNarrow(const std::wstring& wstr)
{
    if (wstr.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string out(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), out.data(), n, nullptr, nullptr);
    return out;
}

