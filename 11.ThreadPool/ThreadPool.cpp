#include <Windows.h>
#include <stdio.h>
#include <stdint.h>

VOID NTAPI SimpleCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context);
VOID CALLBACK SimpleCallback2(PTP_CALLBACK_INSTANCE, PVOID Context, PTP_WORK);
VOID CALLBACK BatchCallback(PTP_CALLBACK_INSTANCE, PVOID Context, PTP_WORK);
VOID CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context, PTP_TIMER Timer);

void TestSimpleThreadPool();
void TestCreateThreadPool();
void TestBatchCallback();
void TestBatchTimerCallback(DWORD msWindowsLength = 0);

volatile long gCounter = 0;
HANDLE ghEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

int main() {
	printf("TestSimpleThreadPool\n");
	TestSimpleThreadPool();

	printf("\n");
	printf("TestCreateThreadPool\n");
	TestCreateThreadPool();

	printf("\n");
	printf("TestBatchCallback\n");
	TestBatchCallback();

	printf("\n");
	printf("TestBatchTimerCallback with 0 msWindowsLength\n");
	TestBatchTimerCallback();

	printf("\n");
	printf("TestBatchTimerCallback with 2000 msWindowsLength\n");
	TestBatchTimerCallback(2000);
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