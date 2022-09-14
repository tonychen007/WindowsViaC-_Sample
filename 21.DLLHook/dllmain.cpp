﻿#include "dllhook.h"

extern HINSTANCE g_hInstDll;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        g_hInstDll = hModule;
        printf("DLL_PROCESS_ATTACH, g_hInstDll is : %p\n", g_hInstDll);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

