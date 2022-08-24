#include <Windows.h>
#include <stdio.h>
#include <thread>
using namespace std;

void TestTwoThreadWaitObejct();

int main() {
    printf("TestTwoThreadWaitObejct\n");
    TestTwoThreadWaitObejct();
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

