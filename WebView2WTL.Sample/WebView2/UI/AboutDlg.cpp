// aboutdlg.cpp : implementation of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "resource.h"
#include "aboutdlg.h"

LRESULT CAboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// load ddx values
	DoDataExchange(FALSE);
	CenterWindow(GetParent());
	return TRUE;
}

LRESULT CAboutDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}
void CAboutDlg::InitDDX(std::wstring webview_version, std::wstring browserDirectory, std::wstring userDataDirectory, std::wstring channel)
{
	m_webview_channel = channel.c_str();
	m_webview_version = webview_version.c_str();
	m_webview_browserDirectory = browserDirectory.c_str();
	m_webview_userDataDirectory = userDataDirectory.c_str();
}

LRESULT CAboutDlg::OnStnClickedStaticVersion2(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	return 0;
}
