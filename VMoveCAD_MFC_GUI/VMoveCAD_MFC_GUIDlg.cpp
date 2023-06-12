// VMoveCAD_MFC_GUIDlg.cpp : implementation file
//

#include "stdafx.h"
#include "VMoveCAD_MFC_GUI.h"
#include "VMoveCAD_MFC_GUIDlg.h"
#include "folder_dialog.h"
#include "../vcUtils.h"
#include "../vcCadTranslator.h"
#include "datakit.h"
#include <string>
#include <sstream>
#include <vector>
#include "../vcCad2Cax.h"
#include "htmlhelp.h"
#include "../version.h"
#include "../StopWatch.h"
#include "../vcExtractMetadata.h"

extern char g_sAppTitle[256];
extern void ClearMemory();

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
 
typedef Cax::uint32_t uint32_t;

extern double g_fTessTolerance;
extern std::vector<float *> FloatArrays;
extern std::vector<uint32_t *> IntArrays;
extern DtkErrorStatus g_dtkErrorStatus;
extern std::string sOutputFormat;
extern std::string sSupportedOutputFormat;
extern int g_iInputFilesCount = 0;
extern int g_iOutputFilesCount = 0;
extern std::string g_sVer;
extern bool g_bIgnoreTransparency;
extern int g_iUnit;
extern bool g_bUserUnits;
extern bool g_bDetailedLog;
extern bool g_bDataLoss;
extern bool g_bMesh,g_b2DElements,g_bPointSet, g_bCombinePartsInGroup;;

CString GetFileName(const CString &str)
{
	CString instr( str );
	CString file;
	int found = instr.ReverseFind( _TCHAR( '\\' ) );
	if( found != -1 )
		file = instr.Right( str.GetLength( ) - ( found + 1 ) );

	return file;
}

CString GetFileNameWithoutExt(CString &str)
{
	CString sTempStr = GetFileName(str);
	CString sOnlyFileName = sTempStr;
	CString filename;
	int found = sTempStr.ReverseFind( _TCHAR( '.' ) );
	if( found != -1 )
		filename=sTempStr.Left( found );
	else
		filename = sOnlyFileName;

	return filename;
}

CString GetFileExtn(CString inputPath)
{
	CString ext;//=inputPath;
	int index=inputPath.ReverseFind('.');
	if(index!=-1)
		ext=inputPath.Right(inputPath.GetLength()-index-1);

	return ext;
}

CString GetDirectoryName(const CString &str)
{
	CString instr(str);
	CString directory;
	int index = instr.ReverseFind(_TCHAR( '\\' ));
	if( index != -1 )
		directory = instr.Left(index);

	return directory;
}

bool GetFolder(std::string& folderpath, const char* szCaption = NULL, HWND hOwner = NULL)
{
	bool retVal = false;

	// The BROWSEINFO struct tells the shell 
	// how it should display the dialog.
	BROWSEINFO bi;
	memset(&bi, 0, sizeof(bi));

	bi.ulFlags   = BIF_USENEWUI;
	bi.hwndOwner = hOwner;
	bi.lpszTitle = szCaption;

	// must call this if using BIF_USENEWUI
	::OleInitialize(NULL);

	// Show the dialog and get the itemIDList for the selected folder.
	LPITEMIDLIST pIDL = ::SHBrowseForFolder(&bi);

	if(pIDL != NULL)
	{
		// Create a buffer to store the path, then get the path.
		char buffer[_MAX_PATH] = {'\0'};
		if(::SHGetPathFromIDList(pIDL, buffer) != 0)
		{
			// Set the string value.
			folderpath = buffer;
			retVal = true;
		}		

		// free the item id list
		CoTaskMemFree(pIDL);
	}

	::OleUninitialize();

	return retVal;
}


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

#include "SuiteVersion_V2.h"
extern char* g_sAppName; 
extern char* g_sMSC_CEID;
BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CSuiteVersion suiteVersion;
	
	char* suite_version_str;
	suite_version_str = suiteVersion.GetVersion();

	// TODO:  Add extra initialization here
	CString sTitle;
	//sTitle.Format("About %s %s",g_sAppTitle, suite_version_str.c_str());
	sTitle.Format("About %s", g_sAppName);
	this->SetWindowTextA(sTitle);
 

	CString sVersion;
#if defined(_WIN64)
	//sVersion.Format("VCollab %s\n\nVMoveCAD (64-Bit) : Build # %s", suite_version_str.c_str(),g_sVer.c_str());
	sVersion.Format("%s %s \n\nBuild # %s  (64-bit)", g_sAppName,suite_version_str, g_sVer.c_str());
#else
	sVersion.Format("VMoveCAD %s (32-Bit)",g_sVer.c_str());
#endif
	((CStatic*)GetDlgItem(IDC_VERSION_STATIC))->SetWindowText(sVersion);

	std::stringstream ss;
	ss << g_sCopyright;
	if (g_sMSC_CEID && strlen(g_sMSC_CEID))
		ss << "\n\nMSC Customer Entitlement ID : " << g_sMSC_CEID;

	
	((CStatic*)GetDlgItem(IDC_COPY_RIGHT_STATIC))->SetWindowText(ss.str().c_str());
	UpdateData(false);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
// CVMoveCAD_MFC_GUIDlg dialog




CVMoveCAD_MFC_GUIDlg::CVMoveCAD_MFC_GUIDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVMoveCAD_MFC_GUIDlg::IDD, pParent)
	, m_sInputFile(_T(""))
	, m_sOutputFile(_T(""))
	, m_fTessTolerance(0.0005)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVMoveCAD_MFC_GUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_INPUT_FILE_EDIT, m_sInputFile);
	DDX_Text(pDX, IDC_OUTPUT_FILE_EDIT, m_sOutputFile);
	DDX_Text(pDX, IDC_TESS_TOLERANCE_EDIT, m_fTessTolerance);
	DDX_Control(pDX, IDC_UNITS_COMBO, m_UnitsComboCtrl);
}

BEGIN_MESSAGE_MAP(CVMoveCAD_MFC_GUIDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_FILE_RADIO, &CVMoveCAD_MFC_GUIDlg::OnBnClickedFileRadio)
	ON_BN_CLICKED(IDC_DIRECTORY_RADIO, &CVMoveCAD_MFC_GUIDlg::OnBnClickedDirectoryRadio)
	ON_BN_CLICKED(IDC_INPUT_BUTTON, &CVMoveCAD_MFC_GUIDlg::OnBnClickedInputButton)
	ON_BN_CLICKED(IDC_OUTPUT_BUTTON, &CVMoveCAD_MFC_GUIDlg::OnBnClickedOutputButton)
	ON_BN_CLICKED(IDC_IGNORE_TRANS_CHECK, &CVMoveCAD_MFC_GUIDlg::OnBnClickedIgnoreTransCheck)
	ON_BN_CLICKED(IDOK, &CVMoveCAD_MFC_GUIDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_ABOUT_BUTTON, &CVMoveCAD_MFC_GUIDlg::OnBnClickedAboutButton)
	ON_BN_CLICKED(IDC_HELP_BUTTON, &CVMoveCAD_MFC_GUIDlg::OnBnClickedHelpButton)
	ON_BN_CLICKED(IDCANCEL, &CVMoveCAD_MFC_GUIDlg::OnBnClickedCancel)
	ON_CBN_SELCHANGE(IDC_OUTPUT_FORMAT_COMBO, &CVMoveCAD_MFC_GUIDlg::OnCbnSelchangeOutputFormatCombo)
	ON_CBN_SELCHANGE(IDC_UNITS_COMBO, &CVMoveCAD_MFC_GUIDlg::OnCbnSelchangeUnitsCombo)
	ON_BN_CLICKED(IDC_DETAILED_LOG_CHECK, &CVMoveCAD_MFC_GUIDlg::OnBnClickedDetailedLogCheck)
	ON_BN_CLICKED(IDC_MESH_CHECK, &CVMoveCAD_MFC_GUIDlg::OnBnClickedMeshCheck)
	ON_BN_CLICKED(IDC_2D_ELEMENTS_CHECK, &CVMoveCAD_MFC_GUIDlg::OnBnClicked2dElementsCheck)
	ON_BN_CLICKED(IDC_POINT_SET_CHECK, &CVMoveCAD_MFC_GUIDlg::OnBnClickedPointSetCheck)
	ON_BN_CLICKED(IDC_COMBINE_PARTS_CHECK, &CVMoveCAD_MFC_GUIDlg::OnBnClickedCombinePartsCheck)
END_MESSAGE_MAP()


// CVMoveCAD_MFC_GUIDlg message handlers

BOOL CVMoveCAD_MFC_GUIDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	this->SetWindowTextA(g_sAppTitle);
	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	m_hWaitCursor=LoadCursor(NULL,IDC_WAIT);
	m_hArrowCursor=LoadCursor(NULL,IDC_ARROW);

	SetWindowTextA(g_sAppName);

	((CButton*)GetDlgItem(IDC_FILE_RADIO))->SetCheck(true);
	((CComboBox*)GetDlgItem(IDC_OUTPUT_FORMAT_COMBO))->SetCurSel(0);

	((CButton*)GetDlgItem(IDC_INPUT_FILE_STATIC))->SetWindowText("Input CAD File");
	((CButton*)GetDlgItem(IDC_OUTPUT_FILE_STATIC))->SetWindowText("Output File");


	m_sOutputFormat = "cax";
#ifdef PART_WISE_TESSELLATION
	m_fTessTolerance = 0.0005f;
#else
	m_fTessTolerance = 0.05f;
#endif
	m_bFileMode = true;

	m_UnitsComboCtrl.ResetContent();
	m_UnitsComboCtrl.AddString("Millimeter");
	m_UnitsComboCtrl.AddString("Meter");
	m_UnitsComboCtrl.AddString("Inch");
	m_UnitsComboCtrl.AddString("Feet");
	m_UnitsComboCtrl.AddString("Centimeter");
	m_UnitsComboCtrl.SetCurSel(vcCad2Cax::MILLIMETER);

	((CButton*)GetDlgItem(IDC_DETAILED_LOG_CHECK))->SetCheck(false);
	g_bDetailedLog = false;
	
	((CButton*)GetDlgItem(IDC_MESH_CHECK))->SetCheck(true);
	((CButton*)GetDlgItem(IDC_2D_ELEMENTS_CHECK))->SetCheck(true);
	((CButton*)GetDlgItem(IDC_POINT_SET_CHECK))->SetCheck(false);
	((CButton*)GetDlgItem(IDC_COMBINE_PARTS_CHECK))->SetCheck(true);

	UpdateData(false);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CVMoveCAD_MFC_GUIDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CVMoveCAD_MFC_GUIDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CVMoveCAD_MFC_GUIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CVMoveCAD_MFC_GUIDlg::OnBnClickedFileRadio()
{
	m_UnitsComboCtrl.SetCurSel(0);
	m_bFileMode = true;
	((CButton*)GetDlgItem(IDC_INPUT_FILE_STATIC))->SetWindowText("Input CAD File");
	((CButton*)GetDlgItem(IDC_OUTPUT_FILE_STATIC))->SetWindowText("Output File");
}

void CVMoveCAD_MFC_GUIDlg::OnBnClickedDirectoryRadio()
{
	m_UnitsComboCtrl.SetCurSel(0);
	m_bFileMode = false;
	((CButton*)GetDlgItem(IDC_INPUT_FILE_STATIC))->SetWindowText("Input Directory");
	((CButton*)GetDlgItem(IDC_OUTPUT_FILE_STATIC))->SetWindowText("Output Directory");
}

#include "..\vcCad2Cax.h"
extern bool g_bInfoAboutCAD;

void CVMoveCAD_MFC_GUIDlg::OnBnClickedInputButton()
{
	CString inputPath;
	CString FileNameWithPath;
	
	UpdateData(true);
	if(((CButton*)GetDlgItem(IDC_DIRECTORY_RADIO))->GetCheck())
	{
#if defined(_WIN64)
		std::string sFolderName;
		if(GetFolder(sFolderName))
		{
			m_sInputFile = sFolderName.c_str();
			m_sOutputFile = m_sInputFile;
			UpdateData(false);
		}
#else
		CFolderDialog inputOpenDialog(&inputPath);
		if(inputOpenDialog.DoModal()==IDOK)
		{
			m_sInputFile = inputPath;
			m_sOutputFile = inputPath;
			UpdateData(false); 
		}//end	if(inputOpenDialog.DoModal()==IDOK)
#endif
	}
	else
	{
		//static char BASED_CODE szFilter[] ="All supported files(*.3dxml;*.CATProduct;*CATPart;*.model;*.prt;*.prt.*;*.asm;*.asm.*;*.step;*.stp;*.iges;*.igs;*.cgr;*.ipt;*.iam;*.prt;*.par;*.psm;*.asm;*.sldasm;*.sldprt;*.asat;*.sat;*.sab;*.xmt;*.x_t;*.x_b;*.xmt_txt;*.xmt_bin)|*.3dxml;*.CATProduct;*CATPart;*.model;*.prt;*.prt.*;*.asm;*.asm.*;*.step;*.stp;*.iges;*.igs;*.cgr;*.ipt;*.iam;*.prt;*.par;*.psm;*.asm;*.sldasm;*.sldprt;*.asat;*.sat;*.sab;*.xmt;*.x_t;*.x_b;*.xmt_txt;*.xmt_bin;*.rvt;*.rfa|Catia V6(*.3dxml)|*.3dxml|Catia V5 3D(*.CATProduct;*CATPart)|*.CATProduct;*CATPart|Catia V4 3D(*.model)|*.model|ProE 3D(*.prt;*.prt.*;*.asm;*.asm.*)|*.prt;*.prt.*;*.asm;*.asm.*|STEP(*.step;*.stp)|*.step;*.stp|IGES 3D(*.iges;*.igs)|*.iges;*.igs|CGR(*.cgr)|*.cgr|Inventor 3D(*.ipt;*.iam)|*.ipt;*.iam|UG NX 3D(*.prt)|*.prt|SolidWorks(*.sldasm;*.sldprt)|*.sldasm;*.sldprt|Solid Edge(*.par;*.psm;*.asm)|*.par;*.psm;*.asm|Acis(*.asat;*.sat;*.sab)|*.asat;*.sat;*.sab|Parasolid(*.xmt;*.x_t;*.x_b;*.xmt_txt;*.xmt_bin)|*.xmt;*.x_t;*.x_b;*.xmt_txt;*.xmt_bin|Revit(*.rvt;*.rfa)|*.rvt;*.rfa||";
		static char BASED_CODE szFilter[] = "All supported files(*.3dxml;*.CATProduct;*CATPart;*.model;*.prt;*.prt.*;*.asm;*.asm.*;*.step;*.stp;*.iges;*.igs;*.cgr;*.ipt;*.iam;*.prt;*.par;*.psm;*.asm;*.sldasm;*.sldprt;*.asat;*.sat;*.sab;*.xmt;*.x_t;*.x_b;*.xmt_txt;*.xmt_bin)|*.3dxml;*.CATProduct;*CATPart;*.model;*.prt;*.prt.*;*.asm;*.asm.*;*.step;*.stp;*.iges;*.igs;*.cgr;*.ipt;*.iam;*.prt;*.par;*.psm;*.asm;*.sldasm;*.sldprt;*.asat;*.sat;*.sab;*.xmt;*.x_t;*.x_b;*.xmt_txt;*.xmt_bin|Catia V6(*.3dxml)|*.3dxml|Catia V5 3D(*.CATProduct;*CATPart)|*.CATProduct;*CATPart|Catia V4 3D(*.model)|*.model|ProE 3D(*.prt;*.prt.*;*.asm;*.asm.*)|*.prt;*.prt.*;*.asm;*.asm.*|STEP(*.step;*.stp)|*.step;*.stp|IGES 3D(*.iges;*.igs)|*.iges;*.igs|CGR(*.cgr)|*.cgr|Inventor 3D(*.ipt;*.iam)|*.ipt;*.iam|UG NX 3D(*.prt)|*.prt|SolidWorks(*.sldasm;*.sldprt)|*.sldasm;*.sldprt|Solid Edge(*.par;*.psm;*.asm)|*.par;*.psm;*.asm|Acis(*.asat;*.sat;*.sab)|*.asat;*.sat;*.sab|Parasolid(*.xmt;*.x_t;*.x_b;*.xmt_txt;*.xmt_bin)|*.xmt;*.x_t;*.x_b;*.xmt_txt;*.xmt_bin||";

		CFileDialog file_dlg(TRUE, NULL, NULL, OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT,LPCTSTR(szFilter));
		if(file_dlg.DoModal() == IDOK)
		{
			m_sInputFile=file_dlg.GetPathName();
			int index = m_sInputFile.ReverseFind('.');
			if(index!=-1)
			{
				m_sOutputFile = m_sInputFile.Left(index+1);
				m_sOutputFile+=m_sOutputFormat;

				std::string sTmpDir = vcUtils::m_sVMoveCADTmpDir;
				std::string InputFile = (LPCTSTR)m_sInputFile;
				std::string OutputFile = (LPCTSTR)m_sOutputFile;
				bool bSuccess = true;
				g_bInfoAboutCAD = true;
				g_bUserUnits = false;
				m_UnitsComboCtrl.SetCurSel(0);
				::SetCursor(m_hWaitCursor);
				vcUtils::LogMsg("CVMoveCAD_MFC_GUIDlg::OnBnClickedInputButton : Reading the Units Information...");
				vcCad2Cax cad2cax(const_cast<char *>(InputFile.c_str()),const_cast<char *>(OutputFile.c_str()),const_cast<char *>(sTmpDir.c_str()),bSuccess);
				m_UnitsComboCtrl.SetCurSel(cad2cax.m_iUnit);
				vcUtils::LogMsg("CVMoveCAD_MFC_GUIDlg::OnBnClickedInputButton : Completed reading Units Information.");
#ifndef PART_WISE_TESSELLATION
				StopWatch sw;
				sw.Restart();

				vcExtractMetadata extractor;
				extractor.ExtractMetadata(InputFile, sTmpDir);

				std::stringstream ss;
				ss.str("");
				ss << "Time taken for BB for Model (µs) ******************* :" << sw.ElapsedUs();

				vcUtils::LogMsg(ss.str());
				float fTessTolerance = extractor.GetMinimumDistance() * 0.005;
				m_fTessTolerance = fTessTolerance;
#endif
				
				::SetCursor(m_hArrowCursor);
				






			}
			UpdateData(false); 
		}		
	}
}

void CVMoveCAD_MFC_GUIDlg::OnBnClickedOutputButton()
{
	CString outputPath;
	UpdateData(true);
	if(((CButton*)GetDlgItem(IDC_DIRECTORY_RADIO))->GetCheck())
	{
#if defined(_WIN64)
		std::string sFolderName;
		if(GetFolder(sFolderName))
		{
			m_sOutputFile = sFolderName.c_str();
			UpdateData(false);
		}
#else
		CFolderDialog output_OpenDlg(&outputPath);
		if(output_OpenDlg.DoModal()==IDOK)
		{
			m_sOutputFile=outputPath;
			UpdateData(false);
		}
#endif
	}
	else
	{
		static char BASED_CODE szFilter[] ="VCollab Files(*.cax)|*.cax|CGR(*.cgr)|*.cgr||";
		CFileDialog file_dlg_save(FALSE, _T("cax"), NULL, OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT,(LPCTSTR)szFilter);

		/*if(m_sInputFile.GetLength())
		{
			CString FileName  = m_sInputFile;
			int index = m_sInputFile.Find('.');
			if(index!=-1)
			{
				FileName = m_sInputFile.Left(index);
			}
			FileName = FileName.Right (FileName.GetLength ()-1 - FileName.ReverseFind ('\\') );
			char name[256];
			strcpy(name,FileName);
			file_dlg_save.m_ofn .lpstrFile = name;
		}*/

		if(file_dlg_save.DoModal()==IDOK)
		{
			m_sOutputFile=file_dlg_save.GetPathName();
			CString ext = file_dlg_save.GetFileExt();
			if(ext.CompareNoCase("cax")==0)
				((CComboBox*)GetDlgItem(IDC_OUTPUT_FORMAT_COMBO))->SetCurSel(0);
			else if(ext.CompareNoCase("cgr")==0)
				((CComboBox*)GetDlgItem(IDC_OUTPUT_FORMAT_COMBO))->SetCurSel(1);

			UpdateData(false);
		}
	}
}

void CVMoveCAD_MFC_GUIDlg::OnBnClickedIgnoreTransCheck()
{
	g_bIgnoreTransparency = !g_bIgnoreTransparency;
}

void CVMoveCAD_MFC_GUIDlg::OnBnClickedOk()
{
	UpdateData(true);

	std::string InputPath = (LPCTSTR)m_sInputFile;
	std::string OutputPath = (LPCTSTR)m_sOutputFile;
	bool bSuccess = true;
	sOutputFormat = m_sOutputFormat;

	g_fTessTolerance = m_fTessTolerance;

	if(m_bFileMode)
		vcUtils::LogMsg("VMoveCAD::OnTranslateButtonClick : File Mode");
	else
		vcUtils::LogMsg("VMoveCAD::OnTranslateButtonClick : Directory Mode");


	

	g_bDataLoss = false;
		 
	::SetCursor(m_hWaitCursor);

	if(!vcCadTranslator::Translate(InputPath,OutputPath,m_bFileMode,bSuccess))
	{
		return;
	}

	::SetCursor(m_hArrowCursor);

 


	if(m_bFileMode)
	{
		if(bSuccess)
		{
			if(g_dtkErrorStatus == dtkErrorFileNotExist)
			{ 
				//AfxMessageBox("                     Translation completed.\nBut there might be a data loss in the output file \nas one or more part files are missing in the input directory!!!");
				::MessageBox(NULL,"                     Translation completed,\nbut there might be a data loss in the output file \nas one or more part files are missing in the input directory 1 !!!","VMoveCAD",MB_ICONINFORMATION);
			}   
			else if(g_dtkErrorStatus == dtkErrorVersionNotSupported)
			{
				::MessageBox(NULL,"                     Translation completedb\nbut there might be a data loss in the output file \nas one or more input part files version is not supported 2!!!","VMoveCAD",MB_ICONINFORMATION);
			} 
			else if(g_bDataLoss)
			{
				::MessageBox(NULL,"                     Translation completed,\nbut there might be a data loss in the output file. \nPlease contact support@vcollab.com for support 3 !!!","VMoveCAD",MB_ICONINFORMATION);
			}
			else
			{
				AfxMessageBox("Translated successfully!!!");
			}
		}
		else
		{
			remove(OutputPath.c_str());
			//wxMessageBox(_("Translation failed!!!"), wxT("Translation Result"),wxICON_ERROR, NULL);
		}
	}
	else
	{
		std::stringstream ss;
		ss.str("");
		ss<<"Translated "<<g_iOutputFilesCount<<" of "<<g_iInputFilesCount<<" files successfully!!!";
		AfxMessageBox(ss.str().c_str(),MB_ICONINFORMATION);
		ss.str("");
	}
	g_iInputFilesCount = 0;
	g_iOutputFilesCount = 0;
	//OnOK();
}

void CVMoveCAD_MFC_GUIDlg::OnBnClickedAboutButton()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CVMoveCAD_MFC_GUIDlg::OnBnClickedHelpButton()
{
	//::HtmlHelp(GetDesktopWindow()->GetSafeHwnd(), "VMoveCAD.chm", HH_DISPLAY_TOPIC, NULL);
	CString sHelpPath;
	TCHAR vcollabPath[1024];
	CString envVar = _T("VCOLLAB_DIR");
	if (!GetEnvironmentVariable(envVar, vcollabPath, 1024))
	{
		return;
	}
	sHelpPath = vcollabPath + CString(_T("VMoveCAD64\\VMoveCAD.chm"));


	CFileStatus status;
	if (!CFile::GetStatus(sHelpPath, status))
	{
		AfxMessageBox(sHelpPath + _T(" Help file is not available"));
		return;
	}
	//#ifndef _WIN64
	::HtmlHelp(GetDesktopWindow()->GetSafeHwnd(), sHelpPath, HH_DISPLAY_TOPIC, 0);
}

void CVMoveCAD_MFC_GUIDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	CDialog::OnCancel();
}

void CVMoveCAD_MFC_GUIDlg::OnCbnSelchangeOutputFormatCombo()
{
	// TODO: Add your control notification handler code here
	if(((CComboBox*)GetDlgItem(IDC_OUTPUT_FORMAT_COMBO))->GetCurSel() == 0)//cax
		m_sOutputFormat = "cax";
	else if(((CComboBox*)GetDlgItem(IDC_OUTPUT_FORMAT_COMBO))->GetCurSel() == 1)//cgr
		m_sOutputFormat = "cgr";

	CString ext;
	ext = GetFileExtn(m_sOutputFile);
	m_sOutputFile.Delete(m_sOutputFile.GetLength ()- ext.GetLength(),ext.GetLength());
	m_sOutputFile+=m_sOutputFormat;
	UpdateData(false); 

}

void CVMoveCAD_MFC_GUIDlg::OnCbnSelchangeUnitsCombo()
{
	// TODO: Add your control notification handler code here
	//0 - MILLIMETER
	//1 - METER
	//2 - INCH
	//3 - FEET
	switch(m_UnitsComboCtrl.GetCurSel())
	{
	case 0:
		g_iUnit = vcCad2Cax::MILLIMETER;
		break;
	case 1:
		g_iUnit = vcCad2Cax::METER;
		break;
	case 2:
		g_iUnit = vcCad2Cax::INCH;
		break;
	case 3:
		g_iUnit = vcCad2Cax::FEET;
		break;
	case 4:
		g_iUnit = vcCad2Cax::CENTIMETER;
		break;
	}
	g_bUserUnits = true;
}

void CVMoveCAD_MFC_GUIDlg::OnBnClickedDetailedLogCheck()
{
	g_bDetailedLog = !g_bDetailedLog;
}

void CVMoveCAD_MFC_GUIDlg::OnBnClickedMeshCheck()
{
	// TODO: Add your control notification handler code here
	g_bMesh = ((CButton*)GetDlgItem(IDC_MESH_CHECK))->GetCheck();
}

void CVMoveCAD_MFC_GUIDlg::OnBnClicked2dElementsCheck()
{
	// TODO: Add your control notification handler code here
	g_b2DElements = ((CButton*)GetDlgItem(IDC_2D_ELEMENTS_CHECK))->GetCheck();
}

void CVMoveCAD_MFC_GUIDlg::OnBnClickedPointSetCheck()
{
	// TODO: Add your control notification handler code here
	g_bPointSet = ((CButton*)GetDlgItem(IDC_POINT_SET_CHECK))->GetCheck();
}


void CVMoveCAD_MFC_GUIDlg::OnBnClickedCombinePartsCheck()
{
	// TODO: Add your control notification handler code here
	g_bCombinePartsInGroup = ((CButton*)GetDlgItem(IDC_COMBINE_PARTS_CHECK))->GetCheck();
}
