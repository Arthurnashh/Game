#ifdef _MSC_VER
#pragma once
#endif

#ifndef _FSM_ENTITY_HPP
#define _FSM_ENTITY_HPP

#include <Es/Strings/rString.hpp>
#include <Es/Containers/bankArray.hpp>
#include <El/ParamFile/paramFile.hpp>

#include "fsm.hpp"
#include "vehicle.hpp"

/// returned by FSMEntity::StateInfo::Check when state stays unchanged
const FSM::State UnchangedStateEntity = INT_MIN;

/// unique identifier of FSMEntityType for usage in BankArray
struct FSMEntityTypeName
{
  /// config class name
  RString _name;
  MapNameToConditionFunc *_mapCondition;
  MapNameToActionFunc *_mapActionIn;
  MapNameToActionFunc *_mapActionOut;

  FSMEntityTypeName(const EntityType *type, RString name)
  {
    _mapCondition = type->GetCreateEntityConditionFunc();
    _mapActionIn = type->GetCreateEntityActionInFunc();
    _mapActionOut = type->GetCreateEntityActionOutFunc();
    _name = name;
  }
};

/// encapsulation of FSM condition
struct FSMEntityCondition
{
  /// member function
  SRef<FSMEntityConditionFunc> _function;
  /// negation means that the function return is transformed to 1-retVal
  bool negateFunction;
  /// for special cases, source text is needed
  RString _text;
  /// member function parameters
  AutoArray<float> _parameters;
  /// index of threshold used for testing of condition
  int _threshold;

  void Load(ParamEntryPar cls, const FSMEntityTypeName &type);
  bool Process(const FSMEntity *fsm, Entity *context, const AutoArray<float> &thresholds) const;
};

/// instructions for threshold initialization
struct ThresholdInit
{
  /// index of threshold
  int _index;
  /// range of threshold value
  float _min;
  /// range of threshold value
  float _max;
};
TypeIsSimple(ThresholdInit)

/// encapsulation of FSM actions
struct FSMEntityAction
{
  /// member function
  SRef<FSMEntityActionFunc> _functionIn;
  /// member function
  SRef<FSMEntityActionFunc> _functionOut;
  /// for special cases, source text is needed
  RString _text;
  /// member function parameters
  AutoArray<float> _parameters;
  /// which thresholds to initialize
  AutoArray<ThresholdInit> _thresholds;

  void Load(ParamEntryPar cls, const FSMEntityTypeName &type);
  /// process state entry function
  void ProcessIn(FSMEntity *fsm, Entity *context, AutoArray<float> &thresholds) const;
  /// process state exit function
  void ProcessOut(FSMEntity *fsm, Entity *context) const;
};

/// type of FSM - describes structure of FSM (states, links)
class FSMEntityType : public RefCount
{
  friend class FSMEntity;

protected:
  typedef void Context;

public:
  /// FSM state
  class StateInfo
  {
  public:
    /// FSM link
    class LinkInfo
    {
    protected:
      float _priority;
      int _nextState;
      FSMEntityCondition _condition;
      FSMEntityAction _action;

    public:
      LinkInfo() {_priority = 0; _nextState = UnchangedStateEntity;}
      /// sets link as link to FinalState
      void LinkToFinal();
      void Load(ParamEntryPar cls, const FSMEntityTypeName &type, const FSMEntityType &fsm);

      float GetPriority() const {return _priority;}
      int GetNextState() const {return _nextState;}
      const FSMEntityCondition &GetCondition() const {return _condition;}
      const FSMEntityAction &GetAction() const {return _action;}

      ClassIsMovable(LinkInfo);
    };

  protected:
    RString _id;
    RString _name;
    FSMEntityAction _action;
    AutoArray<LinkInfo> _links;

  public:
    StateInfo() {}
    /// first pass loading (state itself)
    void Load(ParamEntryPar cls, const FSMEntityTypeName &type);
    /// sets state as final (creates link to FinalState)
    void SetFinal();
    /// second pass loading (links)
    void LoadLinks(ParamEntryPar cls, const FSMEntityTypeName &type, const FSMEntityType &fsm);

    RString GetId() const {return _id;}
    const RString &GetName() const {return _name;}
    const FSMEntityAction &GetAction() const {return _action;}
    const AutoArray<LinkInfo> &GetLinks() const {return _links;}

    /// state reached
    void Init(FSMEntity *fsm, Context *context, AutoArray<float> &thresholds);
    /// leaving a state
    void Exit(FSMEntity *fsm, Context *context);
    /// state update
    FSM::State Check(FSMEntity *fsm, Context *context, AutoArray<float> &thresholds); // returns index of the next state

    /// check if there is atomic state update requested
    FSM::State CheckAtomic(FSMEntity *fsm, Context *context, AutoArray<float> &thresholds); // returns index of the next state
    
    ClassIsMovable(StateInfo);
  };

protected:
  FSMEntityTypeName _name;
  AutoArray<StateInfo> _states;
  int _initState;

public:
  FSMEntityType(const FSMEntityTypeName &name);

  void Load(ParamEntryPar cls, const FSMEntityTypeName &name);
  FSMEntityTypeName GetName() const {return _name;}

protected:
  // implementation
  int FindStateIndex(RString id) const;
};

/// instance of FSM - contains state data
class FSMEntity : public FSM
{
protected:
  typedef FSM base;
  typedef Entity Context;

protected:
  /// FSM type
  Ref<FSMEntityType> _type;
  /// Threshold slots
  AutoArray<float> _thresholds;
  Time _timeOut;
  /// initialization of init state need to be done
  bool _initNeeded;
  bool _debugLog;

  /// is this FSM exclusive (should it suspend other FSMs)?
  bool _isExclusive;
#if DEBUG_FSM
  DWORD _threadId;
  RString _tempFile;
#endif

  DECL_SERIALIZE_TYPE_INFO(FSMEntity, FSM)

public:
  FSMEntity(FSMEntityType *type);
  FSMEntity(ForSerializationOnly)
  {
    _initNeeded = false;
    _isExclusive = false;
#if DEBUG_FSM
    _threadId = 0;
#endif  
    _debugLog = false;
  }
  ~FSMEntity();

  // FSM interface
  virtual const char *GetStateName() const;
  virtual RString GetDebugName() const;

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
  // declare protected for type safety
  virtual void SetState(State state, base::Context *context);

  virtual bool Update(base::Context *context); // true -> final state reached
  virtual void Refresh(base::Context *context); // use only when FSM state is supposed to be lost
  virtual bool IsExclusive(Context *context);

  virtual void Enter(base::Context *context);
  virtual void Exit(base::Context *context);

  /// helper for Update
  bool ProceedTo(base::Context *context, State newState);
  
public:
  FSMEntityType *GetFSMType() {return _type;}
  // public wrappers
  bool Update(Context *context) {return Update((base::Context *)context);}
  void Refresh(Context *context) {Refresh((base::Context *)context);}


  LSError Serialize(ParamArchive &ar);

#if DEBUG_FSM
  virtual void Debug();
#endif
  virtual void EnableDebugLog(bool enable) { _debugLog = enable; }

protected:
  // implementation
  const FSMEntityType::StateInfo &CurState() const
  {
    Assert(_curState >= 0 && _curState < _type->_states.Size());
    return _type->_states[_curState];
  }
  FSMEntityType::StateInfo &CurState()
  {
    Assert(_curState >= 0 && _curState < _type->_states.Size());
    return _type->_states[_curState];
  }
};

struct FSMEntityTypeBankTraits
{
  typedef FSMEntityType Type;
  typedef FSMEntityTypeName NameType;

  //@{ declarations based on Type
  static void ReuseCachedItem(const Type *item){}
  /// default container is RefArray
  typedef RefArray<Type> ContainerType;
  //@}

  // default name comparison
  static int CompareNames(const NameType &n1, const NameType &n2)
  {
    if (n1._mapCondition != n2._mapCondition) return 1;
    if (n1._mapActionIn  != n2._mapActionIn ) return 1;
    if (n1._mapActionOut != n2._mapActionOut ) return 1;
    return stricmp(n1._name, n2._name);
  }
};

class FSMEntityTypeBank: public BankArray<FSMEntityType, FSMEntityTypeBankTraits>
{
};

extern FSMEntityTypeBank FSMEntityTypes;

#endif
