
#include "ErrorLookup.h"
#include "resource.h"


DWORD GetErrorText(HWND hwnd) {
    DWORD dwError = 0;
    TCHAR BUF[BUFSIZE] = { '\0' };
    TCHAR* pBuf;
    DWORD dwLen;
    BOOL isHex = FALSE;

    Edit_GetText(hwnd, BUF, BUFSIZE);
    dwLen = lstrlen(BUF);

    if (dwLen == 0)
        return -1;

    // tolower first
    pBuf = BUF;
    while (*pBuf != L'\0') {
        *pBuf = towlower(*pBuf);
        pBuf++;
    }

    pBuf = BUF;
    if (BUF[0] == L'0' && BUF[1] == L'x') {
        isHex = TRUE;
        pBuf += 2;
    }
    
    while (*pBuf != L'\0') {
        if (isHex) {
            if ((L'0' <= *pBuf && *pBuf <= L'9') || (L'a' <= *pBuf && *pBuf <= L'f')) {
                int c = (L'a' <= *pBuf && *pBuf <= L'f') ? *pBuf - 'a' + 10 : *pBuf - '0';
                dwError = dwError * 16 + c;
            }
            else {
                dwError = -1;
                break;
            }
        }
        else {
            if (L'0' <= *pBuf && *pBuf <= L'9') {
                int c = *pBuf - L'0';
                dwError = dwError * 10 + c;
            } else {
                dwError = -1;
                break;
            }
        }
        pBuf++;
    }

    return dwError;
}

VOID FormatMessage(HWND hwnd, BOOL isEnglish) {
    BOOL ret;
    DWORD dwError = 0;
    DWORD systemLocale = isEnglish ? MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US) : MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    HLOCAL hlocal = NULL;
    DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM
        | FORMAT_MESSAGE_IGNORE_INSERTS
        | FORMAT_MESSAGE_ALLOCATE_BUFFER;
    
    dwError = GetErrorText(GetDlgItem(hwnd, IDC_ERRCODE));
    if (dwError == -1) {
        SetDlgItemText(hwnd, IDC_ERRTEXT, TEXT("錯誤的資訊代碼"));
        return;
    }
    
    ret = FormatMessage(dwFlags, NULL, dwError, systemLocale, (LPTSTR)&hlocal, 0, NULL);
    if (ret && hlocal) {
        SetDlgItemText(hwnd, IDC_ERRTEXT, (PCTSTR)LocalLock(hlocal));
        LocalFree(hlocal);
    }
    else {
        SetDlgItemText(hwnd, IDC_ERRTEXT, TEXT("找不到此錯誤的資訊！"));
    }
}

BOOL IsCheckBoxChecked(HWND hwndCtl) {
    int ret = Button_GetCheck(hwndCtl);
    if (ret == BST_CHECKED) {
        return TRUE;
    }
    
    return FALSE;
}

LOGFONT GetLOGFONT(HWND hwnd) {
    LOGFONT lf;
    HFONT hf = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
    GetObject(hf, sizeof(LOGFONT), &lf);

    return lf;
}

int GetFontSize(HWND hwnd) {
    LOGFONT lf = GetLOGFONT(hwnd);
    HDC hdc = GetDC(hwnd);
    int fz = -lf.lfHeight * 72.0 / GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(hwnd, hdc);

    return fz;
}

VOID ChangeFontSize(HWND hwnd) {
    int fz = SendMessage(GetDlgItem(hwnd, IDC_FONTSIZE_SPIN), UDM_GETPOS32, 0, 0);

    LOGFONT lf = GetLOGFONT(hwnd);
    HDC hdc = GetDC(hwnd);
    lf.lfHeight = -fz * GetDeviceCaps(GetDC(hwnd), LOGPIXELSY) / 72;
    ReleaseDC(hwnd, hdc);
    
    HFONT hf = CreateFontIndirect(&lf);
    
    // need to get all the controls
    EnumChildWindows(hwnd, EnumChildProc, (LPARAM)hf);
}

BOOL CALLBACK EnumChildProc(HWND hwndChild, LPARAM lParam) {
    SendMessage(hwndChild, WM_SETFONT, (WPARAM)lParam, 1);
    return TRUE;
}

VOID OnUpdateSpinBox(HWND hSpin, HWND hEditBox, NMHDR* pnmhdr) {
    TCHAR BUF[BUFSIZE] = { '\0' };
    LPNMUPDOWN lpnmud = (LPNMUPDOWN)pnmhdr;

    int pos = SendMessage(hSpin, UDM_GETPOS32, 0, 0);
    pos += lpnmud->iDelta;
    if (pos < MIN_SPIN_VAL || pos > MAX_SPIN_VAL)
        return;

    SendMessage(pnmhdr->hwndFrom, UDM_SETPOS32, 0, pos);
    _itow_s(pos, BUF, 10);
    Edit_SetText(hEditBox, BUF);
}

BOOL DlgOnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
    TCHAR BUF[BUFSIZE] = { '\0' };
    chSETDLGICONS(hwnd, IDI_ICONERR);

    Edit_LimitText(GetDlgItem(hwnd,IDC_ERRCODE), MAX_TEXT_LEN);
    SetDlgItemText(hwnd,IDC_ERRCODE, L"0");

    // get dialog font
    int fontSize = GetFontSize(hwnd);
    _itow_s(fontSize, BUF, 10);
    Edit_LimitText(GetDlgItem(hwnd, IDC_FONTSIZE_BOX), MAX_FONT_LEN);
    SetDlgItemText(hwnd, IDC_FONTSIZE_BOX, BUF);
    ZeroMemory(BUF, BUFSIZE);

    // set spin range
    HWND hSpin = GetDlgItem(hwnd, IDC_FONTSIZE_SPIN);
    SendMessage(hSpin, UDM_SETRANGE32, MIN_SPIN_VAL, MAX_SPIN_VAL);
    SendMessage(hSpin, UDM_SETPOS32, 0, fontSize);

    /*
    // set picture
    HANDLE hBitMap = LoadImage(0, L"me.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    LONG hh = SendMessage(GetDlgItem(hwnd, IDC_PIC), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitMap);
    InvalidateRect(GetDlgItem(hwnd, IDC_PIC), NULL, FALSE);
    */

    return TRUE;
}

VOID DlgOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
    switch (id) {

    case IDCANCEL:
        EndDialog(hwnd, id);
        break;

    case IDC_GO: {
            HWND hChkEng = GetDlgItem(hwnd, IDC_CHKENGLISH);
            int ret = IsCheckBoxChecked(hChkEng);
            FormatMessage(hwnd, ret);
            break;
        }
    }

    switch (codeNotify) {
    case BN_CLICKED:
        switch (id) {
        case IDC_CHKENGLISH:
            IsCheckBoxChecked(hwndCtl) ? FormatMessage(hwnd, TRUE) : FormatMessage(hwnd);
            break;
        }
        break;
    }
}

LONG DlgOnNotify(HWND hwnd, int idFrom, NMHDR* pnmhdr) {

    switch (pnmhdr->code) {
    case UDN_DELTAPOS:
        switch (idFrom) {
        case IDC_FONTSIZE_SPIN:
            OnUpdateSpinBox(pnmhdr->hwndFrom, GetDlgItem(hwnd, IDC_FONTSIZE_BOX), pnmhdr);
            ChangeFontSize(hwnd);
            break;
        }
        break;
    }

    return TRUE;
}

INT_PTR WINAPI DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg) {
        chHANDLE_DLGMSG(hwnd, WM_INITDIALOG, DlgOnInitDialog);
        chHANDLE_DLGMSG(hwnd, WM_COMMAND, DlgOnCommand);
        chHANDLE_DLGMSG(hwnd, WM_NOTIFY, DlgOnNotify);
    }

    return FALSE;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
	DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_ERRLOOKUP), NULL, DlgProc, _ttoi(lpCmdLine));

    return 0;
}