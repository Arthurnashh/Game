// CfgEditView.h : interface of the CCfgEditView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_CFGEDITVIEW_H__BA1937EF_85EB_11D3_94CD_C0140D8C4B14__INCLUDED_)
#define AFX_CFGEDITVIEW_H__BA1937EF_85EB_11D3_94CD_C0140D8C4B14__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CBrowserView view

class CBrowserView : public CTreeView
{
protected:
	CBrowserView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CBrowserView)

	CImageList _images;

	// Attributes
public:
	CCfgEditDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrowserView)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CBrowserView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CBrowserView)
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in CfgEditView.cpp
inline CCfgEditDoc* CBrowserView::GetDocument()
   { return (CCfgEditDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditorView view

class CEditorView : public CRichEditView
{
protected: // create from serialization only
	CEditorView();
	DECLARE_DYNCREATE(CEditorView)

// Attributes
public:
	CCfgEditDoc* GetDocument();

// Operations
public:
	void GoToLine(int line);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditorView)
	public:
	virtual void OnInitialUpdate();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CEditorView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void Serialize(CArchive& ar, BOOL bSelection);
	virtual void Serialize(CArchive& ar);

protected:
	static DWORD CALLBACK EditStreamCallBack(DWORD dwCookie,
		LPBYTE pbBuff, LONG cb, LONG *pcb);

	// Generated message map functions
	//{{AFX_MSG(CEditorView)
	afx_msg void OnChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in CfgEditView.cpp
inline CCfgEditDoc* CEditorView::GetDocument()
   { return (CCfgEditDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CFGEDITVIEW_H__BA1937EF_85EB_11D3_94CD_C0140D8C4B14__INCLUDED_)
