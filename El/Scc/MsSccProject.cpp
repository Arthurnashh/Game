#include <windows.h>
#include <malloc.h>
#include "MsSccProject.hpp"

SCCRTN (*MsSccProjectBase::OnTextOut)(const char *message, long msgtype)=NULL;

MsSccProjectBase::MsSccProjectBase(MsSccLib &scc, SccProviderInfo *sinfo, HWND defWindow):
_scc(scc),
_defaultWindow(defWindow),
_projectActive(false),
_enableTextOut(true),
GetActiveWindowCallback(0),
_context(NULL)
{
  if (sinfo==NULL)     
  {
    sinfo=(SccProviderInfo *)alloca(sizeof( SccProviderInfo));    
    sinfo->callerName=NULL;
  }
  if (sinfo->callerName==NULL)
  {
    char *name=(char *)alloca(MAX_PATH);
    GetModuleFileNameA(NULL,name,MAX_PATH);
    sinfo->callerName=name;
  }
  SCCRTN ret=_scc.Initialize(&_context,
    defWindow,
    sinfo->callerName,
    sinfo->sccName,
    &sinfo->sccCaps,
    sinfo->sccDllName,
    &sinfo->maxCheckoutCommentLen,
    &sinfo->maxCommentLen);
  sinfo->result=ret;  
  _user[0]=0;
  _project[0]=0;
  _auxProjPath[0]=0;
  _localPath[0]=0;
}

MsSccProjectBase::~MsSccProjectBase()
{
  if (IsValid())
  {
    if (IsProjectOpened()) CloseProject();
    _scc.Uninitialize(_context);
  }
}

SCCRTN MsSccProjectBase::OpenProject(const char *user,const char *projName, const char *localProjPath, const char *database,HWND hWnd)
{
  if (_projectActive) CloseProject();  
  if (user) strcpy(_user,user);
  else
  {
    DWORD sz=sizeof(_user)/sizeof(*_user);
    GetUserNameA(_user,&sz);
  }
  strcpy(_project,projName?projName:"");
  strcpy(_auxProjPath,database?database:"");
  strcpy(_localPath,localProjPath?localProjPath:"");  
  SCCRTN res=_scc.OpenProject(_context,hWnd?hWnd:DefaultWindow(),_user,_project,_localPath,_auxProjPath,"",(LPTEXTOUTPROC)OnTextOut,0);
  if (res==SCC_OK) _projectActive=true;
  return res;
}

SCCRTN MsSccProjectBase::CloseProject()
{
  SCCRTN res;
  if (_projectActive) 
  {
    res=_scc.CloseProject(_context);
    if (res==SCC_OK) _projectActive=false;
    return res;
  }
  else 
    return SCC_E_UNKNOWNPROJECT;
}

SCCRTN MsSccProjectBase::OpenProjectAskUser(const char *user,  const char *projName,
                                      const char *localPath,const char *database,
                                      bool bAllowChangePath, bool *new_project, HWND hWnd)
{
  if (_projectActive) CloseProject();  
  if (user) strcpy(_user,user);
  else
  {
    DWORD sz=sizeof(_user)/sizeof(*_user);
    GetUserNameA(_user,&sz);
  }
  strcpy(_project,projName?projName:"");
  strcpy(_auxProjPath,database?database:"");
  strcpy(_localPath,localPath?localPath:"");  
  SCCRTN res=_scc.GetProjPath(_context,hWnd?hWnd:DefaultWindow(),
    _user,_project,_localPath,_auxProjPath,(BOOL)bAllowChangePath,(BOOL *)&new_project);
  if (res==SCC_OK)
  {
    SCCRTN res=_scc.OpenProject(_context,hWnd?hWnd:DefaultWindow(),_user,_project,_localPath,_auxProjPath,"",(LPTEXTOUTPROC)OnTextOut,0);
    if (res==SCC_OK) _projectActive=true;
  }
  return res;
}

 SCCRTN MsSccProject::GetLatestVersionDir(const char* local_file_name, bool recursive)
  {
  if ( recursive )
	  return Get(1, (LPCSTR*)&local_file_name, NULL,SCC_GET_ALL|SCC_GET_RECURSIVE, NULL);
  else
	  return Get(1, (LPCSTR*)&local_file_name,NULL, SCC_GET_ALL, NULL);
  }

SccFunctions *MsSccProject::Clone() const
  {
  MsSccLib &provider=GetProvider();
  MsSccProject *newproj=new MsSccProject(provider,NULL,DefaultWindow());
  newproj->SetWindowCallback(this->GetWindowCallback());
  if (!_projectActive || newproj->Open(GetProjName(),GetLocalPath(),GetServerName(),GetUserName())==0) return newproj;
  delete newproj;
  return NULL;
  }