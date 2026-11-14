#include ".\scriptdebuggerbase.h"

ScriptDebuggerBase::ScriptDebuggerBase(void)
{
  Reset();
  _detectUndeclared=false;
}

ScriptDebuggerBase::~ScriptDebuggerBase(void)
{
}

void ScriptDebuggerBase::Reset()
{
  _curTraceMode=traceRun;
  _curTraceLevel=0;
  _breakExpr=NULL;
  _nextBreak=NULL;
  memset(&_context,0,sizeof(_context));
}

void ScriptDebuggerBase::SetTraceMode(TraceMode mode)
{
  _curTraceMode=mode;
  if (mode==traceStepOver || mode==traceStepOut)
  {
    _curTraceLevel=_context._curContext;
  }
  if (mode==traceStepLines)
  {
    _breakExpr=_context._expression;
    _nextBreak=strchr(_breakExpr,'\n');
    _curTraceLevel=_context._curContext;
  }
    
}

void ScriptDebuggerBase::SetVarBreakpoint(const char *variableName)
{
  if (IsVarBreakpoint(variableName)) return;
  _varBreaks.Append()=variableName;
}

void ScriptDebuggerBase::UnsetVarBreakpoint(const char *variableName)
{
  for (int i=0;i< _varBreaks.Size();i++)
    if (_stricmp(variableName,_varBreaks[i])==0)
    {
      _varBreaks.Delete(i);
      break;
    }
}

bool ScriptDebuggerBase::BeforeLineProcessed(GameState &script)
{
  switch (_curTraceMode)
  {
  case traceRun:  TestBreak();break;
  case traceBreakNext:
    _curTraceMode=traceRun;
    GetContext(script);
    OnTrace();
    break;
  case traceStepOver:    
    GetContext(script);
    if (_curTraceLevel>=_context._curContext)
    {
      _curTraceMode=traceRun;
      OnTrace();
    }
    else 
      TestBreak();
    break;
  case traceStepOut:
    GetContext(script);
    if (_curTraceLevel>_context._curContext)
    {
      _curTraceMode=traceRun;
      OnTrace();
    }
    else 
      TestBreak();
    break;
  case traceStepLines:    
    GetContext(script);
    if (_curTraceLevel<=_context._curContext && _breakExpr==_context._expression && _nextBreak<(_breakExpr+_context._pc))
    {
      _curTraceMode=traceRun;
      OnTrace();
    }
    else 
      TestBreak();
    break;    
  }
return _curTraceMode!=traceTerminate;
}

 bool ScriptDebuggerBase::OnConsoleOutput(GameState &script,const char *text)
 {
   LogF(text);
   return true;
 }

 bool ScriptDebuggerBase::OnUserBreakpoint(GameState &script)
 {
   SetTraceMode(traceBreakNext);
   return true;
 }

bool ScriptDebuggerBase::OnError(GameState &script, const char *error)
{
   LogF(error);
   return true;
}

bool ScriptDebuggerBase::OnVariableChanged(GameState &script,const char *name, const GameValue &val, bool added)
{
  if (_detectUndeclared && added && name[0]=='_' && _context._expression!=NULL)
    OnError(script,"Assignment to undeclared variable.");
  if (IsVarBreakpoint(name)) 
  {
    GetContext(script);
    OnVariableBreakpoint(name,val);
  }
  return true;
}

int ScriptDebuggerBase::GetCurLine()
{
  int p=_context._pc;

  while (_context._expression[p]>1 && _context._expression[p]<33) p++;
   int line=1;
  for (int i=0;i<p;i++)
    if (_context._expression[i]=='\n') line++;
  return line;
}