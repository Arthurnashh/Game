#ifdef _MSC_VER
#pragma once
#endif

#ifndef _ZOMBIE_HPP
#define _ZOMBIE_HPP

#include "soldierOld.hpp"
#include "baseWorld/enf_shapes.h"

struct AttackAction
{
  RStringB _actionName;
  Ref<EntityType> _ammoType;
  bool _canWalk;
  float _minHeight;
  float _maxHeight;
  float _minRange;
  float _maxRange;
  float _traceHeight;
  float _traceLength;
  float _traceRadius;
  AttackAction(): _canWalk(false), _minHeight(0), _maxHeight(0), _minRange(0), _maxRange(0), _traceHeight(0), _traceLength(0), _traceRadius(0)
  {
  }
};

/*!
\file
New LightAIType class
*/

class LightAIType: public ManType
{
  typedef ManType base;
  friend class LightAI;
  friend class Zombie;
  friend class Animal;

protected:
  Ref<EntityType> _meleeType;

  AutoArray<SRef<AttackAction> > _attackActions;

public:
  AutoArray<RStringB> _attackSounds;
  AutoArray<RStringB> _spottedSounds;
  AutoArray<RStringB> _chaseSounds;
  AutoArray<RStringB> _idleSounds;

  float _wakeDistance;
  float _minIdleStayTime;
  float _maxIdleStayTime;
  float _minRoamRange;
  float _maxRoamRange;
  float _maxTimeNoLOS;

  LightAIType( ParamEntryPar param );
  const AttackAction& GetAttackAction(int n) const { return *_attackActions[n]; }
  int GetNumAttackActions() const { return _attackActions.Size(); }
  void LoadSounds(RStringB soundsName, AutoArray<RStringB>& outArray, ParamEntryVal sounds);
};

class ZombieType: public LightAIType
{
  typedef LightAIType base;
public:
  ZombieType( ParamEntryPar param ): base(param) {}
  virtual Object* CreateObject(bool fullCreate) const override;
};

class AnimalType: public LightAIType
{
  typedef LightAIType base;
public:
  AnimalType( ParamEntryPar param ): base(param) {}
  virtual Object* CreateObject(bool fullCreate) const override;

};


struct TrailWaypoint
{
  Vector3 _pos;
  Time _time;
  bool _safe; //2 subsequent created by target so considered as safe
  TrailWaypoint() {}
  TrailWaypoint(Vector3Par p, Time t, bool safe): _pos(p), _time(t), _safe(safe) {}

};

TypeIsSimpleZeroed(TrailWaypoint);

class ChaseTarget: public RefCount
{
protected:
  Time _lastUpdate;
public:
  Ref<Man> _target;
  AutoArray<TrailWaypoint> _trail;
  bool _pathBlocked;

  ChaseTarget() : _pathBlocked(false) {}

  void AddWaypoint(Vector3Par pos);
  float Age() const;
  void Clear()
  {
    _trail.Clear();
    _pathBlocked = false;
  }
  ChaseTarget* CreateReversed() const;
  bool ReachedGoal(Vector3Par pos) const;

};

typedef FSMTyped<LightAI> LightAIFSM;
template <class T1, class T2>
struct KeyPair
{
  T1 first;
  T2 second;
  bool operator==(const KeyPair<T1, T2>& other) const { return (first == other.first); }
  bool operator<(const KeyPair<T1, T2>& other) const { return (first < other.first); }
  bool operator>(const KeyPair<T1, T2>& other) const { return (first > other.first); }

  KeyPair(){}
  KeyPair(const T1 &t1, const T2 &t2):first(t1),second(t2){}
};

typedef KeyPair<const Man*,Time> TIgnoreTarget;
TypeIsSimpleZeroed(TIgnoreTarget);

class LightAIActionContext: public PlayActionContext
{
public:
  void Do(Man *ai) override;
  void Event(int userType, const RStringB& param) override;
  LightAIActionContext(Man *man, const MoveIdExt &move):PlayActionContext(man,move){}
};

class LightAI: public Man
{
private:
  typedef Man base;

protected:
  enum AIState
  {
    AISNone = 0,
    AISIdle,
    AISWandering,
    AISFeeding,
    AISChasing,
    AISMelee,
    AISRunaway,
  };

  AutoArray<Ref<ChaseTarget> > _targets;
  Ref<ChaseTarget> _actualTarget;
  Ref<ChaseTarget> _walkTarget;
  Ref<ChaseTarget> _fleeTarget;

  ChaseTarget _toSafeSpot;
  Vector3 _finalTarget;
  SortedArray<TIgnoreTarget> _ignoredTargets;
  float _nearestPlayer;
  RefArray<enf::Shape> _shapes;

  int _checkAttack;
  int _attackAction;
  float _tiredFactor;
  float _tiredFactorHysteresis;
  float _tiredFactorMax;
  float _tiredFactorNoSprint;
  Time _nextPathOptimization;
  Time _nextObstacleCheckTime;
  Time _lastAttackTime;

  // wandering
  Vector3 _wanderingSteeringForce;
  Vector3 _wanderTarget;
  float _wanderRadius;
  float _wanderDistance;
  float _wanderJitter;

  InitPtr<LightAIFSM> _fsm;

  void SimulateAttack(const ManVisualState& moveTrans);
  void AIPathPilot(SteerInfo& info, float deltaT);

  void Say(const AutoArray<RStringB>& sounds, bool pos3D);
  float CalculateError(NetworkMessageContextWithError &ctx);
  void DrawDiags();
  void Sound( bool inside, float deltaT ) override;
  void FindPathAroundObstacles(AutoArray<TrailWaypoint>& _waypoints, Entity* pTarget);
	bool InitPathToTarget(ChaseTarget* target, Vector3Par testOffset, float testRadius, float maxDistForPathfinder);

  // steering behaviors
  Vector3 Wander();
  Vector3 WallAvoidance(float maxVelocity);

  int FindAttackAction(float dist, float height) const;

  void IgnoreTarget(const Man* target, float timeout);
	void UnignoreTarget(const Man* target);

  bool IsIgnored(const Man* target);
  void UpdateIgnored();

  ChaseTarget* UpdateTargets();
  ChaseTarget* FindFeedingTarget();
  void SetActualTarget(ChaseTarget* targ);
  void RemoveTarget(const Man* target);

  virtual void IdleStateInit() {}
  virtual void IdleStateCheck() {}
  virtual void WanderingStateInit() {}
  virtual void WanderingStateCheck() {}
  virtual void FeedingStateInit() {}
  virtual void FeedingStateCheck() {}
  virtual void ChasingStateInit() {}
  virtual void ChasingStateCheck() {}
  virtual void MeleeStateInit() {}
  virtual void MeleeStateCheck() {}
  virtual void RunawayStateInit() {}
  virtual void RunawayStateCheck() {}

  static void IdleStateInit(LightAI* context) {context->IdleStateInit();}
  static void IdleStateCheck(LightAI* context) {context->IdleStateCheck();}
  static void WanderingStateInit(LightAI* context) { context->WanderingStateInit(); }
  static void WanderingStateCheck(LightAI* context) { context->WanderingStateCheck(); }
  static void FeedingStateInit(LightAI* context) { context->FeedingStateInit(); }
  static void FeedingStateCheck(LightAI* context) {context->FeedingStateCheck();}
  static void ChasingStateInit(LightAI* context) {context->ChasingStateInit();}
  static void ChasingStateCheck(LightAI* context) {context->ChasingStateCheck();}
  static void MeleeStateInit(LightAI* context) {context->MeleeStateInit();}
  static void MeleeStateCheck(LightAI* context) {context->MeleeStateCheck();}
  static void RunawayStateInit(LightAI* context) {context->RunawayStateInit();}
  static void RunawayStateCheck(LightAI* context) {context->RunawayStateCheck();}

public:

  LightAI(const EntityAIType *name, bool fullCreate=true);
  ~LightAI();

  const LightAIType *Type() const
  {
    return static_cast<const LightAIType *>(GetType());
  }

  void SetNearestPlayer(float dist)
  {
    _nearestPlayer = dist;
  }
  virtual void OnAnimDone(MoveId moveId);
  virtual void OnAnimEvent(MoveId moveId, int userType, const RStringB& param);

  void Simulate( float deltaT, SimulationImportance prec ) override;
  void SimulateAI(float deltaT, SimulationImportance prec) override;
  void AIPilot(float deltaT, SimulationImportance prec);
  void FakePilot( float deltaT, SimulationImportance prec );

  bool CanWalk() const
  {
    if(_attackAction && !Type()->GetAttackAction(_attackAction - 1)._canWalk)
      return false;
    return true;
  }

};

class Zombie: public LightAI
{
private:
  typedef LightAI base;

protected:

  Time _nextRoamtime;

  Time _lastTargetTime;
  Time _timeForChasingSound;
  
  RandomGenerator _feedRandom;
  RandomGenerator _sniffRandom;
  
  //common state checks for combat states
  bool CombatStateCheck();

public:
  Zombie(const EntityAIType *name, bool fullCreate=true);
  ~Zombie();

  const ZombieType *Type() const
  {
    return static_cast<const ZombieType *>(GetType());
  }

  virtual void IdleStateInit();
  virtual void IdleStateCheck();
  virtual void FeedingStateInit();
  virtual void FeedingStateCheck();
  virtual void ChasingStateInit();
  virtual void ChasingStateCheck();
  virtual void MeleeStateInit();
  virtual void MeleeStateCheck();
  virtual void RunawayStateInit();
  virtual void RunawayStateCheck();

  LSError Serialize(ParamArchive &ar);

  virtual void OnAnimDone(MoveId moveId) override;


  USE_CASTING(base)
};

class Animal: public LightAI
{
private:
  typedef LightAI base;

protected:

  Time _nextIdleTime;
  Time _nextWanderTime;
  Time _lastWanderingDirChangeTime;


  //common state checks
  bool FleeStateCheck();

  ChaseTarget* FindThreatTarget();

public:
  Animal(const EntityAIType *name, bool fullCreate=true);
  ~Animal();

  const AnimalType *Type() const
  {
    return static_cast<const AnimalType *>(GetType());
  }

  virtual void IdleStateInit();
  virtual void IdleStateCheck();
  virtual void WanderingStateInit();
  virtual void WanderingStateCheck();
  virtual void FeedingStateInit();
  virtual void FeedingStateCheck();
  virtual void RunawayStateInit();
  virtual void RunawayStateCheck();

  LSError Serialize(ParamArchive &ar);

  USE_CASTING(base)
};


bool IsInWater (const Vector3 & center);
float MaxTerrainGradient (const Vector3 & center, float radius);
bool IsTooSteep (const Vector3 & center, float radius, float maxGradient);
#endif //_ZOMBIE_HPP
