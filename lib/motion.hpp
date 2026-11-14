#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MOTION_HPP
#define _MOTION_HPP

/*!
\file
Interface file for movement diagram
*/

#include <El/Enum/dynEnum.hpp>
#include "El/ParamFile/paramFile.hpp"
#include "El/Time/time.hpp"
#include "rtAnimation.hpp"

template<>
struct BankTraits<AnimationRT>: public DefLinkBankTraits<AnimationRT>
{
  typedef const AnimationRTName &NameType;
  static int CompareNames( const AnimationRTName &n1, const AnimationRTName &n2 )
  {
    // compare pointers
    int d = strcmpi(n1.name,n2.name);
    if (d) return d;
    d = ComparePointerAddresses(n1.skeleton.GetRef(),n2.skeleton.GetRef());
    return d;
  }
};

extern BankArray<AnimationRT> AnimationRTBank;

//! fictious enum - only in order to have typechecking
#ifndef DECL_ENUM_MOVE_ID
#define DECL_ENUM_MOVE_ID
DECL_ENUM(MoveId)
#endif
DEFINE_ENUM_BEG(MoveId)
MoveIdNone=-1
DEFINE_ENUM_END(MoveId)

TypeIsSimple(MoveId)

class MotionType;

//! Class containing soem useful static functions for interpolation
class InterpolationFunctions
{
public:
  static inline float LerpWrap01(float x, float y, float t);

  // Smoothing algorithms for linear function f(x) = x/totalTime | x = elapsedTime
  // If elapsedTime is a linearly updated variable, these functions serve to retrieve a smoothed
  // version of any variable the change of which is not supposed to be linear.

  //! Smoothing in 3 parts (accelerate,linear,brake)
  static float SmoothSimple3Parts(float elapsedTime, float totalTime = 1);

  //! Smoothing in 2 parts (accelerate,brake)
  static float SmoothSimple2Parts(float elapsedTime, float totalTime = 1);

  //! Heuristic smoothing of y = x based on y = x^3 | x ~=<0,1>
  static float SmoothPower3(float x);

  /**
  * Applies constant acceleration or braking according to current value, target value and current speed.
  *
  * @param remaining Difference between current value and target value
  * @return New value and speed
  */
  static void AccelerateBrakeConstant(float &outValue, float &outSpeed, float const remaining, float const acceleration, float const deceleration, float time);
};

typedef InterpolationFunctions LerpWrapHelper;

// Motion path

class MovesTypeBase;
class Man;

DECL_ENUM(MoveFinishF)

struct ActionContextBase: public RefCount
{
  bool _done;

  MoveFinishF function;
  ActionContextBase() {function = MoveFinishF(0);_done=false;}

  bool DoOnce() {bool wasDone = _done; _done = true; return !wasDone;}

  //!called by anim hooks
  virtual void Event(int userType, const RStringB& param) {}
  /// called once given animation is started
  virtual void Started() {}
  /// called once given animation is canceled
  virtual void Cancel() {}
  /// called once given context is finished
  virtual void Do(Man *ai) {}
  /// called while animation is being performed
  /**
  No guarantee on how many times it will be called or if is called at all
  Use only for visual effects and similar things.
  */
  virtual void Progress(float progress) {}
  virtual LSError Serialize(ParamArchive &ar) = 0;
  static ActionContextBase *CreateObject(ParamArchive &ar);

  virtual MoveId GetPlayMove() const {return MoveIdNone;}
};

struct MotionPathItem
{
  MoveId id;
  mutable Ref<ActionContextBase> context;
  
  MotionPathItem(){}
  MotionPathItem(MoveId i, ActionContextBase *c=NULL)
  {
    id = i; context = c;
  }
  void operator = (const MotionPathItem &item)
  {
    //DoAssert(!context || context==item.context); //TODO (temporary remove)
    id = item.id, context = item.context;
  }
  
  void Done() {context = nullptr;}
  //@{ helper functions for event reporting
  void ReportStarted(){if (context) context->Started();}
  //@}

  LSError Serialize(const char *prefix, const MovesTypeBase *type, ParamArchive &ar);
};
TypeIsMovableZeroed(MotionPathItem)

class MotionPath: public AutoArray<MotionPathItem, MemAllocLocal<MotionPathItem,64> >
{
public:
  MotionPath();
  ~MotionPath();
};


// Action map

struct ActionMapName
{
  ConstParamEntryPtr entry;
  const MotionType *motion;
};

#include <Es/Memory/normalNew.hpp>

/// define a movement layer ID
enum MoveLayer
{
  MovePrimary,
  MoveGesture
};

/// distinguish actions in a separate layers
struct MoveIdExt
{
  MoveId id;
  MoveLayer layer;

  MoveIdExt(){}
  MoveIdExt(MoveId mid, MoveLayer mlayer):id(mid),layer(mlayer){}

  operator MoveId() const
  {
    // most often we can handle only the primary layer
    if(layer!=MovePrimary)
    {
      Fail("Secondary layer not supported here");
      return MoveIdNone;
    }
    return id;
  }
  static MoveIdExt none;
};

TypeIsSimple(MoveIdExt)

//! Motion capture - normal actions
/*!
define list of actions corresponding to normal moves
*/
class ActionMap: public RefCount
{
public:
  //! fictions enum - used just to have type checking
  enum Index
  {
    IndexNone = -1
  };

private:
  //! List of actions defined by the config
  /*!
    The list is addressed by names like "ReloadMagazine" (using dynamic enums)
  */
  AutoArray<MoveIdExt> _actions;
  ActionMapName _name;

  // min is most combat
  // max is most safe
  int _upDegree;
  float _turnSpeed;
  float _limitFast;
  bool _useFastMove;

  //!{ Leaning parameters
  float _leanRRot;
  float _leanRShift;
  float _leanLRot;
  float _leanLShift;
  //!}

public:
  ActionMap(const ActionMapName &name);

  const ActionMapName &GetName() const {return _name;}
  int NActions() const {return _actions.Size();}
  const MoveIdExt &GetAction(Index index) const {return _actions[index];}
  int GetUpDegree() const {return _upDegree;}
  float GetTurnSpeed() const {return _turnSpeed;}
  float GetLimitFast() const {return _limitFast;}
  bool GetUseFastMove() const {return _useFastMove;}
  float GetLeanRRot() const {return _leanRRot;}
  float GetLeanRShift() const {return _leanRShift;}
  float GetLeanLRot() const {return _leanLRot;}
  float GetLeanLShift() const {return _leanLShift;}

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

template<>
struct BankTraits<ActionMap>: public DefBankTraits<ActionMap>
{
  typedef const ActionMapName &NameType;
  static int CompareNames( NameType n1, NameType n2 )
  {
    // compare pointers
    if (n1.entry!=n2.entry) return 1;
    if (n1.motion!=n2.motion) return 1;
    return 0;
  }
};

// Blend animations

struct BlendAnimTypeName
{
  // MovesType is defined by pair: LODShape and MotionTypeBase
  const MotionType *motion;
  ConstParamEntryPtr cfg;
};

class BlendAnimType: public RemoveLinks, public BlendAnimSelections
{
  BlendAnimTypeName _name;

public:
  BlendAnimType(const BlendAnimTypeName &name);
  const BlendAnimTypeName &GetName() const {return _name;}
};

template <>
struct BankTraits<BlendAnimType>: public DefLinkBankTraits<BlendAnimType>
{
  // default name is character
  typedef const BlendAnimTypeName &NameType;
  // default name comparison
  static int CompareNames( BlendAnimTypeName n1, BlendAnimTypeName n2 )
  {
    return n1.motion!=n2.motion || n1.cfg!=n2.cfg;
  }
};

typedef BankArray<BlendAnimType> BlendAnimTypes;
extern BlendAnimTypes GBlendAnimTypes;

// Move info

struct MoveVariant
{
  MoveId _move;
  float _probab;
};
TypeIsSimple(MoveVariant)

typedef bool (*MoveInfoPreloadFunc)(ParamEntryPar entry);

#include <Es/Memory/normalNew.hpp>

enum HookType
{
  AH_UNKNOWN,
  AH_SOUNDEDGE, //legacy support
  AH_EVENT,
};

struct AnimHook
{
  float _time;
  HookType _type;
  int _userType;
  RStringB _param;
  AnimHook(): _time(0), _type(AH_UNKNOWN), _userType(0) {}
  AnimHook(HookType type, float time, int userType, RStringB param): _time(time), _type(type), _userType(userType), _param(param) {}
};
TypeIsGeneric(AnimHook);

// information about a single move state in the animation graph
class MoveInfoBase : public RefCount
{
protected:
  //! _subclasses indexes driven by MotionType::_animSubclasses enum
  RefArray<MoveInfoBase> _subAnim;
  Ref<AnimationRT> _move;
  Ref<ActionMap> _actions;

  float _speed;
	float _fps;
	float _speedMult;
  float _skillSpeedCoef;
  float _relSpeedMin, _relSpeedMax;
  float _interpolSpeed;
  float _walkcycles;
  //! How much of the anim must be played before interpolation from it can start
  float _minPlayTime;

  bool _terminal;
  bool _soundEnabled;
  int _interpolRestart;

  RStringB _soundOverride; // default - no override
  AutoArray<AnimHook> _hooks; // in what relative positions action should start

  MoveId _equivalentTo;

public:
  MoveInfoBase(MotionType *motion, ParamEntryPar entry);

  static bool Preload(ParamEntryPar entry);

  __forceinline operator AnimationRT *() const {return _move;}
  __forceinline AnimationRT *operator ->() const {return _move;}
  __forceinline ActionMap *GetActionMap() const {return _actions;} //Beware! It does not depend on _subAnim ix. Should be called to subAnim already resolved

  __forceinline float GetLooped() const {return _move ? _move->GetLooped() : true;}
  float GetSpeed() const;
  __forceinline float GetRelSpeedMin() const {return _relSpeedMin;}
  __forceinline float GetRelSpeedMax() const {return _relSpeedMax;}
  __forceinline float GetInterpolSpeed() const {return _interpolSpeed;}
  __forceinline int GetInterpolRestart() const {return _interpolRestart;}
  __forceinline float GetWalkcycles() const {return _walkcycles;}
  __forceinline bool GetTerminal() const {return _terminal;}
  __forceinline bool GetSoundEnabled() const {return _soundEnabled;}
  __forceinline const AutoArray<AnimHook>& GetAnimHooks() const {return _hooks;}
  __forceinline const RStringB &GetSoundOverride() const {return _soundOverride;}
  MoveId GetEquivalentTo() const {return _equivalentTo;}
  virtual bool CanBlendStep() const {return false;}
  __forceinline float GetMinPlayTime() const { return _minPlayTime; }
  virtual ActionContextBase *GetAutoContext() const {return nullptr;}
	__forceinline float GetOriginalSpeed() const
	{
		float speed = _speed;
		if(_fps != 0 && _move)
		{
			speed = _speedMult / ((float)_move->GetOriginalKeyframeCount() / _fps);
		}
		return speed;
	}


  virtual void LoadAsSubclass(MotionType *motion, ParamEntryPar entry, ParamEntryPar actionMaps);
  MoveInfoBase *SubAnim(int ix) { return (ix<_subAnim.Size() && _subAnim[ix]) ? _subAnim[ix] : this; }

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

// Motion edge

#ifndef DECL_ENUM_MOTION_EDGE_TYPE
#define DECL_ENUM_MOTION_EDGE_TYPE
DECL_ENUM(MotionEdgeType)
#endif
DEFINE_ENUM_BEG(MotionEdgeType)
  MEdgeNone,
  MEdgeSimple,
  MEdgeInterpol
DEFINE_ENUM_END(MotionEdgeType)

struct MotionEdge
{
  enum {CostScale=1000};
  short target;
  short cost; // <0 means infinity
  SizedEnum<MotionEdgeType,char> type;

  MotionEdge() : _immediate(false) {target=-1,cost=-1,type=MEdgeNone;}
  MotionEdge( int t, int c, MotionEdgeType p, bool immediate)
    : _immediate(immediate) {target=t,cost=c,type=p;}  
  float GetCost() const {return cost*(1.0f/CostScale);}

  //! Suggests that minPlayTime parameter should be ignored in this case
  bool _immediate;
};

TypeIsSimple(MotionEdge)

struct RepairMove
{
  Vector3 dir;
  float fLenght;
};

TypeIsSimple(RepairMove)

typedef AutoArray<MotionEdge> MotionEdges;

/// Motion type
class MotionType
{
protected:
  DynEnum _actions;
  DynEnum _moveIds;
  DynEnum _animSubclasses;
  BankArray<ActionMap> _actionMaps;
  BankArray<BlendAnimType> _blendAnimTypes;

  AutoArray<MotionEdges> _vertex;
  Ref<Skeleton> _skeleton;

  Ref<ActionMap> _noActions; // always have some actions ready

  ConstParamEntryPtr _entry;
public:
  AutoArray<int> _animEnumToSubIx;
  static const int NoSubclassIx = -1;
  enum EAnimSubclass
  {
    ASOriginal,
    ASInjuredLight,
    ASInjuredHeavy,
    ASN
  };
  
public:
  MotionType();
  ~MotionType();
  const RStringB &GetEntryName() const {return _entry->GetName();}
  const DynEnum &GetActions() const {return _actions;}

  void Load( ParamEntryVal entry );
  void Unload();
  ParamEntryVal GetEntry() const {return *_entry;}

  const EnumName *GetMoveIdNames() const {return _moveIds.GetEnumNames();}
  RStringB GetMoveName( MoveId id ) const;
  //!the same as GetMoveName but without <none> translation
  RStringB GetMoveNameEx( MoveId id ) const;
  MoveId GetMoveId( RStringB name ) const;
  /// interface for accessing MoveId in additional layers
  virtual MoveId GetMoveId(MoveLayer layer, RStringB name ) const {return MoveIdNone;}
  
  int MoveIdN() const {return _moveIds.FirstInvalidValue();}

  ActionMap::Index FindActionIndex(RString action) const {return (ActionMap::Index)_actions.GetValue(action);}

  const MoveIdExt &GetMove(int upDegree, ActionMap::Index index) const;
  const MoveIdExt &GetMove(int upDegree, RString action) const {return GetMove(upDegree, (ActionMap::Index)_actions.GetValue(action));}
  const MoveIdExt &GetMove(const ActionMap *map, RString action) const;

  void InitNoActions(ParamEntryPar cfg);

  ActionMap *NewActionMap(ParamEntryPar cfg);
  BlendAnimType *NewBlendAnimType(ParamEntryVal cfg);
  Skeleton *GetSkeleton() const {return _skeleton;}

  ActionMap *GetNoActions() const {return _noActions;}

  //@{ subclasses maintanance
  int NSubClasses() const { return _animSubclasses.FirstInvalidValue(); }
  const RStringB &GetSubClassName( int ix ) const { return _animSubclasses.GetName(ix); }
  int GetSubClassIx( const char *name ) const { return _animSubclasses.GetValue(name); }
  //@}

  int EdgeCost( MoveId a, MoveId b ) const;
  const MotionEdge &Edge( MoveId a, MoveId b ) const;
  void AddEdge(MoveId a, MoveId b, MotionEdgeType type, float cost, bool immediate = false);
  void DeleteEdge(MoveId a, MoveId b);
  /// tries to find moveId A, so that targetMove connects to A (only edges of type MEdgeSimple)
  MoveId FindConnectedSource(MoveId targetMove) const;

  bool FindPath(MotionPath &path, MoveId from, MotionPathItem to) const;
};

// Moves type

struct MovesTypeName
{
  // MovesType is defined by MotionType only
  RStringB skeletonName;
  
  MovesTypeName(){}
  MovesTypeName(RStringB name):skeletonName(name){}
};

/// basic shared info about individual moves (animations)
class MovesTypeBase: public RefCountWithLinks, public MotionType
{
  MovesTypeName _name; // shape and motionType
  RefArray<MoveInfoBase> _moves;

public:
  MovesTypeBase();
  ~MovesTypeBase();

  virtual void Load(const MovesTypeName &name);

  const MovesTypeName &GetName() const {return _name;}
  AnimationRT *GetAnimation( MoveId move, int subIx ) const
  {
    if (move == MoveIdNone) return NULL; // no animation
    if (subIx>=0) return *_moves[move]->SubAnim(subIx);
    else return *_moves[move];
  }
  ActionMap *GetActionMap(MoveId move, int subIx ) const;
  const MoveInfoBase *GetMoveInfoBase(MoveId move, int subIx ) const
  {
    if (move == MoveIdNone) return NULL; // no animation
    if (subIx>=0) return _moves[move]->SubAnim(subIx);
    else return _moves[move];
  }
  MoveId GetEquivalent( MoveId move ) const;

  virtual MoveInfoPreloadFunc GetPreloadFunc() const {return &MoveInfoBase::Preload;}

  virtual MoveInfoBase *NewMoveInfo(ParamEntryPar entry)
  {
    return new MoveInfoBase(this, entry);
  }
};

template <>
struct BankTraits<MovesTypeBase>: public DefLinkBankTraits<MovesTypeBase>
{
  // default name is character
  typedef const MovesTypeName &NameType;
  // default name comparison
  static int CompareNames( NameType n1, NameType n2 )
  {
    return strcmpi(n1.skeletonName,n2.skeletonName);
  }
};

template <>
struct BankTraitsExt< BankTraits<MovesTypeBase> >: public DefBankTraitsExt< BankTraits<MovesTypeBase> >
{
  /// create an object based on the name
  static Type *Create(NameType name)
  {
    Type *instance = new Type();
    instance->Load(name);
    return instance;
  }
};

typedef BankArray<MovesTypeBase> MovesTypeBaseBank;
extern MovesTypeBaseBank MovesTypesBase;

/// result of AdvanceMoveQueue function

struct AdvanceQueueInfo
{
  float moveX;
  float moveZ;
  bool step;
  bool stepLeft;
  RStringB soundOverride;
  Ref<ActionContextBase> context;

  AdvanceQueueInfo()
  {
    moveX = 0;
    moveZ = 0;
    step = false;
    stepLeft = false;
  }
};

typedef float (MoveInfoBase::*TestValueBase)() const;


class MotionInfo
{
protected:
  MotionPathItem _move; ///< Current move.
  float _time; ///< Current position in _move animation.
  float _dTime; ///< delta of _time done in the last simulation
  AnimationRTBufferCopyOnWrite _buffer; ///< Cyclic buffer used for streaming

public:
  MotionInfo()
    : _move(ENUM_CAST(MoveId, MoveIdNone)),
    _time(0),_dTime(0)
  {
  }
  // access to members
  const MotionPathItem &GetMove() const {return _move;}
  float GetTime() const {return _time;}
  float GetTimeDelta() const {return _dTime;}
  AnimationRTBufferCopyOnWrite &GetBuffer() {return _buffer;}
  const AnimationRTBuffer &GetBuffer() const {return _buffer;}
  /// primary function to change the move
  void SetMove(MotionPathItem move, bool reportStarted)
  {
    if (move.id != _move.id) _buffer.Init();
    _move = move;
    if (reportStarted) _move.ReportStarted();
  }
  /// primary function to change the time
  void SetTime(float time, float dTime=0)
  {
    _time = time;
    _dTime = dTime;
  }
  void ClearContext(bool cancel)
  {
    if (cancel) _move.context->Cancel();
    _move.context = NULL;
  }
  // for physics
  void SetState(MotionPathItem move, float time, float dTime=0)
  {
    if (move.id != _move.id) _buffer.Init();
    _move = move;
    _time = time;
    _dTime = dTime;
  }
  // for physics - TODO: different implementation - check what is accessed from code using that
  void SetTempMove(MoveId move)
  {
    _move.id = move;
  }
  LSError Serialize(const char *prefix, const MovesTypeBase *type, ParamArchive &ar);

  void VisualCopy(const MotionInfo &src)
  {
    _move = src._move.id;
    _time = src._time;
    _dTime = src._dTime;
    _dTime = src._dTime;
    _buffer = src._buffer;
  }
};

/// visual state for the animation
class MovesVisualState
{
  friend class Moves;
  
  MotionInfo _primary;
  MotionInfo _secondary;
  bool _paused;
  bool _finished; ///< the animation has finished in previous step and we are expected to start a new one
  float _primaryFactor; ///< (0,1) interpolation btw. primary and secondary move. _primaryFactor = 0, take just _secondaryMove. 
  
  //! (0 to 1), updated in a linear manner (unlike _primaryFactor)
  float _blendTime;

  public:
  MovesVisualState();

  void Interpolate(const MovesVisualState & t1state, float t, MovesVisualState& interpolatedResult) const;

  //! Sets the blend time to reverse value and gets new value of primary factor
  void ReverseBlending();

  //! Sets primary factor to 1
  void Complete();
  RString DiagText() const;
};

//! Factor of step of a single animation
struct BlendedStep
{
  MoveId _move;
  float _factor;
  //! Control against the combination of small step and high speed
  bool _static;

  BlendedStep(MoveId move, bool isStatic, bool connected) 
    : _move(move), _factor(connected ? 1 : 0), _static(isStatic) {}
};
TypeIsSimple(BlendedStep)
typedef AutoArray<BlendedStep,MemAllocLocal<BlendedStep,8> > TBlendedSteps;

//! Array of multiple currently blended steps
class AnimStepsBlender
{
  TBlendedSteps _steps;

  //! Previous step is stored to prevent lagging
  Vector3 _lastStep;

public:

  AnimStepsBlender() : _lastStep(0,0,0) {}
  //! Checks and adds new move to the blended set
  void AddMove(MoveId move, MovesTypeBase const *type, int subIx, bool connected = false);
  //! Updates factors of all currently blended steps according to current primary and queue end ("desired" move)
  Vector3 UpdateAndGetStep(float deltaT, float adjustSpeed, MovesTypeBase const *type, MoveId primary, bool connected, MoveId queueEnd, int subIx);
};

/// animation control, including graph of the connecting moves
class Moves
{
protected:
  int _defaultUpDegree;
  int _animSubIx;
  Time _upDegreeChangeTime; ///< time of last change
  // changes are tracked whenever _primaryMove changes

  //MovesVisualState _vs;


public:
  /// value of _primaryFactor interpreted as 0.0f (avoid comparing with 0 directly because of floating point representation)
  static const float _primaryFactor0;
  /// value of _primaryFactor interpreted as 1.0f (avoid comparing with 1 directly because of floating point representation)
  static const float _primaryFactor1;

protected:

  struct PlayMove
  {
    /// scripted move, received from a remote object
    MoveId _wanted;
    /// scripted move, received from a remote object
    MoveId _playing;

    PlayMove():_wanted(MoveIdNone),_playing(MoveIdNone){}
  };

  PlayMove _playMove;
  /// shortest "path" to desired move
  /** desired move is last member of _queueMove */
  MotionPath _queueMove;

  MotionPathItem _forceMove; ///< do not do anything else until this move is completed

  /// a queue of moves waiting to become a _forceMove
  AutoArray<MotionPathItem,MemAllocLocal<MotionPathItem,4>> _forcedQueue;

  //! Mix multiple steps together
  AnimStepsBlender _blendedSteps;


  void QueueMoveClear()
  {
    for (int i=0; i<_queueMove.Size(); i++) DoAssert(!_queueMove[i].context);
    _queueMove.Resize(0);
  }

  void SetPrimaryMove(MovesVisualState &vs, const MovesTypeBase *type, MotionPathItem item); 

public:
  MoveId GetCurrentPlayMove(const MovesVisualState &vs) const;

  Moves(int defaultUpDegree);

  /// get the move to which the queue leads (ending move of the queue)
  MoveId GetDesiredMove() const;
  RString GetCurrentMove(const MovesVisualState &vs, const MovesTypeBase *type) const;
  RString GetCurrentMoveEq(const MovesVisualState &vs, const MovesTypeBase *type) const;
  int GetActUpDegree(const MovesVisualState &vs, const MovesTypeBase *type) const; // helper function
  Time GetUpDegreeChangeTime() {return _upDegreeChangeTime;}
  void UpDegreeChanged();
  const MotionPathItem &GetPrimaryMove(const MovesVisualState &vs) const {return vs._primary.GetMove();}
  float GetPrimaryTime(const MovesVisualState &vs) const {return vs._primary.GetTime();}
  float GetPrimaryTimeDelta(const MovesVisualState &vs) const {return vs._primary.GetTimeDelta();}
  float GetPrimaryTimeLeft(const MovesVisualState &vs, const MovesTypeBase *type) const;
  float GetPrimaryFactor(const MovesVisualState &vs) const {return vs._primaryFactor;}
  bool GetFinished(const MovesVisualState &vs) const {return vs._finished;}
  bool GetPaused(const MovesVisualState &vs) const {return vs._paused;}
  const MotionPathItem &GetSecondaryMove(const MovesVisualState &vs) const {return vs._secondary.GetMove();}
  float GetSecondaryTime(const MovesVisualState &vs) const {return vs._secondary.GetTime();}
  float GetSecondaryTimeDelta(const MovesVisualState &vs) const {return vs._secondary.GetTimeDelta();}
  int GetMoveQueueSize() const {return _queueMove.Size();}
  void ResetMoveQueue() {QueueMoveClear();}
  void SetMoveQueueContext(ActionContextBase *context);

  float GetMajorAnimationTime(const MovesVisualState &vs, const MovesTypeBase *type) const;
  bool IsActionInProgress(const MovesVisualState &vs, MoveFinishF action) const;
  const ActionContextBase *GetActionInProgress(const MovesVisualState &vs) const;

  template <class Type>
  void ForEachAction(Type &func, const MovesVisualState &vs) const
  {
    ActionContextBase *context = GetPrimaryMove(vs).context;
    if (context && func(context)) return;
    // check moves waiting in queues
    context = GetForceMove().context;
    if (context && func(context)) return;

    for (int i=0; i<_forcedQueue.Size(); i++)
    {
      ActionContextBase *context = _forcedQueue[i].context;
      if (context && func(context)) return;
    }
  }

  bool IsForceMove() const {return _forceMove.id!=MoveIdNone || _forcedQueue.Size()>0;}
  const MotionPathItem &GetForceMove() const {return _forceMove;}
  /// \param permanent -> move will remain set until replaced by another force move 
  void AddForceMove(MotionPathItem item, MovesVisualState& vs);

  void ResetForcedQueue();

  void Pause(MovesVisualState &vs, MoveId move)
  {
    if(vs._primary.GetMove().id == move)
    {
      vs._paused = true;
    }
  }

  void Continue(MovesVisualState &vs)
  {
    vs._paused = false;
  }

  // for physics
  void SetState(MovesVisualState &vs, MotionPathItem primaryMove, float primaryTime, float primaryTDelta,
    MotionPathItem secondaryMove, float secondaryTime, float secondaryTDelta, float primaryFactor, bool finished)
  {
    vs._primary.SetState(primaryMove, primaryTime, primaryTDelta);
    vs._secondary.SetState(secondaryMove, secondaryTime, secondaryTDelta);
    vs._primaryFactor = primaryFactor;
    vs._finished = finished;
  }
  // for physics
  MoveId SetTempPrimaryMove(MovesVisualState &vs, MoveId move)
  {
    MoveId old = vs._primary.GetMove().id;
    vs._primary.SetTempMove(move);
    return old;
  }
  void CancelPrimaryAction(MovesVisualState &vs);
  void CancelForcedActions();

  void PatchNoMove(MovesVisualState &vs, MotionPathItem item);
  void ResetMovement(MovesVisualState &vs, const MovesTypeBase *type, MotionPathItem item);
  void SwitchMove(MovesVisualState &vs, MoveId id, ActionContextBase *context);


  void AdvanceForcedQueue();
  // return true if anything has changed
  bool AdvanceMoveQueue(MovesVisualState &vs, const MovesTypeBase *type, float deltaT, float adjustSpeed, AdvanceQueueInfo &info, bool &done, bool blendception = false, Entity *man = NULL);
  bool ChangeMoveQueue(MovesVisualState &vs, const MovesTypeBase *type, bool &animChanged, MotionPathItem item);

  static void PrepareMatrices(const MovesTypeBase *type, Matrix4Array &matrices, LODShape *shape, int level, const BlendAnims &blend);

  /// animations streaming - load needed frames
  void LoadAnimationPhases(MovesVisualState &vs, const MovesTypeBase *type, float pastTimeNeeded);

  /// verify object integrity (debugging)
  bool CheckValidState(const MovesVisualState &vs, const MovesTypeBase * type) const;

  /// get blending setup for current animations
  void GetBlendAnims(const MovesVisualState &vs, BlendAnims &blend, const MovesTypeBase * type, float maskFactor=1, const BlendAnimSelections *mask=NULL) const;
  /// get "neutral" position (always from the 1st frame)
  void GetBlendAnimsNeutral(const MovesVisualState &vs, BlendAnims &blend, const MovesTypeBase * type) const;
  
  //void PrepareShapeDrawMatrices(const MovesTypeBase *type, Matrix4Array &matrices, LODShape *shape, int level);
  void AnimateBoneMatrix(const BlendAnims &blend, const MovesTypeBase *type, Matrix4 &mat, SkeletonIndex boneIndex) const;

  /// get current position of the point
  Vector3 AnimatePoint(const BlendAnims &blend, const MovesTypeBase *type, LODShape *lShape, int level, int index ) const;

  float ValueTest(const MovesVisualState &vs, const MovesTypeBase *type, TestValueBase func, float defValue = 0) const;

  #define MOVE_INFO_VALUE_BASE_DEF(name, def) \
    float name(const MovesVisualState &vs, const MovesTypeBase *type) const {return ValueTest(vs, type, &MoveInfoBase::name, def);}

  MOVE_INFO_VALUE_BASE_DEF(GetInterpolSpeed, 6)

  void GetRelSpeedRange(const MovesVisualState &vs, const MovesTypeBase *type, float &speedX, float &speedZ, float &minSpd, float &maxSpd);
  RString GetDiagInfo(const MovesVisualState &vs, const MovesTypeBase *type) const;

  LSError Serialize(MovesVisualState &vs, const char *prefix, const MovesTypeBase *type, ParamArchive &ar);
  void ChangePlayMove( MoveId moveM );

  void SetAnimSubIx(int ix) { _animSubIx=ix; }
  int AnimSubIx() const { return _animSubIx; }
protected:
  // used in serialization
  float GetCurrentTime(const MovesVisualState &vs) const;
};

#endif
