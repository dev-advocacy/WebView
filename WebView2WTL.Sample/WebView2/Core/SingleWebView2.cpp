#include "pch.h"
#include "SingleWebView2.h"

namespace WebView2::Core
{

wil::com_ptr<ICoreWebView2Environment> SingleWebView2::get_webViewEnvironment()
{
	return m_webViewEnvironment;
}

void SingleWebView2::set_webViewEnvironment(wil::com_ptr<ICoreWebView2Environment> CoreWebView2Environment)
{    
	m_webViewEnvironment = CoreWebView2Environment;
}

} // namespace WebView2::Core