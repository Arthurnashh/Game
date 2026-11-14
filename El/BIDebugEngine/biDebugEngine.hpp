#ifdef _MSC_VER
#pragma once
#endif

#ifndef _BI_DEBUG_ENGINE_HPP
#define _BI_DEBUG_ENGINE_HPP

#include <El/Interfaces/iDebugEngine.hpp>
#include "debugEngineInterface.hpp"

//! evaluator using GameState
class BIDebugEngineFunctions : public DebugEngineFunctions, public IToApp
{
protected:
  void *_library;
  IToDll *_toDll;
  
public:
  BIDebugEngineFunctions();
  ~BIDebugEngineFunctions();

  // implementation of DebugEngineFunctions interface
  virtual void Startup();
  virtual void Shutdown();

  virtual void ScriptLoaded(IDebugScript *script, const char *name);
  virtual void ScriptEntered(IDebugScript *script);
  virtual void ScriptTerminated(IDebugScript *script);

  virtual void FireBreakpoint(IDebugScript *script, unsigned int bp);
  virtual void Breaked(IDebugScript *script);

  virtual void DebugEngineLog(const char *str);

  // implementation of IToApp interface
  virtual void DebugLogF(const char *str);

protected:
  // connect to BIDebugEngine.dll
  void Connect();
};

#endif
