// VMoveCAD_MFC_GUI.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CVMoveCAD_MFC_GUIApp:
// See VMoveCAD_MFC_GUI.cpp for the implementation of this class
//

class CVMoveCAD_MFC_GUIApp : public CWinApp
{
public:
	CVMoveCAD_MFC_GUIApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();

	bool CheckOutLicense();
	void WriteAppVersionIntoLogFile();
	void WriteOSVersionIntoLogFile();
	void WriteSystemInfo();
};

extern CVMoveCAD_MFC_GUIApp theApp;