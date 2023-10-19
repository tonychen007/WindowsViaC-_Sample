#define MYLIBAPI extern "C" __declspec(dllexport)

#include "dll_test.h"

int gVal;

#pragma comment(linker, "/export:add2=ntdll.NtReadFile")

int add(int a, int b) {
	gVal = a + b;
	return a + b;
}