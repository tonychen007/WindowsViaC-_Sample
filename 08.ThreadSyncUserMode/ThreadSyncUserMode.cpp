#include <Windows.h>
#include <stdint.h>
#include <stdio.h>


#define BEGIN_FOR(x, times) for(x = 0; x < times; x++) {
#define END_FOR }

#define GET_REAL_THREAD_HANDLE(x) { \
	DuplicateHandle( \
	GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), \
	&x, THREAD_ALL_ACCESS, FALSE, DUPLICATE_SAME_ACCESS); }

int64_t gInt64;

void TestAtomic();
void TestCacheLine();
void TestCriticalSection();
void TestSlimReadWriteLock();


DWORD WINAPI ThreadFunc1(LPVOID args);
DWORD WINAPI ThreadFuncCS1(LPVOID args);
DWORD WINAPI ThreadFuncReadLock(LPVOID args);
DWORD WINAPI ThreadFuncWriteLock(LPVOID args);


int main() {
	printf("Test Atmoic\n");
	TestAtomic();

	printf("\n");
	printf("Test Cache Line\n");
	TestCacheLine();

	printf("\n");
	printf("Test Critical Section\n");
	gInt64 = 0;
	TestCriticalSection();

	printf("\n");
	printf("Test Slim Read Write Lock\n");
	gInt64 = 0;
	TestSlimReadWriteLock();

	printf("All test is done...\n");
	getchar();
}

void TestAtomic() {
	HANDLE hArr[3];

	hArr[0] = CreateThread(NULL, 0, ThreadFunc1, 0, 0, 0);
	hArr[1] = CreateThread(NULL, 0, ThreadFunc1, 0, 0, 0);
	hArr[2] = CreateThread(NULL, 0, ThreadFunc1, 0, 0, 0);

	WaitForMultipleObjects(_countof(hArr), hArr, TRUE, INFINITE);
}

void TestCacheLine() {
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION cpuInfo;
	DWORD sz = 0;
	int cacheSize;

	GetLogicalProcessorInformation(&cpuInfo, &sz);
	cacheSize = cpuInfo.Cache.LineSize;
	printf("Cache line size is: %d\n", cpuInfo.Cache.LineSize);

}

void TestCriticalSection() {
	CRITICAL_SECTION cs;
	HANDLE hArr[2];

	InitializeCriticalSection(&cs);
	hArr[0] = CreateThread(NULL, 0, ThreadFuncCS1, &cs, 0, 0);
	hArr[1] = CreateThread(NULL, 0, ThreadFuncCS1, &cs, 0, 0);

	WaitForMultipleObjects(_countof(hArr), hArr, TRUE, INFINITE);
}

void TestSlimReadWriteLock() {
	SRWLOCK rwLck;
	InitializeSRWLock(&rwLck);

	HANDLE hArr[4];
	hArr[0] = CreateThread(NULL, 0, ThreadFuncReadLock, &rwLck, 0, 0);
	hArr[1] = CreateThread(NULL, 0, ThreadFuncWriteLock, &rwLck, 0, 0);
	hArr[2] = CreateThread(NULL, 0, ThreadFuncWriteLock, &rwLck, 0, 0);
	hArr[3] = CreateThread(NULL, 0, ThreadFuncWriteLock, &rwLck, 0, 0);

	WaitForMultipleObjects(_countof(hArr), hArr, TRUE, INFINITE);

}

DWORD WINAPI ThreadFunc1(LPVOID args) {
	HANDLE h;
	int i;

	GET_REAL_THREAD_HANDLE(h);
	BEGIN_FOR(i, 100)
		InterlockedAdd64(&gInt64, 1);
		printf("Thread %x, gInt64 is: %lld\n", h, gInt64);
		Sleep(10);
	END_FOR

	return 0;
}

DWORD WINAPI ThreadFuncCS1(LPVOID args) {
	CRITICAL_SECTION* pCS = (CRITICAL_SECTION*)args;
	HANDLE h;
	int i;

	GET_REAL_THREAD_HANDLE(h);
	BEGIN_FOR(i, 100)
		EnterCriticalSection(pCS);
		gInt64++;
		LeaveCriticalSection(pCS);
	Sleep(10);
	END_FOR
	printf("Thread %x, gInt64 is: %lld\n", h, gInt64);

	return 0;
}

DWORD WINAPI ThreadFuncReadLock(LPVOID args) {
	int i;
	SRWLOCK* pSRWLck = (SRWLOCK*)args;

	AcquireSRWLockShared(pSRWLck);
	printf("read thread gInt64 is: %lld\n", gInt64);
	ReleaseSRWLockShared(pSRWLck);

	return 0;
}

DWORD WINAPI ThreadFuncWriteLock(LPVOID args) {
	int i;
	SRWLOCK* pSRWLck = (SRWLOCK*)args;

	AcquireSRWLockExclusive(pSRWLck);
	BEGIN_FOR(i, 100)
	gInt64++;
	SwitchToThread();
	printf("write thread gInt64 is: %lld\n", gInt64);
	END_FOR
	ReleaseSRWLockExclusive(pSRWLck);

	return 0;
}