// CLeftView.cpp: 實作檔案
//



#include "CLeftView.h"
#include "VMemMapDoc.h"
#include "VMemMapView.h"
#include "resource.h"
#include "MainFrm.h"

#include "VMQuery.h"
#include "../CommonFiles/Toolhelp.h"

#include <psapi.h>

using namespace std;

// CLeftView

class CMainFrame;

IMPLEMENT_DYNCREATE(CLeftView, CTreeView)

CLeftView::CLeftView()
{
}

CLeftView::~CLeftView()
{
}

BEGIN_MESSAGE_MAP(CLeftView, CTreeView)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CLeftView::OnNMCustomdraw)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, &CLeftView::OnTvnSelchanged)
END_MESSAGE_MAP()


BOOL CLeftView::PreCreateWindow(CREATESTRUCT& cs) {
	cs.style |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT;
	return CTreeView::PreCreateWindow(cs);
}

void CLeftView::OnInitialUpdate() {
	static CFont font;
	CTreeView::OnInitialUpdate();

	{
		HDC hdc = GetDC()->m_hDC;
		LOGFONT lf;
		HFONT hf;
		float fz;

		hf = (HFONT)SendMessage(WM_GETFONT, 0, 0);
		GetObject(hf, sizeof(LOGFONT), &lf);

		fz = -lf.lfHeight * 72 / GetDeviceCaps(hdc, LOGPIXELSY);
		fz *= 1.5f;
		lf.lfHeight = -fz * GetDeviceCaps(hdc, LOGPIXELSY) / 72;
		lstrcpy(lf.lfFaceName, L"@Fixedsys");
		font.CreateFontIndirect(&lf);
	}

	SetFont(&font);
	PopulateTree();
}

CVMemMapDoc* CLeftView::GetDocument() {
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CVMemMapDoc)));
	return (CVMemMapDoc*)m_pDocument;
}

static const char HexArr[17] = "0123456789ABCDEF";

const wchar_t* HexToString(int64_t val, std::wstring &str) {
	// 8bytes + '0' + 'x' + '\0';
	const int len = 19;
	int idx = 0;

	while (val > 0) {
		wchar_t c = HexArr[val & 15];
		// 1111, 4 bits
		val >>= 4;
		str += c;
		idx++;
	}

	for (; idx < len - 3;) {
		str += '0';
		idx++;
	}
	str += 'x';
	str += '0';
	str += L'\0';

	_wcsrev(const_cast<wchar_t*>(str.data()));
	return str.data();
}

void CLeftView::PopulateTree() {
	CTreeCtrl& tc = GetTreeCtrl();

	GetVirtualMemoryInfo(tc);

	if (0) {
		CBitmap bmClassical, bmJazz, bmRock;

		// Create image list
		m_ImageList.Create(48, 48, ILC_COLORDDB | ILC_MASK, 0, 1);

		// load images and populate image list -GGH
		bmClassical.LoadBitmap(IDB_CLASSICAL);
		bmJazz.LoadBitmap(IDB_JAZZ);
		bmRock.LoadBitmap(IDB_ROCK);

		m_ImageList.Add(&bmClassical, RGB(192, 192, 192));
		m_ImageList.Add(&bmJazz, RGB(192, 192, 192));
		m_ImageList.Add(&bmRock, RGB(192, 192, 192));

		tc.SetImageList(&m_ImageList, TVSIL_NORMAL);

		HTREEITEM hItem;
		hItem = tc.InsertItem(L"Classical", 0, 0);
		if (hItem)
		{
			tc.InsertItem(L"J. S. Bach", 0, 0, hItem);
			tc.InsertItem(L"W. A. Mozart", 0, 0, hItem);
			tc.InsertItem(L"F. Chopin", 0, 0, hItem);
		}
		// expand
		tc.Expand(hItem, TVE_EXPAND);

		hItem = tc.InsertItem(L"Jazz", 1, 1);
		if (hItem)
		{
			tc.InsertItem(L"Johnny Hodges", 1, 1, hItem);
			tc.InsertItem(L"Charlie Parker", 1, 1, hItem);
			tc.InsertItem(L"John Coltrane", 1, 1, hItem);
		}
		// expand
		tc.Expand(hItem, TVE_EXPAND);

		hItem = tc.InsertItem(L"Rock", 2, 2);
		if (hItem)
		{
			tc.InsertItem(L"Genesis", 2, 2, hItem);
			tc.InsertItem(L"Jethro Tull", 2, 2, hItem);
			tc.InsertItem(L"Yes", 2, 2, hItem);
		}
		// expand
		tc.Expand(hItem, TVE_EXPAND);
	}
}

PCTSTR GetMemStorageText(DWORD dwStorage) {
	PCTSTR p = TEXT("Unknown");
	switch (dwStorage) {
	case MEM_FREE:    p = TEXT("Free   "); break;
	case MEM_RESERVE: p = TEXT("Reserve"); break;
	case MEM_IMAGE:   p = TEXT("Image  "); break;
	case MEM_MAPPED:  p = TEXT("Mapped "); break;
	case MEM_PRIVATE: p = TEXT("Private"); break;
	}
	return(p);
}

void CLeftView::GetVirtualMemoryInfo(CTreeCtrl& tc) {
	VMQUERY vmem;
	PVOID pvAddress = NULL;
	BOOL ret = TRUE;
	CToolhelp tool32;
	TCHAR pszFilename[MAX_PATH];
	DWORD idx = 0;

	DWORD pid = GetCurrentProcessId();
	tool32.CreateSnapshot(TH32CS_SNAPALL, pid);
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);

	while (ret) {
		ret = VMQuery(hProcess, pvAddress, &vmem);
		wstring str;

		const wchar_t* p = HexToString((int64_t)pvAddress, str);
		HTREEITEM hItem;
		hItem = tc.InsertItem(p, idx, idx);

		wstring* detail = m_memInfo[p];
		detail[MEM_STAT] = GetMemStorageText(vmem.dwRgnStorage);
		wsprintf(pszFilename, L"%I64d", vmem.RgnSize);
		detail[MEM_REG_SIZE] = pszFilename;
		wsprintf(pszFilename, L"%I64d", vmem.dwRgnBlocks);
		detail[MEM_NUM_BLOCK] = pszFilename;

		if ((vmem.dwRgnStorage != MEM_FREE) && (vmem.pvRgnBaseAddress != NULL)) {
			MODULEENTRY32 me = { sizeof(me) };
			if (tool32.ModuleFind(vmem.pvRgnBaseAddress, &me)) {
				wsprintf(pszFilename, L"%s", me.szExePath);
				detail[MEM_MODULE] = pszFilename;
			}
			else {
				DWORD len = GetMappedFileName(hProcess, vmem.pvRgnBaseAddress, pszFilename, MAX_PATH);
				if (len != 0) {
					detail[MEM_MODULE].append(pszFilename);
				}
			}
		}

		if (vmem.bRgnIsAStack) {
			wsprintf(pszFilename, L"%ws", L"Thread Stack");
			detail[MEM_MODULE] = pszFilename;
		}

		for (int i = 0; ret && (i < vmem.dwRgnBlocks); i++) {
			std::wstring blkstr;

			const wchar_t* p1 = HexToString((int64_t)pvAddress, blkstr);
			tc.InsertItem(p1, idx, idx, hItem);
			//tc.Expand(hItem, TVE_EXPAND);

			wstring* detailBlk = m_memInfo[p1];
			detailBlk[MEM_STAT] = GetMemStorageText(vmem.dwRgnStorage);
			wsprintf(pszFilename, L"%I64d", vmem.BlkSize);
			detailBlk[MEM_REG_SIZE] = pszFilename;
			blkstr.clear();

			pvAddress = ((PBYTE)pvAddress + vmem.BlkSize);
			if (i < vmem.dwRgnBlocks - 1) {
				// Don't query the memory info after the last block.
				ret = VMQuery(hProcess, pvAddress, &vmem);
			}
		}

		str.clear();
		tc.Expand(hItem, TVE_EXPAND);
		pvAddress = ((PBYTE)vmem.pvRgnBaseAddress + vmem.RgnSize);
		idx++;
	}
}

// CLeftView 診斷

#ifdef _DEBUG
void CLeftView::AssertValid() const
{
	CTreeView::AssertValid();
}

#ifndef _WIN32_WCE
void CLeftView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}
#endif
#endif //_DEBUG


void CLeftView::OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult) {
	LPNMTVCUSTOMDRAW pNMCD = reinterpret_cast<LPNMTVCUSTOMDRAW>(pNMHDR);

	CRect rcItem;
	HBRUSH hbrBkgnd = 0;
	CTreeCtrl& tc = GetTreeCtrl();
	HDC hdc = pNMCD->nmcd.hdc;
	COLORREF oldColor = RGB(0,0,0);
	CString str;

	switch (pNMCD->nmcd.dwDrawStage) {
		case CDDS_PREPAINT:
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;

		case CDDS_ITEMPREPAINT:
			switch (pNMCD->iLevel) {
			case 0:
				if (pNMCD->nmcd.uItemState == (CDIS_FOCUS | CDIS_SELECTED)) // selected
					oldColor = pNMCD->clrText = RGB(255, 255, 255);
				else
					oldColor = pNMCD->clrText = RGB(0, 0, 255);
				break;
			}

			// show how to custom draw rect
			::SetTextColor(hdc, RGB(40, 125, 255));
			tc.GetItemRect((HTREEITEM)pNMCD->nmcd.dwItemSpec, &rcItem, TRUE);
			::DrawFocusRect(hdc, &rcItem);
			::SetTextColor(hdc, oldColor);

			*pResult = CDRF_NOTIFYPOSTPAINT;
			break;

		case CDDS_ITEMPOSTPAINT:
			/*
			CTreeCtrl& tc = GetTreeCtrl();
			tc.GetItemRect((HTREEITEM)pNMCD->nmcd.dwItemSpec, &rcItem, TRUE);
			hbrBkgnd = CreateSolidBrush(RGB(120, 12, 45));
			::FillRect(hdc, &rcItem, hbrBkgnd);
			DeleteObject(hbrBkgnd);
			*/
			break;
	}
}

void CLeftView::OnTvnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) {
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 在此加入控制項告知處理常式程式碼

	CTreeCtrl& tc = GetTreeCtrl();
	CString str = tc.GetItemText(pNMTreeView->itemNew.hItem);

	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	CWnd* pWind = pFrame->m_wndSplitter.GetPane(0, 1);
	CVMemMapView* pRightView = DYNAMIC_DOWNCAST(CVMemMapView, pWind);

	pRightView->ShowMemoryContent(m_memInfo[str.GetBuffer()]);

	*pResult = 0;
}