#pragma once
#include "pch.h"
#include "resource.h"
#include "headersprop.h"
#include "logger.h"
#include "Cookie.h"
#include "CompositionHost.h"
#include "WebViewEvents.h"
#include "WebViewAuthentication.h"
#include "Wininet.h"
#include "SingleWebView2.h"

namespace WebView2
{
	// Add this struct at the top or in a suitable header

	class ContextData
	{
	public:
		std::wstring			m_classname = L"Chrome_WidgetWin_0";
		HWND					m_hwnd = nullptr;
	};
	
	template <class T>
	class CWebView2Impl2 : public CCompositionHost<T>, public IWebWiew2ImplEventCallback
	{
	public:
		std::wstring										m_url;
		std::wstring										m_browser_directory;
		std::wstring										m_user_data_directory;
		bool												m_is_modal = false;
		bool												m_start = true;
	private:
		HWND												m_hwnd = nullptr;
		wil::com_ptr<ICoreWebView2Environment>				m_webViewEnvironment = nullptr;
		wil::com_ptr<ICoreWebView2CompositionController>	m_compositionController = nullptr;
		wil::com_ptr<ICoreWebView2Controller>				m_controller = nullptr;
		wil::com_ptr<ICoreWebView2Settings>					m_webSettings = nullptr;
		wil::com_ptr<ICoreWebView2CookieManager>			m_cookieManager = nullptr;
		wil::com_ptr<ICoreWebView2>							m_webView = nullptr;
		std::unique_ptr<webview2_events>					m_webview2_events = nullptr;
		std::unique_ptr<webview2_authentication_events>		m_webview2_authentication_events = nullptr;
		std::list<std::future<void>>						m_asyncResults;
		std::mutex											m_asyncResultsMutex;
		bool												m_is_test = false;
		std::wstring										m_port;
		HWND												m_hwnd_parent = nullptr;
	public:
		// Message map and handlers
		BEGIN_MSG_MAP(CWebView2Impl2)
			CHAIN_MSG_MAP(CCompositionHost<T>)
			MESSAGE_HANDLER(WM_CREATE, OnCreate)
			MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
			MESSAGE_HANDLER(WM_RUN_FUNCTOR, OnRunFunctor)
		END_MSG_MAP()

	CWebView2Impl2()
	{
		LOG_TRACE(__FUNCTION__);
		m_webview2_events = std::make_unique<webview2_events>();
		m_webview2_authentication_events = std::make_unique<webview2_authentication_events>();
	};

		void set_test(bool isTest, std::wstring port)
		{
			m_is_test = isTest;
			m_port = port;
		}

		/// <summary>
		/// Sets the parent window handle.
		/// </summary>
		/// <param name="hwnd">The handle to the parent window.</param>
		void set_parent(HWND hwnd)
		{
			m_hwnd_parent = hwnd;
		}
	virtual ~CWebView2Impl2()
	{
		LOG_TRACE(__FUNCTION__);
	}
	#pragma region windows_event
		/// <summary>
		/// Handler for the WM_INITDIALOG message.
		/// </summary>
		/// <param name="uMsg">The message identifier.</param>
		/// <param name="wParam">Additional message information.</param>
		/// <param name="lParam">Additional message information.</param>
		/// <param name="bHandled">Indicates if the message was handled.</param
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		LOG_TRACE(__FUNCTION__);

		T* pT = static_cast<T*>(this);
			if (::IsWindow(pT->m_hWnd))
			{
				m_hwnd = pT->m_hWnd;
				RETURN_IF_FAILED(InitializeWebView(true));
				m_is_modal = true;
			}
			return 0L;
		}
		/// <summary>
		/// Handler for the WM_CREATE message.
		/// </summary>
		/// <param name="uMsg">The message identifier.</param>
		/// <param name="wParam">Additional message information.</param>
		/// <param name="lParam">Additional message information.</param>
		/// <param name="bHandled">Indicates if the message was handled.</param>
		/// <returns>The result of the message processing.</returns>
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		LOG_TRACE(__FUNCTION__);
		T* pT = static_cast<T*>(this);
			if (::IsWindow(pT->m_hWnd))
			{
				m_hwnd = pT->m_hWnd;
				RETURN_IF_FAILED(InitializeWebView(true));
			}
			return 0L;
		}
		/// <summary>
		/// Handler for the WM_RUN_FUNCTOR message.
		/// </summary>
		/// <param name="uMsg">The message identifier.</param>
		/// <param name="wParam">Additional message information.</param>
		/// <param name="lParam">Additional message information.</param>
		/// <param name="bHandled">Indicates if the message was handled.</param>
		/// <returns>The result of the message processing.</returns>
		LRESULT	OnRunFunctor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			auto* functor = reinterpret_cast<UIFunctorBase*>(wParam);

			if (functor == nullptr)
				return -1; // Error while posting the message. 

			functor->Invoke();
			functor->SignalComplete();

			// Clean up future<void> instances for fire-and-forget operations that have completed.
			std::lock_guard guard(m_asyncResultsMutex);
			m_asyncResults.remove_if([](std::future<void>& value)
				{
					return value.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
				});

			return 0;
		}
		#pragma endregion windows_event

		#pragma region WebView2_event
		/// <summary>
		/// Navigates to the specified URL.
		/// </summary>
		/// <param name="url">The URL to navigate to.</param>
		/// <returns>The result of the navigation.</returns>
		HRESULT navigate(std::wstring url)
		{
			if (m_webView)
			{
				RETURN_IF_FAILED(m_webView->Navigate(url.c_str()));
			}
			else
				return ERROR_INVALID_ADDRESS;
			return S_OK;
		}

		HRESULT getwininetcookies(std::wstring uri)
		{
			//os::Wininet wininet;

			//wininet.GetCookies(uri);

			return S_OK;
		}

		HRESULT getcookies(std::wstring uri)
		{
			//get_cookies(uri);
			return S_OK;
		}
		HRESULT getcookies()
		{
			//get_cookies_by_devtools();
			return S_OK;
		}
	HRESULT showdevtools()
	{
		LOG_TRACE(__FUNCTION__);

		HRESULT hr= m_webView->OpenDevToolsWindow();

			RETURN_IF_FAILED_MSG(hr, "Failed to delete all cookies, hr = % d", hr);
			return (hr);
		}

	HRESULT clearcookies()
	{
		LOG_TRACE(__FUNCTION__);
		//RETURN_IF_NULL_ALLOC(m_cookieManager);
		//	HRESULT hr = m_cookieManager->DeleteAllCookies();
		//	return (hr);
    return S_OK;
		}

	HRESULT add_cookie(std::wstring domain, std::wstring name, std::wstring value)
	{
		LOG_TRACE(std::string(__FUNCTION__) + " domain=" + WideToNarrow(domain) + " name=" + WideToNarrow(name) + " value=" + WideToNarrow(value));
		//  RETURN_IF_NULL_ALLOC(m_cookieManager);
		//	wil::com_ptr<ICoreWebView2Cookie> cookie;
		//	HRESULT hr = m_cookieManager->CreateCookie(name.c_str(), value.c_str(), domain.c_str(), L"/", &cookie);
		//	RETURN_IF_FAILED_MSG(hr, "function = % s, message = % s, hr = % d", __func__, std::system_category().message(hr).data(), hr);

			// cookie->put_IsHttpOnly(TRUE);
			// cookie->put_IsSecure(TRUE);

			// hr = m_cookieManager->AddOrUpdateCookie(cookie.get());
			// RETURN_IF_FAILED_MSG(hr, "function = % s, message = % s, hr = % d", __func__, std::system_category().message(hr).data(), hr);
			return S_OK;
		}
	HRESULT delete_all_cookies()
	{
		LOG_TRACE(__FUNCTION__);
		//RETURN_IF_NULL_ALLOC(m_cookieManager);
		//	HRESULT hr = m_cookieManager->DeleteAllCookies();
		//	RETURN_IF_FAILED_MSG(hr, "function = % s, message = % s, hr = % d", __func__, std::system_category().message(hr).data(), hr);
			return true;
		}

		HRESULT get_cookies(std::wstring uri)
		{
			/*RETURN_IF_NULL_ALLOC(m_cookieManager);
			HRESULT hr = m_cookieManager->GetCookies(uri.c_str(), Microsoft::WRL::Callback<ICoreWebView2GetCookiesCompletedHandler>([this, uri](HRESULT error_code, ICoreWebView2CookieList* list) -> HRESULT
				{
					if (SUCCEEDED(error_code))
					{
						std::wstring result;
						UINT cookie_list_size;
						RETURN_IF_FAILED(list->get_Count(&cookie_list_size));
						if (cookie_list_size == 0)
						{
							result += L"No cookies found.";
						}
						else
						{
							result += std::to_wstring(cookie_list_size) + L" cookie(s) found";
							if (!uri.empty())
							{
								result += L" on " + uri;
							}
							result += L"\n[";
							for (UINT i = 0; i < cookie_list_size; ++i)
							{
								wil::com_ptr<ICoreWebView2Cookie> cookie;
								RETURN_IF_FAILED(list->GetValueAtIndex(i, &cookie));

								if (cookie.get())
								{
									result += Utility::CookieToString(cookie.get());
									if (i != cookie_list_size - 1)
									{
										result += L",\n";
									}
								}
							}
						result += L"]";
					}
					LOG_TRACE(WideToNarrow(result));
				}
				return error_code;
				}
			).Get());
			RETURN_IF_FAILED(hr);*/
			return S_OK;
		}
		HRESULT getpostData(std::wstring postData, std::unique_ptr<char[]>& postDataBytes,int& sizeNeededForMultiByte)
		{
			
			sizeNeededForMultiByte = WideCharToMultiByte(CP_UTF8, 0, postData.c_str(), int(postData.size()), nullptr, 0, nullptr, nullptr);
			if (sizeNeededForMultiByte > 0)
			{
				postDataBytes = std::make_unique<char[]>(sizeNeededForMultiByte);
				WideCharToMultiByte(CP_UTF8, 0, postData.c_str(), int(postData.size()), postDataBytes.get(), sizeNeededForMultiByte, nullptr, nullptr);
			}
			return S_OK;
		}
		HRESULT WebRequest(std::wstring uri, std::wstring verb, std::wstring data,std::wstring contenttype)
		{
			HRESULT hr = S_OK;
			bool readall = true;
			if (m_webView && m_webViewEnvironment)
			{
				std::unique_ptr<char[]> postDataBytes = nullptr;
				int sizeNeededForMultiByte = 0;

				if (!data.empty())
				{
					std::wstring postData = /*std::wstring(L"input=") + */data;
					hr = getpostData(postData, postDataBytes, sizeNeededForMultiByte);
				}

				if (SUCCEEDED(hr))
				{
					wil::com_ptr<ICoreWebView2Environment2> webviewEnvironment2;
					RETURN_IF_FAILED(m_webViewEnvironment->QueryInterface(IID_PPV_ARGS(&webviewEnvironment2)));
					wil::com_ptr<ICoreWebView2WebResourceRequest> webResourceRequest;
					wil::com_ptr<IStream> postDataStream = SHCreateMemStream(reinterpret_cast<const BYTE*>(postDataBytes.get()), sizeNeededForMultiByte);
					auto hr = webviewEnvironment2->CreateWebResourceRequest(uri.c_str(), verb.c_str(), postDataStream.get(), contenttype.c_str(), &webResourceRequest);
					wil::com_ptr<ICoreWebView2WebResourceRequest> WebRequest = webResourceRequest.get();				
					wil::com_ptr<ICoreWebView2_2> webview2Ex;
					RETURN_IF_FAILED(m_webView.get()->QueryInterface(IID_PPV_ARGS(&webview2Ex)));
					RETURN_IF_FAILED(webview2Ex->NavigateWithWebResourceRequest(webResourceRequest.get()));
				}
			}
			return hr;
		}
		std::wstring GetPreviewOfContent(IStream* content, bool& readAll)
		{
			char buffer[50];
			unsigned long read;
			content->Read(buffer, 50U, &read);
			readAll = read < 50;

			WCHAR converted[50];
			MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, buffer, 50, converted, 50);
			return std::wstring(converted);
		}

		/// <summary>
		/// Gets the handle to the WebView2 window.
		/// </summary>
		/// <returns>The handle to the WebView2 window.</returns>
		HWND get_hwnd()
		{
			HWND parent = nullptr;
			if (SUCCEEDED(m_controller->get_ParentWindow(&parent)))
			{
				ContextData context;

				EnumChildWindows(parent, EnumChildProc, (LPARAM)&context);

				return context.m_hwnd;
			}
			return nullptr;
		}
		/// <summary>
		/// Copies the selected content in the WebView2 control.
		/// </summary>
		void copy()
		{
			if (m_webView)
			{
				m_webView->ExecuteScript(L"document.execCommand(\"copy\")", nullptr);
			}		
		}
		/// <summary>
		/// Pastes the copied content into the WebView2 control.
		/// </summary>
		/// <param name="hwnd">The handle to the WebView2 window.</param>
		void paste(HWND hwnd)
		{
			if (m_webView)
			{
				m_webView->ExecuteScript(L"document.execCommand(\"paste\")", nullptr);
			}
		}
		/// <summary>
		/// Cuts the selected content in the WebView2 control.
		/// </summary>
		void cut()
		{
			if (m_webView)
			{
				m_webView->ExecuteScript(L"document.execCommand(\"cut\")", nullptr);
			}
		}
		#pragma endregion WebView2_event

		inline std::wstring ToReadableDateTime(const std::wstring& wtimestamp)
		{
			// Convert std::wstring to std::string (UTF-8) using WideCharToMultiByte
			int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wtimestamp.c_str(), -1, nullptr, 0, nullptr, nullptr);
			std::string timestamp_str;
			if (utf8Len > 0) {
				timestamp_str.resize(utf8Len - 1); // exclude null terminator
				WideCharToMultiByte(CP_UTF8, 0, wtimestamp.c_str(), -1, &timestamp_str[0], utf8Len, nullptr, nullptr);
			}
			else {
				return wtimestamp;
			}

			// Convert string to double
			double timestamp = 0.0;
			try {
				timestamp = std::stod(timestamp_str);
			}
			catch (...) {
				return wtimestamp;
			}

			// Separate into seconds and fractional part
			std::time_t seconds = static_cast<std::time_t>(timestamp);
			double fractional = timestamp - seconds;

			// Convert to UTC time
			std::tm tm{};
			gmtime_s(&tm, &seconds);

			// Format time into std::string
			std::ostringstream oss;
			oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
			oss << "." << std::setw(6) << std::setfill('0') << static_cast<int>(fractional * 1'000'000) << " UTC";
			std::string formatted_time_str = oss.str();

			// Convert std::string (UTF-8) to std::wstring using MultiByteToWideChar
			int wlen = MultiByteToWideChar(CP_UTF8, 0, formatted_time_str.c_str(), -1, nullptr, 0);
			std::wstring formatted_time;
			if (wlen > 0) {
				formatted_time.resize(wlen - 1); // exclude null terminator
				MultiByteToWideChar(CP_UTF8, 0, formatted_time_str.c_str(), -1, &formatted_time[0], wlen);
			}
			else {
				return wtimestamp;
			}

			return formatted_time;

		}

		HRESULT process_cookie_dev_tools(PCWSTR resultJson)
		{
			std::vector<Cookie> cookies;
			utility::stringstream_t jsonCookieArray;
			jsonCookieArray << resultJson;
			web::json::value jsonCookieArrayValue = web::json::value::parse(jsonCookieArray);
			if (jsonCookieArrayValue.is_object() == true)
			{
				for (auto iter = std::begin(jsonCookieArrayValue.as_object()); iter != std::end(jsonCookieArrayValue.as_object()); ++iter)
				{
					std::wstring name = iter->first;
					web::json::value cookiecoll = iter->second;
					if (cookiecoll.is_array())
					{
						for (auto& cookieVal : cookiecoll.as_array())
						{
							if (cookieVal.is_object())
							{
								Cookie cookie;
								cookie.name = cookieVal[NAME_PROP].as_string();
								cookie.value = cookieVal[VALUE_PROP].as_string();
								cookie.domain = cookieVal[DOMAIN_PROP].as_string();
								cookie.path = cookieVal[PATH_PROP].as_string();
								cookie.expires = cookieVal[EXPIRES_PROP].serialize();
								cookie.expiresReadable = ToReadableDateTime(cookieVal[EXPIRES_PROP].is_string()
									? cookieVal[EXPIRES_PROP].as_string()
									: cookieVal[EXPIRES_PROP].serialize());
								cookie.httpOnly = cookieVal.has_field(L"httpOnly") ? cookieVal[L"httpOnly"].as_bool() : false;
								cookie.secure = cookieVal.has_field(L"secure") ? cookieVal[L"secure"].as_bool() : false;
								// Add more fields as needed

								cookies.push_back(cookie);
								
								std::wstring s = std::format(
									L"domain:{0} expire:{1} expiresReadable:{2} path:{3} name={4} value={5} httpOnly={6} secure={7}",
									cookie.domain, cookie.expires, cookie.expiresReadable, cookie.path, cookie.name, cookie.value,
									cookie.httpOnly ? L"true" : L"false", cookie.secure ? L"true" : L"false");
								LOG_TRACE(WideToNarrow(s));
							}
						}
					}					
				}
			}
			if (m_webview2_events)
				m_webview2_events->raise_cookies_received_event(cookies);

			return S_OK;
		}
	HRESULT get_cookies_by_devtools()
	{
		LOG_TRACE(__FUNCTION__);
		RETURN_IF_NULL_ALLOC(m_webSettings);
			wil::com_ptr<ICoreWebView2DevToolsProtocolEventReceiver> receiver;
			RETURN_IF_FAILED(m_webView->GetDevToolsProtocolEventReceiver(L"Network.getAllCookies", &receiver));
			HRESULT hr = m_webView->CallDevToolsProtocolMethod(L"Network.getAllCookies", L"{}", Microsoft::WRL::Callback<ICoreWebView2CallDevToolsProtocolMethodCompletedHandler>([this](HRESULT error, PCWSTR resultJson) -> HRESULT
				{
					if (SUCCEEDED(error))
					{
						error = process_cookie_dev_tools(resultJson);
					}
					return error;
				}
			).Get());
			RETURN_IF_FAILED(hr);
			return S_OK;
		}

		// Implement IWebWiew2ImplEventCallback
		virtual HWND GetHWnd() override
		{
			return m_hwnd;
		}
		virtual void KeepAliveAsyncResult(std::future<void>&& result) override
		{
			// Need to keep alive future<void> instance until fire-and-forget async operation completes.
			// See Herb Sutter's paper: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3451.pdf
			std::lock_guard guard(m_asyncResultsMutex);
			m_asyncResults.push_back(std::move(result));
		}
	virtual void NavigationStartingEvent(std::wstring_view uri, unsigned long long navigationId, 
										 bool isRedirected, bool isUserInitiated) override
	{
		LOG_TRACE(__FUNCTION__);
		LOG_TRACE(std::string("  uri=") + WideToNarrow(std::wstring(uri)) + ", ID=" + std::to_string(navigationId) + 
				  ", redirected=" + std::to_string(isRedirected) + ", user initiated=" + std::to_string(isUserInitiated));
	}
		virtual void NavigationCompleteEvent(bool isSuccess, unsigned long long navigationId,
											 COREWEBVIEW2_WEB_ERROR_STATUS errorStatus) override
		{
		LOG_TRACE(__FUNCTION__);
		LOG_TRACE(std::string("  success=") + std::to_string(isSuccess) + ", ID=" + std::to_string(navigationId) +
				  ", error status=" + std::to_string(errorStatus));
			
			if (m_is_test == true && m_start == true)
			{
				T* pT = static_cast<T*>(this);
				if (::IsWindow(pT->m_hWnd))
				{
					m_start = false;
					std::cout << "WebView2 initialized" << std::endl;
				}
			}
		}
		virtual void ResponseReceivedEvent(std::wstring_view method, std::wstring_view uri) override
		{
		LOG_TRACE(__FUNCTION__);
		LOG_TRACE(std::string("  method=") + WideToNarrow(std::wstring(method)) + ", uri=" + WideToNarrow(std::wstring(uri)));
		}
		virtual void RequestEvent(std::wstring_view method, std::wstring_view uri,
								  COREWEBVIEW2_WEB_RESOURCE_CONTEXT resourceContext) override
		{
		LOG_TRACE(__FUNCTION__);
		LOG_TRACE(std::string("  method=") + WideToNarrow(std::wstring(method)) + ", uri=" + WideToNarrow(std::wstring(uri)) +
				  ", resource context=" + std::to_string(resourceContext));
		}
		virtual void ClientCertificateRequestedEvent(std::vector<ClientCertificate> client_certificates, wil::com_ptr<ICoreWebView2Deferral> deferral) override
		{
			LOG_TRACE(__FUNCTION__);
		}
		virtual void OnCookiesReceived(const std::vector<Cookie>& cookies) override
		{
			LOG_TRACE(__FUNCTION__);
		}
	private:

		/// <summary>
		/// Enumerates child windows and finds the window with the specified class name.
		/// </summary>
		/// <param name="hwnd">The handle to the parent window.</param>
		/// <param name="lParam">The pointer to the ContextData structure.</param>
		/// <returns>Returns TRUE if the window with the specified class name is found, otherwise FALSE.</returns>
		static BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
		{
			ContextData* pThis = (ContextData*)lParam;

			std::array<wchar_t, 1024> dataclassName;

			::GetClassName(hwnd, dataclassName.data(), (int)dataclassName.size());
			std::wstring classname(dataclassName.data());
			if (classname == pThis->m_classname)
			{
				pThis->m_hwnd = hwnd;
			}

			return TRUE;
		}

		HRESULT OnCreateCoreWebView2ControllerCompleted(HRESULT result, ICoreWebView2CompositionController* compositionController)
		{
			LOG_TRACE(__FUNCTION__);
			if (result != S_OK)
				return result;
			m_compositionController = compositionController;
			RETURN_IF_FAILED(m_compositionController->QueryInterface(IID_PPV_ARGS(&m_controller)));
			RETURN_IF_FAILED(m_controller->get_CoreWebView2(&m_webView));


			RETURN_IF_FAILED(m_webview2_events->initialize(this, m_webView, m_controller, m_hwnd_parent));
			RETURN_IF_FAILED(m_webview2_authentication_events->initialize(m_hwnd, m_webView, m_controller));
			RETURN_IF_FAILED((static_cast<T*>(this))->initialize(m_hwnd, m_controller, m_compositionController));


			if (m_webView != nullptr)
			{
				RETURN_IF_FAILED(m_webView->get_Settings(&m_webSettings));

				wil::com_ptr<ICoreWebView2_2> WebView2;
				RETURN_IF_FAILED(m_webView->QueryInterface(IID_PPV_ARGS(&WebView2)));
				RETURN_IF_FAILED(WebView2->get_CookieManager(&m_cookieManager));
			}

			CRect bounds;
			GetClientRect(m_hwnd , &bounds);

			RETURN_IF_FAILED(m_controller->put_IsVisible(true));		
			RETURN_IF_FAILED(m_webView->Navigate(m_url.c_str()));
			(static_cast<T*>(this))->put_bounds(bounds);

			return S_OK;
		}

		/// <summary>
		/// Initializes the WebView2 control.
		/// </summary>
		/// <param name="log">Indicates if logging is enabled.</param>
		/// <returns>The result of the initialization.</returns>
		HRESULT InitializeWebView(bool log=false)
		{
			LOG_TRACE(std::string(__FUNCTION__) + " Using browser directory:" + WideToNarrow(m_browser_directory));
			auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
			HRESULT hr = S_OK;
			std::wstring argbrowser;


			// enable single sign on with Azure Entra ID
			hr = options->put_AllowSingleSignOnUsingOSPrimaryAccount(TRUE);


			if (m_is_test == true)
			{
				m_port = m_port.empty() ? L"9222" : m_port;
				LOG_DEBUG(std::string("Start the WebView2 process with the Chrome DevTools Protocol enabled which allows the automation by Playwright. Port=") + WideToNarrow(m_port));
				argbrowser = L"--remote-debugging-port=" + m_port;
			}
			if (log == true)
			{
				fs::path unique_file;
				if (!Utility::GetUniqueLogFileName(unique_file))
				{
					LOG_DEBUG(std::string("Create unique log file for log-net-log filename: ") + unique_file.string());
					auto log = L"--log-net-log=" + unique_file.native();
					argbrowser += L" " + log;					
				}
				else
				{
					LOG_ERROR("Failed to create unique log file name for log-net-log");
				}
			}
			if (!argbrowser.empty())
			{
				hr = options->put_AdditionalBrowserArguments(argbrowser.c_str());
				RETURN_IF_FAILED(hr);
			}

			auto webViewEnvironment = SingleWebView2::get().get_webViewEnvironment();
			

			if (webViewEnvironment == nullptr)
			{

				hr = CreateCoreWebView2EnvironmentWithOptions(
					m_browser_directory.empty() ? nullptr : m_browser_directory.data(),
					m_user_data_directory.data(),
					options.Get(),
					Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>([this](
						HRESULT result, ICoreWebView2Environment* environment) -> HRESULT
						{
							HRESULT hr = S_OK;
							m_webViewEnvironment = environment;
							wil::com_ptr<ICoreWebView2Environment3> webViewEnvironment3 = m_webViewEnvironment.try_query<ICoreWebView2Environment3>();
							if (webViewEnvironment3)
							{
								auto hr = webViewEnvironment3->CreateCoreWebView2CompositionController(m_hwnd,
									Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler>(
										[this](HRESULT hr, ICoreWebView2CompositionController* compositionController) -> HRESULT
										{
											if (SUCCEEDED(hr)) {

												hr = OnCreateCoreWebView2ControllerCompleted(hr, compositionController);
											}
											else
												RETURN_IF_FAILED(hr);
											return hr;
										})
									.Get());
								if (SUCCEEDED(hr))
								{
									SingleWebView2::get().set_webViewEnvironment(m_webViewEnvironment);
								}
							}
							return hr;
						}).Get());
			}
			else
			{

				HRESULT hr = S_OK;
				m_webViewEnvironment = webViewEnvironment;
				wil::com_ptr<ICoreWebView2Environment3> webViewEnvironment3 = m_webViewEnvironment.try_query<ICoreWebView2Environment3>();
				if (webViewEnvironment3)
				{
					auto hr = webViewEnvironment3->CreateCoreWebView2CompositionController(m_hwnd,
						Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler>(
							[this](HRESULT hr, ICoreWebView2CompositionController* compositionController) -> HRESULT
							{
								if (SUCCEEDED(hr)) {

									hr = OnCreateCoreWebView2ControllerCompleted(hr, compositionController);
								}
								else
									RETURN_IF_FAILED(hr);
								return hr;
							})
						.Get());
				}
				return hr;
			}
			return hr;
		}
	

};
}