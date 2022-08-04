
// ProcessGUI.h: PROJECT_NAME 應用程式的主要標頭檔
//

#pragma once

#include "resource.h"		// 主要符號


// CProcessGUIApp:
// 查看 ProcessGUI.cpp 以了解此類別的實作
//

class CProcessGUIApp : public CWinApp
{
public:
	CProcessGUIApp();

// 覆寫
public:
	virtual BOOL InitInstance();

// 程式碼實作

	DECLARE_MESSAGE_MAP()
};

extern CProcessGUIApp theApp;
