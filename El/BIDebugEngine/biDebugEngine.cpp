#include <El/elementpch.hpp>

#include <Es/Common/win.h>
#include <Es/Framework/appFrame.hpp>
#include <El/BIDebugEngine/biDebugEngine.hpp>

#if _DEBUG
static const char *dllName = "BIDebugEngineD";
#else
static const char *dllName = "BIDebugEngine";
#endif

/*
#if _DEBUG
static const char *dllName = "BIDebuggerD";
#else
static const char *dllName = "BIDebugger";
#endif
*/

BIDebugEngineFunctions::BIDebugEngineFunctions()
{
  #if defined(_WIN32)
  _library = (void *)LoadLibrary(dllName);
  _toDll = NULL;
  Connect();
  #endif
}

BIDebugEngineFunctions::~BIDebugEngineFunctions()
{
  #if defined(_WIN32)
  if (_library) FreeLibrary((HMODULE)_library);
  #endif
}


// implementation of DebugEngineFunctions interface

void BIDebugEngineFunctions::Startup()
{
  #if defined(_WIN32) 
  if (_toDll) _toDll->Startup();
  #endif
}

void BIDebugEngineFunctions::Shutdown()
{
  #if defined(_WIN32) 
  if (_toDll) _toDll->Shutdown();
  #endif
}

void BIDebugEngineFunctions::ScriptLoaded(IDebugScript *script, const char *name)
{
#if defined(_WIN32) 
  if (_toDll) _toDll->ScriptLoaded(script, name);
  else script->AttachScript();
#endif
}

void BIDebugEngineFunctions::ScriptEntered(IDebugScript *script)
{
#if defined(_WIN32) 
  if (_toDll) _toDll->ScriptEntered(script);
  else
  {
    script->EnterScript();
    script->RunScript(true);
  }
#endif
}

void BIDebugEngineFunctions::ScriptTerminated(IDebugScript *script)
{
#if defined(_WIN32) 
  if (_toDll) _toDll->ScriptTerminated(script);
#endif
}

void BIDebugEngineFunctions::FireBreakpoint(IDebugScript *script, unsigned int bp)
{
#if defined(_WIN32) 
  if (_toDll) _toDll->FireBreakpoint(script, bp);
#endif
}

void BIDebugEngineFunctions::Breaked(IDebugScript *script)
{
#if defined(_WIN32) 
  if (_toDll) _toDll->Breaked(script);
#endif
}

void BIDebugEngineFunctions::DebugEngineLog(const char *str)
{
#if defined(_WIN32) 
  if (_toDll) _toDll->DebugEngineLog(str);
#endif
}

// implementation of IToApp interface

void BIDebugEngineFunctions::DebugLogF(const char *str)
{
#if _ENABLE_REPORT
  LogF(str);
#endif
}

// Connect to dll

typedef IToDll *(*ConnectFunc)(IToApp *toApp);

void BIDebugEngineFunctions::Connect()
{
  #if defined(_WIN32) 
  if (!_library) return;

  ConnectFunc func = (ConnectFunc)GetProcAddress((HMODULE)_library, "Connect");
  if (func) _toDll = func(this);
  #endif
}
