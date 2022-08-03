
#include <Windows.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <memory>
#include "Tools.h"

TCHAR PROC_NAME2[] = L"03.DuplicateHandle2.exe";
TCHAR PROC_NAME3[] = L"03.DuplicateHandle3.exe";

#define MUTEX_NAME_FORMAT  L"\\Sessions\\%u\\BaseNamedObjects\\TonyMutex"

/*
* Current process is the source
* "03.DuplicateHandle2.exe" is the target process
*/

void ClearProcesses(LPCWSTR pszProcName);
void CreateProcesses(LPCWSTR pszName, HANDLE* ph, DWORD* ppid);
void GetProcessHandleInformation(HANDLE h, std::unique_ptr<BYTE[]>& buffer);
void FindProcessHandle(BYTE* buffer, LPCWSTR pszHandleName, HANDLE hP, DWORD pid, HANDLE* pOut);


void TestDupTwoProcesses() {
	HANDLE hP;
	DWORD hPID;
	HANDLE hObj;
	BOOL ret;
	HANDLE hMutex;

	ClearProcesses(PROC_NAME3);
	CreateProcesses(PROC_NAME3, &hP, &hPID);

	Sleep(1000);
	
	hMutex = CreateMutex(NULL, 0, L"TonyMutex");
	ret = DuplicateHandle(GetCurrentProcess(), hMutex, hP, &hObj, 0, TRUE, DUPLICATE_SAME_ACCESS);
	if (!ret) {
		goto end;
	}
	else {
		printf("Success dup handle. Please watch in Process Explorer.\n");
		printf("Press Enter ton continue\n");
		getchar();
	}

end:
	TerminateProcess(hP, 0);
	CloseHandle(hP);
	CloseHandle(hMutex);
	CloseHandle(hObj);
}

/*
* Current process is the catalyst process
* "03.DuplicateHandle2.exe" is the source process
* "03.DuplicateHandle3.exe" is the target process
*  Maybe, we need to go through the process handle table
*/

void TestDupThreeProcesses() {
	HANDLE h2P, h3P;
	DWORD h2PID, h3PID;
	HANDLE h3Obj = 0;
	HANDLE hMutex = 0;
	BOOL ret;

	ClearProcesses(PROC_NAME2);
	ClearProcesses(PROC_NAME3);
	CreateProcesses(PROC_NAME2, &h2P, &h2PID);
	CreateProcesses(PROC_NAME3, &h3P, &h3PID);

	Sleep(1000);

	std::unique_ptr<BYTE[]> buffer;
	GetProcessHandleInformation(h2P, buffer);
	if (!buffer)
		goto end;
	FindProcessHandle(buffer.get(), MUTEX_NAME_FORMAT, h2P, h2PID, &hMutex);

	ret = DuplicateHandle(h2P, hMutex, h3P, &h3Obj, 0, TRUE, DUPLICATE_SAME_ACCESS);
	if (!ret) {
		goto end;
	}
	else {
		printf("Use 3rd process to dup \"TonyMutex\" handle from A to B successfully. Please watch in Process Explorer.\n");
		printf("Press Enter to continue\n");
		getchar();
	}

end:
	TerminateProcess(h2P, 0);
	TerminateProcess(h3P, 0);
	CloseHandle(h2P);
	CloseHandle(h3P);
}

void ClearProcesses(LPCWSTR pszProcName) {
	DWORD pid;
	HANDLE h;

	pid = GetProcessIDByName(pszProcName);
	if (pid) {
		h = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
		TerminateProcess(h, 0);
	}
}

void CreateProcesses(LPCWSTR pszName, HANDLE* ph, DWORD* ppid) {
	BOOL ret;

	STARTUPINFO sinfo = { 0 };
	sinfo.cb = sizeof(sinfo);
	PROCESS_INFORMATION pinfo = { 0 };

	ret = CreateProcess(nullptr, (LPWSTR)pszName, nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, 0, 0, &sinfo, &pinfo);
	if (!ret)
		exit(0);

	*ppid = pinfo.dwProcessId;
	*ph = pinfo.hProcess;
}

void GetProcessHandleInformation(HANDLE h, std::unique_ptr<BYTE[]>& buffer) {
	ULONG size = 1 << 10;
	
	for (;;) {
		buffer = std::make_unique<BYTE[]>(size);
		auto status = ::NtQueryInformationProcess(h, ProcessHandleInformation,
			buffer.get(), size, &size);
		if (NT_SUCCESS(status))
			break;
		if (status == STATUS_INFO_LENGTH_MISMATCH) {
			size += 1 << 10;
			continue;
		}
		printf("Error enumerating handles\n");
		exit(0);
	}
}

void FindProcessHandle(BYTE* buffer, LPCWSTR pszHandleName, HANDLE hP, DWORD pid, HANDLE* pOut) {
	BYTE nameBuffer[1 << 10];
	auto info = reinterpret_cast<PROCESS_HANDLE_SNAPSHOT_INFORMATION*>(buffer);

	for (ULONG i = 0; i < info->NumberOfHandles; i++) {
		HANDLE hv = info->Handles[i].HandleValue;
		HANDLE hTarget;
		if (!::DuplicateHandle(hP, hv, ::GetCurrentProcess(), &hTarget, 0, FALSE, DUPLICATE_SAME_ACCESS))
			continue;

		auto status = ::NtQueryObject(hTarget, ObjectNameInformation, nameBuffer, sizeof(nameBuffer), nullptr);
		::CloseHandle(hTarget);
		if (!NT_SUCCESS(status))
			continue;

		WCHAR targetName[256];
		DWORD sessionId;
		::ProcessIdToSessionId(pid, &sessionId);
		::swprintf_s(targetName, pszHandleName, sessionId);

		auto len = ::wcslen(targetName);
		auto name = reinterpret_cast<UNICODE_STRING*>(nameBuffer);
		if (name->Buffer) {
			wprintf(L"Handle Index: 0x%llX,\tHandle Name: %ws\n", hv, name->Buffer);
			if (::_wcsnicmp(name->Buffer, targetName, len) == 0) {
				// found it!
				*pOut = hv;
				break;
			}
		}
	}
	printf("\n");
}

BOOL CALLBACK OnClose(DWORD dwCtrlType) {
	if (dwCtrlType == CTRL_CLOSE_EVENT) {
		ClearProcesses(PROC_NAME2);
		ClearProcesses(PROC_NAME3);
	}

	return TRUE;
}

int main() {

	SetConsoleCtrlHandler(OnClose, TRUE);
	//TestDupTwoProcesses();
	TestDupThreeProcesses();

	return 0;
}
