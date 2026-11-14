// PerfLogView.h : interface of the CPerfLogView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_PERFLOGVIEW_H__C8B8CC90_B3D0_11D3_8373_00A0C9DF4D61__INCLUDED_)
#define AFX_PERFLOGVIEW_H__C8B8CC90_B3D0_11D3_8373_00A0C9DF4D61__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#define RICHTEXT_VIEW 1

#if RICHTEXT_VIEW
	#include <afxrich.h>
	#define CBaseView CRichEditView
#else
	#include <afxcview.h>
	#include "listvwex.h"
	#define CBaseView CListViewEx
#endif

#include <Es/essencePch.hpp>
#include <Es/Containers/array.hpp>

class CPerfLogDoc;

class CPerfLogView : public CBaseView
{
	friend class CPerfLogApp;

	int _nColumns; // used to maintaing listview headers
	int _nLines; // used to maintaing listview lines
	AutoArray<int> _showColumns;

	int _currentSection;

protected: // create from serialization only
	CPerfLogView();
	DECLARE_DYNCREATE(CPerfLogView)

// Attributes
public:
	CPerfLogDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPerfLogView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	protected:
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	void Busy();
	void Ready();

	void UpdateHeaders();
	#if RICHTEXT_VIEW
	void ListText( const char *line, bool clear=false );
	void DeleteLastChar();
	#endif
	void ListHeaders();
	void ListCount( const char *text, int count );
	void ListLine( const AutoArray<double> &line, bool allFloats=false );
	void ListSection( int index );
	void ListSectionSummary( int index );
	void ListSectionAnalysis( int index );
	void ListDelimiter( const char *text );

	void UpdateView();
	void UpdateSummaryView();

	void Serialize(CArchive& ar, BOOL bSelection);
	virtual ~CPerfLogView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

#if RICHTEXT_VIEW
	static DWORD CALLBACK LineStreamInCallBack
	(
		DWORD cookie, LPBYTE buff, LONG cb, LONG *pcb
	);
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CPerfLogView)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CPerfLogSummaryView : public CPerfLogView
{

	public:
	CPerfLogSummaryView();
	~CPerfLogSummaryView();

	DECLARE_DYNCREATE(CPerfLogSummaryView)


	virtual void OnInitialUpdate();
	protected:
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	
};

#ifndef _DEBUG  // debug version in PerfLogView.cpp
inline CPerfLogDoc* CPerfLogView::GetDocument()
   { return (CPerfLogDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PERFLOGVIEW_H__C8B8CC90_B3D0_11D3_8373_00A0C9DF4D61__INCLUDED_)
