#pragma once

// DetectDlg.h : interface of the CDetectDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once



class CDetectDlg : public CDialogImpl<CDetectDlg>,
	public CWinDataExchange<CDetectDlg>
{
public:
	enum { IDD = IDD_DIALOG_DETECT};

	BEGIN_DDX_MAP(CCertificateDlg)
		DDX_TEXT(IDC_EDIT_INFO, m_information)
	END_DDX_MAP()

	// IDC_STATIC_EDGEINFORMATION

	BEGIN_MSG_MAP(CDetectDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_HANDLER(IDC_COMBO_EDGE_VERSION, CBN_SELCHANGE, OnCbnSelChange)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCbnSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
	void GetVersionFromRegistry(HKEY key, std::wstring key_entry);
	void GetVersion();


protected:
	CComboBox _ComboEdgeVersion;

private:
	std::list<EdgeInfomation> m_edge_versions;
	ATL::CString	m_information;
	
};
