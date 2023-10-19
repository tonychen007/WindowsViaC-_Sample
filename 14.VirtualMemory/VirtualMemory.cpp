#include <stdio.h>
#include <stdint.h>
#include <Windows.h>
#include <tchar.h>
#include <Psapi.h>

#include "VMQuery.h"
#include "../CommonFiles/Toolhelp.h"

void TestCPUInfo();
void TestSysInfo();
void NumberToString(int64_t num, LPWSTR pszout, size_t cch);
void TestNUMA();
void TestVMQuery();

int main() {
    printf("Test CPU Info\n");
    TestCPUInfo();

    printf("\n");
    printf("Test Sys Info\n");
    TestSysInfo();

    printf("\n");
    printf("Test NUMA\n");
    TestNUMA();

    printf("\n");
    printf("Test VMQuery\n");
    TestVMQuery();
}

void TestCPUInfo() {
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buf = NULL;
    SYSTEM_INFO sysInfo;
    DWORD dwSize = 0;
    int cpuCore = 0;

    GetNativeSystemInfo(&sysInfo);
    GetLogicalProcessorInformation(buf, &dwSize);
    buf = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(dwSize);
    GetLogicalProcessorInformation(buf, &dwSize);
    int cnt = dwSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);

    for (int i = 0; i < cnt; i++) {
        if (buf[i].Relationship == RelationProcessorCore) {
            if (buf[i].ProcessorCore.Flags == 1) {
                printf("+ one CPU Core(Hyper Threading)\n");
            }
            else {
                printf("+ one CPU Core\n");
            }
            cpuCore++;
        }
    }

    printf("-> %d active core\n", cpuCore);
}

void TestSysInfo() {
    SYSTEM_INFO sysInfo;
    TCHAR psz[MAX_PATH] = { 0 };
    char cpuArch[MAX_PATH] = { 0 };

    GetNativeSystemInfo(&sysInfo);

    switch (sysInfo.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
        strcpy_s(cpuArch, _countof(cpuArch), "AMD64");
        break;
    default:
        strcpy_s(cpuArch, _countof(cpuArch), "Other");
        break;
    }

    printf("%25s:%20s\n","Process Architecture", cpuArch);
    printf("%25s:%20d\n", "Process Level", sysInfo.wProcessorLevel);
    printf("%25s:%18s%x\n", "Process Model", "0x", HIBYTE(sysInfo.wProcessorRevision));
    printf("%25s:%20d\n", "Process Stepping", LOBYTE(sysInfo.wProcessorRevision));
    printf("%25s:%20d\n", "Process Number", sysInfo.dwNumberOfProcessors);
    printf("%25s:%4s%016x\n", "ActiveProcess Masks", "0x", sysInfo.dwActiveProcessorMask);

    NumberToString(sysInfo.dwAllocationGranularity, psz, _countof(psz));
    wprintf(L"%25s:%20s\n", L"Allocation Granularity", psz);

    NumberToString(sysInfo.dwPageSize, psz, _countof(psz));
    wprintf(L"%25s:%20s\n", L"Page Size", psz);

    printf("%25s:%4s%016x\n", "Min App address", "0x", sysInfo.lpMinimumApplicationAddress);
    printf("%25s:%4s%016x\n", "Max App address", "0x", sysInfo.lpMaximumApplicationAddress);
}

void NumberToString(int64_t num, LPWSTR pszout, size_t cch) {
    TCHAR sz[MAX_PATH];
    NUMBERFMT numFMT;

    wsprintf(sz, L"%ld", num);
    numFMT.NumDigits = 0;
    numFMT.LeadingZero = 0;
    numFMT.NegativeOrder = 0;
    numFMT.Grouping = 3;
    numFMT.lpDecimalSep = (LPWSTR)L".";
    numFMT.lpThousandSep = (LPWSTR)L",";

    GetNumberFormat(LOCALE_SYSTEM_DEFAULT, 0, sz, &numFMT, pszout, cch);
}

void TestNUMA() {
    MEMORYSTATUSEX memStatEx = { sizeof(MEMORYSTATUSEX) };
    PROCESS_MEMORY_COUNTERS pmc;
    ULONG numaNode;
    UCHAR nodeNumber;
    ULONGLONG memBytes;

    GlobalMemoryStatusEx(&memStatEx);

    printf("Total Page File: %llu\n", memStatEx.ullTotalPageFile);
    printf("Total Physical Memory: %llu\n",memStatEx.ullTotalPhys);
    printf("Total Virtual Memory: %llu\n", memStatEx.ullTotalVirtual);

    GetNumaHighestNodeNumber(&numaNode);
    GetNumaProcessorNode(0, &nodeNumber);
    GetNumaAvailableMemoryNode(nodeNumber, &memBytes);
    printf("The avail memory on node %d is %llu MB\n", nodeNumber, memBytes / 1024 / 1024);

    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    printf("Working set is : %llu\n", pmc.WorkingSetSize);
    printf("Commit size is : %llu\n", pmc.PagefileUsage);
}

PCTSTR GetMemStorageText(DWORD dwStorage) {

    PCTSTR p = TEXT("Unknown");
    switch (dwStorage) {
        case MEM_FREE:    p = TEXT("Free   "); break;
        case MEM_RESERVE: p = TEXT("Reserve"); break;
        case MEM_IMAGE:   p = TEXT("Image  "); break;
        case MEM_MAPPED:  p = TEXT("Mapped "); break;
        case MEM_PRIVATE: p = TEXT("Private"); break;
    }
    return(p);
}

void TestVMQuery() {
    VMQUERY vmem;
    PVOID pvAddress = NULL;
    BOOL ret = TRUE;
    CToolhelp tool32;
    TCHAR pszFilename[MAX_PATH];

    DWORD pid = GetCurrentProcessId();
    tool32.CreateSnapshot(TH32CS_SNAPALL, pid);
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);

    printf("%s%25s%18s%8s%10s\n", "Address", "MEM State", "Region Size(KB)", "Blocks", "Module");
    while (ret) {
        ret = VMQuery(hProcess, pvAddress, &vmem);

        printf("0x%016llX", pvAddress);
        wprintf(L"%12s", GetMemStorageText(vmem.dwRgnStorage));
        printf("%17lld", vmem.RgnSize / 1024);
        printf("%8lld", vmem.dwRgnBlocks);

        if ((vmem.dwRgnStorage != MEM_FREE) && (vmem.pvRgnBaseAddress != NULL)) {
            MODULEENTRY32 me = { sizeof(me) };
            if (tool32.ModuleFind(vmem.pvRgnBaseAddress, &me)) {
                printf("%7s", "");
                wprintf(L"%s", me.szExePath);
            }
            else {
                DWORD len = GetMappedFileName(hProcess, vmem.pvRgnBaseAddress, pszFilename, MAX_PATH);
                if (len != 0) {
                    printf("%7s", "");
                    wprintf(L"%s", pszFilename);
                }
            }
        }

        if (vmem.bRgnIsAStack) {
            printf("%7s", "");
            printf("Thread Stack");
        }
        printf("\n");

        // todo blocks
        for (int i = 0; ret && (i < vmem.dwRgnBlocks); i++) {
            printf("  0x%016llX", pvAddress);
            wprintf(L"%10s", GetMemStorageText(vmem.dwRgnStorage));
            printf("%17lld", vmem.BlkSize);
            printf("\n");

            pvAddress = ((PBYTE)pvAddress + vmem.BlkSize);
            if (i < vmem.dwRgnBlocks - 1) {
                // Don't query the memory info after the last block.
                ret = VMQuery(hProcess, pvAddress, &vmem);
            }
        }

        pvAddress = ((PBYTE)vmem.pvRgnBaseAddress + vmem.RgnSize);
    }
}