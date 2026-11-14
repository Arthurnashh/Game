// implementation of scripts

#include "wpch.hpp"
#include "scripts.hpp"
#include <El/QStream/qbStream.hpp>
#include "gameStateExt.hpp"
#include "paramArchiveExt.hpp"
#include "AI/ai.hpp"
#include "global.hpp"
#include "world.hpp"
#include <ctype.h>
#include <El/HiResTime/hiResTime.hpp>
#include <El/Evaluator/express.hpp>

#include <El/Common/perfProf.hpp>

RString FindScript(RString name);

#define DIAG_DEBUG 0 // script debug level (0..90)

#if SCRIPTS_DEBUGGING
#include "Es/Common/win.h"

static void ProcessMessages()
{
  MSG msg;
  while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
  {
#if _GAMES_FOR_WINDOWS
    if (!XLivePreTranslateMessage(&msg))
#endif
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}

static RString GetFullPath(RString name)
{
  char buffer[_MAX_PATH];
  if (GetFullPathName(name, _MAX_PATH, buffer, NULL) == 0) return name;
  LogF(buffer);
  return buffer;
}

#endif

/*!
\patch_internal 1.36 Date 12/12/2001 by Jirka
- Improved: script processing optimization
*/
Script::Script()
: _vars(true)
{
  _globals = NULL;
  _loaded = true;
#if SCRIPTS_DEBUGGING
  _attached = false;
  _entered = false;
  _running = false;
  _breaked = false;
  _step = false;
#endif

	_baseTime = Glob.time;
	_currentLine = 0;

	_time = 0;
	_suspendedUntil = 0;
	_maxLinesAtOnce = 100;

   // Script is registered in Script::Serialize
}

static void ReadLine(QIStream &in, char *buffer, int bufferSize)
{
	char *p = buffer;
	char *endP = buffer+bufferSize-1;
	char c = in.get();
	while (!in.eof() && c == ' ')
		c = in.get();
	while (!in.eof() && c != '\r' && c != '\n')
	{
		if (p<endP) *(p++) = c;
		c = in.get();
	}
	*p = 0;
	while (!in.eof() && (c == '\r' || c == '\n'))
		c = in.get();
	if (!in.eof())
		in.unget();
}

bool OpenScript(QIFStreamB &in, RString name)
{
	RString fullname = FindScript(name);

	#if DIAG_DEBUG>=20
	LogF("Load %s",(const char *)name);
	#endif

	in.AutoOpen(fullname);
	return !in.fail();
}

void Script::RequestLoading()
{
  QIFStreamB in;
  if (!OpenScript(in,_name))
  {
    // if file cannot be opened, we are done
    _loaded = true;
    return;
  }
  in.PreReadObject(this,NULL,FileRequestPriority(300));
}

void Script::WaitUntilLoaded()
{
  if (_loaded) return;
  _loaded = true;
	QIFStreamB in;
	OpenScript(in,_name);
  int line = 0;
	while (!in.eof() && !in.fail())
	{
		char buffer[4096];
		ReadLine(in, buffer, sizeof(buffer));
		ProcessLine(buffer, line++);
	}
#if SCRIPTS_DEBUGGING
  _filename = GetFullPath(FindScript(_name));
#endif
  AttachDebugger();
}

int Script::ProcessingThreadId() const
{
  return 0;
}

void Script::RequestDone(RequestContext *context, RequestResult result)
{
  // whole file is ready - we may safely open it
  WaitUntilLoaded();
  RequestableObject::RequestDone(context,result);
}

Script::Script(RString name, GameValuePar argument, GameDataNamespace *globals, int maxLines)
: _vars(true)
{
  _globals = globals;
  _loaded = false;
#if SCRIPTS_DEBUGGING
  _attached = false;
  _entered = false;
  _running = false;
  _breaked = false;
  _step = false;
#endif

	_argument = argument;
	_baseTime = Glob.time;
	_currentLine = 0;
	_maxLinesAtOnce = maxLines;

	_time = 0;
	_suspendedUntil = 0;
	_name = name;

  RequestLoading(); // Script is registered in Script::WaitUntilLoaded
}

Script::Script(const AutoArray<RString> &lines, GameValuePar argument, GameDataNamespace *globals, int maxLines)
: _vars(true)
{
  _globals = globals;
  _loaded = true;
#if SCRIPTS_DEBUGGING
  _attached = false;
  _entered = false;
  _running = false;
  _breaked = false;
  _step = false;
#endif

  _argument = argument;
	_baseTime = Glob.time;
	_currentLine = 0;
	_maxLinesAtOnce = maxLines;

	_time = 0;
	_suspendedUntil = 0;
	_name = "Constructed";

	for (int i=0; i<lines.Size(); i++)
	{
		ProcessLine(lines[i]);
	}

  AttachDebugger();
}

Script::Script(ScriptDirectHelper direct, RString content, GameValuePar argument, GameDataNamespace *globals, int maxLines)
: _vars(true)
{
  _globals = globals;
  _loaded = true;
#if SCRIPTS_DEBUGGING
  _attached = false;
  _entered = false;
  _running = false;
  _breaked = false;
  _step = false;
#endif
  
	_argument = argument;
	_baseTime = Glob.time;
	_currentLine = 0;
	_maxLinesAtOnce = maxLines;

	_time = 0;
	_suspendedUntil = 0;
	_name = "<spawn>";

	QIStrStream in(content,content.GetLength());
	
	while (!in.eof() && !in.fail())
	{
		char buffer[4096];
		ReadLine(in, buffer, sizeof(buffer));
		ProcessLine(buffer);
	}

  AttachDebugger();
}

Script::~Script()
{
}

/*!
\patch 1.45 Date 2/18/2002 by Jirka
- Fixed: Scripting: Leading and trailing spaces were considered to be a part of label name.
*/
static void Trim(RString &str)
{
	int n = str.GetLength();
	int beg = 0, end = n;
	for (int i=0; i<n; i++)
	{
		if (!isspace((unsigned char)str[i]))
		{
			beg = i; break;
		}
	}
	for (int i=n-1; i>=0; i--)
	{
		if (!isspace((unsigned char)str[i]))
		{
			end = i + 1; break;
		}
	}
	str = str.Substring(beg, end);
}

/*!
\patch_internal 1.43 Date 1/31/2002 by Ondra
- Optimized: Script handling of default true conditions and empty statements faster.
*/

void Script::ProcessLine(const char *line, int number)
{
	const char *pos = line;
	while (*pos > 0 && isspace((unsigned char)*pos)) pos++;

	static RString trueCond("");
	static RString emptyCommand("");
	static RString noSuspend("");

	switch (*pos)
	{
		case 0:		// empty line
		case ';':	// comment
			break;
		case '#':	// label
			{
				int i = _labels.Add();
				_labels[i].name = pos + 1;
				_labels[i].line = _lines.Size();
				Trim(_labels[i].name);
			}
			break;
		case '&':	// time wait condition (absolute)
			{
				const char *time = pos + 1;
				// char buf[256];
				// sprintf(buf,"_time>=(%s)",time);
				int i = _lines.Add();
				// _lines[i].waitUntil = buf;
				_lines[i].waitUntil = trueCond;
				_lines[i].suspendUntil = time;
				_lines[i].condition = trueCond;
				_lines[i].statement = emptyCommand;
#if SCRIPTS_DEBUGGING
        _lines[i].fileLine = number;
#endif
#if USE_PRECOMPILATION
        _lines[i].Compile(_globals);
#endif
			}
			break;
		case '~':	// time wait condition (relative)
			{
				const char *time = pos + 1;
				char buf[256];
				sprintf(buf,"__waituntil = _time+(%s)",time);

				int i = _lines.Add();
				_lines[i].waitUntil = trueCond;
				_lines[i].suspendUntil = noSuspend;
				_lines[i].condition = trueCond;
				_lines[i].statement = buf;
#if SCRIPTS_DEBUGGING
        _lines[i].fileLine = number;
#endif
#if USE_PRECOMPILATION
        _lines[i].Compile(_globals);
#endif

				i = _lines.Add();
				_lines[i].waitUntil = trueCond;
				_lines[i].suspendUntil = "__waituntil";
				_lines[i].condition = trueCond;
				_lines[i].statement = emptyCommand;
#if SCRIPTS_DEBUGGING
        _lines[i].fileLine = number;
#endif
#if USE_PRECOMPILATION
        _lines[i].Compile(_globals);
#endif
			}
			break;
		case '@':	// wait condition
			{
				int i = _lines.Add();
				const char *until = pos + 1;
				_lines[i].waitUntil = until;
				_lines[i].suspendUntil = noSuspend;
				_lines[i].condition = trueCond;
				_lines[i].statement = emptyCommand;
#if SCRIPTS_DEBUGGING
        _lines[i].fileLine = number;
#endif
#if USE_PRECOMPILATION
        _lines[i].Compile(_globals);
#endif
			}
			break;
		case '?':	// condition : statement
			{
				const char *field1 = pos + 1;
				while (isspace((unsigned char)*field1)) field1++;
				const char *field2 = strchr(field1, ':');
				if (!field2)
				{
					RptF("Only one field in line \"%s\".", line);
					break;
				}
				const char *field1End = field2;
				// we need to skip :
				field2++;
				// we want to skip any spaces as well
				while (isspace((unsigned char)*field2)) field2++;
				int i = _lines.Add();
				_lines[i].waitUntil = trueCond;
				_lines[i].suspendUntil = noSuspend;
				_lines[i].condition = *field1 ? RString(field1,field1End-field1) : trueCond;
				_lines[i].statement = field2;
#if SCRIPTS_DEBUGGING
        _lines[i].fileLine = number;
#endif
#if USE_PRECOMPILATION
        _lines[i].Compile(_globals);
#endif
			}
			break;
		default:
			{
				const char *field1 = pos;
				while (isspace((unsigned char)*field1)) field1++;
				int i = _lines.Add();
				_lines[i].waitUntil = trueCond;
				_lines[i].suspendUntil = noSuspend;
				_lines[i].condition = trueCond;
				_lines[i].statement = field1;
#if SCRIPTS_DEBUGGING
        _lines[i].fileLine = number;
#endif
#if USE_PRECOMPILATION
        _lines[i].Compile(_globals);
#endif
			}
			break;
	}
}

#if USE_PRECOMPILATION
void ScriptLine::Compile(GameDataNamespace *globals)
{
  GameState *gstate = GWorld->GetGameState();
  GameVarSpace local(gstate->GetContext(), false);
  int gshandle = gstate->BeginContext(&local);

  if (suspendUntil.GetLength() > 0)
  {
    SourceDoc doc; doc._content = suspendUntil;
    SourceDocPos pos(doc);
    gstate->Compile(doc, pos, suspendUntilCompiled, globals);
  }
  else suspendUntilCompiled.Clear();
  if (waitUntil.GetLength() > 0)
  {
    SourceDoc doc; doc._content = waitUntil;
    SourceDocPos pos(doc);
    gstate->Compile(doc, pos, waitUntilCompiled, globals);
  }
  else waitUntilCompiled.Clear();
  if (condition.GetLength() > 0)
  {
    SourceDoc doc; doc._content = condition;
    SourceDocPos pos(doc);
    gstate->Compile(doc, pos, conditionCompiled, globals);
  }
  else conditionCompiled.Clear();
  if (statement.GetLength() > 0)
  {
    SourceDoc doc; doc._content = statement;
    SourceDocPos pos(doc);
    gstate->CompileMultiple(doc, pos, statementCompiled, false, globals);
  }
  else statementCompiled.Clear();
  
  gstate->EndContext(gshandle);
}
#endif

LSError ScriptLine::Serialize(ParamArchive &ar)
{
	//CHECK(ar.Serialize("time", time, 1, 0))
	CHECK(ar.Serialize("waitUntil", waitUntil, 1, "true"))
	CHECK(ar.Serialize("suspendUntil", suspendUntil, 1, "0"))
	CHECK(ar.Serialize("condition", condition, 1, "true"))
	CHECK(ar.Serialize("statement", statement, 1))
#if USE_PRECOMPILATION
  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
  {
    Compile(GWorld->GetGameState()->GetGlobalVariables());
  }
#endif
	return LSOK;
}

LSError ScriptLabel::Serialize(ParamArchive &ar)
{
	CHECK(ar.Serialize("name", name, 1))
	CHECK(ar.Serialize("line", line, 1, 0))
	return LSOK;
}

LSError Script::Serialize(ParamArchive &ar)
{
	// BUG
	if (!GWorld->GetGameState())
	{
		ErrF("Error: cannot load script - GWorld->GetGameState() == NULL");
		return LSStructure;
	}

  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
  {
    // try to recover the namespace
    _globals = GWorld->GetGameState()->GetGlobalVariables();
  }

	if (ar.GetArVersion() >= 4)
	{
		void *old=ar.GetParams();
		ar.SetParams(GWorld->GetGameState());
		CHECK(ar.Serialize("argument", _argument, 4))
		ar.SetParams(old);
	}
	else
	{
		if (ar.IsLoading())
		{
			OLink(Object) veh;
			CHECK(ar.SerializeRef("Vehicle", veh, 1))
			_argument = GameValueExt(veh.GetLink());
		}
	}
	CHECK(::Serialize(ar, "baseTime", _baseTime, 1))
	CHECK(ar.Serialize("currentLine", _currentLine, 1, 0))
	CHECK(ar.Serialize("Lines", _lines, 1))
	CHECK(ar.Serialize("Labels", _labels, 1))
	CHECK(ar.Serialize("Name", _name, 1, RString()))
	CHECK(ar.Serialize("maxLinesAtOnce", _maxLinesAtOnce, 1, -1))

	// set archive context to gamestate
	void *old=ar.GetParams();
	ar.SetParams(GWorld->GetGameState());
	//CHECK(ar.Serialize("Variables", _vars, 1))
  DoAssert(_vars.IsSerializationEnabled());
	CHECK(ar.Serialize("Vars", _vars._vars, 1))
	ar.SetParams(old);

	LSError err = ar.Serialize("time", _time, 1);
	if (err != LSOK)
	{
		if (ar.IsSaving()) return err;
		else _time = VarGet("_time");
	}
	CHECK(ar.Serialize("suspendedUntil", _suspendedUntil, 1, -FLT_MAX))

  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassSecond)
  {
    AttachDebugger();
  }
  return LSOK;
}

LSError Script::LoadLLinkRef(ParamArchive &ar, LLink<Script> &link)
{
  int id = -1;
  CHECK(ar.Serialize("id", id, 1, -1));
  link = GWorld->GetScript(id);
  return LSOK;
}
LSError Script::SaveLLinkRef(ParamArchive &ar, LLink<Script> &link)
{
  int id = GWorld->GetScriptId(link);
  CHECK(ar.Serialize("id", id, 1, -1));
  return LSOK;
}
	
Script *Script::_currentScript = NULL;

bool Script::IsTerminated() const
{
	return _loaded && _currentLine >= _lines.Size();
}

#define CHECK_SCRIPT_PERF 1 

static const float scriptTimeLimit = 0.003f;

/*!
\patch 1.43 Date 1/28/2002 by Ondra
- Fixed: Infinite loop in script caused game was frozen.
*/

bool Script::SimulateBody()
{
	PROFILE_SCOPE_EX(scrpt,*);
#ifdef _WIN32
#if 0 //_ENABLE_PERFLOG
  // TODO: perform profiling only when extended timing is enabled
	PerfCounter counter(_name,"sim",&GPerfProfilers,PROF_COUNT_SCALE);
	ScopeProfiler scopeProfL(counter, counter.TimingEnabled());
#endif
#endif

  if (!_loaded) return false;

#if SCRIPTS_DEBUGGING
  if (!IsRunning()) return false;
#endif

#if CHECK_SCRIPT_PERF>=2 && _ENABLE_PERFLOG
  int startOnLine = _currentLine;
#endif

	GameState *gstate = GWorld->GetGameState();
	gstate->VarSetLocal("_this", _argument, true);
	// if simulate body takes too long, interrupt it
	#if DIAG_DEBUG>=10
	DWORD timeStart = GlobalTickCount();
	#endif
	int cntExecute = 0;
	int maxExecute = _maxLinesAtOnce>0 ? _maxLinesAtOnce : 100;
	while (_currentLine < _lines.Size())
	{
		ScriptLine &line = _lines[_currentLine];
#if SCRIPTS_DEBUGGING
    // check for breakpoint
    if (!_breaked && line.breakpoint.enabled)
    {
      Assert(line.breakpoint.id != 0);
      CurrentDebugEngineFunctions->FireBreakpoint(this, line.breakpoint.id);
      _breaked = true;
      break;
    }
#endif

    #if CHECK_SCRIPT_PERF && _ENABLE_PERFLOG
    /// check if line execution took longer than normal
    struct GuardScopeTime
    {
      const char *_name;
      int _lineNum;
      const ScriptLine &_line;
      SectionTimeHandle _start;
      
      GuardScopeTime(const ScriptLine &line, const char *name, int lineNum)
      :_line(line),_name(name),_lineNum(lineNum)
      {
        _start = StartSectionTime();
      }
      ~GuardScopeTime()
      {
        float time = GetSectionTime(_start);
        if (time>scriptTimeLimit)
        {
          RString buffer = Format("  %s", (const char *)_line.statement);
          if (_line.condition.GetLength() > 0) buffer = buffer + Format(" (condition: %s)", (const char *)_line.condition);
          if (_line.waitUntil.GetLength() > 0) buffer = buffer + Format(" (wait until: %s)", (const char *)_line.waitUntil);
          if (_line.suspendUntil.GetLength() > 0) buffer = buffer + Format(" (suspend until: %s)", (const char *)_line.suspendUntil);
          LogF("Script %s:%d takes %.1f: %s", _name, _lineNum, time*1000,cc_cast(buffer));
        }
      }
    };
    GuardScopeTime scope(line,_name,_currentLine);
    #endif

    if (PROFILE_SCOPE_NAME(scrpt).IsActive())
    {
	    PROFILE_SCOPE_NAME(scrpt).AddMoreInfo(Format("%s:%d",cc_cast(_name),_currentLine));
	  }
    
		if (line.suspendUntil.GetLength() > 0)
		{
#if USE_PRECOMPILATION
      _suspendedUntil = gstate->Evaluate(line.suspendUntil, line.suspendUntilCompiled, GameState::EvalContext::_reportUndefined, _globals);
#else
			_suspendedUntil = gstate->Evaluate(line.suspendUntil, GameState::EvalContext::_reportUndefined, _globals);
#endif
			if (_time < _suspendedUntil) break; 
		}
		if (line.waitUntil.GetLength() > 0)
    {
#if USE_PRECOMPILATION
      if (!gstate->EvaluateBool(line.waitUntil, line.waitUntilCompiled, GameState::EvalContext::_reportUndefined, _globals)) break;
#else
      if (!gstate->EvaluateBool(line.waitUntil, GameState::EvalContext::_reportUndefined, _globals)) break;
#endif
    }
		_currentLine++;
    // empty condition is considered true
    bool cond = true;
    if (line.condition.GetLength() > 0)
    {
#if USE_PRECOMPILATION
      cond = gstate->EvaluateBool(line.condition, line.conditionCompiled, GameState::EvalContext::_reportUndefined, _globals);
#else
      cond = gstate->EvaluateBool(line.condition, GameState::EvalContext::_reportUndefined, _globals);
#endif
    }
    // empty command is not executed
		if (cond && line.statement.GetLength() > 0)
		{
			#if DIAG_DEBUG>=20
				LogF("Execute %s",(const char *)line.statement);
			#endif
#if USE_PRECOMPILATION
      gstate->Execute(line.statement, line.statementCompiled, GameState::EvalContext::_reportUndefined, _globals);
#else
			gstate->Execute(line.statement, GameState::EvalContext::_reportUndefined, _globals);
#endif
			_time = gstate->VarGetLocal("_time");
		}
#if SCRIPTS_DEBUGGING
    _breaked = false;
    if (_step)
    {
      CurrentDebugEngineFunctions->Breaked(this);
      _step = false;
      break;
    }
#endif
		if (++cntExecute>=maxExecute)
		{
			#if DIAG_DEBUG>=10
				DWORD time = GlobalTickCount();
				LogF("Script interrupted after %d ms",time-timeStart);
			#endif
			break;
		}
	}
	#if DIAG_DEBUG>=10
		if (cntExecute>0)
		{
			DWORD time = GlobalTickCount();
			LogF("Script paused after %d ms",time-timeStart);
		}
	#endif

#if CHECK_SCRIPT_PERF>=2 && _ENABLE_PERFLOG
  int value = scopeProfL.TimeSpentNorm();
  if (value >= 2000)
  {
    LogF("Script %s, lines %d ... %d takes %.2f ms", (const char *)_name, startOnLine, _currentLine, value*0.01f);
    for (int i=startOnLine; i<=_currentLine; i++)
    {
      if (i >= _lines.Size()) break;
      ScriptLine &line = _lines[i];
      RString buffer = Format("  %d: %s", i, (const char *)line.statement);
      if (line.condition.GetLength() > 0) buffer = buffer + Format(" (condition: %s)", (const char *)line.condition);
      if (line.waitUntil.GetLength() > 0) buffer = buffer + Format(" (wait until: %s)", (const char *)line.waitUntil);
      if (line.suspendUntil.GetLength() > 0) buffer = buffer + Format(" (suspend until: %s)", (const char *)line.suspendUntil);
      LogF(buffer);
    }
  }
#endif

  bool done = _currentLine >= _lines.Size() && _childs.Size() == 0;

  if (done) DetachDebugger();
	return done;
}

bool Script::Simulate(float deltaT)
{
  if (!_loaded) return false;
  if (_currentLine >= _lines.Size())
  {
    _childs.RemoveNulls();
    return _childs.Size() == 0;
  }
	if (_currentScript) return false;
	bool ret = false;
	if (_time >= _suspendedUntil)
	{
		_currentScript = this;
		GameState *gstate = GWorld->GetGameState();
		int gshandle = gstate->BeginContext(&_vars);
		gstate->VarSetLocal("_time",_time); // make sure there is such variable
		ret = SimulateBody();
		_time = gstate->VarGetLocal("_time");
		gstate->EndContext(gshandle);
		_currentScript = NULL;
	}
	_time += deltaT;
	return ret;
}

bool Script::OnSimulate()
{
  if (!_loaded) return false;
  if (_currentLine >= _lines.Size())
  {
    _childs.RemoveNulls();
    return _childs.Size() == 0;
  }
	if (_currentScript) return false;
	bool ret = false;
	_time = Glob.time - _baseTime;
	if (_time >= _suspendedUntil)
	{
		_currentScript = this;
		GameState *gstate = GWorld->GetGameState();
		int gshandle = gstate->BeginContext(&_vars);
		gstate->VarSetLocal("_time", _time);
		ret = SimulateBody();
		_time = gstate->VarGetLocal("_time");
		_baseTime = Glob.time - _time;
		gstate->EndContext(gshandle);
		_currentScript = NULL;
	}
	return ret;
}

GameValuePar Script::VarGet(const char *name) const
{
	GameState *gstate = GWorld->GetGameState();
	int gshandle = gstate->BeginContext(const_cast<GameVarSpace *>(&_vars));
	GameValuePar retval = gstate->VarGetLocal(name);
	gstate->EndContext(gshandle);
	return retval;
}

void Script::VarSet(const char *name, GameValuePar value, bool readOnly)
{
	GameState *gstate = GWorld->GetGameState();
	int gshandle = gstate->BeginContext(&_vars);
	gstate->VarSetLocal(name, value, readOnly, true);
	gstate->EndContext(gshandle);
}

void Script::GoTo(RString label)
{
	for (int i=0; i<_labels.Size(); i++)
	{
		if (stricmp(_labels[i].name, label) == 0)
		{
			_currentLine = _labels[i].line;
			return;
		}
	}
	LogF("Unknown label %s.", (const char *)label);
	Exit();
}

void Script::Exit()
{
	_currentLine = _lines.Size();
}

void Script::Terminate()
{
  // try to goto "exit"
  for (int i=0; i<_labels.Size(); i++)
  {
    if (stricmp(_labels[i].name, "exit") == 0)
    {
      _currentLine = _labels[i].line;
      _maxLinesAtOnce = INT_MAX;
      OnSimulate();
      break;
    }
  }

  Exit();
}

void Script::TerminateFamily()
{
  for (int i=0; i<_childs.Size(); i++)
  {
    if (_childs[i]) _childs[i]->TerminateFamily();
  }
  Terminate();
}

void Script::AttachDebugger()
{
#if SCRIPTS_DEBUGGING
  // attach debugger
  CurrentDebugEngineFunctions->ScriptLoaded(this, _name);
  
  // wait until debugger is attached
  while (!IsAttached()) ProcessMessages();

  // enter script
  CurrentDebugEngineFunctions->ScriptEntered(this);
  // wait until script is entered
  while (!IsEntered()) ProcessMessages();
#endif
}

void Script::DetachDebugger()
{
#if SCRIPTS_DEBUGGING
  CurrentDebugEngineFunctions->ScriptTerminated(this);
#endif
}

// IDebugScript interface implementation

bool Script::IsAttached() const
{
#if SCRIPTS_DEBUGGING
  return _attached;
#else
  return true;
#endif
}

bool Script::IsEntered() const
{
#if SCRIPTS_DEBUGGING
  return _entered;
#else
  return true;
#endif
}

bool Script::IsRunning() const
{
#if SCRIPTS_DEBUGGING
  return _running;
#else
  return true;
#endif
}

void Script::AttachScript()
{
#if SCRIPTS_DEBUGGING
  _attached = true;
#endif
}

void Script::EnterScript()
{
#if SCRIPTS_DEBUGGING
  _entered = true;
#endif
}

void Script::RunScript(bool run)
{
#if SCRIPTS_DEBUGGING
  _running = run;
#endif
}

void Script::Step(StepKind kind, StepUnit unit)
{
#if SCRIPTS_DEBUGGING
  _step = true;
  _running = true;
#endif
}

void Script::GetScriptContext(IDebugScope * &scope)
{
  // no longer supported for sqs scripts
  scope = NULL;
}

#define DEBUG_LOG(args) CurrentDebugEngineFunctions->DebugEngineLog(Format##args + RString("\n"))

void Script::SetBreakpoint(const char *file, int line, DWORD bp, bool enable)
{
#if SCRIPTS_DEBUGGING
  if (stricmp(file, _filename) != 0) return;
  int n = _lines.Size();
  for (int i=0; i<n; i++)
  {
    ScriptLine &info = _lines[i];
    if (info.fileLine >= line)
    {
      info.breakpoint.id = bp;
      info.breakpoint.enabled = enable;
      return;
    }
  }
#endif
}

void Script::RemoveBreakpoint(DWORD bp)
{
#if SCRIPTS_DEBUGGING
  int n = _lines.Size();
  for (int i=0; i<n; i++)
  {
    ScriptLine &info = _lines[i];
    if (info.breakpoint.id == bp)
    {
      info.breakpoint.id = 0;
      info.breakpoint.enabled = false;
      return;
    }
  }
#endif
}

void Script::EnableBreakpoint(DWORD bp, bool enable)
{
#if SCRIPTS_DEBUGGING
  int n = _lines.Size();
  for (int i=0; i<n; i++)
  {
    ScriptLine &info = _lines[i];
    if (info.breakpoint.id == bp)
    {
      info.breakpoint.enabled = enable;
      return;
    }
  }
#endif
}

#define NOTHING GameValue()

GameValue ScriptExit(const GameState *state)
{
  if (Script::GetCurrentScript()) Script::GetCurrentScript()->Exit();
	return NOTHING;
}

GameValue ScriptGoto(const GameState *state, GameValuePar oper1)
{
	DoAssert(oper1.GetType() == GameString)
	if (Script::GetCurrentScript()) Script::GetCurrentScript()->GoTo(oper1);
	return NOTHING;
}
