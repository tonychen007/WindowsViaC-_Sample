#include <stdio.h>
#include <Windows.h>


#define CREATE_NEW_COMPLETION_PORT(numThreads) \
    CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, numThreads);

#define ASSOCIATE_DEVICE_WITH_COMPLETION_PORT(hCompletionPort, hDevice, dwCompletionKey) \
    CreateIoCompletionPort(hDevice, hCompletionPort, dwCompletionKey, 0);


const int NUM_THREAD = 4;
const int READ_KEY = 1234;
const int WRITE_KEY = 4321;

int main() {
    LPCWSTR pszFilename = L"./test.txt";
    DWORD ret;
    DWORD dwSt;
    BYTE readBuf[256] = { 0 };

    HANDLE hFile = CreateFile(pszFilename, GENERIC_WRITE, FILE_SHARE_WRITE, 0,
        CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, 0);

    HANDLE hIO = CREATE_NEW_COMPLETION_PORT(NUM_THREAD);
    HANDLE hAssIO = ASSOCIATE_DEVICE_WITH_COMPLETION_PORT(hIO, hFile, READ_KEY);

    if (hAssIO != hIO) {
        printf("Init Completion IO failed\n");
        return -1;
    }

    int a = 0;
}