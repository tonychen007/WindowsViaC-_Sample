#define MYLIBAPI extern "C" __declspec(dllexport)

#include "tlsdll.h"

BOOL WINAPI StoreData(DWORD dw) {
    LPVOID lpvData;
    DWORD* pData;

    lpvData = TlsGetValue(tlsIndex);
    if (lpvData == NULL) {
        lpvData = (LPVOID)LocalAlloc(LPTR, 256);

        if (lpvData == NULL)
            return FALSE;

        if (!TlsSetValue(tlsIndex, lpvData))
            return FALSE;
    }

    pData = (DWORD*)lpvData;
    (*pData) = dw;

    return TRUE;
}

BOOL WINAPI GetData(DWORD* pdw) {
    LPVOID lpvData;
    DWORD* pData;

    lpvData = TlsGetValue(tlsIndex);
    if (lpvData == NULL)
        return FALSE;

    pData = (DWORD*)lpvData;
    (*pdw) = (*pData);

    return TRUE;
}