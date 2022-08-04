#include <Windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <string>
using namespace std;

extern "C" const IMAGE_DOS_HEADER __ImageBase;

using StringDict = map<string, string>;
using StringDictItr = map<string, string>::const_iterator;

void ExtractEnviron(char** env, StringDict &envDict);
void DumpEnviron(StringDict& envDict);
void DumpModule(LPCWSTR pszModule, void* pAddress);

#define DUMP_MODULE(a) DumpModule(L#a, a)

int main(int argc, char** argv, char** env) {
    StringDict envDict;

    ExtractEnviron(env, envDict);
   ///DumpEnviron(envDict);

    HMODULE hModule = GetModuleHandle(NULL);
    printf("with GetModuleHandle(NULL) = 0x%x\r\n", hModule);
    printf("with __ImageBase = 0x%x\r\n", (HINSTANCE)&__ImageBase);

    DUMP_MODULE(DumpModule);
    DUMP_MODULE(MessageBox);
    DUMP_MODULE(CreateProcess);
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