
// ThreadQueueDlg.h: 標頭檔
//

#pragma once

#include <queue>

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.6000.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")

// CThreadQueueDlg 對話方塊
class CThreadQueueDlg : public CDialogEx
{
// 建構
public:
	CThreadQueueDlg(CWnd* pParent = nullptr);	// 標準建構函式

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_THREADQUEUE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支援


// 程式碼實作
protected:
	HICON m_hIcon;

	// 產生的訊息對應函式
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedStop();
	afx_msg void OnBnClickedStart();

	void Init();
	void Clearup();
	template<class T> void AddListText(int ctrlID, PCTSTR pszFormat, ...);
	static DWORD WINAPI ConsumeThread(LPVOID args);
	static DWORD WINAPI ProducerThread(LPVOID args);
	static DWORD WINAPI StopThread(LPVOID args);

	DECLARE_MESSAGE_MAP()

private:
	volatile LONG m_IsShutDown;
	std::queue<int> m_queue;
	SRWLOCK m_srwLck;
	CONDITION_VARIABLE m_consumerCond;
	CONDITION_VARIABLE m_producerCond;

	HANDLE m_hConsumer[2];
	HANDLE m_hProducer[4];

	CListBox* m_producerList;
	CListBox* m_consumerList;
	CButton* m_stop;
	CButton* m_start;
};
