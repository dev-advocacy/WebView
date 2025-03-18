// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once


#include "WebView2.h"
#include "WebViewProfile.h"
#include "WebViewDlg.h"
#include "WebView2Impl2.h"
#include "UrlCombo.h"

class CMainFrame : public CFrameWindowImpl<CMainFrame>, public CUpdateUI<CMainFrame>,public CMessageFilter, public CIdleHandler
{
public:
	DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)

	CCommandBarCtrl						m_CmdBar;
	std::unique_ptr <CWebView2>			m_webview2 = nullptr;	
	std::unique_ptr<CDlgWebView2>		m_dlgWebWiew2Modeless = nullptr;	
	ProfileInformation_t				m_webviewprofile;
	CURLCombo							m_wndCombo;
	
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();
	HWND CreateAddressBarCtrl(HWND hWndParent);

	BEGIN_UPDATE_UI_MAP(CMainFrame)
		UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)

		// Command menu
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_FILE_NEW, OnFileNew)
		COMMAND_ID_HANDLER(ID_FILE_NEW_WINDOW, OnFileNewWindow)
		COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)

		COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCopy)
		COMMAND_ID_HANDLER(ID_EDIT_PASTE, OnEditPaste)
		COMMAND_ID_HANDLER(ID_EDIT_CUT, OnEditCut)

		COMMAND_ID_HANDLER(ID_SCENARIO_MODALDIALOG, OnScenarioWebView2Modal)
		COMMAND_ID_HANDLER(ID_SCENARIO_MODELELESSDIALOG, OnScenarioWebView2Modeless)
		COMMAND_ID_HANDLER(ID_SCENARIO_WEBRESOURCEREQUEST, OnScenarioWebRequest)

		COMMAND_ID_HANDLER(ID_SCENARIO_INSTALLATION, OnScenarioInstallation)

		COMMAND_ID_HANDLER(ID_SCENARIO_DETECT, OnScenarioDetect)
		

		MESSAGE_HANDLER(MSG_NAVIGATE_CALLBACK, OnNavigate)
		
		

		REFLECT_NOTIFICATIONS()
		CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	// Command menu
	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileNewWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditPaste(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditCut(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);	

	LRESULT OnScenarioWebView2Modal(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnScenarioWebView2Modeless(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnNavigate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnScenarioInstallation(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnScenarioWebRequest(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnScenarioDetect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	
private:
	std::wstring _webview_version;
	
};
