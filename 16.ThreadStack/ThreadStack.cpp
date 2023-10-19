
#include <stdio.h>
#include <malloc.h>
#include <Windows.h>

#include "../CommonFiles/Tools.h"

#pragma comment(lib, "ntdll")

int g_cnt;
DWORD64 g_rsp;
int pageSize = 4096;

#define QUERY_ADDR(p, mbi, tib) { \
    VirtualQuery(p, &mbi, sizeof(mbi)); \
    DumpMbi(tib, mbi); \
}

void DumpMbi(const NT_TIB &tib, const MEMORY_BASIC_INFORMATION& mbi) {
    printf("Stack Base: \t%p\n", tib.StackBase);
    printf("Stack Limit: \t%p\n", mbi.BaseAddress);

    printf("Page Type: \t");
    if (mbi.State & MEM_COMMIT) {
        printf("MEM_COMMIT\n");
    }
    else if (mbi.State & MEM_RESERVE) {
        printf("MEM_RESERVE\n");
    }

    printf("Page Attribue: \t");
    if (mbi.AllocationProtect & PAGE_READWRITE) {
        printf("PAGE_READWRITE\n");
    }
    if (mbi.AllocationProtect & PAGE_GUARD) {
        printf(" | PAGE_GUARD\n");
    }

    printf("\n");
}

void GetTib(NT_TIB& tib) {
    CONTEXT ctx;
    ctx.ContextFlags = CONTEXT_CONTROL;
    THREAD_BASIC_INFORMATION threadInfo;

    /* 0 is ThreadBasicInformation */
    NtQueryInformationThread(GetCurrentThread(), (THREADINFOCLASS)0, &threadInfo, sizeof(threadInfo), NULL);
    GetThreadContext(GetCurrentThread(), &ctx);

    tib = *(NT_TIB*)(threadInfo.TebBaseAddress);
    g_rsp = ctx.Rsp;

    printf("addr of rsp: \t%p\n", g_rsp);
}

void Stackflow() {
    g_cnt++;
    char array[4096] = { 0 };

    MEMORY_BASIC_INFORMATION mbi;
    NT_TIB tib;

    GetTib(tib);
    QUERY_ADDR((DWORD64*)g_rsp, mbi, tib);
    // vs cannot query PAGE_GUARD, see it it windbg

    Stackflow();
}

void TestStackFlow() {
    __try {
        Stackflow();
    }
    __except (GetExceptionCode() == EXCEPTION_STACK_OVERFLOW ?
        EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        _resetstkoflw();
    }
}

void deadloop() {
    g_cnt++;
    char szMsg[512] = "";
    CONTEXT ctx;
    ctx.ContextFlags = CONTEXT_CONTROL;
    
    GetThreadContext(GetCurrentThread(), &ctx);
    g_rsp = ctx.Rsp;

    deadloop();
}

int main() {
    TestStackFlow();
    //deadloop();
}
