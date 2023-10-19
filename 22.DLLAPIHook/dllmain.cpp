#define MYLIBAPI extern "C" __declspec(dllexport)

#include "dlltest.h"
#include "../CommonFiles/Toolhelp.h"
#include <ImageHlp.h>
#include <strsafe.h>

HHOOK g_hHook = NULL;
extern CAPIHook g_MessageBoxA;
extern CAPIHook g_MessageBoxW;
extern CAPIHook g_ExitProcess;

#pragma data_seg("Shared")
DWORD g_conPid = NULL;
HANDLE gStdOut = NULL;
#pragma data_seg()
#pragma comment(linker, "/section:Shared,rws")

typedef int (WINAPI* PFNMESSAGEBOXA)(HWND hWnd, LPCSTR pszText, LPCSTR pszCaption, UINT uType);
typedef int (WINAPI* PFNMESSAGEBOXW)(HWND hWnd, LPCWSTR pszText, LPCWSTR pszCaption, UINT uType);
typedef void (WINAPI* PFNEXITPROCESS)(UINT code);

static HMODULE ModuleFromAddress(PVOID pv) {
	MEMORY_BASIC_INFORMATION mbi;
	return((VirtualQuery(pv, &mbi, sizeof(mbi)) != 0) ? (HMODULE)mbi.AllocationBase : NULL);
}

int WINAPI Hook_MessageBoxA(HWND hWnd, LPCSTR pszText, LPCSTR pszCaption, UINT uType) {
	LPCWSTR pszstr = L"Hook MessageBoxA\n";
	WriteConsole(gStdOut, pszstr, lstrlen(pszstr), 0, 0);
	((PFNMESSAGEBOXA)(PROC)g_MessageBoxA)(hWnd, pszText, pszCaption, uType);
	return 0;
}

int WINAPI Hook_MessageBoxW(HWND hWnd, LPCWSTR pszText, LPCWSTR pszCaption, UINT uType) {
	LPCWSTR pszstr = L"Hook MessageBoxW\n";
	WriteConsole(gStdOut, pszstr, lstrlen(pszstr), 0, 0);
	((PFNMESSAGEBOXW)(PROC)g_MessageBoxW)(hWnd, pszText, pszCaption, uType);
	return 0;
}

void WINAPI Hook_ExitProcess(int code) {
	((PFNMESSAGEBOXW)(PROC)g_MessageBoxW)(NULL, L"Hook Exit Process", 0, 0);
	((PFNEXITPROCESS)(PROC)g_ExitProcess)(code);
}

CAPIHook g_MessageBoxA("user32.dll", "MessageBoxA", (PROC)Hook_MessageBoxA);
CAPIHook g_MessageBoxW("user32.dll", "MessageBoxW", (PROC)Hook_MessageBoxW);
CAPIHook g_ExitProcess("kernel32.dll", "ExitProcess", (PROC)Hook_ExitProcess);

CAPIHook* CAPIHook::sm_pHead = NULL;
CAPIHook CAPIHook::sm_LoadLibraryW("Kernel32.dll", "LoadLibraryW", (PROC)CAPIHook::LoadLibraryW);
CAPIHook CAPIHook::sm_GetProcAddress("Kernel32.dll", "GetProcAddress", (PROC)CAPIHook::GetProcAddress);

CAPIHook::CAPIHook(PCSTR pszModName, PCSTR pszFuncName, PROC pfnHook) {
	m_pNext = sm_pHead;
	sm_pHead = this;
	m_pszModName = pszModName;
	m_pszFuncName = pszFuncName;
	m_pfnHook = pfnHook;
	m_pfnOrigin = GetProcAddressRaw(GetModuleHandleA(pszModName), m_pszFuncName);
	if (m_pfnOrigin == NULL) {
		::LoadLibraryA(pszModName);
		m_pfnOrigin = GetProcAddressRaw(GetModuleHandleA(pszModName), m_pszFuncName);
	}

	if (m_pfnOrigin == NULL) {
#ifdef _DEBUG
		wchar_t szPathname[MAX_PATH];
		GetModuleFileNameW(NULL, szPathname, _countof(szPathname));
		wchar_t sz[1024];
		StringCchPrintfW(sz, _countof(sz),
			TEXT("[%4u - %s] impossible to find %S\r\n"),
			GetCurrentProcessId(), szPathname, pszFuncName);
		OutputDebugString(sz);
#endif
		return;
	}

	ReplaceIATEntryInAllMods(m_pszModName, m_pfnOrigin, m_pfnHook);
}

CAPIHook::~CAPIHook() {

	// Unhook this function from all modules
	ReplaceIATEntryInAllMods(m_pszModName, m_pfnHook, m_pfnOrigin);

	// Remove this object from the linked list
	CAPIHook* p = sm_pHead;
	if (p == this) {     // Removing the head node
		sm_pHead = p->m_pNext;
	}
	else {
		BOOL bFound = FALSE;

		// Walk list from head and fix pointers
		for (; !bFound && (p->m_pNext != NULL); p = p->m_pNext) {
			if (p->m_pNext == this) {
				// Make the node that points to us point to our next node
				p->m_pNext = p->m_pNext->m_pNext;
				bFound = TRUE;
			}
		}
	}
}

FARPROC CAPIHook::GetProcAddressRaw(HMODULE hmod, PCSTR pszProcName) {
	return ::GetProcAddress(hmod, pszProcName);
}

void CAPIHook::ReplaceIATEntryInAllMods(PCSTR pszModName, PROC pfnCurrent, PROC pfnNew) {
	HMODULE hmodThisMod = ModuleFromAddress(ReplaceIATEntryInAllMods);
	CToolhelp th(TH32CS_SNAPMODULE, GetCurrentProcessId());
	MODULEENTRY32 me = { sizeof(me) };

	for (BOOL bOk = th.ModuleFirst(&me); bOk; bOk = th.ModuleNext(&me)) {
		if (me.hModule != hmodThisMod) {
			ReplaceIATEntryInOneMod(pszModName, pfnCurrent, pfnNew, me.hModule);
		}
	}
}

void CAPIHook::ReplaceIATEntryInOneMod(PCSTR pszModName, PROC pfnCurrent, PROC pfnNew, HMODULE hmodCaller) {
	BOOL ret;
	ULONG ulSize;
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = NULL;
	pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToData(
		hmodCaller, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &ulSize);

	if (pImportDesc == NULL)
		return;

	// Name is the offset of address
	for (; pImportDesc->Name; pImportDesc++) {
		LPSTR pszModNameRVA = (LPSTR)((CHAR*)(hmodCaller) + pImportDesc->Name);

		if (lstrcmpiA(pszModName, pszModNameRVA) == 0) {
			PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)((CHAR*)(hmodCaller) + pImportDesc->FirstThunk);

			for (; pThunk->u1.Function; pThunk++) {
				PROC* ppfn = (PROC*)&pThunk->u1.Function;
				if (*ppfn == pfnCurrent) {
					ret = WriteProcessMemory(GetCurrentProcess(), ppfn, &pfnNew, sizeof(pfnNew), NULL);
					if (!ret && GetLastError() == ERROR_NOACCESS) {
						DWORD dwOldPro;
						ret = VirtualProtect(ppfn, sizeof(pfnNew), PAGE_WRITECOPY, &dwOldPro);
						if (ret) {
							WriteProcessMemory(GetCurrentProcess(), ppfn, &pfnNew, sizeof(pfnNew), NULL);
							VirtualProtect(ppfn, sizeof(pfnNew), dwOldPro, &dwOldPro);
						}

					}
				}
			}
		} // end of lstrcmpiA
	}
}

void CAPIHook::FixupNewlyLoadedModule(HMODULE hmod, DWORD dwFlags) {
	// If a new module is loaded, hook the hooked functions
	if ((hmod != NULL) &&
		(hmod != ModuleFromAddress(FixupNewlyLoadedModule)) &&
		((dwFlags & LOAD_LIBRARY_AS_DATAFILE) == 0) &&
		((dwFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) == 0) &&
		((dwFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE) == 0)
		) {

		for (CAPIHook* p = sm_pHead; p != NULL; p = p->m_pNext) {
			if (p->m_pfnOrigin != NULL) {
				ReplaceIATEntryInAllMods(p->m_pszModName,
					p->m_pfnOrigin, p->m_pfnHook);
			}
		}
	}
}

HMODULE CAPIHook::LoadLibraryW(PCWSTR pszModulePath) {
	HMODULE hmod = ::LoadLibraryW(pszModulePath);
	FixupNewlyLoadedModule(hmod, 0);
	return(hmod);
}

FARPROC CAPIHook::GetProcAddress(HMODULE hmod, PCSTR pszProcName) {
	FARPROC pfn = GetProcAddressRaw(hmod, pszProcName);

	CAPIHook* p = sm_pHead;
	for (; (pfn != NULL) && (p != NULL); p = p->m_pNext) {
		if (pfn == p->m_pfnOrigin) {
			// The address to return matches an address we want to hook
			// Return the hook function address instead
			pfn = p->m_pfnHook;
			break;
		}
	}

	return pfn;
}

static LRESULT WINAPI GetMsgProc(int code, WPARAM wParam, LPARAM lParam) {
	return(CallNextHookEx(g_hHook, code, wParam, lParam));
}

void setHook(int installed, DWORD dwThreadId) {
	if (installed) {
		if (g_hHook == NULL) {
			g_hHook = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc,
				ModuleFromAddress(setHook), dwThreadId);
		}
	} else {
		if (g_hHook != NULL) {
			UnhookWindowsHookEx(g_hHook);
			g_hHook = NULL;
		}
	}
}

MYLIBAPI void setConsole(DWORD pid) {
	g_conPid = pid;
	AttachConsole(pid);
	gStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
}