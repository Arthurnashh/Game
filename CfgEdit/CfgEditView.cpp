// CfgEditView.cpp : implementation of the CCfgEditView class
//

#include "stdafx.h"
#include "CfgEdit.h"

#include "CfgEditDoc.h"
#include "CfgEditView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditorView

IMPLEMENT_DYNCREATE(CEditorView, CRichEditView)

CEditorView::CEditorView()
{
	m_nWordWrap = WrapNone;
}

CEditorView::~CEditorView()
{
}

BEGIN_MESSAGE_MAP(CEditorView, CRichEditView)
	//{{AFX_MSG_MAP(CEditorView)
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

class EditorCookie
{
public:
	CArchive& m_ar;
	DWORD m_dwError;
	EditorCookie(CArchive& ar) : m_ar(ar) {m_dwError=0;}
};

void CEditorView::Serialize(CArchive& ar, BOOL bSelection)
{
	EDITSTREAM es = {0, 0, EditStreamCallBack};
	EditorCookie cookie(ar);
	es.dwCookie = (DWORD)&cookie;
	int nFormat = SF_TEXT;
	if (bSelection)
		nFormat |= SFF_SELECTION;
	if (ar.IsStoring())
		GetRichEditCtrl().StreamOut(nFormat, es);
	else
	{
		GetRichEditCtrl().StreamIn(nFormat, es);
		Invalidate();
	}
	if (cookie.m_dwError != 0)
		AfxThrowFileException(cookie.m_dwError);
}

DWORD CALLBACK CEditorView::EditStreamCallBack(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	EditorCookie *pCookie = (EditorCookie *)dwCookie;
	CArchive &ar = pCookie->m_ar;
	ar.Flush();
	DWORD dw = 0;
	*pcb = cb;
	TRY
	{
		if (ar.IsStoring())
			ar.GetFile()->WriteHuge(pbBuff, cb);
		else
			*pcb = ar.GetFile()->ReadHuge(pbBuff, cb);
	}
	CATCH(CFileException, e)
	{
		*pcb = 0;
		pCookie->m_dwError = (DWORD)e->m_cause;
		dw = 1;
//		DELETE_EXCEPTION(e);
	}
	AND_CATCH_ALL(e)
	{
		*pcb = 0;
		pCookie->m_dwError = (DWORD)CFileException::generic;
		dw = 1;
//		DELETE_EXCEPTION(e);
	}
	END_CATCH_ALL
	return dw;
}

void CEditorView::Serialize(CArchive& ar)
{
	ASSERT_VALID(this);
	ASSERT(m_hWnd != NULL);
	Serialize(ar, FALSE);
	ASSERT_VALID(this);
}

void CEditorView::GoToLine(int line)
{
	int index = GetRichEditCtrl().LineIndex(line - 1);
	if (index >= 0)
	{
		GetRichEditCtrl().SetSel(index, index);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CEditorView diagnostics

#ifdef _DEBUG
void CEditorView::AssertValid() const
{
	CRichEditView::AssertValid();
}

void CEditorView::Dump(CDumpContext& dc) const
{
	CRichEditView::Dump(dc);
}

CCfgEditDoc* CEditorView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CCfgEditDoc)));
	return (CCfgEditDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CEditorView message handlers

void CEditorView::OnInitialUpdate() 
{
	CRichEditView::OnInitialUpdate();

	bool indexValid = GetDocument()->IsIndexValid();
	
	DWORD mask = GetRichEditCtrl().GetEventMask();
	mask |= ENM_CHANGE;
	GetRichEditCtrl().SetEventMask(mask);

	CHARFORMAT cf;
  cf.cbSize = sizeof(cf); 
  cf.dwMask = CFM_FACE | CFM_SIZE | CFM_BOLD; 
	cf.dwEffects = 0;
  cf.yHeight = 200; 
  cf.bCharSet = DEFAULT_CHARSET; 
  cf.bPitchAndFamily = FIXED_PITCH | FF_DONTCARE; 
  strncpy(cf.szFaceName, "Courier", LF_FACESIZE); 
	GetRichEditCtrl().SetDefaultCharFormat(cf);

	PARAFORMAT pf;
  pf.cbSize = sizeof(pf); 
  pf.dwMask = PFM_TABSTOPS; 
  pf.cTabCount = MAX_TAB_STOPS;
	for (int i=0; i<MAX_TAB_STOPS; i++) pf.rgxTabs[i] = 200 * i;
	GetRichEditCtrl().SetParaFormat(pf);

	GetDocument()->SetIndexValid(indexValid);
	GetDocument()->SetModifiedFlag(FALSE);
}

void CEditorView::OnChange() 
{
	GetDocument()->SetIndexValid(false);
	GetDocument()->SetModifiedFlag();
}

