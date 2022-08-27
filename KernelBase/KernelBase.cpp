#include <Windows.h>
#include <stdio.h>
#include <wchar.h>
#include <thread>

using namespace std;

void TestTwoThreadWaitObejct();
void TestHandShake();
void TestWaitableTimer(bool isOneShot, PTIMERAPCROUTINE pCallBack = NULL, LPVOID pCallBackArgs = NULL);
void TestSemaphore();
void TestMutex();
void TestMutexWaitAgain();

VOID APIENTRY TimerApcCallBack(PVOID pvArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue);

int main() {
    printf("TestTwoThreadWaitObejct\n");
    //TestTwoThreadWaitObejct();

    printf("\n");
    printf("TestHandShake\n");
    //TestHandShake();

    printf("\n");
    printf("TestWaitableTimer\n");
    //TestWaitableTimer(FALSE);

    printf("\n");
    printf("TestOneShotTimer\n");
    //TestWaitableTimer(TRUE);

    printf("\n");
    printf("TestTimerApcCallback\n");
    //TestWaitableTimer(TRUE, TimerApcCallBack);

    printf("\n");
    printf("TestSemaphore\n");
    //TestSemaphore();

    printf("\n");
    printf("TestMutex\n");
    //TestMutex();

    printf("\n");
    printf("TestMutexWaitAgain\n");
    TestMutexWaitAgain();
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

void TestSemaphore() {
    HANDLE h;
    int idx = 0;
    int times = 5;
    int semaCnt = 5;

    h = CreateSemaphore(NULL, 0, semaCnt, NULL);

    thread th1 = thread([&]() {
        while (1) {
            WaitForSingleObject(h, INFINITE);
            printf("sema wake up every 100 mills\n");
            Sleep(100);

            if (idx >= times)
                break;
        }
    });

    while (idx < times) {
        printf("Rlease 5 sema every 1 seconds\n");
        Sleep(1000);
        ReleaseSemaphore(h, 5, NULL);
        idx++;
    }

    th1.join();
    CloseHandle(h);
}

void TestMutex() {
    HANDLE h;
    int gi = 0;

    h = CreateMutex(NULL, FALSE, NULL);

    thread th1 = thread([&]() {
        while (gi < 99) {
            WaitForSingleObject(h, 1000);
            gi++;
            printf("[Thread %d]:gi is %d\n", th1.get_id(), gi);
            Sleep(10);
            ReleaseMutex(h);
        }
    });

    thread th2 = thread([&]() {
        while (gi < 99) {
            WaitForSingleObject(h, 1000);
            gi++;
            printf("[Thread %d]:gi is %d\n", th2.get_id(), gi);
            Sleep(20);
            ReleaseMutex(h);
        }
    });

    th1.join();
    th2.join();
    CloseHandle(h);
}

void TestMutexWaitAgain() {
    HANDLE hMutex, hEvent;
    int gi = 0;
    DWORD dwSt = 0;

    hMutex = CreateMutex(NULL, FALSE, L"TonyMutex");
    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    thread th1 = thread([&]() {
        WaitForSingleObject(hEvent, INFINITE);
        printf("[Thread % d] After acquiring second mutex, event is set\n", th1.get_id());
        printf("[Thread % d] But only release mutex once\n", th1.get_id());
        WaitForSingleObject(hMutex, 1000);
        printf("[Thread % d] Wait for mutex time out. The mutex should be released twice\n", th1.get_id());

        WaitForSingleObject(hEvent, INFINITE);
        WaitForSingleObject(hMutex, INFINITE);
        printf("[Thread % d] After release twice mutex, I am awake\n", th1.get_id());
        ResetEvent(hEvent);
        SetEvent(hEvent);
    });

    thread th2 = thread([&]() {
        while (1) {
            dwSt = WaitForSingleObject(hMutex, INFINITE);
            if (dwSt == WAIT_OBJECT_0) {
                printf("[Thread % d] Acquire first mutex, try to wait again\n", th2.get_id());
                dwSt = WaitForSingleObject(hMutex, INFINITE);
                if (dwSt == WAIT_OBJECT_0) {
                    printf("[Thread % d] Acquire second mutex\n", th2.get_id());
                    ReleaseMutex(hMutex);
                    SetEvent(hEvent);

                    Sleep(2000);
                    ResetEvent(hEvent);
                    printf("[Thread % d] Release mutex once twice\n", th2.get_id());
                    ReleaseMutex(hMutex);
                    SetEvent(hEvent);
                    Sleep(2000);
                    WaitForSingleObject(hEvent, INFINITE);
                    break;
                }
            }
        }
    });

    th1.join();
    th2.join();
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