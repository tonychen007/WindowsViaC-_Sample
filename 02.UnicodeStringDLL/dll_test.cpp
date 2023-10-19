#include "dll_test.h"

BOOL StringReverseW(PWSTR pWideCharStr, DWORD cchLength) {
	BOOL ret = FALSE;

	PWSTR pEndOfStr = pWideCharStr + wcsnlen_s(pWideCharStr, cchLength) - 1;
	wchar_t cCharT;

	while (pWideCharStr < pEndOfStr) {
		cCharT = *pWideCharStr;
		*pWideCharStr = *pEndOfStr;
		*pEndOfStr = cCharT;
		
		pWideCharStr++;
		pEndOfStr--;
	}
	
	return TRUE;
}

BOOL StringReverseA(PSTR pMultiByteStr, DWORD cchLength) {
	PWSTR pWideCharStr;
	int nLenOfWideCharStr;
	BOOL fOk = FALSE;
	
	nLenOfWideCharStr = MultiByteToWideChar(CP_ACP, 0, pMultiByteStr, cchLength, NULL, 0) + 1;
	pWideCharStr =  new TCHAR[nLenOfWideCharStr * sizeof(wchar_t)];
	memset(pWideCharStr, 0, nLenOfWideCharStr * sizeof(wchar_t));
	if (pWideCharStr == NULL)
		return fOk;

	MultiByteToWideChar(CP_ACP, 0, pMultiByteStr, cchLength, pWideCharStr, nLenOfWideCharStr);

	fOk = StringReverseW(pWideCharStr, cchLength);
	if (fOk) {
		WideCharToMultiByte(CP_ACP, 0, pWideCharStr, cchLength, pMultiByteStr, (int)strlen(pMultiByteStr), NULL, NULL);
	}
	delete[] pWideCharStr;
	pWideCharStr = 0;

	return fOk;
}