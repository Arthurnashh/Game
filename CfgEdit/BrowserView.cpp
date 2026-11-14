// BrowserView.cpp : implementation file
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
// CBrowserView

IMPLEMENT_DYNCREATE(CBrowserView, CTreeView)

CBrowserView::CBrowserView()
{
}

CBrowserView::~CBrowserView()
{
}


BEGIN_MESSAGE_MAP(CBrowserView, CTreeView)
	//{{AFX_MSG_MAP(CBrowserView)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBrowserView drawing

void CBrowserView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CBrowserView diagnostics

#ifdef _DEBUG
void CBrowserView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CBrowserView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CCfgEditDoc* CBrowserView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CCfgEditDoc)));
	return (CCfgEditDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBrowserView message handlers

void InsertItems(CTreeCtrl &tree, const ParamClass *cls, HTREEITEM parent)
{
	if (!cls) return;

	CString name = cls->GetName();
	int line = cls->GetLine();
/*
	char buffer[16];
	itoa(cls->GetLine(), buffer, 10);
	CString name = buffer;
	name += " - ";
	name += cls->GetName();
*/

	switch (cls->GetType())
	{
		case PTClass:
			{
				CString base = cls->GetBase();
				if (base.GetLength() > 0)
				{
					name += " : ";
					name += base;
				}
				HTREEITEM item = tree.InsertItem
				(
					name, 0, 1,
					parent, TVI_LAST
				);
				tree.SetItemData(item, line);
				for (int i=0; i<cls->GetEntryCount(); i++)
					InsertItems(tree, cls->GetEntry(i), item);
			}
			break;
		case PTValue:
			{
				HTREEITEM item = tree.InsertItem
				(
					name, 2, 3,
					parent, TVI_LAST
				);
				tree.SetItemData(item, line);
			}
			break;
		case PTEnum:
			{
				HTREEITEM item = tree.InsertItem
				(
					name, 4, 5,
					parent, TVI_LAST
				);
				tree.SetItemData(item, line);
			}
			break;
	}
}

void CBrowserView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	if (pSender == this) return;

	CTreeCtrl &tree = GetTreeCtrl();
	tree.DeleteAllItems();

	const ParamClass &root = GetDocument()->RootClass();
	for (int i=0; i<root.GetEntryCount(); i++)
		InsertItems(tree, root.GetEntry(i), TVI_ROOT);
}

BOOL CBrowserView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	m_dwDefaultStyle |=
		TVS_HASLINES |
		TVS_LINESATROOT |
		TVS_HASBUTTONS |
		TVS_SHOWSELALWAYS |
		TVS_DISABLEDRAGDROP;

	return CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}

void CBrowserView::OnInitialUpdate() 
{
	CTreeView::OnInitialUpdate();

	if (_images.m_hImageList) _images.DeleteImageList();
	_images.Create(IDB_TREE, 16, 1, ILC_COLOR);
	
	GetTreeCtrl().SetImageList(&_images, TVSIL_NORMAL);
}

void CBrowserView::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
	if (GetDocument() && !GetDocument()->IsIndexValid())
	{
		GetDocument()->RebuildIndex();
	}

	CTreeCtrl &ctrl = GetTreeCtrl();
	ASSERT(ctrl.GetSafeHwnd() == pNMHDR->hwndFrom);
	HTREEITEM item = ctrl.GetSelectedItem();
	if (!item)
	{
		*pResult = 0;
		return;
	}

	int line = ctrl.GetItemData(item);

	if (GetEditorView())
	{
		GetEditorView()->GoToLine(line);
		GetEditorView()->PostMessage(WM_SETFOCUS);
	}

	*pResult = 1;
}
