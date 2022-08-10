
// ProcessGUIDlg.cpp: 實作檔案
//

#include "framework.h"
#include "ProcessGUI.h"
#include "ProcessGUIDlg.h"
#include "afxdialogex.h"

#include <windowsx.h>
#include <strsafe.h>
#include <winternl.h>
#include "Toolhelp.h"
#include "Tools.h"


#pragma comment(lib, "ntdll")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;

#define FAIL_GOTO_MODULE(hr, m) {if (!hr) goto m;}

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
	ON_CBN_SELCHANGE(IDC_PROCESS_DROPLIST, &CProcessGUIDlg::OnSelchangeProcessDroplist)
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
	m_processInfoEdit = (CEdit*)GetDlgItem(IDC_PROCESS_INFO_EDIT);

	itemHeight = tm.tmHeight * 2;
	m_processDropList->SetItemHeight(-1, itemHeight);
	m_processDropList->GetClientRect(&comboRect);
	m_processDropList->SetWindowPos(NULL, 0, 0, comboRect.right, dropCount * itemHeight, SWP_NOMOVE | SWP_NOZORDER);
	EnumProcess();
	m_processDropList->SetCurSel(0);
	m_processDropList->SetRedraw();
	m_processDropList->InvalidateRect(NULL, TRUE);
	m_processDropList->UpdateData();
	OnSelchangeProcessDroplist();
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

void CProcessGUIDlg::EnumProcess() {
	CToolhelp thProcesses(TH32CS_SNAPPROCESS);
	PROCESSENTRY32 pe = { sizeof(pe) };
	BOOL fOk = thProcesses.ProcessFirst(&pe);

	while (fOk) {
		TCHAR sz[1024];

		// Place the process name (without its path) & ID in the list
		PCTSTR pszExeFile = _tcsrchr(pe.szExeFile, TEXT('\\'));
		if (pszExeFile == NULL) {
			pszExeFile = pe.szExeFile;
		}
		else {
			pszExeFile++; // Skip over the slash
		}

		StringCchPrintf(sz, _countof(sz), TEXT("%s     PID:0x%08X  %s    [%s]"),
			pszExeFile, pe.th32ProcessID, L"", L"");

		int n = m_processDropList->AddString(sz);
		m_processDropList->SetItemData(n, pe.th32ProcessID);

		//
		fOk = thProcesses.ProcessNext(&pe);
	}
}

// Add a string to an edit control
void CProcessGUIDlg::AddText(PCTSTR pszFormat, ...) {

	va_list argList;
	va_start(argList, pszFormat);

	TCHAR sz[4096];
	_vstprintf_s(sz, 4096, pszFormat, argList);
	m_str.append(sz);
	va_end(argList);
}

void CProcessGUIDlg::ShowProcessInfo(DWORD dwPid) {
	m_processInfoEdit->Clear();
	m_str.clear();
	m_str.reserve(4096);

	DWORD dwSize;
	SIZE_T sz;
	DWORD status;
	BOOL ret;
	PROCESS_BASIC_INFORMATION  pbi;
	SYSTEM_PROCESS_INFORMATION* spi;
	TCHAR szBuf[4096];

	// get parant-pid, process priority, thread num, heap num
	CToolhelp th(TH32CS_SNAPALL);
	PROCESSENTRY32 pe = { sizeof(pe) };
	BOOL fOk = th.ProcessFirst(&pe);

	/*
	// use ntdll internal func to walk through process
	status = ZwQuerySystemInformation(SystemProcessInformation, NULL, 0, &dwSize);
	LPVOID buffer;
	buffer = malloc(dwSize);
	status = ZwQuerySystemInformation(SystemProcessInformation, buffer, dwSize, 0);
	spi = (SYSTEM_PROCESS_INFORMATION*)buffer;

	while (spi->NextEntryOffset) {
		spi = (SYSTEM_PROCESS_INFORMATION*)((char*)spi + spi->NextEntryOffset);
	}
	*/

	while (fOk) {
		if (pe.th32ProcessID == dwPid) {
			AddText(
				TEXT("The pid is=0x%08X, parent pid is=0x%08X, priority class=%d, threads=%d, heap=%d\r\n"),
				dwPid, pe.th32ParentProcessID, pe.pcPriClassBase, pe.cntThreads, th.HowManyHeaps());
			break;
		}
		fOk = th.ProcessNext(&pe);
	}

	// get command line
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
	if (hProcess == 0)
		goto module;

	status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), &dwSize);
	if (status >= 0) {
		_PEB peb;
		RTL_USER_PROCESS_PARAMETERS userParam;
		ret = ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(peb), &sz);
		FAIL_GOTO_MODULE(ret, module);

		ret = ReadProcessMemory(hProcess, (LPVOID)peb.ProcessParameters, &userParam, sizeof(userParam), &sz);
		FAIL_GOTO_MODULE(ret, module);

		wchar_t wszCmdLine[MAX_PATH + 1] = { L'\0' };
		ret = ReadProcessMemory(hProcess, (LPVOID)userParam.CommandLine.Buffer, wszCmdLine, MAX_PATH * sizeof(wchar_t), &sz);
		FAIL_GOTO_MODULE(ret, module);

		AddText(TEXT("  CommandLine is: %ws\r\n"), wszCmdLine);
	}

	// get owner
	HANDLE hToken;
	CToolhelp::EnablePrivilege(SE_TCB_NAME, TRUE);
	ret = OpenProcessToken(hProcess, TOKEN_QUERY, &hToken);
	FAIL_GOTO_MODULE(ret, module);

	DWORD cbti = 0;
	ret = GetTokenInformation(hToken, TokenUser, NULL, 0, &cbti);
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		PTOKEN_USER ptiUser = (PTOKEN_USER)HeapAlloc(GetProcessHeap(), 0, cbti);
		ret = GetTokenInformation(hToken, TokenUser, ptiUser, cbti, &cbti);
		FAIL_GOTO_MODULE(ret, module);

		SID_NAME_USE   snu;
		TCHAR          szUser[MAX_PATH];
		DWORD          chUser = MAX_PATH;
		PDWORD         pcchUser = &chUser;
		TCHAR          szDomain[MAX_PATH];
		DWORD          chDomain = MAX_PATH;
		PDWORD         pcchDomain = &chDomain;

		ret = LookupAccountSid(NULL, ptiUser->User.Sid, szUser, pcchUser, szDomain, pcchDomain, &snu);
		FAIL_GOTO_MODULE(ret, module);

		StringCchCopy(szBuf, _countof(szBuf), TEXT("\\\\"));
		StringCchCat(szBuf, _countof(szBuf), szDomain);
		StringCchCat(szBuf, _countof(szBuf), TEXT("\\"));
		StringCchCat(szBuf, _countof(szBuf), szUser);
		StringCchCat(szBuf, _countof(szBuf), TEXT("\0"));
		AddText(TEXT("Owner is: %ws\r\n"), szBuf);
	}

	module:
	AddText(TEXT("\r\nModules Information:\r\n")
		TEXT("  Usage  \t%s(%s)%10s    Module\r\n"),
		TEXT("BaseAddr"),
		TEXT("ImagAddr"), TEXT("Size"));
	m_processInfoEdit->SetWindowText(m_str.data());

	// Get Module name, address, size, fullpath
	IMAGE_DOS_HEADER dosHeader;
}

void CProcessGUIDlg::OnSelchangeProcessDroplist() {
	int idx = m_processDropList->GetCurSel();
	DWORD pid = m_processDropList->GetItemData(idx);
	ShowProcessInfo(pid);
}