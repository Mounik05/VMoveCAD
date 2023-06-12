// VMoveCAD_MFC_GUI.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "VMoveCAD_MFC_GUI.h"
#include "VMoveCAD_MFC_GUIDlg.h"
#include <vector>
#include <sstream>
#include "datakit.h"
#include "../version.h"

#include "../vcUtils.h"
#include "../vcLicense.h"

#include "VctCaxScene.h"
#include "VctException.h"
using namespace Vct;
typedef Cax::uint32_t uint32_t;

#include "SuiteVersion_V2.h"
char* g_sAppName = NULL;
extern char* g_sMSC_CEID;

bool g_bIgnoreTransparency = false;
std::vector<float *> FloatArrays;
std::vector<uint32_t *> IntArrays;
//float g_fTessTolerance = 0.05f;
DtkErrorStatus g_dtkErrorStatus = dtkNoError;
std::string g_sVer;

bool g_bMesh = true;
bool g_b2DElements = true;
bool g_bPointSet = false;
bool g_bCombinePartsInGroup = true;

#ifdef _DEBUG
#define new DEBUG_NEW 
#endif


// CVMoveCAD_MFC_GUIApp

BEGIN_MESSAGE_MAP(CVMoveCAD_MFC_GUIApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CVMoveCAD_MFC_GUIApp construction

CVMoveCAD_MFC_GUIApp::CVMoveCAD_MFC_GUIApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CVMoveCAD_MFC_GUIApp object

CVMoveCAD_MFC_GUIApp theApp;


// CVMoveCAD_MFC_GUIApp initialization

BOOL CVMoveCAD_MFC_GUIApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls; 
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use 
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	//AfxGetApp()->m_pszAppName = CString("VMoveCAD");
	CSuiteVersion sv;
	g_sAppName = sv.GetAppName("vmovecad");

	vcUtils::InitLog("VMoveCAD_Log.txt");
	
	WriteAppVersionIntoLogFile();
	
	WriteOSVersionIntoLogFile();

	WriteSystemInfo();
	
	if(!CheckOutLicense())
		return false;


	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	try
	{
		CVMoveCAD_MFC_GUIDlg dlg;
		m_pMainWnd = &dlg;
		INT_PTR nResponse = dlg.DoModal();
		if (nResponse == IDOK)
		{
			// TODO: Place code here to handle when the dialog is
			//  dismissed with OK
		}
		else if (nResponse == IDCANCEL)
		{
			// TODO: Place code here to handle when the dialog is
			//  dismissed with Cancel
		}
	}
	catch(std::exception& ex) 
	{
		std::stringstream ss;
		ss.str("");
		ss<<"Exception: "<<ex.what();
		vcUtils::LogMsg(ss.str());
		AfxMessageBox(ss.str().c_str());
		//return FALSE;
	}
	catch(...)
	{
		//AfxMessageBox("exception");
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

int CVMoveCAD_MFC_GUIApp::ExitInstance()
{
	// TODO: Add your specialized code here and/or call the base class
	vcLicense::CheckInLicense(); 
	vcUtils::LogMsg("VMoveCAD::OnExit : Exit");
	return CWinApp::ExitInstance();
}

bool CVMoveCAD_MFC_GUIApp::CheckOutLicense()
{
	return vcLicense::CheckOutLicense();
}

void CVMoveCAD_MFC_GUIApp::WriteAppVersionIntoLogFile()
{
	std::stringstream ss;
	ss.str("");
	ss<<"VMoveCAD: Version: "<<g_sVersion;
	vcUtils::LogMsg(ss.str());
    ss.str("");
	g_sVer = g_sVersion;

#ifdef _WIN64
	vcUtils::LogMsg("VMoveCAD: Win64 Application");
#else
	vcUtils::LogMsg("VMoveCAD: Win32 Application");
#endif

	ss.str("");
	ss<<"VMoveCAD: DTK Version: "<<g_sDatakitVersion;
	vcUtils::LogMsg(ss.str());
}

void CVMoveCAD_MFC_GUIApp::WriteOSVersionIntoLogFile()
{
	wchar_t *ver = new wchar_t[1024];
	vcUtils::GetWindowsVersionName(ver,1024);
	std::stringstream ss;
	ss.str("");
	ss<<"VMoveCAD: OS Version: "<<vcUtils::ToNarrow(ver);
	vcUtils::LogMsg(ss.str());
    ss.str("");
}
void CVMoveCAD_MFC_GUIApp::WriteSystemInfo()
{
	MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    GlobalMemoryStatusEx (&statex);

	std::stringstream ss;
	ss<<"VMoveCAD: System Memory size: "<<statex.ullTotalPhys/(1024*1024)<<" MB";
	vcUtils::LogMsg(ss.str());
}