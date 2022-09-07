#define MYLIBAPI extern "C" __declspec(dllexport)

#include "dll_test.h"

int gVal;

int add(int a, int b) {
	gVal = a + b;
	return a + b;
}