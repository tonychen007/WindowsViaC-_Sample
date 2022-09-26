
// 15.VirtualAllocDlg.h: 標頭檔
//

#pragma once

#define STOCK_WHITE 0
#define STOCK_GRAY 2
#define STOCK_BLACK 4

// CMy15VirtualAllocDlg 對話方塊
class CVirtualAllocDlg : public CDialogEx
{
// 建構
public:
	CVirtualAllocDlg(CWnd* pParent = nullptr);	// 標準建構函式

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支援


// 程式碼實作
protected:
	HICON m_hIcon;

	// 產生的訊息對應函式
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedReserveMem();
	afx_msg void OnBnClickedCommitMem();
	afx_msg void OnBnClickedDecommitMem();
	afx_msg void OnBnClickedReleaseMem();
	afx_msg void OnEnChangeEditIndex();
	DECLARE_MESSAGE_MAP()

private:
	void InitSystemInfo();
private:
	int m_pageSize;
	int m_maxPage;
	int m_MemMapWidth;
	RECT m_rcMemMap;
	LPVOID m_someData;
public:

};
