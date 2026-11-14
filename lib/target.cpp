/*!
  \file
  Implementation of EntityAI functions related to target tracking,
  target list management and weapon selection
*/


#include "wpch.hpp"

#include "vehicleAI.hpp"
#include "Inventory/inventory.hpp"
#include "person.hpp"
#include "global.hpp"
#include <El/Common/randomGen.hpp>
#include "paramArchiveExt.hpp"
#include "world.hpp"
#include "AI/ai.hpp"
#include "landscape.hpp"
#include "visibility.hpp"
#include "diagModes.hpp"

#include <El/Common/perfLog.hpp>
#include <El/Common/perfProf.hpp>
#include <Es/Containers/sortedArray.hpp>
#include <Es/ReportStack/reportStack.hpp>

#if _PROFILE
# pragma optimize("",off)
#endif

#if _ENABLE_CHEATS

# define DIAG_RESULT 0
# define DIAG_SHOOT_RESULT 0
# define DIAG_ATTACK 0

# define DIAG_TARGET 0 // SelectFireWeapon function (0..3)

# define LOG_TARGETS 0

# if DIAG_TARGET
#   define SUSPEND_OTHER_AI 0 // when 1, only camera watched AI is performing fire decisions
#   define NODIAG_OTHER_AI 0 // when 1, only camera watched AI is logging its fire decisions
# endif

#endif

const float ValidTime=5.0f;

int TargetCategories::GetCategory( const char *name )
{
  if (name==nullptr) return -1;
  int value = _categories.GetValue(name);
  if (value<0)
  {
    Array2D<float> old = _maxRange;

    value = _categories.AddValue(name);
    _maxRange.Dim(value+1,value+1);

    for (int y=0; y<old.GetYRange(); y++) for (int x=0; x<old.GetXRange(); x++)
    {
      _maxRange(x,y) = old(x,y);
    }
    for (int y=0; y<old.GetYRange(); y++) for (int x=old.GetXRange(); x<_maxRange.GetXRange(); x++) 
    {
      _maxRange(x,y) = _maxRange(0,y);
    }
    for (int y=old.GetYRange(); y<_maxRange.GetYRange(); y++) for (int x=0; x<_maxRange.GetXRange(); x++) 
    {
      _maxRange(x,y) = FLT_MAX;
    } 
  }
  return value;
}

void TargetCategories::SetRangeLimit( const char *from, const char *target, float limit )
{
  int fromI = GetCategory(from);
  int tgtI = GetCategory(target);
  _maxRange(fromI,tgtI) = limit;
}

void TargetCategories::SetRangeLimit( const char *from, float limit )
{
  int fromI = GetCategory(from);
  for (int tgtI=0; tgtI<_maxRange.GetYRange(); tgtI++)
  {
    _maxRange(fromI,tgtI) = limit;
  }
}

float TargetCategories::GetRangeLimit( int fromCat, int targetCat ) const
{
  if (fromCat<0 || targetCat<0) return FLT_MAX;
  return _maxRange(fromCat,targetCat);
}

TargetCategories::TargetCategories()
{
  Reset();
}

void TargetCategories::Reset()
{
  _categories.Clear();
  _maxRange.Clear();
  GetCategory(""); // introduce default category
}

/*!
\patch 1.28 Date 10/24/2001 by Ondra.
- Fixed: AI was not able to fire at armored training target.
*/
const float MinVisibleFire=0.55f;

float TargetNormal::FadingSpotability() const
{
  // apply accuracy rules
  float validTime = floatMax(ValidTime,_validTime);
  float old=Glob.time-spotabilityTime-validTime-1;
  if( old<0 ) return spotability;  
  float ret=spotability-old*(1.0f/90);
  saturate(ret,0,4);
  return ret;
}

void TargetNormal::OnSuppressed(const AmmoType *ammo)
{
  _timeSuppressed = Glob.time;
  // check suppression
  // assume ammo worth of 7mm of armor hit causes 100 % suppression
  // this corresponds to a rifle bullet 
  if (ammo) _suppressed = ammo->hit*(1.0f/7);
}

float TargetNormal::FadingSuppressed() const
{
  const float suppressTimeout = 60.0f;
  if (_timeSuppressed<Glob.time-suppressTimeout) return 0;
  float suppressedAge = Glob.time-_timeSuppressed;
  // never considered the target completely suppresed - even when we are firing at it, it might fire back
  // if possible, we still want to stay out of its line of fire
  float factor = InterpolativC(suppressedAge,5.0f,suppressTimeout,0.95,0.0f);
  // source _suppressed may be bigger than 1 when strong ammo is used
  return floatMin(1,_suppressed*factor);
}

/*!
\patch 1.01 Date 6/26/2001 by Ondra.
- Fixed: slower forgetting target type and side.
\patch 2.92 Date 1/23/2006 by Jirka
- Fixed: forgetting of target type and size disabled (only position is fading)
*/

float TargetNormal::FadingAccuracy() const
{
  // the side and type accuracy is something which does not reduce when older
  return accuracy;
}

bool TargetNormal::IsVanishedOrDestroyed() const
{
  return vanished || destroyed;
}
AIBrain *TargetNormal::SeenBy() const
{
  return idSensor;
}

void TargetNormal::UpdateRemotePosition(EntityAI *ent)
{
  Assert(ent==idExact);
  position=ent->AimingPosition(ent->FutureVisualState());
  dir = ent->FutureVisualState().Transform().Direction();
  UpdateSnappedPosition();
}


float TargetNormal::FadingSideAccuracy() const
{
  return sideAccuracy;
}

float TargetNormal::FadingPosAccuracy() const
{
  // we assume error 1 m per 1 sec unless we know a type
  //float errorGrowInTime = 1.0f;
  float errorGrowInTime = 1.0f;
  // FIX: avoid moving of static objects
  if (type)
  {
    // if the target is known to be empty, we should suppose it does not move
    // as it is quite likely we know such thing if we can see the target, we use the actual information
    // (minor cheat on AI side)
    if (idExact && idExact->PilotUnit()==NULL)
    {
      errorGrowInTime = 0;
    }
    else
    {
      errorGrowInTime = 0.25f * type->GetTypSpeedMs();
    }
  }
  
  const float maxAge = 180;
  float validTime = floatMax(ValidTime,_validTime);
  if (Glob.time<posAccuracyTime+validTime) return posAccuracy;
  float age = posAccuracyTime>Glob.time-maxAge ? Glob.time-posAccuracyTime : maxAge;
  // first grow error quite fast
  // after a few seconds grow much slower
  // without this hot-fix helicopter has forgotten the position before it came back to engage
  float errorAge = age-validTime;
  const float fastForget = 10.0f;
  float error = 0;
  if (errorAge<fastForget)
  {
    error= errorAge*errorGrowInTime;
  }
  else
  {
    error= fastForget*errorGrowInTime+(errorAge-fastForget)*errorGrowInTime*0.25f;
  }
  return posAccuracy+error;
}

void TargetNormal::UpdatePosition(Vector3Par pos, float typeAccuracy, float posAcc)
{
  position = pos;
  posAccuracy = posAcc;
  accuracy = typeAccuracy;
  sideAccuracy = typeAccuracy;
  posAccuracyTime = Glob.time;
  UpdateSnappedPosition();
}

void TargetNormal::UpdateSnappedPosition()
{
  // update snappedPosition
  float gridDist;
  const EntityAIType *tarType = GetType();
  if (tarType)
  {
    gridDist =  tarType->GetTypSpeed() * 5.0f/3.6f; //distance using typical speed after 5 seconds
    saturate(gridDist, 20.0f, 500.0f);    
  }
  else gridDist = 20.0f;
  const float gridDist2 = gridDist*gridDist;
  if ( position.Distance2(snappedPosition)>gridDist2 )
  {
    snappedPosition = (position + Vector3(gridDist/2, gridDist/2, gridDist/2));
    snappedPosition = snappedPosition - Vector3(fmod(snappedPosition.X(),gridDist), fmod(snappedPosition.Y(),gridDist), fmod(snappedPosition.Z(),gridDist));
  }
}

Target::Target()
{
}

/// general "interest" value
float Target::HowMuchInteresting() const
{
  float interesting = 1;
  // we get some points for being human
  if (IsKindOf(GWorld->Preloaded(VTypeMan))) interesting += 10;
  if (idExact)
  {
    // we get many points for firing, screaming and moving
    // this is all calculated in audibility - making noise
    // if target is visible, we should also check VisibleMovement()
    // we check it always - hopefully it will not matter too much
    interesting += idExact->Audible()*5;
    // we get some points for moving
    interesting += idExact->VisibleMovement();
    // and also for speaking
    interesting += idExact->GetSpeaking()*30;
    // we add something very small to ensure stable sort ordering
    interesting += (idExact->ID().Encode()&0x3ff)*(1.0f/0x3ff);
  }
  return interesting;
}


DEFINE_FAST_ALLOCATOR(TargetNormal)

TargetNormal::~TargetNormal()
{
}

TargetNormal::TargetNormal()
{
  Init();
}

void TargetNormal::RandomPosError()
{
  posErrorDirection = Vector3(
    GRandGen.RandomValue()*2-1,
    GRandGen.RandomValue()*2-1,
    GRandGen.RandomValue()*2-1
  );
}

void TargetNormal::Init()
{
  position=VZero;
  snappedPosition=VZero;
  dir = VForward;
  RandomPosError();
  // create
  speed=VZero;
  side=TSideUnknown;
  sideChecked = false;
  type=NULL;
  spotability=0; // fading
  spotabilityTime=Time(0);
  accuracy=0; // fading
  sideAccuracy=0; // fading
  posAccuracy = 1000; // no accuracy by default
  posAccuracyReal = 1000;
  _validTime = 0;
  posAccuracyTime = TIME_MIN;
  _isKnown = false; // do we remember it
  posReported=VZero;
  timeReported = TIME_MIN;
  vanished=false; // send disappeared units until disappearConfirmed
  hasDisclosedUs=false;
  destroyed=false; // send confirmed kill
  category = TgtCatEnemy;
  reactedToDestruction = false;
  idExact=NULL;
  idSensor=NULL;
  delay=TIME_MAX; // never seen
  delaySensor=TIME_MAX;
  delayVehicle=TIME_MAX;
  lastSeen=TIME_MIN;
  subjectiveCost=0;
  _timeSuppressed = TIME_MIN;
  _suppressed = 0;
  //@{ helpers for target aggregation
  aggrGroupCost = 0;
  aggrIsReported = aggrPresent = false;
  aggrPosition = VZero;
  //@}
}

void TargetNormal::Init(EntityAI *ai)
{
  // it is visible - update state information
  position=ai->AimingPosition(ai->FutureVisualState());
  snappedPosition = VZero;
  UpdateSnappedPosition();
  dir = ai->FutureVisualState().Direction();
  speed=ai->PredictVelocity();
  // TODO: error based on visibility
  sideChecked = false;
  side = TSideUnknown;
  category = TgtCatEnemy;
  type=ai->GetType(0); // unknown type
  //dammage=ai->GetTotalDamage();
  //idSensor=this;
  idExact=ai;
  spotabilityTime=Glob.time-60;
  //visibility=0;
  accuracy=0;
  sideAccuracy=0;
  spotability=0;
  _isKnown = false;
  timeReported=TIME_MIN;
  posReported = VZero;
  destroyed=false;
  reactedToDestruction = false;
  vanished=false;
  hasDisclosedUs=false;
  _timeSuppressed = TIME_MIN;
  _suppressed = 0;
  #if LOG_TARGETS
    LogF
    (
      "New target %s %s",
      (const char *)GetDebugName(),(const char *)ai->GetDebugName()
    );
  #endif
}

bool TargetNormal::IsKnownNoDelay() const
{
  if (!_isKnown) return false;
  const float minSpotability = 0.06f;
  if (FadingSpotability() < minSpotability)
  {
    // target forgotten
    // forget targets only when you do not seen them for a long time
    if (lastSeen < Glob.time - 120)
    {
      return false;
    }
  }

  return true;
}

bool TargetNormal::IsKnownBy( const AIBrain *unit ) const
{
  // known by specific group member
  if (!IsKnownNoDelay()) return false;
  if( delay<=Glob.time ) return true; // whole group already knows the target
  // the same vehicle
  if (idSensor && idSensor->GetVehicle()==unit->GetVehicle())
  {
    if (delayVehicle<=Glob.time) return true;
  }
  if (delaySensor>Glob.time) return false; // even sensor does not know it yet
  if( unit!=idSensor )
  {
    return false; // each agent has its own target list, no need to use _idSeen
  }
  return true;
}

bool TargetNormal::IsKnownByAll() const
{
  // known by all group members
  return IsKnownNoDelay() && delay<=Glob.time;
}

bool TargetNormal::IsKnownBySome() const
{
  // know by at least one group member
  return IsKnownNoDelay() && ( delay<=Glob.time || delaySensor<=Glob.time );
}

void TargetNormal::Forget()
{
  _isKnown = false;
}

void TargetNormal::Reveal(const SideInfo &side, TargetList *list, float accuracy)
{
  if (_isKnown) return;
  _isKnown = true;
  //LogF("TargetNormal::Reveal %s",cc_cast(GetDebugName()));
  UpdateSideAccuracy(accuracy,side,true);
  list->ForceReport(Glob.time+0.2);
}

/*!
\patch 5111 Date 12/21/2006 by Ondra
- Fixed: Targets are made known to AI once message about them is transmitted over radio.
  This prevents situations where AI gunner silently rejected a command because not knowing the target.
\patch 5112 Date 1/4/2007 by Ondra
- Fixed: Crash caused by target handling change in 5111.
*/
void TargetNormal::RevealInVehicle(Transport *veh)
{
  Time maxDelay = Glob.time + 0.3f;
  if (delayVehicle>maxDelay)
  {
    // check if it is the vehicle of idSensor
    // if not, we cannot do anything useful
    if (idSensor && veh==idSensor->GetVehicle())
    {
      delayVehicle = maxDelay;

    }
  }
}


bool TargetNormal::IsKnown() const
{
  return IsKnownBySome();
}

bool TargetNormal::SeenRecently(AIBrain *brain, float maxAge) const
{
  if (!idExact || !brain) return true;
  // for a player this function is used when determining if the target is lockable by a semiautomatic weapon (Javelin)
  // this should not be limited only to situations where a player already revealed a target
  // TODO: consider verifying if the player could possibly see the target
  if (brain->IsAnyPlayer()) return true;
  if (!IsKnownBy(brain) || lastSeen<Glob.time-maxAge)
  {
    return false;
  }
  // check if accuracy corresponds to "seen"
  //Object::ProtectedVisualState<const EntityVisualState> vs = idExact->RenderVisualStateScope(true);
  //if (FadingPosAccuracy()>idExact->VisibleSize(vs))
  if (FadingPosAccuracy()>idExact->AimingSize()*4)
  {
    return false; 
  }

  return true;
}

float TargetNormal::PositionAccuracy(const AIBrain *brain) const
{
  float memoryError = 0;
  if (idExact && brain)
  {
    const float ignoreShortTime = 5.0f;
    // check last visibility information and its age
    float memoryAge = floatMax(Glob.time-posAccuracyTime-ignoreShortTime,0);
    // for more distant targets the memory error should be growing faster
    if (memoryAge>0)
    {
      Vector3 brainPos = brain->GetVehicle()->FutureVisualState().Position();
      memoryError = memoryAge * idExact->FutureVisualState().Position().Distance(brainPos)*0.005f;
//       float movementError = type->GetTypSpeedMs()*0.5f*memoryAge;
//       memoryError = floatMax(movementError,distanceError);
    }
  }

  return floatMax(FadingPosAccuracy(),memoryError);
}

bool TargetNormal::AccurateEnoughForHit(const AmmoType *ammo, AIBrain *brain) const
{
  // check if there is current LOS 
  // _visTracker used before tests LOS for fire geometries
  // we want normal visibility here
  // use normal cached visibility query
  float curError = PositionAccuracy(brain);
  float accuracyRequired = AccuracyRequiredForHit();
  return curError < (ammo ? ammo->indirectHitRange*2 : 0)+accuracyRequired;
}

/*!
\patch 5158 Date 5/16/2007 by Ondra
- Fixed: AI target position accuracy sometimes was fluctuating a little bit for distant targets.
*/
bool TargetNormal::LimitError(float error, float errorReal, float *oldError)
{
  float curError = FadingPosAccuracy();
  if (oldError) *oldError = curError;
  if (error<=curError)
  {
    // note: sometimes older information may be actually more accurate
    // however we prefer the newer one if we think the old is too old (faded away)
    posAccuracy = error;
    posAccuracyReal = errorReal;
    posAccuracyTime = Glob.time;
    return true;
  }
  return false;
}


/// how much do we extrapolate the unit movement
const float MaxPredictMovementTime = 10.0f;
/*!
@param moved how much the function moved the position since some which is know to be valid.
  This is done so that called may keep surface relative Y when interested.

\patch 5122 Date 1/23/2007 by Ondra
- Fixed: Explosion sometimes revealed the exact unit position to the AI units.
*/

Vector3 TargetNormal::ExactAimingPosition(
  const AIBrain *who, const AmmoType *ammo, float &moved, bool head, float maxPredict, bool *isExact
) const
{
    DoAssert(!ammo || who); // when ammo is specified, who must be specified as well
  // calculate position directly if target is fresh known
  // if it is never seen (not isKnown), position is complete crap
  if( idExact )
  {
    bool seen = true;
    if (who && who->LSIsAlive())
    {
      seen = GWorld->Visibility(who,idExact)>0;
    }
    // seen very recently? If yes, track actual position
    // if it is friendly, the delay may be much larger
    if( seen && Glob.time<lastSeen+1.0f)
    {
      Object::ProtectedVisualState<const EntityVisualState> vs = idExact->FutureVisualStateScope(true);
      // fix 5122 - only return exact information if almost exact information was already available      
      if (FadingPosAccuracy()<idExact->VisibleSize(vs))
      {
        moved = 0;
        if (isExact) *isExact = true;
        Vector3 aimFromDir = ammo && who ? (who->GetVehicle()->FutureVisualState().Position()-idExact->FutureVisualState().Position()).Normalized() : VZero;
        // when we are returning exact information, we may be able to return head position as well
        return head ? idExact->AimingPositionHeadShot(vs, ammo, aimFromDir) : idExact->AimingPosition(vs, ammo, aimFromDir);
      }
    }
  }
  if (isExact) *isExact = false;
  // estimate target position - do not use lastSeen (may be in future), use posAccuracyTime
  
  float timeFromSeen=Glob.time.Diff(posAccuracyTime);
  saturateMin(timeFromSeen,maxPredict);
  // speed prediction should not change height above surface - it should not make a solider flying
  moved = speed.Size()*timeFromSeen;
  return position+speed*timeFromSeen;
}

bool TargetNormal::NeedsZoom(float dist) const
{
  float spotErrorEye = dist*GetVisualError();
  return (
    idExact && dist<Glob.config.tacticalZ &&
    // seen by plain eye (not heard only)
    posAccuracyReal<spotErrorEye*6.0f &&
    // but we need a more accurate info
    (
      // the position is inaccurate
      posAccuracyReal>spotErrorEye*0.1f && posAccuracyReal>1.0f
      // or the type/side is inaccurate
      || GetSide()==TSideUnknown && idExact->GetTargetSide()!=TSideUnknown && FadingSideAccuracy()<GetSideAccuracyExact()
    )
  );
}

Vector3 TargetNormal::GetSpeed(const AIBrain *who) const
{
  // if we have a fresh position, assume we have a fresh speed as well
  // calculate position directly is target is fresh known
  // if it is never seen (not isKnown), position is complete crap
  Object *exact=idExact;
  if( exact )
  {
    bool seen = true;
    if (who)
    {
      seen = GWorld->Visibility(who,idExact)>0;
    }
    // seen very recently? If yes, track actual position
    // if it is friendly, the delay may be much larger
    if( seen && Glob.time<lastSeen+1.0f)
    {
      // fix 5122 - only return exact information if almost exact information was already available      
      if (FadingPosAccuracy()<idExact->VisibleSize(idExact->RenderVisualState()))      
      {
        return exact->PredictVelocity();
      }
    }
  }
  return speed;
}

static bool TransportCanMove(Transport *veh)
{
  if (!veh->GetType()->HasDriver()) return true;
  if (!veh->IsPossibleToGetIn()) return false;
  // check if there is a driver, and the driver is alive
  AIBrain *driver = veh->PilotUnit();
  if (!driver || !driver->LSIsAlive())
  {
    // check if we can know the driver is dead
    if (driver)
    {
      const float longEnough = 3.0f;
      Person *person = driver->GetPerson();
      // prevent immediate reaction
      if (person && person->GetDestroyedTime()>Glob.time-longEnough) return true;
    }
    return false;
  }
  return true;
}

TargetState TargetNormal::State(AIBrain *sensor) const
{
  // check if target is alive
  if (destroyed) return TargetDestroyed;
  if (vanished) return TargetDestroyed;
  if (!sensor) return TargetAlive;
  if (!sensor->IsEnemy(side)) return TargetAlive;
  if (!idExact) return TargetEnemyEmpty;
  Transport *trans = dyn_cast<Transport,TargetType>(idExact);
  if (trans && !TransportCanMove(trans))
  {
    // enemy vehicles with alive gunner are dangerous even when they cannot move
    if (idExact->IsAbleToFireAnyTurret())
    {
      return TargetEnemyCombat;
    }
    // if there is any cargo, do not report as empty
    if (idExact->IsCrewed()) return TargetEnemy;
    return TargetEnemyEmpty;
  }
  
  if (idExact->IsCombatUnit()) // if target has no weapons, ignore it cannot fire and still consider it combatant
  {
    if (!idExact->IsAbleToFireAnyTurret())
    { // if target has no gunner, it is less dangerous - TODO: handle assigned replacement gunner
      // when no driver, it is a static defense - not able to fire means empty
      return idExact->GetType()->HasDriver() ? TargetEnemy : TargetEnemyEmpty;
    }
  }
  return TargetEnemyCombat;
}

/*!
\patch 5122 Date 1/23/2007 by Ondra
- Fixed: AI could sometimes aim into the air when targeting an unknown land unit.
*/

Vector3 TargetNormal::LandAimingPosition(const AIBrain *who, const AmmoType *ammo, bool head, bool *isExact) const
{
  float moved = 0;
  bool isExactLocal = false;
  Vector3 pos = ExactAimingPosition(who,ammo,moved,head,MaxPredictMovementTime,&isExactLocal);
  if (isExact) *isExact = isExactLocal;
  // check last actually seen position above surface
  Vector3 posForAbove = isExactLocal ? pos : position;
  Assert(!isExactLocal || moved==0); // when exact, moved must be 0
  float aboveSurface=posForAbove.Y()-GLandscape->SurfaceYAboveWater(posForAbove.X(),posForAbove.Z());
  pos += posErrorDirection*posAccuracy;
  // if it was not flying, assume it is not flying now - same height
  // posAccuracy>X is an optimization (avoid SurfaceYAboveWater) - if accurate, no need to update
  if ((aboveSurface<5 || !type->CanBeAirborne()) && posAccuracy+moved>0.10f)
  {
    pos[1]=GLandscape->SurfaceYAboveWater(pos.X(),pos.Z())+aboveSurface;
  }
  return pos;
}

Vector3 TargetNormal::LastKnownPosition(const AIBrain *who, const AmmoType *ammo, bool head, bool *isExact) const
{
  float moved = 0;
  // we want to use very little prediction here, if any
  static float predict = 0.1f;
  bool isExactLocal = false;
  Vector3 pos = ExactAimingPosition(who,ammo,moved,head,predict,&isExactLocal);
  if (isExact) *isExact = isExactLocal;

  // check last actually seen position above surface
  Vector3 posForAbove = isExactLocal ? pos : position;
  Assert(!isExactLocal || moved==0); // when exact, moved must be 0
  float aboveSurface=posForAbove.Y()-GLandscape->SurfaceYAboveWater(posForAbove.X(),posForAbove.Z());
  pos += posErrorDirection*posAccuracy;
  // if it was not flying, assume it is not flying now - same height
  // posAccuracy>X is an optimization (avoid SurfaceYAboveWater) - if accurate, no need to update
  if ((aboveSurface<5 || !type->CanBeAirborne()) && posAccuracy+moved>0.10f)
  {
    pos[1]=GLandscape->SurfaceYAboveWater(pos.X(),pos.Z())+aboveSurface;
  }
  return pos;
}

Vector3 TargetNormal::LandAimingPosition() const
{
  float moved = 0;
  bool isExactLocal = false;
  Vector3 pos = ExactAimingPosition(NULL,NULL,moved,false,MaxPredictMovementTime,&isExactLocal);
  // check last actually seen position above surface
  Vector3 posForAbove = isExactLocal ? pos : position;
  Assert(!isExactLocal || moved==0); // when exact, moved must be 0
  float aboveSurface=posForAbove.Y()-GLandscape->SurfaceYAboveWater(posForAbove.X(),posForAbove.Z());
  pos += posErrorDirection*posAccuracy;
  if ((aboveSurface<5 || !type->CanBeAirborne()) && posAccuracy+moved>0.10f)
  {
    pos[1]=GLandscape->SurfaceYAboveWater(pos.X(),pos.Z())+aboveSurface;
  }
  return pos;
}

Vector3 TargetNormal::AimingPosition() const
{
  // calculate position directly if target is fresh known
  float moved = 0;
  return ExactAimingPosition(NULL,NULL,moved,false,MaxPredictMovementTime,NULL)+posErrorDirection*posAccuracy;
}

float TargetNormal::VisibleSize() const
{
  Object *exact=idExact;
  if( exact )
  {
    return exact->VisibleSize(exact->FutureVisualState());
  }
  return type->VisibleSize();
}

TargetNormal *TargetNormal::LoadRef(ParamArchive &ar)
{
  Target *tgt = base::LoadRef(ar);
  Assert( !tgt || dynamic_cast<TargetNormal *>(tgt) );
  return static_cast<TargetNormal *>(tgt);
}

#if _ENABLE_INDEPENDENT_AGENTS

DEFINE_FAST_ALLOCATOR(TargetInAgent)

TargetInAgent::TargetInAgent(AIAgent *agent)
: _agentOwner(agent)
{
}

LSError TargetInAgent::Serialize(ParamArchive &ar)
{
  base::Serialize(ar);
  CHECK(ar.SerializeRef("agent", _agentOwner, 1))
  if (ar.IsSaving())
  {
    RString type("agent");
    CHECK(ar.Serialize("targetType", type, 1))
  }
  return LSOK;
}

const SideInfo *TargetInAgent::GetOwnerSideInfo() const
{
  return _agentOwner;
}

LSError TargetInAgent::SaveRef(ParamArchive &ar)
{
  RString type("agent");
  CHECK(ar.Serialize("targetType", type, 1))

  int index = -1;

  AIAgent *agent = _agentOwner;
  if (agent)
  {
    const TargetList &list = agent->GetTargetList();
    for (int i=0; i<list.AnyCount(); i++)
    {
      Target *tgt = list.GetAny(i);
      if (tgt == this)
      {
        index = i;
        break;
      }
    }
    CHECK(agent->SaveRef(ar));
    CHECK(ar.Serialize("index", index, 1))
  }
  return LSOK;
}

#endif

DEFINE_FAST_ALLOCATOR(TargetKnowledge)

void TargetKnowledge::Init()
{
  idExact = NULL;
  position = VZero;
  side = TSideUnknown;
  type = NULL;
  lastSeen = TIME_MIN;
  vanished = false; // send disappeared units until disappearConfirmed
  destroyed = false; // send confirmed kill
}

void TargetKnowledge::Init(Target *src)
{
  idExact = src->idExact;
  position = src->GetPosition();
  side = src->GetSide();
  type = src->GetType();
  lastSeen = src->GetLastSeen();
  vanished = src->IsVanished();
  destroyed = src->IsDestroyed();
}

TargetKnowledge *TargetKnowledge::CreateObject(ParamArchive &ar)
{
  return new TargetKnowledge();
}

LSError TargetKnowledge::Serialize(ParamArchive &ar)
{
  base::Serialize(ar);

  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst) Init();

  CHECK(::Serialize(ar, "position", position, 1))
  CHECK(ar.SerializeEnum("side", side, 1, TSideUnknown))
  CHECK(::Serialize(ar, "type", type, 1))
  CHECK(::Serialize(ar,"lastSeen",lastSeen,1, TIME_MIN));

  CHECK (ar.Serialize("vanished",vanished,1, false));
  CHECK (ar.Serialize("destroyed",destroyed,1, false));

  return LSOK;
}

LSError TargetKnowledge::SaveRef(ParamArchive &ar)
{
  Fail("Reference not allowed");
  return LSStructure;
}

float TargetKnowledge::FadingSideAccuracy() const {return 4;}
float TargetKnowledge::FadingAccuracy() const {return 4;}
float TargetKnowledge::FadingSpotability() const {return 4;}
float TargetKnowledge::FadingPosAccuracy() const {return 0;}

bool TargetKnowledge::IsKnownBy( const AIBrain *unit ) const {return true;}
bool TargetKnowledge::IsKnownByAll() const {return true;}
bool TargetKnowledge::IsKnownBySome() const {return true;}
void TargetKnowledge::Reveal(const SideInfo &side, TargetList *list, float accuracy) {}
void TargetKnowledge::RevealInVehicle(Transport *veh) {}

void TargetKnowledge::UpdatePosition(Vector3Par pos, float typeAccuracy, float posAccuracy)
{
  position = pos;
}

float TargetKnowledge::PositionAccuracy(const AIBrain *brain) const
{
  return 0; // currently we return as if the information is absolutely accurate, this is not correct
}

bool TargetKnowledge::AccurateEnoughForHit(const AmmoType *ammo, AIBrain *brain) const 
{
  return true;
}

bool TargetKnowledge::SeenRecently(AIBrain *brain, float maxAge) const
{
  return true;
}

bool TargetKnowledge::IsKnown() const {return true;}

// ??? better implementation needed ???
float TargetKnowledge::VisibleSize() const
{
  return 1;
}
Vector3 TargetKnowledge::AimingPosition() const
{
  return position;
}
Vector3 TargetKnowledge::LandAimingPosition() const
{
  return position;
}
Vector3 TargetKnowledge::LandAimingPosition(const AIBrain *who, const AmmoType *ammo, bool head, bool *isExact) const
{
  return position;
}

TargetState TargetKnowledge::State(AIBrain *sensor) const
{
  if (destroyed) return TargetDestroyed;
  if (vanished) return TargetDestroyed;
  return TargetAlive;
}

DEFINE_FAST_ALLOCATOR(TargetMinimal)

TargetMinimal::TargetMinimal(TargetId entity)
{
  idExact = entity;
}

const EntityAIType *TargetMinimal::GetType() const
{
  // return type info only with displayable accuracy
  return idExact->GetType(4.0f);
}

float TargetMinimal::FadingSideAccuracy() const {return 4;}
float TargetMinimal::FadingAccuracy() const {return 4;}
float TargetMinimal::FadingSpotability() const {return 4;}
float TargetMinimal::FadingPosAccuracy() const {return 0;}

void TargetMinimal::UpdatePosition(Vector3Par pos, float typeAccuracy, float posAccuracy)
{
  // no updates possible on virtual targets
}


bool TargetMinimal::IsKnownBy( const AIBrain *unit ) const {return true;}
bool TargetMinimal::IsKnownByAll() const {return true;}
bool TargetMinimal::IsKnownBySome() const {return true;}
bool TargetMinimal::IsKnown() const {return true;}
void TargetMinimal::Reveal(const SideInfo &side, TargetList *list, float accuracy) {}
void TargetMinimal::RevealInVehicle(Transport *veh) {}

Time TargetMinimal::GetLastSeen() const {return Glob.time;}

float TargetMinimal::PositionAccuracy(const AIBrain *brain) const {return 0;}

bool TargetMinimal::AccurateEnoughForHit(const AmmoType *ammo, AIBrain *brain) const {return true;}

bool TargetMinimal::SeenRecently(AIBrain *brain, float maxAge) const {return true;}

float TargetMinimal::VisibleSize() const
{
  // note: objects used for minimal can never stop existing
  return idExact->VisibleSize(idExact->FutureVisualState());
}
Vector3 TargetMinimal::AimingPosition() const
{
  return idExact->AimingPosition(idExact->RenderVisualState());
}
Vector3 TargetMinimal::LandAimingPosition() const
{
  return idExact->AimingPosition(idExact->RenderVisualState());
}
Vector3 TargetMinimal::LandAimingPosition(const AIBrain *who, const AmmoType *ammo, bool head, bool *isExact) const
{
  // for "minimal" targets (buildings) aiming position does not depend on ammo
  return head ? idExact->AimingPosition(idExact->RenderVisualState()) : idExact->AimingPositionHeadShot(idExact->RenderVisualState());
}

TargetState TargetMinimal::State(AIBrain *sensor) const
{
  // combat units should never by handled by this class
  Assert (!idExact->IsCombatUnit());
  // check if target is alive
  if (idExact->IsDamageDestroyed()) return TargetDestroyed;
  return TargetAlive;
}

LSError TargetMinimal::Serialize(ParamArchive &ar)
{
  base::Serialize(ar);

  return LSOK;
}

// TODO: move to paramArchiveExt.hpp

template <class BoolArray>
LSError SerializeBoolArray(ParamArchive &ar, const RStringB &name, BoolArray &value, int minVersion)
{
  if (ar.GetArVersion() < minVersion) return LSError(0);
  if (ar.IsSaving())
  {
    if (value.Size()==0) return LSError(0); // default value - empty array
    //ParamArchive arArray;
    SRef<ClassEntry> array = ar.OpenArray(name);
    array->ReserveArrayElements(value.GetCount());
    for (int i=0; i<value.Size(); i++)
    {
      if (value.Get(i))
      {
        array->AddValue(i);
      }
    }
    ar.CloseArray(array);
  }
  else
  {
    if (ar.GetPass() != ParamArchive::PassFirst) return LSError(0);
    SRef<ClassEntry> array = ar.OpenArray(name);
    if (!array)
    {
      value.Clear();
      return LSError(0);
    }
    int n = array->GetSize();
    for (int i=0; i<n; i++)
    {
      SRef<ClassArrayItem> item = (*array)[i];
      value.Set(int(*item),true);
    }
  }
  return LSError(0);
}

LSError TargetNormal::Serialize(ParamArchive &ar)
{
  base::Serialize(ar);
  if (ar.IsLoading())
  {
    if (ar.GetPass() == ParamArchive::PassFirst)
    {
      Init();
    }
  }
  CHECK (ar.Serialize("isKnown",_isKnown,1, false));
  if (!_isKnown)
  {
    if (ar.IsSaving())
    {
      return LSOK;
    }
    else
    {
      if (ar.GetPass() == ParamArchive::PassSecond)
      {
        if (idExact)
        {
          // as in Init(idExact)
          type = idExact->GetType(0); // unknown type
        }
      }
      return LSOK;
    }
  }



  CHECK(ar.SerializeRef("idSensor", idSensor, 1))
  CHECK(ar.SerializeRef("idKiller", idKiller, 1))

  CHECK(::Serialize(ar, "position", position, 1))
  if (ar.IsLoading())
  {
    snappedPosition = VZero;
    UpdateSnappedPosition(); //align position to grid
  }
  CHECK(::Serialize(ar, "dir", dir, 1, VForward))
  CHECK(::Serialize(ar, "posErrorDirection", posErrorDirection, 1, VZero))
  CHECK(::Serialize(ar, "speed", speed, 1, VZero))
  CHECK(ar.SerializeEnum("side", side, 1, TSideUnknown))
  CHECK(ar.Serialize("sideChecked", sideChecked, 1, true))
  CHECK(::Serialize(ar, "type", type, 1))

  CHECK(ar.Serialize("spotability",spotability,1,4.0));
  CHECK(::Serialize(ar, "spotabilityTime",spotabilityTime,1));
  CHECK(ar.Serialize("accuracy",accuracy,1,4.0));
  CHECK(ar.Serialize("sideAccuracy",sideAccuracy,1,4.0));
  
  CHECK(ar.Serialize("posAccuracy",posAccuracy,1,0));
  CHECK(ar.Serialize("posAccuracyReal",posAccuracyReal,1,posAccuracy));
  CHECK(::Serialize(ar,"posAccuracyTime",posAccuracyTime,1,TIME_MIN));

  CHECK(::Serialize(ar,"delay",delay,1, Time(0)));
  CHECK(::Serialize(ar,"delaySensor",delaySensor,1, Time(0)));
  CHECK(::Serialize(ar,"delayVehicle",delaySensor,1, Time(0)));
  CHECK(::Serialize(ar,"lastSeen",lastSeen,1, TIME_MIN));

  CHECK (ar.Serialize("vanished",vanished,1, false));
  CHECK (ar.Serialize("destroyed",destroyed,1, false));
  CHECK (ar.Serialize("reactedToDestruction",reactedToDestruction,1, false));
  CHECK (ar.Serialize("hasDisclosedUs",hasDisclosedUs,1, false));
  CHECK (::Serialize(ar,"timeReported",timeReported,1, TIME_MIN));
  CHECK (::Serialize(ar,"posReported",posReported,1, VZero));

  CHECK(ar.Serialize("subjectiveCost",subjectiveCost,1,0.0));
  CHECK(::Serialize(ar,"timeSuppressed",_timeSuppressed,1,TIME_MIN));
  CHECK(ar.Serialize("suppressed",_suppressed,1,0.0));

  //@{ helper variables for TargetAggregation
  CHECK(::Serialize(ar, "aggrPosition", aggrPosition, 1, VZero));
  CHECK(ar.Serialize("aggrGroupCost", aggrGroupCost, 1, 0));
  CHECK(ar.Serialize("aggrIsReported", aggrIsReported, 1, false));
  CHECK(ar.Serialize("aggrPresent", aggrPresent, 1, false));
  //@}
  
  return LSOK;
}

LSError Target::Serialize(ParamArchive &ar)
{
  CHECK(ar.SerializeRef("idExact", idExact, 1))

  return LSOK;
}

TargetNormal *TargetNormal::CreateObject(ParamArchive &ar)
{
  // only normal targets are stored explicitly
  // TODO: implement Target::CreateObject instead
  RString type;
  if (ar.Serialize("targetType", type, 1, RString("group")) != LSOK) return NULL;

#if _ENABLE_INDEPENDENT_AGENTS
  if (strcmp(type, "agent") == 0)
    return new TargetInAgent(NULL);
#endif

  Fail("Unknown target type");
  return NULL;
}

Target *LoadTargetGhostRef(ParamArchive &ar);

Target *Target::LoadRef(ParamArchive &ar)
{
  RString type;
  if (ar.Serialize("targetType", type, 1, RString("group")) != LSOK) return NULL;

  if (strcmp(type, "minimal") == 0)
  {
    OIDInt id;
    if (ar.Serialize("id", id, 0) != LSOK) return NULL;
    // create a virtual target
    ObjectId objId;
    objId.Decode(id);
    const StaticEntityLink *link = GLandscape->FindBuildingInfo(objId);
    if (!link) return NULL;
    return link->GetVirtualTarget();
  }
  
  if (strcmp(type,"ghost")==0)
  {
    // we need to 
    return LoadTargetGhostRef(ar);
  }

#if _ENABLE_INDEPENDENT_AGENTS

  if (strcmp(type, "agent") == 0)
  {
    AIAgent *agent = static_cast<AIAgent *>(AIAgent::LoadRef(ar));
    if (!agent) return NULL;
    int index;
    if (ar.Serialize("index", index, 1) != LSOK) return NULL;
    const TargetList &list = agent->GetTargetList();
    return list.GetAny(index);
  }

#endif

  return NULL;
}

LSError TargetMinimal::SaveRef(ParamArchive &ar)
{
  RString type("minimal");
  CHECK(ar.Serialize("targetType", type, 1))

  // save object id only - nothing else is used
  OIDInt id = idExact ? idExact->GetObjectId().Encode() : -1;
  CHECK(ar.Serialize("id",id,0,-1));
  return LSOK;
}

/// context used for EntityAIFull::TrackTargets, CanSee

struct TargetNormal::TrackContext: public SideInfo
{
  const EntityAIType *type;

  float invAbilitySpotDistance;
  float invAbilitySpotTime;

  float maxDist;
  float irRange;
  bool notManual;
  EntityAIFull *vehicle;
  Vector3 eDir;
  Vector3 wepDir;

  float opticsFov;
  float opticsZoom;
  float cosOpt;
  /// what we can see
  float cosEye;
  /// center zone of the screen
  float cosEyeFocus;
  /// what we can see, including peripheral vision
  float cosEyePeripheral;

  float night;
  float thermalOptics;

  float sensitivityEar;

  bool initialize;

  float deadDetectTime;
  float nearestEnemy2;
  const TargetList *targets;

  virtual bool IsValid() const = 0;

  /// conditions for danger state detected
  virtual void OnDanger(AIBrain *me, Target *detected) = 0;
  /// dead body detected
  virtual void OnDeadDetected(AIBrain *me, AIBrain *detected) = 0;
  virtual void CheckAdditionalTargetDatabases() = 0;
};

/// check if given target can be tracked on radar/IR scanner/laser scanner extended range
static inline bool IsNotRadarTracked(const EntityAIType *type, const EntityAIType *vehType, const EntityAI *ai)
{
  return (
    !type->GetIRTarget() && (!type->GetLaserTarget() || type->GetLaserScanner()) ||
    !vehType->GetIRScanGround() && !ai->Airborne() && !type->GetNvTarget()
    );

}

#if _ENABLE_INDEPENDENT_AGENTS

struct TargetInAgent::TrackContext : public TargetNormal::TrackContext
{
  AIBrain *brain;

  virtual bool IsValid() const {return brain != NULL;}

  //TODO: Sides are not used anymore. Remove all relevant code
  virtual bool SideIsFriendly(TargetSide side) const {return false; /*brain->IsFriendly(side);*/}
  virtual bool SideIsEnemy(TargetSide side) const {return true; /*brain->IsEnemy(side);*/}
  virtual bool SideIsNeutral(TargetSide side) const {return false; /*brain->IsNeutral(side);*/}

  virtual void OnDanger(AIBrain *me, Target *detected);
  virtual void OnDeadDetected(AIBrain *me, AIBrain *detected);
  virtual void CheckAdditionalTargetDatabases();
};

void TargetInAgent::TrackContext::OnDanger(AIBrain *me, Target *detected)
{
  switch (me->GetCombatMode())
  {
    case CMSafe:
    case CMAware:
  EntityAI *detectedAI = detected->idExact;
  if(
    detectedAI && me->GetVehicle()!=detectedAI && // not for my vehicle
    !me->IsAnyPlayer() // not for player
  )
  {
    me->SetDanger(DCFire, detected->AimingPosition(), detected);
  }
      break;
  }
}

void TargetInAgent::TrackContext::OnDeadDetected(AIBrain *me, AIBrain *detected)
{
  // no reaction yet
}

void TargetInAgent::TrackContext::CheckAdditionalTargetDatabases()
{
  // no additional target databases yet
}

const SideInfo *TargetListInAgent::GetSideInfo(AIBrain *brain) const
{
  return brain->GetAgent();
}

TargetNormal::TrackContext &TargetListInAgent::InitTrackContext(AIBrain *brain)
{
  static TargetInAgent::TrackContext context;

  context.brain = brain;

  return context;
}

TargetNormal *TargetListInAgent::CreateTarget(AIBrain *brain)
{
  return new TargetInAgent(brain->GetAgent());
}

void TargetListInAgent::Manage(AIAgent *agent)
{
  Assert(agent);
  EntityAI *veh = agent->GetVehicle();
  if (!veh)
  {
    ErrF("Agent %s has no vehicle", (const char *)agent->GetDebugName());
    return;
  }
  if (agent->IsInCargo()) return;

  for (int s=0; s<AnyCount();)
  {
    TargetNormal *tVis = GetAny(s);

    // check if information is not too old
    EntityAI *ai = tVis->idExact;
    if (!ai)
    {
      // target object disappeared
      if (tVis->_isKnown) agent->OnTargetDeleted(tVis);
      AnyDeleteAt(s);
      continue;
    }

    float dist2 = veh->FutureVisualState().Position().Distance2(ai->FutureVisualState().Position());

    float irRange = IsNotRadarTracked(ai->GetType(),veh->GetType(),ai) ? 0 : veh->GetType()->GetIRScanRange();

    float maxDistance2 = Square(floatMax(irRange, TACTICAL_VISIBILITY));
    bool canSee = dist2 < maxDistance2;

    if (!canSee)
    {
      if (tVis->_isKnown) agent->OnTargetDeleted(tVis);
      AnyDeleteAt(s);
      continue;
    }

    if (tVis->IsKnownNoDelay())
    {
      // handle vanishing units
      tVis->vanished = !ai->IsInLandscape();
    }
    else
    {
      // delete unknown units that got-in something
      if (!ai->IsInLandscape())
      {
        if (tVis->_isKnown) agent->OnTargetDeleted(tVis);
        AnyDeleteAt(s);
        continue;
      }
    }
    s++;
  }
}

#endif

TargetNormal *TargetList::GetAny(int i) const
{
  for (int c=0; c<NTgtCategory; c++)
  {
    if (i<_list[c].Size()) return _list[c].Get(i);
    i -= _list[c].Size();
  }
  Fail("Out of range");
  return NULL;
}


bool TargetList::DeleteKey(TargetNormal *tgt)
{
  EntityAI *ai = tgt->idExact;
  if (ai) _presence.Remove(ai);
  // first of all try removing from list based on tgt properties
  TgtCategory cat = tgt->category;
  if (_list[cat].DeleteKey(tgt)) return true;
  Fail("Target to delete not found by category");
  for (int i=0; i<NTgtCategory; i++)
  {
    if (_list[i].DeleteKey(tgt)) return true;
  }
  return false;
}

void TargetList::AnyDeleteAt(int i)
{
  for (int c=0; c<NTgtCategory; c++)
  {
    if (i<_list[c].Size())
    {
      TargetNormal *tgt = _list[c][i];
      if (tgt->idExact) _presence.Remove(tgt->idExact);
      return _list[c].Delete(i);
    }
    i -= _list[c].Size();
  }
}

void TargetList::Add(const SideInfo &who, TargetNormal *tgt)
{
  if (tgt->idExact) _presence.Add(tgt);
  TgtCategory cat = tgt->CheckCategory(who);
  // note: category does not change very often
  tgt->category = cat;
  _list[cat].Add(tgt);
}
void TargetList::Clear()
{
  for (int c=0; c<NTgtCategory; c++)
  {
    _list[c].Clear();
  }
  _presence.Clear();
}


void TargetList::CompactIfNeeded()
{
  _list[TgtCatEnemy].CompactIfNeeded(2,8);
  _list[TgtCatEnemyUnknown].CompactIfNeeded(2,8);
  //TODO: Sides are not used anymore. Remove all relevant code
  //_list[TgtCatFriendly].CompactIfNeeded(2,8);
  
  // optimize performs rounding to pow2 on its own, no need to check overhead here
  _presence.Optimize();
}

LSError TargetList::Serialize(ParamArchive &ar, const RStringB &name, int minVersion)
{
  // we want to serialize one list only
  if (ar.IsSaving())
  {
    RefArray<TargetNormal,MemAllocLocal<Ref<TargetNormal>,512> > list;
    for (int i=0; i<NTgtCategory; i++)
    {
      list.Merge(_list[i]);
    }
    // if we want to do this, we need to perform it prior serialization,
    // otherwise indices of targets in target list will change and saved references to targets will become invalid
    /*
    // we have to avoid saving references to objects which are not serialized
    struct IgnoreNonSaved
    {
      bool operator () (const TargetNormal *tgt) const
      {
        return tgt->idExact && !tgt->idExact->MustBeSaved();
      }
    };
    list.ForEachWithDelete(IgnoreNonSaved());
    */
    CHECK(ar.Serialize(name,list,minVersion));
    CHECK(_aggrTargets.Serialize(ar));
    return LSOK;
  }
  else
  {
    if (ar.GetPass()==ParamArchive::PassFirst)
    {
      for (int i=0; i<NTgtCategory; i++)
      {
        _list[i].Clear();
      }
      _presence.Clear();
    }
    CHECK(ar.Serialize(name,_list[TgtCatEnemy],minVersion));
    CHECK(_aggrTargets.Serialize(ar));
    if (ar.GetPass()==ParamArchive::PassSecond)
    {
      // rebuild the hash table
      // force category check
      _presence.Reserve(_list[TgtCatEnemy].Size());
      for (int i=0; i<_list[TgtCatEnemy].Size(); i++)
      {
        TargetNormal *tgt = _list[TgtCatEnemy][i];
        if (!tgt) continue;
        const SideInfo *info = tgt->GetOwnerSideInfo();
        if (info) tgt->category = tgt->CheckCategory(*info);
        if (tgt->idExact) _presence.Add(tgt);
      }
      DistributeTargets(TgtCatEnemy);
    }
    return LSOK;
  }
}

void TargetList::AggregatedTargets::AggregatedGroup::UpdatePosition(int index, float tgtNewCost)
{ // index into _toReport, as nothing inside reported can be changed
  TargetNormal *tgt = _targets[index];
  float costAll = _toReportCost+_reportedCost;
  /*
    NewPosition as subjectiveCost weighted target group center
    We also need to remove the former position from this weighted center

              SUM_t[tPos*tCost] - tgtOldPos*tgtOldCost + tgtNewPos*tgtNewCost
    newPos = -----------------------------------------------------------------
                       SUM_t[tCost] - tgtOldCost + tgtNewCost
  */
  Vector3 sumtPosXtCost = position * costAll;
  Vector3 newPos = (sumtPosXtCost - tgt->aggrPosition*tgt->aggrGroupCost + tgt->position*tgtNewCost)
                 / (costAll - tgt->aggrGroupCost + tgtNewCost);
  float posDiff = newPos.DistanceXZ(position);
  radius += posDiff; // circle(newPos,radius+posDiff) contains previous circle(position,radius)
  position = newPos;
  // update target position as reported (to prevent AIGroup::CreateTargetList to add it again)
  tgt->posReported = tgt->position;
  tgt->aggrPosition = tgt->position; //TODO: for the AIGroup::CreateTargetList purposes use the aggrPosition instead of posReported?!
}

void TargetList::AggregatedTargets::AggregatedGroup::Add(TargetNormal *target)
{
  // update position and radius
  float costAll = _toReportCost+_reportedCost+target->aggrGroupCost;
  // newPosition as subjectiveCost weighted target group center
  Vector3 newPos = ((_toReportCost+_reportedCost)*position + target->aggrGroupCost*target->position) / costAll;
  float posDiff = newPos.DistanceXZ(position);
  radius += posDiff; // circle(newPos,radius+posDiff) contains previous circle(position,radius)
  position = newPos;
  _targets.Add(target);
  // update cost
  if (target->aggrIsReported)
  {
    _reportedCost += target->aggrGroupCost;
    // update target position as reported (to prevent AIGroup::CreateTargetList to add it again)
    target->posReported = target->aggrPosition = target->position;
  }
  else
  {
    _toReportCost += target->aggrGroupCost;
    target->posReported = target->aggrPosition = target->position;
  }
}

void TargetList::AggregatedTargets::AggregatedGroup::Remove(int ix, TargetList::AggregatedTargets *parent)
{
  Ref<TargetNormal> target = _targets[ix]; 
  if (!target) // can happen after Load
  {
    _targets.DeleteAt(ix);
    return;
  }
  float costAll = _toReportCost+_reportedCost;
  if (target->aggrIsReported)
    _reportedCost -= target->aggrGroupCost;
  else
    _toReportCost -= target->aggrGroupCost;
  // remove it
  _targets.DeleteAt(ix);
  // update position (radius can be used the same)
  float cost = _toReportCost+_reportedCost;
  if (cost>0.001f) //the group is empty otherwise
    position = (costAll*position - target->aggrGroupCost*target->aggrPosition) / cost;
  else
  { // possibly split some other group to use one more slot
    parent->SetEmptySlotAvailable();
    position = VZero;
  }
}

void TargetList::AggregatedTargets::SetEmptySlotAvailable()
{
  // the split is rather expensive operation - do it only when there is a chance to succeed
  // One slot is fresh empty. But if there is another slot already empty, there is no need to try splitting, as
  // targets were added into full slot rather than to empty one (aggregation of neighboring targets)
  int count = 0;
  for (int i=0; i<NAggrGroups; i++)
  {
    if (!_targetGroup[i].TargetsCount()) count++;
  }
  if ( count>1 )
    _tryToSplitGroup = true;
}

void TargetList::AddAggrTarget(TargetNormal *vtar, float cost)
{ 
  //LogF("TargetList::AddAggrTarget %s",cc_cast(vtar->GetDebugName()));
  vtar->timeReported = Glob.time;
  //vtar->posReported is updated inside AddTarget
  _aggrTargets.AddTarget(vtar, cost);
#if DIAG_AGGREGATED_TARGETS
  if (_aggrTargets._debugMe) _aggrTargets.DumpState(NULL);
#endif
}

void TargetList::ConsiderAggrTargetsReported()
{
  _aggrTargets.MakeAllReported();
}

// AggregatedTargets implementation
void TargetList::AggregatedTargets::AddTarget(TargetNormal *target, float cost)
{
#if DIAG_AGGREGATED_TARGETS
  if (_debugMe) 
    LogF("  .AGGR. AddTarget");
#endif
  // check whether the target is already present (if so, move it possibly into _toReport list)
  for (int gn=0; gn<NAggrGroups; gn++)
  {
    AggregatedGroup &grp = _targetGroup[gn];
    // search
    for (int i=0, siz=grp._targets.Size(); i<siz; i++)
    {
      if (grp._targets[i]==target)
      {
        if (target->aggrIsReported)
        { // already reported - change it to not reported (update)
          // the position should be updated too
          grp.UpdatePosition(i, cost);
          //make it unreported
          target->aggrIsReported = false;
          // update costs
          grp._reportedCost -= target->aggrGroupCost;
          target->aggrGroupCost = cost;
          grp._toReportCost += cost;
          // test it for report
        }
        else
        { // not reported yet
          grp.UpdatePosition(i, cost);
          // update costs
          grp._toReportCost -= target->aggrGroupCost;
          target->aggrGroupCost = cost;
          grp._toReportCost += cost;
          // test it for report
        }
        return;
      }
    }
  }
  
  // set the cost (it is new target, so aggrGroupCost is not set yet)
  target->aggrGroupCost = cost;

  // choose the aggregated group (possibly empty) to add it and possibly reorganize groups
  AddNewTarget(target);
}

void TargetList::AggregatedTargets::MakeAllReported()
{
  for (int i=0; i<NAggrGroups; i++)
  {
    AggregatedGroup &grp = _targetGroup[i];
    for (int j=0; j<grp._targets.Size(); j++)
    {
      // update costs
      grp._reportedCost += grp._toReportCost;
      grp._toReportCost = 0;
      // set as reported
      grp._targets[j]->aggrIsReported = true;
    }
  }
}

int TargetList::AggregatedTargets::FindNearestTargetGroup(TargetNormal *target)
{
  float best2 = 1e10;
  int bestIx = 0;
  for (int i=0; i<NAggrGroups; i++)
  {
    // empty aggregated group is considered to be at the effectiveBattle distance of the target
    // So the aggregation is possible (for targets close enough) even there are still empty slots
    float dist2 = _targetGroup[i].TargetsCount() ? 
      target->position.DistanceXZ2(_targetGroup[i].position) : Square(GWorld->GetMartaEffectiveBattleDistance(target->GetType()));
    if (dist2 < best2)
    {
      bestIx = i;
      best2 = dist2;
    }
  }
  return bestIx;
}

void TargetList::AggregatedTargets::AddNewTarget(TargetNormal *target)
{
  target->aggrIsReported = false;
  target->aggrPosition = target->position;
  // find the nearest group
  int nearestIx = FindNearestTargetGroup(target);
  AggregatedGroup &tgtGrp = _targetGroup[nearestIx];
  // compute what would happen to position and radius if the target is added directly into tgtGrp;
  float costAll = tgtGrp.GetGRCost()+target->aggrGroupCost;
  Vector3 newPos = (tgtGrp.GetGRCost()*tgtGrp.position + target->aggrGroupCost*target->position) / costAll;

  // Are there any targets in tgtGrp closer to another group? Move them.
  MoveTargetsToNearestGroup(tgtGrp, newPos, target, nearestIx);

  // compute newRadius precisely
  float rad2 = 1e10;
  for (int i=0; i<tgtGrp._targets.Size(); i++)
  {
    saturateMin(rad2, newPos.DistanceXZ2(tgtGrp._targets[i]->aggrPosition));
  }
  saturateMin(rad2, newPos.DistanceXZ2(target->position));
  float newRadius = sqrt(rad2);

  // is newRadius too large, that union of other groups is better? Join them then.
  if (newRadius - tgtGrp.radius > 0)
  {
    int ugr1=0, ugr2=0;
    float unionRadiusEst=newRadius;
    for (int gr1=0; gr1<NAggrGroups-1; gr1++)
    {
      const AggregatedGroup &g1 = _targetGroup[gr1];
      if (!g1.TargetsCount()) continue; // skip empty groups
      for (int gr2=gr1+1; gr2<NAggrGroups; gr2++)
      {
        // compute unionRadius estimation
        // determine unionPos
        const AggregatedGroup &g2 = _targetGroup[gr2];
        if (!g2.TargetsCount()) continue; // skip empty groups
        float unionCost = g1.GetGRCost()+g2.GetGRCost();
        saturateMax(unionCost, FLT_MIN);
        Vector3 unionPos = (g1.position*g1.GetGRCost() + g2.position*g2.GetGRCost())
                         / unionCost;
        float unionRad = g1.radius + unionPos.DistanceXZ(g1.position);
        saturateMax(unionRad, g2.radius + unionPos.DistanceXZ(g2.position));
        if (unionRad < unionRadiusEst)
        {
          unionRadiusEst = unionRad;
          ugr1 = gr1; ugr2 = gr2;
        }
      }
    }
    if (unionRadiusEst < newRadius)
    { // ugr1 and ugr2 points to groups which can be joined
      int grIx = _targetGroup[ugr1].TargetsCount()<_targetGroup[ugr2].TargetsCount() ? ugr1 : ugr2;
      AggregatedGroup &joinGrp = _targetGroup[grIx];
      RefArray<TargetNormal> joinTargets = joinGrp._targets.GetRefArray();
      // make the grIx group co contain ONLY new target
      joinGrp._targets.Clear();
      joinGrp._targets.Add(target);
      joinGrp._reportedCost = 0;
      joinGrp._toReportCost = target->aggrGroupCost;
      joinGrp.position = target->position;
      joinGrp.radius = 0;
      // add all targets from joinTargetR and jointTargetTR to their nearest groups
      MoveTargetsToNearestGroup(joinTargets);
      return; //target is added already
    }
  }
  // add the target finally
  tgtGrp._targets.Add(target);
  // update position, radius and cost
  tgtGrp.position = newPos;
  tgtGrp.radius = newRadius; //this is precise value
  tgtGrp._toReportCost += target->aggrGroupCost;
}

void TargetList::AggregatedTargets::MoveTargetsToNearestGroup(AggregatedGroup &tgtGrp, Vector3 &newPos, TargetNormal *target, int ignoreGrp)
{
  RefArray<TargetNormal> &targets = tgtGrp._targets.GetRefArray();
  for (int i=0; i<targets.Size(); i++)
  {
    TargetNormal *tgt = targets[i];
    float dist2 = tgt->aggrPosition.DistanceXZ2(newPos);
    float bestDist2grp = 1e10;
    int bestIxGrp = 0;
    for (int gi=0; gi<NAggrGroups; gi++)
    {
      //all groups except for tgtGrp and emptyGroups
      if (gi==ignoreGrp || !_targetGroup[gi].TargetsCount()) continue;
      float dist2grp = tgt->aggrPosition.DistanceXZ2(_targetGroup[gi].position);
      if (dist2grp < bestDist2grp)
      {
        bestDist2grp = dist2grp;
        bestIxGrp = gi;
      }
    }
    // prevent frequent target migration by hysteresis
    const float AggrMigCoef = 1.1f; //10%
    if (bestDist2grp*AggrMigCoef < dist2)
    { // move target from tgtGrp into _targetGroup[bestIxGrp]
      MoveTarget(tgtGrp,i,bestIxGrp);
      i--; //i refers to different target now
      // update newPos
      float costAll = tgtGrp.GetGRCost()+target->aggrGroupCost;
      newPos = (tgtGrp.GetGRCost()*tgtGrp.position + target->aggrGroupCost*target->aggrPosition) / costAll;
    }
  }
}

void TargetList::AggregatedTargets::MoveTargetsToNearestGroup(RefArray<TargetNormal> &targets)
{
  for (int i=0; i<targets.Size(); i++)
  {
    TargetNormal *tgt = targets[i];
    float bestDist2grp = 1e10;
    int bestIxGrp = 0; //choose some (even empty)
    for (int gi=0; gi<NAggrGroups; gi++)
    {
      //all groups except the empty ones
      if (!_targetGroup[gi].TargetsCount()) continue;
      float dist2grp = tgt->aggrPosition.DistanceXZ2(_targetGroup[gi].position);
      if (dist2grp < bestDist2grp)
      {
        bestDist2grp = dist2grp;
        bestIxGrp = gi;
      }
    }
    _targetGroup[bestIxGrp].Add(tgt);
  }
}

void TargetList::AggregatedTargets::MoveTarget(AggregatedGroup &tgtGrpFrom, int ixFrom, int groupIxTo)
{
  TargetNormal *tgt = tgtGrpFrom._targets[ixFrom];
  //NOTE: ADD first, REMOVE later! (because of possible refcount==1 before the operation!)
  _targetGroup[groupIxTo].Add(tgt);
  tgtGrpFrom.Remove(ixFrom, this);
  // But Remove has cleared the _isAggregated flag which still should be true
  tgt->aggrPresent = true;
}

void TargetList::AggregatedTargets::RecomputePositionAndRadius()
{
  for (int i=0; i<NAggrGroups; i++)
  {
    AggregatedGroup &grp = _targetGroup[i];
    if (!grp.TargetsCount()) 
    {
      grp._toReportCost = grp._reportedCost = 0;
      continue; // skip empty group
    }
    Vector3 sumtPosXCost = VZero;
    // update and SUM position based on _targets
    grp._reportedCost = grp._toReportCost = 0;
    for (int j=0; j<grp._targets.Size(); j++)
    {
      TargetNormal *tgt = grp._targets[j];
      if (tgt->aggrIsReported)
        grp._reportedCost += tgt->aggrGroupCost;
      else
        grp._toReportCost += tgt->aggrGroupCost;
      sumtPosXCost += tgt->aggrGroupCost * tgt->aggrPosition;
    }
    // update position (using both _reported and _toReport targets)
    float sumCost = grp._reportedCost + grp._toReportCost;
    saturateMax(sumCost, FLT_MIN);
    grp.position = sumtPosXCost / sumCost;
    // update radius
    float rad2 = 0;
    for (int j=0; j<grp._targets.Size(); j++)
    {
      TargetNormal *tgt = grp._targets[j];
      float dist2 = grp.position.DistanceXZ2(tgt->aggrPosition);
      if (dist2 > rad2) rad2 = dist2;
    }
    grp.radius = sqrt(rad2);
  }
}

void TargetList::AggregatedTargets::TryToSplitGroup()
{
  // try to split the largest group only
  int grpIx = 0;
  int maxCount = 0;
  for (int i=0; i<NAggrGroups; i++)
  {
    if (_targetGroup[i].TargetsCount()>maxCount)
    {
      maxCount = _targetGroup[i].TargetsCount();
      grpIx = i;
    }
  }
  if ( maxCount>1 ) //at least two targets needed for split
  {
#if DIAG_AGGREGATED_TARGETS
    LogF("... TryToSplitGroup");
#endif
    AggregatedGroup &grp = _targetGroup[grpIx];
    // select the representatives of the new aggregations (couple of most distant targets)
    int c1 = -1, c2 = -1;
    float maxDist2 = -FLT_MAX; // FIX crash: zero was not sufficient, as DistanceXZ2 happened to be zero
    for (int i=1; i<maxCount; i++)
    {
      Vector3 &pos1 = grp._targets[i]->aggrPosition;
      for (int j=0; j<i; j++)
      {
        float dist2 = pos1.DistanceXZ2(grp._targets[j]->aggrPosition);
        if (dist2 > maxDist2)
        {
          c1 = i; c2 = j; maxDist2 = dist2; // c1 > c2 holds!
        }
      }
    }
    Assert(c1 > c2 && c2 >= 0);
    // c1 and c2 points to targets which could possibly starts its own aggregated groups
    float minBattleDist = floatMin( GWorld->GetMartaEffectiveBattleDistance(grp._targets[c1]->GetType()), GWorld->GetMartaEffectiveBattleDistance(grp._targets[c2]->GetType()) );
    if ( maxDist2 > Square(minBattleDist) )
    { // split is feasible
#if DIAG_AGGREGATED_TARGETS
      LogF("((DumpState before Group Split");
      if (_debugMe) DumpState(NULL);
#endif
      // find the empty group
      int grpIx2 = 0;
      for (int i=0; i<NAggrGroups; i++)
      {
        if (!_targetGroup[i].TargetsCount())
        {
          grpIx2 = i;
          break;
        }
      }
      AggregatedGroup &grp2 = _targetGroup[grpIx2];
      // put the splitTargets outside the group first
      RefArray<TargetNormal> splitTargets = grp._targets.GetRefArray();
      grp._targets.Clear();
      grp.StartNewGroup(splitTargets[c1]);
      grp2.StartNewGroup(splitTargets[c2]);
      splitTargets.DeleteAt(c1); //c1 > c2 holds!
      splitTargets.DeleteAt(c2);
      // add all other targets to their nearest groups
      MoveTargetsToNearestGroup(splitTargets);
      // estimation of the radius is not enough after upper bunch 
      RecomputePositionAndRadius();
#if DIAG_AGGREGATED_TARGETS
      if (_debugMe) DumpState(NULL);
      LogF("^^^ DumpState after Group Split))");
#endif
    }
  }
}

void TargetList::AggregatedTargets::AggregatedGroup::StartNewGroup(TargetNormal *tgt)
{
  _targets.Add(tgt); //add the first
  if (tgt->aggrIsReported)
  {
    _reportedCost = tgt->aggrGroupCost;
    _toReportCost = 0;
  }
  else
  {
    _toReportCost = tgt->aggrGroupCost;
    _reportedCost = 0;
  }
  position = tgt->position;
  radius = 0;
}

bool TargetList::AggregatedTargets::SomethingToReport() const
{
  for (int i=0; i<NAggrGroups; i++)
    if (_targetGroup[i]._toReportCost>0) return true;
  return false;
}

#if DIAG_AGGREGATED_TARGETS
void TargetList::AggregatedTargets::DumpState(const AIGroup *group)
{
  LogF("*** AggregatedTargets::DumpState %s ***", group ? cc_cast(group->GetDebugName()) : "NULL");
  for (int i=0; i<NAggrGroups; i++)
  {
    if (_targetGroup[i].TargetsCount())
    {
      RString aggregatedNameRep;
      RString aggregatedNameToRep;
      int repSize=0, toRepSize=0;
      for (int j=0; j<_targetGroup[i]._targets.Size(); j++)
      {
        if (_targetGroup[i]._targets[j]->idExact)
        {
          if (!_targetGroup[i]._targets[j]->aggrIsReported)
          {
            if (toRepSize++) aggregatedNameToRep = aggregatedNameToRep + ", ";
            aggregatedNameToRep = aggregatedNameToRep + _targetGroup[i]._targets[j]->GetType()->GetTextSingular();
          }
          else
          {
            if (repSize++) aggregatedNameRep = aggregatedNameRep + ", ";
            aggregatedNameRep = aggregatedNameRep + _targetGroup[i]._targets[j]->GetType()->GetTextSingular();
          }
        }
      }
      LogF("  %d: to report %d(%.1f) {%s}", i, toRepSize, _targetGroup[i]._toReportCost, cc_cast(aggregatedNameToRep));
      LogF("     reported  %d(%.1f) {%s}",    repSize, _targetGroup[i]._reportedCost, cc_cast(aggregatedNameRep));
    }
  }
}
#endif

struct CompareInt
{
  int operator () (const int *a, const int *b) const
  { // reversed order, bigger first
    return *b-*a;
  }
};

const float TargetList::AggregatedTargets::ReportDelay = 1.0f; // seconds
const float TargetList::AggregatedTargets::GlobalUpdateDelay = 7.0f; // seconds

LSError TargetList::AggregatedTargets::Serialize(ParamArchive &ar)
{ 
  CHECK(ar.Serialize("TargetGroup", _targetGroup, 1));
  CHECK(::Serialize(ar, "ReportTime", _reportTime, 1));
  CHECK(::Serialize(ar, "GlobalUpdateTime", _globalUpdateTime, 1));
  CHECK(ar.Serialize("Finished", _finished, 1, true)); //true is most common
  return LSOK; 
}

void TargetList::AggregatedTargets::AggregatedGroup::Clear()
{
  _reportedCost = _toReportCost = radius = 0;
  position = VZero; 
  _targets.Clear();
}

// enable to test if results using _presence are correct

#define VERIFY_PRESENCE 0

bool TargetList::IsPresent(EntityAI *ai)
{
  const Ref<TargetNormal> &find = _presence.Get(ai);
  bool fast = _presence.NotNull(find);
  #if VERIFY_PRESENCE
    bool slow = false;
    for( int i=0; i<Size(); i++ )
    {
      if( Get(i)->idExact==ai ) {slow=true;break;}
    }
    DoAssert(fast==slow);
  #endif
  return fast;
}

const TargetNormal *TargetList::FindTarget( TargetType *ai ) const
{
  const Ref<TargetNormal> &find = _presence.Get(ai);
  TargetNormal *fast = find;
  #if VERIFY_PRESENCE
    TargetNormal *slow = NULL;
    for( int i=0; i<Size(); i++ )
    {
      if( Get(i)->idExact==ai ) {slow=Get(i);break;}
    }
    DoAssert(fast==slow);
  #endif
  return fast;
}

TargetNormal *TargetList::FindTarget( TargetType *ai )
{
  const Ref<TargetNormal> &find = _presence.Get(ai);
  TargetNormal *fast = find;
  #if VERIFY_PRESENCE
    TargetNormal *slow = NULL;
    for( int i=0; i<Size(); i++ )
    {
      if( Get(i)->idExact==ai ) {slow=Get(i);break;}
    }
    DoAssert(fast==slow);
  #endif
  return fast;
}

/*!
\patch 5156 Date 5/10/2007 by Ondra
- Fixed: AI no longer starts firing in Hold fire unless the enemy is really threatening it.
*/

static bool CheckIfDangerousToMe(EntityAI *who, EntityAI *me)
{
  // check if some weapon is aligned to fire at me
  // some preliminary tests
  // 1. are there any weapons?
  // 2. if AI, what about combat mode / behavior
  
  AIBrain *unit = who->CommanderUnit();
  if (!unit) return false;
  // when in safe mode, he poses no threat to us
  if (unit->GetCombatModeLowLevel()<=CMSafe) return false;
  return true;
}

/**
Test if unit ai is under a direct threat by who

@param me the entity being disclosed
@param who the entity disclosing

this TargetList belongs to 'who'
*/
bool TargetList::IsDisclosing(TargetType *me, EntityAI *who) const
{
  const TargetNormal *target = FindTarget(me);
  if (!target) return false;
  
  // the group is aware of us - react
  if (target->IsKnownBySome())
  {
    // check if they know our position close enough to be dangerous to us
    // they may know about us because they have seen some explosion caused by us
    // in such case there is no need to panic
    if (target->posAccuracy < me->VisibleSize(me->FutureVisualState())*10 && CheckIfDangerousToMe(who,me))
    {
      return true;
    }
  }

  AIBrain *observer = who->EffectiveObserverUnit();
  if ( observer && observer->IsAnyPlayer())
  {
    // apply somewhat stricter threshold than during the real simulation
    // if the enemy knows about us, the condition above should already succeed
    // if the player opened fire, DCFire or DCHit already probably disclosed the unit
    // this is just to avoid enemy staring at player
    float spotTreshold = 0.75f;
    if( target->FadingSpotability()>spotTreshold)
    {
      // we could check some more conditions for the player:
      // e.g. does he have a weapon raised?
      return true;
    }
  
  }

  return false;
}

void TargetList::AggregatedTargets::ForceReport(Time time)
{
  // force target aggregation to report the target soon
  if ( _reportTime >time)
  {
    _reportTime = time;
  }
//   if ( _globalUpdateTime > time)
//   {
//     _globalUpdateTime = time;
//   }
}

struct EntityAIFull::AddNewTargetContext
{
  float maxVisibility2;
  float nearestEnemyDist2;
  AIBrain *brain;
  const SideInfo &_info;
  
  explicit AddNewTargetContext(const SideInfo &info):_info(info){}
};

/** Typical usage:
  when one particular target changed location abruptly,
  we wan to update target lists for nearby enemies
*/

void EntityAIFull::AddNewTarget(TargetList &res, bool initialize, EntityAI *obj)
{
  PROFILE_SCOPE_EX(tgNew,ai);

  //not interested in ourself
  if(obj == this)
    return;

  AIBrain *unit = EffectiveObserverUnit();
  if (!unit) return;

  //filter out as soon as possible before more expensive tests.
  float maxRange = GWorld->GetTargetCategories().GetRangeLimit(GetTargetCategory(),obj->GetTargetCategory());
  if(maxRange<=0) return;
  const SideInfo *info = res.GetSideInfo(unit);
  if (!info) return;

  // interested in enemies only?
  // can we bear with slight delay for other units?

  
  // first do a basic test for a distance
  // if the unit is quite far, there is no need to hurry and we can wait for a regular update
  // we want to simulate only TrackNearTargets
  // see also other nearTargetLimit
  const float nearTargetLimit = 100.f;
  
  float dist2=obj->FutureVisualState().Position().Distance2Inline(FutureVisualState().Position());
  if( dist2>Square(nearTargetLimit) ) return;
  if( dist2>Square(maxRange) ) return;
  
  AddNewTargetContext ctx(*info);

  float irRange=GetType()->GetIRScanRange();
  ctx.maxVisibility2 = Square(floatMax(TACTICAL_VISIBILITY,irRange));

  ctx.brain = unit;

  // update the target list
  ctx.nearestEnemyDist2 = unit->GetNearestEnemyDist2();

  AddNewTarget(res,initialize,ctx,obj);

  ctx.brain->SetNearestEnemyDist2(ctx.nearestEnemyDist2);
}

void EntityAIFull::AddNewTarget(TargetList &res, bool initialize, AddNewTargetContext &ctx, Entity *obj)
{
  //not interested in ourself
  if(obj == this)
    return;

  // ignore temporary vehicles (tracks...)
  EntityAI *ai=dyn_cast<EntityAI,Entity>(obj);
  if( !ai ) return;
  
  // record all (even invisible)  vehicles
  // out vehicles - if not in list, no need to add them
  if (!ai->IsInLandscape()) return;

  float maxRange = GWorld->GetTargetCategories().GetRangeLimit(GetTargetCategory(),ai->GetTargetCategory());
  //filter out as soon as possible before more expensive tests.
  if(maxRange<=0)
  {
    return;
  }

  // TODO: perform unit traversal here, group together units which are close to each other
  // and have the same scanning range
  float dist2=obj->FutureVisualState().Position().Distance2Inline(FutureVisualState().Position());
  if( dist2>ctx.maxVisibility2 ) return;
  // note: max range may be different for different vehicles

  const EntityAIType *type = ai->GetType();
  if(dist2>Square(TACTICAL_VISIBILITY))
  {
    // far target - check if it can be seen by radar
    if (IsNotRadarTracked(type,GetType(),ai))
    {
      return;
    }
  }

  if(maxRange<FLT_MAX && dist2>Square(maxRange))
  {
    return;
  }

  // check if it is enemy
  //TODO: Sides are not used anymore. Remove all relevant code
  //if (ctx.brain->IsEnemy(ai->GetTargetSide()))
  {
    saturateMin(ctx.nearestEnemyDist2,dist2);
  }

  if (res.IsPresent(ai))
  {
    return;
  }

  TargetNormal *target = res.CreateTarget(ctx.brain);
  target->Init(ai);
  res.Add(ctx._info,target);
}

void EntityAIFull::ReactToGetIn(TargetList &res, EntityAI *obj, Transport *trans)
{
  // if the target side of the transport and entity is the same, there is nothing to do
  // this is a common case - units boarding friendly vehicles
  // TODO: this makes the function useless. When units boards a vehicle, it changes its side to its own
  if (obj->GetTargetSide()==trans->GetTargetSide()) return;

  // if the unit is too far, we do not perform any reaction
  // if the target has no IR signature, we may reduce the max. distance
  float maxVisibility2 = (
    obj->GetType()->GetIRTarget() ?
    Square(floatMax(TACTICAL_VISIBILITY,GetType()->GetIRScanRange())):
    Square(TACTICAL_VISIBILITY)
  );

  float dist2=trans->FutureVisualState().Position().Distance2Inline(FutureVisualState().Position());
  if( dist2>maxVisibility2 ) return;


  AIBrain *unit = EffectiveObserverUnit();
  if (!unit) return;

  // if the entity getting in is know, we cannot do anything
  TargetNormal *objTgt = res.FindTarget(obj);
  if (!objTgt || !objTgt->IsKnownBy(unit)) return;
  TargetNormal *transTgt = res.FindTarget(trans);
  // TODO: check: we might want to reveal the vehicle here if necessary
  if (!transTgt || !transTgt->IsKnownBy(unit)) return;

  // we could perform a more elaborate handling, but we are mostly interested in enemies only
  const SideInfo *info = res.GetSideInfo(unit);
  if (!info) return;

  // if the unit is friendly or the vehicle already enemy, we are not interested
  if (!info->SideIsEnemy(objTgt->side) || info->SideIsEnemy(transTgt->side))
  {
    return;
  }

  if (objTgt->side==transTgt->side) return;

  // if we cannot see the unit now, we cannot see it is getting in
  // however, if we see a vehicle nearby, we will most likely notice what has happened
  if (GWorld->Visibility(unit,obj)<0.2f && GWorld->Visibility(unit,trans)<0.2f) return;

  // now it is a time to update the trans side info
  // we know side has changed
  transTgt->side = objTgt->side;
  transTgt->sideAccuracy = objTgt->sideAccuracy;
  transTgt->sideChecked = objTgt->sideChecked;

  transTgt->category = transTgt->CheckCategory(*info);
}

/**
give a target list opportunity to react to a kill which was seen
\patch 5106 Date 12/20/2006 by Ondra
- Fixed: Dead units might sometimes be able to report kills in their neighbourhood.
*/

void EntityAIFull::CheckKiller( TargetList &res, EntityAI *killer, EntityAI *obj )
{
  // check if we can see the kill

  // check distance from the kill
  const float seeKillLimit = 100.f;
  float dist2=obj->FutureVisualState().Position().Distance2Inline(FutureVisualState().Position());
  if( dist2>Square(seeKillLimit) ) return;

  // check if we know the target
  AIBrain *unit = EffectiveObserverUnit();
  if (!unit || !unit->LSCanTrackTargets() || !GetType()->ScanTargets()) return;

  TargetNormal *tgt = res.FindTarget(obj);
  if (!tgt || !tgt->IsKnownBy(unit)) return;

  // no reason to react to my own destruction - I am dead anyway
  // (and even if not, there is another mechanism for this in place)
  if (obj==this) return;
  // check if there is LOS
  if (GWorld->Visibility(unit,obj)<0.2f) return;

  const SideInfo *info = res.GetSideInfo(unit);
  if (!info) return;

  TargetNormal::TrackContext &ctx = res.InitTrackContext(unit);

  // unit should be still valid
  AIBrain *aiUnit = obj->CommanderUnit();

  // we have seen the kill - react
  tgt->SeenKill(killer,ctx,obj,unit,aiUnit,0.5);
}

class EntityAIFullAddNewTarget
{
  EntityAIFull *_entity;
  EntityAIFull::AddNewTargetContext &_ctx;
  TargetList &_res;
  bool _initialize;
  
  public:
  EntityAIFullAddNewTarget(
    EntityAIFull *entity, EntityAIFull::AddNewTargetContext &ctx,
    TargetList &res, bool initialize
  )
  :_entity(entity),_ctx(ctx),_res(res),_initialize(initialize)
  {
  }
  
  __forceinline bool operator () (Entity *obj) const
  {
    _entity->AddNewTarget(_res,_initialize,_ctx,obj);
    return false;
  }
};

/*!
\patch 5161 Date 5/28/2007 by Ondra
- Fixed: Laser target acquire distance for AI was limited by rendering distance.
*/
void EntityAIFull::AddNewTargets(TargetList &res, bool initialize)
{

  _newTargetsTime=Glob.time;

  PROFILE_SCOPE_EX(tgNew,ai);

  AIBrain *unit = EffectiveObserverUnit();
  if (!unit) return;

  const SideInfo *info = res.GetSideInfo(unit);
  if (!info) return;
  
  AddNewTargetContext ctx(*info);

  ctx.brain = unit;

  ctx.nearestEnemyDist2 = 1e10;

  // check which vehicles are visible from this one

  float irRange=GetType()->GetIRScanRange();
  ctx.maxVisibility2=Square(floatMax(TACTICAL_VISIBILITY,irRange));

  EntityAIFullAddNewTarget forEach(this,ctx,res,initialize);
  GWorld->ForEachVehicle(forEach);
  // TODO: perhaps we could walk only slow vehicles, and ignore very slow ones? (Very slow are usually lamps)
  GWorld->ForEachSlowVehicle(forEach);

  ctx.brain->SetNearestEnemyDist2(ctx.nearestEnemyDist2);
}

float TargetNormal::AdjustMyDelay(AIBrain *unit, float spotability) const
{
  Target *assigned = unit->GetTargetAssigned();
  if (assigned && assigned->IsKnownBy(unit) && assigned!=this)
  {
    // we are already busy - longer reaction time
    return InterpolativC(spotability,0.75f,4.0f,3.0f,1.0f); // when the target is visible very well, do not make the reaction time much longer
  }
  return 1;
}


void TargetNormal::UpdateSideAccuracy(float sensorSideAccuracy, const SideInfo &ctx, bool forceCheckCategory)
{
  EntityAI * ai = idExact;
  float fadingSideAccuracy = sideAccuracy;
  TargetSide tSide = ai->GetTargetSide(sensorSideAccuracy);
  // hack - enable to change side to enemy when unit becomes a renegade
  bool checked = sensorSideAccuracy>=GetSideAccuracyExact();
  if (sensorSideAccuracy>=fadingSideAccuracy || checked || tSide == TEnemy && side != TEnemy)
  {
    // special check for a vehicle "stolen" by a renegade
    if (ctx.SideIsFriendly(tSide) || tSide==TCivilian)
    {
      if (sensorSideAccuracy>=GetSideAccuracyExact()*0.9f && ctx.SideIsEnemy(ai->GetTargetSide()))
      {
        // unless we know for sure it is enemy, we consider stolen vehicles as unknown
        // as they are not supposed to be moving
        tSide = TSideUnknown;
        checked = true;
      }
    }
    bool checkCategory = side!=tSide;
    side = tSide;
    sideChecked = checked;
    // do not replace a larger value with a smaller one without reason (side changed)
    if (checkCategory || sensorSideAccuracy>=fadingSideAccuracy)
    {
      sideAccuracy = floatMax(sensorSideAccuracy,sideAccuracy);
    }
    if (checkCategory || forceCheckCategory)
      category = CheckCategory(ctx);
  }
}

/*!
\patch 5164 Date 7/9/2007 by Ondra
- Fixed: AI was able to pinpoint enemy by ear after seeing him for a while.
\patch 5164 Date 9/14/2007 by Ondra
- Fixed: AI was sometimes unable to move when seeing enemy very near.

\patch 5209 Date 12/31/2007 by Ondra
- Fixed: Sometimes AI might wrongly favor audible information over a visual one after not seeing the target for a while.
*/

void TargetNormal::Sees(AIBrain *unit, float dist2, TrackContext &ctx, float trackTargetsPeriod)
{
  EntityAI *ai = idExact;
  #if _ENABLE_CHEATS
    // debugging opportunity - camera unit checking the cursor target
    if (ctx.vehicle==GWorld->GetObserver())
    {
      if (GWorld->UI()->GetCursorTarget() && ai==GWorld->UI()->GetCursorTarget()->idExact)
      {
        __asm nop;
      }
    }
    // debugging opportunity - some unit checking the player
    if (ai==GWorld->PlayerOn())
    {
      __asm nop;
    }
    // debugging opportunity - camera unit checking the player
    if (ctx.vehicle==GWorld->GetObserver() && ai==GWorld->PlayerOn())
    {
      __asm nop;
    }
    // debugging opportunity - camera unit checking the camera unit?
  #endif
  // AI FIX in 1.33
  // {{ distance is increased for units with low skill
  float incDist2 = 1+(ctx.invAbilitySpotDistance-1)*0.4f;
  saturate(incDist2,0.001f,1000.0f);
  dist2 *= incDist2;
  if (dist2>Square(ctx.maxDist)) return;
  
  bool laserContact =
  (
    (ai->GetType()->GetLaserTarget() && ctx.type->GetLaserScanner()) ||
    (ai->GetType()->GetNvTarget() && ctx.type->GetNVScanner())
  );
  // no need to track invisible targets
  // note: this was introduced to handle ghost targets, but they use TargetGhost now
  // track laser targets even when invisible - being tracked is their only purpose
  if (ai->Invisible() && !laserContact && idSensor!=unit)
  {
    return;
  }

  _validTime = trackTargetsPeriod+1.0f;

  // }}

  bool radarContact=
  (
    dist2<Square(ctx.irRange) && ai->GetType()->GetIRTarget() &&
    (ctx.type->GetIRScanGround() || ai->Airborne())
  );
#if _ENABLE_CHEATS
  if (ctx.vehicle==GWorld->GetObserver())
  {
    // debug opportunity
    __asm nop;
  }
#endif
  // variable spotability limit depending on unit "busy" status
  Target *assigned = unit->GetTargetAssigned();
  // threshold for radar contacts is derived from tactical display rendering
  // - see InGameUI::DrawTacticalDisplay - if (visible<0.1f)
  float spotTreshold = radarContact || laserContact ? 0.05f : 0.75f;
  // when busy, spotting other targets than the assigned one may be somewhat more difficult
  if (assigned && assigned!=this) spotTreshold *= 1.2f;
  if (!ctx.notManual)
  {
    // player auto-spotting has more strict conditions
    // currently condition for veteran mode is never auto-spot
    // we can auto spot friendly or neutral targets
    // they are never reported in radio
    TargetSide aiSide = ai->GetTargetSide();

    spotTreshold = Glob.config.IsEnabled(DTAutoSpot) ? 0.75f : 100.0f; 

    if (ctx.SideIsFriendly(aiSide) || ctx.SideIsNeutral(aiSide))
    {
      // override player auto-report
      // but leave threshold a little bit stricter than normal
      saturateMin(spotTreshold,0.6f);
    }
  }

  float movement = ai->VisibleMovement();
  float aiAudible = ai->Audible();

  float audibility = 0;
  float visibility = 0;
  float sharpness = 0; // when picture is foggy, no zooming can improve it
  if (!ai->GetType()->GetLaserTarget() || ctx.type->GetLaserScanner() || !ai->GetType()->GetNvTarget() || ctx.type->GetNVScanner())
    visibility = ctx.vehicle->CalcVisibility(ai,dist2,&audibility,&sharpness);

  /*
  LogF
  (
    "Track: %s from %s - %g, night %g",
    (const char *)ai->GetDebugName(),
    (const char *)GetDebugName(),
    visibility,night
  );
  */

  // small object are less visible
  // if sensor is moving fast, sensibility may be decreased
  if (!radarContact && !laserContact)
  {
    if (ctx.vehicle->FutureVisualState().Speed().SquareSize()>Square(10))
    {
      // we should compute relative speed only
      // what we are really interested about it our velocity relative to the target surroundings
      // when we are moving towards the target, it is stationary and the movement is not distracting us at all
      // compute the part of the velocity we can discard (the closing/moving away one)
      Vector3 relPos = ai->FutureVisualState().Position()-ctx.vehicle->FutureVisualState().Position();
      float relPosSize2 = relPos.SquareSize();
      if (relPosSize2>0)
      {
        // direction of relPos
        Vector3 mySpeed = ctx.vehicle->FutureVisualState().Speed();
        // compute angular speed
        Vector3 angSpeed = mySpeed.CrossProduct(relPos)/relPosSize2;
        float angSpeedSize = angSpeed.Size();
        // assume when moving 2 rad/sec we can barely see anything, 1 rad/s already makes recognition much harder
        float speedCoef = angSpeedSize*0.5f;
        saturateMin(speedCoef,0.95f);
        visibility *= 1-speedCoef;
      }
    }

    {
      // sensibility to audio should actually depend on how much noise we are making
      // this may be unrelated to the speed
      // TODO: use GetEngineVol, GetEnvironVol, or Audible itself?
      float mySpeed=ctx.vehicle->FutureVisualState().Speed().Size();
      float speedCoef=mySpeed*(1.0f/200);
      saturateMin(speedCoef,0.8f);
      audibility -= speedCoef;
      saturateMax(audibility,0);
    }
  }

  float actSpotabilityEye=visibility;
  float actSpotabilityEar=audibility;

  //LogF("  actSpotability %g",actSpotability);

  if (!radarContact && !laserContact)
    saturate(visibility,0,10);
  saturate(audibility,0,120);

  // update visible movement/fire
  // spot
  float visibleAccuracy=visibility*3;
  saturateMin(visibleAccuracy,30);

  float audibleAccuracy=audibility*3;

  //Vector3Val eDir=ctx.eDir;
  float cosFiEye=ctx.eDir.CosAngle(ai->FutureVisualState().Position()-ctx.vehicle->FutureVisualState().Position());
  float cosFiWep=cosFiEye;

  // TODO: which turret?
  TurretContext context;
  if (unit->GetPerson() && ctx.vehicle->FindTurret(unit->GetPerson(),context) && context._weapons->ValidatedCurrentWeapon().IsValid())
    cosFiWep = ctx.wepDir.CosAngle(ai->FutureVisualState().Position()-ctx.vehicle->FutureVisualState().Position());

  float tgtNight = floatMax(0, ctx.night - ai->GetLit());
  //LogF("  pre visibleAccuracy %g",visibleAccuracy);
  // FIX: target recognition accuracy in night
  float cosEyeAdjusted = ctx.cosEye;
  if (ai->GetType()->GetLaserTarget() || ai->GetType()->GetNvTarget())
    // eye cannot be used for laser targets
    cosEyeAdjusted = -1;

  float nightAccCoef = 1-tgtNight;
  if (radarContact || laserContact) visibleAccuracy*=ctx.type->_sensitivity;
  else if (cosFiWep>ctx.cosOpt) visibleAccuracy*=ctx.type->_sensitivity*ctx.opticsZoom*nightAccCoef * ctx.thermalOptics; // optics
  else if (cosFiEye>ctx.cosEyeFocus) visibleAccuracy*=ctx.type->_sensitivity*nightAccCoef;
  else if (cosFiEye>cosEyeAdjusted) visibleAccuracy*=ctx.type->_sensitivity*0.5f*nightAccCoef;
  else if (cosFiEye>ctx.cosEyePeripheral) visibleAccuracy*=ctx.type->_sensitivity*0.05f*nightAccCoef;
  else visibleAccuracy = 0;

  // when sharpness is extremely low, assume accuracy may not go high
  static float foggy = 0.25f;
  float maxVisAccuracy = sharpness>foggy ? InterpolativC(sharpness,foggy,1,foggy,27) : sharpness;

  if (visibleAccuracy>maxVisAccuracy)
    visibleAccuracy = maxVisAccuracy; // zooming prevented to provide more accurate info by fogging
  audibleAccuracy *= ctx.sensitivityEar*aiAudible*0.25;
  //LogF("  post visibleAccuracy %g",visibleAccuracy);

  // we can never recognize "real" side only by hearing
  // real side is recognized when accuracy is GetSideAccuracyExact() or higher
  float audibleSideAccuracy = floatMin(audibleAccuracy*0.5f,GetSideAccuracyExact()*0.9f);
  float visibleSideAccuracy = floatMin(visibleAccuracy*0.15f,4);

  saturateMin(visibleAccuracy,4);
  saturateMin(audibleAccuracy,4);

  float fadingAccuracy=accuracy;// actually not fading - the side and type accuracy does not get obsolete, as it does not change
  float sensorAccuracy = floatMax(audibleAccuracy,visibleAccuracy);
  if (ctx.initialize)
  {
    // during init assume all friendly units within range are fully known
    if (ctx.SideIsFriendly(ai->GetTargetSide()))
    {
      sensorAccuracy = 4;
      visibleAccuracy = 4;
      audibleAccuracy = 4;
      if (ai->CommanderUnit())
      {
        saturateMax(visibleSideAccuracy,1.6);
        saturateMax(audibleSideAccuracy,1.6);
      }
    }
  }

  if (destroyed)
  {
    // special handling of destroyed targets
    // dead body or vehicle is always civilian - and not checked, as it is not "real" side
    // when unit is respawn, it may become enemy again
    side = TCivilian;
    sideChecked = false;
  }
  else
  {
    float sensorSideAccuracy = floatMax(audibleSideAccuracy,visibleSideAccuracy);
    // It has no sense to compare the values above GetSideAccuracyExact() - everything means we know the exact information
    // comparing it exactly means we cannot replace older information by a newer one unless we are closer to the target
    UpdateSideAccuracy(sensorSideAccuracy, ctx);
  }
  float fadingSideAccuracy=sideAccuracy; 
  // check if object is visible
  //float visibility=FadingVisibility();
  float fSpotability=FadingSpotability();      
  
  const EntityAIType *aiType=ai->GetType();
  float lightCoef=1;

  if (!radarContact && !laserContact && tgtNight>0)
  {
    float lightsOnOff=ai->VisibleLights();
    // dark night value is chosen experimentally, based on observation of AI detection, see news:hse1mm$95m$2@new-server.localdomain
    const float lightNight = 0.997f;
    const float darkNight = 0.999f;
    float spotableLightsOff = InterpolativC(
      tgtNight,lightNight,darkNight,aiType->GetSpotableNightLightsOff(),aiType->GetSpotableDarkNightLightsOff()
    );
    float nightSpotable = Lerp(spotableLightsOff,aiType->GetSpotableNightLightsOn(),lightsOnOff);
    lightCoef = Lerp(1,nightSpotable,tgtNight);
  }

  float dist = sqrt(dist2);

  float delayCoef = 1;
  float hidden = ai->GetHidden(dist); // actually opposite of hidden
  // pattern hides is lower on short distance
  const float patternFullDistance = 60;
  if (dist2<Square(patternFullDistance) && hidden<1)
  {
    float patternFactor = dist/patternFullDistance;
    // if unit is not moving, this effect is not so strong
    float moveFactor = movement;
    saturate(moveFactor,0,1);
    // the bigger patternFactor, the better is unit concealed in patterns
    //LogF(" pattern %.3f, move %.3f, hidden %.3f",patternFactor,moveFactor,hidden);
    patternFactor = 1-(1-patternFactor)*moveFactor;
    hidden = 1 - (1-hidden)*patternFactor;
    //LogF("   patternFactor %.3f, result %.3f",patternFactor,hidden);
  }
  

  float visibleFire=visibility*ai->VisibleFire();
  float audibleFire=audibility*ai->AudibleFire()*0.3f;

  // we have no clue where the target is
  float spotErrorEye = 1e10;
  float spotErrorEar = 1e10;
  float spotCoefEye = 0;
  if (radarContact || laserContact)
  {
    spotCoefEye = ctx.type->_sensitivity*8;
    delayCoef = 0.5f; // faster reaction
    spotErrorEye = 0;
    spotCoefEye = 1;
  }
  else
  {
    if (visibility>1e-4)
    {
      // we can see something
      if (cosFiWep>ctx.cosOpt)
      {
        // optics vision
        float spotOptics = floatMax(ctx.opticsZoom*0.5,1);
        spotCoefEye = movement* spotOptics*ctx.type->_sensitivity* ctx.thermalOptics;
        spotCoefEye*=lightCoef * hidden;
        delayCoef = 1.0f;
        spotErrorEye = dist*GetVisualError()*floatMin(ctx.opticsFov,1.0f) * (1 / ctx.thermalOptics); //decrease error with TI optics
      }
      else if (cosFiEye>cosEyeAdjusted)
      {
        // plain eye vision
        spotCoefEye = movement*ctx.type->_sensitivity;
        spotCoefEye *= lightCoef * hidden;
        delayCoef = 1.5f; // slightly slower reaction
        spotErrorEye = dist*GetVisualError();
      }
      else if (cosFiEye>ctx.cosEyePeripheral)
      {
        // peripheral vision - much lower sensitivity and precision
        spotCoefEye = movement*ctx.type->_sensitivity*0.1f;
        spotCoefEye *= lightCoef*hidden;
        delayCoef = 3.0f; // much slower reaction
        spotErrorEye = dist*GetVisualError()*50;
      }
      else
      {
        visibleFire = 0;
        spotCoefEye = 0;
      }
    }
    float howMuchWeHear = aiAudible*audibility*ctx.sensitivityEar;
    if (howMuchWeHear>1e-3)
    {
      // we can hear something
      // position prediction is very inaccurate
      spotErrorEar = dist*0.7f;
    }
  }
  
  float spotErrorReal = spotErrorEye;

  spotErrorEye *= ctx.invAbilitySpotDistance;
  spotErrorEar *= ctx.invAbilitySpotDistance;

  // make sure error is never too high, and at least approximate direction is known
  saturateMin(spotErrorEar,dist*0.9f);
  
  actSpotabilityEye *= spotCoefEye;
  actSpotabilityEar *= aiAudible*ctx.sensitivityEar;

  // select if we better hear or see
  float actSpotability = actSpotabilityEye;
  float spotError = spotErrorEye;
  if (actSpotability<1 && actSpotability<actSpotabilityEar)
  {
    spotError = spotErrorEar;
    spotErrorReal = spotErrorEar; // we approximate ear as a very wide FOV
    delayCoef = 2.5f; // slower reaction
    actSpotability =  actSpotabilityEar;
    hidden = 1;
#if 0 //_ENABLE_CHEATS
    if (ai->CommanderUnit())
      LogF("%s heard by %s, spot %.3f, spotEye %.3f",
        (const char *)ai->GetDebugName(),
        (const char *)GetDebugName(),
        actSpotabilityEar,actSpotabilityEye);
#endif
  }

  //LogF("  spotCoef %g, actSpotability %g",spotCoef,actSpotability);
  saturateMin(actSpotability,4);
  float newSpotability=actSpotability;

  float newSideAccuracy=fadingSideAccuracy;

  // consider if unit is firing
  float sensorFire = floatMax(visibleFire,audibleFire);
  if( sensorFire>0.8f ) // we can see somebody firing
  {
    //LogF("Fire vis %.2f, aud %.2f",visibleFire,audibleFire);
    TargetType *fireTarget=ai->GetFiredAt();
    // for player we do not know what we fired at
    if (fireTarget)
    {
      // check what we know about the target
      const TargetNormal *fireTVis = ctx.targets->FindTarget(fireTarget);
      if (fireTVis)
      {
        saturateMax(newSideAccuracy,fireTVis->FadingAccuracy());
        saturateMax(newSideAccuracy,fireTVis->FadingSideAccuracy());
        // TODO: this might be also used to reveal something about the target
        // 1) disclose the target
        // 2) estimate side of the target
      }
      // if we are not in combat - this means danger?
    }
    ctx.OnDanger(unit, this);
  }

  float myDelay = 1.0f*ctx.invAbilitySpotTime*delayCoef;

  if (radarContact || laserContact)
  {
    // radar equipped vehicles have much shorter reaction time
    if (dist<ctx.irRange)
    {
      float maxTime = dist2*0.0000003f+0.07f*ctx.invAbilitySpotTime;
      saturateMin(myDelay,maxTime);
    }     
  }
  else
  {
    if (dist2<Square(100) )
    {
      if ( spotError<1 )
      {
        // visible target is very close - improve reaction time
        float maxTime = dist2*0.0002f+0.02f*ctx.invAbilitySpotTime;
        saturateMin(myDelay,maxTime);
      }
      else
      {
        // audible target very close - improve reaction time
        float maxTime = dist2*0.001f+0.1f*ctx.invAbilitySpotTime;
        saturateMin(myDelay,maxTime);
      }
    }
  }

  if( IsKnownNoDelay() )
  {
    if (ctx.initialize)
    {
      delay = Glob.time;
      delaySensor = Glob.time;
      delayVehicle = Glob.time;
      myDelay = 0;
    }
    else if (newSpotability>=FadingSpotability())
    {
      // unit may improve delay
      myDelay *= AdjustMyDelay(unit, newSpotability);
      if (ctx.notManual)
      {
#if 0
        if (unit->GetUnit())
        {
          int id = unit->GetUnit()->ID();
          _idSeen.Set(id,true);
        }
#endif // _DEBUG
        Time newDelay = Glob.time+myDelay;
        if (newDelay<delaySensor)
        {
          //LogF("+++ %s reduce delay to %s at %.2f, delay to %.2f",cc_cast(unit->GetDebugName()),cc_cast(GetDebugName()),Glob.time.toFloat(),newDelay.toFloat());
          delaySensor = newDelay;
          idSensor = unit;
          unit->OnNewTarget(this,delaySensor);
         }
      }
    }
  }
  else
  {
    // target not known yet
    // idSensor should be the unit that sees it best
    // we take unit with best spotability
    if( newSpotability>spotTreshold )
    {
#if 0
      LogF("Set init error %s to %s, %.1f",(const char *)GetDebugName(),(const char *)ai->GetDebugName(),spotError);
#endif
      RandomPosError();

      bool checkCategory = !_isKnown;
      _isKnown = true;
#if DIAG_AGGREGATED_TARGETS
      {
        AIGroup *grp = unit->GetGroup();
        if (grp && grp->GetDebugName()=="B 1-1-A")
          LogF("Target %s:%s _isKnown=true!", cc_cast(ai->GetType()->GetTextSingular()), idExact ? cc_cast(idExact->GetDebugName()) : "???");
      }
#endif
      if (checkCategory)
        category = CheckCategory(ctx);
      position = ai->AimingPosition(ai->FutureVisualState());
      UpdateSnappedPosition();
      speed = ai->PredictVelocity();      
      // whenever updating position, we need to update position accuracy as well
      posAccuracy = spotError;
      posAccuracyTime = Glob.time;      

      // TODO: direction is not always known
      dir = ai->FutureVisualState().Direction();

      // reveal to me
      float groupDelay = 2.5f*myDelay+2.5f;
      myDelay *= AdjustMyDelay(unit, newSpotability);

      // max. 10 sec. radio delay
      saturateMin(groupDelay,myDelay+10.0f);

      if (ctx.initialize)
      {
        myDelay = 0;
        groupDelay = 0;
      }

      // reveal to rest of the group
      delaySensor = Glob.time + myDelay;
      delay = Glob.time + groupDelay;
      delayVehicle = Glob.time + groupDelay;
      idSensor = unit;
      //LogF("+++ %s notice %s at %.2f, delay to %.2f",cc_cast(unit->GetDebugName()),cc_cast(GetDebugName()),Glob.time.toFloat(),delaySensor.toFloat());

#if 0
      if (unit->GetUnit())
      {
        int id = unit->GetUnit()->ID();
        _idSeen.Set(id,true);
      }
#endif // 0
    }
  }

  bool seenNow = false;

  // using lower bound we can ignore targets which are sure to be not known
  const float minSpotabilityLimit = 0.02f;
  if (_isKnown && actSpotability>minSpotabilityLimit)
  {
    // spotability limits depends on whether exact target location is already known
    // we assume less sensory input is needed to track already known target that to discover it

    // note: using posAccuracy is not enough, as even position may be completely different than real target position
    // we should distinguish this somehow in the target
    // for now we assume posAccuracy is set meaningfully and in such situation it will be large enough

    float eyeSpotabilityLimit = minSpotabilityLimit;
    float earSpotabilityLimit = minSpotabilityLimit;

    // spotErrorEye tells what precision we would see the target with
    // this is related to the eye resolution and distance
    // we need the posAccuracy to be within a certain limit based on this
    if (!radarContact && !laserContact)
    {
    // check if discovery is needed
    float maxAccuracyForTracking = spotErrorEye*20.0f;
    // posAccuracy-idExact->VisibleSize() given how much we may be looking "aside" of the object
    // besides of low posAccuracy the error may be caused by the fact unit has moved elsewhere meanwhile
    // if we were able to predict the movement, it does not make the detection harder
    float predictAge = floatMinMax(Glob.time.Diff(posAccuracyTime),0,MaxPredictMovementTime);
    float distanceFromPredictedLine = NearestPointDistance(
      position,position+speed*predictAge,idExact->AimingPosition(idExact->FutureVisualState())
    );

    if (posAccuracy+distanceFromPredictedLine-idExact->VisibleSize(idExact->FutureVisualState())>maxAccuracyForTracking)
    {
      // if it is, the stimulus need to be much higher (cf. spotTreshold, which is even larger)
      eyeSpotabilityLimit = 0.20f;
      earSpotabilityLimit = 0.10f;
    }

    }
    if (actSpotabilityEye>eyeSpotabilityLimit || actSpotabilityEar>earSpotabilityLimit)
    {
      seenNow = true;
    }
  }
  
  // it is quite possible the target is known and it is close enough to have a significant accuracy
  // but is was not noticed yet - in such case we cannot update its info

  if (seenNow)
  {
    if( newSideAccuracy>fadingSideAccuracy )
    {
      //LogF("  set sideAccuracy from newSideAccuracy %g",newSideAccuracy);
      // introduce slight delay before detecting dead unit side
      if (!ai->IsDamageDestroyed() || ai->GetDestroyedTime()<=Glob.time-ctx.deadDetectTime)
      {
#if 0 //_ENABLE_REPORT
        if (ai->IsDamageDestroyed() && unit->GetGroup()->GetCenter()->GetSide()==TWest)
          LogF("  dead %s",(const char *)ai->GetDebugName());
#endif
        side = ai->GetTargetSide(newSideAccuracy);
        sideChecked = true;
        sideAccuracy=newSideAccuracy;
        fadingSideAccuracy=newSideAccuracy;
      }
    }

    //LogF("  accuracy: aud %.2f, vis %.2f",audibleAccuracy,visibleAccuracy);
    if (sensorAccuracy>fadingAccuracy)
    {
      accuracy = sensorAccuracy;
      fadingAccuracy = sensorAccuracy;
    }

    //side=ai->GetTargetSide(fadingSideAccuracy);
    type=ai->GetType(fadingAccuracy);
    if (!sideChecked && !destroyed)
    {
      TargetSide tSide = type->_typicalSide;
      if (tSide!=TSideUnknown && (!ai->IsDamageDestroyed() || ai->GetDestroyedTime()<=Glob.time-ctx.deadDetectTime))
      {
        AIBrain *aiUnit = ai->CommanderUnit();
        if (aiUnit)
        {
          if (aiUnit->GetCaptive()) tSide = TCivilian;
          else
          {
            // check if it is some vehicle friendly units have stolen
            TargetSide realSide = ai->GetTargetSide(4);
            if (ctx.SideIsFriendly(realSide)) tSide = realSide;
          }
        }
        else
        {
          // no commander unit - it is probably empty
          if (!ai->IRSignatureOn() && ctx.SideIsEnemy(tSide))
          {
            // if it is empty, recognize it magically
            tSide = TCivilian;
          }
        }
        #if 0 //_ENABLE_REPORT
        if (ai->IsDamageDestroyed() && unit->GetGroup()->GetCenter()->GetSide()==TWest)
        {
          LogF("  dead 2 %s",(const char *)ai->GetDebugName());
        }
        #endif
        side = tSide;
      }
    }
  }
  if (!destroyed && ai->IsDamageDestroyed() && ai->GetDestroyedTime()<=Glob.time-ctx.deadDetectTime)
  {
    // not reported as destroyed yet
    // set target for reporting
    #if 0 //_ENABLE_REPORT
    if (unit->GetGroup()->GetCenter()->GetSide()==TWest)
    {
      LogF
      (
        "%s dead detected by %s at %.3f, died at %.3f",
        (const char *)ai->GetDebugName(),(const char *)unit->GetDebugName(),
        Glob.time.toFloat(),ai->GetDestroyedTime().toFloat()
      );
    }
    #endif
    destroyed = true;
    reactedToDestruction = false;
    timeReported = TIME_MIN;
    posReported = VZero;
    side = TCivilian;
    sideChecked = false;
    category = CheckCategory(ctx);
  }

  if (seenNow)
  {
    // spot. error is gradually growing - unless there is something which will limit it
    float oldError;
    bool improved = LimitError(spotError,spotErrorReal,&oldError);
    #if 0 // _ENABLE_REPORT
    if (improved && ai->GetTargetSide()!=TCivilian)
    {
      LogF(
        "%s to %s: spot error %g, spot eye %g, spot ear %g, angle %g",
        cc_cast(ctx.vehicle->GetDebugName()),cc_cast(ai->GetDebugName()),
        spotError,actSpotabilityEye,actSpotabilityEar,
        acos(cosFiEye)*(180/H_PI)
      );
    }
    #endif
#if 0
    // if someone is claiming the information is recent and accurate, but in fact it is very wrong, hot-fix it
    if (
      !improved && oldError<1.0 && Glob.time<posAccuracyTime+5.0f &&
      ai->AimingPosition().Distance2(position)>Square(oldError*5)
    )
    {
      if (ai->CommanderUnit()->GetUnit())
      {
        LogF(
          "Hotfixing target %s from %s: reported accuracy %.2f by %s, actual %.2f",
          cc_cast(ai->GetDebugName()),cc_cast(ctx.vehicle->GetDebugName()),
          oldError,idSensor ? cc_cast(idSensor->GetDebugName()) : "",ai->AimingPosition().Distance(position)
        );
      }
      oldError = FLT_MAX;
      improved = true;
    }
#endif

    // use float oldError = FadingPosAccuracy() or change LimitError prototype
    // the information we have may have high precision, but it may be still inaccurate because it is old
    // to counteract this: when spotError is higher than it was before, we limit the position update
    if (improved)
    {
      position = ai->AimingPosition(ai->FutureVisualState());
      UpdateSnappedPosition();
      speed = ai->PredictVelocity();
            
      // whenever updating position, we need to update position accuracy as well
      posAccuracy = spotError; // TODO: done twice, already done in LimitError, eliminate one
      posAccuracyTime = Glob.time;      
      // TODO: direction is not always known
      dir = ai->FutureVisualState().Direction();
      // when our information is a lot more precise than the stored one, become the primary reporter
      if (ctx.notManual && spotError<oldError*0.8f && IsKnownBy(unit))
      {
        idSensor = unit;
      }
    }
    else if (idSensor == unit)
    {
      // when multiple observers provide contradictory information, we want to use only the most precise one
      // which is tracked by idSensor

      // both new and old information can be viewed as a sphere
      // old sphere: position,FadingPosAccuracy()
      // new sphere: AimingPosition(),spotError
      Vector3 currentPos = ai->AimingPosition(ai->FutureVisualState());
      Vector3 currentSpeed = ai->PredictVelocity();
      float dist2 = currentPos.Distance2(position);
      float posAge = Glob.time-posAccuracyTime; // whenever posAccuracy and position is updated always together
      // old is part of new: dist<=rNew-rOld
      // old overlays new: dist<=rNew+rOld
      if (dist2>=Square(oldError+spotError))
      {
        // to avoid losing precision quickly we try to extrapolate
        // if extrapolation matches, we assume it is correct and we leave the information as it is
        if (spotError<0.5f || posAccuracy>0.25f || posAge>5.0f || currentPos.Distance2(position+speed*posAge)>=Square(oldError+spotError))
        {
          #if 0 // _ENABLE_CHEATS
          Vector3 newPos = position+speed*posAge;
          float newDist = currentPos.Distance(position+speed*posAge);
          #endif
        // new information contradicts the old one
        // use the new one - the old must be obsolete now
        posAccuracy = spotError;
        posAccuracyTime = Glob.time;
        position = ai->AimingPosition(ai->FutureVisualState());
        UpdateSnappedPosition();
          speed = currentSpeed;
        // TODO: direction is not always known
        dir = ai->FutureVisualState().Direction();
      }
        else
        {
          #if _ENABLE_CHEATS
          __asm nop;
          #endif
    }
      }
    }
        
    AIBrain *aiUnit = ai->CommanderUnit();
    if (ctx.notManual || !aiUnit || aiUnit->GetLifeState()!=LifeStateDead)
    {
      // player should not report dead bodies as seen, while AI should
      // this is because when AI is not able to report they are dead
      // it will almost certainly see it still in the next sample
      lastSeen=Glob.time+trackTargetsPeriod;
    }

    // check if it is my group dead unit
    if(
      (destroyed || aiUnit && aiUnit->GetLifeState()==LifeStateDead) &&
      // only AI or AutoSpot will report casualties
      (ctx.notManual || Glob.config.IsEnabled(DTAutoSpot))
    )
    {
      // we see a dead body - somebody must have killed it
      if (!reactedToDestruction && ctx.notManual)
      {
        // make sure we react only once
        reactedToDestruction = true;
        // if it was killed by enemy, reveal that enemy
        EntityAI *killer = ai->GetKiller();

        // reaction is supposed to be quite slow here, we do not see the kill, we see only a body
        // ai,aiUnit - the unit killed, unit - the unit seeing the kill
        SeenKill(killer, ctx, ai, unit, aiUnit,3.0f);
      }
      
      // TODO: report all units in it as dead
      ctx.OnDeadDetected(unit, aiUnit);
    }
  }

  if( newSpotability>fSpotability )
  {
    spotability = newSpotability;
    spotabilityTime = Glob.time;
  }

  
  // !hasDisclosedUs - each enemy can disclose us only once
  if (IsKnownByAll() && ctx.SideIsEnemy(side) && !destroyed && !vanished && !hasDisclosedUs)
  {
    if (ctx.SideIsEnemy(ai->GetTargetSide()) && ai->CommanderUnit())
    {
      // hack - check if I'm in the enemy target list
      const TargetList *list = ai->CommanderUnit()->AccessTargetList();
      if (list && list->IsDisclosing(ctx.vehicle,ai) )
      {
        // enemy knows about me - we are probably disclosed
        // open fire and change behavior to danger
        // remember it has disclosed the owner of the target list
        hasDisclosedUs = true;
        if (unit->IsLocal() && !unit->IsPlayer())
        {
          /*
          LogF
          (
          "%s disclosed by %s",
          (const char *)GetDebugName(),(const char *)ai->GetDebugName()
          );
          */
          unit->Disclose(DCEnemyNear, position, ai);
        }
      }
    }
  }

  //Assert( FadingSideAccuracy()<3 || side!=TSideUnknown );

  // check nearest known enemy
  if
  (
    ctx.SideIsEnemy(side) && IsKnownNoDelay() &&
    (ai->GetType()->GetIRTarget() || !ai->GetType()->GetLaserTarget() || !ai->GetType()->GetNvTarget())
  )
  {
    float dist2=position.Distance2(ctx.vehicle->FutureVisualState().Position());
    saturateMin(ctx.nearestEnemy2,dist2);
  }

}

/** 
This determines category. Category is normally cached - this function should not be called very often.
*/

TgtCategory TargetNormal::CheckCategory(const SideInfo &who)
{
  TgtCategory cat = TgtCatEnemy;
  //TODO: Sides are not used anymore. Remove all relevant code
  /*
  TgtCategory cat = TgtCatFriendly;
  // dead bodies should be handled as friendly
  // they are no longer much important
  if (!destroyed)
  {
    // if we think it is an enemy or if it is an enemy we want it to be listed in the enemy list
    if (who.SideIsEnemy(side) || idExact && who.SideIsEnemy(idExact->GetTargetSide(4)))
    {
      cat = TgtCatEnemy;*/
      if (!_isKnown) cat = TgtCatEnemyUnknown;
/*    }
  }*/
  return cat;
}

void TargetNormal::SeesItself(AIBrain *unit, const SideInfo &sideInfo, float trackTargetsPeriod)
{ // anyone sees himself perfectly
  EntityAI *ai = idExact;
  type=ai->GetType(4);
  TargetSide tSide = ai->GetTargetSide(4);
  bool checkCategory = side != tSide;
  side = tSide;
  if (checkCategory)
  {
    category = CheckCategory(sideInfo);
  }
  sideChecked = true;
  spotability = 4;
  accuracy = 4;
  sideAccuracy = 4;
  posAccuracy = 0;
  _validTime = 0;
  spotabilityTime = Glob.time;
  posAccuracyTime = Glob.time;
  _isKnown = true;
  // it will almost certainly see itself until next sample
  lastSeen = Glob.time+trackTargetsPeriod;
  // multiple units may be present in the same vehicle. Check most preferred sensor
  // we do not care who sees it, but we want it to be stable
  if (!idSensor || idSensor->GetVehicle()!=ai || idSensor.GetLink()<unit)
    idSensor = unit;

  delay = Glob.time-5; // no delay
  delaySensor = Glob.time-5; // no delay
  delayVehicle = Glob.time-5;
  position=ai->AimingPosition(ai->FutureVisualState());
  UpdateSnappedPosition();
  dir = ai->FutureVisualState().Direction();
  posErrorDirection=VZero;
  speed=ai->PredictVelocity();
  timeReported=Glob.time; // no need to report itself
  posReported = position;
}

/*!
  \patch 5091 Date 11/28/2006 by Ondra
  - Fixed: Improved AI reaction to friendly units observed being killed.
*/

        // ai,aiUnit - the unit killed, unit - the unit seeing the kill


void TargetNormal::SeenKill(
  EntityAI * killer, TrackContext &ctx, EntityAI *killed, AIBrain *seenKill, AIBrain *killedUnit,
  float basicReactionTime
) const
{
  Target *tgt = NULL;
  if (killer && ctx.SideIsEnemy(killer->GetTargetSide()))
  {
    // we guess it was an enemy, but we do not have a slightest idea of what kind
    float accuracy = 0.1f;
    float sideAcc = 1.5f;
    // assume the enemy should be someplace near
    Vector3 pos = killed->FutureVisualState().Position();

    // add owner to group sensor list
    float delay = basicReactionTime*seenKill->GetInvAbility(AKSpotTime);
    // if vehicle already busy, reaction is slower
    if (seenKill->GetTargetAssigned()) delay *= 2;

    tgt = seenKill->AddTarget(killer,accuracy,sideAcc,delay,&pos,seenKill,delay);
  }

  seenKill->SetDanger(DCDeadBody, AimingPosition(), tgt);
}
/*!
\param ai target considered
\param dist2 square distance to target
\param audibility (optional) pointer receiving target audibility
\return
0 when full occluded, 1 when fully visible
Multiplied by inverse square distance and constant factor
\patch 1.22 Date 8/27/2001 by Ondra.
- Fixed: AI Sound perception less decreased by obstructions.
\patch 1.27 Date 10/18/2001 by Ondra.
- Fixed: AI hearing through solid objects was too good.
\patch_internal 1.22 Date 8/27/2001 by Ondra.
- New: Separate audibility / visibility calculation.
\patch 5150 Date 3/28/2007 by Ondra
- Fixed: Improved laser target detection and engaging by AI.
*/
float EntityAIFull::CalcVisibility(
  EntityAI *ai, float dist2,  float *audibility, float *sharpness, bool assumeLOS
)
{
  if (audibility) *audibility = 0;
  if (sharpness) *sharpness = 0;


  AIBrain *brain=EffectiveObserverUnit();
  if (!brain || !brain->LSIsAlive())
  {
    // dead unit cannot see anything
    return 0;
  }

  float vis = 1;
  const EntityAIType *type = GetType();
  float irRange = type->GetIRScanRange();
  bool irScan= dist2<Square(irRange) && ai->GetType()->GetIRTarget();
  if (ai->GetType()->GetLaserTarget() || ai->GetType()->GetNvTarget())
  {
    // laser targets require special processing
    if (!type->GetLaserScanner()) 
    // perform the same distance calculations as with IR scanning
    irScan = true;
  }

  if (!type->GetIRScanGround() && !ai->Airborne()) irScan = false;
  if( !irScan )
  { // no IR - use eye
    if( dist2>Square(GScene->GetTacticalVisibility()) ) vis=0;
    else vis=1-GScene->TacticalFog8(dist2)*(1.0f/255);
    if (sharpness) *sharpness = vis;
  }
  else
  { // use IR
    if( dist2>Square(irRange) ) vis=0;
    else
    {
      float dist= dist2 > 0 ? dist2*InvSqrt(dist2) : 0; // sqrt(dist2)
      vis=1-dist/irRange;
      // we want radar accuracy to degrade only in last 25 % of the range
      if (sharpness) *sharpness = floatMin(1,vis*4);
    }
  }

  // Note: aiRadius should really be VisibleSize
  // but it was discovered too late (1.28)
  // and impact of fixing it would be too unpredictable
  //float aiRadius=ai->VisibleSize();
  // size is used in calculations from VisibleMovement instead
  // see also Man::VisibleMovement and Man::VisibleSize
  float aiRadius=ai->GetRadius();
  float landVis=assumeLOS ? 1 : GWorld->Visibility(brain,ai);

  if (ai->GetType()->GetLaserTarget() || ai->GetType()->GetNvTarget())
  {
    aiRadius = 160;
  }
  float radiusCoef=5000;
  if( irScan )
  {
    const float increaseIRSensitivity = 1500.0f;
    if (Airborne() && irRange>increaseIRSensitivity)
    {
      // depending on the scan range, 25x magnification may be not enough
      radiusCoef *= irRange * (1.0f/ increaseIRSensitivity);
    }
    radiusCoef*=25;
  }
  float sizeVis = dist2 > 0 ? aiRadius * radiusCoef / dist2 : 100;
  // FIX: near hearing/seeing
  saturateMin(sizeVis,40);

  vis*=sizeVis;
  vis*=landVis;
  vis*=_surfaceVisibility;

  if (audibility)
  {
    // calculate audibility
    float aud = 1;

    // units can hear even behind obstructions to some extent
    // we assume landVis is between 0 and 1
    Assert (landVis>=0 && landVis<=1);
    float landAud = 1 - (1-landVis)*0.90f;
    float audRadiusCoef=5000;
    // we assume bigger objects are also more noisy
    float audRadius = ai->GetRadius();
    float sizeAud = dist2 > 0 ? audRadius * audRadiusCoef / dist2 : 1000;
    saturateMin(sizeAud,240);

    aud*=sizeAud;
    aud*=landAud;
    aud*=_surfaceAudibility;
    *audibility = aud;
  }
  return vis;
}

//#if _ENABLE_CHEATS
//#include <El/Statistics/statistics.hpp>
//#endif

/*!
\patch 1.01 Date 06/11/2001 by Ondra
- Fixed: too low visibility/spotability of near targets.
- Most problems were related to too strict clamping of near values
- Following updates were made:
- near spotability (day & night) is more sensitive, especially on lying soldiers
- night vision is more sensitive
- recognition in night is less sensitive
\patch 1.01 Date 6/26/2001 by Ondra.
- Fixed: Too fast target forgetting.
\patch 1.01 Date 6/26/2001 by Ondra.
- Improved: AI takes NV goggles on/off.
\patch 1.27 Date 10/18/2001 by Ondra.
- Fixed: AI ability to distinguish enemy from friendly by ear was too good.
\patch 1.28 Date 10/19/2001 by Ondra.
- Improved: AI reaction to weapon sound.
\patch 1.28 Date 10/22/2001 by Ondra.
- Fixed: AI: When hearing target that has been seen and then not seen
for some time, position information was too accurate.
\patch 1.28 Date 10/22/2001 by Ondra.
- Fixed: AI: When target was very loud, is was sometimes
considered audible but not visible.
\patch 1.30 Date 11/02/2001 by Ondra.
- Fixed: Vehicles destroyed and later repaired with setdammage 0
were considered dead by AI.
\patch 1.33 Date 11/28/2001 by Ondra.
- Fixed: Detecting laser targets not in front of vehicle.
\patch 1.33 Date 11/30/2001 by Ondra.
- Improved: Low skill soldier have less sensitive eyes and ears.
This make game easier when playing against low skill enemy and
makes difference between low and high skill more important.
\patch_internal 1.59 Date 5/22/2002 by Ondra
- Fixed: AI sensitivity was very high, seeing even back (introduced in 1.58).
\patch 1.85 Date 9/21/2002 by Ondra
- Fixed: More realistic side detection by AI, should make stealing enemy equipment easier.
\patch 1.85 Date 9/21/2002 by Ondra
- Fixed: AI often fired on empty targets.
*/
TargetNormal::TrackContext *EntityAIFull::InitTrackContext( TargetList &res, AIBrain * unit, int canSee, bool initialize, float maxDist )
{
  TargetNormal::TrackContext &ctx = res.InitTrackContext(unit);
  if (!ctx.IsValid()) return NULL;

  ctx.vehicle = this;
  ctx.type = GetType();

  // check which vehicles are visible from this one
  //const float maxVisibility=TACTICAL_VISIBILITY;

  //Assert( GLOB_WORLD->CheckVehicleStructure() );

  //int id=brain->ID()-1;


  // set default zoom - corresponding to r-button zoom
  ctx.opticsFov = 0.30;
  ctx.opticsZoom = 1;
  ctx.cosOpt = 0.95393920142; // no optics - cos 17 deg
  ctx.cosEye=0.7f; // cos 45 deg
  ctx.cosEyePeripheral = (canSee&CanSeePeripheral) ? 0.5f : ctx.cosEye; // cos 60 deg
  ctx.cosEyeFocus = 0.96592582629f; // cos 15 deg (r-button zoom ability)
  ctx.initialize = initialize;
  ctx.thermalOptics = 1;

  if (canSee&CanSeeOptics)
  {
    // different vehicles have different optics zoom
    // check max. zoom or check zoom of current weapon
    TurretContext context;
    if (FindTurret(unit ? unit->GetPerson() : NULL, context))
    {
      WeaponModePtr selected = context._weapons->ValidatedCurrentWeapon();

      if (selected.IsValid() && EnableViewThroughOptics())
      {
        // if we are manual, check current optics
        // TODO: check if current weapon is binocular
        // if it is, check if binoculars are enabled
        const OpticsInfo *opticsInfo = selected.GetOpticsInfo();
        if (opticsInfo)
        {
          const MuzzleType *muzzle = selected.GetMuzzleType();
          if (muzzle) ctx.opticsFov = opticsInfo->_opticsZoomInit;
          //with Thermal optics, gunner has ?2x? higher chance to spot somebody
          ctx.thermalOptics = opticsInfo->HasVisionMode(VMTi)? 2 : 1; 
        }
      }
    }

    if (ctx.opticsFov<0.7)
    {
      // calculate max. cos based on fov
      // fov is sin of angle
      ctx.cosOpt = sqrt(1-ctx.opticsFov*ctx.opticsFov);
      // calculate zoom in terms of area
      ctx.opticsZoom = Square(floatMax(0.70/ctx.opticsFov,1));
      /*
      LogF
      (
      "%s: fov %.2f, zoom %.2f, cos %.3f, ang %.3f",
      (const char *)GetDebugName(),
      opticsFov,opticsZoom,cosOpt,acos(cosOpt)*(180/H_PI)
      );
      */
    }
  }

  if (!(canSee&CanSeeEye))
  {
    // some units cannot see with eye, disable eye to optics level
    // if there is no optices, eye is disabled completely
    ctx.cosEye = ctx.cosOpt;
  }

  ctx.sensitivityEar = GetType()->_sensitivityEar;
  if (!(canSee&CanSeeEar))
  {
    ctx.sensitivityEar = 0;
  }

  ctx.notManual = unit->HasAI() && !unit->GetPerson()->IsRemotePlayer();
  ctx.nearestEnemy2=1e10;

  ctx.irRange = GetType()->GetIRScanRange();
  if (!(canSee&CanSeeRadar)) ctx.irRange=0;

  ctx.maxDist=maxDist;
  saturateMax(ctx.maxDist,ctx.irRange);
  saturateMax(ctx.maxDist,TACTICAL_VISIBILITY);

  ctx.invAbilitySpotTime = unit->GetInvAbility(AKSpotTime);
  ctx.invAbilitySpotDistance = unit->GetInvAbility(AKSpotDistance);
  ctx.deadDetectTime = ctx.invAbilitySpotTime*0.3f+0.75f;
  ctx.targets = &res;

  const LightSun *sun = GScene->MainLight();
  ctx.night=sun->NightEffect();
  if (ctx.night>0)
  {
    // check light intensity
    // with light brightness 0.02 we can recognize shapes, not colors
    // this allows for some enemy identification, but somewhat harder
    float dark = 1.02-sun->GetAmbient().Brightness()-sun->GetDiffuse().Brightness();
    saturateMin(ctx.night,dark);

    // check if soldier has NV goggles
    //bool hasNV = unit->GetPerson()->IsNVEnabled() || GetType()->GetNightVision();
    Person *person = unit->GetPerson();
    bool hasNV =
      (
      person->IsNVEnabled() && person->IsNVWanted() || GetType()->GetNightVision()
      );

    if (hasNV || person->HasTiOptics())
    {
      saturateMin(ctx.night,0.75f);
    }
    else
    {
      // 1 Oct 2008: dark = 1.01979
      // 11 Oct 2008: dark = 0.975
      static float darkestNight = 0.999f; // was 0.997 until rev. 70321
      saturateMin(ctx.night,darkestNight);
    }
  }

  ctx.wepDir = FutureVisualState().Direction();
  ctx.eDir = FutureVisualState().Direction();

  Person *gunner = unit ? unit->GetPerson() : NULL;
  TurretContext context;
  if (FindTurret(gunner, context))
  {
    WeaponModePtr selected = context._weapons->ValidatedCurrentWeapon();
    if (selected.IsValid())
    {
      ctx.wepDir = GetWeaponDirection(FutureVisualState(), context, selected);
    }
    ctx.eDir = gunner->GetLookAroundDirection(FutureVisualState(), context);
    //ctx.eDir = GetLookAroundDirection(FutureVisualState(), context);
  }
  return &ctx;
}

void EntityAIFull::TrackTargets(
  TargetList &res, AIBrain *unit, int canSee,
  bool initialize, float maxDist,
  float trackTargetsPeriod[NTgtCategory]
)
{
  if (!canSee) return; // nothing to test

  PROFILE_SCOPE_EX(tgTrk,ai);

  // track targets for single unit (gunner or commander)
  TargetNormal::TrackContext *ctx = InitTrackContext(res, unit, canSee, initialize, maxDist);
  if (ctx)
  {
    res.TrackTargets(unit,this,trackTargetsPeriod,maxDist,*ctx);

    ctx->CheckAdditionalTargetDatabases();

    _nearestEnemy=sqrt(ctx->nearestEnemy2);
  }
}

/*!
\patch 5142 Date 3/19/2007 by Ondra
- Fixed: Improved target scanning for fast moving units.
*/

void EntityAIFull::TrackOneTarget(TargetList &res, Target *target, float trackTargetsPeriod)
{
  const EntityAIType *type = GetType();
  AIBrain *unit=EffectiveObserverUnit();
  if (unit)
  {
    float maxDist = TACTICAL_VISIBILITY;
    TargetNormal::TrackContext *ctx = InitTrackContext(res,unit,type->_commanderCanSee,false,maxDist);
    if (ctx)
    {
      TrackOneTarget(target,unit,trackTargetsPeriod,maxDist,*ctx);
    }
  }
}

void TargetList::DistributeTargets(TgtCategory from)
{
  // array of all categories
  RefArray<TargetNormal> &fromList = _list[from];
  int dst = 0;
  for (int src=0; src<fromList.Size(); src++)
  {
    TargetNormal *tgt = fromList[src];
    // check which category this target should belong to
    TgtCategory cat = tgt->category;
    if (cat!=from)
    {
      // move into a different list
      RefArray<TargetNormal> &tgtList = _list[cat];
      tgtList.Add(tgt);
      continue;
    }
    fromList[dst++]=fromList[src];
  }
  fromList.Resize(dst);
}

void TargetList::TrackTargets(
  AIBrain *unit, EntityAIFull *veh,
  float trackTargetsPeriod[NTgtCategory], float maxDist, TargetNormal::TrackContext &ctx
)
{
  // TODO: remove scopes, use loop
  // scan first, distribute later
  // we do not want one item to be scanned twice because of being distribute meanwhile
  if (trackTargetsPeriod[TgtCatEnemy]>=0)
  {
    PROFILE_SCOPE_EX(tgTrE,ai);
    veh->TrackTargetsPart(_list[TgtCatEnemy],unit,trackTargetsPeriod[TgtCatEnemy],maxDist,ctx);
  }
  if (trackTargetsPeriod[TgtCatEnemyUnknown]>=0)
  {
    PROFILE_SCOPE_EX(tgTrU,ai);
    veh->TrackTargetsPart(_list[TgtCatEnemyUnknown],unit,trackTargetsPeriod[TgtCatEnemyUnknown],maxDist,ctx);
  }
  //TODO: Sides are not used anymore. Remove all relevant code
/*  if (trackTargetsPeriod[TgtCatFriendly]>=0)
  {
    PROFILE_SCOPE_EX(tgTrF,ai);
    veh->TrackTargetsPart(_list[TgtCatFriendly],unit,trackTargetsPeriod[TgtCatFriendly],maxDist,ctx);
  }*/
  // now make sure targets are distributed as needed
  for (int i=0; i<NTgtCategory; i++)
  {
    if (trackTargetsPeriod[i]>=0)
    {
      PROFILE_SCOPE_EX(tgTrD,ai);
      DistributeTargets((TgtCategory)i);
    }
  }
}

#if _ENABLE_CONVERSATION


  struct FindTarget
  {
    const Target *t;
    const TargetKnowledge *&found;
    FindTarget(const Target *t, const TargetKnowledge *&found):t(t),found(found){}
    bool operator ()(const TargetKnowledge *tgt) const
    {
      if (tgt->idExact && tgt->idExact==t->idExact)
      {
        found = tgt;
        return true;
      }
      return false;
    }
};

bool TargetKBList::Find(const Target *t) const
{
  const TargetKnowledge *found = NULL;
  return base::ForEachF(FindTarget(t,found));
}
#endif

void EntityAIFull::TrackMyVehicle(TargetNormal *target, AIBrain *unit, float trackTargetsPeriod, const SideInfo &ctx)
{
  EntityAI *ai=target->idExact;
  if( !ai ) return;

  Assert(ai==this);
  
  if( ai==this )
  {
    target->SeesItself(unit,ctx,trackTargetsPeriod);
  }

}

void EntityAIFull::TrackOneTarget(
  Target *target, AIBrain *unit,
  float trackTargetsPeriod, float maxDist, TargetNormal::TrackContext &ctx
)
{
  // TODO: avoid dynamic cast here
  TargetNormal *tgt = dynamic_cast<TargetNormal *>(target);
  // select needs to be a part of the active target list
  if(!tgt) return;

  EntityAI *ai=tgt->idExact;
  if( !ai ) return;

  // update visibility information
  // distant objects are less visible
  if( ai==this )
  {
    tgt->SeesItself(unit,ctx,trackTargetsPeriod);
    return;
  }
  if (!ai->IsInLandscape()) return; // cannot be visible - is not in landscape

  // check if target is within tracked range
  float dist2=ai->FutureVisualState().Position().Distance2Inline(FutureVisualState().Position());
  if (dist2>Square(maxDist)) return;

  tgt->Sees(unit,dist2,ctx,trackTargetsPeriod);

}

/**
Traverse one category of targets
*/

void EntityAIFull::TrackTargetsPart(
  RefArray<TargetNormal> &tgt, AIBrain *unit,
  float trackTargetsPeriod, float maxDist, TargetNormal::TrackContext &ctx
)
{
  for( int index=0; index<tgt.Size(); index++ )
  {
    TargetNormal *target=tgt[index];
    EntityAI *ai=target->idExact;
    if( !ai ) continue;
    
    // update visibility information
    // distant objects are less visible
    if( ai==this )
    {
      target->SeesItself(unit,ctx,trackTargetsPeriod);
      continue;
    }
    if (!ai->IsInLandscape()) continue; // cannot be visible - is not in landscape

    // check if target is within tracked range
    float dist2=ai->FutureVisualState().Position().Distance2Inline(FutureVisualState().Position());
    if (dist2>Square(maxDist)) continue;

    PROFILE_SCOPE_DETAIL_EX(tgTgS,ai);
    #if _PROFILE
    if (PROFILE_SCOPE_NAME(tgTgS).IsActive())
    {
      PROFILE_SCOPE_NAME(tgTgS).AddMoreInfo(Format("%s -> %s",cc_cast(GetType()->GetName()),cc_cast(ai->GetType()->GetName())));
    }
    #endif

    target->Sees(unit,dist2,ctx,trackTargetsPeriod);

  } // for( all vehicles )

}

/*!
\param res target list to track
\param initialize set when initializing. When true, situation is considered steady
and all values should be set as it was the same for very long time.
\param trackTargetsPeriod how long it is expected
until next TrackTargets will execute
*/

void EntityAIFull::TrackTargets(
  TargetList &res, bool initialize, float trackTargetsPeriod[NTgtCategory]
)
{
  const EntityAIType *type = GetType();
  AIBrain *unit=EffectiveObserverUnit();
  if (unit)
  {
    TrackTargets
    (
      res,unit,type->_commanderCanSee,initialize,1e10,trackTargetsPeriod
    );
  }

}

const float TrackNearPeriod = 1.0f;

/*!\note
When some enemy is near, it is called often,
in every AIGroup::Think (about 0.1 sec)
*/

void EntityAIFull::TrackNearTargets(TargetList &res, float maxDist)
{
  const EntityAIType *type = GetType();
  AIBrain *unit=EffectiveObserverUnit();
  // avoid using zero period, as this could make targets invalid when moving
  // across the distance border
  // we want to scan enemies only here
  if (unit)
  {
    // we want also to find new enemies
    static float trackTargetsPeriod[]=
    {
      TrackNearPeriod, // TgtCatEnemy
      TrackNearPeriod, // TgtCatEnemyUnknown
  //TODO: Sides are not used anymore. Remove all relevant code
//      -1.0f, // TgtCatFriendly
    };
    Assert(lenof(trackTargetsPeriod)==NTgtCategory);
    TrackTargets(res,unit,type->_commanderCanSee,false,maxDist,trackTargetsPeriod);
  }
}

void EntityAIFull::TrackLockedTargets(TargetList &res)
{
  // track assigned target or fire target
  Target *oneTarget = NULL;
  // first try to check what are we firing at
  if (!oneTarget)
  {
    AIBrain *unit=EffectiveObserverUnit();
    if (unit)
    {
      oneTarget = unit->GetTargetAssigned();
    }
  }
  if (oneTarget)
  {
    TrackOneTarget(res,oneTarget,TrackNearPeriod);
  }
}

/*!
  Perform target tracking
  Update target list to maintain actual visibility and spotability
  information about all potentially visible targets.
  (uses EntityAI::AddNewTargets and EntityAI::TrackTargets)
*/

void EntityAIFull::WhatIsVisible( TargetList &res, bool initialize )
{
  if (initialize)
  {
    float trackAllPeriod[NTgtCategory]=
    {
      1.0f, // TgtCatEnemy
      1.0f, // TgtCatEnemyUnknown
  //TODO: Sides are not used anymore. Remove all relevant code
      //1.0f, // TgtCatFriendly
    };
    AddNewTargets(res,initialize);
    TrackTargets(res,initialize,trackAllPeriod);
    return;
  }
  // note:
  // this function is called only from CreateTargetList
  // CreateTargetList itself is called 
  // once per 1 second in Combat mode

  AIBrain *unit = EffectiveObserverUnit();
  if (!unit) return;

  float newTargetsPeriod = 1.0f;
  float trackTargetsPeriod[NTgtCategory]=
  {
    1.0f, // TgtCatEnemy
    2.0f, // TgtCatEnemyUnknown
  //TODO: Sides are not used anymore. Remove all relevant code
    //5.0f, // TgtCatFriendly
  };
  
  if (GetType()->_irScanRangeMax<=0) newTargetsPeriod = 5.0f;

  float trackRange = floatMax(GetType()->GetIRScanRange(),TACTICAL_VISIBILITY);


  // check if there is some enemy in tracked range
  if (unit->GetNearestEnemyDist2()>Square(trackRange))
  {
    // track 5x less when no enemy is near
    newTargetsPeriod *= 5.0f;
  }

  // TODO: perform AddNewTargets for all units at once
  if (Glob.time>_newTargetsTime+newTargetsPeriod)
  {
    AddNewTargets(res,initialize);
  }
  // disable categories which did not reach their time yet
  bool doAny = false;
  for (int i=0; i<NTgtCategory; i++)
  {
    if (Glob.time<=_trackTargetsTime[i]+trackTargetsPeriod[i])
    {
      trackTargetsPeriod[i] = -1.0f;
    }
    else
    {
      _trackTargetsTime[i] = Glob.time;
      doAny = true;
    }
  }
  
  if (doAny)
  {
    TrackTargets(res,initialize,trackTargetsPeriod);
  }

  // note: new targets are added once for all units the vehicle
}


VisibilityTracker::VisibilityTracker()
{
}

VisibilityTracker::VisibilityTracker( EntityAI *obj )
:_obj(obj),_lastTime(Glob.time-60),_lastTimeForLastVisiblePos(TIME_MIN),_lastVisiblePositionValid(false)
{
}

VisibilityTracker::~VisibilityTracker()
{
}


/*!
\patch 1.31 Date 11/23/2001 by Ondra.
- Fixed: When firing, line of fire between weapon and target is checked.
Line between eye and target was checked before, which sometimes
could lead to friendly fire.
*/

float VisibilityTracker::Value(
  bool &headOnly,
  const EntityAIFull *sensor, const TurretContext &context, const WeaponModePtr &ptr, float reserve, float maxDelay
)
{
  // sensor and weapon are assumed const (tracker hard linked to sensor)
  if (!_obj)
  {
    LogF("VisibilityTracker expired");
    headOnly = _lastHeadOnly;
    return _lastValue;
  }
  if (_lastTime<Glob.time-maxDelay)
  {
    _lastTime=Glob.time;

    // check if it is within tactical range
    EntityAI *ai = _obj;
    float irRange = sensor->GetType()->GetIRScanRange();
    if (!ai->GetType()->GetIRTarget()) irRange = 0;
    else if (!sensor->GetType()->GetIRScanGround() && !ai->Airborne()) irRange = 0;
    float maxDistance2=Square(floatMax(irRange,TACTICAL_VISIBILITY));
    float visible=0;
    bool headOnly = false;
    if( sensor->FutureVisualState().Position().Distance2(ai->FutureVisualState().Position())<maxDistance2 )
    {
      // in OFP 1.33 VisibilityTime was used here
      // this is not very robust, as line of fire is checked here, not line of sight
      // it was moved into GetAimed
      // it was seen in near past. Check if we can fire.
      Vector3 weaponPos;
      if (!ptr.IsValid())
      {
        weaponPos = sensor->EyePosition(sensor->FutureVisualState());
      }
      else
      {
        weaponPos = sensor->FutureVisualState().PositionModelToWorld(sensor->GetWeaponPoint(sensor->FutureVisualState(),context, ptr));
      }
      visible = GLandscape->Visible(headOnly,weaponPos,sensor,ai,reserve,ObjIntersectFire);
    }
    _lastHeadOnly = headOnly;
    _lastValue = visible;
    if (_lastValue>0.5f)
    {
      _lastVisiblePosition = headOnly ? ai->AimingPositionHeadShot(ai->FutureVisualState()) : ai->AimingPosition(ai->FutureVisualState());
      _lastVisiblePositionValid = true;
      _lastVisiblePositionTime = Glob.time;
    }
    return visible;
  }
  headOnly = _lastHeadOnly;
  return _lastValue;
}

float VisibilityTracker::ValueForLastVisible(
  Time &time, const EntityAIFull *sensor, const TurretContext &context, const WeaponModePtr &ptr, float reserve, float maxDelay
)
{
  // sensor and weapon are assumed const (tracker hard linked to sensor)
  // if there is no last visible position, we cannot return any data for it
  if (!_lastVisiblePositionValid) return 0;

  if (!_obj)
  {
    LogF("VisibilityTracker expired");
    time = _lastVisiblePositionTime;
    return _lastValueForLastVisiblePos;
  }

  if (_lastTimeForLastVisiblePos<Glob.time-maxDelay)
  {
    _lastTimeForLastVisiblePos=Glob.time;

    // check if it is within tactical range
    EntityAI *ai = _obj;
    // visual range only
    float maxDistance2=Square(TACTICAL_VISIBILITY);
    float visible=0;
    if( sensor->FutureVisualState().Position().Distance2(ai->FutureVisualState().Position())<maxDistance2 )
    {
      // in OFP 1.33 VisibilityTime was used here
      // this is not very robust, as line of fire is checked here, not line of sight
      // it was moved into GetAimed
      // it was seen in near past. Check if we can fire.
      // if weapon is not selected, no need to test for a suppressive fire
      if (ptr.IsValid())
      {
        Vector3 weaponPos = sensor->FutureVisualState().PositionModelToWorld(sensor->GetWeaponPoint(sensor->FutureVisualState(), context, ptr));
        visible=GLandscape->Visible(sensor->GetFutureVisualStateAge(),weaponPos,_lastVisiblePosition,ai->VisibleSize(ai->FutureVisualState()),sensor,ai,ObjIntersectFire);
      }
    }
    _lastValueForLastVisiblePos=visible;
  }
  time = _lastVisiblePositionTime;
  return _lastValueForLastVisiblePos;
}

VisibilityTrackerCache::VisibilityTrackerCache()
{
}

void VisibilityTrackerCache::Clear()
{
  _trackers.Clear();
}

VisibilityTrackerCache::~VisibilityTrackerCache()
{
}

float VisibilityTrackerCache::CheckKnownValue(
  bool &headOnly, 
  const EntityAIFull *sensor, const TurretContext &context, const WeaponModePtr &ptr, EntityAI *obj, float reserve, float maxDelay
)
{
  DoAssert( obj );
  // check cached results
  for( int i=0; i<_trackers.Size(); i++ )
  {
    VisibilityTracker &tracker=_trackers[i];
    if (tracker._obj==obj && tracker._turret==context._turret)
    {
      // if there is a recent negative about last visible position, we do not test direct visibility at all
      if (tracker._lastTimeForLastVisiblePos>=Glob.time-maxDelay && tracker._lastValueForLastVisiblePos<=0.01f)
      {
        return 0;
      }

      // check this one
      if( tracker._lastTime>=Glob.time-maxDelay )
      {
        headOnly = tracker._lastHeadOnly;
        return tracker._lastValue;
      }
      return tracker.Value(headOnly,sensor,context,ptr,reserve,maxDelay);
    }
  }
  return -1;
}

/*!
\patch 1.27 Date 10/12/2001 by Ondra.
- Fixed: bug in visibility management
  that could cause excessive memory allocation during mission.
*/

float VisibilityTrackerCache::KnownValue(
  bool &headOnly, 
  const EntityAIFull *sensor, const TurretContext &context, const WeaponModePtr &ptr, EntityAI *obj, float reserve, float maxDelay
)
{
  // check cached results
  float ret = CheckKnownValue(headOnly,sensor,context,ptr,obj,reserve,maxDelay);
  if (ret>=0)
  {
    return ret;
  }
  if (_trackers.Size()<=4)
  {
    // create a new tracker if needed, but only if there are not too many trackers yet
    // this makes this function somewhat more lightweight than Value
    VisibilityTracker &tracker=_trackers[_trackers.Add(obj)];
    tracker._turret = context._turret;
    return tracker.Value(headOnly,sensor,context,ptr,reserve,maxDelay);
  }
  return -1;
}

float VisibilityTrackerCache::Value(
  bool &headOnly, 
  const EntityAIFull *sensor, const TurretContext &context, const WeaponModePtr &ptr, EntityAI *obj, float reserve, float maxDelay
)
{
  // check cached results
  float ret = CheckKnownValue(headOnly,sensor,context,ptr,obj,reserve,maxDelay);
  if (ret>=0)
  {
    return ret;
  }

  // before creating a new tracker discard the old ones 
  const float discardAfter = 20.0f;
  for( int i=0; i<_trackers.Size(); i++ )
  {
    VisibilityTracker &tracker=_trackers[i];
    if( Glob.time-tracker._lastTime>discardAfter || !tracker._obj )
    {
      // if tracker is out of date, remove it
      _trackers.Delete(i),i--;
    }
  }
  
  // create a new tracker
  VisibilityTracker &tracker=_trackers[_trackers.Add(obj)];
  tracker._turret = context._turret;
  return tracker.Value(headOnly,sensor,context,ptr,reserve,maxDelay);
}

float VisibilityTrackerCache::ValueForLastVisible(
  Time &time, const EntityAIFull *sensor, const TurretContext &context, const WeaponModePtr &ptr, EntityAI *obj,
  float reserve, float maxDelay
)
{
  DoAssert( obj );
  // check cached results
  for( int i=0; i<_trackers.Size(); i++ )
  {
    VisibilityTracker &tracker=_trackers[i];
    if( tracker._obj==obj && tracker._turret==context._turret)
    {
      // if there is positive direct visibility information which is recent enough, we may use it
      if (tracker._lastValue>0.5 && tracker._lastTime>tracker._lastTimeForLastVisiblePos && tracker._lastTime>=Glob.time-maxDelay)
      {
        time = tracker._lastTime;
        return tracker._lastValue;
      }

      // otherwise check last pos visibility only
      if (tracker._lastTimeForLastVisiblePos>=Glob.time-maxDelay)
      {
        time = tracker._lastVisiblePositionTime;
        return tracker._lastValueForLastVisiblePos;
      }

      return tracker.ValueForLastVisible(time,sensor,context,ptr,reserve,maxDelay);

    }
  }

  // creating a new tracker has no sense here - if there is no last visible pos, there is nothing to test
  return 0;
}


bool VisibilityTrackerCache::GetLastKnownVisiblePosition(Vector3 &ret, EntityAI *obj) const
{
  DoAssert( obj );
  // check cached results
  for( int i=0; i<_trackers.Size(); i++ )
  {
    const VisibilityTracker &tracker=_trackers[i];
    // we do not care which turret is this from
    // consider: with multiple turrets we might search for most recent info based on _lastVisiblePositionTime
    if( tracker._obj==obj && tracker._lastVisiblePositionValid)
    {
      ret = tracker._lastVisiblePosition;
      return true;
    }
  }
  return false;
}

const float TimeToAttack=5.0f; // attack aim time
const float MinTimeToLive=5.0f; // see also AIBrain::GetTimeToLive


/// Functor checking if direction is in fire angle of some weapons
class FireAngleInRangeFunc : public ITurretFunc
{
protected:
  float &_inRange; // out
  Vector3 _direction;

public:
  FireAngleInRangeFunc(float &inRange, Vector3Par direction) : _inRange(inRange), _direction(direction) {}
  bool operator () (EntityAIFull *entity, TurretContext &context)
  {
    float inRange = entity->FireAngleInRange(context, WeaponModePtr(), _direction);
    saturateMax(_inRange, inRange);
    return false; // continue
  }
};

/// Functor checking visibility from some weapons
class VisibleFunc : public ITurretFunc
{
protected:
  float &_visible; // out
  Matrix4Val _transform;
  const EntityAIFull *_who;
  EntityAI *_ai;

public:
  VisibleFunc(float &visible, Matrix4Val transform, const EntityAIFull *who, EntityAI *ai)
    : _visible(visible), _transform(transform), _who(who), _ai(ai) {}
  bool operator () (EntityAIFull *entity, TurretContext &context)
  {
    Vector3Val weaponPos = _transform * entity->GetWeaponCenter(entity->FutureVisualState(), context, WeaponModePtr());
    bool headOnly = false;
    float visible = GLandscape->Visible(headOnly,weaponPos, _who, _ai, 1.5);
    saturateMax(_visible, visible);
    return false; // continue
  }
};

/*
possible sutiable functions 
http://en.wikipedia.org/wiki/Logistic_function
http://en.wikipedia.org/wiki/Sigmoid_function
plot x/sqrt(1+x^2), 2/pi*arctan((pi/2)x), x/(1+|x|), 1-1/(1+exp(x*2))*2
http://www.wolframalpha.com/input/?i=plot+x%2Fsqrt%281%2Bx%5E2%29%2C+2%2Fpi*arctan%28%28pi%2F2%29x%29%2C+x%2F%281%2B%7Cx%7C%29%2C+1-1%2F%281%2Bexp%28x*2%29%29*2
*/

/** usefull when we need to avoid high values, but do not want to clamp them
Like when we still want high values to be sorted, only with less importance given to the position cost
*/
    
static float CostSigmoidFunction(float x)
{
  return x*InvSqrt(1+x*x);
}


static float ChangeCost(const EntityAIFull *who, Vector3Par bestAggressivePos, Vector3Par attackAggressivePos, float mySecondCost, float myCost)
{
  // find a point on the "old path" (line from current position to the currently planned position) and estimate a cost to it
  Vector3 pos = who->FutureVisualState().Position();
  Vector3 onLine = NearestPoint(pos,attackAggressivePos,bestAggressivePos);
  return CostSigmoidFunction(who->EstimateTravelCost(bestAggressivePos,onLine,mySecondCost)/myCost)*myCost*0.5f;
}

ShowDiags GDiagsShown;

ShowDiags::ShowDiags()
{
  _handle = 0;
}

void ShowDiagDelegateDefault::Draw()
{
  if (_obj->RenderVisualState().Scale()>1e-3f) // no need to show zero sized or very small objects
  {
    GScene->ObjectForDrawing(_obj);    
    GScene->DrawDiagText(_text, _textPos, _textPrior, _textSize);
  }
}

Vector3 ShowDiagDelegateDefault::Pos() const
{
  return _obj->FutureVisualState().Position();
}

void ShowDiags::ShowArrow(int &handle, int id, float timeout, RString text, Vector3Par pos, Vector3Par dir, float size, ColorVal color, float textSize)
{
  ShowDiagDelegateDefault *show = new ShowDiagDelegateDefault;
  
  LODShape *forceArrow=GScene->ForceArrow();
  show->_obj = new ObjectColored(forceArrow,VISITOR_NO_ID);

  show->_text = text;
  show->_textPos = pos+dir.Normalized()*size*0.8;
  show->_textSize = textSize;
  // make priority dependent on the arrow size and the text size
  float prior = size * 10;
  saturate(prior, 2, 20);
  show->_textPrior = toInt(textSize * prior);
  
  float length = forceArrow->Max()[2] - forceArrow->Min()[2];
  float scale = size/length;

  Vector3 aside=dir.CrossProduct(VAside);
  Vector3 zAside=dir.CrossProduct(VForward);
  if( zAside.SquareSize()>aside.SquareSize() ) aside=zAside;
  show->_obj->SetPosition(pos);
  show->_obj->SetOrient(dir,aside);
  show->_obj->SetPosition(show->_obj->RenderVisualState().PositionModelToWorld(forceArrow->BoundingCenter()*scale));
  show->_obj->SetScale(scale);
  show->_obj->SetConstantColor(color);
  
  Show(handle,id,timeout,show);
}
  
void ShowDiags::Show(int &handle, int id, float timeout, IShowDiagDelegate *show)
{
  int index = -1;
  if (handle<=0)
  {
    // first valid handle is 1, 0 is invalid (this is done so that static int handle; provides invalid handle)
    handle = ++_handle;
  }
  // first check if the handle,id combination has a slot assigned
  index = FindHandle(handle,id);
  if (index<0)
  {
    // new slot needed
    index = _diags.Add();
    _diags[index]._handle = handle;
    _diags[index]._id = id;
  }
  ShowDiagInfo &info = _diags[index];

  info._hideTime = Glob.time+timeout;
  info._show = show;
}

bool ShowDiags::IsDiagXZ(Vector3Par pos, float maxDist) const
{
  for (int i=0; i<_diags.Size(); i++)
  {
    Vector3Par dPos = _diags[i]._show->Pos();
    if (dPos.DistanceXZ2(pos)<Square(maxDist)) return true;
  }
  return false;
}

void ShowDiags::Render()
{
  // remove expired diags
  for (int i=0; i<_diags.Size(); i++)
  {
    ShowDiagInfo &info = _diags[i];
    if (info._hideTime<Glob.time)
    {
      _diags.Delete(i);
      i--;
    }
  }
  // submit all diagnostics for rendering
  for (int i=0; i<_diags.Size(); i++)
  {
    const ShowDiagInfo &info = _diags[i];
    if (info._show) info._show->Draw();
    }
  }

void ShowDiags::Reset()
{
  _diags.Clear();
  // we cannot reset handles - the are used by static variables
}

#define HIDE_DIAGS 0


Target *EntityAIFull::FindNearestEnemy(Vector3Par pos)
{
  AIBrain *unit = CommanderUnit();
  if (!unit) return NULL;

  Target *target = NULL;
  const TargetList *targetList = unit->AccessTargetList();
  if (targetList)
  {
    float minFunc = 1e10;
    // in stealth we need to track Unknown as well
    // enemy list currently contains enemies which were not identified yet
    for (int i=0; i<targetList->EnemyCount(); i++)
    {
      Target *tar = targetList->GetEnemy(i);
      if (!tar->idExact) continue;
      if (tar->idExact->IsDamageDestroyed()) continue;
      if (tar->IsVanishedOrDestroyed()) continue;
      if (!tar->IsKnownBy(unit) ) continue;
      // do not hide in front of yourself (could happen with renegade status)
      if (tar->idExact==unit->GetVehicle()) continue;
      // in stealth hide also from unknown targets
      TargetSide side = tar->GetSide();
      if( unit->GetCombatMode()!=CMStealth )
      {
        if (!unit->IsEnemy(side)) break;
      }
      else
      {
        if (!unit->IsEnemy(side) && side!=TSideUnknown ) break;
      }

      // TODO: better hide target selection
      float func=tar->AimingPosition().Distance(pos);
      if( func<minFunc )
      {
        minFunc = func;
        target  = tar;
      }
    }
  }
  return target;
}

#if _VBS3
float EntityAIFull::GetTotalWeightKg()
{
  float weight = base::GetTotalWeightKg();

  for (int i=0; i<_weaponsState._weapons.Size(); i++)
  {
    const WeaponType *w = _weaponsState._weapons[i];
    if (!w) continue;
    weight += w->GetMass();
  }
  for (int i=0; i<_weaponsState._magazines.Size(); i++)
  {
    const Magazine *m = _weaponsState._magazines[i];
    if (!m) continue;
    const MagazineType *mType = m->_type;
    if (!mType || mType->_maxAmmo == 0) continue;      
    float ammoRatio = (float)m->GetAmmo() / (float)mType->_maxAmmo;
    weight += ammoRatio * mType->_mass;
  }

  return weight;
}
#endif

bool EntityAIFull::CheckTargetKnown(const TurretContext &context, Target * target) const
{
  if (context._gunner)
  {
    AIBrain *brain = context._gunner->Brain();
    if (!brain || !brain->LSIsAlive() || !target->IsKnownBy(brain))
    {
      // we cannot aim until we know the target
      return false;
    }
  }
  return true;
}



float EntityAIFull::FireAngleInRange(const TurretContext &context, const WeaponModePtr &ptr, Vector3Par rel) const
{
  // most vehicle cannot fire high and low
  float size2=rel.SquareSizeXZ();
  if (size2 == 0.0f) return 0;
  float y2=Square(rel.Y());
  const float maxY=0.5;
  if( y2>size2*Square(maxY) ) return 0;
  // nearly same level
  return 1-fabs(rel.Y())*InvSqrt(size2)*(1/maxY);
}

void EntityAIFull::FireAimingLimits(float &minAngle, float &maxAngle, const TurretContext &context, const WeaponModePtr &ptr) const
{
  // by default assume no aiming is possible and body needs to be turned
  // this is safest setting as it makes sure all vehicles can engage their targets
  minAngle = maxAngle = 0;
}

/**
@param seenBefore [out] how much time elapsed since we had clean line of sight to the target
*/
float EntityAIFull::GetAimingPosition(Vector3 &pos, Vector3 &speed, float &seenBefore,
  bool exact, const TurretContext &context, Target *target, const AmmoType *ammo) const
{
  seenBefore = FLT_MAX;
  if (!context._gunner)
    return 0;
  AIBrain *brain = context._gunner->Brain();
  if (!brain)
    return 0;
  if (exact)
  {
    pos = target->idExact->AimingPosition(target->idExact->RenderVisualState());
    speed = target->idExact->PredictVelocity();
    seenBefore = 0;
    return 1;
  }
  else if (CheckSuppressiveFire()<SuppressYes)
  {
    bool headOnly = false;
    float visible=0;
    if (target->idExact && target->SeenRecently(brain))
      visible = _visTracker.Value(headOnly,this, context, context._weapons->ValidatedCurrentWeapon(), target->idExact, 0.9f);
    seenBefore = 0;
    pos = target->LandAimingPosition(brain,ammo,headOnly);
    speed = target->GetSpeed(brain);

    pos += brain->ScanTargetOffset(target);

    // decrease landscape occlusion significance (see bellow)
    visible=1-(1-visible)*0.3f;
    return visible;
  }
  else
  {
    bool headOnly = false;
    float visible=0;
    if (target->idExact && target->SeenRecently(brain))
    {
      WeaponModePtr selected = context._weapons->ValidatedCurrentWeapon();
      visible = _visTracker.Value(headOnly,this, context, selected, target->idExact, 0.9f);
      if (visible<0.1f)
      {
        Time time;
        visible  = _visTracker.ValueForLastVisible(time,this,context, selected, target->idExact, 0.9f);
        if (visible>0)
        {
          seenBefore = Glob.time.Diff(time);
        }
      }
      else
        seenBefore = 0;
    }

    bool isExact = false;
    pos = target->LastKnownPosition(brain,ammo,headOnly,&isExact);

    // TODO: consider using ScanTargetOffset when suppressing, but not firing right now
    //pos += brain->ScanTargetOffset(target);

    speed = target->GetSpeed(brain);
    if (!isExact && target->idExact)
    {
      // when LastKnownPosition did not return an exact position, but we have seen the target recently, we want to check _visTracker cache
      Vector3 lastPos;
      bool knownPos = _visTracker.GetLastKnownVisiblePosition(lastPos,target->idExact);
      if (knownPos)
      {
        pos = lastPos;
        // we are aiming at a point in a space, not at the target - point is not moving
        speed = VZero;
      }
    }

    // TODO: improved handling for large or non-spherical objects
    // we should be interested only in the visibility of the area we are aiming at
    // decrease landscape occlusion significance
    visible=1-(1-visible)*0.3f;
    return visible;
  }
}

float EntityAIFull::GetSuppressiveFireBoost() const
{
  return CheckSuppressiveFire()>=SuppressIntense ? 0.33f : 1.0f;
}
/**
@param boost in some situations we want suppressive fire to be more intense. Smaller number means more intense fire.
*/
float EntityAIFull::SuppressiveFireInterval(float seenBefore, const TurretContext &context, float boost) const
{
  // if the target is too old, stop suppressing it
  if (seenBefore>300)
    return FLT_MAX;
  // count ammo which can be used for suppressive fire
  int nSupressiveAmmo = 0;
  context._weapons->ForEachMagazine([&nSupressiveAmmo](Magazine *mag) -> bool
  {
    const MagazineType *type = mag->Type();
    if (!type) return false; // continue
    // we want to suppress using bullets only
    if (!type->_ammo || (type->_ammo->_simulation!=AmmoShotBullet && type->_ammo->_simulation!=AmmoShotSpread))
      return false; // continue
    nSupressiveAmmo += mag->GetAmmo();
    return false; // continue
  });
  float lotOfAmmoFactor = InterpolativC(nSupressiveAmmo,150,300,1.0f,0.5f);
  // the longer the target was not seen, the longer will the pauses between firing be
  // we cannot use GetLastSeen, as the target may be seen by one unit only
  float agePause = seenBefore<30 ? InterpolativC(seenBefore,0,30,0,15) : InterpolativC(seenBefore,30,120,15,30);

  // the less ammo we have, the longer pause between the shots we want
  float ammoLowPause = InterpolativC(nSupressiveAmmo,20.0f,150.0f,15.0f,0.0f);
  // with a lot of ammo, we may want to provide a more reliable suppression even on older targets

#if _ENABLE_CHEATS
  if (CHECK_DIAG(DECombat) && GWorld->GetObserver()==this)
  {
    float maxAge = InterpolativC(nSupressiveAmmo,100,300,60,240);
    DIAG_MESSAGE(1000,Format("%s: Suppressive pause: A %.1f*%.1f, C %.1f (age %.1f<%.1f, ammo %d), last shot %.1f",
      cc_cast(GetDebugName()),agePause,lotOfAmmoFactor,ammoLowPause,seenBefore,maxAge,nSupressiveAmmo,
      Glob.time-context._weapons->_lastShotTime));
  }
#endif

  return floatMax(agePause*lotOfAmmoFactor,ammoLowPause)*boost;
}


float EntityAIFull::GetTrackedVisibility(
  bool &headOnly,
  AIBrain *unit, const TurretContext &context, const WeaponModePtr &ptr, const Target &tgt, float reserve
) const
{
  if (!tgt.SeenRecently(unit))
  {
    return 0;
  }
  // if visibility is tracked, check it
  float visibility = _visTracker.KnownValue(headOnly,this,context,ptr,tgt.idExact,reserve);
  if( visibility<0 )
  {
    // if not, check visibility matrix
    // TODO: add head-only into World::Visibility
    headOnly = false;
    visibility = GWorld->Visibility(unit,tgt.idExact);
  }
  return visibility;
}

/**
TODO: we could precalculate this
*/
float EntityAIType::GetMaxWeaponRange() const
{
  return _weapons.GetMaxRange();
}

Time EntityAI::GetLastShotTime() const
{
  return Glob.time - 120;
} 

bool EntityAIFull::CanFireUsing(const TurretContext &context) const 
{
  return context._weapons->ForEachWeapon([](Weapon *weapon) -> bool
  {
    const WeaponType *type = weapon->Type();
    if (type && type->GetSimulation() != WSCMLauncher) return true; // found
    return false; // continue;
  });
}
