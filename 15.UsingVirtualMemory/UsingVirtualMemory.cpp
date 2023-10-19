
#include <assert.h>
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
void TestChangeProtectType();
void TestMemReset(int accessLater = 0);

int main() {
    printf("TestLargeMemoryPage at bootom\n");
    TestLargeMemoryPage();

    printf("\n");
    printf("TestLargeMemoryPage at top\n");
    TestLargeMemoryPage(1);

    printf("\n");
    printf("TestChangeProtectType\n");
    //TestChangeProtectType();

    printf("\n");
    printf("TestMemReset no access later\n");
    TestMemReset();

    printf("\n");
    printf("TestMemReset access later\n");
    TestMemReset(1);
}

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

void TestChangeProtectType() {
    int page = 4096;
    int size = page * 8;
    DWORD dwOld;
    char* buf = (char*)VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    printf("Alloc 8 pages\n");
    *((char*)buf + page + 1) = '1';
    *((char*)buf + page*3 + 1) = '3';
    printf("Modify 2nd and 4th page\n");

    printf("Try to modify 2nd and 4th page to PAGE_NOACCESS\n");
    VirtualProtect(buf + page, page, PAGE_NOACCESS, &dwOld);
    VirtualProtect(buf + page*3, page, PAGE_NOACCESS, &dwOld);

    printf("Modify 2nd and 4th page\n");
    __try {
        *((char*)buf + page + 1) = '1';
        *((char*)buf + page * 3 + 1) = '3';
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        printf("Error occurs\n");
    }

    VirtualFree(buf, 0, MEM_RELEASE);
}

void TestMemReset(int accessLater) {
    const char* psz = "Tony Chen";
    const char* pszMsg = "To see MEM_TEST effect, virtual memory size should be the same as RAM!!!";
    int page = 4096;
    int size = page * 3;
    MEMORYSTATUS mst;
    LPVOID buf;
    LPVOID data;
    LPVOID pvDummy = NULL;

#pragma message(pszMsg)
    data = VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    strcpy_s((char*)data, size, psz);

    if (!accessLater) {
        buf = VirtualAlloc((void*)data, page, MEM_RESET, PAGE_READWRITE);
        assert(buf == data);
    }

    printf("%s\n", pszMsg);
    GlobalMemoryStatus(&mst);
    pvDummy = VirtualAlloc(NULL, mst.dwTotalPhys, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if (pvDummy != NULL) {
        ZeroMemory(pvDummy, mst.dwTotalPhys);
    }

    if (strcmp((char*)data, psz) == 0) {
        printf("ZeroMemory forced the page to be written to the paging file\n");
    }
    else {
        printf("ZeroMemory did not forced the page to be written to the paging file\n");
    }

    printf("The data is : %s\n", data);
    if (pvDummy)
        VirtualFree(pvDummy, 0, MEM_RELEASE);

    VirtualFree(data, 0, MEM_RELEASE);

    printf("DO NOT FORGET TO SET BACK VIRTUAL MEMORY SIZE.\n");
}