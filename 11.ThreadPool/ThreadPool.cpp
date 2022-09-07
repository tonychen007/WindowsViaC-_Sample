#include <Windows.h>
#include <stdio.h>
#include <stdint.h>
#include <thread>
#include <locale.h>
using namespace std;

VOID CALLBACK SimpleCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context);
VOID CALLBACK SimpleCallback2(PTP_CALLBACK_INSTANCE, PVOID Context, PTP_WORK);
VOID CALLBACK BatchCallback(PTP_CALLBACK_INSTANCE, PVOID Context, PTP_WORK);
VOID CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_TIMER Timer);
VOID CALLBACK WaitCallback(PTP_CALLBACK_INSTANCE pInstance, PVOID Context, PTP_WAIT Wait, TP_WAIT_RESULT WaitResult);
VOID CALLBACK IOCPCallBack(PTP_CALLBACK_INSTANCE Instance, PVOID Context,
	PVOID Overlapped, ULONG IoResult, ULONG_PTR NumberOfBytesTransferred, PTP_IO Io);

void TestSimpleThreadPool();
void TestCreateThreadPool();
void TestBatchCallback();
void TestBatchTimerCallback(DWORD msWindowsLength = 0);
void TestWaitObjectCallBack();
void TestWaitOnAsyncIOCP();
void TestRegNotify();
void TestThreadPoolEnviron();

volatile long gCounter = 0;
HANDLE ghEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

int main() {
	printf("TestSimpleThreadPool\n");
	//TestSimpleThreadPool();

	printf("\n");
	printf("TestCreateThreadPool\n");
	//TestCreateThreadPool();

	printf("\n");
	printf("TestBatchCallback\n");
	//TestBatchCallback();

	printf("\n");
	printf("TestBatchTimerCallback with 0 msWindowsLength\n");
	//TestBatchTimerCallback();

	printf("\n");
	printf("TestBatchTimerCallback with 2000 msWindowsLength\n");
	//TestBatchTimerCallback(2000);

	printf("\n");
	printf("TestWaitObjectCallBack\n");
	//TestWaitObjectCallBack();

	printf("\n");
	printf("TestWaitOnAsyncIOCP\n");
	//TestWaitOnAsyncIOCP();

	printf("\n");
	printf("TestRegNotify\n");
	//TestRegNotify();

	printf("\n");
	printf("TestThreadPoolEnviron\n");
	TestThreadPoolEnviron();
}

VOID NTAPI SimpleCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context) {
	char* str = (char*)Context;
	printf("callback %s\n", str);
}

VOID CALLBACK SimpleCallback2(PTP_CALLBACK_INSTANCE, PVOID Context, PTP_WORK) {
	char* str = (char*)Context;
	printf("callback %s\n", str);
	Sleep(1000);
}

VOID CALLBACK BatchCallback(PTP_CALLBACK_INSTANCE, PVOID Context, PTP_WORK) {
	CRITICAL_SECTION cs = *(CRITICAL_SECTION*)Context;
	DWORD tid = GetCurrentThreadId();

	for (int i = 0; i < 100; i++) {
		EnterCriticalSection(&cs);
		InterlockedIncrement(&gCounter);
		printf("[Thread : %d] is working, gCounter is : %d\n", tid, gCounter);
		LeaveCriticalSection(&cs);
		Sleep(10);
	}
}

VOID CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_TIMER Timer) {
	DWORD tid = GetCurrentThreadId();
	CRITICAL_SECTION cs = *(CRITICAL_SECTION*)Context;

	EnterCriticalSection(&cs);
	if (gCounter == 0)
		return;

	InterlockedDecrement(&gCounter);
	printf("[Thread : %d] is working, gCounter is : %d\n", tid, gCounter);
	if (gCounter == 0) {
		printf("gCounter is 0, stop timer\n");
		SetEvent(ghEvent);
		return;
	}
	LeaveCriticalSection(&cs);
}

VOID CALLBACK WaitCallback(PTP_CALLBACK_INSTANCE pInstance, PVOID Context, PTP_WAIT Wait, TP_WAIT_RESULT WaitResult) {
	printf("Inside WaitCallback\n");
	printf("pass pInstance to SetEventWhenCallbackReturns\n");
	SetEventWhenCallbackReturns(pInstance, ghEvent);
	//SetEvent(ghEvent);
}

VOID CALLBACK IOCPCallBack(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PVOID Overlapped,
	ULONG IoResult, ULONG_PTR NumberOfBytesTransferred, PTP_IO Io) {

	BOOL ret = CallbackMayRunLong(Instance);
	printf("IO Result: %d, bytes transferred: %d\n", IoResult, NumberOfBytesTransferred);
}

void TestSimpleThreadPool() {
	TrySubmitThreadpoolCallback(SimpleCallback, (void*)"tony chen", NULL);
	Sleep(1000);
}

void TestCreateThreadPool() {
	PTP_WORK worker = CreateThreadpoolWork(SimpleCallback2, (void*)"franky chen", NULL);

	SubmitThreadpoolWork(worker);

	// multiple submit for a single worker item
	// additional worker will be cancelled when
	// the currently running item is completed
	WaitForThreadpoolWorkCallbacks(worker, FALSE);
	WaitForThreadpoolWorkCallbacks(worker, FALSE);
	WaitForThreadpoolWorkCallbacks(worker, TRUE);
	printf("WaitForThreadpoolWorkCallbacks\n");
}

void TestBatchCallback() {
	const int threads = 4;
	PTP_WORK worker;
	CRITICAL_SECTION cs;

	worker = CreateThreadpoolWork(BatchCallback, &cs, NULL);
	InitializeCriticalSection(&cs);

	for (int i = 0; i < threads; i++) {
		SubmitThreadpoolWork(worker);
	}

	WaitForThreadpoolWorkCallbacks(worker, FALSE);
	CloseThreadpoolWork(worker);
	printf("gCounter is : %d\n", gCounter);
	DeleteCriticalSection(&cs);
}

void TestBatchTimerCallback(DWORD msWindowsLength) {
	PTP_TIMER timer1, timer2, timer3;
	LARGE_INTEGER li1, li2, li3;
	FILETIME ft1, ft2, ft3;
	CRITICAL_SECTION cs;

	InitializeCriticalSection(&cs);
	gCounter = 10;
	const int nanoseconds = 1000 * 1000 * 1000LL / 100LL;
	const int64_t timeOff1 = 1;
	const int64_t timeOff2 = 2;
	const int64_t timeOff3 = 3;
	li1.QuadPart = -timeOff1 * nanoseconds;
	li2.QuadPart = -timeOff2 * nanoseconds;
	li3.QuadPart = -timeOff3 * nanoseconds;
	ft1.dwHighDateTime = li1.HighPart;
	ft1.dwLowDateTime = li1.LowPart;
	ft2.dwHighDateTime = li2.HighPart;
	ft2.dwLowDateTime = li2.LowPart;
	ft3.dwHighDateTime = li3.HighPart;
	ft3.dwLowDateTime = li3.LowPart;

	timer1 = CreateThreadpoolTimer(TimerCallback, &cs, NULL);
	timer2 = CreateThreadpoolTimer(TimerCallback, &cs, NULL);
	timer3 = CreateThreadpoolTimer(TimerCallback, &cs, NULL);

	SetThreadpoolTimer(timer1, &ft1, 1000, msWindowsLength);
	SetThreadpoolTimer(timer2, &ft2, 1000, msWindowsLength);
	SetThreadpoolTimer(timer3, &ft3, 1000, msWindowsLength);

	printf("wait three threads decreasing gCounter down to 0...\n");
	WaitForSingleObject(ghEvent, INFINITE);

	WaitForThreadpoolTimerCallbacks(timer1, FALSE);
	WaitForThreadpoolTimerCallbacks(timer2, FALSE);
	WaitForThreadpoolTimerCallbacks(timer3, FALSE);
	CloseThreadpoolTimer(timer1);
	CloseThreadpoolTimer(timer2);
	CloseThreadpoolTimer(timer3);
	DeleteCriticalSection(&cs);
}

void TestWaitObjectCallBack() {
	PTP_WAIT waiter;

	thread th1 = thread([&] {
		printf("Sleep 3 second to set event\n");
		Sleep(3000);
		SetEvent(ghEvent);
	});
	th1.detach();

	waiter = CreateThreadpoolWait(WaitCallback, NULL, NULL);
	SetThreadpoolWait(waiter, ghEvent, NULL);
	WaitForSingleObject(ghEvent, INFINITE);

	WaitForThreadpoolWaitCallbacks(waiter, FALSE);
	CloseThreadpoolWait(waiter);
}

void TestWaitOnAsyncIOCP() {
	OVERLAPPED ov = {};
	CHAR buf[256] = { 0 };
	int len = _countof(buf) - 1;

	HANDLE hFile = CreateFile(L"ThreadPool.cpp", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	PTP_IO pIo = CreateThreadpoolIo(hFile, IOCPCallBack, NULL, NULL);
	StartThreadpoolIo(pIo);

	ReadFile(hFile, buf, len, NULL, &ov);
	if (ERROR_IO_PENDING == GetLastError()) {
		CancelThreadpoolIo(pIo);
	}
	char* p = buf;

	// skip UTF8 bom
	if (buf[0] == char(0xef) && buf[1] == char(0xbb) && buf[2] == char(0xbf))
		p += 3;
	printf("%s\n", p);

	WaitForThreadpoolIoCallbacks(pIo, false);
	CloseThreadpoolIo(pIo);
	CloseHandle(hFile);
}

void TestRegNotify() {
	HKEY hKey;
	LPCTSTR  path = L"Network";
	RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_READ, &hKey);

	RegNotifyChangeKeyValue(hKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, ghEvent, TRUE);
	WaitForSingleObject(ghEvent, INFINITE);

	RegCloseKey(hKey);
}

void TestThreadPoolEnviron() {
	TP_CALLBACK_ENVIRON CallBackEnviron;
	PTP_WORK worker;
	PTP_POOL pool;
	PTP_CLEANUP_GROUP cleanupgroup;

	InitializeThreadpoolEnvironment(&CallBackEnviron);
	pool = CreateThreadpool(NULL);
	SetThreadpoolThreadMaximum(pool, 1);
	SetThreadpoolThreadMinimum(pool, 1);

	cleanupgroup = CreateThreadpoolCleanupGroup();
	SetThreadpoolCallbackPool(&CallBackEnviron, pool);
	SetThreadpoolCallbackCleanupGroup(&CallBackEnviron, cleanupgroup, NULL);

	worker = CreateThreadpoolWork(SimpleCallback2, NULL, &CallBackEnviron);

	printf("min and max thread is set to 1, so it will run singly\n");
	SubmitThreadpoolWork(worker);
	SubmitThreadpoolWork(worker);
	SubmitThreadpoolWork(worker);
	SubmitThreadpoolWork(worker);

	CloseThreadpoolCleanupGroupMembers(cleanupgroup, FALSE, NULL);
	CloseThreadpool(pool);
}