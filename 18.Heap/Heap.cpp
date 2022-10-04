
#include <stdio.h>
#include <Windows.h>
#include <tchar.h>

void HeapWalk(HANDLE& hHeap) {
	PROCESS_HEAP_ENTRY Entry;

    Entry.lpData = NULL;

    while (HeapWalk(hHeap, &Entry) != FALSE) {
        if ((Entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) != 0) {
            _tprintf(TEXT("Allocated block"));

            if ((Entry.wFlags & PROCESS_HEAP_ENTRY_MOVEABLE) != 0) {
                _tprintf(TEXT(", movable with HANDLE %#p"), Entry.Block.hMem);
            }

            if ((Entry.wFlags & PROCESS_HEAP_ENTRY_DDESHARE) != 0) {
                _tprintf(TEXT(", DDESHARE"));
            }
        }
        else if ((Entry.wFlags & PROCESS_HEAP_REGION) != 0) {
            _tprintf(TEXT("Region\n  %d bytes committed\n") \
                TEXT("  %d bytes uncommitted\n  First block address: %#p\n") \
                TEXT("  Last block address: %#p\n"),
                Entry.Region.dwCommittedSize,
                Entry.Region.dwUnCommittedSize,
                Entry.Region.lpFirstBlock,
                Entry.Region.lpLastBlock);
        }
        else if ((Entry.wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE) != 0) {
            _tprintf(TEXT("Uncommitted range\n"));
        }
        else {
            _tprintf(TEXT("Block\n"));
        }

        _tprintf(TEXT("  Data portion begins at: %#p\n  Size: %d bytes\n") \
            TEXT("  Overhead: %d bytes\n  Region index: %d\n\n"),
            Entry.lpData,
            Entry.cbData,
            Entry.cbOverhead,
            Entry.iRegionIndex);
    }
}

/*
* The size you can alloc is large than create
* because the heap has some internal structures to keep its metainfo
*/
int main() {
	HANDLE hHeap = GetProcessHeap();
    SYSTEM_INFO sysInfo;
	ULONG HeapInformationValue = 2;
	BYTE* buf;
	BOOL ret;
	DWORD size;

    /*
    GetSystemInfo(&sysInfo);
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
	ret = HeapSetInformation(hHeap, HeapCompatibilityInformation, &HeapInformationValue, sizeof(HeapInformationValue));
    */

	hHeap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, 1, 1);
    HeapWalk(hHeap);
    printf("----------------------------------\n");

	buf = (BYTE*)HeapAlloc(hHeap, HEAP_GENERATE_EXCEPTIONS, 3000);
    HeapWalk(hHeap);
    printf("----------------------------------\n");

	buf = (BYTE*)HeapAlloc(hHeap, HEAP_GENERATE_EXCEPTIONS, 3224);
    HeapWalk(hHeap);
    printf("----------------------------------\n");

    buf = (BYTE*)HeapAlloc(hHeap, HEAP_GENERATE_EXCEPTIONS, 24);
    HeapWalk(hHeap);
    printf("----------------------------------\n");

    HeapFree(hHeap, 0, buf);
    HeapDestroy(hHeap);

    // try to alloc large heap
    size = 1024 * 1024 * 1024;
    hHeap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, size, size);
    __try {
        buf = (BYTE*)HeapAlloc(hHeap, HEAP_GENERATE_EXCEPTIONS, size);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        printf("alloc 1G heap failed\n");
    }

    HeapDestroy(hHeap);
}