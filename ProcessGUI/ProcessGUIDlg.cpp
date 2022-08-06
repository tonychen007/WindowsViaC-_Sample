
// ProcessGUIDlg.cpp: 實作檔案
//

#include "framework.h"
#include "ProcessGUI.h"
#include "ProcessGUIDlg.h"
#include "afxdialogex.h"

#include <windowsx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;

CProcessGUIDlg::CProcessGUIDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_PROCESSGUI_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CProcessGUIDlg::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_ENV_LIST, m_listCtrl);
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CProcessGUIDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY(LVN_GETDISPINFO, IDC_ENV_LIST, &CProcessGUIDlg::OnGetdispinfoEnvList)
	ON_BN_CLICKED(IDC_GET_ENV_VAR_BTN, &CProcessGUIDlg::OnBnClickedGetEnvVarBtn)
END_MESSAGE_MAP()


// CProcessGUIDlg 訊息處理常式

BOOL CProcessGUIDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 設定此對話方塊的圖示。當應用程式的主視窗不是對話方塊時，
	// 框架會自動從事此作業
	SetIcon(m_hIcon, TRUE);			// 設定大圖示
	SetIcon(m_hIcon, FALSE);		// 設定小圖示

	InitEnvListControl();
	InitProcessDropListControl();

	return TRUE;  // 傳回 TRUE，除非您對控制項設定焦點
}

void CProcessGUIDlg::OnPaint()
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
HCURSOR CProcessGUIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CProcessGUIDlg::OnLButtonDown(UINT nFlags, CPoint point) {
	RECT rect;
	m_listCtrl.GetWindowRect(&rect);

	if ((rect.left <= point.x && point.x <= rect.right) &&
		(rect.top <= point.y && point.y <= rect.bottom)) {

	}
	else {
		m_listCtrl.ClearEditBox();
	}

	CDialog::OnLButtonDown(nFlags, point);
}

void CProcessGUIDlg::InitEnvListControl() {
	RECT rect;
	LONG lStyle;
	DWORD dwStyle;
	LONG width;
	TEXTMETRIC tm;
	CDC* pDC = GetDC();
	GetTextMetrics(*pDC, &tm);

	dwStyle = m_listCtrl.GetExtendedStyle();
	dwStyle |= LVS_EX_GRIDLINES;
	m_listCtrl.SetExtendedStyle(dwStyle);;
	m_listCtrl.GetWindowRect(&rect);
	width = rect.right - rect.left;

	LVCOLUMN lvcol1, lvcol2;
	lvcol1.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol2.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol1.fmt = LVCFMT_LEFT;
	lvcol2.fmt = LVCFMT_LEFT;
	lvcol1.iSubItem = 0;
	lvcol2.iSubItem = 1;

	lvcol1.pszText = L"Env Name";
	lvcol2.pszText = L"Env Value";
	lvcol1.cchTextMax = 512;
	lvcol2.cchTextMax = 512;
	lvcol1.cx = (lstrlenW(lvcol1.pszText) * 2) * tm.tmMaxCharWidth;
	lvcol2.cx = (width - lvcol1.cx - lstrlenW(lvcol2.pszText) + 1);

	m_listCtrl.InsertColumn(0, &lvcol1);
	m_listCtrl.InsertColumn(1, &lvcol2);
}

void CProcessGUIDlg::InitProcessDropListControl() {
	CRect comboRect;
	int itemHeight;
	TEXTMETRIC tm;
	CDC* pDC = GetDC();
	GetTextMetrics(*pDC, &tm);
	CONST int dropCount = 10;

	m_processDropList = (CComboBox*)GetDlgItem(IDC_PROCESS_DROPLIST);
	itemHeight = tm.tmHeight * 2;
	m_processDropList->SetItemHeight(-1, itemHeight);
	m_processDropList->AddString(L"one");
	m_processDropList->AddString(L"two");
	m_processDropList->AddString(L"three");
	m_processDropList->AddString(L"four");
	m_processDropList->AddString(L"five");

	m_processDropList->GetClientRect(&comboRect);
	m_processDropList->SetWindowPos(NULL, 0, 0, comboRect.right, dropCount * itemHeight, SWP_NOMOVE | SWP_NOZORDER);
	m_processDropList->SetCurSel(0);
	m_processDropList->SetRedraw();
	m_processDropList->InvalidateRect(NULL, TRUE);
}

void CProcessGUIDlg::OnGetdispinfoEnvList(NMHDR* pNMHDR, LRESULT* pResult) {
	NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	LVITEM item = pDispInfo->item;
	int id = item.iItem;
	int i = 0;
	StringDictItr itr = m_envDict.begin();

	if (item.mask & LVIF_TEXT) {
		CString str;

		switch (item.iSubItem) {
		case 0:
			for (; i < id; i++) {
				itr++;
			}
			str = itr->first.c_str();
			break;
		case 1:
			for (; i < id; i++) {
				itr++;
			}
			str = itr->second.c_str();
			break;
		}

		lstrcpyn(item.pszText, str, item.cchTextMax);
	}
}

void CProcessGUIDlg::OnBnClickedGetEnvVarBtn() {
	char** env = _environ;

	while (*env != NULL) {
		char* p = *env;
		size_t len = strlen(*env);
		size_t l1 = 0;
		string k;
		string v;

		while (p[l1] != '=')
			l1++;

		k.assign(*env, l1);
		v.assign(*env + l1 + 1, len - l1);
		m_envDict[k] = v;

		env++;
	}

	m_listCtrl.SetItemCount(m_envDict.size());

}