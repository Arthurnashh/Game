#include "wpch.hpp"
#include "depMake.hpp"
#include "engine.hpp"
#include "engDummy.hpp"
#include "textbank.hpp"
#include <El/FileBinMake/fileBinMake.hpp>
#include <Es/Files/filenames.hpp>
#include <El/QStream/qbStream.hpp>
#include <El/Debugging/debugTrap.hpp>

static FileMakeFunctions SMakeFunctionsDefault;
FileMakeFunctions *GFileMakeFunctions = &SMakeFunctionsDefault;

#if _ENABLE_BULDOZER

#include <Es/Common/win.h>

///New implementation if BinMakeFunctions that shares buldozer's console
class BuldozerBinMakeFunctions: public BinMakeFunctions
{
  HANDLE _hThreadCloseConsole;
  HANDLE _hLock;
  bool _drop;  
public:
  BuldozerBinMakeFunctions();
  ~BuldozerBinMakeFunctions();
  //! Virtual method
  virtual int Make(const char *src, const char *trg, const char *rules, BinMakeHandle *handle);
  void CloseConsoleThread();

public:
  bool disableConsoleThread;
};

///Function to disable Close button on console;
static BOOL WINAPI UserClosedConsoleWindow(DWORD type)
{
  return TRUE;
}

///Thread that watches request to close shared console in right time
static UINT ConsoleThread(BuldozerBinMakeFunctions *self)
{
  self->CloseConsoleThread();
  return 0;
};

///Constructs the BinMake interfaces
BuldozerBinMakeFunctions::BuldozerBinMakeFunctions()
{
  disableConsoleThread=false;
  _hThreadCloseConsole=0;
  _hLock=0;
  _drop=false;
}

///Destructs the BinMake interface - waits for thread
BuldozerBinMakeFunctions::~BuldozerBinMakeFunctions()
{
  if (_hThreadCloseConsole)
  {  
    WaitForSingleObject(_hThreadCloseConsole,INFINITE);
    CloseHandle(_hThreadCloseConsole);
  }
  if (_hLock) CloseHandle(_hLock);
}

///Watches requests and closes console, when it is nolonger used
void BuldozerBinMakeFunctions::CloseConsoleThread()
{
  bool exit;
  WaitForSingleObject(_hLock,INFINITE);
  _drop=false;
notnow:
  ReleaseMutex(_hLock);
  do
  {
    WaitForSingleObject(_hLock,INFINITE);
    exit=_drop;
    _drop=true;
    ReleaseMutex(_hLock);
    if (!exit) Sleep(20000);
  }
  while (!exit);
  WaitForSingleObject(_hLock,INFINITE);
  if (_drop==false) goto notnow;
  FreeConsole();
  SetStdHandle(STD_OUTPUT_HANDLE,0);
  ReleaseMutex(_hLock);

}

///Make function
int BuldozerBinMakeFunctions::Make(const char *src, const char *trg, const char *rules, BinMakeHandle *handle)
{
  DWORD wr;
  WaitForSingleObject(_hLock,INFINITE);
  // console application should be able to handle console output
  #ifndef _CONSOLE
  if (!disableConsoleThread)
  {
    if (_hThreadCloseConsole==0 || GetStdHandle(STD_OUTPUT_HANDLE)==0)
    {
      DWORD tid;
      if (_hThreadCloseConsole) CloseHandle(_hThreadCloseConsole);
      if (_hLock==0) _hLock=CreateMutex(0,0,0);
      _hThreadCloseConsole=CreateThread(0,0,(LPTHREAD_START_ROUTINE)ConsoleThread,this,0,&tid);
      AllocConsole();
      SetConsoleCtrlHandler(UserClosedConsoleWindow,TRUE);
      const char *text="Converting resources, please wait -----------\r\n";
      WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),text,strlen(text),&wr,0);
    }
  }
  #endif
  _drop=false;
  int res=BinMakeFunctions::Make(src,trg,rules,handle);
  if (_hLock)
  { // write divisor line only when using redirected output
    const char *text="---------------------------------------------\r\n";
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),text,strlen(text),&wr,0);
    ReleaseMutex(_hLock);
  }
  return res;
}


static BuldozerBinMakeFunctions SBinMakeFunctions;
void DisableBinMakeConsoleThread() { SBinMakeFunctions.disableConsoleThread=true; }

#endif //_ENABLE_BULDOZER

void SetBinMakeFunctions()
{
  #if _ENABLE_BULDOZER
  GFileMakeFunctions = &SBinMakeFunctions;
  #endif
}

RString GlobResolveTexture(RString name)
{
  const char *ext = GetFileExt(GetFilenameExt(name));
  if (!strcmp(ext,".tga") || !strcmp(ext,".png"))
  {
    char tgtName[1024];
    strcpy(tgtName,name);
    char *tgtExt = GetFileExt(GetFilenameExt(tgtName));
    strcpy(tgtExt,".paa");
#if _ENABLE_BULDOZER
    if (GFileMakeFunctions == &SBinMakeFunctions)
    {
      Debugger::PauseCheckingScope scope(GDebugger);

#if _USE_FCPHMANAGER
      // If some process with the tgtName is running, then don't continue
      if (GEngine->IsAsynchronousTextureLoadingSupported())
      {
        if (GFCPHManager.Find(tgtName)) return tgtName;
      }
#endif

      // if the file is already up-to-date, do not make it
      QFileTime sTime = QFBankQueryFunctions::TimeStamp(name);
      QFileTime tTime = QFBankQueryFunctions::TimeStamp(tgtName);
      if (tTime>=sTime) return tgtName;
      // before making the file we need to release the texture
      AbstractTextBank::TextureSuspendScope texScope(GEngine->TextBank(),tgtName);
      if (!GFileServerFunctions->FlushReadHandle(tgtName))
      {
        // cannot write the target - it is locked
        return tgtName;
      }
      // check if we can make the file
#if _USE_FCPHMANAGER
      if (GEngine->IsAsynchronousTextureLoadingSupported())
      {
        //printf("Info: Starting to make the file %s, process manager has got %d items", tgtName, GFCPHManager.Size());
        // Create new process
        BinMakeHandle processHandle;
        if (GFileMakeFunctions->Make(name, tgtName, "binarize", &processHandle) >= 0)
        {
          GFCPHManager.AddItem(tgtName, (HANDLE)processHandle);
          return tgtName;
        }
      }
      else
#endif
      {
        if (GFileMakeFunctions->Make(name,tgtName,"binarize",NULL)>=0)
        {
          return tgtName;
        }
      }
    }
#endif
    // even when the target was not built, we still want to use its name
    return tgtName;
  }
  return name;
}
