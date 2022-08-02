#pragma once

#include <Windows.h>

BOOL StringReverseW(PWSTR pWideCharStr, DWORD cchLength);
BOOL StringReverseA(PSTR pMultiByteStr, DWORD cchLength);

#if defined UNICODE
#define StringReverse StringReverseW
#else
#define StringReverse StringReverseA
#endif
