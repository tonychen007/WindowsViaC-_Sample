﻿
// ProcessGUIDlg.h: 標頭檔
//

#pragma once

#include <map>
#include <string>

#include "CListCtrlEdit.h"

// CProcessGUIDlg 對話方塊
class CProcessGUIDlg : public CDialog
{
// 建構
public:
	CProcessGUIDlg(CWnd* pParent = nullptr);	// 標準建構函式

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROCESSGUI_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支援

// 程式碼實作
protected:
	HICON m_hIcon;

	// 產生的訊息對應函式
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg HCURSOR OnQueryDragIcon();
	void InitEnvListControl();
	void InitDropListControl();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnGetdispinfoEnvList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedGetEnvVarBtn();
	afx_msg void OnBnClickedEnumProcess();
	afx_msg void OnBnClickedEnumModule();

	void AddText(PCTSTR pszFormat, ...);
	void EnumProcess();
	void EnumModule();
	void ShowProcessInfo(DWORD dwPid);
	void ShowModuleInfo(LPCWSTR pzName);
	afx_msg void OnSelchangeDroplist();

	DECLARE_MESSAGE_MAP()

public:
	using StringDict = std::map<std::string, std::string>;
	using StringDictItr = std::map<std::string, std::string>::const_iterator;

	CListCtrlEdit m_listCtrl;
	StringDict m_envDict;

	CComboBox* m_dropList;
	CEdit* m_infoEdit;

	std::wstring m_str;
	int m_isModule;

	CPoint m_oldWinPos;
};
