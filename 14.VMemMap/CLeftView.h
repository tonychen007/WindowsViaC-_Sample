#pragma once


// CLeftView 檢視
#include "framework.h"
#include <map>
#include <string>

#define MEM_INFO_COL 4
#define MEM_STAT 0
#define MEM_REG_SIZE 1
#define MEM_NUM_BLOCK 2
#define MEM_MODULE 3

using memoryMap = std::map<std::wstring, std::wstring[MEM_INFO_COL]>;
using memoryMapItr = std::map<std::wstring, std::wstring[MEM_INFO_COL]>::iterator;

class CVMemMapDoc;

class CLeftView : public CTreeView {
	DECLARE_DYNCREATE(CLeftView)

protected:
	CLeftView();           // 動態建立所使用的保護建構函式
	virtual ~CLeftView();

public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	CVMemMapDoc* GetDocument();
	void PopulateTree();
	void GetVirtualMemoryInfo(CTreeCtrl &tc);


#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif

protected:
	CImageList m_ImageList;
	memoryMap m_memInfo;
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTvnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
};


