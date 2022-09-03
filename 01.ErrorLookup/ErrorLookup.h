#pragma once

#include <WindowsX.h>
#include <tchar.h>
#include "CmnHdr.h"


CONST INT BUFSIZE = 256;
CONST INT MAX_TEXT_LEN = 10;
CONST INT MAX_FONT_LEN = 2;

CONST INT MIN_SPIN_VAL = 8;
CONST INT MAX_SPIN_VAL = 48;


DWORD GetErrorText(HWND hwnd);
VOID FormatMessage(HWND hwnd, BOOL isEnglish = FALSE);
BOOL IsCheckBoxChecked(HWND hwnd, int nIDDlgItem);

LOGFONT GetLOGFONT(HWND hwnd);
int GetFontSize(HWND hwnd);
VOID ChangeFontSize(HWND hwnd);
BOOL CALLBACK EnumChildProc(HWND hwndChild, LPARAM lParam);

VOID OnUpdateSpinBox(HWND hSpin, HWND hEditBox);

BOOL DlgOnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
VOID DlgOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
//VOID DlgOnNotify(HWND hwnd, int idFrom, NMHDR* pnmhdr);

INT_PTR WINAPI DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
