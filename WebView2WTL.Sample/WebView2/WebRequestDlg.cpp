#include "pch.h"
#include "resource.h"
#include "WebRequestDlg.h"


//GET, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, PATCH, LINK, UNLINK

/// <summary>
///  
/// </summary>
/// <param name=""></param>
/// <param name=""></param>
/// <param name=""></param>
/// <param name=""></param>
/// <returns></returns>
LRESULT CWebRequestDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow(GetParent());

	m_verbs.SubclassWindow(GetDlgItem(IDC_COMBO_VERB));

	m_verbs.AddString(L"GET");
	m_verbs.AddString(L"POST");
	DoDataExchange(false);

	return TRUE;
}

LRESULT CWebRequestDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	DoDataExchange(true);

	TCHAR szBuff[MAX_PATH] = { 0 };
	
	m_verbs.GetWindowText(szBuff, MAX_PATH);
	m_selected_verb = szBuff;

	m_selected_url = m_url;
	m_selected_data = m_data;

	EndDialog(wID);
	return 0;
}

std::wstring CWebRequestDlg::get_verb()
{
	return m_selected_verb;
}

std::wstring CWebRequestDlg::get_uri()
{
	return m_selected_url;
}

std::wstring CWebRequestDlg::get_data()
{
	return m_selected_data;
}