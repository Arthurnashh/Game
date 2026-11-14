#ifdef _MSC_VER
#pragma once
#endif

#ifndef _I_DEBUG_ENGINE_HPP
#define _I_DEBUG_ENGINE_HPP

#include "iDebugEngineInterface.hpp"

class DebugEngineFunctions
{
public:
  DebugEngineFunctions() {}
  virtual ~DebugEngineFunctions() {}

  //! callback function to startup debug engine
  virtual void Startup() {}
  //! callback function to shutdown debug engine
  virtual void Shutdown() {}

  //! callback function to register script in debug engine
  virtual void ScriptLoaded(IDebugScript *script, const char *name) {script->AttachScript();}
  //! callback function to tell debug engine that script is executed
  virtual void ScriptEntered(IDebugScript *script) {script->EnterScript(); script->RunScript(true);}
  //! callback function to unregister script in debug engine
  virtual void ScriptTerminated(IDebugScript *script) {}

  //! callback function called when breakpoint conditions met
  virtual void FireBreakpoint(IDebugScript *script, unsigned int bp) {}
  //! callback function called when script breaks (after step) 
  virtual void Breaked(IDebugScript *script) {}

  //! callback function to output string to attached debugger
  virtual void DebugEngineLog(const char *str) {}
};

extern DebugEngineFunctions *CurrentDebugEngineFunctions;

#endif
