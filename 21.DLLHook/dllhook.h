#pragma once

#include <stdio.h>
#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>

#ifdef MYDLL
#else
#define MYDLL extern "C" __declspec(dllimport)
#endif

extern HINSTANCE g_hInstDll;
extern TCHAR g_szRegSubKey[];

MYDLL BOOL WINAPI SetHook(DWORD threadID);
MYDLL VOID SaveListViewItemPositions(HWND hWndLV);
MYDLL VOID RestoreListViewItemPositions(HWND hWndLV);