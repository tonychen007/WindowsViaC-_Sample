#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <Windows.h>
#include <Psapi.h>

void TestMemoryRegion();
void TestLargeAlloc();
void TestVirtualAllocRoundup();
void TestVirtualAllocReservedAndCommit();

int main() {
	printf("TestMemoryRegion\n");
	TestMemoryRegion();
	printf("\n");
	printf("TestVirtualAllocRoundup\n");
	TestVirtualAllocRoundup();

	printf("\n");
	printf("TestVirtualAllocReservedAndCommit\n");
	TestVirtualAllocReservedAndCommit();

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

void TestLargeAlloc() {
	HANDLE hP = GetCurrentProcess();
	SIZE_T size = 1024LL * 1024LL * 1024LL * 1.8;
	LPVOID buf;
	int flag = 1;
	PROCESS_MEMORY_COUNTERS pmc = { 0 };

	printf("Try to alloc 1.5GB mem\n");
	goto alloc;

alloc:
	GetProcessMemoryInfo(hP, &pmc, sizeof(pmc));
	printf("Before alloc, the mem is %lld\n", pmc.PagefileUsage);
	buf = VirtualAllocEx(hP, NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	assert(buf != NULL);
	GetProcessMemoryInfo(hP, &pmc, sizeof(pmc));
	printf("After alloc, the mem is %lld\n", pmc.PagefileUsage);
	VirtualFree(buf, 0, MEM_RELEASE);

	if (flag == 2) return;

	printf("Try to alloc 2.5GB mem\n");
	size = 1024LL * 1024LL * 1024LL * 8;
	flag = 2;
goto alloc;

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