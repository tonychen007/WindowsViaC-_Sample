#define MYDLL extern "C" __declspec(dllexport)

#include "dllhook.h"
#include "resource.h"

HINSTANCE g_hInstDll = NULL;
TCHAR g_szRegSubKey[] = L"Software\\TonyChen\\Desktop Item Position Saver";

#pragma data_seg("Shared")
HHOOK g_hHook = NULL;
DWORD g_dwThreadIdDIPS = 0;
#pragma data_seg()

// Instruct the linker to make the Shared section
// readable, writable, and shared.
#pragma comment(linker, "/section:Shared,rws")

LRESULT WINAPI GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI GetKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

INT_PTR WINAPI DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void DlgOnClose(HWND hWnd);

MYDLL BOOL WINAPI SetHook(DWORD threadID, int hookID, hookFn fn) {
	BOOL ret = FALSE;

	if (threadID) {
		if (g_hHook != NULL)
			goto end;

		g_dwThreadIdDIPS = GetCurrentThreadId();
		g_hHook = SetWindowsHookExW(hookID, fn, g_hInstDll, threadID);
		ret = (g_hHook != NULL);
	}
	else {
		if (g_hHook == NULL)
			goto end;
		ret = UnhookWindowsHookEx(g_hHook);
		g_hHook = NULL;
	}
end:
	return ret;
}

LRESULT WINAPI GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam) {
	static BOOL bFirstTime = TRUE;

	if (bFirstTime) {
		bFirstTime = FALSE;

		CreateDialog(g_hInstDll, MAKEINTRESOURCE(IDD_DUMMY_DLG), NULL, DlgProc);
		PostThreadMessage(g_dwThreadIdDIPS, WM_NULL, 0, 0);
	}

	return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

LRESULT WINAPI GetKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	static BOOL bFirstTime = TRUE;
	static HANDLE hConsole;
	LPCWSTR pszConsoleHello = L"DLL KeyboardProc Conosle!";

	if (bFirstTime) {
		bFirstTime = FALSE;

		AllocConsole();
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		WriteConsole(hConsole, pszConsoleHello, lstrlen(pszConsoleHello), 0, 0);
	}

	if (nCode == HC_ACTION) {
		BYTE ks[256];
		TCHAR buf[256];
		TCHAR buf2[8];
		GetKeyboardState(ks);
		//ToAscii(wParam, 0, ks, &w, 0);
		ToUnicode(wParam, 0, ks, buf2, _countof(buf2), 0);
		wsprintf(buf, L"The char is : %s\n", buf2);
		WriteConsole(hConsole, buf, lstrlen(buf), 0, 0);
	}

	return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

INT_PTR WINAPI DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CLOSE:
		DlgOnClose(hWnd);
		break;
	case WM_APP:
		SaveListViewItemPositions((HWND)wParam);
		break;
	}

	return(FALSE);
}

void DlgOnClose(HWND hWnd) {
	DestroyWindow(hWnd);
}

VOID SaveListViewItemPositions(HWND hWndLV) {
	HKEY hKey;
	LONG l;
	int nMaxItems = ListView_GetItemCount(hWndLV);

	l = RegDeleteKey(HKEY_CURRENT_USER, g_szRegSubKey);
	l = RegCreateKeyEx(HKEY_CURRENT_USER, g_szRegSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL);
	if (l != ERROR_SUCCESS) {
		return;
	}

	for (int i = 0; i < nMaxItems; i++) {
		TCHAR szName[MAX_PATH];
		POINT pt;

		ListView_GetItemText(hWndLV, i, 0, szName, _countof(szName));
		ListView_GetItemPosition(hWndLV, i, &pt);
		if (GetLastError() == ERROR_INVALID_WINDOW_HANDLE) {
			printf("Without dll inject, get item name failed\n");
			break;
		}
		RegSetValueEx(hKey, szName, 0, REG_BINARY, (PBYTE)&pt, sizeof(pt));
	}

	RegCloseKey(hKey);
}
