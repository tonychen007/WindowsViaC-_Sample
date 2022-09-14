#pragma once

#include <stdio.h>
#include <Windows.h>
#include <windowsx.h>

#ifdef MYDLL
#else
#define MYDLL extern "C" __declspec(dllimport)
#endif

MYDLL BOOL WINAPI SetHook(DWORD threadID);