#include "Tools.h"
#include <TlHelp32.h>

#pragma comment(lib, "ntdll")

#define NT_SUCCESS(status) (status >= 0)
#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)

DWORD GetProcessIDByName(LPCWSTR pName) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hSnapshot) {
		return NULL;
	}

	PROCESSENTRY32 pe = { sizeof(pe) };
	for (BOOL ret = Process32First(hSnapshot, &pe); ret; ret = Process32Next(hSnapshot, &pe)) {
		if (lstrcmpW(pe.szExeFile, pName) == 0) {
			CloseHandle(hSnapshot);
			return pe.th32ProcessID;
		}
	}

	CloseHandle(hSnapshot);
	return 0;
}

BOOL GetProcessElevation(TOKEN_ELEVATION_TYPE* pElevationType, BOOL* pIsAdmin) {

    HANDLE hToken = NULL;
    DWORD dwSize;
    BOOL bResult = FALSE;

    // Get current process token
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return(FALSE);

    // Retrieve elevation type information 
    if (GetTokenInformation(hToken, TokenElevationType, pElevationType, sizeof(TOKEN_ELEVATION_TYPE), &dwSize)) {
        // Create the SID corresponding to the Administrators group
        byte adminSID[SECURITY_MAX_SID_SIZE];
        dwSize = sizeof(adminSID);
        CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, &adminSID, &dwSize);

        if (*pElevationType == TokenElevationTypeLimited) {
            // Get handle to linked token (will have one if we are lua)
            HANDLE hUnfilteredToken = NULL;
            GetTokenInformation(hToken, TokenLinkedToken, (VOID*)&hUnfilteredToken, sizeof(HANDLE), &dwSize);

            // Check if this original token contains admin SID
            if (CheckTokenMembership(hUnfilteredToken, &adminSID, pIsAdmin)) {
                bResult = TRUE;
            }

            // Don't forget to close the unfiltered token
            CloseHandle(hUnfilteredToken);
        }
        else {
            *pIsAdmin = IsUserAnAdmin();
            bResult = TRUE;
        }
    }

    // Don't forget to close the process token
    CloseHandle(hToken);

    return(bResult);
}