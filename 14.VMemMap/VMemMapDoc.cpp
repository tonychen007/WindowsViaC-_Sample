
#include "framework.h"

#ifndef SHARED_HANDLERS
#include "VMemMap.h"
#endif

#include "VMemMapDoc.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMy14VMemMapDoc

IMPLEMENT_DYNCREATE(CVMemMapDoc, CDocument)

BEGIN_MESSAGE_MAP(CVMemMapDoc, CDocument)
END_MESSAGE_MAP()


CVMemMapDoc::CVMemMapDoc() noexcept
{

}

CVMemMapDoc::~CVMemMapDoc()
{
}

BOOL CVMemMapDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	return TRUE;
}

void CVMemMapDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: 在此加入儲存程式碼
	}
	else
	{
		// TODO: 在此加入載入程式碼
	}
}

#ifdef _DEBUG
void CVMemMapDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CVMemMapDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CMy14VMemMapDoc 命令
