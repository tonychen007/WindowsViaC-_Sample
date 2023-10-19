
#include "CListCtrlEdit.h"

IMPLEMENT_DYNAMIC(CListCtrlEdit, CListCtrl)

CListCtrlEdit::CListCtrlEdit():
	m_hoverRow(-1),
	m_hoverCol(-1),
	m_curRow(-1),
	m_curCol(-1) {

	EnableToolTips(TRUE);

	m_listCtrlTips.Create(this, TTS_ALWAYSTIP);
	m_listCtrlTips.Activate(TRUE);
	m_listCtrlTips.SetDelayTime(TTDT_INITIAL, 500);
}

CListCtrlEdit::~CListCtrlEdit() {}

BEGIN_MESSAGE_MAP(CListCtrlEdit, CListCtrl)
	ON_WM_MOUSEMOVE()
	ON_NOTIFY_REFLECT(NM_CLICK, &CListCtrlEdit::OnNMClick)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CListCtrlEdit::OnLvnColumnclick)
	ON_NOTIFY_REFLECT(LVN_BEGINSCROLL, &CListCtrlEdit::OnLvnBeginScroll)
	ON_NOTIFY(HDN_ITEMCHANGING, 0, &CListCtrlEdit::OnHdnItemchanging)

	ON_EN_KILLFOCUS(TONY_EDIT, &CListCtrlEdit::OnEnEdtKillFocus)
END_MESSAGE_MAP()

BOOL CListCtrlEdit::PreTranslateMessage(MSG* pMsg) {
	if (pMsg->message == WM_LBUTTONDOWN ||
		pMsg->message == WM_LBUTTONUP ||
		pMsg->message == WM_MOUSEMOVE) {

		m_listCtrlTips.RelayEvent(pMsg);
	}

	return CListCtrl::PreTranslateMessage(pMsg);
}

void CListCtrlEdit::OnMouseMove(UINT nFlags, CPoint point) {

	CString str;
	LVHITTESTINFO hit;
	hit.pt = point;
	int ret = SubItemHitTest(&hit);

	if ((hit.iItem != m_hoverRow || hit.iSubItem != m_hoverCol)) {
		m_hoverRow = hit.iItem;
		m_hoverCol = hit.iSubItem;

		if (m_hoverRow != -1 && m_hoverCol != -1) {
			str = GetItemText(hit.iItem, hit.iSubItem);
			m_listCtrlTips.AddTool(this, str);
			m_listCtrlTips.Pop();
		}
	}

	CListCtrl::OnMouseMove(nFlags, point);
}

void CListCtrlEdit::OnNMClick(NMHDR* pNMHDR, LRESULT* pResult) {
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	int ret;
	LVHITTESTINFO hit;
	hit.pt = pNMItemActivate->ptAction;

	ret = SubItemHitTest(&hit);
	if (ret == -1)
		return;

	if ((hit.flags & LVHT_ONITEMLABEL) == 0)
		return;

	m_curRow = hit.iItem;
	m_curCol = hit.iSubItem;

	CRect rect;
	CDC* pDC = GetDC();
	TEXTMETRIC tm;

	GetSubItemRect(m_curRow, m_curCol, LVIR_LABEL, rect);
	CString str = GetItemText(m_curRow, m_curCol);
	int len = str.GetLength();
	GetTextMetrics(*pDC, &tm);

	// dynamic create edit box
	if (!m_edit.m_hWnd) {
		m_edit.Create(WS_CHILD | ES_MULTILINE | WS_BORDER, rect, this, TONY_EDIT);
		m_edit.SetFont(this->GetFont(), TRUE);
	}

	// cacl width to check if need multiline rect
	int textWidth = tm.tmAveCharWidth * len * 2;
	int rectWidth = rect.right - rect.left;

	if (textWidth > rectWidth) {
		int line = textWidth / rectWidth + 1;
		rect.bottom += (rect.bottom - rect.top) * line;
	}

	m_edit.SetWindowText(str);
	m_edit.MoveWindow(rect);
	m_edit.ShowWindow(SW_SHOW);
	m_edit.SetSel(0, str.GetLength(), 0);
	m_edit.SetFocus();
}

void CListCtrlEdit::OnEnEdtKillFocus() {
	ClearEditBox();
}

void CListCtrlEdit::ClearEditBox() {
	if (m_edit)
		m_edit.ShowWindow(SW_HIDE);
}

void CListCtrlEdit::OnLvnColumnclick(NMHDR* pNMHDR, LRESULT* pResult) {
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	ClearEditBox();
	*pResult = 0;
}

void CListCtrlEdit::OnLvnBeginScroll(NMHDR* pNMHDR, LRESULT* pResult) {
	LPNMLVSCROLL pStateChanged = reinterpret_cast<LPNMLVSCROLL>(pNMHDR);

	ClearEditBox();
	*pResult = 0;
}

void CListCtrlEdit::OnHdnItemchanging(NMHDR* pNMHDR, LRESULT* pResult) {
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	ClearEditBox();
	*pResult = 0;
}
