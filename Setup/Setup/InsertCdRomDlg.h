#if !defined(AFX_INSERTCDROMDLG_H__DD212DED_58AD_4011_BD62_893F27F6128F__INCLUDED_)
#define AFX_INSERTCDROMDLG_H__DD212DED_58AD_4011_BD62_893F27F6128F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// InsertCdRomDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CInsertCdRomDlg dialog

class CInsertCdRomDlg : public CDialog
{
// Construction
public:
	CInsertCdRomDlg(CSetupObject *setupObj, CCdRom *cd, CString &drive, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CInsertCdRomDlg)
	enum { IDD = IDD_INSERT_CDROM_DIALOG };
	CStatic	_message;
	//}}AFX_DATA

	CCdRom			*_cd;
	CString			&_drive;
	UINT			_timerId;
	CSetupObject	*_setupObj;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInsertCdRomDlg)
	public:
	virtual int DoModal();
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CInsertCdRomDlg)
	afx_msg void OnTimer(UINT nIDEvent);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INSERTCDROMDLG_H__DD212DED_58AD_4011_BD62_893F27F6128F__INCLUDED_)
