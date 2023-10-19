
// 15.VirtualAllocDlg.cpp: 實作檔案
//

#include "framework.h"
#include "VirtualAlloc.h"
#include "VirtualAllocDlg.h"
#include "afxdialogex.h"

#include <strsafe.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMy15VirtualAllocDlg 對話方塊

// A dummy data structure used for the array.
typedef struct {
	BOOL bInUse;
	BYTE bOtherData[2048-sizeof(BOOL)];
} SOMEDATA, * PSOMEDATA;

#define SOMEDATA_SIZE 50

CVirtualAllocDlg::CVirtualAllocDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLG, pParent), m_someData(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVirtualAllocDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CVirtualAllocDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_RESERVE_MEM, &CVirtualAllocDlg::OnBnClickedReserveMem)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_COMMIT_MEM, &CVirtualAllocDlg::OnBnClickedCommitMem)
	ON_BN_CLICKED(IDC_DECOMMIT_MEM, &CVirtualAllocDlg::OnBnClickedDecommitMem)
	ON_BN_CLICKED(IDC_RELEASE_MEM, &CVirtualAllocDlg::OnBnClickedReleaseMem)
	ON_EN_CHANGE(IDC_EDIT_INDEX, &CVirtualAllocDlg::OnEnChangeEditIndex)
END_MESSAGE_MAP()


// CMy15VirtualAllocDlg 訊息處理常式

BOOL CVirtualAllocDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 設定此對話方塊的圖示。當應用程式的主視窗不是對話方塊時，
	// 框架會自動從事此作業
	SetIcon(m_hIcon, TRUE);			// 設定大圖示
	SetIcon(m_hIcon, FALSE);		// 設定小圖示

	// TODO: 在此加入額外的初始設定
	InitSystemInfo();

	return TRUE;  // 傳回 TRUE，除非您對控制項設定焦點
}

// 如果將最小化按鈕加入您的對話方塊，您需要下列的程式碼，
// 以便繪製圖示。對於使用文件/檢視模式的 MFC 應用程式，
// 框架會自動完成此作業。

void CVirtualAllocDlg::OnDestroy() {
	CDialogEx::OnDestroy();

	if (m_someData) {
		VirtualFree(m_someData, 0, MEM_RELEASE);
	}
}

void CVirtualAllocDlg::OnPaint()
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
		CPaintDC dc(this);
		if (m_someData == NULL) {
			dc.SelectObject(GetStockObject(STOCK_WHITE));
			dc.Rectangle(m_rcMemMap.left, m_rcMemMap.top, m_rcMemMap.right, m_rcMemMap.bottom);
		}
		else {
			for (int i = 0; i < m_maxPage; i++) {
				// cell size may < or > pageSize, so determine the lower and upper
				int low = i * m_pageSize / sizeof(SOMEDATA);
				int high = low + m_pageSize / sizeof(SOMEDATA);

				for (; low < high; low++) {
					MEMORY_BASIC_INFORMATION mbi;
					VirtualQuery(&((SOMEDATA*)m_someData)[low], &mbi, sizeof(mbi));
					int brush;

					switch (mbi.State) {
					case MEM_RESERVE:
						brush = STOCK_GRAY;
						break;
					case MEM_COMMIT:
						brush = STOCK_BLACK;
						break;
					case MEM_FREE:
						brush = STOCK_WHITE;
						break;
					}

					dc.SelectObject(GetStockObject(brush));
					dc.Rectangle(
						m_rcMemMap.left + m_MemMapWidth / m_maxPage * i,
						m_rcMemMap.top,
						m_rcMemMap.left + m_MemMapWidth / m_maxPage * (i + 1),
						m_rcMemMap.bottom);
				}
			}
		}

		CDialogEx::OnPaint();
	}
}

// 當使用者拖曳最小化視窗時，
// 系統呼叫這個功能取得游標顯示。
HCURSOR CVirtualAllocDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


HBRUSH CVirtualAllocDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) {
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	switch (pWnd->GetDlgCtrlID()) {
	case IDC_STATIC_MEM_WHITE:
	case IDC_STATIC_MEM_GRAY:
	case IDC_STATIC_MEM_BLACK:
		pDC->SetBkMode(TRANSPARENT);
		pDC->SetTextColor(RGB(0, 0, 255));
	}

	return hbr;
}


void CVirtualAllocDlg::InitSystemInfo() {
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	m_pageSize = sysInfo.dwPageSize;

	TCHAR szBuf[16] = { 0 };
	StringCchPrintf(szBuf, _countof(szBuf), L"%d KB", m_pageSize / 1024);
	SetDlgItemText(IDC_STATIC_PAGESIZE, szBuf);

	GetDlgItem(IDC_MEMMAP)->GetWindowRect(&m_rcMemMap);
	::MapWindowPoints(NULL,GetSafeHwnd(), (LPPOINT)&m_rcMemMap,2);
	GetDlgItem(IDC_MEMMAP)->DestroyWindow();
}

void CVirtualAllocDlg::OnBnClickedReserveMem() {
	GetDlgItem(IDC_COMMIT_MEM)->EnableWindow();
	GetDlgItem(IDC_DECOMMIT_MEM)->EnableWindow();
	GetDlgItem(IDC_RELEASE_MEM)->EnableWindow();
	GetDlgItem(IDC_STATIC_INDEX)->EnableWindow();
	GetDlgItem(IDC_EDIT_INDEX)->EnableWindow();
	GetDlgItem(IDC_EDIT_INDEX)->SetFocus();
	GetDlgItem(IDC_RESERVE_MEM)->EnableWindow(0);

	int sz = SOMEDATA_SIZE * sizeof(SOMEDATA);
	m_someData = VirtualAlloc(NULL, sz, MEM_RESERVE, PAGE_READWRITE);
	if (m_someData == NULL) {
		MessageBox(L"VirtualAlloc Failed", NULL, MB_OK);
		return;
	}

	m_maxPage = sz / m_pageSize;
	m_MemMapWidth = m_rcMemMap.right - m_rcMemMap.left;

	int rem = m_MemMapWidth % m_maxPage;
	int quo = m_MemMapWidth / m_maxPage;
	if (rem) {
		RECT rect;
		m_MemMapWidth = m_maxPage * quo;
		m_rcMemMap.right = m_MemMapWidth + m_rcMemMap.left;
		GetDlgItem(IDC_STATIC_GP1)->GetWindowRect(&rect);
		GetDlgItem(IDC_STATIC_GP1)->SetWindowPos(NULL, 0, 0,
			m_MemMapWidth, rect.bottom - rect.top, SWP_NOMOVE);
	}

	InvalidateRect(&m_rcMemMap, 1);
}


void CVirtualAllocDlg::OnBnClickedCommitMem() {
	int idx = GetDlgItemInt(IDC_EDIT_INDEX);

	if (m_someData == NULL || idx < 0 || idx >= SOMEDATA_SIZE) {
		return;
	}

	VirtualAlloc(&((SOMEDATA*)m_someData)[idx], sizeof(SOMEDATA), MEM_COMMIT, PAGE_READWRITE);
	((SOMEDATA*)m_someData + idx)->bInUse = TRUE;
	GetDlgItem(IDC_COMMIT_MEM)->EnableWindow(0);
	GetDlgItem(IDC_DECOMMIT_MEM)->EnableWindow(1);

	InvalidateRect(&m_rcMemMap, 1);
}

void CVirtualAllocDlg::OnBnClickedDecommitMem() {
	int idx = GetDlgItemInt(IDC_EDIT_INDEX);

	if (m_someData == NULL || idx < 0 || idx >= SOMEDATA_SIZE) {
		return;
	}

	((SOMEDATA*)m_someData + idx)->bInUse = FALSE;
	VirtualFree(&((SOMEDATA*)m_someData)[idx], sizeof(SOMEDATA), MEM_DECOMMIT);
	GetDlgItem(IDC_COMMIT_MEM)->EnableWindow(1);
	GetDlgItem(IDC_DECOMMIT_MEM)->EnableWindow(0);

	InvalidateRect(&m_rcMemMap, 1);
}

void CVirtualAllocDlg::OnBnClickedReleaseMem() {
	VirtualFree(m_someData, 0, MEM_RELEASE);
	m_someData = NULL;

	GetDlgItem(IDC_RESERVE_MEM)->EnableWindow(1);
	InvalidateRect(&m_rcMemMap, 1);
}

void CVirtualAllocDlg::OnEnChangeEditIndex() {
	int idx = GetDlgItemInt(IDC_EDIT_INDEX);

	if (m_someData == NULL || idx < 0 || idx >= SOMEDATA_SIZE) {
		GetDlgItem(IDC_COMMIT_MEM)->EnableWindow(0);
		GetDlgItem(IDC_DECOMMIT_MEM)->EnableWindow(0);
	}

	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery(&((SOMEDATA*)m_someData)[idx], &mbi, sizeof(mbi));

	if (mbi.State == MEM_COMMIT) {
		((SOMEDATA*)m_someData + idx)->bInUse = TRUE;
		GetDlgItem(IDC_COMMIT_MEM)->EnableWindow(0);
		GetDlgItem(IDC_DECOMMIT_MEM)->EnableWindow(1);
	}
	else {
		GetDlgItem(IDC_COMMIT_MEM)->EnableWindow(1);
		GetDlgItem(IDC_DECOMMIT_MEM)->EnableWindow(0);
	}
}
