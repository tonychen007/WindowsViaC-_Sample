
// KernelQueueDlg.h: 標頭檔
//

#pragma once

#include <queue>
#include <wct.h>

#define MAX_COUNT 50

// CKernelQueueDlg 對話方塊
class CKernelQueueDlg : public CDialogEx
{
// 建構
public:
	CKernelQueueDlg(CWnd* pParent = nullptr);	// 標準建構函式
	virtual ~CKernelQueueDlg();

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_KERNELQUEUE_DIALOG };
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
	afx_msg void OnBnClickedStart();
	afx_msg void OnBnClickedStop();
	afx_msg void OnBnClickedClearDeadlock();
	afx_msg void OnBnClickedUpdateDeadlock();

	void Init();
	void Clearup();
	template<class T, int AUTOSCROLL = 1> void AddListText(int ctrlID, PCTSTR pszFormat, ...);
	static DWORD WINAPI ConsumeThread(LPVOID args);
	static DWORD WINAPI ProducerThread(LPVOID args);
	static DWORD WINAPI StopThread(LPVOID args);

	void InitWaitChain();
	DECLARE_MESSAGE_MAP()

private:
	void ParseThread();
	void ParseThread0(DWORD tid);
	void OnThread(DWORD TID, BOOL bDeadlock, DWORD nodeCount);
	void OnChainNodeInfo(DWORD rootTID, DWORD currentNode, WAITCHAIN_NODE_INFO nodeInfo);
	LPCTSTR GetWCTObjectStatus(WCT_OBJECT_STATUS objectStatus);
	LPCTSTR GetWCTObjectType(WCT_OBJECT_TYPE objectType);
	static DWORD WINAPI MonitorDeadLockThread(LPVOID args);

private:
	volatile LONG m_IsShutDown;
	std::queue<int> m_queue;
	HANDLE m_hMutex;
	HANDLE m_hSema;

	HANDLE m_hConsumer[2];
	HANDLE m_hProducer[4];

	CListBox* m_producerList;
	CListBox* m_consumerList;
	CButton* m_stop;
	CButton* m_start;
	CListBox* m_deadLockList;

	DWORD m_dwPid;
	HANDLE m_hWTCSession;
};
