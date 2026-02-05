#pragma once
#include "pch.h"
#include "Utility.h"
#include "Wininet.h"
#include "logger.h"

//				RETURN_IF_FAILED_MSG(ERROR_INVALID_PARAMETER, "message = %ls, hr = %d", A2W(std::system_category().message(ERROR_INVALID_PARAMETER).data()), ERROR_INVALID_PARAMETER);

namespace WebView2
{
	class webview2_authentication_events
	{
	private:
		wil::com_ptr<ICoreWebView2>				m_webviewEventSource = nullptr;
		wil::com_ptr<ICoreWebView2_2>           m_webviewEventSource2 = nullptr;
		wil::com_ptr<ICoreWebView2_10>          m_webviewEventSource3 = nullptr;
		wil::com_ptr<ICoreWebView2Controller>	m_controllerEventSource;
		HWND						            m_hwnd = nullptr;
		EventRegistrationToken                  m_basicAuthenticationRequestedToken = {};
		EventRegistrationToken                  m_webResourceResponseReceivedToken = {};
	public:
		webview2_authentication_events()
		{
		}
		~webview2_authentication_events()
		{
			if (m_webResourceResponseReceivedToken.value != 0)
				m_webviewEventSource2->remove_WebResourceResponseReceived(m_webResourceResponseReceivedToken);
			if (m_basicAuthenticationRequestedToken.value != 0)
				m_webviewEventSource3->remove_BasicAuthenticationRequested(m_basicAuthenticationRequestedToken);
		}
		HRESULT initialize(HWND hwnd, wil::com_ptr<ICoreWebView2> webviewEventSource, wil::com_ptr<ICoreWebView2Controller> controllerEventSource)
		{
			USES_CONVERSION;



			if (!::IsWindow(hwnd) || webviewEventSource == nullptr || controllerEventSource == nullptr)
			{

				RETURN_IF_FAILED_MSG(ERROR_INVALID_PARAMETER, "message = %ls, hr = %d", A2W(std::system_category().message(ERROR_INVALID_PARAMETER).data()), ERROR_INVALID_PARAMETER);
			}
			m_webviewEventSource = webviewEventSource;
			m_controllerEventSource = controllerEventSource;
			m_hwnd = hwnd;
			if (m_webviewEventSource.try_query_to<ICoreWebView2_2>(&m_webviewEventSource2) != true)
				return(ERROR_INVALID_DATA);
			if (m_webviewEventSource.try_query_to<ICoreWebView2_10>(&m_webviewEventSource3) != true)
				return(ERROR_INVALID_DATA);
			RETURN_IF_FAILED(enable_authentication_event());
			RETURN_IF_FAILED(enable_basic_authentication());
			return S_OK;
		}
	private:

		HRESULT set_basic_authentication_tocken()
		{
		}

		void trace_webresource_basic_authentication_event()
		{
			LOG_TRACE(__FUNCTION__);
		}
		void raise_webresource_basic_authentication_event()
		{
			LOG_TRACE(__FUNCTION__);
		}

		HRESULT trace_cookies_event(wil::com_ptr <ICoreWebView2WebResourceResponseReceivedEventArgs> args)
		{
			wil::com_ptr<ICoreWebView2WebResourceRequest> request;
			RETURN_IF_FAILED(args->get_Request(&request));

			wil::com_ptr<ICoreWebView2WebResourceResponseView> response;
			RETURN_IF_FAILED(args->get_Response(&response));

			wil::com_ptr<ICoreWebView2HttpResponseHeaders> responseHeaders;
			RETURN_IF_FAILED(response->get_Headers(&responseHeaders));


			wil::com_ptr<ICoreWebView2HttpHeadersCollectionIterator> iterator;
			RETURN_IF_FAILED(responseHeaders->GetIterator(&iterator));
			
			//BOOL hasCurrent = FALSE;
			//LOG_TRACE << "Dumping all headers for debugging purposes:";
			//while (SUCCEEDED(iterator->get_HasCurrentHeader(&hasCurrent)) && hasCurrent)
			//{
			//	wil::unique_cotaskmem_string headerName;
			//	wil::unique_cotaskmem_string headerValue;
			//	RETURN_IF_FAILED(iterator->GetCurrentHeader(&headerName, &headerValue));
			//	LOG_TRACE << __FUNCTION__ << " Header: " << headerName.get() << " Value: " << headerValue.get();
			//	RETURN_IF_FAILED(iterator->MoveNext(&hasCurrent));
			//}

			wil::com_ptr<ICoreWebView2HttpHeadersCollectionIterator> it;
			if (responseHeaders->GetHeaders(L"set-cookie", &it) == S_OK)
			{
				BOOL hasNext;				
				wil::unique_cotaskmem_string uri;
				RETURN_IF_FAILED(request->get_Uri(&uri));
				// Dump all Set-Cookie headers
				//LOG_TRACE << "Dumping all Set-Cookie headers:";

				for (it->get_HasCurrentHeader(&hasNext); hasNext; it->MoveNext(&hasNext))
				{
					wil::unique_cotaskmem_string name, value;
					it->GetCurrentHeader(&name, &value);
					// dump values

					//os::Wininet wininet;

					//wininet.SyncCookie(uri.get(), name.get(), value.get());

					//LOG_TRACE << __FUNCTION__ << " uri:" << uri.get() << " Set-Cookie Header: " << name.get() << " Value: " << value.get();
				}
			}			
			return S_OK;
		}


		HRESULT trace_authentication_event(wil::com_ptr <ICoreWebView2WebResourceResponseReceivedEventArgs> args)
		{
			wil::com_ptr<ICoreWebView2WebResourceRequest> request;
			RETURN_IF_FAILED(args->get_Request(&request));

			wil::unique_cotaskmem_string uri;
			RETURN_IF_FAILED(request->get_Uri(&uri));

			wil::com_ptr<ICoreWebView2HttpRequestHeaders> requestHeaders;
			RETURN_IF_FAILED(request->get_Headers(&requestHeaders));
			wil::unique_cotaskmem_string authHeaderValue;
			if (requestHeaders->GetHeader(L"Authorization", &authHeaderValue) == S_OK)
			{
				LOG_TRACE(std::string(__FUNCTION__) + " uri:" + WideToNarrow(uri.get()) + " Authorization header:" + WideToNarrow(authHeaderValue.get()));
			}
			return S_OK;
		}

		HRESULT enable_authentication_event()
		{
			RETURN_IF_FAILED(m_webviewEventSource2->add_WebResourceResponseReceived(Microsoft::WRL::Callback<ICoreWebView2WebResourceResponseReceivedEventHandler>([this](
				ICoreWebView2* core_web_view2,
				ICoreWebView2WebResourceResponseReceivedEventArgs* args)	-> HRESULT
				{
					trace_authentication_event(args);
					trace_cookies_event(args);
					return S_OK;

				}).Get(), &m_webResourceResponseReceivedToken));
			return S_OK;
		}

		HRESULT enable_basic_authentication()
		{
			LOG_TRACE(__FUNCTION__);
			RETURN_IF_FAILED(m_webviewEventSource3->add_BasicAuthenticationRequested(Microsoft::WRL::Callback<ICoreWebView2BasicAuthenticationRequestedEventHandler>([this](
				ICoreWebView2* sender,
				ICoreWebView2BasicAuthenticationRequestedEventArgs* args) 	-> HRESULT
				{
					trace_webresource_basic_authentication_event();
					return S_OK;

				}).Get(), &m_basicAuthenticationRequestedToken));
			return S_OK;
		}
	};
}
