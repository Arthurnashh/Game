#pragma once
#include <El/Evaluator/express.hpp>
#include "iscriptdebugger.h"

class ScriptDebuggerBase :public IScriptDebugger
{  
public:
  enum TraceMode
  {
    traceRun,       ///<no tracing, script running continuosly
    traceBreakNext, ///<script will run to the next command, then will broken to the debugger
    traceStepOver,  ///<script will run to the next command in the same or lower context level
    traceStepLines, ///<script will run to the next line in current script, or lower context level
    traceTerminate, ///<terminates current script. Script will exit to the highest level after debugger processing returns.
    traceStepOut, ///<script will run to the end of current context
  };
protected:
  TraceMode _curTraceMode;  
  int _curTraceLevel;
  const char *_breakExpr;
  const char *_nextBreak;
  AutoArray<RString>_varBreaks; ///<list of breakpoints defined on variables  
  bool _detectUndeclared;

  class FindVarBreakpointFunctor
  {
    const char * _search;
  public:
    FindVarBreakpointFunctor(const char *search):_search(search) {}
    bool operator()(const RString &p) const
      {return (_stricmp(p,_search)==0);}
  };
public:
  void SetTraceMode(TraceMode mode); ///<Sets trace mode. Each time is script stopped, mode is reset to traceRun. Don't forget set TraceMode before exit OnTrace
  TraceMode GetTraceMode() {return _curTraceMode;}
  
  virtual void OnTrace()=0; ///called everytime the break conditions is reached
  virtual void OnVariableBreakpoint(const char *name, const GameValue &val)=0; ///called everytime that  variable changed


  ScriptDebuggerBase(void);
  ~ScriptDebuggerBase(void);

  void SetVarBreakpoint(const char *variableName);
  void UnsetVarBreakpoint(const char *variableName);
  bool IsVarBreakpoint(const char *variableName) const
  {
    FindVarBreakpointFunctor ff(variableName);
    return ForEachVarBreakpoint(ff);
  }
  
  template<class Functor>
    bool ForEachVarBreakpoint(Functor &f) const
    {
    return _varBreaks.ForEachF(f);
    }

  virtual bool OnConsoleOutput(GameState &script,const char *text);
  virtual bool OnUserBreakpoint(GameState &script);
  virtual bool OnError(GameState &script,const char *error);

  ///Resets debuggers internal state
  /** It useful before new debug session is started. It clears last context, state, but
  leaves breakpoints */
  void Reset();

  /** Enables debugger to break script. This function is called between commands in traceRun mode.
  Function should change trace mode, if need to break script
  */  
  virtual void TestBreak() {}
  int GetCurLine();

  void EnableDetectUndeclared(bool enable) {_detectUndeclared=enable;}
  bool IsDetectUndeclaredEnabled() {return _detectUndeclared;}

private:
  virtual bool BeforeLineProcessed(GameState &script);
  virtual bool OnVariableChanged(GameState &script,const char *name, const GameValue &val,bool added);
};
