#include "pch.h"
#include "resource.h"
#include "EdgeInfomation.h"
#include "Utility.h"
#include "DetectDlg.h"

LRESULT CDetectDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// load ddx values
	

	auto p = GetDlgItem(IDC_COMBO_EDGE_VERSION);
	if (!p)
	{
		return 0;
	}
	CComboBox* pComPortSelection = reinterpret_cast<CComboBox*>(&p);
	
	m_edge_versions = WebView2::Utility::EnumEdgeVersion();

	for (auto it = m_edge_versions.begin(); it != m_edge_versions.end(); ++it)
	{
		pComPortSelection->AddString(it->m_name.c_str());
	}

	if (!m_edge_versions.empty())
	{
		pComPortSelection->SetCurSel(0);
	}
	CString str;
	pComPortSelection->GetWindowTextW(str);
	std::wstring selected_version = str.GetString();



	for (auto it = m_edge_versions.begin(); it != m_edge_versions.end(); ++it)
	{
		if (wcscmp(it->m_name.c_str(), selected_version.c_str()) == 0)
		{
			m_information.Format(L"Name: %s\r\nVersion: %s\r\nInstall Location: %s", it->m_name.c_str(), it->m_version.c_str(), it->m_install_location.c_str());
			DoDataExchange(FALSE);
			break;
		}
	}
	CenterWindow(GetParent());
	return TRUE;
}

LRESULT CDetectDlg::OnCbnSelChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	auto p = GetDlgItem(IDC_COMBO_EDGE_VERSION);
	if (!p)
	{
		return 0;
	}
	CComboBox* pComPortSelection = reinterpret_cast<CComboBox*>(&p);

	// Get the selected item use  CSimpleString
	CString str;
	pComPortSelection->GetWindowTextW(str);
	std::wstring selected_version = str.GetString();

	for (auto it = m_edge_versions.begin(); it != m_edge_versions.end(); ++it)
	{
		if (wcscmp(it->m_name.c_str(), selected_version.c_str()) == 0)
		{
			m_information.Format(L"Name: %s\r\nVersion: %s\r\nInstall Location: %s", it->m_name.c_str(), it->m_version.c_str(), it->m_install_location.c_str());
			DoDataExchange(FALSE);
			break;
		}
	}
	return 0;
}

LRESULT CDetectDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}


