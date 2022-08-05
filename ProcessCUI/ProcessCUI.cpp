#include <Windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <strsafe.h>
#include <map>
#include <string>
using namespace std;

extern "C" const IMAGE_DOS_HEADER __ImageBase;

using StringDict = map<string, string>;
using StringDictItr = map<string, string>::const_iterator;

void ExtractEnviron(char** env, StringDict &envDict);
void DumpEnviron(StringDict& envDict);
void TestDumpModule();
void DumpModule(LPCWSTR pszModule, void* pAddress);

void DumpEnviron();
void ExtractArgs(char** argv);
void TestEnvVarAndCurrentDir();


#define DUMP_MODULE(a) DumpModule(L#a, a)

int main(int argc, char** argv, char** env) {
    StringDict envDict;

    printf("***Get environ by char** env***\n");
    ExtractEnviron(env, envDict);
    DumpEnviron(envDict);

    printf("\n\n");
    TestDumpModule();

    printf("\n\n");
    printf("***Get environ by GetEnvironmentStrings()***\n");
    DumpEnviron();

    // -help -name Tony -age 25
    printf("\n\n");
    printf("***Get commandLineArgsW***\n");
    ExtractArgs(argv);

    printf("\n\n");
    printf("***Test Expand Env String and Current Working Direcotry***\n");
    TestEnvVarAndCurrentDir();

    int a = 0;
}

void ExtractEnviron(char** env, StringDict& envDict) {
    while (*env != NULL) {
        char* p = *env;
        size_t len = strlen(*env);
        size_t l1 = 0;
        string k;
        string v;

        while (p[l1] != '=')
            l1++;

        k.assign(*env, l1);
        v.assign(*env + l1 + 1, len - l1);
        envDict[k] = v;

        env++;
    }
}

void DumpEnviron(StringDict& envDict) {
    StringDictItr itr;

    itr = envDict.begin();
    for (; itr != envDict.end(); itr++) {
        auto k = itr->first.c_str();
        auto v = itr->second.c_str();
        printf("%s=%s\n", k, v);
    }
}

void DumpEnviron() {
    LPWCH env = GetEnvironmentStrings();
    LPCWSTR pszPos;
    DWORD len;
    TCHAR szName[MAX_PATH];
    TCHAR szValue[MAX_PATH];
    HRESULT hr;

    /* the first env may be = :: = ::\ */
    while (env != NULL) {
        if (*env != L'=') {
            pszPos = wcschr(env, L'=');
            pszPos++;

            // var name
            len = pszPos - env - sizeof(TCHAR);
            hr = StringCchCopyN(szName, MAX_PATH, env, len);
            if (FAILED(hr))
                break;

            // var value, 1 is ok null-terminated ch
            hr = StringCchCopyN(szValue, MAX_PATH, pszPos, lstrlen(pszPos) + 1);
            if (SUCCEEDED(hr)) {
                wprintf(L"%ws=%ws\n", szName, szValue);
            }
            else if (hr == STRSAFE_E_INSUFFICIENT_BUFFER) {
                // just display partial value is OK
                wprintf(L"%ws=%ws...\n", szName, szValue);
            }
        }

        while (*env != L'\0') {
            env++;
        }
        env++;

        if (*env == L'\0')
            break;
    }
}

void DumpModule(LPCWSTR pszModule, void* pAddress) {
    HMODULE hModule = NULL;
    TCHAR buf[MAX_PATH] = { L'\0' };
    int len = 0;

    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)pAddress, &hModule);
    GetModuleFileName(hModule, buf, MAX_PATH);

    len = lstrlenW(buf);
    while (buf[len - 1] != L'\\')
        len--;
    lstrcpyW(buf, buf + len);
    buf[len + 1] = L'0';

    wprintf(L"with function: %ws, the module is: %ws. GetModuleHandleEx = 0x%x\r\n", pszModule, buf, hModule);
}

void ExtractArgs(char** argv) {
    int num;
    PWSTR* targv = CommandLineToArgvW(GetCommandLineW(), &num);

    while (--num > 0) {
        ++targv;
        if (**targv == '-' || **targv == '/') {
            if (!lstrcmpW(targv[0], L"-help") || targv[0][1] == 'h') {
                printf("Help Usage.\n");
            }
            if (!lstrcmpW(targv[0], L"-name") || targv[0][1] == 'n') {
                wprintf(L"The name is: %ws.\n", (++targv)[0]);
                --num;
            }
            if (!lstrcmpW(targv[0], L"-age") || targv[0][1] == 'a') {
                wprintf(L"The age is: %ws.\n", (++targv)[0]);
                --num;
            }
        }
    }

    GlobalFree(targv);
}

void TestDumpModule() {
    HMODULE hModule = GetModuleHandle(NULL);
    printf("with GetModuleHandle(NULL) = 0x%x\r\n", hModule);
    printf("with __ImageBase = 0x%x\r\n", (HINSTANCE)&__ImageBase);

    DUMP_MODULE(DumpModule);
    DUMP_MODULE(MessageBox);
    DUMP_MODULE(CreateProcess);
}

void TestEnvVarAndCurrentDir() {

    // test for env variable
    LPCWSTR pszFile = L"C:test.txt";
    TCHAR buf[MAX_PATH] = { '\0' };
    LPCWSTR pszBuf = buf;
    BOOL ret;

    GetEnvironmentVariable(L"USERPROFILE", buf, MAX_PATH);
    wprintf(L"%s\n", buf);
    ExpandEnvironmentStrings(L"USERPROFILE=%USERPROFILE%", buf, MAX_PATH);
    wprintf(L"%s\n", buf);

    DWORD len = GetCurrentDirectory(0, buf);
    GetCurrentDirectory(len, buf);
    wprintf(L"Current working directory is: %ws\n", buf);

    // test driver working directory
    GetFullPathName(TEXT("C:"), MAX_PATH, buf, NULL);
    wprintf(L"working directort of C is: %ws\n", buf);
    HANDLE hf = CreateFile(pszFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FAILED(hf)) {
        wprintf(L"Create file :%ws failed. The error code is: %d\n", pszFile, GetLastError());
        goto end;
    }

    ret = WriteFile(hf, L"123", 6, NULL, NULL);
    if (!ret) {
        printf("Write to file failed.\n");
        goto end;
    }
    GetFinalPathNameByHandle(hf, buf, MAX_PATH, VOLUME_NAME_DOS);
    // strip "\\?\"
    pszBuf += lstrlen(L"\\\\?\\");
    wprintf(L"The file name is: %ws, and is written successfully.\n", pszBuf);
 end:
    CloseHandle(hf);
    DeleteFile(L"C:test.txt");
}