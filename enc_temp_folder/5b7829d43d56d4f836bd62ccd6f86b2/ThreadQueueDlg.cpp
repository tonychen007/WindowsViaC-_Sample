
// ThreadQueueDlg.cpp: 實作檔案
//

#include "framework.h"
#include "ThreadQueue.h"
#include "ThreadQueueDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CThreadQueueDlg 對話方塊

CThreadQueueDlg::CThreadQueueDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_THREADQUEUE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CThreadQueueDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CThreadQueueDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_STOP, &CThreadQueueDlg::OnBnClickedStop)
	ON_BN_CLICKED(IDC_START, &CThreadQueueDlg::OnBnClickedStart)
END_MESSAGE_MAP()


// CThreadQueueDlg 訊息處理常式

BOOL CThreadQueueDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CMenu* pSysMenu = GetSystemMenu(FALSE);

	// 設定此對話方塊的圖示。當應用程式的主視窗不是對話方塊時，
	// 框架會自動從事此作業
	SetIcon(m_hIcon, TRUE);			// 設定大圖示
	SetIcon(m_hIcon, FALSE);		// 設定小圖示

	// TODO: 在此加入額外的初始設定
	Init();

	return TRUE;  // 傳回 TRUE，除非您對控制項設定焦點
}

void CThreadQueueDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	CDialogEx::OnSysCommand(nID, lParam);
}

// 如果將最小化按鈕加入您的對話方塊，您需要下列的程式碼，
// 以便繪製圖示。對於使用文件/檢視模式的 MFC 應用程式，
// 框架會自動完成此作業。

void CThreadQueueDlg::OnPaint()
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
		CDialogEx::OnPaint();
	}
}

// 當使用者拖曳最小化視窗時，
// 系統呼叫這個功能取得游標顯示。
HCURSOR CThreadQueueDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CThreadQueueDlg::OnBnClickedStop() {
	m_start->EnableWindow(0);
	m_stop->EnableWindow(0);

	Clearup();
}

void CThreadQueueDlg::OnBnClickedStart() {
	m_start->EnableWindow(0);
	m_stop->EnableWindow();

	m_IsShutDown = FALSE;
	m_consumerList->ResetContent();
	m_producerList->ResetContent();
	m_queue.swap(std::queue<int>());

	int cl = _countof(m_hConsumer);
	for (int i = 0; i < cl; i++) {
		m_hConsumer[i] = CreateThread(NULL, 0, ConsumeThread, this, 0, 0);
	}

	int pl = _countof(m_hProducer);
	for (int i = 0; i < pl; i++) {
		m_hProducer[i] = CreateThread(NULL, 0, ProducerThread, this, 0, 0);
	}
}

void CThreadQueueDlg::Init() {
	m_IsShutDown = FALSE;
	InitializeSRWLock(&m_srwLck);
	InitializeConditionVariable(&m_consumerCond);
	InitializeConditionVariable(&m_producerCond);

	m_producerList = (CListBox*)GetDlgItem(IDC_PRODUCER_LIST);
	m_consumerList = (CListBox*)GetDlgItem(IDC_CONSUMER_LIST);
	m_start = (CButton*)GetDlgItem(IDC_START);
	m_stop = (CButton*)GetDlgItem(IDC_STOP);

	m_stop->EnableWindow(0);
}

void CThreadQueueDlg::Clearup() {
	if (m_IsShutDown)
		return;

	InterlockedExchange(&m_IsShutDown, TRUE);

	WakeAllConditionVariable(&m_consumerCond);
	WakeAllConditionVariable(&m_producerCond);

	//WaitForMultipleObjects(_countof(m_hProducer), m_hProducer, TRUE, INFINITE);
	//WaitForMultipleObjects(_countof(m_hConsumer), m_hConsumer, TRUE, INFINITE);

	int cl = _countof(m_hConsumer);
	for (int i = 0; i < cl; i++) {
		CloseHandle(m_hConsumer[i]);
	}

	int pl = _countof(m_hProducer);
	for (int i = 0; i < pl; i++) {
		CloseHandle(m_hProducer[i]);
	}
}

template<class T>
void CThreadQueueDlg::AddListText(int ctrlID, PCTSTR pszFormat, ...) {
	T* wnd = (T*)GetDlgItem(ctrlID);
	TCHAR buf[256] = { 0 };
	va_list args;
	va_start(args, pszFormat);

	wvsprintf(buf, pszFormat, args);
	int n = wnd->AddString(buf);
	va_end(args);
	wnd->SetCurSel(n);
}

DWORD CThreadQueueDlg::ConsumeThread(LPVOID args) {
	CThreadQueueDlg* pDlg = (CThreadQueueDlg*)args;

	while (1) {
		AcquireSRWLockExclusive(&pDlg->m_srwLck);

		while (pDlg->m_queue.size() == 0 && !pDlg->m_IsShutDown) {
			SleepConditionVariableSRW(&pDlg->m_consumerCond, &pDlg->m_srwLck, INFINITE, 0);
		}

		if (pDlg->m_IsShutDown && pDlg->m_queue.size() == 0) {
			ReleaseSRWLockExclusive(&pDlg->m_srwLck);
			WakeAllConditionVariable(&pDlg->m_consumerCond);
			
			pDlg->m_start->EnableWindow(1);
			break;
		}
		else {
			if (!pDlg->m_IsShutDown) {
				int v = pDlg->m_queue.size();
				pDlg->m_queue.pop();
				pDlg->AddListText<CListBox>(IDC_CONSUMER_LIST, L"The queue size is: %d", v);
			}
			else {
				pDlg->AddListText<CListBox>(IDC_CONSUMER_LIST, L"%s", L"Processing remaining elements...");
				pDlg->m_queue.pop();
				int v = pDlg->m_queue.size();
				pDlg->AddListText<CListBox>(IDC_CONSUMER_LIST, L"The queue size is: %d", v);
				Sleep(10);
			}

			ReleaseSRWLockExclusive(&pDlg->m_srwLck);
			WakeAllConditionVariable(&pDlg->m_producerCond);
			Sleep(80);
		}
	}

	return 0;
}

DWORD CThreadQueueDlg::ProducerThread(LPVOID args) {
	CThreadQueueDlg* pDlg = (CThreadQueueDlg*)args;
	TCHAR buf[256] = { 0 };

	while (1) {

		AcquireSRWLockExclusive(&pDlg->m_srwLck);

		while ((pDlg->m_queue.size() == 100) && !pDlg->m_IsShutDown) {
			int n = pDlg->m_producerList->AddString(L"Queue is full...");
			pDlg->m_producerList->SetCurSel(n);
			SleepConditionVariableSRW(&pDlg->m_producerCond, &pDlg->m_srwLck, INFINITE, 0);
		}

		if (pDlg->m_IsShutDown && pDlg->m_queue.size() == 0) {
			ReleaseSRWLockExclusive(&pDlg->m_srwLck);
			WakeAllConditionVariable(&pDlg->m_producerCond);
			break;
		}
		else {
			if (!pDlg->m_IsShutDown) {
				int v = pDlg->m_queue.size();
				++v;
				pDlg->m_queue.push(v);
				_itow_s(v, buf, 10);
				int n = pDlg->m_producerList->AddString(buf);
				pDlg->m_producerList->SetCurSel(n);
			}

			ReleaseSRWLockExclusive(&pDlg->m_srwLck);
			WakeAllConditionVariable(&pDlg->m_consumerCond);
			Sleep(50);
		}
	}

	return 0;
}

