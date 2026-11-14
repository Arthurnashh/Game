// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "PerfLog.h"

#include "MainFrm.h"
#include "PerfLogView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	//ID_INDICATOR_CAPS,
	//ID_INDICATOR_NUM,
	//ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	/*
	if (!m_wndToolBar.Create(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
	*/

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Remove this if you don't want tool tips or a resizeable toolbar
	/*
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	m_wndStatusBar.SetPaneInfo(0,m_wndStatusBar.GetItemID(0),SBPS_NORMAL,400);
	m_wndStatusBar.SetPaneInfo(1,m_wndStatusBar.GetItemID(1),SBPS_STRETCH,100);
	*/

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	/*
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
	*/

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	return CFrameWnd::PreCreateWindow(cs);
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


CPerfLogView * CMainFrame::GetPerfLogView()
{
	CWnd *view = GetActiveView();
	return (CPerfLogView *)view;
}


// export for suma classes

void LogF( const char *format, ... )
{
}


void ErrF( const char *format, ... )
{
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	// CSplitterWnd

	#if 0
		DWORD style = WS_CHILD | WS_VISIBLE |WS_HSCROLL | WS_VSCROLL | SPLS_DYNAMIC_SPLIT;

		CSize size(10,10);
		//SIZE size(10,10);

		if (!m_wndSplitter.Create(this, 2, 1, size, pContext, style ) ) return FALSE;
	#else

		if (!m_wndSplitter.CreateStatic(this, 2, 1))	return FALSE;
		CRect rect;
		GetClientRect(rect);

		SIZE initSize;
		initSize.cx = rect.right;
		initSize.cy = 100;
		if (!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CPerfLogSummaryView), initSize, pContext))
			return FALSE;
		initSize.cy = rect.bottom - initSize.cy;
		if (!m_wndSplitter.CreateView(1, 0, RUNTIME_CLASS(CPerfLogView), initSize, pContext))
			return FALSE;
	#endif

	return TRUE;
}

void CMainFrame::SetStatusText(const char * text)
{
	m_wndStatusBar.SetPaneText( 0,text, TRUE );
}
