#include <Windows.h>
#include <stdio.h>
#include <wchar.h>
#include <thread>

using namespace std;

void TestTwoThreadWaitObejct();
void TestHandShake();
void TestWaitableTimer(bool isOneShot, PTIMERAPCROUTINE pCallBack = NULL, LPVOID pCallBackArgs = NULL);
void TestTimerApcQueue();

VOID APIENTRY TimerApcCallBack(PVOID pvArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue);

int main() {
    printf("TestTwoThreadWaitObejct\n");
    //TestTwoThreadWaitObejct();

    printf("\n");
    printf("TestHandShake\n");
    TestHandShake();

    printf("\n");
    printf("TestWaitableTimer\n");
    TestWaitableTimer(FALSE);

    printf("\n");
    printf("TestOneShotTimer\n");
    TestWaitableTimer(TRUE);

    printf("\n");
    printf("TestTimerApcCallback\n");
    TestWaitableTimer(TRUE, TimerApcCallBack);
}

void TestTwoThreadWaitObejct() {
    HANDLE hArr[2];
    hArr[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
    hArr[1] = CreateEvent(NULL, FALSE, FALSE, NULL);

    thread th1 = thread([&]() {
        WaitForMultipleObjects(_countof(hArr), hArr, TRUE, INFINITE);
        printf("thread1 waits finished\n");
    });

    thread th2 = thread([&]() {
        WaitForMultipleObjects(_countof(hArr), hArr, TRUE, INFINITE);
        printf("thread2 waits finished\n");
    });

    thread th3 = thread([&]() {
        Sleep(1000);
        SetEvent(hArr[0]);
        Sleep(1000);
        SetEvent(hArr[1]);

        // set again because it is auto-reset event
        printf("set again because it is auto-reset event\n");
        Sleep(1000);
        SetEvent(hArr[0]);
        Sleep(1000);
        SetEvent(hArr[1]);
    });

    th1.join();
    th2.join();
    th3.join();
}

void TestHandShake() {
    HANDLE hReqSubmitted, hReqReturned;
    char buf[256];

    hReqSubmitted = CreateEvent(NULL, FALSE, FALSE, NULL);
    hReqReturned = CreateEvent(NULL, FALSE, FALSE, NULL);

    thread clientTh = thread([&]() {
        printf("Please enter the words:\n");
        scanf_s("%[^\n]", buf, 256);
        printf("\n");
        SignalObjectAndWait(hReqSubmitted, hReqReturned, INFINITE, FALSE);
        printf("[Client Thread]: The reversed words are: %s\n", buf);
    });

    thread serverTh = thread([&]() {
        BOOL bIsShutdown = FALSE;

        while (!bIsShutdown) {
            WaitForSingleObject(hReqSubmitted, INFINITE);

            printf("[Server thread]: Request has received\n");
            _strrev(buf);
            SetEvent(hReqReturned);
            bIsShutdown = TRUE;
        }
    });

    clientTh.join();
    serverTh.join();

    CloseHandle(hReqSubmitted);
    CloseHandle(hReqReturned);
}

void TestWaitableTimer(bool isOneShot, PTIMERAPCROUTINE pCallBack, LPVOID pCallBackArgs) {
    HANDLE hTimer;
    LARGE_INTEGER li;
    DWORD dwSt;
    int idx = 0;
    int times = 10;

    hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
    const int nanoseconds = 1000 * 1000 * 1000LL / 100LL;
    const int64_t timeOff = 2;
    li.QuadPart = -timeOff * nanoseconds;
    const int timeIntv = isOneShot ? 0 : 100;

    printf("Timer will be signaled in %lld seconds, and afterwards the interval is 0.5s\n", timeOff);
    if (!hTimer)
        return;

    SetWaitableTimer(hTimer, &li, timeIntv, pCallBack, pCallBackArgs, FALSE);

    while (idx < times) {
        dwSt = WaitForSingleObject(hTimer, 5*1000);
        if (dwSt == WAIT_OBJECT_0) {
            printf("Timer was signaled.\n");
            idx++;
            if (pCallBack)
                SleepEx(0, TRUE);
        }
        else if (dwSt == WAIT_TIMEOUT) {
            printf("wait timeout, maybe it is a one-shot timer\n");
            break;
        }
    }

    CloseHandle(hTimer);
}

VOID APIENTRY TimerApcCallBack(PVOID pvArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue) {
    FILETIME ftUTC, ftLocal;
    SYSTEMTIME st;
    TCHAR szBuf[256];

    ftUTC.dwLowDateTime = dwTimerLowValue;
    ftUTC.dwHighDateTime = dwTimerHighValue;

    FileTimeToLocalFileTime(&ftUTC, &ftLocal);
    FileTimeToSystemTime(&ftLocal, &st);

    GetDateFormat(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), DATE_LONGDATE, &st, NULL, szBuf, _countof(szBuf));
    wcscat_s(szBuf, _countof(szBuf), L" ");
    GetTimeFormat(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), 0, &st, NULL, wcschr(szBuf, TEXT('\0')), (int)(_countof(szBuf) - wcslen(szBuf)));

    wprintf(L"Inside TimerApcCallBack, the time is: %ws\n", szBuf);
}