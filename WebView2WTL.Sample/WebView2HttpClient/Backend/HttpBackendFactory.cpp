#include "HttpBackendFactory.h"
#include "WinHttpBackend.h"
#include "WinINetBackend.h"
#include "CppRestBackend.h"
#include <stdexcept>

namespace WebView2Http
{
	std::unique_ptr<IHttpBackend> HttpBackendFactory::Create(BackendType type)
	{
		switch (type)
		{
		case BackendType::WinHTTP:  return std::make_unique<WinHttpBackend>();
		case BackendType::WinINet:  return std::make_unique<WinINetBackend>();
		case BackendType::CppRest:  return std::make_unique<CppRestBackend>();
		default:
			throw std::invalid_argument("Unknown BackendType");
		}
	}
}
