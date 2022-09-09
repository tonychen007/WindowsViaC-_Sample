#include <stdio.h>
#include <Windows.h>
#include <thread>
using namespace std;

#define DLL_NAME_20 L"20.DLLAdvanced.dll"
#define DLL_NAME_19 L"19.DLLBasic.dll"

typedef int (*addFunpadd)(int a, int b);

LONG MyExceptionHandler(EXCEPTION_POINTERS* excpInfo) {
	excpInfo->ContextRecord;

	return EXCEPTION_EXECUTE_HANDLER;
}

void TestLoadDLLWithFlag(DWORD flag = 0);
void TestFreeLibraryAndExitThread();
void TestLoadDLLName();
void TestLoadFuncFromDLL();
void TestDLLDetach(BOOL isTerminate = FALSE);

int main() {
	printf("TestLoadDLLNoRef\n");
	//TestLoadDLLWithFlag(DONT_RESOLVE_DLL_REFERENCES);

	printf("\n");
	printf("TestLoadDLLAsDATA\n");
	//TestLoadDLLWithFlag(LOAD_LIBRARY_AS_DATAFILE);

	printf("\n");
	printf("TestFreeLibraryAndExitThread\n");
	//TestFreeLibraryAndExitThread();

	printf("\n");
	printf("TestLoadDLLName\n");
	TestLoadDLLName();

	printf("\n");
	printf("TestLoadFuncFromDLL\n");
	TestLoadFuncFromDLL();

	printf("\n");
	printf("TestDLLDetach With TerminateProcess\n");
	//TestDLLDetach(TRUE);

	printf("\n");
	printf("TestDLLDetach Without TerminateProcess\n");
	TestDLLDetach();
}

void TestLoadDLLWithFlag(DWORD flag) {
	HMODULE hDLL = LoadLibraryEx(L"19.DLLBasic.dll", NULL, flag);

	addFunpadd add = (addFunpadd)GetProcAddress(hDLL, "add");
	__try {
		add(1, 2);
	}
	__except (MyExceptionHandler(GetExceptionInformation())) {
		printf("GetProcAddress is OK, but the call of add should fail\n");
	}
}

void TestFreeLibraryAndExitThread() {
	HMODULE hDLL = LoadLibrary(DLL_NAME_20);

	Sleep(1000);
	hDLL = GetModuleHandle(L"20.DLLAdvanced.dll");
	printf("After free inside DLL, the hDLL is %p\n", hDLL);
}

void TestLoadDLLName() {
	HMODULE hDll1 = LoadLibrary(DLL_NAME_20);
	HMODULE hDll2 = LoadLibraryEx(DLL_NAME_20, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
	HMODULE hDll3 = LoadLibraryEx(DLL_NAME_20, NULL, LOAD_LIBRARY_AS_DATAFILE);

	printf("Load with no flag first\n");
	printf("The name of dll1 is %p\n", hDll1);
	printf("The name of dll2 is %p\n", hDll2);
	printf("The name of dll3 is %p\n", hDll3);
	FreeLibrary(hDll1);
	FreeLibrary(hDll2);
	FreeLibrary(hDll3);

	hDll1 = LoadLibraryEx(DLL_NAME_20, NULL, LOAD_LIBRARY_AS_DATAFILE);
	hDll2 = LoadLibraryEx(DLL_NAME_20, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
	hDll3 = LoadLibrary(DLL_NAME_20);
	printf("Load with flag first\n");
	printf("The name of dll1 is %p\n", hDll1);
	printf("The name of dll2 is %p\n", hDll2);
	printf("The name of dll3 is %p\n", hDll3);
	FreeLibrary(hDll1);
	FreeLibrary(hDll2);
	FreeLibrary(hDll3);
}


void TestLoadFuncFromDLL() {
	HMODULE hDLL = LoadLibraryEx(DLL_NAME_19, NULL, 0);

	addFunpadd add = (addFunpadd)GetProcAddress(hDLL, "add");
	printf("call add : %d\n", add(1, 2));

	int gVal = *(int*)GetProcAddress(hDLL, "gVal");
	printf("gVal is : %d\n", gVal);

	__try {
		printf("call invalid add func gVal\n");
		add = (addFunpadd)GetProcAddress(hDLL, "gVal");
		add(1, 2);
	}
	__except (MyExceptionHandler(GetExceptionInformation())) {
		printf("GetProcAddress is OK, but the call of add should fail\n");
	}

	FreeLibrary(hDLL);
}

void TestDLLDetach(BOOL isTerminate) {
	HMODULE hDll1 = LoadLibrary(DLL_NAME_20);

	if (isTerminate) {
		printf("Call terminate process without FreeLibrary\n");
		TerminateProcess(GetCurrentProcess(), 0);
	}
	else {
		printf("Do not call terminate and FreeLibrary\n");
	}
}