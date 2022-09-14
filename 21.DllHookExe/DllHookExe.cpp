#include <stdio.h>
#include <locale.h>
#include "../21.DLLHook/dllhook.h"

void TestGetDesktopListViewWithoutDLLInject();
void TestGetDesktopListViewWithDLLInject();
TCHAR g_szRegKey[] = L"Software\\TonyChen\\Desktop Item Position Saver";

int main() {
	printf("TestGetDesktopListViewWithoutDLLInject\n");
	//TestGetDesktopListViewWithoutDLLInject();

	printf("\n");
	printf("TestGetDesktopListViewWithDLLInject\n");
	TestGetDesktopListViewWithDLLInject();
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

	if (!SetHook(dwThreadId)) {
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
