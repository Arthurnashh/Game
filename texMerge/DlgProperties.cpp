// DlgProperties.cpp : implementation file
//

#include "stdafx.h"
#include "texMerge.h"
#include <es/strings/rstring.hpp>
#include "texMergeDoc.h"
#include "DlgProperties.h"
#include "resource.h"
#include <shlobj.h>

#define IDC_VALUEEDIT 999

// DlgProperties dialog

IMPLEMENT_DYNAMIC(DlgProperties, CDialog)
DlgProperties::DlgProperties(CWnd* pParent /*=NULL*/)
	: CDialog(DlgProperties::IDD, pParent)
{
  _editControl=0;
  _newVal=false;
}

DlgProperties::~DlgProperties()
{
  if (_editControl) delete _editControl;
}

void DlgProperties::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_PROPLIST, wList);
}


BEGIN_MESSAGE_MAP(DlgProperties, CDialog)
  ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_PROPLIST, OnLvnBeginlabeleditProplist)
  ON_EN_KILLFOCUS(IDC_VALUEEDIT,OnEndValueEdit)
  ON_COMMAND(IDC_EDITNAME, OnBnClickedEditname)
  ON_COMMAND(IDC_EDITVALUE, OnBnClickedEditvalue)
  ON_COMMAND(IDC_PICKAFILE, OnBnClickedPickafile)
  ON_COMMAND(IDC_PICKAFOLDER, OnBnClickedPickafolder)
  ON_BN_CLICKED(IDC_ADD, OnBnClickedAdd)
  ON_BN_CLICKED(IDC_DELETE, OnBnClickedDelete)
END_MESSAGE_MAP()


// DlgProperties message handlers

BOOL DlgProperties::OnInitDialog()
{
  CDialog::OnInitDialog();

  CRect rc;
  wList.GetClientRect(&rc);
  rc.right=rc.right-GetSystemMetrics(SM_CXVSCROLL)+2*GetSystemMetrics(SM_CXDLGFRAME);

  wList.InsertColumn(0,CString(MAKEINTRESOURCE(IDS_PROPDLGNAME)),LVCFMT_LEFT,rc.right*1/4,0);
  wList.InsertColumn(1,CString(MAKEINTRESOURCE(IDS_PROPDLGVALUE)),LVCFMT_LEFT,rc.right*3/4,1);
  ListView_SetExtendedListViewStyleEx(wList,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
  for (int i=0;i<_properties.Size();i++)
  {
    int p=wList.InsertItem(i,_properties[i].name);
    wList.SetItemText(p,1,_properties[i].value);
  }  

  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}


void DlgProperties::OnLvnBeginlabeleditProplist(NMHDR *pNMHDR, LRESULT *pResult)
{
  NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);  
  CPoint pt(GetMessagePos());
  wList.ScreenToClient(&pt);
  int w=wList.GetColumnWidth(0);
  if (pt.x<w)
  {
    StartEdit(pDispInfo->item.iItem,0);
  }
  else
  {
    StartEdit(pDispInfo->item.iItem,1);
  }
  *pResult = 1;    
}

void DlgProperties::StartEdit(int item, int subItem)
{
  if (_editControl) OnEndValueEdit();
  CRect rcEdit;
_itemPosEdit=item;
_itemEdit=subItem;
  wList.GetItemRect(item,&rcEdit,LVIR_BOUNDS);
  for (int i=0;i<subItem;i++) rcEdit.left+=wList.GetColumnWidth(i);
  rcEdit.right=rcEdit.left+wList.GetColumnWidth(subItem);

  wList.MapWindowPoints(this,&rcEdit);
  CString content=wList.GetItemText(item,subItem);
  _editControl=new CEdit();
  _editControl->Create(WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,rcEdit,this,IDC_VALUEEDIT);
  _editControl->SetWindowText(content);
  _editControl->SetFont(wList.GetFont());
  _editControl->SetSel(0,-1);
  _editControl->SetFocus();
  wList.Invalidate(TRUE);
}

void DlgProperties::OnEndValueEdit()
{
  if (_editControl==0) return;
  CString value;
  _editControl->GetWindowText(value);
  wList.SetItemText(_itemPosEdit,_itemEdit,value);
  _editControl->DestroyWindow();
  delete _editControl;
  _editControl=0;
  bool canceled=_itemEdit==0 && value.GetLength()==0;
  if (canceled) 
    {wList.DeleteItem(_itemPosEdit);_newVal=false;_itemPosEdit=-1;}
}

BOOL DlgProperties::PreTranslateMessage(MSG* pMsg)
{
  if (pMsg->message==WM_KEYDOWN && (pMsg->wParam==VK_TAB || pMsg->wParam==VK_RETURN || pMsg->wParam==VK_ESCAPE) && _editControl)
  {
    wList.SetFocus();
    if (_itemPosEdit>=0 && _itemEdit==0 && pMsg->wParam!=VK_ESCAPE) {StartEdit(_itemPosEdit,1);_newVal=false;}    
    return TRUE;
  }
  if (pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_RETURN && GetKeyState(VK_CONTROL)<0 && _editControl==0)
  {
    OnBnClickedEditname();
    return TRUE;
  }
  if (pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_DELETE && _editControl==0)
  {
    OnBnClickedDelete();
    return TRUE;
  }
  if (pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_INSERT && _editControl==0)
  {
    OnBnClickedAdd();
    return TRUE;
  }
  return CDialog::PreTranslateMessage(pMsg);
}

void DlgProperties::OnBnClickedEditname()
{
  POSITION pos=wList.GetFirstSelectedItemPosition();
  if (pos) StartEdit(wList.GetNextSelectedItem(pos),0);
}

void DlgProperties::OnBnClickedEditvalue()
{
  POSITION pos=wList.GetFirstSelectedItemPosition();
  if (pos) StartEdit(wList.GetNextSelectedItem(pos),1);
}

void DlgProperties::OnBnClickedPickafile()
{
  POSITION pos=wList.GetFirstSelectedItemPosition();
  if (pos) 
  {
    int item=wList.GetNextSelectedItem(pos);
    CString content=wList.GetItemText(item,1);
    CFileDialog fdlg(TRUE,0,content,OFN_HIDEREADONLY,0);
    if (fdlg.DoModal())
    {
      wList.SetItemText(item,1,fdlg.GetPathName());
    }
  }
}


static int WINAPI PosBrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData)
{
  const char *curpath=(const char *)lpData;  
  if (uMsg == BFFM_INITIALIZED)
  {
    if (curpath && curpath[0])
    {
      ::SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM)((LPCSTR)curpath));
      ::SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)((LPCSTR)curpath));
    }
  }
  else if (uMsg == BFFM_SELCHANGED)
  {
    char buff[_MAX_PATH];
    if (SHGetPathFromIDList((LPITEMIDLIST)lParam,buff))
    {        
      ::SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)buff);
    }
  }
  return 0;
};

static bool PathBrowser(HWND hWnd, char *path /* MAX_PATH size */)
{
  BROWSEINFO brw;
  memset(&brw,0,sizeof(brw));
  brw.hwndOwner=hWnd;
  brw.pidlRoot=NULL;
  brw.pszDisplayName=path;
  brw.lParam=(LPARAM)path;
  brw.ulFlags= BIF_RETURNONLYFSDIRS |BIF_STATUSTEXT|BIF_USENEWUI ; 	
  brw.lpfn = (BFFCALLBACK)(PosBrowseCallbackProc);
  LPITEMIDLIST il=SHBrowseForFolder( &brw );
  if (il==NULL) return false;
  SHGetPathFromIDList(il,path);
  IMalloc *shmalloc;
  SHGetMalloc(&shmalloc);
  shmalloc->Free(il);
  if (path[0]==0) return false;
  return true;
}


void DlgProperties::OnBnClickedPickafolder()
{
  POSITION pos=wList.GetFirstSelectedItemPosition();
  if (pos) 
  {
    int item=wList.GetNextSelectedItem(pos);
    CString content=wList.GetItemText(item,1);    
    bool ok=PathBrowser(*this,content.GetBufferSetLength(MAX_PATH));
    content.ReleaseBuffer();
    if (ok)
    {
      wList.SetItemText(item,1,content);
    }
  }
}

void DlgProperties::OnBnClickedAdd()
{
  int p=wList.InsertItem(wList.GetItemCount(),"ZZZZZZZZZZZ");
  wList.SetItemText(p,0,"");
  for (int i=0,cnt=wList.GetItemCount();i<cnt;i++)     
    wList.SetItemState(i,i==p?(LVIS_FOCUSED|LVIS_SELECTED):0,LVIS_FOCUSED|LVIS_SELECTED);
  _newVal=true;  
  StartEdit(p,0);
}

void DlgProperties::OnBnClickedDelete()
{
  POSITION pos=wList.GetFirstSelectedItemPosition();
  if (pos) wList.DeleteItem(wList.GetNextSelectedItem(pos));
}

void DlgProperties::OnOK()
{
  _properties.Clear();
  for (int i=0,cnt=wList.GetItemCount();i<cnt;i++)
    _properties.Append(TextureProp(RString(wList.GetItemText(i,0)),RString(wList.GetItemText(i,1))));
  CDialog::OnOK();
}
