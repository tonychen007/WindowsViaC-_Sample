#include <Windows.h>
#include <stdio.h>

VOID NTAPI SimpleCallback(PTP_CALLBACK_INSTANCE Instance, PVOID Context);
VOID CALLBACK SimpleCallback2(PTP_CALLBACK_INSTANCE, PVOID Context, PTP_WORK);
VOID CALLBACK BatchCallback(PTP_CALLBACK_INSTANCE, PVOID Context, PTP_WORK);

void TestSimpleThreadPool();
void TestCreateThreadPool();
void TestBatchCallback();


volatile long gCounter = 0;

int main() {
	printf("TestSimpleThreadPool\n");
	TestSimpleThreadPool();

	printf("\n");
	printf("TestCreateThreadPool\n");
	TestCreateThreadPool();

	printf("\n");
	printf("TestBatchCallback\n");
	TestBatchCallback();
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
	printf("gCounter is : %d\n", gCounter);
	DeleteCriticalSection(&cs);
}