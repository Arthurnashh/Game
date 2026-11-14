#include <es\common\win.h>
#include <commdlg.h>
#include ".\dialogs.h"
#include <strstream>
#include <malloc.h>
#include <windowsx.h>
#include <commctrl.h>
#include <El\BTree\BTree.h>
#include <shlobj.h>
//#include "globalState.h"

using namespace std;


#pragma comment (lib, "comctl32.lib")

#define Category "dialogs"
#define CURDIALOGVARNAME "___current_dialog"

static int timerIdCounter=1000;

struct WindowInfoStruct
{
  int left;
  int maxcpx;
  int cpxsave;
  int cpysave;
  int linesize;
};

struct DialogDescriptorItem
{
  int uid;    //uid item
  RString variable; //attached variable
  GameArrayType defline;

  DialogDescriptorItem(int uid=0, RString variable=RString(),GameArrayType defline=GameArrayType()):uid(uid),variable(variable),defline(defline) {}

  int Compare(const DialogDescriptorItem& other) const
  {
    return (uid>other.uid)-(uid<other.uid);
  }
  bool operator==(const DialogDescriptorItem& other) const {return Compare(other)==0;}
  bool operator>=(const DialogDescriptorItem& other) const {return Compare(other)>=0;}
  bool operator<=(const DialogDescriptorItem& other) const {return Compare(other)<=0;}
  bool operator!=(const DialogDescriptorItem& other) const {return Compare(other)!=0;}
  bool operator>(const DialogDescriptorItem& other) const {return Compare(other)>0;}
  bool operator<(const DialogDescriptorItem& other) const {return Compare(other)<0;}
  DialogDescriptorItem &operator=(int i) {uid=i;return *this;}
};

struct DialogEvent
{
  enum EventType {OnEnter,OnExit,OnChange, OnClick, OnDblClick, OnRClick, OnRDblClick,OnExitChanged,OnInit, OnExitDlg};
  int uid;  //uid item
  EventType event;
  GameValue code;

  DialogEvent(int uid=0, EventType event=OnEnter, GameValue code=GameValue()):uid(uid),event(event),code(code) {}
  int Compare(const DialogEvent& other) const
  {
    if (event==other.event)  return (uid>other.uid)-(uid<other.uid);
    else return (event>other.event)-(event<other.event);
  }
  bool operator==(const DialogEvent& other) const {return Compare(other)==0;}
  bool operator>=(const DialogEvent& other) const {return Compare(other)>=0;}
  bool operator<=(const DialogEvent& other) const {return Compare(other)<=0;}
  bool operator!=(const DialogEvent& other) const {return Compare(other)!=0;}
  bool operator>(const DialogEvent& other) const {return Compare(other)>0;}
  bool operator<(const DialogEvent& other) const {return Compare(other)<0;}  
};

struct DialogTimerDesc
{
  DWORD timerId;
  GameValue code;
  bool operator==(const DialogTimerDesc& other) const {return timerId==other.timerId;}
  bool operator>=(const DialogTimerDesc& other) const {return timerId>=other.timerId;}
  bool operator<=(const DialogTimerDesc& other) const {return timerId<=other.timerId;}
  bool operator!=(const DialogTimerDesc& other) const {return timerId!=other.timerId;}
  bool operator>(const DialogTimerDesc& other) const {return timerId>other.timerId;}
  bool operator<(const DialogTimerDesc& other) const {return timerId<other.timerId;}
  int operator=(int i) const {return i;}
};

struct DialogDescriptor
{
  BTree<DialogDescriptorItem> items;
  BTree<DialogEvent> events;
  BTree<DialogTimerDesc> timers;
  GameState *gState;  
  bool updated;
  int level;
  int stoplevel;
  int curcontrol;
  DialogDescriptor(): stoplevel(INT_MAX),level(0) {}
};


static RString GetStringVar(const GameState *gs, RString varname)
{
  GameValue val=gs->VarGet(varname);
  if (val.GetNil()) return RString();
  if (val.GetType()!=GameString) return val.GetText();
  else
    return (RString)val;
}

void Scr_SaveValues(HWND hWnd);

static void Scr_UpdateVariable(HWND hDlg, const DialogDescriptor &ddesc, RString name)
{
  BTreeIterator<DialogDescriptorItem> iter(ddesc.items);
  iter.Reset();
  DialogDescriptorItem *item;
  GameValue val=ddesc.gState->VarGet(name);
  while ((item=iter.Next())!=NULL) if (item->variable==name)
  {
    const GameArrayType row=item->defline;
    
    RString type=row[0];
    if (type=="select")
    {
      int sel=toInt(val);
      SendDlgItemMessage(hDlg,item->uid,CB_SETCURSEL,sel,0);
    }
    else if (type=="listbox")
    {
      int sel=toInt(val);
      SendDlgItemMessage(hDlg,item->uid,LB_SETCURSEL,sel,0);
    }
    else if (type=="multi-listbox" || type=="extended-listbox")
    {
      const GameArrayType &arr=val;
      SendDlgItemMessage(hDlg,item->uid,LB_SETSEL,FALSE,-1);
      for (int i=0;i<arr.Size();i++) 
        SendDlgItemMessage(hDlg,item->uid,LB_SETSEL,TRUE,toInt(arr[i]));
    }
    else if (type=="check-box")
    {
      if ((bool)val) CheckDlgButton(hDlg,item->uid,BST_CHECKED);
      else CheckDlgButton(hDlg,item->uid,BST_UNCHECKED);
    }
    else if (type=="radio-button")
    {
      HWND hRadio=GetDlgItem(hDlg,item->uid);
      if (val.IsEqualTo(row[5])) 
        CheckDlgButton(hDlg,item->uid,BST_CHECKED);
      else
        CheckDlgButton(hDlg,item->uid,BST_UNCHECKED);
    }
    else if (type=="htrackbar" || type=="vtrackbar")
    {
      HWND track=GetDlgItem(hDlg,item->uid);
      float min=row[4],max=row[5];
      int imin=0;
      int imax=toInt(row[7]);
      float fval=val;
      int curpos=toInt((fval-min)*(imax-imin)/(max-min));
      SendMessage(track,TBM_SETPOS,TRUE,curpos);
      SetWindowLong(track,GWL_USERDATA,(LONG)&row);
    }
    else if (type=="mclist")
    {
      HWND hWnd=GetDlgItem(hDlg,item->uid);
      const GameArrayType &arr=val;
      for (int i=-1;(i=ListView_GetNextItem(hWnd,i,LVIS_SELECTED))!=-1;)
        ListView_SetItemState(hWnd,i,0,LVIS_SELECTED);
      for (int i=0;i<arr.Size();i++) 
        ListView_SetItemState(hWnd,toInt(arr[i]),LVIS_SELECTED,LVIS_SELECTED);
    }
    else if (type=="progressbar")
    {
      int ival=toInt(val);
      SendMessage(GetDlgItem(hDlg,item->uid),PBM_SETPOS,ival,0);
    }
    else 
      SetDlgItemText(hDlg,item->uid,GetStringVar(ddesc.gState,name));
  }
}


static void Scr_UpdateLists(HWND hDlg, const DialogDescriptor &ddesc, RString name, const GameValue &content)
{
  BTreeIterator<DialogDescriptorItem> iter(ddesc.items);
  iter.Reset();
  DialogDescriptorItem *item;
  GameValue val=ddesc.gState->VarGet(name);
  while ((item=iter.Next())!=NULL) if (item->variable==name)
  {
    const GameArrayType row=item->defline;
    
    RString type=row[0];

    if (type=="combo-edit" || type=="select")
    {
      SendDlgItemMessage(hDlg,item->uid,CB_RESETCONTENT,0,0);
      const GameArrayType &list=content;
      for (int j=0;j<list.Size();j++)
      {
        const char *load=(RString)list[j];
        SendDlgItemMessage(hDlg,item->uid,CB_ADDSTRING,0,(LPARAM)load);
      }
    }
    else if (type=="listbox" || type=="multi-listbox" || type=="extended-listbox")
    {
      SendDlgItemMessage(hDlg,item->uid,LB_RESETCONTENT,0,0);
      const GameArrayType &list=content;
      for (int j=0;j<list.Size();j++)
      {
        const char *load=(RString)list[j];
        SendDlgItemMessage(hDlg,item->uid,LB_ADDSTRING,0,(LPARAM)load);
      }
    }
  }
}


static GameValue EvaluateEventProcedure(GameVarSpace &context, DialogDescriptor &ddesc,HWND hWnd, GameValue code)
{
    char buff[50];
    sprintf(buff,"%x",hWnd);
    context.VarLocal(CURDIALOGVARNAME);
    context.VarSet(CURDIALOGVARNAME,buff,true);
    ddesc.gState->BeginContext(&context);    
    ddesc.level++;    
    GameValue result=ddesc.gState->EvaluateMultiple((RString)code,false);
    if (ddesc.level==ddesc.stoplevel) ddesc.stoplevel=INT_MAX;
    ddesc.level--;
    ddesc.gState->EndContext();
    return result;

}

static bool Scr_CallEvent(HWND hDlg, int uid, DialogEvent::EventType eventType, const GameValue *thisVal=0, bool savedlg=true)
{
  LONG wl=GetWindowLong(hDlg,DWL_USER);
  if (wl==NULL) return false;
  DialogDescriptor &ddesc=*(DialogDescriptor *)wl;
  switch (eventType)
    {
    case DialogEvent::OnEnter: ddesc.curcontrol=uid;ddesc.updated=false;break;
    case DialogEvent::OnChange: if (uid==ddesc.curcontrol) ddesc.updated=true;break;
    case DialogEvent::OnExit: if (uid==ddesc.curcontrol && ddesc.updated) Scr_CallEvent(hDlg,uid,DialogEvent::OnExitChanged);break;
    case DialogEvent::OnExitChanged: ddesc.updated=false;break;
    };
  DialogEvent *dlgEv=ddesc.events.Find(DialogEvent(uid,eventType));
  if (dlgEv!=NULL)
  {
    if (savedlg) Scr_SaveValues(hDlg);
    GameVarSpace context(ddesc.gState->GetContext());
    if (thisVal!=0)
    {
      context.VarLocal("_this");
      context.VarSet("_this",*thisVal,true);
    }
    GameValue result=EvaluateEventProcedure(context,ddesc,hDlg,dlgEv->code);
    if (result.GetType()==GameBool && (bool)result==false) return true;
    if (result.GetType()==GameArray)
    {
      const GameArrayType &resupdate=result;    
      for (int i=0;i<resupdate.Size();i++)
        Scr_UpdateVariable(hDlg,ddesc,resupdate[i]);
      return true;
    }
  }  
  return false;
}

static int Scr_OnInitDialog(HWND hWnd,const DialogDescriptor &initarr)
{
  SetWindowLong(hWnd,DWL_USER,(LONG)&initarr);
  BTreeIterator<DialogDescriptorItem> iter(initarr.items);
  iter.Reset();
  DialogDescriptorItem *item;
  while ((item=iter.Next())!=NULL)
  {
    const GameArrayType row=item->defline;
    RString type=row[0];
    if (type=="textarea") {
         SendDlgItemMessage(hWnd,item->uid,EM_SETLIMITTEXT,32*1024*1024,0);
    }
    if (type=="select")
    {
      const GameArrayType &list=row[4];
      for (int j=0;j<list.Size();j++)
      {
        const char *load=(RString)list[j];
        SendDlgItemMessage(hWnd,item->uid,CB_ADDSTRING,0,(LPARAM)load);
      }
      RString var=item->variable;
      GameValue val=initarr.gState->VarGet(var);
      int sel=toInt(val);
      SendDlgItemMessage(hWnd,item->uid,CB_SETCURSEL,sel,0);
    }
    else if (type=="combo-edit")
    {
      const GameArrayType &list=row[4];
      for (int j=0;j<list.Size();j++)
      {
        const char *load=(RString)list[j];
        SendDlgItemMessage(hWnd,item->uid,CB_ADDSTRING,0,(LPARAM)load);
      }
      RString var=row[3];
      SetDlgItemText(hWnd,item->uid,GetStringVar(initarr.gState,var));
    }
    else if (type=="listbox")
    {
      const GameArrayType &list=row[4];
      for (int j=0;j<list.Size();j++)
      {
        const char *load=(RString)list[j];
        SendDlgItemMessage(hWnd,item->uid,LB_ADDSTRING,0,(LPARAM)load);
      }
      RString var=item->variable;
      GameValue val=initarr.gState->VarGet(var);
      int sel=toInt(val);
      SendDlgItemMessage(hWnd,item->uid,LB_SETCURSEL,sel,0);
    }
    else if (type=="multi-listbox" || type=="extended-listbox")
    {
      const GameArrayType &list=row[4];
      for (int j=0;j<list.Size();j++)
      {
        const char *load=(RString)list[j];
        SendDlgItemMessage(hWnd,item->uid,LB_ADDSTRING,0,(LPARAM)load);
      }
      RString var=item->variable;
      GameValue val=initarr.gState->VarGet(var);
      const GameArrayType &arr=val;
      for (int i=0;i<arr.Size();i++) 
        SendDlgItemMessage(hWnd,item->uid,LB_SETSEL,TRUE,toInt(arr[i]));
    }
    else if (type=="check-box")
    {
      GameValue val=initarr.gState->VarGet(item->variable);
      if ((bool)val) 
        CheckDlgButton(hWnd,item->uid,BST_CHECKED);
    }
    else if (type=="radio-button")
    {
      HWND hRadio=GetDlgItem(hWnd,item->uid);
      GameValue curval=initarr.gState->VarGet(item->variable);
      if (curval.IsEqualTo(row[5])) 
        CheckDlgButton(hWnd,item->uid,BST_CHECKED);
    }
    else if (type=="htrackbar" || type=="vtrackbar")
    {
      HWND track=GetDlgItem(hWnd,item->uid);
      float min=row[4],max=row[5];
      int ticks=toInt(row[6]);
      int imin=0;
      int imax=toInt(row[7]);
      SendMessage(track,TBM_SETRANGE,TRUE,MAKELONG(imin,imax));
      for (int i=0;i<ticks;i++)
      {
        int lpos=i*(imax-imin)/(ticks-1);
        SendMessage(track,TBM_SETTIC,0,lpos);
      }       
      RString var=row[3];
      GameValue gval=initarr.gState->VarGet(var);
      float val=gval;
      int curpos=toInt((val-min)*(imax-imin)/(max-min));
      SendMessage(track,TBM_SETPOS,TRUE,curpos);
      SetWindowLong(track,GWL_USERDATA,(LONG)&row);
    }
    else if (type=="mclist")
    {
      HWND hListview=GetDlgItem(hWnd,item->uid);
      const GameArrayType &list=row[4];
      const GameArrayType &sizes=row[5];
      RECT rc;
      GetClientRect(hListview,&rc);
      rc.right-=GetSystemMetrics(SM_CXVSCROLL)+2*GetSystemMetrics(SM_CXDLGFRAME);
      for (int i=0;i<list.Size();i++)
      {
        LVCOLUMN colmn;
        colmn.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_FMT;
        colmn.pszText=const_cast<char *>(((RString)(list[i])).Data());
        if (sizes.Size()>i)
        {
          float size=sizes[i];
          int isize=0;
          if (size>=0)
          {
            if (size<2)            
              isize=toInt(rc.right*size);
            else
              isize=toInt(size);
          }
          colmn.cx=isize;
        }
        else
        {
          colmn.cx=0;
        }
        colmn.iSubItem=i;
        switch (colmn.pszText[0])
        {
          case '<': colmn.fmt=LVCFMT_LEFT;colmn.pszText++;break;
          case '>': colmn.fmt=LVCFMT_RIGHT;colmn.pszText++;break;
          case '=': colmn.fmt=LVCFMT_CENTER;colmn.pszText++;break;
          default: colmn.fmt=LVCFMT_LEFT;
        }       
        ListView_InsertColumn(hListview,i,&colmn);
      }
      RString var=item->variable;
      GameValue val=initarr.gState->VarGet(var);
      const GameArrayType &arr=val;
      for (int i=0;i<arr.Size();i++) 
        ListView_SetItemState(hListview,toInt(arr[i]),LVIS_SELECTED,LVIS_SELECTED);
      RString flagStr=row.Size()>6?row[6]:"";
      unsigned long flags=LVS_EX_FULLROWSELECT|LVS_EX_LABELTIP;
      flagStr.Upper();
      if (strchr(flagStr,'G')!=NULL) flags|=LVS_EX_GRIDLINES;
      if (strchr(flagStr,'T')!=NULL) flags|=LVS_EX_TRACKSELECT;
      if (strchr(flagStr,'H')!=NULL) flags|=LVS_EX_UNDERLINEHOT;
      if (strchr(flagStr,'C')!=NULL) flags|=LVS_EX_UNDERLINECOLD;
      if (strchr(flagStr,'1')!=NULL) flags|=LVS_EX_ONECLICKACTIVATE;
      if (strchr(flagStr,'2')!=NULL) flags|=LVS_EX_TWOCLICKACTIVATE;
      if (strchr(flagStr,'S')!=NULL) SetWindowLong(hListview,GWL_STYLE,GetWindowLong(hListview,GWL_STYLE)|LVS_SINGLESEL);
      ListView_SetExtendedListViewStyleEx(hListview,flags,flags);
    }
    else if (type=="progressbar")
    {
      GameValue val=initarr.gState->VarGet(item->variable);
      int ival=toInt(val);
      SendMessage(GetDlgItem(hWnd,item->uid),PBM_SETPOS,ival,0);
    }

  }
  SetForegroundWindow(hWnd);
  BringWindowToTop(hWnd);  
  return Scr_CallEvent(hWnd,-1,DialogEvent::OnInit)?1:0;
}

static inline char *TeckaCarka(char *buff)
{
  char *c;
  while ((c=strchr(buff,','))!=NULL) *c='.';
  return buff;
}

static void Scr_SaveValue(HWND hWnd, int id, DialogDescriptor &ddesc)
{
  DialogDescriptorItem *descitem=ddesc.items.Find(DialogDescriptorItem(id));
  if (descitem==NULL) return;
  HWND ctrl=GetDlgItem(hWnd,id);  

  RString name=descitem->variable;
  if (name[0]=='#') return; //special id, skip it
  GameValue var=ddesc.gState->VarGet(name);
  if (name.GetLength()!=0)
  {
    char buff[256];
    bool deftext=false;
    GetClassName(ctrl,buff,sizeof(buff));
    DWORD style=GetWindowLong(ctrl,GWL_STYLE);
    if (_stricmp(buff,"BUTTON")==0)
      if ((style &BS_TYPEMASK)== BS_AUTOCHECKBOX || (style  & BS_TYPEMASK)== BS_CHECKBOX)      
        var=GameValue(Button_GetCheck(ctrl)==BST_CHECKED);
      else if (((style & BS_TYPEMASK)==BS_AUTORADIOBUTTON || (style & BS_TYPEMASK)==BS_RADIOBUTTON))
      {
        if ( Button_GetCheck(ctrl)==BST_CHECKED)
        {
          GameArrayType &arr=descitem->defline;
          var=arr[5];
        }
      }
      else
      {
        deftext=true;
      }
    else if (_stricmp(buff,"COMBOBOX")==0)
      if ((style & 3)== CBS_DROPDOWNLIST)
      {
        int sel=SendMessage(ctrl,CB_GETCURSEL,0,0);
        var=(float)sel;
      }
      else
      {
        deftext=true;
      }
    else if (_stricmp(buff,"LISTBOX")==0)
    {
      const GameArrayType row=descitem->defline;
      RString type=row[0];
      if (type=="multi-listbox" || type=="extended-listbox")
      {
        int total=SendMessage(ctrl,LB_GETSELCOUNT,0,0);
        int *list=new int[total];
        SendMessage(ctrl,LB_GETSELITEMS,total,(LPARAM)list);
        GameArrayType arr;
        for (int i=0;i<total;i++) arr.Append()=(float)(list[i]);
        delete [] list;
        var=arr;
      }
      else
      {
        int sel=SendMessage(ctrl,LB_GETCURSEL,0,0);
        var=(float)(sel);
      }
    }
    else if (_stricmp(buff,WC_LISTVIEW)==0)
    {      
      GameArrayType arr;
      for (int i=-1;(i=ListView_GetNextItem(ctrl,i,LVIS_SELECTED))!=-1;)
        arr.Append()=(float)i;
      var=arr;
    }
    else if (_stricmp(buff,TRACKBAR_CLASS)==0)
    {
      const GameArrayType row=descitem->defline;
      float min=row[4],max=row[5];
      int presnost=toInt(row[7]);
      int pos=SendMessage(ctrl,TBM_GETPOS,0,0);
      var=(max-min)*(float)pos/(float)presnost+min;      
    }
    else if (_stricmp(buff,PROGRESS_CLASS)==0)
    {

    }
    else
      deftext=true;
    if (deftext)
    {
      int len=GetWindowTextLength(ctrl)+1;
      char *buff;
      if (len>65536) buff=(char *)malloc(len);
      else buff=(char *)alloca(len);
      GetWindowText(ctrl,buff,len);
      float fval;      
      if (buff[0]=='#') //vyhodnocení výrazu
      {
        GameVarSpace spc;
        ddesc.gState->BeginContext(&spc);
        var=ddesc.gState->Evaluate(buff+1);
        ddesc.gState->EndContext();
      }
      else
        if (var.GetType()==GameScalar && sscanf(TeckaCarka(buff),"%f",&fval)==1)
          var=fval;              
      else
        var=buff;
      if (len>65536) free(buff);
    }

  ddesc.gState->VarSet(name,var);
  }

}

static void Scr_SaveValues(HWND hWnd)
{
 DialogDescriptor &ddesc=*(DialogDescriptor *)GetWindowLong(hWnd,DWL_USER);
 HWND cur=GetWindow(hWnd,GW_CHILD);
 while (cur) 
 {
   Scr_SaveValue(hWnd,GetDlgCtrlID(cur),ddesc);
   cur=GetWindow(cur,GW_HWNDNEXT);
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


static bool Scr_DoBrowse(HWND hWnd, UINT id)
{
  DialogDescriptor &ddesc=*(DialogDescriptor *)GetWindowLong(hWnd,DWL_USER);
  DialogDescriptorItem *ditem=ddesc.items.Find(DialogDescriptorItem(id));
  if (ditem==NULL || ditem->variable!=RString("#browse")) return false;
  const GameArrayType &arr=ditem->defline;
  RString filter=arr[3];
  bool save=arr.Size()>4?arr[4]:false;
  const char *title = arr.Size()>5 ? cc_cast((RString)arr[5]) : NULL;
  if (filter=="") filter="*.*|*.*|";
  if (filter[filter.GetLength()-1]!='|') filter=filter+"|";
  char *cfilter=filter.MutableData();
  char *zero=strchr(cfilter,'|');
  while (zero) {*zero=0;zero=strchr(zero+1,'|');}

  SRef<char> largebuf=new char[65536];
  GetDlgItemText(hWnd,ditem->uid-1,largebuf,65535);

  if (_stricmp(cfilter,"folders")==0)
  {
    if (PathBrowser(hWnd,largebuf)==false) return false;
  }
  else
  {
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof(ofn));
    ofn.lStructSize=sizeof(ofn);
    ofn.hwndOwner=hWnd;
    ofn.lpstrFilter=cfilter;
    ofn.lpstrFile=largebuf;
    ofn.nMaxFile=65535;
    ofn.lpstrTitle=title;
    ofn.Flags=(save?OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT:0)|OFN_HIDEREADONLY;

    if (save)  
    {
      if (GetSaveFileName(&ofn)==FALSE) return true;
    }
    else
    {
        if (GetOpenFileName(&ofn)==FALSE) return true;
    }
  }
  SetDlgItemText(hWnd,ditem->uid-1,largebuf);
  int sz=strlen(largebuf);
  SendDlgItemMessage(hWnd,ditem->uid-1,EM_SETSEL,sz,sz);
  Scr_CallEvent(hWnd,id-1,DialogEvent::OnExitChanged);
  return true;
}


static void CALLBACK DelayedOnChange(HWND hWnd, UINT nMsg, UINT nIDEvent, DWORD dwTime)
{
  KillTimer(hWnd,nIDEvent);
  Scr_CallEvent(hWnd,nIDEvent,DialogEvent::OnChange);
}


static void Scr_EndDialog(HWND hWnd, UINT res)
{
  LONG wl=GetWindowLong(hWnd,DWL_USER);
  if (wl==NULL) {EndDialog(hWnd,res);}
  DialogDescriptor &ddesc=*(DialogDescriptor *)wl;
  ddesc.stoplevel=0;
  EndDialog(hWnd,res);

}

static HRESULT WINAPI ScriptDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
  case WM_INITDIALOG:
    return Scr_OnInitDialog(hWnd,*(const DialogDescriptor *)lParam);
  case WM_COMMAND:
    {
      int res=LOWORD(wParam);
    if (res<1000)
    {
      if (res!=IDCANCEL)
        Scr_SaveValues(hWnd);
      if (!Scr_CallEvent(hWnd,-1,DialogEvent::OnExitDlg,&GameValue((float)res),false)) Scr_EndDialog(hWnd,res);
    }
    else
    {
      if (HIWORD(wParam)!=BN_CLICKED || Scr_DoBrowse(hWnd,res)==false)
      {
        int uid=res;
        char buff[256];
        bool deftext=false;
        GetClassName(GetDlgItem(hWnd,uid),buff,sizeof(buff));
        if (_stricmp(buff,"EDIT")==0)
        {
            switch (HIWORD(wParam))
            {                      
            case EN_SETFOCUS:Scr_CallEvent(hWnd,uid,DialogEvent::OnEnter);break;
            case EN_KILLFOCUS:Scr_CallEvent(hWnd,uid,DialogEvent::OnExit);break;
            case EN_CHANGE: Scr_CallEvent(hWnd,uid,DialogEvent::OnChange);break;        
            }
        }
        else if (_stricmp(buff,"BUTTON")==0)
        {
          switch (HIWORD(wParam))
          {                      
          case BN_CLICKED: Scr_CallEvent(hWnd,uid,DialogEvent::OnClick);break;
          case BN_SETFOCUS: Scr_CallEvent(hWnd,uid,DialogEvent::OnEnter);break;
          case BN_KILLFOCUS: Scr_CallEvent(hWnd,uid,DialogEvent::OnExit);break;
          case BN_DBLCLK: Scr_CallEvent(hWnd,uid,DialogEvent::OnDblClick);break;
          }
        }
        else if (_stricmp(buff,"LISTBOX")==0)
        {
          switch (HIWORD(wParam))
          {                      
          case LBN_SETFOCUS:Scr_CallEvent(hWnd,uid,DialogEvent::OnEnter);break;
          case LBN_KILLFOCUS:Scr_CallEvent(hWnd,uid,DialogEvent::OnExit);break;
          case LBN_DBLCLK:Scr_CallEvent(hWnd,uid,DialogEvent::OnDblClick);break;
          case LBN_SELCHANGE: Scr_CallEvent(hWnd,uid,DialogEvent::OnChange);break;        
          }
        }
        else if (_stricmp(buff,"COMBOBOX")==0)
        {
          switch (HIWORD(wParam))
          {                      
          case CBN_DBLCLK:Scr_CallEvent(hWnd,uid,DialogEvent::OnDblClick);break;
          case CBN_SETFOCUS:Scr_CallEvent(hWnd,uid,DialogEvent::OnEnter);break;
          case CBN_KILLFOCUS:Scr_CallEvent(hWnd,uid,DialogEvent::OnExit);break;
          case CBN_SELCHANGE:
          case CBN_EDITCHANGE: Scr_CallEvent(hWnd,uid,DialogEvent::OnChange);break;
          case CBN_SELENDOK: Scr_CallEvent(hWnd,uid,DialogEvent::OnClick);break;
          }
        }        
      }

    }
    return 1;
    }
  case WM_DIALOGUNLOCKNOTIFY:
    {
      SetWindowLong(hWnd,DWL_MSGRESULT,1);
      return 1;
    }
  case WM_NOTIFY:
    {
      NMHDR *hdr=(NMHDR *)lParam;
      int uid=wParam;
      switch (hdr->code)
      {
      case NM_CLICK: Scr_CallEvent(hWnd,uid,DialogEvent::OnClick);break;
      case NM_DBLCLK: Scr_CallEvent(hWnd,uid,DialogEvent::OnDblClick);break;
      case NM_RCLICK: Scr_CallEvent(hWnd,uid,DialogEvent::OnRClick);break;
      case NM_RDBLCLK: Scr_CallEvent(hWnd,uid,DialogEvent::OnRDblClick);break;
      case NM_SETFOCUS: Scr_CallEvent(hWnd,uid,DialogEvent::OnEnter);break;
      case NM_KILLFOCUS: Scr_CallEvent(hWnd,uid,DialogEvent::OnExit);break;
      case LVN_ITEMCHANGED:SetTimer(hWnd,uid,1,DelayedOnChange);break;
      }
      return 1;
    }
  case WM_VSCROLL:
  case WM_HSCROLL:
    {
      HWND hScroll=(HWND)lParam;
      if (hScroll) Scr_CallEvent(hWnd,GetDlgCtrlID(hScroll),DialogEvent::OnChange);
      return 1;
    }
  }
  return 0;
}

static void CreateDlgWindow(const char *classname,const char *text,DWORD style, POINT &cp, int xs,int ys,int id, ostrstream &dlgtemplate, WindowInfoStruct &winfo, int paddingx,int paddingy)
{ 
  if (cp.x!=winfo.left && cp.x+2*paddingx+xs>winfo.maxcpx)
  {
    cp.x=winfo.left;
    cp.y+=winfo.linesize;
  }
  DLGITEMTEMPLATE item;
  item.style=style|WS_VISIBLE|WS_CHILD;
  item.dwExtendedStyle=0;
  item.x=(short)(cp.x+paddingx);
  item.y=(short)(cp.y+paddingy);
  item.cx=xs;
  item.cy=ys;
  item.id=id;
  dlgtemplate.write((char *)&item,sizeof(item));
  unsigned short data=0xFFFF;
  if ((int)classname<data)
  {
    dlgtemplate.write((char *)&data,sizeof(data));
    dlgtemplate.write((char *)&classname,2);
  }
  else
  {
    int uclasssz=(strlen(classname)+1)*2;
    LPWSTR uname=(LPWSTR)alloca(uclasssz);
    MultiByteToWideChar(CP_THREAD_ACP,0,classname,strlen(classname)+1,uname,uclasssz);
    dlgtemplate.write((char *)uname,uclasssz);
  }
  {
    int uclasssz=(strlen(text)+1)*2;
    LPWSTR uname=(LPWSTR)alloca(uclasssz);
    MultiByteToWideChar(CP_THREAD_ACP,0,text,strlen(text)+1,uname,uclasssz);
    dlgtemplate.write((char *)uname,uclasssz);
  }
  data=0;
  dlgtemplate.write((char *)&data,sizeof(data));
  int pos=(4-(dlgtemplate.tellp() & 0x3)) & 0x3;
  if (pos) dlgtemplate.write((char *)&data,pos);
  int cy=item.cy+paddingy*2;
  if ((int)classname==0x0085) cy=13;
  if (winfo.linesize<cy)
    winfo.linesize=cy;
  cp.x+=2*paddingx+xs;  
}


TypeIsSimple(WindowInfoStruct);

static GameValue dialogBox(const GameState *gs, GameValuePar oper1)
{
  ostrstream dlgtemplate;
  const GameArrayType &arr=oper1;
  if (arr.Size()<3) {gs->SetError(EvalForeignError,"Invalid parameter count");return GameValue();}
  int xs=toInt(arr[0]);
  int ys=toInt(arr[1]);
  RString title=arr[2];
  if (xs<1 || ys<1) 
    {gs->SetError(EvalForeignError,"Invalid dialog dimensions");return GameValue();}    
  DLGTEMPLATE dlgtmp,*pdlgtmp;
  dlgtmp.cdit=1;
  dlgtmp.cx=xs;
  dlgtmp.cy=ys;
  dlgtmp.dwExtendedStyle=0;
  dlgtmp.style=WS_VISIBLE|WS_DLGFRAME|WS_SYSMENU|WS_CAPTION|WS_OVERLAPPED|DS_3DLOOK|DS_NOFAILCREATE|DS_SETFONT|DS_CENTER;  
  dlgtmp.x=0;
  dlgtmp.y=0;
  dlgtemplate.write((char *)&dlgtmp,sizeof(dlgtmp));
  unsigned short data=0;
  dlgtemplate.write((char *)&data,sizeof(data));
  dlgtemplate.write((char *)&data,sizeof(data));
  int bufsize=(title.GetLength()+1)*2;
  LPWSTR unicodetitle=(LPWSTR )alloca(bufsize);
  MultiByteToWideChar(CP_THREAD_ACP,0,title,title.GetLength()+1,unicodetitle,bufsize);
  dlgtemplate.write((char *)unicodetitle,bufsize);
  data=8;
  dlgtemplate.write((char *)&data,sizeof(data));
  unicodetitle=L"MS Sans Serif";
  dlgtemplate.write((char *)unicodetitle,wcslen(unicodetitle)*2+2);
  int pos=(4-(dlgtemplate.tellp() & 0x3)) & 0x3;
  if (pos) dlgtemplate.write((char *)&data,pos);
  POINT cp={5,5};  
  int items=0;
  int paddingx=1,paddingy=1;
  DialogDescriptor ddesc;
  WindowInfoStruct curwinfo;
  curwinfo.cpxsave=-1;
  curwinfo.cpysave=-1;
  curwinfo.left=5;
  curwinfo.maxcpx=xs-10;
  curwinfo.linesize=0;
  int group=-1;
  int itemid=1000;
  AutoArray<WindowInfoStruct> wstack;
  for (int i=3;i<arr.Size();i++)
  {
    
    if (arr[i].GetType()!=GameArray) 
      {gs->SetError(EvalForeignError,"Invalid dialog item descriptor");return GameValue();}
    const GameArrayType &item=arr[i];
    RString type=item[0];
    if (type=="label" && item.Size()>=5) 
    {
      int align=toInt(item[4]) & 0x7;
      int styles[8]={SS_LEFT|SS_CENTERIMAGE,SS_LEFT,SS_CENTER,SS_RIGHT,SS_LEFT|SS_CENTERIMAGE,SS_CENTER|SS_CENTERIMAGE,SS_CENTER|SS_RIGHT};
      CreateDlgWindow(
        MAKEINTRESOURCE(0x0082),
        (RString)item[3],        
        styles[align]|WS_GROUP,
        cp,
        toInt(item[1]),
        toInt(item[2]),
        -1,
        dlgtemplate, curwinfo, paddingx, paddingy);
      items++;
    }
    else if (type=="dynlabel" && item.Size()>=5) 
    {
      RString var=item[3];
      int align=toInt(item[4]) & 0x7;
      int styles[8]={SS_LEFT|SS_CENTERIMAGE,SS_LEFT,SS_CENTER,SS_RIGHT,SS_LEFT|SS_CENTERIMAGE,SS_CENTER|SS_CENTERIMAGE,SS_CENTER|SS_RIGHT};
      CreateDlgWindow(
        MAKEINTRESOURCE(0x0082),
        GetStringVar(gs,var),
        styles[align]|WS_GROUP,
        cp,
        toInt(item[1]),
        toInt(item[2]),
        itemid,
        dlgtemplate, curwinfo, paddingx, paddingy);
      ddesc.items.Add(DialogDescriptorItem(itemid++,var,item));
      items++;
    }
    else if (type=="padding" || type=="margin")
    {
      paddingx=toInt(item[1]);
      paddingy=toInt(item[2]);
    }
    else if (type=="break")
    {
      cp.x=curwinfo.left;
      cp.y+=curwinfo.linesize;
      curwinfo.linesize=0;
    }
    else if (type=="begin-block")
    {
      wstack.Append()=curwinfo;
//      curwinfo.maxcpx+=curwinfo.left-cp.x;
      curwinfo.left=cp.x;
      group=-1;
    }
    else if (type=="end-block")
    {
      int line=curwinfo.linesize;
      if (wstack.Size()>0) curwinfo=wstack[wstack.Size()-1];
      wstack.Delete(wstack.Size()-1);
      curwinfo.linesize=line;
      group=-1;
    }
    else if (type=="move-to" && item.Size()>=3)
    {
      int x=toInt(item[1]);
      int y=toInt(item[2]);
      cp.x=x;
      cp.y=y;
    }
    else if (type=="move-rel" && item.Size()>=3)
    {
      int x=toInt(item[1]);
      int y=toInt(item[2]);
      cp.x+=x;
      cp.y+=y;
    }
    else if (type=="textline" && item.Size()>=5)
    {
      int xs=toInt(item[1]);
      int ys=toInt(item[2]);
      RString var=item[3];
      int align=toInt(item[4]);
      int styles[4]={ES_LEFT,ES_LEFT,ES_CENTER,ES_RIGHT};
      int style=styles[align & 0x3] | ((align & 0x4)?ES_READONLY:0) | ((align & 0x8)?ES_PASSWORD:0) | ((align & 0x10)?ES_NUMBER:0) |
             ((align & 0x20)?(ES_AUTOHSCROLL|WS_HSCROLL):0);
      CreateDlgWindow(
        MAKEINTRESOURCE(0x0081),
        GetStringVar(gs,var),
        style|ES_AUTOHSCROLL|ES_NOHIDESEL|WS_BORDER|WS_TABSTOP|WS_GROUP,
        cp,
        toInt(xs),
        toInt(ys),
        itemid,
        dlgtemplate, curwinfo, paddingx, paddingy);
      ddesc.items.Add(DialogDescriptorItem(itemid++,var,item));
      items++;
    }
    else if (type=="textarea" && item.Size()>=5)
    {
      int xs=toInt(item[1]);
      int ys=toInt(item[2]);
      RString var=item[3];
      int align=toInt(item[4]);
      int styles[4]={ES_LEFT,ES_LEFT,ES_CENTER,ES_RIGHT};
      int style=styles[align & 0x3] | ((align & 0x4)?ES_READONLY:0) | ((align & 0x8)?ES_PASSWORD:0) | ((align & 0x10)?ES_NUMBER:0) |
             ((align & 0x20)?(ES_AUTOHSCROLL|WS_HSCROLL):0);
                        
      CreateDlgWindow(
        MAKEINTRESOURCE(0x0081),
        GetStringVar(gs,var),
        style|ES_MULTILINE|ES_NOHIDESEL|ES_WANTRETURN|WS_BORDER|WS_TABSTOP|WS_GROUP|ES_AUTOVSCROLL|WS_VSCROLL,
        cp,
        toInt(xs),
        toInt(ys),
        itemid,
        dlgtemplate, curwinfo, paddingx, paddingy);
      ddesc.items.Add(DialogDescriptorItem(itemid++,var,item));
      items++;
    }
    else if (type=="button" && item.Size()>=4)
    {
      int xs=toInt(item[1]);
      int ys=toInt(item[2]);
      RString text=item[3];
      RString name;
      if (item.Size()>=5) name=RString("#",(RString)item[4]);else name=RString("#",(RString)item[3]);
      CreateDlgWindow(
        MAKEINTRESOURCE(0x0080),
        text,
        BS_PUSHBUTTON|BS_CENTER|BS_VCENTER|BS_MULTILINE|(group!=items?(WS_TABSTOP|WS_GROUP):0),
        cp,
        toInt(xs),
        toInt(ys),
        itemid,
        dlgtemplate, curwinfo, paddingx, paddingy);
      ddesc.items.Add(DialogDescriptorItem(itemid++,name,item));
      items++;
      group=items;
    }
    else if (type=="ok-button" && item.Size()>=3)
    {
      int xs=toInt(item[1]);
      int ys=toInt(item[2]);
      CreateDlgWindow(
        MAKEINTRESOURCE(0x0080),
        "OK",
        BS_DEFPUSHBUTTON|BS_CENTER|BS_VCENTER|BS_MULTILINE|(group!=items?(WS_TABSTOP|WS_GROUP):0),
        cp,
        toInt(xs),
        toInt(ys),
        IDOK,
        dlgtemplate, curwinfo, paddingx, paddingy);
      items++;
      group=items;
    }
    else if (type=="cancel-button" && item.Size()>=3)
    {
      int xs=toInt(item[1]);
      int ys=toInt(item[2]);
      CreateDlgWindow(
        MAKEINTRESOURCE(0x0080),
        "Cancel",
        BS_PUSHBUTTON|BS_CENTER|BS_VCENTER|BS_MULTILINE|(group!=items?(WS_TABSTOP|WS_GROUP):0),
        cp,
        toInt(xs),
        toInt(ys),
        IDCANCEL,
        dlgtemplate, curwinfo, paddingx, paddingy);
      items++;
      group=items;
    }
    else if (type=="submit" && item.Size()>=5)
    {
      int xs=toInt(item[1]);
      int ys=toInt(item[2]);
      int id=toInt(item[3]);
      RString text=item[4];
      CreateDlgWindow(
        MAKEINTRESOURCE(0x0080),
        text,
        BS_DEFPUSHBUTTON|BS_CENTER|BS_VCENTER|BS_MULTILINE|(group!=items?(WS_TABSTOP|WS_GROUP):0),
        cp,
        toInt(xs),
        toInt(ys),
        id,
        dlgtemplate, curwinfo, paddingx, paddingy);
      items++;
      group=items;
    }
    else if (type=="browse-button" && item.Size()>=4)
    {
      int xs=toInt(item[1]);
      int ys=toInt(item[2]);      
      CreateDlgWindow(
        MAKEINTRESOURCE(0x0080),
        "Browse",
        BS_PUSHBUTTON|BS_CENTER|BS_VCENTER|BS_MULTILINE|WS_TABSTOP|WS_GROUP,
        cp,
        toInt(xs),
        toInt(ys),
        itemid,
        dlgtemplate, curwinfo, paddingx, paddingy);            
      items++;
      ddesc.items.Add(DialogDescriptorItem(itemid++,"#browse",item));
      group=items;
    }
    else if (type=="check-box" && item.Size()>=5)
    {
      int xs=toInt(item[1]);
      int ys=toInt(item[2]);      
      RString text=item[3];
      RString var=item[4];
      CreateDlgWindow(
        MAKEINTRESOURCE(0x0080),
        text,
        BS_AUTOCHECKBOX|BS_LEFT|BS_VCENTER|BS_MULTILINE|(group!=items?(WS_TABSTOP|WS_GROUP):0),
        cp,
        toInt(xs),
        toInt(ys),
        itemid,
        dlgtemplate, curwinfo, paddingx, paddingy);
      ddesc.items.Add(DialogDescriptorItem(itemid++,var,item));
      items++;
      group=items;
    }
    else if (type=="radio-button" && item.Size()>=6)
    {
      int xs=toInt(item[1]);
      int ys=toInt(item[2]);      
      RString text=item[3];
      RString var=item[4];
      CreateDlgWindow(
        MAKEINTRESOURCE(0x0080),
        text,
        BS_AUTORADIOBUTTON|BS_LEFT|BS_VCENTER|BS_MULTILINE|(group!=items?(WS_TABSTOP|WS_GROUP):0),
        cp,
        toInt(xs),
        toInt(ys),
        itemid,
        dlgtemplate, curwinfo, paddingx, paddingy);
      ddesc.items.Add(DialogDescriptorItem(itemid++,var,item));
      items++;
      group=items;
    }
    else if (type=="select" && item.Size()>=5)
    {
      int xs=toInt(item[1]);
      int ys=toInt(item[2]);      
      RString var=item[3];
      CreateDlgWindow(
        MAKEINTRESOURCE(0x0085),
        "",
        CBS_DROPDOWNLIST|CBS_NOINTEGRALHEIGHT|WS_BORDER|WS_TABSTOP|WS_GROUP|WS_VSCROLL,
        cp,
        toInt(xs),
        toInt(ys),
        itemid,
        dlgtemplate, curwinfo, paddingx, paddingy);
      ddesc.items.Add(DialogDescriptorItem(itemid++,var,item));
      items++;
    }
    else if (type=="combo-edit" && item.Size()>=5)
    {
      int xs=toInt(item[1]);
      int ys=toInt(item[2]);      
      RString var=item[3];
      CreateDlgWindow(
        MAKEINTRESOURCE(0x0085),
        GetStringVar(gs,var),
        CBS_DROPDOWN|CBS_NOINTEGRALHEIGHT|CBS_AUTOHSCROLL|WS_BORDER|WS_TABSTOP|WS_GROUP|WS_VSCROLL,
        cp,
        toInt(xs),
        toInt(ys),
        itemid,
        dlgtemplate, curwinfo, paddingx, paddingy);
      ddesc.items.Add(DialogDescriptorItem(itemid++,var,item));
      items++;
    }
    else if (type=="listbox" && item.Size()>=5)
    {
      int xs=toInt(item[1]);
      int ys=toInt(item[2]);      
      RString var=item[3];
      CreateDlgWindow(
        MAKEINTRESOURCE(0x0083),
        "",
        LBS_USETABSTOPS |LBS_NOINTEGRALHEIGHT|WS_BORDER|WS_TABSTOP|WS_GROUP|WS_HSCROLL|WS_VSCROLL|LBS_NOTIFY ,
        cp,
        toInt(xs),
        toInt(ys),
        itemid,
        dlgtemplate, curwinfo, paddingx, paddingy);
      ddesc.items.Add(DialogDescriptorItem(itemid++,var,item));
      items++;
    }
    else if ((type=="multi-listbox" || type=="extended-listbox") && item.Size()>=5)
    {
      int xs=toInt(item[1]);
      int ys=toInt(item[2]);      
      RString var=item[3];
      CreateDlgWindow(
        MAKEINTRESOURCE(0x0083),
        "",
        LBS_USETABSTOPS |(type=="extended-listbox"?LBS_EXTENDEDSEL:LBS_MULTIPLESEL )|LBS_NOINTEGRALHEIGHT|WS_BORDER|WS_TABSTOP|WS_GROUP|WS_HSCROLL|WS_VSCROLL|LBS_NOTIFY ,
        cp,
        toInt(xs),
        toInt(ys),
        itemid,
        dlgtemplate, curwinfo, paddingx, paddingy);
      ddesc.items.Add(DialogDescriptorItem(itemid++,var,item));
      items++;
    }
    else if (type=="begin-subframe" && item.Size()>=4)
    {      
      int xs=toInt(item[1]);
      int ys=toInt(item[2]);      
      RString text=item[3];
      CreateDlgWindow(
        MAKEINTRESOURCE(0x0080),
        text,
        BS_GROUPBOX|BS_LEFT|WS_GROUP,
        cp,
        toInt(xs),
        toInt(ys),
        -1,
        dlgtemplate, curwinfo, paddingx, paddingy);
      wstack.Append()=curwinfo;
      curwinfo.cpxsave=cp.x;
      curwinfo.cpysave=cp.y;
      cp.x-=xs-paddingx;
      cp.y+=paddingy+10;
      curwinfo.left=cp.x;
      curwinfo.maxcpx=cp.x+xs-10;      
      curwinfo.linesize=0;
      items++;
    }
    else if (type=="end-subframe")
    {
      cp.x=curwinfo.cpxsave;
      cp.y=curwinfo.cpysave;
      if (wstack.Size()>0) curwinfo=wstack[wstack.Size()-1];
      wstack.Delete(wstack.Size()-1);   
      group=-1;
    }
    else if ((type=="vtrackbar" || type=="htrackbar") && item.Size()>=8)
    {
      int xs=toInt(item[1]);
      int ys=toInt(item[2]);      
      RString var=item[3];
      CreateDlgWindow(
        TRACKBAR_CLASS,
        "",
        type=="vtrackbar"?TBS_VERT:TBS_HORZ|WS_GROUP,
        cp,
        toInt(xs),
        toInt(ys),
        itemid,
        dlgtemplate, curwinfo, paddingx, paddingy);
      ddesc.items.Add(DialogDescriptorItem(itemid++,var,item));
      items++;      
    }
  /*
    ["mclist",xs,ys,"variable",[header],[sizes]]
  */
    else if ((type=="mclist") && item.Size()>=5)
    {
      int xs=toInt(item[1]);
      int ys=toInt(item[2]);   
      RString var=item[3];
      CreateDlgWindow(
        WC_LISTVIEW,
        "",
        WS_GROUP|LVS_REPORT|LVS_SHOWSELALWAYS|LVS_NOSORTHEADER|WS_BORDER|WS_TABSTOP|WS_GROUP,
        cp,
        toInt(xs),
        toInt(ys),
        itemid,
        dlgtemplate,curwinfo,paddingx,paddingy);
      ddesc.items.Add(DialogDescriptorItem(itemid++,var,item));
      items++;
    }
    else if (type.Mid(0,2)=="on" && item.Size()>=2) 
    {
      int itmid=itemid-1;
      DialogEvent::EventType event;
      if (type=="onclick") event=DialogEvent::OnClick;
      else if (type=="onenter") event=DialogEvent::OnEnter;
      else if (type=="onexit") event=DialogEvent::OnExit;
      else if (type=="onchange") event=DialogEvent::OnChange;
      else if (type=="ondblclick") event=DialogEvent::OnDblClick;
      else if (type=="onrclick") event=DialogEvent::OnRClick;
      else if (type=="onrdblclick") event=DialogEvent::OnRDblClick;
      else if (type=="onexitchanged") event=DialogEvent::OnExitChanged;
      else if (type=="onexitdlg") {event=DialogEvent::OnExitDlg;itmid=-1;}
      else 
      {
        gs->SetError(EvalForeignError,"Unknown dialog event");
        return GameValue();
      }    

      DialogEvent curev(itmid,event,item[1]);
      DialogEvent *found=ddesc.events.Find(curev);
      if (found!=NULL)  
      {        
        gs->SetError(EvalForeignError,"One event defined twice.");
        return GameValue();
      }    
      ddesc.events.Add(curev);
    }
    else if (type=="init") 
    {
      DialogEvent curev(-1,DialogEvent::OnInit,item[1]);
      DialogEvent *found=ddesc.events.Find(curev);
      if (found!=NULL)  
      {        
        gs->SetError(EvalForeignError,"One event defined twice.");
        return GameValue();
      }    
      ddesc.events.Add(curev);
    }
    else if (type=="minimize-button")
    {
      dlgtmp.style|=WS_MINIMIZEBOX;
      group=-1;
    }
    else if (type=="progressbar" && item.Size()>=4)
    {
      int xs=toInt(item[1]);
      int ys=toInt(item[2]);   
      RString var=item[3];
      int type=item.Size()>4?toInt((float)item[4]):0;
      CreateDlgWindow(
        PROGRESS_CLASS,
        "",
        ((type & 1)?PBS_SMOOTH:0)|((type & 2)?PBS_VERTICAL:0),
        cp,
        toInt(xs),
        toInt(ys),
        itemid,
        dlgtemplate,curwinfo,paddingx,paddingy);
      ddesc.items.Add(DialogDescriptorItem(itemid++,var,item));
      items++;  
    }
    else if (type=="group-separator")
    {
      group=-1;
    }
  }
pdlgtmp=(DLGTEMPLATE *)dlgtemplate.str();
pdlgtmp->cdit=items;
pdlgtmp->style=dlgtmp.style;
InitCommonControls();
ddesc.gState=const_cast<GameState *>(gs);
RString curw=gs->VarGet(CURDIALOGVARNAME);
HWND hWnd=NULL;
if (curw.GetLength()) sscanf(curw,"%x",&hWnd);
int res=DialogBoxIndirectParam(GetModuleHandle(NULL),pdlgtmp,hWnd,(DLGPROC)ScriptDlgProc,(LPARAM)&ddesc);
dlgtemplate.freeze(false);
return (float)res;
}


static GameValue messageBox(const GameState *gs, GameValuePar oper1)
{
  RString curw=gs->VarGet(CURDIALOGVARNAME);
  HWND hWnd=NULL;
  if (curw.GetLength()) sscanf(curw,"%x",&hWnd);

  const GameArrayType &arr=oper1;
  if (arr.Size()<2) {gs->SetError(EvalType);return 0.0f;}
  RString text=arr[0];
  int mode=toInt(arr[1]);
  RString topic=arr.Size()>2?arr[2]:"Message from script.";
  int res=MessageBox(hWnd,text,topic,mode|MB_TASKMODAL);
  return (float)res;
}



static bool GetCurrentDialog(const GameState *gs, HWND *hWnd, DialogDescriptor **ddesc)
{
  GameValue val=gs->VarGet(CURDIALOGVARNAME);
  RString dlgstr=val;
  if (dlgstr[0]==0) 
  {
    gs->SetError(EvalForeignError,"No active dialog");
    return false;
  }
  HWND hDlg;
  sscanf(dlgstr,"%x",&hDlg);
  if (hDlg==0)
  {
    gs->SetError(EvalForeignError,"NULL dialog");
    return false;
  }
  DialogDescriptor *dd;
  LONG wl=GetWindowLong(hDlg,DWL_USER);
  if (wl==0)
  {
    gs->SetError(EvalForeignError,"Current window is not controled by the script");
    return false;
  }
  dd=(DialogDescriptor *)wl;
  if (hWnd) *hWnd=hDlg;
  if (ddesc) *ddesc=dd;
  return true;
}

static GameValue enableControl(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  HWND hWnd;
  if (GetCurrentDialog(gs,&hWnd,NULL)==false) return GameValue();
  HWND cntrl=GetDlgItem(hWnd,toInt(oper1));
  if (cntrl!=NULL)
  {
    GameValue res=(bool)(EnableWindow(cntrl,(bool)oper2)!=FALSE);
    return res;
  }
  return false;
}

static GameValue showControl(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  HWND hWnd;
  if (GetCurrentDialog(gs,&hWnd,NULL)==false) return GameValue();
  HWND cntrl=GetDlgItem(hWnd,toInt(oper1));
  if (cntrl!=NULL)
  {
    GameValue res=(bool)(ShowWindow(cntrl,(bool)oper2?SW_SHOW:SW_HIDE)!=FALSE);
    return res;
  }
  return false;
}

static GameValue getControl(const GameState *gs, GameValuePar oper1)
{
  DialogDescriptor *ddesc;
  if (GetCurrentDialog(gs,NULL,&ddesc)==false) return GameValue();
  GameArrayType arr;
  BTreeIterator<DialogDescriptorItem> iter(ddesc->items);
  DialogDescriptorItem *item;
  iter.Reset();
  while ((item=iter.Next())!=NULL) if (item->variable==oper1)
  {
    arr.Append()=(float)(item->uid);
  }
  return arr;
}

static GameValue UpdateNow(const GameState *gs, GameValuePar oper1)
{
  HWND hWnd;
  DialogDescriptor *ddesc;
  if (GetCurrentDialog(gs,&hWnd,&ddesc)==false) return GameValue();
  Scr_UpdateVariable(hWnd,*ddesc,oper1);
  UpdateWindow(hWnd);
  return true;
}

static GameValue setFocusCtrl(const GameState *gs, GameValuePar oper1)
{
  HWND hWnd;
  if (GetCurrentDialog(gs,&hWnd,NULL)==false) return GameValue();
  HWND cntrl=GetDlgItem(hWnd,toInt(oper1));
  if (cntrl!=NULL)
  {
    SetFocus(hWnd);
    return true;
  }
  return false;
}

static GameValue closeDialog(const GameState *gs, GameValuePar oper1)
{
  HWND hWnd;
  if (GetCurrentDialog(gs,&hWnd,NULL)==false) return GameValue();
  int res=toInt(oper1);
  if (!Scr_CallEvent(hWnd,-1,DialogEvent::OnExitDlg,&GameValue((float)res),false)) Scr_EndDialog(hWnd,res);
  else return false;
  return true;
}

static GameValue waitCursor(const GameState *gs)
{
  SetCursor(::LoadCursor(NULL,IDC_WAIT));
  return true;
}

static GameValue updateList(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  RString varname=oper1;
  HWND hWnd;
  DialogDescriptor *ddesc;
  if (GetCurrentDialog(gs,&hWnd,&ddesc)==false) return false;
  Scr_UpdateLists(hWnd,*ddesc,varname,oper2);  
  UpdateWindow(hWnd);
  return true;
}

static GameValue dlgUpdateMCList(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  RString varname=oper1;const GameArrayType &params=oper2;
  if (params.Size()<1 || params[0].GetType()!=GameArray) 
  {
    gs->SetError(EvalType);
    return GameValue();
  }
  int fromItem=params.Size()>1?toInt(params[1]):-1;
  int count=params.Size()>2?toInt(params[2]):-1;  
  const GameArrayType &data=params[0];
  HWND hWnd;
  DialogDescriptor *ddesc;
  if (GetCurrentDialog(gs,&hWnd,&ddesc)==false) return false;
  BTreeIterator<DialogDescriptorItem> iter(ddesc->items);
  iter.Reset();
  DialogDescriptorItem *item;
  bool oneline=data.Size()!=0 && data[0].GetType()!=GameArray;
  int datasize=oneline?1:data.Size();
  while ((item=iter.Next())!=NULL) if (item->variable==oper1)
  {
    HWND hControl=GetDlgItem(hWnd,item->uid);
    int pos=fromItem;
    int cnt=count;
    if (pos<0) pos=ListView_GetItemCount(hControl);
    if (cnt<0) cnt=ListView_GetItemCount(hControl)-pos;
    while (cnt<datasize)
    {
      LVITEM lvitem;
      lvitem.mask=LVIF_TEXT;
      lvitem.pszText="";
      lvitem.iItem=pos;
      lvitem.iSubItem=0;
      ListView_InsertItem(hControl,&lvitem);
      cnt++;
    }
    for (int i=0;i<datasize;i++)
    {
      const GameArrayType &cols=oneline?data:data[i];
      for (int j=0;j<cols.Size();j++)
      {
        RString text=cols[j].GetType()==GameString?cols[j]:cols[j].GetText();
        ListView_SetItemText(hControl,i+pos,j,const_cast<char *>(text.Data()));
      }
    }
    while (cnt>datasize)
    {
      ListView_DeleteItem(hControl,pos+cnt-1);
      cnt--;
    }
    return (float)ListView_GetItemCount(hControl);
  }  
  return (float)-1;
}

static RString GetTimerName(int id)
{
  RString name;
  sprintf(name.CreateBuffer(15),"Timer%08X",id);
  return name;
}


static GameValue showConsole(const GameState *gs, GameValuePar oper1)
{
  BOOL res;
  bool f=oper1;  
  if (!f) res=FreeConsole();
  else res=AllocConsole();
  return res!=0;
}

static void CALLBACK Scr_ProcessTimer(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
  DialogDescriptor *dd;
  LONG wl=GetWindowLong(hWnd,DWL_USER);
  if (wl==0) {KillTimer(hWnd,idEvent); return;}
  dd=(DialogDescriptor *)wl;

  DialogTimerDesc timerdesc,*fndtimer;
  timerdesc.timerId=idEvent;
  fndtimer=dd->timers.Find(timerdesc);
  if (fndtimer==0) {KillTimer(hWnd,idEvent);return;}

  GameVarSpace gsp(dd->gState->GetContext());
  GameArrayType arr;
  arr.Append()=GameValue(GetTimerName(idEvent));
  arr.Append()=GameValue(dwTime*0.001f);
  gsp.VarLocal("_this");
  gsp.VarSet("_this",arr);
  GameValue ret=EvaluateEventProcedure(gsp,*dd,hWnd,fndtimer->code);
  if ((bool)ret==false) 
  {
    KillTimer(hWnd,idEvent);
    dd->timers.Remove(*fndtimer);
  }
}


static GameValue setTimer(const GameState *gs, GameValuePar oper1)
{
  const GameArrayType &arr=oper1;
  if (arr.Size()!=2 || arr[1].GetType()!=GameCode || arr[0].GetType()!=GameScalar)
  {
    gs->SetError(EvalType);return GameValue();
  }

  HWND hWnd;
  DialogDescriptor *ddesc;
  if (GetCurrentDialog(gs,&hWnd,&ddesc)==false) return GameValue();

  DWORD timeout=toLargeInt((float)arr[0]*1000);  
  if (SetTimer(hWnd,timerIdCounter,timeout,Scr_ProcessTimer)==FALSE) return RString();
  DialogTimerDesc timerdesc;
  timerdesc.timerId=timerIdCounter;
  timerdesc.code=arr[1];
  ddesc->timers.Add(timerdesc);
  timerIdCounter++;
  if (timerIdCounter>0x7FFFFFFF) timerIdCounter=1000;
  return GetTimerName(timerdesc.timerId);  
}

static GameValue unsetTimer(const GameState *gs, GameValuePar oper1)
{
  RString name=oper1;
  DWORD id;
  if (sscanf(name,"Timer%X",&id)!=1) return false;

  HWND hWnd;
  DialogDescriptor *ddesc;
  if (GetCurrentDialog(gs,&hWnd,&ddesc)==false) return GameValue();
  DialogTimerDesc timerdesc;
  timerdesc.timerId=id;;
  ddesc->timers.Remove(timerdesc);
  return KillTimer(hWnd,id)!=0;
}

static GameValue DialogSync(const GameState *gs, GameValuePar oper1)
{
  DialogDescriptor *ddesc;
  HWND hWnd;
  if (GetCurrentDialog(gs,&hWnd,&ddesc)==false) return GameValue();
  BTreeIterator<DialogDescriptorItem> iter(ddesc->items);
  DialogDescriptorItem *item;
  while ((item=iter.Next())!=NULL) if (item->variable==oper1)
    Scr_SaveValue(hWnd,item->uid,*ddesc);
  return GameValue();
}

static GameValue YieldControl(const GameState *gs)
{
  DialogDescriptor *ddesc;
  HWND hWnd;
  if (GetCurrentDialog(gs,&hWnd,&ddesc)==false) return GameValue();

  MSG msg;
  while (PeekMessage(&msg,0,0,0,PM_REMOVE))
  {
    if (!IsDialogMessage(hWnd,&msg))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  return ddesc->stoplevel>ddesc->level;
}

static GameValue StopEvent(const GameState *gs)
{
  DialogDescriptor *ddesc;
  if (GetCurrentDialog(gs,0,&ddesc)==false) return GameValue();
  if (ddesc->level>1)
    ddesc->stoplevel=ddesc->level-1;
  return GameValue();
}


static GameValue StopAllEvents(const GameState *gs)
{
  DialogDescriptor *ddesc;
  if (GetCurrentDialog(gs,0,&ddesc)==false) return GameValue();
  ddesc->stoplevel=1;
  return GameValue();
}

static GameValue delay(const GameState *gs, GameValuePar oper1)
{
  DWORD wait=toLargeInt((float)oper1*1000.0f);
  Sleep(wait);
  return GameValue();
}

static GameValue MoveDlg(const GameState *gs, GameValuePar oper1)
{
  HWND hWnd;
  if (GetCurrentDialog(gs,&hWnd,0)==false) return GameValue();

  const GameArrayType &arr=oper1;
  if (arr.Size()!=2 || arr[0].GetType()!=GameScalar || arr[1].GetType()!=GameScalar)
  {
    gs->SetError(EvalType);return GameValue();
  }
  int xs=toInt(arr[0]);
  int ys=toInt(arr[0]);
  SetWindowPos(hWnd,0,xs,ys,0,0,SWP_NOSIZE|SWP_NOZORDER);
  return GameValue();
}

static GameValue SizeDlg(const GameState *gs, GameValuePar oper1)
{
  HWND hWnd;
  if (GetCurrentDialog(gs,&hWnd,0)==false) return GameValue();

  const GameArrayType &arr=oper1;
  if (arr.Size()!=2 || arr[0].GetType()!=GameScalar || arr[1].GetType()!=GameScalar)
  {
    gs->SetError(EvalType);return GameValue();
  }
  int xs=toInt(arr[0]);
  int ys=toInt(arr[0]);
  SetWindowPos(hWnd,0,0,0,xs,ys,SWP_NOMOVE|SWP_NOZORDER);
  return GameValue();
}

static GameValue MapSize(const GameState *gs, GameValuePar oper1)
{
  HWND hWnd;
  if (GetCurrentDialog(gs,&hWnd,0)==false) return GameValue();

  const GameArrayType &arr=oper1;
  if (arr.Size()!=2 || arr[0].GetType()!=GameScalar || arr[1].GetType()!=GameScalar)
  {
    gs->SetError(EvalType);return GameValue();
  }
  RECT rc={0,0,toInt(arr[0]),toInt(arr[1])};
  MapDialogRect(hWnd,&rc);
  AdjustWindowRectEx(&rc,GetWindowLong(hWnd,GWL_STYLE),GetMenu(hWnd)!=0,GetWindowLong(hWnd,GWL_EXSTYLE));
  GameArrayType res;
  res.Append()=GameValue((float)(rc.right-rc.left));
  res.Append()=GameValue((float)(rc.bottom-rc.top));
  return res;
}

static GameValue UnlockParent(const GameState *gs)
{
  HWND hWnd;
  if (GetCurrentDialog(gs,&hWnd,0)==false) return GameValue();
  HWND hParent=GetWindow(hWnd,GW_OWNER);
  LRESULT res=SendMessage(hParent,WM_DIALOGUNLOCKNOTIFY,(WPARAM)hWnd,(LPARAM)gs);
  if (res!=0) 
  {
    EnableWindow(hParent,TRUE);
  }
  return (res>=0);
}

static GameValue GetDialogRect(const GameState *gs)
{
  HWND hWnd;
  if (GetCurrentDialog(gs,&hWnd,0)==false) return GameValue();
  RECT rc;
  GetWindowRect(hWnd,&rc);
  GameArrayType res;
  res.Append()=GameValue((float)(rc.left));
  res.Append()=GameValue((float)(rc.top));
  res.Append()=GameValue((float)(rc.right-rc.left));
  res.Append()=GameValue((float)(rc.bottom-rc.top));
  return res;
}

static GameValue GetScreenRect(const GameState *gs)
{
  HWND hWnd;
  if (GetCurrentDialog(gs,&hWnd,0)==false) return GameValue();
  HMONITOR hmon=MonitorFromWindow(hWnd,MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi;
  mi.cbSize=sizeof(mi);
  GetMonitorInfo(hmon,&mi);
  GameArrayType res;
  res.Append()=GameValue((float)(mi.rcWork.left));
  res.Append()=GameValue((float)(mi.rcWork.top));
  res.Append()=GameValue((float)(mi.rcWork.right-mi.rcWork.left));
  res.Append()=GameValue((float)(mi.rcWork.bottom-mi.rcWork.top));
  return res;
}


#define OPERATORS_DEFAULT(XX, Category) \
  XX(GameBool, "dlgEnableControl",function, enableControl, GameScalar,GameBool, "controlID","enable", "Enables control in dialog. 'controlID' specified a control identifier, that can be optained by dlgGetControl function. 'enable' can be true=control is enabled or false=control is disabled", "", "", "", "",Category) \
  XX(GameBool, "dlgVisibleControl",function, showControl, GameScalar,GameBool, "controlID","visible", "Changes visibility of a control. 'controlID' specified a control identifier, that can be optained by dlgGetControl function. 'visible' can be true=control is visible or false=control is hidden", "", "", "","", Category) \
  XX(GameBool, "dlgUpdateList",function, updateList, GameString,GameArray, "controlName","_newlist", "Updates list in control - applicable with comboboxes or listboxes. Completly removes all strings from list and loads new list from _newlist variable. NOTE: Current selection will be remove, so don't forget call dlgUpdate.", "", "", "","", Category) \
  XX(GameScalar, "dlgUpdateMCList",function, dlgUpdateMCList, GameString,GameArray, "controlName","[newitems,firstItem,count]", \
    "Function updates block of items in multi-collumn list. Parameters are:<br /><br />"\
    "newitems - array of items. Each item is array of fields, and each field is assigned to column."\
    " Example: [[col1,col2,col3],[col1,col2,col3],...,[col1,col2,col3]]<br />"\
    "firstItem - index of first item to update<br />"\
    "count - count of items to update. The count don't need match the count of new items. List will remove or"\
    " add extra items. Count=0 - Only inserts new items, Count=-1 replaces all items from firstItem till end."\
    " If you specify positive count and newitems is empty, function deletes count items.", "", "Count if items in listView","","",Category) \

#define FUNCTIONS_DEFAULT(XX, Category) \
  XX(GameScalar, "dialogBox", dialogBox, GameArray, "desc", "Starts dialog. Returns ID of button that closes dialog", "","", "", "", Category) \
  XX(GameScalar, "messageBox",messageBox,GameArray, "[text,flags]", "Shows message box with text and buttons. Returns ID of pressed button","","","","",Category) \
  XX(GameArray, "dlgGetControls",getControl,GameString, "controlName/variable", "Returns array of controls' IDs for specified name of variable. One variable can be attached with more controls. If control is named, name is specified starting with '#'","","","","",Category) \
  XX(GameBool, "dlgSetFocus",setFocusCtrl,GameScalar, "controlID", "Sets focus to another control. It triggers 'onexit'('onexitchanged') and 'onenter' events","","","","",Category) \
  XX(GameNothing, "dlgUpdate",UpdateNow,GameString, "variable", "Updates the dialog to reflect value of 'variable'. It is usefull to update dialog before event procedure reach its end. There are little disadvantages: Updating may slow script processing, some events may be triggered in reaction that controls are updated.","","","","",Category) \
  XX(GameBool, "dlgSync",DialogSync,GameString, "variable", "Reads values from dialog and updates value of specified variable. This is done automatically on each event procedure except timers.","","","","",Category) \
  XX(GameBool, "dlgClose",closeDialog,GameScalar, "res", "Closes dialogs. 'res' specified result of dialog. 1 often means OK button, 2 means Cancel button. This command should be the last command in event procedure.","","","","",Category) \
  XX(GameBool, "showConsole",showConsole,GameBool, "mode", "Shows or hides standard console. mode=false console hidden, mode=true console shown. When console is hidden, console IO is unavailable. Note: Opening console stream causes that console is unhidded.","","","","",Category) \
  XX(GameString, "setTimer",setTimer,GameArray, "[timeout,code]", "Installs timer. Timer will call code after timeout period ellapses [in seconds]. Code can be processed once or repeatly. "\
                                                                  "Timer is bind with current dialog and it is automatically destroyed when dialog is closed. "\
                                                                  ,"","Function returns name of timer, which can be used with unsetTimer function.","","",Category) \
  XX(GameBool, "unsetTimer",unsetTimer,GameString, "name", "Removes timer with specified name. Name is returned by setTimer function","","","","",Category) \
  XX(GameNothing, "delay",delay,GameScalar, "time", "Pauses script for specified seconds.","","","","",Category) \
  XX(GameNothing, "dlgMove",MoveDlg,GameArray, "[x,y]", "Moves dialog at position (x,y are specifing new coordinates of left top corner).","","","","",Category) \
  XX(GameNothing, "dlgSize",SizeDlg,GameArray, "[xs,ys]", "Resizes the dialog (xs,ys are specifing new size in pixels including title bar and border).","","","","",Category) \
  XX(GameArray, "dlgMapRect",MapSize,GameArray, "[xs,ys]", "Maps dialog coordinates to pixels. It useful to calculate real dimensions of dialog on screen","","","","",Category) \

#define NULARS_DEFAULT(XX, Category) \
  XX(GameBool, "dlgWaitCursor",waitCursor, "Shows wait cursor during long operation is processed. Wait cursor is changed to normal when event procedure is finished", "", "", "", "", Category) \
  XX(GameBool, "dlgYield",YieldControl, "Checks and processes any user input made on dialog. When all inputs are processed, function returns. Return value is true - event procedure can continue or false - event procedure should exit soon","","","","",Category) \
  XX(GameBool, "dlgStopEvent",StopEvent, "Function notifies latest started event procedure, that it should exit soon. This notify is reported through dlgYield result","","","","",Category) \
  XX(GameBool, "dlgStopAllEvents",StopAllEvents, "Function notifies all started event procedures, that they should exit soon. This notify is reported through dlgYield result","","","","",Category) \
  XX(GameBool, "dlgUnlockParent",UnlockParent, "Function unlocks parent window that allows to user access controls on it. Beware recursion cycles, parent window can allow to open new instance current window again","","","","",Category) \
  XX(GameArray, "dlgGetDialogRect",GetDialogRect, "Returns dimension of current dialog/window [left,top,width,height]","","","","",Category) \
  XX(GameArray, "dlgGetScreenRect",GetScreenRect, "Returns dimension of screen that displayes current dialog [left,top,width,height]","","","","",Category) \


static GameOperator BinaryFuncts[]=
{
  OPERATORS_DEFAULT(REGISTER_OPERATOR, Category)
};

static GameFunction UnaryFuncts[]=
{
  FUNCTIONS_DEFAULT(REGISTER_FUNCTION, Category)
};

static GameNular NularyFuncts[]=
{
  NULARS_DEFAULT(REGISTER_NULAR, Category)
};

/*static GameData *CreateGameFor() {return new gdForClass;}

#define TYPES_DEFAULT(XX, Category) \
  XX("ForType",GameFor,CreateGameFor,"@FORTYPE","ForType",\
    "This type handles for cycles. Usage of this type: for \"_var\" from :expr: to :expr: [step &lt;expr&gt;] do {..code..};"\
    "Second usage: for [\":initPhase:\",\":condition:\",\":updatePhase:\"] do {...code...};",Category)\

static GameTypeType TypeDef[]=
{
    TYPES_DEFAULT(REGISTER_TYPE,Category)
};
*/
#if DOCUMENT_COMREF
static ComRefFunc DefaultComRefFunc[] =
{
  NULARS_DEFAULT(COMREF_NULAR, Category)
  FUNCTIONS_DEFAULT(COMREF_FUNCTION, Category)
  OPERATORS_DEFAULT(COMREF_OPERATOR, Category)
};

/*static ComRefType DefaultComRefType[] =
{
//  TYPES_DEFAULT(COMREF_TYPE, Category)
/*  TYPES_DEFAULT_COMB(COMREF_TYPE, "Default")
};*/
#endif

void GdDialogs::RegisterToGameState(GameState *gState)
{
//  gState->NewType(TypeDef[0]);
  gState->NewOperators(BinaryFuncts,lenof(BinaryFuncts));
  gState->NewFunctions(UnaryFuncts,lenof(UnaryFuncts));
  gState->NewNularOps(NularyFuncts,lenof(NularyFuncts));
#if DOCUMENT_COMREF
  gState->AddComRefFunctions(DefaultComRefFunc,lenof(DefaultComRefFunc));
//  gState->AddComRefTypes(DefaultComRefType,lenof(DefaultComRefType));
#endif
}

void GdDialogs::SetParentWindowImpl(GameState *gState, const char *windowName)
{
  gState->VarLocal(CURDIALOGVARNAME);
  gState->VarSet(CURDIALOGVARNAME,windowName);
}
