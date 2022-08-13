#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <strsafe.h>

#include "Tools.h"

LPCWSTR g_pszJob = L"TonyJob";

#define CREATE_JOB_AND_GET_PROC(hJob, hProc) { \
	hJob = CreateJobObject(NULL, g_pszJob); \
	hProc = GetCurrentProcess(); \
	}

// Completion keys for the completion port
ULONG COMPKEY_TERMINATE = 0;
ULONG COMPKEY_STATUS = 1;
ULONG COMPKEY_JOBOBJECT = 2;

void TestCloseJob();
void TestJobBasicLimit();
void TestJobExtLimit();
void TestJobUILimit();
void TestJobUIHandleLimit();
void TestQueryInformationJob();
void TestBreakAwayFromJob();
void TestEnumProcessInJob();
void TestJobCompletionPort();

DWORD WINAPI JobNotify(PVOID);

int main() {
	TestCloseJob();
	printf("\n");
	printf("Test job basic limit\n");
	//TestJobLimit();

	printf("\n");
	printf("Test job ext limit\n");
	TestJobExtLimit();

	printf("\n");
	printf("Test job ui limit\n");
	TestJobUILimit();

	printf("\n");
	printf("Test job ui handle limit\n");
	TestJobUIHandleLimit();

	printf("\n");
	printf("Test job information query\n");
	TestQueryInformationJob();

	printf("\n");
	printf("Test enum process job\n");
	TestEnumProcessInJob();

	printf("\n");
	printf("Test job completion port\n");
	TestJobCompletionPort();

	printf("\n");
	printf("Test job break away\n");
	TestBreakAwayFromJob();

	return 0;
}

void TestCloseJob() {
	HANDLE hJob;
	HANDLE hProc;
	CREATE_JOB_AND_GET_PROC(hJob, hProc);
	AssignProcessToJobObject(hJob, hProc);

	printf("Watch through Process Explorer. The process should be in brown. Job: 0x%X\n", hJob);

	CloseHandle(hJob);
	hJob = OpenJobObject(JOB_OBJECT_ALL_ACCESS, FALSE, g_pszJob);

	printf("Close jobobject and open again, but should fail. Job: 0x%X\n", hJob);

	CloseHandle(hJob);
	CloseHandle(hProc);
}

void TestJobBasicLimit() {
	JOBOBJECT_BASIC_LIMIT_INFORMATION basicLimit;
	LARGE_INTEGER li;
	HANDLE hJob;
	HANDLE hProc;
	CREATE_JOB_AND_GET_PROC(hJob, hProc);

	// the time is 100-nano, so set to 5 secs
	li.QuadPart = 5 * 10 * 1000 * 1000;
	basicLimit.PerProcessUserTimeLimit = li;
	basicLimit.LimitFlags = JOB_OBJECT_LIMIT_PROCESS_TIME;
	SetInformationJobObject(hJob, JobObjectBasicLimitInformation, &basicLimit, sizeof(basicLimit));
	AssignProcessToJobObject(hJob, hProc);
	printf("Job will terminate in five seconds.\n");
	while (1);
}

void TestJobExtLimit() {
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION extLimit = { 0 };
	LARGE_INTEGER li;
	HANDLE hJob;
	HANDLE hProc;
	CREATE_JOB_AND_GET_PROC(hJob, hProc);

	printf("Job cannot allocate more than 16M memory.\n");
	extLimit.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_JOB_MEMORY;
	extLimit.JobMemoryLimit = 1024 * 1024 * 16;
	SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &extLimit, sizeof(extLimit));
	AssignProcessToJobObject(hJob, hProc);

	// mem will leak, so next time the mem limit should plus this 16M
	PROCESS_MEMORY_COUNTERS pmc = { 0 };
	GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc));
	while (1) {
		void* p = malloc(65536);
		if (!p) {
			printf("Reach commit size!!\n");
			break;
		}

		GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc));
		printf("Working set is: %d\n", pmc.PagefileUsage / 1024);
		ZeroMemory(&pmc, sizeof(pmc));
		Sleep(10);
	}

	// remove mem limit
	extLimit.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_JOB_MEMORY;
	extLimit.JobMemoryLimit = 0;
	SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &extLimit, sizeof(extLimit));

	CloseHandle(hJob);
	CloseHandle(hProc);
}

void TestJobUILimit() {
	JOBOBJECT_BASIC_UI_RESTRICTIONS uiLimit = { 0 };
	HANDLE hJob;
	HANDLE hProc;
	HGLOBAL hClipboardData;
	LPVOID lpClipData;
	CREATE_JOB_AND_GET_PROC(hJob, hProc);

	// set clipboard
	OpenClipboard(NULL);
	hClipboardData = GlobalAlloc(GMEM_MOVEABLE, 10);
	lpClipData = GlobalLock(hClipboardData);
	memset(lpClipData, 0, 10);
	memcpy(lpClipData, "tony", 4);
	GlobalUnlock(hClipboardData);
	SetClipboardData(CF_TEXT, hClipboardData);

	printf("Before restrict on reading clipboard.\n");
	hClipboardData = GetClipboardData(CF_TEXT);
	lpClipData = GlobalLock(hClipboardData);
	printf("The clipboard data is: %s\n", lpClipData);
	GlobalUnlock(hClipboardData);

	printf("\nRestrict on ExitWindow.\n");
	uiLimit.UIRestrictionsClass = JOB_OBJECT_UILIMIT_EXITWINDOWS | JOB_OBJECT_UILIMIT_READCLIPBOARD | JOB_OBJECT_UILIMIT_DESKTOP;
	SetInformationJobObject(hJob, JobObjectBasicUIRestrictions, &uiLimit, sizeof(uiLimit));
	AssignProcessToJobObject(hJob, hProc);
	printf("Calling ExitWindows...\n");
	ExitWindows(EWX_LOGOFF, 0);
	Sleep(100);
	printf("Log off window should not be shown.\n");

	printf("\nRestrict on Read clipboard.\n");
	printf("After restrict on reading clipboard.\n");
	hClipboardData = GetClipboardData(CF_TEXT);
	printf("hClipboardData %p should be 0\n", hClipboardData);

	CloseHandle(hJob);
	CloseHandle(hProc);
}

void TestJobUIHandleLimit() {
	BOOL ret;
	JOBOBJECT_BASIC_UI_RESTRICTIONS uiLimit = { 0 };
	HANDLE hJob;
	HANDLE hProc, hSysProc;
	HANDLE hToken, hSysTokenDup;
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi = { 0 };
	// change on your env
	TCHAR pszApp[] = L"C:\\Devtools\\WIN10SDK\\bin\\10.0.19041.0\\x64\\inspect.exe";
	CREATE_JOB_AND_GET_PROC(hJob, hProc);

	// we need to elevate right
	DWORD pid = GetProcessIDByName(L"winlogon.exe");
	hSysProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	OpenProcessToken(hSysProc, TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY, &hToken);
	DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityIdentification, TokenPrimary, &hSysTokenDup);

	// now, we can winlogon.exe token
	TOKEN_PRIVILEGES tp;
	DWORD dwUIAccess = 1;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	LookupPrivilegeValue(NULL, SE_ASSIGNPRIMARYTOKEN_NAME, &tp.Privileges[0].Luid);
	AdjustTokenPrivileges(hSysTokenDup, FALSE, &tp, sizeof(tp), NULL, NULL);
	ImpersonateLoggedOnUser(hSysTokenDup);

	SetTokenInformation(hSysTokenDup, TokenUIAccess, &dwUIAccess, sizeof(dwUIAccess));

	printf("inspect can only see it own windows\n");
	CreateProcessAsUser(hSysTokenDup, pszApp, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);

	uiLimit.UIRestrictionsClass = JOB_OBJECT_UILIMIT_HANDLES;
	SetInformationJobObject(hJob, JobObjectBasicUIRestrictions, &uiLimit, sizeof(uiLimit));
	AssignProcessToJobObject(hJob, pi.hProcess);
	ResumeThread(pi.hThread);
	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(hJob);
	CloseHandle(hProc);
}

void TestQueryInformationJob() {
	JOBOBJECT_BASIC_LIMIT_INFORMATION basicLimit, basicLimit2;
	LARGE_INTEGER li;
	HANDLE hJob;
	HANDLE hProc;
	CREATE_JOB_AND_GET_PROC(hJob, hProc);

	// the time is 100-nano, so set to 5 secs
	li.QuadPart = 5 * 10 * 1000 * 1000;
	basicLimit.PerProcessUserTimeLimit = li;
	basicLimit.LimitFlags = JOB_OBJECT_LIMIT_PROCESS_TIME;
	SetInformationJobObject(hJob, JobObjectBasicLimitInformation, &basicLimit, sizeof(basicLimit));
	QueryInformationJobObject(hJob, JobObjectBasicLimitInformation, &basicLimit2, sizeof(basicLimit2), 0);

	CloseHandle(hJob);
	CloseHandle(hProc);
}

void TestEnumProcessInJob() {
	const int MAX_PROCESSES = 10;
	JOBOBJECT_BASIC_PROCESS_ID_LIST* pJobList;
	HANDLE hJob;
	HANDLE hProc;
	CREATE_JOB_AND_GET_PROC(hJob, hProc);

	AssignProcessToJobObject(hJob, hProc);
	SIZE_T cb = sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) + (MAX_PROCESSES - 1) * sizeof(SIZE_T);
	pJobList = (JOBOBJECT_BASIC_PROCESS_ID_LIST*)malloc(cb);
	pJobList->NumberOfProcessIdsInList = MAX_PROCESSES;
	QueryInformationJobObject(hJob, JobObjectBasicProcessIdList, pJobList, cb, 0);

	for (int i = 0; i < pJobList->NumberOfProcessIdsInList; i++) {
		int pid = pJobList->ProcessIdList[i];
		printf("Pid in job: %d\n", pid);
	}

	printf("Press any key...\n");
	getchar();
	free(pJobList);
	CloseHandle(hJob);
	CloseHandle(hProc);
}

DWORD WINAPI JobNotify(LPVOID args) {
	TCHAR sz[2000];
	BOOL fDone = FALSE;
	HANDLE* hArr = (HANDLE*)args;
	HANDLE hIOCP = hArr[1];
	HANDLE hJob = hArr[0];

	while (!fDone) {
		DWORD dwBytesXferred;
		ULONG_PTR CompKey;
		LPOVERLAPPED po;
		GetQueuedCompletionStatus(hIOCP,
			&dwBytesXferred, &CompKey, &po, INFINITE);

		if (CompKey == COMPKEY_TERMINATE) {
			printf("receive job terminate notification\n");
			fDone = TRUE;
		}

		if (CompKey == COMPKEY_JOBOBJECT) {
			switch (dwBytesXferred) {
			case JOB_OBJECT_MSG_JOB_MEMORY_LIMIT:
				printf("receive job mem limit notification\n");
				break;
			}
		}

		if (CompKey == COMPKEY_STATUS) {
			JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION jobai;
			QueryInformationJobObject(
				hJob,
				JobObjectBasicAndIoAccountingInformation,
				&jobai, sizeof(jobai), NULL);

			StringCchPrintf(sz, _countof(sz),
				TEXT("Total Time: User=%I64u, Kernel=%I64u        ")
				TEXT("Period Time: User=%I64u, Kernel=%I64u"),
				jobai.BasicInfo.TotalUserTime.QuadPart,
				jobai.BasicInfo.TotalKernelTime.QuadPart,
				jobai.BasicInfo.ThisPeriodTotalUserTime.QuadPart,
				jobai.BasicInfo.ThisPeriodTotalKernelTime.QuadPart);

			wprintf(L"%s\n", sz);
		}
	}

	return 0;
}

void TestJobCompletionPort() {
	HANDLE hJob;
	HANDLE hProc;
	HANDLE hIOCP;
	HANDLE hThreadIOCP;
	HANDLE hArr[2];
	JOBOBJECT_ASSOCIATE_COMPLETION_PORT jobPort;
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION extLimit = { 0 };

	CREATE_JOB_AND_GET_PROC(hJob, hProc);
	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	jobPort.CompletionKey = (PVOID)COMPKEY_JOBOBJECT;
	jobPort.CompletionPort = hIOCP;
	hArr[0] = hJob;
	hArr[1] = hIOCP;


	// previous leak is 16M, this time the limit is 16M, so total is 32M
	printf("Job cannot allocate more than 32M memory.\n");
	extLimit.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_JOB_MEMORY;
	extLimit.JobMemoryLimit = 1024 * 1024 * 32;
	SetInformationJobObject(hJob, JobObjectAssociateCompletionPortInformation, &jobPort, sizeof(jobPort));
	SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &extLimit, sizeof(extLimit));
	AssignProcessToJobObject(hJob, hProc);

	hThreadIOCP = CreateThread(NULL, 0, JobNotify, hArr, 0, 0);

	// mem will leak, so next time the mem limit should plus this 32M
	PROCESS_MEMORY_COUNTERS pmc = { 0 };
	GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc));
	while (1) {
		void* p = malloc(65536);
		if (!p) {
			printf("Reach commit size!!\n");
			break;
		}

		GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc));
		printf("Working set is: %d\n", pmc.PagefileUsage / 1024);
		ZeroMemory(&pmc, sizeof(pmc));
		Sleep(10);
	}

	// remove mem limit
	extLimit.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_JOB_MEMORY;
	extLimit.JobMemoryLimit = 0;
	SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &extLimit, sizeof(extLimit));

	PostQueuedCompletionStatus(hIOCP, 0, COMPKEY_STATUS, NULL);
	PostQueuedCompletionStatus(hIOCP, 0, COMPKEY_TERMINATE, NULL);
	WaitForSingleObject(hThreadIOCP, INFINITE);

	CloseHandle(hIOCP);
	CloseHandle(hThreadIOCP);
	CloseHandle(hJob);
	CloseHandle(hProc);
}

void TestBreakAwayFromJob() {
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION extLimit = { 0 };
	LARGE_INTEGER li;
	HANDLE hJob;
	HANDLE hProc;
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi = { 0 };
	BOOL ret;
	CREATE_JOB_AND_GET_PROC(hJob, hProc);

	extLimit.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_BREAKAWAY_OK;
	ret = SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &extLimit, sizeof(extLimit));
	AssignProcessToJobObject(hJob, hProc);

	printf("Create process without break from job\n");
	CreateProcess(L"C:\\Windows\\notepad.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	//TerminateJobObject(hJob, 0);
	TerminateProcess(pi.hProcess, 0);
	CloseHandle(pi.hProcess);

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);
	printf("Create process break from job, terminate job will not terminate the subprocess\n");
	CreateProcess(L"C:\\Windows\\notepad.exe", NULL, NULL, NULL, FALSE, CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &si, &pi);
	TerminateJobObject(hJob, 0);

	CloseHandle(hJob);
	CloseHandle(hProc);
}
