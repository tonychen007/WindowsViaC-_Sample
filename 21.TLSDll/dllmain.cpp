#include "tlsdll.h"

int tlsIndex;

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
) {
    LPVOID lpvData;

    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        if ((tlsIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES)
            return FALSE;
        break;
    case DLL_THREAD_ATTACH:
        lpvData = (LPVOID)LocalAlloc(LPTR, 256);
        if (lpvData != NULL)
            TlsSetValue(tlsIndex, lpvData);
        break;
    case DLL_THREAD_DETACH:
        lpvData = TlsGetValue(tlsIndex);
        if (lpvData != NULL)
            LocalFree((HLOCAL)lpvData);
        break;
    case DLL_PROCESS_DETACH:
        lpvData = TlsGetValue(tlsIndex);
        if (lpvData != NULL)
            LocalFree((HLOCAL)lpvData);
        TlsFree(tlsIndex);
        break;
    }
    return TRUE;
}
