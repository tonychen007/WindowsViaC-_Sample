
#include <stdio.h>
#include <algorithm>
#include <Windows.h>

#pragma data_seg("tony_shared")
volatile long long g_inst = 0;
#pragma data_seg()

_declspec(allocate("tony_shared")) int a1 = 0;

// section should be created first
//_declspec(allocate("tony_shared1")) int a2 = 0;

#pragma comment(linker, "/SECTION:tony_shared,RWS")

void TestInstNum();
void TestReverseByte(const char* filename);
void ReverseChar(char* buf, int64_t len);
void TestMemMapped();
void TestFileMappedReverseByte(LPCWSTR filename);

int main(int argc, char** argv) {
    printf("TestInstNum\n");
    //TestInstNum();

    printf("\n");
    printf("TestReverseByte\n");
    TestReverseByte("./1.cpp");

    if (0) {
        if (argc == 2) {
            printf("Test large file\n");
            DWORD st = GetTickCount();
            TestReverseByte(argv[1]);
            DWORD ed = GetTickCount();
            printf("Total time is : %g\n", (ed - st) / 1000.0f);
        }
    }

    printf("\n");
    printf("TestMemMapped\n");
    TestMemMapped();

    printf("\n");
    printf("TestFileMappedReverseByte\n");
    DWORD st = GetTickCount();
    TestFileMappedReverseByte(L"./test.pdb");
    DWORD ed = GetTickCount();
    printf("Total time is : %g\n", (ed - st) / 1000.0f);
}

void TestInstNum() {
    InterlockedAdd64(&g_inst, 1);
    printf("Inst num is: %lld\n", g_inst);
    printf("Press key to end...\n");
    getchar();
}

void TestReverseByte(const char* filename) {
    const int totalLen = 16;
    const long long bufSize = 512*1024*1024LL;
    long long beginPos, endPos;
    long long i, j;
    char* beginBuf = (char*)malloc(bufSize);
    char* endBuf = (char*)malloc(bufSize);
    long long actLen;
    int actBufSize = bufSize;
    FILE* fp;

    // create file
    if (0) {
        fopen_s(&fp, ".\\test", "wb+");
        if (!fp)
            return;

        for (int i = 0; i < totalLen; i++) {
            fwrite(&i, 1, 1, fp);
        }

        fclose(fp);
    }

    fopen_s(&fp, filename, "rb+");
    if (!fp)
        return;

    fseek(fp, 0, SEEK_END);
    actLen = _ftelli64(fp);

    i = 0;
    j = actLen;
    fseek(fp, 0, SEEK_SET);
    beginPos = 0;
    endPos = -bufSize;

    // start to read, we stop if j - i < 2 * bufSize
    while (i + bufSize < j - bufSize) {
        fread(beginBuf, 1, bufSize, fp);
        i += bufSize;
        ReverseChar(beginBuf, bufSize);

        _fseeki64(fp, endPos, SEEK_END);

        fread(endBuf, 1, bufSize, fp);
        j -= bufSize;
        ReverseChar(endBuf, bufSize);

        // write end
        _fseeki64(fp, endPos, SEEK_END);
        fwrite(beginBuf, 1, bufSize, fp);
        endPos -= bufSize;

        // write begin
        _fseeki64(fp, beginPos, SEEK_SET);
        fwrite(endBuf, 1, bufSize, fp);
        beginPos += bufSize;

        // important!!!
        _fseeki64(fp, beginPos, SEEK_SET);
    }

    // total 16 bytes 0123456789ABCDEF
    // remaining is FEDCBA6789543210
    // need to reverse 6789
    _fseeki64(fp, beginPos, SEEK_SET);

    // at least it can read a full bufSize
    if (i < j) {

        // endBuf is smaller than bufSize
        if (j - i < bufSize) {
            actBufSize = j - i;
            endPos = -actBufSize;
        }

        memset(endBuf, 0, actBufSize);
        fread(beginBuf, 1, actBufSize, fp);
        ReverseChar(beginBuf, actBufSize);
        i += actBufSize;

        int rem = j - i;
        if (rem > 0) {
            endPos += bufSize - rem;
            _fseeki64(fp, endPos, SEEK_END);
            fread(endBuf, 1, rem, fp);
            ReverseChar(endBuf, actBufSize);
            j -= rem;
            endPos += rem - bufSize;
        }

        // write end
        _fseeki64(fp, endPos, SEEK_END);
        fwrite(beginBuf, 1, actBufSize, fp);

        // write begin
        if (rem > 0) {
            _fseeki64(fp, beginPos, SEEK_SET);
            fwrite(endBuf, 1, rem, fp);
        }
    }

    // last chunk
    fclose(fp);
    free(beginBuf);
    free(endBuf);
}

void ReverseChar(char* buf, int64_t len) {
    int64_t i = 0, j = len - 1;

    while (i < j) {
        char c = buf[i];
        buf[i] = buf[j];
        buf[j] = c;
        i++;
        j--;
    }
}

void TestMemMapped() {
    HANDLE hFile;
    HANDLE hFileMapped;
    LPVOID buf = 0;
    LPCWSTR pszFile = L"./MemoryMapped.cpp";
    //LPCWSTR pszFile = L"D:\\VMWARE\\WIN7\\WIN7.vmdk";
    DWORD chunkSize = 1024 * 1024 * 1024;
    LARGE_INTEGER li;
    LARGE_INTEGER fileSize;
    DWORD highSize;
    DWORD granuality = 65536;

    hFile = CreateFile(pszFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    hFileMapped = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    li.QuadPart = 0;
    int lowSize = GetFileSize(hFile, &highSize);
    fileSize.LowPart = lowSize;
    fileSize.HighPart = highSize;

    while (li.QuadPart < fileSize.QuadPart) {
        int rem = (fileSize.QuadPart - li.QuadPart) % chunkSize;
        int size = (rem == 0) ? chunkSize : rem;

        buf = MapViewOfFile(hFileMapped, FILE_MAP_READ, li.HighPart, li.LowPart, size);
        if (buf == NULL) {
            break;
        }
        UnmapViewOfFile(buf);
        li.QuadPart += size;
    }

    if (buf)
        UnmapViewOfFile(buf);

    CloseHandle(hFileMapped);
    CloseHandle(hFile);
}

void TestFileMappedReverseByte(LPCWSTR filename) {
    LARGE_INTEGER memSize, fileSize;
    DWORD highSize;
    DWORD granuality = 65536;
    HANDLE hFile;
    HANDLE hFileMapped;
    LPVOID buf;
    MEMORYSTATUS mst;

    hFile = CreateFile(filename, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    hFileMapped = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);

    int lowSize = GetFileSize(hFile, &highSize);
    fileSize.LowPart = lowSize;
    fileSize.HighPart = highSize;

    GlobalMemoryStatus(&mst);
    if (fileSize.QuadPart > mst.dwAvailPhys - 512 * 1024*1024) {
        printf("Not enough ram to map the file\n");
    }

    buf = MapViewOfFile(hFileMapped, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    ReverseChar((char*)buf, fileSize.QuadPart);
    UnmapViewOfFile(buf);
}