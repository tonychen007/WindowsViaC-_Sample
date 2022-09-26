#include <stdio.h>
#include <Windows.h>

#define ADDR_WIN_SIZE 1.9*1024*1024*1024

int EnablePrivilege(LPCWSTR name);

int main() {
#ifdef X64
	return - 1;
#else
    EnablePrivilege(SE_LOCK_MEMORY_NAME);

    LPVOID win;
    DWORD dwPageSize;
    ULONG_PTR uRamPage;
    ULONG_PTR* uRamFrameArray1;
    ULONG_PTR* uRamFrameArray2;
    SYSTEM_INFO sysInfo;
    BOOL ret;

    GetSystemInfo(&sysInfo);
    dwPageSize = sysInfo.dwPageSize;
    uRamPage = (ADDR_WIN_SIZE + dwPageSize - 1) / dwPageSize;

    win = VirtualAlloc(NULL, ADDR_WIN_SIZE, MEM_RESERVE | MEM_PHYSICAL, PAGE_READWRITE);
    if (!win)
        return -1;

    uRamFrameArray1 = (ULONG_PTR*)HeapAlloc(GetProcessHeap(), 0, sizeof(ULONG_PTR) * uRamPage);
    uRamFrameArray2 = (ULONG_PTR*)HeapAlloc(GetProcessHeap(), 0, sizeof(ULONG_PTR) * uRamPage);

    AllocateUserPhysicalPages(GetCurrentProcess(), &uRamPage, uRamFrameArray1);
    AllocateUserPhysicalPages(GetCurrentProcess(), &uRamPage, uRamFrameArray2);

    MapUserPhysicalPages(win, uRamPage, uRamFrameArray1);
    printf("Mapped ram block 1 to address win successfully\n");
    printf("copy something into address win:\n\n");
    strcpy_s((char*)win, ADDR_WIN_SIZE, "123");

    MapUserPhysicalPages(win, uRamPage, uRamFrameArray2);
    printf("Mapped ram block 2 to address win successfully\n");
    printf("copy something into address win:\n\n");
    strcpy_s((char*)win, ADDR_WIN_SIZE, "456");

    MapUserPhysicalPages(win, uRamPage, uRamFrameArray1);
    printf("ram block 1 is %s\n", win);
    MapUserPhysicalPages(win, uRamPage, uRamFrameArray2);
    printf("ram block 2 is %s\n", win);
end:
    FreeUserPhysicalPages(GetCurrentProcess(), &uRamPage, uRamFrameArray1);
    FreeUserPhysicalPages(GetCurrentProcess(), &uRamPage, uRamFrameArray2);
    VirtualFree(win, 0, MEM_RELEASE);
    HeapFree(GetProcessHeap(), 0, uRamFrameArray1);
    HeapFree(GetProcessHeap(), 0, uRamFrameArray2);
#endif
}

int EnablePrivilege(LPCWSTR name) {
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    int ret;

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
    if (!ret)
        goto end;

    ret = LookupPrivilegeValue(NULL, name, &tp.Privileges[0].Luid);
    if (!ret) {
        goto end;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    ret = AdjustTokenPrivileges(hToken, 0, &tp, sizeof(tp), 0, 0);
    if (!ret) {
        goto end;
    }

end:
    CloseHandle(hToken);
    return ret;
}