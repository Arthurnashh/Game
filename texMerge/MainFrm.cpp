// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "texMerge.h"

#include "MainFrm.h"

#include "TexMergeDoc.h"
#include "TexMergeView.h"
#include <El/Pathname/Pathname.h>
#include <El/Math/Math3dp.hpp>
#include ".\mainfrm.h"

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
  ON_WM_CREATE()
  ON_WM_DROPFILES()
  ON_COMMAND_EX(ID_VIEW_SOURCECONTROLTOOLBAR,OnBarCheck)
  ON_UPDATE_COMMAND_UI(ID_VIEW_SOURCECONTROLTOOLBAR, OnUpdateControlBarMenu)
  //}}AFX_MSG_MAP
  ON_UPDATE_COMMAND_UI(ID_INDICATOR_PAGE, OnUpdatePage)
  ON_MESSAGE(TMM_OPENSERVER, OnOpenServer)


  ON_COMMAND(ID_VIEW_SERVERCONSOLE, OnViewServerconsole)
END_MESSAGE_MAP()

static UINT indicators[] =
{
  ID_INDICATOR_PAGE,
    ID_SEPARATOR,           // status line indicator
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
    ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
  _setName="";
  _setNumber=0;
  _setCount=0;
  _textureName="";
}

CMainFrame::~CMainFrame()
{
}

#define InstallDialogBar(name,IDD,menuid,align,dock)\
  name.Create(this,IDD,align,menuid);\
  name.SetBarStyle(name.GetBarStyle() |\
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);\
		name.EnableDocking(dock);\
		
#define InstallDialogBar2(name,IDD,menuid,align)\
InstallDialogBar(name,IDD,menuid,align,CBRS_ALIGN_ANY)


#define InstallToolBar(name,IDD,align,menuid)\
  if (name.Create(this,WS_CHILD | WS_VISIBLE | align, menuid ) && \
  name.LoadToolBar(IDD))\
{\
  name.SetBarStyle(name.GetBarStyle() |\
  CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);\
  name.EnableDocking(CBRS_ALIGN_ANY);\
}	  
  

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
    return -1;

  if (!m_wndToolBar.Create(this) ||
    !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
  {
    TRACE0("Failed to create toolbar\n");
    return -1;      // fail to create
  }

  if (!m_wndStatusBar.Create(this) ||
    !m_wndStatusBar.SetIndicators(indicators,
    sizeof(indicators)/sizeof(UINT)))
  {
    TRACE0("Failed to create status bar\n");
    return -1;      // fail to create
  }

  m_wndStatusBar.SetPaneInfo(0,m_wndStatusBar.GetItemID(0),SBPS_NORMAL,400);
  m_wndStatusBar.SetPaneInfo(1,m_wndStatusBar.GetItemID(1),SBPS_STRETCH,100);

  // TODO: Remove this if you don't want tool tips or a resizeable toolbar
  m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
    CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
  m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);

  InstallToolBar(m_wndSourceSafe,IDR_SOURCESAFE,CBRS_TOP,ID_VIEW_SOURCECONTROLTOOLBAR);
  InstallDialogBar2(m_wndSetName,IDD_SETNAMEBAR,ID_VIEW_SETNAMEBAR,CBRS_TOP);


  // TODO: Delete these three lines if you don't want the toolbar to
  //  be dockable
  EnableDocking(CBRS_ALIGN_ANY);
  DockControlBar(&m_wndToolBar);
  DockControlBar(&m_wndSourceSafe,AFX_IDW_DOCKBAR_RIGHT );    
  DockControlBar(&m_wndSetName,AFX_IDW_DOCKBAR_TOP,0);
  DockControlBar(&m_wndSetName,AFX_IDW_DOCKBAR_BOTTOM);

  return 0;
}

void CMainFrame::OnUpdatePage(CCmdUI *pCmdUI)
{
  pCmdUI->Enable(); 

  CString strPage;
  if( !_textureName.IsEmpty() )
  {
    const char *nameOnly=strrchr(_textureName,'\\');
    if( !nameOnly ) nameOnly=_textureName;
    else nameOnly++;
    strPage.Format
      (
      "%s : %g (%d..%d,%d..%d of %dx%d)",
      nameOnly,_itemImportance,
      _itemX,_itemX+_itemW,_itemY,_itemY+_itemH,_setW,_setH
      ); 
  }
  else
  {
    strPage.Format
      (
      "%s: (%d of %d) - %dx%d",
      _setName, _setNumber+1, _setCount, _setW,_setH
      ); 
  }
  pCmdUI->SetText( strPage ); 
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

static inline bool Testpow2(int x)
{
  if (x==0) return false;
  while (!(x & 1)) x>>=1;
  return x==1;
}

static inline void MultipleLines(char *line)
{
  char *p=strstr(line,"::");
  while (p!=NULL)
  {
    p[0]='\r';
    p[1]='\n';
    if (strnicmp(p+2,"foreach",7)==0) break;
    p=strstr(p,"::");
  }
}

void CMainFrame::ServerMode(PipeServer &server,bool quiet)
{
  CTexMergeDoc *document=static_cast<CTexMergeDoc *>(GetActiveDocument());
  CTexMergeView *view=static_cast<CTexMergeView *>(GetActiveView());
  if (!quiet) server.SendOK("Texture Merge Server Ready. Type HELP for list of commands");
  for(;;)
  {
    if (server.Eof()) return;
    if (server.TestCommand("HELP"))
    {
      server.SendOK();
      server.SendMultiline(
        "OPEN filename        - opens PTM file\r\n"
        "NEW                  - creates empty project\r\n"
        "SAVE [filename]      - save PTM file\r\n"
        "GENERATE [path]      - generate textures\r\n"
        "AUTOARRANGE set,grow - auto arranges textures\r\n"
        "                         set = <index>|current|all\r\n"
        "                         grow = (on|off|limit)\r\n"
        "AUTOARRANGE multi,cnt,optimize\r\n"
        "                     - auto arranges actual set int multiple sets\r\n"
        "                         multi - word 'multi' to identify command\r\n"
        "                         cnt - count of additional set use\r\n"
        "                         optimize - (on|off)\r\n"
        "ADDSET               - adds new set and selects it as current\r\n"
        "SETSIZE width,height - sets with and height of set\r\n"
        "SETTEX filename      - sets texture of set\r\n"
        "INFOSET              - displays information about set\r\n"
        "INFOSET[]            - displays information about set-script form (array)\r\n"
        "DELSET i             - removes set\r\n"
        "SELSET i             - selects set\r\n"
        "MERGE dst,src        - merges src to dst (removes src)\r\n"
        "CURSET               - shows current set\r\n"
        "NSETS                - displays number of sets\r\n"
        "ADDITEM filename     - adds texture to set\r\n"
        "DELETEITEM i         - removes texture from set\r\n"
        "INFOITEM i           - informations about item\r\n"
        "INFOITEM i[]         - informations about item in script form (array)\r\n"
        "SETITEMTEX i,tex     - changes items texture\r\n"
        "NITEMS               - displays number of items\r\n"
        "TRANSFORM texture    - calculates transform matrix for texture, and show\r\n"
        "                       name of merged texture\r\n"                                 
        "TRANSFORMBACK u,v,tex- transforms back from merged to unmerged\r\n"
        "                         u,v - coordinates of center of bounding rectangle\r\n"
        "                         tex - merged texture name\r\n"
        "                       To get u,v parameters,take all uvs from face(s) and\r\n"
        "                       calculate the center of UV bounding rectangle\r\n"
        "SELECTTEX cmd,name   - select of deselects texture identified by name\r\n"
        "                         cmd = true or false (selected, notselected)\r\n"
        "                         name = texture name\r\n"
        "TGAMODE [on|off]     - enables or disables TGA export\r\n"
        "TEXPREFIX prefix     - sets texture prefix (doesn't save inf)\r\n"
        "MERGEPATH prefix     - sets merged path (doesn't save inf)\r\n"
        "FOREACHSET command   - function repeates command for each set. \r\n"
        "                     - note: use %d as index\r\n"
        "                     - note: Each command prints status\r\n"
        "FOREACHITEM command  - function repeates command for each item. \r\n"
        "                     - note: use %d as index\r\n"
        "                     - note: Each command prints status\r\n"
        "ECHO text            - prints text (+OK is omnitted)\r\n"
        "QUIT                 - exit server\r\n"
        "QUITAPP              - exit server and TexMerge\r\n");
    }
    else if (server.TestCommand("QUIT"))
    {
      server.Ack();
      return;
    }
    else if (server.TestCommand("QUITAPP"))
    {
      DestroyWindow();
      return;
    }
    else if (server.TestCommand("OPEN_+([^\r\n]+)"))
    {
      RString fname(server.GetSubExp(),server.GetSubExpLen());
      if (document->OnOpenDocument(fname)) server.SendOK();else server.SendError();
    }   
    else if (server.TestCommand("GENERATE_+([^\r\n]+)"))
    {
      RString path(server.GetSubExp(),server.GetSubExpLen());
      path=path+"\\";
      if (document->ExportMerged(path)) server.SendOK();
      else server.SendError("Generate Error");
    }
    else if (server.TestCommand("GENERATE"))
    {
      CString path=document->GetMergedPath();      
      path=path+"\\";
      if (document->ExportMerged(path))  server.SendOK(path);      
      else server.SendError("Generate error");
    }
    else if (server.TestCommand("SAVE") || server.TestCommand("SAVE_+([^\r\n]+)"))
    {
      RString fname(server.GetSubExp(),server.GetSubExpLen());
      if (fname[0]==0) fname=(LPCTSTR)document->GetPathName();
      document->OnSaveDocument(fname);
      server.SendOK(fname);
    }
    else if (server.TestCommand("TGAMODE (on|off)"))
    {
      RString mode(server.GetSubExp(),server.GetSubExpLen());
      if (stricmp(mode,"on")==0) document->_tgaOutput=true;
      else document->_tgaOutput=false;
      server.SendOK(mode);
    }
    else if (server.TestCommand("NEW"))
    {
      if (document->OnNewDocument())
        server.SendOK();
      else
        server.SendError();
    }
    else if (server.TestCommand("AUTOARRANGE multi,([0-9]+),(on|off)"))
    {
      int count=server.GetSubExpInt(1);
      RString optimize(server.GetSubExp(2),server.GetSubExpLen(2));
      int set=view->GetActSet();
      document->AutoArrangeMulti(set,count,stricmp(optimize,"on")==0,true);
      server.SendOK();
    }
    else if (server.TestCommand("AUTOARRANGE ([0-9]+|current|all),(on|off|limit)"))
    {
      RString mode(server.GetSubExp(1),server.GetSubExpLen(1));
      RString grow(server.GetSubExp(2),server.GetSubExpLen(2));
      AutoArrangeMode growmode=AAM_KeepSize;
      if (stricmp(grow,"on")==0) growmode=AAM_Grow;
      else if (stricmp(grow,"limit")==0) growmode=AAM_ShrinkOnly;

      if (stricmp(mode,"all")==0)
      {
        for (int i=0,cnt=document->GetTextureSetCount();i<cnt;i++)
        {
          document->AutoArrange(i,growmode);
        }
        server.SendOK();
      }
      else
      {
        int set=-1;
        if (stricmp(mode,"current")==0)
        {
          set=view->GetActSet();
        }
        else 
        {
          set=server.GetSubExpInt(1);
        }
        if (set>=0 && set<document->GetTextureSetCount())
        {
          document->AutoArrange(set,growmode);
          server.SendOK();
        }
        else
          server.SendError("Invalid set index");
      }
    }
    else if (server.TestCommand("NSETS"))
    {
      int sets=document->GetTextureSetCount();
      char tmp[50];
      server.SendOK(itoa(sets,tmp,10));
    }
    else if (server.TestCommand("ADDSET"))
    {
      int set=document->AddTextureSet();
      view->SetActiveSet(set);
      char tmp[50];
      server.SendOK(itoa(set,tmp,10));
    }
    else if (server.TestCommand("CURSET"))
    {
      int set=view->GetActSet();
      char tmp[50];
      server.SendOK(itoa(set,tmp,10));
    }
    else if (server.TestCommand("DELSET_+([0-9]+)"))
    {
      int set=server.GetSubExpInt();
      if (set>=0 && set<document->GetTextureSetCount())
      {
        document->DeleteSet(set);
        server.SendOK();
      }
      else
        server.SendError("Invalid set index");
    }
    else if (server.TestCommand("ADDITEM_+([^\r\n]+)"))
    {
      RString path(server.GetSubExp(),server.GetSubExpLen());
      document->AddItem(view->GetActSet(),path);
      server.SendOK();
    }
    else if (server.TestCommand("SELSET_+([0-9]+)"))
    {
      int set=server.GetSubExpInt();
      if (set>=0 && set<document->GetTextureSetCount())
      {
        view->SetActiveSet(set);
        server.SendOK();
      }
      else
        server.SendError("Invalid set index");
    }
    else if (server.TestCommand("MERGE_+([0-9]+),([0-9]+)"))
    {
      int params[2];
      params[0]=server.GetSubExpInt(1);
      params[1]=server.GetSubExpInt(2);
      if (params[0]>=0 && params[0]<document->GetTextureSetCount() &&
        params[1]>=0 && params[1]<document->GetTextureSetCount() && params[0]!=params[1])
      {
          view->SetActiveSet(params[0]);
          document->MergeToOne(Array<int>(params,2));
          server.SendOK();
      }
      else
        server.SendError("Invalid parameters");
    }
    else if (server.TestCommand("DELETEITEM_+([0-9]+)"))
    {
      int item=server.GetSubExpInt();
      TextureSet *set=document->GetTextureSet(view->GetActSet());
      if (item>=0 && item<set->Size())
      {
        set->Delete(item);
        server.SendOK();
      }
      else
        server.SendError("Invalid set index");
    }
    else if (server.TestCommand("NITEMS"))
    {
      char tmp[50];
      TextureSet *set=document->GetTextureSet(view->GetActSet());
      server.SendOK(itoa(set->Size(),tmp,10));
    }
    else if (server.TestCommand("INFOITEM_+([0-9]+)(\\[\\])?"))
    {
      TextureSet *set=document->GetTextureSet(view->GetActSet());
      bool scriptForm=server.GetSubExpLen(2)!=0;
      int item=server.GetSubExpInt();
      if (item>=0 && item<set->Size())
      {
        const char *form;
        if (scriptForm)
        {
          server.SendOK(quiet?NULL:"ScriptForm: [name,x,y,scale,angle,inportance,w,h,chksum]");
          form="[\"%s\",%d,%d,%g,%d,%g,%d,%d,\"%08X\"];\r\n";
        }
        else
        {
          server.SendOK(quiet?NULL:"Information follows");
          form="NAME %s\r\n"
                "X %d\r\n"
                "Y %d\r\n"
                "SCALE %g\r\n"
                "ANGLE %d\r\n"
                "IMPORTANCE %g\r\n"
                "W %d\r\n"
                "H %d\r\n"
                "CHKSUM %08X\r\n";
        }
        TextureItem &iteminfo=(*set)[item];
        server.SendFormatted(form,
          iteminfo.GetTexture(),
          iteminfo.x,
          iteminfo.y,
          iteminfo.GetScaleCoef(),
          iteminfo.angle*90,
          iteminfo.importance,
          iteminfo.w,
          iteminfo.h,
          iteminfo.checksum);
        server.SendDot();
      }
      else
        server.SendError("Invalid set index");      
    }
    else if (server.TestCommand("TRANSFORM_+([^\r\n]+)"))
    {
      RString fname(server.GetSubExp(),server.GetSubExpLen());
      TextureSet *set;
      int item;
      if (document->FindTexture(fname,set,item))
      {
        float transform[3][2];
        set->CalculateTransform(item,transform);
        server.SendOK("Transform follows");
        CString mpath=document->GetMergedPath();
        if (mpath.Right(1)!="\\") mpath+="\\";
        CString name=mpath+(LPCTSTR)(set->_name)+(document->_tgaOutput?".tga":".paa");
        server.SendLine(name);
        for (int i=0;i<3;i++)
        server.SendFormatted("%g %g\r\n",transform[i][0],transform[i][1]);
        server.SendDot();
      }
      else
        server.SendError("Texture has not been found");
    }
    else 
      if (server.TestCommand("TRANSFORMBACK_+([-+]?[0-9]*[.]?[0-9]+([ed][-+]?[0-9]+)?)_*[,]_*"
      "([-+]?[0-9]*[.]?[0-9]+([ed][-+]?[0-9]+)?)_*[,]_*([^\r\n]+)"))
    {
       float u=server.GetSubExpDouble(1);
       float v=server.GetSubExpDouble(3);
       RString fname(server.GetSubExp(5),server.GetSubExpLen(5));
       Pathname pthnm=fname;
       int set=document->FindByMergedTexture(pthnm.GetTitle());
       if (set!=-1)
       {
         TextureSet *pset=document->GetTextureSet(set);
         int idx=pset->FindTextureByUV(u,v);
         if (idx!=-1)
         {
            float transform[3][2];
            pset->CalculateTransform(idx,transform);
            Matrix4P mx(transform[0][0],transform[1][0],0,transform[2][0],
                        transform[0][1],transform[1][1],0,transform[2][1],
                        0,0,1,0);
            Matrix4P inv;
            inv.SetInvertGeneral(mx);
            server.SendOK("Transform follows");
            server.SendLine((*pset)[idx].GetTexture());
            for (int i=0;i<4;i++) if (i!=2)
              server.SendFormatted("%g %g\r\n",inv(0,i),inv(1,i));
            server.SendDot();
         }
         else
           server.SendError("Invalid UV coordinates (no texture matches)");
       }
       else
          server.SendError("Merged texture has not been found in PTM");
    }
    else if (server.TestCommand("SETSIZE_+([0-9]+),([0-9]+)"))
    {
      int width=server.GetSubExpInt(1);
      int height=server.GetSubExpInt(2);
      if (Testpow2(width) && Testpow2(height))
      {
        TextureSet *set=document->GetTextureSet(view->GetActSet());
        set->_w=width;
        set->_h=height;
        server.SendOK();
      }
      else
        server.SendError("Invalid size");
    }
    else if (server.TestCommand("SETTEX_+([^\r\n]+)"))
    {
      RString fname(server.GetSubExp(),server.GetSubExpLen());
      TextureSet *set=document->GetTextureSet(view->GetActSet());
      set->_name=fname;
      server.SendOK();
    }
    else if (server.TestCommand("INFOSET(\\[\\])?"))
    {
      bool scriptform=server.GetSubExpLen()!=0;
      const char *form;
      if (scriptform)
      {
        server.SendOK(quiet?NULL:"ScriptForm: [name,width,height,alpha]");
        form="[\"%s\",%d,%d,%s];\r\n";
      }
      else
      {
        form="NAME %s\r\nW %d\r\nH %d\r\nALPHA %s\r\n";
        server.SendOK(quiet?NULL:"Information follows");
      }

      TextureSet *set=document->GetTextureSet(view->GetActSet());
      server.SendFormatted(form,set->_name.Data(),set->_w,set->_h,set->_alpha?"true":"false");
      server.SendDot();
    }
    else if (server.TestCommand("FOREACHSET_+([^\r\n]+)"))
    {
      server.SendOK("Batch start");
      RString text(server.GetSubExp(),server.GetSubExpLen());      
      MultipleLines(text.MutableData());
      server.Ack();
      RString tmp;
      char *buff=tmp.CreateBuffer(text.GetLength()+250);
      int cnt=document->GetTextureSetCount();
      for (int i=0;i<cnt;i++)
      {        
        sprintf(buff,text,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i);
        strcat(buff,"\r\nQuit\r\n");
        server.Push(buff,strlen(buff));
        view->SetActiveSet(i);
        ServerMode(server,true);
        if (cnt>document->GetTextureSetCount()) i--,cnt=document->GetTextureSetCount();
      }
      server.SendOK("Batch end");
    }
    else if (server.TestCommand("FOREACHITEM_+([^\r\n]+)"))
    {
      server.SendOK("Batch start");
      RString text(server.GetSubExp(),server.GetSubExpLen());      
      MultipleLines(text.MutableData());
      server.Ack();
      RString tmp;
      char *buff=tmp.CreateBuffer(text.GetLength()+250);
      TextureSet *curset=document->GetTextureSet(view->GetActSet());
      int cnt=curset->Size();
      for (int i=0;i<cnt;i++)
      {        
        sprintf(buff,text,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i);
        strcat(buff,"\r\nQuit\r\n");
        server.Push(buff,strlen(buff));
        ServerMode(server,true);
        if (cnt>curset->Size()) i--,cnt=curset->Size();
      }
      server.SendOK("Batch end");
    }
    else if (server.TestCommand("ECHO_+([^\r\n]*)"))
    {
      RString text(server.GetSubExp(),server.GetSubExpLen());      
      server.SendLine(text);
    }
    else if (server.TestCommand("TEXPREFIX_+([^\r\n]+)"))
    {
      RString text(server.GetSubExp(),server.GetSubExpLen());      
      CTexMergeApp *app=static_cast<CTexMergeApp *>(AfxGetApp());
      app->SetTexturePath(CString(text.Data()),true);
      server.SendOK();
    }
    else if (server.TestCommand("MERGEPATH_+([^\r\n]+)"))
    {
      RString text(server.GetSubExp(),server.GetSubExpLen());      
      CTexMergeApp *app=static_cast<CTexMergeApp *>(AfxGetApp());
      app->SetMergedPath(CString(text.Data()),true);
      server.SendOK();
    }
    else if (server.TestCommand("SETITEMTEX_+([0-9]+)_*,_*([^\r\n]+)"))
    {
      int index=server.GetSubExpInt(1);
      RString tex(server.GetSubExp(2),server.GetSubExpLen(2));
      TextureSet *set=document->GetTextureSet(view->GetActSet());
      if (index>=0 && index<set->Size())
      {
        TextureItem &iteminfo=(*set)[index];
        iteminfo.SetTexture(tex.Data());
        server.SendOK();
      }
      else
        server.SendError("Invalid texture number");      
    }
    else if (server.TestCommand("SELECTTEX_*(true|false),([^\r\n]+)"))
    {
      RString sel(server.GetSubExp(1),server.GetSubExpLen(1));
      RString name(server.GetSubExp(2),server.GetSubExpLen(2));
      bool sl=stricmp(sel,"true")==0;
      bool ok=false;
      for (int i=0;i<document->GetTextureSetCount();i++)
      {
        for (int j=0;j<document->GetTextureSet(i)->Size();j++)
        {
          TextureItem &itm=document->GetTextureSet(i)->operator[](j);
          if (stricmp(itm.GetTexture(),name)==0)
          {
            itm._selected=sl;
            ok=true;
          }
        }
      }
      if (ok)
        server.SendOK();
      else
        server.SendError("Texture was not found");
    }
    else if (server.Test("\n"))
    {      
      server.SendError("Unknown command");
    }
    else
    {
      continue;
    }
    server.Ack();
  }
}

LRESULT CMainFrame::OnOpenServer(WPARAM wParam,LPARAM lParam)
{  
  CTexMergeDoc *document=static_cast<CTexMergeDoc *>(GetActiveDocument());
  document->_ROCheck.SetServerMode(true);
  if (lParam==NULL && wParam==NULL)
  {
    PipeServer server;
    CloseWindow();
    ServerMode(server);
    OpenIcon();
  }
  else
  {
    HANDLE in=(HANDLE)wParam;
    HANDLE out=(HANDLE)lParam;
    PipeServer server(in,out);
    ServerMode(server);
    CloseHandle(in);
    CloseHandle(out);
  }
  if (IsWindow(*this))
        GetActiveDocument()->UpdateAllViews(NULL);
  document->_ROCheck.SetServerMode(false);
  return 0;
}




void CMainFrame::OnDropFiles(HDROP hDropInfo) 
{

  //SetActiveWindow();      // activate us first !
  UINT nFiles = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);

  CWinApp* pApp = AfxGetApp();

  //ProcessMessage

  for (UINT iFile = 0; iFile < nFiles; iFile++)
  {
    TCHAR szFileName[_MAX_PATH];
    ::DragQueryFile(hDropInfo, iFile, szFileName, _MAX_PATH);
    // check if file can be opened
    const char *ext=strrchr(szFileName,'.');
    if( ext && !strcmpi(ext,".ptm") )
    {
      pApp->OpenDocumentFile(szFileName);
    }
    else
    {
      MSG msg;
      msg.message=WM_USER;
      msg.wParam=0;
      msg.lParam=(LPARAM)szFileName;
      pApp->ProcessMessageFilter(0,&msg);
    }
  }
  ::DragFinish(hDropInfo);
}



void CMainFrame::OnViewServerconsole()
{
  PostMessage(TMM_OPENSERVER,0,0);
}
