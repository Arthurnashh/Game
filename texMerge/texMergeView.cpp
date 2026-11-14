// texMergeView.cpp : implementation of the CTexMergeView class
//

#include "stdafx.h"
#include "texMerge.h"
#include "MainFrm.h"

#include "texMergeDoc.h"
#include "texMergeView.h"
#include "WinDraw.h"

#include "..\lib\pacText.hpp"
#include <El/elementpch.hpp>
#include <El/ParamFile/paramFile.hpp>//FLY#include "..\lib\paramFile.hpp"
#include ".\texmergeview.h"
#include "DlgProperties.h"
#include "DlgAutoArrangeMulti.h"
#include "DlgMergeToOne.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTexMergeView

IMPLEMENT_DYNCREATE(CTexMergeView, CScrollView)

BEGIN_MESSAGE_MAP(CTexMergeView, CScrollView)
	//{{AFX_MSG_MAP(CTexMergeView)
    ON_CONTROL(EN_CHANGE,IDC_NAME,OnSetNameChange)
	ON_COMMAND(ID_SET_ADD, OnSetAdd)
	ON_COMMAND(ID_SET_DELETE, OnSetDelete)
	ON_COMMAND(ID_SET_DUPLICATE, OnSetDuplicate)
	ON_COMMAND(ID_ITEM_NEXT, OnItemNext)
	ON_COMMAND(ID_ITEM_PREV, OnItemPrev)
	ON_COMMAND(ID_SET_NEXT, OnSetNext)
	ON_COMMAND(ID_SET_PREV, OnSetPrev)
	ON_COMMAND(ID_ITEM_ADD, OnItemAdd)
	ON_COMMAND(ID_ITEM_DELETE, OnItemDelete)
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_VIEW_ZOOM_IN, OnViewZoomIn)
	ON_COMMAND(ID_VIEW_ZOOM_OUT, OnViewZoomOut)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_ITEM_SHRINK, OnItemShrink)
	ON_COMMAND(ID_ITEM_GROW, OnItemGrow)
	ON_COMMAND(ID_SET_SHRINK_ALL, OnSetShrinkAll)
	ON_COMMAND(ID_SET_GROW_ALL, OnSetGrowAll)
	ON_COMMAND(ID_SET_SHRINK_X, OnSetShrinkX)
	ON_COMMAND(ID_SET_SHRINK_Y, OnSetShrinkY)
	ON_COMMAND(ID_SET_GROW_X, OnSetGrowX)
	ON_COMMAND(ID_SET_GROW_Y, OnSetGrowY)
	ON_COMMAND(ID_ITEM_ROT_LEFT, OnItemRotLeft)
	ON_COMMAND(ID_ITEM_ROT_RIGHT, OnItemRotRight)
	ON_COMMAND(ID_SET_ARRANGE, OnSetArrange)
	ON_COMMAND(ID_SET_MODEL, OnSetModel)
	ON_COMMAND(ID_SET_COPY, OnSetCopy)
	ON_COMMAND(ID_SET_MERGE, OnSetMerge)
	ON_COMMAND(ID_SET_APPLYDAMMAGE, OnSetApplydammage)
	ON_COMMAND(ID_SET_ARRANGE_USE, OnSetArrangeUse)
	//}}AFX_MSG_MAP
  ON_COMMAND(ID_ITEM_PROPERTIES, OnItemProperties)
  ON_UPDATE_COMMAND_UI(ID_ITEM_PROPERTIES, OnUpdateItemProperties)
  ON_WM_CONTEXTMENU()
  ON_COMMAND(ID_ITEM_KEEPSIZE, OnItemKeepsize)
  ON_UPDATE_COMMAND_UI(ID_ITEM_KEEPSIZE, OnUpdateItemProperties)
  ON_COMMAND(ID_SET_PROPERTIES, OnSetProperties)
  ON_COMMAND(ID_SET_ARRANGE_MULTI, OnSetArrangeMulti)
  ON_COMMAND(ID_SET_MERGETOONE, OnSetMergetoone)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTexMergeView construction/destruction

CTexMergeView::CTexMergeView()
{
	// TODO: add construction code here
	_actSet=0;
	_actItem=0;
	_zoom=-1;
	_drag=false;
  _selectionMove=CPoint(0,0);
  _multiSets=0;
}

CTexMergeView::~CTexMergeView()
{
  delete _multiSets;
}

BOOL CTexMergeView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CScrollView::PreCreateWindow(cs);
}



/////////////////////////////////////////////////////////////////////////////
// CTexMergeView drawing

void CTexMergeView::OnDraw(CDC* dc)
{
	CTexMergeDoc* doc = GetDocument();
	ASSERT_VALID( doc);

	// TODO: add draw code for native data here

	CPoint org=dc->GetViewportOrg();

	RECT rect;
	GetClientRect(&rect);

	rect.left-=org.x;
	rect.right-=org.x;
	rect.top-=org.y;
	rect.bottom-=org.y;

	// CScrollView
	WinDraw draw(rect);

	const TextureSet *set=doc->GetTextureSet(_actSet);
	if( set )
	{
		// draw all textures in current set
		for( int i=0; i<set->Size(); i++ )
		{
			const TextureItem &item=set->Get(i);
			PictureData *tex=item.GetTex();
			if( !tex ) continue;
      int x=item.x;
      int y=item.y;
      if (item.IsSelected()) {x+=_selectionMove.x;y+=_selectionMove.y;}
			draw.DrawTexture
			(
				tex,
				Zoom(x,_zoom),Zoom(y,_zoom),
				Zoom(item.UsedW(),_zoom),Zoom(item.UsedH(),_zoom),
				item.angle
			);
      if (item.IsSelected())
      {
        draw.Draw50Transp
          (
          Zoom(x,_zoom),Zoom(y,_zoom),
          Zoom(item.UsedW(),_zoom),Zoom(item.UsedH(),_zoom),
          0xFFE0
          );
      }
		}
		// draw rectangle around whole set
		draw.DrawRect
		(
			Zoom(0,_zoom),Zoom(0,_zoom),
			Zoom(set->_w,_zoom),Zoom(set->_h,_zoom),
			0
		);
		// draw rectangle around actual texture set
		if( _actItem>=0 && _actItem<set->Size() )
		{
			const TextureItem &item=set->Get(_actItem);
			PictureData *tex=item.GetTex();
      int x=item.x;
      int y=item.y;
      if (item.IsSelected()) {x+=_selectionMove.x;y+=_selectionMove.y;}
			if( tex )
			{
				draw.DrawRect
				(
					Zoom(x,_zoom),Zoom(y,_zoom),
					Zoom(item.UsedW(),_zoom),Zoom(item.UsedH(),_zoom),
					0xffff
				);
			}
		}
	}

	draw.Draw(dc->m_hDC,rect);
}

/////////////////////////////////////////////////////////////////////////////
// CTexMergeView diagnostics

#ifdef _DEBUG
void CTexMergeView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CTexMergeView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}

#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
// CTexMergeView message handlers



void CTexMergeView::OnInitialUpdate() 
{
	CScrollView::OnInitialUpdate();
	
	// TODO: Add your specialized code here and/or call the base class
}

void CTexMergeView::OnSetAdd() 
{
	CTexMergeDoc* doc = GetDocument();
	_actSet=doc->AddTextureSet();
}

void CTexMergeView::OnSetDelete() 
{
	CTexMergeDoc* doc = GetDocument();
	doc->DeleteSet(_actSet);
}

void CTexMergeView::OnSetDuplicate() 
{
	CTexMergeDoc* doc = GetDocument();
	_actSet=doc->DuplicateSet(_actSet);
}

// attributes

CString CTexMergeView::GetActTexture() const
{
	const CTexMergeDoc *doc=GetDocument();
	const TextureSet *set=doc->GetTextureSet(_actSet);
	if( !set ) return "";
	if( _actItem<0 || _actItem>=set->Size() ) return "";
	const TextureItem &item=set->Get(_actItem);
	return item.GetTexture();

}

void CTexMergeView::OnItemNext() 
{
	const CTexMergeDoc *doc=GetDocument();
	const TextureSet *set=doc->GetTextureSet(_actSet);
	if( !set ) return;
	if( _actItem<set->Size()-1 ) _actItem++;
	Invalidate();
	ReleaseDrag();
}

void CTexMergeView::OnItemPrev() 
{
	const CTexMergeDoc *doc=GetDocument();
	const TextureSet *set=doc->GetTextureSet(_actSet);
	if( !set ) return;
	if( _actItem>0 ) _actItem--;
	Invalidate();
	ReleaseDrag();
}

void CTexMergeView::OnSetNext() 
{
	CTexMergeDoc *doc=GetDocument();
	int maxSet=doc->GetTextureSetCount()-1;
	if( _actSet<maxSet ) _actSet++;
	_actItem=-1;
	UpdatePage();
	ReleaseDrag();
}

void CTexMergeView::OnSetPrev() 
{
	//CTexMergeDoc *doc=GetDocument();
	if( _actSet>0 ) _actSet--;
	_actItem=-1;
	UpdatePage();
	ReleaseDrag();
}

void CTexMergeView::OnItemAdd() 
{
	CTexMergeDoc *doc=GetDocument();
	// TODO: use listview to select from textures in current directory
	// use file selector to get texture name
	static CString path;
	if( path.IsEmpty() )
	{
		path=doc->GetTexturePath();		
	}

	static char filter[] =
		"Textures (*.pac;*.paa;*.tga)|*.pac; *.paa; *.tga|"
		"8-bit Textures (*.pac)|*.pac|"
		"16-bit Textures (*.paa)|*.paa|"
		"32-bit Targa (*.tga)|*.tga||";
		//"All Files (*.*)|*.*||";
	CFileDialog fsel
	(
		TRUE,NULL,NULL,
		OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|
		OFN_ALLOWMULTISELECT|OFN_LONGNAMES,
		filter,this
	);
    fsel.m_ofn.lpstrInitialDir=path;
	int ret=fsel.DoModal();
	if( ret==IDOK )
	{
		// TODO: make path relative to current document directory
		POSITION pos=fsel.GetStartPosition();
		while( pos )
		{
			path=fsel.GetNextPathName(pos);
			AddItem(path);                        
		}
        const char *c=strrchr(path,'\\');
        if (c) path=path.Mid(0,c-(LPCTSTR)path);
	}

}

void CTexMergeView::OnSetModel() 
{
	CTexMergeDoc *doc=GetDocument();
	// TODO: use listview to select from textures in current directory
	// use file selector to get texture name
	static CString path;
	if( path.IsEmpty() )
	{
		path=doc->GetTexturePath();
		/*
		int pos=path.ReverseFind('\\');
		if( pos>=0 )
		{
			path=path.Left(pos+1);
			path+="*.pac";
		}
		*/
		path+="*.p3d";
	}

	static char filter[] =
		"Model (*.p3d)|*.p3d||";
	CFileDialog fsel
	(
		TRUE,NULL,path,
		OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|
		OFN_ALLOWMULTISELECT|OFN_LONGNAMES,
		filter,this
	);
	int ret=fsel.DoModal();
	if( ret==IDOK )
	{
		// TODO: make path relative to current document directory
		POSITION pos=fsel.GetStartPosition();
		while( pos )
		{
			path=fsel.GetNextPathName(pos);
			AddModel(path);
		}
	}	
}

void CTexMergeView::OnItemDelete() 
{
	CTexMergeDoc *doc=GetDocument();
	doc->DeleteItem(_actSet,_actItem);
}


void CTexMergeView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CTexMergeDoc *doc=GetDocument();
	NotifyData *msg=(NotifyData *)pHint;
	switch( lHint )
	{
		case NMReset:
			_actSet=0;
			_actItem=-1;
			_zoom=-1;
		break;
		case NMItemDeleted:
			if( msg->_set==_actSet && msg->_item<_actItem ) _actItem--;
		break;
		case NMSetDeleted:
			if( msg->_set<_actSet ) _actSet--;
			if( doc->GetTextureSetCount()<=0 )
			{
				_actSet=doc->AddTextureSet();
			}
			if( _actSet>=doc->GetTextureSetCount() )
			{
				_actSet--;
			}
		break;
		case NMSetInserted:
		case NMItemInserted:
		break;
		case NMSetChanged:
		case NMItemChanged:
		break;
		default:
			ASSERT( FALSE );
		break;
	}

	UpdatePage();
}

BOOL CTexMergeView::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	
	//return CScrollView::OnEraseBkgnd(pDC);
	return 0;
}


void CTexMergeView::OnSetShrinkX() 
{
	CTexMergeDoc *doc=GetDocument();
	TextureSet *set=doc->GetTextureSet(_actSet);
	if( !set ) return;
	if( set->_w<MinSetDimension && set->_h<MinSetDimension ) return;
	if( set->_w<=8 ) return;
	set->_w>>=1;
	NotifyData data(_actSet,-1);
	doc->UpdateAllViews(NULL,NMSetChanged,&data);
	doc->SetModifiedFlag();
	
}

void CTexMergeView::OnSetShrinkY() 
{
	CTexMergeDoc *doc=GetDocument();
	TextureSet *set=doc->GetTextureSet(_actSet);
	if( !set ) return;
	if( set->_w<MinSetDimension && set->_h<MinSetDimension ) return;
	if( set->_h<=8 ) return;
	set->_h>>=1;
	NotifyData data(_actSet,-1);
	doc->UpdateAllViews(NULL,NMSetChanged,&data);
	doc->SetModifiedFlag();
	
}

void CTexMergeView::OnSetGrowX() 
{
	CTexMergeDoc *doc=GetDocument();
	TextureSet *set=doc->GetTextureSet(_actSet);
	if( !set ) return;
	
	if( set->_w>=MaxSetDimension ) return;
	set->_w<<=1;
	NotifyData data(_actSet,-1);
	doc->UpdateAllViews(NULL,NMSetChanged,&data);
	doc->SetModifiedFlag();
	
}

void CTexMergeView::OnSetGrowY() 
{
	CTexMergeDoc *doc=GetDocument();
	TextureSet *set=doc->GetTextureSet(_actSet);
	if( !set ) return;
	
	if( set->_h>=MaxSetDimension ) return;
	set->_h<<=1;
	NotifyData data(_actSet,-1);
	doc->UpdateAllViews(NULL,NMSetChanged,&data);
	doc->SetModifiedFlag();
	
}


void CTexMergeView::OnSetGrowAll() 
{
	CTexMergeDoc *doc=GetDocument();
	TextureSet *set=doc->GetTextureSet(_actSet);
	if( !set ) return;
	
	if( set->_w>=MaxSetDimension || set->_h>=MaxSetDimension ) return;
	set->_w<<=1;
	set->_h<<=1;
	for( int i=0; i<set->Size(); i++ )
	{
		set->Set(i).scale++;
		set->Set(i).x<<=1;
		set->Set(i).y<<=1;
	}
	NotifyData data(_actSet,-1);
	doc->UpdateAllViews(NULL,NMSetChanged,&data);
	doc->SetModifiedFlag();
}

void CTexMergeView::OnSetShrinkAll() 
{
	CTexMergeDoc *doc=GetDocument();
	TextureSet *set=doc->GetTextureSet(_actSet);
	if( !set ) return;
	if( set->_w<MinSetDimension && set->_h<MinSetDimension ) return;
	if( set->_w<=8 || set->_h<=8 ) return;
	set->_w>>=1;
	set->_h>>=1;
	for( int i=0; i<set->Size(); i++ )
	{
		set->Set(i).scale--;
		set->Set(i).x>>=1;
		set->Set(i).y>>=1;
	}
	NotifyData data(_actSet,-1);
	doc->UpdateAllViews(NULL,NMSetChanged,&data);
	doc->SetModifiedFlag();
}

void CTexMergeView::OnViewZoomIn() 
{
	if( _zoom<+2 ) _zoom++;
	UpdatePage();	
}

void CTexMergeView::OnViewZoomOut() 
{
	if( _zoom>-4 ) _zoom--;
	UpdatePage();	
}

void CTexMergeView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CTexMergeDoc *doc=GetDocument();
	int logX=GetLogicalX(point.x);
	int logY=GetLogicalY(point.y);
	int select=SelectItem(logX,logY);
  if (~nFlags & MK_CONTROL)
  {
    for (int i=0,cnt=doc->GetTexturesCount(_actSet);i<cnt;++i)
    {
      doc->SelectItem(_actSet,i,false);
    }
  }
	if( select>=0 )
	{
		_actItem=select;
    _dragDist=point;
	}
	else
	{
		_actItem=-1;
		Invalidate();
	}
	CScrollView::OnLButtonDown(nFlags, point);
}

void CTexMergeView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CTexMergeDoc *doc=GetDocument();
	if( _drag )
	{
    ReleaseCapture();
    _selectionMove.x=GetLogicalX(point.x)-_dragBegX;
    _selectionMove.y=GetLogicalY(point.y)-_dragBegY;
    CWaitCursor wc;    
    doc->SelectionMove(_actSet,_selectionMove.x,_selectionMove.y,&_actItem);
    _selectionMove=CPoint(0,0);
	  Invalidate();
	}
  else
  {
      if (_actItem!=-1) doc->SelectItem(_actSet,_actItem, !doc->IsSelected(_actSet,_actItem));
      Invalidate();
  }
	_drag=false;
	
	CScrollView::OnLButtonUp(nFlags, point);
}

void CTexMergeView::OnMouseMove(UINT nFlags, CPoint point) 
{
	CTexMergeDoc *doc=GetDocument();
	if( _drag )
	{
		const TextureItem *item=doc->GetItem(_actSet,_actItem);
		if( item )
		{
      _selectionMove.x=GetLogicalX(point.x)-_dragBegX;
      _selectionMove.y=GetLogicalY(point.y)-_dragBegY;
      _selectionMove.x=(_selectionMove.x+7)&~15;
      _selectionMove.y=(_selectionMove.y+7)&~15;
			Invalidate();
		}
	}
  else if (nFlags & MK_LBUTTON)
  {
    CPoint diff=_dragDist-point;
    if (abs(diff.x)>5 || abs(diff.y)>5)
    {    
      const TextureItem *item=doc->GetItem(_actSet,_actItem);
      if( item )
      {
        int logX=GetLogicalX(point.x);
        int logY=GetLogicalY(point.y);
        _dragBegX=logX;
        _dragBegY=logY;
        _dragX=item->x;
        _dragY=item->y;
        _dragW=item->UsedW();
        _dragH=item->UsedH();
        _drag=true;
        SetCapture();
        doc->SelectItem(_actSet,_actItem,true);
      }
    }
  }
	
	CScrollView::OnMouseMove(nFlags, point);
}

int CTexMergeView::GetLogicalX(int mouseX)
{
	// convert mouse coord to logical
	// take care of CScrollView offset
	CPoint orig=GetDeviceScrollPosition();
	return Zoom(mouseX+orig.x,-_zoom);
}

int CTexMergeView::GetLogicalY(int mouseY)
{
	// take care of offset
	CPoint orig=GetDeviceScrollPosition();
	return Zoom(mouseY+orig.y,-_zoom);
}

int CTexMergeView::SelectItem(int logX, int logY)
{
	const CTexMergeDoc *doc=GetDocument();
	const TextureSet *set=doc->GetTextureSet(_actSet);
	if( !set ) return -1;
	int ret=-1;
	// find last item that matches given criteria
	for( int i=0; i<set->Size(); i++ )
	{
		const TextureItem &item=set->Get(i);
		if
		(
			logX>=item.x && logX<=item.x+item.UsedW() &&
			logY>=item.y && logY<=item.y+item.UsedH()
		)
		{
			ret=i;
		}
	}
	return ret;
}

void CTexMergeView::AddItem( const char *name )
{
	CTexMergeDoc *doc=GetDocument();
	_actItem=doc->AddItem(_actSet,name);
}

void CTexMergeView::AddModel( const char *name )
{
	CTexMergeDoc *doc=GetDocument();
	_actSet=doc->AddModel(name);
}

void CTexMergeView::UpdatePage()
{
	const CTexMergeDoc *doc=GetDocument();

	const TextureSet *set=doc->GetTextureSet(_actSet);
	if( set )
	{
		SetScrollSizes( MM_TEXT, CSize(Zoom(set->_w,_zoom),Zoom(set->_h,_zoom)) );
	}
	//SetScrollSizes( MM_TEXT, CSize(512,512) );
	//ResizeParentToFit( );   // Default bShrinkOnly argument
	Invalidate();
    UpdateSetName();
}

void CTexMergeView::ReleaseDrag()
{
	// release drag - something has happened
	if( _drag )
	{
		_drag=false;
	}
}

void CTexMergeView::OnItemShrink() 
{
	CTexMergeDoc *doc=GetDocument();
	const TextureItem *item=doc->GetItem(_actSet,_actItem);
	if( !item ) return;
	TextureItem mod=*item;
	mod.scale--;
	while( Zoom(mod.w,mod.scale)<2 ) mod.scale++;
	while( Zoom(mod.h,mod.scale)<2 ) mod.scale++;
	while( Zoom(max(mod.h,mod.w),mod.scale)<8 ) mod.scale++;
	doc->ModifyItem(_actSet,_actItem,mod);
}

void CTexMergeView::OnItemGrow() 
{
	CTexMergeDoc *doc=GetDocument();
	const TextureItem *item=doc->GetItem(_actSet,_actItem);
	if( !item ) return;
	TextureItem mod=*item;
	mod.scale++;
	doc->ModifyItem(_actSet,_actItem,mod);
}


void CTexMergeView::OnItemRotLeft() 
{
	CTexMergeDoc *doc=GetDocument();
	const TextureItem *item=doc->GetItem(_actSet,_actItem);
	if( !item ) return;
	TextureItem mod=*item;
	mod.angle=(mod.angle-1)&3;
	doc->ModifyItem(_actSet,_actItem,mod);
}

void CTexMergeView::OnItemRotRight() 
{
	CTexMergeDoc *doc=GetDocument();
	const TextureItem *item=doc->GetItem(_actSet,_actItem);
	if( !item ) return;
	TextureItem mod=*item;
	mod.angle=(mod.angle+1)&3;
	doc->ModifyItem(_actSet,_actItem,mod);
}

void CTexMergeView::OnSetArrange() 
{
	CTexMergeDoc *doc=GetDocument();
	doc->AutoArrange(_actSet,AAM_Grow);
	_actItem=-1;
}

void CTexMergeView::OnSetArrangeUse() 
{
	CTexMergeDoc *doc=GetDocument();
	doc->AutoArrange(_actSet,AAM_KeepSize);
	_actItem=-1;
}



void CTexMergeView::OnSetCopy() 
{
	CTexMergeDoc *doc=GetDocument();
	doc->CopySetToClipboard(_actSet);
}

void CTexMergeView::OnSetMerge() 
{
	CTexMergeDoc *doc=GetDocument();
	doc->MergeSetFromClipboard(_actSet);	
}

void CTexMergeView::OnSetApplydammage() 
{
	// ask for hpp file
	static CString path;
	static char filter[] =
		"Dammage pairs (*.hpp)|*.hpp||";
	CFileDialog fsel
	(
		TRUE,NULL,path,
		OFN_EXPLORER|OFN_OVERWRITEPROMPT|OFN_LONGNAMES,
		filter,this
	);
	int ret=fsel.DoModal();
	if (ret!=IDOK) return;
	path = fsel.GetPathName();
	// process the file
	ParamFile wounds;
	wounds.Parse(path);
	
	ConstParamEntryPtr entry = wounds.FindEntry("wounds");
	if (!entry) return; // bad file format
	// duplicate old set

	CTexMergeDoc* doc = GetDocument();
	_actSet=doc->DuplicateSet(_actSet);

	TextureSet *set = doc->GetTextureSet(_actSet);

	for (int i=0; i<entry->GetSize()-1; i+=2)
	{
		RStringB oldTex = (*entry)[i];
		RStringB newTex = (*entry)[i+1];
		// replace any oldTex with newTex
		for (int i=0; i<set->Size(); i++)
		{
			TextureItem &item = set->Set(i);
			char oldName[512];
			strcpy(oldName, item.GetTexture());
			strlwr(oldName);
			char oldTexName[512];
			strcpy(oldTexName,"\\");
			strcat(oldTexName,oldTex);
			if (!strchr(oldTexName,'.')) strcat(oldTexName,".");
			strlwr(oldTexName);
			if (strstr(oldName,oldTexName))
			{

				char newTexName[512];
				strcpy(newTexName,"\\");
				strcat(newTexName,newTex);
				if (!strchr(newTexName,'.')) strcat(newTexName,".");
				strlwr(newTexName);

				char *rep = strstr(oldName,oldTexName);
				char end[512];
				strcpy(end,rep+strlen(oldTexName));
				strcpy(rep,newTexName);
				strcat(rep,end);
				// replace oldTex with newTex
				item.SetTexture(oldName);
			}
		}
	}
}


void CTexMergeView::UpdateSetName()
{
  CMainFrame *mainFrm=static_cast<CMainFrame *>(AfxGetMainWnd());
  TextureSet &set=*GetDocument()->GetTextureSet(GetActSet());
  mainFrm->m_wndSetName.SetDlgItemText(IDC_NAME,set._name);
}

void CTexMergeView::OnSetNameChange()
{
  CMainFrame *mainFrm=static_cast<CMainFrame *>(AfxGetMainWnd());
  TextureSet &set=*GetDocument()->GetTextureSet(GetActSet());
  CString name;
  mainFrm->m_wndSetName.GetDlgItemText(IDC_NAME,name);
  set._name=name.GetString();
}

void CTexMergeDoc::SelectItem(int setid, int item, bool sel)
{
  TextureSet *set=GetTextureSet(setid);
  if (set)
  {
    (*set)[item].Select(sel);
  }
}

bool CTexMergeDoc::IsSelected(int setid, int item)
{
  TextureSet *set=GetTextureSet(setid);
  if (set)  
    return (*set)[item].IsSelected();
  else
    return false;
}

class FunctorApplyChangesItems
{
CTexMergeDoc *doc;
int set;
public:
  FunctorApplyChangesItems(CTexMergeDoc *doc, int set):doc(doc),set(set) {}

  void Added(const TextureProp &prop) const
  {
    doc->SelectionChangeTextureProp(set,prop);
    doc->SetModifiedFlag();
  }

  void Removed(const TextureProp &prop) const
  {
    doc->SelectionRemoveTextureProp(set,prop);
    doc->SetModifiedFlag();
  }

  void Changed(const TextureProp &prop) const
  {
    if (stricmp(prop.name,"Source")==0) doc->SelectionSetSource(set,prop.value);
    else if (stricmp(prop.name,"Importance")==0) 
      {        
        char *stp;
        float importance=(float)strtod(prop.value,&stp);
        if (importance>0)  doc->SelectionSetImportance(set,importance);
      }
    else if (stricmp(prop.name,"Scale")==0)
    {
      int scale=atoi(prop.value);
      doc->SelectionSetScale(set,scale);
    }
    else  doc->SelectionChangeTextureProp(set,prop);
    doc->SetModifiedFlag();
  }
};

void CTexMergeView::OnItemProperties()
{
  TextureSet *set=GetDocument()->GetTextureSet(_actSet);
  TextureItem &itm=(*set)[_actItem];

  DlgPropertiesList<MemAllocStack<TextureProp,50> > allprop;
  allprop=itm._properties;
  char buff[50];

  sprintf(buff,"%g",itm.importance);
  allprop.Append(TextureProp(RString("Importance"),RString(buff)));
  allprop.Append(TextureProp(RString("Source"),RString(itm.GetTexture())));  
  allprop.Append(TextureProp(RString("Scale"),RString(itoa(itm.scale,buff,10))));

  DlgProperties dlg;
  dlg.SetValues(allprop);
  if (dlg.DoModal())
  {
    allprop.CompareWith(dlg.GetValues(),FunctorApplyChangesItems(GetDocument(),_actSet));
    Invalidate();
  }
}

void CTexMergeView::OnUpdateItemProperties(CCmdUI *pCmdUI)
{
  pCmdUI->Enable(_actItem>=0 && _actSet>=0);
}

void CTexMergeView::OnContextMenu(CWnd* pWnd, CPoint point)
{
  CPoint client=point;
  ScreenToClient(&client);
  int logX=GetLogicalX(client.x);
  int logY=GetLogicalY(client.y);
  int select=SelectItem(logX,logY);
  _actItem=select;
  if (select>=0) GetDocument()->SelectItem(_actSet,select,true);
  Invalidate();

  CMenu mnu;
  mnu.LoadMenu(IDR_MAINFRAME);
  CMenu *sub=mnu.GetSubMenu(2);
  sub->TrackPopupMenu(TPM_LEFTALIGN,point.x,point.y,AfxGetMainWnd());
}

void CTexMergeView::OnItemKeepsize()
{
  GetDocument()->KeepSize(_actSet);
}

class FunctorApplyChangesSet
{
  CTexMergeDoc *doc;
  int set;
  mutable int width;
  mutable int height;
public:
  FunctorApplyChangesSet(CTexMergeDoc *doc, int set):doc(doc),set(set)
  {
    width=height=0;
  }
    void Added(const TextureProp &prop) const
    {
      doc->SetChangeTextureProp(set,prop);
      doc->SetModifiedFlag();
    }

    void Removed(const TextureProp &prop) const
    {
      doc->SetRemoveTextureProp(set,prop);
      doc->SetModifiedFlag();
    }

    void Changed(const TextureProp &prop) const
    {
      if (stricmp(prop.name,"Name")==0) 
        doc->ChangeSetName(set,prop.value);
      else if (stricmp(prop.name,"Dim.Width")==0) 
      {        
        width=atoi(prop.value);
      }
      else if (stricmp(prop.name,"Dim.Height")==0) 
      {        
        height=atoi(prop.value);
      }
      else if (stricmp(prop.name,"Background color")==0)
      {
        float bg[4];
        if (sscanf(prop.value,"(%f,%f,%f,%f)",bg+0,bg+1,bg+2,bg+3)==4)
          doc->ChangeSetBgColor(set,bg);
      }
      else  doc->SetChangeTextureProp(set,prop);
      doc->SetModifiedFlag();
    }
    ~FunctorApplyChangesSet()
    {
      if (width>0 || height>0)
      {
        if (width<1) width=doc->GetTextureSet(set)->_w;
        if (height<1) width=doc->GetTextureSet(set)->_h;
        doc->ChangeSetDimensions(set,width,height);
        doc->SetModifiedFlag();
        NotifyData data(set,-1);
        doc->UpdateAllViews(NULL,NMSetChanged,&data);
      }
    }
  };






void CTexMergeView::OnSetProperties()
{
  TextureSet *set=GetDocument()->GetTextureSet(_actSet);
  DlgPropertiesList<MemAllocStack<TextureProp,50> > allprop;

  allprop=set->_properties;

  char buff[255];

  allprop.Append(TextureProp(RString("Name"),RString(set->_name)));
  allprop.Append(TextureProp(RString("Dim.Width"),RString(itoa(set->_w,buff,10))));  
  allprop.Append(TextureProp(RString("Dim.Height"),RString(itoa(set->_h,buff,10))));
  sprintf(buff,"(%g,%g,%g,%g)",set->_bgcolor[0],set->_bgcolor[1],set->_bgcolor[2],set->_bgcolor[3]);
  allprop.Append(TextureProp(RString("Background color"),RString(buff)));  

  DlgProperties dlg;
  dlg.SetValues(allprop);
  if (dlg.DoModal())
  {
    allprop.CompareWith(dlg.GetValues(),FunctorApplyChangesSet(GetDocument(),_actSet));
    Invalidate();
  }
}

void CTexMergeView::OnSetArrangeMulti()
{
  CTexMergeDoc *doc=GetDocument();
  if (_multiSets==0) _multiSets=new DlgAutoArrangeMulti;
  if (_multiSets->DoModal()==IDOK)
  {  
    doc->AutoArrangeMulti(_actSet,_multiSets->vNSets,_multiSets->vOptimize!=FALSE,_multiSets->vDontMoveSel);
    _actItem=-1;
  }
}

void CTexMergeView::OnSetMergetoone()
{
  CTexMergeDoc *doc=GetDocument();
  DlgMergeToOne dlg;
  AutoArray<CString,MemAllocStack<CString,32> >titles;
  AutoArray<int,MemAllocStack<int,32> >results;
  for (int i=0;i<doc->GetTextureSetCount();i++)
  {
    char buff[256];
    _snprintf(buff,256,"%s (%d)",doc->GetTextureSet(i)->_name.Data(),i);
    titles.Append(CString(buff));
  }
  results.Resize(titles.Size());
  dlg.vSetTitles=titles;
  dlg.vResult=results;
  if (dlg.DoModal()==IDOK)
  { 
    if (results.Size())
    {  
      SetActiveSet(dlg.vResult[0]);
      doc->MergeToOne(dlg.vResult);
    }
  }
}
