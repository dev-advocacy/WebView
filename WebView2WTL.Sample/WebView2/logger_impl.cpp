// logger_impl.cpp - Simple local logging implementation (NO PCH)
// This file intentionally does not include OpenTelemetry headers.

#include "logger.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

void InitializeLogging()
{
    // no-op for simple logger
}

static const char* SeverityToString(opentelemetry::logs::Severity s)
{
    using Severity = opentelemetry::logs::Severity;
    switch (s)
    {
    case Severity::kTrace: return "TRACE";
    case Severity::kTrace2: return "TRACE2";
    case Severity::kTrace3: return "TRACE3";
    case Severity::kTrace4: return "TRACE4";
    case Severity::kDebug: return "DEBUG";
    case Severity::kDebug2: return "DEBUG2";
    case Severity::kDebug3: return "DEBUG3";
    case Severity::kDebug4: return "DEBUG4";
    case Severity::kInfo: return "INFO";
    case Severity::kInfo2: return "INFO2";
    case Severity::kInfo3: return "INFO3";
    case Severity::kInfo4: return "INFO4";
    case Severity::kWarn: return "WARN";
    case Severity::kWarn2: return "WARN2";
    case Severity::kWarn3: return "WARN3";
    case Severity::kWarn4: return "WARN4";
    case Severity::kError: return "ERROR";
    case Severity::kError2: return "ERROR2";
    case Severity::kError3: return "ERROR3";
    case Severity::kError4: return "ERROR4";
    case Severity::kFatal: return "FATAL";
    case Severity::kFatal2: return "FATAL2";
    case Severity::kFatal3: return "FATAL3";
    case Severity::kFatal4: return "FATAL4";
    default: return "UNKNOWN";
    }
}

void LogMessage(opentelemetry::logs::Severity severity, std::string_view message)
{
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t tt = system_clock::to_time_t(now);
    tm local_tm;
    localtime_s(&local_tm, &tt);

    char timebuf[64];
    if (std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &local_tm) == 0)
        timebuf[0] = '\0';

    std::string line;
    line.reserve(128 + message.size());
    line += '[';
    line += timebuf;
    line += "] [";
    line += SeverityToString(severity);
    line += "] ";
    line += message;
    line += '\n';

    std::cout << line;
    OutputDebugStringA(line.c_str());
}
