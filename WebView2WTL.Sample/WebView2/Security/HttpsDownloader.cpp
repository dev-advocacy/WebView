// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "pch.h"
#include "HttpsDownloader.h"
#include "ClientCertificateSelector.h"
#include "../Logger/logger.h"
#include <atlstr.h>
#include <cryptuiapi.h>

namespace webview::net
{
	// -----------------------------------------------------------------------
	// HttpsDownloader implementation
	// -----------------------------------------------------------------------

	std::string HttpsDownloader::Download(const std::wstring& url,
										  const std::wstring& clientCertSubjectFilter,
										  SelectedCertInfo*   outCertInfo)
	{
		URL_COMPONENTS comp{};
		std::wstring   host(256, L'\0');
		std::wstring   path(2048, L'\0');

		comp.dwStructSize      = sizeof(comp);
		comp.lpszHostName      = host.data();
		comp.dwHostNameLength  = static_cast<DWORD>(host.size());
		comp.lpszUrlPath       = path.data();
		comp.dwUrlPathLength   = static_cast<DWORD>(path.size());
		comp.dwSchemeLength    = 1;

		if (!InternetCrackUrlW(url.c_str(), 0, 0, &comp))
			throw WinInetException("InternetCrackUrlW", GetLastError());

		host.resize(comp.dwHostNameLength);
		path.resize(comp.dwUrlPathLength);

		const auto internet    = OpenInternet();
		const auto connection  = OpenConnection(internet.get(), host, comp.nPort);
		const auto request     = OpenRequest(connection.get(), path);

		UniqueCertContext selectedCert = SendRequestWithCertAuth(
			request.get(), clientCertSubjectFilter, host, comp.nPort);

		// Populate outCertInfo if provided and a certificate was selected
		if (outCertInfo && selectedCert)
		{
			outCertInfo->host         = host;
			outCertInfo->port         = comp.nPort;
			outCertInfo->subject      = ClientCertificateSelector::GetSubjectName(selectedCert.get());
			outCertInfo->issuer       = ClientCertificateSelector::GetIssuerName(selectedCert.get());
			outCertInfo->friendlyName = ClientCertificateSelector::GetFriendlyName(selectedCert.get());
			outCertInfo->notBefore    = selectedCert->pCertInfo->NotBefore;
			outCertInfo->notAfter     = selectedCert->pCertInfo->NotAfter;
			outCertInfo->certContext  = std::move(selectedCert);
		}

		const DWORD status = QueryStatusCode(request.get());
		if (status >= 400)
			throw std::runtime_error("HTTP error status: " + std::to_string(status));

		return ReadAll(request.get());
	}

	UniqueInternetHandle HttpsDownloader::OpenInternet()
	{
		HINTERNET h = InternetOpenW(L"WebView2WTL/1.0",
			INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
		if (!h) throw WinInetException("InternetOpenW", GetLastError());
		return UniqueInternetHandle(h);
	}

	UniqueInternetHandle HttpsDownloader::OpenConnection(HINTERNET internet,
														  const std::wstring& host,
														  INTERNET_PORT port)
	{
		HINTERNET h = InternetConnectW(internet, host.c_str(), port,
			nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
		if (!h) throw WinInetException("InternetConnectW", GetLastError());
		return UniqueInternetHandle(h);
	}

	UniqueInternetHandle HttpsDownloader::OpenRequest(HINTERNET conn,
													   const std::wstring& path)
	{
		const wchar_t* accept[] = { L"*/*", nullptr };
		HINTERNET h = HttpOpenRequestW(conn, L"GET", path.c_str(),
			nullptr, nullptr, accept,
			INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
			0);
		if (!h) throw WinInetException("HttpOpenRequestW", GetLastError());
		return UniqueInternetHandle(h);
	}

	UniqueCertContext HttpsDownloader::SendRequestWithCertAuth(
		HINTERNET            request,
		const std::wstring&  clientCertSubjectFilter,
		const std::wstring&  host,
		INTERNET_PORT        port)
	{
		UniqueCertContext usedCert;

		for (int attempt = 0; attempt < 4; ++attempt)
		{
			if (HttpSendRequestW(request, nullptr, 0, nullptr, 0))
			{
				if (!usedCert)
					usedCert = QueryClientCertFromRequest(request); // best effort fallback only
				return usedCert;
			}

			const DWORD err = GetLastError();
			if (err != ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED)
				throw WinInetException("HttpSendRequestW", err);

			if (clientCertSubjectFilter.empty())
			{
				// Use smart filtering with request handle
				usedCert = SelectClientCertificateWithDialog(host, port, request);
				if (!usedCert)
					throw std::runtime_error("Client certificate selection cancelled by user.");
			}
			else
			{
				// Use smart filtering with subject filter
				auto filteredCerts = ClientCertificateSelector::SelectClientAuthCertificates(
					request, clientCertSubjectFilter);

				if (filteredCerts.empty())
				{
					throw std::runtime_error("No suitable client certificate found matching filter and server requirements.");
				}

				// Use the first matching certificate
				usedCert = std::move(filteredCerts[0]);


			}

			if (!InternetSetOptionW(request,
					INTERNET_OPTION_CLIENT_CERT_CONTEXT,
					const_cast<CERT_CONTEXT*>(usedCert.get()),
					sizeof(CERT_CONTEXT)))
			{
				throw WinInetException(
					"InternetSetOptionW(INTERNET_OPTION_CLIENT_CERT_CONTEXT)",
					GetLastError());
			}
		}

		throw std::runtime_error(
				"Server requires client certificate and request retry failed.");
	}

	UniqueCertContext HttpsDownloader::SelectClientCertificateWithDialog(
		const std::wstring& host,
		INTERNET_PORT port,
		HINTERNET request)
	{
		// Use smart filtering if request handle is provided
		if (request)
		{
			auto filteredCerts = ClientCertificateSelector::SelectClientAuthCertificates(request);

			if (filteredCerts.empty())
			{

				return UniqueCertContext();
			}

			// Create a memory store with only filtered certificates
			HCERTSTORE memStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
				CERT_STORE_CREATE_NEW_FLAG, nullptr);
			if (!memStore)
				throw WinInetException("CertOpenStore(MEMORY)", GetLastError());

			for (auto& certUnique : filteredCerts)
			{
				CertAddCertificateContextToStore(memStore, certUnique.get(),
					CERT_STORE_ADD_USE_EXISTING, nullptr);
			}

			const std::wstring title = L"Select client certificate for " + host + L":" + std::to_wstring(port);
			PCCERT_CONTEXT selected = CryptUIDlgSelectCertificateFromStore(
				memStore,
				GetDesktopWindow(),
				title.c_str(),
				L"Choose a client certificate (filtered by server requirements)",
				0,
				0,
				nullptr);

			CertCloseStore(memStore, 0);



			return UniqueCertContext(selected);
		}
		else
		{
			// Fallback: show all certificates from MY store (no filtering)
			HCERTSTORE store = CertOpenSystemStoreW(0, L"MY");
			if (!store)
				throw WinInetException("CertOpenSystemStoreW", GetLastError());

			const std::wstring title = L"Select client certificate for " + host + L":" + std::to_wstring(port);
			PCCERT_CONTEXT selected = CryptUIDlgSelectCertificateFromStore(
				store,
				GetDesktopWindow(),
				title.c_str(),
				L"Choose a client certificate",
				0,
				0,
				nullptr);

			CertCloseStore(store, 0);
			return UniqueCertContext(selected);
		}
	}

	UniqueCertContext HttpsDownloader::QueryClientCertFromRequest(HINTERNET request)
	{
		// IMPORTANT:
		// - INTERNET_OPTION_CLIENT_CERT_CONTEXT is primarily documented for InternetSetOption.
		// - On some WinInet/request combinations, InternetQueryOption may fail with
		//   ERROR_INVALID_PARAMETER.
		// - We treat this as non-fatal and log it for diagnostics.
		CERT_CONTEXT ctx{};
		DWORD ctxSize = sizeof(ctx);
		if (InternetQueryOptionW(request,
				INTERNET_OPTION_CLIENT_CERT_CONTEXT,
				&ctx,
				&ctxSize))
		{
			PCCERT_CONTEXT dup = CertDuplicateCertificateContext(&ctx);
			if (dup)
			{
				return UniqueCertContext(dup);
			}
			return {};
		}

		return {};
	}

	DWORD HttpsDownloader::QueryStatusCode(HINTERNET request)
	{
		DWORD code = 0, sz = sizeof(code);
		if (!HttpQueryInfoW(request,
				HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
				&code, &sz, nullptr))
			throw WinInetException("HttpQueryInfoW", GetLastError());
		return code;
	}

	std::string HttpsDownloader::ReadAll(HINTERNET request)
	{
		std::string       body;
		std::vector<char> buf(8192);
		for (;;)
		{
			DWORD read = 0;
			if (!InternetReadFile(request, buf.data(),
					static_cast<DWORD>(buf.size()), &read))
				throw WinInetException("InternetReadFile", GetLastError());
			if (read == 0) break;
			body.append(buf.data(), read);
		}
		return body;
	}

} // namespace webview::net

