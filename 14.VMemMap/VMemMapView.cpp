
// 14.VMemMapView.cpp: CMy14VMemMapView 類別的實作
//

#include "framework.h"
// SHARED_HANDLERS 可以定義在實作預覽、縮圖和搜尋篩選條件處理常式的
// ATL 專案中，並允許與該專案共用文件程式碼。
#ifndef SHARED_HANDLERS
#include "VMemMap.h"
#endif

#include "VMemMapDoc.h"
#include "VMemMapView.h"
#include "CLeftView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMy14VMemMapView

IMPLEMENT_DYNCREATE(CVMemMapView, CListView)

BEGIN_MESSAGE_MAP(CVMemMapView, CListView)
END_MESSAGE_MAP()

// CMy14VMemMapView 建構/解構

CVMemMapView::CVMemMapView() noexcept
{
}

CVMemMapView::~CVMemMapView()
{
}

BOOL CVMemMapView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= LVS_REPORT;
	return CListView::PreCreateWindow(cs);
}

void CVMemMapView::OnInitialUpdate() {
	GetListCtrl().SetExtendedStyle(LVS_EX_GRIDLINES);

	RECT rect;
	TEXTMETRIC tm;
	LONG width;
	CDC* pDC = GetDC();
	GetTextMetrics(*pDC, &tm);

	GetWindowRect(&rect);
	width = rect.right - rect.left;

	LVCOLUMN lvcol1, lvcol2, lvcol3, lvcol4;
	lvcol1.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol2.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol3.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol4.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvcol1.fmt = LVCFMT_LEFT;
	lvcol2.fmt = LVCFMT_LEFT;
	lvcol3.fmt = LVCFMT_LEFT;
	lvcol4.fmt = LVCFMT_LEFT;
	lvcol1.iSubItem = 0;
	lvcol2.iSubItem = 1;
	lvcol3.iSubItem = 2;
	lvcol4.iSubItem = 3;

	lvcol1.pszText = L"MEM State";
	lvcol2.pszText = L"Region Size(KB)";
	lvcol3.pszText = L"Blocks";
	lvcol4.pszText = L"Module";
	lvcol1.cchTextMax = MAX_PATH;
	lvcol2.cchTextMax = MAX_PATH;
	lvcol3.cchTextMax = MAX_PATH;
	lvcol4.cchTextMax = MAX_PATH;
	lvcol1.cx = (lstrlenW(lvcol1.pszText) * 2) * tm.tmMaxCharWidth;
	lvcol2.cx = (lstrlenW(lvcol2.pszText) * 2) * tm.tmMaxCharWidth;
	lvcol3.cx = (lstrlenW(lvcol3.pszText) * 2) * tm.tmMaxCharWidth;
	lvcol4.cx = (width - lvcol3.cx - lstrlenW(lvcol4.pszText) + 1);

	CListCtrl& refCtrl = GetListCtrl();

	refCtrl.InsertColumn(0, &lvcol1);
	refCtrl.InsertColumn(1, &lvcol2);
	refCtrl.InsertColumn(2, &lvcol3);
	refCtrl.InsertColumn(3, &lvcol4);

	InitFont();
}

void CVMemMapView::ShowMemoryContent(std::wstring* str) {
	if (str == NULL)
		return;

	CListCtrl& refCtrl = GetListCtrl();

	refCtrl.DeleteAllItems();

	LVITEM item1,item2,item3,item4;
	item1.iItem = 0;
	item1.iSubItem = 0;
	item1.pszText = (LPWSTR)str[MEM_STAT].data();
	item1.mask = LVIF_TEXT;

	item2.iItem = 0;
	item2.iSubItem = 1;
	item2.pszText = (LPWSTR)str[MEM_REG_SIZE].data();
	item2.mask = LVIF_TEXT;

	item3.iItem = 0;
	item3.iSubItem = 2;
	item3.pszText = (LPWSTR)str[MEM_NUM_BLOCK].data();
	item3.mask = LVIF_TEXT;

	item4.iItem = 0;
	item4.iSubItem = 3;
	item4.pszText = (LPWSTR)str[MEM_MODULE].data();
	item4.mask = LVIF_TEXT;

	refCtrl.InsertItem(&item1);
	refCtrl.SetItem(&item2);
	refCtrl.SetItem(&item3);
	refCtrl.SetItem(&item4);
}


void CVMemMapView::InitFont() {
	HDC hdc = GetDC()->m_hDC;
	LOGFONT lf;
	HFONT hf;
	float fz;

	hf = (HFONT)SendMessage(WM_GETFONT, 0, 0);
	GetObject(hf, sizeof(LOGFONT), &lf);

	fz = -lf.lfHeight * 72 / GetDeviceCaps(hdc, LOGPIXELSY);
	fz *= 1.4f;
	lf.lfHeight = -fz * GetDeviceCaps(hdc, LOGPIXELSY) / 72;

	m_font.CreateFontIndirectW(&lf);
	SetFont(&m_font);
}

// CMy14VMemMapView 診斷

#ifdef _DEBUG
void CVMemMapView::AssertValid() const
{
	CListView::AssertValid();
}

void CVMemMapView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CVMemMapDoc* CVMemMapView::GetDocument() const // 內嵌非偵錯版本
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CVMemMapDoc)));
	return (CVMemMapDoc*)m_pDocument;
}
#endif //_DEBUG