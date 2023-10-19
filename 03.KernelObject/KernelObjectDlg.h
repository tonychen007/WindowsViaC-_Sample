
// KernelObjectDlg.h: 標頭檔
//

#pragma once

#include <string>

// CKernelObjectDlg 對話方塊
class CKernelObjectDlg : public CDialog
{
// 建構
public:
	CKernelObjectDlg(CWnd* pParent = nullptr);	// 標準建構函式

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_KERNELOBJECT_DIALOG };
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
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	DECLARE_MESSAGE_MAP()

	void TestCloseProtectHandle();
public:
	void InitControls();
	void InitFont();
	void AppendText(LPCWSTR pszFormat, ...);
	void ClearText();

	CString* GetObjectName();
	BOOL GetInheritChk();
	void GetSecurityAttr();
	void AppendErrorText(DWORD errCode, LPCWSTR pszFormat, ...);
	void AppendSuccessAndErrorText(HANDLE handle, DWORD dwErrorCode, LPCWSTR pszErrFormat, LPCWSTR pszOKFormat);

	void CheckInstance();

private:
	CFont m_font;
	CEdit* m_editText;
	std::wstring m_editStr;
	CEdit* m_editObjName;
	CButton *m_interitChk;
	
	CButton* m_clearBtn;
	CButton* m_createMutexBtn;
	CButton* m_createSemaPhBtn;

	SECURITY_ATTRIBUTES m_sa;
	HANDLE m_hSingleton;
	HANDLE m_hBoundary;
	HANDLE m_hNamespace;
	HANDLE m_hLastNamespace;
	BOOL m_isNamespaceOpened;

	static const int STRING_BUFFER = 10 * 1024;
	static PCWSTR s_szBoundary;
	static PCWSTR s_szNamespace;

public:
	afx_msg void OnBnClickedCleartext();
	afx_msg void OnChangeEdit();
	afx_msg void OnBnClickedCreateMutexBtn();
	afx_msg void OnBnClickedCreateSemaphBtn();
	afx_msg void OnBnClickedOpenMutexBtn();
	afx_msg void OnBnClickedOpenSemaphBtn();
	afx_msg void OnBnClickedCloseNamespaceBtn();
	afx_msg void OnBnClickedCreatNamespaceBtn();
	afx_msg void OnBnClickedCreatePrivateMutexBtn();
	afx_msg void OnBnClickedOpenNamespaceBtn();
};
