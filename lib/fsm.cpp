// AI - implementation of AI and AICenter

#include "wpch.hpp"

#include "fsm.hpp"
#include "global.hpp"
//#include "loadStream.hpp"
#include <El/ParamFile/paramFile.hpp>
#include "paramArchiveExt.hpp"

FSM::FSM()
: _curState(FinalState)
{
}

FSM::~FSM()
{
}

DEFINE_SERIALIZE_TYPE_INFO_ROOT(FSM);

LSError FSM::Serialize(ParamArchive &ar)
{
  // store the type info for dynamic creation in FSM::CreateObject
  SERIALIZE_TYPE_INFO_ROOT(FSM);

  // state is stored by name, so serialize it in inherited classes
  return LSOK;
}

FSMCoded::FSMCoded()
{
	_state=NULL;
	_special=NULL;
	_nStates=0;
	for( int i=0; i<MaxVar; i++ )
	{
		Var(i)=0; // reset all variables
	}
	for( int i=0; i<MaxVar; i++ )
	{
		VarTime(i)=Time(0); // reset all variables
	}
#if _HELISIM_LARGE_OBJECTID
  VarId() = 0; 
#endif // _HELISIM_LARGE_OBJECTID
}

FSMCoded::FSMCoded
(
	const StateInfo *states, int n,
	const pStateFunction *functions, int nFunc
)
{
	Init(states, n, functions, nFunc);
}

void FSMCoded::Init
(
	const StateInfo *states, int n,
	const pStateFunction *functions, int nFunc
)
{
	_state=states;
	_nStates=n;
	_special=functions;
	_nSpecials=nFunc;
	for( int i=0; i<MaxVar; i++ )
	{
		Var(i)=0; // reset all variables
	}
	for( int i=0; i<MaxVar; i++ )
	{
		VarTime(i)=Time(0); // reset all variables
	}
	_timeOut=TIME_MAX; // default - no timeout
#if _HELISIM_LARGE_OBJECTID
  VarId() = 0; 
#endif // _HELISIM_LARGE_OBJECTID
}

void FSMCoded::SetState( State state, Context *context )
{
	//if( _curState==state ) return;
	_curState=state;
	if( _curState==FinalState ) return;
	const StateInfo &curState=_state[_curState];
	curState.Init(context);
}

bool FSMCoded::Update( Context *context )
{
	if( _curState==FinalState ) return true;
	// check transition function
	const StateInfo &curState=CurState();
	curState.Check(context); // this function may change state
	return _curState==FinalState;
}

void FSMCoded::Refresh( Context *context )
{
	if( _curState==FinalState ) return;
	const StateInfo &curState=CurState();
	curState.Init(context);
}

void FSMCoded::Enter( Context *context )
{
	if (_nSpecials < 1) return;
	_special[0](context);
}

void FSMCoded::Exit( Context *context )
{
	if (_nSpecials < 2) return;
	_special[1](context);
}

void FSMCoded::SetTimeOut( float sec )
{
  // handle special case - maximum (i.e. no) timeout
  if (sec>=FLT_MAX) _timeOut = TIME_MAX;
	else _timeOut=Glob.time+sec;
}

float FSMCoded::GetTimeOut() const
{
	return _timeOut-Glob.time;
}

bool FSMCoded::TimedOut() const
{
	return Glob.time>_timeOut;
}

LSError FSMCoded::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar));

	if (ar.IsSaving())
	{
		RString str = CurState().GetName();
		CHECK(ar.Serialize("curState", str, 1))
	}
	else if (ar.GetPass() == ParamArchive::PassFirst)
	{
		RString str;
		CHECK(ar.Serialize("curState", str, 1))
		_curState = -1;
		for (int i=0; i<_nStates; i++)
		{
			if (stricmp(_state[i].GetName(), str) == 0)
			{
				_curState = i;
			}
		}
		if (_curState < 0) return LSStructure;
	}
	CHECK(::Serialize(ar, "timeOut", _timeOut, 1))
	for (int i=0; i<MaxVar; i++)
	{
		char buffer[256];
		sprintf(buffer, "Var%d", i);
		CHECK(ar.Serialize(buffer, _iVar[i], 1, 0))
	}
	for (int i=0; i<MaxVar; i++)
	{
		char buffer[256];
		sprintf(buffer, "VarTime%d", i);
		CHECK(::Serialize(ar, buffer, _tVar[i], 1, Time(0)))
	}
#if _HELISIM_LARGE_OBJECTID
  char buffer[256];
  sprintf(buffer, "VarId");
  CHECK(::Serialize(ar, buffer, _waypointIdStatic, 1));
#endif // _HELISIM_LARGE_OBJECTID
	return LSOK;
}
