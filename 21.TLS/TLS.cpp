#include <Windows.h>
#include <stdio.h>
#include <thread>

#include "../21.TLSDll/tlsdll.h"
using namespace std;

void TestSimpleTls();
void TestSimpleTls2();
DWORD WINAPI SimpleTls2Thread(LPVOID args);

void TestDynamicDllTls();
DWORD WINAPI DllTlsThread(LPVOID args);

void TestStaticDllTls();
DWORD WINAPI DllTlsStaticThread(LPVOID args);

int main() {
    printf("TestSimpleTls\n");
    TestSimpleTls();

    printf("\n");
    printf("TestSimpleTls2\n");
    TestSimpleTls2();

    printf("\n");
    printf("TestDllTls\n");
    TestDynamicDllTls();

    printf("\n");
    printf("TestStaticDllTls\n");
    TestStaticDllTls();
}

void TestSimpleTls() {
    const char* pszThread = "[Thread %d] Tls idx is : %d\n";

    thread th1 = thread([&] {
        int idx = TlsAlloc();
        printf(pszThread, th1.get_id(), idx);
        TlsFree(idx);
        });

    thread th2 = thread([&] {
        int idx = TlsAlloc();
        printf(pszThread, th2.get_id(), idx);
        TlsFree(idx);
        });

    th1.join();
    th2.join();
}

void TestSimpleTls2() {
    DWORD idx = TlsAlloc();
    TlsSetValue(idx, (void*)"Tony Chen");
    printf("TlsSetValue in main thread\n");

    HANDLE h = CreateThread(NULL, 0, SimpleTls2Thread, &idx, 0, 0);
    CloseHandle(h);

    const char* val = (const char*)TlsGetValue(idx);
    printf("TlsGetValue in main thread: %s\n", val);
    Sleep(100);
    TlsFree(idx);
}

DWORD WINAPI SimpleTls2Thread(LPVOID args) {
    DWORD idx  = *(DWORD*)args;
    Sleep(10);
    const char* val = (const char*)TlsGetValue(idx);
    printf("TlsGetValue in other thread: %s\n", val);

    return 0;
}

int gTlsVal;

void TestDynamicDllTls() {
    const int THREAD_CNT = 3;
    HMODULE hDll = LoadLibrary(L"21.TLSDll.dll");
    HANDLE hThs[THREAD_CNT];

    for (int i = 0; i < THREAD_CNT; i++) {
        hThs[i] = CreateThread(NULL, 0, DllTlsThread, (void*)&THREAD_CNT, 0, NULL);
    }

    WaitForMultipleObjects(THREAD_CNT, hThs, TRUE, INFINITE);
    FreeLibrary(hDll);
}

DWORD WINAPI DllTlsThread(LPVOID args) {
    DWORD dwThreadCnt = *(DWORD*)args;
    BOOL ret;

    /* wrong for global varibale
    gTlsVal = GetCurrentThreadId();
    Sleep(10);
    ret = StoreData(gTlsVal);
    */
    ret = StoreData(GetCurrentThreadId());
    if (!ret)
        ExitProcess(0);

    for (int i = 0; i < dwThreadCnt; i++) {
        DWORD dwOut;
        if (!GetData(&dwOut))
            ExitThread(-1);

        if (dwOut != GetCurrentThreadId())
            printf("thread %d: data is incorrect (%d)\n", GetCurrentThreadId(), dwOut);
        else
            printf("thread %d: data is correct\n", GetCurrentThreadId());

        Sleep(0);
    }

    return 0;
}

void TestStaticDllTls() {
    const int THREAD_CNT = 3;
    HMODULE hDll = LoadLibrary(L"21.TLSDll.dll");
    HANDLE hThs[THREAD_CNT];

    for (int i = 0; i < THREAD_CNT; i++) {
        hThs[i] = CreateThread(NULL, 0, DllTlsStaticThread, (void*)&THREAD_CNT, 0, NULL);
    }

    WaitForMultipleObjects(THREAD_CNT, hThs, TRUE, INFINITE);
    FreeLibrary(hDll);
}

__declspec(thread) DWORD threadVal;

DWORD WINAPI DllTlsStaticThread(LPVOID args) {
    DWORD dwThreadCnt = *(DWORD*)args;
    BOOL ret;

    DWORD tid = GetCurrentThreadId();

    for (int i = 0; i < 100; i++) {
        threadVal++;
        printf("[Thread %d] threadVal is : %d\n", tid, threadVal);
    }

    return 0;
}