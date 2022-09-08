#include <stdio.h>
#include <Windows.h>
#include <thread>
using namespace std;

typedef int (*addFunpadd)(int a, int b);
LONG MyExceptionHandler(EXCEPTION_POINTERS* excpInfo) {
	excpInfo->ContextRecord;

	return EXCEPTION_EXECUTE_HANDLER;
}

void TestLoadDLLWithFlag(DWORD flag = 0);
void TestFreeLibraryAndExitThread();

int main() {
	printf("TestLoadDLLNoRef\n");
	//TestLoadDLLWithFlag(DONT_RESOLVE_DLL_REFERENCES);

	printf("\n");
	printf("TestLoadDLLAsDATA\n");
	//TestLoadDLLWithFlag(LOAD_LIBRARY_AS_DATAFILE);

	printf("\n");
	printf("TestFreeLibraryAndExitThread\n");
	TestFreeLibraryAndExitThread();
}

void TestLoadDLLWithFlag(DWORD flag) {
	HMODULE hDLL = LoadLibraryEx(L"19.DLLBasic.dll", NULL, flag);

	addFunpadd add = (addFunpadd)GetProcAddress(hDLL, "gVal");
	__try {
		add(1, 2);
	}
	__except (MyExceptionHandler(GetExceptionInformation())) {
		printf("GetProcAddress is OK, but the call of add should fail\n");
	}

	int a = 0;
}

void TestFreeLibraryAndExitThread() {
	HMODULE hDLL = LoadLibrary(L"20.DLLAdvanced.dll");

	printf("Press key to end...\n");
	Sleep(1000);
	printf("After free inside DLL, please watch the hDLL address via breakpoint\n");
}