#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SCRIPTS_HPP
#define _SCRIPTS_HPP

#include <Es/Strings/rString.hpp>
#include <Es/Containers/array.hpp>
//#include "object.hpp"
#include <El/Time/time.hpp>
#include <El/Evaluator/expressImpl.hpp>

#include <El/Interfaces/iDebugEngine.hpp>

#if   !defined(_WIN32)
#define SCRIPTS_DEBUGGING	0
#elif _SUPER_RELEASE
#define SCRIPTS_DEBUGGING	0
#else
#define SCRIPTS_DEBUGGING	1
#endif

#if SCRIPTS_DEBUGGING
struct BreakpointInfo
{
  DWORD id;
  bool enabled;

  BreakpointInfo()
  {
    id = 0;
    enabled = false;
  }
};
#endif

struct ScriptLine
{
	RString waitUntil; // wait until condition is met
	RString suspendUntil; // wait until time reach this value
	RString condition; // skip if condition not met
	RString statement; // execute
#if USE_PRECOMPILATION
  CompiledExpression waitUntilCompiled;
  CompiledExpression suspendUntilCompiled;
  CompiledExpression conditionCompiled;
  CompiledExpression statementCompiled;
#endif
#if SCRIPTS_DEBUGGING
  int fileLine;
  BreakpointInfo breakpoint;
#endif

	LSError Serialize(ParamArchive &ar);
#if USE_PRECOMPILATION
  void Compile(GameDataNamespace *globals);
#endif
};
TypeIsMovable(ScriptLine);

struct ScriptLabel
{
	RString name;
	int line;

	LSError Serialize(ParamArchive &ar);
};
TypeIsMovableZeroed(ScriptLabel);

#include <Es/Types/lLinks.hpp>

enum ScriptDirectHelper {ScriptDirect};

class Script : public RequestableObject, public IDebugScript
{
protected:
	GameValue _argument;
	Time _baseTime;
	int _currentLine;
	AutoArray<ScriptLine> _lines;
	AutoArray<ScriptLabel> _labels;
	/// local variable space
  GameVarSpace _vars;
  /// global variable space
  Ref<GameDataNamespace> _globals;
	float _time;
	float _suspendedUntil;
	int _maxLinesAtOnce;
	RString _name;
#if SCRIPTS_DEBUGGING
  RString _filename;
  // maybe this flags needs to be replaced by some synchronization objects
  bool _attached;
  bool _entered;
  bool _running;
  bool _breaked;
  bool _step;
#endif
	bool _loaded;

  static Script *_currentScript;
  LLinkArray<Script> _childs;

public:
	Script();
	/// string loaded from a file
	Script(RString name, GameValuePar argument, GameDataNamespace *globals, int maxLines=-1);
	/// script defined directly, by a list of strings
	Script(const AutoArray<RString> &lines, GameValuePar argument, GameDataNamespace *globals, int maxLines=-1);
	/// script defined directly, by a string
	Script(ScriptDirectHelper direct, RString content, GameValuePar argument, GameDataNamespace *globals, int maxLines=-1);
	~Script();

  
  void SetGlobalVariables(GameDataNamespace *globals) {_globals = globals;}

  /// start background loading of the script
  void RequestLoading();

  virtual int ProcessingThreadId() const;
  //@{ implementation of RequestableObject
  virtual void RequestDone(RequestContext *context, RequestResult result);
  //@}
  
  /// sometimes we need to make sure script is loaded before proceeding
  void WaitUntilLoaded();

	RString GetDebugName() const {return _name;}
	void SetName(RString name) {_name=name;}

  void AddChild(Script *child) {_childs.Add(child);}

	LSError Serialize(ParamArchive &ar);
	static Script *CreateObject(ParamArchive &ar) {return new Script();}
	
  static LSError LoadLLinkRef(ParamArchive &ar, LLink<Script> &link);
  static LSError SaveLLinkRef(ParamArchive &ar, LLink<Script> &link);
	
	bool SimulateBody();
	bool OnSimulate();
	bool Simulate(float deltaT);
	bool IsTerminated() const;
	void Terminate();
  void TerminateFamily();
	void GoTo(RString label);
	void Exit();

	GameValuePar VarGet(const char *name) const;
	void VarSet(const char *name, GameValuePar value, bool readOnly = false);

  static Script *GetCurrentScript() {return _currentScript;}

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

protected:
	void ProcessLine(const char *line, int number = -1);

  void AttachDebugger();
public:
  void DetachDebugger();
};
//TypeContainsOLink(Script)

#endif


