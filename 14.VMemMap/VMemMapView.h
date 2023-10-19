
// 14.VMemMapView.h: CMy14VMemMapView 類別的介面
//

#pragma once

#include <string>

class CVMemMapView : public CListView
{
protected: // 僅從序列化建立
	CVMemMapView() noexcept;
	DECLARE_DYNCREATE(CVMemMapView)

// 屬性
public:
	CVMemMapDoc* GetDocument() const;

// 作業
public:

// 覆寫
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	void ShowMemoryContent(std::wstring* str);
protected:
	void InitFont();
// 程式碼實作
public:
	virtual ~CVMemMapView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
protected:
	DECLARE_MESSAGE_MAP()
private:
	CFont m_font;
public:
};
