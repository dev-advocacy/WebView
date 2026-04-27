// View.cpp : implementation of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "resource.h"

#include "WebView2Wnd.h"
#include "logger.h"

/// <summary>
/// TODO : validate parameters
/// </summary>
/// <param name="browerdirectory"></param>
/// <param name="userdatedirectory"></param>
/// <param name="url"></param>
WebView2::Core::CWebView2::CWebView2(std::wstring browerdirectory, std::wstring userdatedirectory, std::wstring url)
{
	m_user_data_directory = userdatedirectory;
	m_url = url;
	m_browser_directory = browerdirectory;
}
WebView2::Core::CWebView2::~CWebView2()
{

}
LRESULT WebView2::Core::CWebView2::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return WebView2::Core::CWebView2Impl2<CWebView2>::OnCreate(uMsg, wParam, lParam, bHandled);
}

BOOL WebView2::Core::CWebView2::PreTranslateMessage(MSG* pMsg)
{
	//if (CWebView2Impl2<CWebView2>::PreTranslateMessage(pMsg))
		//return TRUE;

	return 0L;
}
LRESULT WebView2::Core::CWebView2::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CPaintDC dc(m_hWnd);
	return 0L;
}
void WebView2::Core::CWebView2::CreationCompleted()
{
	LOG_TRACE(__FUNCTION__);
}
void WebView2::Core::CWebView2::NavigationCompleted(std::wstring url)
{
	LOG_TRACE(std::string(__FUNCTION__) + " url=" + WideToNarrow(url));
}
void WebView2::Core::CWebView2::AuthenticationCompleted()
{
	LOG_TRACE(__FUNCTION__);
}
