#include <stdio.h>
#include "../21.DLLHook/dllhook.h"

void TestGetDesktopListViewWithoutDLLInject();
void TestGetDesktopListViewWithDLLInject();

MSG msg;
HWND hWndDesktop = GetFirstChild(GetFirstChild(FindWindow(TEXT("Progman"), NULL)));
DWORD dwThreadId = GetWindowThreadProcessId(hWndDesktop, NULL);

int main() {
    printf("TestGetDesktopListViewWithoutDLLInject\n");
    TestGetDesktopListViewWithoutDLLInject();

    printf("\n");
    printf("TestGetDesktopListViewWithDLLInject\n");
    //TestGetDesktopListViewWithDLLInject();
}

void TestGetDesktopListViewWithoutDLLInject() {

}

void TestGetDesktopListViewWithDLLInject() {
    if (!SetHook(dwThreadId)) {
        printf("Set hook failed\n");
        return;
    }

    MessageBox(NULL, L"Inject to Explore OK. Use attach DLL to debug!!", L"", MB_OK);
    PostThreadMessage(dwThreadId, WM_NULL, 0, 0);
    GetMessage(&msg, NULL, 0, 0);

    HWND hWndDIPS = FindWindow(NULL, L"Dummy Tony Chen");
    SendMessage(hWndDIPS, WM_APP, 0, 1);
    SendMessage(hWndDIPS, WM_CLOSE, 0, 0);    
}
