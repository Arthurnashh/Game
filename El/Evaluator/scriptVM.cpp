// implementation of Script Virtual Machine 

#include <El/elementpch.hpp>
#include "scriptVM.hpp"

#if USE_PRECOMPILATION

#include <El/QStream/qbStream.hpp>
#include <El/Common/perfProf.hpp>
#include <El/ParamArchive/paramArchive.hpp>

ScriptVM::ScriptVM(RString filename, GameDataNamespace *globals, bool findScript)
: _context(true), // by default, the script can be serialized
  _findScript(findScript)
{
#if DEBUG_PRELOADING
  _debugMe = false;
#endif

  _globals = globals;
  _loaded = false;

  // context initialization
  _context._canSuspend = true;
  _context._doc._filename = filename;
  _context._localOnly = false;
  // we want all script files to report any undefined variables
  _context._undefinedIsNil = false;

  RequestLoading();
}

ScriptVM::ScriptVM(RString filename, GameValuePar argument, GameDataNamespace *globals, bool findScript)
: _context(true), // by default, the script can be serialized
  _findScript(findScript)
{
#if DEBUG_PRELOADING
  _debugMe = false;
#endif
  _globals = globals;
  _loaded = false;
  _argument = argument;

  // context initialization
  _context._canSuspend = true;
  _context._doc._filename = filename;
  _context._localOnly = false;
  // we want all script files to report any undefined variables
  _context._undefinedIsNil = false;

  RequestLoading();
}

ScriptVM::ScriptVM(ScriptVMDirectHelper direct, const GameDataCode &content, GameValuePar argument, GameDataNamespace *globals, bool findScript)
: _context(true), // by default, the script can be serialized
  _findScript(findScript)
{
#if DEBUG_PRELOADING
  _debugMe = false;
#endif
  _globals = globals;
  _loaded = true;
  _argument = argument;

  // context initialization
  _context._canSuspend = true;
  _context._doc._content = content.GetString();
  _context._localOnly = false;
  // we want all script files to report any undefined variables
  _context._undefinedIsNil = false;

  // create top level of VM call stack
  Assert(content.IsCompiled());
  CallStackItemSimple *level;
  if (content.IsCompiled())
  {
    level = new CallStackItemSimple(
      globals, NULL, NULL, 0, &GGameState, "ScriptVM", _context._doc, content.GetCode(), _context.IsSerializationEnabled()
    ); // root
    #if 0 // _ENABLE_REPORT
      // verify new compilation gives the same result as the already compiled code
      CallStackItemSimple temp(
        globals, NULL, NULL, 0, &GGameState, "ScriptVM", _context._doc, true, _context.IsSerializationEnabled()
      ); // root
      if (temp.IsDifferent(*level))
      {
        ErrF("ScriptVM code does not match. Script content: '%s'",cc_cast(_context._doc._content));
      }
    #endif
  }
  else
  {
    level = new CallStackItemSimple(
      globals, NULL, NULL, 0, &GGameState, "ScriptVM", _context._doc, true, _context.IsSerializationEnabled()
    ); // root
  }
  _context.AddLevel(level);
  level->GetVariables().VarLocal("_this");
  level->GetVariables().VarSet("_this", _argument, true);

  AttachDebugger();
}

ScriptVM::~ScriptVM()
{
}

RString FindScript(RString name);

void ScriptVM::RequestLoading()
{
  QIFStreamB in;
  in.AutoOpen(_findScript ? FindScript(_context._doc._filename) : _context._doc._filename);
  if (in.fail())
  {
    // if file cannot be opened, we are done
    RptF("ScriptVM file %s cannot be opened!", cc_cast(_context._doc._filename));
    _loaded = true;
    return;
  }

#if DEBUG_PRELOADING
  if (stricmp(cc_cast(_context._doc._filename),"Warfare\\MPMissions\\Warfare.SaraLite\\StartServer.sqf")==0)
  {
    _debugMe = true;
  }
#endif

  in.PreReadObject(this, NULL, FileRequestPriority(300));
}

void ScriptVM::WaitUntilLoaded()
{
  if (_loaded) return;
  _loaded = true;
  //RptF("ScriptVM file %s loaded", cc_cast(_context._doc._filename));

  QOStrStream out;
  if (!_defaultPreprocFunctions->Preprocess(out, _findScript ? FindScript(_context._doc._filename) : _context._doc._filename, true))
  {
    // preprocessor error
    //RptF("ScriptVM file %s preprocessor error!", cc_cast(_context._doc._filename));
    return;
  }

  GameDataNamespace *globals = _globals;
  if (!globals)
  {
    // check if default namespace is enabled
    if (GGameState.AreDefaultGlobalsEnabled()) globals = &GDefaultNamespace;
    else
    {
      ErrF("Global namespace not passed (script %s)", cc_cast(_context._doc._filename));
    }
  }

  _context.Clear();
  _context._doc._content = RString(out.str(), out.pcount());
  
  // create top level of VM call stack
  RString scopeName = Format("ScriptVM %s", cc_cast(GetDebugName()));
  CallStackItem *level = new CallStackItemSimple(globals, NULL, NULL, 0, &GGameState, scopeName, _context._doc, true, _context.IsSerializationEnabled()); // root
  _context.AddLevel(level);
  level->GetVariables().VarLocal("_this");
  level->GetVariables().VarSet("_this", _argument, true);

  AttachDebugger();
}

int ScriptVM::ProcessingThreadId() const
{
  return 0;
}

void ScriptVM::RequestDone(RequestContext *context, RequestResult result)
{
  // whole file is ready - we may safely open it
  //RptF("RequestDone for ScriptVM file %s!", cc_cast(_context._doc._filename));
  WaitUntilLoaded();
  RequestableObject::RequestDone(context,result);
}

#ifndef CHECK_SCRIPT_PERF
# define CHECK_SCRIPT_PERF 1 
#endif

# include <El/HiResTime/hiResTime.hpp>
#if CHECK_SCRIPT_PERF && _ENABLE_PERFLOG
# include <Es/Framework/appFrame.hpp>
#endif

/// Interrupt script when its time limit is reached
class EvalTimeInterrupt : public IEvalInterrupt
{
protected:
  float _timeLimit;
  SectionTimeHandle _start;

public:
  EvalTimeInterrupt(float timeLimit)
  {
    _start = StartSectionTime();
    _timeLimit = timeLimit;
  }
  virtual bool operator() () const
  {
    return GetSectionTime(_start) >= _timeLimit;
  }
#if CHECK_SCRIPT_PERF && _ENABLE_PERFLOG
  float GetTotalTime() const {return GetSectionTime(_start);}
#endif
};

/// generate warning when the time limit is exceeded for more than this value (so probably a single command took too much time)
static const float scriptTimeLimitReserve = 0.0005f;

bool ScriptVM::Simulate(float deltaT, float timeLimit)
{
  if (!_loaded) return false;

	PROFILE_SCOPE_EX(scrVM,*);
  if (PROFILE_SCOPE_NAME(scrVM).IsActive())
  {
    PROFILE_SCOPE_NAME(scrVM).AddMoreInfo(Format("%s:%d",cc_cast(GetDebugName()),_context._lastInstruction._sourceLine));
  }

  GameDataNamespace *globals = _globals;
  if (!globals)
  {
    // check if default namespace is enabled
    if (GGameState.AreDefaultGlobalsEnabled()) globals = &GDefaultNamespace;
    else
    {
      ErrF("Global namespace not passed (script %s)", cc_cast(_context._doc._filename));
    }
  }

  EvalTimeInterrupt func(timeLimit);

  GameDataNamespace *oldGlobals = GGameState.SetGlobalVariables(globals);

  VMContext *oldContext = GGameState.SetVMContext(&_context);
  bool done = _context.EvaluateCore(GGameState, -1, this, func);
  GGameState.ShowError();
  GGameState.SetVMContext(oldContext);

/*
  // optional filename report
  static volatile bool doIt = false;
  if (doIt)
  {
    RptF("Script file: %s", cc_cast(GetDebugName()));
  }
*/

  GGameState.SetGlobalVariables(oldGlobals);

#if CHECK_SCRIPT_PERF && _ENABLE_PERFLOG
  float time = func.GetTotalTime();
  if (time > timeLimit + scriptTimeLimitReserve)
  {
    LogF("Script %s took %.1f ms (%.1f ms allowed)", cc_cast(GetDebugName()), 1000.0f * time, 1000.0f * timeLimit);
    LogF("  (last instruction at %s:%d)", cc_cast(_context._lastInstruction._sourceFile), _context._lastInstruction._sourceLine);
  }
#endif

  if (done) DetachDebugger();
  return done;
}

void ScriptVM::Terminate()
{
  _context.Clear();
}

RString ScriptVM::GetDebugName() const
{
  RString name = _context.GetDisplayName();
  if (name.GetLength() > 0) return name;
  return "<spawn>";
}

LSError ScriptVM::Serialize(ParamArchive &ar)
{
  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
  {
    // try to recover the namespace
    _globals = GGameState.GetGlobalVariables();
  }

  CHECK(ar.Serialize("Context", _context, 1))
  CHECK(ar.Serialize("Argument", _argument, 1))
  CHECK(ar.Serialize("loaded", _loaded, 1))

  CHECK(ar.Serialize("findScript", _findScript, 1))

  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassSecond)
  {
    if (_loaded) AttachDebugger();
  }
  return LSOK;
}

// IDebugScript interface implementation

bool ScriptVM::IsAttached() const
{
#if SCRIPTVM_DEBUGGING
  return _context.GetDebugState() >= DSAttached;
#else
  return true;
#endif
}

bool ScriptVM::IsEntered() const
{
#if SCRIPTVM_DEBUGGING
  return _context.GetDebugState() >= DSEntered;
#else
  return true;
#endif
}

bool ScriptVM::IsRunning() const
{
#if SCRIPTVM_DEBUGGING
  DebugState state = _context.GetDebugState();
  return state == DSRunning || state == DSStepInit || state == DSStep;
#else
  return true;
#endif
}

void ScriptVM::AttachScript()
{
#if SCRIPTVM_DEBUGGING
  // _debugState is aligned, so calling this from other process should be safe
  DoAssert(_context.GetDebugState() == DSNone);
  _context.SetDebugState(DSAttached);
#endif
}

void ScriptVM::EnterScript()
{
#if SCRIPTVM_DEBUGGING
  // _debugState is aligned, so calling this from other process should be safe
  DoAssert(_context.GetDebugState() == DSAttached);
  _context.SetDebugState(DSEntered);
#endif
}

void ScriptVM::RunScript(bool run)
{
#if SCRIPTVM_DEBUGGING
  // _debugState is aligned, so calling this from other process should be safe
  if (run)
  {
    DoAssert(_context.GetDebugState() == DSEntered || _context.GetDebugState() == DSBreaked);
    _context.SetDebugState(DSRunning);
  }
  else
  {
    DoAssert(_context.GetDebugState() == DSRunning);
    _context.SetDebugState(DSBreaked);
  }
#endif
}

void ScriptVM::Step(StepKind kind, StepUnit unit)
{
#if SCRIPTVM_DEBUGGING
  // _debugState is aligned, so calling this from other process should be safe
  DebugState state = _context.GetDebugState();
  DoAssert(state == DSEntered || state == DSBreaked);
  if (state == DSBreaked)
  {
    _context.SetDebugState(DSStepInit);
    _context.SetStep(kind, unit);
  }
  else
  {
    _context.SetDebugState(DSStep);
    _context.SetStep(SKInto, unit);
  }
#endif
}

void ScriptVM::GetScriptContext(IDebugScope * &scope)
{
#if SCRIPTVM_DEBUGGING
  scope = _context.GetCallStack();
#endif
}

void ScriptVM::SetBreakpoint(const char *file, int line, DWORD bp, bool enable)
{
#if SCRIPTVM_DEBUGGING
  VMBreakpointInfo breakpoint;
  breakpoint._file = file;
  breakpoint._line = line;
  breakpoint._id = bp;
  breakpoint._enabled = enable;
  _context.SetBreakpoint(breakpoint);
#endif
}

void ScriptVM::RemoveBreakpoint(DWORD bp)
{
#if SCRIPTVM_DEBUGGING
  _context.RemoveBreakpoint(bp);
#endif
}

void ScriptVM::EnableBreakpoint(DWORD bp, bool enable)
{
#if SCRIPTVM_DEBUGGING
  _context.RemoveBreakpoint(bp);
#endif
}

void ScriptVM::AttachDebugger()
{
#if SCRIPTVM_DEBUGGING
  // attach debugger
  CurrentDebugEngineFunctions->ScriptLoaded(this, _context._doc._filename);

  // wait until debugger is attached
  while (!IsAttached()) ProcessMessages();

  // enter script
  CurrentDebugEngineFunctions->ScriptEntered(this);
  // wait until script is entered
  while (!IsEntered()) ProcessMessages();
#endif
}

void ScriptVM::DetachDebugger()
{
#if SCRIPTVM_DEBUGGING
  CurrentDebugEngineFunctions->ScriptTerminated(this);
#endif
}

#endif // USE_PRECOMPILATION
