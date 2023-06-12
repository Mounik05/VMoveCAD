// VMoveCAD_MFC_GUIDlg.h : header file
//

#pragma once
#include "afxwin.h"


// CVMoveCAD_MFC_GUIDlg dialog
class CVMoveCAD_MFC_GUIDlg : public CDialog
{
// Construction
public:
	CVMoveCAD_MFC_GUIDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_VMOVECAD_MFC_GUI_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	HCURSOR m_hWaitCursor,m_hArrowCursor;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedFileRadio();
	afx_msg void OnBnClickedDirectoryRadio();
	afx_msg void OnBnClickedInputButton();
	afx_msg void OnBnClickedOutputButton();
	afx_msg void OnBnClickedIgnoreTransCheck();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedAboutButton();
	afx_msg void OnBnClickedHelpButton();
	afx_msg void OnBnClickedCancel();
	CString m_sInputFile;
	CString m_sOutputFile;
	float m_fTessTolerance;
	afx_msg void OnCbnSelchangeOutputFormatCombo();

	CString m_sOutputFormat;
	bool m_bFileMode;
	CComboBox m_UnitsComboCtrl;
	afx_msg void OnCbnSelchangeUnitsCombo();
	afx_msg void OnBnClickedDetailedLogCheck();
	afx_msg void OnBnClickedMeshCheck();
	afx_msg void OnBnClicked2dElementsCheck();
	afx_msg void OnBnClickedPointSetCheck();
	afx_msg void OnBnClickedCombinePartsCheck();
};
