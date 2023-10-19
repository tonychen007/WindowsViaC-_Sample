#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <Windows.h>
#include <Psapi.h>

void TestMemoryRegion();
void TestLargeAlloc();
void TestVirtualAllocRoundup();
void TestVirtualAllocReservedAndCommit();

int main() {
#ifdef X64
	printf("TestMemoryRegion\n");
	TestMemoryRegion();
	printf("\n");
	printf("TestVirtualAllocRoundup\n");
	TestVirtualAllocRoundup();

	printf("\n");
	printf("TestVirtualAllocReservedAndCommit\n");
	TestVirtualAllocReservedAndCommit();
#endif

	printf("\n");
	printf("TestLargeAddrAware\n");
	TestLargeAlloc();
}

void TestMemoryRegion() {
	int64_t addr = 0x00000010;
	LPVOID buf;

	buf = VirtualAlloc((int*)addr, 4096, MEM_RESERVE, PAGE_READWRITE);
	assert(buf == NULL);
	printf("Alloc at %p failed\n", (int*)addr);

	addr = 0x00010000;
	buf = VirtualAlloc((int*)addr, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	assert(buf != NULL);
	*(int*)buf = 1;
	printf("Alloc at %p succeeded\n", buf);
	VirtualFree(((int*)addr), 0, MEM_FREE);

	// windows 10's user mode range is 0 ~ 0x7FFF'FFFFFFFF
	addr = 0x7FFFFFFF0000;
	//addr = 0x7FFFFFFE0000;
	buf = VirtualAlloc((int*)addr, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	assert(buf == NULL);
	printf("Alloc at 64k off-limits %p failed\n", (int*)addr);

	addr = 0x800000000000;
	buf = VirtualAlloc((int*)addr, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	assert(buf == NULL);
	printf("Alloc at kernel %p failed\n", (int*)addr);
}

BOOL LoggedSetLockPagesPrivilege(HANDLE hProcess, BOOL bEnable) {
	struct {
		DWORD Count;
		LUID_AND_ATTRIBUTES Privilege[1];
	} Info;

	HANDLE Token;
	BOOL Result;

	// Open the token.
	Result = OpenProcessToken(hProcess,
		TOKEN_ADJUST_PRIVILEGES,
		&Token);

	if (Result != TRUE) {
		return FALSE;
	}

	// Enable or disable?
	Info.Count = 1;
	Info.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

	// Get the LUID.
	Result = LookupPrivilegeValue(NULL,
		SE_LOCK_MEMORY_NAME,
		&(Info.Privilege[0].Luid));

	if (Result != TRUE) {
		return FALSE;
	}

	Result = AdjustTokenPrivileges(Token, FALSE,
		(PTOKEN_PRIVILEGES)&Info,
		0, NULL, NULL);

	if (Result != TRUE) {
		return FALSE;
	}
	else {
		if (GetLastError() != ERROR_SUCCESS) {
			return FALSE;
		}
	}

	CloseHandle(Token);

	return TRUE;
}

void TestLargeAlloc() {
	HANDLE hP = GetCurrentProcess();
	SIZE_T size = 1024LL * 1024LL * 1024LL * 1.8;
	LPVOID buf;
	PROCESS_MEMORY_COUNTERS pmc = { 0 };
	SYSTEM_INFO sSysInfo;
	BOOL bResult;

	LPVOID lpMemReserved = VirtualAlloc(NULL, size, MEM_RESERVE | MEM_PHYSICAL, PAGE_READWRITE);

	GetSystemInfo(&sSysInfo);
	ULONG_PTR NumberOfPages = size / sSysInfo.dwPageSize;
	ULONG_PTR PFNArraySize = NumberOfPages * sizeof(ULONG_PTR);
	ULONG_PTR* aPFNs1 = (ULONG_PTR*)HeapAlloc(GetProcessHeap(), 0, PFNArraySize);
	ULONG_PTR* aPFNs2 = (ULONG_PTR*)HeapAlloc(GetProcessHeap(), 0, PFNArraySize);
	ULONG_PTR* aPFNs3 = (ULONG_PTR*)HeapAlloc(GetProcessHeap(), 0, PFNArraySize);

	if (!LoggedSetLockPagesPrivilege(GetCurrentProcess(), TRUE)) {
		return;
	}

	AllocateUserPhysicalPages(GetCurrentProcess(), &NumberOfPages, aPFNs1);
	AllocateUserPhysicalPages(GetCurrentProcess(), &NumberOfPages, aPFNs2);
	AllocateUserPhysicalPages(GetCurrentProcess(), &NumberOfPages, aPFNs3);

	GetProcessMemoryInfo(hP, &pmc, sizeof(pmc));
#ifndef X64
	printf("The commit mem is %lu\n", pmc.PagefileUsage);
#else
	printf("The commit mem is %llu\n", pmc.PagefileUsage);
#endif

	MapUserPhysicalPages(lpMemReserved, NumberOfPages, aPFNs1);
	printf("Write to First block\n");
	memcpy(lpMemReserved, "123\0", 4);
	MapUserPhysicalPages(lpMemReserved, NumberOfPages, aPFNs2);
	printf("Write to Second block\n");
	memcpy(lpMemReserved, "456\0", 4);
	MapUserPhysicalPages(lpMemReserved, NumberOfPages, aPFNs3);
	printf("Write to Third block\n");
	memcpy(lpMemReserved, "678\0", 4);

	// go back to mapped mem for check
	MapUserPhysicalPages(lpMemReserved, NumberOfPages, aPFNs1);
	printf("First block %s\n", lpMemReserved);
	MapUserPhysicalPages(lpMemReserved, NumberOfPages, aPFNs2);
	printf("Second block %s\n", lpMemReserved);
	MapUserPhysicalPages(lpMemReserved, NumberOfPages, aPFNs3);
	printf("Third block %s\n", lpMemReserved);

	MapUserPhysicalPages(lpMemReserved, NumberOfPages, NULL);
	FreeUserPhysicalPages(GetCurrentProcess(), &NumberOfPages, aPFNs1);
	FreeUserPhysicalPages(GetCurrentProcess(), &NumberOfPages, aPFNs2);
	FreeUserPhysicalPages(GetCurrentProcess(), &NumberOfPages, aPFNs3);

	VirtualFree(lpMemReserved, 0, MEM_RELEASE);
	HeapFree(GetProcessHeap(), 0, aPFNs1);
	HeapFree(GetProcessHeap(), 0, aPFNs2);
	HeapFree(GetProcessHeap(), 0, aPFNs3);
}

void TestVirtualAllocRoundup() {
	MEMORY_BASIC_INFORMATION mbi;
	int allocBys = 10 * 1024;
	LPVOID addr = VirtualAlloc(NULL, allocBys, MEM_COMMIT, PAGE_READWRITE);

	VirtualQuery(addr, &mbi, sizeof(mbi));
	printf("Virtual alloc %d bytes, but actual round-up bytes is %d\n", allocBys, mbi.RegionSize);
	VirtualFree(addr, 0, MEM_FREE);
}

void TestVirtualAllocReservedAndCommit() {
	int allocBys = 10 * 4096;
	printf("Alloc 40K reserved mem\n");
	LPVOID addr = VirtualAlloc(NULL, allocBys, MEM_RESERVE, PAGE_READWRITE);

	__try {
		*(int*)(addr) = 1;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		printf("Exception: access mem without commit\n");
	}

	printf("Commit 2nd, 5th page\n");
	BYTE* page2 = (BYTE*)addr + 2 * 4096;
	BYTE* page5 = (BYTE*)addr + 5 * 4096;

	LPVOID addr2 = VirtualAlloc(page2, 4096, MEM_COMMIT, PAGE_READWRITE);
	LPVOID addr5 = VirtualAlloc(page5, 4096, MEM_COMMIT, PAGE_READWRITE);
	
	*(int*)page2 = 2;
	*(int*)page5 = 5;

	printf("Alloc reserved at \t\t%p\n", addr);
	printf("Alloc commit 2nd page at \t%p\n", addr2);
	printf("Alloc commit 5th page at \t%p\n", addr5);
}