﻿#include <stdio.h>
#include <Windows.h>

HMODULE g_hDll = NULL;
HANDLE hThread = NULL;

DWORD WINAPI FreeSelfProc(PVOID param) {
    //MessageBox(NULL, L"Press OK to unload inside DLL", NULL, MB_OK);

    // after call to FreeLibrary, the process will crash
    // because the EIP no long existed
    //FreeLibrary(g_hDll);
    FreeLibraryAndExitThread(g_hDll, 0);

    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        g_hDll = (HMODULE)hModule;
        hThread = ::CreateThread(NULL, 0, FreeSelfProc, NULL, 0, NULL);
        CloseHandle(hThread);
    }
        break;
    case DLL_THREAD_ATTACH:
        printf("thread dll attach\n");
        break;
    case DLL_THREAD_DETACH:
        printf("thread dll detach\n");
        break;
    case DLL_PROCESS_DETACH:
        printf("process dll detach\n");
        break;
    }
    return TRUE;
}

