#include <El/elementpch.hpp>
#include <Es/Common/win.h>

#include "debugConsole.hpp"

#if 0//defined _XBOX && !_SUPER_RELEASE
#include <XbDm.h>
#pragma comment(lib, "XbDm")

#define MAXRCMDLENGTH       256                 // Size of the remote cmd buffer

class DebugConsole: public IDebugConsole
{
  AutoArray< InitPtr<IDebugCommandCommandHandler> > _handler;
  
  bool _initialized;
  

  // buffer to receive remote commands from the debug console. Note that
  // since this data is accessed by the app's main thread, and the debug monitor
  // thread, we need to protect access with a critical section
  char _strRemoteBuf[MAXRCMDLENGTH];


  // The critical section used to protect data that is shared between threads
  CRITICAL_SECTION _criticalSection;

  public:
  DebugConsole();
  ~DebugConsole();
  
  /// perform regular command handling
  virtual void HandleCommands();
  /// output any string to the console
  virtual void Printf(const char *format, ...);

  virtual void Register(IDebugCommandCommandHandler *handler);
  
  private:
  void Init();
  
  HRESULT __stdcall CmdProcessor(
    const CHAR* strCommand, CHAR* strResponse, DWORD dwResponseLen, PDM_CMDCONT pdmcc
  );
  
  // command processor thread callback
  static HRESULT __stdcall CmdProcessorCallback(
    const CHAR* strCommand, CHAR* strResponse, DWORD dwResponseLen, PDM_CMDCONT pdmcc
  );
};

static DebugConsole SDebugConsole;

// Command prefix for things sent across the dubg channel
static const char StrDebugConsoleCommandPrefix[] = "XCMD";


//-----------------------------------------------------------------------------
// Name: dbgtolower()
// Desc: Returns lowercase of char
//-----------------------------------------------------------------------------
inline char dbgtolower( char ch )
{
  if( ch >= 'A' && ch <= 'Z' )
    return ch - ( 'A' - 'a' );
  else
    return ch;
}




//-----------------------------------------------------------------------------
// Name: dbgstrnicmp()
// Desc: Critical section safe string compare.
//-----------------------------------------------------------------------------
static bool dbgstrnicmp( const char* str1, const char* str2, int n )
{
  while( ( dbgtolower( *str1 ) == dbgtolower( *str2 ) ) && *str1 && n > 0 )
  {
    --n;
    ++str1;
    ++str2;
  }

  return( n == 0 || dbgtolower( *str1 ) == dbgtolower( *str2 ) );
}




//-----------------------------------------------------------------------------
// Name: dbgstrcpy()
// Desc: Critical section safe string copy
//-----------------------------------------------------------------------------
void dbgstrcpy( char* strDest, const char* strSrc )
{
  while( ( *strDest++ = *strSrc++ ) != 0 );
}


//-----------------------------------------------------------------------------
// Name: DebugConsoleCmdProcessor()
// Desc: Command notification proc that is called by the Xbox debug monitor to
//       have us process a command.  What we'll actually attempt to do is tell
//       it to make calls to us on a separate thread, so that we can just block
//       until we're able to process a command.
//
// Note: Do NOT include newlines in the response string! To do so will confuse
//       the internal WinSock networking code used by the debug monitor API.
//-----------------------------------------------------------------------------
HRESULT __stdcall DebugConsole::CmdProcessor(
  const CHAR* strCommand,
  CHAR* strResponse, DWORD dwResponseLen,
  PDM_CMDCONT pdmcc
)
{
  // Skip over the command prefix and the exclamation mark
  strCommand += strlen(StrDebugConsoleCommandPrefix) + 1;

  // Check if this is the initial connect signal
  if( dbgstrnicmp( strCommand, "__connect__", 11 ) )
  {
    // If so, respond that we're connected
    lstrcpynA( strResponse, "Connected.", dwResponseLen );
    return XBDM_NOERR;
  }

  // g_strRemoteBuf needs to be protected by the critical section
  EnterCriticalSection( &_criticalSection );
  if( _strRemoteBuf[0] )
  {
    // This means the application has probably stopped polling for debug commands
    dbgstrcpy( strResponse, "Cannot execute - previous command still pending" );
  }
  else
  {
    dbgstrcpy( _strRemoteBuf, strCommand );
  }
  LeaveCriticalSection( &_criticalSection );

  return XBDM_NOERR;
}

HRESULT __stdcall DebugConsole::CmdProcessorCallback(
  const CHAR* strCommand,
  CHAR* strResponse, DWORD dwResponseLen,
  PDM_CMDCONT pdmcc
)
{
  return SDebugConsole.CmdProcessor(strCommand,strResponse,dwResponseLen,pdmcc);
}

void DebugConsole::Register(IDebugCommandCommandHandler *handler)
{
  Init();
  _handler.Add(handler);
}

void DebugConsole::HandleCommands()
{
  Init();
  char strLocalBuf[MAXRCMDLENGTH]; // local copy of command

  // If there's nothing waiting, return.
  if( !_strRemoteBuf[0] ) return;

  // Grab a local copy of the command received in the remote buffer
  EnterCriticalSection( &_criticalSection );

  lstrcpyA( strLocalBuf, _strRemoteBuf );
  _strRemoteBuf[0] = 0;

  LeaveCriticalSection( &_criticalSection );

  // string is now local and we are free to process it without any multithreaded hassle
  // anyone can handle it as needed
  for (int i=0; i<_handler.Size(); i++)
  {
    if (_handler[i]) _handler[i]->Do(strLocalBuf);
  }
}

DebugConsole::DebugConsole()
{
  _initialized = false;
}


void DebugConsole::Init()
{
  if (_initialized) return;

  // Register our command handler with the debug monitor
  HRESULT hr = DmRegisterCommandProcessor(
    StrDebugConsoleCommandPrefix, 
    CmdProcessorCallback
  );
  if( FAILED(hr) )
    return;

  // We'll also need a critical section to protect access to g_strRemoteBuf
  InitializeCriticalSection( &_criticalSection );
  
  _initialized = true;
}



//-----------------------------------------------------------------------------
// Name: DebugConsolePrintf()
// Desc: Asynchronous printf routine that sends the string to the remote debug
//       console
//-----------------------------------------------------------------------------
void DebugConsole::Printf( const CHAR* strFormat, ... )
{
  // Copy command prefix into buffer
  CHAR strBuffer[MAXRCMDLENGTH];
  int length = _snprintf( strBuffer, MAXRCMDLENGTH, "%s!", StrDebugConsoleCommandPrefix );

  // Format arguments
  va_list arglist;
  va_start( arglist, strFormat );
  _vsnprintf( strBuffer + length, MAXRCMDLENGTH - length, strFormat, arglist );
  va_end( arglist );

  // Send it out the string
  DmSendNotificationString( strBuffer );
}

DebugConsole::~DebugConsole()
{
  DeleteCriticalSection( &_criticalSection );
}

IDebugConsole *GDebugConsole = &SDebugConsole;


#else

static IDebugConsole SDebugConsoleEmpty;
IDebugConsole *GDebugConsole = &SDebugConsoleEmpty;

#endif