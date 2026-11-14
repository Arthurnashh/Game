#ifdef _MSC_VER
#pragma once
#endif

#ifndef _AI_HPP
#define _AI_HPP

#include "../types.hpp"
#include "aiTypes.hpp"
#include "../transport.hpp"
#include <El/ParamArchive/paramArchive.hpp>
#include "../speaker.hpp"

#include <El/Time/time.hpp>
#include <El/ParamArchive/serializeClass.hpp>

#include "../camEffects.hpp"
#include "../titEffects.hpp"

#include "../arcadeWaypoint.hpp"

#include "../fsm.hpp"

#include "../scripts.hpp"

#include "../Network/networkObject.hpp"

#include <time.h>

#include "operMap.hpp"

#include <Es/Threads/multisync.hpp>

#if _ENABLE_CONVERSATION
#include <RV/KBBase/kbCenter.hpp>
#endif

#if _ENABLE_INDEPENDENT_AGENTS || _ENABLE_IDENTITIES
#include "aiTeams.hpp"
#endif

//#define COEF_EXPOSURE                 4e-6F   // coefficient for including exposure into cost
#define COEF_EXPOSURE                   1e-6F   // coefficient for including exposure into cost
#define COEF_EXPOSURE_UNABLE_TO_DEFEND  16e-6F  // coefficient for including exposure into cost

#define DIAG_WANTED_POSITION  0

struct VehicleInitMessages;

///////////////////////////////////////////////////////////////////////////////
// class AI - base class for all classes in hierarchy

struct ExperienceDestroyInfo
{
  float maxCost;
  float exp;
};
TypeIsSimple(ExperienceDestroyInfo)

extern AutoArray<float> ExperienceTable;
extern AutoArray<ExperienceDestroyInfo> ExperienceDestroyTable;

extern float ExperienceDestroyEnemy;
extern float ExperienceDestroyFriendly;
extern float ExperienceDestroyCivilian;
extern float ExperienceDestroyStatic;

extern float ExperienceRenegadeLimit;

extern float ExperienceKilled;
  
// for subordinate soldier only
extern float ExperienceCommandCompleted;
extern float ExperienceCommandFailed;
extern float ExperienceFollowMe;

// for leadership only
extern float ExperienceDestroyYourUnit; 
extern float ExperienceMissionCompleted;
extern float ExperienceMissionFailed;

void AIGlobalInit();
void AIGlobalCleanUp();

extern AutoArray<ArcadeMarkerInfo> markersMap;

///////////////////////////////////////////////////////////////////////////////
// class AILocker

class RoadLink;
//TypeIsSimpleZeroed(InitPtr<RoadLink>);

struct BuildingLockInfo
{
  OLink(Object) house;
  int index;
};
TypeContainsOLink(BuildingLockInfo)

/// used for temporary unlocks of active locks
/**
No memory allocation used - memory is moved by rebinding the pointer
*/

struct AILockerSavedState
{
  AutoArray< InitPtr<RoadLink> > _roadsSoldiers;
  AutoArray< InitPtr<RoadLink> > _roadsVehicles;
  AutoArray<BuildingLockInfo> _buildings;
};

/// for occupied location locking
class AILocker : public TaskEnabledRefCount
{
  /// locked fields
  RefArray<LockField> _fields;  
  /// locked roads (by soldiers)
  AutoArray< InitPtr<RoadLink> > _roadsSoldiers;
  /// locked roads (by vehicles)
  AutoArray< InitPtr<RoadLink> > _roadsVehicles;
  /// locked building positions
  AutoArray<BuildingLockInfo> _buildings;
  
  typedef AILockerSavedState SavedState;

  /// Locks are actually locked in locking map
  bool _tempLocked;
  /// Type of lock (all vehicles or all vehicles but soldiers)
  bool _lockedSoldier;
  /// Type of lock (waiting only, or a long term stop)
  bool _lockedWaiting;
  /// How large is the lock?
  float _lockedRadius;
  /// Size of the locking vehicle (used for the roads and buildings locks)
  float _lockedSize;
  /// Where is the lock?
  Vector3 _lockedBeg;

  /// time of last movement, used to distinguish between waiting/full lock
  Time _lastMovementTime;
  /// Only single thread can access structures at a time
  mutable CriticalSection _criticalSection;

public:
  AILocker();
  ~AILocker();

  /// lock in both layers
  void Lock(Vector3Val pos, float radius, bool soldier, bool canWait, float size);
  /// unlock in both layers
  void Unlock(SavedState *state = NULL);
  /// relock in both layers
  void Relock(SavedState &state);

  /// react to movement - touch the movement time
  void Move();

  /// check if these locker is locking the field
  int IsLocking(OperMapIndex x, OperMapIndex z, bool soldier) const;
  /// check if these locker is locking the road segment
  int IsLocking(const RoadLink *road, bool soldier) const;
  /// check if these locker is locking the building position
  int IsLocking(const Object *house, int index) const;

protected:
  // implementation
  void LockItem(OperMapIndex x, OperMapIndex z, bool soldier);
  void UnlockItem(OperMapIndex x, OperMapIndex z, bool soldier);

  /// lock position with the LockMaskVeh - lock valid only for vehicles
  void LockPosition( Vector3Val pos, float radius, bool soldier, bool waiting, float size);
  /// unlock position with the LockMaskVeh
  void UnlockPosition( Vector3Val pos, float radius, bool soldier, bool waiting, SavedState *state=NULL);
  /// relock position with the LockMaskVeh
  void RelockPosition( Vector3Val pos, float radius, bool soldier, bool waiting, float size, SavedState &state);

  /// lock position with the LockMaskAll - lock valid for both vehicles and men
  void LockPositionAll( Vector3Val pos, float radius, bool soldier, bool waiting, float size);
  /// unlock position with the LockMaskAll
  void UnlockPositionAll( Vector3Val pos, float radius, bool waiting,SavedState *state=NULL);
  /// relock position with the LockMaskAll
  void RelockPositionAll( Vector3Val pos, float radius, bool waiting, SavedState &state);



};

///////////////////////////////////////////////////////////////////////////////
// class AIUnit

extern const Vector3 VUndefined;

#include "../pathSteer.hpp"

//! callback function type - see also FindNearestEmptyCallback
typedef bool FindFreePositionCallback(Vector3Par pos, void *context);

extern bool DefFindFreePositionCallback(Vector3Par pos, void *context);

#ifndef DECL_ENUM_ABILITY_KIND
#define DECL_ENUM_ABILITY_KIND
DECL_ENUM(AbilityKind)
#endif
class MagazineType;

class TargetFunctor
{
public:
  // return true to break enumeration
  virtual bool operator ()(Target *target) const = 0;
};

/// interface to provide information about friendly / enemy
struct SideInfo
{
  virtual bool SideIsFriendly(TargetSide side) const = 0;
  virtual bool SideIsEnemy(TargetSide side) const = 0;
  virtual bool SideIsNeutral(TargetSide side) const = 0;
};

class TargetGhost;

/// Role of long-term FSM
#define BEHAVIOUR_FSM_ENUM(type, prefix, XX) \
  XX(type, prefix, Formation)

#ifndef DECL_ENUM_BEHAVIOUR_FSM_ROLE
#define DECL_ENUM_BEHAVIOUR_FSM_ROLE
DECL_ENUM(BehaviourFSMRole)
#endif
DECLARE_ENUM(BehaviourFSMRole, BFSM, BEHAVIOUR_FSM_ENUM)

/// Role of short-term FSM
#define REACTION_FSM_ENUM(type, prefix, XX) \
  XX(type, prefix, Danger)

#ifndef DECL_ENUM_REACTION_FSM_ROLE
#define DECL_ENUM_REACTION_FSM_ROLE
DECL_ENUM(ReactionFSMRole)
#endif
DECLARE_ENUM(ReactionFSMRole, RFSM, REACTION_FSM_ENUM)


#ifndef DECL_ENUM_COMBAT_MODE
#define DECL_ENUM_COMBAT_MODE
DECL_ENUM(CombatMode)
#endif
DEFINE_ENUM_BEG(CombatMode)
        CMUnchanged,
        CMCareless,
        CMSafe,
        CMAware,
        CMCombat,
        CMStealth
DEFINE_ENUM_END(CombatMode)

struct CombatModeRange
{
  CombatMode beg,end;
  
  CombatModeRange(CombatMode beg, CombatMode end=CMStealth):beg(beg),end(end){}
};

struct CautionRange
{
  float beg,end;
  
  CautionRange(float beg=0.0f, float end=1.0f):beg(beg),end(end){}
};


DEFINE_ENUM_BEG(DangerCause)
  DCEnemyDetected, // the first enemy detected
  DCFire, // fire visible
  DCHit, // vehicle hit
  DCEnemyNear, // enemy very close to me
  DCExplosion, // explosion detected
  DCDeadBodyGroup, // dead soldier from my group found
  DCDeadBody, // other dead soldier found
  DCScream, // hit soldier screaming
  DCCanFire // new opportinity to fire on an enemy target
DEFINE_ENUM_END(DangerCause)

struct TargetToFire
{
  LinkTarget _tgt;
  TargetState _initState;

  TargetToFire():_initState(TargetFireMin){}
  TargetToFire(Target *tgt, TargetState initState):_tgt(tgt),_initState(initState>TargetFireMin ? TargetFireMin : initState){}

  Target *operator -> () const {return _tgt;}
  operator Target *() const {return _tgt;}
};


struct IdentityInfo
{
  RString _name;
  RString _face;
  RString _glasses;
  RString _speaker;
  float _pitch;
};

//! Abstract brain structure


class AIBrain : public AI // TODO: derive AIBrain directly from SerializedObject
{
public:
  friend class SetGunnerStateFunc;
  friend class ClearGunnerMapFunc;

#if _ENABLE_DIRECT_MESSAGES
  enum SelectedChannel
  {
    SCRadio, // prefer radio channel
    SCDirect, // prefer direct speaking
    SCAuto // select channel automatically
  };
  enum GestureEnabled
  {
    GEEnabled,
    GEDisabled,
    GEAuto
  };
  enum VoiceEnabled
  {
    VEEnabled,
    VEDisabled,
    VEAuto
  };
#endif
#if _VBS3
  enum POW_Status
  {
    POWNone = 0,
    POWCaptive = 1,
    POWCanCapture = 2,
    POWHandcuffed = 4,
    POWCaptured = 8
  };
#endif

  enum State
  {
    /// another plan is needed (current partial plan was done or is not present)
    Wait,
    /// operative planing should start - endpoint is set
    Init,
    /// init failed because data are not ready
    InitFailed,
    /// operative plan ready and valid
    Busy,
    /// completed whole strategic plan
    Completed,
    /// repeat search - usually when failed
    Delay,
    /// unit is in vehicle cargo - no planning needed
    InCargo,
    /// unit is stopping (usually because of Getin/getout request)
    Stopping,
    /// we have a plan, but we would like to search a new one
    Replan,
    /// unit is stopped - no planning needed
    Stopped,
    /// operative planning pending
    Planning
  };
  enum PlanningMode
  {
    /// used when not moving
    DoNotPlan,
    /// used in formation when simple path testing is used
    DoNotPlanFormation,
    /// used for formation leader (full path finding used)
    LeaderPlanned,
    /// used for DirectGo (like getin, supply)
    LeaderDirect,
    /// used for vehicle driver
    VehiclePlanned,
  };
  enum Mode
  {
    DirectNormal,
    DirectExact,
    Normal,
    Exact
  };
  enum WatchMode
  {
    WMNo,
    WMDir,
    /// look permanently at given position
    WMPos,
    /// glance at given position
    WMPosGlance,
    /// look permanently at given target
    WMTgt,
    /// look around
    WMAround,
    /// glance at given target
    WMTgtGlance,
    NWatchModes
  };
  enum DisabledAI
  { // note: serialized as value - only add values to retain compatilibity
    DATarget=1, // group commander may not assign targets
    DAMove=2, // do not move
    DAAutoTarget=4, // no automatic target selection
    DAAnim=8, // no automatic animation selection
    _unused_DATeamSwitch=16, // AI disabled because of Team Switch
#if _VBS2 && _VBS3  // disableAI PATHPLAN
    DAPathPlan=32, // don't path plan, just move
#endif
    DAFSM=64,
  };

protected:
  OLinkPermO(Person) _person;   // who we are
  OLinkPermO(Transport) _inVehicle; // which vehicle are we in

  LifeState _lifeState;
  int _disabledAI; // disabled single elements of AI (DisabledAI)
#if !_ENABLE_IDENTITIES
  float _ability;
#endif
  /// "major" is explicitly set via waypoints or player given commands
  CombatMode _combatModeMajor;
  Semaphore _semaphore;
  Time _dangerUntil;

#if DEBUG_FSM
  signed char _fsmAutoDebug;
#else
  static const signed char _fsmAutoDebug = 0;
#endif

  FSM *CreateFSM(const EntityType *type, RString name);

  /// long-term FSM instances
  SRef<FSM> _behaviourFSMs[NBehaviourFSMRole];
  /// short-term FSM instance - react to danger
  SRef<FSM> _reactionFSM;
#if _ENABLE_CONVERSATION
  /// short-term FSM instance - social behaviours
  /** lower priority than _reactionFSM, suspended while _reactionFSM is executed */
  SRef<FSM> _conversationFSM;

  /// talk events to be processed
  /**
  Includes some events other than talk, like when somebody approached us
  */
public:
  struct TalkEvent
  {
    /// who is source of the event
    OLinkPermNO(AIBrain) _from;
    /// content of the event
    Ref<const KBMessageInfo> _event;

    //@{ mimic SerializeClass interface
    LSError Serialize(ParamArchive &ar);
    //@}

    ClassContainsOLink(TalkEvent)
  };
protected:
  /// a queue of talk related events to be processed
  AutoArray<TalkEvent> _talkEvents;

  /// list of other characters we are currently assuming to be in conversation with
  /**
  Any unit which we address or which addresses us is added here.
  It is removed once reasonable, e.g. it goes too far away.
  "Addressing" may be even non-verbal (like saluting, or simply standing in front of someone)

  Currently we are always convesing with one unit only. In future this may be an array of units.
  */
  OLinkPermNO(AIBrain) _conversingWith;
  //OLinkPermNOArray(AIBrain) _conversingWith;

#endif

  /// preferred target for attacking / watching
  TargetToFire _targetAssigned;
  /// leader has enabled firing at the target
  LinkTarget _targetEnableFire;

  /// fictive (ghost) target used for fire sector/suppressive fire control
  RefR<TargetGhost> _ghostTarget;
  
  WatchMode _watchMode;
  /// position to watch (look at)
  Vector3 _watchPos;
  /// position to glance
  Vector3 _glancePos;
  /// target to watch (look at)
  LinkTarget _watchTgt;
  /// target to glance at
  LinkTarget _glanceTgt;
  Time _watchDirSet; // for WMAround
  Vector3 _watchDirHead; //!< direction to watch by head - world space
  Vector3 _watchDir; //!< direction to watch by head and body - world space

  float _nearestEnemyDist2;
  
  /// approximate direction range to enemies we are hiding from - leftmost angle
  Vector3 _enemyDirL;
  /// approximate direction range to enemies we are hiding from - rightmost angle
  Vector3 _enemyDirR;

  /// value of _wantedPosition before transformed by a FSM
  PlanPosition _wantedPositionWanted;
  /// what precision is desired when moving into _wantedPositionWanted
  float _wantedPrecision;
  /// value of _planningMode before transformed by a FSM
  PlanningMode _planningModeWanted;
  /// value of forceReplan flag (passed to SetWantedPosition) before transformed by a FSM
  bool _forceReplanWanted;

  /// forceSpeed < 0 means forceSpeed disabled. Speed forced otherwise.
  float _forceSpeed;

#if _VBS3
  bool _allowSpeech;
#endif
  // captive, changed to int for VBS, can now code different states (see POW)
  int _captive;


  bool _completedReceived;

  bool _noPath;
  bool _updatePath;
  bool _lastPlan;

  /// voice used for radio and other communications
  SRef<Speaker> _speaker;

  /// after cover is left, FSM should set the search parameters. Until this is done, we cannot plan
  bool _planParametersSet;
  
  /// desired end point of strategic plan
  PlanPosition _wantedPosition;
  /// end point of current strategic plan
  PlanPosition _plannedPosition;

  /// desired end point of operation path planning
  PlanPosition _expPosition;

  /// current operative plan searching state
  mutable SRef<OperMap> _map;
  //@{ planning FSM state
  State _state;
  PlanningMode _planningMode;
  Mode _mode;
  /// when failed, we repeat a few more times with updated heuristics
  int _iter;
  /// counter for trying of finding path
  Time _delay;
  
  /// operative path
  Path _path;
  /// index of the path (increased with each operative planning)
  int _pathId;
  
  
  /// target location in the house - which house
  OLink(Object) _house;
  /// target location in the house - which position
  int _housePos;

  /// part of the old path we will reuse
  AutoArray<OperInfoResult> _oldPath;
  
  /// max. time to get into a cover (see also CoverDisposition::_maxCost)
  float _maxCostToCover;
  /// max. delay imposed by a cover (see also CoverDisposition::_maxDelay)
  float _maxDelayByCover;
  //@}

  //@{ current operative plan searching state
  Vector3 _from;
  PlanPosition _to;
  /// when from is not free, path finding started at nearest free location
  bool _fromBusy;
  /// if path is ending on the road, we are happy with it
  bool _toRoad;
  //@}

#if _ENABLE_AI
  //@{ current strategic plan searching state
  SRef<IAIPathPlanner> _planner;
  float _completedTime;
  Time _waitWithPlan;
  int _attemptPlan;
  //@}
#endif

#if _ENABLE_CONVERSATION
  Ref<KBCenter> _kbCenter;
  int _speaking;
  int _listening;

#endif

#if _ENABLE_DIRECT_MESSAGES
  SelectedChannel _selectedChannel;
  GestureEnabled _gestureEnabled;
  VoiceEnabled _voiceEnabled;
#endif
  // when set to true, someone load identity before delayed EH is processed - avoid identity overwrite
  bool _loadIdentityDelayed;

public:
  AIBrain(Person *vehicle);

  Person *GetPerson() const {return _person;}
  void SetPerson(Person *vehicle);
  Transport *GetVehicleIn() const {return _inVehicle;}
  void SetVehicleIn(Transport *vehicle) {_inVehicle = vehicle;}
  EntityAIFull *GetVehicle() const;
  bool IsFreeSoldier() const {return GetVehicleIn() == NULL;}
  bool IsSoldier() const
  {
    return _inVehicle.GetLink() == NULL || IsInCargo();
  }

  static bool FindFreePosition
  (
    Vector3 &pos, Vector3 &normal, bool soldier, const EntityAIType *vehType,
    FindFreePositionCallback *isFree=DefFindFreePositionCallback,
    void *context=NULL
  );

  bool ProcessGetIn2(Transport *veh, UIActionType pos, Turret *turret, int cargoIndex);

  bool ActionDriverFreeLook() const;

  bool LookAroundEnabled() const;

  LifeState GetLifeState() const {return _lifeState;}
  void SetLifeState(LifeState state);

  #define TRANSFER_BRAIN_UPDATE(MessageName,type,name,datatype,rawType,compres, defval, description, xfer, errType, errCoef) \
    type &Transf_##name() {return reinterpret_cast<type &>(_##name);}

  UPDATE_AI_BRAIN_MSG(CreateVehicleBrain,TRANSFER_BRAIN_UPDATE)

  //@{ Check if unit is able to do some things with the current _lifeState
  bool LSIsAlive() const {return _lifeState == LifeStateAlive || _lifeState == LifeStateAsleep || _lifeState == LifeStateUnconscious;}
  bool LSCanSpeak() const {return _lifeState == LifeStateAlive || _lifeState == LifeStateAsleep || _lifeState == LifeStateUnconscious;}
  bool LSCanLead() const {return _lifeState == LifeStateAlive || _lifeState == LifeStateAsleep || _lifeState == LifeStateUnconscious;}
  // Unconscious person cannot command
  bool LSCanCommand() const {return _lifeState == LifeStateAlive || _lifeState == LifeStateAsleep;}
  bool LSCanTrackTargets() const {return _lifeState == LifeStateAlive || _lifeState == LifeStateAsleep || _lifeState == LifeStateUnconscious;}
  bool LSCanSupport() const {return _lifeState == LifeStateAlive || _lifeState == LifeStateAsleep || _lifeState == LifeStateUnconscious;}
  bool LSCanGetInGetOut() const {return _lifeState == LifeStateAlive || _lifeState == LifeStateAsleep || _lifeState == LifeStateUnconscious;}
  // Unconscious person cannot use actions menu
  bool LSCanProcessActions() const {return (_lifeState == LifeStateAlive || _lifeState == LifeStateAsleep) && !_captive;}
  // Unconscious person can be looted
  bool LSCanBeLooted() const {return _lifeState == LifeStateDead || _lifeState == LifeStateUnconscious || _captive ;}
  //@}

  int GetAIDisabled() const {return _disabledAI;}
  void SetAIDisabled(int state) {_disabledAI = state;}
  bool IsAIDisabled(DisabledAI state) const {return (_disabledAI&state)!=0;}

  Speaker *GetSpeaker() const {return _speaker;}
  void SetSpeaker(RString speaker, float pitch);
  /// Set the Speaker using _info for AI but PlayerIdentity for Players
  void SetSpeaker();

  //! no difficulty settings correction
  float GetRawAbility() const; // returns from 1 (maximal) to 0.2 (unable)
  void SetRawAbility(float ability);

  //! include correction based on difficulty settings
  float GetAbility(AbilityKind kind) const; // returns from 1 (maximal) to 0.2 (unable)
  //! include correction based on difficulty settings
  float GetInvAbility(AbilityKind kind) const; // returns from 5 (unable) to 1 (maximal)

  /// process target list, creating an aggregate information (general enemy direction)
  void CheckEnemies();

  /// we want to avoid some decision being synchronized - each brain has its own seed
  virtual int RandomSeed() const = 0;

  //! random decision model
  bool RandomDecision(float period, float ratio) const;

  /// check current morale level
  virtual float GetMorale() const = 0;

  State GetState() const {return _state;}
  //! Set state of unit
  /*!
  \param vehChanged if true, enable change of InCargo state
  */
  bool SetState(State state, bool vehChanged = false);
  void ClearCompletedReceived() {_completedReceived = false;}
  Mode GetMode() const {return _mode;}
  void SetMode(Mode mode) {_mode = mode;}
  bool IsPlanning() const;
  PlanningMode GetPlanningMode() const {return _planningMode;}
  int GetIter() const {return _iter;}
  const OperMap &GetOperMap() const;
  OperMap &GetOperMap();

  //@{ behaviour FSM diagnostics
  int NBehaviourDiags() const;
  RString BehaviourDiags(int i) const;

#if _ENABLE_CHEATS
  virtual void DrawDiags(){}
#endif
  //@}
  

  bool IsPlayer() const; // main (local) player
  bool IsAnyPlayer() const; // any player - local or remote
  bool IsRemotePlayer() const;

  bool HasAI() const;

  /// can the unit be given orders?
  bool IsUnit() const;
  bool IsCommander() const;
  bool IsDriver() const;
  bool IsGunner() const;

  UnitPosition GetUnitPosition() const;
  void SetUnitPositionCommanded(UnitPosition status);
  void SetUnitPositionScripted(UnitPosition status);
  void SetUnitPositionFSM(UnitPosition status);

  /// check if unit wants to fire
  /** used to select appropriate stance or hatch state in which firing is possible */
  bool IsFiringWanted() const;
  
  
  /// proper combat modes, used for behaviours (higher level)
  CombatMode GetCombatMode() const;
  /// low-level combat mode, used for basic behaviours (stance, hatch, reactions)
  CombatMode GetCombatModeLowLevel() const;
  
  CombatMode GetCombatModeMajor() const {return _combatModeMajor;}
  void SetCombatModeMajor(CombatMode mode);
  void OnCombatModeChanged(CombatMode oldMode, CombatMode newMode);
  /// set combat mode, call OnCombatModeChanged if result as seen be GetCombatMode() has changed
  void SetCombatModeMajorReactToChange(CombatMode mode);

  /// notify a new target will be known soon
  virtual void OnNewTarget(TargetNormal *target, Time knownSince);

  bool IsDanger() const;
  void SetDanger(DangerCause cause, Vector3Par position, const Target *causedBy, float until = -1.0);

  Semaphore GetSemaphore() const {return _semaphore;}
  void SetSemaphore(Semaphore status);
  bool IsHoldingFire() const;
  bool IsKeepingFormation() const;

  /// Path type we (or units before us) are currently moving on
  bool IsOnBridge() const {return IsOnRoad()>1;}
  /// Check path type we (or units before us) are currently moving on
  int IsOnRoad() const;

  Target *FindTargetAll(TargetType * id, bool includeBuildings=true) const; // find any (even unknown) target
  Target *FindTarget(TargetType * id, bool includeBuildings=true) const; // find known target 
  Target *AddTarget(
    EntityAI *object, float accuracy, float sideAccuracy, float delay,
    const Vector3 *pos=NULL, AIBrain *sensor=NULL, float sensorDelay=1e10
  );
  
  /// ghost target creation
  Target *CreateGhostTarget(
    const EntityAIType *type, Vector3Par pos,
    float typeAccuracy, float posAccuracy, float sensorDelay
  );
  /// intended for ghost targets, but can be used for any target
  void MoveTarget(Target *tgt, Vector3Par pos, float typeAccuracy, float posAccuracy);
  /// delete ghost target
  void DeleteGhostTarget(Target *tgt);

  TargetToFire GetTargetAssigned() const {return _targetAssigned;}
  void AssignTarget(TargetToFire target) {_targetAssigned = target;}

  Target *GetEnableFireTarget() const {return _targetEnableFire;}
  void EnableFireTarget(Target *target) {_targetEnableFire = target;}

  void SetGhostTarget(TargetGhost *tgt);
  TargetGhost *GetGhostTarget() const;
  /// get current target denoting the fire sector
  virtual Target *GetFireSectorTarget() const {return NULL;}

  float GetNearestEnemyDist2() const {return _nearestEnemyDist2;}
  void SetNearestEnemyDist2(float val) {_nearestEnemyDist2 = val;}

  int GetCaptive() const {return _captive;}
  void SetCaptive(int captive) {_captive=captive;}
#if _VBS3
  bool CanSpeak(){ return _allowSpeech;}
  void SetAllowSpeech(bool speech){_allowSpeech = speech;}
#endif
  void SetNoWatch();
  void SetWatchPosition(Vector3Val pos, bool glanceOnly=false);
  void SetWatchTarget(Target *tgt, bool glanceOnly=false);
  void SetWatchDirection( Vector3Val dir );
  void SetWatchAround();

  /// world space direction watched
  Vector3Val GetWatchHeadDirection() const {return _watchDirHead;}
  /// world space direction watched
  Vector3Val GetWatchDirection() const {return _watchDir;}
  /// watch mode determined what is the unit watching and how
  WatchMode GetWatchMode() const {return _watchMode;}

  ObjectVisualState const* GetFutureVisualState() const { return GetVehicle() ? &(GetVehicle()->FutureVisualState()) : NULL; }
  ObjectVisualState const* GetRenderVisualState() const { return GetVehicle() ? &(GetVehicle()->RenderVisualState()) : NULL; }
  
  Vector3Val Position(ObjectVisualState const* vs) const {return vs ? vs->Position() : VZero;}
  Vector3Val Direction(ObjectVisualState const* vs) const {return vs ? vs->Direction() : VForward;}

  Vector3 GetCurrentPosition() const {return Position(GetFutureVisualState());}

  Vector3 COMPosition(ObjectVisualState const* vs) const {return vs ? GetVehicle()->COMPosition(*vs) : VZero;}
  Vector3 VisiblePosition(ObjectVisualState const* vs) const {return vs ? GetVehicle()->VisiblePosition(*vs) : VZero;}
  Vector3 AimingPosition(ObjectVisualState const* vs) const {return vs ? GetVehicle()->AimingPosition(*vs) : VZero;}
  Vector3 AimingPosition(ObjectVisualState const* vs, const AmmoType *ammo, Vector3Par aimFromDir) const {return vs ? GetVehicle()->AimingPosition(*vs, ammo,aimFromDir) : VZero;}
  float VisibleSize(ObjectVisualState const* vs) const {return vs ? GetVehicle()->VisibleSize(*vs) : 0.0f;}

  void AddExp(float exp);
  void IncreaseExperience(const EntityAIType& type, TargetSide side); // called when you kill/destroy sth.

  /// find agent side from commanding structure
  virtual TargetSide GetSide() const = 0;
  /// check if given target is enemy to me
  virtual bool IsEnemy(TargetSide side) const = 0;
  virtual bool IsNeutral( TargetSide side) const = 0;
  virtual bool IsFriendly(TargetSide side) const = 0;

  virtual bool IsInCargo() const = 0;
  virtual void SetInCargo(bool set) = 0;

  virtual void SetSearchTime(Time time) = 0;

  void DoGetOut(Transport *veh, bool eject, bool parachute, Vector3Val diff = VZero);
  bool ProcessGetOut(bool eject, bool parachute, Vector3Val diff = VZero);

  Path& GetPath() {return _path;}
  void SetPathCosts(float costReplan, float costForceReplan);
  const Path& GetPath() const {return _path;}
  int GetPathId() const {return _pathId;}
  
  void SetHouse(Object *house, int pos) {_house = house; _housePos = pos;}

  Vector3Val GetPlanningFrom() const {return _from;}

  Time GetDelay() const {return _delay;}

  bool IsSimplePath(Vector3Val from, Vector3Val pos, float &simpleCost, float &simpleHeurCost) const;
  /// check if IsSimplePath has all data ready, preload them if necessary
  bool IsSimplePathReady(Vector3Val from, Vector3Val pos) const;

  Vector3 NearestIntersection(Vector3Val from, Vector3Val pos);

  void RemoveAction(const PathAction *action);

  virtual const SideInfo *GetSideInfo() const = 0;
  
#if _ENABLE_INDEPENDENT_AGENTS
  virtual AIAgent *GetAgent() {return NULL;}
#endif

#if _ENABLE_AI
  void CreateStrategicPath(ThinkImportance prec);
  bool VerifyPath(bool waiting=false, float timeAhead=FLT_MAX) const;
  void CopyPath(const IAIPathPlanner &planner); 

  const IAIPathPlanner &GetPlanner() const {return *_planner;}
  float GetTimeCompleted() {return _completedTime;}
  void SetTimeCompleted(float time) {_completedTime = time;}
#endif

  bool IsFireEnabled(Target *tgt) const; // check if fire enabled at given target

  void Disclose(DangerCause cause, Vector3Par position, EntityAI *causedBy, bool discloseGroup = true); // unit was disclosed - do something

  void Load(const IdentityInfo &info); // init unit info

  bool CanHear(ChatChannel channel, AIBrain *sender) const;

  float GetTimeToLive() const;

  bool CheckEmpty(Vector3Par pos);
  /// before checking for nearest empty you may wish to check if the data are ready
  bool NearestEmptyReady(Vector3Par pos) const;

  static bool FindEmptyInRect(
    Vector3 &emptyPos, bool soldier, EntityAI *veh,
    const FrameBase &pos, const Vector3 *minmax, Vector3Par nearTo,
    int mask = MASK_AVOID_OBJECTS | MASK_PREFER_ROADS | MASK_USE_BUFFER
  );

  static bool FindNearestEmpty(Vector3 &pos, bool soldier, EntityAI *veh, int mask = MASK_AVOID_OBJECTS | MASK_PREFER_ROADS | MASK_USE_BUFFER);
  /// find an empty place nearest to the given location
  bool FindNearestEmpty(Vector3 &pos, int mask = MASK_AVOID_OBJECTS | MASK_PREFER_ROADS | MASK_USE_BUFFER);
  /// find an empty place in given rectangle nearest to the given location
  bool FindEmptyInRect(
    Vector3 &emptyPos, const FrameBase &pos, const Vector3 *minmax, Vector3Par nearTo,
    int mask = MASK_AVOID_OBJECTS | MASK_PREFER_ROADS | MASK_USE_BUFFER
  );

  /// search for the free area nearest to the given position
  static bool FindEmptyArea(Vector3 &center, float radius, float maxDistance, const EntityAIType *vehType);

  void SetWantedPosition(const PlanPosition &pos, float precision, PlanningMode mode, bool forceReplan = false);
  const PlanPosition &GetWantedDestination() const {return _wantedPosition;}
  void ForceReplan(bool clear = true);

  void SetPlanParametersSet(bool val=true){_planParametersSet=val;}
  bool GetPlanParametersSet() const {return _planParametersSet;}
  
  PlanningMode GetPlanningModeWanted() const {return _planningModeWanted;}
  void SetPlanningMode(PlanningMode mode, bool forceReplan);
  float GetForceSpeed() const {return _forceSpeed;}
  void SetForceSpeed(float speed) {_forceSpeed=speed;}

  //@{ interface for formation FSM
  /// check position which higher level requested from the formation FSM
  const PlanPosition &GetWantedPosition() const {return _wantedPositionWanted;}

  float GetWantedPrecision() const {return _wantedPrecision;}
  /// check position and planning modes which higher level requested from the formation FSM
  void GetExpectedDestination(PlanPosition &destination, float &precision, PlanningMode &planningMode, bool &forceReplan);
  /// request formation FSM to move at given position
  void SetWantedDestination(const PlanPosition &destination, PlanningMode planningMode, bool forceReplan);

  bool CheckReplanNeeded(float baseCoef, PlanningMode planningMode, bool forceReplan) const;
  //@}

  // serialization
  virtual RString GetAgentType() const = 0;
  virtual LSError Serialize(ParamArchive &ar);
  static AIBrain *CreateObject(ParamArchive &ar);
  static AIBrain *LoadRef(ParamArchive &ar);
  LSError SaveRef(ParamArchive &ar) const;

#if _ENABLE_CONVERSATION
  /// get primary KB center
  const KBCenter *GetKBCenter() const;
  /// get KB parent (secondary)
  const KBCenter *GetKBParent() const;

  void AddKBTopic(RString name, RString filename, RString taskType, GameValuePar eventHandler);
  void RemoveKBTopic(RString name);
  bool HasKBTopic(RString name);
  KBMessageInfo *CreateKBMessage(RString topic, GameValuePar sentence);
  void SetKBMessageArgument(KBMessageInfo *message, GameValuePar argument);
  void KBTell(AIBrain *to, KBMessageInfo *message, bool forceRadio = false, ChatChannel chan=CCNone);
  void KBReact(AIBrain *sender, const KBMessageInfo *question);

  bool IsSpeaking() const {return _speaking > 0;}
  bool IsListening() const {return _listening > 0;}
  void ChangeSpeaking(int diff) {_speaking += diff;}
  void ChangeListening(int diff) {_listening += diff;}
#else
  bool IsSpeaking() const {return false;}
  bool IsListening() const {return false;}
  void ChangeSpeaking(int diff) {}
  void ChangeListening(int diff) {}
#endif

#if _ENABLE_DIRECT_MESSAGES
  bool IsGestureEnabled(ChatChannel channel) const;
  bool IsVoiceEnabled(ChatChannel channel) const;
#else
  bool IsGestureEnabled(ChatChannel channel) const {return true;} // by default perform both gestures and voice
  bool IsVoiceEnabled(ChatChannel channel) const {return true;}
#endif

  // simple expression variables for sentence variants
  float GetSERank() const;
  float GetSECaptive() const;
  float GetSESafe() const;
  float GetSECombat() const;
  float GetSEStealth() const;
  float GetSEMorale() const;

  virtual bool AssertValid() const;

  virtual CombatModeRange GetCombatModeBasedOnCommand() const {return CombatModeRange(CMSafe,CMStealth);}
  virtual CautionRange GetCautionBasedOnCommand() const {return CautionRange();}
  virtual bool GetCoveringBasedOnCommand() const {return true;}
  virtual OperCover GetCover() const {return OperCover();}
  virtual void GetHideFrom(Vector3 &hideL, Vector3 &hideR) const {hideL = hideR = VZero;}

  /// check if path finding is allowed to find a path into a cover
  bool IsCoveringEnabled() const {return _maxCostToCover>0;}

  float GetPreferredCostToCover() const {return _maxCostToCover*0.7f;}

  void OnStepTimedOut();
  void OnStepCompleted();

  // clear operative plan and start planning in not in progress
  void UpdateOperativePlan();

  virtual TargetList *AccessTargetList() = 0;
  virtual const TargetList *AccessTargetList() const = 0;

  virtual bool ForEachTarget(const TargetFunctor &func) const = 0;

  /// check what is the combat mode based on observed behaviour of given unit
  CombatMode GetCombatModeAutoDetected() const;

  /// compute focus movement needed to scan for the target
  Vector3 ScanTarget(Target * tgt);
  Vector3 ScanTargetOffset(const Target * tgt) const;

#if _ENABLE_CHEATS
  /// start FSM debugger for all FSMs currently running
  virtual void DebugFSM();
  /// start FSM debugger for all newly created FSMs
  void DebugFSM(int level);
#endif

  /// Can be identity load from delay processed EH
  bool CanLoadIdentityDelayed() const { return _loadIdentityDelayed; }
  /// Identity loaded before EH process
  void LoadIdentityDelayed() { _loadIdentityDelayed = false; }

protected:
  void ClearOperativePlan();
  void ClearStrategicPlan();
  void RefreshStrategicPlan();
#if _ENABLE_AI
  void OnStrategicPathFound();
  void OnStrategicPathNotFound(bool update);

  bool OperPath(ThinkImportance prec); // returns false if waiting for files
  /// process the path searching (call ProcessFindPath / CreateOperativePath bellow)
  bool CreateOperativePath();
  /// start the path searching (encapsulate BeginOperativePath)
  bool CreateOperativePath(Vector3Par from, const PlanPosition &pos);
  /// path searching initialization
  bool BeginOperativePath(Vector3Par from, const PlanPosition &pos);

  void FinishOperativePath(bool found);
  void OnOperativePathFound();
  void OnOperativePathNotFound();
#endif

  /// look at _watchTgt
  void WatchTarget();

  
  /// look as dictated by formation fire sectors
  virtual void WatchFormation();

  /// if no target is given, we may want to look around
  void GlanceRandomDirection();
  /// control head turning (glance behaviour)
  void GlanceAt(Target *tgt);

  // think subtasks
  void SetWatch();

};

#if _ENABLE_CONVERSATION
/// Used to store the context of conversation for example in HUD
struct ConversationContext
{
  /// speaking unit
  OLinkPermNO(AIBrain) _askingUnit;
  /// listening unit
  OLinkPermNO(AIBrain) _askedUnit;
  /// we need to store question for serialization
  Ref<const KBMessageInfo> _question;

  /// KB center defining the conversation topic
  Ref<const KBCenter> _center;
  /// topic reacting to
  RString _topic;

  /// the description of UI based on the event handler result
  GameValue _handlerResult;
  /// the description of the UI based on the class UI in the knowledge base file
  Ref<const KBUINodeInfo> _current;

  ConversationContext(AIBrain *askingUnit, AIBrain *askedUnit, const KBMessageInfo *question);
  ~ConversationContext();

  GameValue ExecuteHandler(GameValuePar handler, RString topic, const KBMessage *message);
};
#endif

#if _ENABLE_IDENTITIES

class FSMScriptedType;

typedef RString AITaskTypeName;

class AITaskType : public RemoveLLinks, public CountInstances<AITaskType>
{
protected:
  /// name of ParamFile definition
  AITaskTypeName _name;

  /// ParamFile definition
  ConstParamClassDeepPtr _parClass;

  /// used for item identification
  RString _typeName;
  /// type of FSM
  RefR<const FSMScriptedType, const RefCount> _fsmType;
  /// source code of condition
  RString _condition;
#if USE_PRECOMPILATION
  /// compiled condition
  CompiledExpression _conditionCompiled;
#endif
  /// expression for task description calculation
  RString _descriptionExp;
  /// expression for short task description calculation
  RString _descriptionShortExp;
  /// expression for task description used in in-game UI
  RString _descriptionHUDExp;
  /// expression for task destination calculation
  RString _destinationExp;
  // list of required resources
  AutoArray<AITaskResource> _resources;

  mutable int _instancesCreated;
  mutable int _instancesDeleted;

public:
  AITaskType(const AITaskTypeName &name);

  const AITaskTypeName &GetName() const {return _name;}
  RString GetTypeName() const {return _typeName;}
  const FSMScriptedType *GetFSMType() const;
  const AutoArray<AITaskResource> &GetResources() const {return _resources;}
  RString GetCondition() const {return _condition;}
#if USE_PRECOMPILATION
  const CompiledExpression &GetConditionCompiled() const {return _conditionCompiled;}
#endif
  RString GetDescriptionExp() const {return _descriptionExp;}
  RString GetDescriptionShortExp() const {return _descriptionShortExp;}
  RString GetDescriptionHUDExp() const {return _descriptionHUDExp;}
  RString GetDestinationExp() const {return _destinationExp;}
  RString GetSentence(RString name) const;

  int GetInstancesCreated() const {return _instancesCreated;}
  int GetInstancesDeleted() const {return _instancesDeleted;}
  void OnInstanceCreated() const {_instancesCreated++;}
  void OnInstanceDeleted() const {_instancesDeleted++;}

  LSError Serialize(ParamArchive &ar);
  static AITaskType *CreateObject(ParamArchive &ar);
};

typedef BankArray<AITaskType, DefLLinkBankTraits<AITaskType> > AITaskTypeBank;

extern AITaskTypeBank AITaskTypes;

/// Implementation of task using the FSM
class AITask : public Task, public CountInstances<AITask>
{
  typedef Task base;

protected:
#if _ENABLE_INDEPENDENT_AGENTS
  /// Who is performing the task
  AITeamMemberLink _owner;
  /// Who send the task
  AITeamMemberLink _sender;
#else
  /// Who is performing the task
  OLinkPerm<AIBrain> _owner;
#endif
  /// Type of the task
  Ref<const AITaskType> _type;
  /// State of the task FSM
  SRef<FSM> _fsm;
  /// Task priority
  float _priority;
  /// Task is running (is not suspended due to resources)
  bool _running;

public:
  AITask(ForSerializationOnly) {_priority = 0; _running = false;} // used for serialization
#if _ENABLE_INDEPENDENT_AGENTS
  AITask(
    AITeamMember *owner, AITeamMember *sender, Task *parent, const AITaskType *type,
    FSM *fsm, float priority);
#else
  AITask(
    AIBrain *owner, Task *parent, const AITaskType *type,
    FSM *fsm, float priority);
#endif

  const AITaskType *GetType() const {return _type;}
  FSM *GetFSM() {return _fsm;}
#if _ENABLE_INDEPENDENT_AGENTS
  AITeamMember *GetOwner() const {return _owner;}
  AITeamMember *GetSender() const {return _sender;}
#else
  AIBrain *GetOwner() const {return _owner;}
#endif
  virtual float GetPriority() const {return _priority;}
  virtual bool GetDescription(RString &result) const;
  virtual bool GetDescriptionShort(RString &result) const;
  virtual bool GetDescriptionHUD(RString &result) const;
  virtual bool GetDestination(Vector3 &result) const;
  RString GetSentence(RString name) const
  {
    return _type ? _type->GetSentence(name) : RString();
  }

  bool IsRunning() const {return _running;}
  void Run(bool run) {_running = run;}

  /// for direct serialization of AITask
  static AITask *CreateObject(ParamArchive &ar);
  LSError Serialize(ParamArchive &ar);

  static AITask *LoadRef(ParamArchive &ar);
  virtual LSError SaveRef(ParamArchive &ar);
  /// for direct serialization of Ref<AITask>
  static LSError LoadRefRef(ParamArchive &ar, Ref<AITask> &link)
  {
    link = AITask::LoadRef(ar);
    return (LSError)0;
  }
  /// for direct serialization of Ref<AITask>
  static LSError SaveRefRef(ParamArchive &ar, Ref<AITask> &link)
  {
    return link->SaveRef(ar);
  }

  DECL_SERIALIZE_TYPE_INFO(AITask, Task);
};

#endif

#if _ENABLE_CONVERSATION
struct TargetKBList: public RefArray<TargetKnowledge>
{
  typedef RefArray<TargetKnowledge> base;
  bool Find(const Target *t) const;
};
#endif

#if _ENABLE_INDEPENDENT_AGENTS

struct AITeamMemberContext
{
  AITeamMember *_teamMember;
  AITeamMemberContext(AITeamMember *teamMember) {_teamMember = teamMember;}
};

/// Agent - implementation of leaf node in AI hierarchy - brain of the Person
class AIAgent : public AIBrain, public AITeamMember, public SideInfo, public CountInstances<AIAgent>
{
  typedef AIBrain base;

protected:
  /// Target list
  TargetListInAgent _targetList;
#if _ENABLE_CONVERSATION
  /// List of targets no more stored in target list, but usable in conversation
  TargetKBList _targetKB;
#endif

  //@{ Expansive think timing
  Time _expensiveThinkTime;
  int _expensiveThinkFrac; // frac in ms
  void ExpensiveThinkDone(); // calculate time of next expensive think
  //@}

public:
  AIAgent(Person *vehicle);

  //@{ Reference counting redirection
  virtual int IAddRef() {return AddRef();}
  virtual int IRelease() {return Release();}
  //@}

  //@{ Soft link support redirection
  virtual TrackSoftLinks<IdTraits> *ITrack() {return Track();}
  virtual RemoveSoftLinks<IdTraits> *GetRemoveLinks() {return this;}
  virtual AITeamMember::LinkId GetLinkId() const {return AIBrain::GetLinkId();}
  //@}

  // AITeamMember implementation
  virtual void SimulateAI();
  virtual void ApplyContext(GameState &state);
  virtual AIAgent *GetAgent() {return this;}
  virtual GameVarSpace *GetVars() override {return _person ? _person->GetVars() : NULL;}
  virtual const GameVarSpace *GetVars() const override {return _person ? _person->GetVars() : NULL;}

  // AIBrain implementation
  virtual bool IsInCargo() const;
  virtual void SetInCargo(bool set);
  virtual void SetSearchTime(Time time);
  virtual TargetSide GetSide() const;

  virtual bool IsEnemy(TargetSide side) const;
  virtual bool IsNeutral( TargetSide side) const;
  virtual bool IsFriendly(TargetSide side) const;

  virtual int RandomSeed() const;

  virtual float GetMorale() const;

  //@{ implementation of SideInfo
  virtual bool SideIsFriendly(TargetSide side) const {return IsFriendly(side);}
  virtual bool SideIsEnemy(TargetSide side) const {return IsEnemy(side);}
  virtual bool SideIsNeutral(TargetSide side) const {return IsNeutral(side);}
  //@}

  virtual const SideInfo *GetSideInfo() const {return this;}

  virtual RString GetAgentType() const {return "Agent";}

  // NetworkObject implementation
  virtual RString GetDebugName() const;
  virtual AITeamMember *GetTeamMember() {return this;}
  virtual CombatModeRange GetCombatModeBasedOnCommand() const;
  virtual CautionRange GetCautionBasedOnCommand() const;
  virtual bool GetCoveringBasedOnCommand() const;

  virtual LSError Serialize(ParamArchive &ar);
  virtual LSError SaveTeamMemberRef(ParamArchive &ar);
  static LSError SaveLinkRef(ParamArchive &ar, OLinkPermNO(AIAgent) &link) {return AIBrain::SaveLinkRef(ar, link);}
  static LSError LoadLinkRef(ParamArchive &ar, OLinkPermNO(AIAgent) &link) {return AIBrain::LoadLinkRef(ar, link);}

  const TargetList &GetTargetList() const {return _targetList;}
  virtual TargetList *AccessTargetList() {return &_targetList;}
  virtual const TargetList *AccessTargetList() const {return &_targetList;}

  virtual bool ForEachTarget(const TargetFunctor &func) const;
  /// add the target to the long-term database
  void OnTargetDeleted(Target *t);

#if _ENABLE_CHEATS
  virtual void DebugFSM();
#endif

protected:
  // implementation
  bool CreateTargetList(bool initialize = false, bool report = true);
  float GetSubjectiveCost(TargetNormal *tar) const;
};

/// Team - implementation of node in AI hierarchy containing other nodes
class AITeam : public SerializedObject, public AITeamMember, public CountInstances<AITeam>
{
protected:
  /// List of contained members
  FindArrayKey<AITeamMemberLink> _teamMembers;
  /// Leader (speaker) of the team
  AITeamMemberLink _teamLeader;
  
  RString _typeName;
  RString _name;
  RString _formation;

  int _id; // unique identifier used for serialization (of references)

  /// Unique identification over network game
  NetworkId _networkId;
  /// True when the object is local on this network client
  bool _local;

  /// local variables
  GameVarSpace _vars;

public:
  AITeam() : _vars(true) {_id = -1; _local = true;} // used for serialization
  AITeam(RString typeName, RString name) : _vars(true) {_typeName = typeName; _name = name; _id = -1; _local = true;}

  RString GetTypeName() const {return _typeName;}
  RString GetName() const {return _name;}

  AITeamMember *GetLeader() {return _teamLeader;}
  void SetLeader(AITeamMember *leader) {_teamLeader = leader;}

  RString GetFormation() const {return _formation;}
  void SetFormation(RString formation) {_formation = formation;} 

  void SetId(int id) {_id = id;}

  virtual GameVarSpace *GetVars() override {return &_vars;}
  virtual const GameVarSpace *GetVars() const override {return &_vars;}

  //@{ Reference counting redirection
  virtual int IAddRef() {return AddRef();}
  virtual int IRelease() {return Release();}
  //@}

  //@{ Soft link support redirection
  virtual TrackSoftLinks<IdTraits> *ITrack() {return Track();}
  virtual RemoveSoftLinks<IdTraits> *GetRemoveLinks() {return this;}
  virtual AITeamMember::LinkId GetLinkId() const {return SerializedObject::GetLinkId();}
  //@}

  // AITeamMember implementation
  virtual AITeam *GetTeam() {return this;}
  virtual void SimulateAI();
  virtual void ApplyContext(GameState &state);
  virtual LSError Serialize(ParamArchive &ar);
  virtual LSError SaveTeamMemberRef(ParamArchive &ar);

  static AITeamMember *LoadRef(ParamArchive &ar);

  virtual void SetNetworkId(NetworkId &id) {_networkId = id;}
  virtual NetworkId GetNetworkId() const {return _networkId;}

  virtual RString GetDebugName() const {return _name;}
  virtual AITeamMember *GetTeamMember() {return this;}

  /// Add a new member
  int AddTeamMember(AITeamMember *teamMember) {return _teamMembers.AddUnique(teamMember);}
  /// Remove a member
  bool RemoveTeamMember(AITeamMember *teamMember) {return _teamMembers.DeleteKey(teamMember);}

  /// Return number of members
  int NTeamMembers() const {return _teamMembers.Size();}
  /// Return member with given index
  const AITeamMember *GetTeamMember(int i) const {return _teamMembers[i];}
  /// Return member with given index
  AITeamMember *GetTeamMember(int i) {return _teamMembers[i];}
};

#endif

DEFINE_ENUM_BEG(EEndMode)
  EMContinue, EMKilled, EMLoser, EMCheat,
  EMEnd1, EMEnd2, EMEnd3, EMEnd4, EMEnd5, EMEnd6
DEFINE_ENUM_END(EEndMode)

void InitNoCenters(IMission &t, VehicleInitMessages &inits, GameMode mode);

class AIStatsMission : public SerializeClass
{
protected:

public:
  Time _start;
  time_t _time;

public:
  AIStatsMission();
  void Init();
  
  void Clear();
  void OnMPMissionStart();

  LSError Serialize(ParamArchive &ar);
};

struct AIUnitHeader : public SerializeClass
{
  int index;
  float experience;
  Rank rank;

  OLinkPerm<AIBrain> unit;
  bool used;

  LSError Serialize(ParamArchive &ar);
};
TypeIsGeneric(AIUnitHeader)


class AIStats : public SerializeClass
{
public:
  AIStatsMission _mission;
  AIUnitHeader _playerInfo;

public:
  AIStats();
  ~AIStats();

  void Init() {_mission.Init();}
  
  void ClearAll() {_mission.Clear();}
  void ClearMission() {_mission.Clear();}

  void OnMPMissionStart();
  void OnMPMissionEnd();

  void OnVehicleDestroyed(EntityAI *killed, EntityAI *killer);

  LSError Serialize(ParamArchive &ar);
};

extern AIStats GStats;
inline AIUnitHeader &PlayerInfo() {return GStats._playerInfo;}

extern SRef<IMission> CurrentTemplate;

extern RString CurrentMission;
extern RString CurrentWorld;
extern RString CurrentFile;

LSError AIGlobalSerialize(ParamArchive &ar);

#endif
