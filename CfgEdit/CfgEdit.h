// CfgEdit.h : main header file for the CFGEDIT application
//

#if !defined(AFX_CFGEDIT_H__BA1937E7_85EB_11D3_94CD_C0140D8C4B14__INCLUDED_)
#define AFX_CFGEDIT_H__BA1937E7_85EB_11D3_94CD_C0140D8C4B14__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CCfgEditApp:
// See CfgEdit.cpp for the implementation of this class
//

class CCfgEditApp : public CWinApp
{
public:
	CCfgEditApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCfgEditApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CCfgEditApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CEditorView;
class CBrowserView;

CEditorView *GetEditorView();
CBrowserView *GetBrowserView();
void SetActivePane(int row, int col);

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CFGEDIT_H__BA1937E7_85EB_11D3_94CD_C0140D8C4B14__INCLUDED_)
