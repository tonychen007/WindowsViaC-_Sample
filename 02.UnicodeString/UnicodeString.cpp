
#include <stdlib.h>
#include <locale.h>
#include <tchar.h>
#include <Shlwapi.h>
#include <crtdbg.h>
#include <strsafe.h>

#include "dll_test.h"

VOID MyHanlderForString(wchar_t const* expr, wchar_t const* func, wchar_t const* file, unsigned int line, uintptr_t t) {
	wprintf(L"%s %s %s %d\n\n", expr, func, file, line);
}

VOID TestCCopyString() {
	errno_t err;

	char buf[5] = { '\0' };
	err = strcpy_s(buf, 5, "123456");

	// szBuffer fail, the first char is 00 00, and the remaining are fe fe fe ...
	TCHAR szBuffer[10] = { '\0' };
	err = wcscpy_s(szBuffer, _countof(szBuffer), L"0123456789");
}

VOID TestWindowsString() {
	HRESULT hr;
	TCHAR* szBufferEnd;

	// '\0' is not counted, and it checks how many 00 00 at the ends of the chars
	size_t rem;
	TCHAR szBuffer[10] = { '\0' };

	// the remaining bytes will be filled with 78 78
	// szBufferEnd pointer to the '\0', so szBufferEnd - szBuffer is the length not include '\0'
	hr = StringCchCopyEx(szBuffer, _countof(szBuffer), L"01234567", &szBufferEnd, &rem, STRSAFE_FILL_BYTE(120));
	
	// szBuffer fail, but the char is not fe fe fe
	hr = StringCchCopy(szBuffer, _countof(szBuffer), L"0123456789");

	StrFormatKBSize(1024, szBuffer, 10);
	wprintf(L"Using StrFormatKBSize: %s\n", szBuffer);
	StrFormatByteSize64(4096LL * 1024LL * 1024LL * 1024LL * 1024LL, szBuffer, 10);
	wprintf(L"Using StrFormatByteSize64: %s\n", szBuffer);
}

VOID TestWindowsCompareString() {

	int ret1;
	int ret2;

	// Windows CompareString
	(void)CSTR_LESS_THAN;		// 1
	(void)CSTR_EQUAL;			// 2
	(void)CSTR_GREATER_THAN;	// 3

	LCID langTW = GetThreadLocale();
	LCID langEN = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);

	// sort bt stroke
	LPCWSTR pstr1 = L"三們";
	LPCWSTR pstr2 = L"二們";
	ret1 = CompareString(langTW, 0, pstr1, -1, pstr2, -1);

	pstr1 = L"appassionate";
	pstr2 = L"extraordinary";
	ret2 = CompareString(langEN, 0, pstr1, -1, pstr2, -1);

	(void)0;
}

VOID TestConvertString() {
	PCSTR str1 = "I am a Student";
	PCWSTR str2 = L"國藝樂";
	LPCCH defCh = "#";
	BOOL isAllConvted = TRUE;
	size_t len;

	len = MultiByteToWideChar(CP_UTF8, 0, str1, -1, NULL, 0);
	LPWSTR wstr = new TCHAR[len + 1];
	ZeroMemory(wstr, 0);
	MultiByteToWideChar(CP_UTF8, 0, str1, -1, wstr, len);
	wprintf(L"Wide Char is : %s\n", wstr);

	// isAllConvted should be false
	len = WideCharToMultiByte(CP_UTF8, 0, str2, -1, NULL, 0, defCh, &isAllConvted);
	PSTR mstr = new CHAR[len + 1];
	ZeroMemory(mstr, 0);
	isAllConvted = TRUE;	
	WideCharToMultiByte(CP_ACP, 0, str2, -1, mstr, len, defCh, &isAllConvted);
	printf("Multi Char is : %s\n", mstr);
}

VOID TestDLLUnicodeAndANSI() {
	TCHAR str1[10] = { '\0' };
	CCHAR str2[10] = { '\0' };
	wcscpy_s(str1, _countof(str1), L"123456");
	strcpy_s(str2, _countof(str2), "123456");

	StringReverse(str1, 3);
	wprintf(L"Call from DLL StringReverseW: %s\n", str1);
	StringReverseA(str2, 3);
	printf("Call from DLL StringReverseA: %s\n", str2);

}

int main() {
	_set_invalid_parameter_handler(MyHanlderForString);
	_CrtSetReportMode(_CRT_ASSERT, 0);

	TestCCopyString();
	TestWindowsString();
	TestWindowsCompareString();
	TestConvertString();
	TestDLLUnicodeAndANSI();

	return 0;

}
