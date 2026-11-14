#ifdef _MSC_VER
#pragma once
#endif

#ifndef _FSM_HPP
#define _FSM_HPP

// universal FSM core
#include <Es/Containers/staticArray.hpp>
#include <Es/Strings/rString.hpp>
//#include "loadSave.hpp"
#include <El/Time/time.hpp>
#include <El/ParamArchive/serializeClass.hpp>
#include "objectId.hpp"

class GameValue;
class GameVarSpace;

#if _SUPER_RELEASE || !defined _WIN32
  #define DEBUG_FSM 0
#else
  #define DEBUG_FSM 1
#endif

#if DEBUG_FSM
#define MSG_FSM_STATE_CHANGED (WM_USER + 0)
#define MSG_FSM_EVENT_HANDLE  (WM_USER + 1)
#endif

class FSM : public SerializeClass
{
public:
  typedef int State;
  enum {FinalState=-1};

  typedef void Context;

protected:
  State _curState;

public:
  FSM();
  virtual ~FSM();

  void SetState(State state) {_curState = state;}
  State GetState() const {return _curState;}

  virtual void SetState(State state, Context *context) = 0;
  /// debugging - this FSM name
  virtual RString GetDebugName() const {return RString();}
  /// debugging: name + state
  RString GetDebugState() const {return Format("%s:%s",cc_cast(GetDebugName()),cc_cast(GetStateName()));}
  /// debugging - current state name
  virtual const char *GetStateName() const = 0;

  /// return the unique id, used for reference to mission FSM in the world
  virtual int GetId() const {return 0;}
  /// set the unique id, used for reference to mission FSM in the world
  virtual void SetId(int id) {}

  // variables
  virtual int &Var(int i) = 0;
  virtual int Var(int i) const = 0;

#if _HELISIM_LARGE_OBJECTID
  virtual OIDInt &VarId() = 0;
  virtual OIDInt VarId() const = 0;
#endif // _HELISIM_LARGE_OBJECTID

  virtual Time &VarTime(int i) = 0;
  virtual Time VarTime(int i) const = 0;

  virtual void SetTimeOut(float sec) = 0;
  virtual float GetTimeOut() const = 0;
  virtual bool TimedOut() const = 0;

  virtual bool Update(Context *context) = 0; // true -> final state reached
  virtual void Refresh(Context *context) = 0; // use only when FSM state is supposed to be lost
  virtual void Enter(Context *context) = 0;
  virtual void Exit(Context *context) = 0;
  
  /// check if FSM needs to suspend other lower priority FSMs
  virtual bool IsExclusive(Context *context) {return false;}

  /// execute expression in environment of FSM
  virtual bool Evaluate(GameValue &result, const char *expression) {return false;}
  /// access to the variable state
  virtual GameVarSpace *GetVars() {return NULL;}
  virtual const GameVarSpace * GetVars() const {return NULL;}

  LSError Serialize(ParamArchive &ar);

  DECL_SERIALIZE_TYPE_INFO_ROOT_ONLY(FSM);

#if DEBUG_FSM
  virtual void Debug() {}
#else
  void Debug() {}
#endif
  virtual void EnableDebugLog(bool enable) {}
};

class FSMCoded : public FSM
{
	public:
  typedef FSM base;

	typedef void StateFunction( Context *context );
	typedef StateFunction * pStateFunction;

	class StateInfo
	{
	public:
    StateInfo(){}
	protected:
		friend class FSMCoded;
		const char *_name;
		StateFunction *_init;
		StateFunction *_check;
	
		StateInfo( const char *name, StateFunction *init, StateFunction *check )
		:_name(name),_init(init),_check(check)
		{}
		void Init( Context *context ) const {_init(context);}
		void Check( Context *context ) const {_check(context);}
		const char *GetName() const {return _name;}
		ClassIsSimpleZeroed(StateInfo);
	};

	enum {MaxVar=8};
	
	private:

  const StateInfo *_state;
	const pStateFunction *_special;	// special functions
																		// 0 - entry
																		// 1 - exit
	Time _timeOut;
  /*! Each index has its own meaning:
  Var(0) - waypoint index
  Var(1) - waypoint type
  Var(2) - waypoint idStatic
  Var(3) - waypoint id
  Var(4) - waiting for synchronization
  Var(5) - counter for Brown movement
  Var(6) - recursion level during waypoint setting
  */
	int _iVar[MaxVar];
#if _HELISIM_LARGE_OBJECTID
  OIDInt _waypointIdStatic;
#endif // _HELISIM_LARGE_OBJECTID
	Time _tVar[MaxVar];

	int _nStates;
	int _nSpecials;

	const StateInfo &CurState() const
	{
		Assert( _curState>=0 && _curState<_nStates );
		return _state[_curState];
	}

	public:
  FSMCoded();	
  FSMCoded
  (
    const StateInfo *states, int n,
    const pStateFunction *functions = NULL, int nFunc = 0
  ); // typical constructor - static state space
  void Init
  (
    const StateInfo *states, int n,
    const pStateFunction *functions, int nFunc
  );

  // FSM interface
  virtual void SetState( State state, Context *context );
	virtual const char *GetStateName() const {return CurState().GetName();}

	// variables
	virtual int &Var( int i ) {return _iVar[i];}
	virtual int Var( int i ) const {return _iVar[i];}

#if _HELISIM_LARGE_OBJECTID
  virtual OIDInt &VarId() {return _waypointIdStatic;}
  virtual OIDInt VarId() const {return _waypointIdStatic;}
#endif // _HELISIM_LARGE_OBJECTID

	virtual Time &VarTime( int i ) {return _tVar[i];}
	virtual Time VarTime( int i ) const {return _tVar[i];}

	virtual void SetTimeOut( float sec );
	virtual float GetTimeOut() const;
	virtual bool TimedOut() const;

	virtual bool Update( Context *context ); // true -> final state reached
	virtual void Refresh( Context *context ); // use only when FSM state is supposed to be lost
	virtual void Enter( Context *context );
	virtual void Exit( Context *context );

	LSError Serialize(ParamArchive &ar);
};

// type-safe implementation of FSM
template <class ContextType>
class FSMTyped: public FSMCoded
{
  typedef FSMCoded base;

	public:
	typedef void StateFunction( ContextType *context );
	typedef StateFunction * pStateFunction;
	class StateInfo: public base::StateInfo
	{
		public:
		StateInfo( const char *name, StateFunction *init, StateFunction *check )
		:base::StateInfo(name,(base::StateFunction *)init,(base::StateFunction *)check)
		{}
	};

	FSMTyped() : base() {}
	FSMTyped
	(
		const StateInfo *states, int n,
		const pStateFunction *functions = NULL, int nFunc = 0
	);
	~FSMTyped(){}

	void SetState( State state, ContextType *context ) {base::SetState(state,context);}
	bool Update( ContextType *context ) {return base::Update(context);}
};

template <class ContextType>
FSMTyped<ContextType>::FSMTyped
(
	const StateInfo *states, int n,
	const pStateFunction *functions, int nFunc
)
:base(states,n,(const base::pStateFunction *)functions,nFunc)
{
}

#endif
