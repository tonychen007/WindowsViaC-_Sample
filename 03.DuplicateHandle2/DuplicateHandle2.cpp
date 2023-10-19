#include <Windows.h>
#include <stdio.h>

int main() {
	HANDLE h = CreateMutex(NULL, FALSE, L"TonyMutex");
	printf("mutex handle is: %lld\n", (DWORD)h);

	while (1);
}