#include <stdio.h>
#include <Windows.h>
#include <tchar.h>

#define DLL_NAME_19 L"19.DLLBasic.dll"

PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "LoadLibraryW");

void TestSimpleCreateRemoteThreadFail();
void TestSimpleCreateRemoteThread();

int main() {
	printf("TestSimpleCreateRemoteThreadFail\n");
	//TestSimpleCreateRemoteThreadFail();

	printf("\n");
	printf("TestSimpleCreateRemoteThread\n");
	TestSimpleCreateRemoteThread();
}

void TestSimpleCreateRemoteThreadFail() {
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	DWORD dwCode = 0;

	CreateProcess(L"C:\\Windows\\notepad.exe", 0, 0, 0, 0, 0, 0, 0, &si, &pi);
	Sleep(1000);

	HANDLE th = CreateRemoteThread(pi.hProcess, 0, 0, pfnThreadRtn, (void*)DLL_NAME_19, 0, 0);
	WaitForSingleObject(pi.hProcess, INFINITE);
	printf("thread and process have been terminated.\n");
}

void TestSimpleCreateRemoteThread() {
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	DWORD dwCode = 0;
	HANDLE hProcess = 0, hRemoteThread = 0;
	TCHAR szLib[MAX_PATH];
	LPWSTR pszFilename;
	LPWSTR pszLibFileRemote = 0;

	GetModuleFileName(NULL, szLib, _countof(szLib));
	pszFilename = wcsrchr(szLib, L'\\') + 1;
	wcscpy_s(pszFilename, _countof(szLib) - (pszFilename - szLib), DLL_NAME_19);

	int cch = 1 + lstrlenW(pszFilename);
	int cb = cch * sizeof(TCHAR);

	CreateProcess(L"C:\\Windows\\notepad.exe", 0, 0, 0, 0, 0, 0, 0, &si, &pi);
	Sleep(1000);

	__try {
		hProcess = OpenProcess(
			PROCESS_QUERY_INFORMATION |   // Required by Alpha
			PROCESS_CREATE_THREAD |	// For CreateRemoteThread
			PROCESS_VM_OPERATION |   // For VirtualAllocEx/VirtualFreeEx
			PROCESS_VM_WRITE,		// For WriteProcessMemory
			FALSE, pi.dwProcessId);
		if (hProcess == NULL) __leave;

		// Allocate space in the remote process for the pathname
		pszLibFileRemote = (PWSTR)VirtualAllocEx(hProcess, NULL, cb, MEM_COMMIT, PAGE_READWRITE);
		if (pszLibFileRemote == NULL) __leave;

		// Copy the DLL's pathname to the remote process' address space
		if (!WriteProcessMemory(hProcess, pszLibFileRemote, (PVOID)pszFilename, cb, NULL)) __leave;

		hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, pfnThreadRtn, pszLibFileRemote, 0, NULL);
		if (hRemoteThread == NULL) __leave;

		printf("Create remote thread successfully\n");
		WaitForSingleObject(hRemoteThread, INFINITE);
		printf("return from remote thread successfully\n");
	}
	__finally {
		// Free the remote memory that contained the DLL's pathname
		if (pszLibFileRemote != NULL)
			VirtualFreeEx(hProcess, pszLibFileRemote, 0, MEM_RELEASE);

		if (hRemoteThread != NULL)
			CloseHandle(hRemoteThread);

		if (hProcess != NULL)
			CloseHandle(hProcess);
	}
}