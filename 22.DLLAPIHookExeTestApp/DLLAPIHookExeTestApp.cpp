#include <Windows.h>
#include <stdio.h>

int main() {
    MessageBoxA(NULL, "TestAppA", 0, 0);
    MessageBoxW(NULL, L"TestAppW", 0, 0);
    ExitProcess(0);
}