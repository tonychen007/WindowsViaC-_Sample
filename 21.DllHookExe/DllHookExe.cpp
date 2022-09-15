#include <stdio.h>
#include <locale.h>
#include "../21.DLLHook/dllhook.h"

TCHAR g_szRegKey[] = L"Software\\TonyChen\\Desktop Item Position Saver";

void TestGetDesktopListViewWithoutDLLInject();
void TestGetDesktopListViewWithDLLInject();
void TestKeyboardInject();


int main() {
	printf("TestGetDesktopListViewWithoutDLLInject\n");
	//TestGetDesktopListViewWithoutDLLInject();

	printf("\n");
	printf("TestGetDesktopListViewWithDLLInject\n");
	//TestGetDesktopListViewWithDLLInject();

	printf("\n");
	printf("TestKeyboardInject\n");
	TestKeyboardInject();
}

void TestGetDesktopListViewWithoutDLLInject() {
	HWND hWndDesktop = GetFirstChild(GetFirstChild(FindWindow(TEXT("Progman"), NULL)));
	DWORD dwThreadId = GetWindowThreadProcessId(hWndDesktop, NULL);

	SaveListViewItemPositions(hWndDesktop);
}

void TestGetDesktopListViewWithDLLInject() {
	MSG msg;
	HKEY hKey;
	int i = 0;
	LONG lRet;
	TCHAR subKey[MAX_PATH] = { 0 };
	POINT pt;
	HWND hWndDesktop = GetFirstChild(GetFirstChild(FindWindow(TEXT("Progman"), NULL)));
	DWORD dwThreadId = GetWindowThreadProcessId(hWndDesktop, NULL);

	if (!SetHook(dwThreadId, WH_GETMESSAGE, GetMsgProc)) {
		printf("Set hook failed\n");
		return;
	}

	//MessageBox(NULL, L"Inject to Explore OK. Use attach DLL to debug!!", L"", MB_OK);
	PostThreadMessage(dwThreadId, WM_NULL, 0, 0);
	GetMessage(&msg, NULL, 0, 0);

	HWND hWndDIPS = FindWindow(NULL, L"Dummy Tony Chen");
	SendMessage(hWndDIPS, WM_APP, (WPARAM)hWndDesktop, 1);
	SendMessage(hWndDIPS, WM_CLOSE, 0, 0);

	// GBK
	setlocale(LC_ALL, ".936");
	// UTF8
	SetConsoleOutputCP(65001);

	// Get RegValue
	RegOpenKeyEx(HKEY_CURRENT_USER, g_szRegKey, 0, KEY_READ | KEY_QUERY_VALUE, &hKey);

	while (1) {
		ZeroMemory(subKey, sizeof(subKey));
		ZeroMemory(&pt, sizeof(pt));
		DWORD sizeKey = _countof(subKey);
		DWORD szCbData = sizeof(pt);
		DWORD dwType = 0;

		lRet = RegEnumValue(hKey, i, subKey, &sizeKey, NULL, &dwType, (LPBYTE)&pt, &szCbData);
		if (lRet != ERROR_SUCCESS)
			break;
		i++;

		wprintf(L"Name: %ls, Pos: (%d, %d)\n", subKey, pt.x, pt.y);
	}
}

void TestKeyboardInject() {
	STARTUPINFO si = {sizeof(si)};
	PROCESS_INFORMATION pi;
	HWND hWndNotepad = NULL;

	CreateProcess(L"C:\\Windows\\NOTEPAD.EXE", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

	while (1) {
		hWndNotepad = FindWindow(TEXT("Notepad"), NULL);
		if (hWndNotepad != NULL)
			break;
		Sleep(100);
	}

	DWORD dwThreadId = GetWindowThreadProcessId(hWndNotepad, NULL);

	printf("Try to inject into notepad\n");
	if (dwThreadId == 0) {
		printf("Notepad is not running\n");
		return;
	}

	if (!SetHook(dwThreadId, WH_KEYBOARD, GetKeyboardProc)) {
		printf("Set hook failed\n");
		return;
	}

	PostThreadMessage(dwThreadId, WM_KEYDOWN, 0, 0);
	printf("Press key to end...\n");
	getchar();
	TerminateProcess(pi.hProcess, 0);
}