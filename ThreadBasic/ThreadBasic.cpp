#include <Windows.h>
#include <winternl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <chrono>
using namespace std;

#include "Tools.h"

#pragma comment(lib, "ntdll")

class Foo {
public:
	~Foo() { printf("Foo::~Foo()\n"); }
};

void TestExitThread(int flag);
void TestBeginThread(int flag);
void TestGetRealHandle();
void TestAffinity();
void TestThreadBackgroundIO();

DWORD WINAPI TestExitThread(LPVOID args);
unsigned TestBeginThread(void* args);
DWORD WINAPI TestThreadBackgroundIO(LPVOID args);


int main() {

	TestExitThread(0);

	printf("\n");
	TestExitThread(1);

	printf("\n");
	TestBeginThread(0);

	printf("\n");
	TestBeginThread(1);

	printf("\n");
	printf("Convert pseudoHandle to real handle\n");
	TestGetRealHandle();

	printf("\n");
	printf("Test CPU affinity\n");
	TestAffinity();

	printf("\n");
	printf("Test thread BACKGROUND_IO Priority\n");
	TestThreadBackgroundIO();
}

void TestExitThread(int flag) {
	HANDLE h = CreateThread(0, 0, TestExitThread, &flag, 0, 0);
	WaitForSingleObject(h, INFINITE);
}

void TestBeginThread(int flag) {
	HANDLE h = (HANDLE)_beginthreadex(0, 0, TestBeginThread, &flag, 0, 0);
	WaitForSingleObject(h, INFINITE);
}

void TestGetRealHandle() {
	HANDLE realProcHandle;
	HANDLE realThreadHandle;

	HANDLE hP = GetCurrentProcess();
	HANDLE hT = GetCurrentThread();

	DuplicateHandle(hP, hP, hP, &realProcHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);
	DuplicateHandle(hP, hT, hP, &realThreadHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);

	printf("The real proc handle is: %p\n", realProcHandle);
	printf("The real thread handle is: %p\n", realThreadHandle);

	CloseHandle(realProcHandle);
	CloseHandle(realThreadHandle);
}

void TestAffinity() {
	DWORD_PTR procAffMask = 0;
	DWORD_PTR systemAffMask = 0;
	HANDLE hProc;
	PROCESSOR_NUMBER cpuNum;
	char buf1[256] = { 0 };
	char buf2[256] = { 0 };

	hProc = GetCurrentProcess();
	GetProcessAffinityMask(hProc, &procAffMask, &systemAffMask);
	_itoa_s(procAffMask, buf1, 2);
	_itoa_s(systemAffMask, buf2, 2);
	printf("ProcAffmask is: %s, SystemAffmask is: %s\n", buf1, buf2);

	// run on cpu0 and cpu3, 0x05 = 101
	memset(buf1, 0, sizeof(buf1));
	memset(buf2, 0, sizeof(buf2));
	SetProcessAffinityMask(hProc, 0x05);
	GetProcessAffinityMask(hProc, &procAffMask, &systemAffMask);
	_itoa_s(procAffMask, buf1, 2);
	_itoa_s(systemAffMask, buf2, 2);
	printf("ProcAffmask is: %s, SystemAffmask is: %s\n", buf1, buf2);

	GetThreadIdealProcessorEx(GetCurrentThread(), &cpuNum);
	printf("The ideal cpu for current thread is : %d\n", cpuNum.Number);
	int a = 0;
}

void TestThreadBackgroundIO() {
	HANDLE hThread;
	int flags = 0;
	chrono::system_clock::time_point st;
	chrono::system_clock::time_point ed;
	chrono::system_clock::duration diff;
	chrono::milliseconds ss;

	st = chrono::system_clock::now();
	printf("Create thread with lowest priority\n");
	hThread = CreateThread(NULL, 0, TestThreadBackgroundIO, &flags, THREAD_PRIORITY_LOWEST | CREATE_SUSPENDED, 0);
	ResumeThread(hThread);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	ed = chrono::system_clock::now();
	diff = ed - st;
	ss = chrono::duration_cast<chrono::milliseconds>(diff);
	printf("Total time is: %f\n", ss.count() / 1000.0f);

	printf("\n");
	flags = 1;
	st = chrono::system_clock::now();
	printf("Create thread with background io priority\n");
	hThread = CreateThread(NULL, 0, TestThreadBackgroundIO, &flags, 0, 0);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	ed = chrono::system_clock::now();
	diff = ed - st;
	ss = chrono::duration_cast<chrono::milliseconds>(diff);
	printf("Total time is: %f\n", ss.count() / 1000.0f);
}

DWORD WINAPI TestExitThread(LPVOID args) {
	Foo f;
	int flag = *(int*)args;

	Sleep(10);
	if (flag) {
		printf("Call ExitThread, object's destructor will not be called.\n");
		ExitThread(0);
	}
	else {
		printf("Do not Call ExitThread, object's destructor will be called.\n");
	}

	return 0;
}

unsigned TestBeginThread(void* args) {
	Foo f;
	int flag = *(int*)args;

	Sleep(10);
	if (flag) {
		printf("Call ExitThread inside _beginthreadex, object's destructor will not be called.\n");
		ExitThread(0);
	}
	else {
		printf("Do not Call ExitThread inside _beginthreadex, object's destructor will be called.\n");
	}

	return 0;
}

DWORD WINAPI TestThreadBackgroundIO(LPVOID args) {
	HANDLE hThread;
	HANDLE hFile;
	TCHAR pszFileBuf[MAX_PATH] = { 0 };
	TCHAR pszTmpBuf[MAX_PATH] = { 0 };
	_int64 sz = 1024 * 1024 * 512LL;
	int* buf = (int*)malloc(sz * sizeof(int));
	int flags = *(int*)args;
	int itr = 3;

	if (flags)
		SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);

	printf("Create a temp file with %lld, looping for %d times\n", sz * sizeof(int), itr);
	for (int i = 0; i < itr; i++) {
		GetTempPath(MAX_PATH, pszTmpBuf);
		GetTempFileName(pszTmpBuf, L"", 0, pszFileBuf);
		hFile = CreateFile(pszFileBuf, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (!hFile)
			return -1;

		for (int j = 0; j < sz; j++) {
			buf[j] = j;
		}

		WriteFile(hFile, buf, sz, 0, 0);
		CloseHandle(hFile);
		DeleteFile(pszFileBuf);
		ZeroMemory(pszFileBuf, MAX_PATH);
		ZeroMemory(pszTmpBuf, MAX_PATH);
	}

	free(buf);

	if (flags)
		SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);

	return 0;
}
