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