#define DELAYIMP_INSECURE_WRITABLE_HOOKS

#include <Windows.h>
#include <delayimp.h>
#include <stdio.h>
#include <strsafe.h>
#include "../19.DLLBasic/dll_test.h"

#pragma comment(lib, "Delayimp.lib")

// C:\Devtools\VS2019\Community\VC\Tools\MSVC\14.29.30133\lib\x64\Delayimp.lib
LPCWSTR DLL_NAME_19 = L"19.DLLBasic.dll";
LPCSTR DLL_NAME_NO_EXIST = "tonychen.dll";
LONG WINAPI DelayLoadDllExceptionFilter(PEXCEPTION_POINTERS pep);
FARPROC WINAPI DliHook(unsigned dliNotify, PDelayLoadInfo pdli);

void IsModuleLoaded(PCTSTR pszModuleName) {

	HMODULE hmod = GetModuleHandle(pszModuleName);
	char sz[128];

	StringCchPrintfA(sz, _countof(sz), "Module \"%S\" is %Sloaded.\n",
		pszModuleName, (hmod == NULL) ? L"not " : L"");

	printf(sz);
}

PfnDliHook __pfnDliNotifyHook2 = DliHook;
PfnDliHook __pfnDliFailureHook2 = DliHook;

void TestDelayLoadDLLSimple();
void TestDelayLoadMissingDLL();

int main() {
	printf("TestDelayLoadDLLSimple\n");
	TestDelayLoadDLLSimple();

	// comment TestDelayLoadDLLSimple if you want to test this
	printf("\n");
	printf("TestDelayLoadMissingDLL\n");
	TestDelayLoadMissingDLL();
}


void TestDelayLoadDLLSimple() {
	HMODULE hDll;
	IsModuleLoaded(DLL_NAME_19);
	add(1, 2);
	IsModuleLoaded(DLL_NAME_19);
	hDll = GetModuleHandle(DLL_NAME_19);

	__FUnloadDelayLoadedDLL2("19.DLLBasic.dll");
	IsModuleLoaded(DLL_NAME_19);
}

void TestDelayLoadMissingDLL() {
	BOOL ret;
	LPCWSTR pszDLLBak = L"19.DLLBasic.dll.bak";

	ret = CopyFile(DLL_NAME_19, pszDLLBak, FALSE);
	ret = DeleteFile(DLL_NAME_19);

	__try {
		IsModuleLoaded(DLL_NAME_19);
		add(1, 2);
	}
	__except (DelayLoadDllExceptionFilter(GetExceptionInformation())) {

	}

	CopyFile(pszDLLBak, DLL_NAME_19, FALSE);
}

LONG WINAPI DelayLoadDllExceptionFilter(PEXCEPTION_POINTERS pep) {

	// Assume we recognize this exception
	LONG lDisposition = EXCEPTION_EXECUTE_HANDLER;

	// If this is a Delay-load problem, ExceptionInformation[0] points
	// to a DelayLoadInfo structure that has detailed error info
	PDelayLoadInfo pdli = PDelayLoadInfo(pep->ExceptionRecord->ExceptionInformation[0]);

	// Create a buffer where we construct error messages
	char sz[512] = { 0 };

	switch (pep->ExceptionRecord->ExceptionCode) {
	case VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND):
		// The DLL module was not found at runtime
		StringCchPrintfA(sz, _countof(sz), "Dll not found: %s\n", pdli->szDll);
		break;

	case VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND):
		// The DLL module was found, but it doesn't contain the function
		if (pdli->dlp.fImportByName) {
			StringCchPrintfA(sz, _countof(sz), "Function %s was not found in %s\n",
				pdli->dlp.szProcName, pdli->szDll);
		}
		else {
			StringCchPrintfA(sz, _countof(sz), "Function ordinal %d was not found in %s\n",
				pdli->dlp.dwOrdinal, pdli->szDll);
		}
		break;

	default:
		// We don't recognize this exception
		lDisposition = EXCEPTION_CONTINUE_SEARCH;
		break;
	}

	if (lDisposition == EXCEPTION_EXECUTE_HANDLER) {
		// We recognized this error and constructed a message, show it
		printf(sz);
	}

	return(lDisposition);
}

FARPROC WINAPI DliHook(unsigned dliNotify, PDelayLoadInfo pdli) {
	FARPROC fp = NULL;


	switch (dliNotify) {
	case dliStartProcessing:
		// Called when __delayLoadHelper2 attempts to find a DLL/function
		// Return 0 to have normal behavior or nonzero to override
		// everything (you will still get dliNoteEndProcessing)
		printf("dliStartProcessing\n");
		break;

	case dliNotePreLoadLibrary:
		// Called just before LoadLibrary
		// Return NULL to have __delayLoadHelper2 call LoadLibary
		// or you can call LoadLibrary yourself and return the HMODULE
		fp = (FARPROC)(HMODULE)NULL;
		printf("dliNotePreLoadLibrary\n");
		break;

	case dliFailLoadLib:
		// Called if LoadLibrary fails
		// Again, you can call LoadLibary yourself here and return an HMODULE
		// If you return NULL, __delayLoadHelper2 raises the
		// ERROR_MOD_NOT_FOUND exception
		fp = (FARPROC)(HMODULE)NULL;
		printf("dliFailLoadLib\n");
		break;

	case dliNotePreGetProcAddress:
		// Called just before GetProcAddress
		// Return NULL to have __delayLoadHelper2 call GetProcAddress,
		// or you can call GetProcAddress yourself and return the address
		fp = (FARPROC)NULL;
		break;

	case dliFailGetProc:
		// Called if GetProcAddress fails
		// You can call GetProcAddress yourself here and return an address
		// If you return NULL, __delayLoadHelper2 raises the
		// ERROR_PROC_NOT_FOUND exception
		fp = (FARPROC)NULL;
		printf("dliFailGetProc\n");
		break;

	case dliNoteEndProcessing:
		// A simple notification that __delayLoadHelper2 is done
		// You can examine the members of the DelayLoadInfo structure
		// pointed to by pdli and raise an exception if you desire
		printf("dliNoteEndProcessing\n");
		break;
	}

	fp = (FARPROC)(HMODULE)NULL;
	return fp;
}