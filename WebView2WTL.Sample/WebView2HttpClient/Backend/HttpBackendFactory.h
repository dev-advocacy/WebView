#pragma once
#include "../IHttpBackend.h"
#include <memory>

namespace WebView2Http
{
	class HttpBackendFactory
	{
	public:
		static std::unique_ptr<IHttpBackend> Create(BackendType type);
	};
}
