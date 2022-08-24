#include <Windows.h>
#include <stdio.h>
#include <wchar.h>
#include <thread>

using namespace std;

void TestTwoThreadWaitObejct();
void TestHandShake();

int main() {
    printf("TestTwoThreadWaitObejct\n");
    //TestTwoThreadWaitObejct();

    printf("\n");
    printf("TestHandShake\n");
    TestHandShake();
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