#include <malloc.h>
#include <strstream>
#include <el/elementpch.hpp>
#include <el\pathname\pathname.h>
#include ".\gdsourcesafe.h"
#define WIN32_LEAN_AND_MEAN
#define NOATOM
#define NOGDICAPMASKS
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NORASTEROPS
#define NOSCROLL
#define NOSOUND
#define NOSYSMETRICS
#define NOTEXTMETRIC
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOCRYPT
#define NOMCX
#include <Windows.h>
#include <commdlg.h>
#include <El\Scc\MsSccLib.hpp>
#include <El\Scc\MsSccProject.hpp>
#include <EL\Interfaces\IROCheck.hpp>
#include <PROJECTS\Objektiv2\ROCheck.h>
//#include "dialogs.h"



static unsigned char SSSettingsDialog[]=
//Generated from RC file 
{1,0,255,255,0,0,0,0,0,0,0,0,200,8,200,128,14,0,0,0,
0,0,22,1,170,0,0,0,0,0,83,0,99,0,114,0,105,0,112,0,116,
0,32,0,83,0,111,0,117,0,114,0,99,0,101,0,32,0,67,0,111,0,
110,0,116,0,114,0,111,0,108,0,32,0,115,0,101,0,116,0,116,0,105,
0,110,0,103,0,115,0,0,0,8,0,144,1,0,1,77,0,83,0,32,0,
83,0,104,0,101,0,108,0,108,0,32,0,68,0,108,0,103,0,0,0,0,
0,0,0,0,0,0,0,1,0,1,80,7,0,149,0,124,0,14,0,1,0,
0,0,255,255,128,0,79,0,75,0,32,0,45,0,32,0,69,0,110,0,97,
0,98,0,108,0,101,0,32,0,83,0,83,0,32,0,99,0,111,0,109,0,
109,0,97,0,110,0,100,0,115,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,1,80,147,0,149,0,124,0,14,0,2,0,0,0,255,255,
128,0,67,0,97,0,110,0,99,0,101,0,108,0,32,0,45,0,32,0,68,
0,105,0,115,0,97,0,98,0,108,0,101,0,32,0,83,0,83,0,32,0,
99,0,111,0,109,0,109,0,97,0,110,0,100,0,115,0,0,0,0,0,0,
0,0,0,0,0,0,0,3,0,1,80,23,0,20,0,73,0,10,0,232,3,
0,0,255,255,128,0,69,0,110,0,97,0,98,0,108,0,101,0,32,0,67,
0,104,0,101,0,99,0,107,0,32,0,79,0,117,0,116,0,0,0,0,0,
0,0,0,0,0,0,0,0,3,0,1,80,23,0,38,0,67,0,10,0,233,
3,0,0,255,255,128,0,69,0,110,0,97,0,98,0,108,0,101,0,32,0,
67,0,104,0,101,0,99,0,107,0,32,0,73,0,110,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,3,0,1,80,23,0,57,0,98,0,10,0,
234,3,0,0,255,255,128,0,69,0,110,0,97,0,98,0,108,0,101,0,32,
0,71,0,101,0,116,0,32,0,76,0,97,0,116,0,101,0,115,0,116,0,
32,0,86,0,101,0,114,0,115,0,105,0,111,0,110,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,3,0,1,80,23,0,76,0,91,0,10,0,
235,3,0,0,255,255,128,0,69,0,110,0,97,0,98,0,108,0,101,0,32,
0,85,0,110,0,100,0,111,0,32,0,67,0,104,0,101,0,99,0,107,0,
32,0,79,0,117,0,116,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,3,0,1,80,23,0,94,0,105,0,10,0,236,3,0,0,255,255,128,0,
69,0,110,0,97,0,98,0,108,0,101,0,32,0,114,0,101,0,99,0,117,
0,114,0,115,0,105,0,118,0,101,0,32,0,111,0,112,0,101,0,114,0,
97,0,116,0,105,0,111,0,110,0,115,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,7,0,0,80,145,0,7,0,126,0,106,0,255,255,255,255,
255,255,128,0,80,0,114,0,111,0,109,0,112,0,116,0,0,0,0,0,0,
0,0,0,0,0,0,0,9,0,0,80,155,0,20,0,63,0,10,0,76,4,
0,0,255,255,128,0,68,0,105,0,115,0,97,0,98,0,108,0,101,0,32,
0,112,0,114,0,111,0,109,0,112,0,116,0,0,0,0,0,0,0,0,0,
0,0,0,0,9,0,0,80,155,0,45,0,69,0,10,0,77,4,0,0,255,
255,128,0,80,0,114,0,111,0,109,0,112,0,116,0,32,0,67,0,104,0,
101,0,99,0,107,0,32,0,73,0,110,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,9,0,0,80,155,0,70,0,83,0,10,0,78,4,0,0,
255,255,128,0,80,0,114,0,111,0,109,0,112,0,116,0,32,0,67,0,104,
0,101,0,99,0,107,0,32,0,73,0,110,0,47,0,79,0,117,0,116,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,0,0,80,155,0,95,
0,47,0,10,0,79,4,0,0,255,255,128,0,80,0,114,0,111,0,109,0,
112,0,116,0,32,0,97,0,108,0,108,0,0,0,0,0,0,0,0,0,0,
0,0,0,3,0,1,80,84,0,126,0,109,0,10,0,248,3,0,0,255,255,
128,0,79,0,117,0,116,0,112,0,117,0,116,0,32,0,99,0,111,0,109,
0,109,0,97,0,110,0,100,0,115,0,32,0,116,0,111,0,32,0,99,0,
111,0,110,0,115,0,111,0,108,0,101,0,0,0,0,0,0,0,0,0,0,
0,0,0,7,0,0,80,7,0,7,0,129,0,106,0,255,255,255,255,255,255,
128,0,83,0,101,0,116,0,116,0,105,0,110,0,103,0,115,0,0,0,0,
0};

#define Category "SourceSafe"
#define CURDIALOGVARNAME "___current_dialog"

SccFunctions *GdSourceSafe::GScc=NULL;
bool GdSourceSafe::GSccActive=false;

RString GdSourceSafe::lastComment;
static MsSccLib GMsSccLib;
static SccFunctions emptyScc;

#define SSCS_ENABLECHECKOUT 1
#define SSCS_ENABLECHECKIN  2
#define SSCS_ENABLEGETLASTVERSION 4
#define SSCS_ENABLEUNDOCHECKOUT 8
#define SSCS_ENABLERECURSIVE 16
#define SSCS_OUTPUT 0x10000

#define SSPS_DISABLEPROMPT 0
#define SSPS_PROMPTCHECKIN 1
#define SSPS_PROMPTCHECKINOUT 2
#define SSPS_PROMPTALL 3


static unsigned long enableFlags=SSCS_ENABLECHECKOUT|SSCS_ENABLEGETLASTVERSION|SSCS_ENABLEUNDOCHECKOUT|SSCS_ENABLERECURSIVE|SSCS_ENABLECHECKIN;
static unsigned long promptFlags=SSPS_PROMPTCHECKIN;


static LRESULT SSControlDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_INITDIALOG:
    {
      for (int i=0;i<32;i++)
        if (enableFlags & (1<<i)) CheckDlgButton(hWnd,1000+i,BST_CHECKED);
      CheckDlgButton(hWnd,1100+promptFlags,BST_CHECKED);
      return 1;
    }
  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
    case IDOK:
      {      
      for (int i=0;i<32;i++)
        if (IsDlgButtonChecked(hWnd,1000+i)==BST_CHECKED) enableFlags|=(1<<i);
        else enableFlags&=~(1<<i);
      for (int i=0;i<32;i++)
        if (IsDlgButtonChecked(hWnd,1100+i)==BST_CHECKED) promptFlags=i;        
      EndDialog(hWnd,IDOK);
      return 1;
      }
    case IDCANCEL: EndDialog(hWnd,IDCANCEL);return 1;
    }
  }
  return 0;
}
static HWND GetCurrentDialog(const GameState *gs)
{
  GameValue val=gs->VarGet(CURDIALOGVARNAME);
  RString dlgstr=val;
  if (dlgstr[0]==0) return NULL;
  HWND hDlg;
  sscanf(dlgstr,"%x",&hDlg);
  return hDlg;
}


static SccFunctions *GetSS(const char *localFolder=NULL)
{
  if (GdSourceSafe::GScc==NULL)
  {
    GMsSccLib.LoadScc();
    if (!GMsSccLib.IsLoaded())
    {
      GdSourceSafe::GScc=&emptyScc;
    }
    GdSourceSafe::GScc=new MsSccProject(GMsSccLib);    
    char buff[256];
    DWORD size=sizeof(buff);
    GetUserName(buff,&size);
    if (GdSourceSafe::GScc->ChooseProjectEx(NULL,localFolder,NULL,buff)!=0)
    {
      delete GdSourceSafe::GScc;
      GdSourceSafe::GScc=&emptyScc;      
    }
  }
  if (!GdSourceSafe::GSccActive)
  {
    if (DialogBoxIndirect(GetModuleHandle(NULL),(LPDLGTEMPLATE)SSSettingsDialog,NULL,(DLGPROC)SSControlDlgProc)==IDCANCEL)
    {    
      GdSourceSafe::GScc=&emptyScc;
    }
    GdSourceSafe::GSccActive=true;
  }
  return GdSourceSafe::GScc;
}

static bool ControlSS(const char *name, const GameState *gs, GameValuePar oper1, unsigned long flag, unsigned long prompt)
{
rep:
  GetSS();
  //no rules on empty scc
  if (GdSourceSafe::GScc==&emptyScc) return false;
  if ((enableFlags & flag)!=flag) return false;
  if (prompt<=promptFlags) 
  {
    HWND hWnd=GetCurrentDialog(gs);
    char buff[1024];
    _snprintf(buff,sizeof(buff),"%s %s",name,oper1.GetText().Data());
    int id=MessageBox(hWnd,buff,name,MB_YESNOCANCEL|MB_ICONQUESTION|MB_SYSTEMMODAL);
    if (id==IDCANCEL)
    {
      GdSourceSafe::GSccActive=false;
      goto rep;
    }
    if (id==IDNO) return false;
  }
  if (enableFlags & SSCS_OUTPUT)
  {
    DWORD written;
    char buff[1024];
    _snprintf(buff,sizeof(buff),"(SourceSafe) %s %s\r\n",name,oper1.GetText().Data());
    HANDLE console=GetStdHandle(STD_OUTPUT_HANDLE);
    WriteFile(console,buff,strlen(buff),&written,NULL);
  }
  return true;
}

static GameValue SSTestRO(const GameState *gs, GameValuePar oper1)
{
  WinROCheck rocheck;
  rocheck.SetScc(enableFlags & SSCS_ENABLECHECKOUT?GetSS():NULL);
  rocheck.hWndOwner=GetCurrentDialog(gs);
  return rocheck.TestFileRO((RString)oper1,ROCHF_DisableSaveAs)==ROCHK_FileOK;
}

static GameValue SSTestROSave(const GameState *gs, GameValuePar oper1)
{
  WinROCheck rocheck;
  rocheck.SetScc(enableFlags & SSCS_ENABLECHECKOUT?GetSS():NULL);
  rocheck.hWndOwner=GetCurrentDialog(gs);
  Pathname pathname=(RString)oper1;
  ROCheckResult res=rocheck.TestFileRO(pathname,0);
  while (res==ROCHK_FileSaveAs)
  {
    OPENFILENAME ofn;
    char fileName[MAX_PATH*4];
    char *filter=(char *)alloca(strlen(pathname.GetExtension())*2+100);
    char *fp=filter;
    sprintf(filter,"Filter (*%s)|*%s|All files (*.*)|*.*|",pathname.GetExtension(),pathname.GetExtension());
    while ((fp=strchr(fp,'|'))!=NULL) *fp++=0;
    strncpy(fileName,pathname,lenof(fileName)-1);
    fileName[lenof(fileName)-1]=0;
    memset(&ofn,0,sizeof(ofn));
    ofn.lStructSize=sizeof(ofn);
    ofn.hwndOwner=rocheck.hWndOwner;
    ofn.hInstance=GetModuleHandle(NULL);
    ofn.lpstrFile=fileName;
    ofn.nMaxFile=lenof(fileName);
    ofn.lpstrFilter=filter;
    ofn.lpstrDefExt=pathname.GetExtension()+1;
    ofn.Flags=OFN_PATHMUSTEXIST|OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY;
    if (GetSaveFileName(&ofn)==TRUE)     
      pathname=fileName;    
    res=rocheck.TestFileRO(pathname,0);
  }
  return res==ROCHK_FileOK?RString(pathname):RString("");
}

#define PREPAREARRAY(oper,larr,size) \
   const GameArrayType &arr=oper; \
   int size=arr.Size(); \
   const char **larr=(const char **)alloca((size)*sizeof(char *)); \
   for (int i=0;i<size;i++) larr[i]=(RString)arr[i];   

static GameValue SSCheckOut(const GameState *gs, GameValuePar oper1)
{
  if (!ControlSS("CheckOut",gs,oper1,SSCS_ENABLECHECKOUT, SSPS_PROMPTCHECKINOUT)) return true;
  if (oper1.GetType()==GameArray)
  {
    PREPAREARRAY(oper1,larr,size);
    return GetSS()->CheckOut(larr,size,GdSourceSafe::GetLastComment())==0;
  }
  else
  {
    return GetSS()->CheckOut((RString)oper1,GdSourceSafe::GetLastComment())==0;
  }
}

static GameValue SSCheckIn(const GameState *gs, GameValuePar oper1)
{
  if (!ControlSS("CheckIn",gs,oper1,SSCS_ENABLECHECKIN,SSPS_PROMPTCHECKIN)) return true;
  if (oper1.GetType()==GameArray)
  {
    PREPAREARRAY(oper1,larr,size);
    return GetSS()->CheckIn(larr,size,GdSourceSafe::GetLastComment())==0;
  }
  else
  {
    return GetSS()->CheckIn((RString)oper1,GdSourceSafe::GetLastComment())==0;
  }
}

static GameValue SSAdd(const GameState *gs, GameValuePar oper1)
{
  if (!ControlSS("Add",gs,oper1,SSCS_ENABLECHECKIN,SSPS_PROMPTCHECKIN)) return true;
  if (oper1.GetType()==GameArray)
  {
    PREPAREARRAY(oper1,larr,size);
    return GetSS()->Add(larr,size,GdSourceSafe::GetLastComment())==0;
  }
  else
  {
    return GetSS()->Add((RString)oper1,GdSourceSafe::GetLastComment())==0;
  }
}

static GameValue SSGetLatestVersion(const GameState *gs, GameValuePar oper1)
{
  if (!ControlSS("GetLatestVersion",gs,oper1,SSCS_ENABLEGETLASTVERSION,SSPS_PROMPTALL)) return true;
  if (oper1.GetType()==GameArray)
  {
    PREPAREARRAY(oper1,larr,size);
    return GetSS()->GetLatestVersion(larr,size)==0;
  }
  else
  {
    return GetSS()->GetLatestVersion((RString)oper1)==0;
  }
}

static GameValue SSGetLatestVersionDir(const GameState *gs, GameValuePar oper1)
{
  if (!ControlSS("GetLatestVersionDir",gs,oper1,SSCS_ENABLEGETLASTVERSION,SSPS_PROMPTALL)) return true;
    return GetSS()->GetLatestVersionDir((RString)oper1)==0;
}

static GameValue SSGetLatestVersionDirRecursive(const GameState *gs, GameValuePar oper1)
{
  if (!ControlSS("GetLatestVersionDirRecursive",gs,oper1,SSCS_ENABLEGETLASTVERSION|SSCS_ENABLERECURSIVE,SSPS_PROMPTALL)) return true;
    return GetSS()->GetLatestVersionDir((RString)oper1,true)==0;
}

static GameValue SSUndoCheckOut(const GameState *gs, GameValuePar oper1)
{
  if (!ControlSS("UndoCheckOut",gs,oper1,SSCS_ENABLEUNDOCHECKOUT,SSPS_PROMPTALL)) return true;
  if (oper1.GetType()==GameArray)
  {
    PREPAREARRAY(oper1,larr,size);
    return GetSS()->UndoCheckOut(larr,size)==0;
  }
  else
  {
    return GetSS()->UndoCheckOut((RString)oper1)==0;
  }
}

static GameValue SSHistory(const GameState *gs, GameValuePar oper1)
{
  if (oper1.GetType()==GameArray)
  {
    PREPAREARRAY(oper1,larr,size);
    return GetSS()->History(larr,size)!=0;
  }
  else
  {
    return GetSS()->History((RString)oper1)!=0;
  }
}

static GameValue SSComment(const GameState *gs, GameValuePar oper1)
{
  GdSourceSafe::lastComment=oper1;
  return true;
}

static GameValue SSIsControled(const GameState *gs, GameValuePar oper1)
{  
  return GetSS()->UnderSSControl((RString)oper1);
}

static GameValue SSIsControledNotDeleted(const GameState *gs, GameValuePar oper1)
{  
  return GetSS()->UnderSSControlNotDeleted((RString)oper1);
}

static GameValue SSIsDeleted(const GameState *gs, GameValuePar oper1)
{  
  return GetSS()->Deleted((RString)oper1);
}

static GameValue SSIsCheckedOut(const GameState *gs, GameValuePar oper1)
{  
  return GetSS()->CheckedOut((RString)oper1);
}

static GameValue SSRemove(const GameState *gs, GameValuePar oper1)
{  
  if (!ControlSS("Remove",gs,oper1,SSCS_ENABLECHECKIN,SSPS_PROMPTCHECKIN)) return true;
  if (oper1.GetType()==GameArray)
  {
    PREPAREARRAY(oper1,larr,size);
    return GetSS()->Remove(larr,size,GdSourceSafe::GetLastComment())!=0;
  }
  else
  {
    return GetSS()->Remove((RString)oper1,GdSourceSafe::GetLastComment())!=0;
  }
}

static GameValue SSBrowse(const GameState *gs, GameValuePar oper1)
{  
  bool dobrowse=true;
  RString result;
  if (GdSourceSafe::GScc==NULL) dobrowse=false;   //project not open, so user also must select project firsttime
  if (GetSS((RString)oper1)==&emptyScc) return RString("");      //we have no SS - cancel request
  if (dobrowse)
  {
    SccFunctions *clone=GetSS()->Clone();
    if (clone->ChooseProjectEx(NULL,(RString)oper1)==0) 
      result=clone->GetLocalPath();
    delete clone;    
  }
  else
    result=GetSS()->GetLocalPath();
  return result;
}

static GameValue SSGetProjectFolder(const GameState *gs)
{  
  return RString(GetSS()->GetLocalPath());
}

static GameValue SSIsActive(const GameState *gs)
{  
  return GdSourceSafe::GScc!=NULL;
}

static GameValue SSIsCanceled(const GameState *gs)
{  
  return GdSourceSafe::GScc==&emptyScc;
}

static GameValue SSLogin(const GameState *gs, GameValuePar oper1)
{
  const GameArrayType &arr=oper1;
  if (GdSourceSafe::GScc!=0 || GdSourceSafe::GSccActive) return false;
  if (arr.Size()==0)
  {
    return GetSS()!=0;
  }
  if (arr.Size()<3 || arr[0].GetType()!=GameString || arr[1].GetType()!=GameString || arr[2].GetType()!=GameString)
  {
    gs->SetError(EvalType);
    return false;
  }
  if (arr.Size()>3 && arr[3].GetType()!=GameString)
  {
    gs->SetError(EvalType);
    return false;
  }

  RString database=arr[0];
  RString project=arr[1];
  RString folder=arr[2];
  RString username;
  if (arr.Size()<4)
  {
    DWORD sz=0;
    GetUserName(0,&sz);    
    GetUserName(username.CreateBuffer(sz),&sz);
  }
  else
  {
    username=arr[3];
  }

  GMsSccLib.LoadScc();
  if (!GMsSccLib.IsLoaded())
  {
    GdSourceSafe::GScc=&emptyScc;
    return false;
  }
  GdSourceSafe::GScc=new MsSccProject(GMsSccLib);    
  promptFlags=0;
  if (project[0])
    project=RString(RString("\"",project),"\"");
  bool ok=GdSourceSafe::GScc->Open(project,folder,database,username)==0;
  if (ok)
  { 
    GdSourceSafe::GSccActive=ok;
    return true;
  }
  else
  {
   GdSourceSafe::GScc=0;
   return false;
  }  
}

static GameValue SSRename(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  if (!ControlSS("Rename",gs,oper1,SSCS_ENABLECHECKIN,SSPS_PROMPTCHECKIN)) return true;
  RString o1=oper1;
  RString o2=oper2;
  return GetSS()->Rename(o1,o2);
}

#define OPERATORS_DEFAULT(XX, Category) \
  XX(GameBool, "SSRename",function, SSRename,GameString, GameString, "oldName","newName", "Renames object in source safe. It doesn't rename local file, only file in database", "\"p:\data\dopis.txt\" SSRename \"p:\data\email.txt\"", "true = success", "", "", Category) \


#define FUNCTIONS_DEFAULT(XX, Category) \
  XX(GameBool, "SSTestRO", SSTestRO, GameString, "fname", "Function returns true, if file is R/W. If file is R/O, it popups dialog for user action, also enables to user Check Out the file.", "SSTestRO \"file.txt\"", "", "", "", Category) \
  XX(GameString, "SSTestROSave", SSTestROSave, GameString, "fname", "Function returns parameter, if file is R/W. If file is R/O, it popups dialog for user action, also enables to user Check Out the file. This funcion is designed to Save operation. If user cancels dialog, result is empty string. If user select SaveAs, it returns entered name.", "SSTestROSave \"file.txt\"", "", "", "", Category) \
  XX(GameBool, "SSAdd", SSAdd, GameString, "fname", "Adds files to source control. Returns true, if it was successful", "SSAdd \"file.txt\"", "", "", "", Category) \
  XX(GameBool, "SSAdd", SSAdd, GameArray, "[fnames]", "Adds file to source control. Returns true, if it was successful", "SSAdd [\"file1.txt\",\"file1.txt\"]", "", "", "", Category) \
  XX(GameBool, "SSCheckOut", SSCheckOut, GameString, "fname", "Checks out file. Returns true, if it was successful", "SSCheckOut \"file.txt\"", "", "", "", Category) \
  XX(GameBool, "SSCheckOut", SSCheckOut, GameArray, "[fnames]", "Checks out files. Returns true, if it was successful", "SSCheckOut [\"file1.txt\",\"file1.txt\"]", "", "", "", Category) \
  XX(GameBool, "SSCheckIn", SSCheckIn, GameString, "fname", "Checks in file. Returns true, if it was successful", "", "", "", "", Category) \
  XX(GameBool, "SSCheckIn", SSCheckIn, GameArray, "[fnames]", "Checks in files. Returns true, if it was successful", "", "", "", "", Category) \
  XX(GameBool, "SSGetLatestVersion", SSGetLatestVersion, GameString, "fname", "Gets latest version of file", "", "", "", "", Category) \
  XX(GameBool, "SSGetLatestVersion", SSGetLatestVersion, GameArray, "[fname]", "Gets latest version of files", "", "", "", "", Category) \
  XX(GameBool, "SSGetLatestVersionDir",SSGetLatestVersionDir, GameString, "directory", "Gets latest version of directory", "", "", "", "", Category) \
  XX(GameBool, "SSGetLatestVersionDirRecursive", SSGetLatestVersionDirRecursive, GameString, "directory", "Gets latest version of directory and all subdirectories", "", "", "", "", Category) \
  XX(GameBool, "SSUndoCheckOut", SSUndoCheckOut, GameString, "file", "Undoes check out", "", "", "", "", Category) \
  XX(GameBool, "SSUndoCheckOut", SSUndoCheckOut, GameArray, "[files]", "Undoes check out", "", "", "", "", Category) \
  XX(GameBool, "SSHistory", SSHistory,GameString, "[files]", "Function shows history of files. Returns true, if user select another version of file", "", "", "", "", Category) \
  XX(GameBool, "SSHistory", SSHistory,GameArray, "file", "Function shows history of file. Returns true, if user select another version of file", "", "", "", "", Category) \
  XX(GameBool, "SSComment", SSComment,GameString, "comment", "Sets comment for next CheckIn/CheckOut operation. Comment is reset after CheckIn/CheckOut", "", "", "", "", Category) \
  XX(GameBool, "SSIsControled", SSIsControled,GameString, "file", "Returns true, if file is controled", "", "", "", "", Category) \
  XX(GameBool, "SSIsControledNotDeleted", SSIsControledNotDeleted,GameString, "file", "Returns true, if file is controled and not deleted", "", "", "", "", Category) \
  XX(GameBool, "SSIsCheckedOut", SSIsCheckedOut,GameString, "file", "Returns true, if file is checked out", "", "", "", "", Category) \
  XX(GameBool, "SSIsDeleted", SSIsDeleted,GameString, "file", "Returns true, if file is deleted", "", "", "", "", Category) \
  XX(GameBool, "SSRemove", SSRemove,GameString, "file", "Removes file form source control", "", "", "", "", Category) \
  XX(GameBool, "SSRemove", SSRemove,GameArray, "[files]", "Removes files form source control", "", "", "", "", Category) \
  XX(GameString, "SSBrowse", SSBrowse,GameString, "path", "Opens source save browser. Function returns selected path or empty string, if dialog has been canceled. Parameter path is default path selected in browser", "", "", "", "", Category) \
  XX(GameBool, "SSLogin", SSLogin, GameArray, "[database, project, working_folder (,username)]","","", "", "", "", Category) \


#define NULARS_DEFAULT(XX, Category) \
  XX(GameString, "SSGetProjectFolder", SSGetProjectFolder, "Returns default Source Safe project folder.", "", "", "", "", Category) \
  XX(GameString, "SSIsActive", SSIsActive, "Returns true, SourceControl is initialized", "", "", "", "", Category) \
  XX(GameString, "SSIsCanceled", SSIsCanceled, "Returns true, if SourceControl is canceled. In this state, all SS commands are ignored", "", "", "", "", Category) \

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


#define TYPES_DEFAULT(XX, Category) \

/*static GameTypeType TypeDef[]=
{
    TYPES_DEFAULT(REGISTER_TYPE,Category)
};*/

#if DOCUMENT_COMREF
static ComRefFunc DefaultComRefFunc[] =
{
  NULARS_DEFAULT(COMREF_NULAR, Category)
  FUNCTIONS_DEFAULT(COMREF_FUNCTION, Category)
  OPERATORS_DEFAULT(COMREF_OPERATOR, Category)
};

/*static ComRefType DefaultComRefType[] =
{
  TYPES_DEFAULT(COMREF_TYPE, Category)
/*  TYPES_DEFAULT_COMB(COMREF_TYPE, "Default")
};*/
#endif

void GdSourceSafe::RegisterToGameState(GameState *gState)
{
  //gState->NewType(TypeDef[0]);
  gState->NewOperators(BinaryFuncts,lenof(BinaryFuncts));
  gState->NewFunctions(UnaryFuncts,lenof(UnaryFuncts));
  gState->NewNularOps(NularyFuncts,lenof(NularyFuncts));
#if DOCUMENT_COMREF
  gState->AddComRefFunctions(DefaultComRefFunc,lenof(DefaultComRefFunc));
//  gState->AddComRefTypes(DefaultComRefType,lenof(DefaultComRefType));
#endif
}
