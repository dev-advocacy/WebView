#pragma once

// OpenTelemetry logging wrapper replacing Boost.Log usage.

#include <string>
#include <string_view>
#include <memory>

#include "opentelemetry/nostd/shared_ptr.h"
#include "opentelemetry/logs/logger.h"
#include "opentelemetry/logs/provider.h"
#include "opentelemetry/logs/severity.h"
#include "opentelemetry/sdk/logs/logger_provider_factory.h"
#include "opentelemetry/sdk/logs/simple_log_record_processor_factory.h"
#include "opentelemetry/exporters/ostream/log_record_exporter_factory.h"

namespace logs_api = opentelemetry::logs;
namespace logs_sdk = opentelemetry::sdk::logs;
namespace logs_exporter = opentelemetry::exporter::logs;
namespace nostd = opentelemetry::nostd;

inline void InitializeLogging()
{
	static bool initialized = false;
	if (initialized)
	{
		return;
	}

	auto exporter = logs_exporter::OStreamLogRecordExporterFactory::Create();
	auto processor = logs_sdk::SimpleLogRecordProcessorFactory::Create(std::move(exporter));
	auto provider = logs_sdk::LoggerProviderFactory::Create(std::move(processor));

	nostd::shared_ptr<logs_api::LoggerProvider> shared_provider(provider.release());
	logs_api::Provider::SetLoggerProvider(shared_provider);
	
	initialized = true;
}

inline nostd::shared_ptr<logs_api::Logger> GetLogger()
{
	static nostd::shared_ptr<logs_api::Logger> logger;
	if (!logger)
	{
		InitializeLogging();
		auto provider = logs_api::Provider::GetLoggerProvider();
		logger = provider->GetLogger("webview2client");
	}
	return logger;
}

inline void LogMessage(logs_api::Severity severity, std::string_view message)
{
	auto logger = GetLogger();
	if (!logger)
	{
		return;
	}

	logger->EmitLogRecord(severity, message);
}

// ===== log macros =====

// These accept a std::string / const char* message.
#define LOG_TRACE(message)   ::LogMessage(::opentelemetry::logs::Severity::kTrace,   (message))
#define LOG_DEBUG(message)   ::LogMessage(::opentelemetry::logs::Severity::kDebug,   (message))
#define LOG_INFO(message)    ::LogMessage(::opentelemetry::logs::Severity::kInfo,    (message))
#define LOG_WARNING(message) ::LogMessage(::opentelemetry::logs::Severity::kWarn,    (message))
#define LOG_ERROR(message)   ::LogMessage(::opentelemetry::logs::Severity::kError,   (message))
#define LOG_FATAL(message)   ::LogMessage(::opentelemetry::logs::Severity::kFatal,   (message))
