#include <stdio.h>
#include <Windows.h>


void TestAllVEHContinueSearch();
void TestAllVEHContinueException();
void TestAllVEHContinueHanlder();

LONG VEH1(EXCEPTION_POINTERS* ExceptionInfo);
LONG VEH2(EXCEPTION_POINTERS* ExceptionInfo);
LONG VEH3(EXCEPTION_POINTERS* ExceptionInfo);
LONG VEH4(EXCEPTION_POINTERS* ExceptionInfo);
LONG UnhandleFilterExecuteHandler(EXCEPTION_POINTERS* ExceptionInfo);

int a = 0;

int main() {
	printf("TestAllVEHContinueSearch\n");
	//TestAllVEHContinueSearch();

	printf("\n");
	printf("TestAllVEHContinueException\n");
	TestAllVEHContinueException();

	printf("\n");
	printf("TestAllVEHContinueHanlder\n");
	TestAllVEHContinueHanlder();
}

void TestAllVEHContinueSearch() {
	AddVectoredExceptionHandler(0, VEH1);
	AddVectoredExceptionHandler(1, VEH2);

	int b = 3 / a;
	int c = 0;
}

void TestAllVEHContinueException() {
	PVOID h = AddVectoredExceptionHandler(1, VEH3);

	int b = 3 / a;
	int c = 0;

	printf("after int c = 0\n");

	RemoveVectoredExceptionHandler(h);
}

void TestAllVEHContinueHanlder() {
	SetUnhandledExceptionFilter(UnhandleFilterExecuteHandler);
	AddVectoredExceptionHandler(0, VEH1);
	AddVectoredExceptionHandler(1, VEH2);
	AddVectoredContinueHandler(2, VEH4);

	a = 0;
	int b = 3 / a;
	int c = 0;
}

LONG VEH1(EXCEPTION_POINTERS* ExceptionInfo) {
	printf("veh1\n");

	return EXCEPTION_CONTINUE_SEARCH;
}

LONG VEH2(EXCEPTION_POINTERS* ExceptionInfo) {
	printf("veh2\n");

	return EXCEPTION_CONTINUE_SEARCH;
}

LONG VEH3(EXCEPTION_POINTERS* ExceptionInfo) {
	a = 1;
	return EXCEPTION_CONTINUE_EXECUTION;
}

LONG VEH4(EXCEPTION_POINTERS* ExceptionInfo) {
	// only call in debug
	printf("In VEH4\n");
	return EXCEPTION_CONTINUE_EXECUTION;
}

LONG UnhandleFilterExecuteHandler(EXCEPTION_POINTERS* ExceptionInfo) {
	// in debug it will never called
	printf("In UnhandleFilterExecuteHandler\n");
	return EXCEPTION_CONTINUE_SEARCH;
}