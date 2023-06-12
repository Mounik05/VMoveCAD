/////////////////////////////////////////////////////////////////////////////
// Name:        vmovecadapp.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     09/11/2010 14:50:44
// RCS-ID:      
// Copyright:   Visual Collaboration Technologies Inc.
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _VMOVECADAPP_H_
#define _VMOVECADAPP_H_

#ifndef __midl
#define _SXS_ASSEMBLY_VERSION "8.0.50727.4053"
#define _CRT_ASSEMBLY_VERSION _SXS_ASSEMBLY_VERSION
#define _MFC_ASSEMBLY_VERSION _SXS_ASSEMBLY_VERSION
#define _ATL_ASSEMBLY_VERSION _SXS_ASSEMBLY_VERSION

#ifdef __cplusplus
extern "C" {
#endif
__declspec(selectany) int _forceCRTManifest;
__declspec(selectany) int _forceMFCManifest;
__declspec(selectany) int _forceAtlDllManifest;
__declspec(selectany) int _forceCRTManifestRTM;
__declspec(selectany) int _forceMFCManifestRTM;
__declspec(selectany) int _forceAtlDllManifestRTM;
#ifdef __cplusplus
}
#endif
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/help.h"
#include "wx/image.h"
#include "vmovecadDlg.h"
////@end includes


/*!
 * Forward declarations
 */

////@begin forward declarations
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
////@end control identifiers

/*!
 * VMoveCADApp class declaration
 */

class VMoveCADApp: public wxApp
{    
    DECLARE_CLASS( VMoveCADApp )
    DECLARE_EVENT_TABLE()

public:
    /// Constructor
    VMoveCADApp();

    void Init();

    /// Initialises the application
    virtual bool OnInit();

    /// Called on exit
    virtual int OnExit();

////@begin VMoveCADApp event handler declarations

////@end VMoveCADApp event handler declarations

////@begin VMoveCADApp member function declarations

	/// Returns the help controller
	wxHelpControllerBase* GetHelpController() { return m_helpController; }

	/// Creates the help controller
	wxHelpControllerBase* CreateHelpController();

	/// Shows help
	bool ShowHelp(const wxString& topic, wxWindow* modalWindow = NULL);

	/// Gets the help filename
	wxString GetHelpFilename();

////@end VMoveCADApp member function declarations

////@begin VMoveCADApp member variables
	wxHelpControllerBase* m_helpController;
////@end VMoveCADApp member variables

private:
	void InitializeWX();
	bool CheckOutLicense();
	void WriteAppVersionIntoLogFile();
	void WriteOSVersionIntoLogFile();
};

/*!
 * Application instance declaration 
 */

////@begin declare app
DECLARE_APP(VMoveCADApp)
////@end declare app

#endif
    // _VMOVECADAPP_H_
