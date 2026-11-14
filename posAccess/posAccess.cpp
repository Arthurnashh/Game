// Poseidon - shape access library

#include "..\lib\wpch.hpp"
#include "..\lib\global.hpp"
#include "El/paramFile/paramFile.hpp"
#include "El/Modules/modules.hpp"
#include "El/Stringtable/stringtable.hpp"
#include "El/Stringtable/stringtable.hpp"
#include <El/Interfaces/iAppInfo.hpp>

#include <El/FileServer/fileServer.hpp>
#include "..\lib\keyInput.hpp"

#include "posAccess.hpp"

#include "../lib/textBank.hpp"
#include "../lib/engine.hpp"
#include "../lib/Shape/shape.hpp"
#include <El/qstream/qbstream.hpp>
#include <El/evaluator\express.hpp>
#include <Es/common\win.h>

bool LandEditor = true;
bool EnableHWTL = false;
bool UseGlide = false;
bool AutoTest = false;
bool GClientServer = false;

void EnableDesktopCursor(bool enable)
{
}

bool PrintConfigInfo(HANDLE file)
{
  return false;
}

class InitStaticData
{
  public:
  InitStaticData();
};

Globals Glob;

extern ParamFile Pars;

Engine *CreateEngineDummy();

extern GameState GGameState;
extern GameVarSpace GGameVarSpace;
/*!
\patch_internal 1.52 Date 4/23/2002 by Jirka
- Added: support for variant config paths in posAccess
*/
void PosAccessInit(const char *folder)
{
  static bool once = true;
  if (!once) return;
  once = false;


  //GFileServer=new FileServerST(10*1024*1024);

  GFileServer->Start();

  // CHANGE AFTER RELEASE 1.00 (INCLUDE STRUCTURE CHANGED IN config.cpp)
/*
  char buf[1024];
  GetCurrentDirectory(sizeof(buf),buf);
  SetCurrentDirectory(RString(folder) + RString("bin"));
  Pars.Parse("config.cpp");

  GLanguage = "English";
  LoadStringtable("Global", "stringtable.csv", 0, false);

  SetCurrentDirectory(buf);
*/
  RString dir = RString(folder) + RString("bin\\");
  Pars.Parse(dir + RString("config.cpp"), NULL, NULL, &GDefaultNamespace); // use default namespace
  GLanguage = "English";
  LoadStringtable("Global", dir + RString("stringtable.csv"), 0, false);
  
  InitModules();

  //GGameState.Init();

}

void PosAccessCreateEngine()
{
  GEngine = CreateEngineDummy();
  if (GEngine)
  {
    GEngine->Init();
  }
}


void PosAccessDone()
{
  Pars.Clear();
  Pars.DeleteVariables();
}


/*
void __cdecl WarningMessage(char const *format,...)
{
  static char buf[256];
  va_list arglist;
  va_start( arglist, format );
  vsprintf( buf, format, arglist );
  va_end( arglist );
  // kill direct draw
  Log("Warning Message: %s",buf);
  #if _DEBUG
    __asm {int 3}
  #endif
}

void __cdecl ErrorMessage(char const *format,...)
{
  static char buf[256];
  va_list arglist;
  va_start( arglist, format );
  vsprintf( buf, format, arglist );
  va_end( arglist );
  Log("Warning Message: %s",buf);
  #if _DEBUG
    __asm {int 3}
  #endif
}

void __cdecl LogF( char const *format,... )
{
  char buf[512];
    
  va_list arglist;
  va_start( arglist, format );

  vsprintf(buf,format,arglist);
  strcat(buf,"\n");
  OutputDebugString(buf);
  //printf(buf);
}

void __cdecl LstF( char const *format,... )
{
  char buf[512];
    
  va_list arglist;
  va_start( arglist, format );

  vsprintf(buf,format,arglist);
  strcat(buf,"\n");
  OutputDebugString(buf);
  printf(buf);
}

void __cdecl ErrF( char const *format,... )
{
  char buf[512];
    
  va_list arglist;
  va_start( arglist, format );

  vsprintf(buf,format,arglist);
  strcat(buf,"\n");
  OutputDebugString(buf);
  printf(buf);
}
*/

//StartVehicle PlayStart=StartHelicopter;

#if _ENABLE_CHEATS
KeyLights KeyState;
#endif

extern const char DefLoadFile[]="";
char LoadFile[256];

// dummy main.cpp stubs

void Input::ProcessMouse(DWORD timeDelta)
{
  // used from outside too
}
void ProcessMessagesNoWait()
{
}


float GetBuldozerMouseSpeed() {return 1;}

HINSTANCE GHInstance;

void PrintMissionInfo( HANDLE file )
{
}

void Config::LoadDifficulties(ParamEntryPar userCfg)
{
}

void Config::SaveDifficulties(ParamFile &userCfg)
{
}

RString GetPublicKey() {return "";}

void DDTerm()
{
  //if( GEngine ) GEngine->ReinitCounters();

  //MainCDThread.Free();
  /*
  CanRender=false;
  markersMap.Clear();
  CurrentTemplate.Clear();
  GClipboard.Clear();
  if( GEngine ) GEngine->StopAll();
  if( GWorld ) delete GWorld,GWorld=NULL;
  GPreloadedTextures.Clear();
  if( GSoundsys ) delete GSoundsys,GSoundsys=NULL;
  if( GStringTable ) GStringTable=NULL;
  CampaignStringTable.Clear();
  MissionStringTable.Clear();
  Glob.Clear();
  DestroyEngine();
  if (dInput) dInput->Release(), dInput = NULL;
  */
}

bool EnableLowFpsDetection() {return false;}
bool FpsCap = false;

int GetNetworkPort()
{
  return 0;
}
unsigned int CalculateExeCRC(int,int)
{
  return 0;
}
unsigned int CalculateDataCRC(char const *,int,int)
{
  return 0;
}
#if _ENABLE_DEDICATED_SERVER
   bool IsDedicatedServer ()
      { return false; };
#endif
int ConsoleTitle(char const *,...)
{
  return 0;
}
int ConsoleF(char const *,...)
{
  return 0;
}
bool IsAppPaused() {return false;}
bool IsAppFocused() {return true;}
RString GetNetworkPassword() {return "";}
RString GetBEPath() {return "";}
void RemoveInputMessages() {}
bool IsUseSockets() {return false;}
#if 0
float VersionToFloat(const char *ptr) {return 0;}
#else
int VersionToInt(const char *ptr) {return 0;}
#endif
RString GetServerConfig() {return "";}
RString GetRankingLog() {return "";}
void SetUseSockets(bool set) {}

extern const int DefaultNetworkPort = 0;

bool NoSound=true;

bool NoBlood()
{
  return false;
}

bool IsVBS()
{
  return false;
}

struct IDirectSoundCapture;
typedef struct IDirectSoundCapture          *LPDIRECTSOUNDCAPTURE;
struct IUnknown;
typedef /* [unique] */ IUnknown *LPUNKNOWN;

HRESULT WINAPI DirectSoundCaptureCreate( 
                                        LPCGUID lpGUID, 
                                        LPDIRECTSOUNDCAPTURE *lplpDSC, 
                                        LPUNKNOWN pUnkOuter 
                                        )
{
  return 0;
}

#include "../lib/appFrameExt.hpp"

ErrorMessageLevel  GetMaxError()
{
  return EMNote;
}

RString GetMaxErrorMessage() {return RString();}

RString FlashpointCfg = "Flashpoint.cfg";

void ParseFlashpointCfg(ParamFile &file)
{
  file.Parse(FlashpointCfg);
}

void SaveFlashpointCfg(ParamFile &file)
{
}

void InitWorld(){}
void DoneWorld(){}

void ResetErrors()
{
}

#include <Es/ErrorProp/errorProp.hpp>

void OFPFrameFunctions::ErrorMessage(const char *format, va_list argptr)
{
  RptF(format,argptr);
  #if _DEBUG
    __asm {int 3}
  #endif
  ERROR_THROW_NOVAL(ErrorInfo)
}

void OFPFrameFunctions::WarningMessage( const char *format, va_list argptr)
{
  RptF(format,argptr);
  #if _DEBUG
    __asm {int 3}
  #endif
}

void OFPFrameFunctions::ErrorMessage(ErrorMessageLevel level, const char *format, va_list argptr)
{
  RptF(format,argptr);
  #if _DEBUG
    //__asm {int 3}
  #endif
  ERROR_THROW_NOVAL(ErrorInfo)
}
#if _ENABLE_REPORT

void OFPFrameFunctions::LogF( const char *format, va_list argptr)
{
  BString<512> buf;
    
  vsprintf(buf,format,argptr);
  strcat(buf,"\n");
  OutputDebugString(buf);
  //printf(buf);
}

void OFPFrameFunctions::ErrF( const char *format, va_list argptr)
{
  BString<512> buf;
    
  vsprintf(buf,format,argptr);
  strcat(buf,"\n");
  OutputDebugString(buf);
  fprintf(stderr,buf);
}

void OFPFrameFunctions::LstF( const char *format, va_list argptr)
{
  BString<512> buf;
    
  vsprintf(buf,format,argptr);
  strcat(buf,"\n");
  OutputDebugString(buf);
  fprintf(stderr,buf);
}

void OFPFrameFunctions::LstFDebugOnly(const char *format, va_list argptr)
{
  LstF(format, argptr);
}

class PosAccessInfoFunctions: public ApplicationInfoFunctions
{
  public:
  virtual void FlushLogs(FILE_HANDLE file);
};

void PosAccessInfoFunctions::FlushLogs( FILE_HANDLE file )
{
  fflush(stderr);
}

void FlushLogFile()
{
  fflush(stderr);
}

PosAccessInfoFunctions GApplicationInfoFunctions INIT_PRIORITY_URGENT;

#else // of ENABLE_REPORT
ApplicationInfoFunctions GApplicationInfoFunctions INIT_PRIORITY_URGENT;
#endif

ApplicationInfoFunctions *CurrentAppInfoFunctions = &GApplicationInfoFunctions;

TimeStampFormat GTimeStampFormat = TSFNone;

int GetServerPort()
{
  return 0;
}

RString GetConfigSection()
{
  return RString();
}
