#pragma once

// OpenTelemetry logging wrapper replacing Boost.Log usage.

#include <string_view>
#include <string>
#include <sstream>
#include <format>
// Windows APIs used for character conversion
#include <Windows.h>

// Forward declare OpenTelemetry namespace and types to avoid header pollution
namespace opentelemetry
{
namespace logs
{
enum class Severity : int
{
	kTrace = 1,
	kTrace2 = 2,
	kTrace3 = 3,
	kTrace4 = 4,
	kDebug = 5,
	kDebug2 = 6,
	kDebug3 = 7,
	kDebug4 = 8,
	kInfo = 9,
	kInfo2 = 10,
	kInfo3 = 11,
	kInfo4 = 12,
	kWarn = 13,
	kWarn2 = 14,
	kWarn3 = 15,
	kWarn4 = 16,
	kError = 17,
	kError2 = 18,
	kError3 = 19,
	kError4 = 20,
	kFatal = 21,
	kFatal2 = 22,
	kFatal3 = 23,
	kFatal4 = 24
};
}
}

// Function declarations (implementations in logger_impl.cpp)
void InitializeLogging();
void LogMessage(opentelemetry::logs::Severity severity, std::string_view message);

// ===== log macros =====

// These accept a std::string / const char* message.
#define LOG_TRACE(message)   ::LogMessage(::opentelemetry::logs::Severity::kTrace,   (message))
#define LOG_DEBUG(message)   ::LogMessage(::opentelemetry::logs::Severity::kDebug,   (message))
#define LOG_INFO(message)    ::LogMessage(::opentelemetry::logs::Severity::kInfo,    (message))
#define LOG_WARNING(message) ::LogMessage(::opentelemetry::logs::Severity::kWarn,    (message))
#define LOG_ERROR(message)   ::LogMessage(::opentelemetry::logs::Severity::kError,   (message))
#define LOG_FATAL(message)   ::LogMessage(::opentelemetry::logs::Severity::kFatal,   (message))

// ===== Helper functions for string conversion =====

// Convert wide string to narrow string
inline std::string WideToNarrow(const std::wstring& wstr)
{
	if (wstr.empty()) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
	return strTo;
}

// Helper to build log messages (for stream-style replacement)
class LogMessageBuilder
{
public:
	template<typename T>
	LogMessageBuilder& operator<<(const T& value)
	{
		if constexpr (std::is_same_v<T, std::wstring> || std::is_same_v<T, const wchar_t*>)
		{
			m_stream << WideToNarrow(value);
		}
		else
		{
			m_stream << value;
		}
		return *this;
	}

	std::string str() const { return m_stream.str(); }
	operator std::string() const { return str(); }

private:
	std::ostringstream m_stream;
};

