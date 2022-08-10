
// ProcessGUIDlg.cpp: 實作檔案
//

#include "framework.h"
#include "ProcessGUI.h"
#include "ProcessGUIDlg.h"
#include "afxdialogex.h"

#include <windowsx.h>
#include <strsafe.h>
#include <winternl.h>
#include <shlwapi.h>
#include <AclAPI.h>

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
	ON_CBN_SELCHANGE(IDC_DROPLIST, &CProcessGUIDlg::OnSelchangeDroplist)
	ON_BN_CLICKED(IDC_ENUM_PROCESS, &CProcessGUIDlg::OnBnClickedEnumProcess)
	ON_BN_CLICKED(IDC_ENUM_MODULE, &CProcessGUIDlg::OnBnClickedEnumModule)
	ON_WM_SIZE()
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
	InitDropListControl();
	m_isModule = FALSE;

	CRect rect;
	GetClientRect(&rect);
	m_oldWinPos.x = rect.right - rect.left;
	m_oldWinPos.y = rect.bottom - rect.top;

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

void CProcessGUIDlg::OnSize(UINT nType, int cx, int cy) {
	CDialog::OnSize(nType, cx, cy);

	if (nType != SIZE_MINIMIZED) {
		int woc;
		float diff[2];
		POINT newWinPos;
		CRect rect;
		CRect clientRect;
		GetClientRect(&clientRect);

		newWinPos.x = clientRect.right - clientRect.left;
		newWinPos.y = clientRect.bottom - clientRect.top;
		diff[0] = (float)newWinPos.x - m_oldWinPos.x;
		diff[1] = (float)newWinPos.y - m_oldWinPos.y;

		CPoint oldTopLeftPt, topLeftPt;
		CPoint oldBottomRightPt, bottomRightPt;
		HWND hwndChild = ::GetWindow(m_hWnd, GW_CHILD);

		while (hwndChild) {
			woc = ::GetDlgCtrlID(hwndChild);
			GetDlgItem(woc)->GetWindowRect(rect);
			ScreenToClient(rect);
			oldTopLeftPt = rect.TopLeft();
			oldBottomRightPt = rect.BottomRight();

			topLeftPt.x = oldTopLeftPt.x;
			topLeftPt.y = long(oldTopLeftPt.y + diff[1]);
			bottomRightPt.x = long(oldBottomRightPt.x + diff[0]);
			bottomRightPt.y = long(oldBottomRightPt.y + diff[1]);
			rect.SetRect(topLeftPt, bottomRightPt);
			if (woc == IDC_GET_ENV_VAR_BTN || woc == IDC_ENUM_PROCESS || woc == IDC_ENUM_MODULE) {
				// do not change btn width x
				bottomRightPt.x = oldBottomRightPt.x;
				rect.SetRect(topLeftPt, bottomRightPt);
			}

			// just change ENV_LIST height and width
			if (woc == IDC_ENV_LIST) {
				topLeftPt.y = oldTopLeftPt.y;
				rect.SetRect(topLeftPt, bottomRightPt);
			}

			GetDlgItem(woc)->MoveWindow(rect, TRUE);
			hwndChild = ::GetWindow(hwndChild, GW_HWNDNEXT);
		}
		m_oldWinPos = newWinPos;
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

void CProcessGUIDlg::InitDropListControl() {
	CRect comboRect;
	int itemHeight;
	TEXTMETRIC tm;
	CDC* pDC = GetDC();
	GetTextMetrics(*pDC, &tm);
	CONST int dropCount = 10;

	m_dropList = (CComboBox*)GetDlgItem(IDC_DROPLIST);
	m_infoEdit = (CEdit*)GetDlgItem(IDC_INFO_EDIT);

	itemHeight = tm.tmHeight * 2;
	m_dropList->SetItemHeight(-1, itemHeight);
	m_dropList->GetClientRect(&comboRect);
	m_dropList->SetWindowPos(NULL, 0, 0, comboRect.right, dropCount * itemHeight, SWP_NOMOVE | SWP_NOZORDER);
	EnumProcess();
	m_dropList->SetCurSel(0);
	m_dropList->SetRedraw();
	m_dropList->InvalidateRect(NULL, TRUE);
	m_dropList->UpdateData();
	OnSelchangeDroplist();
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

void CProcessGUIDlg::OnBnClickedEnumProcess() {
	m_dropList->ResetContent();
	m_infoEdit->SetWindowText(L"");
	m_str.clear();

	EnumProcess();
	m_isModule = FALSE;
	m_dropList->SetCurSel(0);
	OnSelchangeDroplist();
}

void CProcessGUIDlg::OnBnClickedEnumModule() {
	m_dropList->ResetContent();
	m_infoEdit->SetWindowText(L"");
	m_str.clear();

	EnumModule();
	m_isModule = TRUE;
	m_dropList->SetCurSel(0);
	OnSelchangeDroplist();
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

		int n = m_dropList->AddString(sz);
		m_dropList->SetItemData(n, pe.th32ProcessID);

		fOk = thProcesses.ProcessNext(&pe);
	}
}

void CProcessGUIDlg::EnumModule() {
	CList<CString> listBox;
	CString str;
	CToolhelp thProcesses(TH32CS_SNAPPROCESS);
	PROCESSENTRY32 pe = { sizeof(pe) };
	BOOL fOk = thProcesses.ProcessFirst(&pe);

	while (fOk) {
		CToolhelp thModules(TH32CS_SNAPMODULE, pe.th32ProcessID);
		MODULEENTRY32 me = { sizeof(me) };
		BOOL fOk2 = thModules.ModuleFirst(&me);

		while (fOk2) {
			POSITION n = listBox.Find(me.szExePath);
			if (n == NULL) {
				listBox.AddTail(me.szExePath);
			}
			fOk2 = thModules.ModuleNext(&me);
		}

		fOk = thProcesses.ProcessNext(&pe);
	}

	for (int i = 0; i < listBox.GetSize(); i++) {
		POSITION p = listBox.FindIndex(i);
		str = listBox.GetAt(p);
		int pos  = str.ReverseFind(L'\\');
		LPCWSTR pszName = str.GetString();
		pszName += pos + 1;
		int n = m_dropList->AddString(pszName);
	}
}

void CProcessGUIDlg::ShowProcessInfo(DWORD dwPid) {
	m_infoEdit->SetWindowText(L"");
	m_str.clear();

	DWORD dwSize;
	SIZE_T sz;
	DWORD status;
	BOOL ret;
	PROCESS_BASIC_INFORMATION  pbi;
	SYSTEM_PROCESS_INFORMATION* spi;
	TCHAR szBuf[4096];
	HANDLE hToken;
	HANDLE hProcess;

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
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
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

	CloseHandle(hProcess);
	CloseHandle(hToken);
module:
	AddText(TEXT("\r\nModules Information:\r\n")
		TEXT("  Usage  \t%s(%s)%36s    Module\r\n"),
		TEXT("BaseAddr"),
		TEXT("ImageAddr"), TEXT("Size"));

	CToolhelp th2 = CToolhelp(TH32CS_SNAPALL, dwPid);
	MODULEENTRY32 me = { sizeof(me) };
	fOk = th2.ModuleFirst(&me);
	while (fOk) {
		if (me.ProccntUsage == 65535) {
			// Module was implicitly loaded and cannot be unloaded
			AddText(TEXT("  Fixed"));
		}
		else {
			AddText(TEXT("  %5d"), me.ProccntUsage);
		}


		// Get module address
		IMAGE_DOS_HEADER idh;
		IMAGE_NT_HEADERS inth;
		PVOID pvPerfAddr = NULL;
		Toolhelp32ReadProcessMemory(dwPid, me.modBaseAddr, &idh, sizeof(idh), NULL);
		if (idh.e_magic == IMAGE_DOS_SIGNATURE) {
			Toolhelp32ReadProcessMemory(dwPid, me.modBaseAddr + idh.e_lfanew, &inth, sizeof(inth), NULL);
			if (inth.Signature == IMAGE_NT_SIGNATURE) {
				pvPerfAddr = (PVOID)inth.OptionalHeader.ImageBase;
			}
		}

		// Try to format the size in kb.
		TCHAR szFormattedSize[64];
		if (StrFormatKBSize(me.modBaseSize, szFormattedSize, _countof(szFormattedSize)) == NULL) {
			StringCchPrintf(szFormattedSize, _countof(szFormattedSize), TEXT("%10u"), me.modBaseSize);
		}

		AddText(TEXT("\t\t%p(%p) %20s    %10s\r\n"), me.modBaseAddr, pvPerfAddr, szFormattedSize, me.szExePath);
		fOk = th2.ModuleNext(&me);
	}


	// show thread info
	AddText(TEXT("\r\nThread Information:\r\n")
		TEXT("  TID  \t\tPriority\r\n"));
	THREADENTRY32 te = {sizeof(te)};
	fOk = th2.ThreadFirst(&te);
	while (fOk) {
		if (te.th32OwnerProcessID == dwPid) {
			AddText(TEXT("   %08X       %2d\r\n"),
				te.th32ThreadID, te.tpBasePri);
		}
		fOk = th2.ThreadNext(&te);
	}

	m_infoEdit->SetWindowText(m_str.data());
}

void CProcessGUIDlg::ShowModuleInfo(LPCWSTR pszName) {
	m_infoEdit->SetWindowText(L"");
	m_str.clear();

	CToolhelp th(TH32CS_SNAPPROCESS);
	PROCESSENTRY32 pe = { sizeof(pe) };
	BOOL fOk = th.ProcessFirst(&pe);

	AddText(TEXT("Pathname: %s\r\n\r\n"), pszName);
	AddText(TEXT("Process Information:\r\n"));
	AddText(TEXT("     PID    %10s          Process\r\n"), TEXT("BaseAddr"));

	while (fOk) {
		CToolhelp th2(TH32CS_SNAPMODULE, pe.th32ProcessID);
		MODULEENTRY32 me = { sizeof(me) };
		BOOL fOk2 = th2.ModuleFirst(&me);

		while (fOk2) {
			TCHAR* str = _tcsrchr(me.szExePath, L'\\');
			str++;

			if (_tcscmp(str, pszName) == 0) {
				AddText(TEXT("  %08X    %p  %s\r\n"), pe.th32ProcessID, me.modBaseAddr, pe.szExeFile);
			}
			fOk2 = th2.ModuleNext(&me);
		}

		fOk = th.ProcessNext(&pe);
	}

	m_infoEdit->SetWindowText(m_str.data());
}

void CProcessGUIDlg::OnSelchangeDroplist() {
	int idx = m_dropList->GetCurSel();

	if (!m_isModule) {
		DWORD pid = m_dropList->GetItemData(idx);
		ShowProcessInfo(pid);
	}
	else {
		TCHAR buf[256] = { '\0' };
		m_dropList->GetLBText(idx, buf);
		ShowModuleInfo(buf);
	}
}