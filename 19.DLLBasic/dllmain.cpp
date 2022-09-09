#include <Windows.h>
#include <stdio.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        printf("process dll attach\n");
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

