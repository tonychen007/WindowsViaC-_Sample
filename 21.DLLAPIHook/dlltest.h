#pragma once

#include <Windows.h>

#ifdef MYLIBAPI
#else
#define MYLIBAPI extern "C" __declspec(dllimport)
#endif

MYLIBAPI void setHook(int installed, DWORD deThreadId);
MYLIBAPI void setConsole(DWORD pid);

class CAPIHook {
public:
	CAPIHook(PCSTR pszModName, PCSTR pszFuncName, PROC pfnHook);
	~CAPIHook();

	operator PROC() { return(m_pfnOrigin); }
	static FARPROC WINAPI GetProcAddressRaw(HMODULE hmod, PCSTR pszProcName);

	// Replaces a symbol's address in all module's import section
	static void WINAPI ReplaceIATEntryInAllMods(PCSTR pszModName,
		PROC pfnOrig, PROC pfnHook);

	// Replaces a symbol's address in a modules' import sections
	static void WINAPI ReplaceIATEntryInOneMod(PCSTR pszModName,
		PROC pfnOrig, PROC pfnHook, HMODULE hmodCaller);

	// Replaces a symbol's address in a module's export sections
	static void ReplaceEATEntryInOneMod(HMODULE hmod, PCSTR pszFunctionName, PROC pfnNew);

	// Used when a DLL is newly loaded after hooking a function
	static void WINAPI FixupNewlyLoadedModule(HMODULE hmod, DWORD dwFlags);

private:
	static HMODULE WINAPI LoadLibraryW(PCWSTR pszModulePath);

	// Returns address of replacement function if hooked function is requested
	static FARPROC WINAPI GetProcAddress(HMODULE hmod, PCSTR pszProcName);

private:
	static PVOID sm_pvMaxAppAddr; // Maximum private memory address
	static CAPIHook* sm_pHead;    // Address of first object
	CAPIHook* m_pNext;            // Address of next  object

	PCSTR m_pszModName;
	PCSTR m_pszFuncName;
	PROC  m_pfnOrigin;              // Original function address in callee
	PROC  m_pfnHook;              // Hook function address

	static CAPIHook sm_LoadLibraryW;
	static CAPIHook sm_GetProcAddress;
};