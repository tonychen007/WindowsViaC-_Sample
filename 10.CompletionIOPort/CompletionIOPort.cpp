#include <stdio.h>
#include <Windows.h>

#define CREATE_NEW_COMPLETION_PORT(numThreads) \
    CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, numThreads);

#define ASSOCIATE_DEVICE_WITH_COMPLETION_PORT(hDevice, hCompletionPort, dwCompletionKey) \
    CreateIoCompletionPort(hDevice, hCompletionPort, dwCompletionKey, 0);

const int NUM_THREAD = 4;
const ULONG_PTR READ_KEY = 1234;
const ULONG_PTR WRITE_KEY = 4321;
const int BUFFER_SIZE = 4 * 1024 * 1024;

void TestSimpleQueueCompletionPort(int isQueue = 1);
void TestQueueCompletionPort();
void TestFileCopyIOCP();

struct OVIOCP : public OVERLAPPED {
    OVIOCP() {
        Internal = InternalHigh = 0;
        Offset = OffsetHigh = 0;
        hEvent = NULL;
        m_nBuffSize = 0;
        m_pvData = NULL;
    }

    ~OVIOCP() {
        if (m_pvData != NULL)
            VirtualFree(m_pvData, 0, MEM_RELEASE);
    }

    BOOL AllocBuffer(SIZE_T nBuffSize) {
        m_nBuffSize = nBuffSize;
        m_pvData = VirtualAlloc(NULL, m_nBuffSize, MEM_COMMIT, PAGE_READWRITE);
        return(m_pvData != NULL);
    }

    SIZE_T m_nBuffSize;
    PVOID  m_pvData;
};

int main() {
    printf("TestSimpleQueueCompletionPort\n");
    //TestSimpleQueueCompletionPort();

    printf("\n");
    printf("TestSimpleNotQueueCompletionPort\n");
    //TestSimpleQueueCompletionPort(0);

    printf("\n");
    printf("TestQueueCompletionPort\n");
    //TestQueueCompletionPort();

    printf("\n");
    printf("TestFileCopyIOCP\n");
    TestFileCopyIOCP();
}

void TestSimpleQueueCompletionPort(int isQueue) {
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
    ret = GetQueuedCompletionStatus(hIO, &dw, const_cast<ULONG_PTR*>(&READ_KEY), &pOverlapped, 1000);
    dwSt = GetLastError();

    if (ret) {
        printf("Process completed IO OK. The bytes transfered is :%d\n", pOverlapped->InternalHigh);
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

    if (!isQueue) {
        ovRead.hEvent = (HANDLE)((DWORD_PTR)ovRead.hEvent & ~1);
    }

    CloseHandle(ovRead.hEvent);
    CloseHandle(hIO);
    CloseHandle(hFile);
}

void TestQueueCompletionPort() {
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

    SetFileCompletionNotificationModes(hFile, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);
    ReadFile(hFile, readBuf, 10, NULL, &ovRead);
    ret = GetQueuedCompletionStatus(hIO, &dw, const_cast<ULONG_PTR*>(&READ_KEY), &pOverlapped, 1000);
    dwSt = GetLastError();
    if (ret == 0 && dwSt == WAIT_TIMEOUT) {
        printf("Get queue timeout, set FILE_SKIP_COMPLETION_PORT_ON_SUCCESS successfully\n");
    }
    else {
        printf("Set FILE_SKIP_COMPLETION_PORT_ON_SUCCESS failed\n");
    }

    CloseHandle(hIO);
    CloseHandle(hFile);
    DeleteFile(pszFilename);
}

void TestFileCopyIOCP() {
    OVERLAPPED ovRead = { 0 };
    LPCWSTR pszSrcFilename = L"./vminst.zip";
    LPCWSTR pszDstFilename = L"./vminst.bak";
    LARGE_INTEGER srcFileSize, dstFileSize;
    HANDLE hTemp, hIO, hSrcFile, hDstFile;
    OVIOCP ovs[NUM_THREAD];
    DWORD dwStart, dwEnd;
    BOOL ret;
    LARGE_INTEGER liNextReadOffset = { 0 };
    int nReadsInProgress = 0;
    int nWritesInProgress = 0;

    hSrcFile = CreateFile(pszSrcFilename, GENERIC_READ, 0, 0, OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, 0);
    hDstFile = CreateFile(pszDstFilename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, 0);

    GetFileSizeEx(hSrcFile, &srcFileSize);
    dstFileSize.QuadPart = (srcFileSize.QuadPart / BUFFER_SIZE) * BUFFER_SIZE + ((srcFileSize.QuadPart % BUFFER_SIZE) > 0 ? BUFFER_SIZE : 0);
    SetFilePointerEx(hDstFile, dstFileSize, NULL, FILE_BEGIN);
    SetEndOfFile(hDstFile);

    hIO = CREATE_NEW_COMPLETION_PORT(NUM_THREAD);
    hTemp = ASSOCIATE_DEVICE_WITH_COMPLETION_PORT(hSrcFile, hIO, READ_KEY);
    if (hTemp != hIO)
        goto end;
    hTemp = ASSOCIATE_DEVICE_WITH_COMPLETION_PORT(hDstFile, hIO, WRITE_KEY);
    if (hTemp != hIO)
        goto end;

    dwStart = GetTickCount();
    for (int i = 0; i < NUM_THREAD; i++) {
        // Each I/O request requires a data buffer for transfers
        if (!ovs[i].AllocBuffer(BUFFER_SIZE))
            goto end;

        nWritesInProgress++;
        PostQueuedCompletionStatus(hIO, 0, WRITE_KEY, &ovs[i]);
    }

    printf("Start to copy file %ws, the size is :%d\n", pszSrcFilename, srcFileSize.QuadPart);

    while ((nReadsInProgress > 0) || (nWritesInProgress > 0)) {
        ULONG_PTR completionKey;
        DWORD dwByteTransfered;
        OVIOCP* ov;
        ret = GetQueuedCompletionStatus(hIO, &dwByteTransfered, &completionKey, (OVERLAPPED**)&ov, INFINITE);

        switch (completionKey) {
        case READ_KEY:
            nReadsInProgress--;
            printf("Start to write from %d to %d\n", ov->Offset, ov->Offset + ov->m_nBuffSize);
            WriteFile(hDstFile, ov->m_pvData, ov->m_nBuffSize, NULL, ov);
            nWritesInProgress++;
            break;
        case WRITE_KEY:
            nWritesInProgress--;
            if (liNextReadOffset.QuadPart < srcFileSize.QuadPart) {
                nReadsInProgress++;
                ov->Offset = liNextReadOffset.LowPart;
                ov->OffsetHigh = liNextReadOffset.HighPart;
                printf("Start to read from %d to %d\n", ov->Offset, ov->Offset + ov->m_nBuffSize);
                ReadFile(hSrcFile, ov->m_pvData, ov->m_nBuffSize, NULL, ov);
                liNextReadOffset.QuadPart += BUFFER_SIZE;
            }
            break;
        }
    }

    // truncate file size
    CloseHandle(hDstFile);
    hDstFile = CreateFile(pszDstFilename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    SetFilePointerEx(hDstFile, srcFileSize, NULL, FILE_BEGIN);
    SetEndOfFile(hDstFile);
    FlushFileBuffers(hDstFile);
    dwEnd = GetTickCount();
    printf("Copy time is %fs\n", (dwEnd - dwStart) / 1000.0f);

end:
    CloseHandle(hSrcFile);
    CloseHandle(hDstFile);
    CloseHandle(hIO);
    DeleteFile(pszDstFilename);
}