#include <Windows.h>
#include <winternl.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>

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

DWORD WINAPI TestExitThread(LPVOID args);
unsigned TestBeginThread(void* args);


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
	TestAffinity();
}

void TestExitThread(int flag) {
	HANDLE h = CreateThread(0, 0, TestExitThread, &flag, 0, 0);
	WaitForSingleObject(h, INFINITE);
}

void TestBeginThread(int flag) {
	HANDLE h = (HANDLE)_beginthreadex(0, 0, TestBeginThread, &flag, 0, 0);
	WaitForSingleObject(h, INFINITE);
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