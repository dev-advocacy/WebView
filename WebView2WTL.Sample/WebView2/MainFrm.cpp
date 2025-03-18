// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "resource.h"
#include "aboutdlg.h"
#include "MainFrm.h"
#include "WebView2Impl2.h"
#include "CertificateDlg.h"
#include "WebRequestDlg.h"
#include "DetectDlg.h"

#include "Utility.h"

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if (CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;
	if (m_wndCombo.PreTranslateMessage(pMsg))
		return TRUE;
	return m_webview2->PreTranslateMessage(pMsg);
	return FALSE;
}

BOOL CMainFrame::OnIdle()
{
	UIUpdateToolBar();
	return FALSE;
}

HWND CMainFrame::CreateAddressBarCtrl(HWND hWndParent)
{
	RECT rc = { 50, 0, 300, 100 };
	THROW_LAST_ERROR_IF_NULL(m_wndCombo.Create(hWndParent, rc, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBS_DROPDOWN | CBS_AUTOHSCROLL | CBS_SORT));
	m_wndCombo.SetFrame(this->m_hWnd);
	return m_wndCombo;
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// create command bar window
	HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, nullptr, ATL_SIMPLE_CMDBAR_PANE_STYLE);
	HWND hWndAddressBar = CreateAddressBarCtrl(m_hWnd);
	// attach menu
	m_CmdBar.AttachMenu(GetMenu());
	// load command bar images
	m_CmdBar.LoadImages(IDR_MAINFRAME);
	// remove old menu
	SetMenu(nullptr);

	HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndCmdBar);
	AddSimpleReBarBand(hWndToolBar, nullptr, TRUE);
	AddSimpleReBarBand(hWndAddressBar, _T("Address"), TRUE);

	CreateSimpleStatusBar();

	
	
	HRESULT hr = CWebViewProfile::Profile(m_webviewprofile);
	if (FAILED(hr))
	{
		
		return 0;
	}

	std::wstring version = m_webviewprofile.version;
	if (version.empty())
		this->MessageBoxW(L"Please install the WebView2 Runtime: menu Scenario/Installation",L"Warning", MB_OK | MB_ICONWARNING);

	_webview_version = version;

	m_webview2 = std::make_unique<CWebView2>(m_webviewprofile.browserDirectory, m_webviewprofile.userDataDirectory, L"https://msdn.microsoft.com");
	m_hWndClient = m_webview2->Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);


	m_webview2->set_parent(this->m_hWnd);

	UIAddToolBar(hWndToolBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != nullptr);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	return 0;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != nullptr);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	bHandled = FALSE;
	return 1;
}


LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: add code to initialize document

	return 0;
}

LRESULT CMainFrame::OnFileNewWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	::PostThreadMessage(_Module.m_dwMainThreadID, WM_USER, 0, 0L);
	return 0;
}

LRESULT CMainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;

	

	dlg.InitDDX(_webview_version, m_webviewprofile.browserDirectory, m_webviewprofile.userDataDirectory, m_webviewprofile.channel);
	dlg.DoModal();
	return 0;
}

LRESULT CMainFrame::OnScenarioWebView2Modal(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CDlgWebView2 dlg(m_webviewprofile.browserDirectory, m_webviewprofile.userDataDirectory, L"https://www.msdn.microsoft.com");
	dlg.DoModal();
	return 0;
}

//setup.exe(under Webview installation directory) with following targets.
//--force - uninstall
//--uninstall
//--msedgewebview
//--system - level
//setup.exe --uninstall --msedgewebview --system-level --verbose-logging --force-uninstall

LRESULT CMainFrame::OnScenarioWebView2Modeless(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (m_dlgWebWiew2Modeless == nullptr)
		m_dlgWebWiew2Modeless = std::make_unique<CDlgWebView2>(m_webviewprofile.browserDirectory, m_webviewprofile.userDataDirectory, L"https://msdn.microsoft.com");
	if ((m_dlgWebWiew2Modeless != nullptr) && !::IsWindow(m_dlgWebWiew2Modeless->m_hWnd))
	{
		m_dlgWebWiew2Modeless->put_modeless(true);
		m_dlgWebWiew2Modeless->Create(this->m_hWnd);
		m_dlgWebWiew2Modeless->ShowWindow(SW_SHOW);
	}
	return 0;
}

LRESULT CMainFrame::OnScenarioWebRequest(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CWebRequestDlg webrequestdlg;
	auto ret = webrequestdlg.DoModal();
	if (ret == TRUE)
	{

		auto hr = m_webview2->WebRequest(webrequestdlg.get_uri(), webrequestdlg.get_verb(), webrequestdlg.get_data(), L"Content-Type: application/json");
		if (FAILED(hr))
		{
			LOG_TRACE << "The WebView2 WebRequest failed | " << "message" << ": 0x" << std::hex << std::setw(8) << hr;			
		}
	}
	return 0;
}

LRESULT CMainFrame::OnScenarioInstallation(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	constexpr size_t MaxMessageLength = 1024;
	std::wstring message(MaxMessageLength, L'\0');
	std::wstring version = WebView2::Utility::GetWebView2Version();
	
	if (!version.empty())
	{	//TODO: Replace with std::format when C++20 is enabled.
		_snwprintf_s(message.data(), message.size(), _TRUNCATE,
			L"WebView2 version %s is already installed.", version.c_str());
		this->MessageBoxW(message.c_str(), L"Information", MB_OK | MB_ICONINFORMATION);
		return 0;
	}
	
	// Download WebView2 boostrapper from the web.
	std::wstring path;
	HRESULT hr = WebView2::Utility::DownloadWebView2Bootstrapper(path);
	
	if FAILED(hr)
	{   //TODO: Replace with std::format when C++20 is enabled.
		_snwprintf_s(message.data(), message.size(), _TRUNCATE,
			L"Failed to download the latest WebView2 version. Error code: 0x%08X", hr);
		this->MessageBoxW(message.c_str(), L"Error", MB_OK | MB_ICONERROR);
		return 0;
	}
		
	// Install WebView2 in per-user mode.
	hr = WebView2::Utility::InstallWebView2(path, /*elevated*/ false);
		
	if FAILED(hr)
	{	//TODO: Replace with std::format when C++20 is enabled.
		_snwprintf_s(message.data(), message.size(), _TRUNCATE,
			L"Failed to install the latest WebView2 version. Error code: 0x%08X", hr);
		this->MessageBoxW(message.c_str(), L"Error", MB_OK | MB_ICONERROR);
		return 0;
	}		
	this->MessageBoxW(L"Successfully installed the latest WebView2 version. "
		L"Restart the application to refresh.",
		L"Success", MB_OK | MB_ICONINFORMATION);
	return 0;
}

LRESULT CMainFrame::OnNavigate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	std::wstring text_url = reinterpret_cast<wchar_t*>(lParam);

	if (!text_url.empty())
	{		
		m_webview2->navigate(text_url);
	}
	return 0;
}

LRESULT CMainFrame::OnEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	HWND hwnd = GetFocus();
	CEdit edit = m_wndCombo.GetEditCtrl();
	auto webview_hwnd = m_webview2->get_hwnd();
	if (hwnd == edit.m_hWnd)
	{
		m_wndCombo.Copy();
	}
	else if (webview_hwnd == hwnd)
	{
		m_webview2->copy();
	}
	return 0L;
}
LRESULT CMainFrame::OnEditPaste(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	HWND hwnd = GetFocus();
	CEdit edit = m_wndCombo.GetEditCtrl();

	auto webview_hwnd = m_webview2->get_hwnd();
	if (hwnd == edit.m_hWnd)
	{
		m_wndCombo.Paste();
	}
	else if (webview_hwnd == hwnd)
	{
		m_webview2->paste(webview_hwnd);
	}
	return 0L;
}
LRESULT CMainFrame::OnEditCut(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	HWND hwnd = GetFocus();
	CEdit edit = m_wndCombo.GetEditCtrl();

	auto webview_hwnd = m_webview2->get_hwnd();
	if (hwnd == edit.m_hWnd)
	{
		m_wndCombo.Cut();
	}
	else if (webview_hwnd == hwnd)
	{
		m_webview2->cut();
	}
	return 0L;
}

LRESULT CMainFrame::OnScenarioDetect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CDetectDlg detectdlg;
	auto ret = detectdlg.DoModal();
	return 0L;
}

