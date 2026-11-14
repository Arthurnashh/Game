/*!
\file
Implementation for Object class
Implements dammage related functions
*/
#include "wpch.hpp"

#include "object.hpp"
#include "shots.hpp"
#include "global.hpp"
#include <El/Common/randomGen.hpp>
#include "world.hpp"
#include "landscape.hpp"
#include "vehicleAI.hpp"
#include "objDestroy.hpp"
#include "AI/ai.hpp"
#include "Network/network.hpp"

#if _PROFILE
# pragma optimize( "", off )
#endif

#if _VBS2 && _ENABLE_CHEATS
#define DAMMAGE_DIAGS 0
#else
#define DAMMAGE_DIAGS 0
#endif

bool Object::CanSmoke() const
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
  return false;
#else
  // cannot smoke underwater
  if(
    GLandscape->GetSeaLevelNoWave() > COMPosition(FutureVisualState()).Y() // low enough
    && GLandscape->SurfaceY(COMPosition(FutureVisualState()))<GLandscape->GetSeaLevelNoWave() // there is a water in that place
  )
    return false;
  return _canSmoke;
#endif
}

IExplosion *Object::GetSmoke() const
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
  return NULL;
#else
  if (!_damage)
    return NULL;
  if (!_damage->_smoke)
    return NULL;
  return _damage->_smoke->GetExplosionInterface();
#endif
}

void Object::SetSmoke(Vehicle *smoke)
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else
  if( !_damage )
    _damage = new DamageRegions;
  /// make sure no smoke is set yet
  Assert(!_damage->_smoke);
  _damage->_smoke = smoke;
  _canSmoke = false;
#endif
}

void Object::DeleteSmoke()
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else
  if (_damage && _damage->_smoke)
    _damage->_smoke->SetDelete();
#endif
}

bool Object::HasGeometry() const
{
  if (!_isDestroyed)
    return true;
  return _destrType!=DestructTree && _destrType!=DestructTent;
}


#if DAMMAGE_DIAGS
#define ReportRegion(reg,text) \
LogF("Dammage %s %.3f:(%.1f,%.1f,%.1f x %.1f)", \
text,reg.Dammage(),reg.Center()[0],reg.Center()[1],reg.Center()[2],reg.Radius() \
);
#else
#define ReportRegion(reg,text)
#endif

DEFINE_FAST_ALLOCATOR(DamageRegions)


float DamageRegions::Repair(float ammount)
{
  _totalDamage -= ammount;
  // avoid numerical precision errors
  if (_totalDamage<1e-6f)
    _totalDamage = 0;
  saturateMin(_totalDamage,1);
  return _totalDamage;
}

inline float CalcDammage(float distance2, float valRange2)
{
  if (distance2<=valRange2)
    return 1;
  else
    return Square(valRange2)/Square(distance2);
}

//const int maxTotDammage=180;
//const int maxPVDammage=220;
// pos is in world coords.
void Object::IndirectDamage(DoDamageResult &result, const ShotInfo &shot, float val, float valRange)
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else
  if (val<=0)
    return;
  if (_type==Temporary)
    return; // no dammage for temporary objects
  if (_type==TypeTempVehicle)
    return;
  //if( (ObjectType)_type==Network ) return;
  if (GetDestructType()==DestructNo)
    return;
  // calculate simple per-object dammage
  // use armor
  val *= GetInvArmor();
  Assert(valRange>0);
#if DAMMAGE_DIAGS
  LogF
    ("Indirect dammage %s,%s armor %.1f: %.3f (range %.1f)", _shape->Name(),(const char *)GetDebugName(),GetArmor(),val,valRange);
#endif
  // calculate dammage in three points - near, middle and far
  LocalDamage(result,shot,val,valRange);
#endif
}

// pos is in world coordinates
void Object::DirectDamage(DoDamageResult &result, const ShotInfo &shot, float val, float energyFactor)
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else
  if( val<=0 ) return;  
  if( _type==Temporary ) return; // no dammage for temporary objects
  if( _type==TypeTempVehicle ) return;
  //if( _type==Network ) return;
  if( GetDestructType()==DestructNo ) return;
  if( !GetShape() ) return;
  // use armor
  float valRange=sqrt(val)*0.27f;
  val*=GetInvArmor()*energyFactor;

#if DAMMAGE_DIAGS
  LogF("Direct dammage %s armor %.1f: %.3f:(%.1f)",_shape->Name(),GetArmor(),val,valRange);
#endif
  LocalDamage(result,shot,val,-valRange);
#endif
}

void Object::LocalDamage(DoDamageResult &result, const ShotInfo &shot, float val, float valRange)
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else
  if (!shot.local)
  {
    // no dammage from remote shots
    // no crash dammage from from remote vehicles
    return;
  }
  RString ammoName = shot.ammo ? RString(shot.ammo->ParClass().GetName()) : RString();
  DoDamage(result,shot.modelPos, val, valRange, ammoName, shot.originDir,shot.ignoreSmall);
#endif
}

void Object::SimulateForceDamage(DoDamageResult &result, EntityAI *owner, float force, Vector3Par modelPos)
{
  // high force applied on bodies will also make a damage,
  // TODO now we have no use for it... so create the code and use it when it will be needed
  /*float damage = force * GetInvMass() * GetInvArmor() * 0.01f;

  if (damage > 0.01f)
  {
    LocalDamage(result, NULL,owner, modelPos, damage, GetRadius() * 0.3f); 
  }*/
}

void Object::SimulateImpulseDamage(DoDamageResult &result, EntityAI *owner, float impulse, Vector3Par modelPos)
{
  float damage = impulse * GetInvMass() * GetInvArmor() * 10.f;
  if (damage > 0.01f)
    LocalDamage(result, ShotInfo(NULL,owner, this, modelPos), damage, GetRadius() * 0.3f); 
}

void Object::DamageOnImpulse(EntityAI *owner, float impulse, Vector3Par modelPos)
{
  if (!GetNetworkManager().IsServerOwner())
    return;
  DoDamageResult result;
  SimulateImpulseDamage(result, owner, impulse, modelPos);
  // TODO: consider computing skinning here. For Dayz this is not interesing, DamageOnImpluse is mostly VZero anyway, except for some vehicle-person collisions
  ApplyDoDamage(result,owner, RString(), SkinnedPosition());
}


#include "shots.hpp"

/*!
  Perform LocalHit, set total (structural) dammage and call HitBy.
  Call also Destroy when necessary.
*/
void Object::DoDamage(DoDamageResult &result, Vector3Par pos, float val, float valRange, RString ammo, Vector3Par originDir, bool ignoreSmall)
{
#if _ENABLE_REPORT
  if (dyn_cast<Shot>(this))
    RptF("Damaged shot %s",(const char *)GetDebugName());
#endif
  bool diffArmor = Glob.config.IsEnabled(DTArmor);

#if _ENABLE_LIFE_CHEAT
  if (Glob.config.super || diffArmor)
#else
  if (diffArmor)
#endif
  {
    VehicleWithBrain *player = GWorld->PlayerOn();
    AIBrain *playerUnit = player ? player->Brain() : NULL;
    if (playerUnit)
    {
      if (playerUnit->GetVehicle() == this)
      {
#if _ENABLE_LIFE_CHEAT
        if (Glob.config.super) val = 0; else
#endif
        val /= Glob.config.GetMyArmorCoef();
      }
    }
  }

  /*
  if (valRange<0)
  {
    // check which component is this hit in
    // call DirectLocalHit for hit
    // TODO: pass index as argument to this function
    // or find the component here

    int index = -1;

    val *= DirectLocalHit(index,val * GetArmor());

  }
  */

  val *= LocalHit(result, pos, val * GetArmor(), valRange);

  saturateMin(val,2000);

  #if DAMMAGE_DIAGS
  LogF("  %s: Do dammage %.3f (range %.1f)",(const char *)GetDebugName(),val,valRange);
  #endif

  // TODO: some reasonable calculation of dammage radius
  // TODO: consider pre-calculating of dammage sum
  float objRadius = _shape->GeometrySphere();
#if _VBS3
  // we cannot apply any damage to an object with no radius
  if(objRadius==0.0f)
    return;
#endif
  float dist2ToCenter=pos.SquareSize();

  if( valRange<=0 )
  {
    // direct hit
    // some reasonable calculation of hit area for direct hit
    if( valRange==0  )
    {
      Fail("Obsolete direct damage value");
      valRange = 2.0f * val;
      saturateMin(valRange, objRadius);
    }
    else
      valRange=-valRange;
    // some examples:
    // 6 m for bullet (3mm direct) on soldier (1mm) - do not care, he is dead anyway
    // 0.2m  for grenade (20mm direct) on M60 (250mm)
    // 1.6m  for AT3 (200mm direct) on M60 (250mm)

    #if DAMMAGE_DIAGS
    LogF("  val %.3f valRange %.3f objRadius %.3f",val,valRange,objRadius);
    #endif
    
    // calculate total armor mass
    float avgDammage=val*Square(valRange/objRadius);
    result.damage += avgDammage;
  }
  else
  {
    // sum of regions in central area will be val*1.33
    //val*=(1.0/1.33);
    // calculate dammage at sample points: nearest, center, furthest
    float distToCenter=sqrt(dist2ToCenter);
    // TODO: consider more sample points?
    float minDist=floatMax(distToCenter-objRadius,0);
    float valRange2=Square(valRange);

    float minDammage=CalcDammage(Square(minDist),valRange2);
    const float ignoreLimit = ignoreSmall ? 1e-3f : 0;
    if (minDammage*val<=ignoreLimit)
    {
      #if DAMMAGE_DIAGS
      LogF("  Dist: min %.3f",minDist);
      LogF("  Damm: min %.3f (ignored)",minDammage);
      #endif
    }
    else
    {
      float midDist=distToCenter;
      float maxDist=distToCenter+objRadius;
      float midDammage=CalcDammage(Square(midDist),valRange2);
      float maxDammage=CalcDammage(Square(maxDist),valRange2);
      #if DAMMAGE_DIAGS
      LogF("  Dist: min %.3f mid %.3f max %.3f",minDist,midDist,maxDist);
      LogF("  Damm: min %.3f mid %.3f max %.3f",minDammage,midDammage,maxDammage);
      #endif

      float avgDammage=(minDammage+midDammage+maxDammage)*(0.33*val);
      result.damage += avgDammage;
    }
  }
}

void Object::DoDamageResult::GrowHit(int size)
{
  if (size>hits.Size())
  {
    int oldSize = hits.Size();
    // will be automatically zero init - type DmgZeroInit used
    hits.Resize(size);
    for (int i=oldSize; i<size; i++)
      hits[i] = 0;
  }
}

void Object::DoDamageResult::operator += (const DoDamageResult &add)
{
  damage+=add.damage;
  // accumulate hits as well
  // if necessary, resize target hit array
  GrowHit(add.hits.Size());
  for (int i=0; i<add.hits.Size(); i++)
    hits[i] += add.hits[i];
}

void Object::ApplyDoDamage(const DoDamageResult &result, EntityAI *owner, RString ammo, const SkinnedPosition &pos)
{
  // if there is no damage do apply, do nothing
  if (result.damage<=0 && result.hits.Size()==0) return;
  if (_destrType==DestructNo) return;
  // consider all buildings as destructible
#if 0
  if (_destrType==DestructBuilding && !GetDestroyAnimation() && !CanDestroyUsingEffects())
  {
    // if the object is a building and has no destruction animation,
    // it has no sense applying damage to it
    // but in FP2 we are able to destroy buildings
    // however, this should be marked as DestructBuildingPart?
    return;
  }
#endif
  DoAssert(GetNetworkManager().IsServerOwner());

  if (GetNetworkManager().IsServerOwner())
  {
    // static objects have no owner - broadcast the message
    if (GetNetworkId().creator == CreatorId(STATIC_OBJECT))
    {
      // broadcast dammage of static object over network
      GetNetworkManager().CreateObjectStatic(this);
    }
    // if the object was already destroyed, there is no need to destroy it any more
    ApplyDoDamageLocalHandleHitBy(result,owner,ammo,pos);
  }
  else
  {
  }
}

void Object::ApplyDoDamageLocal(const DoDamageResult &result, EntityAI *owner, RString ammo, const SkinnedPosition &pos)
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else

#if DAMMAGE_DIAGS
  LogF("  Total %.4f",GetRawTotalDamage());
#endif

#if _EXT_CTRL
  if(!IsExternalControlled())
#endif
  // apply the damage
  SetTotalDamage(GetRawTotalDamage()+result.damage, true);
#endif
}

/* 
 OnEventAnyRet<float>(EEHandleDamage, ...) is called in nested function HandleDamage, so when we want to postpone eval. EH
we have to postpone call all functions dependent on EH return value.
*/
class EventApplyDoDammage: public IPostponedEvent
{
  OLinkObject _object;
  const Object::DoDamageResult _result;
  OLinkO(EntityAI) _owner;
  const RString _ammo;
  SkinnedPosition _pos;

public:
  EventApplyDoDammage(Object *object, const Object::DoDamageResult result, EntityAI *owner, const RString &ammo, const SkinnedPosition &pos)
    : _object(object), _result(result), _owner(owner), _ammo(ammo), _pos(pos)
  {
  }

  void DoEvent() const
  {
    if (_object)
      _object->ApplyDoDamageLocalHandleHitByDo(_result, _owner, _ammo, _pos);
  }

  USE_FAST_ALLOCATOR
};

void Object::ApplyDoDamageLocalHandleHitBy(const DoDamageResult &result, EntityAI *owner, RString ammo, const SkinnedPosition &pos)
{
  DoAssert(GetNetworkManager().IsServerOwner());
  GWorld->AppendPostponedEvent(new EventApplyDoDammage(this, result, owner, ammo, pos));
}

void Object::ApplyDoDamageLocalHandleHitByDo(const DoDamageResult &result, EntityAI *owner, RString ammo, const SkinnedPosition &pos)
{
  DoAssert(GetNetworkManager().IsServerOwner());
  bool oldDestroyed = IsDamageDestroyed();
  
  ApplyDoDamageLocal(result,owner,ammo,pos);
  
  HitBy(owner,result.damage,ammo, oldDestroyed);

  if (!oldDestroyed && IsDamageDestroyed())
  {
    // can be never destroyed
    //Assert( !!IsDestroyed() );
    Destroy(owner, GetRawTotalDamage(), 0.05f, 0.1f);
  }
}

float Object::GetRawTotalDamage() const
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
  return 0.0;
#else
  return _damage ? _damage->GetTotalDamage() : 0.0f;
#endif
}

/*!
  Set only value. Do not change any other part of dammage state
  (like smoke).
*/
void Object::SetTotalDamage(float val, bool showDamage)
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else
  if( fabs(val-GetTotalDamage())<1e-6 )
    return;
  if (val > 0.0f)
  {
    // dammage may already be set - it may contain some smoke source
    if (!_damage)
      _damage=new DamageRegions;
    _damage->SetTotalDamage(val);
  }
  else _damage.Free();
#endif
}

void Object::SetDamageNetAware(float dammage)
{
  DoAssert(GetNetworkManager().IsServerOwner());
  SetDamage(dammage,true);
}

void Object::SetTotalDamageHandleDead(float value, bool setHitpoints)
{
  SetTotalDamage(value, true);
}

/*!
\patch 5128 Date 2/15/2007 by Ondra
- Fixed: Units created dead were partially alive.
*/
void Object::SetDamage(float damage, bool setHitpoints)
{
  bool oldDestroyed = IsDamageDestroyed();

  if (oldDestroyed && damage<1)
  {
    // we want to repair - first we will ask the smoke to undo
    IExplosion *smoke = GetSmoke();
    if (smoke)
      smoke->UndoResults();
    DeleteSmoke();
  }

  SetTotalDamageHandleDead(damage,setHitpoints);

  if (!oldDestroyed)
  {
    if (IsDamageDestroyed())
    {
      EntityAI *owner = NULL;
      // owner must be something which can be attributed for the damage, never be a static object
      // the requirement is because permanent links are created to owners
      if (!GetObjectId().IsObject())
      {
        owner = dyn_cast<EntityAI>(this);
        /*
        if (!owner)
        {
          ErrF("SetDamage applied on non-EntityAI %s",(const char *)GetDebugName());
        }
        */
      }
      Destroy(owner, damage, 0.05, 0.1);
#if defined SERVER_CORPSE_PERFORMANCE_PATCH
      if (owner)
        GWorld->SuspendEntity(owner);
#endif
    }
  }
  else if (GetTotalDamage() < 1.0)
  {
    _canSmoke = true;
    _isDestroyed = false;
    SetDestroyPhase(0);
  }
}

void Object::SetDamageAndHitpoints(float damage)
{
  SetDamage(damage,true);
}

#include "AI/operMap.hpp"

static inline float ScaleFromPhase(int phase)
{
  return 1-phase*(0.9f/255);
}

static inline float BuildingScaleFromPhase(int phase)
{
  return 1-(phase*phase)*(0.95f/(255*255));
}

static inline float BuildingPosFromPhase(int phase)
{
  return (phase*phase)*(-0.25f/(255*255));
}

void Object::SetDestroyPhase(int phase)
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else
  // adjust matrix according to destroy phase
  if (phase==_destroyPhase) return;
  if (!GetDestroyAnimation()) switch (GetDestructType())
  {
    case DestructTree:
    {
      // apply destruction to the object transformation matrix
      // simulate non-linear destruction function
      float newAngle = sin(phase*H_PI/2/255)*(H_PI/2);
      float oldAngle = sin(_destroyPhase*H_PI/2/255)*(H_PI/2);
      float animChange = newAngle-oldAngle;
      // rotate around x-axis (suitable for walls)
      Vector3 bCenter = GetShape()->BoundingCenter();
      // object rotates around -bCenter
      Matrix4 rot=
      (
        Matrix4(MTranslation,-bCenter)*
#if _VBS2
        Matrix4(MRotationAxis, _destructAxis, animChange)*
#else        
        Matrix4(MRotationX,animChange)*
#endif
        Matrix4(MTranslation,bCenter)
      );
      // apply rotation to object matrix
      Matrix4 trans = FutureVisualState().Transform() * rot;
      // note: Move only when object is in landscape
      // how can we know it?
      Move(trans);
      GScene->GetShadowCache().ShadowChanged(this);
      break;
    }
    case DestructTent:
    {
      // apply destruction to the object transformation matrix
      float newYScale = ScaleFromPhase(phase);
      float oldYScale = ScaleFromPhase(_destroyPhase);
      float animChange = newYScale/oldYScale;
      
      Vector3 bCenter = FutureVisualState().PositionModelToWorld(-GetShape()->BoundingCenter());
      // object rotates around -bCenter
      // caution - matrix already contains a landscape skew
      // M = S * T
      Matrix4 bCenterTranslation(MTranslation,-bCenter);
      Matrix4 invBCenterTranslation(MTranslation,bCenter);
      Matrix4 yScale(MScale,1,animChange,1);
      // calculate land-slope skew and invSkew
      float dX,dZ;
#if _VBS3_CRATERS_DEFORM_TERRAIN
      // This is here because resurrected tents were having wrong skew matrix, because GLandscape->SurfaceY can be changed in deformable terrain
      dX = 0.0f;
      dZ = 0.0f;
#else
      GLandscape->SurfaceY(FutureVisualState().Position().X(),FutureVisualState().Position().Z(),&dX,&dZ);
#endif
      
	    Matrix4 skew = MIdentity, invSkew = MIdentity;
	    skew(1,0) = dX;
	    skew(1,2) = dZ;
	    invSkew(1,0) = -dX;
	    invSkew(1,2) = -dZ;

      // apply scale to object matrix
      Matrix4 trans = (
        invBCenterTranslation * skew * yScale * invSkew * bCenterTranslation
        * FutureVisualState().Transform()
      );
      // note: Move only when object is in landscape
      // how can we know it?
      SetSkewed();
      Move(trans);
      GScene->GetShadowCache().ShadowChanged(this);
      break;
    }
    case DestructBuilding:
    {
      // move the building into the ground when destroyed
      // assume no skew yet
      // apply destruction to the object transformation matrix
      float newYScale = BuildingScaleFromPhase(phase);
      float oldYScale = BuildingScaleFromPhase(_destroyPhase);
      //float animChange = newYScale/oldYScale;
      float newYPos = BuildingPosFromPhase(phase);
      float oldYPos = BuildingPosFromPhase(_destroyPhase);
      // - boundingCenter is 0,0,0 in the model space, we want max-(-bc)
      float yRange = _shape->Max().Y()+_shape->BoundingCenter().Y();
      float yMoveNewOffset = yRange*newYPos;
      float yMoveOldOffset = yRange*oldYPos;
      Vector3 bCenter = -GetShape()->BoundingCenter();
      Matrix4 bCenterTranslation(MTranslation,-bCenter);
      Matrix4 invBCenterTranslation(MTranslation,bCenter);
      Matrix4 yScaleNew(MScale,1,newYScale,1);
      Matrix4 yScaleOld(MScale,1,1/oldYScale,1);
      Matrix4 yMoveNew(MTranslation,Vector3(0,+yMoveNewOffset,0));
      Matrix4 yMoveOld(MTranslation,Vector3(0,-yMoveOldOffset,0));

      // apply scale and movement to object matrix
      Matrix4 trans = (
        yMoveNew * invBCenterTranslation * yScaleNew *
        yScaleOld * bCenterTranslation * yMoveOld * FutureVisualState().Transform()
      );
      
//      LogF(
//        "%d: rng %.2f, pos %.3f,%.3f,%.3f, scale %.3f, up %.3f",
//        ID().Encode(),yRange,
//        trans.Position().X(),trans.Position().Y(),trans.Position().Z(),
//        trans.Scale(),trans.DirectionUp().Y()
//      );
      // note: Move only when object is in landscape
      // how can we know it?
      SetSkewed();
      Move(trans);
      GScene->GetShadowCache().ShadowChanged(this);
      
      break;
    }
  }

  _destroyPhase=phase;
#endif
}

/***
Used to determine if position updates in MP can be accepted.
*/
bool Object::IsDestroyingByPos() const
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else
  // if there is no destruction, the position is normal
  if (_destroyPhase<=0) return false;
  // if there is a destroy animation, the position is always valid
  if (!GetDestroyAnimation()) switch (GetDestructType())
  {
    // for some destruction types the destruction is done by changing position matrix
    case DestructTree:
    case DestructTent:
    case DestructBuilding:
      return true;
  }
#endif
  return false;
}

void Object::SetDestroyed(float anim)
{
  int phase=toInt(anim*255);
  saturate(phase,0,255);
  SetDestroyPhase(phase);
  bool wasDestroyed = _isDestroyed;
  _isDestroyed = phase>0;
  // if destruction status is changed, we need to update
  if (_isDestroyed !=wasDestroyed)
  {
    // check if field where this object is in field cache
    if (GetType()==Primary)
    {
      LandIndex xLand(toIntFloor(FutureVisualState().Position().X() * InvLandGrid));
      LandIndex zLand(toIntFloor(FutureVisualState().Position().Z() * InvLandGrid));
      for (LandIndex x(xLand-1); x<=xLand+1; x++)
        for (LandIndex z(zLand-1); z<=zLand+1; z++)
          GLandscape->OperationalCache()->RemoveField(x,z);
    }
  }
}

void Object::Repair(float ammount)
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else
  if (GetTotalDamage()<=0 && ammount>=0)
    return;
  if (!_damage)
    _damage=new DamageRegions;
  if (_damage->Repair(ammount)<=0)
    _damage.Free();
  float tot = GetTotalDamage();
  if( tot<0.90 )
  {
    DeleteSmoke();
    _canSmoke=true;
  }
  _isDestroyed=false;
  SetDestroyPhase(0);
#endif
}

void Object::GetDestroySound(SoundPars &pars) const
{
  switch( GetDestructType() )
  {
    case DestructTree:
    case DestructTent:
    {
      ParamEntryVal par=Pars>>"CfgDestroy">>"TreeHit";
      GetValue(pars, par>>"sound");
    }
    break;
    default:
    {
      ParamEntryVal par=Pars>>"CfgDestroy">>"BuildingHit";
      SoundPars soundPars;
      GetValue(pars, par>>"sound");
    }
    break;
  }
  return;
}

const EntityType *Object::CanDestroyUsingEffects() const
{
  const EntityType *entityType = GetEntityType();
  if (entityType && entityType->_destructionEffectsValid)
    return entityType;
  return NULL;
}

bool Object::DestroyUsingEffects(EntityAI *owner, bool immediate, bool doDamage)
{
  const EntityType *entityType = CanDestroyUsingEffects();
  if (!entityType)
  {
    return false;
  }
        
  float objSize = GetRadius();
  float fuelAmount = GetFuelTotal();

  float intensity, interval, fireIntensity, fireInterval, lifeTime;
  DestructionEffects::ComputeDestructionEffectsPars(fuelAmount, fireIntensity, objSize, intensity, lifeTime, fireInterval, interval);
  
  DestructionEffects *effects = new DestructionEffects(
    entityType->_destructionEffects, owner, this, intensity, interval, fireIntensity, fireInterval, lifeTime, NULL, doDamage);
  effects->SetPosition(FutureVisualState().Position());
  GWorld->AddAnimal(effects);
  SetSmoke(effects);
  // for static objects everyone will create his own effects
  if (GetNetworkId().creator != CreatorId(STATIC_OBJECT))
    GetNetworkManager().CreateVehicle(effects, VLTAnimal, "", -1);
  if (immediate)
    // done during Join In Progress to spawn ruins of destructed buildings
    effects->SimulateResults();
  return true;
}

void Object::Destroy(EntityAI *owner, float overkill, float minExp, float maxExp)
{
  // start destruction
  switch( GetDestructType() )
  {
    case DestructTree:
    case DestructTent:
    {
/*
      SoundPars soundPars;
      GetDestroySound(soundPars);
*/
      // temporary destruction vehicle
      float time = 1.5;
      if (GetDestructType()==DestructTent) time = 0.75;
      ObjectDestructed *destroyer=new ObjectDestructed(this,/*soundPars,*/time);
      destroyer->SetPosition(FutureVisualState().Position());
      GLOB_WORLD->AddAnimal(destroyer);
      // if object is local everywhere, everyone will cause its destruction
      // while processing ApplyDoDamage
      if (IsLocal() && GetNetworkId().creator != CreatorId(STATIC_OBJECT))
      {
        GetNetworkManager().CreateVehicle(destroyer, VLTAnimal, "", -1);
      }
    }
    break;
    case DestructBuilding:
    case DestructDefault:
    {
      // if there is a smoke, some destruction is already going on
      if (GetSmoke())
        break;
      // first of all try destruction using destruction effects
      if( CanSmoke() && CanDestroyUsingEffects())
      {
        if ((IsLocal() || GetNetworkId().creator == CreatorId(STATIC_OBJECT) ))
          // start smoke
          DestroyUsingEffects(owner);
        break;
      }

/*
      SoundPars soundPars;
      GetDestroySound(soundPars);
*/
      // temporary destruction vehicle
      ObjectDestroyAnim *anim = GetDestroyAnimation();
      float timeToLive;
      float randomize = GLandscape->GetRandomValueForObject(this);
      if (anim)
        timeToLive = randomize*anim->GetAnimTime(GLandscape->GetRandomValueForObject(this));
      else
      {
        // time should depend on object size and height 
        // or better said, on height of topmost point above ground
        float aboveGround = GetRadius()+FutureVisualState().Position().Y()-GLandscape->SurfaceY(FutureVisualState().Position());
        // we assume falling from 5 m takes one sec
        // 0.44 = 1/sqrt(5)
        timeToLive = (randomize*0.8f+0.4f)*0.44f*sqrt(aboveGround);
      }
      ObjectDestructed *destroyer=new ObjectDestructed(this, timeToLive);
      destroyer->SetPosition(FutureVisualState().Position());
      GLOB_WORLD->AddAnimal(destroyer);
      // if object is local everywhere, everyone will cause its destruction
      // while processing ApplyDoDamage
      if (IsLocal() && GetNetworkId().creator != CreatorId(STATIC_OBJECT))
        GetNetworkManager().CreateVehicle(destroyer, VLTAnimal, "", -1);
    }
    break;
    case DestructEngine:
    case DestructWreck:
    {
      // if object is local everywhere, everyone will cause its destruction
      // while processing ApplyDoDamage
      if( CanSmoke() && !GetSmoke() && (IsLocal() || GetNetworkId().creator == CreatorId(STATIC_OBJECT) ))
      {
        // start smoke
        if (!DestroyUsingEffects(owner))
        {
          Vector3Val com = _shape->CenterOfMass();
          float objSize = GetRadius();
          // old way of destruction - no effects defined
          // acquire CenterOfMass
          // attach smoke to vehicle
          // check if there is a vehicle specific destruction smoke
          float density=objSize*(1.0/3);
          float size=objSize*(1.0/3);
          saturate(density,0.25,2.0);
          saturate(size,0.25,4.0);
          size*=GRandGen.RandomValue()*0.4+0.8;
          density*=GRandGen.RandomValue()*0.4+0.8;
          float rndTime=GRandGen.RandomValue()*0.4+0.8;

          SmokeSourceVehicle *smoke = new SmokeSourceOnVehicle(
            Pars >> "CfgCloudlets" >> "DestructedEngineSmoke", density, size, owner, this, com);
          float smokeSourceCoef=0.2;
          rndTime*=smokeSourceCoef;
          smoke->SetSourceTimes(5*rndTime,30*rndTime,30*rndTime);
          float color=GRandGen.RandomValue();
          smoke->SetColor(Color(0.15,0.15,0.10)*color);
          smoke->SetPosition(FutureVisualState().PositionModelToWorld(com));
          GLOB_WORLD->AddAnimal(smoke);
          SetSmoke(smoke);
          // for static objects everyone will create his own effects
          if (GetNetworkId().creator != CreatorId(STATIC_OBJECT))
            GetNetworkManager().CreateVehicle(smoke, VLTAnimal, "", -1);
        }
        
/*
        SoundPars soundPars;
        soundPars.name=RString(NULL); // no sound
        soundPars.vol=soundPars.freq=0;
*/
        ObjectDestroyAnim *anim = GetDestroyAnimation();
        float timeToLive = anim ? anim->GetAnimTime(GLandscape->GetRandomValueForObject(this)) : 0.5;
        ObjectDestructed *destroyer=new ObjectDestructed(this,/*soundPars,*/timeToLive);
        destroyer->SetPosition(FutureVisualState().Position());
        GLOB_WORLD->AddAnimal(destroyer);
        // for static objects everyone will create his own destroyer
        if (GetNetworkId().creator != CreatorId(STATIC_OBJECT))
          GetNetworkManager().CreateVehicle(destroyer, VLTAnimal, "", -1);
      }
    }
    break;
  }

  // Traverse all objects in neighborhood and cancel their stop status. This code is here for instance for
  // bridge destruction to make objects on it to fall down
  {
    // Consider object radius and 25m border
    const float radius = GetRadius();
    const float maxRadius = radius + 25.0f;

    // Traverse objects in the radius
    int xMin,xMax,zMin,zMax;
    ObjRadiusRectangle(xMin, xMax, zMin, zMax, FutureVisualState().Position(), maxRadius);
    int x,z;
    for (x = xMin; x <= xMax; x++)
    {
      for (z = zMin; z <= zMax; z++)
      {
        const ObjectList &list = GLandscape->GetObjects(z, x);
        int n = list.Size();
        for (int i = 0; i < n; i++)
        {
          // Get the object
          Object *obj = list[i];

          // If object is not in the radius, skip it
          if (FutureVisualState().Position().Distance2(obj->FutureVisualState().Position()) > Square(radius + obj->GetRadius())) continue;

          // Start the simulation
          obj->OnMoved();
        }
      }
    }
  }
}



DEF_RSB(oid)
DEF_RSB(canSmoke)
DEF_RSB(isDestroyed)
DEF_RSB(destroyed)
DEF_RSB(destroyPhase)
DEF_RSB(dammage)

LSError Object::Serialize(ParamArchive &ar)
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else
  if (ar.IsSaving()) CHECK(_objId.Serialize(ar,RSB(oid), 1))
  if (ar.IsLoading())
  {
    // note: for Entity, the _objId is already serialized. For other objects the condition below should
    // not be taken anyway, as they are primary
    if (!_objId.IsObject())
      _id = VisitorObjId(_objId.GetVehId());
  }
  SerializeBitBool(ar, RSB(canSmoke), _canSmoke, 1, true)
  SerializeBitBool(ar, RSB(isDestroyed), _isDestroyed, 1, false)
  if (ar.IsSaving())
  {
    float destroyed = GetDestroyed();
    CHECK(ar.Serialize(RSB(destroyed), destroyed, 1, 0))
    float damage = GetRawTotalDamage();
    // note: damage is sometimes infinite
    if (!_finite(damage))
      RptF("Saving infinite damage in %s",(const char *)GetDebugName());
    CHECK(ar.Serialize(RSB(dammage), damage, 1, 0))
  }
  else if (ar.GetPass() == ParamArchive::PassFirst)
  {
    // serializing destroyed seems to be done only because of _isDestroyed
    float destroyed;
    CHECK(ar.Serialize(RSB(destroyed), destroyed, 1, 0))
    if (_objId.IsObject())
      // object is already there, we need to simulate is destruction
      SetDestroyed(destroyed);
    else
    {
      // simulate what happened during Object::SetDestroyed before saving
      int phase=toInt(destroyed*255);
      saturate(phase,0,255);
      _destroyPhase = phase;
      _isDestroyed = phase>0;
    }
    
    float damage;
    CHECK(ar.Serialize(RSB(dammage), damage, 1, 0))
    SetTotalDamage(damage, false);
  }

  // seems to be obsolete, but we are afraid to remove it a day before Xbox Goldmaster
  
  int t = _destroyPhase;
  CHECK(ar.Serialize(RSB(destroyPhase), t, 1, 0));
  Assert(_destroyPhase == t);
  if (_objId.IsObject())
    SetDestroyPhase(t);
  else
    _destroyPhase = t;
#endif
  return LSOK;

  // set in creation: _shape
  // only for non-primary vehicles (in Vehicle::Serialize) _id
  // ?? _constantColor

  // TODO: check and remove: Time _lastAnimTime;

  // set in creation: _type
  // set in creation: _destrType
  
  // set in creation: _static
  // temporary: _inList

  // set in creation: _scale
}

Object *Object::CreateObject(ParamArchive &ar)
{
  OIDInt id;
  if (ar.Serialize(RSB(oid), id, 1) != LSOK)
    return NULL;
  ObjectId oid;
  oid.Decode(id);
  return GLandscape->GetObject(oid);
}

/*!\note
  Object does not have to be saved when all properties have default values.
*/
bool Object::MustBeSaved() const
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else
  if( _isDestroyed ) return true;
  if( !_canSmoke ) return true;
  if( _damage && _damage->MustBeSaved() ) return true;
  if (!_objId.IsObject()) return true; // inserted from the mission editor
#endif
  return false;
}

NetworkId Object::GetNetworkId() const
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else
  if (_type != Primary && _type != Network
#if _ARMA3_TERRAIN_DECALS
    && _type!=LandDecal
#endif     
    )
    ErrF("Type is not primary for object %s",(const char *)GetDebugName());
#endif
  return NetworkId(CreatorId(STATIC_OBJECT), _objId.Encode());
}

#define OBJECT_MSG_LIST(XX) \
  XX(Create, CreateObject) \
  XX(UpdateGeneric, UpdateObject) \
  XX(UpdateDamage, UpdateDamageObject)

DEFINE_NETWORK_OBJECT(Object, NetworkObject, OBJECT_MSG_LIST)

DEFINE_NET_INDICES_EX(CreateObject, NetworkObject, CREATE_OBJECT_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdateObject, NetworkObject, UPDATE_OBJECT_MSG, NoErrorInitialFunc)
DEFINE_NET_INDICES_EX_ERR(UpdateDamageObject, NetworkObject, UPDATE_DAMMAGE_OBJECT_MSG, Object::CalculateErrorInitialDamage)

NetworkMessageFormat &Object::CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format)
{
  switch (cls)
  {
  case NMCCreate:
    NetworkObject::CreateFormat(cls, format);
    CREATE_OBJECT_MSG(CreateObject, MSG_FORMAT)
    break;
  case NMCUpdateDamage:
    NetworkObject::CreateFormat(cls, format);
    UPDATE_DAMMAGE_OBJECT_MSG(UpdateDamageObject, MSG_FORMAT_ERR)
    break;
  case NMCUpdateGeneric:
    NetworkObject::CreateFormat(cls, format);
    UPDATE_OBJECT_MSG(UpdateObject, MSG_FORMAT_ERR)
    break;
  default:
    NetworkObject::CreateFormat(cls, format);
    break;
  }
  return format;
}

Object *Object::CreateObject(NetworkMessageContext &ctx)
{
  PREPARE_TRANSFER(NetworkObject)

  OIDInt id;
  if (TRANSF_BASE(objectId, id) != TMOK) return NULL;
  ObjectId oid;
  return GLandscape->GetObject(oid);
}

void Object::DestroyObject()
{
  Fail("Cannot remove primary object");
}

TMError Object::TransferMsg(NetworkMessageContext &ctx)
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else
  switch (ctx.GetClass())
  {
  case NMCCreate:
    TMCHECK(NetworkObject::TransferMsg(ctx))
    break;
  case NMCUpdateDamage:
    {
      PREPARE_TRANSFER(UpdateDamageObject)
      TMCHECK(NetworkObject::TransferMsg(ctx))
      TRANSF_BITBOOL(isDestroyed)
      if (ctx.IsSending())
      {
        float dammage = GetRawTotalDamage();
        TRANSF_EX(dammage, dammage)
      }
      else
      {
        float dammage;
        TRANSF_EX(dammage, dammage)
        SetDamage(dammage,false);
      }
    }
    break;
  case NMCUpdateGeneric:
    TMCHECK(NetworkObject::TransferMsg(ctx))
    break;
  default:
    TMCHECK(NetworkObject::TransferMsg(ctx))
    break;
  }
#endif
  return TMOK;
}

float Object::CalculateError(NetworkMessageContextWithError &ctx)
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
  return 0.0f;
#else
  // TODO: implementation
  float error = 0;
  switch (ctx.GetClass())
  {
  case NMCUpdateDamage:
    {
      error += NetworkObject::CalculateError(ctx);

      PREPARE_TRANSFER(UpdateDamageObject)

      ICALCERR_NEQ(bool, isDestroyed, ERR_COEF_STRUCTURE)
      ICALCERRE_ABSDIF(float, dammage, GetRawTotalDamage(), ERR_COEF_VALUE_MAJOR)
    }
    break;
  case NMCUpdateGeneric:
    {
      error += NetworkObject::CalculateError(ctx);
    }
    break;
  default:
    error += NetworkObject::CalculateError(ctx);
    break;
  }
  return error;
#endif
}

float Object::CalculateErrorInitial(NetworkMessageClass cls)
{
  float error = 0.0f;
  switch (cls)
  {
  case NMCUpdateDamage:
    {
      // do not include base class error (FLT_MAX)
      if (_isDestroyed) error += ERR_COEF_STRUCTURE;
      error += ERR_COEF_VALUE_MAJOR * fabs(GetRawTotalDamage());
    }
    break;
  default:
    error += NetworkObject::CalculateErrorInitial(cls);
    break;
  }
  return error;
}

float Object::CalculateErrorInitialDamage(NetworkMessage *msg)
{
  float error = 0.0f;

  NetworkMessageUpdateDamageObject *message = static_cast<NetworkMessageUpdateDamageObject *>(msg);
  if (message->_isDestroyed) error += ERR_COEF_STRUCTURE;
  error += ERR_COEF_VALUE_MAJOR * fabs(message->_dammage);

  return error;
}

void Object::ResetStatus()
{
#if _BINARIZE_REDUCEOBJECT
  Fail("Waring: Object functionality reduced");
#else
  _damage.Free();
  // DeleteSmoke(); // DeleteSmoke has no sense here - _damage is already NULL
  _canSmoke=true;
  _isDestroyed=false;
  SetDestroyPhase(0);
  GameVarSpace *vars = GetVars();
  if (vars)
    vars->Reset();
#endif
}
