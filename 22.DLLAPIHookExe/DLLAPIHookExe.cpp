#include <stdio.h>
#include "../22.DLLAPIHook/dlltest.h"

int main() {
	setConsole(GetCurrentProcessId());
    setHook(1, 0);
    LoadLibrary(L"comctl32.dll");
	printf("Try to open notepad for test...\n");
	// localTest
    //ExitProcess(0);

	getchar();
	setHook(0, 0);
}