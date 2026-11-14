// interface of Script Virtual Machine 

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SCRIPT_VM_HPP
#define _SCRIPT_VM_HPP

#include <El/Evaluator/expressImpl.hpp>

#if USE_PRECOMPILATION

#include <El/Interfaces/iPreproc.hpp>
#include <El/Interfaces/iDebugEngine.hpp>

enum ScriptVMDirectHelper {ScriptVMDirect};

/// Virtual Machine for compiled script
/**
  This class encapsulates working environment for compiled expression with asynchronous evaluation.
*/
class ScriptVM : public RequestableObject, public IDebugScript
{
protected:
  VMContext _context;
  /// global variable space
  Ref<GameDataNamespace> _globals;

  GameValue _argument;
  bool _loaded;
  bool _findScript;  // call FindScript on the filename (used in scripted waypoints to mirror SQS behaviour)

  static PreprocessorFunctions *_defaultPreprocFunctions;

private:
  ScriptVM() : _context(true) // by default, the script can be serialized
  {
    _globals = NULL;
  } // used for serialization only

public:
  ScriptVM(RString filename, GameDataNamespace *globals, bool findScript = false);
  ScriptVM(RString filename, GameValuePar argument, GameDataNamespace *globals, bool findScript = false);
  ScriptVM(ScriptVMDirectHelper direct, const GameDataCode &content, GameValuePar argument, GameDataNamespace *globals, bool findScript = false);
  ~ScriptVM();

  void SetGlobalVariables(GameDataNamespace *globals) {_globals = globals;}
  void SetUIContext() { _context._isUIContext = true; }

  /// serialization of contained variables is enabled
  bool IsSerializationEnabled() const {return _context.IsSerializationEnabled();}

  /// start background loading of the script
  void RequestLoading();
  /// sometimes we need to make sure script is loaded before proceeding
  void WaitUntilLoaded();

  //@{ implementation of RequestableObject
  virtual int ProcessingThreadId() const;
  virtual void RequestDone(RequestContext *context, RequestResult result);
  //@}

  // try to limit simulation time to timeLimit
  bool Simulate(float deltaT, float timeLimit);
  void Terminate();

  RString GetDebugName() const;

  static PreprocessorFunctions* DefaultPreprocessorFunctions() { return _defaultPreprocFunctions; }

  // serialization
  static ScriptVM *CreateObject(ParamArchive &ar) {return new ScriptVM();}
  LSError Serialize(ParamArchive &ar);

  // IDebugScript interface implementation
  virtual int IAddRef() {return AddRef();}
  virtual int IRelease() {return Release();}
  virtual bool IsAttached() const;
  virtual bool IsEntered() const;
  virtual bool IsRunning() const;
  virtual void AttachScript();
  virtual void EnterScript();
  virtual void RunScript(bool run);
  virtual void Step(StepKind kind, StepUnit unit);
  virtual void GetScriptContext(IDebugScope * &scope);
  virtual void SetBreakpoint(const char *file, int line, DWORD bp, bool enable);
  virtual void RemoveBreakpoint(DWORD bp);
  virtual void EnableBreakpoint(DWORD bp, bool enable);

#if DEBUG_PRELOADING
  bool _debugMe;
  virtual bool DebugMe() const { return _debugMe; }
#endif

protected:
  void AttachDebugger();
public:
  void DetachDebugger();
};

#endif // USE_PRECOMPILATION

#endif
