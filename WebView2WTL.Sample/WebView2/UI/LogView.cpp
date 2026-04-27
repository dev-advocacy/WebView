#include "pch.h"
#include "LogView.h"
#include "logger.h"

CLogView::CLogView()
{
}

void CLogView::DeleteItem(DELETEITEMSTRUCT* lParam)
{
    COwnerDraw<CLogView>::DeleteItem(lParam);
}

void CLogView::SetFont(HFONT hFont)
{
    CListViewCtrl::SetFont(hFont);
    GetHeader().Invalidate();

    // Trigger WM_MEASUREPOS
    // See: http://www.codeproject.com/Articles/1401/Changing-Row-Height-in-an-owner-drawn-Control
    CRect rect;
    GetWindowRect(&rect);
    WINDOWPOS wp;
    wp.hwnd = *this;
    wp.cx = rect.Width();
    wp.cy = rect.Height();
    wp.flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER;
    SendMessage(WM_WINDOWPOSCHANGED, 0, reinterpret_cast<LPARAM>(&wp));
}

void CLogView::Create(HWND hWndParent, ATL::_U_RECT rect)
{
    __super::Create(hWndParent, rect, CListViewCtrl::GetWndClassName(), WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL, WS_EX_CLIENTEDGE);
}

void CLogView::MeasureItem(MEASUREITEMSTRUCT* pMeasureItemStruct)
{
    LOG_DEBUG("CLogView::MeasureItem - debug");
    CClientDC dc(*this);
}

/// <summary>
/// 
/// </summary>
/// <param name="iItem"></param>
/// <param name="code"></param>
/// <returns></returns>
RECT CLogView::GetItemRect(int iItem, unsigned code) const
{
    RECT rect;
    CListViewCtrl::GetItemRect(iItem, &rect, code);
    return rect;
}

/// <summary>
/// 
/// </summary>
/// <param name="pDrawItemStruct"></param>
void CLogView::DrawItem(DRAWITEMSTRUCT* pDrawItemStruct)
{
    LOG_DEBUG("CLogView::DrawItem - debug");
    DrawItem(pDrawItemStruct->hDC, pDrawItemStruct->itemID, pDrawItemStruct->itemState);
}

/// <summary>
/// 
/// </summary>
/// <param name="dc"></param>
/// <param name="iItem"></param>
/// <param name=""></param>
void CLogView::DrawItem(CDCHandle dc, int iItem, unsigned /*iItemState*/)
{
    auto rect = GetItemRect(iItem, LVIR_BOUNDS);

    bool selected = GetItemState(iItem, LVIS_SELECTED) == LVIS_SELECTED;
    bool focused = GetItemState(iItem, LVIS_FOCUSED) == LVIS_FOCUSED;
    auto bkColor = selected ? RGB(255, 200, 200) : RGB(255, 255, 255);
    auto txColor = RGB(0, 0, 0);

    dc.FillSolidRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, bkColor);
}