// texMergeView.h : interface of the CTexMergeView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_TEXMERGEVIEW_H__7E5B94A6_7BE3_11D3_8373_00A0C9DF4D61__INCLUDED_)
#define AFX_TEXMERGEVIEW_H__7E5B94A6_7BE3_11D3_8373_00A0C9DF4D61__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class DlgAutoArrangeMulti;
class CTexMergeView : public CScrollView
{
private:
	// cursor position
	int _actSet;
	int _actItem;

	int _zoom; // log 2 of zoom

	bool _drag; // perform drag on current item
	int _dragBegX,_dragBegY; // drag original. position
	int _dragX,_dragY; // drag rect. position
	int _dragW,_dragH;

  CPoint _selectionMove;

  CPoint _dragDist;

  DlgAutoArrangeMulti *_multiSets;  

protected: // create from serialization only
	CTexMergeView();
	DECLARE_DYNCREATE(CTexMergeView)

// Attributes
public:
	CTexMergeDoc *GetDocument() { return (CTexMergeDoc*)m_pDocument; }
	const CTexMergeDoc *GetDocument() const { return (const CTexMergeDoc*)m_pDocument; }

	int GetActSet() const {return _actSet;}
	int GetActItem() const {return _actItem;}
	CString GetActTexture() const;
    void SetActiveSet(int set) {_actSet=set;}
    void SetActiveItem(int item) {_actItem=item;}
    void SelectItem(int item, bool sel=true)
    {
      GetDocument()->SelectItem(_actSet,item,sel);
    }

// Operations
public:
	// notify function enable multiple views keep synchronized
	// currently there is only one view and Notify is not implemented

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTexMergeView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	protected:
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	void UpdatePage();
	void ReleaseDrag();
	int SelectItem( int logX, int logY );
	int GetLogicalY( int mouseY );
	int GetLogicalX( int mouseX );

	void AddItem( const char *name );
	void AddModel( const char *name );

	virtual ~CTexMergeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CTexMergeView)
	afx_msg void OnSetAdd();
	afx_msg void OnSetDelete();
	afx_msg void OnSetDuplicate();
	afx_msg void OnItemNext();
	afx_msg void OnItemPrev();
	afx_msg void OnSetNext();
	afx_msg void OnSetPrev();
	afx_msg void OnItemAdd();
	afx_msg void OnItemDelete();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnViewZoomIn();
	afx_msg void OnViewZoomOut();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnItemShrink();
	afx_msg void OnItemGrow();
	afx_msg void OnSetShrinkAll();
	afx_msg void OnSetGrowAll();
	afx_msg void OnSetShrinkX();
	afx_msg void OnSetShrinkY();
	afx_msg void OnSetGrowX();
	afx_msg void OnSetGrowY();
	afx_msg void OnItemRotLeft();
	afx_msg void OnItemRotRight();
	afx_msg void OnSetArrange();
	afx_msg void OnSetModel();
	afx_msg void OnSetCopy();
	afx_msg void OnSetMerge();
	afx_msg void OnSetApplydammage();
	afx_msg void OnSetArrangeUse();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
  void UpdateSetName();
  void OnSetNameChange();
  afx_msg void OnItemProperties();
  afx_msg void OnUpdateItemProperties(CCmdUI *pCmdUI);
  afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
  afx_msg void OnItemKeepsize();
  afx_msg void OnSetProperties();
  afx_msg void OnSetArrangeMulti();
  afx_msg void OnSetMergetoone();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEXMERGEVIEW_H__7E5B94A6_7BE3_11D3_8373_00A0C9DF4D61__INCLUDED_)
