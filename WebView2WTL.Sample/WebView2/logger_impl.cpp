// logger_impl.cpp - Simple local logging implementation (NO PCH)
// This file intentionally does not include OpenTelemetry headers.

#include "logger.h"

// Defensive: WTL/ATL macro collisions
#ifdef L
#undef L
#endif
#ifdef R
#undef R
#endif

#include <chrono>
#include <ctime>

// telemetry
#include <memory>
#include <opentelemetry/logs/provider.h>
#include <opentelemetry/sdk/logs/logger_provider.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor.h>

namespace logs_api = opentelemetry::logs;

static opentelemetry::nostd::shared_ptr<logs_api::Logger> g_logger;

static logs_api::Severity ToOtelSeverity(LogSeverity s)
{
    switch (s)
    {
    case LogSeverity::Trace: return logs_api::Severity::kTrace;
    case LogSeverity::Debug: return logs_api::Severity::kDebug;
    case LogSeverity::Info:  return logs_api::Severity::kInfo;
    case LogSeverity::Warn:  return logs_api::Severity::kWarn;
    case LogSeverity::Error: return logs_api::Severity::kError;
    case LogSeverity::Fatal: return logs_api::Severity::kFatal;
    default:                 return logs_api::Severity::kInfo;
    }
}

static const char* ToText(LogSeverity s)
{
    switch (s)
    {
    case LogSeverity::Trace: return "TRACE";
    case LogSeverity::Debug: return "DEBUG";
    case LogSeverity::Info:  return "INFO";
    case LogSeverity::Warn:  return "WARN";
    case LogSeverity::Error: return "ERROR";
    case LogSeverity::Fatal: return "FATAL";
    default:                 return "INFO";
    }
}

void InitializeLogging()
{
    // Use default provider for now; later you can plug OTLP provider here.
    auto provider = logs_api::Provider::GetLoggerProvider();
    g_logger = provider->GetLogger("webview2client");
}

static opentelemetry::nostd::string_view ToOtelView(std::string_view s) noexcept
{
    return opentelemetry::nostd::string_view{ s.data(), s.size() };
}

void LogMessage(LogSeverity severity, std::string_view message)
{
    if (!g_logger)
    {
        InitializeLogging();
    }

    // OutputDebugString sink
    std::string line(message);
    line.push_back('\n');
    OutputDebugStringA(line.c_str());

    // OpenTelemetry sink
    if (g_logger)
    {
        g_logger->EmitLogRecord(ToOtelSeverity(severity), ToOtelView(message));
    }
}