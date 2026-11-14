#include <windows.h>

#define SCC_MODULE_HANDLE HMODULE

#include "MsSccLib.hpp"



MsSccLib::MsSccLib(SCCRTN *initresult, const char *dllpath)
{
  _dllPath=NULL;    
  _library=NULL;
  if (initresult)             
    *initresult=LoadScc(dllpath);
}

MsSccLib::~MsSccLib()
{
  free(_dllPath);
  if (_library) FreeLibrary(_library);
}

bool MsSccLib::IsLoaded() 
{
  return _library!=NULL;
}

#define REGISTERFUNCTION(Funct) *((FARPROC *)&Funct)=GetProcAddress(_library,"Scc"#Funct)

void MsSccLib::FindLibrary()
{
  HKEY key;
  char scc_provider[256];
  //find provider
  if ( RegOpenKeyExA(HKEY_LOCAL_MACHINE, STR_SCC_PROVIDER_REG_LOCATION, 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS )
  {
    DWORD size = 256;

    LONG ret = RegQueryValueExA(key, STR_PROVIDERREGKEY, NULL, NULL, (LPBYTE)scc_provider, &size);
    if ( ret == ERROR_SUCCESS )
      RegCloseKey(key);
    else        
      return;
  }
  else
    return;

  //find Scc dll name
  if ( RegOpenKeyExA(HKEY_LOCAL_MACHINE, scc_provider, 0, KEY_QUERY_VALUE, &key)  == ERROR_SUCCESS )
  {
    DWORD size;
    if ( RegQueryValueExA(key, STR_SCCPROVIDERPATH, NULL, NULL, NULL, &size)==ERROR_SUCCESS)
    {
      _dllPath=(char *)malloc(size);
      if ( RegQueryValueExA(key, STR_SCCPROVIDERPATH, NULL, NULL, (LPBYTE)_dllPath, &size) == ERROR_SUCCESS)
      {
        RegCloseKey(key);
        return;
      }
    }
  }
}

SCCRTN MsSccLib::LoadScc(const char *dllpath)
{
  if (IsLoaded()) return SCC_OK;

  if (_dllPath==NULL) FindLibrary();    
  else _dllPath=_strdup(dllpath);

  if (_dllPath==NULL) return SCC_CE_NOTINSTALLED;

  _library=LoadLibraryA(_dllPath);
  if (_library==NULL) return SCC_CE_LOADLIBERROR;

  REGISTERFUNCTION(Initialize);
  REGISTERFUNCTION(Uninitialize);
  REGISTERFUNCTION(OpenProject);
  REGISTERFUNCTION(CloseProject);
  REGISTERFUNCTION(Get);
  REGISTERFUNCTION(Checkout);
  REGISTERFUNCTION(Uncheckout);
  REGISTERFUNCTION(Checkin);
  REGISTERFUNCTION(Add);
  REGISTERFUNCTION(Remove);
  REGISTERFUNCTION(Rename);
  REGISTERFUNCTION(Diff);
  REGISTERFUNCTION(DirDiff);
  REGISTERFUNCTION(History);
  REGISTERFUNCTION(Properties);
  REGISTERFUNCTION(RunScc);
  REGISTERFUNCTION(GetCommandOptions);
  REGISTERFUNCTION(QueryInfo);
  REGISTERFUNCTION(DirQueryInfo);
  REGISTERFUNCTION(GetEvents);
  REGISTERFUNCTION(GetProjPath);
  REGISTERFUNCTION(PopulateList);
  REGISTERFUNCTION(AddFromScc);
  REGISTERFUNCTION(SetOption);
  REGISTERFUNCTION(GetVersion);
  REGISTERFUNCTION(CreateSubProject);
  REGISTERFUNCTION(GetParentProjectPath);
  REGISTERFUNCTION(BeginBatch);
  REGISTERFUNCTION(EndBatch);
  REGISTERFUNCTION(IsMultiCheckoutEnabled);
  REGISTERFUNCTION(WillCreateSccFile);
  if (Initialize &&Uninitialize &&OpenProject &&CloseProject &&Get &&Checkout &&
    Uncheckout &&Checkin &&Add &&Remove &&Rename &&Diff &&DirDiff &&History &&
    Properties &&RunScc &&GetCommandOptions &&QueryInfo &&DirQueryInfo &&
    GetEvents &&GetProjPath &&PopulateList &&AddFromScc &&SetOption &&
    GetVersion &&CreateSubProject &&GetParentProjectPath &&BeginBatch &&
    EndBatch &&IsMultiCheckoutEnabled &&WillCreateSccFile) 
    return SCC_OK;
  else
    return SCC_CE_LIBLINKERROR;    
}


