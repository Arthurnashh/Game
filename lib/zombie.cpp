#include "wpch.hpp"

/*!
\file
Implementation file for Zombie classes
*/

#include "transport.hpp"
#include "scene.hpp"
#include "world.hpp"
#include "landscape.hpp"
#include <El/Common/randomGen.hpp>
#include "tracks.hpp"
#include <El/Common/perfLog.hpp>
#include <El/Common/perfProf.hpp>
#include "txtPreload.hpp"
#include "diagModes.hpp"
#include <Es/Strings/bString.hpp>
#include <Es/Types/scopeLock.hpp>

#include "move_actions.hpp"

#include "Network/network.hpp"

#include <El/QStream/qbStream.hpp>
#include <El/PreprocC/preprocC.hpp>
#include <El/Evaluator/express.hpp>
#include "paramArchiveExt.hpp"
#include <El/Enum/enumNames.hpp>
#include "ai/ai.hpp"

#include "Es/Common/langExt.hpp"

#include "house.hpp"
#include "zombie.hpp"
#include "Common/enf_mathlib2d.h"
#include "Common/enf_intersection.h"
#include <stdio.h>
#include <stdarg.h>

static void ZombieProbeLog(const char *format, ...)
{
  FILE *file = fopen("!ConfigServer\\Logs\\zombie_collision_cpp.log", "a");
  if (!file)
    return;
  va_list args;
  va_start(args, format);
  vfprintf(file, format, args);
  va_end(args);
  fputc('\n', file);
  fclose(file);
}

#if _PROFILE
# pragma optimize( "", off )
#endif

//DEFINE_CASTING(LightAI);
DEFINE_CASTING(Zombie);
DEFINE_CASTING(Animal);

const float ZombieRayTestRadius = 0.31f;
const Vector3 ZombieRayTestOffset(0, ZombieRayTestRadius + 0.4f, 0);
const float MELEE_DISTANCE_IN = 5.5;
const float MELEE_DISTANCE_OFF = 7;

bool IsNotAlive(const Object* obj)
{
#ifndef _BINARIZE
  const Person* person = dynamic_cast<const Person*>(obj);
  if(person)
  {
    AIAgent* agent = static_cast<AIAgent*>(person->Brain());
    if(agent->GetLifeState() > LifeStateAlive)
      return true;
  }
#endif
  return false;
}

bool IsUnconscious(const Object* obj)
{
#ifndef _BINARIZE
  const Person* person = dynamic_cast<const Person*>(obj);
  if(person)
  {
    AIAgent* agent = static_cast<AIAgent*>(person->Brain());
    if(agent->GetLifeState() == LifeStateUnconscious)
      return true;
  }
#endif
  return false;
}

bool InBuildingOrColliding(const Man* obj)
{
  if(!obj) return false;

  Object* walkOnObj = obj->WalkingOnObject();

  if(walkOnObj && dynamic_cast<Building*>(walkOnObj))
    return true;
  return obj->IsInCollision();
}


class FilterIgnoreOneAndDead : public IntersectionFilter
{
protected:
  const Object * _obj;
public:
  FilterIgnoreOneAndDead(const Object * obj) :_obj(obj) {}
  bool operator()(const Object * obj) const
  {
    if(IsNotAlive(obj))
      return false;
    return Landscape::FilterIgnoreOneHelper(_obj,obj);
  }
};

class FilterIgnoreTwoAndDead : public IntersectionFilter
{
protected:
  const Object * _obj1;
  const Object * _obj2;
public:
  FilterIgnoreTwoAndDead(const Object * obj1, const Object * obj2) :_obj1(obj1),_obj2(obj2) {}
  bool operator()(const Object * obj) const
  {
    if(IsNotAlive(obj))
      return false;
    return Landscape::FilterIgnoreOneHelper(_obj1,obj) && Landscape::FilterIgnoreOneHelper(_obj2,obj);
  }

};

Object* ZombieType::CreateObject(bool fullCreate) const
{ 
  return new Zombie(this, fullCreate);
}

Object* AnimalType::CreateObject(bool fullCreate) const
{ 
  return new Animal(this, fullCreate);
}

void LightAIType::LoadSounds(RStringB soundsName, AutoArray<RStringB>& outArray, ParamEntryVal sounds)
{
  if(soundsName.IsEmpty())
    return;
  if(!sounds.FindEntry(soundsName))
    return;
  ParamEntryVal soundsClass = sounds >> soundsName;
  if(soundsClass.IsClass())
  {
    ParamEntryVal sounds = soundsClass >> "sounds";
    if(sounds.IsArray())
    {
      outArray.Reserve(sounds.GetSize());
      for(int n = 0; n < sounds.GetSize();  n++)
      {
        outArray.Add(sounds[n]);
      }
    }
  }

}

LightAIType::LightAIType( ParamEntryPar param ): ManType(param),
  _wakeDistance(500)
{
  RStringB meleeAmmo = param.ReadValue("meleeAmmo", RStringB("MeleeFist"));
  _meleeType = VehicleTypes.New(meleeAmmo);

  _wakeDistance = param.ReadValue("wakeDistance", 500);
  _minIdleStayTime = param.ReadValue("minIdleStayTime", 5);
  _maxIdleStayTime = param.ReadValue("maxIdleStayTime", 15);
  _minRoamRange = param.ReadValue("minRoamRange", 4);
  _maxRoamRange = param.ReadValue("maxRoamRange", 20);
  _maxTimeNoLOS = param.ReadValue("maxTimeNoLOS", 30);

  if(param.CheckIfEntryExists("AttackActions"))
  {
    ParamEntryVal attacksPar = param >> "AttackActions";
    if(attacksPar.IsClass())
    {
      for (ParamClass::Iterator<> i(attacksPar.GetClassInterface()); i; ++i)
      {
        if((*i).IsClass())
        {
          AttackAction* action = new AttackAction;
          action->_actionName = (*i) >> "action";
          action->_minRange = (*i) >> "minRange";
          action->_maxRange = (*i) >> "maxRange";
          action->_minHeight = (*i) >> "minHeight";
          action->_maxHeight = (*i) >> "maxHeight";
          action->_canWalk = (*i) >> "canWalk";
          RStringB ammoType = (*i) >> "ammoType";
          if(!ammoType.IsEmpty())
            action->_ammoType = VehicleTypes.New(ammoType);
          //defaults until config will be binarized
          action->_traceLength = (*i).ReadValue("traceLength", action->_actionName == RStringB("AttackLong") ? 2.1f : 1.4f);
          action->_traceHeight = (*i).ReadValue("traceHeight", action->_actionName == RStringB("AttackShortLow") ? 0.55f : 1.2f);
          action->_traceRadius = (*i).ReadValue("traceRadius", 0.65f);
          _attackActions.Add(action);
        }
      }
      _attackActions.Compact();
    }
  }

  ParamEntryVal actionSounds = Pars >> "CfgActionSounds";
  if(actionSounds.IsClass())
  {
    LoadSounds(param.ReadValue("attackSounds", RStringB("zombie_attack")), _attackSounds, actionSounds);
    LoadSounds(param.ReadValue("spottedSounds", RStringB("zombie_spotted")), _spottedSounds, actionSounds);
    LoadSounds(param.ReadValue("chaseSounds", RStringB("zombie_chase")), _chaseSounds, actionSounds);
    LoadSounds(param.ReadValue("idleSounds", RStringB("zombie_idle")), _idleSounds, actionSounds);
/*
    LoadSounds(param.ReadValue("attackSounds", RStringBEmpty), _attackSounds, actionSounds);
    LoadSounds(param.ReadValue("spottedSounds", RStringBEmpty), _spottedSounds, actionSounds);
    LoadSounds(param.ReadValue("chaseSounds", RStringBEmpty), _chaseSounds, actionSounds);
    LoadSounds(param.ReadValue("idleSounds", RStringBEmpty), _idleSounds, actionSounds);
*/
  }

}



void Dummy(LightAI* context)
{
}

LightAI::LightAI(const EntityAIType *name, bool fullCreate): Man(name, fullCreate), _nearestPlayer(0),
  _checkAttack(0), _attackAction(0),
  _tiredFactor(0),
  _tiredFactorHysteresis(0),
  _nextPathOptimization(TIME_MIN), _nextObstacleCheckTime(TIME_MIN),
  _lastAttackTime(TIME_MIN)
{
}

LightAI::~LightAI()
{
  SetActualTarget(NULL);
}

void LightAI::UnignoreTarget(const Man* target)
{
	int index = _ignoredTargets.Find(TIgnoreTarget(target, Time(0)));
	if(index >= 0 && index < _ignoredTargets.Size())
	{
		_ignoredTargets.DeleteAt(index);
	}
}

void LightAI::IgnoreTarget(const Man* target, float timeout)
{
  int index = _ignoredTargets.Find(TIgnoreTarget(target, Time(0)));
  if(index >= 0 && index < _ignoredTargets.Size())
  {
    _ignoredTargets[index].second = Glob.time + timeout;
  }
  else
    _ignoredTargets.AddUnique(TIgnoreTarget(target, Glob.time + timeout));
}

bool LightAI::IsIgnored(const Man* target)
{
  int index = _ignoredTargets.Find(TIgnoreTarget(target, Time(0)));
  if(index >= 0 && index < _ignoredTargets.Size())
  {
    return (Glob.time < _ignoredTargets[index].second);
  }
  return false;
}

void LightAI::UpdateIgnored()
{
  for(int n = 0; n < _ignoredTargets.Size();)
  {
    if(Glob.time >= _ignoredTargets[n].second)
    {
      _ignoredTargets.DeleteAt(n);
      continue;
    }
    n++;
  }
}

void LightAI::RemoveTarget(const Man* target)
{
  for(int n = 0; n < _targets.Size(); n++)
  {
    if(_targets[n]->_target != target)
      continue;
    if(_actualTarget->_target == target)
    {
      SetActualTarget(NULL);
    }
    _targets.Delete(n);
    break;
  }
}

#if _PROFILE && USER_Ondra
//debug testing
bool GBlind = true;
#else
bool GBlind = false;//false;
#endif


class CombatTarget
{
public:
  Ref<Man> _target;
  //fixed positions around target
  Zombie* _attackSlots[4];
  SortedArray<LightAI*> _attackers;
  CombatTarget()
  {
    _attackSlots[0] = NULL;
    _attackSlots[1] = NULL;
    _attackSlots[2] = NULL;
    _attackSlots[3] = NULL;
  }
};

typedef KeyPair<const Man*,CombatTarget*> TCombatTarget;
TypeIsSimpleZeroed(TCombatTarget);

SortedArray<TCombatTarget> GCombatTargets;



void LightAI::SetActualTarget(ChaseTarget* targ)
{
  //remove old
  if(_actualTarget && _actualTarget->_target)
  {
    int index = GCombatTargets.Find(TCombatTarget(_actualTarget->_target, NULL));

    if(index >= 0 && index < GCombatTargets.Size())
    {
      CombatTarget* ct = GCombatTargets.Get(index).second;
      ct->_attackers.Delete(this);
      if(!ct->_attackers.Size())
      {
        GCombatTargets.DeleteAt(index);
      }
    }
  }

  //set new
  if(targ && targ->_target)
  {
    CombatTarget* ct;
    int index = GCombatTargets.Find(TCombatTarget(targ->_target, NULL));
    if(index >= 0 && index < GCombatTargets.Size())
    {
      ct = GCombatTargets.Get(index).second;
    }
    else
    {
      ct = new CombatTarget;
      ct->_target = targ->_target;
      GCombatTargets.AddUnique(TCombatTarget(targ->_target, ct));
    }
    ct->_attackers.AddUnique(this);
  }
  _actualTarget = targ;
}

void LightAI::SimulateAttack(const ManVisualState& moveTrans)
{
  Vector3 pos = moveTrans.Position();
  Vector3 dir = moveTrans.Direction();
  dir = (dir - Vector3(0,0.1f,0)).Normalized();

  const AttackAction& action = Type()->GetAttackAction(_attackAction - 1);
  pos = pos + Vector3(0, action._traceHeight, 0);
  Vector3 to = pos + dir * action._traceLength;

  //  GScene->DrawDiagSphere(pos, 0.35f, HBlack);
  //GScene->DrawDiagSphere(to, 0.35f, ColorP(1,0,0));

  //TODO: remember last position and for bigger distances from current one interpolate these positions and do more traces!!!
  CollisionBuffer col;
  //cannot use CF_NEAREST_CONTACT as it's often weapon. And we cannot get just parent objects as we won't have correct component index!
  GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(),col,FilterIgnoreOneAndDead(this), pos, to, action._traceRadius, ObjIntersectFire, ObjIntersectNone, 0);
  if(col.Size())
  {
    int bestIndex = -1;
    float bestDist = FLT_MAX;

    for(int n = 0; n < col.Size(); n++)
    {
      //items in hands cannot be used anyway as component indices do not match then
      if(col[n].parentObject)
        continue;

      Man* directHit = dynamic_cast<Man*>(col[n].object.GetRef());
      if(!directHit)
        continue;

      LODShape *hitShape = directHit->GetShape();
      if (!hitShape)
        continue;

      int componentIndex = -1;
      int component = col[n].component;

      const Shape *fireGeom = hitShape->FireGeometryLevel();
      if (!fireGeom)
        continue;

      const ConvexComponents &fireCc = hitShape->GetFireComponents();
      componentIndex = fireCc.FindNameIndex(component);
      if(componentIndex == -1)
        continue;

      float localDist = col[n].pos.Distance2(pos);
      if(localDist < bestDist)
      {
        if(dynamic_cast<LightAI*>(directHit))
        {
          //ignore hits behind back
          Vector3 dirToHit = directHit->FutureVisualState().Position() - moveTrans.Position();
          if(dirToHit.DotProduct(dir) < 0)
            continue;
        }

        bestDist = localDist;
        bestIndex = n;
      }
    }

    if(bestIndex >= 0)
    {
      AmmoType *ammo = dynamic_cast<AmmoType *>(Type()->_meleeType.GetRef());
      if (ammo)
      {
        HitInfo hitInfo(ammo,col[bestIndex].pos + dir * 0.5f,VUp,VZero);
        Object* directHit = col[bestIndex].object;
        LODShape *hitShape = directHit->GetShape();
        /*const Shape *fireGeom =*/ hitShape->FireGeometryLevel();

        const ConvexComponents &fireCc = hitShape->GetFireComponents();
        int componentIndex = fireCc.FindNameIndex(col[bestIndex].component);

        // provide reasonable default values where we do not expect the script to be interested in them
        float energyFactor = 1.0f;
        float explosionFactor = 1.0f;
        Vector3 originDir = VZero;

        Vector3 directHitModelPos = directHit->RenderVisualState().GetInvTransform().FastTransform(col[bestIndex].pos + dir * 0.5f);
        // TODO: more direct interface to create ExplosionDamageEffectsMessage
        GLandscape->ExplosionDamage(this,originDir,directHit,directHitModelPos,hitInfo,directHit,componentIndex,energyFactor,explosionFactor);
      }
      _checkAttack =  0;
      return;
    }
  }
  //if animation marked it for end, end that, or mark it as used once
  _checkAttack = (_checkAttack == 3) ? 0 : 2;
}

void LightAI::Sound( bool inside, float deltaT )
{
  if (IsSimulationDisabled())
    return;
  //test optimization
  if(_nearestPlayer > 150)
    return;
  base::Sound(inside, deltaT);
}

void LightAI::Simulate( float deltaT, SimulationImportance prec )
{  
    static Time nextZombieSimEntryProbe = TIME_MIN;
  if (dynamic_cast<Zombie *>(this) && Glob.time >= nextZombieSimEntryProbe)
  {
    ZombieProbeLog("[ZombieSimEntry] %s disabled=%d local=%d server=%d prec=%d dt=%.3f", cc_cast(Type()->GetName()), IsSimulationDisabled() ? 1 : 0, IsLocal() ? 1 : 0, GetNetworkManager().IsServer() ? 1 : 0, (int)prec, deltaT);
    nextZombieSimEntryProbe = Glob.time + 1.0f;
  }
  if (IsSimulationDisabled())
    return;

  const bool isZombie = dynamic_cast<Zombie *>(this) != NULL;
  if (isZombie && _ignoreCollisionIfStuck)
  {
    // Man disables Geometry while it is recovering from a prolonged stuck
    // state. That recovery is unsafe for zombies: it makes HasGeometry()
    // false, so they can pass through the very wall they are stuck against.
    // Restore collision before building this simulation step.
    static Time nextZombieStuckDiagnostic = TIME_MIN;
    if (GetNetworkManager().IsServer() && Glob.time >= nextZombieStuckDiagnostic)
    {
      ZombieProbeLog("[ZombieCollisionDiag] cleared stuck-ignore on %s at %.2f %.2f %.2f",
        cc_cast(Type()->GetName()), FutureVisualState().Position().X(),
        FutureVisualState().Position().Y(), FutureVisualState().Position().Z());
      nextZombieStuckDiagnostic = Glob.time + 1.0f;
    }
    _ignoreCollisionIfStuck = false;
    _stuckStarted = TIME_MAX;
  }

  _toSafeSpot.AddWaypoint(FutureVisualState().Position());

  float deltaT0 = deltaT;
  // we may be forced to use different timer
  if (_useAudioTimeForMoves)
  {
    UITime now = GSoundScene->GetAudioTime();
    if (CHECK_DIAG(DEAnimation) && Brain()==GWorld->FocusOn())
    {
      DIAG_MESSAGE(2000,"%s: deltaT %.3f, audio %.3f",cc_cast(GetDebugName()),deltaT,now-_audioTimeProcessed);
    }
    deltaT = now-_audioTimeProcessed;
    _audioTimeProcessed = now;
    // note: KeyboardPilot (called from Man::SimulateAI) still uses deltaT.
    // When audio time is a lot different from game time, controls may feel very sluggish (e.g. with setAccTime 0.01)
  }

  if (!IsControlled() && CheckPredictionFrozen())
  {
    float timeOffset = AI_MAX_SIM_TIME;
    SetTimeOffset(timeOffset);

    // Man is frozen. Just do the animation, but don't move him. 
    float turn,moveX,moveZ;
    SimulateAnimations(turn,moveX,moveZ,deltaT,prec);
    base::Simulate(deltaT,prec);    
    BasicSimulationCore(deltaT, prec);
    SimulatePost(deltaT, prec);
    _impulseForce = VZero;
    _impulseTorque = VZero;

    _collVolume = INFINITY_VOLUME;
    return;
  }

  // Apply Impulses 
  float impulse2=_impulseForce.SquareSize();
  if (impulse2 > 0.0001f)
    DamageOnImpulse(this, sqrt(impulse2), VZero);

  if (impulse2>Square(GetMass() * 5))
  {        
    _lastMovementTime=Glob.time;        
    FreeFall(1.0f);        

    if( impulse2>Square(GetMass()*60) )
    {
      _impulseForce=_impulseForce.Normalized()*(GetMass()*60);
    }
  }

  if (_primaryMoveBackUp.id != _moves.GetPrimaryMove(FutureVisualState()._moves).id || _primaryTimeBackUp != _moves.GetPrimaryTime(FutureVisualState()._moves) ||
    _primaryFactorBackUp != _moves.GetPrimaryFactor(FutureVisualState()._moves)) //I do not have control of the move so let disable dammage system.
  {
#if _DEBUG
    RStringB str1 = GetMovesType()->GetMoveName(_primaryMoveBackUp.id);
    RStringB str2 = GetMovesType()->GetMoveName(_moves.GetPrimaryMove(FutureVisualState()._moves).id);      
#endif

    _collVolume = INFINITY_VOLUME;
  }

#if _ENABLE_ATTACHED_OBJECTS
  if (IsAttached()) _landContact = true; // if man is attached, assume land contact
#endif

  // some objects contacts may prevent us from falling?
  // when in contact with an object and speed is stabilized, do not assume free fall any more
  // x/z speed is not as important as the y one
  if (!_landContact && !(_objectContact && FutureVisualState()._speed.Modulate(Vector3(0.05f,1,0.05f)).SquareSize()<Square(0.3f)))
  {
    if (!_useAudioTimeForMoves) // audio time is used for precise cutscene moves - no free fall here
    {
      FreeFall(0.3f);
    }
  }

  _freeFall = (Glob.time < _freeFallUntil);    

  float timeOffset = AI_MAX_SIM_TIME;
  if (QIsManual() && !CHECK_DIAG(DEVisualStates))
  {
    SetSimulationPrecision(PLAYER_MAX_SIM_TIME,AI_MAX_SIM_TIME); /// player "AI" needs to be called in each frame
    timeOffset = 0;
  }
  else if (Glob.time < _staticCollisionUntil || _freeFall && !CHECK_DIAG(DEVisualStates))
  {
    SetSimulationPrecision(PLAYER_MAX_SIM_TIME,AI_MAX_SIM_TIME);
    timeOffset = AI_MAX_SIM_TIME_NEAR;
  }
  else
  {
    float step = prec<=SimulateVisibleNear ? AI_MAX_SIM_TIME_NEAR : AI_MAX_SIM_TIME;
    SetSimulationPrecision(step,AI_MAX_SIM_TIME);
    timeOffset = step;
  }

  SetTimeOffset(timeOffset);

  bool bChange = false;

  ManVisualState moveTrans = FutureVisualState(); // store old position
  Vector3 prevPos = moveTrans.Position();

  if(Brain()->GetLifeState() == LifeStateUnconscious)
  {
    //clear active attack (if any) and let him wait before next attack
    _checkAttack = false;
    _attackAction = 0;
    _lastAttackTime = Glob.time;
  }

  if(_checkAttack && _attackAction)
  {
    SimulateAttack(moveTrans);
  }

  // Freefall state must be handled separately
  if( _freeFall && !_useAudioTimeForMoves) // audio time is used for precise cutscene moves - no free fall here
  { 
    bChange = SimulateFreeFall(moveTrans, deltaT, prec);           
  }
  else
  {           
    _freeFallStartTime = TIME_MAX; //no freeFall now
    // not freefall
    //Assert(g_TestPos == VZero || g_TestPos == Position());      
    Vector3 repairMove;

    float fVoll = 0;      
    bool bCalculatedVoll = false;                                   
    bool bTakeAllCost = false;
    _ignoreLowPriorituContact = false;
    //Assert(g_fVoll == -1 || g_fVoll == fVoll);
    float fCollVol = 0;

    float turn;
    bChange = CreatePotentialState(moveTrans, turn, deltaT, prec);

    if (bChange || _swimming)
    {
      ApplyTurn(moveTrans,turn);
    }

    ApplyRemoteStateAdjustSpeed(deltaT,moveTrans);

    //   if (_remoteState)
    //   {
    //     LogF(
    //       "localX,remoteX,localSpeedX,remoteSpeedX,localSpeedSize,remoteSpeedSize ; %.2f,%.2f,%.3f,%.3f,%.3f,%.3f",
    //       FutureVisualState().Position().X(),_remoteState->pos.X()+_remoteState->speed.X()*(Glob.time-_remoteState->time),
    //       FutureVisualState().Speed().X(),_remoteState->speed.X(),
    //       FutureVisualState().Speed().Size(),_remoteState->speed.Size()
    //     );
    //   }

    _objectContact = false; // cannot be done before ApplyRemoteState, it needs to react on collisions

    if (bChange || _swimming)            
    {
      // check current up and turn
      SetleInPosition(moveTrans,turn,deltaT);
    }

    bool ignoreCollisions = _useAudioTimeForMoves;

#if _ENABLE_ATTACHED_OBJECTS
    if (IsAttached()) ignoreCollisions = true;
#endif

    if (ignoreCollisions) // ignore all collisions
    {
      // attached or invehicle units
      _collVolume = INFINITY_VOLUME;
    }
    else
    {
      if (isZombie)
        _staticCollision = true; // zombies need a physical fallback when their route misses an obstacle
      else if(_attackAction && Type()->GetAttackAction(_attackAction - 1)._canWalk)
        _staticCollision = true; //avoid jumping into wall
      else
      {
        // Change static collision status 
        if (_staticCollision)
        {
          if (!QIsManual() && !IsRemotePlayer() && _staticCollisionUntil < Glob.time)      
            _staticCollision = false;
        }
        else
        {
          if (QIsManual() || IsRemotePlayer())      
            _staticCollision = true;

          if (_objectContactNonStatic)
          {        
#define STATIC_COLL_TIME 5.0f   
            _staticCollision = true;
            _staticCollisionUntil = Glob.time + STATIC_COLL_TIME;
            SetSimulationPrecision(PLAYER_MAX_SIM_TIME);
            // SetTimeOffset(PLAYER_MAX_SIM_TIME);
          }
        }
      }

      _objectContactNonStatic = false;

      // LightAI::Simulate is a separate copy of Man::Simulate. The human
      // simulation subdivides long moves and validates the full animated body
      // at each intermediate position; without that step a fast AI update can
      // tunnel through thin Geometry LODs even when both endpoints are clear.
      if (isZombie && prec <= SimulateVisibleFar && !_useAudioTimeForMoves)
      {
        static const float zombieCollisionSubdiv = 0.2f;
        const float moveDist = prevPos.Distance(moveTrans.Position());
        if (moveDist > zombieCollisionSubdiv + 0.01f)
        {
          const Time backupVegCollisionTime = _vegCollisionTime;
          const bool backupHighPriorityContact = _highPriorityContact;
          const bool backupPurePersonContact = _purePersonContact;
          const bool backupObjectContact = _objectContact;

          ManVisualState collisionTest(moveTrans);
          int lastClearStep = 0;
          const int stepCount = intMax((int)ceilf(moveDist / zombieCollisionSubdiv), 2);
          const Vector3 endPos = moveTrans.Position();
          for (int step = 0; step < stepCount; step++)
          {
            collisionTest.SetPosition(prevPos.Lerp(endPos, (float)step / (stepCount - 1)));
            const float collisionVolume = ValidatePotentialState(collisionTest);

            // ValidatePotentialState updates contact bookkeeping. Keep the
            // real simulation's state intact while probing intermediate poses.
            _vegCollisionTime = backupVegCollisionTime;
            _highPriorityContact = backupHighPriorityContact;
            _purePersonContact = backupPurePersonContact;
            _objectContact = backupObjectContact;

            if (collisionVolume > 0.0f)
              break;

            lastClearStep = step;
          }

          // Leave a small forward step so the normal collision repair can
          // resolve contact instead of freezing the zombie exactly at the edge.
          if (lastClearStep < 2)
            lastClearStep = intMin(lastClearStep + 1, stepCount - 1);

          const float moveRatio = (float)lastClearStep / (stepCount - 1);
          moveTrans.SetPosition(prevPos.Lerp(endPos, moveRatio));
          // Keep movement/turn integration time consistent with the portion
          // of the candidate path that survived subdivision. Man::Simulate
          // scales deltaT before its collision-repair pass for this reason.
          deltaT *= moveRatio;
        }
      }

      const bool farCollision = false;
      if (prec<=SimulateVisibleFar) 
      {
        if (prec<=(farCollision ? SimulateVisibleFar : SimulateVisibleNear)) 
        {
          ManVisualState moveTransOrig = moveTrans;

          if (!bChange)
          {
            fVoll = ValidatePotentialStateThis();          

            bCalculatedVoll = true;
            if (fVoll > 0)         
              bChange = true;                      
          }

          if (bChange)
          {
            bool switchedState = false;
            bool reportContact = true;
            do 
            {
              if (switchedState)
              {
                // neutralize situation to original state
                moveTrans = moveTransOrig;
                SetleInPosition(moveTrans, 0,0, false);
                bCalculatedVoll = false;
                fVoll = 0;
                switchedState = false;
                reportContact = false; // do not report contacts during second pass
              }

              const MoveInfoMan * info = Type()->GetMoveInfo(_moves.GetPrimaryMove(FutureVisualState()._moves).id, AnimSubIx());
              bool hasCapsuleSafe = (info != NULL ? info->HasCollCapsuleSafe() : false);
              if (!_safeSystem || !hasCapsuleSafe)
              {
                _useSafeCapsule = false;
              }            

#if _ENABLE_CHEATS
              if (CHECK_DIAG(DEManCollision))
              {
                fVoll = ValidatePotentialStateThis();
                bCalculatedVoll = true;                 
              }
#endif
              fCollVol = CheckAndRepairPotencialState(moveTrans, turn, deltaT, fVoll, reportContact);

              if (fCollVol > 0 && !bCalculatedVoll)    
              {
                fVoll = ValidatePotentialStateThis();              
              }

              if (fCollVol * ALLOWED_VOL_GROW > fVoll  && fabsf(turn) > 1e-6)
              { 
                if (_safeSystem && info != NULL && info->HasCollCapsuleSafe())
                {           
                  if (_useSafeCapsule)
                  {
                    // Try it without move
                    // Take it on all cost.
                    moveTrans = FutureVisualState();
                    ApplyTurn(moveTrans,turn);
                    SetleInPosition(moveTrans,turn, deltaT);

                    fCollVol = CheckAndRepairPotencialState(moveTrans, turn, deltaT, fVoll);  
                    _highPriorityContactEnd = _highPriorityContact;

                    bTakeAllCost = true;
                  }
                  else
                  {
                    // Try it without move
                    ManVisualState moveTransNew = FutureVisualState();
                    ApplyTurn(moveTransNew,turn);
                    SetleInPosition(moveTransNew,turn, deltaT);                 

                    float fNewCollVol = CheckAndRepairPotencialState(moveTransNew, turn, deltaT, fVoll);
                    if (fNewCollVol <= fVoll)
                    {
                      moveTrans = moveTransNew;                   
                      fCollVol = fNewCollVol;  
                      _highPriorityContactEnd = _highPriorityContact;
                    }                     
                  } //if (_useSafeCapsule)
                } //if (_safeSystem)
#define TRY_TURN_MOVE_SEPARATELY 1
#if TRY_TURN_MOVE_SEPARATELY
                else
                {
                  //Try it without rotation
                  ManVisualState moveTransNew = moveTransOrig;
                  SetleInPosition(moveTransNew,0, deltaT);

                  float fNewCollVol = CheckAndRepairPotencialState(moveTransNew, 0, deltaT, fVoll);
                  if (fNewCollVol <= fVoll)
                  {
                    moveTrans = moveTransNew;                  
                    fCollVol = fNewCollVol;  
                    _highPriorityContactEnd = _highPriorityContact;
                  }
                  else
                  {
                    // Try it without move
                    moveTransNew = FutureVisualState();
                    ApplyTurn(moveTransNew,turn);
                    SetleInPosition(moveTransNew,turn, deltaT);

                    fNewCollVol = CheckAndRepairPotencialState(moveTransNew, turn, deltaT, fVoll);
                    if (fNewCollVol <= fVoll)
                    {
                      moveTrans = moveTransNew;                  
                      fCollVol = fNewCollVol; 
                      _highPriorityContactEnd = _highPriorityContact;
                    }                                
                  }
                }
#endif           
              } //if (fCollVol * ALLOWED_VOL_GROW > fVoll && fabsf(turn) > 1e-6)

              if (hasCapsuleSafe && fVoll < fCollVol * ALLOWED_VOL_GROW && _safeSystem && !_useSafeCapsule &&  fabsf(turn) > 1e-6 )
              {
                switchedState = true; 
                _useSafeCapsule = true; 
                //_collVolume = INFINITY_VOLUME;
              }
              else
              {
                if (QIsManual() && _lowPriorityContact && _highPriorityContactEnd && !_ignoreLowPriorituContact && 
                  fCollVol > 0 && fVoll > 0 && !hasCapsuleSafe)
                {
                  switchedState = true; 
                  _ignoreLowPriorituContact = true;

                  if (fVoll * 0.999 /* num. barier */ > _collVolume && fCollVol > 0 &&
                    _disableDamageFromObjUntil < Glob.time && !_purePersonContact)
                  {
                    DammageOnCollVol(fCollVol);                           
                  }                   
                }
              }
            } while(switchedState);    

            if (!_ignoreLowPriorituContact && fVoll * 0.999 /* num. barier */ > _collVolume && fCollVol > 0 &&
              _disableDamageFromObjUntil < Glob.time && !_purePersonContact)
            {
              DammageOnCollVol(fCollVol);                           
            } 

            //I'm sure it should be called somewhere here... otherwise on all non-colliding Man classes stay _collideWith set to last collision object...
            if(fVoll == 0)
            {
              OnNoCollide();
            }

          } //if (bChange) 
          else
            OnNoCollide();
        } //if (prec<=SimulateVisibleFar) 
        else
          OnNoCollide();

        if (bChange)
        {
          if ((fVoll >= fCollVol * ALLOWED_VOL_GROW || bTakeAllCost) /*&& prec <= SimulateVisibleNear && fCollVol == 0*/)
          {
            // take move
            if (_landContact)
            {
              // calculate y speed from x,z to follow surface
              FutureVisualState()._speed[1]=FutureVisualState()._speed[0]*_landDX+FutureVisualState()._speed[2]*_landDZ;
            }

            // calculate acceleration
            if( FutureVisualState()._speed.SquareSize()>=Square(0.1f) ) 
              _lastMovementTime=Glob.time;

            FutureVisualState()._acceleration = VZero;
            _angMomentum = VZero;
            _angVelocity = VZero;          

            if (!_ignoreLowPriorituContact)
            {
              _collVolume = fCollVol;
            }
            else
            {
              _ignoreLowPriorituContact = false;
              _collVolume = ValidatePotentialState(moveTrans);            
            }

            // Switch capsule back to normal one, if there is enough space. 
            if (_useSafeCapsule)
            {
              _useSafeCapsule = false;
              if (ValidatePotentialState(moveTrans)>0)
              {
                // safe capsule needs to be used again - normal capsule in collision
                _useSafeCapsule = true;
              }
              else if (!_useSafeCapsule)
              {
                // detection confirmed we can safely use normal capsule
                _collVolume = INFINITY_VOLUME;
              }
            }
          }
          else
          {
            // use old state
            UseOldState();      

            moveTrans = FutureVisualState();
            bChange = false;
          }
        }
      }
      else
      {
        OnNoCollide(); //if (prec<=SimulateVisibleFar) 
      }


      if (_landContact && _afterGetOut)
      {
        _afterGetOut = false; // after get out is only till landcontact
      }  
      // if all collisions was ignored because of "stuck" and man is not in collision anymore do not ignore ... 
      if (_ignoreCollisionIfStuck)
      {
        _stuckStarted = TIME_MAX;
        _ignoreCollisionIfStuck = false;
        if (!isZombie && ValidatePotentialState(moveTrans) > 0)
          _ignoreCollisionIfStuck = true;              
      }

      // LightAI has its own copy of Man::Simulate. Keep the swept movement
      // collision check here as well: endpoint overlap tests alone can miss
      // thin walls/fences when an AI step moves completely through them.
      if (deltaT > 0 && (prec <= SimulateVisibleNear || (isZombie && prec <= SimulateVisibleFar)))
      {
        // Use the same swept body radius and vertical offset as zombie route
        // checks. A zero-radius ray through the aiming point can miss a wall
        // when the zombie's capsule clips an edge or thin fence section.
        Vector3 oldPos = FutureVisualState().Position() + ZombieRayTestOffset;
        Vector3 newPos = moveTrans.Position() + ZombieRayTestOffset;

        CollisionBuffer collision;
        if (newPos.Distance2(oldPos) > Square(0.1f) && !_ignoreCollisionIfStuck)
        {
          GLandscape->ObjectCollisionLine(
            GetFutureVisualStateAge(), collision, Landscape::FilterIgnoreOne(this),
            oldPos, newPos, ZombieRayTestRadius, ObjIntersectGeom);
        }

        if (collision.Size() > 0)
        {
          // Keep diagnostic evidence even for objects rejected by the common
          // IsPassable() mass threshold. This distinguishes a missing
          // Geometry hit from a hit that collision policy intentionally
          // discards (a likely failure mode for lightweight map structures).
          static Time nextZombieCollisionProbe = TIME_MIN;
          if (isZombie && GetNetworkManager().IsServer() && Glob.time >= nextZombieCollisionProbe)
          {
            int logged = 0;
            for (int i = 0; i < collision.Size() && logged < 3; i++)
            {
              const CollisionInfo &info = collision[i];
              Object *obj = info.object;
              if (!obj)
                continue;
              ZombieProbeLog("[ZombieCollisionProbe] %s hit=%s t=%.3f entry=%d exit=%d mass=%.2f passable=%d static=%d hierarchy=%d geom=%d component=%d point=%.2f %.2f %.2f normal=%.2f %.2f %.2f",
                cc_cast(Type()->GetName()), cc_cast(obj->GetDebugName()), info.under,
                info.entry ? 1 : 0, info.exit ? 1 : 0,
                obj->GetMass(), obj->IsPassable() ? 1 : 0, obj->Static() ? 1 : 0,
                info.hierLevel, info.geomLevel, info.component,
                info.pos.X(), info.pos.Y(), info.pos.Z(),
                info.dirOutNotNorm.X(), info.dirOutNotNorm.Y(), info.dirOutNotNorm.Z());
              logged++;
            }
            nextZombieCollisionProbe = Glob.time + 1.0f;
          }

          float minT = 1e10f;
          Object *hitObject = NULL;
          bool hitEntry = false;
          bool hitExit = false;
          int hitGeomLevel = -1;
          int hitComponent = -1;
          bool sawStartOverlap = false;
          Object *startOverlapObject = NULL;
          bool startOverlapExit = false;
          for (int i = 0; i < collision.Size(); i++)
          {
            const CollisionInfo &info = collision[i];
            Object *obj = info.object;
            if (!obj)
              continue;

            // The generic passability threshold is mass-based. Static map
            // buildings can have a Geometry LOD but a mass below that
            // threshold; zombies must still stop at their physical surfaces.
            // IsPassable() is a generic mass threshold (<10), not a map
            // collision policy. Static world geometry includes fences and
            // other obstacle classes that are not necessarily Building
            // entities, so a zombie must respect every static Geometry hit.
            const bool zombieStaticWorldBlock = isZombie && obj->Static();
            if (obj->IsPassable() && !zombieStaticWorldBlock)
              continue;

            _objectContact = true;
            _objectContactNonStatic = _objectContactNonStatic || !obj->Static();

            // ObjectCollisionLine reports under == 0 when its start point is
            // already inside (or touching) the expanded Geometry LOD. That is
            // not an entry time: clamping to 1% of the proposed move keeps the
            // zombie in the same overlap on every following frame. The normal
            // animated-body collision repair and the subdivided pose checks
            // above are the authoritative fallback for this ambiguous case.
            // Keep the supplemental sweep for actual outside-to-inside hits,
            // which are the cases that can tunnel through thin geometry.
            if (!info.entry)
            {
              if (isZombie && !sawStartOverlap)
              {
                sawStartOverlap = true;
                startOverlapObject = obj;
                startOverlapExit = info.exit;
              }
              continue;
            }

            if (info.under < minT)
            {
              minT = info.under;
              hitObject = obj;
              hitEntry = info.entry;
              hitExit = info.exit;
              hitGeomLevel = info.geomLevel;
              hitComponent = info.component;
            }
          }

          if (sawStartOverlap && minT >= 1e10f && isZombie && GetNetworkManager().IsServer())
          {
            static Time nextZombieOverlapDiagnostic = TIME_MIN;
            if (Glob.time >= nextZombieOverlapDiagnostic)
            {
              ZombieProbeLog("[ZombieCollisionStartOverlap] %s object=%s exit=%d from %.2f %.2f %.2f to %.2f %.2f %.2f; leaving to body solver",
                cc_cast(Type()->GetName()),
                startOverlapObject ? cc_cast(startOverlapObject->GetDebugName()) : "<unknown>",
                startOverlapExit ? 1 : 0,
                oldPos.X(), oldPos.Y(), oldPos.Z(), newPos.X(), newPos.Y(), newPos.Z());
              nextZombieOverlapDiagnostic = Glob.time + 1.0f;
            }
          }

          if (minT < 1)
          {
            static Time nextZombieSweepDiagnostic = TIME_MIN;
            if (isZombie && GetNetworkManager().IsServer() && Glob.time >= nextZombieSweepDiagnostic)
            {
              ZombieProbeLog("[ZombieCollisionDiag] %s swept into %s t=%.3f entry=%d exit=%d geom=%d component=%d from %.2f %.2f %.2f to %.2f %.2f %.2f",
                cc_cast(Type()->GetName()),
                hitObject ? cc_cast(hitObject->GetDebugName()) : "<unknown>",
                minT, hitEntry ? 1 : 0, hitExit ? 1 : 0, hitGeomLevel, hitComponent,
                oldPos.X(), oldPos.Y(), oldPos.Z(), newPos.X(), newPos.Y(), newPos.Z());
              nextZombieSweepDiagnostic = Glob.time + 1.0f;
            }
            // Stop just before the first entering contact. For very short
            // steps the old 1% lower bound moved the body into the obstacle,
            // turning the next query into an ambiguous start-overlap.
            minT = floatMax(minT - 0.005f, 0.0f);
            Vector3 col = FutureVisualState().Position() * (1 - minT) + moveTrans.Position() * minT;
            repairMove += col - moveTrans.Position();
            moveTrans.SetPosition(col);
          }
        }
      }
    }
  } // if( _freeFall )

  MoveWeapons( deltaT, bChange);

  // Calculate body position after hit
  if(fabs(_hitSkeletonRotWanted - _hitSkeletonRot) > 0.01f)
  {
    //delta = _hitSkeletonRotWanted - _hitSkeletonRot;
    float d = deltaT * 6.0f;
    float delta = _hitSkeletonRotWanted - _hitSkeletonRot;
    Limit(delta, -d, d);

    if(fabs(delta) < 0.04f)
    {
      _hitSkeletonRot = _hitSkeletonRotWanted;
    }
    else
    {
      //delta *= 0.3f;
      _hitSkeletonRot = _hitSkeletonRot + delta;
    }

    RecalcAffectSkeletonTransform();
  }
  else  //vrat se zpatky do vzprimeny pozice
  {
    _hitSkeletonRotWanted = 0.f;
  }

  _impulseForce = VZero; 
  _impulseTorque = VZero; 

  if (_doMark>0)
  {
    // it makes no sense to create stepmarks which are far from the camera
    VisualState &vs = FutureVisualState();
    if (EnableVisualEffects(prec, 20) && ValueTest(vs,&MoveInfoMan::OnLand) < 0.5f)
    {
      int index=(_doMark>1 ? Type()->_stepLIndex : Type()->_stepRIndex );
      // check foot position
      if (index>=0)
      {
        Vector3 mPos=AnimatePoint(vs,_shape->FindMemoryLevel(),index);
        Vector3 pos=vs.PositionModelToWorld(mPos);
        // leave stepmark

        float timeToLive= 10.0f * (GEngine->GetThermalVision() ? 5.0f : 1.0f);

        //timeToLive*=0.3;
        //Ref<Mark> mark=new Mark(shape,0.15,timeToLive);
        float ys = GLandscape->SurfaceY(pos.X(),pos.Z());
        bool isWater = false;
        Texture *texture = NULL;
        float yr = GLandscape->RoadSurfaceYAboveWater(pos, NULL, NULL, &texture);

#if _HELISIM_TERRAIN_RIVERS
        if (texture)
          isWater = texture->IsWater();
#endif

        if (!isWater && yr<=ys+0.2f)
        {
          LODShape *shape=GScene->Preloaded(_doMark>1 ? FootStepL : FootStepR);
          // TODO: VSMP: place mark on the finally confirmed position, not here
          Ref<Mark> mark=new Mark(shape,(GEngine->GetThermalVision() ? 1.0f : 0.5f),timeToLive);
          mark->SetDirectionAndUp(vs.Direction(),VUp);
          mark->SetPosition(pos);
          GWorld->AddAnimal(mark);
        }
      }
    }
    _doMark = 0;
  }

  //move
  {
    Move(moveTrans);
    DirectionWorldToModel(FutureVisualState()._modelSpeed, FutureVisualState()._speed);
  }

  // tell the animation change to the scripts
  if (_moves.GetPrimaryMove(FutureVisualState()._moves).id != _primaryMoveBackUp.id)
  {
    OnEvent(EEAnimStateChanged, GetMovesType()->GetMoveName(_moves.GetPrimaryMove(FutureVisualState()._moves).id));
  }

  if (bChange)
  {
    // if position or animation changed, we need to recalculate some world space / model space properties     
    RecalcPositions(moveTrans); 
  }

  _primaryMoveBackUp = _moves.GetPrimaryMove(FutureVisualState()._moves);   
  _primaryTimeBackUp = _moves.GetPrimaryTime(FutureVisualState()._moves);
  _primaryTimeDBackUp = _moves.GetPrimaryTimeDelta(FutureVisualState()._moves);
  _primaryFactorBackUp = _moves.GetPrimaryFactor(FutureVisualState()._moves);   
  _finishedBackUp = _moves.GetFinished(FutureVisualState()._moves);

  // Simulation of head movement, it set up action, mimic
  // Can be called at the end;
  BasicSimulationCore(deltaT, prec); 

  if (_waterDepth>0) //old solution kept when there is no _headAxisPoint
  {
    _waterContact = true;
  }
  else
  {
    _waterContact = false;
  }  

  // fake distance hiding simulation
  const float MaxCoverGrassSpeed = 0.07f;
  float delta = (_coverGrassHeightWanted - _coverGrassHeight) * deltaT;
  saturate(delta, -MaxCoverGrassSpeed, MaxCoverGrassSpeed);
  _coverGrassHeight += delta;

  //can't use base::Simulate here, as we reimplemented Man::Simulate and a lot of stuff could be done twice
  Person::Simulate(deltaT,prec);
  SimulatePost(deltaT0,prec);
  // DoAssert(_moves.CheckValidState(GetMovesType()));
  // DoAssert(_gesture.CheckValidState(GetGesturesType()));
  UpdateJoinedObject(); // we call it in each frame to stay safe (we do not know if we are handling all possible changes)
}

void LightAI::Say(const AutoArray<RStringB>& sounds, bool pos3D)
{
  //don't waste time
  if(sounds.Size() == 0 || IsDamageDestroyed() || _nearestPlayer > Square(100))
    return;

  int index = GRandGen.RandomInt(0, sounds.Size());

  // right side argument(s)
  GameStringType voice;
  float distance = 100.0f;
  float speed = 1.0f;

  Vehicle *veh = new SoundOnVehicle(sounds[index], this, true, distance, speed, NULL);
  veh->SetPosition(FutureVisualState().Position());
  GWorld->AddSlowVehicle(veh);
  if (pos3D)
  {
    GetNetworkManager().CreateVehicle(veh, VLTSlowVehicle, sounds[index], -1);
  }
}

void LightAI::SimulateAI( float deltaT, SimulationImportance prec )
{
    static Time nextZombieAISimEntryProbe = TIME_MIN;
  if (dynamic_cast<Zombie *>(this) && Glob.time >= nextZombieAISimEntryProbe)
  {
    ZombieProbeLog("[ZombieAISimEntry] %s disabled=%d local=%d server=%d prec=%d", cc_cast(Type()->GetName()), IsSimulationDisabled() ? 1 : 0, IsLocal() ? 1 : 0, GetNetworkManager().IsServer() ? 1 : 0, (int)prec);
    AutoArray<NetPlayerInfo> probePlayers;
    GetNetworkManager().GetPlayers(probePlayers);
    ZombieProbeLog("[ZombiePlayerPos] count=%d", probePlayers.Size());
    for (int playerIndex = 0; playerIndex < probePlayers.Size(); playerIndex++)
    {
      Person *probePerson = dyn_cast<Person, NetworkObject>(probePlayers[playerIndex].person);
      if (probePerson)
      {
        Vector3 probePos = probePerson->FutureVisualState().Position();
        ZombieProbeLog("[ZombiePlayerPos] name=%s pos=%.2f %.2f %.2f", cc_cast(probePlayers[playerIndex].name), probePos.X(), probePos.Y(), probePos.Z());
      }
    }
    nextZombieAISimEntryProbe = Glob.time + 1.0f;
  }
  if (IsSimulationDisabled())
    return;

  PROFILE_SCOPE_EX(siUSo,sim);

  if (dynamic_cast<Zombie *>(this) && GetNetworkManager().IsServer())
  {
    static Time nextZombieAISimulationDiagnostic = TIME_MIN;
    if (Glob.time >= nextZombieAISimulationDiagnostic)
    {
      ZombieProbeLog("[ZombieAISim] zombie=%s local=%d brain=%d fsm=%d pos=%.2f %.2f %.2f",
        cc_cast(Type()->GetName()), IsLocal() ? 1 : 0, Brain() ? 1 : 0,
        _fsm ? (int)_fsm->GetState() : -1,
        FutureVisualState().Position().X(), FutureVisualState().Position().Y(),
        FutureVisualState().Position().Z());
      nextZombieAISimulationDiagnostic = Glob.time + 1.0f;
    }
  }

  if( !_isDead )
  {
    //in idle we are in rest
    if(_fsm->GetState() == AISIdle)
      _tiredFactor -= deltaT * 2.5f;

    if(_tiredFactor <= 0)
    {
      _tiredFactor = 0;
      _tiredFactorHysteresis = 0;
    }
    else if(_tiredFactor >= 60)
    {
      _tiredFactor = 60;
      _tiredFactorHysteresis = 60;
    }
    else if((_tiredFactor - 20) > _tiredFactorHysteresis)
      _tiredFactorHysteresis = _tiredFactor;
    else if((_tiredFactor + 20) < _tiredFactorHysteresis)
      _tiredFactorHysteresis = _tiredFactor;

    if (!IsLocal()) FakePilot(deltaT,prec);
#if _ENABLE_AI
    else if ( Brain() )
    {
      //start the "AI" now
      if(_fsm->GetState() == AISNone)
        _fsm->SetState(AISIdle, this);

      UpdateIgnored();

      //don't think when unconscious
      if(Brain()->GetLifeState() == LifeStateUnconscious)
      {
        //just update targets
        UpdateTargets();
      }
      else
      {
        _fsm->Update(this);
      }
      //    if (Brain()->GetAIDisabled() & AIBrain::DAAnim)
      //    DisabledPilot(deltaT,prec);
      //else
      AIPilot(deltaT,prec);
    }
#endif
  }
  else
  {
    //ActionMap *map = Type()->GetActionMap(_primaryMove);
    // get which action is it (based on action map)
    //ManAction selAction = ManActDie;
    //MoveId moveWanted = map->GetAction(selAction);

    // some actions are edge sensitive - must be executed
    if( _moves.GetForceMove().id!=MoveIdNone )
    {
      SetMoveQueue(MotionPathItem(_moves.GetForceMove()),prec<=SimulateVisibleFar);
    }   
  }

  base::SimulateAI(deltaT,prec);
}

inline ManPos SaturatePos(int wantedPos, int minPos, int maxPos)
{
  saturate(wantedPos, minPos, maxPos);
  return (ManPos)wantedPos;
}

#define SATURATE_POS(wantedPos) SaturatePos(wantedPos, minPos, maxPos)

//-----------------------------------------------------------------------------
class FilterIgnoreSet : public IntersectionFilter
{
protected:
  SortedArray<Object*>& _avoided;

public:
  FilterIgnoreSet(SortedArray<Object*>& avoided) :_avoided(avoided) {}
  bool operator()(const Object * obj) const
  {
    if(_avoided.Find(const_cast<Object*>(obj)) >= 0)
      return false;
    //if(dynamic_cast<Man*>(obj))
      //return false;
    return true;
  }
};


bool LineLineIntersection(const enf::Vector2& p1, const enf::Vector2& p2, const enf::Vector2& p3, const enf::Vector2& p4)
{
  float p0_x = p1.x, p0_y = p1.y;
  float p1_x = p2.x, p1_y = p2.y;
  float p2_x = p3.x, p2_y = p3.y;
  float p3_x = p4.x, p3_y = p4.y;

  float s1_x, s1_y, s2_x, s2_y;
  s1_x = p1_x - p0_x;     s1_y = p1_y - p0_y;
  s2_x = p3_x - p2_x;     s2_y = p3_y - p2_y;

  float denominator = -s2_x * s1_y + s1_x * s2_y;
  if (fabsf(denominator) < 1e-6f)
    return false;

  float s, t;
  s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / denominator;
  t = ( s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / denominator;

//  if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
  if (s > 0 && s < 1 && t > 0 && t < 1)
  {
    // Collision detected
    return true;
  }

  return false; // No collision
}

struct PFNode;
typedef AutoArray<SRef<PFNode> > TNodesArray;

struct PFObstacle
{
  short nodes[4];
  enf::AAB2 bounds;
  int _internNodesFrom;
  int _internNodesCount;

  PFObstacle(): _internNodesFrom(-1), _internNodesCount(0) {}
  bool Intersects(const enf::Vector2& from, const enf::Vector2& to, const TNodesArray& nodes) const;
  bool Intersects(const enf::Vector2& from, const TNodesArray& nodes) const;
  bool Intersects(int from, int to, const TNodesArray& nodes) const;
  bool Intersects(int from, const TNodesArray& nodes) const;
};

struct PFEdge
{
  short _target;
  short _cost;
  PFEdge(): _target(0), _cost(0) {}
  PFEdge(int target, float cost): _target((short)target), _cost((short)cost) {}

  bool operator==(const PFEdge& other) const
  {
    return _target == other._target;
  }
  bool operator<(const PFEdge& other) const
  {
    return _target < other._target;
  }
  bool operator>(const PFEdge& other) const
  {
    return _target > other._target;
  }
};
TypeIsMovableZeroed(PFEdge);

static const int MAX_EDGES = 8;
static const float MAX_EDGE_LENGTH = 45;
const float PF_BOUNDS_HYSTERESIS = 10;

enum NodeType
{
  NT_ENABLED = 1 << 0,
  NT_ENTRANCE = 1 << 1,
  NT_BUILDING = 1 << 2,
};

struct PFNode: public enf::Vector2
{
  SortedArray<PFEdge> _edges;
  int _type;
  float _height;
  float _longestEdge;

  PFNode(): _type(NT_ENABLED), _height(0), _longestEdge(0) {}

  bool AddEdge(int to, float dist, bool force)
  {
    if(!force)
    {
      if(_edges.Size() > MAX_EDGES)
      {
        //we are not any shorter. Give it up!
        if(dist >= _longestEdge)
        {
          return false;
        }

        int bestEdge = -1;
        short bestCost = (short)dist;
        for(int n = 0; n < _edges.Size(); n++)
        {
          if(_edges[n]._cost > bestCost)
          {
            bestEdge = n;
            bestCost = _edges[n]._cost;
          }
        }
        if(bestEdge < 0)
          return false;

        _edges.DeleteAt(bestEdge);
        _longestEdge = dist;
        for(int n = 0; n < _edges.Size(); n++)
        {
          saturateMax(_longestEdge, _edges[n]._cost);
        }
        _edges.AddUnique(PFEdge(to, dist));
        return true;
      }
    }
    _edges.AddUnique(PFEdge(to, dist));
    saturateMax(_longestEdge, dist);

    return true;
  }

};

TypeIsMovableZeroed(PFObstacle);

class PathFinder
{
public:
  PathFinder(bool visualize): _baseNodes(0), _visualize(visualize) {}
  ~PathFinder()
  {
    Clear();
  }
  void Clear()
  {
    _nodes.Clear();
    _obstacles.Clear();
    _obstacleObjects.Clear();
    _shapes.Clear();
    _shapes2.Clear();
    _bounds.Init();
  }

  bool IsValid(const enf::AAB2& bounds) const
  {
    if((bounds.GetMins().x - PF_BOUNDS_HYSTERESIS) < _bounds.GetMins().x ||
      (bounds.GetMins().y - PF_BOUNDS_HYSTERESIS) < _bounds.GetMins().y ||
      (bounds.GetMaxs().x + PF_BOUNDS_HYSTERESIS) > _bounds.GetMaxs().x ||
      (bounds.GetMaxs().y + PF_BOUNDS_HYSTERESIS) > _bounds.GetMaxs().y)
      return false;

    return true;
  }

  int AddNode(Vector3Par p)
  {
    PFNode* node = new PFNode;
    node->x = p.X();
    node->y = p.Z();
    node->_height = p.Y();
    float h = GLandscape->SurfaceY(p.x, p.z);
    if(h > node->_height)
      node->_height = h;
    _nodes.Add(node);
    return _nodes.Size() - 1;
  }

  void AddConnection(int from, int to, bool force = false)
  {
    if(!_nodes[from] || !_nodes[to])
      return;

    float dist = _nodes[from]->Distance(*_nodes[to]);
    if(!force && dist > MAX_EDGE_LENGTH)
      return;
    _nodes[from]->AddEdge(to, dist, force);
    _nodes[to]->AddEdge(from, dist, force);
  }

  int AddNodeOnGround(Vector3Par p)
  {
    float h = GLandscape->SurfaceY(p.x, p.z);
    return AddNode(Vector3(p.X(), h, p.Z()));
  }

  void AddObstacle(Object* obj, float radius)
  {
    if(_obstacleObjects.Find(obj) >= 0)
      return;

    Matrix4 _transform = obj->WorldTransform(obj->FutureVisualState());
    if(_transform.DirectionUp().Normalized().DotProduct(VUp) < 0.9f)
      return;

    const Vector3* minMax = obj->GetShape()->GeometryLevel()->MinMax();

    enf::AAB bounds(true);
    
    int level = obj->GetShape()->FindGeometryLevel();
    const Shape *geom = obj->GetShape()->GeometryLevel();
    const ConvexComponents &geomComponent = obj->GetShape()->GetGeomComponents();
    
    AnimationContextStorageGeom storage;
    AnimationContext animContext(obj->FutureVisualState(), storage);
    geom->InitAnimationContext(animContext, obj->GetShape()->GetConvexComponents(obj->GetShape()->FindGeometryLevel()), true);
    obj->Animate(animContext, level, false, obj, -FLT_MAX);
    
    float bottom = animContext.MinMax()[0].Y();
    for(int n = 0; n < geomComponent.Size(); n++)
    {
      const Vector3* minsMaxs = animContext.GetCCMinMax(n);
      float lbottom = minsMaxs[0].Y();
      float above = lbottom - bottom;
      if(above > 2.5f)
        continue;
      bounds.AddMin(minsMaxs[0]);
      bounds.AddMax(minsMaxs[1]);
    }
    obj->Deanimate(level,true);
    if(!bounds.IsValid())
      return;

    minMax = bounds.GetMinMax();

    {
      Vector3 mins = minMax[0] - Vector3(radius);
      Vector3 maxs = minMax[1] + Vector3(radius);

      if(_visualize)
      {
        Ref<enf::Shape> shape = enf::Shape::CreateBox(mins, maxs, 0x07f07f00, enf::SS_TRANSP|enf::SS_NOZWRITE);
        shape->SetOutlineColor(0x7f00ff00);
        shape->SetMatrix(_transform);
        _shapes.Add(shape);
      }

      PFObstacle& obst = _obstacles.Append();
       
      Vector3 p0 = _transform.FastTransform(Vector3(mins.X(), 0, mins.Z()));
      Vector3 p1 = _transform.FastTransform(Vector3(maxs.X(), 0, mins.Z()));
      Vector3 p2 = _transform.FastTransform(Vector3(maxs.X(), 0, maxs.Z()));
      Vector3 p3 = _transform.FastTransform(Vector3(mins.X(), 0, maxs.Z()));

      obst.bounds.Init(enf::Vector2(p0.x, p0.z));
      obst.bounds.Add(enf::Vector2(p1.x, p1.z));
      obst.bounds.Add(enf::Vector2(p2.x, p2.z));
      obst.bounds.Add(enf::Vector2(p3.x, p3.z));

      obst.nodes[0] = AddNodeOnGround(p0);
      obst.nodes[1] = AddNodeOnGround(p1);
      obst.nodes[2] = AddNodeOnGround(p2);
      obst.nodes[3] = AddNodeOnGround(p3);
#if 0
    IPaths *building = unconst_cast(obj->GetIPaths());
    if (building)
    {
      int posNodes = _nodes.Size();
      obst._internNodesFrom = posNodes;
      obst._internNodesCount = building->NPoints();

      for(int n = 0; n < building->NPoints(); n++)
      {
        Vector3 p = building->GetPosition(n);
        if(GLandscape->SurfaceY(p.X(),p.Z()) > p.Y())
        {
          _nodes.Add(NULL); //dummy node
          continue;
        }

        PathAction *pa = building->GetPathAction(n);
        if(pa)
        {
          RString name = pa->GetActionName();
          if(pa->GetType() == PATUserActionBegin)
          {
            const AnimationHolder* anims = obj->GetShape()->GetAnimations();
            const AnimationHolder* anims2 = obj->Type() ? &obj->Type()->GetAnimations() : NULL;
            for(int n = 0; n < anims->Size(); n++)
            {
              AnimationType* animType = (*anims)[n];
              RString animName = animType->GetName();
              animName = animName;

            }

          }
          if(pa->GetType() == PATUserActionEnd)
          {
            const AnimationHolder* anims = obj->GetShape()->GetAnimations();
            for(int n = 0; n < anims->Size(); n++)
            {
              AnimationType* animType = (*anims)[n];
              RString animName = animType->GetName();
              animName = animName;
            }
          }
        }

        if(_visualize)
        {
          _shapes.Add(enf::Shape::CreatePyramid(p + Vector3(-0.05,0.25,-0.05), p + Vector3(0.05,0.3,0.05), 0x3fffffff, enf::SS_TRANSP|enf::SS_NOZWRITE));
        }
        int node = AddNode(p);
        _nodes[node]->_type |= NT_BUILDING;
      }

      for(int n = 0; n < building->NPoints(); n++)
      {
        const FindArray<int> &connections = building->GetConnections(n);
        for(int c = 0; c < connections.Size(); c++)
        {
          int to = connections[c];
          AddConnection(n + posNodes, to + posNodes, true);
        }
      }

      for(int n = 0; n < building->NExits(); n++)
      {
        int from = building->GetExit(n);
        if(_nodes[from + posNodes])
          _nodes[from + posNodes]->_type |= NT_ENTRANCE;
      }
    }
#endif
    }
    _obstacleObjects.AddUnique(obj);
    _baseNodes = _nodes.Size();
  }

  int Intersects(const enf::Vector2& p) const
  {
    for(int n = 0; n < _obstacles.Size(); n++)
    {
      if(_obstacles[n].Intersects(p, _nodes))
        return n;
    }
    return -1;
  }

  bool Intersects(const enf::Vector2& from, const enf::Vector2& to) const
  {
    for(int n = 0; n < _obstacles.Size(); n++)
    {
      if(_obstacles[n].Intersects(from, to, _nodes))
        return true;
    }
    return false;
  }
  bool Intersects(int from, int to) const
  {
    for(int n = 0; n < _obstacles.Size(); n++)
    {
      if(_obstacles[n].Intersects(from, to, _nodes))
        return true;
    }
    return false;
  }

  bool Intersects(int from) const
  {
    for(int n = 0; n < _obstacles.Size(); n++)
    {
      if(_obstacles[n].Intersects(from, _nodes))
        return true;
    }
    return false;
  }

  int GenerateEdges(int y, bool force = false)
  {
    if(!_nodes[y])
      return 0;

    int fromType = _nodes[y]->_type;

    //avoid disabled nodes
    if(!(fromType & NT_ENABLED))
      return 0;

    //avoid internal building nodes
    if((fromType & (NT_BUILDING|NT_ENTRANCE)) == NT_BUILDING)
      return 0;

    int num = 0;

    for(int x = 0; x < y; x++)
    {
      if(!_nodes[x])
        continue;
      int toType = _nodes[x]->_type;
      if((toType & (NT_BUILDING|NT_ENTRANCE)) == NT_BUILDING) //avoid internal edges
        continue;
      if(!(toType & NT_ENABLED))
        continue;

      if(_nodes[y]->_edges.Find(PFEdge(x,0)) >= 0)
        continue; //already exists

      if(Intersects(y, x))
        continue; //not visible

      AddConnection(y, x, force);
      num++;
    }
    return num;
  }

  void GenerateGraph()
  {
    //mark occluded nodes
    for(int y = 0; y < _nodes.Size(); y++)
    {
      if(_nodes[y] && (_nodes[y]->_type & (NT_BUILDING|NT_ENTRANCE)) == 0)
      {
        if(Intersects(y))
          _nodes[y]->_type &= ~NT_ENABLED;
        else
        {
          _nodes[y]->_type |= NT_ENABLED;
        }
        
/*        if(_visualize)
        {
          float h = GLandscape->SurfaceY(_nodes[y]->x, _nodes[y]->y);
          Vector3 p(_nodes[y]->x, h, _nodes[y]->y);
          _shapes.Add(enf::Shape::CreatePyramid(p - Vector3(0.05, 0.25, 0.05), p + Vector3(0.05, 0.3, 0.05), (_nodes[y]->_type & NT_ENABLED) ? 0xff00ff00 : 0xffff0000, 0));
        }*/
      }
    }

    for(int y = 0; y < _nodes.Size(); y++)
    {
      GenerateEdges(y);
    }

    if(_visualize)
    {
      for(int y = 0; y < _nodes.Size(); y++)
      {
        if(!_nodes[y])
          continue;

        const PFNode& node = *_nodes[y];
        for(int x = 0; x < node._edges.Size(); x++)
        {
          if(node._edges[x]._target < y)
            continue;

          const PFNode& targetNode = *_nodes[node._edges[x]._target];

          enf::Vector2 fromVec = node;
          enf::Vector2 toVec = targetNode;
          enf::uint color = 0xffff0000;
          int fromNode = 0;
          if(node._type & NT_BUILDING)
          {
            if(node._type & NT_ENTRANCE)
              fromNode = 2;
            else
              fromNode = 1;
          }
          int toNode = 0;
          if(targetNode._type & NT_BUILDING)
          {
            if(targetNode._type & NT_ENTRANCE)
              toNode = 2;
            else
              toNode = 1;
          }

          if(fromNode == 0 && toNode == 0)
            color = 0xffffffff;
          else if(fromNode == 1 && toNode == 1)
            color = 0xff0000ff;
          else if((fromNode == 1 && toNode == 2) || (fromNode == 2 && toNode == 1))
            color = 0xff00ffff;
          else if((fromNode == 0 && toNode == 2) || (fromNode == 2 && toNode == 0))
            color = 0xffffff00;

          Vector3 fromVec3 = Vector3(fromVec.x, node._height + 0.25f, fromVec.y);
          Vector3 toVec3 = Vector3(toVec.x, targetNode._height + 0.25f, toVec.y);
          _shapes.Add(enf::Shape::CreateLine(fromVec3, toVec3, color, 0));
        }
      }
    }
  }

  struct HeapIntByCost
  {
    const int *_cost;

    HeapIntByCost(const int *cost):_cost(cost){}
    bool IsLess(int a, int b) {return _cost[a]<_cost[b];}
  };

  int FindNode(Vector3Par pos) const
  {
    int obst = Intersects(enf::Vector2(pos.X(), pos.Z()));
    if(obst != -1 && _obstacles[obst]._internNodesFrom != -1)
    {
      float bestDist = FLT_MAX;
      int bestNode = -1;
      for(int n = 0; n < _obstacles[obst]._internNodesCount; n++)
      {
        int node = n + _obstacles[obst]._internNodesFrom;

        if(!_nodes[node] || _nodes[node]->_height > pos.Y() || (_nodes[node]->_height + 2.0f) < pos.Y())
          continue;

        float dist = _nodes[node]->SquaredDistance(enf::Vector2(pos.x, pos.z));
        if(dist < bestDist)
        {
          //TODO: check intersection with edges!

          bestDist = dist;
          bestNode = node;
        }
      }
      if(bestDist < 2.0f)
      {
        return bestNode;
      }
    }
    return -1;
  }

  Vector3 NodePosition(int node) const
  {
    if(!_nodes[node])
      return VZero;
    return Vector3(_nodes[node]->x, _nodes[node]->_height, _nodes[node]->y);
  }

  void Build(const enf::AAB& bounds);

  int GenerateValidNode(Vector3Par fromPos)
  {
    //end-points can be inside obstacle, as they are "bigger"
    static const Vector3 jitter[] = {
      Vector3(0,0,0),
      Vector3(-1,0,-1), Vector3(1,0,-1),
      Vector3(1,0,1), Vector3(-1,0,1)
    };

    for(int s = 1; s <= 3; s++)
    {
      for(int n = 0; n < 5; n++)
      {
        Vector3 newPos = fromPos + (jitter[n] * (float)s);
        if(Intersects(enf::Vector2(newPos.X(), newPos.Z())) >= 0)
          continue;

        int fromNode = AddNode(newPos);
        if(GenerateEdges(fromNode, true) <= 0)
        {
          //node cannot be connected
          _nodes.Delete(fromNode);
          continue;
        }
        return fromNode;
      }
    }
    return -1;
  }

  bool FindPath(Vector3Par fromPos, Vector3Par toPos, AutoArray<int>& path)
  {
    _shapes2.Clear();

    //remove old end-nodes
    if(_baseNodes < _nodes.Size())
    {
      for(int n = _baseNodes; n < _nodes.Size(); n++)
      {
        const PFNode& node = *_nodes[n];
        for(int i = 0; i < node._edges.Size(); i++)
        {
          _nodes[node._edges[i]._target]->_edges.Delete(PFEdge(n, 0));
        }
      }
      _nodes.Resize(_baseNodes);
    }

    //try existing nodes (buildings)
    int fromNode = FindNode(fromPos);
    int toNode = FindNode(toPos);

    if(fromNode < 0)
    {
      fromNode = GenerateValidNode(fromPos);
      if(fromNode < 0)
        return false;
    }

    if(toNode < 0)
    {
      toNode = GenerateValidNode(toPos);
      if(toNode < 0)
        return false;
    }

    //perform Dijkstra's search
    int nVerts=_nodes.Size();
    AUTO_STATIC_ARRAY(int,d,1024);
    AUTO_STATIC_ARRAY(int,p,1024);
    d.Resize(nVerts);
    p.Resize(nVerts);
    for( int i=0; i<nVerts; i++ ) d[i]=INT_MAX,p[i]=-1; // no paths yet
    // source has trivial solution
    d[fromNode]=0;

    // use heap array for vs (need fast extract min)

    HeapIntByCost cost(d.Data());
    HeapArray<int,MemAllocDataStack<int,1024>,HeapIntByCost> vs(cost);

    for( int i=0; i<nVerts; i++ )
    {
      // only add vertices that have some edges connected to them
      if (!_nodes[i] || !_nodes[i]->_edges.Size())
        continue;
      vs.HeapInsert(i);
    }

    const int unaccessible=INT_MAX/16;

#if _DEBUG
    int iter = 0;
#endif
    while (d[toNode] == INT_MAX)
    {
#if _DEBUG
      iter++;
#endif
      int v;
      if (!vs.HeapRemoveFirst(v)) break;
      int vCost=d[v];
      if (vCost>=unaccessible) break;
      // relax all adjacent vertices
      const PFNode &e=*_nodes[v];
      for( int i = 0; i < e._edges.Size(); i++ )
      {
        const PFEdge &ei = e._edges[i];
        int target = ei._target;
        if( target < 0 ) continue;
        int costToTarget = vCost + ei._cost;
        if( costToTarget < d[target] )
        {
          d[target] = costToTarget;
          p[target] = v;
          vs.HeapUpdateUp(target);
        }
      }
    }

    // finished: check if we have some path found
    if( p[toNode] < 0 )
    {
      return false;
    }

    path.Add(toNode);
    int curr = toNode;
    for(;;)
    {
      Assert( p[curr]>=0);
      curr = p[curr];
      path.Insert(0, curr);
      if( curr==fromNode )
      {
        break;
      }
    }

    if(_visualize)
    {
      Vector3* points = ALLOCA(Vector3, path.Size());
      for(int n = 0; n < path.Size(); n++)
      {
        points[n] = NodePosition(path[n]) + Vector3(0,0.25,0);
        _shapes2.Add(enf::Shape::CreateDiamond(points[n] - Vector3(0.25,0.25,0.25), points[n] + Vector3(0.25,0.25,0.25), 0xffff0000, 0));
      }

      _shapes2.Add(enf::Shape::CreateLines(points, path.Size(), 0xffff0000, 0));
    }

    return true;
  }
protected:
  TNodesArray _nodes;
  AutoArray<PFObstacle> _obstacles;
  SortedArray<Ref<Object> > _obstacleObjects;

  enf::AAB2 _bounds;
  int _baseNodes;
  bool _visualize;
  RefArray<enf::Shape> _shapes;
  RefArray<enf::Shape> _shapes2;
};

class PFQuery: public World::ObjectQuery
{
  PathFinder& _pf;
public:
  PFQuery(PathFinder& pf): _pf(pf)
  {
  }
  //!Give opportunity to filter out object based on cheap tests, before actual geometric test
  virtual bool Check(Object* obj) const
  {
    if(!obj->GetShape())
      return false;
    if(obj->GetShape()->FindGeometryLevel() < 0)
      return false;
    
    //add trees manually
    //if(!ent->Type()->IsKindOf("Building"))
      // return false;
    if(dynamic_cast<ObjectWindAnim*>(obj))
      return false;

    return true;
  }
  virtual bool operator () (Object *obj) const
  {
    _pf.AddObstacle(obj, ZombieRayTestRadius);
    return true;
  }
};

enf::AAB2 Box3Dto2D(const enf::AAB& bbox)
{
  return enf::AAB2(bbox.GetMins().x,bbox.GetMins().z,bbox.GetMaxs().x,bbox.GetMaxs().z);
}

const float DIST_EPSILON = 0.001f;

bool PFObstacle::Intersects(int from, int to, const TNodesArray& _nodes) const
{
  //check if they are actually edges of this obstacle
  for(int n = 0; n < 4; n++)
  {
    if(nodes[n] == from)
    {
      if(nodes[(n + 3) % 4] == to)
        return false;
      if(nodes[(n + 1) % 4] == to)
        return false;
      if(nodes[(n + 2) % 4] == to)
        return true;
      break;
    }
  }

  return Intersects(*_nodes[from], *_nodes[to], _nodes);
}

bool PFObstacle::Intersects(int from, const TNodesArray& _nodes) const
{
  //check if it's actually node of this obstacle
  for(int n = 0; n < 4; n++)
  {
    if(nodes[n] == from)
      return false;
  }

  return Intersects(*_nodes[from], _nodes);
}

bool PFObstacle::Intersects(const enf::Vector2& from, const enf::Vector2& to, const TNodesArray& _nodes) const
{
  if(!enf::IntersectionLineAABB(from, to, bounds.GetMins(), bounds.GetMaxs()))
    return false;

  for(int side = 0; side < 4; side++)
  {
    const enf::Vector2& e0 = *_nodes[nodes[side]];
    const enf::Vector2& e1 = *_nodes[nodes[(side + 1) % 4]];
    if(LineLineIntersection(e0, e1, from, to))
      return true;
  }
  return false;
}

bool PFObstacle::Intersects(const enf::Vector2& from, const TNodesArray& _nodes) const
{
  if(!bounds.Intersects(from))
    return false;

  for(int e = 0; e < 4; e++)
  {
    const enf::Vector2& e0 = *_nodes[nodes[e]];
    const enf::Vector2& e1 = *_nodes[nodes[(e + 1) % 4]];
    enf::Vector2 edge = e0 - e1;
    enf::Vector2 norm = enf::Vector2(edge.y, -edge.x);
    float d = norm.Dot(e0);
    if(norm.Dot(from) < d)
      return false;
  }
  return true;
}

void PathFinder::Build(const enf::AAB& bounds)
{
  Clear();

  _shapes.Clear();

  PFQuery query(*this);
  GWorld->QueryObjects(query, bounds.GetMins(), bounds.GetMaxs(), World::QF_STATIC);
  _bounds = Box3Dto2D(bounds);
}

void PathAroundObstacles(Vector3Par from, Vector3 to, Entity* pAgent)
{
  enf::AAB bbox;
  
  bbox.InitFromMovingSphere(from, to, 0);

  static PathFinder pf(true);
  if(!pf.IsValid(Box3Dto2D(bbox)))
  {
    pf.Clear();
    bbox.InitFromMovingSphere(from, to, 20);
    pf.Build(bbox);
    pf.GenerateGraph();
  }

  //LineLineIntersection(enf::Vector2(0,100), enf::Vector2(100,100), enf::Vector2(100,100), enf::Vector2(200,200));
  //LineLineIntersection(enf::Vector2(0,100), enf::Vector2(100,100), enf::Vector2(50,0), enf::Vector2(50,200));
  //LineLineIntersection(enf::Vector2(0,100), enf::Vector2(100,100), enf::Vector2(0,100), enf::Vector2(100,200));

  AutoArray<int> path;
  if(pf.FindPath(from, to, path))
  {
  }
}

void LightAI::FindPathAroundObstacles(AutoArray<TrailWaypoint>& _waypoints, Entity* pTarget)
{
/*
  AutoArray<TrailWaypoint> waypoints(_waypoints);
  waypoints.Insert(0, TrailWaypoint(FutureVisualState().Position(), Glob.time, true));

  if(solved.Size())
  {
    int size = waypoints.Size();
    if(size > 0)
    {
      Vector3 offset(0, 1, 0);
      Vector3* lines = ALLOCA(Vector3, size + 1);
      int lineSize = 0;

      _shapes.Clear();

      for(int w = 0; w < size; w++)
      {
        Vector3 pos = waypoints[w]._pos + offset;

        _shapes.Add(enf::Shape::CreateDiamond(pos - Vector3(0.3,0.3,0.3), pos - Vector3(0.3,0.3,0.3), 0x7fffffff, enf::SS_TRANSP|enf::SS_NOZWRITE));
        lines[lineSize++] = pos;
      }

      if(pTarget)
        lines[lineSize++] = pTarget->RenderVisualState().Position() + offset;

      _shapes.Add(enf::Shape::CreateLines(lines, lineSize, 0xffffffff, 0));
    }
    if(waypoints.Size())
    {
      _waypoints = waypoints;
      _waypoints.Delete(0);
    }
  }
  */
}

bool LightAI::InitPathToTarget(ChaseTarget* target, Vector3Par testOffset, float testRadius, float maxDistForPathfinder)
{
	Vector3 newPos = target->_target->FutureVisualState().Position();
  const EntityVisualState& vs = FutureVisualState();
   
  float dist2 = newPos.Distance2(vs.Position());

  CollisionBuffer col;
  GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(),col,FilterIgnoreTwoAndDead(this, target->_target), vs.Position() + testOffset, newPos + testOffset, testRadius,ObjIntersectGeom, ObjIntersectNone, CF_ALL_OBJECTS|CF_FIRST_CONTACT|CF_ONLY_STATIC);
  if (dynamic_cast<Zombie *>(this) && col.Size() > 0 && GetNetworkManager().IsServer())
  {
    static Time nextZombiePathBlockDiagnostic = TIME_MIN;
    if (Glob.time >= nextZombiePathBlockDiagnostic)
    {
      int logged = 0;
      for (int i = 0; i < col.Size() && logged < 4; i++)
      {
        Object *obj = col[i].object;
        if (!obj)
          continue;
        ZombieProbeLog("[ZombiePathBlock] zombie=%s object=%s t=%.3f mass=%.2f passable=%d static=%d hierarchy=%d dist=%.2f",
          cc_cast(Type()->GetName()), cc_cast(obj->GetDebugName()), col[i].under,
          obj->GetMass(), obj->IsPassable() ? 1 : 0, obj->Static() ? 1 : 0,
          col[i].hierLevel, sqrtf(dist2));
        logged++;
      }
      nextZombiePathBlockDiagnostic = Glob.time + 1.0f;
    }
  }
  if(col.Size())
  {
    //too much
    if(dist2 > Square(maxDistForPathfinder))
    {
      if (dynamic_cast<Zombie *>(this) && GetNetworkManager().IsServer())
      {
        static Time nextZombiePathOutcomeDiagnostic = TIME_MIN;
        if (Glob.time >= nextZombiePathOutcomeDiagnostic)
        {
          ZombieProbeLog("[ZombiePathResult] zombie=%s outcome=out_of_range dist=%.2f max=%.2f directHits=%d",
            cc_cast(Type()->GetName()), sqrtf(dist2), maxDistForPathfinder, col.Size());
          nextZombiePathOutcomeDiagnostic = Glob.time + 1.0f;
        }
      }
      return false;
    }

    //find path around
    PathFinder pf(false);

    enf::AAB bounds(true);
    bounds.Add(vs.Position());
    bounds.Add(newPos);
    bounds.Add(bounds.GetMins() - Vector3(15,1,15));
    bounds.Add(bounds.GetMaxs() + Vector3(15,2,15));

    pf.Build(bounds);
    //add trees
    for(int n = 0; n < col.Size(); n++)
    {
      if(col[n].hierLevel == 0)
      {
        if(dynamic_cast<ObjectWindAnim*>(col[n].object.GetRef()))
          pf.AddObstacle(col[n].object, testRadius);
      }
    }

    pf.GenerateGraph();
    AutoArray<int> path;
    if(!pf.FindPath(vs.Position(), newPos, path))
    {
      if (dynamic_cast<Zombie *>(this) && GetNetworkManager().IsServer())
      {
        static Time nextZombiePathOutcomeDiagnostic = TIME_MIN;
        if (Glob.time >= nextZombiePathOutcomeDiagnostic)
        {
          ZombieProbeLog("[ZombiePathResult] zombie=%s outcome=no_graph_route dist=%.2f directHits=%d",
            cc_cast(Type()->GetName()), sqrtf(dist2), col.Size());
          nextZombiePathOutcomeDiagnostic = Glob.time + 1.0f;
        }
      }
      return false;
    }
    target->_trail.Clear();
    for(int n = 0; n < path.Size(); n++)
      target->AddWaypoint(pf.NodePosition(path[n]));
    if (dynamic_cast<Zombie *>(this) && GetNetworkManager().IsServer())
    {
      static Time nextZombiePathOutcomeDiagnostic = TIME_MIN;
      if (Glob.time >= nextZombiePathOutcomeDiagnostic)
      {
        ZombieProbeLog("[ZombiePathResult] zombie=%s outcome=detour waypoints=%d dist=%.2f",
          cc_cast(Type()->GetName()), path.Size(), sqrtf(dist2));
        nextZombiePathOutcomeDiagnostic = Glob.time + 1.0f;
      }
    }
/*
    Vector3* points = ALLOCA(Vector3, path.Size());
    for(int n = 0; n < path.Size(); n++)
    {
      points[n] = pf.NodePosition(path[n]);
      enf::Shape::CreateDiamond(points[n] - Vector3(0.25,0.25,0.25), points[n] + Vector3(0.25,0.25,0.25), n == 0 ? 0xffff0000 : 0xff0000ff, 0);
    }

    enf::Shape::CreateLines(points, path.Size(), 0xffff0000, 0);
*/
	}
	return true;
}

Vector3 LightAI::Wander()
{
  const VisualState& vs = FutureVisualState();
  // randomize wander target
  _wanderTarget += Vector3((-1 + 2 * GRandGen.RandomValue()) * _wanderJitter, 0, (-1 + 2 * GRandGen.RandomValue()) * _wanderJitter);
  _wanderTarget.Normalize();
  _wanderTarget *= _wanderRadius;

  Vector3 targetLocal = _wanderTarget + Vector3(0, 0, _wanderDistance);
  Vector3 targetWorld = vs.PositionModelToWorld(targetLocal);

  return targetWorld - vs.Position();
}

Vector3 LightAI::WallAvoidance(float maxVelocity)
{
  const VisualState& vs = FutureVisualState();
  const int feelersCount = 3;
  const float feelersLength = 3 * maxVelocity;

  Vector3 testStartPos = vs.Position() + 0.5f*VUp;
  Vector3 feelers[feelersCount] =
  {
    testStartPos + vs.Direction() * feelersLength,
    testStartPos + vs.DirectionModelToWorld(Vector3( 0.7071, 0, 0.7071)) * feelersLength / 2,
    testStartPos + vs.DirectionModelToWorld(Vector3(-0.7071, 0, 0.7071)) * feelersLength / 2
  };

  Vector3 steeringForce = VZero;
  for( int i = 0; i < feelersCount; ++i )
  {
    CollisionBuffer col;
    GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(), col, Landscape::FilterIgnoreOne(this), testStartPos, feelers[i], 0.3f, ObjIntersectView, ObjIntersectNone, CF_NEAREST_CONTACT);
    if( col.Size() > 0 )
    {
      col[0].dirOutNotNorm.y = 0;
      steeringForce += col[0].dirOutNotNorm.Normalized() * (feelers[i] - col[0].pos).Size();
    }
  }

  return steeringForce;
}

void LightAI::AIPathPilot(SteerInfo& info, float deltaT)
{
  AIBrain *unit = Brain();
  if( !unit ) return;

  Object* walkOnObj = WalkingOnObject();
  info.building = dynamic_cast<Building*>(walkOnObj);

  const VisualState& vs = FutureVisualState();
  const FSM::State fsmState = _fsm->GetState();

  if( _walkTarget && (_walkTarget->_trail.Size() || _walkTarget->_pathBlocked) && CanWalk() )
  {
    Vector3 position = vs.Position();
    Vector3 dir = vs.Direction();

    //try to shorten the path
    if(/*false &&*/ _walkTarget->_trail.Size() > 1 && Glob.time > _nextPathOptimization )
    {
      //do it only once a while, based on distance
      Vector3 finalPos = _walkTarget->_trail[_walkTarget->_trail.Size() - 1]._pos;
      float pathLength = finalPos.DistanceXZ(position);
      float delay = InterpolativC(pathLength, 5.0f, 70.0f, 0.5f, 7.0f);
      _nextPathOptimization = Glob.time + delay * (GRandGen.RandomValue() * 0.1f + 0.95f);


      for( int n = _walkTarget->_trail.Size() - 1; n > 0; n-- )
      {
        //Vector3 dirToWp = (_walkTarget->_trail[n]._pos - position).Normalized();
        //don't shorten to waypoints behind us, as we cannot see them
        //if(dirToWp.DotProduct(dir) < 0)
        //  continue;

        CollisionBuffer col;
        GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(), col, Landscape::FilterIgnoreTwo(this, _walkTarget->_target), position + ZombieRayTestOffset, _walkTarget->_trail[n]._pos + ZombieRayTestOffset, ZombieRayTestRadius, ObjIntersectGeom, ObjIntersectNone, CF_FIRST_CONTACT);

        if( !col.Size() )
        {
          /// for doublecheck ;)          GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(),col,Landscape::FilterIgnoreTwo(this, _walkTarget->_target), position + ZombieRayTestOffset, _walkTarget->_trail[n]._pos + ZombieRayTestOffset, ZombieRayTestRadius, ObjIntersectGeom);
          _walkTarget->_trail.Delete(0, n);
          break;
        }
      }
    }

    if (_walkTarget->_target && Glob.time >= _nextObstacleCheckTime)
    {
      _nextObstacleCheckTime = Glob.time + 1.0f;
      const Vector3 targetPos = _walkTarget->_target->FutureVisualState().Position();
      const float targetDist2 = targetPos.Distance2(position);
      const bool pathFound = InitPathToTarget(_walkTarget, ZombieRayTestOffset, ZombieRayTestRadius, 100);

      // A failed local route must not leave an old breadcrumb trail that can
      // point through blocked geometry. Stop and retry after one second.
      // Keep long-range breadcrumb following until the target enters the
      // local pathfinder's 100 m working radius.
      if (!pathFound && targetDist2 <= Square(100.0f))
      {
        _walkTarget->_pathBlocked = true;
        _walkTarget->_trail.Clear();

        static Time nextZombieBlockedRouteDiagnostic = TIME_MIN;
        if (dynamic_cast<Zombie *>(this) && GetNetworkManager().IsServer() && Glob.time >= nextZombieBlockedRouteDiagnostic)
        {
          ZombieProbeLog("[ZombieRouteBlocked] zombie=%s target=%s dist=%.2f",
            cc_cast(Type()->GetName()), cc_cast(_walkTarget->_target->Type()->GetName()), sqrtf(targetDist2));
          nextZombieBlockedRouteDiagnostic = Glob.time + 1.0f;
        }
      }
      else
      {
        _walkTarget->_pathBlocked = false;
      }

      if (_walkTarget->_pathBlocked)
      {
        info.speedWanted = 0;
        info.sideSpeedWanted = 0;
        info.headChange = 0;
        info.turnPredict = 0;
        _walkTarget = NULL;
        return;
      }
    }

    if (_walkTarget->_pathBlocked)
    {
      info.speedWanted = 0;
      info.sideSpeedWanted = 0;
      info.headChange = 0;
      info.turnPredict = 0;
      _walkTarget = NULL;
      return;
    }
    if (!_walkTarget->_trail.Size())
    {
      info.speedWanted = 0;
      info.sideSpeedWanted = 0;
      _walkTarget = NULL;
      return;
    }

    Vector3 walkPos = _walkTarget->_trail[0]._pos;

    float distTreshold = 0.35f;

    if( _walkTarget->_target && _walkTarget->_trail.Size() <= 1 )
    {
      const VisualState& vs = _walkTarget->_target->FutureVisualState();
      //when the final target is object, place the position before and ahead
      walkPos = vs.Position();

      if( !info.building )
        walkPos += vs.Speed() * 0.5f;

      distTreshold = 1.4f;
    }

    Vector3 walkDir;
    Vector3 tdir;

    while( true )
    {
      walkDir = walkPos - position;
      walkDir[1] = 0.0f;

      tdir = walkDir.Normalized();

      if( _walkTarget->_trail.Size() > 1 )
      {
        //near to waypoint or already missed one
        if( tdir.DotProduct(dir) < 0 || (InBuildingOrColliding(this) ? walkDir.SizeXZ() < 0.125f : walkDir.SizeXZ() < 0.25f) )
        {
          _walkTarget->_trail.Delete(0);
          walkPos = _walkTarget->_trail[0]._pos;
          continue;
        }
      }
      break;
    }

    float distToTarget = position.DistanceXZ(_finalTarget);

    //don't avoid if we are following trail or are too close!
    if( _walkTarget->_trail.Size() <= 1 && _avoidAside && !info.building && distToTarget > 2.0f )
    {
      Vector3 asideOffset = FutureVisualState().DirectionAside() * _avoidAside;
      walkPos += asideOffset;
      walkDir = walkPos - position;
      walkDir[1] = 0.0f;
    }

    _finalTarget = walkPos;

    /*
    CollisionBuffer col;
    GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(),col,Landscape::FilterIgnoreOne(this),position + ZombieRayTestOffset, walkPos + ZombieRayTestOffset,ZombieRayTestRadius,ObjIntersectGeom, ObjIntersectNone, CF_NEAREST_CONTACT|CF_ONLY_STATIC);
    if(col.Size())
    {
    int level = col[0].HitObject()->GetShape()->FindGeometryLevel();
    const Vector3* minMax = col[0].HitObject()->GetShape()->GeometryLevel()->MinMax();
    enf::Shape::CreateDiamond(col[0].pos - Vector3(0.5,0.5,0.5),col[0].pos + Vector3(0.5,0.5,0.5),0x7f7f0000, enf::SS_ONCE|enf::SS_NOZWRITE);
    enf::Shape* sh = enf::Shape::CreateBox(minMax[0], minMax[1], 0x4000ff00, enf::SS_ONCE|enf::SS_TRANSP|enf::SS_NOZWRITE);
    if(sh) sh->SetMatrix(col[0].HitObject()->FutureVisualState().Transform());
    }
    */

    float heading = atan2(dir.X(), dir.Z());
    float walkHeading = atan2(walkDir.X(), walkDir.Z());
    float turn = AngleDifference(heading, walkHeading);
    info.headChange = -turn;
    info.turnPredict = -turn;

    //    GScene->DrawDiagSphere(_finalTarget, 0.6f, ColorP(1,0,1));
    _walkTarget = NULL;

    //test
    if( distToTarget >= distTreshold )
    {
      if( _fsm->GetState() == AISChasing )
      {
        if( distToTarget > 3 && _tiredFactorHysteresis < _tiredFactorNoSprint )
        {
          _tiredFactor += deltaT * 1.5f;
          info.speedWanted = 6;
        }
        else
        {
          _tiredFactor -= deltaT * 0.5f;
          info.speedWanted = 3;
        }
        //         info.speedWanted = 2.35;
      }
      else if( _fsm->GetState() == AISMelee )
        info.speedWanted = 3;
      else if( _fsm->GetState() == AISIdle )
      {
        //just walk in idle
        //if(position.DistanceXZ(_finalTarget) > 1.5)
        //info.speedWanted = 3.0;
        //else
        info.speedWanted = 2.35;
      }
    }
    else
      info.speedWanted = 0;
    //info.speedWanted = 0;
  }
  else if( fsmState == AISWandering || fsmState == AISRunaway )
  {
    Vector3 actDir = vs.Direction();
    Vector3 actPos = vs.Position();
    Vector3 actVelocity = vs.Speed();

    Vector3 steeringForce = VZero;
    float steeringForceTweaker = 100;
    float maxSteeringForce = 1.0f;

    float wantedVelocity = 0;

    float heading = atan2(actVelocity.X(), actVelocity.Z());

    if( fsmState == AISWandering )
    {
      wantedVelocity = 2.5f;
      steeringForce += _wanderingSteeringForce;
    }
    else if( fsmState == AISRunaway )
    {
      wantedVelocity = Type()->GetMaxSpeedMs();

      if( _fleeTarget && _fleeTarget->_target )
      {
        Vector3 targetPos = _fleeTarget->_target->FutureVisualState().Position();
        Vector3 desiredDir = actPos - targetPos; desiredDir.y = 0;
        Vector3 desiredVelocity = desiredDir.Normalized() * wantedVelocity;
        steeringForce += desiredVelocity - actVelocity;
      }
    }

    if( (fsmState == AISWandering || fsmState == AISRunaway) )
    {
      steeringForce += 10 * WallAvoidance(wantedVelocity);
    }

    if( wantedVelocity > 0 )
    {
      if( steeringForce.Size() > maxSteeringForce )
        steeringForce = steeringForce.Normalized() * maxSteeringForce;
      steeringForce = steeringForceTweaker * steeringForce;

      Vector3 newVelocity = actVelocity + steeringForce * deltaT;
      if( newVelocity.Size() > wantedVelocity )
        newVelocity = newVelocity.Normalized() * wantedVelocity;

      float predictedHeading = atan2(newVelocity.X(), newVelocity.Z());
      float turn = AngleDifference(heading, predictedHeading);
      info.headChange = -turn;
      info.turnPredict = -turn;
      info.speedWanted = wantedVelocity;
      info.slowDownWhenTurning = false;
    }
  }
}

void LightAI::AIPilot(float deltaT, SimulationImportance prec)
{
  //PROFILE_SCOPE_EX(manAI,sim);
  _holdBreath = 0; // AI is currently never holding breath
  _leanZRotWanted = 0;
  _headXRotWantedCont = _headYRotWantedCont = _headXRotWantedPan = _headYRotWantedPan = 0;
  _irWanted = _irEnabled;
  _flir = false;

  if (IsSuspendedByActivity())
  {
    // In get in action soldier must not move or turn, just play animation
    _turnWanted = 0;
    return;
  }

  SteerInfo info;
  info.headChange=0;
  info.speedWanted=0;
  info.turnPredict=0;
  info.slowDownWhenTurning = true;

  AIBrain *unit = Brain();
  if (!unit) return;

  // periodically change between looking forward and on target
  if (_lookTargetTimeLeft>0)
  {
    _lookTargetTimeLeft -= deltaT;
  }
  else
  {
    _lookForwardTimeLeft -= deltaT;
    if (_lookForwardTimeLeft<=0)
    {
      if (unit->GetCombatModeLowLevel()<=CMSafe || !(_itemInHands && _itemInHands->GetWeapon()))
      {
        if (fabs(_walkSpeedWantedZ)<0.1f)
        {
          // safe and static - almost no need to look forward
          _lookTargetTimeLeft = 1+GRandGen.RandomValue()*4;
          _lookForwardTimeLeft = 2+GRandGen.RandomValue()*3;
        }
        else
        {
          // safe, but walking - look forward often
          _lookTargetTimeLeft = 4+GRandGen.RandomValue()*8;
          _lookForwardTimeLeft = 3+GRandGen.RandomValue()*5;
        }
      }
      else if (fabs(_walkSpeedWantedZ)<0.1f)
      { 
        // combat, but static - look around often to keep situational awareness
        _lookTargetTimeLeft = 1.5f+GRandGen.RandomValue()*2;
        _lookForwardTimeLeft = 2.0f+GRandGen.RandomValue()*4;
      }
      else
      { 
        // combat and moving - look mostly forward, but watch the target often, but briefly
        _lookTargetTimeLeft = 0.5f+GRandGen.RandomValue()*0.5f;
        _lookForwardTimeLeft = 3.0f+GRandGen.RandomValue()*3.0f;
      }
    }
  }

  // follow watch direction
  Vector3Val dir = unit->GetWatchHeadDirection();

  if (dir.SquareSize()>0.1f)
  {
    // check if we have any
    AimHeadAI(dir,NULL,deltaT);
  }
  else
  {
    _headXRotWanted = _headYRotWanted = 0;
    _lookXRotWanted = _lookYRotWanted = 0;
  }


  // TODO: limit AIPilot simulation rate (10 ps)

  bool inPosition = true;
  
  if( unit->GetState()==AIBrain::Stopping )
  {
    unit->OnStepCompleted();
    // unit can be destroyed in unit->SendAnswer
    // unit = Brain();
    info.speedWanted=0;
    return;
  }
  else if( unit->GetState()==AIBrain::Stopped )
  {
    info.speedWanted=0;
  }
  else
  {
    _fireState=FireDone;

    if(unit->GetLifeState() != LifeStateUnconscious)
    {
      AIPathPilot(info, deltaT);
    }
    else
      _attackAction = 0;
    // leader is always assumed to be in position
    inPosition = true;
  }

  if ((!_landContact || _freeFall) && _freeFallStartTime<Glob.time-0.6f && !_swimming)
  {
    info.speedWanted = 0;
    info.headChange = 0;
  }
  if (info.action)
  {
    switch (info.action->GetType())
    {
    case PATUserActionBegin:
    case PATUserActionEnd:
      {
        EntityAI *building = dyn_cast<EntityAI, Object>(info.building);
        RString actionName = info.action->GetActionName();
        if (building && actionName.GetLength() > 0)
        {
          building->ProcessUserAction(actionName);
          if (!building->DoneUserAction(actionName))
          {
            // wait until action is processed
            info.headChange = 0;
            info.speedWanted = 0;
            info.turnPredict = 0;
          }
        }
      }
      break;
    }
  }

  //if (goBack) speedWanted=-speedWanted;
  //AvoidCollision(deltaT,info.speedWanted,info.headChange);

  if( info.slowDownWhenTurning )
  {
    // slow down when turning
    float maxSpeed=Type()->GetMaxSpeedMs();
    float limitSpeed=InterpolativC(fabs(info.headChange),0,H_PI*0.5f,maxSpeed,0);
    saturate(info.speedWanted,-limitSpeed,+limitSpeed);

    // when in buildings, slow down even more soon enough
    if (info.building)
    {
      float limitSpeed=InterpolativC(fabs(info.turnPredict), 0, H_PI*0.15f, maxSpeed, 0.1f*maxSpeed);
      saturate(info.speedWanted,-limitSpeed,+limitSpeed);
//      saturate(info.speedWanted,-2.35,+2.35);
      saturate(info.speedWanted,-4,+4);

      if (CHECK_DIAG(DEPath) && this==GWorld->CameraOn())
        DIAG_MESSAGE(500, Format("Speed in building limited by %.2f to %.2f", limitSpeed, info.speedWanted));
    }
  }

  // make appropriate reaction based on speedWanted, headChange and wantedPosition

  const ManType *type = Type();
  _turnWanted = info.headChange;
  if( info.slowDownWhenTurning )
    _turnWanted = info.headChange * 3;
  float maxTurnAngVelocity = type->GetMaxTurnAngularVelocity(); 
  saturate(_turnWanted,-maxTurnAngVelocity,+maxTurnAngVelocity);

  // if movement is necessary, stand up

  ManPos minPos = ManPosLying; // default is: we can be in any position
  ManPos maxPos = ManPosStand;

  // convert from minPos,maxPos to old scheme
  CombatMode unitMode = unit->GetCombatModeLowLevel();
  // when player is a group leader, follow his lead
  CombatMode autoCM = CMSafe;
  
  // select the stricter mode (more combat, meaning more prone)
  CombatMode stanceMode = CombatMode(autoCM>unitMode ? autoCM : unitMode);
  
  float delay = 0.5f; 

  if (info.clearance<ClearanceNormal && Type()->_canCrawl)
  {
    // we need to go prone - low clearance ahead
    maxPos = SATURATE_POS(ManPosLying);
    delay = 0;
  }
  // FIX weapon activation before _unitPos check
  else if (unit->GetPlanningMode() == AIBrain::LeaderDirect
#if _VBS3  // disableAI PATHPLAN
    && (unit->GetAIDisabled() & AIBrain::DAPathPlan) == 0
#endif
  )
  {
    minPos = ManPosCombat; // disable lying - force run
  }
  else
  {
    minPos = ManPosCombat;
    
    if( stanceMode == CMAware )
    {
      minPos = maxPos = SATURATE_POS(ManPosCombat);
    }
    else if( stanceMode <= CMSafe )
    {
      minPos = maxPos = SATURATE_POS(ManPosStand);
    }
    else if( stanceMode == CMCombat )
    {
      // check speed wanted
      float limitSpeed = type->_lyingLimitSpeedCombat;
      // check if our position corresponds to the position we should have in the formation
      // if not, never lie down
      if (info.speedWanted<limitSpeed && !_inBuilding)
      {
        // if we are not lying yet, consider crouching
        if (!IsDown() && (!inPosition || unit->RandomDecision(30,type->_crouchProbabilityCombat)))
        {
          maxPos = SATURATE_POS(ManPosCrouch);
        }
        else
        {
          maxPos = SATURATE_POS(ManPosLying);
        }
      }
      else
      {
        minPos = maxPos = SATURATE_POS(ManPosCombat);
      }
    }
    else if( stanceMode == CMStealth )
    {
      // if there is no enemy known or expected, do not lay down
      float exposure = 0;
      minPos = maxPos = SATURATE_POS((exposure<400 && info.speedWanted>type->_lyingLimitSpeedStealth) ? ManPosCombat : ManPosLying);
    }
  }

  // prepare for new animation
  // check which action map is used now
  ActionMap *map = Type()->GetActionMap(_moves.GetPrimaryMove(FutureVisualState()._moves).id, AnimSubIx());

  ManAction selAction = ManActN;

  int upDegree = map ? map->GetUpDegree() : ManPosStand;
  if (_posWanted<minPos || _posWantedTime>Glob.time+30)
  {
    _posWanted = minPos;
    _posWantedTime = Glob.time+(GetInvAbility(AKSpotTime)*delay+delay*0.5f)*GRandGen.RandomValue();
  }
  else if( _posWanted>maxPos )
  {
    _posWanted = maxPos;
    _posWantedTime = Glob.time+(GetInvAbility(AKSpotTime)*delay+delay*0.5f)*GRandGen.RandomValue();
  }

  if (!_itemInHands)
  {
    if (_posWanted==ManPosCombat || _posWanted==ManPosStand)
    {
      SetPosWanted(ManPosNoWeapon);
    }
    else if (_posWanted==ManPosLying)
    {
      SetPosWanted(ManPosLyingNoWeapon);
    }
  }

  //LogF("_posWanted %d",_posWanted);

  if (_posWanted==ManPosWeapon && _posWantedTime<=Glob.time && GetActUpDegree(FutureVisualState())!=ManPosWeapon)
  {
    selAction = ManActWeaponOn;
    //LogF("%s: action weapon",(const char *)GetDebugName());
  }
  else if (upDegree!=_posWanted && _posWantedTime<=Glob.time)
  {
    //_posWantedTime = TIME_MAX; // cancel request
    if (upDegree==ManPosWeapon)
    {
      if (_posWanted!=ManPosWeapon)
      {
        selAction = ManActWeaponOff;
      }
      else
      {
        selAction = ManActWeaponOn;
      }
    }
    else if (_posWanted<ManPosNormalMin)
    {
    }
    else
    {
      if (_posWanted == ManPosLying || _posWanted==ManPosHandGunLying)
      {
        if (upDegree != ManPosHandGunLying)
        {
          selAction = ManActLying;
        }
      }
      else if (_posWanted == ManPosCombat || _posWanted==ManPosHandGunStand)
      {
        // note: some upDegrees may be valid
        if (upDegree != ManPosWeapon && upDegree != ManPosHandGunStand)
        {
          selAction = ManActCombat;
        }
      }
      else if( _posWanted==ManPosStand ) 
      {
        if (upDegree != ManPosHandGunStand)
        {
          selAction = ManActStand;
        }
      }
      else if( _posWanted==ManPosNoWeapon ) selAction = ManActCivil;
      else if( _posWanted==ManPosLyingNoWeapon ) selAction = ManActCivilLying;
      else if( _posWanted==ManPosDead ) {/*will be handled elsewhere*/}
      else
      {
        LogF("Transition ManPos %d to %d not handled",upDegree,_posWanted);
      }
    }
  }
  
  if (selAction==ManActN)
  {
    selAction=ManActStop;
    // normal movement - check speedWanted (maybe also speedAside?)
    float limitFast = map ? map->GetLimitFast() : 5;
    float limitSlow = limitFast*0.6f;

    // water limits
    if (_waterDepth>0.6)
    {
      if (_waterDepth>1.0)
      {
        // in deep water only walking is allowed
        saturateMin(info.speedWanted,1.5f);
      }
      else
      {
        // in shallow water only slow running is allowed
        saturateMin(info.speedWanted,2.5f);
      }
    }

    // under certain speed we rather want to "slide" than to switch animations forth and back
    // switching would be likely, as small speeds are usually result of micro manoeuvring
    float limitAnim = floatMin(0.1f,limitFast*0.2f);
    
    // when fine control is needed and target is near (desired speed is low), always use walking
    if (_walkSpeedFineControl && Square(info.speedWanted)+Square(info.sideSpeedWanted)<Square(1.5f)) limitFast=limitSlow = 100;
    
    //int fast = 2;
    // selAction = ENUM_CAST(ManAction,SelectManAction[fastFlag][fbFlag+1][rlFlag+1]);
    if (!Type()->IsAnimal())
    {
      if (0)//_posWanted == ManPosNoWeapon)
      {
        if (map)
        { // check fast/slow/walk actions
          MoveId moveS = Type()->GetMove(map, ManActSlowF); 
          MoveId moveW = Type()->GetMove(map, ManActWalkF); 
          if (moveS!=moveW) // do not disable sprint when there is no run (run==walk - woman, see http://dev-heaven.net/issues/26444)
          {
            limitFast = 100; /* TODO: implement a better fix, sprint/run walkcycle do not match: http://dev-heaven.net/issues/26280*/
          }
        }
      }
    }
    if (info.clearancePredict<ClearanceNormal) limitFast = 100, limitSlow = 100; // slow down before entering a hole to be prone in time

    if( info.speedWanted>limitFast ) selAction = ManActFastF;
    else if( info.speedWanted>limitSlow ) selAction = ManActSlowF;
    else if( info.speedWanted>+limitAnim ) selAction = ManActWalkF;
    else if( info.speedWanted<-limitAnim ) selAction = ManActWalkB;
    else if( info.speedWanted<-limitFast ) selAction = ManActFastB;
    else if( info.speedWanted<-limitSlow ) selAction = ManActSlowB;
    // sometimes make strafing movement
    if (info.sideSpeedWanted<=-limitAnim)
    {
      if (info.sideSpeedWanted<-limitFast)
      {
        if (selAction==ManActStop) selAction = ManActSlowL;
        else if (selAction==ManActWalkF) selAction = ManActWalkLF;
        else if (selAction==ManActWalkB) selAction = ManActWalkLB;
        else if (selAction==ManActSlowF) selAction = ManActSlowLF;
        else if (selAction==ManActSlowB) selAction = ManActSlowLB;
        else if (selAction==ManActFastF) selAction = ManActFastLF;
        else if (selAction==ManActFastB) selAction = ManActFastLB;
      }
      else
      {
        if (selAction==ManActStop) selAction = ManActWalkL;
        else if (selAction==ManActWalkF) selAction = ManActWalkLF;
        else if (selAction==ManActWalkB) selAction = ManActWalkLB;
        else if (selAction==ManActSlowF) selAction = ManActSlowLF;
        else if (selAction==ManActSlowB) selAction = ManActSlowLB;
        else if (selAction==ManActFastF) selAction = ManActSlowLF;
        else if (selAction==ManActFastB) selAction = ManActSlowLB;
      }

    }
    if (info.sideSpeedWanted>=+limitAnim)
    {
      if (info.sideSpeedWanted>limitFast)
      {
        if (selAction==ManActStop) selAction = ManActSlowR;
        else if (selAction==ManActWalkF) selAction = ManActWalkRF;
        else if (selAction==ManActWalkB) selAction = ManActWalkRB;
        else if (selAction==ManActSlowF) selAction = ManActSlowRF;
        else if (selAction==ManActSlowB) selAction = ManActSlowRB;
        else if (selAction==ManActFastF) selAction = ManActFastRF;
        else if (selAction==ManActFastB) selAction = ManActFastRB;
      }
      else
      {
        if (selAction==ManActStop) selAction = ManActWalkR;
        else if (selAction==ManActWalkF) selAction = ManActWalkRF;
        else if (selAction==ManActWalkB) selAction = ManActWalkRB;
        else if (selAction==ManActSlowF) selAction = ManActSlowRF;
        else if (selAction==ManActSlowB) selAction = ManActSlowRB;
        else if (selAction==ManActFastF) selAction = ManActSlowRF;
        else if (selAction==ManActFastB) selAction = ManActSlowRB;
      }
    }
  }

  if (selAction==ManActStop)
  {
    if( fabs(_turnWanted)>0.5f)
    {
      selAction = _turnWanted<0 ? ManActTurnL : ManActTurnR;
      if (unit->GetCombatModeLowLevel()<CMCombat)
      {
        // first choice - relaxed
        ManAction relAction = _turnWanted<0 ? ManActTurnLRelaxed : ManActTurnRRelaxed;
        if (Type()->GetMove(map, relAction).id!=MoveIdNone)
        {
          selAction = relAction;
        }
      }
      //no animation. Just walk!
      if (Type()->GetMove(map, selAction).id==MoveIdNone)
        selAction=ManActWalkF;
    }
    // never use Relaxed animations once you have some target
    // relaxed animations can prevent aligning the target properly
    else if (unit->GetCombatModeLowLevel()<CMCombat)
    {
      if (Type()->GetMove(map, ManActStopRelaxed).id!=MoveIdNone)
      {
        selAction = ManActStopRelaxed;
      }
    }
  }
  
  
  // get which action is it (based on action map)
  MoveId moveWanted = map ? Type()->GetMove(map, selAction) : GetDefaultMove(); 
  // LogF("%s: moveWanted %s (action %s)",DNAME(),NAME_T(Type(), moveWanted), (const char *)ManActionNames[selAction].name);

  if (!IsLocal() && _targetMoveWanted!=MoveIdNone)
  {
    ActionMap *targetMap = Type()->GetActionMap(_targetMoveWanted, AnimSubIx());
    if (map!=targetMap && Type()->GetMove(map, ManActStop)!=Type()->GetMove(targetMap, ManActStop))
    {
      moveWanted = _targetMoveWanted;
    }
  }

  if (!SimulateForcedMove(prec))
  {
    if(_attackAction)
    {
      //end animation is not reliable
      
      if(_moves.GetActionInProgress(FutureVisualState()._moves) == NULL && (_gesture.GetActionInProgress(FutureVisualState()._gestures) == NULL && FutureVisualState()._gestureFactor == 0))
        _attackAction = 0;
    }

    SetTargetMove(moveWanted);

    MotionPathItem motionPathItem;
    if( _moves.GetForceMove().id!=MoveIdNone )
    {
      motionPathItem = _moves.GetForceMove();
    }
    else
    {
      motionPathItem = MotionPathItem(moveWanted);
    }
    SetMoveQueue(motionPathItem,prec<=SimulateVisibleFar);    
  }

  float limitFast = map ? map->GetLimitFast() : 5;
  _walkSpeedWantedZ=info.speedWanted;
  _walkSpeedWantedX=info.sideSpeedWanted;
  // avoid moving faster than given movement allows to move
  saturate(_walkSpeedWantedZ,-limitFast,+limitFast);
  saturate(_walkSpeedWantedX,-limitFast,+limitFast);
  _walkSpeedFineControl = !info.allowFreeTurn;

  #if _ENABLE_REPORT
    if (CHECK_DIAG(DEPath))
    {
      if (this==GWorld->CameraOn())
      {
        DIAG_MESSAGE(500,Format(
          "%s: speed Z/X %.1f/%.1f, %s",
          cc_cast(GetDebugName()),
          _walkSpeedWantedZ,_walkSpeedWantedX,_walkSpeedFineControl ? "fine" : "fast"
        ));
      }
    }
  #endif
}

void LightAI::FakePilot( float deltaT, SimulationImportance prec )
{
  _turnWanted = 0;
  _walkSpeedFineControl = false;
  _holdBreath = 0;

  if (!SimulateForcedMove(prec))
  {
    if (_targetMoveWanted!=MoveIdNone)
    {
      //const ActionMap *map = GetMovesType()->GetActionMap(_moves.GetPrimaryMove(FutureVisualState()._moves).id, AnimSubIx());
      //if (map && map->GetName())
      MoveId oldQueue = _moves.GetDesiredMove();
      if (!ChangeMoveQueueSetStillEnd(MotionPathItem(_targetMoveWanted)) && oldQueue!=MoveIdNone)
      {
        // change move may fail when dying from variant animations
        // different variant animation may be chosen on remote and local versions
        // note: remote units select dying animation in the remote simulation
        const MoveInfoMan *info = GetMovesType()->GetMoveInfo(_targetMoveWanted, AnimSubIx());
        if (info && info->GetTerminal())
        { // the move received is a terminal animation
          MoveId primary = _moves.GetPrimaryMove(FutureVisualState()._moves).id;
          const MoveInfoBase *primInfo = GetMovesType()->GetMoveInfoBase(primary, AnimSubIx());
          if (!primInfo || !primInfo->GetTerminal())
          { // the current move is not a terminal animation
            ChangeMoveQueueSetStillEnd(MotionPathItem(oldQueue)); // restore the animation selected by us
          }
        }
      }
    }
  }
}

void LightAI::DrawDiags()
{
  if(_actualTarget)
  {
    int size = _actualTarget->_trail.Size();
    if(size > 0)
    {
      Vector3 offset(0, 1, 0);
      Vector3* lines = ALLOCA(Vector3, size + 2);
      int lineSize = 0;
      lines[lineSize++] = RenderVisualState().Position() + offset;
      for(int w = 0; w < size; w++)
      {
        Vector3 pos = _actualTarget->_trail[w]._pos + offset;

        enf::Shape::CreateDiamond(pos - Vector3(0.3,0.3,0.3), pos + Vector3(0.3,0.3,0.3), _actualTarget->_trail[w]._safe ? 0xff00ff00 : 0xffff0000, enf::SS_ONCE|enf::SS_TRANSP|enf::SS_NOZWRITE);
        lines[lineSize++] = pos;
      }
      
      if(_actualTarget->_target)
        lines[lineSize++] = _actualTarget->_target->RenderVisualState().Position() + offset;

      enf::Shape::CreateLines(lines, lineSize, 0xffffffff, enf::SS_ONCE);
    }
  }
  base::DrawDiags();
}

float LightAI::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = 0;
  switch( ctx.GetClass() )
  {
  case NMCUpdateGeneric:
    PREPARE_TRANSFER(UpdateVehicle)
    ICALCERR_NEQ(bool, simulationDisabled, ERR_COEF_VALUE_MAJOR);
    break;
  }

  if( IsSimulationDisabled() && error == 0 )
    return 0;

  return base::CalculateError(ctx);
}

// ===================== Animal ======================

Animal::Animal(const EntityAIType *name, bool fullCreate): LightAI(name, fullCreate)
{
  static const LightAIFSM::StateInfo states[] = {
    LightAIFSM::StateInfo("none", Dummy, Dummy),
    LightAIFSM::StateInfo("idle", LightAI::IdleStateInit, LightAI::IdleStateCheck),
    LightAIFSM::StateInfo("wandering", LightAI::WanderingStateInit, LightAI::WanderingStateCheck),
    LightAIFSM::StateInfo("feeding", LightAI::FeedingStateInit, LightAI::FeedingStateCheck),
    LightAIFSM::StateInfo("chasing", LightAI::ChasingStateInit, LightAI::ChasingStateCheck),
    LightAIFSM::StateInfo("melee", LightAI::MeleeStateInit, LightAI::MeleeStateCheck),
    LightAIFSM::StateInfo("runaway", LightAI::RunawayStateInit, LightAI::RunawayStateCheck)
  };
  _fsm = new LightAIFSM(states, lenof(states), NULL, 0);
  _fsm->SetState(AISNone, this);
  SetTargetCategory("animal");

  _tiredFactorMax = GRandGen.RandomValue() * 7 + 40;
  _tiredFactorNoSprint = GRandGen.RandomValue() * 5 + 30;
}

Animal::~Animal()
{
}

bool Animal::FleeStateCheck()
{
  ChaseTarget* bestTarg = FindThreatTarget();
  if( !bestTarg )
  {
    if( !_fleeTarget )
      return false;

    if( _fleeTarget && _fleeTarget->_target )
    {
      Vector3 targetPos = _fleeTarget->_target->FutureVisualState().Position();
      Vector3 myPos = FutureVisualState().Position();
      float dist2 = targetPos.Distance2(myPos);
      if( dist2 > 40 * 40 )
      {
        _fleeTarget = NULL;
        return false;
      }

      return true;
    }
  }

  _fleeTarget = bestTarg;
  return true;
}

void Animal::IdleStateInit()
{
  _nextWanderTime = Glob.time + GRandGen.RandomValue() * 8.0f + 2.0f;
  SetActualTarget(NULL);
}

void Animal::IdleStateCheck()
{
  if( FleeStateCheck() )
    _fsm->SetState(AISRunaway, this);

  if( Glob.time > _nextWanderTime )
    _fsm->SetState(AISWandering, this);
}

void Animal::WanderingStateInit()
{
  _nextIdleTime = Glob.time + GRandGen.RandomValue() * 20.0f + 5.0f;
  SetActualTarget(NULL);

  const VisualState& vs = FutureVisualState();
  // todo: get from config
  _wanderTarget = vs.Direction();
  _wanderRadius = 1.2f;
  _wanderDistance = 2.0f;
  _wanderJitter = 80.0f;

  _wanderingSteeringForce = Wander();
  _lastWanderingDirChangeTime = Glob.time;
}

void Animal::WanderingStateCheck()
{
  if( FleeStateCheck() )
    _fsm->SetState(AISRunaway, this);

  if( Glob.time > _nextIdleTime )
  {
    if( GRandGen.RandomValue() < 0.5f )
      _fsm->SetState(AISIdle, this);
    else
      _fsm->SetState(AISFeeding, this);

    return;
  }
  if( Glob.time - _lastWanderingDirChangeTime > 1.0f )
  {
    _wanderingSteeringForce = Wander();
    _lastWanderingDirChangeTime = Glob.time;
  }
}

void Animal::FeedingStateInit()
{
  _nextWanderTime = Glob.time + GRandGen.RandomValue() * 8.0f + 2.0f;
  const MoveIdExt &moveWanted = GetMove("FeedIn");
  if( moveWanted.id != MoveIdNone )
  {
    PlayForcedMove(moveWanted, new LightAIActionContext(this, moveWanted));
  }
  else
  {
    _fsm->SetState(AISIdle, this);
  }

  return;
}

void Animal::FeedingStateCheck()
{
  if( FleeStateCheck() )
    _fsm->SetState(AISRunaway, this);

  if( Glob.time > _nextWanderTime )
    _fsm->SetState(AISWandering, this);
}

void Animal::RunawayStateInit()
{
}

void Animal::RunawayStateCheck()
{
  if( !FleeStateCheck() )
    _fsm->SetState(AISIdle, this);
}

ChaseTarget* Animal::FindThreatTarget()
{
#ifndef _BINARIZE
  const TargetNormal* bestTarg = NULL;
  float bestDist = FLT_MAX;
  Vector3 actPos = FutureVisualState().Position();
  AIAgent* agent = static_cast<AIAgent*>(Brain());

  for( int n = 0; n < agent->GetTargetList().EnemyCount(); n++ )
  {
    const TargetNormal* targ = agent->GetTargetList().GetEnemy(n);
    const EntityAI* entityAI = targ->idExact;
    const Man* manAI = dynamic_cast<const Man*>(entityAI);
    if( manAI == NULL ) //filter out buildings
      continue;

    if( dynamic_cast<const Animal*>(manAI) != NULL )
      continue;

    if( IsUnconscious(manAI) )
      continue;

    Vector3 targPos = manAI->FutureVisualState().Position();

    float dist = targ->Distance(actPos);
    if( dist > 10 )
      continue;

    if( dist < bestDist )
    {
      bestDist = dist;
      bestTarg = targ;
    }
  }

  if( bestTarg )
  {
    ChaseTarget* actTarg = new ChaseTarget;
    const EntityAI* entityAI = bestTarg->idExact;
    const Man* manAI = dynamic_cast<const Man*>(entityAI);
    actTarg->_target = const_cast<Man*>(manAI);
    actTarg->AddWaypoint(manAI->FutureVisualState().Position());
    return actTarg;
  }
#endif

  return NULL;
}

// ===================== Zombie ======================

Zombie::Zombie(const EntityAIType *name, bool fullCreate) : LightAI(name, fullCreate)
{
  ZombieProbeLog("[ZombieCtor] type=%s fullCreate=%d", cc_cast(name->GetName()), fullCreate ? 1 : 0);
  static const LightAIFSM::StateInfo states[] = {
    LightAIFSM::StateInfo("none", Dummy, Dummy),
    LightAIFSM::StateInfo("idle", LightAI::IdleStateInit, LightAI::IdleStateCheck),
    LightAIFSM::StateInfo("wandering", LightAI::WanderingStateInit, LightAI::WanderingStateCheck),
    LightAIFSM::StateInfo("feeding", LightAI::FeedingStateInit, LightAI::FeedingStateCheck),
    LightAIFSM::StateInfo("chasing", LightAI::ChasingStateInit, LightAI::ChasingStateCheck),
    LightAIFSM::StateInfo("melee", LightAI::MeleeStateInit, LightAI::MeleeStateCheck),
    LightAIFSM::StateInfo("runaway", LightAI::RunawayStateInit, LightAI::RunawayStateCheck)
  };
  _fsm = new LightAIFSM(states, lenof(states), NULL, 0);
  _fsm->SetState(AISNone, this);
  SetTargetCategory("infected");

  _tiredFactorMax = GRandGen.RandomValue() * 7 + 40;
  _tiredFactorNoSprint = GRandGen.RandomValue() * 5 + 30;
}

Zombie::~Zombie()
{
}

LSError Zombie::Serialize(ParamArchive &ar)
{
  _fsm->Serialize(ar);
  return Man::Serialize(ar);
}

LSError Animal::Serialize(ParamArchive &ar)
{
  _fsm->Serialize(ar);
  return Man::Serialize(ar);
}

float ChaseTarget::Age() const
{
  return (Glob.time - _lastUpdate);
}

void ChaseTarget::AddWaypoint(Vector3Par pos)
{
#if 0 //bypass breadcrumb system
  if(_trail.Size() > 0)
    _trail[0] = TrailWaypoint(pos, Glob.time, false);
  else
    _trail.Add(TrailWaypoint(pos, Glob.time, false));
  return;
#endif

  int count = _trail.Size();
  _lastUpdate = Glob.time;
  //bool prevIsSafe = false;

  if(count != 0)  
  {
    float dist = pos.DistanceXZ2(_trail[count - 1]._pos);

    //target is not moving
    if(dist < 0.01f)
      return;

    //when target is in collision, be more precise
    bool morePrecise = InBuildingOrColliding(_target);
    float distLimit = morePrecise ? Square(0.15f) : Square(0.5f);

    if(dist < distLimit)
      return;
    bool inLine = false;
    if(_trail.Size() >= 2)
    {
      //are we out of straight line?
      const Vector3& posLast = _trail[count - 1]._pos;
      Vector3 dirLast = posLast - _trail[count - 2]._pos;
      dirLast[1] = 0; 
      dirLast.Normalize();
      float planex = dirLast.Z();
      float planez = -dirLast.X();
      float planed = planex * posLast.X() + planez * posLast.Z();

      float pdist = (planex * pos.X() + planez * pos.Z()) - planed;
      if(fabs(pdist) < (morePrecise ? 0.2f : 0.4f))
        inLine = true;
    }

    if(inLine)
    {
      _trail[count - 1] = TrailWaypoint(pos, Glob.time, count > 1);
      return;
    }
  }
  if(_trail.Size() > 100)
  {
    //TODO: decimate it
    _trail.Delete(0);
  }

  _trail.Add(TrailWaypoint(pos, Glob.time, _trail.Size() > 0));
}

ChaseTarget* ChaseTarget::CreateReversed() const
{
  ChaseTarget* targ = new ChaseTarget;
  targ->_lastUpdate = _lastUpdate;
  targ->_trail.Reserve(_trail.Size());
  targ->_trail.Resize(_trail.Size());
  for(int n = 0; n < _trail.Size(); n++)
    targ->_trail[n] = _trail[_trail.Size() - n - 1];
  return targ;
}

bool ChaseTarget::ReachedGoal(Vector3Par pos) const
{
  if(!_trail.Size())
    return true;

  return pos.DistanceXZ(_trail[_trail.Size() - 1]._pos) < 1.0f;
}

/*
float IgnoreTimeByDist(float minTime, float maxTime)
{
}
*/

ChaseTarget* LightAI::UpdateTargets()
{
  if(GBlind)
    return NULL;

  ChaseTarget* bestTarg = NULL;
#ifndef _BINARIZE
  float bestDist = FLT_MAX;
  Vector3 actPos = FutureVisualState().Position();
  AIAgent* agent = static_cast<AIAgent*>(Brain());

  if (dynamic_cast<Zombie *>(this) && GetNetworkManager().IsServer())
  {
    static Time nextZombieTargetListDiagnostic = TIME_MIN;
    if (Glob.time >= nextZombieTargetListDiagnostic)
    {
      ZombieProbeLog("[ZombieTargetList] zombie=%s pos=%.2f %.2f %.2f enemyCount=%d",
        cc_cast(Type()->GetName()), actPos.X(), actPos.Y(), actPos.Z(),
        agent ? agent->GetTargetList().EnemyCount() : -1);
      if (agent)
      {
        int logged = 0;
        for (int i = 0; i < agent->GetTargetList().EnemyCount() && logged < 4; i++)
        {
          const TargetNormal *target = agent->GetTargetList().GetEnemy(i);
          const EntityAI *entity = target ? target->idExact : NULL;
          if (!entity)
            continue;
          ZombieProbeLog("[ZombieTargetCandidate] zombie=%s enemy=%s dist=%.2f lastSeenAge=%.2f",
            cc_cast(Type()->GetName()), cc_cast(entity->GetType()->GetName()),
            target->Distance(actPos), Glob.time - target->GetLastSeen());
          logged++;
        }
      }
      nextZombieTargetListDiagnostic = Glob.time + 1.0f;
    }
  }

  //update trails
  for(int t = 0; t < _targets.Size();)
  {
    bool removeTarget = false;
    if(_targets[t]->Age() > 20.0f)
    {
      removeTarget = true;
    }
    else
    {
      if(IsNotAlive(_targets[t]->_target))
        removeTarget = true;
    }
		 
    if(removeTarget)
    {
      if(_actualTarget == _targets[t])
        SetActualTarget(NULL);

      _targets.Delete(t);
      continue;
    }
    _targets[t]->AddWaypoint(_targets[t]->_target->FutureVisualState().Position());
    t++;
  }

  for(int n = 0; n < agent->GetTargetList().EnemyCount(); n++)
  {
    const TargetNormal* targ = agent->GetTargetList().GetEnemy(n);
    const EntityAI* entityAI = targ->idExact;
    const Man* manAI = dynamic_cast<const Man*>(entityAI);
    if(manAI == NULL) //filter out buildings
      continue;

    if(dynamic_cast<const Zombie*>(manAI) != NULL)
      continue; //WTF????

    //AIAgent* agent = static_cast<AIAgent*>(manAI->Brain());
    if(IsNotAlive(manAI))
      continue;

    bool seenRecently = (Glob.time - targ->GetLastSeen()) < Type()->_maxTimeNoLOS;

    if(!seenRecently)
      continue;

    if(IsIgnored(manAI))
      continue;

    //update recent targets
    ChaseTarget* actTarg = NULL;
    for(int t = 0; t < _targets.Size(); t++)
    {
      if(_targets[t]->_target == targ->idExact)
      {
        actTarg = _targets[t];
        actTarg->AddWaypoint(manAI->FutureVisualState().Position());
        break;
      }
    }

    //new target
    if(!actTarg)
    {
      Vector3 targPos = manAI->FutureVisualState().Position();

      //isn't too much attackers there?
      int index = GCombatTargets.Find(TCombatTarget(manAI, NULL));
      if(index >= 0 && index < GCombatTargets.Size())
      {
        CombatTarget* ct = GCombatTargets[index].second;

        if(ct->_attackers.Size() >= 4)
        {
          //are we closer to the target than another attacker?
          float worstDist = actPos.DistanceXZ2(targPos);
          int worstIndex = -1;
          for(int i = 0; i < ct->_attackers.Size(); i++)
          {
            float dist = ct->_attackers[i]->FutureVisualState().Position().DistanceXZ2(targPos);
            if(dist > worstDist)
            {
              worstDist = dist;
              worstIndex = i;
            }
          }
          //we are better than other
          if(worstIndex != -1)
          {
            //replace attacker
            LightAI* prevAttacker = ct->_attackers[worstIndex];
            prevAttacker->RemoveTarget(manAI);
            prevAttacker->IgnoreTarget(manAI, 2.0f + GRandGen.RandomValue() * 2.0f); //ignore him for a while
          }
          else
          {
            //we are not better. Ignore him for a while
            IgnoreTarget(manAI, 2.0f + GRandGen.RandomValue() * 2.0f);
            continue;
          }
        }
      }

      actTarg = new ChaseTarget;
      actTarg->_target = const_cast<Man*>(manAI);
      actTarg->AddWaypoint(targPos);
      _targets.Add(actTarg);
    }

    float dist = targ->Distance(actPos);
    if(dist < bestDist)
    {
      bestDist = dist;
      bestTarg = actTarg;
    }
  }
#endif

  return bestTarg;
}

ChaseTarget* LightAI::FindFeedingTarget()
{
#ifndef _BINARIZE
  const TargetNormal* bestTarg = NULL;
  float bestDist = FLT_MAX;
  Vector3 actPos = FutureVisualState().Position();
  AIAgent* agent = static_cast<AIAgent*>(Brain());

  for(int n = 0; n < agent->GetTargetList().EnemyCount(); n++)
  {
    const TargetNormal* targ = agent->GetTargetList().GetEnemy(n);
    const EntityAI* entityAI = targ->idExact;
    const Man* manAI = dynamic_cast<const Man*>(entityAI);
    if(manAI == NULL) //filter out buildings
      continue;

    if(dynamic_cast<const Zombie*>(manAI) != NULL)
      continue; //WTF????

    if(!IsUnconscious(manAI))
      continue;

    Vector3 targPos = manAI->FutureVisualState().Position();

    float dist = targ->Distance(actPos);
    if(dist > 25)
      continue;

    if(dist < bestDist)
    {
      bestDist = dist;
      bestTarg = targ;
    }
  }

  if(bestTarg)
  {
    ChaseTarget* actTarg = new ChaseTarget;
    const EntityAI* entityAI = bestTarg->idExact;
    const Man* manAI = dynamic_cast<const Man*>(entityAI);
    actTarg->_target = const_cast<Man*>(manAI);
    actTarg->AddWaypoint(manAI->FutureVisualState().Position());
    return actTarg;
  }
#endif

  return NULL;
}

bool IsInWater (const Vector3 & center)
{
  float const y =  GLandscape->SurfaceY(center[0], center[2]);
  float const sealevel = GLandscape->GetSeaLevel();
  return y <= sealevel;
}

float MaxTerrainGradient (const Vector3 & center, float radius)
{
  //gradient
  float heightArray[5];
  heightArray[0] = GLandscape->SurfaceY(center[0], center[2]);
  heightArray[1] = GLandscape->SurfaceY(center[0]+radius, center[2]+radius);
  heightArray[2] = GLandscape->SurfaceY(center[0]+radius, center[2]-radius);
  heightArray[3] = GLandscape->SurfaceY(center[0]-radius, center[2]-radius);
  heightArray[4] = GLandscape->SurfaceY(center[0]-radius, center[2]+radius);

  float gradient = -1;
  const float distance = radius * 1.41421f;
  for (int i = 1; i < 5; i++)
  {
    const float tmpGradient = (fabsf(heightArray[0] - heightArray[i])) / distance;
    if (tmpGradient > gradient)
      gradient = tmpGradient;
  }
  for (int i = 1; i < 5 ;i++)
  {
    const int j = (i % 4) + 1;
    const float tmpGradient = (fabsf(heightArray[i] - heightArray[j])) / radius;
    if (tmpGradient > gradient)
      gradient = tmpGradient;
  }
  return gradient;
}

bool IsTooSteep (const Vector3 & center, float radius, float maxGradient)
{
  //gradient
  if (maxGradient > 0.0f && radius > 0.0f)
  {
    const float grad = MaxTerrainGradient(center, radius);
    if (grad > maxGradient)
      return false;
  }
  return true;
}
 


bool IsFlatEmpty(Vector3& center, float radius, float maxGradient = -1, float minDistance = -1)
{
  //water
  float const onWatter =  0;
  bool const inWater = IsInWater(center);
  if(onWatter == 0 && inWater) return false; // has to be on dry land
  if(onWatter == 2 && !inWater) return false; // has to be in water

  float const height = GLandscape->SurfaceY(center[0],center[2]);
  if (IsTooSteep(center, radius, maxGradient))
      return false;
 
  
  if(minDistance > 0)
  {
    class NearObj: public World::ObjectQuery
    {
    public:
      bool Check(Object* obj) const
      {
        const EntityType *type = obj->GetEntityType();
        if (!type)
        {
          if (obj->GetType() == Network) return false;
          if (obj->GetType() == Temporary) return false;
          if (obj->GetType() == TypeTempVehicle) return false;
#if _ARMA3_TERRAIN_DECALS
          if (obj->GetType() == LandDecal) return false;
#endif 
          // return only objects with a geometry
          LODShape *shape = obj->GetShape();
          if (!shape) return false;
          if (shape->FindGeometryLevel() < 0) return false;

          if (obj->GetRadius() < 0.5) return false;
        }
        else
        {
          if (!type->IsKindOf("Building")) return false;
        }
        return true;
      }
      bool operator()(Object* obj) const
      {
        return false;
      }
    };

    NearObj query;
    if(!GWorld->QueryObjects(query, center, minDistance, World::QF_DYNAMIC|World::QF_STATIC))
      return false;
  }

  //all tests are OK
  center[1] = height;
  return true;
}

Vector3 FindSafePos(Vector3Par center, float minRadius, float maxRadius, float objRadius, float maxGradient, float minDistance)
{
  static RandomGenerator randGen;

  float range = maxRadius - minRadius;
  for(int attempts = 0; attempts < 70; attempts++)
  {
    float addX = (randGen.RandomValue() * 2.0 - 1.0) * range;
    addX + (addX < 0) ? -minRadius : minRadius;
    float addZ = (randGen.RandomValue() * 2.0 - 1.0) * range;
    addZ + (addZ < 0) ? -minRadius : minRadius;

    Vector3  newPos = center + Vector3(addX, 0, addZ);
    if(IsFlatEmpty(newPos, objRadius, maxGradient, minDistance))
    {
      return newPos;
    }
  }
  return center;
}

void LightAIActionContext::Do(Man *ai)
{
  if(!_man.IsNull())
  {
    MoveId moveid = GetPlayMove();
    Man* man = _man;
    static_cast<LightAI*>(man)->OnAnimDone(moveid);
  }
}

void LightAIActionContext::Event(int userType, const RStringB& param)
{
  if(!_man.IsNull())
  {
    MoveId moveid = GetPlayMove();
    Man* man = _man;
    static_cast<LightAI*>(man)->OnAnimEvent(moveid, userType, param);
  }
}

void LightAI::OnAnimEvent(MoveId moveId, int userType, const RStringB& param)
{
  //LogF("OnAnimEvent %s %d %s", GetMovesType()->GetMoveName(_moves.GetPrimaryMove(FutureVisualState()._moves).id).Data(), userType, param.Data());

  switch(userType)
  {
  case 1:
    _checkAttack = 1;
    break;
  case 2:
    if(_checkAttack == 2) //has been used at least once?
      _checkAttack = 0;
    else
      _checkAttack = 3; //mark it
    break;
  };
  base::OnEvent(EEAnimHook, userType, param);
}

void LightAI::OnAnimDone(MoveId moveId)
{
  if(_checkAttack)
  {
    if(_checkAttack == 2) //has been used at least once?
      _checkAttack = 0;
    else
      _checkAttack = 3; //mark it
  }
}

void Zombie::OnAnimDone(MoveId moveId)
{
  base::OnAnimDone(moveId);

  if(_fsm->GetState() == AISFeeding)
  {
    const MoveIdExt &moveWanted = GetMove("FeedOut");
    if (moveWanted.id != MoveIdNone)
    {
      PlayForcedMove(moveWanted, new LightAIActionContext(this, moveWanted));
    }
    _fsm->SetState(AISIdle, this);
  }
}

bool Zombie::CombatStateCheck()
{
  ChaseTarget* bestTarg = UpdateTargets();
  if(bestTarg)
  {
    _lastTargetTime = Glob.time;
  }
  else if((Glob.time - _lastTargetTime) > 5)
  {
    //no target for a while. Go back to idle state
    _fsm->SetState(AISIdle, this);
    return false;
  }

  if(bestTarg != _actualTarget)
  {
		//do we have path there?
		if(bestTarg && !InitPathToTarget(bestTarg, ZombieRayTestOffset, ZombieRayTestRadius, 100))
		{
			//no, try it later
			IgnoreTarget(bestTarg->_target, 3.0f);
			return false;
		}

    SetActualTarget(bestTarg);
  }

  if(_actualTarget == NULL)
  {
    //no target. Go back to idle state
    _fsm->SetState(AISIdle, this);
    return false;
  }

  if(_actualTarget->_target && _actualTarget->_target->IsDeadSet())
  {
    //target is dead. Go back to idle state, where is solved feeding state too
    _fsm->SetState(AISIdle, this);
    SetActualTarget(NULL);
    return false;
  }
  return true;
}

bool CanAttack(Zombie* context, Man* targ, Vector3Par myPos, Vector3Par targPos)
{
  CollisionBuffer col;
  GLandscape->ObjectCollisionLine(context->GetFutureVisualStateAge(),col,FilterIgnoreTwoAndDead(context, targ), myPos + ZombieRayTestOffset, targPos + ZombieRayTestOffset, ZombieRayTestRadius, ObjIntersectGeom);
  if(!col.Size())
    return true;

  //TODO: ignore only attackers, that are expected around target
  for(int n = 0; n < col.Size(); n++)
  {
    Entity* ent = dynamic_cast<Entity*>(col[n].object.GetRef());

	if(!ent)
		continue;

    if(ent->GetHierarchyParent()) //skip proxies
      continue;

    Zombie* obstacle = dynamic_cast<Zombie*>(ent);
    if(obstacle)
      continue;

    return false;
  }
  return true;
}

int LightAI::FindAttackAction(float dist, float height) const
{
  int* selection = ALLOCA(int, Type()->GetNumAttackActions());
  int numSelected = 0;
  for(int n = 0; n < Type()->GetNumAttackActions(); n++)
  {
    const AttackAction& action = Type()->GetAttackAction(n);
    if(dist >= action._minRange && dist <= action._maxRange &&
      height >= action._minHeight && height <= action._maxHeight)
    {
      selection[numSelected++] = n;
    }
  }

  if(!numSelected)
    return -1;

  int action = selection[GRandGen.RandomInt(0, numSelected)];
  //TODO: do a trace with _trace* parameters?

  return action;
}

void Zombie::RunawayStateInit()
{
}

void Zombie::RunawayStateCheck()
{
}

void Zombie::IdleStateInit()
{
  
  float delay = InterpolativC(_nearestPlayer, 5.0f, 500.0f, 2.0f, 10.0f);
  _nextRoamtime = Glob.time + GRandGen.RandomValue() * 2.0f + delay;
  SetActualTarget(NULL);

  //try to find feeding target
  ChaseTarget* feedTarget = FindFeedingTarget();
  if(feedTarget)
	{
		if(!InitPathToTarget(feedTarget, ZombieRayTestOffset, ZombieRayTestRadius, 100))
		{
			//try it later
			IgnoreTarget(feedTarget->_target, 5.0f);
			return;
		}

    SetActualTarget(feedTarget);
	}
}

void Zombie::IdleStateCheck()
{
#ifndef _BINARIZE
  //AIAgent* agent = static_cast<AIAgent*>(Brain());

  ChaseTarget* bestTarg = UpdateTargets();

  if(bestTarg && bestTarg != _actualTarget)
  {
    if(Type()->GetNumAttackActions())
    {
      EntityVisualState& vs = FutureVisualState();
    
			if(!InitPathToTarget(bestTarg, ZombieRayTestOffset, ZombieRayTestRadius, 100))
			{
				//try it later
				IgnoreTarget(bestTarg->_target, 5.0f);
				return;
			}

			Vector3 newPos = bestTarg->_target->FutureVisualState().Position();

      float dist2 = newPos.Distance2(vs.Position());

			SetActualTarget(bestTarg);
      _toSafeSpot.Clear(); //consider our position as safe

      //don't play this animation when we are close
      if(dist2 > Square(25))
      {
        const MoveIdExt &moveWanted = GetMove("GetMad");
        if (moveWanted.id != MoveIdNone)
        {
          PlayForcedMove(moveWanted, new LightAIActionContext(this, moveWanted));
        }
      }

      _fsm->SetState(AISChasing, this);
    }
    else
    {
			//TODO:
//			_fsm->SetState(AISRunaway, this);
    }
    return;
  }
  else
  {
    if(!_actualTarget)
    {
      //check last roam time
      if(Glob.time > _nextRoamtime)
      {
        float minRange = Type()->_minRoamRange;
        float maxRange = Type()->_maxRoamRange;
        float minDist = 3.0f;
        if(!_objectLandContact.IsNull())
        {
          minRange = 1.0f;
          maxRange = 8.0f;
          minDist = -1;
        }

        Vector3 newPos;
        int iteration = 0;
        while(true)
        {
          const Man::VisualState& vs = FutureVisualState();

          //generate new roam point
          newPos = FindSafePos(vs.Position(), minRange, maxRange, 0.5f, 0.5f, minDist);
          if(newPos.DistanceXZ(vs.Position()) < 1.0f)
          {
            //nothing new, try it next time
            _nextRoamtime = Glob.time + GRandGen.RandomValue() * (Type()->_maxIdleStayTime - Type()->_minIdleStayTime) + Type()->_minIdleStayTime;
            return;
          }
        
          Vector3 delta = newPos - vs.Position();

          CollisionBuffer col;
          GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(),col,FilterIgnoreOneAndDead(this),vs.Position() + ZombieRayTestOffset, newPos + ZombieRayTestOffset,ZombieRayTestRadius,ObjIntersectGeom, ObjIntersectNone, CF_FIRST_CONTACT|CF_ONLY_STATIC);
          if(col.Size())
          {
            //if we are in building, try to walk even a little
            if(!_objectLandContact.IsNull())
            {
              if(++iteration < 5)
                continue;

              //don't consume time, try it little bit later
              //_nextRoamtime = Glob.time + 0.1;

              //try to go back
              SetActualTarget(_toSafeSpot.CreateReversed());
              _toSafeSpot.Clear();
              return;
            }
            else
            {
              //don't consume time, try it little bit later
              _nextRoamtime = Glob.time + 0.5;
              return;
            }
          }
          break;
        }

        SetActualTarget(new ChaseTarget());
        _actualTarget->AddWaypoint(newPos);
        Say(Type()->_idleSounds, true);

        if(_sniffRandom.RandomValue() > 0.4f)
        {
          const MoveIdExt &moveWanted = GetMove("SniffAir");
          if (moveWanted.id != MoveIdNone)
          {
            PlayForcedMove(moveWanted, new LightAIActionContext(this, moveWanted));
          }
        }
      }
    }
    else
    {

      //check if we reached the waypoint
      if(_actualTarget->ReachedGoal(FutureVisualState().Position()))
      {
        //if the target was unconscious character, switch to feed state
        if(_actualTarget->_target && IsUnconscious(_actualTarget->_target))
        {
          _fsm->SetState(AISFeeding, this);
        }
        SetActualTarget(NULL);
        _nextRoamtime = Glob.time + GRandGen.RandomValue() * (Type()->_maxIdleStayTime - Type()->_minIdleStayTime) + Type()->_minIdleStayTime;
      }
      else
      {
        _walkTarget = _actualTarget;
      }
    }
  }
#endif
}

void Zombie::FeedingStateInit()
{
  //PlayAction("AttackShort", new ZombieActionContext(context, GetMove("AttackShort")), true);
  
  const MoveIdExt &moveWanted = GetMove("FeedIn");
  if (moveWanted.id != MoveIdNone)
  {
    PlayForcedMove(moveWanted, new LightAIActionContext(this, moveWanted));
  }
  else
  {
    _fsm->SetState(AISIdle, this);
  }
  
  return;
}

void Zombie::FeedingStateCheck()
{
  ChaseTarget* bestTarg = UpdateTargets();

  if(bestTarg)
  {
		//do we have path there?
		if(!InitPathToTarget(bestTarg, ZombieRayTestOffset, ZombieRayTestRadius, 100))
		{
			//no, try it later
			IgnoreTarget(bestTarg->_target, 3.0f);
			return;
		}

    const MoveIdExt &moveWanted = GetMove("FeedOut");
    if (moveWanted.id != MoveIdNone)
    {
      PlayForcedMove(moveWanted, new LightAIActionContext(this, moveWanted));
    }

    SetActualTarget(bestTarg);
    _toSafeSpot.Clear(); //consider our position as safe
    _fsm->SetState(AISChasing, this);
  }
}

void Zombie::MeleeStateInit()
{
}


void Zombie::MeleeStateCheck()
{
  if(!CombatStateCheck())
    return;

  if(!CanWalk())
    return;

  const Man::VisualState& vs = FutureVisualState();

  Man* targ = _actualTarget->_target;
  Vector3 targPos = targ->FutureVisualState().Position();
  Vector3 myPos = vs.Position();
  Vector3 myDir = vs.Direction();
  Vector3 toTargDir = targPos - myPos;
  toTargDir[1] = 0;
  toTargDir.Normalize();

  float dist = targPos.DistanceXZ(myPos);
  if(dist > MELEE_DISTANCE_OFF)
  {
    _walkTarget = _actualTarget;
    _fsm->SetState(AISChasing, this);
    return;
  }

  float dist4attack = MELEE_DISTANCE_IN;
  //when standing/walking, the attack distance is much shorter
  float zspeed = vs.Speed().Size();
  if(zspeed < 2.35f)
    dist4attack = 2.0f;

  //if we are facing enemy and we have good distance, try to attack
  if(_attackAction == 0 && dist < dist4attack  && myDir.DotProduct(toTargDir) > 0.98)
  {
    if((Glob.time - _lastAttackTime) > (1.5 + GRandGen.RandomValue() * 1))
    {
      float height = targ->EyePosition(targ->FutureVisualState()).Y() - myPos.Y();
      
      int attackAction = FindAttackAction(dist, height);

      if(attackAction != -1 && CanAttack(this, targ, myPos, targPos))
      {
        _actualTarget->_trail.Clear();
        _lastAttackTime = Glob.time;
      
        const MoveIdExt &moveWanted = GetMove(Type()->GetAttackAction(attackAction)._actionName);
        if (moveWanted.id != MoveIdNone)
        {
          Say(Type()->_attackSounds, true);

          PlayForcedMove(moveWanted, new LightAIActionContext(this, moveWanted));
          _attackAction = attackAction + 1;
          _checkAttack = 0;
          return;
        }
      }
      else
      {
        //TODO:some obstacles. Either try to avoid, or switch back to chase
        return;
      }
    }
  }

  //just follow as when chase
  _walkTarget = _actualTarget;
}

void Zombie::ChasingStateInit()
{
  Say(Type()->_spottedSounds, true);
  _timeForChasingSound = Glob.time + 7;
}

void Zombie::ChasingStateCheck()
{
/*  if(::GetAsyncKeyState(0x50) & 0x8000)
  {
    GBlind = true;
    GetOperMap()._fsm->SetState(AISIdle, context);
    GetOperMap()._actualTarget = GetOperMap()._toSafeSpot.CreateReversed();
    _toSafeSpot.Clear();
    return;
  }
  */
  if(!CombatStateCheck())
    return;

  _walkTarget = _actualTarget;
  
  Man* targ = _actualTarget->_target;
  Vector3 targPos = targ->FutureVisualState().Position();
  Vector3 myPos = FutureVisualState().Position();

  static Time nextZombieChaseDiagnostic = TIME_MIN;
  if (GetNetworkManager().IsServer() && Glob.time >= nextZombieChaseDiagnostic)
  {
    ZombieProbeLog("[ZombieChaseDiag] zombie=%s pos=%.2f %.2f %.2f target=%.2f %.2f %.2f dist=%.2f",
      cc_cast(Type()->GetName()), myPos.X(), myPos.Y(), myPos.Z(),
      targPos.X(), targPos.Y(), targPos.Z(), myPos.DistanceXZ(targPos));
    nextZombieChaseDiagnostic = Glob.time + 1.0f;
  }

  if(_timeForChasingSound < Glob.time)
  {
    Say(Type()->_chaseSounds, true);
    _timeForChasingSound = Glob.time + 7 + GRandGen.RandomValue() * 4;
  }

  float dist = targPos.DistanceXZ(myPos);

  if(dist > 50 && _tiredFactorHysteresis > _tiredFactorNoSprint)
  {
    IgnoreTarget(_actualTarget->_target, GRandGen.RandomValue() * 4.0f + 4.0f);
    _fsm->SetState(AISIdle, this);
    return;
  }

  //close enough for melee state?
  if(dist < MELEE_DISTANCE_IN)
  {
    CollisionBuffer col;
    GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(),col,FilterIgnoreTwoAndDead(this, targ), myPos + ZombieRayTestOffset, targPos + ZombieRayTestOffset, ZombieRayTestRadius, ObjIntersectGeom, ObjIntersectNone, CF_FIRST_CONTACT);
    if(!col.Size())
      _fsm->SetState(AISMelee, this);
  }
}


int AliveZombies = 0;
int SleepingZombies = 0;
void DeleteVehicle(Entity *veh);

typedef RefArray<Man> dead_bodies_t;
dead_bodies_t _deadBodies;

namespace {
  inline float minDist2ToPlayer (const Vector3 * pos, int pos_sz, const Vector3 & veh_pos)
  {
    float nearestDist2 = FLT_MAX;
    for (int n = 0; n < pos_sz; ++n)
    {
      const float actDistance = pos[n].Distance2(veh_pos);
      if (actDistance < nearestDist2)
      {
        nearestDist2 = actDistance;
      }
    }
    return nearestDist2;
  }
}

//TODO: make scheduler configurable from config file!
class ScheduleZombies
{
  dead_bodies_t & _morgue;
  Ref<EntityType> _type;
  float _radius2;
  bool _manageCorpses;

public:
  AutoArray<Vector3, MemAllocLocal<Vector3, 128> > _playerOrigins;

  ScheduleZombies(dead_bodies_t & deadBodies, Ref<EntityType> &type, float radius, bool manageCorpses)
    : _morgue(deadBodies), _type(type), _radius2(Square(radius)), _manageCorpses(manageCorpses)
  { }

  bool operator () (Entity *entity) const
  {
    EntityAI * veh = dyn_cast<EntityAI>(entity);
    if (!veh) return false;
    if (veh->GetType()->IsSlow()) return false; // static object or inventory item
    if (entity->IsDamageDestroyed())
    {
      if (_manageCorpses)
      {
        Man * manAI = dynamic_cast<Man *>(entity);
        if (manAI)
        {
          if (manAI->CorpseAge() > (60.0f * 10.0f))
          {
#if defined SERVER_CORPSE_PERFORMANCE_PATCH
            Vector3Val pos = manAI->WorldTransform(veh->FutureVisualState()).Position();
            const float min_d2 = 20.0f * 20.0f;
            if (minDist2ToPlayer(&_playerOrigins[0], _playerOrigins.Size(), pos) >= min_d2)
            {
              //ErrF("SPAWN DeleteVehicle '%s' age=%f", cc_cast(manAI->Type()->GetName()), manAI->CorpseAge());
              _morgue.Add(manAI);
              DeleteVehicle(manAI);
              GWorld->DeleteSuspendedEntity(manAI);
            }
#endif
          }
        }
      }
      return false;
    }

    if (veh->Type()->IsKindOf(_type))
    {
      Vector3Val pos = veh->WorldTransform(veh->FutureVisualState()).Position();
      //we need to know nearest distance as it is used for further optimizations
      const float nearestDist2 = minDist2ToPlayer(&_playerOrigins[0], _playerOrigins.Size(), pos);

      float maxRadius = _radius2;
      if (LightAI * const zmb = dynamic_cast<LightAI *>(veh))
      {
        maxRadius = Square(zmb->Type()->_wakeDistance);
        zmb->SetNearestPlayer(nearestDist2);
      }

      if (nearestDist2 > maxRadius)
      {
        if (!veh->IsSimulationDisabled())
          veh->DisableSimulation(true);
        SleepingZombies++;
      }
      else
      {
        if (veh->IsSimulationDisabled())
          veh->DisableSimulation(false);
        AliveZombies++;
      }
    }
    return false;
  }
};

void World::ZombieAIScheduler(float deltaT)
{
  if( !GetNetworkManager().IsServerOwner() )
    return;

  if( Glob.time < _nextSchedulerUpdateZombieAI )
    return;

  _nextSchedulerUpdateZombieAI = Glob.time + 5.0f;


  Ref<EntityType> type = VehicleTypes.New("DZ_LightAI");
  if(type.IsNull())
    return;

  AutoArray< Person*, MemAllocLocal<Person*, 128> > players;


  if (!GetNetworkManager().IsServer())
  {
    //when not in game yet
    if(GWorld->PlayerOn())
      players.Add(GWorld->PlayerOn());
  }
  else
  {
    AutoArray< NetPlayerInfo> netplayers;
    GetNetworkManager().GetPlayers(netplayers);
    for (int i=0; i<netplayers.Size(); i++)
    {
      if (netplayers[i].person)
      {
        Person *person = dyn_cast<Person,NetworkObject>(netplayers[i].person);
        if (person) players.Add(person);
      }
    }
  }

#if defined SERVER_CORPSE_PERFORMANCE_PATCH
  // @NOTE: skip player check, we want collect bodies even if no player in game
#else
  if(!players.Size())
    return;
#endif

  const bool isServer = GetNetworkManager().IsServer();
  const bool isClient = GetNetworkManager().IsClient();

#if defined SERVER_CORPSE_PERFORMANCE_PATCH
  ScheduleZombies schedule(_deadBodies, type, 500, isServer || (!isServer && !isClient));
#else
  ScheduleZombies schedule(_deadBodies, type, 500, false);
#endif

  //merge nearby players to minimize number of tests
  for(int n = 0; n < players.Size(); n++)
  {
    Vector3Val pos = players[n]->WorldPosition(players[n]->FutureVisualState());
    int p = 0;
    for(; p < schedule._playerOrigins.Size(); p++)
    {
      if(pos.Distance2(schedule._playerOrigins[p]) < Square(15.0f))
      {
        schedule._playerOrigins[p] = (schedule._playerOrigins[p] + pos) * 0.5f;
        break;
      }
    }
    if(p == schedule._playerOrigins.Size())
    {
      schedule._playerOrigins.Add(pos);
    }
  }
  AliveZombies = 0;
  SleepingZombies = 0;

  ForEachVehicle(schedule);

#if defined SERVER_CORPSE_PERFORMANCE_PATCH
  for (size_t i = 0, ie = _deadBodies.Size(); i < ie; ++i)
    GWorld->RemoveVehicle(_deadBodies[i]);
  _deadBodies.Clear();
#endif
}
