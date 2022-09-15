#pragma once

#include <stdio.h>
#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>

#ifdef MYDLL
#else
#define MYDLL extern "C" __declspec(dllimport)
#endif

typedef LRESULT(WINAPI* hookFn) (int nCode, WPARAM wParam, LPARAM lParam);

extern HINSTANCE g_hInstDll;
extern TCHAR g_szRegSubKey[];

MYDLL LRESULT WINAPI GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam);
MYDLL LRESULT WINAPI GetKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

MYDLL BOOL WINAPI SetHook(DWORD threadID, int hookID, hookFn fn);
MYDLL VOID SaveListViewItemPositions(HWND hWndLV);