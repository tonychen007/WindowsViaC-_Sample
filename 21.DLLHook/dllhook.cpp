#define MYDLL extern "C" __declspec(dllexport)

#include "dllhook.h"
#include "resource.h"

HINSTANCE g_hInstDll = NULL;
static const TCHAR g_szRegSubKey[] = L"Software\\TonyChen\\Desktop Item Position Saver";


#pragma data_seg("Shared")
HHOOK g_hHook = NULL;
DWORD g_dwThreadIdDIPS = 0;
#pragma data_seg()

// Instruct the linker to make the Shared section
// readable, writable, and shared.
#pragma comment(linker, "/section:Shared,rws")


LRESULT WINAPI GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam);
INT_PTR WINAPI DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void DlgOnClose(HWND hWnd);

void SaveListViewItemPositions(HWND hWndLV);
void RestoreListViewItemPositions(HWND hWndLV);

MYDLL BOOL WINAPI SetHook(DWORD threadID) {
	BOOL ret = FALSE;

    if (threadID) {
        if (g_hHook != NULL)
            goto end;

        g_dwThreadIdDIPS = GetCurrentThreadId();
        g_hHook = SetWindowsHookExW(WH_GETMESSAGE, GetMsgProc, g_hInstDll, threadID);
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

INT_PTR WINAPI DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE:
        DlgOnClose(hWnd);
        break;
    case WM_APP:
        if (lParam)
            SaveListViewItemPositions((HWND)wParam);
        else
            RestoreListViewItemPositions((HWND)wParam);
        break;
    }

    return(FALSE);
}

void DlgOnClose(HWND hWnd) {
    DestroyWindow(hWnd);
}


void SaveListViewItemPositions(HWND hWndLV) {

}

void RestoreListViewItemPositions(HWND hWndLV) {

}

