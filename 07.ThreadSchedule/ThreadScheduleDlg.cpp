
// ThreadScheduleDlg.cpp: 實作檔案
//

#include "framework.h"
#include "ThreadSchedule.h"
#include "ThreadScheduleDlg.h"
#include "afxdialogex.h"

#include <windowsx.h>
#include <mmintrin.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CThreadScheduleDlg 對話方塊


HWND				gDlgHWND;
HANDLE				gEvent;
HANDLE				gUIEvent;
CRITICAL_SECTION	gCS;
int					gIdx;
int64_t				gVal;
int					gSleep;

CThreadScheduleDlg::CThreadScheduleDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_THREADSCHEDULE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CThreadScheduleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CThreadScheduleDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_EN_CHANGE(IDC_SLEEP_EDIT, &CThreadScheduleDlg::OnEnChangeSleepEdit)
	ON_CBN_SELCHANGE(IDC_THREAD_PRI, &CThreadScheduleDlg::OnCbnSelchangeThreadPri)
	ON_CBN_SELCHANGE(IDC_PROC_PRI, &CThreadScheduleDlg::OnCbnSelchangeProcPri)
END_MESSAGE_MAP()


// CThreadScheduleDlg 訊息處理常式

BOOL CThreadScheduleDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 設定此對話方塊的圖示。當應用程式的主視窗不是對話方塊時，
	// 框架會自動從事此作業
	SetIcon(m_hIcon, TRUE);			// 設定大圖示
	SetIcon(m_hIcon, FALSE);		// 設定小圖示

	// TODO: 在此加入額外的初始設定
	gDlgHWND = GetSafeHwnd();
	InitializeCriticalSection(&gCS);
	gEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	gUIEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	InitDropListControl();
	InitThread();

	return TRUE;  // 傳回 TRUE，除非您對控制項設定焦點
}

// 如果將最小化按鈕加入您的對話方塊，您需要下列的程式碼，
// 以便繪製圖示。對於使用文件/檢視模式的 MFC 應用程式，
// 框架會自動完成此作業。

void CThreadScheduleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 繪製的裝置內容

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 將圖示置中於用戶端矩形
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 描繪圖示
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// 當使用者拖曳最小化視窗時，
// 系統呼叫這個功能取得游標顯示。
HCURSOR CThreadScheduleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CThreadScheduleDlg::InitDropListControl() {
	CRect comboRect;
	TEXTMETRIC tm;
	CDC* pDC = GetDC();
	GetTextMetrics(*pDC, &tm);
	CONST int dropCount = 5;

	TCHAR* buf[dropCount] = {
		L"High",
		L"Above Normal",
		L"Normal",
		L"Below Normal",
		L"Idle",
	};

	int procPri[dropCount] = {
		HIGH_PRIORITY_CLASS,
		ABOVE_NORMAL_PRIORITY_CLASS,
		NORMAL_PRIORITY_CLASS,
		BELOW_NORMAL_PRIORITY_CLASS,
		IDLE_PRIORITY_CLASS
	};

	int threadPri[dropCount] = {
		THREAD_PRIORITY_HIGHEST,
		THREAD_PRIORITY_ABOVE_NORMAL,
		THREAD_PRIORITY_NORMAL,
		THREAD_PRIORITY_BELOW_NORMAL,
		THREAD_PRIORITY_IDLE
	};

	m_procBox = (CComboBox*)GetDlgItem(IDC_PROC_PRI);
	m_threadBox = (CComboBox*)GetDlgItem(IDC_THREAD_PRI);
	m_sleepEdit = (CEdit*)GetDlgItem(IDC_SLEEP_EDIT);
	m_procList = (CListCtrl*)GetDlgItem(IDC_PROC_LIST);

	int itemHeight = tm.tmHeight * 2;
	m_procBox->SetItemHeight(-1, itemHeight);
	m_threadBox->SetItemHeight(-1, itemHeight);
	m_procBox->GetClientRect(&comboRect);
	m_procBox->SetWindowPos(NULL, 0, 0, comboRect.right, (dropCount + 1) * itemHeight, SWP_NOMOVE | SWP_NOZORDER);
	m_threadBox->GetClientRect(&comboRect);
	m_threadBox->SetWindowPos(NULL, 0, 0, comboRect.right, (dropCount + 1) * itemHeight, SWP_NOMOVE | SWP_NOZORDER);

	for (int i = 0; i < dropCount; i++) {
		m_procBox->AddString(buf[i]);
		m_procBox->SetItemData(i, procPri[i]);
		m_threadBox->AddString(buf[i]);
		m_threadBox->SetItemData(i, threadPri[i]);
	}

	m_procBox->SetCurSel(2);
	m_procBox->InvalidateRect(NULL, TRUE);
	m_procBox->UpdateData();
	m_threadBox->SetCurSel(2);
	m_threadBox->InvalidateRect(NULL, TRUE);
	m_threadBox->UpdateData();

	m_procLastPri = procPri[2];
	m_threadLastPri = procPri[2];

	m_sleepEdit->SetLimitText(4);
	m_sleepEdit->SetWindowText(L"0");
	gSleep = m_SleepTime = 0;
}

void CThreadScheduleDlg::InitThread() {
	SYSTEM_INFO systemInfo;
	int cpuCount;

	GetSystemInfo(&systemInfo);
	cpuCount = systemInfo.dwNumberOfProcessors;

	for (int i = 0; i < cpuCount; i++) {
		threadData *thData = new threadData();
		thData->idx = i;
		thData->m_procList = m_procList;

		HANDLE h = CreateThread(NULL, 0, ThreadFunc, thData, CREATE_SUSPENDED, 0);
		m_threadHandles.push_back(h);
	}

	for (int i = 0; i < cpuCount; i++) {
		ResumeThread(m_threadHandles[i]);
	}

	for (int i = 0; i < cpuCount; i++) {
		LVITEM item;
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = 0;
		item.cchTextMax = MAX_PATH;
		item.pszText = L"0";

		m_procList->InsertItem(&item);
	}

	// create a UI thread for updating, we do not need the handle
	CreateThread(NULL, 0, ThreadUIFunc, m_procList, 0, 0);
}

void CThreadScheduleDlg::OnEnChangeSleepEdit() {
	TCHAR buf[256] = { 0 };
	int val;

	m_sleepEdit->GetWindowText(buf, _countof(buf));

	val = _wtoi(buf);
	if (val > SLEEP_MAX) {
		val = SLEEP_MAX;
		_itow_s(val, buf, 10);
		m_sleepEdit->SetWindowText(buf);
	}

	gSleep = m_SleepTime = val;
	SetEvent(gEvent);
}

void CThreadScheduleDlg::OnCbnSelchangeProcPri() {
	int pri = m_procBox->GetCurSel();
	DWORD_PTR data = m_procBox->GetItemData(pri);

	if (data == m_procLastPri) {
		return;
	}

	m_procLastPri = data;
	SetPriorityClass(GetCurrentProcess(), data);
}

void CThreadScheduleDlg::OnCbnSelchangeThreadPri() {
	int pri = m_threadBox->GetCurSel();
	DWORD_PTR data = m_threadBox->GetItemData(pri);

	if (data == m_threadLastPri) {
		return;
	}

	m_threadLastPri = data;

	for (int i = 0; i < m_threadHandles.size(); i++) {
		SuspendThread(m_threadHandles[i]);
		SetThreadPriority(m_threadHandles[i], data);
		ResumeThread(m_threadHandles[i]);
	}
}

// static
DWORD WINAPI CThreadScheduleDlg::ThreadFunc(LPVOID args) {
	DWORD dw, sl, val;
	threadData* thData = (threadData*)args;
	int idx = thData->idx;
	CListCtrl* procList = thData->m_procList;

	while (1) {
		Sleep(gSleep);
		EnterCriticalSection(&gCS);
		gIdx = idx;
		LeaveCriticalSection(&gCS);
		SetEvent(gUIEvent);
	}

	return 0;
}

DWORD WINAPI CThreadScheduleDlg::ThreadUIFunc(LPVOID args) {
	DWORD dw, val;
	CListCtrl* procList = (CListCtrl*)args;
	TCHAR buf[256] = { 0 };

	while (1) {
		dw = WaitForSingleObject(gUIEvent, 1);
		switch (dw) {
		case WAIT_OBJECT_0: {
			EnterCriticalSection(&gCS);
			procList->GetItemText(gIdx, 0, buf, 256);
			val = _wtoi(buf);
			val++;
			_itow_s(val, buf, 10);
			procList->SetItemText(gIdx, 0, buf);
			LeaveCriticalSection(&gCS);
			ResetEvent(gUIEvent);
			break;
		}
		case WAIT_TIMEOUT:
			break;
		}
	}
}