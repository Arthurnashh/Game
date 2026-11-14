// PosSetup.h : main header file for the POSSETUP application
//

#if !defined(AFX_POSSETUP_H__5923B627_0361_11D3_809E_0060083C95F5__INCLUDED_)
#define AFX_POSSETUP_H__5923B627_0361_11D3_809E_0060083C95F5__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CPosSetupApp:
// See PosSetup.cpp for the implementation of this class
//

class CPosSetupApp : public CWinApp
{
public:
	CPosSetupApp();

// Overrides
	// jméno aplikace
	LPCTSTR			m_pszOldAppName;
	CString			m_sAppName;

	CSetupObject	m_SetupObject;

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPosSetupApp)
	public:
	virtual BOOL InitInstance();
	virtual int	ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CPosSetupApp)
	afx_msg void OnUpdateHelp(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_POSSETUP_H__5923B627_0361_11D3_809E_0060083C95F5__INCLUDED_)
