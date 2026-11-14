#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <io.h>
#include <windows.h>
#include <tchar.h>
#include "ROCheck.h"
#include <El\Interfaces\iScc.hpp>

static unsigned char ROCheckDlg[]=
// NOTE: This data were generated from resource file dumping data
// returned by functions Find/Load/LockResource
{192,0,200,128,0,0,0,0,8,0,0,0,0,0,6,1,76,0,0,0,
0,0,82,0,101,0,97,0,100,0,32,0,111,0,110,0,108,0,121,0,32,
0,119,0,97,0,114,0,110,0,105,0,110,0,103,0,0,0,8,0,77,0,
83,0,32,0,83,0,97,0,110,0,115,0,32,0,83,0,101,0,114,0,105,
0,102,0,0,0,1,0,1,80,0,0,0,0,94,0,55,0,50,0,14,0,
236,3,255,255,128,0,67,0,104,0,101,0,99,0,107,0,32,0,79,0,117,
0,116,0,0,0,0,0,1,0,1,80,0,0,0,0,150,0,55,0,50,0,
14,0,1,0,255,255,128,0,83,0,97,0,118,0,101,0,32,0,65,0,115,
0,46,0,46,0,46,0,0,0,0,0,0,0,1,0,1,80,0,0,0,0,
205,0,55,0,50,0,14,0,2,0,255,255,128,0,79,0,107,0,0,0,0,
0,0,0,0,0,2,80,0,0,0,0,39,0,7,0,27,0,8,0,255,255,
255,255,130,0,84,0,104,0,105,0,115,0,32,0,102,0,105,0,108,0,101,
0,58,0,0,0,0,0,0,0,0,8,1,80,0,0,0,0,39,0,17,0,
216,0,22,0,234,3,255,255,129,0,0,0,0,0,0,0,0,0,2,80,0,
0,0,0,7,0,41,0,171,0,8,0,255,255,255,255,130,0,73,0,115,0,
32,0,109,0,97,0,114,0,107,0,101,0,100,0,32,0,97,0,115,0,32,
0,82,0,69,0,65,0,68,0,45,0,79,0,78,0,76,0,89,0,32,0,
97,0,110,0,100,0,32,0,99,0,97,0,110,0,110,0,111,0,116,0,32,
0,98,0,101,0,32,0,111,0,118,0,101,0,114,0,119,0,114,0,105,0,
116,0,116,0,101,0,110,0,33,0,0,0,0,0,0,0,1,80,0,0,0,
0,7,0,55,0,80,0,14,0,233,3,255,255,128,0,69,0,120,0,112,0,
108,0,111,0,114,0,101,0,32,0,102,0,105,0,108,0,101,0,46,0,46,
0,46,0,0,0,0,0,3,0,0,80,0,0,0,0,7,0,7,0,21,0,
20,0,235,3,255,255,130,0,0,0,0,0};

#undef IDC_RUNEXPLORER
#define IDC_RUNEXPLORER 1001

#undef IDC_FILENAME 
#define IDC_FILENAME 1002

#undef IDC_DLGICON
#define IDC_DLGICON 1003

#undef IDC_CHECKOUT
#define IDC_CHECKOUT 1004

static __declspec( thread ) SccFunctions *activeScc=NULL;

struct RODialogInfo
{
  const _TCHAR *szFilename;
  int flags;
  HICON icon;
  SccFunctions *vss;
};


static void RunExplorer(HWND hOwner,_TCHAR *fname)
{
  _TCHAR *params=(_TCHAR *)alloca((_tcslen(fname)+100)*sizeof(_TCHAR));
  _stprintf(params,_T("explorer.exe /select,\"%s\""),fname);
  STARTUPINFO strinfo;
  PROCESS_INFORMATION pinfo;

  memset(&strinfo,0,sizeof(strinfo));
  strinfo.cb=sizeof(strinfo);
  strinfo.dwFlags=0;  
  BOOL ret=CreateProcess(NULL,params,NULL,NULL,FALSE,CREATE_DEFAULT_ERROR_MODE|NORMAL_PRIORITY_CLASS,
    NULL,NULL,&strinfo,&pinfo);
  if (ret==FALSE)
  {
    _TCHAR buff[256];_TCHAR *appname;
    ::GetModuleFileName(NULL,buff,sizeof(buff)/sizeof(_TCHAR));
    appname=_tcsrchr(buff,'\\');
    if (appname==NULL) appname=buff;else ++appname;
    MessageBox(hOwner,_T("Failed create process"),appname,MB_OK|MB_ICONEXCLAMATION);
  }
  else
  {
    SetCursor(LoadCursor(NULL,IDC_WAIT));
    WaitForInputIdle(pinfo.hProcess,30000);
    CloseHandle(pinfo.hProcess);
    CloseHandle(pinfo.hThread);
  }
}

static void CenterWindow(HWND hDlg)
{
  HWND hParent=GetParent(hDlg);
  if (hParent==NULL) hParent=GetDesktopWindow();
  RECT parentRc;
  RECT curRc;
  POINT pt;
  GetWindowRect(hParent,&parentRc);
  ClientToScreen(hParent,(POINT *)(&parentRc));
  ClientToScreen(hParent,(POINT *)(&parentRc)+1);
  GetWindowRect(hDlg,&curRc);
  pt.x=(parentRc.left+parentRc.right)>>1;
  pt.y=(parentRc.top+parentRc.bottom)>>1;
  pt.x-=(curRc.right+curRc.left)>>1;
  pt.y-=(curRc.top+curRc.bottom)>>1;
  SetWindowPos(hDlg,NULL,pt.x,pt.y,0,0,SWP_NOZORDER|SWP_NOSIZE);
}

static LRESULT CALLBACK TestFileRODlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
  case WM_INITDIALOG:
    {
      RODialogInfo *info=(RODialogInfo *)lParam;
      SetDlgItemText(hDlg,IDC_FILENAME,info->szFilename);
      if (info->flags & ROCHF_DisableSaveAs) EnableWindow(GetDlgItem(hDlg,IDOK),FALSE);
      if (info->flags & ROCHF_DisableExploreButton) EnableWindow(GetDlgItem(hDlg,IDC_RUNEXPLORER),FALSE);
      if (info->flags & ROCHF_GetLatestVersion)            
        SetDlgItemText(hDlg,IDOK,"Get Latest");      

      if (info->icon==NULL)
        SendDlgItemMessage(hDlg,IDC_DLGICON,STM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIcon(NULL,IDI_EXCLAMATION));
      else
        SendDlgItemMessage(hDlg,IDC_DLGICON,STM_SETIMAGE,IMAGE_ICON,(LPARAM)info->icon);
      SetWindowLong(hDlg,DWL_USER,(LONG)info->vss);
      bool ssok=info->vss!=NULL && info->vss->Opened() && info->vss->UnderSSControlNotDeleted(info->szFilename);
      EnableWindow(GetDlgItem(hDlg,IDC_CHECKOUT),ssok);
      if (info->flags & ROCHF_GetLatestVersion)      
        EnableWindow(GetDlgItem(hDlg,IDOK),ssok);
      CenterWindow(hDlg);
      MessageBeep(MB_ICONEXCLAMATION);
      return 0;
    }
  case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {
      case IDOK: EndDialog(hDlg,IDOK);return 1;
      case IDCANCEL: EndDialog(hDlg,IDCANCEL);return 1;
      case IDC_RUNEXPLORER:
        {
          HWND hFileName=GetDlgItem(hDlg,IDC_FILENAME);
          int len=GetWindowTextLength(hFileName)+1;
          _TCHAR *fname=(_TCHAR *)alloca(len*sizeof(_TCHAR));
          GetWindowText(hFileName,fname,len);
          RunExplorer(hDlg,fname);
          return 1;
        }
      case IDC_CHECKOUT:
        {
          SccFunctions *vss=(SccFunctions *)GetWindowLong(hDlg,DWL_USER);
          if (vss)
          {
            HWND hFileName=GetDlgItem(hDlg,IDC_FILENAME);
            int len=GetWindowTextLength(hFileName)+1;
            char *fname=(char *)alloca(len);
            GetWindowTextA(hFileName,fname,len);
            //          SetFileAttributesA(fname,GetFileAttributesA(fname) & ~FILE_ATTRIBUTE_READONLY);
            SCCRTN ret=vss->GetLatestVersion(fname);
            if (ret==0) ret=vss->CheckOut(fname);
            if (ret==0) 
            {
              EndDialog(hDlg,IDCANCEL);
              return 1;
            }
            _TCHAR *buff=(_TCHAR *)alloca(256*sizeof(_TCHAR));
            _stprintf(buff,_T("Source Safe: operation failed (err: %d)"),ret);
            EnableWindow(GetDlgItem(hDlg,IDC_CHECKOUT),FALSE);
            MessageBox(hDlg,buff,NULL,MB_ICONEXCLAMATION);
            return 1;
          }

        }
      }
    }    
  }
  return 0;
}

bool GetFileTime(const _TCHAR *filename,FILETIME& ftime)
{
  WIN32_FIND_DATA fnd;
  HANDLE hfnd=FindFirstFile(filename,&fnd);
  if (hfnd!=INVALID_HANDLE_VALUE)
  {
    ftime=fnd.ftLastWriteTime;    
    FindClose(hfnd);
    return true;
  }
  else
  {
    ftime.dwHighDateTime=ftime.dwLowDateTime=0;
    return false;
  }
}

static ROCheckResult TestFileROStatic(HWND hWndOwner, const _TCHAR *szFilename, int flags,HICON hIcon, HINSTANCE hInst )
{
  RODialogInfo nfo;
  nfo.flags=flags;
  nfo.szFilename=szFilename;
  nfo.icon=hIcon;
  nfo.vss=activeScc;
  if (_access(szFilename,0)!=0) return ROCHK_FileOK;  //Not Exists, it is OK
  if (_access(szFilename,06)==0) return ROCHK_FileOK; //File is R/W, it is OK
  if (hInst==NULL) hInst=GetModuleHandle(NULL);
  if (hWndOwner==NULL) hWndOwner=GetActiveWindow();
  if (GetParent(hWndOwner)!=NULL) hWndOwner=GetParent(hWndOwner);
  FILETIME before,after;
  GetFileTime(szFilename,before);
  int ret=::DialogBoxIndirectParam(hInst,(LPCDLGTEMPLATE)ROCheckDlg,hWndOwner,(DLGPROC)TestFileRODlg,(LPARAM)&nfo);
  GetFileTime(szFilename,after);
  bool updated=(flags & ROCHF_TrackFileUpdate)?(memcmp(&before,&after,sizeof(before))!=0):false;
  if (ret==-1) 
  {
    MessageBox(hWndOwner,_T("Application error: ROCheck dialog creation failed"),NULL,MB_OK|MB_ICONSTOP);
    return ROCHK_FileRO;
  }
  if (ret==IDOK) 
  {
    if (flags & ROCHF_GetLatestVersion) return updated?ROCHK_GetLatestVersionUpdated:ROCHK_GetLatestVersion;
    return ROCHK_FileSaveAs;
  }
  if (_access(szFilename,0)!=0) return updated?ROCHK_FileOKUpdated:ROCHK_FileOK;  //Not Exists, it is OK
  if (_access(szFilename,06)==0) return updated?ROCHK_FileOKUpdated:ROCHK_FileOK; //File is R/W, it is OK
  return ROCHK_FileRO;
}

ROCheckResult TestFileRO(HWND hWndOwner, const _TCHAR *szFilename, int flags,HICON hIcon, HINSTANCE hInst)
{
  return TestFileROStatic(hWndOwner,szFilename, flags,hIcon, hInst);
}

void TestFileRO_ActiveSSDB(SccFunctions *vss)
{
  activeScc=vss;
}

ROCheckResult WinROCheck::TestFileRO(const _TCHAR *szFilename, int flags)
{
  activeScc=_scc;
  ROCheckResult res=::TestFileROStatic(hWndOwner,szFilename,flags,hIcon,hInstance);
  activeScc=NULL;
  return res;
}
