#include "CppRestBackend.h"

// cpprestsdk headers (from stripped fork)
#include <cpprest/http_client.h>
#include <cpprest/uri_builder.h>

#include <future>
#include <stdexcept>

namespace WebView2Http
{
	// Map our HttpMethod -> cpprest method string
	static web::http::method ToCppRestMethod(HttpMethod m)
	{
		switch (m) {
		case HttpMethod::GET:     return web::http::methods::GET;
		case HttpMethod::POST:    return web::http::methods::POST;
		case HttpMethod::PUT:     return web::http::methods::PUT;
		case HttpMethod::PATCH:   return web::http::methods::PATCH;
		case HttpMethod::DELETE_: return web::http::methods::DEL;
		case HttpMethod::HEAD:    return web::http::methods::HEAD;
		case HttpMethod::OPTIONS: return web::http::methods::OPTIONS;
		default:                  return web::http::methods::GET;
		}
	}

	CppRestBackend::CppRestBackend()  = default;
	CppRestBackend::~CppRestBackend() = default;

	HttpResponse CppRestBackend::DoSend(const HttpRequest& request)
	{
		// Build client config
		web::http::client::http_client_config cfg;
		cfg.set_timeout(utility::seconds(
			static_cast<int>(request.timeout.count() / 1000)));
		cfg.set_validate_certificates(request.verifySsl);

		web::http::client::http_client client(request.url, cfg);

		// Build request message
		web::http::http_request msg(ToCppRestMethod(request.method));

		for (auto& [k, v] : request.headers)
			msg.headers().add(k, v);

		if (!request.body.empty())
		{
			msg.set_body(
				concurrency::streams::bytestream::open_istream(request.body),
				request.body.size());
		}

		// Send synchronously (block on task)
		auto task = client.request(msg);
		auto response = task.get();

		HttpResponse result;
		result.statusCode = static_cast<int>(response.status_code());
		result.bytesSent  = request.body.size();

		// Headers
		for (auto& [k, v] : response.headers())
			result.headers[k] = v;

		// Body
		auto bodyTask = response.extract_vector();
		result.body = bodyTask.get();
		result.bytesReceived = result.body.size();

		return result;
	}

	HttpResponse CppRestBackend::Send(const HttpRequest& request)
	{
		return DoSend(request);
	}

	std::future<HttpResponse> CppRestBackend::SendAsync(const HttpRequest& request)
	{
		return std::async(std::launch::async, [this, request]()
		{
			return DoSend(request);
		});
	}
}
