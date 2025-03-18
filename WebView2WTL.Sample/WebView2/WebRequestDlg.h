#pragma once

#include "LogView.h"
#include "resource.h"

class CComboBoxImpl : public CWindowImpl<CComboBoxImpl, CComboBox>
{
	BEGIN_MSG_MAP_EX(CComboBoxImpl)
	END_MSG_MAP()
};

class CWebRequestDlg : public CDialogImpl<CWebRequestDlg>,
					   public CWinDataExchange<CWebRequestDlg>
{
public:
	enum { IDD = IDD_WEBREQUEST };

	BEGIN_DDX_MAP(CWebRequestDlg)
		DDX_TEXT(IDC_EDIT_URL, m_url)
		DDX_TEXT(IDC_EDIT_POST, m_data)
		DDX_CONTROL(IDC_COMBO_VERB, m_verbs)

	END_DDX_MAP()

	BEGIN_MSG_MAP(CWebRequestDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	std::wstring get_verb();
	std::wstring get_uri();
	std::wstring get_data();

protected:
	CComboBoxImpl	m_verbs;
	ATL::CString	m_url;
	ATL::CString	m_data;

private:
	std::wstring m_selected_verb;
	std::wstring m_selected_url;
	std::wstring m_selected_data;
};


