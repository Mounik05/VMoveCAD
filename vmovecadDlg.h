/////////////////////////////////////////////////////////////////////////////
// Name:        vmovecadDlg.h
// Purpose:     
// Author:      
// Modified by: 
// Created:     09/11/2010 15:18:21
// RCS-ID:      
// Copyright:   Visual Collaboration Technologies Inc.
// Licence:     
/////////////////////////////////////////////////////////////////////////////

// Generated by DialogBlocks (unregistered), 09/11/2010 15:18:21

#ifndef _VMOVECADDLG_H_
#define _VMOVECADDLG_H_


/*!
 * Includes
 */

////@begin includes
////@end includes



/*!
* Forward declarations
 */
#include <vector>
////@begin forward declarations
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_VMOVECAD 10000
#define ID_FILE_RADIOBUTTON 10010
#define ID_DIRECTORY_RADIOBUTTON 10011
#define ID_COMBOBOX 10012
#define ID_TEXTCTRL 10001
#define ID_FILEOPEN_BUTTON 10002
#define ID_TEXTCTRL1 10003
#define ID_FILESAVE_BUTTON 10004
#define ID_TEXTCTRL2 10015
#define ID_TRANSPARENCY_CHECKBOX 10014
#define ID_TRANSLATE_BUTTON 10006
#define ID_ABOUT_BUTTON 10007
#define ID_HELP_BUTTON 10013
#define ID_CLOSE_BUTTON 10005
#define SYMBOL_VMOVECAD_STYLE wxCAPTION|wxSYSTEM_MENU|wxCLOSE_BOX|wxTAB_TRAVERSAL
#define SYMBOL_VMOVECAD_TITLE _("VMoveCAD 2011 R1 (Beta) ")
#define SYMBOL_VMOVECAD_IDNAME ID_VMOVECAD
#define SYMBOL_VMOVECAD_SIZE wxSize(400, 300)
#define SYMBOL_VMOVECAD_POSITION wxDefaultPosition
////@end control identifiers


/*!
 * VMoveCAD class declaration
 */

class VMoveCAD: public wxDialog
{    
    DECLARE_DYNAMIC_CLASS( VMoveCAD )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    VMoveCAD();
    VMoveCAD( wxWindow* parent, wxWindowID id = SYMBOL_VMOVECAD_IDNAME, const wxString& caption = SYMBOL_VMOVECAD_TITLE, const wxPoint& pos = SYMBOL_VMOVECAD_POSITION, const wxSize& size = SYMBOL_VMOVECAD_SIZE, long style = SYMBOL_VMOVECAD_STYLE );

    /// Creation
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_VMOVECAD_IDNAME, const wxString& caption = SYMBOL_VMOVECAD_TITLE, const wxPoint& pos = SYMBOL_VMOVECAD_POSITION, const wxSize& size = SYMBOL_VMOVECAD_SIZE, long style = SYMBOL_VMOVECAD_STYLE );

    /// Destructor
    ~VMoveCAD();

    /// Initialises member variables
    void Init();

    /// Creates the controls and sizers
    void CreateControls();

////@begin VMoveCAD event handler declarations

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_FILE_RADIOBUTTON
    void OnFileRadiobuttonSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_DIRECTORY_RADIOBUTTON
    void OnDirectoryRadiobuttonSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX
    void OnComboboxSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_FILEOPEN_BUTTON
    void OnFileopenButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_FILESAVE_BUTTON
    void OnFilesaveButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_TRANSPARENCY_CHECKBOX
    void OnTransparencyCheckboxClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANSLATE_BUTTON
    void OnTranslateButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ABOUT_BUTTON
    void OnAboutButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_HELP_BUTTON
    void OnHelpButtonClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CLOSE_BUTTON
    void OnCloseButtonClick( wxCommandEvent& event );

////@end VMoveCAD event handler declarations

////@begin VMoveCAD member function declarations

    bool GetBFileMode() const { return m_bFileMode ; }
    void SetBFileMode(bool value) { m_bFileMode = value ; }

    wxString GetSOutputFormat() const { return m_sOutputFormat ; }
    void SetSOutputFormat(wxString value) { m_sOutputFormat = value ; }

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end VMoveCAD member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin VMoveCAD member variables
    wxRadioButton* m_pFileCheckBoxCtrl;
    wxComboBox* m_pOutputFormatComboCtrl;
    wxStaticText* m_pInputStaticCtrl;
    wxTextCtrl* m_pInputFileNameTextCtrl;
    wxButton* m_FileOpenButton;
    wxStaticText* m_pOutputStaticCtrl;
    wxTextCtrl* m_pOutputFileNameTextCtrl;
    wxTextCtrl* m_pTessToleranceTextCtrl;
    wxCheckBox* m_pTransparencyCheckBoxCtrl;
    bool m_bFileMode;
    wxString m_sOutputFormat;
////@end VMoveCAD member variables
};

#endif
    // _VMOVECADDLG_H_
