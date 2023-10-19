
// ThreadScheduleDlg.h: 標頭檔
//

#pragma once

#include <vector>

#define SLEEP_MAX 1000

// CThreadScheduleDlg 對話方塊
class CThreadScheduleDlg : public CDialog
{
// 建構
public:
	CThreadScheduleDlg(CWnd* pParent = nullptr);	// 標準建構函式

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_THREADSCHEDULE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支援


// 程式碼實作
protected:
	HICON m_hIcon;

	// 產生的訊息對應函式
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	void InitDropListControl();
	void InitThread();
	afx_msg void OnEnChangeSleepEdit();
	afx_msg void OnCbnSelchangeProcPri();
	afx_msg void OnCbnSelchangeThreadPri();

	static DWORD WINAPI ThreadFunc(LPVOID args);
	static DWORD WINAPI ThreadUIFunc(LPVOID args);

	DECLARE_MESSAGE_MAP()

private:
	struct threadData {
		int idx;
		CListCtrl* m_procList;
	};

	CComboBox* m_procBox;
	CComboBox* m_threadBox;
	CEdit* m_sleepEdit;
	CListCtrl* m_procList;
	int m_procLastPri;
	int m_threadLastPri;
	std::vector<HANDLE> m_threadHandles;

	int m_SleepTime;
};
