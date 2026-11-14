// texMerge.h : main header file for the TEXMERGE application
//

#if !defined(AFX_TEXMERGE_H__7E5B949E_7BE3_11D3_8373_00A0C9DF4D61__INCLUDED_)
#define AFX_TEXMERGE_H__7E5B949E_7BE3_11D3_8373_00A0C9DF4D61__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

#define TMM_OPENSERVER (WM_APP+5)

/////////////////////////////////////////////////////////////////////////////
// CTexMergeApp:
// See texMerge.cpp for the implementation of this class
//

class CTexMergeApp : public CWinApp
{
	CString _texturePath;
	CString _mergedPath;

public:
	CTexMergeApp();
	~CTexMergeApp();

	void SetTexturePath( CString &path,bool nosave=false ){_texturePath=path;if (!nosave) SaveConfig();}
	void SetMergedPath( CString &path, bool nosave=false ){_mergedPath=path;if (!nosave)SaveConfig();}
	CString GetTexturePath() const {return _texturePath;}
	CString GetMergedPath() const {return _mergedPath;}

	void LoadConfig();
	void SaveConfig();

// Overrides
	virtual BOOL ProcessMessageFilter( int code, LPMSG lpMsg );
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTexMergeApp)
	public:
	virtual BOOL InitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CTexMergeApp)
	afx_msg void OnAppAbout();
	afx_msg void OnFilePref();
    afx_msg LRESULT OnOpenServer(WPARAM wParam,LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CTexMergeApp theApp;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEXMERGE_H__7E5B949E_7BE3_11D3_8373_00A0C9DF4D61__INCLUDED_)
