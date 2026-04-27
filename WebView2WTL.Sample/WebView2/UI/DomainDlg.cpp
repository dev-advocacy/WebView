#include "pch.h"
#include "DomainDlg.h"


LRESULT CDomainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// load ddx values
	DoDataExchange(FALSE);
	CenterWindow(GetParent());
	return TRUE;
}

LRESULT CDomainDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (wID == IDOK) {
		DoDataExchange(TRUE);
	}
	EndDialog(wID);
	return 0;
}

std::wstring CDomainDlg::GetDomain()
{
	return m_domain.GetBuffer();
}