/*!
\file
Motorcycle simulation.

\patch 1.01 Date 7/5/2001 by Ondra.
- Added: Motorcycle/bicycle simulation introduced.
*/
#include "wpch.hpp"
#include "keyInput.hpp"
#include "scene.hpp"
#include "world.hpp"
#include "landscape.hpp"
#include "AI/ai.hpp"
#include <El/Common/randomGen.hpp>
#include "diagModes.hpp"

#include "motorcycle.hpp"
#include "Network/network.hpp"
#include "Shape/specLods.hpp"

#include "UI/uiActions.hpp"
#include "stringtableExt.hpp"
#include "camera.hpp"
#include "mbcs.hpp"

static const Color MotorcycleLightColor(0.9,0.8,0.8);
static const Color MotorcycleLightAmbient(0.1,0.1,0.1);

#if !_RELEASE && _ENABLE_CHEATS
  #define ARROWS 1
#elif _ENABLE_CHEATS
  #define ARROWS 1
#endif

MotorcycleType::MotorcycleType( ParamEntryPar param )
:base(param)
{
  _glassRHit = -1;
  _glassLHit = -1;
  _bodyHit = -1;
  _fuelHit = -1;
  _wheelFHit = -1;
  _wheelBHit = -1;
}

void MotorcycleType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par,shape);

  _turnCoef=par>>"turnCoef";
  _terrainCoef=par>>"terrainCoef";
  _isBicycle=par>>"isBicycle";
  if (_isBicycle)
    _fuelCapacity = 0; // does not need fuel

  float damperSize=par>>"damperSize";
  float damperForce=par>>"damperForce";  
  float damperDamping = par>>"damperDamping";

  _dampers.Init(damperSize, damperForce, damperDamping);

  SetHardwareAnimation(par);
}

AnimationSource *MotorcycleType::CreateAnimationSource(RStringB source)
{
  if (!strcmpi(source,"support")) return _animSources.CreateAnimationSource(&Motorcycle::GetCtrlSupportPos);
  if (!strcmpi(source,"pedals")) return _animSources.CreateAnimationSource(&Motorcycle::GetCrtlPedalPos);
  if (!strcmpi(source,"wheel")) return _animSources.CreateAnimationSource(&Motorcycle::GetCtrlWheelRot);
  return NULL;
}

AnimationSource *MotorcycleType::CreateAnimationSourcePar(const AnimationType *type, ParamEntryPar entry)
{
  RStringB sourceType = entry>>"source";
  if (!strcmpi(sourceType,"damper"))
  {
    /// create a damper state
    return _dampers.CreateAnimationSource(entry);
  }
  return base::CreateAnimationSourcePar(type,entry);
}

AnimationSource *MotorcycleType::CreateAnimationSource(const AnimationType *type, RStringB source)
{
  AnimationSource *ret = CreateAnimationSource(source);
  if (ret)
  {
    return ret;
  }
  if (!strcmpi(source,"damper"))
  {
    /// create a damper state
    return _dampers.CreateAnimationSource();
  }
  return base::CreateAnimationSource(type,source);
}

void MotorcycleType::InitShape()
{
  // we need to clear dampers, so that they do not accumulate forever
  _dampers.Clear();
  
  const ParamEntry &par=*_par;

  base::InitShape();

  _plateInfos.Init(_shape, par >> "PlateInfos", GetFontID(Pars >> "fontPlate"));

  // camera positions

  _glassRHit = FindHitPoint("HitRGlass");
  _glassLHit = FindHitPoint("HitLGlass");
  _bodyHit = FindHitPoint("HitBody");
  _fuelHit = FindHitPoint("HitFuel");
  _wheelFHit = FindHitPoint("HitFWheel");
  _wheelBHit = FindHitPoint("HitBWheel");

  _hitZoneTextureUV.Add(14); _hitZoneTextureUV.Add(15);
  _hitZoneTextureUV.Add(16); _hitZoneTextureUV.Add(17);

  {
    Ref<WoundInfo> damageInfo = new WoundInfo;
    damageInfo->Load(_shape,par>>"dammageHalf");
    _glassDamageHalf.Init(_shape,damageInfo,NULL,NULL);
  }
  {
    Ref<WoundInfo> damageInfo = new WoundInfo;
    damageInfo->Load(_shape,par>>"dammageFull");
    _glassDamageFull.Init(_shape,damageInfo,NULL,NULL);
  }
}

void MotorcycleType::DeinitShape()
{
  _dampers.Clear();

  base::DeinitShape();
}

void MotorcycleType::InitShapeLevel(int level)
{
  base::InitShapeLevel(level);

  _plateInfos.InitLevel(_shape,level);

  _glassDamageHalf.InitLevel(_shape,level);
  _glassDamageFull.InitLevel(_shape,level);
  
  if (level==_shape->FindLandContactLevel())
  {
    _dampers.InitLandcontact(_shape,level,GetAnimations(),_animSources);
  }
}
void MotorcycleType::DeinitShapeLevel(int level)
{
  _plateInfos.DeinitLevel(_shape,level);
  
  _glassDamageHalf.DeinitLevel(_shape,level);
  _glassDamageFull.DeinitLevel(_shape,level);

  base::DeinitShapeLevel(level);
}


Object* MotorcycleType::CreateObject(bool unused) const
{ 
  return new Motorcycle(this, NULL,  unused);
}


Motorcycle::Motorcycle( const EntityAIType *name, Person *driver, bool fullCreate)
:base(name, driver, fullCreate),

// pilot controls
_thrustWanted(0),
_turnWanted(0),
_turnIncreaseSpeed(1),_turnDecreaseSpeed(1),
_turnSpeedWanted(0),
_turnSpeedWantedLastActive(TIME_MIN),
_reverseTimeLeft(0),
_forwardTimeLeft(0),

_waterDrown(false),

_track(_shape, *Type()->_par),

_allowEjectAfter(Time(0))
{
  SetSimulationPrecision(1.0/15,1.0/15);
  RandomizeSimulationTime();
  SetTimeOffset(1.0/15);

  FutureVisualState()._rpm=0,_rpmWanted=0;

  _gearBox.Load((*Type()->_par)>>"gearBox",Type()->GetMaxSpeed());
  // synchronize internal and external view

/*
  _rightDust.SetSize(0.35);
  _rightDust.SetAlpha(0.25);
*/
}


MotorcycleVisualState::MotorcycleVisualState(MotorcycleType const& type)
:base(type),
_support(0),
_wheelPhase(0), _wheelPhaseDelta(0), // gearbox
_thrust(0),
_turn(0),
_turnBySpeed(0)
{
  _dampers.Init(type._dampers);
}


void MotorcycleVisualState::Interpolate(Object *obj, ObjectVisualState const& t1state, float t, ObjectVisualState& interpolatedResult) const
{
  base::Interpolate(obj, t1state, t, interpolatedResult);

  MotorcycleVisualState const& s = static_cast<MotorcycleVisualState const&>(t1state);
  MotorcycleVisualState& res = static_cast<MotorcycleVisualState&>(interpolatedResult);

  #define INTERPOLATE(x) res.x = x + t*(s.x - x)

  INTERPOLATE(_support);  // 0..1
  INTERPOLATE(_thrust);  // 0..1
  INTERPOLATE(_turn);  // continuous
  INTERPOLATE(_turnBySpeed);  // -1..1
  res._wheelPhase = fastFmod(_wheelPhase + t * s._wheelPhaseDelta, 1.0f);  // mod 1: assume that `t1state` was created by Simulate() from `*this`
  _dampers.Interpolate(s._dampers, t, res._dampers);

  #undef INTERPOLATE
}


float MotorcycleVisualState::GetRPM(MotorcycleType const& type) const
{
  return (type._isBicycle) ? floatMax(_thrust, 0) : fabs(_thrust) * 0.5 + 0.5;
}


AnimationStyle Motorcycle::IsAnimated( int level ) const {return AnimatedGeometry;}

inline float Motorcycle::GetGlassBroken() const
{
  const MotorcycleType *type = Type();
  float glassDamage = GetHitCont(type->_bodyHit);
  saturateMax(glassDamage,GetTotalDamage());
  saturateMax(glassDamage,GetHitCont(type->_glassLHit));
  saturateMax(glassDamage,GetHitCont(type->_glassRHit));
  return glassDamage;
}

float Motorcycle::GetHitForDisplay(int kind) const
{
  // see InGameUI::DrawTankDirection
  switch (kind)
  {
  case 0: return GetHitCont(Type()->_bodyHit);
  case 1: return GetHitCont(Type()->_engineHit);
  case 2:return GetHitCont(Type()->_fuelHit);
  case 3:
    {
      float hit  = floatMax(GetHitCont(Type()->_wheelBHit), GetHitCont(Type()->_wheelFHit));
      return hit;
    }  
  default: return 0;
  }
}

void Motorcycle::DamageAnimation(AnimationContext &animContext, int level, float dist2)
{
  const MotorcycleType *type = Type();
  // scan corresponding wound

  float glassDamage = GetGlassBroken();
  if (glassDamage>=0.6)
  {
    // TODO: convert to 3-step wounds
    type->_glassDamageFull.Apply(animContext, _shape, level, 2, dist2);
  }
  else if (glassDamage>=0.3)
  {
    type->_glassDamageHalf.Apply(animContext, _shape, level, 1, dist2);
  }
}

void Motorcycle::DamageDeanimation( int level )
{
}

/*!
\patch 1.85 Date 9/12/2002 by Ondra
- New: Support for bicycle pedals animation.
*/

void Motorcycle::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  ShapeUsed shape = _shape->Level(level);
  if (shape.IsNull()) return;

  if (_weaponsState.ShowFire())
  {
    Type()->_weapons._animFire.Unhide(animContext, _shape, level);
    Type()->_weapons._animFire.SetPhase(animContext, _shape, level, _weaponsState._mGunFirePhase);
  }
  else
  {
    Type()->_weapons._animFire.Hide(animContext, _shape, level);
  }

  DamageAnimation(animContext, level, dist2);

  base::Animate(animContext, level, setEngineStuff, parentObject, dist2);
  // assume min-max box is not changed
  animContext.ValidateMinMax();
}

void Motorcycle::Deanimate( int level, bool setEngineStuff )
{
  if( _shape->Level(level).IsNull() ) return;

  base::Deanimate(level,setEngineStuff);

  DamageDeanimation(level);
}


Vector3 Motorcycle::Friction( Vector3Par speed )
{
  Vector3 friction;
  friction.Init();
  friction[0]=speed[0]*fabs(speed[0])*10+speed[0]*20+fSign(speed[0])*10;
  friction[1]=speed[1]*fabs(speed[1])*7+speed[1]*20+fSign(speed[1])*10;
  friction[2]=speed[2]*fabs(speed[2])*5+speed[2]*20+fSign(speed[2])*10;
  return friction*GetMass()*(1.0/4500);
}

void Motorcycle::MoveWeapons(float deltaT)
{
}

bool Motorcycle::IsBlocked() const
{
  return false;
}

void Motorcycle::PlaceOnSurface(Matrix4 &trans)
{
  base::PlaceOnSurface(trans);
  
  if (!GetShape()) return;

 

  // place in steady position
  Vector3 pos=trans.Position();
  Matrix3 orient=trans.Orientation();
  
  float dx,dz;
  pos[1] = GLandscape->RoadSurfaceYAboveWater(pos,&dx,&dz);



  Matrix3 vertical, invVertical;
  vertical.SetUpAndDirection(VUp,orient.Direction());
  invVertical = vertical.InverseRotation();

  // front-back direction based on terrain normal
  Vector3Val landNormal=Vector3(-dx,1,-dz).Normalized();

  // convert upWanted into vertical coordinate space
  Vector3 relUpWanted = invVertical*landNormal;
  relUpWanted[0] = 0;

  orient.SetUpAndDirection(vertical*relUpWanted,trans.Direction());
  trans.SetOrientation(orient);

  const Shape *geom = _shape->LandContactLevel();
  if (!geom) geom = _shape->GeometryLevel();
  if (!geom)
  {
    Fail("No geometry");
  }

  // dynamic vehicle
  if (geom)
  {
    Vector3 minC(0, geom->Min().Y(), 0); 
    pos -= orient*minC;
  }

  pos -= FutureVisualState().DirectionUp() * Type()->_dampers.GetInitPos(); // dampers

  trans.SetPosition(pos);
}


/*!
\patch 5164 Date 6/5/2007 by Ondra
- Fixed: MP: Player driving motorcycle could die suddenly with no apparent reason.
*/
void Motorcycle::SimulateOneIter( float deltaT, SimulationImportance prec )
{
  _isUpsideDown=false;
  _isDead=IsDamageDestroyed();

  if( _isDead )
  {
    IExplosion *smoke = GetSmoke();
    if( smoke ) smoke->Explode();
    NeverDestroy();
  }

  MoveWeapons(deltaT);

  if (!HasSomePilot())
  {
    //_engineOff = true;
    _pilotBrake = true;
  }

  if (IsBlocked())
  {
    _pilotBrake = true;
    _thrustWanted = 0;
    _turnWanted = 0;
  }

  base::Simulate(deltaT,prec);

  Vector3Val speed=FutureVisualState().ModelSpeed();
  float speedSize=fabs(speed.Z());

  bool isBicycle = Type()->_isBicycle;

  if (!isBicycle)
  {  
    ConsumeFuel(deltaT*0.2* GetHit(Type()->_fuelHit));
    if (FutureVisualState()._fuel<=0)
      _engineOff = true;

    // tank is going to explode
    if (GetHit(Type()->_fuelHit)>=0.9 || GetHit(Type()->_engineHit)>=0.9)
    {
      if( IsLocal() && _explosionTime>Glob.time+60 )
        // set some explosion
        _explosionTime=Glob.time+GRandGen.Gauss(2,5,20);
    }
  
    if( !_engineOff )
    {
      ConsumeFuel(deltaT*(0.01+FutureVisualState()._rpm*0.02));
      // calculate engine rpm
      _rpmWanted=speedSize*_gearBox.Ratio();
      saturateMax(_rpmWanted,fabs(FutureVisualState()._thrust)*0.3+0.2);
    }
    else
      _rpmWanted=0;
  }
  else
  {
    // calculate fictive engine rpm
    _rpmWanted=speedSize*_gearBox.Ratio()/Type()->GetMaxSpeedMs();
    saturateMax(_rpmWanted,fabs(FutureVisualState()._thrust)*0.3+0.2);
    FutureVisualState()._fuel = 0;
    _engineOff = false;
  }

  float delta;

  if (!IsDamageDestroyed() && Driver())
  {
    float supportWanted = fabs(FutureVisualState().ModelSpeed().Z());
    saturate(supportWanted,0,1);
    //float supportWanted = 2;

    delta=supportWanted-FutureVisualState()._support;
    Limit(delta,-deltaT,+deltaT);
    FutureVisualState()._support+=delta;
  }

  delta=_rpmWanted-FutureVisualState()._rpm;
  Limit(delta,-0.5*deltaT,+0.3*deltaT);
  FutureVisualState()._rpm+=delta;

  // calculate all forces, frictions and torques
  Vector3 force(VZero),friction(VZero);
  Vector3 torque(VZero),torqueFriction(VZero);

  Vector3 pForce(VZero); // partial force
  Vector3 pCenter(VZero); // partial force application point

  // simulate left/right engine
  {
    static float mouseWheelSpeedCoef = 3.0f; // for mouse we allow the wheel to be turned a lot faster
    delta = _turnSpeedWanted;

    float maxDelta = (delta * FutureVisualState()._turnBySpeed > 0.0f) ? _turnIncreaseSpeed*mouseWheelSpeedCoef : _turnDecreaseSpeed*mouseWheelSpeedCoef;
    Limit(delta, -maxDelta*deltaT, +maxDelta*deltaT);

    FutureVisualState()._turnBySpeed += delta;
    Limit(FutureVisualState()._turnBySpeed, -1, +1);
  }

  // main engine
  // if we go too slow we cannot turn at all
  // turn wheels
  float wheelLen=Type()->_wheelCircumference;
  FutureVisualState()._wheelPhaseDelta = speed.Z()*(1/wheelLen)*deltaT;
  FutureVisualState()._wheelPhase = fastFmod(FutureVisualState()._wheelPhase + FutureVisualState()._wheelPhaseDelta, 1);

  if (_isDead) _engineOff=true,_pilotBrake=true;
  else if (_waterDrown) _engineOff = true;
  else if (_isUpsideDown) _pilotBrake=true;
  if( FutureVisualState()._fuel<=0 && !isBicycle) _engineOff=true;
  if( _engineOff ) _thrustWanted=0;

  if( _thrustWanted*FutureVisualState()._thrust<0 )
    _pilotBrake=true;
  if( FutureVisualState().ModelSpeed()[2]*_thrustWanted<0 && fabs(FutureVisualState().ModelSpeed()[2])>0.5 )
    _pilotBrake=true;

  if( _pilotBrake && fabs(FutureVisualState().ModelSpeed()[2])>0.5 ) _thrustWanted=0;

  if( fabs(_thrustWanted)>0.1 )
    OnMoved();
  else
  {  
    // if there is driver and motorcycle is lying motorcycle, do not switch offf simulation
    if ((QIsDriverIn() || GetManCargoSize() > 0)  && fabs(FutureVisualState().DirectionAside() * VUp) > 0.3f)
      OnMoved();
  }

  {
    // handle impulse
    float impulse2=_impulseForce.SquareSize();
    //float speedSize = ModelSpeed().SquareSize() + _angVelocity.SquareSize();
    //if( impulse2 > 0 || speedSize > 0
    //        /*Square(GetMass()*0.01) */)
    if ( impulse2 > 0 || FutureVisualState()._speed.SquareSize() > 0.1f || _angVelocity.SquareSize() > 0.1f)
      OnMoved();

    if( impulse2>Square(GetMass()*3) && IsLocal())
    {
      // too strong impulse - dammage
      float contact=sqrt(impulse2)/(GetMass()*10);
      // contact>0
      saturateMin(contact,5);
      if( contact>0.1 )
      {
        float radius=GetRadius();
        LocalDamageMyself(VZero,contact*0.1,radius*0.3);
      }
    }
  }

  if (!_isStopped && !CheckPredictionFrozen() && !IsAttached())
  {

    delta=_thrustWanted-FutureVisualState()._thrust;
    if( FutureVisualState()._thrust*_thrustWanted<=0 )
      Limit(delta,-2.0*deltaT,+2.0*deltaT);
    else
      Limit(delta,-0.5*deltaT,+0.5*deltaT);
    FutureVisualState()._thrust+=delta;

    // do not allow fast reverse
    float minThrust=Interpolativ(FutureVisualState().ModelSpeed()[2],-5,0,0,-1);
    Limit(FutureVisualState()._thrust,minThrust,1.0);
    
    /*float asz = fabs(speed.Z());
    //float isFast = *(1.0/Type()->GetMaxSpeedMs());
    //saturate(isFast,0,1);
    float limitTurn=2;
    if (asz>1)
    {
      limitTurn /= asz;
    }
    saturate(_turnWanted,-limitTurn,+limitTurn);
    */

    delta=_turnWanted - FutureVisualState()._turn;
    if( delta * FutureVisualState()._turn>0 )
    {
      float maxDelta=_turnIncreaseSpeed;
      Limit(delta,-maxDelta*deltaT,+maxDelta*deltaT);
    }
    else
    {
      float maxDelta=_turnDecreaseSpeed;
      Limit(delta,-maxDelta*deltaT,+maxDelta*deltaT);
    }
    FutureVisualState()._turn+=delta;

    // simulate front wheel (turning)
    float turnEff = floatMinMax(FutureVisualState()._turn + FutureVisualState()._turnBySpeed, -1, +1);
    float physTurn = turnEff;

    float turnForward = speed.Z()*0.49;        

    saturate(turnForward,-25,+25); // avoid slips in high speed
    turnForward *= 0.4*Type()->_turnCoef;

    float turnWanted = physTurn * turnForward * 0.2 * Type()->_turnCoef;

    // limit turn on motorcycle doesn't look well big turns in high speeds
    saturateMax(turnForward,0.1);
#define MAX_ALLOWED_MOMENT 3
    float maxTurn = MAX_ALLOWED_MOMENT / turnForward;
    saturate(FutureVisualState()._turn, -maxTurn, maxTurn);

    Vector3 wCenter(VFastTransform,FutureVisualState().ModelToWorld(),GetCenterOfMass());   

    // apply left/right thrust
    bool gearChanged=false;
    if( _engineOff )
      // upside down
      gearChanged=_gearBox.Neutral();
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
    }

    float wheelFHit = GetHit(Type()->_wheelFHit);
    float wheelBHit = GetHit(Type()->_wheelBHit);
    float wheelHit = floatMax(wheelFHit,wheelBHit)*0.9;

    if( !_engineOff || fabs(turnWanted)>0.001 )
    {
      if( _landContact || _objectContact )
      {
        float invSpeedSize;
        //const float coefInvSpeed=3;
        //const float minInvSpeed=8;      
        const float coefInvSpeed=1.5;
        const float minInvSpeed=4;

        float power = 2 - speedSize / Type()->GetMaxSpeedMs(); // power opf engine       
        saturateMax(power, 0); 

        if( speedSize<minInvSpeed) 
          invSpeedSize=coefInvSpeed/minInvSpeed;
        else 
          invSpeedSize=coefInvSpeed/speedSize;

        invSpeedSize*=3;
        
        float thrust = FutureVisualState()._thrust*4*( 1-GetHit(Type()->_engineHit) )*(1-wheelHit)* power;

        // Limit wanted turn according to steering angle.
        float maxMoment = 3 * cos(floatMinMax(FutureVisualState()._turn + FutureVisualState()._turnBySpeed, -1, 1));

        saturate(turnWanted, - maxMoment, + maxMoment);

        //LogF("speed %lf, _turn %lf, turnWanted %lf, maxMoment %lf, invSpeedSize %lf, power %lf", _speed.Z(), FutureVisualState()._turn, turnWanted, maxMoment,invSpeedSize);

        float lAccel=(thrust - turnWanted)*invSpeedSize;
        float rAccel=(thrust + turnWanted)*invSpeedSize;

        pForce=Vector3(0,0,lAccel*GetMass()*0.5);
        force+=pForce;
        if (Type()->_dampers.IsUsed()) // has dampers so turn it front/back by acceleration/breaking
          pCenter=Vector3(+4,-0.3,0); // relative to the center of mass
        else
          pCenter=Vector3(+4,0,0);

        torque+=pCenter.CrossProduct(pForce);
#if ARROWS
        AddForce(FutureVisualState().DirectionModelToWorld(pCenter)+wCenter, FutureVisualState().DirectionModelToWorld(pForce*InvMass()), Color(0,0,0));
#endif

        pForce=Vector3(0,0,rAccel*GetMass()*0.5);
        force+=pForce;
        if (Type()->_dampers.IsUsed()) // has dampers so turn it front/back by acceleration/breaking
          pCenter=Vector3(-4,-0.3,0); // relative to the center of mass
        else
          pCenter=Vector3(-4,0,0);

        torque+=pCenter.CrossProduct(pForce);
#if ARROWS
        AddForce(FutureVisualState().DirectionModelToWorld(pCenter)+wCenter, FutureVisualState().DirectionModelToWorld(pForce*InvMass()), Color(0,0,0));
#endif
      }
    }

    if( gearChanged )
    {
      if( _gearSound )
        _gearSound->Stop();
      AbstractWave *sound=GSoundScene->OpenAndPlayOnce(Type()->_gearSound.name,this,true,FutureVisualState().Position(),FutureVisualState().Speed(), Type()->_gearSound.vol, Type()->_gearSound.freq, Type()->_gearSound.distance);
      if( sound )
      {
        GSoundScene->SimulateSpeedOfSound(sound);
        _gearSound=sound;
        GSoundScene->AddSound(sound);
      }
    }
    
    // convert forces to world coordinates
    FutureVisualState().DirectionModelToWorld(torque,torque);
    FutureVisualState().DirectionModelToWorld(force,force);

    // apply gravity
    pForce=Vector3(0,-G_CONST*GetMass(),0);
    force+=pForce;
    
#if ARROWS
    AddForce(wCenter,pForce*InvMass(),Color(1,1,0));
#endif

    // angular velocity causes also some angular friction, this should be simulated as torque
    torqueFriction = _angMomentum * (Driver() ? 0.5f : 2.0f);

    // calculate new position
    VisualState moveTrans = PredictPos<VisualState>(deltaT);

    ApplyRemoteState(deltaT,moveTrans);
    // body air friction
    FutureVisualState().DirectionModelToWorld(friction,Friction(speed));
    //friction=Vector3(0,0,0);
#if ARROWS
    AddForce(wCenter,friction*InvMass());
#endif
    
    wCenter.SetFastTransform(moveTrans.ModelToWorld(),GetCenterOfMass());
    
    float soft=0,dust=0;
    if( deltaT>0 )
    {
      // check collision on new position
      float crash=0;
      float sFactor=Type()->GetMaxSpeedMs()*1.3;
      Vector3 fSpeed=speed-Vector3(0,0,FutureVisualState()._thrust*sFactor);
      // avoid too fast accel/deccel
      float maxAcc = floatMin(10,Type()->GetMaxSpeedMs()*0.14);
      saturate(fSpeed[0],-maxAcc,+maxAcc);
      saturate(fSpeed[1],-maxAcc,+maxAcc);
      saturate(fSpeed[2],-maxAcc*0.6,+maxAcc);
      float brakeFriction=0;

      saturateMax(brakeFriction,_pilotBrake);
      saturateMax(brakeFriction,FutureVisualState().DirectionUp().Y()<=0.3);
      saturateMax(brakeFriction,wheelHit-0.5);

      fSpeed=fSpeed*(1-brakeFriction)+speed*brakeFriction;

      Vector3 objForce(VZero); // total object force
      Vector3 landForce(VZero); // total land force

      _objectContact=false;
      float maxColSpeed2=0;
      float maxCFactor = 0;
      if( prec<=SimulateVisibleFar && IsLocal())
      {
        #define MAX_IN 0.2
        #define MAX_IN_FORCE 0.1
        #define MAX_IN_FRICTION 0.2

        AddImpulseNetAwareAccumulator accum;
        DoDamageAccumulator dmgAcc;
        CollisionBuffer collision;
        GLOB_LAND->ObjectCollision(collision,this,moveTrans);
        for( int i=0; i<collision.Size(); i++ )
        {
          CollisionInfo &info=collision[i];
          Object *obj=info.object;
          if( !obj ) continue;
          if( obj->IsPassable() ) continue;
          _objectContact=true;
          // info.pos is relative to object
          float cFactor=obj->GetMass()*InvMass();
          if( obj->Static() )
          {
            // fixed object - apply fixed collision routines
            // calculate his dammage
            // depending on vehicle speed and mass
            float dFactor=GetMass()*obj->InvMass();
            float dSpeed = FutureVisualState()._speed.SquareSize()+_angVelocity.SquareSize();
            float dammage = dSpeed*obj->GetInvArmor()*dFactor*0.2;
            if( dammage>0.01 )
            {
              if (GetNetworkManager().IsServerOwner()) obj->LocalDamageAccum(dmgAcc,ShotInfo(NULL,this,this,VZero),dammage,obj->GetShape()->GeometrySphere(),RString(), SkinnedPosition());
            }
            if (obj->GetDestructType()==DestructTree || obj->GetDestructType()==DestructTent || obj->GetDestructType()==DestructMan)
              saturate(cFactor,0.001,0.5);
            else
              saturate(cFactor,0.2,2);
          }
          else
          {
            saturate(cFactor,0,10);
            obj->SimulationNeeded(GetVisualStateAge(FutureVisualState()));
          }

          Vector3 pos=info.pos;
          Vector3 dirOut=info.dirOut;
          // create a force pushing "out" of the collision
          float forceIn=floatMin(info.under,MAX_IN_FORCE);
          Vector3 pForce=dirOut*GetMass()*20*cFactor*forceIn;
          Vector3 pTorque=pForce*0.5;
          // apply proportional part of force in place of impact
          pCenter=pos-wCenter;
          if( cFactor>0.05 )
          {
            objForce+=pForce;
            torque+=pCenter.CrossProduct(pTorque);
          }
          Vector3Val objSpeed=info.object->ObjectSpeed();
          Vector3 colSpeed=FutureVisualState()._speed-objSpeed;
          bool isFixed = true;
          Vehicle *veh=dyn_cast<Vehicle,Object>(obj);
          if( veh )
          {
            // transfer all my intertia to him?
            Vector3 relDistance = veh->FutureVisualState().Position()-FutureVisualState().Position();
            Vector3 relSpeed = objSpeed-FutureVisualState().Speed();
            float speedTransfer = relSpeed*relDistance*-relDistance.InvSize()*0.2f;
            saturate(speedTransfer,0,1);
            float transferFactor = GetMass()*veh->GetInvMass();
            saturate(transferFactor,0,0.3);
            transferFactor *= speedTransfer;
            Vector3 impulse = FutureVisualState()._speed*GetMass()*deltaT*transferFactor;
#if 0
            LogF("%s: Motorcycle add impulse %.1f,%.1f,%.1f, spd %.3f, speed %.1f,%.1f,%.1f dist %.1f,%.1f,%.1f",
              (const char *)veh->GetDebugName(), impulse[0],impulse[1],impulse[2],speedTransfer,
              relSpeed[0],relSpeed[1],relSpeed[2], relDistance[0],relDistance[1],relDistance[2]);
            LogF("  car speed %.1f,%.1f,%.1f",_speed[0],_speed[1],_speed[2]);
            LogF("  obj speed %.1f,%.1f,%.1f",objSpeed[0],objSpeed[1],objSpeed[2]);
#endif
            accum.Add(veh,impulse,(info.pos - wCenter).CrossProduct(-pTorque*deltaT));
            // vehicle is considered fixed when it is very heavy and slow moving
            // or it is static object)
            isFixed = veh->Static();
            if (!isFixed)
            {
              if (veh->GetMass()>GetMass()*2)
                isFixed = true;
            }
          }

          saturateMax(maxColSpeed2,colSpeed.SquareSize());
          saturateMax(maxCFactor,cFactor);

          if( cFactor<0.05 ) continue;

          // if info.under is bigger than MAX_IN, move out
          if (isFixed)
          {
            if(info.under>MAX_IN)
            {

              Matrix4 transform=moveTrans.Transform();
              Point3 newPos=transform.Position();
              float moveOut=info.under-MAX_IN;
              Vector3 move=dirOut*moveOut*0.1;
              newPos+=move;
              transform.SetPosition(newPos);
              moveTrans.SetTransform(transform);
            }
            Vector3 colSpeed = FutureVisualState().Speed()-obj->ObjectSpeed();
            // limit relative speed to object we crashed into
            const float maxRelSpeed=0.5;
            if( colSpeed.SquareSize()>Square(maxRelSpeed) )
            {
              // adapt _speed to match criterion
              float crashSpeed = colSpeed.Size()-2.0f;
              if (crashSpeed>0)
                crash+=crashSpeed*0.3f;
              colSpeed.Normalize();
              colSpeed*=maxRelSpeed;
              // only slow down
              float oldSize = FutureVisualState()._speed.Size();
              FutureVisualState()._speed = colSpeed+objSpeed;
              if (FutureVisualState()._speed.SquareSize()>Square(oldSize))
                FutureVisualState()._speed = FutureVisualState()._speed.Normalized()*oldSize;
            }
          }

          // second is "land friction" - causing little momentum
          float frictionIn=floatMin(info.under,MAX_IN_FRICTION);
          pForce[0]=fSign(speed[0])*10000;
          pForce[1]=speed[1]*fabs(speed[1])*1000+speed[1]*8000+fSign(speed[1])*10000;
          pForce[2]=speed[2]*fabs(speed[2])*150+speed[2]*250+fSign(speed[2])*2000;

          FutureVisualState().DirectionModelToWorld(pForce,pForce);
          pForce*=GetMass()*(4.0/10000)*frictionIn;
          //saturateMin(pForce[1],0);
          //torque-=pCenter.CrossProduct(pForce);
#if ARROWS
          AddForce(wCenter+pCenter,-pForce*InvMass());
#endif
          friction+=pForce;
          torqueFriction+=_angMomentum*0.15;
        }
      } // if( object collisions enabled )
      
      // simulate damper forces
      //const float adaptDamper=15*scaleDamper;
      DamperForces forces(Type()->_dampers);
      // check for collisions

      GroundCollisionBuffer gCollision;
      float softFactor=floatMin(1000/GetMass(),0.5);

      // check if motorcycle is already up-down
      bool forceLandContact = FutureVisualState().DirectionUp().Y()>0.1f && !IsDamageDestroyed() && Driver();

      const float aboveRoad = 0.02f;
      int nContactPoint = GLandscape->GroundCollision(gCollision,this,moveTrans,aboveRoad,softFactor,forceLandContact);

      _landContact=false;
      _objectContact=false;
      _waterContact=false;

      float maxUnderWater = 0;

      if( gCollision.Size()>0 )
      {
        if (!Driver())
          torqueFriction += _angMomentum*2;
        #define MAX_UNDER 0.1
        #define MAX_UNDER_FORCE 0.05

        saturateMin(nContactPoint,5);        
        
        float contactCoef = 8.0f/nContactPoint;

        float maxUnder=0;
        
        SetSurfaceInfo(gCollision);

        Vector3 angVelocityModel = DirectionWorldToModel(_angVelocity);
        
        bool breakingProduceMomentum = Type()->_dampers.IsUsed() && 
        (speedSize > 0.5f || FutureVisualState()._thrust > 0);  // otherwise produce instabilities.
        
        // scan all contact points
        for( int i=0; i<gCollision.Size(); i++ )
        {
          // info.pos is world space
          UndergroundInfo &info=gCollision[i];
          // we consider two forces
          if( info.under<0 ) continue;

          // one is ground "pushing" everything out - causing some momentum
          Vector3 dirOut=Vector3(0,info.dZ,1).CrossProduct(Vector3(1,info.dX,0)).Normalized();
          pCenter=info.pos-wCenter;
          
          float under;
          if( info.type==GroundWater )
          {
            const float coefNPoints=16.0/4.0;

            under=info.under*0.002;
            _waterContact=true;

            saturateMax(maxUnderWater,info.under);

            // significant "water friction"
            pForce[0]=speed[0]*fabs(speed[0])*15;
            pForce[1]=speed[1]*fabs(speed[1])*15+speed[1]*160;
            pForce[2]=speed[2]*fabs(speed[2])*6;

            pForce=FutureVisualState().DirectionModelToWorld(pForce*info.under)*GetMass()*(coefNPoints/700);
#if ARROWS
            AddForce(wCenter+pCenter,-pForce*InvMass());
#endif
            friction+=pForce;
          }
          else
          {
            _landContact=true;

            if( info.level==_shape->FindLandContactLevel() && Type()->_dampers.IsUsed())
            {
              under=info.under - aboveRoad;
              if (under < 0) continue;

              if( maxUnder<under ) maxUnder=under;
              saturateMin(under, MAX_UNDER_FORCE); 

              float dirOutUp = dirOut * FutureVisualState().DirectionUp();
              if (dirOutUp > 0.5f ) // very low number can lead to instability 0.5 is "numerical  barrier"
              {              
                under /= dirOutUp;

                // world space velocity, angular motion is considered as well
                Vector3 pointVelocity = FutureVisualState()._speed - pCenter.CrossProduct(_angVelocity);

                // only vertical part of velocity in model space is important for us
                float pointSpeedUp = DirectionWorldToModel(pointVelocity).Y();

                float forceUp,frictionUp;  

                forces.SimulateContact(forceUp, frictionUp, FutureVisualState()._dampers, Type()->_dampers, info.vertex, true, under, pointSpeedUp);

                under *= dirOutUp;                
                  
                // calculate force and torque      
                pForce = FutureVisualState().DirectionUp()*forceUp;
                torque += pCenter.CrossProduct(pForce);
                landForce += pForce;

                // calculate friction
                friction += FutureVisualState().DirectionUp()*frictionUp;
                torqueFriction += pCenter.CrossProduct(FutureVisualState().DirectionUp()*frictionUp);
              }                                        
            }
            else
            {
              under=info.under; /* - aboveRoad; for pure penetration forces must be equilibrium position on road */
              if( maxUnder<under - aboveRoad) maxUnder=under - aboveRoad;
              saturateMin(under, MAX_UNDER_FORCE);
            }

            /// rest is under will be done by penetration forces... 
            pForce=dirOut*GetMass()*80.0f*contactCoef*under;
            torque+=pCenter.CrossProduct(pForce);

            landForce += pForce;
          }

          // some friction is caused by moving the land aside
          // this applies only to soft surfaces
          if( info.texture )
          {
            soft=info.texture->Roughness()*2.5f;
            dust=info.texture->Dustness()*2.5f;
            saturateMin(dust,1);
          }          

#if ARROWS
          AddForce(wCenter+pCenter,pForce*under*InvMass(),Color(0,1,0));
#endif
          
          // second is "land friction" - causing little momentum
         
          if (!Driver() && fabs(FutureVisualState().DirectionAside() *  VUp) > 0.3f) // motorcycle lying
          {
            pForce[0]=fSpeed[0]*7000+fSign(fSpeed[0])*22000;
            pForce[1]=fSpeed[1]*7000+fSign(fSpeed[1])*22000;
            pForce[2]=fSpeed[2]*7000+fSign(fSpeed[2])*22000;
          }
          else
          {
            pForce[0]=fSpeed[0]*7000+fSign(fSpeed[0])*220000;
            pForce[1]=fSpeed[1]*8000+fSign(fSpeed[1])*10000;
            pForce[2]=fSpeed[2]*200+fSign(fSpeed[2])*30000;
            if( brakeFriction<0.7 )
              pForce[2]*=0.1;
          }
          // friction can not be applied in same direction as speed
          if( pForce[0]*speed[0]<0 ) pForce[0]=0;
          if( pForce[1]*speed[1]<0 ) pForce[1]=0;
          if( pForce[2]*speed[2]<0 ) pForce[2]=0;

          pForce *= (GetMass()*(1.0/40000)*contactCoef);

          if (breakingProduceMomentum) // has dampers so turn it front/back by acceleration/breaking
            torque += FutureVisualState().DirectionAside()*(0.3f)*pForce[2]; // breaking force must produce torque
          pForce=FutureVisualState().DirectionModelToWorld(pForce);

#if 0 //ARROWS
          AddForce(wCenter+pCenter,-pForce*InvMass(),Color(0,1,0));
#endif

#if 0
          float scale = fabs(fSpeed[2])-0.5;
          saturate(scale,0,1);
          torque -= pCenter.CrossProduct(pForce*scale);
#endif

          friction+=pForce;

          torqueFriction+=_angMomentum*0.5*contactCoef;
          //**************************************
          //torqueFriction+=_angMomentum*1.5*contactCoef;

          //float landMoved=info.under;
          //saturateMin(landMoved,0.1);
          if (!Driver() && fabs(FutureVisualState().DirectionAside() *  VUp) > 0.3f) // motorcycle lying
            pForce = VZero;
          else
          {          
            pForce[0]=speed[0]*4500;
            pForce[1]=0;
            pForce[2]=speed[2]*500;
          }

          pForce *= GetMass()*(1.0/1000)*contactCoef*0.05f*soft*Type()->_terrainCoef;
#if ARROWS
          AddForce(wCenter+pCenter,-pForce*InvMass(),Color(0,1,1));
#endif
          if (breakingProduceMomentum) // has dampers so turn it front/back by acceleration/breaking
            torque += FutureVisualState().DirectionAside()*(0.3f)*pForce[2]; // breaking force must produce torque, because engine produce opposite torque

          friction+=FutureVisualState().DirectionModelToWorld(pForce);
        }
        // stabilize (advanced approach would be to simulate gyroscopic forces)
        // but we think simple approach should be good enough
               
        float minDirY = Driver() ? 0.1 : 0.8;
       

        // if there is no driver, stabilization can be done only by support
        if (FutureVisualState().DirectionUp().Y()>minDirY && !IsDamageDestroyed())
        {
          if (Driver() || FutureVisualState()._support<0.1 )
          {
            float dx,dz;
            GLandscape->RoadSurfaceY(moveTrans.Position(),&dx,&dz);

            float turnA =0.5f*(FutureVisualState()._turn + FutureVisualState()._turnBySpeed);
            saturate(turnA,-0.15f,+0.15f);
            float asideWanted = turnA*0.1f*fabs(FutureVisualState().ModelSpeed().Z());
            //float asideWanted = 0;

            saturate(asideWanted,-0.86f,+0.86f);

#if 1
            // predict direction up in some time
            float dirEstT = 0.1f;
            const Matrix3 &orientation=moveTrans.Orientation();
            Matrix3 derOrientation=_angVelocity.Tilda()*orientation;
            Matrix3Val estOrientation=orientation+derOrientation*dirEstT;

            Vector3Val estDirectionUp=estOrientation.DirectionUp().Normalized();
#else
            Vector3Val estDirectionUp=moveTrans.DirectionUp();
#endif

            // but with up direction vertical
            Matrix3 orient = moveTrans.Orientation();
            Matrix3 vertical, invVertical;
            vertical.SetUpAndDirection(VUp,orient.Direction());
            invVertical = vertical.InverseRotation();

            // front-back direction based on terrain normal
            Vector3Val landNormal=Vector3(-dx,1,-dz).Normalized();

            const float maxCosA = 0.17; // cos 80
            if (landNormal*FutureVisualState().DirectionUp()<maxCosA ) // not directly after get in ... 
            {
              if (IsLocal() && _allowEjectAfter < Glob.time)
                EjectAllNotFixed();
            }             

            // convert upWanted into vertical coordinate space
            Vector3 upDirWanted = landNormal;
            Vector3 relUpWanted = invVertical*upDirWanted;
            relUpWanted[0] = asideWanted;

            relUpWanted.Normalize();

            upDirWanted = vertical*relUpWanted;


            Vector3 stabilize=upDirWanted-estDirectionUp;
            //stabilize[1]=0;
            const float maxStabForce = 0.8f;
            if (stabilize.SquareSize()>Square(maxStabForce))
              stabilize = stabilize.Normalized()*maxStabForce;

            pForce = stabilize*30*GetMass();
            /**/
            FutureVisualState().DirectionWorldToModel(pForce,pForce);
            pForce[2]=0;
            FutureVisualState().DirectionModelToWorld(pForce,pForce);
            /**/

            pCenter = FutureVisualState().DirectionUp();
#if ARROWS
            AddForce(pCenter+wCenter,pForce*InvMass(),Color(1,0,1));
#endif
            torque += pCenter.CrossProduct(pForce);

            /*
            pForce[0] = -GetMass()*asideWanted*20;
            pForce[1] = 0 ;
            pForce[2] = 0 ;
            DirectionModelToWorld(pForce,pForce);
#if ARROWS
            AddForce(pCenter+wCenter,pForce*InvMass(),Color(0,0,1));
#endif
            torque += pCenter.CrossProduct(pForce);
            */


#if ARROWS
            AddForce(pCenter+wCenter+VUp*2,upDirWanted*3,Color(0.2,0.2,0.2));
            AddForce(pCenter+wCenter+VUp*2,FutureVisualState().DirectionUp()*4,Color(0.4,0.0,0.0));
            AddForce(pCenter+wCenter+VUp*2,estDirectionUp*5,Color(0.5,0.5,0.0));
#endif
          }
        }
        else
        {
          if (IsLocal() && _allowEjectAfter < Glob.time)
            EjectAllNotFixed();           
        }

        //torqueFriction=_angMomentum*1.0;
        if (_waterContact)
        {
          const SurfaceInfo &info = GLandscape->GetWaterSurface();
          soft = info._roughness * 2.5;
          dust = info._dustness * 2.5;
          saturateMin(dust , 1);
        }
        if( maxUnder>MAX_UNDER )
        {
          // it is necessary to move object immediately
          Matrix4 transform=moveTrans.Transform();
          Vector3 newPos=transform.Position();
          float moveUp=maxUnder-MAX_UNDER;
          newPos[1]+=moveUp;
          transform.SetPosition(newPos);
          moveTrans.SetTransform(transform);
          // we move up - we have to maintain total energy
          // what potential energy will gain, kinetic must loose
          const float crashLimit=0.3;
          if( moveUp>crashLimit )
          {
            crash+=moveUp-crashLimit;
            saturateMax(maxCFactor,1);
          }
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

        if (IsLocal())
        { // water induced damage
          const float maxFordMax=1.0f;
          // if more than half in water, water is probably filling the engine
          const Shape *geom = _shape->GeometryLevel();
          float maxFordByShape = (geom->Max().Y()-geom->Min().Y())*0.3f;
          float maxFord = floatMin(maxFordMax,maxFordByShape);

          if( maxUnderWater>maxFord )
          {
            float damage=(maxUnderWater-maxFord)*0.5f;
            saturateMin(damage,0.2f);
            LocalDamageMyself(VZero,damage*deltaT,GetRadius());
            // engine has no way to continue working
            _waterDrown = true;
          }
        }
      }

      forces.Finish(FutureVisualState()._dampers,Type()->_dampers,deltaT);
      
#if _DEBUG
      if (this==GWorld->CameraOn())
        GlobalShowMessage(100,"Dampers %.2f, %.2f",FutureVisualState()._dampers[0],FutureVisualState()._dampers[1]);
#endif
      force+=objForce;
      force+=landForce;

      float crashTreshold=10*GetMass(); // 2G
      float forceCrash=0;
      if( objForce.SquareSize()>Square(crashTreshold) )
      {
        // crash as g-term
        forceCrash=(objForce.Size()-crashTreshold)*InvMass()*(1.0f/10);
        crash+=forceCrash;
      }
      if( crash>0.1f )
      {
        float speedCrash=(maxColSpeed2-3)*Square(1.0f/7);
        if( speedCrash<0.1f ) speedCrash=0;
        if (Glob.time>_disableDamageUntil && speedCrash>0)
        {
          // crash boom bang state - impact speed too high
          _doCrash=CrashLand;
          if( _objectContact ) _doCrash=CrashObject;
          if( _waterContact ) _doCrash=CrashWater;
          _crashVolume=crash*0.2f;
          saturateMin(crash,speedCrash);
          crash *= floatMin(1,maxCFactor);
          CrashDammage(crash); // 1g -> 5 mm dammage
          DamageCrew(Driver(),crash*0.03f,"");
          //LogF("Crash %g, speed %g, factor %g", crash, sqrt(maxColSpeed2), maxCFactor);
        }
      }
    }
    
    // apply all forces
    /*
    LogF("torq %.3f,%.3f,%.3f, torqFric %.3f,%.3f,%.3f, angM %.3f,%.3f,%.3f",
      torque[0],torque[1],torque[2], torqueFriction[0],torqueFriction[1],torqueFriction[2], _angMomentum[0],_angMomentum[1],_angMomentum[2]);
    */
    ApplyForces(deltaT,force,torque,friction,torqueFriction);


    bool stopCondition=false;
    if( _pilotBrake && _landContact && !_waterContact && !_objectContact )
    {
      // apply static friction
      float maxSpeed=Square(0.7f);
      if( !Driver() ) maxSpeed=Square(1.2f);
      if( FutureVisualState()._speed.SquareSize()<maxSpeed && _angVelocity.SquareSize()<maxSpeed*0.3f )
        stopCondition=true;
    }
    if( stopCondition) StopDetected();
    else OnMoved();

    // simulate track drawing   
    if( EnableVisualEffects(prec,300) )
    {
      // Update the track - either prolong or terminate
      _track.Update(RenderVisualStateRaw(), deltaT, !_landContact);  // based on the visible state

      if( _landContact )
      {
        Vector3 lcPos = _track.RightPos(); // consider only back pos
        Vector3 lPos = lcPos + Vector3(0, 0.1f, 0) + 0.2f * FutureVisualState().ModelSpeed();
        Vector3 cSpeed = 0.5f * FutureVisualState()._speed;
        float dSoft=floatMax(dust,0.0025f);
        float density=speedSize*(1.0f/10)*dSoft;
        saturate(density,0,1.0);
        float dustColor=dSoft*8;
        saturate(dustColor,0,1);

        if (density > 0.2f)
        {
          float dustValues[] = {density, dustColor, lPos.X(), lPos.Y(), lPos.Z(),
            cSpeed.X(), cSpeed.Y(), cSpeed.Z()};
          _rightDust.Simulate(deltaT, prec, dustValues, lenof(dustValues));
        }
/*
        Color color=Color(0.51,0.46,0.33)*dustColor+Color(0.5,0.5,0.5)*(1-dustColor);
        //color.SetA(0.3);
        _rightDust.SetColor(color);
        _rightDust.Simulate(lPos+_speed*0.2,_speed*0.5,density,deltaT);
*/
      }
      SimulateExhaust(deltaT,prec);
    }

    StabilizeTurrets(FutureVisualState().Transform().Orientation(), moveTrans.Orientation());
    Move(moveTrans);
    DirectionWorldToModel(FutureVisualState()._modelSpeed, FutureVisualState()._speed);
  }

  // slow steering auto-centering
  static float centerSpeedFast = 0.025f;
  static float centerSpeedSlow = 0.00f;
  static float centerAngleFast = 0.025f;
  static float centerAngleSlow = 0.10f;
  float centerSpeed = Interpolativ(fabs(FutureVisualState()._turnBySpeed),centerAngleFast,centerAngleSlow,centerSpeedFast,centerSpeedSlow);
  FutureVisualState()._turnBySpeed = TowardZero(FutureVisualState()._turnBySpeed,deltaT*centerSpeed);
}

const float ALLOW_EJECT_AFTER_GETIN = 5;
void Motorcycle::GetInDriver( Person *driver, bool sound, bool isFastMoveIn)
{
  _allowEjectAfter = Glob.time + ALLOW_EJECT_AFTER_GETIN;
  base::GetInDriver( driver, sound, isFastMoveIn);
}
void Motorcycle::GetInCargo( Person *driver, bool sound, int posIndex, bool isFastMoveIn)
{
  _allowEjectAfter = Glob.time + ALLOW_EJECT_AFTER_GETIN;
  base::GetInCargo( driver, sound, posIndex, isFastMoveIn);
}

bool Motorcycle::IsStopped() const
{
  /*
  if (!base::IsStopped()) return false;
  if (!Driver()) return true;
  return FutureVisualState()._support<0.05f;
  */
  return base::IsStopped();
}

static float MotoTurnCursorSideCoef = 1.6f;

void Motorcycle::StabilizeTurrets(Matrix3Val oldTrans, Matrix3Val newTrans)
{
  if (fabs(FutureVisualState()._turnBySpeed)>0.01f)
  {
    Vector3 turnDir = Matrix3(MRotationY,-FutureVisualState()._turnBySpeed*MotoTurnCursorSideCoef).Direction();
    Vector3 turnDirNew = newTrans.InverseRotation()*oldTrans*turnDir;
    float newValue = atan2(turnDirNew.X(),turnDirNew.Z())/MotoTurnCursorSideCoef;
    // never allow catching up to actually turn the steering more
    // without a check this would happen when reversing
    if (fabs(newValue)<fabs(FutureVisualState()._turnBySpeed)) FutureVisualState()._turnBySpeed = newValue;
  }

  base::StabilizeTurrets(oldTrans, newTrans);
}

Pair<Vector3,float> Motorcycle::ShowDrivingCursor(CameraType camType, Person *person) const
{
  if (person!=Driver()) return MakePair(VZero,_cursorY);
  float turnEff = floatMinMax(FutureVisualState()._turnBySpeed, -1.0f, +1.0f);
  Vector3 dir = Matrix3(MRotationY,-turnEff*MotoTurnCursorSideCoef).Direction();
  float dimByAge = floatMinMax((10-Glob.time.Diff(_turnSpeedWantedLastActive))*0.3f,0,1);
  float dimByState = floatMinMax(fabs(dir.X())*100,0,0.5f);
  float dim = floatMax(dimByAge,dimByState);
  return MakePair(FutureVisualState().DirectionModelToWorld(dir*dim),_cursorY);
}


void Motorcycle::Simulate( float deltaT, SimulationImportance prec )
{
  float deltaTorg = deltaT;
  if (!GetStopped())
  {
    static float maxStepNear = 1.0f/80;
    static float maxStepFar = 1.0f/30;
    float maxStep = prec<=SimulateVisibleNear ? maxStepNear : maxStepFar;
    while (deltaT>maxStep)
    {
      SimulateOneIter(maxStep,prec);
      if (ToDelete()) return;
      deltaT -= maxStep;
    }
  }
  SimulateOneIter(deltaT,prec);
  SimulatePost(deltaTorg, prec);
}

void Motorcycle::Eject(AIBrain *unit, Vector3Val diff)
{
  base::Eject(unit, diff);
}

void Motorcycle::FakePilot( float deltaT )
{
  _turnIncreaseSpeed=1;
  _turnDecreaseSpeed=1;
}

void Motorcycle::SuspendedPilot(AIBrain *unit, float deltaT )
{
  _pilotBrake = true;
  _thrustWanted = 0;
  _turnWanted = 0;
  _turnSpeedWanted = 0;
  _turnSpeedWantedLastActive = TIME_MIN;
}

#if _ENABLE_CHEATS
#include "dikCodes.h"
#endif

void Motorcycle::KeyboardPilot(AIBrain *unit, float deltaT )
{
  if (!unit->GetPerson() || unit->GetPerson()->KeyboardUsedInFrame()) return;
#if _VBS2 // convoy trainer, indicators
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
    // part of aimY will be used to move the cursor, the rest is to move the head

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
  float forward=(GInput.GetAction(UACarForward)-GInput.GetAction(UACarBack))*0.75;
  forward+=GInput.GetActionCarFastForward();
  forward+=GInput.GetAction(UACarSlowForward)*0.33;
  _thrustWanted=forward;

  float turnEff = floatMinMax(FutureVisualState()._turn + FutureVisualState()._turnBySpeed, -1, +1);
  float maxThrust = 1 - fabs(turnEff) * 0.7;
  saturate(_thrustWanted,-maxThrust,+maxThrust);
  
  {
    // aim and turn with aiming actions
    LookAround(aimX, aimY, deltaT, &Type()->_viewPilot, unit, false, true);
    // turn with arrows
    _turnWanted = (GInput.GetAction(UACarRight)-GInput.GetAction(UACarLeft));

    static float wheelFactor = 0.004;
    float turnSpeedWanted = GInput.GetAction(UACarWheelRight)-GInput.GetAction(UACarWheelLeft);
    if (fabs(turnSpeedWanted)>0.01f) _turnSpeedWantedLastActive = Glob.time;
    _turnSpeedWanted = turnSpeedWanted*wheelFactor;

#if _ENABLE_CHEATS
    if (GInput.keyPressed[DIK_LCONTROL] && GInput.keyPressed[DIK_X])
    {
      wheelFactor += 0.00025;
      DIAG_MESSAGE(1000, Format("Wheel factor: %.5f", wheelFactor));
    }

    if (GInput.keyPressed[DIK_LCONTROL] && GInput.keyPressed[DIK_C])
    {
      wheelFactor -= 0.00025;
      DIAG_MESSAGE(1000, Format("Wheel factor: %.5f", wheelFactor));
    }
#endif

    float asz = fabs(FutureVisualState().ModelSpeed().Z());
    if(_turnWanted < 0)
      _turnWanted = -Type()->_inputTurnCurve.GetInterpolated(asz,-_turnWanted );
    else
      _turnWanted = Type()->_inputTurnCurve.GetInterpolated(asz,_turnWanted );

    float slowTurn = 1-asz*(1.0/15);
    saturateMax(slowTurn,0);

    _turnIncreaseSpeed = (slowTurn*1.5)+0.4;
    _turnDecreaseSpeed = ((1-slowTurn)*2.0)+1.0;
  }


  if( fabs(_thrustWanted)>0.05 && !_waterDrown) CancelStop(),EngineOn();
  
  if( fabs(FutureVisualState()._turn-_turnWanted) + fabs(FutureVisualState()._turn) + fabs(_turnSpeedWanted) + fabs(FutureVisualState()._turnBySpeed) > 0.05 )
    CancelStop();

  if (fabs(_thrustWanted)<0.2 && fabs(FutureVisualState()._modelSpeed.Z())<4.0 || _thrustWanted*FutureVisualState()._modelSpeed.Z()<0)
  {
    _pilotBrake=true;
    if( fabs(FutureVisualState()._modelSpeed.Z())>1 ) _thrustWanted=0;
  }
  else
    _pilotBrake=false;
}

RString Motorcycle::DiagText() const
{
  char buf[256];
  sprintf(buf," %s %.2f,(%.2f)", _pilotBrake ? "B" : "E", _thrustWanted, _turnWanted);
  sprintf(buf+strlen(buf)," RF %.1f,%.1f", _reverseTimeLeft,_forwardTimeLeft);
  return base::DiagText()+(RString)buf;
}

#define DIAG_SPEED 0

#if _ENABLE_AI
void Motorcycle::AIPilot(AIBrain *unit, float deltaT )
{
  _turnIncreaseSpeed=1;
  _turnDecreaseSpeed=1;

  // TODO: limit AIPilot simulation rate (10 ps)
  Assert( unit );

  // move to given point
  // check goto/fire at command status

  Vector3Val speed=FutureVisualState().ModelSpeed();
  
  SteerInfo info;
  info.headChange=0;
  info.speedWanted=0;
  info.turnPredict=0;
  
  if( unit->GetState()==AIBrain::Stopping )
  {
    // special handling of stop state
    if (fabs(speed[2])<0.1 && FutureVisualState()._support<0.05f)
    {
      unit->OnStepCompleted();
    }
    info.speedWanted=0;
  }
  else if( unit->GetState()==AIBrain::Stopped )
    info.speedWanted=0;
  else
    LeaderPilot(info);

  float aHC=fabs(info.headChange);
  
  bool disableForward=false;
  if( FutureVisualState().ModelSpeed().Z()<10 )
  {
    if( aHC<(H_PI*6/16) )
    {
      if( _reverseTimeLeft>0 )
      {
        CreateFreshPlan();
        _forwardTimeLeft=2;
      }
      _reverseTimeLeft=0;
    }
    else if( aHC>(H_PI*8/16) )
      _reverseTimeLeft=2;

    CollisionBuffer retVal;
    GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(),retVal,Landscape::FilterIgnoreOne(this),FutureVisualState().Position(),FutureVisualState().Position()+CollisionSize()*1.5*FutureVisualState().Direction(),0/*1.5*/, ObjIntersectGeom);
    disableForward=( retVal.Size()>0 );
  }

  bool reverse=false;
  if( _forwardTimeLeft>0 )
    _forwardTimeLeft-=deltaT;
  else if( _reverseTimeLeft>0 || disableForward )
  {
    // check if back is free
    CollisionBuffer retVal;
    GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(),retVal,Landscape::FilterIgnoreOne(this),FutureVisualState().Position(),FutureVisualState().Position()-CollisionSize()*2*FutureVisualState().Direction(),0/*1.5*/, ObjIntersectGeom);
    if( retVal.Size()>0 )
    {
      _reverseTimeLeft=0;
      _forwardTimeLeft=1;
    }
    else
    {
      _reverseTimeLeft-=deltaT;
      if( _reverseTimeLeft<=0 )
      {
        CreateFreshPlan();
        _forwardTimeLeft=1;
      }
      reverse=true;
      if( info.speedWanted>0 )
      {
        info.speedWanted=-info.speedWanted;
        info.headChange=AngleDifference(H_PI,info.headChange);
      }
    }
  }

#if DIAG_SPEED
  if( this==GWorld->CameraOn() )
    LogF("Pilot speed %.1f",info.speedWanted);
#endif

  AvoidCollision(deltaT,info.speedWanted,info.headChange);

#if DIAG_SPEED
  if( this==GWorld->CameraOn() )
    LogF("Avoid %.1f",speedWanted);
#endif

  float curHeading=atan2(FutureVisualState().Direction()[0],FutureVisualState().Direction()[2]);
  float wantedHeading=curHeading+info.headChange;

  // estimate inertial orientation change
  float estT = 0.2;
  Matrix3Val orientation=FutureVisualState().Orientation();
  Matrix3Val derOrientation=_angVelocity.Tilda()*orientation;
  Matrix3Val estOrientation=orientation+derOrientation*estT;
  Vector3Val estDirection=estOrientation.Direction();
  float estHeading=atan2(estDirection[0],estDirection[2]);

  info.headChange=AngleDifference(wantedHeading,estHeading);
  
  {
    float aTP=fabs(info.turnPredict);
    if( aTP>H_PI/64 )
    {
      // limit speed only when turning significantly
      //float maxSpeed=GetType()->GetMaxSpeedMs();
      float maxSpeed=GetType()->GetMaxSpeedMs();
      // even with very slow or very fast car use some normal brakes
      // note: actualy in-turn speed is not much dependent on max speed
      // but we can assume faster vehicles have better turning radius
      saturate(maxSpeed,50/3.6,200/3.6);
      float limitSpeed=Interpolativ(aTP,H_PI/64,H_PI/4,maxSpeed,5);

#if _ENABLE_CHEATS
      if( CHECK_DIAG(DEPath) && this==GWorld->CameraOn() )
      {
        GlobalShowMessage(200,"Turn limit %.1f (%.3f, turn %.3f)",limitSpeed*3.6,info.headChange,info.turnPredict);
        //LogF("Turn limit %.1f (%.3f, turn %.3f)",limitSpeed,headChange,turnPredict);
      }
#endif
      saturate(info.speedWanted,-limitSpeed,+limitSpeed);
    }
  }

#if DIAG_SPEED
  if( this==GWorld->CameraOn() )
    LogF("Turn %.1f",info.speedWanted);
#endif
  
  if( fabs(info.speedWanted)>0.5 && !_waterDrown) EngineOn();

  // some thrust is needed to keep speed
  float isSlow=1-fabs(speed.Z())*(1.0/17);
  float maxTurn = 1-fabs(speed.Z())*(1.0/60);
  saturate(isSlow,0.2,1);
  saturate(maxTurn,0.05,1);

  float isLevel=1-fabs(FutureVisualState().Direction()[1]*(1.0/0.6));
  saturate(isLevel,0.2,1);
  saturateMax(isSlow,isLevel); // change thrust slowly on steep surfaces


  if( fabs(info.speedWanted)<0.1 && speed.SquareSize()<1 )
    _thrustWanted=0;
  else
  {
    Vector3 relAccel=DirectionWorldToModel(FutureVisualState()._acceleration);
    // when moving very slow, we want to ignore current acceleration, as it oscillates a lot
    float isVerySlow = floatMinMax(1-fabs(speed.Z())*(1.0f/2),0,1);
    float changeAccel = (info.speedWanted-speed.Z())*(1/0.5f)-relAccel.Z()*(1-isVerySlow);
    changeAccel *= isSlow;
    float thrustOld = FutureVisualState()._thrust;
    float thrust = thrustOld+changeAccel*0.33f;
    Limit(thrust,-1,1);
    _thrustWanted=thrust;
  }

  _turnWanted=info.headChange*0.5f;
  if( reverse ) _turnWanted=-_turnWanted;

  // limit turn based on speed (to avoid slips)
  Limit(_thrustWanted,-1,1);
  Limit(_turnWanted,-maxTurn,+maxTurn);


  if( fabs(speed[2])<5 )
  { // may be switching from/to reverse
    if (fabs(_turnWanted-FutureVisualState()._turn)>0.6 || _turnWanted*FutureVisualState()._turn<0 && fabs(_turnWanted-FutureVisualState()._turn)>0.2)
      saturate(info.speedWanted,-0.6,+0.6);
  }

  _pilotBrake=fabs(info.speedWanted)<0.1 || fabs(info.speedWanted)<fabs(speed[2])-5;

  /*
  if( _pilotBrake )
    LogF("speed %.1f->%.1f",speed[2],speedWanted);
  */
}
#endif // _ENABLE_AI

const float RoadFaster=2.0;

static const float ObjPenalty1[]={1.0,1.05,1.1,1.4};

static const float ObjPenalty2[]={1.0,1.20,1.5,2.0};
static const float ObjRoadPenalty2[]={1.0,1.10,1.2,1.4};

float MotorcycleType::GetFieldCost(const GeographyInfo &info, CombatMode mode) const
{
  // road fields are expected to be faster
  // fields with objects will be passed through slower
  int nObj=info.u.howManyObjects;
  Assert( nObj<=3 );
  if (info.u.road)
    return (1.0/RoadFaster)*ObjRoadPenalty2[nObj];
  else
    return ObjPenalty2[nObj];
}

float MotorcycleType::GetBaseCost(const GeographyInfo &geogr, bool operative, bool includeGradient) const
{
  float cost=GetMinCost()*RoadFaster;
  // avoid water
  if (geogr.u.minWaterDepth>0 && !geogr.u.road) return 1e30; // no movement in water
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
    //static const float gradPenalty[6]={1.0,1.05,1.1,1.5,2.0,3.0};
    static const float gradPenalty[6]={1.0,1.02,1.05,1.1,2.0,3.0};
    cost *= gradPenalty[grad];
  }
  return cost;
}

float MotorcycleType::GetGradientPenalty(float gradient) const
{
  // TODO: continuous function, sign of gradient
  gradient = fabs(gradient);

  if (gradient >= 0.60f) return 1e30f; // level 6
  else if (gradient >= 0.40f) return 3.0f; // level 5
  else if (gradient >= 0.25f) return 2.0f; // level 4
  else if (gradient >= 0.15f) return 1.1f; // level 3
  else if (gradient >= 0.10f) return 1.05f; // level 2
  else if (gradient >= 0.05f) return 1.02f; // level 1
  return 1.0f; // level 0
}

float MotorcycleType::GetCostTurn(int difDir, int &debet, bool forceFullTurn) const
{ // in sec
  if( difDir==0 ) return 0;
  // convert argument into -8..7 range (old direction representation)
  float aDir=fabs(difDir)*(float(15)/255);
  float aDir2=aDir*aDir;
  float cost=aDir*0.15+aDir2*0.02+aDir2*aDir*0.05;
  if( difDir<0 ) return cost*0.8;
  return cost;
}

float MotorcycleType::GetPathCost(const GeographyInfo &geogr, float dist, CombatMode mode) const
{
  // cost based only on distance
  float cost=GetMinCost();
  // avoid any water
  // penalty for objects
  int nObj=geogr.u.howManyObjects;
  Assert( nObj<=3 );
  cost *= ObjRoadPenalty2[nObj];
  return cost*dist;
}

bool Motorcycle::ProcessFireWeapon(const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine, EntityAI *target,
  const RemoteFireWeaponInfo *remoteInfo, RemoteFireWeaponInfo *localInfo, bool forceLock)
{
  const AmmoType *ammo = ptr.GetAmmoType();
  if (!ammo) return false;
  if (localInfo)
  {
    localInfo->_position = FutureVisualState().Position();
    localInfo->_visible=floatMax(ammo->visibleFire,ammo->audibleFire);
  }
  switch (ammo->_simulation)
  {
  case AmmoShotBullet:
  case AmmoShotSpread:
    {
      Matrix4Val shootTrans = context._turretType ? GunTurretTransform(FutureVisualState(), *context._turretType) : M4Identity;
      if (remoteInfo)
      {
        return FireMGun(context, ptr, context._turretType ? context._turretType->GetTurretPos(shootTrans) : VZero,
          remoteInfo->_direction, target, false, true);
      }
      else
      {
        FutureVisualState().DirectionModelToWorld(localInfo->_direction, context._turretType ? context._turretType->GetTurretDir(shootTrans) : VForward);
        return FireMGun(context, ptr, context._turretType ? context._turretType->GetTurretPos(shootTrans) : VZero,
          localInfo->_direction, target, false, false);
      }
    }
  case AmmoNone:
    break;
  default:
    Fail("Unknown ammo used.");
    break;
  }
  return false;
}

bool Motorcycle::FireWeapon(const TurretContextV &context, const WeaponModePtr &ptr, TargetType *target, bool forceLock)
{
  if (GetNetworkManager().IsControlsPaused()) return false;

  if (!ptr.IsValid()) return false;

  if (context._turretType)
  {
    if (!GetWeaponLoaded(*context._weapons, ptr))
      return false;
    if (!IsFireEnabled(context))
      return false;

    const Magazine *magazine = ptr.GetMagazine();
    if (!magazine)
      return false;

    RemoteFireWeaponInfo local;
    bool fired = ProcessFireWeapon(context, ptr, magazine, target, NULL, &local, forceLock);
    if( fired )
    {
      base::PostFireWeapon(context, ptr, target, local);
      return true;
    }
    return false;
  }
  else
  {
    // weapon is horn
    if( !IsFireEnabled(context) )
      return false;
    if( !_hornSound )
    {
      const MuzzleType *muzzle = ptr.GetMuzzleType();
      if (muzzle)
      {
        AbstractWave *sound=GSoundScene->OpenAndPlayOnce(muzzle->_sound.name, this, false, FutureVisualState().Position(), FutureVisualState().Speed(),
          muzzle->_sound.vol, muzzle->_sound.freq, muzzle->_sound.distance);
        if( sound )
        {
          GSoundScene->SimulateSpeedOfSound(sound);
          GSoundScene->AddSound(sound);
          _hornSound=sound;
        }
      }
      return true;
    }
    return false;
  }
}

bool Motorcycle::FireWeaponEffects(
  const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine,EntityAI *target, const RemoteFireWeaponInfo *remoteInfo
)
{
  if (context._turret)
  {
    const AmmoType *ammo =  ValidateFireWeaponWithAmmo(ptr, magazine);
    if (!ammo) return false;
    
    if (EnableVisualEffects(SimulateVisibleNear)) switch (ammo->_simulation)
    {
      case AmmoShotBullet:
      case AmmoShotSpread:
        context._weapons->_mGunClouds.Start(0.1);
        {
          float duration = 0.1f;
          float intensity = 0.012f;
          const WeaponType *weaponType = ptr.GetWeaponType();
          if (weaponType)
          {
            duration = weaponType->_fireLightDuration;
            intensity = weaponType->_fireLightIntensity;
          }
          if (duration > 0 && intensity > 0) context._weapons->_mGunFire.Start(duration, intensity, true);
        }
        context._weapons->_mGunFireUntilFrame = Glob.frameID+1;
        context._weapons->_mGunFireTime = Glob.time;
        int newPhase;
        while ((newPhase = toIntFloor(GRandGen.RandomValue() * 3)) == context._weapons->_mGunFirePhase);
        context._weapons->_mGunFirePhase = newPhase;
        break;
      case AmmoNone:
        break;
    }
  }
  return base::FireWeaponEffects(context, ptr, magazine,target,remoteInfo);
}

Vector3 Motorcycle::GetCameraDirection( CameraType camType ) const
{
#if 0 //Automatic look to the turn direction disabled
  if (!QIsManual()) return base::GetCameraDirection(camType);
  // in world coordinates
  // aside - based on _turnWanted
  if (GWorld->LookAroundEnabled()) return base::GetCameraDirection(camType);
  
  float turnEff = floatMinMax(FutureVisualState()._turn + FutureVisualState()._turnBySpeed,-1,+1);
  Matrix3 rotY(MRotationY, -turnEff * 0.5f);
  return rotY*base::GetCameraDirection(camType);
#else
  return base::GetCameraDirection(camType);
#endif
}


bool Motorcycle::AimWeapon(const TurretContextEx &context, const WeaponModePtr &ptr, Vector3Par direction )
{
  if (!context._turret) return true;

  WeaponModePtr selected = context._weapons->FindValidWeapon(ptr);
  if (!selected.IsValid()) return false;

  context._weapons->SelectWeapon(this, selected);
  // move turret/gun accordingly to direction
  if (context._turret->AimWorld(*context._turretVisualState, *context._turretType, direction))
  {
    CancelStop();
  }
  return true;
}

bool Motorcycle::AimWeaponTgt(const TurretContextEx &context, const WeaponModePtr &ptr, Target *target)
{
  if (!context._turret) return true;
  
  if (!CheckTargetKnown(context,target)) return false;

  WeaponModePtr selected = context._weapons->FindValidWeapon(ptr);
  if (!selected.IsValid()) return false;

  const Magazine *magazine = selected.GetMagazine();
  const MagazineType *aInfo = magazine ? magazine->Type() : NULL;
  const AmmoType *ammo = aInfo ? aInfo->_ammo : NULL;
  Vector3 weaponPos = context._turretType->GetTurretAimCenter(Type());
  Vector3 tgtPos=target->LandAimingPosition(context._gunner->Brain(),ammo);
  // predict his and my movement
  float dist2=tgtPos.Distance2(FutureVisualState().Position());
  float time2=aInfo ? dist2*Square(aInfo->_invInitSpeed*1.2) : 0;

  //const float predTime=0.25;
  float time=sqrt(time2);
  const float minPredTime=0.25;
  float predTime=floatMax(time+0.1,minPredTime);
  Vector3 myPos=FutureVisualState().PositionModelToWorld(weaponPos);
  //tgtPos+=target->ObjectSpeed()*predTime;
  //myPos+=Speed()*minPredTime;
  float fall=0.5*G_CONST*time2;
  // calculate ballistics
  tgtPos[1]+=fall; // consider ballistics
  if( aInfo )
  {
    Vector3 speedEst=target->GetSpeed(context._gunner->Brain())-FutureVisualState().Speed();
    const float maxSpeedEst=aInfo->_maxLeadSpeed;
    if( speedEst.SquareSize()>Square(maxSpeedEst) ) speedEst=speedEst.Normalized()*maxSpeedEst;
    tgtPos+=speedEst*predTime;
  }
  return AimWeapon(context, selected, tgtPos-myPos);
}

Vector3 Motorcycle::GetWeaponDirectionWanted(const TurretContextEx &context, const WeaponModePtr &ptr) const
{
  if (!context._turret) return FutureVisualState().Direction();

  Matrix3Val aim = context._turret->GetAimWantedWorld(context._parentTrans);
  Vector3 dir = context._turretType->GetTurretDir(aim);
  return dir;
}

Vector3 Motorcycle::GetWeaponDirection(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const
{
  if (!context._turretType) return vs.Direction();

  Vector3 dir = context._turretType->GetTurretDir(GunTurretTransform(vs, *context._turretType));
  return vs.Transform().Rotate(dir);
}

Vector3 Motorcycle::GetWeaponCenter(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const
{
  if (!context._turret) return VZero;
  return context._turret->GetCenter(Type(), *context._turretType);
}

float Motorcycle::DriverAnimSpeed() const
{
  if (Type()->_isBicycle)
    return floatMax(FutureVisualState()._thrust,0);
  return 1;
}
float Motorcycle::CargoAnimSpeed(int position) const
{
  return 1;
}

float Motorcycle::GetEngineVol( float &freq ) const
{
  freq=(_randomizer*0.05+0.95)*FutureVisualState()._rpm*1.2;
  if (Type()->_isBicycle)
    return floatMax(FutureVisualState()._thrust,0);
  else
    return fabs(FutureVisualState()._thrust)*0.5+0.5;
}

float Motorcycle::GetEnvironVol( float &freq ) const
{
  freq=1;
  return FutureVisualState()._speed.SquareSize()/Square(Type()->GetMaxSpeedMs());
}


bool Motorcycle::IsPossibleToGetIn() const
{
  float wheelHit=floatMax(GetHit(Type()->_wheelFHit),GetHit(Type()->_wheelBHit));
  if( wheelHit>=0.9 ) return false;
  if( GetHit(Type()->_engineHit)>=0.9 ) return false;
  return base::IsPossibleToGetIn();
}

bool Motorcycle::IsAbleToMove() const
{
  float wheelHit=floatMax(GetHit(Type()->_wheelFHit),GetHit(Type()->_wheelBHit));
  if( wheelHit>=0.9 ) return false;
  if( GetHit(Type()->_engineHit)>=0.9 ) return false;
  return base::IsAbleToMove();
}

bool Motorcycle::IsCautious() const
{
  AIBrain *unit = PilotUnit();
  if (!unit) return false;
  CombatMode mode = unit->GetCombatModeLowLevel();
  return mode == CMStealth || mode == CMCombat; // in AWARE state - lights is on and move on road 
}

void Motorcycle::Sound( bool inside, float deltaT )
{
  const VisualState &vs = RenderVisualState();
  if( _gearSound )
  {
    float obstruction,occlusion;
    GWorld->CheckSoundObstruction(this,true,obstruction,occlusion);
    float gearVol=Type()->_gearSound.vol;
    _gearSound->SetVolume(gearVol); // volume, frequency
    _gearSound->SetObstruction(obstruction,occlusion,deltaT);
    _gearSound->SetPosition(vs.Position(),vs.Speed());
    _gearSound->Set3D(!inside);
  }
  if( _hornSound )
  {
    float obstruction,occlusion;
    GWorld->CheckSoundObstruction(this,false,obstruction,occlusion);
    _hornSound->SetPosition(vs.Position(),vs.Speed());
    _hornSound->SetObstruction(obstruction,occlusion,deltaT);
    _hornSound->Set3D(!inside);
  }

  base::Sound(inside,deltaT);
}

void Motorcycle::UnloadSound()
{
  base::UnloadSound();
}

Matrix4 Motorcycle::InsideCamera( CameraType camType ) const
{
  Matrix4 transf;
  if (!GetProxyCamera(transf, camType))
  {
    Vector3 pos=Type()->_pilotPos;
    transf.SetTranslation(pos);
  }

  Vector3 dir = transf.Direction();
  //if (!unit->LookAroundEnabled())
  {
    //_drivingWheelTurn used, as it is averaged
    const VisualState &vs = RenderVisualState();
    float dispWheel = floatMinMax(vs._turnBySpeed,-1,+1);

    Matrix3 rotY(MRotationY,-dispWheel*0.5f);
    dir = rotY.Direction();
  }
  transf.SetDirectionAndUp(dir,transf.DirectionUp());

  return transf;
}

Vector3 Motorcycle::ExternalCameraPosition( CameraType camType ) const
{
  if (camType == CamGroup)
    return Type()->_groupCameraPosition;
  else
    return Type()->_extCameraPosition;
}

bool Motorcycle::HasFlares( CameraType camType ) const
{
  if( camType==CamGunner || camType==CamInternal ) return false;
  return base::HasFlares(camType);
}

#include "engine.hpp"

void Motorcycle::Draw(int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
  const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
  , int drawListId
#endif
  )
{
  base::Draw(cb,level, clipFlags, dp, ip, dist2, pos, oi
#if _HELISIM_RENDER_TO_TEXTURE
    , drawListId
#endif
    );

  Type()->_plateInfos.Draw(cb,level,clipFlags,pos,_plateNumber);
}

void Motorcycle::SimulateHUD(CameraType camType,float deltaT)
{
}

#if _HELISIM_PRECISE_INTERACTION
void Motorcycle::GetDrawActions3D(UIActions &actions, AIBrain *unit)
{
  base::GetDrawActions3D(actions, unit);
}
#endif

bool Motorcycle::IsContinuous( CameraType camType ) const
{
  return false;
}

bool Motorcycle::IsGunner( CameraType camType ) const
{
  return camType==CamGunner || camType==CamInternal || camType==CamExternal;
}

#define SERIAL_DEF_VISUALSTATE(name, value) CHECK(ar.Serialize(#name, FutureVisualState()._##name, 1, value))

LSError Motorcycle::Serialize(ParamArchive &ar)
{
  SERIAL_BASE;
  
  if (!IS_UNIT_STATUS_BRANCH(ar.GetArVersion()))
  {
    SERIAL_DEF(thrustWanted, 0);
    SERIAL_DEF_VISUALSTATE(thrust, 0);
    SERIAL_DEF(turnWanted, 0);
    SERIAL_DEF_VISUALSTATE(turn, 0);
    SERIAL_DEF_VISUALSTATE(turnBySpeed, 0);
    SERIAL_DEF(turnSpeedWanted, 0);
  }
  return LSOK;
}

#undef SERIAL_DEF_VISUALSTATE

#define MOTORCYCLE_MSG_LIST(XX) \
  XX(UpdateGeneric, UpdateMotorcycle) \
  XX(UpdatePosition, UpdatePositionMotorcycle)

DEFINE_NETWORK_OBJECT(Motorcycle, base, MOTORCYCLE_MSG_LIST)

#define UPDATE_MOTORCYCLE_MSG(MessageName, XX) \
  XX(MessageName, RString, plateNumber, NDTString, RString, NCTNone, DEFVALUE(RString, "XXXXXXXX"), DOC_MSG("Plate number"), TRANSF, ET_NOT_EQUAL, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, thrustWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted thrust"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, turnWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted turning angle"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR)

DECLARE_NET_INDICES_EX_ERR(UpdateMotorcycle, UpdateTankOrCar, UPDATE_MOTORCYCLE_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdateMotorcycle, UpdateTankOrCar, UPDATE_MOTORCYCLE_MSG, NoErrorInitialFunc)

#define UPDATE_POSITION_MOTORCYCLE_MSG(MessageName, XX) \

DECLARE_NET_INDICES_EX_ERR(UpdatePositionMotorcycle, UpdatePositionVehicle, UPDATE_POSITION_MOTORCYCLE_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdatePositionMotorcycle, UpdatePositionVehicle, UPDATE_POSITION_MOTORCYCLE_MSG, NoErrorInitialFunc)

NetworkMessageFormat &Motorcycle::CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format)
{
  switch (cls)
  {
  case NMCUpdateGeneric:
    base::CreateFormat(cls, format);
    UPDATE_MOTORCYCLE_MSG(UpdateMotorcycle, MSG_FORMAT_ERR)
    break;
  case NMCUpdatePosition:
    base::CreateFormat(cls, format);
    UPDATE_POSITION_MOTORCYCLE_MSG(UpdatePositionMotorcycle, MSG_FORMAT_ERR)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError Motorcycle::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    TMCHECK(base::TransferMsg(ctx))
    {
      PREPARE_TRANSFER(UpdateMotorcycle)

      TRANSF(plateNumber)
//      ITRANSF(thrust)
      TRANSF(thrustWanted)
//      ITRANSF(turn)
      TRANSF(turnWanted)
    }
    break;
  case NMCUpdatePosition:
    {
      PREPARE_TRANSFER(UpdatePositionMotorcycle)

      Matrix3 oldTrans = FutureVisualState().Orientation();
      TMCHECK(base::TransferMsg(ctx))
      StabilizeTurrets(oldTrans, FutureVisualState().Orientation());
    }
    break;
  default:
    return base::TransferMsg(ctx);
  }
  return TMOK;
}

float Motorcycle::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = 0;
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    error += base::CalculateError(ctx);
    {
      PREPARE_TRANSFER(UpdateMotorcycle)

      ICALCERR_NEQSTR(plateNumber, ERR_COEF_VALUE_MAJOR)
//      ICALCERR_ABSDIF(float, thrust, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, thrustWanted, ERR_COEF_VALUE_MAJOR)
//      ICALCERR_ABSDIF(float, turn, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, turnWanted, ERR_COEF_VALUE_MAJOR)
    }
    break;
  case NMCUpdatePosition:
    {
      error +=  base::CalculateError(ctx);
/*
      PREPARE_TRANSFER(UpdatePositionMotorcycle)
*/
    }
    break;
  default:
    error += base::CalculateError(ctx);
    break;
  }
  return error;
}

float Motorcycle::NeedsRefuel() const
{
  if (Type()->_isBicycle)
    return 0;
  
  return base::NeedsRefuel();
}

void Motorcycle::CamIntExtSwitched()
{
  Person *player=GWorld->PlayerOn();
  AIBrain *unit=( player ? player->CommanderUnit() : NULL );
  if( !unit || unit->GetVehicle()!=this ) return;
  player->ResetXRotWanted();
}
