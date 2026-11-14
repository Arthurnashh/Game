// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__7E5B94A2_7BE3_11D3_8373_00A0C9DF4D61__INCLUDED_)
#define AFX_MAINFRM_H__7E5B94A2_7BE3_11D3_8373_00A0C9DF4D61__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "PipeServer.h"

class CMainFrame : public CFrameWnd
{
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:
	int _setNumber;
	int _setCount;
	CString _setName;
	CString _textureName;
	int _setW,_setH;
	int _itemX,_itemY,_itemW,_itemH;
  float _itemImportance;
	
// Operations
public:

// Overrides

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;
    CToolBar    m_wndSourceSafe;

// Generated message map functions
protected:
	afx_msg void OnUpdatePage(CCmdUI *pCmdUI);
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDropFiles(HDROP hDropInfo);
    afx_msg LRESULT OnOpenServer(WPARAM wParam,LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()


    void ServerMode(PipeServer &server,bool quiet=false);

public:
  afx_msg void OnViewServerconsole();
  CDialogBar  m_wndSetName;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__7E5B94A2_7BE3_11D3_8373_00A0C9DF4D61__INCLUDED_)
