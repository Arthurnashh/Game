#pragma once
#ifndef _ISCRIPT_DEBUGGER_H_
#define _ISCRIPT_DEBUGGER_H_

class IScriptDebugger
{
public:  
  struct ContextInfo
  {
    const GameValue *_contextValue;
    Ref<GameEvaluator> *_contextStack;
    int _curContext;
    const char *_expression;
    GameEvaluator *_curVars;
    int _pc;  
    GameState *_script;
    void PrepareContext(Ref<GameEvaluator> *contextStack,int curContext, const char *expression, int pc,GameValue *contextValue=NULL)
    {
      _contextValue= contextValue;
      _contextStack=contextStack;
      _curContext=curContext;
      _curVars=contextStack[curContext];
      _pc=pc;
      _expression=expression;
      _script=NULL;      
    }
  }_context;
  virtual bool BeforeLineProcessed(GameState &script)=0;
  virtual bool OnConsoleOutput(GameState &script,const char *text)=0;  
  virtual bool OnUserBreakpoint(GameState &script)=0;
  virtual bool OnError(GameState &script, const char *error)=0;
  virtual bool OnVariableChanged(GameState &script,const char *name, const GameValue &val,bool added)=0;
  void GetContext(GameState &script) 
  {
    script.DebuggerContext(this);
    _context._script=&script;
  }

  protected:
    const char *GetCurrentPosition(int level) {return _context._contextStack[level]->_pos;}
    const char *GetCurrentExpression(int level) {return _context._contextStack[level]->_pos0;}
    GameValue *GetCurrentValueStack(int level) {return _context._contextStack[level]->_stack;}
    GameVarSpace *GetLocalVariables(int level) {return _context._contextStack[level]->local;}
    GameVarSpace *GetLocalVariables() {return _context._curVars->local;}

    void SetLevelCurrentPosition(int level,const char *position)
    {
      _context._contextStack[level]->_pos=position;
    }


};

#endif