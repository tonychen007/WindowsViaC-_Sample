
#include <stdio.h>
#include <stdint.h>
#include <Windows.h>

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

void TestLargeMemoryPage(int isTopDown = 0);

void TestLargeMemoryPage(int isTopDown) {
    int ret = EnablePrivilege(SE_LOCK_MEMORY_NAME);
    if (!ret)
        return;

    uint64_t size = 1024 * 1024 * 1024 * 4LL;
    size_t largePageSize = GetLargePageMinimum();
    DWORD flags = MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES;
    flags = (isTopDown) ? (flags | MEM_TOP_DOWN) : flags;
    LPVOID buf = VirtualAlloc(NULL, size, flags, PAGE_READWRITE);
    if (buf != NULL) {
        printf("Allocate large memory %llu successfully at %p\n", size, buf);
    }

    VirtualFree(buf, 0, MEM_RELEASE);
}

int main() {
    printf("TestLargeMemoryPage at bootom\n");
    TestLargeMemoryPage();

    printf("\n");
    printf("TestLargeMemoryPage at top\n");
    TestLargeMemoryPage(1);
}