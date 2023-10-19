#pragma once


// CListCtrlEdit
#include "framework.h"

#define TONY_EDIT		10000

class CListCtrlEdit : public CListCtrl
{
	DECLARE_DYNAMIC(CListCtrlEdit)

public:
	CListCtrlEdit();
	virtual ~CListCtrlEdit();

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnNMClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnEdtKillFocus();

	void ClearEditBox();

protected:
	DECLARE_MESSAGE_MAP()

private:
	CEdit m_edit;
	CToolTipCtrl m_listCtrlTips;
	int m_hoverRow;
	int m_hoverCol;
	int m_curRow;
	int m_curCol;

public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnLvnColumnclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnBeginScroll(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHdnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHdnItemchanging(NMHDR* pNMHDR, LRESULT* pResult);
};


