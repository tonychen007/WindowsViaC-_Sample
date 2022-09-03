
// KernelObjectDlg.cpp: 實作檔案
//

#include <strsafe.h>
#include <vector>

#include "framework.h"
#include "KernelObject.h"
#include "KernelObjectDlg.h"
#include "afxdialogex.h"

#include <sddl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;


LONG ExceptionFilter(EXCEPTION_POINTERS* excep, DWORD& errCode) {
	DWORD err = excep->ExceptionRecord->ExceptionCode;
	errCode = err;

	return EXCEPTION_EXECUTE_HANDLER;
}

PCWSTR CKernelObjectDlg::s_szBoundary = L"Tony-Boundary";
PCWSTR CKernelObjectDlg::s_szNamespace = L"Tony-Namespace";

CKernelObjectDlg::CKernelObjectDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_KERNELOBJECT_DIALOG, pParent),
	m_hSingleton(NULL),
	m_hBoundary(NULL),
	m_hNamespace(NULL),
	m_hLastNamespace(NULL),
	m_isNamespaceOpened(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CKernelObjectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CKernelObjectDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_CLEARTEXT, &CKernelObjectDlg::OnBnClickedCleartext)
	ON_EN_CHANGE(IDC_EDIT_TEXT, &CKernelObjectDlg::OnChangeEdit)
	ON_BN_CLICKED(IDC_CREATE_MUTEX_BTN, &CKernelObjectDlg::OnBnClickedCreateMutexBtn)
	ON_BN_CLICKED(IDC_CREATE_SEMAPH_BTN, &CKernelObjectDlg::OnBnClickedCreateSemaphBtn)
	ON_BN_CLICKED(IDC_OPEN_MUTEX_BTN, &CKernelObjectDlg::OnBnClickedOpenMutexBtn)
	ON_BN_CLICKED(IDC_OPEN_SEMAPH_BTN, &CKernelObjectDlg::OnBnClickedOpenSemaphBtn)
	ON_BN_CLICKED(IDC_CREAT_NAMESPACE_BTN, &CKernelObjectDlg::OnBnClickedCreatNamespaceBtn)
	ON_BN_CLICKED(IDC_CLOSE_NAMESPACE_BTN, &CKernelObjectDlg::OnBnClickedCloseNamespaceBtn)
	ON_BN_CLICKED(IDC_CREATE_PRIVATE_MUTEX_BTN, &CKernelObjectDlg::OnBnClickedCreatePrivateMutexBtn)
	ON_BN_CLICKED(IDC_OPEN_NAMESPACE_BTN, &CKernelObjectDlg::OnBnClickedOpenNamespaceBtn)
END_MESSAGE_MAP()


BOOL CKernelObjectDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// 設定大圖示
	SetIcon(m_hIcon, FALSE);		// 設定小圖示

	InitControls();
	InitFont();

	//TestCloseProtectHandle();

	return TRUE;  // 傳回 TRUE，除非您對控制項設定焦點
}

void CKernelObjectDlg::OnPaint()
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


HCURSOR CKernelObjectDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CKernelObjectDlg::TestCloseProtectHandle() {
	DWORD errCode = 0;

	__try {
		HANDLE m1 = CreateMutex(NULL, FALSE, NULL);
		SetHandleInformation(m1, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE);
		CloseHandle(m1);
	}
	__except (ExceptionFilter(GetExceptionInformation(), errCode)) {
		AppendText(L"Close Handle Failed. Because set HANDLE_FLAG_PROTECT_FROM_CLOSE on Handle. The exception code is : 0x%llX\r\n\r\n", errCode);
	}
}

void CKernelObjectDlg::InitControls() {
	m_editText = (CEdit*)GetDlgItem(IDC_EDIT_TEXT);
	m_clearBtn = (CButton*)GetDlgItem(IDC_CLEARTEXT);
	m_editStr.reserve(STRING_BUFFER);

	m_createMutexBtn = (CButton*)GetDlgItem(IDC_CREATE_MUTEX_BTN);
	m_createSemaPhBtn = (CButton*)GetDlgItem(IDC_CREATE_SEMAPH_BTN);
	m_editObjName = (CEdit*)GetDlgItem(IDC_EDIT_OBJ_NAME);
	m_interitChk = (CButton*)GetDlgItem(IDC_INHERIT_CHK);

	m_sa.nLength = sizeof(m_sa);
	m_sa.lpSecurityDescriptor = NULL;
	m_sa.bInheritHandle = FALSE;
}

void CKernelObjectDlg::InitFont() {
	LOGFONT lf;
	HFONT hf;
	CDC* pDC = m_editText->GetDC();
	HDC hdc = pDC->GetSafeHdc();
	int fz;

	hf = (HFONT)m_editText->SendMessage(WM_GETFONT, 0, 0);
	GetObject(hf, sizeof(LOGFONT), &lf);

	fz = -lf.lfHeight * 72 / GetDeviceCaps(hdc, LOGPIXELSY);
	fz *= 1.1f;
	lf.lfHeight = -fz * GetDeviceCaps(hdc, LOGPIXELSY) / 72;

	m_font.CreateFontIndirectW(&lf);
	m_editText->SetFont(&m_font);

	ReleaseDC(pDC);
}

void CKernelObjectDlg::AppendText(LPCWSTR pszFormat, ...) {
	va_list args;
	va_start(args, pszFormat);

	int len = _vscwprintf(pszFormat, args) * 2;
	vector<TCHAR> vec(len);
	_vsnwprintf_s(vec.data(), len, len, pszFormat, args);
	m_editStr.append(vec.data());
	m_editText->SetWindowText(m_editStr.c_str());
	m_editText->LineScroll(m_editText->GetLineCount());
	OnChangeEdit();

	va_end(args);
}

void CKernelObjectDlg::ClearText() {
	if (m_editStr.size() == 0)
		return;

	m_editStr.clear();
	m_editStr.shrink_to_fit();
	m_editText->SetWindowText(L"");
}

CString* CKernelObjectDlg::GetObjectName() {
	CString* str = new CString();
	m_editObjName->GetWindowText(*str);
	str->Trim();

	return str;
}

BOOL CKernelObjectDlg::GetInheritChk() {
	return m_interitChk->GetCheck();
}

void CKernelObjectDlg::GetSecurityAttr() {
	m_sa.bInheritHandle = GetInheritChk();
}

void CKernelObjectDlg::AppendErrorText(DWORD errCode, LPCWSTR pszFormat, ...) {
	AppendText(pszFormat, errCode);
}

void CKernelObjectDlg::AppendSuccessAndErrorText(HANDLE handle, DWORD dwErrorCode, LPCWSTR pszErrFormat, LPCWSTR pszOKFormat) {
	if (handle == NULL || handle == INVALID_HANDLE_VALUE || dwErrorCode) {
		LPCWSTR aleStr = L"Object already existed. The handle is: 0x%llX.\r\n\r\n";
		LPCWSTR errStr = (dwErrorCode == ERROR_ALREADY_EXISTS) ? aleStr : pszErrFormat;
		DWORD code = (dwErrorCode == ERROR_ALREADY_EXISTS) ? (DWORD)handle : dwErrorCode;
		AppendErrorText(code, errStr, code);
	}
	else {
		AppendText(pszOKFormat, handle);
	}
}

// for message
HBRUSH CKernelObjectDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) {
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	switch (pWnd->GetDlgCtrlID()) {
	case IDC_EDIT_TEXT:
		pDC->SetBkMode(TRANSPARENT);
		pDC->SetTextColor(RGB(255, 255, 0));
		return CreateSolidBrush(RGB(0, 0, 90));
	}

	return hbr;
}

void CKernelObjectDlg::OnBnClickedCleartext() {
	ClearText();
	OnChangeEdit();
}

void CKernelObjectDlg::OnChangeEdit() {
	m_clearBtn->EnableWindow(m_editStr.size() > 0);
}

void CKernelObjectDlg::OnBnClickedCreateMutexBtn() {
	CString* name = GetObjectName();
	GetSecurityAttr();

	HANDLE hMutex = CreateMutex(&m_sa, FALSE, name->GetString());
	DWORD dwErrorCode = GetLastError();
	AppendSuccessAndErrorText(
		hMutex,
		dwErrorCode,
		L"Create Mutex error, the err code is: 0x%llX\r\n\r\n",
		L"Create Mutex successfully. The handle is: 0x%llX\r\n\r\n");

	delete name;
}

void CKernelObjectDlg::OnBnClickedCreateSemaphBtn() {
	CString* name = GetObjectName();
	GetSecurityAttr();

	HANDLE hSem = CreateSemaphore(&m_sa, 1, 1, name->GetString());
	DWORD dwErrorCode = GetLastError();
	AppendSuccessAndErrorText(
		hSem,
		dwErrorCode,
		L"Create Semaphore error, the err code is: 0x%llX\r\n\r\n",
		L"Create Semaphore successfully. The handle is: 0x%llX\r\n\r\n");

	delete name;
}


void CKernelObjectDlg::OnBnClickedOpenMutexBtn() {
	CString* name = GetObjectName();
	GetSecurityAttr();

	HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, m_sa.bInheritHandle, name->GetString());
	DWORD dwErrorCode = GetLastError();
	AppendSuccessAndErrorText(
		hMutex,
		dwErrorCode,
		L"Open Mutex error, the err code is: 0x%llX\r\n\r\n",
		L"Open Mutex successfully. The handle is: 0x%llX\r\n\r\n");

	delete name;
}


void CKernelObjectDlg::OnBnClickedOpenSemaphBtn() {
	CString* name = GetObjectName();
	GetSecurityAttr();

	HANDLE hSem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, m_sa.bInheritHandle, name->GetString());
	DWORD dwErrorCode = GetLastError();
	AppendSuccessAndErrorText(
		hSem,
		dwErrorCode,
		L"Open Semaphore error, the err code is: 0x%llX\r\n\r\n",
		L"Open Semaphore successfully. The handle is: 0x%llX\r\n\r\n");

	delete name;
}

void CKernelObjectDlg::OnBnClickedCreatNamespaceBtn() {
	BOOL ret;
	PSID psidAdministrators;
	DWORD dwErrorCode;

	m_hBoundary = CreateBoundaryDescriptor(s_szBoundary, 0);

	// create admin sid
	SID_IDENTIFIER_AUTHORITY sidNtAuthority = SECURITY_NT_AUTHORITY;
	ret = AllocateAndInitializeSid(&sidNtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0, &psidAdministrators);

	// extract sid info
	SID* sid = (SID*)psidAdministrators;
	LPCWSTR sidStr = L"SID Version: \t\t%d\r\nSID Subcount: \t\t%d\r\nSID Identifier-Authority: \t{%d,%d,%d,%d,%d,%d}\r\n";
	LPCWSTR sidSubAuthStr = L"SID Sub-Authority[%d]: \t0x%llX\r\n";
	AppendText(sidStr, sid->Revision, sid->SubAuthorityCount,
		sid->IdentifierAuthority.Value[0], sid->IdentifierAuthority.Value[1], sid->IdentifierAuthority.Value[2],
		sid->IdentifierAuthority.Value[3], sid->IdentifierAuthority.Value[4], sid->IdentifierAuthority.Value[5]);
	for (int i = 0; i < sid->SubAuthorityCount; i++) {
		AppendText(sidSubAuthStr, i, sid->SubAuthority[i]);
	}

	TCHAR* SIDString;
	ConvertSidToStringSid(psidAdministrators, &SIDString);
	AppendText(L"SID String is: \t\t%ws\r\n\r\n", SIDString);
	LocalFree(SIDString);

	if (!AddSIDToBoundaryDescriptor(&m_hBoundary, psidAdministrators)) {
		AppendErrorText(GetLastError(), L"Failed to addSID to boundary descriptror.\r\n\r\n");
	}

	// ace_type;ace_flags;rights;object_guid;inherit_object_guid;account_sid;(resource_attribute)
	// see aces.html

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = FALSE;
	LPCWSTR seStr = L"D:(A;;GA;;;BA)";
	if (!ConvertStringSecurityDescriptorToSecurityDescriptor(seStr, SDDL_REVISION_1, &sa.lpSecurityDescriptor, NULL)) {
		AppendText(L"Security Descriptor creation failed: %u\r\n\r\n", GetLastError());
		return;
	}

	m_hNamespace = CreatePrivateNamespace(&sa, m_hBoundary, s_szNamespace);
	dwErrorCode = GetLastError();
	AppendSuccessAndErrorText(m_hNamespace, dwErrorCode,
		L"Create private namespace failed!\r\n\r\n",
		L"Create private namespace with successfully.\r\n\r\n");
	FreeSid(psidAdministrators);
	if (m_hNamespace)
		m_hLastNamespace = m_hNamespace;
	else if (dwErrorCode == ERROR_ALREADY_EXISTS) {
		m_hNamespace = m_hLastNamespace;
	}
}

void CKernelObjectDlg::OnBnClickedCreatePrivateMutexBtn() {
	TCHAR szMutexName[128] = { '\0' };
	StringCchPrintf(szMutexName, _countof(szMutexName), L"%s\\%s", s_szNamespace, L"TonyApp");

	m_hSingleton = CreateMutex(NULL, FALSE, szMutexName);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		// There is already an instance of this Singleton object
		AppendText(L"Another instance of Singleton is running:\r\n");
		AppendText(L"--> Impossible to access application features.\r\n\r\n");
	}
	else {
		// First time the Singleton object is created
		AppendText(L"First instance of Singleton:\r\n");
		AppendText(L"--> Access application features now.\r\n\r\n");
	}
}

void CKernelObjectDlg::OnBnClickedOpenNamespaceBtn() {
	// check if already exist, and try to open the namespace
	m_hNamespace = OpenPrivateNamespace(m_hBoundary, s_szNamespace);
	DWORD dwErrorCode = GetLastError();
	AppendSuccessAndErrorText(m_hNamespace, dwErrorCode,
		L"Open private namespace failed!\r\n",
		L"Open private namespace with successfully.\r\n");

	if (m_hNamespace) {
		m_hLastNamespace = m_hNamespace;
		m_isNamespaceOpened = TRUE;
	}
	else {
		m_hNamespace = m_hLastNamespace;
	}

	if (dwErrorCode == ERROR_DUP_NAME) {
		AppendText(L"Duplicate namespace.\r\n\r\n");
	}
	else if (dwErrorCode == ERROR_NOACCESS) {
		AppendText(L"Maybe not boundary descriptor. Try to create namespace frist.\r\n\r\n");
	}
}

void CKernelObjectDlg::OnBnClickedCloseNamespaceBtn() {
	BOOL ret = ClosePrivateNamespace(m_hNamespace, m_isNamespaceOpened ? PRIVATE_NAMESPACE_FLAG_DESTROY : 0);
	if (ret) {
		m_isNamespaceOpened = FALSE;
		m_hNamespace = 0;
	}
	AppendText(L"Close private namespace: %lld\r\n\r\n", ret);

	if (m_hBoundary) {
		DeleteBoundaryDescriptor(m_hBoundary);
		m_hBoundary = 0;
	}
}