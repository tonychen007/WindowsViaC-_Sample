
// KernelQueueDlg.cpp: 實作檔案
//

#include "framework.h"
#include "KernelQueue.h"
#include "KernelQueueDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CKernelQueueDlg 對話方塊



CKernelQueueDlg::CKernelQueueDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_KERNELQUEUE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CKernelQueueDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CKernelQueueDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_STOP, &CKernelQueueDlg::OnBnClickedStop)
	ON_BN_CLICKED(IDC_START, &CKernelQueueDlg::OnBnClickedStart)
END_MESSAGE_MAP()


// CKernelQueueDlg 訊息處理常式

BOOL CKernelQueueDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 設定此對話方塊的圖示。當應用程式的主視窗不是對話方塊時，
	// 框架會自動從事此作業
	SetIcon(m_hIcon, TRUE);			// 設定大圖示
	SetIcon(m_hIcon, FALSE);		// 設定小圖示

	// TODO: 在此加入額外的初始設定
	Init();

	return TRUE;  // 傳回 TRUE，除非您對控制項設定焦點
}

// 如果將最小化按鈕加入您的對話方塊，您需要下列的程式碼，
// 以便繪製圖示。對於使用文件/檢視模式的 MFC 應用程式，
// 框架會自動完成此作業。

void CKernelQueueDlg::OnPaint()
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
HCURSOR CKernelQueueDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CKernelQueueDlg::OnBnClickedStop() {
	m_stop->EnableWindow(0);

	// create a new thread, or the UI will dead lock
	HANDLE h = CreateThread(NULL, 0, StopThread, this, 0, 0);
	CloseHandle(h);
}

void CKernelQueueDlg::OnBnClickedStart() {
	m_start->EnableWindow(0);
	m_stop->EnableWindow();

	m_IsShutDown = FALSE;
	m_consumerList->ResetContent();
	m_producerList->ResetContent();
	m_queue.swap(std::queue<int>());

	m_hMutex = CreateMutex(NULL, FALSE, NULL);
	m_hSema = CreateSemaphore(NULL, 0, MAX_COUNT, NULL);

	int cl = _countof(m_hConsumer);
	for (int i = 0; i < cl; i++) {
		m_hConsumer[i] = CreateThread(NULL, 0, ConsumeThread, this, 0, 0);
	}

	int pl = _countof(m_hProducer);
	for (int i = 0; i < pl; i++) {
		m_hProducer[i] = CreateThread(NULL, 0, ProducerThread, this, 0, 0);
	}
}

void CKernelQueueDlg::Init() {
	m_IsShutDown = FALSE;

	m_producerList = (CListBox*)GetDlgItem(IDC_PRODUCER_LIST);
	m_consumerList = (CListBox*)GetDlgItem(IDC_CONSUMER_LIST);
	m_start = (CButton*)GetDlgItem(IDC_START);
	m_stop = (CButton*)GetDlgItem(IDC_STOP);
	m_stop->EnableWindow(0);
}

void CKernelQueueDlg::Clearup() {

	InterlockedExchange(&m_IsShutDown, TRUE);

	int cl = _countof(m_hConsumer);
	int pl = _countof(m_hProducer);

	WaitForMultipleObjects(_countof(m_hConsumer), m_hConsumer, TRUE, INFINITE);
	WaitForMultipleObjects(_countof(m_hProducer), m_hProducer, TRUE, INFINITE);

	for (int i = 0; i < cl; i++) {
		CloseHandle(m_hConsumer[i]);
	}

	for (int i = 0; i < pl; i++) {
		CloseHandle(m_hProducer[i]);
	}

	m_start->EnableWindow(1);

	CloseHandle(m_hMutex);
	CloseHandle(m_hSema);
}

template<class T>
void CKernelQueueDlg::AddListText(int ctrlID, PCTSTR pszFormat, ...) {
	T* wnd = (T*)GetDlgItem(ctrlID);
	TCHAR buf[256] = { 0 };
	va_list args;
	va_start(args, pszFormat);

	wvsprintf(buf, pszFormat, args);
	int n = wnd->AddString(buf);
	va_end(args);
	wnd->SetCurSel(n);
}

DWORD CKernelQueueDlg::ConsumeThread(LPVOID args) {
	CKernelQueueDlg* pDlg = (CKernelQueueDlg*)args;
	HANDLE hArr[2] = { pDlg->m_hMutex, pDlg->m_hSema };

	while (1) {

		WaitForMultipleObjects(_countof(hArr), hArr, TRUE, INFINITE);

		if (pDlg->m_IsShutDown && pDlg->m_queue.size() == 0) {
			ReleaseMutex(pDlg->m_hMutex);
			break;
		}
		else {
			if (!pDlg->m_IsShutDown) {
				pDlg->m_queue.pop();
				int v = pDlg->m_queue.size();
				pDlg->AddListText<CListBox>(IDC_CONSUMER_LIST, L"The queue size is: %d", v);
			}
			else {
				pDlg->AddListText<CListBox>(IDC_CONSUMER_LIST, L"%s", L"Processing remaining elements...");
				pDlg->m_queue.pop();
				int v = pDlg->m_queue.size();
				pDlg->AddListText<CListBox>(IDC_CONSUMER_LIST, L"The queue size is: %d", v);
			}

			ReleaseMutex(pDlg->m_hMutex);
			pDlg->m_IsShutDown ? Sleep(10) : Sleep(500);
		}
	}

	return 0;
}

DWORD CKernelQueueDlg::ProducerThread(LPVOID args) {
	CKernelQueueDlg* pDlg = (CKernelQueueDlg*)args;
	TCHAR buf[256] = { 0 };
	DWORD dwSt;

	while (1) {

		WaitForSingleObject(pDlg->m_hMutex, INFINITE);

		if (pDlg->m_IsShutDown && pDlg->m_queue.size() == 0) {
			// consumer thread is wait for both mutex and sema
			ReleaseSemaphore(pDlg->m_hSema, 1, NULL);
			ReleaseMutex(pDlg->m_hMutex);
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

			ReleaseSemaphore(pDlg->m_hSema, 1, NULL);
			ReleaseMutex(pDlg->m_hMutex);
			pDlg->m_IsShutDown ? Sleep(10) : Sleep(200);
		}
	}

	return 0;
}

DWORD CKernelQueueDlg::StopThread(LPVOID args) {
	CKernelQueueDlg* pDlg = (CKernelQueueDlg*)args;

	pDlg->Clearup();

	return 0;
}