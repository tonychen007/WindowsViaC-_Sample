#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <Windows.h>

void TestCreateFile(bool withBuffer = 0);
void TestCloseOnDelete();
void DumpFileinCurrDir();
void TestSetEndFile();
void TestWriteFileOverlapped();
void TestMutipleFileOverlapped();

int main() {
    printf("TestCreateFileWithNoBuffer\n");
    TestCreateFile();

    printf("\n");
    printf("TestCreateFileWithBuffer\n");
    TestCreateFile(1);

    printf("\n");
    printf("TestCloseOnDelete\n");
    TestCloseOnDelete();

    printf("\n");
    printf("TestSetEndFile\n");
    TestSetEndFile();

    printf("\n");
    printf("TestWriteFileOverlapped\n");
    TestWriteFileOverlapped();

    printf("\n");
    printf("TestMutipleFileOverlapped\n");
    TestMutipleFileOverlapped();
}

void TestCreateFile(bool withBuffer) {
    LPCWSTR pszFilename = L"./test.txt";
    BYTE buf[512] = { 0 };
    DWORD written;
    BOOL ret;

    DWORD dwFlags = withBuffer == 0 ? FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH : 0;
    const char* retOk = "write should success\n";
    const char* retStr = withBuffer == 0 ? "write should fail\n" : retOk;

    HANDLE hFile = CreateFile(pszFilename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, dwFlags, 0);
    
    printf("write 4 bytes, not multiple of sector size!\n");
    ret = WriteFile(hFile, buf, 4, &written, 0);
    printf(retStr);
    printf("ret: %d\n", ret);
    printf("\n");

    printf("write 512 bytes, multiple of sector size!\n");
    ret = WriteFile(hFile, buf, 512, &written, 0);
    printf(retOk);
    printf("ret: %d\n", ret);
    printf("\n");

    int64_t sz = 1024 * 1024 * 4LL;
    int64_t* largeBuf = new int64_t[sz];
    memset(largeBuf, 1, sz);

    printf("write %lld bytes, multiple of sector size!\n", sz);
    ret = WriteFile(hFile, largeBuf, sz, &written, 0);
    printf(retOk);
    assert(ret != 0);
    FlushFileBuffers(hFile);

    delete[] largeBuf;
    CloseHandle(hFile);
    DeleteFile(pszFilename);
}

void TestCloseOnDelete() {
    LPCWSTR pszFilename = L"./test.txt";
    DWORD ret;

    HANDLE hFile = CreateFile(pszFilename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, 0);

    printf("Before CloseHandle, files are:\n");
    ret = GetFileAttributes(pszFilename);
    DumpFileinCurrDir();

    printf("\n");
    printf("After CloseHandle, Files are:\n");
    CloseHandle(hFile);
    DumpFileinCurrDir();
}

void DumpFileinCurrDir() {
    WIN32_FIND_DATA findData;
    HANDLE h;

    h = FindFirstFile(L".\\*", &findData);

    do {
        if (lstrcmpW(findData.cFileName, L".") == 0 ||
            lstrcmpW(findData.cFileName, L"..") == 0 ||
            findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        wprintf(L"%s\n", findData.cFileName);
    } while (FindNextFile(h, &findData) != 0);
}

void TestSetEndFile() {
    LPCWSTR pszFilename = L"./test.txt";
    LARGE_INTEGER li;
    DWORD ret;
    DWORD rw;
    BY_HANDLE_FILE_INFORMATION fInfo;

    HANDLE hFile = CreateFile(pszFilename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

    li.QuadPart = 4 * 1024 * 1024;
    ret = SetFilePointerEx(hFile, li , NULL, FILE_BEGIN);
    SetEndOfFile(hFile);
    GetFileInformationByHandle(hFile, &fInfo);
    CloseHandle(hFile);
    printf("Create new file and set file pointer by 4M. The size is %d\n", fInfo.nFileSizeLow);

    hFile = CreateFile(pszFilename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

    int sz = 1024 * 1024 * 8;
    BYTE* buf = new BYTE[sz];
    WriteFile(hFile, buf, sz, &rw, 0);
    GetFileInformationByHandle(hFile, &fInfo);
    CloseHandle(hFile);
    printf("Create new file and write buffer 8M. The size is %d\n", fInfo.nFileSizeLow);
    delete[] buf;

    hFile = CreateFile(pszFilename, GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    li.QuadPart = 2 * 1024;
    ret = SetFilePointerEx(hFile, li, NULL, FILE_BEGIN);
    SetEndOfFile(hFile);
    GetFileInformationByHandle(hFile, &fInfo);
    printf("After setFileEnd truncate to 2K. The size is %d\n", fInfo.nFileSizeLow);
    CloseHandle(hFile);

    DeleteFile(pszFilename);
}

void TestWriteFileOverlapped() {
    OVERLAPPED ov = { 0 };
    LPCWSTR pszFilename = L"./test.txt";
    DWORD ret;
    DWORD rw;
    LARGE_INTEGER endPosition;
    size_t sz = 1024 * 1024 * 100;
    BYTE* buf = new BYTE[sz];

    HANDLE hFile = CreateFile(pszFilename, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS,
        FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, 0);

    // file pointer should be set before call async write
    endPosition.QuadPart = sz;
    ret = SetFilePointerEx(hFile, endPosition, NULL, FILE_BEGIN);
    if (ret == INVALID_SET_FILE_POINTER) {
        return;
    }
    if (!SetEndOfFile(hFile)) {
        return;
    }

    ov.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    ov.Offset = -1;
    ov.OffsetHigh = -1;
    // the max size one time write is 2GB
    WriteFile(hFile, buf, sz, NULL, &ov);
    printf("WriteFile return immediately\n");
    WaitForSingleObject(ov.hEvent, INFINITE);
    printf("%lld size written\n", ov.InternalHigh);

    CloseHandle(hFile);
    DeleteFile(pszFilename);
}

void TestMutipleFileOverlapped() {

}