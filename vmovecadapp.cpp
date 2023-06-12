/////////////////////////////////////////////////////////////////////////////
// Name:        vmovecadapp.cpp
// Purpose:     
// Author:      
// Modified by: 
// Created:     09/11/2010 14:50:44
// RCS-ID:      
// Copyright:   Visual Collaboration Technologies Inc.
// Licence:     
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
#include "vmovecadapp.h"
////@end includes


#include "vmovecadapp.h"
#include <fstream>
#include <sstream>
#include "version.h"
#include "vcUtils.h"
#include "vcLicense.h"

std::string g_sVer;


/*
 * Application instance implementation
 */

////@begin implement app
IMPLEMENT_APP( VMoveCADApp )
////@end implement app 


/*
 * VMoveCADApp type definition
 */

IMPLEMENT_CLASS( VMoveCADApp, wxApp )


/*
 * VMoveCADApp event table definition
 */

BEGIN_EVENT_TABLE( VMoveCADApp, wxApp )

////@begin VMoveCADApp event table entries
////@end VMoveCADApp event table entries

END_EVENT_TABLE()


/*
 * Constructor for VMoveCADApp
 */

VMoveCADApp::VMoveCADApp()
{

    Init();
}


/*
 * Member initialisation
 */

void VMoveCADApp::Init()
{

////@begin VMoveCADApp member initialisation
	m_helpController = NULL;
////@end VMoveCADApp member initialisation
}

/*
 * Initialisation for VMoveCADApp
 */


bool VMoveCADApp::OnInit()
{ 
	vcUtils::InitLog("VMoveCAD_Log.txt");
	
	WriteAppVersionIntoLogFile();
	
	WriteOSVersionIntoLogFile();
	
	if(!CheckOutLicense())
		return false;

	InitializeWX();

	m_helpController = CreateHelpController();
	VMoveCAD* mainWindow = new VMoveCAD(NULL);
	mainWindow->ShowModal();
	mainWindow->Destroy();
	// A modal dialog application should return false to terminate the app.
	return false;


    return true;
}


/*
 * Cleanup for VMoveCADApp
 */

int VMoveCADApp::OnExit()
{ 
	vcLicense::CheckInLicense(); 
	vcUtils::LogMsg("VMoveCADApp::OnExit : Exit");

////@begin VMoveCADApp cleanup
	delete m_helpController;
	return wxApp::OnExit();
////@end VMoveCADApp cleanup
}


/*
 * Creates the help controller
 */

wxHelpControllerBase* VMoveCADApp::CreateHelpController()
{
////@begin VMoveCADApp help controller creation
	wxHelpControllerBase* helpController = new wxHelpController();
	helpController->Initialize(GetHelpFilename());
	return helpController;
////@end VMoveCADApp help controller creation
}


/*
 * Shows help
 */

bool VMoveCADApp::ShowHelp(const wxString& topicName, wxWindow* modalWindow)
{
////@begin VMoveCADApp show help
// Use 'modal help' only where wxHTMLHelpController is being used
#ifndef __WXMSW__
	if (modalWindow)
	{
		wxHtmlModalHelp modalHelp(modalWindow, topicName, GetHelpFilename());
		return true;
	}
#endif

	if (topicName.IsEmpty())
		return GetHelpController()->DisplayContents();
	else
		return GetHelpController()->DisplaySection(topicName);
////@end VMoveCADApp show help
}


/*
 * Returns the help filename
 */

wxString VMoveCADApp::GetHelpFilename()
{
////@begin VMoveCADApp get help filename
	return wxT("VMoveCAD");
////@end VMoveCADApp get help filename
}
bool VMoveCADApp::CheckOutLicense()
{
	return vcLicense::CheckOutLicense();
}

void VMoveCADApp::WriteAppVersionIntoLogFile()
{
#ifdef _WIN64
	vcUtils::LogMsg("VMoveCAD 2011 : Win64 Application");
#else
	vcUtils::LogMsg("VMoveCAD 2011:: Win32 Application");
#endif

	std::stringstream ss;
	ss.str("");
	ss<<"VMoveCAD 2011::Version:: "<<g_sVersion;
	vcUtils::LogMsg(ss.str());
    ss.str("");
	g_sVer = g_sVersion;
}

void VMoveCADApp::WriteOSVersionIntoLogFile()
{
	wchar_t *ver = new wchar_t[1024];
	vcUtils::GetWindowsVersionName(ver,1024);
	std::stringstream ss;
	ss.str("");
	ss<<"VMoveCADApp::OnInit:OS Version:: "<<vcUtils::ToNarrow(ver);
	vcUtils::LogMsg(ss.str());
    ss.str("");
}

void VMoveCADApp::InitializeWX()
{
#if wxUSE_XPM
	wxImage::AddHandler(new wxXPMHandler);
#endif
#if wxUSE_LIBPNG
	wxImage::AddHandler(new wxPNGHandler);
#endif
#if wxUSE_LIBJPEG
	wxImage::AddHandler(new wxJPEGHandler);
#endif
#if wxUSE_GIF
	wxImage::AddHandler(new wxGIFHandler);
#endif
}

