#ifdef _MSC_VER
#pragma once
#endif

#ifndef _FSM_SCRIPTED_HPP
#define _FSM_SCRIPTED_HPP

#include "../fsm.hpp"

#include <Es/Strings/rString.hpp>
#include <El/Evaluator/expressImpl.hpp>
#include <El/ParamFile/paramFile.hpp>
#include <El/Common/perfProf.hpp>

const FSM::State UnchangedStateScripted = INT_MIN;

/*
struct FSMScriptedTypeName
{
  /// used only during loading - not used for name storage
  InitPtr<const ParamEntry> _class;

  /// used for item identification
  RString _name;

  FSMScriptedTypeName(ParamEntryPar cls)
  {
    _class = &cls;
    _name = cls.GetContext();
  }
};
*/
typedef RString FSMScriptedTypeName;

/// type of FSM - describes structure of FSM (states, links)
class FSMScriptedType : public RequestableObject
{
  friend class FSMScripted;

public:
  class StateInfo
  {
  public:
    class LinkInfo
    {
    protected:
      float _priority;
      int _nextState;
      RString _condition;
      RString _action;
      RString _precondition;
      int     _itemNo;
      #if USE_PRECOMPILATION
      CompiledExpression _conditionExp;
      CompiledExpression _actionExp;
      CompiledExpression _preconditionExp;
      #endif
      /// identifier of the expression
      RString _conditionName;
      /// identifier of the expression
      RString _actionName;
      /// identifier of the expression
      RString _preconditionName;

    public:
      LinkInfo() {_priority = 0; _nextState = UnchangedStateScripted;}
      void LinkToFinal() {_priority = 1; _nextState = FSM::FinalState; _condition = RString(); _action = RString();}
      void Load(ParamEntryPar cls, const FSMScriptedType &fsm, const StateInfo &from, GameDataNamespace *globals);

      float GetPriority() const {return _priority;}
      int GetNextState() const {return _nextState;}
      int GetItemNo() const {return _itemNo;}
      bool EvaluateCondition(GameDataNamespace *globals) const
      {
        bool ok = true;
        if (_condition.GetLength() > 0)
        {
          #if USE_PRECOMPILATION
          ok = GGameState.EvaluateBool(_condition, _conditionExp, GameState::EvalContext::_reportUndefined, globals, _conditionName);
          #else
          ok = GGameState.EvaluateBool(_condition, GameState::EvalContext::_reportUndefined, globals);
          #endif
        }
        return ok;
      }
      void ExecuteAction(GameDataNamespace *globals) const
      {
        #if USE_PRECOMPILATION
        GGameState.Execute(_action, _actionExp, GameState::EvalContext::_reportUndefined, globals, _actionName);
        #else
        GGameState.Execute(_action, GameState::EvalContext::_reportUndefined, globals);
        #endif
      }
      void ExecutePrecondition(GameDataNamespace *globals) const
      {
        #if USE_PRECOMPILATION
        GGameState.Execute(_precondition, _preconditionExp, GameState::EvalContext::_reportUndefined, globals, _preconditionName);
        #else
        GGameState.Execute(_precondition, GameState::EvalContext::_reportUndefined, globals);
        #endif
      }
      RString ConditionName() const { return _conditionName; }

      ClassIsMovableZeroed(LinkInfo);
    };

  protected:
    RString _id;
    RString _name;
    RString _init;
    RString _precondition;
    int     _itemNo;
    #if USE_PRECOMPILATION
    CompiledExpression _initExp;
    CompiledExpression _preconditionExp;
    #endif
    /// identifier of the expression
    RString _initName;
    /// identifier of the expression
    RString _preconditionName;
    
    AutoArray<LinkInfo> _links;

  public:
    StateInfo() {}

    void Load(ParamEntryPar cls, const FSMScriptedType &fsm, GameDataNamespace *globals);
    void SetFinal();
    void LoadLinks(ParamEntryPar cls, const FSMScriptedType &fsm, GameDataNamespace *globals);

    RString GetId() const {return _id;}
    RString GetName() const {return _name;}
    int GetItemNo() const {return _itemNo;}

    void Init(const FSMScriptedType *type, GameDataNamespace *globals) const;
    FSM::State Check(const FSMScriptedType *type, GameDataNamespace *globals, int &linkIx) const; // returns index of the next state
    void DebugLogProcessedCondition(int linkIx, int id, RString name) const;
    
    ClassIsMovable(StateInfo);

  protected:
    void ExecutePrecondition(GameDataNamespace *globals) const
    {
#if USE_PRECOMPILATION
      GGameState.Execute(_precondition, _preconditionExp, GameState::EvalContext::_reportUndefined, globals, _preconditionName);
#else
      GGameState.Execute(_precondition, GameState::EvalContext::_reportUndefined, globals);
#endif
    }
  };

protected:
#if _ENABLE_PERFLOG
  /// profiling scope corresponding to this FSM type
  mutable PerfCounter _counter;
  /// initialize counter only once
  mutable InitVal<bool,false> _counterInitialized;
#endif

  FSMScriptedTypeName _name;
  AutoArray<StateInfo> _states;
  int _initState;
  /// set when type is fully created (avoid waiting to files)
  bool _loaded;

  RString _displayName;

public:
  FSMScriptedType(const FSMScriptedTypeName &name) : _name(name)
  {
    _loaded = false;
    RequestLoading();
  }
  
  FSMScriptedTypeName GetName() const {return _name;}
  RString GetDisplayName() const {return _displayName;} 
  bool IsLoaded() const {return _loaded;}

  //@{ implementation of RequestableObject
  virtual int ProcessingThreadId() const;
  virtual void RequestDone(RequestContext *context, RequestResult result);
  //@}

  /// sometimes we need to make sure script is loaded before proceeding
  void WaitUntilLoaded();

protected:
  // implementation
  int FindStateIndex(RString id) const;

  /// start background loading of the FSM type
  void RequestLoading();
};

class FSMScriptedTypeBank: public BankArray<FSMScriptedType, DefBankTraits<FSMScriptedType> >
{
};

extern FSMScriptedTypeBank FSMScriptedTypes;

/// scripted FSM
/**
scripting commands used for conditions and actions
*/

class FSMScripted : public FSM
{
protected:
  typedef FSM base;
  /// delay selection of init state until FSM Type is fully loaded
  enum {InitState = -1};

protected:
  /// FSM type
  Ref<const FSMScriptedType> _type;
  /// global variable space
  Ref<GameDataNamespace> _globals;

  Time _timeOut;
  GameVarSpace _vars;
  bool _initNeeded;
  bool _debugLog;

#if DEBUG_FSM
  /// id of the thread used for debugging (0 when not debugging)
  DWORD _threadId;
  /// source file for debugging
  RString _tempFile;
#else
  const static DWORD _threadId = 0;
#endif

  DECL_SERIALIZE_TYPE_INFO(FSMScripted, FSM)

public:
  FSMScripted(GameDataNamespace *globals, GameVarSpace *parentVars) : _globals(globals), _vars(parentVars, true)
  {
    _initNeeded = false;
#if DEBUG_FSM
    _threadId = 0;
#endif
    _debugLog = false;
  }
  FSMScripted(const FSMScriptedType *type, GameDataNamespace *globals, GameVarSpace *parentVars);
  FSMScripted(ForSerializationOnly) : _vars(true)
  {
    _globals = NULL;
    _initNeeded = false;
    _debugLog = false;
#if DEBUG_FSM
    _threadId = 0;
#endif  
  }
  ~FSMScripted();

  void SetParam(RString name, GameValuePar value);
  void SetGlobalVariables(GameDataNamespace *globals) {_globals = globals;}
  virtual void ApplyContext(GameState &state, Context *context) {};

  // FSM interface
  virtual void SetState(State state, Context *context);
  virtual const char *GetStateName() const;

  // variables
  virtual int &Var(int i);
  virtual int Var(int i) const;

#if _HELISIM_LARGE_OBJECTID
  virtual OIDInt &VarId();
  virtual OIDInt VarId() const;
#endif // _HELISIM_LARGE_OBJECTID

  virtual Time &VarTime(int i);
  virtual Time VarTime(int i) const;

  virtual void SetTimeOut(float sec);
  virtual float GetTimeOut() const;
  virtual bool TimedOut() const;

protected:
  virtual bool Update(Context *context); // true -> final state reached
  virtual void Refresh(Context *context); // use only when FSM state is supposed to be lost
  virtual bool IsExclusive(Context *context);

public:
  virtual void Enter(Context *context);
  virtual void Exit(Context *context);

  virtual bool Evaluate(GameValue &result, const char *expression);
  virtual GameVarSpace *GetVars() override {return &_vars;}
  virtual const GameVarSpace * GetVars() const override {return &_vars;}

  LSError Serialize(ParamArchive &ar);

#if DEBUG_FSM
  virtual void Debug();
#endif
  virtual void EnableDebugLog(bool enable) {_debugLog = enable; }

protected:
  // implementation
  const FSMScriptedType::StateInfo &CurState() const
  {
    Assert(_curState >= 0 && _curState < _type->_states.Size());
    return _type->_states[_curState];
  }
/*
  FSMScriptedType::StateInfo &CurState()
  {
    Assert(_curState >= 0 && _curState < _type->_states.Size());
    return _type->_states[_curState];
  }
*/
  bool DoUpdate(Context *context);
};

// type-safe implementation of FSMScripted
template <typename ContextType>
class FSMScriptedTyped: public FSMScripted
{
  typedef FSMScripted base;

public:
  FSMScriptedTyped(GameVarSpace *parentVars) : base(parentVars) {}
  FSMScriptedTyped(const FSMScriptedType *type, GameDataNamespace *globals, GameVarSpace *parentVars) : base(type, globals, parentVars) {}
  FSMScriptedTyped(ForSerializationOnly dummy) : base(dummy) {}
};

#endif

