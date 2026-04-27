#pragma once

#include "LogView.h"
#include "osutility.h"
#include "resource.h"

class CDomainDlg : public CDialogImpl<CDomainDlg>,
	public CWinDataExchange<CDomainDlg>
{
public:
	enum { IDD = IDD_DIALOG_COOKIE_DOMAIN};

	BEGIN_DDX_MAP(CDomainDlg)
		DDX_TEXT(IDC_EDIT_DOMAIN, m_domain)
	END_DDX_MAP()

	BEGIN_MSG_MAP(CCertificateDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		DEFAULT_REFLECTION_HANDLER()
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCtrlColor(UINT, WPARAM wParam, LPARAM, BOOL&);
	LRESULT OnSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);

	std::wstring GetDomain();

private:
	CString m_domain;
};