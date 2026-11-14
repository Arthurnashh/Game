#ifdef _MSC_VER
#pragma once
#endif

#ifndef _DEBUG_ENGINE_INTERFACE_HPP
#define _DEBUG_ENGINE_INTERFACE_HPP

// Interface with BIDebugEngine.dll

#include "El/Interfaces/iDebugEngineInterface.hpp"

class IToDll
{
public:
  IToDll() {}
  virtual ~IToDll() {}

  virtual void Startup() = 0;
  virtual void Shutdown() = 0;

  virtual void ScriptLoaded(IDebugScript *script, const char *name) = 0;
  virtual void ScriptEntered(IDebugScript *script) = 0;
  virtual void ScriptTerminated(IDebugScript *script) = 0;

  virtual void FireBreakpoint(IDebugScript *script, unsigned int bp) = 0;
  virtual void Breaked(IDebugScript *script) = 0;

  virtual void DebugEngineLog(const char *str) = 0;
};

class IToApp
{
public:
  IToApp() {}
  virtual ~IToApp() {}

  virtual void DebugLogF(const char *str) = 0;
};

#endif
