// Poseidon - vehicle class
// (C) 1997, Suma
#include "wpch.hpp"

#include "tank.hpp"
#include "AI/ai.hpp"

#include "shots.hpp"
#include "scene.hpp"
#include "camera.hpp"
#include "engine.hpp"
#include "world.hpp"
#include "landscape.hpp"
#include "keyInput.hpp"
#include "diagModes.hpp"
#include "dikCodes.h"

#include <El/Common/randomGen.hpp>

#include <El/ParamFile/paramFile.hpp>

#include <El/Common/perfLog.hpp>
#include <El/Common/perfProf.hpp>

#include "Network/network.hpp"
#include "AI/operMap.hpp"

#include "Shape/specLods.hpp"

#include "car.hpp"

#define ARROWS 0

TankType::TankType( ParamEntryPar param )
:base(param)
{
  _hullHit = -1;
  _trackLHit = -1;
  _trackRHit = -1;

  // no dropdown for a cockpit
}

void TankType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par,shape);

  _gunnerHasFlares = par >> "gunnerHasFlares";

  ParamEntryPar cls = par >> "CargoLight";
  _cargoLightDiffuse = GetColor(cls >> "color");
  _cargoLightAmbient = GetColor(cls >> "ambient");
  _cargoLightRadius = cls >> "radius";

  _fireDust = par >> "fireDustEffect";
#if _PUBLICBETA
  if (par.FindEntry("turnCoef"))
    _turnCoef=par>>"turnCoef";
  else
    _turnCoef = 0;
#else
  _turnCoef=par>>"turnCoef";
#endif
  if (par.FindEntry("turnCoef").IsNull())
  { // hotfix: avoid broken addons to prevent vehicles from turning, value was added quite late
    _turnCoef = 1.0f;
  }
  SetHardwareAnimation(par);
}


void TankType::InitShape()
{
  // we need to clear dampers, so that they do not accumulate forever
  _dampers.Clear();

  base::InitShape();  
  
  const ParamEntry &par=*_par;

  _cargoLightPos = _shape->MemoryPoint((par >> "memoryPointCargoLight").operator RString());

  _hullHit = FindHitPoint("HitHull");
  _trackLHit = FindHitPoint("HitLTrack");
  _trackRHit = FindHitPoint("HitRTrack");

  _hitZoneTextureUV.Add(20); _hitZoneTextureUV.Add(21);
  _hitZoneTextureUV.Add(23); _hitZoneTextureUV.Add(24);
  _hitZoneTextureUV.Add(25); _hitZoneTextureUV.Add(22);
}

void TankType::DeinitShape()
{
  base::DeinitShape();
}

void TankType::InitShapeLevel(int level)
{
  base::InitShapeLevel(level);

  if (level==_shape->FindSpecLevel(VIEW_GUNNER))
  {
    ShapeUsed cockpit=_shape->Level(level);
    cockpit->MakeCockpit();
  }

  if (level==_shape->FindLandContactLevel())
  {
    _dampers.InitLandcontact(_shape,level,GetAnimations(),_animSources);
  }
  _dampers.InitShapeLevel(_shape,level,GetAnimations(),_animSources);
}

void TankType::DeinitShapeLevel(int level)
{
  base::DeinitShapeLevel(level);
}


Object* TankType::CreateObject(bool unused) const
{ 
  return new TankWithAI(this, NULL, unused);  // TODO: what's up with _simname="zsu"? Archeology?
}



Tank::Tank( const EntityAIType *name, Person *driver,bool fullCreate)
:base(name,driver, fullCreate),

// pilot controls
_thrustL(0),_thrustR(0),
_thrustLWanted(0),_thrustRWanted(0),
_turnSpeedWanted(0),_turnSpeedWantedLastActive(TIME_MIN),
_randFrequency(1-GRandGen.RandomValue()*0.05f), // do not use same sound frequency

// turret controls

_track(_shape, *Type()->_par),

_doGearSound(false),
_backwardUsedAsBrake(false),
_forwardUsedAsBrake(false),
_parkingBrake(false),

_collisionSimUntil(TIME_MIN)
{
  FutureVisualState()._rpm=0.1f,_rpmWanted=0.1f;
  _gearBox.Load((*Type()->_par)>>"gearBox", Type()->GetMaxSpeed());
  _fireDust.Init(Pars >> Type()->_fireDust, this, EVarSet_Dust);
  SetTimeOffset(0.2f);  // for simple simulation in Tank::Simulate
}



TankVisualState::TankVisualState(TankType const& type)
:base(type)
{
}


void TankVisualState::Interpolate(Object *obj, ObjectVisualState const& t1state, float t, ObjectVisualState& interpolatedResult) const
{
  base::Interpolate(obj, t1state, t, interpolatedResult);

//   TankVisualState const& s = static_cast<TankVisualState const&>(t1state);
//   TankVisualState& res = static_cast<TankVisualState&>(interpolatedResult);
// 
//   #define INTERPOLATE(x) res.x = x + t*(s.x - x)
// 
//   #undef INTERPOLATE
}


float Tank::GetEngineVol( float &freq ) const
{
  freq=_randFrequency*FutureVisualState()._rpm*1.2f;
  return (fabs(_thrustL)+fabs(_thrustR))*0.25f+0.5f;
}

float Tank::GetEnvironVol( float &freq ) const
{
  // with low speeds we change even the frequency
  freq=1;
  float maxSpeedMs = Type()->GetMaxSpeedMs();
  if (maxSpeedMs == 0.0f)
  {
    return 0.0f;
  }
  else
  {
    float speedFactor = (FutureVisualState()._speed.Size()+_angVelocity.Size()) / maxSpeedMs;
    const float lowSpeedFreq = 0.3f;
    const float highSpeedFreq = 1.0f;
    freq = speedFactor*highSpeedFreq+(1-speedFactor)*lowSpeedFreq;
    return speedFactor;
  }
}

void Tank::PerformFF( FFEffects &effects )
{
  base::PerformFF(effects);
}

static float TankTurnCursorSideCoef = 0.33f;

Pair<Vector3,float> Tank::ShowDrivingCursor(CameraType camType, Person *person) const
{
  if (person!=Driver()) return MakePair(VZero,_cursorY);
  float turnEff = _turnSpeedWanted;
  Vector3 dir = Matrix3(MRotationY,-turnEff*TankTurnCursorSideCoef).Direction();
  float dimByAge = floatMinMax((10-Glob.time.Diff(_turnSpeedWantedLastActive))*0.3f,0,1);
  float dimByState = floatMinMax(fabs(dir.X())*100,0,0.5f);
  float dim = floatMax(dimByAge,dimByState);
  return MakePair(FutureVisualState().DirectionModelToWorld(dir*dim),_cursorY);
}



float Tank::GetHitForDisplay(int kind) const
{
  // see InGameUI::DrawTankDirection
  switch (kind)
  {
    case 0: return GetHitCont(Type()->_hullHit);
    case 1: return GetHitCont(Type()->_engineHit);
    case 2: return GetHitCont(Type()->_trackLHit);
    case 3: return GetHitCont(Type()->_trackRHit);
    // TODO: which turret?
    case 4:
      {
        TurretContext context;
        if (!GetPrimaryGunnerTurret(context) || !context._turretType) return 0;
        return GetHitCont(context._turretType->_turretHit);
      }
    case 5:
      {
        TurretContext context;
        if (!GetPrimaryGunnerTurret(context) || !context._turretType) return 0;
        return GetHitCont(context._turretType->_gunHit);
      }
    default: return 0;
  }
}

void Tank::Sound( bool inside, float deltaT )
{
  PROFILE_SCOPE_EX(snTnk,sound);
  const VisualState &vs = RenderVisualState();
  float gearVol=Type()->_gearSound.vol;
  if( inside ) gearVol*=0.2f;
  if( _doGearSound && !_gearSound )
  {
    _doGearSound=false;
    // check if the sound can be hard at all
    if (inside || GSoundScene->CanBeAudible(WaveEffect,vs.Position(),gearVol))
    {
      AbstractWave *sound=GSoundScene->OpenAndPlayOnce(
        Type()->_gearSound.name,this,true,vs.Position(),vs.Speed(),Type()->_gearSound.vol,Type()->_gearSound.freq,Type()->_gearSound.distance
      );
      _gearSound=sound;
      if (sound)
      {
        GSoundScene->SimulateSpeedOfSound(sound);
        GSoundScene->AddSound(sound);
      }
    }
  }
  if( _gearSound )
  {
    _gearSound->SetVolume(gearVol); // volume, frequency
    _gearSound->SetPosition(vs.Position(),vs.Speed());
  }

  base::Sound(inside,deltaT);
}

float Tank::IsGrassFlatenner(Vector3Par pos, float radius, Vector3 &flattenPos) const
{
  if (FutureVisualState().Position().Distance2(pos)>Square(radius)) return 0;
  const float maxClutterRadius = 2.0f;
  flattenPos = FutureVisualState().Position();
  return CollisionSize()+maxClutterRadius;
}
float Tank::FlattenGrass(float &skewX, float &skewZ, Vector3Par pos, float radius) const
{
  static float flattenGrassDist = 0.5f;
  
  const float maxDist = flattenGrassDist;
  // distance where flattening is maximal
  const float minDist = flattenGrassDist*0.5f;

  // distance where flattening is maximal
  const float centerDist = flattenGrassDist*0.25f;

  Vector3 offset = pos-FutureVisualState().Position();
  float dist2 = offset.SquareSize();
  {
    // TODO: precalc constant property: max. distance of tracks from entity zero
    Vector3 lTrack = _track.LeftPos();
    Vector3 rTrack = _track.RightPos();
    float maxTrackDist = floatMax(rTrack.Size(),lTrack.Size());
    
    // pos+radius is a position of the grass clutter
    // TODO: flatten left/right independently
    if (dist2>Square(radius+maxDist+maxTrackDist)) return 0;
  }
  
  
  
  Vector3 relOffset = DirectionWorldToModel(offset);
  
  // check distance from left/right track 
  Vector3 lTrack = _track.LeftPos();
  Vector3 rTrack = _track.RightPos();
  float lTrackDist2 = lTrack.Distance2(relOffset);
  float rTrackDist2 = rTrack.Distance2(relOffset);
  
  // select the one which is closer
  float trackDist2 = floatMin(lTrackDist2,rTrackDist2);
  // if none close enough, do not flatten
  if (trackDist2>Square(radius+maxDist)) return 0;
  Vector3Val trackPos = lTrackDist2<rTrackDist2 ? lTrack : rTrack;
  Vector3Val trackOffset = FutureVisualState().DirectionModelToWorld(relOffset-trackPos);
  
  float skewXDir = trackOffset.X();
  float skewZDir = trackOffset.Z();

  float skewDirSize2 = skewXDir*skewXDir+skewZDir*skewZDir;
  float skewInvSize = skewDirSize2>0 ? InvSqrt(skewDirSize2) : 0;

  float down = _landContact;
  static float skewFactor = 5.0f;
  skewX = skewXDir*skewInvSize*down*skewFactor;
  skewZ = skewZDir*skewInvSize*down*skewFactor;

  // TODO: optimize
  float dist = floatMax(0,sqrt(trackDist2)-radius);
  if (dist<centerDist)
  {
    // as center is closing, reduce skew to prevent singularities
    float toCenter = InterpolativC(dist,0,centerDist,0,1.0f);
    skewX *= toCenter;
    skewZ *= toCenter;
    return 1.0f;
  }

  float factor = InterpolativC(dist,minDist,maxDist,1.0f,0.0f);

  skewX *= factor;
  skewZ *= factor;

  return factor;
  
}

bool Tank::IsTurret( CameraType camType ) const
{
  AIBrain *unit = GWorld->FocusOn();
  if (unit)
  {
    TankVisualState& vs = unconst_cast(RenderVisualState());

    if (unit==CommanderUnit())
    {
      // commander needs picture to be able to give commands
      return true;
    }
    if (unit==PilotUnit())
    {
      if (vs._driverHidden<0.5f) return false;
    }
    else
    {
      TurretContextV context;
      if (FindTurret(vs, unit->GetPerson(), context))
      {
        if (context._turretVisualState->GetGunnerHidden() < 0.5f) return false;
      }
    }
  }
  return true;
}

bool Tank::HasFlares( CameraType camType ) const
{
  if( camType==CamGunner )
  {
    return Type()->_gunnerHasFlares; //Type()->_outPilotOnTurret;
  }
  return base::HasFlares(camType);
}

inline float tanSat(float x)
{
  // tan(pi*0.49) == 31.82
  if (x>+H_PI*0.49f) return +32;
  if (x<-H_PI*0.49f) return -32;
  return atan(x);
}

void Tank::LimitVirtual
(
  CameraType camType, float &heading, float &dive, float &fov
) const
{
/*
  switch( camType )
  {
    case CamGunner:
      saturate(fov,0.07f,0.35f);
      base::LimitVirtual(camType,heading,dive,fov);
    break;
  }
*/
  base::LimitVirtual(camType,heading,dive,fov);
}

void Tank::InitVirtual
(
  CameraType camType, float &heading, float &dive, float &fov
) const
{
  base::InitVirtual(camType,heading,dive,fov);
/*
  switch( camType )
  {
    case CamGunner:
      fov=0.3;
    break;
  }
*/
}

void Tank::UnloadSound()
{
  base::UnloadSound();
}

Vector3 Tank::Friction( Vector3Par speed )
{
  Vector3 friction;
  friction.Init();
  friction[0]=speed[0]*fabs(speed[0])*50+speed[0]*1500+fSign(speed[0])*300;
  friction[1]=speed[1]*fabs(speed[1])*50+speed[1]*1100+fSign(speed[1])*60;
  friction[2]=speed[2]*fabs(speed[2])*15+speed[2]*100+fSign(speed[2])*10;
  return friction*GetMass()*(1.0f/60000);
}

bool Tank::IsTurretLocked(const TurretContextV &context) const
{
  if (Type()->HasDriver() && FutureVisualState()._driverHidden < 0.99f)
  {
    // may be config flag; now, independent machine guns (operated only from outside) are never locked
    bool lockable = !context._turretType || context._turretType->_lockWhenDriverOut;
    if (lockable) return lockable;
  }
  if (!context._turret) return false;
  if (context._turretType->OutGunnerMayFire(context._gunner)) return false;
  return context._gunner && !context._turretVisualState->IsGunnerHidden();
}


void Tank::MoveWeapons(float deltaT )
{
}


LSError TankWithAI::Serialize(ParamArchive &ar)
{
  SERIAL_BASE;
  //CHECK(ar.Serialize("randFrequency", _randFrequency, 1))
  
  if (!IS_UNIT_STATUS_BRANCH(ar.GetArVersion()))
  {
    //bool _doGearSound:1;
    //float _servoVol;

    //WaterSource _leftWater,_rightWater;
    //DustSource _fireDust;

    CHECK(ar.Serialize("invFireDustTimeTotal", _invFireDustTimeTotal, 1))
    CHECK(ar.Serialize("fireDustTimeLeft", _fireDustTimeLeft, 1))
      
    //float _phaseL,_phaseR; // texture animation
    
    //TrackOptimized _track;

    SERIAL_DEF(thrustLWanted, 0)
    SERIAL_DEF(thrustRWanted, 0)
    SERIAL_DEF(thrustL, 0)
    SERIAL_DEF(thrustR, 0)
    SERIAL_DEF(turnSpeedWanted, 0)
  }
  return LSOK;
}

bool Tank::IsPossibleToGetIn() const
{
  if(!_allowCrewInImmobile)
  {
    if( GetHit(Type()->_trackRHit)>=0.9f ) return false;
    if( GetHit(Type()->_trackLHit)>=0.9f ) return false;
    if( GetHit(Type()->_engineHit)>=0.9f ) return false;
  }
  if( GetHit(Type()->_hullHit)>=0.9f ) return false;
  return base::IsPossibleToGetIn();
}

bool Tank::IsAbleToMove() const
{
  if(!_allowCrewInImmobile)
  {
    if( GetHit(Type()->_trackRHit)>=0.9f ) return false;
    if( GetHit(Type()->_trackLHit)>=0.9f ) return false;
    if( GetHit(Type()->_engineHit)>=0.9f ) return false;
  }
  if( GetHit(Type()->_hullHit)>=0.9f ) return false;
  return base::IsAbleToMove();
}

bool Tank::IsAbleToFire(const TurretContext &context) const
{
  if (context._turretType)
  {
    if (GetHit(context._turretType->_turretHit)>=0.9f ) return false;
    if (GetHit(context._turretType->_gunHit)>=0.9f ) return false;
  }
  return base::IsAbleToFire(context);
}

#define TANK_START_STOP_EFFECTS 1

void Tank::LandFriction(
  Vector3 &friction, Vector3 &torqueFriction, Vector3 &torque, 
  bool brakeFriction, Vector3Par fSpeed, Vector3Par speed, Vector3Par pCenter,
  float coefNPoints, const Texture *texture
)
{
  Vector3 pForce;
  pForce.Init();
  // second is "land friction" - causing no momentum
  //hladas 28.7.2010 - 1.25x increased pForce[0] to decrease sliding
  pForce[0]=fSpeed[0]*2500+fSign(fSpeed[0])*37500;
  pForce[1]=fSpeed[1]*12000+fSign(fSpeed[1])*3000;
  if( brakeFriction )
  {
    pForce[2]=fSpeed[2]*200+fSign(fSpeed[2])*40000;
  }
  else
  {
    pForce[2]=fSpeed[2]*100+fSign(fSpeed[2])*2000;
  }

  #if ARROWS
  Vector3 wCenter(VFastTransform,FutureVisualState().ModelToWorld(),GetCenterOfMass());
  #endif

  Vector3 wForce = FutureVisualState().DirectionModelToWorld(pForce)*GetMass()*(1.0f/40000);
  #if ARROWS
    AddForce(wCenter+pCenter,-wForce*InvMass(),Color(1,0,0));
  #endif
  // apply some torque
  friction+=wForce*coefNPoints;
  torqueFriction+=_angMomentum*0.24f*coefNPoints;
  //torqueFriction+=_angMomentum*0.06f*coefNPoints;
#if TANK_START_STOP_EFFECTS
  // apply heuristics: will the friction cause any torque?
  // as long as the motion is significant, it should
  // for start we are interested only in z-direction torque
  {
    //hladas 28.7.2010 - 0.8f is to compensate increased friction
    float fricX = fabs(speed.X())>1.0f ? 0.8f * pForce.X()*coefNPoints : 0;
    float fricZ = fabs(speed.Z())>1.0f ? pForce.Z()*coefNPoints : 0;
    Vector3 torqueApplied = FutureVisualState().DirectionModelToWorld(Vector3(fricX,0,fricZ));
    torque -= pCenter.CrossProduct(torqueApplied);
  }
#endif
  // some friction is caused by moving the land aside
  // this applies only to soft surfaces
  float soft=0;
  if( texture )
  {
    soft=texture->Roughness()*0.7f;
    //soft*=softFactor;
    saturateMin(soft,1);
  }
  float landMoved=0.01f*soft;
  if( brakeFriction ) landMoved*=2;
  pForce[0]=speed[0]*35.0f*landMoved;
  pForce[1]=0;
  pForce[2]=speed[2]*6.0f*landMoved;
  wForce=FutureVisualState().DirectionModelToWorld(pForce)*GetMass();
  #if ARROWS
    AddForce(wCenter+pCenter,-wForce*InvMass(),Color(1,1,0,0.5f));
  #endif
  friction+=wForce*coefNPoints;
  
#if TANK_START_STOP_EFFECTS
  {
    float fricX = fabs(speed.X())>1.0f ? pForce.X()*coefNPoints : 0;
    float fricZ = fabs(speed.Z())>1.0f ? pForce.Z()*coefNPoints : 0;
    Vector3 torqueApplied = FutureVisualState().DirectionModelToWorld(Vector3(fricX,0,fricZ));
    torque -= pCenter.CrossProduct(torqueApplied);
  }
#endif
}


/*!
\patch 1.30 Date 7/30/2001 by Ondra.
- Fixed: tank reaction to collision with other tanks.
\patch 1.36 Date 12/13/2001 by Ondra
- Fixed: APC did very little harm to soldiers when in contact.

\patch 5163 Date 6/1/2007 by Ondra
- Fixed: Tanks now slow down when destroying a tree or other big objects.
*/
void Tank::ObjectContact(
  ObjectVisualState &moveTrans, Vector3Par wCenter, float deltaT,
  Vector3 &torque, Vector3 &friction, Vector3 &torqueFriction,
  Vector3 &totForce, float &crash, CrashType &doCrash
)
{

  Vector3Val speed=FutureVisualState().ModelSpeed();

  CollisionBuffer collision;
#if _VBS3// in case of severe hits, it has to check for ObjHitevents
  CollisionBuffer collisionHit;
#endif
  GLandscape->ObjectCollision(collision,this,moveTrans);
  #define MAX_IN 0.2f
  #define MAX_IN_FORCE 0.1f
  #define MAX_IN_FRICTION 0.2f

  AddImpulseNetAwareAccumulator accum;
  DoDamageAccumulator dmgAcc;

  // CrashObject distinguish types
  int destTree = 0;
  int destBuild = 0;
  int destEng = 0;

  // even objects which we have knocked down should slow us down
  float impulseToSelf = 0.0f;
  for( int i=0; i<collision.Size(); i++ )
  {
    // info.pos is relative to object
    CollisionInfo &info=collision[i];
    Object *obj=info.object;
    if( !obj ) continue;
    if (info.hierLevel>0) 
      obj = info.parentObject;      

    if( !obj->GetShape() ) continue;
    obj->SimulationNeeded(GetVisualStateAge(FutureVisualState()));
    //float maxImpulse=0; // impulse factor
    float cFactor = obj->GetMass()*InvMass();
    //bool isStatic=false;
    if( obj->Static() )
    {
      // fixed object - apply fixed collision routines
      // calculate his damage
      // depending on vehicle speed and mass
      float dFactor=GetMass()*obj->InvMass();       
      if
      (
        obj->GetDestructType()==DestructTree ||
        obj->GetDestructType()==DestructTent ||
        obj->GetDestructType()==DestructMan
      )
      { // old code. Let it untouched...
        float dSpeed = FutureVisualState()._speed.SquareSize()+_angVelocity.SquareSize();
        float damage = dSpeed*obj->GetInvArmor()*dFactor*0.2f;
        if( damage>0.01f )
        {
          if (GetNetworkManager().IsServerOwner()) obj->LocalDamageAccum(dmgAcc,ShotInfo(NULL,this,this,VZero),damage,obj->GetShape()->GeometrySphere(),RString(),SkinnedPosition());
        }

        if (
          obj->GetDestructType()==DestructMan || obj->IsDamageDestroyed() ||
          obj->GetTotalDamage()+damage>=1
        )
        {
          // if it is a heavy object, knocking it down should slow us significantly
          // add an impulse based on mass ratio
          if (cFactor>0.01f)
          {
            // assume object with the same mass as we should almost stop us
            // object with half of our mass should slow us down almost to half speed
            // as the object is static (hard connected to the ground), the ratio should be even much higher
            cFactor *= 10;

            // TODO: consider some torque as well
            // the slower we move, the less effect the collision has on our speed

            impulseToSelf += cFactor;

          }
          
          saturate(cFactor,0.001f,0.03f);
        }
        else
        {
          _objectContact=true;
          cFactor = 1;
        }
      }
      else
      {
        // new code by Fehy. Cases with weapons should more resistant to tanks..
        saturateMin(dFactor,40);
        float dSpeed = -SpeedAtPoint(info.pos) * info.dirOut / float(collision.Size());
        
        // tanks are too heavy, so do the smaller impulse
        DoDamageResult result;
        obj->SimulateImpulseDamage(result, this, dSpeed * GetMass() / 3 , VZero);
        dmgAcc.Add(obj,this,RString(),SkinnedPosition(),result);

        cFactor = 1;
        _objectContact=true;
      }
    }
    else
    {
      saturate(cFactor,0,10);
      _objectContact=true;
    }

    // distinguish crash object on the basis of destruction types
    DestructType dt = obj->GetDestructType();
    if (dt == DestructTree)
    {
      destTree++;
    }
    else if (dt == DestructBuilding)
    {
      destBuild++;
    }
    else if (dt == DestructEngine || dt == DestructWreck)
    {
      destEng++;
    }

    Point3 pos=info.pos;
    Vector3 dirOut=info.dirOut;
    // create a force pushing "out" of the collision
    float forceIn=floatMin(info.under,MAX_IN_FORCE);
    Vector3 pForce=dirOut*GetMass()*40*forceIn*cFactor;
    // apply proportional part of force in place of impact
    Vector3 pCenter=pos-wCenter;
    if( cFactor>0.01f )
    {
      totForce+=pForce;
      // apply same force to second object
      torque+=pCenter.CrossProduct(pForce*0.5f);
    }

    Vehicle *veh=dyn_cast<Vehicle,Object>(obj);
    if( veh )
    {            
      // transfer all my intertia to him?
      Vector3 relDistance = veh->FutureVisualState().Position()-FutureVisualState().Position();
      Vector3 relSpeed = veh->FutureVisualState().Speed()-FutureVisualState().Speed();
      if ( relSpeed.SquareSize() > 0)
      {
        EntityAI * ent=dyn_cast<EntityAI,Object>(obj);
        if (ent && ent->GetStopped())
          ent->OnMoved();
      }
     
      if (
        FutureVisualState()._speed.SquareSize() > Square(3.5f) &&
        relSpeed.SquareSize() > Square(3.5f)
        )
      {
        
        if (dyn_cast<Person,Vehicle>(veh))
        {
          // soldier - different damage calculation
          Vector3 relDistanceDir = relDistance.Normalized();
          float speedTransfer = -relSpeed*relDistanceDir;
          //saturate(speedTransfer,0,0.5f);
          //float limitMass = floatMin(GetMass(),20000);
          //Vector3 impulse = _speed*limitMass**speedTransfer*0.08f;
          Vector3 impulse = speedTransfer*1.3f*veh->GetMass()*relDistanceDir/collision.Size();

          accum.Add(veh,impulse,VZero);
        }
        else                   
        {
          float speedTransfer = relSpeed*relDistance*-relDistance.InvSize();
          saturate(speedTransfer,0,0.5f);
          Vector3 impulse = FutureVisualState()._speed*GetMass()*deltaT*speedTransfer*0.02f;
          accum.Add(veh,impulse,(info.pos - wCenter).CrossProduct(impulse));
          
          // TODO: total inertia transferred is limited by actual inertia
          
        }
      }
#if _VBS3
      if(FutureVisualState()._speed.SquareSize() > Square(0.5f))
      {
        EntityAI * ent=dyn_cast<EntityAI,Object>(obj);

        float kinetic=GetMass()*relSpeed.SquareSize()*0.5f; // E=0.5*m*v^2

        static float minKin = 30000.0f;
        if(ent && ent->IsEventHandler(EEHitPart) && kinetic > minKin)
        {
          CollisionInfo tmp = info;
          tmp.underVolume = kinetic * 0.01; //special use to store modified impulse
          bool found = false;
          for(int j=0; j < collisionHit.Size(); ++j)
            if(collisionHit[j].object == tmp.object) //only store the object once
            {
              found = true;
              break;
            }
            if(!found)
              collisionHit.Add(tmp);
        }
      }
#endif //_VBS3
    }
    
    if (cFactor<0.05f)
    {
      // TODO: apply some friction - based on cFactor
      continue;
    }
    
    // if info.under is bigger than MAX_IN, move out
    if( info.under>MAX_IN && !dyn_cast<Car,Vehicle>(veh))
    {

      Matrix4 transform=moveTrans.Transform();
      Vector3 newPos=transform.Position();
      float moveOut=info.under-MAX_IN;
      Vector3 move=dirOut*moveOut*0.1f;
      newPos+=move;
      transform.SetPosition(newPos);
      moveTrans.SetTransform(transform);
      const float crashLimit=0.3f;
      if( moveOut>crashLimit ) crash+=moveOut-crashLimit;

      Vector3Val objSpeed=info.object->ObjectSpeed();
      Vector3 colSpeed=FutureVisualState()._speed-objSpeed;

      float potentialGain=move[1]*GetMass();
      float oldKinetic=GetMass()*colSpeed.SquareSize()*0.5f; // E=0.5*m*v^2
      // kinetic to potential conversion is not 100% effective
      float crashFactor=(moveOut-crashLimit)*4+1.5f;
      saturateMax(crashFactor,2.5f);
      float newKinetic=oldKinetic-potentialGain*crashFactor;
      float newSpeedSize2=newKinetic*InvMass()*2;
      if( newSpeedSize2<=0 || oldKinetic<=0 ) colSpeed=VZero;
      else colSpeed*=sqrt(newSpeedSize2*colSpeed.InvSquareSize());
      // limit relative speed to object we crashed into
      const float maxRelSpeed=2;
      if( colSpeed.SquareSize()>Square(maxRelSpeed) )
      {
        // adapt _speed to match criterion
        crash+=(colSpeed.Size()-maxRelSpeed)*0.3f;
        colSpeed.Normalize();
        colSpeed*=maxRelSpeed;
      }
      FutureVisualState()._speed=objSpeed+colSpeed;
    }
        

    // second is "land friction" - causing little momentum
    float frictionIn=floatMin(info.under,MAX_IN_FRICTION);
    pForce[0]=fSign(speed[0])*10000;
    pForce[1]=speed[1]*fabs(speed[1])*1000+speed[1]*8000+fSign(speed[1])*10000;
    pForce[2]=speed[2]*fabs(speed[2])*150+speed[2]*250+fSign(speed[2])*2000;

    pForce=FutureVisualState().DirectionModelToWorld(pForce)*GetMass()*(4.0f/10000)*frictionIn;
    #if ARROWS
      AddForce(wCenter+pCenter,-pForce*InvMass());
    #endif
    friction+=pForce;
    torqueFriction+=_angMomentum*0.15f;
  }

#if _VBS3 //process objHits
  if(collisionHit.Size() > 0)
  {
    //requires a new collision detection with fire geo
    for( int i=0; i<collisionHit.Size(); i++ )
    {
      CollisionBuffer tempBuf;
      CheckObjectCollisionContext context;

      CollisionInfo &info=collisionHit[i];
      Object* obj = info.object;

      Vector3 collPos = info.pos;
      Vector3 dir = collPos - Position();
      collPos = Position() + 1.1 * dir;

      EntityAI* ent = dyn_cast<EntityAI>(obj);

      Assert(ent)

      LODShape *shp = obj->GetShape();
      int level = shp->FindFireGeometryLevel();
      if(level < 0) continue;

//        obj->Animate(level,true,obj,-FLT_MAX);

      obj->Intersect(tempBuf, Position(), collPos, GetRadius());

      if(tempBuf.Size() <= 0) continue;

      CollisionInfo &infoHit=tempBuf[0]; //only take the first component

      if(!shp || infoHit.component < 0) continue;

      AutoArray<RString> namedComponents;
      RString surfaceType; 
      float radius = 1;

      const Shape *fire = shp->FireGeometryLevel();
      if(fire)
      {
        const ConvexComponents &fireComponent = shp->GetFireComponents();
        Assert(infoHit.component >= 0 && infoHit.component < fireComponent.Size());
        const Ref<ConvexComponent> component = fireComponent.Get(infoHit.component);
        radius = component->GetRadius();
        const SurfaceInfo* surf = component->GetSurfaceInfo();
        if(surf) surfaceType = surf->_surfaceType;

        for( int i = 0; i < fire->NNamedSel(); ++i )
        {
          // get the first named selection
          // each named selection has an array of faces indecies.
          // those face indecies, have indecies of vertex's.
          const NamedSelection &namSel = fire->NamedSel(i);
          const Selection &faceIndecies = namSel.Faces();

          if(component->NPlanes() > 0)
          {
            int cFaceIndicie = component->GetPlaneIndex(0);
            for( int x = 0; x < faceIndecies.Size(); ++x )
            {
              if( faceIndecies[x] == cFaceIndicie)
              {
                namedComponents.Add(namSel.Name());
                break;
              }
            }
          }
        }
      }

      AutoArray<HitEventElement> list;
      int index = list.Add();
      list[index].Init
        (
        NULL,
        this,
        infoHit.pos,
        Speed(),
        namedComponents,
        infoHit.dirOutNotNorm.Normalized(), 
        radius, 
        surfaceType,
        true,
        info.underVolume   //stores the impulse
        );

      ent->OnHitPartEvent(EEHitPart,list);
    }
  }

#endif //_VBS3

  // select the most incident object
  if (!(destTree == destBuild && destBuild == destEng))
  {
    if (destTree > destBuild)
    {
      if (destTree > destEng) 
        doCrash = CrashTree;
      else
        doCrash = CrashArmor;
    }
    else
    {
      if (destBuild > destEng)
        doCrash = CrashBuilding;
      else
        doCrash = CrashArmor;
    }
  }

  if (impulseToSelf>0)
  {
    float mass = InterpolativC(FutureVisualState()._speed.Size(),0.5f,3,0,GetMass());
    AddImpulse(-FutureVisualState()._speed*(floatMinMax(impulseToSelf,0,0.9f)*mass),VZero);
  }
}

void Tank::ObjectContactStatic(float deltaT, float &crash)
{
  CollisionBuffer collision;
  GLandscape->ObjectCollision(collision,this,FutureVisualState());
#define MAX_IN_STATIC 0.1f

  _objectContact = false;
  if (collision.Size() == 0)
    return;

  Vector3 wCenter(VFastTransform,FutureVisualState().ModelToWorld(),GetCenterOfMass());
  AddImpulseNetAwareAccumulator accum;
  for( int i=0; i<collision.Size(); i++ )
  {
    // info.pos is relative to object
    CollisionInfo &info=collision[i];
    Object *obj=info.object;
    if( !obj ) continue;
    if (info.hierLevel>0) 
      obj = info.parentObject;      

    if( !obj->GetShape() || obj->Static())
      continue;

    //float maxImpulse=0; // impulse factor
    //float cFactor = obj->GetMass()*InvMass();   

    //bool isStatic=false;
    //saturate(cFactor,0,10);    

    Point3 pos=info.pos;
    Vector3 dirOut=info.dirOut;
    // create a force pushing "out" of the collision    

    Vehicle *veh=dyn_cast<Vehicle,Object>(obj);
    if( veh )
    {            
      _objectContact=true;
      // transfer all my intertia to him?
      Vector3 relDistance = veh->FutureVisualState().Position()-FutureVisualState().Position();
      Vector3 relSpeed = veh->FutureVisualState().Speed()-FutureVisualState().Speed();
      if ( relSpeed.SquareSize() > 0)
      {
        EntityAI * ent=dyn_cast<EntityAI,Object>(obj);
        if (ent && ent->GetStopped())
          ent->OnMoved();
      }

      if (relSpeed.SquareSize() > Square(3.5f))
      {
        if (dyn_cast<Person,Vehicle>(veh))
        {
          // soldier - different damage calculation
          Vector3 relDistanceDir = relDistance.Normalized();
          float speedTransfer = -relSpeed*relDistanceDir;
          //saturate(speedTransfer,0,0.5f);
          //float limitMass = floatMin(GetMass(),20000);
          //Vector3 impulse = _speed*limitMass**speedTransfer*0.08f;
          Vector3 impulse = speedTransfer*1.3f*veh->GetMass()*relDistanceDir/collision.Size();

          accum.Add(veh,impulse,VZero);
        }
        else                   
        {
          float speedTransfer = relSpeed*relDistance*-relDistance.InvSize();
          saturate(speedTransfer,0,0.5f);
          Vector3 impulse = FutureVisualState()._speed*GetMass()*deltaT*speedTransfer*0.02f;

          accum.Add(veh,impulse,(info.pos - wCenter).CrossProduct(impulse));
        }
      }
    }   

    // if info.under is bigger than MAX_IN_STATIC, move out
    if(
      obj->GetMass() > 100 && !obj->IsPassable() &&
      info.under > MAX_IN_STATIC && !dyn_cast<Car,Vehicle>(veh)
    )
    {     
      float moveOut=info.under-MAX_IN_STATIC;
      Vector3 move=dirOut*moveOut*0.1f;     
      const float crashLimit=0.3f;
      if( moveOut>crashLimit ) crash+=moveOut-crashLimit;

      Vector3Val objSpeed=info.object->ObjectSpeed();
      Vector3 colSpeed=-objSpeed;
      const float maxRelSpeed = 2;
      if( colSpeed.SquareSize()>Square(maxRelSpeed) )
      {
        // adapt _speed to match criterion
        crash+=(colSpeed.Size()-maxRelSpeed)*0.3f;     
      }      
    }  
  }
}

#if _ENABLE_CHEATS
extern bool disableSimpleSim;
extern bool forceSimpleSim;
extern bool forceFarSim;
#else
const bool disableSimpleSim = false;
const bool forceSimpleSim = false;
const bool forceFarSim = false;
#endif

void Tank::StabilizeTurrets(Matrix3Val oldTrans, Matrix3Val newTrans)
{
  if (fabs(_turnSpeedWanted)>0.01f)
  {
    Vector3 turnDir = Matrix3(MRotationY,-_turnSpeedWanted*TankTurnCursorSideCoef).Direction();
    Vector3 turnDirNew = newTrans.InverseRotation()*oldTrans*turnDir;
    _turnSpeedWanted = atan2(turnDirNew.X(),turnDirNew.Z())/TankTurnCursorSideCoef;

  }

  base::StabilizeTurrets(oldTrans, newTrans);
}

void Tank::OnTurretUpdate(Turret *turret, Matrix3Par oldOrient, Matrix3Par newOrient)
{
  Fail("Not used");
}

Vector3 Tank::GetLandContactPoint() const
{
  // Get the land contact level (or geometry level as a fallback)
  const Shape *lcLevel = GetShape()->LandContactLevel();
  if (!lcLevel) lcLevel = GetShape()->GeometryLevel();

  Vector3 comDown;
  if (lcLevel)
  {
    // check COM-corresponding position on the landcontact level
    comDown = GetCenterOfMass();
    comDown[1] = lcLevel->Min().Y();  
  }
  else
  {
    comDown = VZero;
    RptF("Error: Neither Geometry nor LandContact level is present in model %s", GetShape()->Name());
  }
  return comDown;
}

void Tank::PlaceOnSurface(Matrix4 &trans)
{
  base::PlaceOnSurface(trans);

  Vector3 pos = trans.Position();

  Texture *txt = NULL;
  float dX,dZ;
  GLandscape->RoadSurfaceYAboveWater(pos,&dX,&dZ,&txt);
  if (txt && (GLandscape->GetSeaTexture()==txt || txt->IsWater()))
  {
    // estimate water sub-merging
    pos[1] -= 1;
  }
  else
  {
    Vector3 normal(-dX,1,-dZ);
    normal.Normalize();

    // Get the land contact point in world coordinates
    Vector3 nPos = trans.FastTransform(GetLandContactPoint());

    // check COM position on the ground
    Vector3 oPos = nPos;
    nPos[1]=GLandscape->RoadSurfaceYAboveWater(nPos,&dX,&dZ);

    float offset = 0.075f;
    float normalizedOffset = offset/normal.Y();
    nPos[1] += normalizedOffset;

    pos[1] += nPos[1]-oPos[1];
  }
  trans.SetPosition(pos);
}

static Vector3 AddFriction(Vector3Val force, Vector3Val friction)
{
  Vector3 res = force + friction;
  if (res[0] * force[0] <= 0.0f)
    res[0] = 0.0f;
  if (res[1] * force[1] <= 0.0f)
    res[1] = 0.0f;
  if (res[2] * force[2] <= 0.0f)
    res[2] = 0.0f;
  return res;
}

bool Tank::RemoteStateSupported() const
{
  return Type()->GetFuelCapacity()>0;
}

void Tank::Simulate(float deltaT, SimulationImportance prec)
{
  const TankType *type = Type();
  if (!_cargoLight && type->_cargoLightPos.Size() > 0)
  {
    LODShape *shape = GLOB_SCENE->Preloaded(Marker);
    _cargoLight = new LightPointOnVehicle
    (
      shape, type->_cargoLightDiffuse, type->_cargoLightAmbient, this, type->_cargoLightPos
    );
    _cargoLight->SetRadius(type->_cargoLightRadius);
    GScene->AddLight(_cargoLight);
  }

  Vector3Val speed=FutureVisualState().ModelSpeed();
  float speedSize=fabs(speed.Z());
  if( !_engineOff )
  {
    ConsumeFuel(deltaT*(0.01f+FutureVisualState()._rpm*0.02f));

    if( FutureVisualState()._fuel<=0 ) _engineOff=true, FutureVisualState()._fuel=0, _rpmWanted=0;
    else
    {
      // calculate engine rpm
      _rpmWanted=speedSize*_gearBox.Ratio();
      if( _rpmWanted<0.3f )
      {
        float avgThrust=(fabs(_thrustR)+fabs(_thrustL))*0.5f;
        _rpmWanted=avgThrust*0.7f+0.3f;
      }
      saturate(_rpmWanted,0,2);
    }
  }
  else
  {
    // engine off
    _rpmWanted=0;
  }
  AIBrain *driver = PilotUnit();
  if (!HasSomePilot() || !driver || !driver->LSIsAlive())
  {
    // no driver or driver is not alive - slowly return the pedals into the neutral position
    {
      float delta = -_thrustLWanted;
      Limit(delta,-0.1f*deltaT,+0.1f*deltaT);
      _thrustLWanted += delta;
    }
    {
      float delta = -_thrustRWanted;
      Limit(delta,-0.1f*deltaT,+0.1f*deltaT);
      _thrustRWanted += delta;
    }
    if (!HasSomePilot() || !driver)
    {
      // when there is no driver, once engine is off, activate brake
      // this is not very realistic, but should give reasonable results
      // with dead driver we assume the vehicle may go on unbraked
      if (fabs(_thrustL)<0.01f && fabs(_thrustR)<0.01f)
      {
        _pilotBrake = true;
      }
    }
    
  }
#if 0 // _VBS2 // following VBS2 specific code is no longer needed because of the code above
  if (!HasSomePilot() || (driver && !driver->LSIsAlive()))
  {
    // driver is dead - make the tank slowing down, sometimes make random control changes
    _thrustLWanted = 0;
    _thrustRWanted = 0;
    _rpmWanted -= deltaT*0.1f;
    if (_rpmWanted<=0)
    {
      _pilotBrake = true;
      _rpmWanted = 0;
    }
  }
#endif

  if (GetHit(type->_engineHit)>=0.9f || GetHit(type->_hullHit)>=0.9f)
  // tank is going to explode
  {
    if (IsLocal() && _explosionTime>Glob.time+60)
      // set some explosion
      _explosionTime=Glob.time+GRandGen.Gauss(2,5,20);
  }

  float delta;
  delta=_rpmWanted-FutureVisualState()._rpm;
  Limit(delta,-0.5f*deltaT,+0.3f*deltaT);
  FutureVisualState()._rpm+=delta;
  
  // TODO: commander view need not be attached to main turret
  // check actual selection

  MoveWeapons(deltaT);

  if (_isDead) _engineOff=true,_pilotBrake=true;
  else if(_isUpsideDown ) _pilotBrake=true;
  if( _engineOff ) _thrustLWanted=_thrustRWanted=0,_pilotBrake = true;

  if( fabs(_thrustLWanted)>0.1f || fabs(_thrustRWanted)>0.1f || FutureVisualState()._speed.SquareSize() > 0.0001 || _angVelocity.SquareSize() > 0.0001)
  {
    OnMoved();
  }

  //if( _impulseForce.SquareSize()>Square(GetMass()*1e-6f) )
  if( _impulseForce.SquareSize()>Square(GetMass()*0.005f))
  {
    OnMoved();
    // make this time longer that stop detection
    // so that if vehicle is stable long enough, it goes directly
    // from complete simulation to full stop
  }
  if( !_landContact )
  {
    OnMoved();
  }
  if (GetStopped())
  {
    // reset impulse - avoid accumulation
    _impulseForce = VZero;
    _impulseTorque = VZero;
    FutureVisualState()._modelSpeed = VZero;
    FutureVisualState()._speed = VZero;
    _angVelocity = VZero;
    _angMomentum = VZero;
  }

  static float simPrecisionSuspended = 0.2f;
  static float simPrecisionNormal = 1.0f/15;
  static float simPrecisionCollision = 1.0f/15;
  // when suspended, we can use very big step
  float simulationPrecision = simPrecisionSuspended;

  bool isStatic = type->GetFuelCapacity()<=0;
  if (!GetStopped() && !isStatic && !CheckPredictionFrozen() && !IsAttached())
  {
    // simulate left/right engine
    float deltaLOrig=floatMinMax(_thrustLWanted-_turnSpeedWanted,-1,+1)-_thrustL;
    float deltaROrig=floatMinMax(_thrustRWanted+_turnSpeedWanted,-1,+1)-_thrustR;


    float deltaL = deltaLOrig;
    float deltaR = deltaROrig;
    
    if( _thrustLWanted*_thrustL<=0 ) Limit(deltaL,-2.0f*deltaT,+2.0f*deltaT);
    else Limit(deltaL,-deltaT,+deltaT);

    if( _thrustRWanted*_thrustR<=0 ) Limit(deltaR,-2.0f*deltaT,+2.0f*deltaT);
    else Limit(deltaR,-deltaT,+deltaT);

    float times = 1.0f;
    if (deltaL != deltaLOrig)
      times = deltaLOrig!=0 ? deltaL / deltaLOrig : deltaL;

    if (deltaR != deltaROrig)
    {
      float timesTemp = deltaROrig!=0 ? deltaR / deltaROrig : deltaR;
      saturateMin(times,timesTemp);
    }

    deltaL = times * deltaLOrig;
    deltaR = times * deltaROrig;

    _thrustL+=deltaL;
    Limit(_thrustL,-1.0f,1.0f);
    
    _thrustR+=deltaR;
    Limit(_thrustR,-1.0f,1.0f);

    // calculate all forces, frictions and torques
    Vector3 force(VZero),friction(VZero);
    Vector3 torque(VZero),torqueFriction(VZero);

    Vector3 pForce(NoInit); // partial force
    Vector3 pCenter(NoInit); // partial force application point

    Vector3 wCenter(VFastTransform,FutureVisualState().ModelToWorld(),GetCenterOfMass());

    // apply left/right thrust

    bool gearChanged=false;
    if( _engineOff )
    {
      // upside down
      gearChanged=_gearBox.Neutral();
    }
    else
    {
      // when accelerating we want higher duty
      // when decelerating, we may want slightly lower duty
      Vector3 relAccel=DirectionWorldToModel(FutureVisualState()._acceleration);
      static float accelFactor = 0.1f;
      float accelDuty = 1+floatMinMax(relAccel.Z(),-2,+6)*accelFactor;
      // when going uphill we want a lot higher duty
      // when going downhill we want a slightly lower duty
      static float slopeFactor = 3.0f;
      float slopeDuty = 1+floatMinMax(FutureVisualState().Direction().Y(),-0.05f,0.2f)*slopeFactor;
      gearChanged=_gearBox.Change(speed.Z(),accelDuty*slopeDuty);
      if (_landContact || _objectContact || _waterContact)
      {
        // the more we turn, the more power we loose
        float eff=1-fabs(_thrustR-_thrustL)*0.4f;
        saturateMax(eff,0);
        float invSpeedSize;
        const float coefInvSpeed=3;
        const float defSpeed=80.0f; // model tuned at this speed (plain level grass)
        float power=type->GetMaxSpeed()*(1/defSpeed);
        // FIX: tank climbing (minInvSpeed was 2.5 in 1.00)
        const float minInvSpeed=2.0f;
        // FIX END
        const float invSpeedSizeMax=coefInvSpeed/(minInvSpeed*power);
        if( speedSize<minInvSpeed*power ) invSpeedSize=invSpeedSizeMax;
        else invSpeedSize=coefInvSpeed/speedSize;
        float rpmEff=0.7f;
        const float effC=3.0f*rpmEff*eff;
        float invSpeedSizeL=speed.Z()*_thrustL<0.1f ? invSpeedSizeMax : invSpeedSize;
        float invSpeedSizeR=speed.Z()*_thrustR<0.1f ? invSpeedSizeMax : invSpeedSize;
        invSpeedSizeL*=effC;
        invSpeedSizeR*=effC;
        //saturateMax(invSpeedSize,1/power);
        //float rpmEff=rpm*0.5+0.5;
        // water movement is much less efficient
        if( !_landContact && !_objectContact )
        {
          invSpeedSizeL*=0.1f;
          invSpeedSizeR*=0.1f;
        }
        float radius = CollisionSize();
        float turnEnhancer=speedSize*0.67f*(1/power);

        saturate(turnEnhancer,2,7);
        
        turnEnhancer *= radius*(1.0f/3);
        
        saturate(turnEnhancer,2,30);
        
        float lAccel=_thrustL*invSpeedSizeL*power;
        float rAccel=_thrustR*invSpeedSizeR*power;
        //GlobalShowMessage(100,"Enhancer %.2f",turnEnhancer);
        //LogF("_thrustL %lf, _thrustR %lf, lAccel %lf, rAccel %lf, invSpeedSizeL %lf, invSpeedSizeR %lf, power %lf, turnEnhancer %lf", _thrustL, _thrustR, lAccel, rAccel,invSpeedSizeL, invSpeedSizeR, power, turnEnhancer);

        rAccel *= 1-GetHit(type->_engineHit);
        lAccel *= 1-GetHit(type->_engineHit);
        rAccel *= 1-GetHit(type->_trackLHit); //empirical fix: tank has rotated to different side after track damaged
        lAccel *= 1-GetHit(type->_trackRHit);

        pForce=Vector3(0,0,lAccel*GetMass());
        force+=pForce;
        #if TANK_START_STOP_EFFECTS
        float torqueLevel = _shape->LandContactLevel() ? _shape->LandContactLevel()->Min().Y()-GetCenterOfMass().Y(): -1.0f;
        #else
        float torqueLevel = 0;
        #endif
        pCenter=Vector3(+7*turnEnhancer,torqueLevel,0); // relative to the center of mass
        torque+=pCenter.CrossProduct(pForce);
        #if ARROWS
          AddForce
          (
            DirectionModelToWorld(pCenter)+wCenter,
            DirectionModelToWorld(pForce*InvMass())
          );
        #endif

        pForce=Vector3(0,0,rAccel*GetMass());
        force+=pForce;
        pCenter=Vector3(-7*turnEnhancer,torqueLevel,0); // relative to the center of mass
        torque+=pCenter.CrossProduct(pForce);
        
        #if ARROWS
          AddForce(DirectionModelToWorld(pCenter)+wCenter, DirectionModelToWorld(pForce*InvMass()));
        #endif
      }
    }

    if (gearChanged)
      _doGearSound = true;

    // animate track
    // add movement forward (partially)
    float forwardChangePhase = FutureVisualState().ModelSpeed()[2]*0.5f;
    const float thrustCoef = 0.5f;

    // transform to <0, 1) on usage (different speed of wheels and tracks)
    FutureVisualState()._phaseL+=(_thrustL*thrustCoef+forwardChangePhase)*deltaT;
/*
    if( FutureVisualState()._phaseL>=1 ) FutureVisualState()._phaseL-=1;
    if( FutureVisualState()._phaseL<0 ) FutureVisualState()._phaseL+=1;
*/
    FutureVisualState()._phaseR+=(_thrustR*thrustCoef+forwardChangePhase)*deltaT;
/*
    if( FutureVisualState()._phaseR>=1 ) FutureVisualState()._phaseR-=1;
    if( FutureVisualState()._phaseR<0 ) FutureVisualState()._phaseR+=1;
*/

    // convert forces to world coordinates
    FutureVisualState().DirectionModelToWorld(torque,torque);
    FutureVisualState().DirectionModelToWorld(force,force);

    // apply gravity
    pForce=Vector3(0,-G_CONST*GetMass(),0);
    force+=pForce;
    
    #if ARROWS
      AddForce(wCenter,pForce*InvMass());
    #endif

    // angular velocity causes also some angular friction, this should be simulated as torque
    torqueFriction = _angMomentum * 1.2f;        

    // calculate new position
    VisualState moveTrans = PredictPos<VisualState>(deltaT);

    ApplyRemoteStateAdjustSpeed(deltaT,moveTrans);

    // body air friction
    FutureVisualState().DirectionModelToWorld(friction,Friction(speed));
    //friction=Vector3(0,0,0);
    #if ARROWS
      AddForce(wCenter,friction*InvMass());
    #endif
    
    //wCenter=moveTrans.PositionModelToWorld();
    // recalculate COM to reflect change of position
    wCenter.SetFastTransform(moveTrans.ModelToWorld(),GetCenterOfMass());

    const Texture *texture = NULL;
    CrashType doCrash = CrashNone;

    if( deltaT>0 )
    {
      // check collision on new position
      Vector3 totForce(VZero);

      float crash=0;
      _objectContact=false;
      if( prec<=SimulateVisibleFar && (IsLocal() || GetNetworkManager().IsServerOwner()))
      {
        ObjectContact(moveTrans, wCenter, deltaT, torque, friction, torqueFriction, totForce, crash, doCrash);
        if (_objectContact)
        {  // after a collision use precise simulation for a while
          _collisionSimUntil = Glob.time+1.0f;
        }
      }
      float nSpeed=type->GetMaxSpeed()*0.16f; // km/h
      bool brakeFriction=false;
      Vector3 fSpeed;
      if( _landContact && ( FutureVisualState().DirectionUp().Y()<=0.3f || _pilotBrake ) )
      {
        // brake friction speed
        brakeFriction=true;
        fSpeed=speed;
      }
      else
        fSpeed=speed-Vector3(0,0,(_thrustL+_thrustR)*nSpeed);
      float maxUnderWater=0;
      float softFactor=floatMin(5000*GetInvMass(),1.0f);
      #define MAX_UNDER_FORCE 0.4f
      const float UNDER_OFFSET = 0.0f;

      #define PENET_FORCES_COEF 15.0f
      #define COEF_N_POINTS 9.0f
      { // complex simulation - single point following is not enough, force computation necessary
        // Mark the flag we use the complex simulation
        simulationPrecision = Glob.time<_collisionSimUntil ? simPrecisionCollision : simPrecisionNormal;

        GroundCollisionBuffer gCollision;
        int lcPoints = GLandscape->GroundCollision(gCollision,this,moveTrans,UNDER_OFFSET,softFactor);
          ADD_COUNTER(tGndC,gCollision.Size());
        
        _landContact=false;
        _waterContact=false;
        #define MAX_UNDER 0.4f

        // it is hard to have more than 3 contact points
        saturateMin(lcPoints,5);
        // calculate how may points will be considered (negative points are ignored)
        // note: even with geometry 3 contact points should be quite enough to support stable position
        // note: in case of a tank, free fall may denote not only free fall, but also forced complex simulation
        {
          int nComputed = 0;
          for( int i=0; i<gCollision.Size(); i++ )
          {
            // info.pos is world space
            const UndergroundInfo &info=gCollision[i];
            // we consider two forces
            if( info.under<0 ) continue;
            nComputed++;
          }
          saturateMax(lcPoints,nComputed);
        }

        if (gCollision.Size() > 0) SetSurfaceInfo(gCollision);
        
        const float coefNPoints = COEF_N_POINTS/lcPoints;

        float maxUnder=0;
        for( int i=0; i<gCollision.Size(); i++ )
        {
          // info.pos is world space
          UndergroundInfo &info=gCollision[i];
          // we consider two forces
          if( info.under<0 ) continue;
          if( info.type==GroundWater )
          {
            _waterContact=true;
            // simulate swimming force
            //const float coefNPoints=3;
            // first is water is "pushing" everything up - causing some momentum
            saturateMax(maxUnderWater,info.under);

            pForce=Vector3(0,15000*info.under*coefNPoints,0);
            if( !type->_canFloat ) pForce*=0.3f;
            pCenter=info.pos-wCenter;
            torque+=pCenter.CrossProduct(pForce);
            totForce+=pForce;

            // add stabilizing torque
            // stabilized means DirectionUp() is (0,1,0)
            Vector3 stabilize=VUp-moveTrans.DirectionUp();
            torque+=Vector3(0,coefNPoints*1.8f*GetMass(),0).CrossProduct(stabilize);

            #if ARROWS
              AddForce(wCenter+pCenter,pForce*InvMass());
            #endif
            
            // second is "water friction" - causing no momentum
            pForce[0]=speed[0]*fabs(speed[0])*15;
            pForce[1]=speed[1]*fabs(speed[1])*15+speed[1]*40;
            pForce[2]=speed[2]*fabs(speed[2])*6;

            pForce=FutureVisualState().DirectionModelToWorld(pForce*info.under)*GetMass()*(coefNPoints/350);
            #if ARROWS
              AddForce(wCenter+pCenter,-pForce*InvMass());
            #endif
            friction+=pForce;
            torqueFriction+=_angMomentum*0.15f;

            if( FutureVisualState()._speed.SquareSize()>Square(8) ) crash=2;
          }
          else
          {
            _landContact=true;           
            if( maxUnder<info.under ) maxUnder=info.under;
            float under=floatMin(info.under,MAX_UNDER_FORCE);

            //const float coefNPoints=1.5f;
            // one is ground "pushing" everything out - causing some momentum
            Vector3 dirOut=Vector3(0,info.dZ,1).CrossProduct(Vector3(1,info.dX,0)).Normalized();
            pForce=dirOut*GetMass()*PENET_FORCES_COEF*under*coefNPoints;
            pCenter=info.pos-wCenter;
            torque+=pCenter.CrossProduct(pForce);
            // to do: analyze ground reaction force
            totForce+=pForce;

            #define CRASH_SPEED_LIMIT 4.0f           
            //float fSpeedInDir = SpeedAtPoint(info.pos) * dirOut;           
            float fSpeedInDir = FutureVisualState()._speed * dirOut;
            if (fSpeedInDir < -CRASH_SPEED_LIMIT)
            {
              crash -= coefNPoints * (fSpeedInDir + CRASH_SPEED_LIMIT) / (CRASH_SPEED_LIMIT * 9) ;
              //LogF("fSpeedInDir: %f, crash: %f", fSpeedInDir,crash );
            }

            #if ARROWS
              AddForce(wCenter+pCenter,pForce*under*InvMass());
            #endif
            
            // friction
            LandFriction(friction, torqueFriction, torque, brakeFriction, fSpeed, speed, pCenter, coefNPoints, info.texture);
            // select roughest texture
            if( !texture || info.texture && info.texture->Roughness()>texture->Roughness() )
              texture=info.texture;
          }
        }

        //GlobalShowMessage(100,"MaxUnder %.3f",maxUnder);        

        if (maxUnder > MAX_UNDER)
        {
          // it is neccessary to move object immediatelly
          Matrix4 transform=moveTrans.Transform();
          Point3 newPos=transform.Position();
          float moveUp=maxUnder-MAX_UNDER;
          newPos[1]+=moveUp;
          transform.SetPosition(newPos);
          moveTrans.SetTransform(transform);
          // we move up - we have to maintain total energy
          // what potential energy will gain, kinetic must loose
          const float crashLimit=0.3f;
          //if( moveUp>crashLimit ) crash+=moveUp-crashLimit;
          float potentialGain=moveUp*GetMass();
          float oldKinetic=GetMass()*FutureVisualState()._speed.SquareSize()*0.5f; // E=0.5*m*v^2
          // kinetic to potential conversion is not 100% effective
          float crashFactor=(moveUp-crashLimit)*4+1.5f;
          saturateMax(crashFactor,2.5f);
          float newKinetic=oldKinetic-potentialGain*crashFactor;
          //float newSpeedSize=sqrt(newKinetic*InvMass()*2);
          float newSpeedSize2=newKinetic*InvMass()*2;
          // _speed=_speed*_speed.InvSize()*newSpeedSize
          if( newSpeedSize2<=0 || oldKinetic<=0 ) FutureVisualState()._speed=VZero;
          else FutureVisualState()._speed*=sqrt(newSpeedSize2*FutureVisualState()._speed.InvSquareSize());
        }
      } // simple simulation not applicable

      force+=totForce;
      
      if (doCrash != CrashNone)
      {
        // old style
        if (crash > 0.1f)
        {
          if( Glob.time>_disableDamageUntil )
          {
            // crash boom bang state - impact speed too high
            _doCrash=CrashLand;

            if( _objectContact) _doCrash=CrashObject;
            if( _waterContact ) _doCrash=CrashWater;

            _crashVolume = crash*0.5f;
            if( _doCrash==CrashWater ) crash*=0.1f;
          }
        }
        else
        {
          if (doCrash != CrashNone)
          {
            // crash returned from ObjectContact is zero (very low) for trees or cars
            _doCrash = doCrash;
            _crashVolume = 4.0f * floatMinMax(FutureVisualState()._speed.Size() / Type()->_maxSpeed, 0, 1);
          }
          else
            _crashVolume = 0;
        }
      }
      if( !type->_canFloat && IsLocal())
      {
        const float maxFord=1.8f;
        if( maxUnderWater>maxFord )
        {
          float damage=(maxUnderWater-maxFord)*0.5f;
          saturateMin(damage,0.2f);
          LocalDamageMyself(VZero,damage*deltaT,GetRadius());
        }
      }
    }

    if (_objectContact)
      _turretFrontUntil = Glob.time+30;
    // apply all forces
    // handle special case: brake is on, speed is very low and total forces are low
    Vector3 frictionedForce = AddFriction(force, -friction);
    Vector3 frictionedTorque = AddFriction(torque, -torqueFriction);
    float staticFriction = 0.0f;
    if (_pilotBrake && _landContact)
      staticFriction = GetMass()*G_CONST*0.5f; // assume brakes can hold 0.5 G
    
    /*LogF("force %.2f, torque %.2f, fric %.2f, fricTorque %.2f, staticFriction %.2f, speed %.2f, angv %.2f",
      force.Size()*GetInvMass(),torque.Size()*GetInvMass(),friction.Size()*GetInvMass(),
      torqueFriction.Size()*GetInvMass(),staticFriction*GetInvMass(),
      Speed().Size(),_angVelocity.Size());*/
    
 
    ApplyForces(deltaT,force,torque,friction,torqueFriction,staticFriction);

    bool stopCondition = false;
    if (_pilotBrake && _landContact && !_waterContact && !_objectContact)
    {
      // apply static friction
      float maxSpeed = Driver() ? Square(0.7f) : Square(1.2f);
      if (FutureVisualState()._speed.SquareSize()<maxSpeed && _angVelocity.SquareSize()<maxSpeed*0.3f)
      {
        if(!CHECK_DIAG(DECollision)) // If tank is stopped we do not see collision diagnostic. Will be removed later
          stopCondition = true;
      }
    }
    if (stopCondition)
      StopDetected();
    else
      OnMoved();

    // simulate track drawing   
    if( EnableVisualEffects(prec,300) )
    {
      // Update the track - either prolong or terminate
      _track.Update(RenderVisualStateRaw(), deltaT, !_landContact);  // based on the visible state

      // Dust
      if( _landContact )
      {
        float dust=texture ? texture->Dustness()*0.7f : 0.1f;
        // check texture under 
        Vector3 lPos = _track.LeftPos() + Vector3(+0.5f,0.1f,0) + 0.2f * FutureVisualState().ModelSpeed();
        Vector3 rPos = _track.RightPos() + Vector3(-0.5f,0.1f,0) + 0.2f * FutureVisualState().ModelSpeed();
        Vector3 cSpeed = 0.25f * FutureVisualState()._speed;

        float dSoft=floatMax(dust,0.003f);

        float lDensity = (speedSize*0.5f+fabs(_thrustL))*dSoft;
        float rDensity = (speedSize*0.5f+fabs(_thrustL))*dSoft;

        saturateMin(lDensity,1);
        saturateMin(rDensity,1);
        float dustColor=dSoft*8;
        saturate(dustColor,0,1);

        if (lDensity>0.2f)
        {
          float leftDustValues[] = {lDensity, dustColor, lPos.X(), lPos.Y(), lPos.Z(), cSpeed.X(), cSpeed.Y(), cSpeed.Z()};
          _leftDust.Simulate(deltaT, prec, leftDustValues, lenof(leftDustValues));
        }
        if (rDensity>0.2f)
        {
          float rightDustValues[] = {rDensity, dustColor, rPos.X(), rPos.Y(), rPos.Z(), cSpeed.X(), cSpeed.Y(), cSpeed.Z()};
          _rightDust.Simulate(deltaT, prec, rightDustValues, lenof(rightDustValues));
        }

/*
        Color color=Color(0.51f,0.46f,0.33f)*dustColor+Color(0.5f,0.5f,0.5f)*(1-dustColor);
        _leftDust.SetColor(color);
        _rightDust.SetColor(color);
        if (lDensity>0.2f)
        {
          _leftDust.Simulate(lPos+_speed*0.2f,Speed()*0.25f,lDensity,deltaT);
        }
        if (rDensity>0.2f)
        {
          _rightDust.Simulate(rPos+_speed*0.2f,Speed()*0.25f,rDensity,deltaT);
        }
*/
        if( _fireDustTimeLeft>0 )
        {
          float phase=1-_fireDustTimeLeft*_invFireDustTimeTotal;
          const float thold=0.02f;
          if( phase>thold ) phase=(1-phase)*(1/(1-thold));
          else phase=phase*(1/thold);
          float density=phase*dSoft*1.5f;
          saturateMin(density,1);

          TurretContext context; 
          Vector3 weaponDir = GetPrimaryGunnerTurret(context) ?
            GetWeaponDirection(FutureVisualState(), context, WeaponModePtr()) : FutureVisualState().Direction();

          Vector3 dustPos=(lPos+rPos)*0.5f+DirectionWorldToModel(weaponDir)*6;
          Vector3 dustSpeed(GRandGen.RandomValue()-0.5f,
            GRandGen.RandomValue()*0.8f+0.2f,
            GRandGen.RandomValue()-0.5f);
          if (density > 0)
          {
            float fireDustValues[] = {density, dustColor, dustPos.X(), dustPos.Y(), dustPos.Z(),
              dustSpeed.X(), dustSpeed.Y(), dustSpeed.Z()};
            _fireDust.Simulate(deltaT, prec, fireDustValues, lenof(fireDustValues));
          }
/*
          Color color=Color(0.51f,0.46f,0.33f)*dustColor+Color(0.5f,0.5f,0.5f)*(1-dustColor);
          _fireDust.SetColor(color);
          _fireDust.Simulate(dustPos,dustSpeed,density,deltaT);
*/
          _fireDustTimeLeft-=deltaT;
        }
      }
      if( _waterContact )
      {
        if( type->_canFloat )
        {
          Vector3 lPos=FutureVisualState().PositionModelToWorld(type->_lWaterPos);
          Vector3 rPos=FutureVisualState().PositionModelToWorld(type->_rWaterPos);

          // place on water surface
          lPos[1]=GLandscape->WaterSurfaceY(lPos); // sea level
          rPos[1]=GLandscape->WaterSurfaceY(rPos);
          // TODO: some more effective way to avoid too much conversions
          lPos = PositionWorldToModel(lPos);
          rPos = PositionWorldToModel(rPos);

          float dens=floatMin(speedSize*0.3f,0.7f);
          float densL=fabs(_thrustL)+dens;
          float densR=fabs(_thrustR)+dens;
          saturateMin(densL,1);
          saturateMin(densR,1);
          float coefL=_thrustL*0.3f;
          float coefR=_thrustR*0.3f;
          saturateMax(coefL,0.05f);
          saturateMax(coefR,0.05f);
          Vector3 spdL=FutureVisualState().Speed()*0.1f-FutureVisualState().DirectionModelToWorld(Vector3(0,0,4)*coefL);
          Vector3 spdR=FutureVisualState().Speed()*0.1f-FutureVisualState().DirectionModelToWorld(Vector3(0,0,4)*coefR);

          const float size = 0.1f;
          if (densL > 0.1f)
          {
            float leftWaterValues[] = {densL, size, lPos.X(), lPos.Y(), lPos.Z(), spdL.X(), spdL.Y(), spdL.Z()};
            _leftWater.Simulate(deltaT, prec, leftWaterValues, lenof(leftWaterValues));
          }
          if (densR > 0.1f)
          {
            float rightWaterValues[] = {densR, size, rPos.X(), rPos.Y(), rPos.Z(), spdR.X(), spdR.Y(), spdR.Z()};
            _rightWater.Simulate(deltaT, prec, rightWaterValues, lenof(rightWaterValues));
          }
        }
      }
    }

    SimulateExhaust(deltaT,prec);
  
    StabilizeTurrets(FutureVisualState().Transform().Orientation(),moveTrans.Orientation());

    Move(moveTrans);
    DirectionWorldToModel(FutureVisualState()._modelSpeed, FutureVisualState()._speed);
  } 
  else// if (!GetStopped() && !isStatic)
  {
#if _ENABLE_ATTACHED_OBJECTS
  //otherwise static weapons eliminate rope simulation!
  //TODO: check if which part of simulation should be run if attached!
    if(!IsAttached()) 
#endif
    if (isStatic && !GetStopped())
    {
      // do not move
      FutureVisualState()._speed = VZero;
      FutureVisualState()._modelSpeed = VZero;
      _angVelocity = VZero;
      _angMomentum = VZero;
      _impulseForce = VZero;
      _impulseTorque = VZero;
      
      // even if tank is static, do collision detection and damage it
      if( prec<=SimulateVisibleFar && IsLocal())
      {
        float crash = 0; 
        ObjectContactStatic(deltaT, crash);

        if( crash>0.1f && IsLocal())
        {
          if( Glob.time>_disableDamageUntil )
          {
            // crash boom bang state - impact speed too high            
            _doCrash=CrashObject;            
            _crashVolume=crash*0.5f;    

            LocalDamageMyself(VZero, crash, 1);
          }
        }
      } // if( object collisions enabled )       
    }
  }

  // slow steering autocentering
  static float centerSpeedFast = 0.05f;
  static float centerSpeedSlow = 0.00f;
  static float centerAngleFast = 0.05f;
  static float centerAngleSlow = 0.15f;
  float centerSpeed = Interpolativ(fabs(_turnSpeedWanted),centerAngleFast,centerAngleSlow,centerSpeedFast,centerSpeedSlow);
  _turnSpeedWanted = TowardZero(_turnSpeedWanted,deltaT*centerSpeed);

  base::Simulate(deltaT,prec);
  SimulatePost(deltaT,prec);

  // set simulation precision for a next step
  SetSimulationPrecision(simulationPrecision);
}

AnimationSourceTankDamper::AnimationSourceTankDamper()
:base(RStringB())
{
  // TODO: avoid anonymous dampers
  // bone will be initialized later
  _bone = SkeletonIndexNone;
  _center = VZero;
}

float AnimationSourceTankDamper::GetPhase(const ObjectVisualState &vs, const Animated *obj) const
{
  PROFILE_SCOPE(tnkGP);
  // if center is not initialized, do not animate
  if (_center.SquareSize()<0.1f)
    return 0;
  // convert center location to world space
  Vector3 surfPos = vs.PositionModelToWorld(_center);
  // add surface randomization
  float dX,dZ,bump; // dummy return values
  Texture *tex;
  
  float lY = GLandscape->RoadSurfaceY(surfPos + VUp * 0.5,&dX,&dZ,&tex,Landscape::UONoLock);
  bump = tex ? GLandscape->CalculateBump(surfPos.X(),surfPos.Z(),1) * tex->Roughness() : 0.0f;
  float radius = 0.0f;
  float dist = lY - surfPos.Y() + radius + floatMax(bump*1.5,0);
  saturate(dist,-0.10f,+0.3f); // positive is wheel up
  return dist;
}


TankDamperHolder::TankDamperHolder()
{
}

AnimationSource *TankDamperHolder::CreateAnimationSource()
{
  AnimationSourceTankDamper *source = new AnimationSourceTankDamper();
  _sources.Add(source);
  return source;
}

void TankDamperHolder::InitLandcontact(LODShape *shape, int level, const AnimationHolder &anims, const AnimationSourceHolder &sources)
{
  Assert(level==shape->FindLandContactLevel());

  // LODShape is already loaded, and all animations are registered
  // for each damper scan animations for a corresponding skeleton bone
  // prepare wheel index information
  _sources.Compact();
  for (int w=0; w<_sources.Size(); w++)
  {
    AnimationSourceTankDamper *source = _sources[w];
    int index = sources.Find(source);
    Assert(index>=0);
    AnimationType *type = anims[index];
    source->_bone = type->GetBone(level);
    // check animation center
    // we need to calculate it manually here
    source->_center = type->GetCenter();
  }
}

void TankDamperHolder::InitShapeLevel(LODShape *shape, int level, const AnimationHolder &anims, const AnimationSourceHolder &sources)
{
  for (int w=0; w<_sources.Size(); w++)
  {
    AnimationSourceTankDamper *source = _sources[w];
    int index = sources.Find(source);
    DoAssert(index>=0);
    AnimationType *type = anims[index];
    source->_bone = type->GetBone(level);
    // check animation center
    // we need to calculate it manually here
    if (source->_center.SquareSize()<0.1f)
      // if center not defined for given level, function will return VZero
      source->_center = type->GetCenter(level);
  }
}

void TankDamperHolder::Clear()
{
  _sources.Clear();
}


#include <Es/Common/delegate.hpp>

AnimationSource *TankType::CreateAnimationSource(const AnimationType *type, RStringB source)
{
  if (!strcmpi(source,"damper"))
    /// create a damper state, needs to be linked with a bone
    return _dampers.CreateAnimationSource();
  return base::CreateAnimationSource(type,source);
}


AnimationStyle Tank::IsAnimated( int level ) const {return AnimatedGeometry;}

void Tank::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  ShapeUsed shape = _shape->Level(level);
  if( shape.IsNull() ) return;

  // set gun and turret to correct position
  // calculate animation transformation
  // turret transformation
  
  const TankType *type = Type();

  if (_weaponsState.ShowFire())
  {
    type->_weapons._animFire.Unhide(animContext, _shape, level);
    type->_weapons._animFire.SetPhase(animContext, _shape, level, _weaponsState._mGunFirePhase);
  }
  else
    type->_weapons._animFire.Hide(animContext, _shape, level);
  
  base::Animate(animContext, level, setEngineStuff, parentObject, dist2);

  // TODO: animate min-max box
  // set min-max box and sphere
  Vector3 min = shape->Min();
  Vector3 max = shape->Max();
  Vector3 bCenter = shape->BSphereCenter();
  float bRadius = shape->BSphereRadius();
  // enlarge sure vehicle will fit it
  Vector3 factor(3,1.2f,1.2f);
  float sFactor = 2.5f;
  min = bCenter+factor.Modulate(min-bCenter);
  max = bCenter+factor.Modulate(max-bCenter);
  bRadius *=sFactor;
  animContext.SetMinMax(min, max, bCenter, bRadius);
}

void Tank::Deanimate( int level, bool setEngineStuff )
{
  if( _shape->Level(level).IsNull() ) return;
  base::Deanimate(level,setEngineStuff);
}

void Tank::PreloadGetIn(UIActionType position)
{
  base::PreloadGetIn(position);

  // UI textures
  // - tank direction
  if (Type()->HasObserver())
  {
    if (GWorld && GWorld->UI())
      GWorld->UI()->PreloadTankDirection();
  }
}

RString Tank::DiagText() const
{
  char buf[512];
  float dx,dz;
  Vector3 pos = FutureVisualState().Position();

  pos[1] = GLandscape->SurfaceYAboveWater(pos[0], pos[2], &dx, &dz);

  float slope = FutureVisualState().Direction().Z()*dz + FutureVisualState().Direction().X()*dx;

  sprintf(buf,"  Slope %.0f %%, %s %.2f,%.2f",
    slope*100, _pilotBrake ? "B " : "E ", _thrustLWanted, _thrustRWanted);

  return base::DiagText()+RString(buf);
}

void Tank::Eject(AIBrain *unit, Vector3Val diff)
{
  base::Eject(unit, diff);
}

void Tank::FakePilot(float deltaT)
{
}

void Tank::SuspendedPilot(AIBrain *unit, float deltaT)
{
  _turnSpeedWanted = 0;
  _turnSpeedWantedLastActive = TIME_MIN;
}

void Tank::KeyboardPilot(AIBrain *unit, float deltaT)
{
  if (!unit->GetPerson() || unit->GetPerson()->KeyboardUsedInFrame()) return;
#if _VBS3 // convoy trainer, indicators
  DoIndicators(GInput.GetActionToDo(UAIndicateLeft,true,false),GInput.GetActionToDo(UAIndicateRight,true,false),GInput.GetActionToDo(UAHazardLights,true,false));
#endif

  float aimX(0), aimY(0);
  if (unit->LookAroundEnabled())
  {
    aimX = GInput.GetActionWithCurveExclusive(InputInterface::CurveFovMouse, UACarAimRight, UACarAimLeft);
    aimY = GInput.GetActionWithCurveExclusive(InputInterface::CurveFovMouse, UACarAimUp, UACarAimDown);
  }
  else if (Glob.config.IsEnabled(DTWeaponCursor) && (GWorld->GetCameraType()==CamExternal || GWorld->GetCameraType()==CamInternal))
  {
    float aimCursorY = GInput.GetActionWithCurveExclusive(InputInterface::CurveFovMouse, UACarAimUp, UACarAimDown);

    static float vertFactor = 0.15f;
    _cursorY -= aimCursorY*vertFactor;

    float moveAimY = 0;

    static float vertYMin = 0.1f;
    static float vertYMax = 0.9f;
    if (_cursorY>vertYMax) moveAimY -= _cursorY-vertYMax, _cursorY = vertYMax;
    if (_cursorY<vertYMin) moveAimY -= _cursorY-vertYMin, _cursorY = vertYMin;

    static float headFactor = 60.0f;
    aimY = moveAimY*headFactor;
  }

  float forward=(GInput.GetAction(UACarForward)-GInput.GetAction(UACarBack))*0.75f;
  forward+=GInput.GetActionCarFastForward();
  forward+=GInput.GetAction(UACarSlowForward)*0.33f;

  if (FutureVisualState().ModelSpeed()[2]>2 && forward<-0.5) _backwardUsedAsBrake = true;
  else if (forward>=0) _backwardUsedAsBrake = false;

  if (FutureVisualState().ModelSpeed()[2]<-2 && forward>+0.5) _forwardUsedAsBrake = true;
  else if (forward<=0) _forwardUsedAsBrake = false;

  bool brake = _backwardUsedAsBrake || _forwardUsedAsBrake;
  if (brake)
  {
    forward = -FutureVisualState().ModelSpeed()[2]*10;
    saturate(forward,-1,1);
    if (FutureVisualState()._speed.SquareSize()<0.2f)
    {
      _parkingBrake = true;
      _backwardUsedAsBrake = _forwardUsedAsBrake = false;
    }
  }
  else
  {
    if (fabs(forward)>0.25f)
      _parkingBrake = false;
  }

  _thrustRWanted=_thrustLWanted=forward;

  float estT = 0.10f;
  bool fullTurn = fabs(_thrustLWanted+_thrustRWanted)<0.01f;
  if (!fullTurn) estT *= 2;
 
  // estimate heading
  Matrix3Val orientation=FutureVisualState().Orientation();
  Matrix3Val derOrientation=_angVelocity.Tilda()*orientation;
  Matrix3Val estOrientation=orientation+derOrientation*estT;
  Vector3Val estDirection=estOrientation.Direction();

  float curHeading=atan2(FutureVisualState().Direction()[0],FutureVisualState().Direction()[2]);
  float estHeading=atan2(estDirection[0],estDirection[2]);

  float turnWanted = 0;

  {
    // aim and turn with aiming actions
    LookAround(aimX, aimY, deltaT, &Type()->_viewPilot, unit, false, true);
    //UACarLeft and UACarAimLeft are independent actions, we can look around and turn simultaneously
    // note: keys give wanted turning speed, not turning acceleration
    float mouseLR = (GInput.GetAction(UACarRight)-GInput.GetAction(UACarLeft))*Input::MouseRangeFactor;
    static float turnFactor = 0.15f;
    static float turnFactorSpeed = 0.5f;
    static float coefLR = 1.0f;

    //UACarAimLeft and UACarLeft are separate now, so looking around and turning can be processed simultaneously
    // turn with arrows, mouse steering - more left, more right
    float turnSpeedWanted = (GInput.GetAction(UACarWheelRight)-GInput.GetAction(UACarWheelLeft))*Input::MouseRangeFactor;
    if (fabs(turnSpeedWanted)>0.01f) _turnSpeedWantedLastActive = Glob.time;

#if _ENABLE_CHEATS
    if (CHECK_DIAG(DEMouseSensitivity))
    {
      if (GInput.GetKeyToDo(KEYBOARD_COMBO_OFFSET*DIK_RCONTROL + DIK_NUMPADPLUS)) coefLR -= 0.1;
      else if (GInput.GetKeyToDo(KEYBOARD_COMBO_OFFSET*DIK_RCONTROL + DIK_NUMPADMINUS)) coefLR += 0.1;
      saturate(coefLR,0.1, 50);
      DIAG_MESSAGE(500, Format("Tank: mouseSensitivity = %.1f", coefLR));
    }
#endif
    mouseLR *= coefLR;
#if _VBS3 //going backwards we want to swap direction
    if(forward < 0.0f) mouseLR *= -1;
#endif
    float turnKey = mouseLR * turnFactor;
    // when moving fast, we want to turn slowly

    float slow = floatMax(0,1-fabs(FutureVisualState().ModelSpeed().Z())*(1.0f/20));

    float factor = 1 - slow*0.5f;
    //turnWanted = AngleDifference(curHeading+ turnKey*factor,estHeading);
    turnWanted = (AngleDifference(curHeading,estHeading)+ turnKey*factor)* Type()->_turnCoef;
    //_thrustLWanted+=GInput.GetAction(UATurnLeft)-GInput.GetAction(UATurnRight);
    //_thrustRWanted-=GInput.keyTurnLeft-GInput.keyTurnRight;
    _turnSpeedWanted += (turnSpeedWanted * factor * turnFactorSpeed * deltaT * Type()->_turnCoef);
    Limit(_turnSpeedWanted, -H_PI/2, +H_PI/2);
  }


  // special case - tank moving fast and players wants to brake
  // in such situation tank is usually out of control
  // we need to brake first and turn later
  float bFactor = FutureVisualState().ModelSpeed()[2]*forward;
  //GlobalShowMessage(100,"bFactor %.2f",bFactor);
  if (bFactor<-7) saturate(turnWanted,-0.05f,0.05f);

  _thrustLWanted -= turnWanted*8;
  _thrustRWanted += turnWanted*8;

  // if we are goind forward, no track can go backward
  if( _thrustLWanted+_thrustRWanted>0.01f )
  {
    saturateMax(_thrustLWanted,0);
    saturateMax(_thrustRWanted,0);
  }
  else if (_thrustLWanted+_thrustRWanted<-0.01f )
  {
    saturateMin(_thrustLWanted,0);
    saturateMin(_thrustRWanted,0);
  }
  Limit(_thrustLWanted,-1,1);
  Limit(_thrustRWanted,-1,1);

  if ((fabs(_thrustLWanted-_turnSpeedWanted)+fabs(_thrustRWanted+_turnSpeedWanted)<0.1f || brake || _parkingBrake)
    && fabs(FutureVisualState()._modelSpeed.Z())<5.0f)
    _pilotBrake = true;
  else
  {
    if (FutureVisualState().ModelSpeed()[2]*(_thrustLWanted+_thrustRWanted)<-4)
      _pilotBrake = true;
    else
      _pilotBrake = false;
    if (fabs(_thrustLWanted-_turnSpeedWanted)+fabs(_thrustRWanted+_turnSpeedWanted)>0.1f)
    {
      CancelStop();
      EngineOn();
    }
  }

}

#define DIAG_SPEED 0

#if _ENABLE_AI
void TankWithAI::AIPilot(AIBrain *unit, float deltaT)
{
  // TODO: limit AIPilot simulation rate (10 ps)
  Assert( unit );
  if (!unit) return;

  Vector3Val speed=FutureVisualState().ModelSpeed();
  
  SteerInfo info;
  info.headChange=0;
  info.speedWanted=0;
  info.turnPredict=0;

  if( unit->GetState()==AIBrain::Stopping )
  {
    // special handling of stop state
    if( fabs(speed[2])<1 )
    {
      unit->OnStepCompleted();
    }
    info.speedWanted=0;
    info.headChange=0;
  }
  else if( unit->GetState()==AIBrain::Stopped )
  {
    // special handling of stop state
    info.speedWanted=0;
    info.headChange=0;
  }
  else
    // subgroup leader - if we are near the target we have to operate more precisely
    LeaderPilot(info);

  #if DIAG_SPEED
  if( this==GWorld->CameraOn() )
    LogF("Pilot %.1f",speedWanted*3.6);
  #endif

  //LogF("info.speedWanted %lf, info.headChange %lf", info.speedWanted,info.headChange);

  AvoidCollision(deltaT,info.speedWanted,info.headChange);


  #if DIAG_SPEED
  if( this==GWorld->CameraOn() )
    LogF("AvoidCollision %.1f",info.speedWanted*3.6);
  #endif

  float curHeading=atan2(FutureVisualState().Direction()[0],FutureVisualState().Direction()[2]);
  float wantedHeading=curHeading+info.headChange;

  // estimate inertial orientation change
  Matrix3Val orientation=FutureVisualState().Orientation();
  Matrix3Val derOrientation=_angVelocity.Tilda()*orientation;
  Matrix3Val estOrientation=orientation+derOrientation * 0.4f;
  Vector3Val estDirection=estOrientation.Direction();
  float estHeading=atan2(estDirection[0],estDirection[2]);

  info.headChange= AngleDifference(wantedHeading,estHeading);

  _turnSpeedWanted = 0;
  _turnSpeedWantedLastActive = TIME_MIN;
  {
    float maxSpeed=GetType()->GetMaxSpeedMs();
    float limitSpeed=Interpolativ(fabs(info.turnPredict),H_PI/16,H_PI/4,maxSpeed,3);
    float limitSpeedC=Interpolativ(fabs(info.headChange),H_PI/32,H_PI/2,maxSpeed,0);
    #if DIAG_SPEED
    if( this==GWorld->CameraOn() )
      LogF("Turn limit %.1f (%.3f, turn %.3f)",limitSpeed,info.headChange,info.turnPredict);
    #endif

    saturate(info.speedWanted,-limitSpeed,+limitSpeed);
    saturate(info.speedWanted,-limitSpeedC,+limitSpeedC);
    saturate(info.speedWanted,-_limitSpeedForced,+_limitSpeedForced);
  }


  if (fabs(info.speedWanted)>0.2f) EngineOn();
  if (fabs(info.headChange)>0.2f) EngineOn();

  Vector3 relAccel=DirectionWorldToModel(FutureVisualState()._acceleration);
  float changeAccel=(info.speedWanted-speed.Z())*(1/0.5f)-relAccel.Z();
  // some thrust is needed to keep speed
  float isSlow=1-fabs(speed.Z())*(1.0f/17);
  saturate(isSlow,0.2f,1);

  float isLevel=1-fabs(FutureVisualState().Direction()[1]*(1.0f/0.6f));
  saturate(isLevel,0.2f,1);
  saturateMax(isSlow,isLevel); // change thrust slowly on steep surfaces
  changeAccel*=isSlow;
  float thrustOld=(_thrustL+_thrustR)*0.5f;
  float thrust=thrustOld+changeAccel*0.33f;
  Limit(thrust,-1,1);

  const float rotCoef=5/10.0f*Type()->_turnCoef;
  
  _thrustLWanted=thrust-info.headChange*rotCoef;
  _thrustRWanted=thrust+info.headChange*rotCoef;


  if (fabs(_thrustLWanted)>1)
  {
    _thrustRWanted += (1 - fabs(_thrustLWanted))* fSign(_thrustLWanted);
    _thrustLWanted =  fSign(_thrustLWanted);
  }
  else if (fabs(_thrustRWanted)>1)
  {
    _thrustLWanted += (1 - fabs(_thrustRWanted))* fSign(_thrustRWanted);;
    _thrustRWanted =  fSign(_thrustRWanted);
  }

  //LogF("_thrustLWanted %lf, _thrustRWanted %lf", _thrustLWanted, _thrustRWanted);
  Limit(_thrustLWanted,-1,1);
  Limit(_thrustRWanted,-1,1);
  

  #if DIAG_SPEED
  if( this==GWorld->CameraOn() )
    LogF("Thrust %.1f L %.1f R %.1f",thrust,_thrustLWanted,_thrustRWanted);
  #endif

  if( fabs(info.headChange)<0.05f && fabs(info.speedWanted)<0.5f )
  {
    if( fabs(speed[2])<0.5f ) _thrustLWanted=_thrustRWanted=0;
    _pilotBrake=true;
  }
  else if( fabs(speed[2])<5 && fabs(info.speedWanted)<0.5f && fabs(info.headChange)<0.5f )
    _pilotBrake=true;
  else
    _pilotBrake=false;
}

void TankWithAI::AIGunner(TurretContextEx &context, float deltaT)
{
  if( _isDead || _isUpsideDown ) return;
  base::AIGunner(context, deltaT);
}

#endif //_ENABLE_AI


void TankWithAI::Simulate(float deltaT, SimulationImportance prec)
{
  PROFILE_SCOPE_EX(simTn,sim);
  // if damaged or upside down, tank is dead
  _isUpsideDown=FutureVisualState().DirectionUp().Y()<0.3f;
  _isDead=IsDamageDestroyed();

  base::Simulate(deltaT,prec);
}

#pragma warning(disable:4355)

TankWithAI::TankWithAI(const EntityAIType *name, Person *driver, bool fullCreate)
:Tank(name, driver, fullCreate)
{
}

TankWithAI::~TankWithAI()
{
}


void Tank::SimulateImpulseDamage(DoDamageResult &result, EntityAI *owner, float impulse, Vector3Par modelPos)
{
  float damage = impulse * GetInvMass();
  damage += damage * damage / 20.0f;
  damage *= 0.03f;
  float damageObj = damage * GetInvArmor();

  if (damageObj > 0.02f)
    LocalDamage(result,ShotInfo(NULL,owner, this, modelPos), damageObj, GetRadius() * 0.5); 

  damage *= 0.8;

  if (damage > 0.02f)
    DamageCrew(owner, damage , "");
}

void Tank::DamageOnImpulse(EntityAI *owner, float impulse, Vector3Par modelPos)
{
  if (!IsLocal())
    return;

  // we can 
  float damage = impulse * GetInvMass();
  damage += damage * damage / 20.0f;
  damage *= 0.03f;
  float damageObj = damage * GetInvArmor();

  if (damageObj > 0.02f)
  {    
    DoDamageResult result;
    LocalDamage(result, ShotInfo(NULL,owner, this, modelPos), damageObj, GetRadius() * 0.5); 
    // TODO: find a skinned position in the fire geometry. For Dayz this is not interested, we need this only for bleeding
    ApplyDoDamageLocalHandleHitBy(result,owner, RString(), SkinnedPosition());
  }

  damage *= 0.8;

  if (damage > 0.02f)
    DamageCrew(owner, damage , "");
}

static const float ObjPenalty1[]={1.0,1.05,1.1,1.4};

static const float ObjPenalty2[]={1.0,1.10,1.25,1.5};

static const float ObjRoadPenalty2[]={1.0,1.05,1.1,1.2};
static const float ObjRoadHardPenalty[]={1.0,1.1,1.2,1.6};

static const float ObjHardPenalty[]={1.0,1.5,2.0,4.0};

static const float RoadPenalty[4]={0.1f,1.0f,1.0f,1.2f};

const float RoadFaster=1.2;
float TankType::GetFieldCost( const GeographyInfo &info, CombatMode mode ) const
{
  // road fields are expected to be faster
  // fields with objects will be passed through slower
  int nObj=info.u.howManyObjects;
  int hObj=info.u.howManyHardObjects;
  Assert( nObj<=3 );
  // tanks should ignore roads when not safe
  if (info.u.road)
  {
    int modeIndex = mode-CMSafe;
    saturate(modeIndex, 0, 3);
    return RoadPenalty[modeIndex]*ObjRoadPenalty2[nObj]*ObjRoadHardPenalty[hObj];
  }
  else
    return ObjPenalty2[nObj]*ObjHardPenalty[hObj];
}

float TankType::GetBaseCost(const GeographyInfo &geogr, bool operative, bool includeGradient) const
{
  float cost=GetMinCost()*RoadFaster;
  // avoid any water
  //if( geogr.road ) cost=1.0/20; // road speed is 12 m/s 
  // if there is some deep water, we assume floating is necessary
  if (geogr.u.maxWaterDepth>=2 && !geogr.u.road)
    return ( _canFloat ? 1.0f/2 : 1e30f ); // in water - speed 2 m/s
  // avoid forests
  if( geogr.u.full ) return 1e30;
  // penalty for objects
  int nObj=geogr.u.howManyObjects;
  Assert( nObj<=3 );
  cost *= ObjPenalty1[nObj];
  if (includeGradient)
  {
    // avoid steep hills
    // penalty for hills
    int grad = geogr.u.gradient;
    if( grad>=6 ) return 1e30;
    static const float gradPenalty[6]={1.0,1.0,1.05,1.1,2.0,3.0};
    cost *= gradPenalty[grad];
  }
  // penalty for shallow water - fording
  if( geogr.u.maxWaterDepth==1 && !geogr.u.road) cost+= ( 1.0f/6 ); // ford speed expected 6 m/s
  return cost;
}

float TankType::GetGradientPenalty(float gradient) const
{
  // TODO: continuous function, sign of gradient
  gradient = fabs(gradient);

  if (gradient >= 0.60f) return 1e30f; // level 6
  else if (gradient >= 0.40f) return 3.0f; // level 5
  else if (gradient >= 0.25f) return 2.0f; // level 4
  else if (gradient >= 0.15f) return 1.1f; // level 3
  else if (gradient >= 0.10f) return 1.05f; // level 2
  else if (gradient >= 0.05f) return 1.0f; // level 1
  return 1.0f; // level 0
}

float TankType::GetCostTurn(int difDir, int &debet, bool forceFullTurn) const
{ // in sec
  if( difDir==0 ) return 0;
  // convert argument into -8..7 range (old direction representation)
  float aDir=fabs(difDir)*(float(15)/255);
  float cost=aDir*0.25f+aDir*aDir*0.04f;
  if( difDir<0 ) return cost*0.8f;
  return cost;
}

FieldCost TankType::GetTypeCost(OperItemType type, CombatMode mode, Clearance clearance) const
{
  #define COST_BIG 50
  static const FieldCost costsSafe[] =
  {
    FieldCost(1.0),    // OITNormal,
    FieldCost(1.0),    // OITAvoidBush,
    FieldCost(2.0),    // OITAvoidTree,
    FieldCost(3.0),    // OITAvoid,
    FieldCost(SET_UNACCESSIBLE),  // OITWater,
    FieldCost(SET_UNACCESSIBLE),  // OITSpaceRoad,
    FieldCost(0.5),               // OITRoad
    FieldCost(3.0,1.0),               // OITSpaceBush,
    FieldCost(10.0,2.0),              // OITSpaceHardBush,
    FieldCost(10.0,3.0),              // OITSpaceTree,
    FieldCost(SET_UNACCESSIBLE),  // OITSpace,
    FieldCost(1.0)                // OITRoadForced,
  };
  static const FieldCost costsAware[] =
  {
    FieldCost(1.0),               // OITNormal,
    FieldCost(1.0),               // OITAvoidBush,
    FieldCost(1.2),               // OITAvoidTree,
    FieldCost(3.0),               // OITAvoid,
    FieldCost(SET_UNACCESSIBLE),  // OITWater,
    FieldCost(SET_UNACCESSIBLE),  // OITSpaceRoad,
    FieldCost(0.75,0.5),              // OITRoad
    FieldCost(2.0,1.0),               // OITSpaceBush,
    FieldCost(6.0,2.0),               // OITSpaceHardBush,
    FieldCost(8.0,2.0),               // OITSpaceTree,
    FieldCost(SET_UNACCESSIBLE),  // OITSpace,
    FieldCost(1.0)                // OITRoadForced,
  };
  static const FieldCost costsCombat[] =
  {
    FieldCost(1.0),               // OITNormal,
    FieldCost(1.0),               // OITAvoidBush,
    FieldCost(1.2),               // OITAvoidTree,
    FieldCost(3.0),               // OITAvoid,
    FieldCost(SET_UNACCESSIBLE),  // OITWater,
    FieldCost(SET_UNACCESSIBLE),  // OITSpaceRoad,
    FieldCost(1.0,0.5),               // OITRoad
    FieldCost(1.0),               // OITSpaceBush,
    FieldCost(1.5),               // OITSpaceHardBush,
    FieldCost(1.5),               // OITSpaceTree,
    FieldCost(SET_UNACCESSIBLE),  // OITSpace,
    FieldCost(1.0)                // OITRoadForced,
  };
  static const FieldCost costsStealth[] =
  {
    FieldCost(1.0),               // OITNormal,
    FieldCost(1.0),               // OITAvoidBush,
    FieldCost(2.0),               // OITAvoidTree,
    FieldCost(3.0),               // OITAvoid,
    FieldCost(SET_UNACCESSIBLE),  // OITWater,
    FieldCost(SET_UNACCESSIBLE),  // OITSpaceRoad,
    FieldCost(1.2,0.5),               // OITRoad
    FieldCost(3.0,1.0),               // OITSpaceBush,
    FieldCost(15.0,2.0),  // OITSpaceHardBush,
    FieldCost(30.0,2.0),  // OITSpaceTree,
    FieldCost(SET_UNACCESSIBLE),  // OITSpace,
    FieldCost(1.0)                // OITRoadForced,
  };
  Assert(sizeof(costsSafe)/sizeof(*costsSafe)==NOperItemType);
  Assert(sizeof(costsAware)/sizeof(*costsAware)==NOperItemType);
  Assert(sizeof(costsCombat)/sizeof(*costsCombat)==NOperItemType);
  Assert(sizeof(costsStealth)/sizeof(*costsStealth)==NOperItemType);
  static const FieldCost *costs[] = {costsSafe, costsSafe, costsAware, costsCombat, costsStealth};

  if (type==OITWater && _canFloat)
    return FieldCost(2,1);
  return costs[mode - CMCareless][type];
}

#define TANK_MSG_LIST(XX) \
  XX(UpdateGeneric, UpdateTank) \
  XX(UpdatePosition, UpdatePositionTank)

DEFINE_NETWORK_OBJECT(TankWithAI, base, TANK_MSG_LIST)

#define UPDATE_TANK_MSG(MessageName, XX)

DECLARE_NET_INDICES_EX_ERR(UpdateTank, UpdateTankOrCar, UPDATE_TANK_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdateTank, UpdateTankOrCar, UPDATE_TANK_MSG, NoErrorInitialFunc)

#define UPDATE_POSITION_TANK_MSG(MessageName, XX) \
  XX(MessageName, float, rpmWanted, NDTFloat, float, NCTFloat0To2, DEFVALUE(float, 0), DOC_MSG("Wanted value of RPM"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, thrustLWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted left thrust"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, thrustRWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted right thrust"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR)

DECLARE_NET_INDICES_EX_ERR(UpdatePositionTank, UpdatePositionVehicle, UPDATE_POSITION_TANK_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdatePositionTank, UpdatePositionVehicle, UPDATE_POSITION_TANK_MSG, NoErrorInitialFunc)

NetworkMessageFormat &TankWithAI::CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format)
{
  switch (cls)
  {
  case NMCUpdateGeneric:
    base::CreateFormat(cls, format);
    UPDATE_TANK_MSG(UpdateTank, MSG_FORMAT_ERR)
    break;
  case NMCUpdatePosition:
    base::CreateFormat(cls, format);
    UPDATE_POSITION_TANK_MSG(UpdatePositionTank, MSG_FORMAT_ERR)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError TankWithAI::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    TMCHECK(base::TransferMsg(ctx))
    {
      //PREPARE_TRANSFER(UpdateTank)
    }
    break;
  case NMCUpdatePosition:
    {
      PREPARE_TRANSFER(UpdatePositionTank)

      Matrix3 oldTrans = FutureVisualState().Orientation();
      TMCHECK(base::TransferMsg(ctx))
      // when position is received, we need to stabilize 
      StabilizeTurrets(oldTrans,FutureVisualState().Orientation());

      TRANSF(rpmWanted)
      TRANSF(thrustLWanted)
      TRANSF(thrustRWanted)
      if (!ctx.IsSending() && _rpmWanted>0.5f && fabs(_thrustLWanted)>=0.2f && fabs(_thrustRWanted)>=0.2f && fabs(FutureVisualState().ModelSpeed().Z())>2.0f)
      {
        // once moving fast and still thrusting, we can be sure the car is no longer braking
        // this is done to reduce error caused by remote simulation using out of sync parameters
        _pilotBrake = false;
      }
    }
    break;
  default:
    TMCHECK(base::TransferMsg(ctx))
    break;
  }
  return TMOK;
}

float TankWithAI::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = 0;
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    error +=  base::CalculateError(ctx);
    {
      //PREPARE_TRANSFER(UpdateTank)
    
    }
    break;
  case NMCUpdatePosition:
    {
      error +=  base::CalculateError(ctx);

      PREPARE_TRANSFER(UpdatePositionTank)

      ICALCERR_ABSDIF(float, rpmWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, thrustLWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, thrustRWanted, ERR_COEF_VALUE_MAJOR)
    }
    break;
  default:
    error +=  base::CalculateError(ctx);
    break;
  }
  return error;
}

void Tank::CamIntExtSwitched()
{
  Person *player=GWorld->PlayerOn();
  AIBrain *unit=( player ? player->CommanderUnit() : NULL );
  if( !unit || unit->GetVehicle()!=this ) return;
  player->ResetXRotWanted();
}
