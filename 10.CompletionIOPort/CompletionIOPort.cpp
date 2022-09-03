#include <stdio.h>
#include <Windows.h>

#define CREATE_NEW_COMPLETION_PORT(numThreads) \
    CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, numThreads);

#define ASSOCIATE_DEVICE_WITH_COMPLETION_PORT(hDevice, hCompletionPort, dwCompletionKey) \
    CreateIoCompletionPort(hDevice, hCompletionPort, dwCompletionKey, 0);

const int NUM_THREAD = 4;
ULONG_PTR READ_KEY = 1234;
ULONG_PTR WRITE_KEY = 4321;

void TestSimpleQueueCompletionPort(int isQueue = 1);

int main() {
    printf("TestSimpleQueueCompletionPort\n");
    TestSimpleQueueCompletionPort();

    printf("\n");
    printf("TestSimpleNotQueueCompletionPort\n");
    TestSimpleQueueCompletionPort(0);
}

void TestSimpleQueueCompletionPort(int isQueue = 1) {
    OVERLAPPED ovRead = { 0 };
    LPCWSTR pszFilename = L"./CompletionIOPort.cpp";
    DWORD dw, ret, dwSt;    
    BYTE readBuf[256] = { 0 };
    OVERLAPPED* pOverlapped;

    HANDLE hFile = CreateFile(pszFilename, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);

    HANDLE hIO = CREATE_NEW_COMPLETION_PORT(NUM_THREAD);
    HANDLE hAssIO = ASSOCIATE_DEVICE_WITH_COMPLETION_PORT(hFile, hIO, READ_KEY);

    if (hAssIO != hIO) {
        printf("Init Completion IO failed\n");
        return;
    }

    // do not have io completion queued
    if (!isQueue) {
        ovRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        ovRead.hEvent = (HANDLE)((DWORD_PTR)ovRead.hEvent | 1);
    }
    ReadFile(hFile, readBuf, 10, NULL, &ovRead);
    ret = GetQueuedCompletionStatus(hIO, &dw, &READ_KEY, &pOverlapped, 1000);
    dwSt = GetLastError();

    if (ret) {
        printf("Process completed IO OK. The bytes transfered is :%d\n", pOverlapped->Internal);
    }
    else {
        if (pOverlapped != NULL) {
            printf("Process a failed completed I/O request\n");
        }
        else {
            if (dwSt == WAIT_TIMEOUT) {
                printf("Get Queue timeout. Maybe the overlapped's hEvent is set not to queue\n");
            }
            else {
                printf("Get Queue Failed\n");
            }
        }
    }
}