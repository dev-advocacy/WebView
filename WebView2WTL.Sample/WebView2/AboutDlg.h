// aboutdlg.h : interface of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CAboutDlg :	public CDialogImpl<CAboutDlg>,
					public CWinDataExchange<CAboutDlg>
{
public:
	enum { IDD = IDD_ABOUTBOX };

	BEGIN_DDX_MAP(CCertificateDlg)
		DDX_TEXT(IDC_STATIC_VERSION_INFO, m_webview_version)
		DDX_TEXT(IDC_STATIC_BROWSER_DIRECTORY, m_webview_browserDirectory)
		DDX_TEXT(IDC_STATIC_BROWSER_USER_DATA, m_webview_userDataDirectory)
		DDX_TEXT(IDC_STATIC_CHANNEL, m_webview_channel)		
	END_DDX_MAP()

	BEGIN_MSG_MAP(CAboutDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_HANDLER(IDC_STATIC_VERSION2, STN_CLICKED, OnStnClickedStaticVersion2)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

public:
	void InitDDX(std::wstring webview_version, std::wstring browserDirectory, std::wstring userDataDirectory, std::wstring channel);
private:
	ATL::CString	m_webview_version;
	ATL::CString	m_webview_browserDirectory;
	ATL::CString	m_webview_userDataDirectory;
	ATL::CString	m_webview_channel;
public:
	LRESULT OnStnClickedStaticVersion2(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};
