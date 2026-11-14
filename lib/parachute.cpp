// Poseidon - vehicle class
// (C) 1997, Suma
#include "wpch.hpp"

#include "parachute.hpp"
#include "shots.hpp"
#include "scene.hpp"
#include "world.hpp"
#include "landscape.hpp"
#include "keyInput.hpp"
#include "lights.hpp"
#include "AI/ai.hpp"
#include "diagModes.hpp"
#include "stringtableExt.hpp"

#include <El/Common/randomGen.hpp>
#include "Shape/specLods.hpp"

#include "Network/network.hpp"

#if _RELEASE
  #define ARROWS 0
#else
  #define ARROWS 1
#endif

Parachute::Parachute( const EntityAIType *name, Person *pilot )
:base(name, pilot, true),
/*
_gunYRot(0),_gunYRotWanted(0),
_gunXRot(0),_gunXRotWanted(0),
_gunXSpeed(0),_gunYSpeed(0),
*/
_backRotor(0),_backRotorWanted(0),
_turbulence(VZero),
_lastTurbulenceTime(Glob.time)
{
  SetSimulationPrecision(1.0/15);
  RandomizeSimulationTime();
  SetTimeOffset(1.0/15);
}


ParachuteVisualState::ParachuteVisualState(ParachuteType const& type)
:base(type),
_openState(0)
{
}


void ParachuteVisualState::Interpolate(Object *obj, ObjectVisualState const& t1state, float t, ObjectVisualState& interpolatedResult) const
{
  base::Interpolate(obj, t1state, t, interpolatedResult);

  ParachuteVisualState const& s = static_cast<ParachuteVisualState const&>(t1state);
  ParachuteVisualState& res = static_cast<ParachuteVisualState&>(interpolatedResult);

  res._openState = _openState + t * (s._openState - _openState);  // 0..3
}


Parachute::~Parachute()
{
}

ParachuteType::ParachuteType( ParamEntryPar param )
:base(param)
{

  _pilotPos=VZero;
}

void ParachuteType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par,shape);
  SetHardwareAnimation(par);
}

void ParachuteType::InitShape()
{
  const ParamEntry &par=*_par;

  base::InitShape();

  RString namePilotPos = par >> "memoryPointPilot";

  int level;
  level=_shape->FindLevel(VIEW_PILOT);
  if( level>=0 )
  {
    _shape->Level(level)->MakeCockpit();
    _pilotPos=_shape->NamedPoint(level,namePilotPos);
  }

  if( _pilotPos.SquareSize()<0.1 ) _pilotPos=_shape->MemoryPoint(namePilotPos);

  if (_shape->LoadSkeletonFromSource())
  {
    AnimationRTName name;
    name.skeleton = _shape->GetSkeleton();
    name.name = par >> "animationOpen";
    name.reversed = true;
    name.streamingEnabled = false;

    _open = new AnimationRT(name);
    if (_open->GetBonesAffected()<=0)
    {
      RptF("Bad animation %s used for parachute drop",cc_cast(name.name));
      _open.Free();
    }
    else
    {
      _open->SetLooped(false);
      _open->AddPreloadCount();
    }
    
    name.name = par >> "animationDrop";
    _drop = new AnimationRT(name);
    if (_drop->GetBonesAffected()<=0)
    {
      RptF("Bad animation %s used for parachute drop",cc_cast(name.name));
      _drop.Free();
    }
    else
    {
      _drop->SetLooped(false);
      _drop->AddPreloadCount();
    }
  }
}

void ParachuteType::DeinitShape()
{
  if (_open) _open->ReleasePreloadCount();
  if (_drop) _drop->ReleasePreloadCount();
  _open.Free();
  _drop.Free();
  base::DeinitShape();
}

void ParachuteType::InitShapeLevel(int level)
{
  base::InitShapeLevel(level);
}
void ParachuteType::DeinitShapeLevel(int level)
{
  base::DeinitShapeLevel(level);
}


Object* ParachuteType::CreateObject(bool unused) const
{ 
  return new ParachuteAuto(this, NULL);
}


static Vector3 BodyFriction( Vector3Val speed, float open )
{
  // something between quadratic and linear
  float openFactor = open*open*0.75+open*0.25;
  Vector3 friction;
  friction.Init();
  float linCoef = openFactor*0.95+0.05;
  float quadCoef = openFactor*0.95+0.05;
  friction[0]=speed[0]*fabs(speed[0])*10*quadCoef+speed[0]*30*linCoef;
  friction[1]=speed[1]*fabs(speed[1])*30*quadCoef+speed[1]*100*linCoef;
  friction[2]=speed[2]*fabs(speed[2])*10*quadCoef+speed[2]*30*linCoef;
  return friction;
}


#define FAST_COEF (1.0/25) // use fast/slow simulation mode

/*
static const Color HeliLightColor(0.8,0.8,1.0);
static const Color HeliLightAmbient(0.07,0.07,0.1);
*/

void Parachute::GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit)
{
  // no actions while in parachute
}

#if _HELISIM_PRECISE_INTERACTION
void Parachute::GetDrawActions3D(UIActions &actions, AIBrain *unit)
{
  base::GetDrawActions3D(actions, unit);
}
#endif

void Parachute::Simulate( float deltaT, SimulationImportance prec )
{
  _isDead = IsDamageDestroyed();
  _backRotor = _backRotorWanted = 0;

  //Vector3Val position=Position();

  // calculate all forces, frictions and torques
  Vector3Val speed=FutureVisualState().ModelSpeed();
  Vector3 force(VZero),friction(VZero);
  Vector3 torque(VZero),torqueFriction(VZero);
  
  // world space center of mass
  Vector3 wCenter(VFastTransform,FutureVisualState().ModelToWorld(),GetCenterOfMass());

  // we consider following forces
  // gravity
  // back rotor (in direction of rotor axis - controled together with cyclic)
  // body aerodynamics (air friction)
  // main rotor aerodynamics (air friction and "up" force)
  // body and rotor torque friction (air friction)

  Vector3 pForce(VZero); // partial force
  Vector3 pCenter(VZero); // partial force application point

  if( !_isStopped )
  {
    /*
    // apply aerodynamics of the main rotor
    float backRadius=_shape->BoundingSphere()*0.95;
    // when moving fast, side bank causes torque
    float sizeFactor=backRadius*(1.0/12);
    pForce=Vector3(_backRotor*12500,0,0)*(GetMass()*(1.0/3000)*sizeFactor);
    pCenter=Vector3(0,0,-backRadius);
    torque+=pCenter.CrossProduct(pForce);
    #if ARROWS
      AddForce
      (
        DirectionModelToWorld(pCenter)+wCenter,
        DirectionModelToWorld(pForce)*InvMass()
      );
    #endif
    */
    
    // convert forces to world coordinates

    FutureVisualState().DirectionModelToWorld(torque,torque);
    FutureVisualState().DirectionModelToWorld(force,force);
    
    // angular velocity causes also some angular friction, this should be simulated as torque
    torqueFriction = _angMomentum * (_landContact ? 8.0f : 4.0f);

    // calculate new position
    VisualState moveTrans = PredictPos<VisualState>(deltaT);

    ApplyRemoteState(deltaT,moveTrans);

    // model space turbulence calculation
    if( Glob.time>_lastTurbulenceTime+2 )
    {
      _lastTurbulenceTime=Glob.time;
      const float maxXT=3;
      const float maxYT=1;
      const float maxZT=3;
      float tx=(GRandGen.RandomValue()-0.5)*(maxXT*2);
      float ty=(GRandGen.RandomValue()-0.5)*(maxYT*2);
      float tz=(GRandGen.RandomValue()-0.5)*(maxZT*2);
      _turbulence=Vector3(tx,ty,tz);
    }
    
    // body air friction
    float open = FutureVisualState()._openState-1;
    saturate(open,0,1);

    Vector3 wind = GLandscape->GetWindSpeed()+_turbulence;
    Vector3 airSpeed = speed-DirectionWorldToModel(wind);

    friction = BodyFriction(airSpeed,open);
    pCenter = Vector3(0,5,0);
    pForce = friction;
    torque -= pCenter.CrossProduct(pForce);

    #if ARROWS
      AddForce
      (
        FutureVisualState().DirectionModelToWorld(pCenter)+wCenter,
        FutureVisualState().DirectionModelToWorld(pForce)*InvMass()
        -friction*InvMass(),PackedColor(Color(0.5,0,0))
      );
    #endif

    FutureVisualState().DirectionModelToWorld(friction,friction);

    
    // gravity - no torque
    pForce=Vector3(0,-1,0)*(GetMass()*G_CONST);
    force+=pForce;
    #if ARROWS
      AddForce(wCenter,pForce*InvMass());
    #endif

    _objectContact=false;
    _landContact=false;
    _waterContact=false;
    
    // recalculate COM to reflect change of position
    wCenter.SetFastTransform(moveTrans.ModelToWorld(),GetCenterOfMass());
    if( deltaT>0 )
    {
      Vector3 totForce(VZero);

      // check collision on new position

      float crashSpeed = 0;

      if( prec<=SimulateVisibleFar && IsLocal())
      {
        CollisionBuffer collision;
        GLOB_LAND->ObjectCollision(collision,this,moveTrans);
        #define MAX_IN 0.4
        #define MAX_IN_FORCE 0.1
        #define MAX_IN_FRICTION 0.4

        for( int i=0; i<collision.Size(); i++ )
        {
          _objectContact=true;
          // info.pos is relative to object
          CollisionInfo &info=collision[i];
          if( info.object )
          {
            info.object->SimulationNeeded(GetVisualStateAge(FutureVisualState()));
            float cFactor=1;
            if( info.object->GetMass()<50 || info.object->IsPassable()) continue;
            if( info.object->GetType()==Primary )
            {
              cFactor=1;
            }
            else
            {
              cFactor=info.object->GetMass()*GetInvMass();
              saturate(cFactor,0,5);
            }
            if( cFactor>0.05 )
            {
              /*
              LogF
              (
                "%s: heli colision %.3f",
                (const char *)info.object->GetDebugName(),
                cFactor
              );
              */
              Vector3Val pos=info.pos;
              Vector3Val dirOut=info.dirOut;
              // create a force pushing "out" of the collision
              float forceIn=floatMin(info.under,MAX_IN_FORCE);
              Vector3 pForce=dirOut*GetMass()*40*forceIn;
              // apply proportional part of force in place of impact
              pCenter=pos-wCenter;
              totForce+=pForce;
              torque+=pCenter.CrossProduct(pForce);
              
              Vector3Val objSpeed=info.object->ObjectSpeed();
              Vector3 colSpeed = FutureVisualState()._speed-objSpeed;
              saturateMax(crashSpeed,colSpeed.Size());

              // if info.under is bigger than MAX_IN, move out
              if( info.under>MAX_IN )
              {
                Point3 newPos=moveTrans.Position();
                float moveOut=info.under-MAX_IN;
                newPos+=dirOut*moveOut*0.1;
                moveTrans.SetPosition(newPos);
                // limit speed
              }

              // second is "land friction" - causing no momentum
              float frictionIn=floatMin(info.under,MAX_IN_FRICTION);
              pForce[0]=fSign(speed[0])*20000;
              pForce[1]=speed[1]*fabs(speed[1])*1000+speed[1]*8000+fSign(speed[1])*10000;
              pForce[2]=speed[2]*fabs(speed[2])*150+speed[2]*250+fSign(speed[2])*2000;

              pForce=FutureVisualState().DirectionModelToWorld(pForce)*GetMass()*(4.0/10000)*frictionIn;
              //saturateMin(pForce[1],0);
              //torque-=pCenter.CrossProduct(pForce);
              #if ARROWS
                AddForce(wCenter+pCenter,-pForce*InvMass());
              #endif
              friction+=pForce;
              torqueFriction+=_angMomentum*0.15;
            }
          }
        }
      } // if( object collisions enabled )
      
      GroundCollisionBuffer gCollision;
      GLOB_LAND->GroundCollision(gCollision,this,moveTrans,0.05,0.3,false);

      if( gCollision.Size()>0 )
      {

        Vector3 gFriction(VZero);
        float maxUnder=0;
        #define MAX_UNDER 0.2
        #define MAX_UNDER_FORCE 0.1
      
        for( int i=0; i<gCollision.Size(); i++ )
        {
          // info.pos is world space
          UndergroundInfo &info=gCollision[i];
          if( info.under<0 ) continue;
          // we consider two forces
          //ReportFloat("land",info.under);
          float under;
          if( info.type==GroundWater )
          {
            under=info.under*0.001;
            _waterContact=true;
          }
          else
          {
            _landContact=true;
            // we consider two forces
            //ReportFloat("land",info.under);
            if( maxUnder<info.under ) maxUnder=info.under;
            under=floatMin(info.under,MAX_UNDER_FORCE);
          }
          // one is ground "pushing" everything out - causing some momentum
          Vector3 dirOut=Vector3(0,info.dZ,1).CrossProduct(Vector3(1,info.dX,0)).Normalized();
          pForce=dirOut*GetMass()*40.0*under;
          pCenter=info.pos-wCenter;
          torque+=pCenter.CrossProduct(pForce);
          // to do: analyze ground reaction force
          totForce+=pForce;

          #if ARROWS
            AddForce(wCenter+pCenter,pForce*under*InvMass());
          #endif
          
          // second is "land friction" - causing momentum
          pForce[0]=speed[0]*5000+fSign(speed[0])*10000;
          pForce[1]=speed[1]*fabs(speed[1])*1000+speed[1]*8000+fSign(speed[1])*10000;
          pForce[2]=speed[2]*fabs(speed[2])*150+speed[2]*250+fSign(speed[2])*5000;
          
          pForce=FutureVisualState().DirectionModelToWorld(pForce)*GetMass()*(1.0/10000);
          #if ARROWS
            AddForce(wCenter+pCenter,-pForce*InvMass());
          #endif
          friction+=pForce;

          // torque applied if speed is big enough
          if( fabs(speed[0])<1 ) pForce[0]=0;
          if( fabs(speed[1])<1 ) pForce[1]=0;
          if( fabs(speed[2])<1 ) pForce[2]=0;
          torque-=pCenter.CrossProduct(pForce); // sub: it is friction

          torqueFriction+=_angMomentum*info.under*3;
        }
        //torqueFriction=_angMomentum*1.0;
        if (maxUnder > 0)
          saturateMax(crashSpeed, fabs(FutureVisualState()._speed[1])+FutureVisualState()._speed.SizeXZ()*0.3);

        if( maxUnder>MAX_UNDER )
        {
          // it is necessary to move object immediately
          Point3 newPos=moveTrans.Position();
          float moveUp=maxUnder-MAX_UNDER;
          newPos[1]+=moveUp;
          moveTrans.SetPosition(newPos);
          
          if (FutureVisualState()._speed.SquareSize()>1)
          {
            FutureVisualState()._speed.Normalize();
          }

        }
      }

      if (crashSpeed>8.5)
      {
        float crash = (crashSpeed-8.5)*0.6;
        if( Glob.time>_disableDamageUntil )
        {
          // crash boom bang state - impact speed too high
          _doCrash=CrashLand;
          if( _objectContact ) _doCrash=CrashObject;
          if( _waterContact ) _doCrash=CrashWater;
          _crashVolume = crash;

          CrashDammage(crash*10);

          LogF("para crash %.3f, speed %.1f",crash,crashSpeed);

          _disableDamageUntil = Glob.time+1;
        }
        //GLOB_ENGINE->ShowMessage(1000,"%s crash %d, %f",_name,_doCrash,_crashVolume);
      }

      force+=totForce;
    }

    bool stopCondition=false;
    if( _landContact && !_waterContact && !_objectContact )
    {
      // apply static friction
      float maxSpeed=Square(0.7);
      if( !Driver() ) maxSpeed=Square(1.2);
      if (FutureVisualState()._speed.SquareSize()<maxSpeed && FutureVisualState().DirectionUp().Y()>0.95f)
      {
        stopCondition=true;
      }
      // no more torque applied
      torque = VZero;
    }
    if( stopCondition) StopDetected();
    else OnMoved();

    // apply all forces
    ApplyForces(deltaT,force,torque,friction,torqueFriction);

    Move(moveTrans);
    DirectionWorldToModel(FutureVisualState()._modelSpeed, FutureVisualState()._speed);
  }

  base::Simulate(deltaT,prec);
  SimulatePost(deltaT,prec);

  // check if we have pilot

  /*
  GlobalShowMessage
  (
    100,"speed %.1f,%.1f,%.1f, open %.2f",
    _speed.X()*3.6,_speed.Y()*3.6,_speed.Z()*3.6,FutureVisualState()._openState
  );
  */

  if (_landContact || _waterContact)
  {
    if (!Type()->_drop)
    {
      // no drop animation - open immediately
      FutureVisualState()._openState = 3;
    }
    else
    {
      FutureVisualState()._openState += deltaT*0.5;
    }
    if (FutureVisualState()._openState>=3)
    {
      FutureVisualState()._openState = 3;
      // we can only eject the driver or destroy the parachute if we are local - otherwise we are only a remote "ghost"
      if (IsLocal())
      {
        if (DriverBrain() && DriverBrain()->LSIsAlive())
        {
          DriverBrain()->ProcessGetOut(false,false);
        }
        else if (_driver)
        {
          Ref<Person> soldier = _driver;

          GetInPoint pt = AnimateGetInPoint(Type()->GetDriverGetInPos(0));

          Matrix4 transform;
          FutureVisualState().PositionModelToWorld(pt._pos, pt._pos);
          FutureVisualState().DirectionModelToWorld(pt._dir, pt._dir);
          transform.SetPosition(pt._pos);
          transform.SetUpAndDirection(VUp, -pt._dir);

          GetOutDriver(transform, false, false);
          // switch camera if dead player gets out
          if (GWorld->CameraOn() == this && GWorld->PlayerOn() == soldier)
          {
            CameraType ValidateCamera(CameraType cam);
            GWorld->SwitchCameraTo(soldier, ValidateCamera(GWorld->GetCameraType()), false);
          }
        }
        else
        {
          // once the driver is out, we can delete ourselves
          SetDelete();
        }
      }
    }
  }
  else
  {
    if (FutureVisualState()._openState<1)
    {
      // wait a while before opening
      FutureVisualState()._openState += deltaT*0.7;
    }
    else
    {
      FutureVisualState()._openState += deltaT*0.3;
    }
    // do not close until touched ground
    if (FutureVisualState()._openState>=2) FutureVisualState()._openState = 2;
  }

}

const float FlyLevel=150;

float ParachuteAuto::MakeAirborne(bool playerIn)
{
  _landContact=false;
  _objectContact=false;
  FutureVisualState()._openState=2;
  FutureVisualState()._speed = Vector3(0,-7,0);
  return FlyLevel;
}

bool Parachute::IsPossibleToGetIn() const
{
  return !_landContact;
}

bool Parachute::IsAbleToMove() const
{
  return !_landContact;
}

bool Parachute::Airborne() const
{
  return !_landContact;
}

float Parachute::GetEngineVol( float &freq ) const
{
  freq=1;
  return 0;
}

float Parachute::GetEnvironVol( float &freq ) const
{
  freq=1;
  return FutureVisualState()._speed.Size()/Type()->GetMaxSpeedMs();
}


void Parachute::Sound( bool inside, float deltaT )
{
  base::Sound(inside,deltaT);

}

void Parachute::UnloadSound()
{
  base::UnloadSound();
}

bool Parachute::HasFlares( CameraType camType ) const
{
  return camType!=CamInternal && camType!=CamGunner;
}

Matrix4 Parachute::InsideCamera( CameraType camType ) const
{
  Matrix4 transf;
  if (GetProxyCamera(transf, camType))
    return transf;  
  return MIdentity;
}

int Parachute::InsideLOD( CameraType camType ) const
{
  int level=-1;
  //if( camType==CamCargo ) level=GetShape()->FindLevel(VIEW_CARGO);
  if( level<0 ) level = GetShape()->FindSpecLevel(VIEW_PILOT);
  if (level<0) level = 0;
  return level;
}

void Parachute::InitVirtual( CameraType camType, float &heading, float &dive, float &fov ) const
{
  base::InitVirtual(camType,heading,dive,fov);
/*
  switch( camType )
  {
    case CamGunner:
      fov=0.50;
    break;
    case CamInternal:
      dive=0;
    break;
  }
*/
}

void Parachute::LimitVirtual( CameraType camType, float &heading, float &dive, float &fov ) const
{
  base::LimitVirtual(camType,heading,dive,fov);
  switch( camType )
  {
/*
    case CamGunner:
      saturate(fov,0.3,1.2);
      saturate(heading,-1.8,+1.8);
      saturate(dive,-0.7,+0.3);
    break;
*/
    case CamInternal:
      saturate(fov,0.3,1.2);
      saturate(heading,-1.8,+1.8);
      saturate(dive,-0.7,+0.3);
    break;
  }
}


AnimationStyle Parachute::IsAnimated( int level ) const {return AnimatedGeometry;}

bool Parachute::PrepareShapeDrawMatrices(Matrix4Array &matrices, const ObjectVisualState &vs, LODShape *shape, int level)
{
  // Get the shape and make sure it already exists
  DoAssert(_shape->IsLevelLocked(level));
  const Shape *shapeL = _shape->GetLevelLocked(level);

  // no PrepareShapeDrawMatrices here - we want the array to stay empty for PrepareMatrices
  if (vs.cast<Parachute>()._openState<=2)
  {
    if (Type()->_open)
    {
      float time = vs.cast<Parachute>()._openState-1;
      saturate(time,0,1);
      Type()->_open->PrepareMatrices(matrices, time, 1, shapeL,shape->GetSkeleton());
      return true;
    }
  }
  else
  {
    if (Type()->_drop)
    {
      float time = vs.cast<Parachute>()._openState-2;
      saturate(time,0,1);
      Type()->_drop->PrepareMatrices(matrices, time, 1, shapeL,shape->GetSkeleton());
      return true;
    }
  }
  // TODO: consider combining controller + rtm animations
  base::PrepareShapeDrawMatrices(matrices,vs,shape,level);
  return true;
}

void Parachute::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  if (!setEngineStuff)
  {
    // animate with skeletal animation
    if (animContext.GetVisualState().cast<Parachute>()._openState<=2)
    {
      if (Type()->_open)
      {
        float time = animContext.GetVisualState().cast<Parachute>()._openState-1;
        saturate(time,0,1);
        Type()->_open->Apply(animContext, _shape, level, time);
      }
    }
    else
    {
      if (Type()->_drop)
      {
        float time = animContext.GetVisualState().cast<Parachute>()._openState-2;
        saturate(time,0,1);
        Type()->_drop->Apply(animContext, _shape, level, time);
      }
    }
  }

  // check state

  base::Animate(animContext, level, setEngineStuff, parentObject, dist2);
//  SelectTexture(level,_rotorSpeed);
}

void Parachute::Deanimate( int level, bool setEngineStuff )
{
  base::Deanimate(level,setEngineStuff);
}


// basic autopilot

ParachuteAuto::ParachuteAuto( const EntityAIType *name, Person *pilot )
:Parachute(name,pilot),
_dirCompensate(0.5),
_lastAngVelocity(VZero),
_pilotHeading(0),
_pilotHelper(true) // keyboard helper activated
{
}

void ParachuteAuto::Simulate( float deltaT, SimulationImportance prec )
{
  // perform advanced simulation
  MoveWeapons(deltaT);
  base::Simulate(deltaT,prec);

  _lastAngVelocity=_angVelocity; // helper for prediction
}

bool ParachuteAuto::AimWeapon(const TurretContextEx &context, const WeaponModePtr &ptr, Vector3Par direction )
{
  return true;
}

bool ParachuteAuto::AimWeaponTgt(const TurretContextEx &context, const WeaponModePtr &ptr, Target *target)
{
  return true;
}

Matrix4 ParachuteAuto::GunTransform() const
{
  return MIdentity;
}

const float MissileConeDown=0.00;

Vector3 ParachuteAuto::GetWeaponDirection(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const
{
  return vs.Direction();
}

float ParachuteAuto::GetAimed(const TurretContext &context, const WeaponModePtr &ptr, Target *target, bool exact, bool checkLockDelay) const
{
  return 1;
}


// manual control

void ParachuteAuto::Eject(AIBrain *unit,Vector3Val diff)
{
  base::Eject(unit, diff);
}


void ParachuteAuto::FakePilot( float deltaT )
{
}

void ParachuteAuto::SuspendedPilot(AIBrain *unit, float deltaT )
{
}

void ParachuteAuto::KeyboardPilot(AIBrain *unit, float deltaT )
{ 
  if (!unit->GetPerson() || unit->GetPerson()->KeyboardUsedInFrame()) return;
  _dirCompensate=0; // low heading compensation
  _pilotHelper=true; // keyboard helper activated
  
  Vector3Val direction=FutureVisualState().Direction();
  _pilotHeading=atan2(direction[0],direction[2]);
    
  _pilotHeading+=0.5*(GInput.GetAction(UATurnRight)-GInput.GetAction(UATurnLeft));

  // aim and turn with aiming actions
  float aimX = GInput.GetActionWithCurve(InputInterface::Curve1Mouse,UAAimRight, UAAimLeft);
  float aimY = GInput.GetActionWithCurve(InputInterface::Curve1Mouse,UAAimUp, UAAimDown);
  LookAround(aimX, aimY, deltaT, &Type()->_viewPilot, unit, true);
}

void ParachuteAuto::AvoidGround( float minHeight )
{
}

// AI autopilot


bool ParachuteAuto::FireWeapon(const TurretContextV &context, const WeaponModePtr &ptr, TargetType *target, bool forceLock)
{
  return false;
}

bool ParachuteAuto::FireWeaponEffects(
  const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine,EntityAI *target, const RemoteFireWeaponInfo *remoteInfo
)
{
  return base::FireWeaponEffects(context, ptr, magazine,target,remoteInfo);
}

// AI interface

float ParachuteType::GetFieldCost( const GeographyInfo &info, CombatMode mode ) const
{
  return 1;
}

float ParachuteType::GetBaseCost(const GeographyInfo &geogr, bool operative, bool includeGradient) const
{
  float cost=GetMinCost(); // basic speed
  // avoid any water
  // penalty for objects
  //cost *= 1 + geogr.howManyObjects*0.1;
  if (includeGradient)
  {
    // avoid steep hills
    // penalty for hills
    int grad = geogr.u.gradient;
    Assert( grad<=7 );
    static const float gradPenalty[8]={1.0,1.02,1.05,1.2,1.3,1.5,1.7,2.0};
    cost *= gradPenalty[grad];
  }
  return cost;
}

float ParachuteType::GetGradientPenalty(float gradient) const
{
  // TODO: continuous function, sign of gradient
  gradient = fabs(gradient);

  if (gradient >= 0.95f) return 2.0f; // level 7
  else if (gradient >= 0.60f) return 1.7f; // level 6
  else if (gradient >= 0.40f) return 1.5f; // level 5
  else if (gradient >= 0.25f) return 1.3f; // level 4
  else if (gradient >= 0.15f) return 1.2f; // level 3
  else if (gradient >= 0.10f) return 1.05f; // level 2
  else if (gradient >= 0.05f) return 1.02f; // level 1
  return 1.0f; // level 0
}

float ParachuteType::GetCostTurn(int difDir, int &debet, bool forceFullTurn) const
{ // in sec
  if( difDir==0 ) return 0;
  float aDir=fabs(difDir);
  float cost=aDir*0.15+aDir*aDir*0.02;
  if( difDir<0 ) return cost*0.8;
  return cost;
}

#if _HELISIM_IMPERIAL_SYSTEM
float ParachuteType::ConvertDistance(float meters, int &stringIDS) const 
{
  if(Glob.config.useImperialSystem > 0)
  {
    if(meters >= 1853) 
    {
      stringIDS = IDS_UI_POSITION_DISTANCE_NAUTICAL_MILES;
      return METER_TO_NAUTICAL_MILE(meters);
    }
    else
    {
      stringIDS = IDS_UI_POSITION_DISTANCE_FEET;
      return METER_TO_FEET(meters);
    }
  }
  else
  {
    if(meters > 1000) 
    {
      stringIDS = IDS_UI_POSITION_DISTANCE_KILOMETERS;
      return METER_TO_KILOMETER(meters);
    }
    else
    {
      stringIDS = IDS_UI_POSITION_DISTANCE;
      return meters;
    }
  }
};


float ParachuteType::ConvertSpeed(float metersPerSecond) const 
{
  return Glob.config.useImperialSystem > 0? METER_TO_KNOT(metersPerSecond) : METER_TO_KM_PER_HOUR(metersPerSecond);
};

float ParachuteType::ConvertHeight(float meters) const 
{
  return Glob.config.useImperialSystem > 0? METER_TO_FEET(meters) : meters;
};
#endif

float ParachuteAuto::FireInRange(const TurretContext &context, const WeaponModePtr &ptr, float &timeToAim, const Target &target) const
{
  timeToAim=0;
  //return GetAimed(weapon,target.idExact);
  return 1;
}

float ParachuteAuto::FireAngleInRange(const TurretContext &context, const WeaponModePtr &ptr, Vector3Par rel) const
{
  // helicopter cannot fire high, can fire slight low
  if( rel.Y()>0 ) return 0;
  float size2=rel.SquareSizeXZ();
  float y2=Square(rel.Y());
  const float maxY=0.25;
  if( y2>size2*Square(maxY) ) return 0;
  // nearly same level
  float invSize=InvSqrt(size2);
  return 1-rel.Y()*invSize*(1/maxY);
}

#if _ENABLE_AI

void ParachuteAuto::AIGunner(TurretContextEx &context, float deltaT )
{
}

#endif //_ENABLE_AI

void ParachuteAuto::MoveWeapons( float deltaT )
{
/*
  float delta;
  float speed;
  speed=(_gunXRotWanted-_gunXRot)*8;
  const float maxA=10;
  const float maxV=5;
  delta=speed-_gunXSpeed;
  Limit(delta,-maxA*deltaT,+maxA*deltaT);
  _gunXSpeed+=delta;
  Limit(_gunXSpeed,-maxV,+maxV);
  _gunXRot+=_gunXSpeed*deltaT;
  Limit(_gunXRot,Type()->_minGunElev,Type()->_maxGunElev);

  speed=AngleDifference(_gunYRotWanted,_gunYRot)*6;
  delta=speed-_gunYSpeed;
  Limit(delta,-maxA*deltaT,+maxA*deltaT);
  _gunYSpeed+=delta;
  Limit(_gunYSpeed,-maxV,+maxV);
  _gunYRot+=_gunYSpeed*deltaT;
  _gunYRot=AngleDifference(_gunYRot,0);
  Limit(_gunYRot,Type()->_minGunTurn,Type()->_maxGunTurn);
*/
}

#if _ENABLE_AI
void ParachuteAuto::AIPilot(AIBrain *unit, float deltaT )
{
  Assert( unit );

  _dirCompensate=1;
  // if aiming for fire, we need quick reactions
  /*
  if( _fireMode>=0 )
  {
    if( !_firePrepareOnly ) _dirCompensate=0.1;
    else _dirCompensate=0.3;
  }
  */

  Vector3 steerPos=SteerPoint(2.0,4.0);

  Vector3 steerWant=PositionWorldToModel(steerPos);
  //Vector3Val speed=ModelSpeed();
  
  float headChange=atan2(steerWant.X(),steerWant.Z());
  float speedWanted=0;
  
  bool autopilot=false;
  if( unit->GetState()==AIBrain::Stopping || unit->GetState()==AIBrain::Stopped )
  {
    // special handling of stop state
    // landing position is in _stopPositon
    speedWanted=0;
    if( _landContact && unit->GetState()==AIBrain::Stopping )
    {
      unit->OnStepCompleted();
      // note: Pilot may get out - Brain may be NULL
      if( unit->IsFreeSoldier() ) return;
    }
  }
  else
  {
  }

  Assert(unit);

  if( !autopilot )
  {
    float avoidGround=0.5;
    float speedSize=fabs(FutureVisualState().ModelSpeed().Z());
    saturateMax(avoidGround,speedSize*0.35);    

    float maxTurn=H_PI;
    saturate(headChange,-maxTurn,+maxTurn);
    float curHeading=atan2(FutureVisualState().Direction()[0],FutureVisualState().Direction()[2]);
    _pilotHeading=curHeading+headChange;

  }

  #if 0
  if( this==GWorld->CameraOn() )
  {
    LogF
    (
      "InCombat %.2f wSpd %.1f pSpd %.1f spd %.1f",
      inCombat,speedWanted*3.6,_pilotSpeed[2]*3.6,ModelSpeed()[2]*3.6
    );
  }
  #endif

  #if 0
    if( this==GWorld->CameraOn() && _fireMode>=0 )
    {
      GEngine->ShowMessage
      (
        100,"Heli mode %d state %d, delay %.1f spd %.1f",
        _fireMode,_fireState,_fireStateDelay-Glob.time,speedWanted
      );
    }
  #endif

  // aim to current target
  // if( !_isDead ) AIFire(unit, deltaT);
}
#endif // _ENABLE_AI

void ParachuteAuto::DrawDiags()
{
  if (CHECK_DIAG(DECombat))
  {

    LODShape *forceArrow=GScene->ForceArrow();
    #define DRAW_OBJ(obj) GScene->DrawObject(obj)

    #if 1
      // draw pilot diags
      {
        Matrix3 rotY(MRotationY,-_pilotHeading);
        Vector3 pilotDir=rotY.Direction();
        Ref<ObjectColored> arrow=new ObjectColored(forceArrow,VISITOR_NO_ID);
        Point3 pos=FutureVisualState().Position()+Vector3(0,5,0);

        float size=0.6;
        arrow->SetPosition(pos);
        arrow->SetOrient(pilotDir,VUp);
        arrow->SetPosition
        (
          arrow->FutureVisualState().PositionModelToWorld(forceArrow->BoundingCenter()*size)
        );
        arrow->SetScale(size);
        arrow->SetConstantColor(PackedColor(Color(1,1,0)));

        DRAW_OBJ(arrow);
      }
    #endif

  }
  base::DrawDiags();
}

RString ParachuteAuto::DiagText() const
{
  return base::DiagText();
}

void Parachute::GetAimActions(float &aimX, float &aimY) const
{
  aimX = GInput.GetActionWithCurve(InputInterface::CurveFovMouse, UAAimRight, UAAimLeft);
  aimY = GInput.GetActionWithCurve(InputInterface::CurveFovMouse, UAAimUp, UAAimDown);
}


#define SERIAL_DEF_VISUALSTATE(name, value) CHECK(ar.Serialize(#name, FutureVisualState()._##name, 1, value))

LSError Parachute::Serialize(ParamArchive &ar)
{
  SERIAL_BASE;
  // TODO: serialize other members

  if (!IS_UNIT_STATUS_BRANCH(ar.GetArVersion()))
  {
    SERIAL_DEF_VISUALSTATE(openState, 0);
    SERIAL_DEF(backRotor,0);
    SERIAL_DEF(backRotorWanted,0);
  }

/*
  SERIAL_DEF(gunYRot,0);SERIAL_DEF(gunYRotWanted,0);
  SERIAL_DEF(gunXRot,0);SERIAL_DEF(gunXRotWanted,0);
  SERIAL_DEF(gunXSpeed,0);SERIAL_DEF(gunYSpeed,0);
*/

  return LSOK;
}

#undef SERIAL_DEF_VISUALSTATE


LSError ParachuteAuto::Serialize(ParamArchive &ar)
{
  SERIAL_BASE;
  // TODO: serialize other members

  return LSOK;
}

#define PARACHUTE_MSG_LIST(XX) \
  XX(UpdateGeneric, UpdateParachute)

DEFINE_NETWORK_OBJECT(ParachuteAuto, base, PARACHUTE_MSG_LIST)

#define UPDATE_PARACHUTE_MSG(MessageName, XX) \
  XX(MessageName, float, backRotorWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Obsolete"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR)

DECLARE_NET_INDICES_EX_ERR(UpdateParachute, UpdateTransport, UPDATE_PARACHUTE_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdateParachute, UpdateTransport, UPDATE_PARACHUTE_MSG, NoErrorInitialFunc)

NetworkMessageFormat &ParachuteAuto::CreateFormat
(
  NetworkMessageClass cls,
  NetworkMessageFormat &format
)
{
  switch (cls)
  {
  case NMCUpdateGeneric:
    base::CreateFormat(cls, format);
    UPDATE_PARACHUTE_MSG(UpdateParachute, MSG_FORMAT_ERR)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError ParachuteAuto::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    TMCHECK(base::TransferMsg(ctx))
    {
      PREPARE_TRANSFER(UpdateParachute)

//      ITRANSF(backRotor)
      TRANSF(backRotorWanted)
/*
      ITRANSF(gunYRot)
      ITRANSF(gunYRotWanted)
      ITRANSF(gunYSpeed)
      ITRANSF(gunXRot)
      ITRANSF(gunXRotWanted)
      ITRANSF(gunXSpeed)
*/
    }
    break;
  default:
    return base::TransferMsg(ctx);
  }
  return TMOK;
}

float ParachuteAuto::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = 0;
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    error += base::CalculateError(ctx);
    {
      PREPARE_TRANSFER(UpdateParachute)

//      ICALCERR_ABSDIF(float, backRotor, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, backRotorWanted, ERR_COEF_VALUE_MAJOR)
/*
      ICALCERR_ABSDIF(float, gunYRot, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, gunYRotWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, gunYSpeed, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, gunXRot, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, gunXRotWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, gunXSpeed, ERR_COEF_VALUE_MAJOR)
*/
    }
    break;
  default:
    error += base::CalculateError(ctx);
    break;
  }
  return error;
}

