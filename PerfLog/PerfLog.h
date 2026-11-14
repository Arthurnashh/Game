// PerfLog.h : main header file for the PERFLOG application
//

#if !defined(AFX_PERFLOG_H__C8B8CC88_B3D0_11D3_8373_00A0C9DF4D61__INCLUDED_)
#define AFX_PERFLOG_H__C8B8CC88_B3D0_11D3_8373_00A0C9DF4D61__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPerfLogApp:
// See PerfLog.cpp for the implementation of this class
//

class CPerfLogApp : public CWinApp
{
public:
	CPerfLogApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPerfLogApp)
	public:
	virtual BOOL InitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CPerfLogApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CPerfLogApp theApp;

class CPerfLogView;

CPerfLogView *GetPerfLogView();

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PERFLOG_H__C8B8CC88_B3D0_11D3_8373_00A0C9DF4D61__INCLUDED_)
