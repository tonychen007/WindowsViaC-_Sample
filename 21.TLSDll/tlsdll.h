#pragma once
#include <Windows.h>

#ifdef MYLIBAPI
// MYLIBAPI should be defined in all of the DLL's source
// code modules before this header file is included.
// All functions/variables are being exported.
#else
// This header file is included by an EXE source code module.
// Indicate that all functions/variables are being imported.
#define MYLIBAPI extern "C" __declspec(dllimport)
#endif

extern int tlsIndex;

extern "C" {
	MYLIBAPI BOOL WINAPI StoreData(DWORD dw);
	MYLIBAPI BOOL WINAPI GetData(DWORD *pdw);
}