// Poseidon - vehicle class
// (C) 1997, Suma
#include "wpch.hpp"
#include "keyInput.hpp"
#include "scene.hpp"
#include "world.hpp"
#include "landscape.hpp"
#include "AI/ai.hpp"
#include <El/Common/randomGen.hpp>
#include <El/Common/perfLog.hpp>
#include <El/Common/perfProf.hpp>
#include "diagModes.hpp"
#include "dikCodes.h"
#include "fileLocator.hpp"

#include "car.hpp"
#include "Network/network.hpp"
#include "Shape/specLods.hpp"

#include "UI/uiActions.hpp"
#include "stringtableExt.hpp"
#include "camera.hpp"
#include "mbcs.hpp"

#include "Marina/carCollisions.hpp" // new physics
#include "paramArchiveExt.hpp"

#if _ENABLE_CHEATS && _PROFILE
# pragma optimize("",off)
#endif


#if _ENABLE_CHEATS
  #if !_RELEASE
    #define ARROWS 0
  #else
    #define ARROWS 0
  #endif
#endif


CarType::CarType(ParamEntryPar param)
:base(param)
{
  _glassRHit = -1;
  _glassLHit = -1;
  _bodyHit = -1;
  _fuelHit = -1;
  _wheelLFHit = -1;
  _wheelRFHit = -1;
  _wheelLF2Hit = -1;
  _wheelRF2Hit = -1;
  _wheelLMHit = -1;
  _wheelRMHit = -1;
  _wheelLBHit = -1;
  _wheelRBHit = -1;
}

void CarType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par,shape);

  _turnCoef=par>>"turnCoef";
  _terrainCoef=par>>"terrainCoef";
  // provide a reasonable estimation until shape is loaded and InitWheels called
  // or for models which do not have geometry configured properly and InitWheels is not called at all
  _maxTurnOneStep = toInt(_turnCoef*1.6f* OperItemGrid*(1.0f/2.5f));
  // avoid out of range values
  saturate(_maxTurnOneStep,1,127);

  float damperSize=par>>"damperSize";
  float damperForce=par>>"damperForce";  
  float damperDamping = par>>"damperDamping";

  _holdOffroadFormation = par>>"holdOffroadFormation";

  _dampers.Init(damperSize, damperForce, damperDamping);
  _waterSpeedFactor = par.ReadValue("waterSpeedFactor",0.2);

  SetHardwareAnimation(par);
}

/** wheels can be initialized after the LandContact is loaded */
void CarType::InitWheels()
{
  float cmz = _shape->CenterOfMass().Z();
  const Shape *contact = _shape->LandContactLevel();
  // note that all cars are "reversed", i.e. they are oriented towards the negative Z coordinate
  // TODO: how can we find these points (more) properly?
  float minz = contact->Min().Z();
  float maxz = contact->Max().Z();
  _turningCenterOffset = maxz - cmz;
  _invCenterToFrontWheelsLength = -1.0f / (minz - cmz);   
  _maxTurnOneStep = toInt(_turnCoef * 2.0f * _invCenterToFrontWheelsLength * OperItemGrid * (1.0f / 2.5f));
  // avoid out of range values
  saturate(_maxTurnOneStep, 1, 127);
}

AnimationSource *CarType::CreateAnimationSource(const AnimationType *type, RStringB source)
{
  if (!strcmpi(source,"wheel")) return _animSources.CreateAnimationSource(&Car::GetCtrlWheelRot);
  if (!strcmpi(source,"damper")) return _dampers.CreateAnimationSource();
  if (!strcmpi(source,"scudlaunch")) return _animSources.CreateAnimationSource(&Car::GetCtrlSCUD);
  return base::CreateAnimationSource(type,source);
}

void CarType::InitShape()
{
  // we need to clear dampers, so that they do not accumulate forever
  _dampers.Clear();
  
  const ParamEntry &par=*_par;
  base::InitShape();

  _plateInfos.Init(_shape, par >> "PlateInfos", GetFontID(Pars >> "fontPlate"));

  _glassRHit = FindHitPoint("HitRGlass");
  _glassLHit = FindHitPoint("HitLGlass");
  _bodyHit = FindHitPoint("HitBody");
  _fuelHit = FindHitPoint("HitFuel");
  _wheelLFHit = FindHitPoint("HitLFWheel");
  _wheelRFHit = FindHitPoint("HitRFWheel");
  _wheelLF2Hit = FindHitPoint("HitLF2Wheel");
  _wheelRF2Hit = FindHitPoint("HitRF2Wheel");
  _wheelLMHit = FindHitPoint("HitLMWheel");
  _wheelRMHit = FindHitPoint("HitRMWheel");
  _wheelLBHit = FindHitPoint("HitLBWheel");
  _wheelRBHit = FindHitPoint("HitRBWheel");

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

  RStringB modelName = par >> "scudModel";
  if (modelName.GetLength() > 0)
  {
    _scudModel = Shapes.New(::GetShapeName(modelName), false, false);
    if (_scudModel)
    {
      _scudModel->SetAutoCenter(false);
      _scudModel->CalculateMinMax();
    }

    modelName = par >> "scudModelFire";
    if (modelName.GetLength() > 0)
    {
      _scudModelFire = Shapes.New(::GetShapeName(modelName), false, false);
      if (_scudModelFire)
      {
        _scudModelFire->SetAutoCenter(false);
        _scudModelFire->CalculateMinMax();
      }
    }
      GetValue(_scudSoundElevate, par >> "scudSoundElevate");
      GetValue(_scudSound, par >> "scudSound");
  }
  _invCenterToFrontWheelsLength = 1.0;
}

void CarType::DeinitShape()
{
  _scudModelFire = NULL;
  _scudModel = NULL;
  // we need to clear dampers, so that they do not accumulate forever
  _dampers.Clear();
  base::DeinitShape();
}

void CarType::InitShapeLevel(int level)
{
  base::InitShapeLevel(level);

  _glassDamageHalf.InitLevel(_shape,level);
  _glassDamageFull.InitLevel(_shape,level);
  
  _plateInfos.InitLevel(_shape,level);
  
  int geomLevel = _shape->FindGeometryLevel();
  if (level == _shape->FindLandContactLevel())
  {
    _dampers.InitLandcontact(_shape,level,GetAnimations(),_animSources);
    _dampers.InitGeometry(_shape,geomLevel,GetAnimations(),_animSources);
    InitWheels();
  }
  // run separate geometry init only when landcontact is missing
  if (_shape->FindLandContactLevel()<0 && level==geomLevel)
  {
    RptF("Landcontact missing in %s:%s",cc_cast(GetName()),cc_cast(_shape->Name()));
    _dampers.InitGeometry(_shape,level,GetAnimations(),_animSources);
  }
  
  // check scud proxy
  LevelProxies &proxies=_proxies[level];
  // convert shape proxies to my proxies
  for (int i=0; i<GetProxyCountForLevel(level); i++)
  {
    const ProxyObjectTyped &proxy=GetProxyForLevel(level,i);
    // check proxy class, if it is always visible, count it
    const EntityType *type = proxy.obj->GetEntityType();
    if (type && !strcmp(type->_simName, "scud"))
      proxies._normalProxy.Add(i);
  }
  proxies._normalProxy.Compact();
}

void CarType::DeinitShapeLevel(int level)
{
  _glassDamageHalf.DeinitLevel(_shape,level);
  _glassDamageFull.DeinitLevel(_shape,level);
  _plateInfos.DeinitLevel(_shape,level);
  base::DeinitShapeLevel(level);
}


Object* CarType::CreateObject(bool unused) const
{ 
  return new Car(this, NULL, unused);
}


int GetTemplateSeed();

DEFINE_CASTING(Car)

const float CarSimulationPrecisionNormal = 1.0f/15;
const float CarSimulationPrecisionHigh = 1.0f/30;

Car::Car(const EntityAIType *name, Person *driver, bool fullCreate)
:base(name, driver, fullCreate),

//_currentVisualState(),  // called by NewVehicle()
// pilot controls
_thrustWanted(0),_thrust(0),
_turnWanted(0),_turn(0),
_turnIncreaseSpeed(1),_turnDecreaseSpeed(1),
_turnSpeedWanted(0),_turnSpeedWantedLastActive(TIME_MIN),


_reverseTimeLeft(0),
_forwardTimeLeft(0),
_reverseNeeded(false),_turningWheels(0),

_track(_shape, *Type()->_par),
_doGearSound(false),_waterDrown(false),
_scudStateDir(0),
_maxTurnReached(false)
// different variables
{
  // initial precision set to high, will be reset once settled down
  SetSimulationPrecision(CarSimulationPrecisionHigh,CarSimulationPrecisionNormal); 
  RandomizeSimulationTime();

  SetTimeOffset(1.0/15);
//  SetTimeOffset(1.0);
  
  _isStopped = true;

  FutureVisualState()._rpm = 0.0f;
  _rpmWanted = 0.0f;
  _gearBox.Load((*Type()->_par)>>"gearBox",Type()->GetMaxSpeed());
  // synchronize internal and external view

  if (Type()->_scudModel)
  {
    RString scudSmokeName = (*Type()->_par)>>"ScudEffect";
    _scudSmoke.Init(Pars >> scudSmokeName, this, EVarSet_Effect);
  }
  _scudPos = VZero;
  _scudSpeed = VZero;

  // Driving wheel
  for (int i=0; i < lenof(_drivingWheelTurnOld); i++)
    _drivingWheelTurnOld[i] = 0.0f;
}


const float MaxTurnBySpeed = 1.3333f;

CarVisualState::CarVisualState(CarType const& type)
:base(type),
_wheelPhase(0), _wheelPhaseDelta(0),
_scudState(0),
_drivingWheelTurn(0),
_turnBySpeed(0)
{
  _dampers.Init(type._dampers);
}


void CarVisualState::Interpolate(Object *obj, ObjectVisualState const& t1state, float t, ObjectVisualState& interpolatedResult) const
{
  base::Interpolate(obj, t1state, t, interpolatedResult);

  CarVisualState const& s = static_cast<CarVisualState const&>(t1state);
  CarVisualState& res = static_cast<CarVisualState&>(interpolatedResult);

  #define INTERPOLATE(x) res.x = x + t*(s.x - x)

  INTERPOLATE(_scudState);  // 0..4
  INTERPOLATE(_drivingWheelTurn);  // continuous
  INTERPOLATE(_turnBySpeed);  // -MaxTurnBySpeed..MaxTurnBySpeed
  res._wheelPhase = fastFmod(_wheelPhase + t * s._wheelPhaseDelta, 1.0f);  // mod 1: assume that `t1state` was created by Simulate() from `*this`
  _dampers.Interpolate(s._dampers, t, res._dampers);

  #undef INTERPOLATE
}


inline float Car::GetGlassBroken() const
{
  const CarType *type = Type();
  float glassDamage = GetHitCont(type->_bodyHit);
  saturateMax(glassDamage,GetTotalDamage());
  saturateMax(glassDamage,GetHitCont(type->_glassLHit));
  saturateMax(glassDamage,GetHitCont(type->_glassRHit));
  return glassDamage;
}

void Car::DamageAnimation(AnimationContext &animContext, int level, float dist2)
{
  const CarType *type = Type();
  // scan corresponding wound

  float glassDamage = GetGlassBroken();
  if (glassDamage>=0.6)
    // TODO: convert to 3-step wounds
    type->_glassDamageFull.Apply(animContext, _shape, level, 2, dist2);
  else if (glassDamage>=0.3)
    type->_glassDamageHalf.Apply(animContext, _shape, level, 1, dist2);
}

void Car::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  ShapeUsed shape = _shape->Level(level);
  if (shape.IsNull())
    return;
  if (_weaponsState.ShowFire())
  {
    Type()->_weapons._animFire.Unhide(animContext, _shape, level);
    Type()->_weapons._animFire.SetPhase(animContext, _shape, level, _weaponsState._mGunFirePhase);
  }
  else
    Type()->_weapons._animFire.Hide(animContext, _shape, level);
  DamageAnimation(animContext, level, dist2);
  base::Animate(animContext, level, setEngineStuff, parentObject, dist2);
  // assume min-max box is not changed
  animContext.ValidateMinMax();
}

void Car::Deanimate(int level, bool setEngineStuff)
{
  if (_shape->Level(level).IsNull())
    return;
  base::Deanimate(level,setEngineStuff);
  DamageDeanimation(level);
}

Vector3 Car::Friction(Vector3Par speed)
{
  Vector3 friction;
  friction.Init();
  friction[0]=speed[0]*fabs(speed[0])*10+speed[0]*20+fSign(speed[0])*10;
  friction[1]=speed[1]*fabs(speed[1])*7+speed[1]*20+fSign(speed[1])*10;
  friction[2]=speed[2]*fabs(speed[2])*9+speed[2]*110+fSign(speed[2])*10;
  return friction*GetMass()*(1.0/1700);
}

#if 0
static float LimitFriction(float friction, float speed, float deltaT, float invMass)
{
  // if speed is small, keep friction in some proportion to it
  if (friction * speed < 0.0f)
    return 0.0f;
  float change = friction * invMass * deltaT;
  // change should never be bigger than speed
  if (fabs(change) > fabs(speed))
    return speed / (invMass*deltaT);
  return friction;
}

static Vector3 LimitFriction(Vector3Par friction, Vector3Par speed, float deltaT, float invMass)
{
  return Vector3(LimitFriction(friction[0], speed[0], deltaT, invMass),
                 LimitFriction(friction[1], speed[1], deltaT, invMass),
                 LimitFriction(friction[2], speed[2], deltaT, invMass));
}

#else
  #define LimitFriction(f,s,t,im) (f)
#endif

void Car::PlaceOnSurface(Matrix4 &trans)
{
  base::PlaceOnSurface(trans);
  
  Vector3 pos = trans.Position();
  Texture *txt = NULL;
  float dX,dZ;

  GLandscape->RoadSurfaceYAboveWater(pos,&dX,&dZ,&txt);
  if (txt && (GLandscape->GetSeaTexture()==txt || txt->IsWater()))
  {
      LODShape *shape = GetShape();

      if (shape)
      {
        Vector3Val massCentre = shape->CenterOfMass();
        // estimate water sub-merging
        pos[1] += massCentre[1];
      }

      trans.SetPosition(pos);
  }
  else
  {
    trans.SetPosition(trans.Position() - trans.DirectionUp() * Type()->_dampers.GetInitPos());
  }
}

static inline float CalculateDamageFromSpeed(float speed)
{
  return (speed > 10.0f) ? (speed - 10.0f) * 3.0f : 0.0f;
}

/// surface usage count
/** used for audio controllers */
struct SurfInfoCount
{
  const SurfaceInfo *surf;
  int count;
};
TypeIsSimple(SurfInfoCount);

template <>
struct FindArrayKeyTraits<SurfInfoCount>
{
  typedef const SurfaceInfo * KeyType;
  static bool IsEqual(KeyType a, KeyType b) {return a == b;}
  static KeyType GetKey(const SurfInfoCount &a) {return a.surf;}
};

static float TurnCursorSideCoef = 1.2f;

Pair<Vector3,float> Car::ShowDrivingCursor(CameraType camType, Person *person) const
{
  if (person!=Driver()) return MakePair(VZero,_cursorY);
  float turnEff = floatMinMax(FutureVisualState()._turnBySpeed, -MaxTurnBySpeed, +MaxTurnBySpeed);
  Vector3 dir = Matrix3(MRotationY,-turnEff*TurnCursorSideCoef).Direction();
  float dimByAge = floatMinMax((10-Glob.time.Diff(_turnSpeedWantedLastActive))*0.3f,0,1);
  float dimByState = floatMinMax(fabs(dir.X())*100,0,0.5f);
  float dim = floatMax(dimByAge,dimByState);
  return MakePair(FutureVisualState().DirectionModelToWorld(dir*dim),_cursorY);
}


void Car::StabilizeTurrets(Matrix3Val oldTrans, Matrix3Val newTrans)
{
  if (fabs(FutureVisualState()._turnBySpeed)>0.01f)
  {
    Vector3 turnDir = Matrix3(MRotationY,-FutureVisualState()._turnBySpeed*TurnCursorSideCoef).Direction();
    Vector3 turnDirNew = newTrans.InverseRotation()*oldTrans*turnDir;
    float newValue = atan2(turnDirNew.X(),turnDirNew.Z())/TurnCursorSideCoef;
    // never allow catching up to actually turn the steering more
    // without a check this would happen when reversing
    if (fabs(newValue)<fabs(FutureVisualState()._turnBySpeed)) FutureVisualState()._turnBySpeed = newValue;
  }


  base::StabilizeTurrets(oldTrans, newTrans);
}

void Car::Simulate(float deltaT, SimulationImportance prec)
{
  PROFILE_SCOPE_EX(simCa,sim);

  ProtectedVisualState<const VisualState> vs = FutureVisualStateScope();

  const float cos60deg = 0.5f;
  if (!GetStopped() && (!_landContact || vs->DirectionUp().Y()<cos60deg || _objectContact))
  {
    SetSimulationPrecision(CarSimulationPrecisionHigh); // higher precision needed for unstable cars
  }
  else
  {
    SetSimulationPrecision(CarSimulationPrecisionNormal);
  }
#if 0 // _PROFILE
  #define PROFILE_SCOPE_CAR PROFILE_SCOPE
#else
  #define PROFILE_SCOPE_CAR(x)
#endif

  //***************************
  // Section 1. Not really connected with physical simulation.
  //***************************
  bool wakeUp = GetStopped(); // Will be true if car wake up from Stopped state during simulation.

  //GlobalShowMessage(50,"_rpmWanted %f, _thrustWanted %f, _thrust %f, speedSize %f, _gearBox.Ratio() %f", _rpmWanted, _thrustWanted, _thrust, fabs(_modelSpeed.Z()), _gearBox.Ratio());

  _isUpsideDown = FutureVisualState().DirectionUp().Y() < 0.3f;
  _isDead = IsDamageDestroyed();

  AIBrain *driver = PilotUnit();
  if (!HasSomePilot() || !driver || !driver->LSIsAlive())
  {
    // driver is dead - make the car slowing down
    // TODO: sometimes make random control changes
    _thrustWanted -= deltaT * 0.1f;
    if (_thrust <= 1e-3f)
    {
      _pilotBrake = true;
      _thrustWanted = 0.0f;
    }
  }

  MoveWeapons(deltaT);

  if (IsBlocked())
  {
    _pilotBrake = true;
    _thrustWanted = 0;
    //_turnWanted = 0;
  }

  
  const CarType *type = Type();
  int memoryLevel = _shape->FindMemoryLevel();
  if (memoryLevel >= 0)
  {
    PROFILE_SCOPE_CAR(carPX);
    int nProxies = type->GetProxyCountForLevel(memoryLevel);
    for (int i=0; i<nProxies; i++)
    {
      const ProxyObjectTyped &proxy = type->GetProxyForLevel(memoryLevel,i);

      Object *obj = proxy.obj;
      const EntityType *objType = obj->GetEntityType();
      if (type && !strcmp(objType->_simName, "scud"))
      {
        Matrix4 proxyTransform = obj->FutureVisualState().Transform();
        Vector3 offset(VFastTransform, proxyTransform, obj->GetShape()->MemoryPoint("tryska"));
        proxyTransform.SetPosition(offset);
        SkeletonIndex boneIndex = type->GetProxyBoneForLevel(memoryLevel,i);
        AnimateBoneMatrix(proxyTransform, FutureVisualState(), memoryLevel, boneIndex);

        Vector3Val pos = proxyTransform.Position();
        Vector3 speed = (pos - _scudPos) / deltaT;
        _scudPos = pos;
        _scudSpeed = speed;
      }
    }
  }

  if (FutureVisualState()._scudState >= 1 && FutureVisualState()._scudState <=2)
  {
    FutureVisualState()._scudState += _scudStateDir * deltaT;  // launching
    if (_scudStateDir<0 && FutureVisualState()._scudState<1)
    {
      FutureVisualState()._scudState = 0;
      _scudStateDir = 0;
    }
  }
  else if (FutureVisualState()._scudState >= 3 && FutureVisualState()._scudState < 4)
  {
    Assert(_scudStateDir>0);
    FutureVisualState()._scudState += _scudStateDir * deltaT;  // starting
    float effectValues[] = {_scudSpeed.Size(), FutureVisualState()._scudState-3, _scudPos.X(), _scudPos.Y(), _scudPos.Z()};
    _scudSmoke.Simulate(deltaT, prec, effectValues, lenof(effectValues));
  }
    
  { 
    PROFILE_SCOPE_CAR(carBS);  
    base::Simulate(deltaT, prec);  
  }

  ConsumeFuel(deltaT * 0.2f * GetHit(type->_fuelHit));
  if (FutureVisualState()._fuel <= 0.0f)
    _engineOff = true;

  // if car is going to explode
  if ((GetHit(type->_fuelHit) >= 0.9f || GetHit(type->_engineHit) >= 0.9f) && IsLocal() && _explosionTime > Glob.time + 60)
    // set some explosion
    _explosionTime = Glob.time + GRandGen.Gauss(2,5,20);

  float forwSpeed = FutureVisualState()._modelSpeed.Z();
  float speedSize = fabs(forwSpeed);

  if (!_engineOff)
  {
    ConsumeFuel(deltaT * (0.01f + FutureVisualState()._rpm * 0.02f));
    // calculate engine rpm
    _rpmWanted = speedSize * _gearBox.Ratio();
    saturateMax(_rpmWanted, fabs(_thrust) * 0.3f + 0.2f);
    saturate(_rpmWanted, 0.0f, 2.0f);
  }
  else
    _rpmWanted = 0.0f;

  float delta;
  delta =_rpmWanted - FutureVisualState()._rpm;
  Limit(delta, -0.5f * deltaT, +0.3f * deltaT);
  FutureVisualState()._rpm += delta;  

  // simulate left/right engine

  // main engine
  // if we go too slow we cannot turn at all
  // turn wheels
  float wheelLen = type->_wheelCircumference;
  FutureVisualState()._wheelPhaseDelta = forwSpeed * (1.0f / wheelLen) * deltaT;
  FutureVisualState()._wheelPhase = fastFmod(FutureVisualState()._wheelPhase + FutureVisualState()._wheelPhaseDelta, 1.0f);

  float turnEff = floatMinMax(_turn + FutureVisualState()._turnBySpeed, -1.0f, +1.0f);

  FutureVisualState()._phaseR+=FutureVisualState()._wheelPhaseDelta * (turnEff + 1.0f);
  FutureVisualState()._phaseL+=FutureVisualState()._wheelPhaseDelta * (1.0f - turnEff);

  if (_isDead)
  {
    _engineOff = true;
    _pilotBrake = true;
  }
  else if (_waterDrown)
    _engineOff = true;
  else if (_isUpsideDown)
    _pilotBrake = true;
  if (FutureVisualState()._fuel <= 0.0f)
    _engineOff = true;
  if (_engineOff)
    _thrustWanted = 0.0f;
  if (_thrustWanted * _thrust < 0.0f)
    _pilotBrake = true;
  if (FutureVisualState().ModelSpeed()[2] * _thrustWanted < 0.0f && fabs(FutureVisualState().ModelSpeed()[2]) > 0.5f)
    _pilotBrake = true;
  if (_pilotBrake && fabs(FutureVisualState().ModelSpeed()[2]) > 0.5f)
    _thrustWanted = 0;

  if (fabs(_thrustWanted)>0.1 || FutureVisualState()._speed.SquareSize() > 0.1f || _angVelocity.SquareSize() > 0.1f)
    OnMoved();

  // handle impulse
  float impulse2 = _impulseForce.SquareSize() + _impulseTorque.SquareSize();
  if (impulse2 > /*Square(GetMass()*0.01f)*/ 0.0f)
  {
    OnMoved();
    DamageOnImpulse(this,_impulseForce.Size(), VZero);
  }

  if (GetStopped())
  {
    // reset impulse - avoid cumulation
    _impulseForce = VZero;
    _impulseTorque = VZero;
    FutureVisualState()._modelSpeed = VZero;
    FutureVisualState()._speed = VZero;
    _angVelocity = VZero;
    _angMomentum = VZero;
  }
  else
  {
    const float maxSpeed = 0.7f;
    // if car woke up from stopped state or car is moving
    if (wakeUp || FutureVisualState()._speed.SquareSize() > maxSpeed || _angVelocity.SquareSize() > maxSpeed * 0.3f)
      wakeUp = true; // car woke up from stopped state
    else 
      wakeUp = false;
  }

  //***************************
  // Section 2. Physical simulation
  //***************************

  // calculate all forces, frictions and torques
  Vector3 force(VZero);
  Vector3 friction(VZero);
  Vector3 torque(VZero);
  Vector3 torqueFriction(VZero);

  Vector3 pForce(VZero); // partial force
  Vector3 pCenter(VZero); // partial force application point

  DamperForces damperForces(type->_dampers);

  float newPotencialEnergy = 0.0f;
  bool validPotencialEnergy = (_impulseForce == VZero && _impulseTorque == VZero);

  Vector3 motorForceV = VZero; // Force produced by motor

  { // simulate steering wheel - needs to be done for stopped car as well
    delta = _turnWanted - _turn;
    float maxDelta = (delta * _turn > 0.0f) ? _turnIncreaseSpeed : _turnDecreaseSpeed;
    Limit(delta, -maxDelta * deltaT, +maxDelta * deltaT);

    //maxMoment is already smaller than turnwanted
    if(!_maxTurnReached || _turn * delta <0) _turn += delta;

    // Calculate driving wheel turn
    FutureVisualState()._drivingWheelTurn += (_turn - _drivingWheelTurnOld[DRIVING_WHEEL_MEMORY - 1]) / DRIVING_WHEEL_MEMORY;
    memmove(_drivingWheelTurnOld + 1,_drivingWheelTurnOld, (DRIVING_WHEEL_MEMORY - 1) * sizeof(*_drivingWheelTurnOld));
    _drivingWheelTurnOld[0] = _turn;
  }

  { // simulate steering wheel - alternate "wheel speed" simulation
    delta = _turnSpeedWanted;
    static float mouseWheelSpeedCoef = 3.0f; // for mouse we allow the wheel to be turned a lot faster
    float maxDelta = (delta * FutureVisualState()._turnBySpeed > 0.0f) ? _turnIncreaseSpeed*mouseWheelSpeedCoef : _turnDecreaseSpeed*mouseWheelSpeedCoef;
    Limit(delta, -maxDelta * deltaT, +maxDelta * deltaT);
    FutureVisualState()._turnBySpeed += delta;
    Limit(FutureVisualState()._turnBySpeed, -MaxTurnBySpeed, +MaxTurnBySpeed);
  }

  if (!_isStopped && !CheckPredictionFrozen() && !IsAttached())
  {
    PROFILE_SCOPE_CAR(carSI);
    //***************************
    // Section 2.1 Physical simulation - calculate new position prediction according to velocities
    //***************************
    VisualState moveTrans = PredictPos<VisualState>(deltaT);

    ApplyRemoteStateAdjustSpeed(deltaT,moveTrans);
    //***************************
    // Section 2.2 Physical simulation - check if in new position are deep contacts
    //***************************
  const float MAX_IN  = 0.2f;
  const float ABOVE_ROAD = 0.05f;

    GroundCollisionBuffer groundCollBuffer;
    CollisionBuffer objCollBuffer;
    bool doObjectColl = (prec <= SimulateVisibleFar) && ( IsLocal() || GetNetworkManager().IsServerOwner() );

    // Objects are setposed. If car "eat" the pillar it starts shaking because of setposing.
#define SET_POS_OBJECT 1

    float softFactor = floatMin(4000.0f / GetMass() / 2.0f * speedSize / 4.0f, 1.0f);

      PROFILE_SCOPE(carCD);
      GLOB_LAND->GroundCollision(groundCollBuffer, this, moveTrans, ABOVE_ROAD, softFactor);
#if SET_POS_OBJECT
      if (doObjectColl)
        GLOB_LAND->ObjectCollision(objCollBuffer, this, moveTrans);
#endif

    // Find all deep contacts and calculate average repair move
    int numberOfDeepColl = 0;
    Vector3 averageRepairMove = VZero;
    for(int i = 0; i < groundCollBuffer.Size(); i++)
    {
      const UndergroundInfo &info = groundCollBuffer[i];
      if (info.type == GroundWater)
        continue;
      float depth = info.under - MAX_IN;
      if (depth <= 0.0f)
        continue;
      numberOfDeepColl++;
      Vector3 dirOut = Vector3(-info.dX, 1.0f, -info.dZ).Normalized();
      averageRepairMove += depth * dirOut.Y() * dirOut;
    }

#if SET_POS_OBJECT
#define MOVE_OBJECT_LESS 1.0f  // collisions with objects are less important or big repairs does not looks good.
    if (doObjectColl)
    {       
      for(int i = 0; i < objCollBuffer.Size();)
      {
        Object *obj=objCollBuffer[i].object;

        if (dyn_cast<Person,Object>(obj)) 
        {
          i++;
          continue;
        }

        unsigned int imax = i;
        float underMax = objCollBuffer[i].under;

        i++;
        while((i < objCollBuffer.Size()) && (obj == objCollBuffer[i].object))
        {
          float under = objCollBuffer[i].under;
          if (under > underMax)
          {
            imax = i;
            underMax = under;
          }
          i++;
        }
        if (obj->IsPassable())
          continue;
        const CollisionInfo &infoMax = objCollBuffer[imax];
        float depth = infoMax.under - MAX_IN;
        if (depth > 0.0f)
        {                    
          averageRepairMove += depth * infoMax.dirOut * MOVE_OBJECT_LESS;
          numberOfDeepColl++;                
        }
      }
    }
#endif        

    if (numberOfDeepColl > 0)
    {
      // Apply repair move to avoid deep contacts.
      averageRepairMove /= numberOfDeepColl;

      Matrix4 transform = moveTrans.Transform();
      Point3 newPos = transform.Position();            
      newPos += averageRepairMove;
      transform.SetPosition(newPos);
      moveTrans.SetTransform(transform);

      // re-estimate collisions after the repairing move
      groundCollBuffer.Clear();
      objCollBuffer.Clear();

        PROFILE_SCOPE_CAR(carCD);
        GLOB_LAND->GroundCollision(groundCollBuffer, this, moveTrans, ABOVE_ROAD, softFactor);
#if SET_POS_OBJECT
        if (doObjectColl)
          GLOB_LAND->ObjectCollision(objCollBuffer, this, moveTrans);
#endif
      }

#if !SET_POS_OBJECT
    if (doObjectColl)
      GLOB_LAND->ObjectCollision(objCollBuffer, this, moveTrans);
#endif

    //***************************
    // Section 2.3 Physical simulation - turning + trust
    //***************************
    delta = _thrustWanted - _thrust;
    if (_thrust * _thrustWanted <= 0.0f)
      Limit(delta, -2.0f * deltaT, +2.0f * deltaT);
    else
      Limit(delta, -deltaT, +deltaT);
    _thrust += delta;

    // do not allow fast reverse
    float minThrust = Interpolativ(FutureVisualState().ModelSpeed()[2], -5.0f, 0.0f, 0.0f, -1.0f);
    Limit(_thrust, minThrust, 1.0);

    //GlobalShowMessage(30, "_TurnWanted: %f limitTurn: %f", _turnWanted, limitTurn);

    //limit turn
    float turnEff = floatMinMax(_turn + FutureVisualState()._turnBySpeed, -1.0f, +1.0f);
    float physTurn = turnEff;

    /*float maxTurnCoef=1-fabs(speed.Z())*(1.0/5);
    saturateMax(maxTurnCoef,0);

    // limit max turn based on speed
    float maxTurn=maxTurnCoef*1.0+(1-maxTurnCoef)*0.5;
    //saturate(physturn,-maxTurn,+maxTurn);

    float limitTurn=1-(fabs(speed.Z())-3)*(1.0/type->GetMaxSpeedMs());
    limitTurn=limitTurn*limitTurn*4;
    saturate(limitTurn,0.05,1);
    //saturate(physturn,-limitTurn,+limitTurn);
    */

    // simulate front wheel (turning)

    // calculate radial velocity of steering point
#if ARROWS
    Vector3 steeringPoint(0,0,_shape->CenterOfMass()[2]-1/type->_invCenterToFrontWheelsLength);
    AddForce(PositionModelToWorld(steeringPoint),VUp*6,Color(1,0,0));
#endif

    float turnForward = forwSpeed * 0.7f;
    if(_thrust>0) saturate(turnForward, -12.0f, +12.0f); // avoid slips in high speed
    else saturate(turnForward, -25.0f, +25.0f); // avoid slips in high speed
    float turnWanted = physTurn * turnForward * 0.4f * type->_turnCoef;

    Vector3 wCenter(VFastTransform, FutureVisualState().ModelToWorld(), GetCenterOfMass());

    // apply left/right thrust
    bool gearChanged = false;
    if (_engineOff)
      // upside down
      gearChanged = _gearBox.Neutral();
    else
    {
      Vector3 relAccel = DirectionWorldToModel(FutureVisualState()._acceleration);
      // when accelerating we want higher duty
      // when decelerating, we may want slightly lower duty
      static float accelFactor = 0.1f;
      float accelDuty = 1.0f + floatMinMax(relAccel.Z(), -2.0f, +6.0f) * accelFactor;
      // when going uphill we want a lot higher duty
      // when going downhill we want a slightly lower duty
      static float slopeFactor = 3.0f;
      float slopeDuty = 1.0f + floatMinMax(FutureVisualState().Direction().Y(), -0.05f, 0.2f) * slopeFactor;
      gearChanged = _gearBox.Change(forwSpeed, accelDuty * slopeDuty);
    }

    float lWheelHit = floatMax(GetHit(type->_wheelLFHit), GetHit(type->_wheelLBHit));
    float rWheelHit = floatMax(GetHit(type->_wheelRFHit), GetHit(type->_wheelRBHit));

    if ((!_engineOff || fabs(turnWanted)>0.001) && (_landContact || _objectContact || (type->_canFloat && _waterContact)))
    {
      // the more we turn, the more power we loose
      float invSpeedSize;
      const float coefInvSpeed = 3.0f;
      const float minInvSpeed = 8.0f;
      const float defSpeed = 50.0f; // model tuned at this speed (plain level road)
      float power = type->GetMaxSpeed() * (1.0f / defSpeed) * 0.75f;
      if (power > 1.0f)
        power = Square(power);
      //float defPower=Square(1);
      if (speedSize < minInvSpeed * power)
        invSpeedSize = coefInvSpeed / minInvSpeed;
      else
        invSpeedSize = power * coefInvSpeed / speedSize;
      invSpeedSize *= 3.0f;
      //saturateMax(invSpeedSize,1/power);

      // floating
      if (!_landContact && !_objectContact)
        turnWanted *= 0.5f;

      // Limit wanted turn according to steering angle.
      float maxMoment = 5.0f / type->_invCenterToFrontWheelsLength * cos(0.7f * turnEff) / invSpeedSize;

      _maxTurnReached = (fabs(turnWanted) > fabs(maxMoment))? true : false;
      //GlobalShowMessage(30, "turnWanted: %f maxMoment: %f _turn: %f invSpeedSize: %f", turnWanted, maxMoment, _turn, invSpeedSize);                
      saturate(turnWanted, - maxMoment, + maxMoment);

      float lAccel = 0.0f;
      float rAccel = 0.0f;
      // destroyed wheel cannot accelerate at all
      float thrustDamage = 1.0f - GetHit(type->_engineHit);
      if (lWheelHit < 0.99f)
        lAccel = (_thrust * 4.0f * thrustDamage * (1.0f - lWheelHit * 0.5f) + turnWanted ) * invSpeedSize;
      if (rWheelHit < 0.99f) 
        rAccel = (_thrust * 4.0f * thrustDamage * (1.0f - rWheelHit * 0.5f) - turnWanted) * invSpeedSize;
      if (_waterContact && !_landContact)
      {
        float wsFactor = Type()->_waterSpeedFactor;
        lAccel *= wsFactor;
        rAccel *= wsFactor;
      }
      float motorForce = (lAccel + rAccel) * GetMass();
      float deltaForce = (lAccel - rAccel) * GetMass();
      motorForceV = Vector3(0.0f, 0.0f, motorForce);
#if 0
      if (this==GWorld->CameraOn())
        GlobalShowMessage(200,"turn %.2f, turnValue %.2f, coef %.2f, la %.2f, ra %.2f",_turn,turnWanted,type->_turnCoef,lAccel,rAccel);
#endif

      // left wheel
      pForce = Vector3(0.0f, 0.0f, lAccel * GetMass());
      force += pForce;
      pCenter = Vector3(-2.0f, -0.3f, 0.0f); // relative to the center of mass
      torque += pCenter.CrossProduct(pForce);
#if ARROWS
      AddForce(DirectionModelToWorld(pCenter + Vector3(0,0.5,0)) + wCenter, DirectionModelToWorld(pForce * InvMass()), Color(0,1,0));
#endif

      // right wheel
      pForce = Vector3(0.0f, 0.0f, rAccel * GetMass());
      force += pForce;
      pCenter = Vector3(2.0f, -0.3f, 0.0f); // relative to the center of mass
      torque += pCenter.CrossProduct(pForce);
#if ARROWS
      AddForce(DirectionModelToWorld(pCenter + Vector3(0,0.5,0)) + wCenter, DirectionModelToWorld(pForce * InvMass()), Color(0,1,0));
#endif

      // WIP:STEERING
      { // steering compensation
        // As implemented, car turns around its center of mass. This looks horrible, so we want to force
        // cars to turn around the "turningCenter" point provided in their configs instead. And that's
        // done by an additional shift of the car so the given point doesn't "slide" at all
        float d = (FutureVisualState().DirectionUp() * _angVelocity) * type->_turningCenterOffset * deltaT;
        Vector3 deltaPos = FutureVisualState().DirectionModelToWorld(Vector3(d, 0.0f, 0.0f));
        Vector3 pos = moveTrans.Position() + deltaPos;
        moveTrans.SetPosition(pos);
      }

      // moment that turns the top of vehicle outside the curve (just a big simplification)
      float moment = deltaForce * 0.25f * type->_invCenterToFrontWheelsLength;
      torque += Vector3(0.0f, 0.0f, moment);
#if ARROWS
      AddForce(Vector3(0,4,0)+wCenter,FutureVisualState().DirectionModelToWorld(Vector3(0,0,moment))*InvMass(),Color(0,1,0));
#endif
    }

    if (gearChanged)
      _doGearSound = true;

    // convert forces to world coordinates
    FutureVisualState().DirectionModelToWorld(torque, torque);
    FutureVisualState().DirectionModelToWorld(force, force);
    FutureVisualState().DirectionModelToWorld(motorForceV, motorForceV);

    //***************************
    // Section 2.4 Physical simulation - gravitation
    //***************************
    // apply gravity
    pForce = Vector3(0, -G_CONST * GetMass(), 0);
    force += pForce;
#if ARROWS
    AddForce(wCenter, pForce * InvMass());
#endif

    //***************************
    // Section 2.5 Physical simulation - friction angular + air friction.
    //***************************
    // angular velocity causes also some angular friction, this should be simulated as torque
    torqueFriction = _angMomentum * 0.5f;
    // body air friction
    FutureVisualState().DirectionModelToWorld(friction, Friction(FutureVisualState()._modelSpeed));
#if ARROWS
    AddForce(wCenter, -friction * InvMass());
#endif

    //***************************
    // Section 2.6 Physical simulation - collision resolving
    //***************************
    // Apply all collected impulses
    ApplyImpulses();

    wCenter.SetFastTransform(moveTrans.ModelToWorld(), GetCenterOfMass());

    float maxImpactSpeed = 0.0f;
    // how much in the collision is the max. under detected vertex
    // this can be used to rewind the time back to make sure high speed collisions are handled better
    float maxImpactUnder = 0.0f;
    // surface normal direction corresponding to maxImpactSpeed
    Vector3 maxImpactDirOut(VZero);
    Vector3 maxImpactSpeedLocal(VZero);

    float soft = 0.0f;
    float dust = 0.0f;
    if (deltaT > 0.0f)
    {
      // check collision on new position
      //float sFactor=type->GetMaxSpeedMs()*1.3;
      Vector3 fSpeed = FutureVisualState()._modelSpeed /*-Vector3(0,0,_thrust*sFactor)*/;
      // avoid too fast accel/deccel
      float maxAcc = floatMin(5.0f, type->GetMaxSpeedMs() * 0.14f);
      saturate(fSpeed[0], -maxAcc, +maxAcc);
      saturate(fSpeed[1], -maxAcc, +maxAcc);
#if _VBS3 //increase brake
      saturate(fSpeed[2], -maxAcc, +maxAcc);
#else
      saturate(fSpeed[2], -maxAcc * 0.6f, +maxAcc);
#endif
      float brakeFriction = 0.0f;
      saturateMax(brakeFriction, _pilotBrake);
      saturateMax(brakeFriction, (lWheelHit - 0.5f) * 0.5f);
      saturateMax(brakeFriction, (rWheelHit - 0.5f) * 0.5f);
      //fSpeed=fSpeed*(1-brakeFriction)+speed*brakeFriction;

      Vector3 objForce(VZero); // total object force
      Vector3 landForce(VZero); // total land force
      Vector3 objTorque(VZero); // total torque made by object forces.
      _objectContact = false;

      // For crash sound
      float maxImpactSpeedWater = 0.0f;
      float appliedImpulseObj = 0.0f;
      float maxImpactSpeedLand = 0.0f;
      float maxUnderWater = 0.0f;

      bool onlyStaticObjectContact = true;
      
      // check if we would rather go front or back
      float frontBackPreference = 0.0f;
      float leftRightPreference = 0.0f;
      if (doObjectColl)
      {
        PROFILE_SCOPE_CAR(carOC);
        //***************************
        // Section 2.6.1 Physical simulation - collision resolving with objects
        //***************************

#define MAX_IN_FORCE 0.1        

        int i = 0;
        bool takeStatic = false;  // first resolve non-static objects and then static ones 
                                  // it tries to avoid penetration with static objects                                   

        while (i < objCollBuffer.Size())
        {
          const CollisionInfo &info = objCollBuffer[i];
          Object *obj = (info.hierLevel == 0) ? info.object : info.parentObject;
          if (!obj || obj->IsPassable() || (!takeStatic && obj->Static()) || (takeStatic && !obj->Static()))
          {
            i++;
            if (!takeStatic && i == objCollBuffer.Size())
            {
              takeStatic = true;
              i = 0;
            }
            continue;
          }

          _objectContact = true;

          // Find all contacts with the object
          int fromIndx = i;
          int toIndx = i + 1;                    
          while (toIndx < objCollBuffer.Size() && obj == ((objCollBuffer[toIndx].hierLevel == 0) ? objCollBuffer[toIndx].object : objCollBuffer[toIndx].parentObject))
            toIndx++;

          EntityAI * bodyTmp = dyn_cast<EntityAI,Object>(obj); 
          if (bodyTmp && bodyTmp->GetStopped())
            bodyTmp->OnMoved();

          bool calculatedImpulses = false;

          DestructType destType = obj->GetDestructType();
          bool staticObj = obj->Static();

          onlyStaticObjectContact = onlyStaticObjectContact && staticObj;

          if (!staticObj)
            validPotencialEnergy = false;

          bool isPerson = dyn_cast<Person, Object>(obj) != NULL;
          if (isPerson)
          {
              // following line was causing ICE with X360 SDK 5632
              // TODOX360: fix instead of removing
            // Persons are not physical objects, that is why they must be handled in different way..
            ObjectCollisionWithPerson(objCollBuffer, fromIndx, toIndx, appliedImpulseObj);
            calculatedImpulses = true;
          }
          else
          {                                                
            if ((!staticObj /*&& destType != DestructMan*/) || (staticObj && !(destType == DestructTree|| destType == DestructTent || destType == DestructMan)))
            {
              // for such objects run marina simulation.
              // fixed static object
              PROFILE_SCOPE_CAR(carMA);

              CarCollisionsContactResolver resolver;
              resolver.Init(deltaT, this, objCollBuffer, fromIndx, toIndx);
              resolver.Resolve();

              calculatedImpulses = true;
              appliedImpulseObj += resolver.GetAppliedImpulse();
              Vector3 contactPos = objCollBuffer.GetContactPos(fromIndx,toIndx);
              Vector3 contactRel = PositionWorldToModel(contactPos);
              frontBackPreference -= contactRel.Z();
              leftRightPreference -= contactRel.X();
              obj->SimulationNeeded(GetVisualStateAge(*vs));
            }
            else
            {
              if (staticObj && (destType == DestructTree|| destType == DestructTent || destType == DestructMan))
              {       
                // The object can be destroyed. Apply just impulse till value that destroys it,.
                float maxAllowedImpulse = 1.0f/ (obj->GetInvArmor()* obj->GetInvMass() * 5.0f);
                PROFILE_SCOPE_CAR(carMA);

                CarCollisionsResolverMaxImpulse resolver;
                resolver.Init(deltaT,this, objCollBuffer, fromIndx, toIndx, maxAllowedImpulse);                                
                resolver.Resolve();
                
                if (maxAllowedImpulse <= resolver.GetAppliedImpulse())
                {
                  // the impulse was limited - this means the object is not strong enough to stop it
                  if (GetNetworkManager().IsServerOwner())
                  {
                    // crush it down
                    DoDamageResult result;
                    result.damage = 1;
                    obj->ApplyDoDamage(result, this, RString(), SkinnedPosition()); // skinned position not interesting, as there is most likely no handler to handle it anyway
                  }
                }
                else
                {
                  Vector3 contactPos = objCollBuffer.GetContactPos(fromIndx,toIndx);
                  Vector3 contactRel = PositionWorldToModel(contactPos);
                  frontBackPreference -= contactRel.Z();
                  leftRightPreference -= contactRel.X();
                }
                calculatedImpulses = true;
                appliedImpulseObj += resolver.GetAppliedImpulse();
              }             
            }                 
          }

          for(; i < toIndx; i++)
          {
            float penFactor; // factor for penetration forces
            if (staticObj)
              penFactor = GetMass();//floatMin(info.object->GetMass(), GetMass());                                                                        
            else
            {
              if (isPerson)
                // For man lower penFactor, otherwise people would pushing cars without problem
                penFactor = obj->GetMass() / 10.0f;
              else
              {                            
                penFactor = (GetMass() + obj->GetMass()) / 2.0f;
                saturateMax(penFactor, GetMass() / 2.0f);
                saturateMin(penFactor, GetMass() * 2.0f);
              }
            }
            /// Penetration force in contact
            const CollisionInfo &info = objCollBuffer[i];
            // info.pos is relative to object
            Vector3 pos = info.pos;
            Vector3 dirOut = info.dirOut;
            // create a force pushing "out" of the collision
            float forceIn = floatMin(info.under,MAX_IN);
#define MAX_SPRING_POINTS 10
#define MAX_OUT_ACCEL 5.0f 
            penFactor *= MAX_OUT_ACCEL / MAX_IN;
            if (objCollBuffer.Size() > MAX_SPRING_POINTS)
              penFactor *= MAX_SPRING_POINTS / objCollBuffer.Size();
            Vector3 pForce = dirOut * penFactor * forceIn;
            newPotencialEnergy += 0.5f * penFactor * forceIn * forceIn;
            Vector3 pTorque = pForce;
            pCenter = pos - wCenter;                        
            objForce += pForce;
            objTorque += pCenter.CrossProduct(pTorque);      
#if ARROWS
            AddForce(pos,pForce*InvMass(),Color(0,1,0));
#endif
            // Friction
            Vector3 speedLocal = SpeedAtPoint(pos);
            if (!obj->Static())
            {
              obj->SimulationNeeded(GetVisualStateAge(*vs));
              Entity *pcBody2 = dyn_cast<Entity,Object>(obj);
              if (pcBody2)
                speedLocal -= pcBody2->SpeedAtPoint(pos);
            }
            Vector3 speedFriction = speedLocal - dirOut * (dirOut * speedLocal);
            float speedFrictionSize = speedFriction.Size();
            if (speedFrictionSize > 0.01f)
            {
              //speedFriction.Normalize();
              //if (speedFrictionSize < 1.0f)
              //    speedFrictionSize *= 10.0f;
              //else
              //    saturateMax(speedFrictionSize, 10.0f);                            
#define FRICTION_COEF 0.4f
              pForce = speedFriction / speedFrictionSize * penFactor * forceIn * FRICTION_COEF;
              friction += pForce;
              torqueFriction += pCenter.CrossProduct(pForce);                             
#if ARROWS
              AddForce(pos, pForce*InvMass(), Color(1,1,0));
#endif
            }

            //damping
            float speedInDirection = dirOut * speedLocal;
            if (!isPerson)
            {
              // damping is really huge and can stop car... 
              pForce = 15.0f * dirOut * speedInDirection * GetMass() / objCollBuffer.Size();
              friction += pForce;
              torqueFriction += pCenter.CrossProduct(pForce);
            }
#if ARROWS
            AddForce(pos, pForce * InvMass(), Color(1,0,1));
#endif
            // DoDamage
            if (!calculatedImpulses && (speedInDirection > 0.0f) /*&& obj->GetDestructType() != DestructMan*/&& IsLocal())
            {                                                        
              float damage = speedInDirection * obj->GetMass() * GetInvMass() * GetInvArmor() * 0.02f / (toIndx - fromIndx);
              if (damage > 0.1f)
                LocalDamageMyself(VZero,damage,GetRadius());
              appliedImpulseObj += speedInDirection * GetMass() / 10.0f;
            }
          }
          if (!takeStatic && (i == objCollBuffer.Size()))
          {
            takeStatic = true;
            i = 0;
          }
        }
      } // if( object collisions enabled )

      // if there was some contact, we would like to know what can we do to avoid it
      /*
      if (frontBackPreference!=0)
        LogF("%s: front/back %g, left/right %g", cc_cast(GetDebugName()),frontBackPreference,leftRightPreference);
      */
            
      //***************************
      // Section 2.6.2 Physical simulation - collision resolving with ground
      //***************************                   
      // check for collisions
      _landContact = false;
      _waterContact = false;
      _waterDrown = false;

      const float menInvArmor = 0.066f;

      if (groundCollBuffer.Size() > 0)
      {
#define MAX_UNDER_FORCE 0.1f
        PROFILE_SCOPE_CAR(carGC);
        const Shape *landcontact = GetShape()->LandContactLevel();
        int numberOfContactPoints = landcontact ? landcontact->NPos() : 8;
        // if there are more collision points than possible contact points,
        // number of contact points must be low
        // this may be due to object geometry used instead of landcontact
        //saturateMax(numberOfContactPoints,groundCollBuffer.Size());
        float contactCoef = 8.0f / numberOfContactPoints;
        Vector3 damperDir = FutureVisualState().DirectionUp();
        Vector3 forwardDir = FutureVisualState().Direction();
        bool simulateDampers = GetDestroyed() == 0;
        SetSurfaceInfo(groundCollBuffer);
        // information for the detected collision
        //LogF("nCollision %d",gCollision.Size());
        for (int i = 0; i < groundCollBuffer.Size(); i++ )
        {
          UndergroundInfo &info = groundCollBuffer[i];
          // we consider two forces
          if (info.under < 0.0f)
          {
            /*
            LogF("Negative under %.3f",info.under);
            LogF("  tex %s",info.texture ? info.texture->Name() :"<null>");
            LogF("  obj %s",info.obj ? (const char *)info.obj->GetDebugName() : "<null>");
            */
            continue;
          }
          if (info.type == GroundWater)
          {
            _waterContact = true;
            // simulate swimming force
#if _VBS3
            const float coefNPoints = 16.0f / numberOfContactPoints;
#else
            const float coefNPoints = (16.0f / 1.5f) / numberOfContactPoints;
#endif
            // first is water is "pushing" everything up - causing some momentum
            saturateMax(maxUnderWater, info.under);

            pForce = Vector3(0,GetMass()*0.5f*info.under*coefNPoints,0);
            if (!type->_canFloat)
              pForce *= 0.1f;
            pCenter = info.pos - wCenter;
            torque += pCenter.CrossProduct(pForce);
            landForce += pForce;
            // add stabilizing torque
            // stabilized means DirectionUp() is (0,1,0)
            Vector3 stabilize = VUp - moveTrans.DirectionUp();
            torque += Vector3(0.0f, coefNPoints * 1.5f * GetMass(), 0.0f).CrossProduct(stabilize);
#if ARROWS
            AddForce(wCenter+pCenter,pForce*InvMass());
#endif
            // second is "water friction" - causing no momentum
            pForce[0] = FutureVisualState()._modelSpeed[0] * fabs(FutureVisualState()._modelSpeed[0]) * 15.0f;
            pForce[1] = FutureVisualState()._modelSpeed[1] * fabs(FutureVisualState()._modelSpeed[1]) * 15.0f + FutureVisualState()._modelSpeed[1] * 160.0f;
            pForce[2] = FutureVisualState()._modelSpeed[2] * fabs(FutureVisualState()._modelSpeed[2]) * 6.0f;
            pForce = FutureVisualState().DirectionModelToWorld(pForce * info.under) * GetMass() * (coefNPoints/700);
#if ARROWS
            AddForce(wCenter+pCenter,-pForce*InvMass());
#endif
            friction += pForce;
            torqueFriction += _angMomentum * 0.3f;

            float colSpeed2 = FutureVisualState()._speed.SquareSize();
            if (colSpeed2 > Square(8))
              saturateMax(maxImpactSpeedWater, colSpeed2);
          }
          else
          {
            // Collision with solid ground
            _landContact = true;
            Vector3 contactForce = VZero;                        
            Vector3 contactFriction = VZero;   
            // some friction is caused by moving the land aside
            // this applies only to soft surfaces
            if (info.texture)
            {
              soft = info.texture->Roughness() * 2.5f;
              dust = info.texture->Dustness() * 2.5f;
              saturateMin(dust,1);
            }

            Vector3 dirOut = Vector3(0.0f, info.dZ, 1.0f).CrossProduct(Vector3(1.0f, info.dX, 0.0f)).Normalized();
            Vector3 speedLocal = SpeedAtPoint(info.pos);

            // If the damper dir is nearly the same as dirOut
            // It is simple and more stable if it is really in dirOut direction.
            Vector3 localDamperDir = (damperDir * dirOut > 0.9f) ? dirOut : damperDir;

            // braking friction for this point
            float brakeFrictionWheel = brakeFriction;

            bool foundWheel = false;
            float under = (info.under - ABOVE_ROAD) * dirOut.Y(); // under is in VUp direction
            // If DamperDir is to different to dirOut, simulation is not correct.
            if ((under > -ABOVE_ROAD) && (damperDir * dirOut > 0.5f) && simulateDampers)
            {
              // wheel is affected, resize damper.
              float forceUp = 0.0f;
              float frictionUp = 0.0f;
              under /= damperDir * dirOut;
              float underOld = under;
              damperForces.SimulateContact(forceUp, frictionUp, FutureVisualState()._dampers, type->_dampers, info.vertex,
                info.level == _shape->FindLandContactLevel(), under, localDamperDir * speedLocal);
              foundWheel = underOld != under; 
              under *= damperDir * dirOut;   
              contactForce = localDamperDir * forceUp;
              contactFriction = localDamperDir * frictionUp;
            }

            float speedToGround = -dirOut * speedLocal;
            if (under > 0.0f)
            {
              contactForce += dirOut * GetMass() * 40.0f * contactCoef * under;
              contactFriction -= dirOut * GetMass() * contactCoef * (speedToGround * 0.2f + fSign(speedToGround) * 0.25f);
              newPotencialEnergy += 0.5f * GetMass() * 40.0f * contactCoef * under * under;
            }

            // check which damper is affected
            if (!type->_dampers.CheckWheel(info.vertex, info.level == _shape->FindLandContactLevel()))
            {
              // points other than wheels brake almost as much as wheel brake would
              saturateMax(brakeFrictionWheel, 0.8f);
              // contact with something else but the wheel
              // depending on the speed we may want to damage the vehicle
              // we may also want to simulate some kind of impact / absorb some part of the inertia

              if (speedToGround > 0.0f)
              {
                if (speedToGround > maxImpactSpeed)
                {
                  maxImpactSpeed = speedToGround;
                  maxImpactUnder = info.under;
                  maxImpactDirOut = dirOut;
                  maxImpactSpeedLocal = speedLocal;
                }
                saturateMax(maxImpactSpeedLand, speedToGround);
              }
            }
            else
            {
              // TODO: replace with damage based on damper simulation
              // do damage on speed 
              if (speedToGround > 0.0f && IsLocal())
              {                             
                float damage = CalculateDamageFromSpeed(speedToGround) * 2.5f * floatMin(deltaT, 0.03f) / groundCollBuffer.Size();
                if (damage > 0.0f)
                {
                  LocalDamageMyself(PositionWorldToModel(info.pos), damage * GetInvArmor(), GetRadius() * 0.2f);
                  DamageCrew(Driver(), damage * menInvArmor, "");
                }
              }
            }

            pCenter = info.pos - wCenter;
            torque += pCenter.CrossProduct(contactForce);
            torqueFriction += pCenter.CrossProduct(contactFriction);           
#if ARROWS
            AddForce(wCenter+pCenter, contactForce*InvMass(), Color(1,0,0));
            AddForce(wCenter+pCenter + Vector3(0,2,0), contactForce, Color(0,0,1));
            AddForce(wCenter+pCenter, -contactFriction*InvMass(), Color(0,1,1));
#endif
            // Friction                                             
            Vector3 contactFriction2 = VZero;

            Vector3 forwardDirLocal = forwardDir - dirOut * (forwardDir * dirOut);
            if (forwardDirLocal.SquareSize() < 0.1f)
            {
              // Forward dir is nearly parallel with dirOut
              // choose arbitrary perpendicular vector. 
              forwardDirLocal = localDamperDir - dirOut * (localDamperDir * dirOut);
              Assert(forwardDirLocal.SquareSize() >= 0.1f);
            }

            forwardDirLocal.Normalize();
            Vector3 sideDirLocal = forwardDirLocal.CrossProduct(dirOut);

            DirectionWorldToModel(forwardDirLocal, forwardDirLocal);
            DirectionWorldToModel(sideDirLocal, sideDirLocal);

            // Friction in point
            // second is "land friction" - causing little momentum                            
            float forwardSpeed1 = fSpeed * forwardDirLocal;
            float sideSpeed1 = fSpeed * sideDirLocal;                                       
            pForce = forwardDirLocal * (forwardSpeed1 * 30.0f + fSign(forwardSpeed1) * (3000.0f + brakeFrictionWheel * 40000.0f));
            if (forwSpeed > 1.0f && foundWheel)
              torque += 0.3f * FutureVisualState().DirectionModelToWorld(Vector3(pForce[2], 0.0f, 0.0f)) * (GetMass() *(1.0f/40000.0f) * contactCoef);
            pForce += sideDirLocal * (sideSpeed1 * 500.0f + fSign(sideSpeed1) * 90000.0f);
            pForce = FutureVisualState().DirectionModelToWorld(pForce) * (GetMass() * (1.0f / 40000.0f) * contactCoef);
            contactFriction2 += LimitFriction(pForce, _modelSpeed, deltaT, InvMass());
            torqueFriction += _angMomentum * 0.5f * contactCoef;

            // Friction according to land roughness
            float landMoved = 0.02f;
            float forwardSpeed2 = FutureVisualState()._modelSpeed * forwardDirLocal;
            float sideSpeed2 = FutureVisualState()._modelSpeed * sideDirLocal;          
            pForce = forwardDirLocal * (forwardSpeed2 * 4500.0f);
            if (forwSpeed > 1.0f && foundWheel)
              torque += 0.5f * FutureVisualState().DirectionModelToWorld(Vector3(pForce[2], 0.0f, 0.0f)) * (GetMass() * (1.0f/1000.0f) * contactCoef * landMoved * soft * type->_terrainCoef);
            pForce += sideDirLocal * (sideSpeed2 * 2000.0f);
            pForce = FutureVisualState().DirectionModelToWorld(pForce) * (GetMass() * (1.0f / 1000.0f) * contactCoef * landMoved * soft * type->_terrainCoef);
            contactFriction2 += LimitFriction(pForce, _modelSpeed, deltaT, InvMass());
#if ARROWS
            AddForce(wCenter+pCenter, -contactFriction2*InvMass(), Color(1,1,1));  
#endif              
            contactFriction += contactFriction2;             
            landForce += contactForce;                        
            friction += contactFriction;
          }
        }

#if 0 //_ENABLE_CHEATS
        GlobalShowMessage(100,"Under %.2f, brake %.2f, force %.1f, fric %.1f",totalUnder,brakeFriction,DirectionWorldToModel(force).Z(),DirectionWorldToModel(friction).Z());
#endif
  
        if (_waterContact)
        {
          const SurfaceInfo &info = GLandscape->GetWaterSurface();
          soft = info._roughness * 2.5f;
          dust = info._dustness * 2.5f;
          saturateMin(dust, 1.0f);
        }
      }                        
      force += objForce;
      torque += objTorque;
      force += landForce;

      //***************************
      // Section 2.6.4 Physical simulation - Crash
      //*************************** 
      if (IsLocal())
      {
        const float maxAllowedImpactSpeed = 10.0f;
        if (maxImpactSpeed > maxAllowedImpactSpeed && _maxContactSpeed<maxAllowedImpactSpeed)
        {
#if ENABLE_REPORT
          if (this == GWorld->CameraOn())
            LogF("Impact maxImpactSpeed=%g, under=%g",maxImpactSpeed,maxImpactUnder);
#endif
          float damage = CalculateDamageFromSpeed(maxImpactSpeed);
          if (damage > 0.0f)
          {
            LocalDamageMyself(VZero, damage * GetInvArmor(),GetRadius());
            DamageCrew(Driver(), damage * menInvArmor, "");
          }
          // reduce the object impact speed
          // maxImpactSpeed was computed as -maxImpactDirOut*maxImpactSpeedLocal;
          // target is maxAllowedImpactSpeed
          // speed is in, and quite large - we know the angle is < 90 deg
          // reduce overall speed to avoid excessive jumping back
          // TODO: some part may be caused by the rotation
          // we may need to reduce the rotation as well

          float speedImpact = -maxImpactDirOut * FutureVisualState().Speed();
          if (speedImpact > maxAllowedImpactSpeed)
          {
            Vector3 newSpeed = FutureVisualState().Speed() + maxImpactDirOut * (speedImpact - maxAllowedImpactSpeed);
#if ENABLE_REPORT
            if (this == GWorld->CameraOn())
            {
              LogF("Reduced speed from %g to %g (%g,%g,%g to (%g,%g,%g)",speedImpact,maxAllowedImpactSpeed,Speed()[0],Speed()[1],Speed()[2],newSpeed[0],newSpeed[1],newSpeed[2]);
              FutureVisualState()._speed = newSpeed;
            }
#endif
          }
        }
        // remember the information from previous frame
        _maxContactSpeed = maxImpactSpeed;

        // Under water damage
        if (!type->_canFloat)
        {
          const float maxFordMax = 1.8f;
          // if more than half in water, water is probably filling the engine
          const Shape *geom = _shape->GeometryLevel();
          float maxFordByShape = (geom->Max().Y() - geom->Min().Y()) * 0.5f;
          float maxFord = floatMin(maxFordMax, maxFordByShape);

          if (maxUnderWater > maxFord)
          {
            float damage = (maxUnderWater-maxFord) * 0.5f;
            saturateMin(damage, 0.2f);
            LocalDamageMyself(VZero, damage * deltaT, GetRadius());
            // engine has no way to continue working
            _waterDrown = true;
          }
        }

        // starting time when no crash are evaluated
        if ((Glob.time > _disableDamageUntil) && (_doCrash == CrashNone))
        {
          // Choose the biggest crash for sound
          float crashLand = fabsf(maxImpactSpeedLand);
          float crashObj = appliedImpulseObj * GetInvMass();
          float crashWater = fabsf(maxImpactSpeedWater);
          float crash = 0.0f;

          if (crashLand > crashObj)
          {
            if (crashLand > crashWater)
            {
              _doCrash = CrashLand;
              crash = crashLand;
            }
            else
            {
              _doCrash = CrashWater;
              crash = crashWater;
            }                        
          }
          else
          {
            if (crashObj > crashWater)
            {
              _doCrash = CrashObject;
              crash = crashObj;
            }
            else
            {
              _doCrash = CrashWater;
              crash = crashWater;
            }
          }

          // specify CrashObject
          if (_doCrash == CrashObject)
          {
            // check collision buffer
            if (objCollBuffer.Size() > 0)
            {
              // CrashObject distinguish types
              int destTree = 0;
              int destBuild = 0;
              int destEng = 0;

              for (int i = 0; i < objCollBuffer.Size(); i++)
              {
                CollisionInfo &cInfo = objCollBuffer[i];
                if (cInfo.object)
                {
                  // distinguish crash object on the basis of destruction types
                  DestructType dt = cInfo.object->GetDestructType();
                  if (dt == DestructTree)
                    destTree++;
                  else if (dt == DestructBuilding)
                    destBuild++;
                  else if (dt == DestructEngine || dt == DestructWreck)
                    destEng++;
                }
              }

              // no suitable category found
              if (destEng == 0 && destTree == 0 && destBuild == 0)
                _doCrash = CrashNone;
              else
              {
                // select the most principal object
                if (destTree > destBuild)
                {
                  if (destTree > destEng) 
                    _doCrash = CrashTree;
                  else
                    _doCrash = CrashArmor;
                }
                else
                {
                  if (destBuild > destEng)
                    _doCrash = CrashBuilding;
                  else
                    _doCrash = CrashArmor;
                }
              }
            }
          }
          _crashVolume = floatMinMax(0.0f, 1.0f, crash * 0.1f);
          if (crash < 0.01f)
          {
            _doCrash = CrashNone;
            _crashVolume = 0.0f;
          }
        }
      }
    }

    //***************************
    // Section 3. Dust, head movement ... 
    //***************************        
    // simulate track drawing   
    if (EnableVisualEffects(prec, 300.0f))
    {
      //PROFILE_SCOPE_CAR(carVE);

      // Update the track - either prolong or terminate
      {
        PROFILE_SCOPE_CAR(carTR);
        _track.Update(RenderVisualStateRaw(), deltaT, !_landContact);  // based on the visible state
      }

      if (_landContact)
      {
        PROFILE_SCOPE_CAR(carDS);
        Vector3 lcPos = (_track.BackLeftPos() + _track.FrontLeftPos()) * 0.5;
        Vector3 rcPos = (_track.BackRightPos() + _track.FrontRightPos()) * 0.5f;
        Vector3 lPos = lcPos + Vector3(-0.2f, 0.1f, 0) + 0.2f * FutureVisualState().ModelSpeed();
        Vector3 rPos = rcPos + Vector3(+0.2f, 0.1f, 0) + 0.2f * FutureVisualState().ModelSpeed();
        Vector3 cSpeed = 0.5f * FutureVisualState()._speed;
        float dSoft = floatMax(dust, 0.0025f);
        float density = speedSize * (1.0f / 10.0f) * dSoft;
        saturate(density, 0.0f, 1.0f);
        float dustColor = dSoft * 8.0f;
        saturate(dustColor, 0.0f, 1.0f);
        if (density > 0.2f)
        {
          float leftDustValues[] = {density, dustColor, lPos.X(), lPos.Y(), lPos.Z(), cSpeed.X(), cSpeed.Y(), cSpeed.Z()};
          _leftDust.Simulate(deltaT, prec, leftDustValues, lenof(leftDustValues));
          float rightDustValues[] = {density, dustColor, rPos.X(), rPos.Y(), rPos.Z(), cSpeed.X(), cSpeed.Y(), cSpeed.Z()};
          _rightDust.Simulate(deltaT, prec, rightDustValues, lenof(rightDustValues));
        }
      }
      PROFILE_SCOPE_CAR(carEX);
      SimulateExhaust(deltaT, prec);
    }

    if (_waterContact && type->_canFloat)
    {
      Vector3 lPos = FutureVisualState().PositionModelToWorld(type->_lWaterPos);
      Vector3 rPos = FutureVisualState().PositionModelToWorld(type->_rWaterPos);

      // place on water surface
      lPos[1] = GLandscape->WaterSurfaceY(lPos); // sea level
      rPos[1] = GLandscape->WaterSurfaceY(rPos);
      // TODO: some more effective way to avoid too much conversions
      lPos = PositionWorldToModel(lPos);
      rPos = PositionWorldToModel(rPos);

      float dens = floatMin(speedSize * 0.3f, 0.7f);
      float thrustR = _thrust * (turnEff + 1.0f) * 0.5f;
      float thrustL = _thrust * (1.0f - turnEff) * 0.5f;
      float densL = fabs(thrustL) + dens;
      float densR = fabs(thrustR) + dens;
      saturateMin(densL, 1.0f);
      saturateMin(densR, 1.0f);
      float coefL = thrustL * 0.3f;
      float coefR = thrustR * 0.3f;
      saturateMax(coefL, 0.05f);
      saturateMax(coefR, 0.05f);
      Vector3 spdL = FutureVisualState().Speed()*0.1f-FutureVisualState().DirectionModelToWorld(Vector3(0,0,4)*coefL);
      Vector3 spdR = FutureVisualState().Speed()*0.1f-FutureVisualState().DirectionModelToWorld(Vector3(0,0,4)*coefR);
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

    {
      PROFILE_SCOPE_CAR(carTR);
      StabilizeTurrets(FutureVisualState().Transform().Orientation(), moveTrans.Orientation());
    }

    //***************************
    // Section 2.7 Move to new position and re-normalize energy
    //***************************
    PROFILE_SCOPE_CAR(carEn);
    Vector3 oldPosition = FutureVisualState().Position();
    Move(moveTrans);

    // Enlarge not simulated dampers
    damperForces.Finish(FutureVisualState()._dampers, type->_dampers, deltaT);
    newPotencialEnergy += FutureVisualState()._dampers.Energy(type->_dampers);

    // Energy renormalization
    float newKinetic = GetMass() * FutureVisualState()._speed.SquareSize() * 0.5f; // E=0.5*m*v^2
    newKinetic += _angMomentum * _angVelocity * 0.5f;
    float newDiffPotencial = (FutureVisualState().Position()[1] - oldPosition[1]) * GetMass() * G_CONST;                
    float energyFromEngine = (FutureVisualState().Position() - oldPosition).DotProduct(motorForceV);

    if (_validEnergy && validPotencialEnergy && (newDiffPotencial + newKinetic - energyFromEngine + newPotencialEnergy) > _oldEnergy /*+ GetMass()/10*/)
    {
      float repairedKinetic = _oldEnergy - newDiffPotencial + energyFromEngine - newPotencialEnergy;                        
      if (repairedKinetic <= 0.0f)
      {                
        FutureVisualState()._speed = VZero;
        _angVelocity = VZero;
        _angMomentum = VZero;
      }
      else
      {
        float renormalizationCoef = sqrt(repairedKinetic / newKinetic);
        FutureVisualState()._speed *= renormalizationCoef;
        _angVelocity *= renormalizationCoef;
        _angMomentum *= renormalizationCoef;
      }
    }

    newKinetic = GetMass() * FutureVisualState()._speed.SquareSize() * 0.5f; // E=0.5*m*v^2
    newKinetic += _angMomentum * _angVelocity * 0.5f;

    _oldEnergy = newKinetic + newPotencialEnergy;
    _validEnergy = validPotencialEnergy;        

    //***************************
    // Section 2.7 Calculate new velocities
    //***************************
#if  ARROWS
    // Show force and torque
    AddForce(wCenter + Vector3(0,4,0), force * InvMass(), Color(1,0.5, 0.25));
    AddForce(wCenter + Vector3(0,4,0), torque * InvMass(), Color(0.25, 0.5, 1));
#endif
    ApplyForces(deltaT, force, torque, friction, torqueFriction/*, 5.5f * GetMass()*/,0, true);
    // apply static friction
    bool stopCondition = false;
    if (_pilotBrake && _landContact && !_waterContact /*&& !_objectContact*/)        
    {
      float maxSpeed = Driver() ? Square(0.7f) : Square(1.2f);
      if (FutureVisualState()._speed.SquareSize() < maxSpeed && _angVelocity.SquareSize() < maxSpeed * 0.3f)
        stopCondition = true;
    }
    if (stopCondition)
      StopDetected();
    else
      OnMoved();
    DirectionWorldToModel(FutureVisualState()._modelSpeed, FutureVisualState()._speed);
  }

  // slow steering auto-centering
  static float centerSpeedFast = 0.025f;
  static float centerSpeedSlow = 0.00f;
  static float centerAngleFast = 0.025f;
  static float centerAngleSlow = 0.10f;
  float centerSpeed = Interpolativ(fabs(FutureVisualState()._turnBySpeed),centerAngleFast,centerAngleSlow,centerSpeedFast,centerSpeedSlow);
  FutureVisualState()._turnBySpeed = TowardZero(FutureVisualState()._turnBySpeed,deltaT*centerSpeed);

  PROFILE_SCOPE_EX(simCP,sim);
  SimulatePost(deltaT, prec);
}

void Car::FakePilot(float deltaT)
{
  _turnIncreaseSpeed = 1.0f;
  _turnDecreaseSpeed = 1.0f;
}

void Car::PerformFF(FFEffects &eff)
{
  base::PerformFF(eff);
  float stiffX = fabs(FutureVisualState().ModelSpeed().Z())*1.5f/GetType()->GetMaxSpeedMs()+0.3f;
  saturate(stiffX,0,1);
  eff.stiffnessX = stiffX;
  eff.stiffnessY = 0.3f;
  Vector3 relAccel = DirectionWorldToModel(FutureVisualState()._acceleration);
  eff.surfaceMag = 0.1 * relAccel.Y();
}

void Car::SuspendedPilot(AIBrain *unit, float deltaT )
{
  _pilotBrake = true;
  _thrustWanted = 0;
  _turnWanted = 0;
  _turnSpeedWanted = 0;
  _turnSpeedWantedLastActive = TIME_MIN;
}

#if defined _WIN32 && _VBS2
#include "joystick.hpp"
#endif

void Car::KeyboardPilot(AIBrain *unit, float deltaT )
{
  if (!unit->GetPerson() || unit->GetPerson()->KeyboardUsedInFrame()) return;
  //CancelStop();
  InputInterface *input = GetNetworkManager().GetInputInterface(unit->GetPerson()->GetRemotePlayer());
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
    float aimCursorY = GInput.GetActionWithCurveExclusive(InputInterface::CurveFov1, UACarAimUp, UACarAimDown);
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

  float forward;
  forward = (input->GetAction(UACarForward)-input->GetAction(UACarBack))*0.75;
  forward += input->GetAction(UACarSlowForward)*0.33;

  forward += input->GetActionCarFastForward();
  _thrustWanted = forward;

  float turnEff = floatMinMax(_turn+FutureVisualState()._turnBySpeed,-1,+1);
  float maxThrust = 1-fabs(turnEff)*0.7;
  saturate(_thrustWanted,-maxThrust,+maxThrust);
  
  float asz=fabs(FutureVisualState().ModelSpeed().Z());

  //if (!GWorld->HasMap())
  {
    // aim and turn with aiming actions

    Vector3 dir = LookIntoTurn(GWorld->GetCameraType());
    unit->GetPerson()->ProcessLookAroundAuto(dir);

    LookAround(aimX, aimY, deltaT, &Type()->_viewPilot, unit, false, true);
    //UACarAimLeft and UACarLeft are separate now, so looking around and turning can be processed simultaneously
    // turn with arrows
    float mouseLR = (input->GetAction(UACarRight) - input->GetAction(UACarLeft))*Input::MouseRangeFactor;
    static float coefLR = 1.0f;
    mouseLR *= coefLR;
    const float turnFactor = 0.4;
    _turnWanted = mouseLR * turnFactor;

    static float wheelFactor = 0.04f;
    float turnSpeedWanted = input->GetAction(UACarWheelRight)-input->GetAction(UACarWheelLeft);
    if (fabs(turnSpeedWanted)>0.01f) _turnSpeedWantedLastActive = Glob.time;
    _turnSpeedWanted = turnSpeedWanted*wheelFactor;

    float slowTurn = 1-asz*(1.0/15);
    saturateMax(slowTurn,0);
    // TODO: different increase for speed / direct control
    // TODO: try speed control with a keyboard
      _turnIncreaseSpeed = (slowTurn*0.6)+0.3;
      _turnDecreaseSpeed = ((1-slowTurn)*0.8)+0.8;
#if 0 // turn is limited by Type()->_inputTurnCurve
    float maxTurnCoef=1-asz*(1.0/10);
    saturateMax(maxTurnCoef,0);

    // limit max turn based on speed
    float maxTurn=maxTurnCoef*1.0+(1-maxTurnCoef)*0.3;
    saturate(_turnWanted,-maxTurn,+maxTurn);
#endif
  }

  Limit(_thrustWanted,-1,1);

  // add input curve... 
  //float turnWantedOrig = _turnWanted;
  if(_turnWanted < 0)
    _turnWanted = -Type()->_inputTurnCurve.GetInterpolated(asz,-_turnWanted );
  else
    _turnWanted = Type()->_inputTurnCurve.GetInterpolated(asz,_turnWanted );

  /*char out[1024];
  sprintf(out, "sS %lf, _tWO %lf, _tWI %lf", asz,turnWantedOrig, _turnWanted);
  DIAG_MESSAGE(10, out);
  LogF(out);*/

  Limit(_turnWanted,-1,1);
  if (fabs(forward)>0.05 && !_waterDrown)
  {
    CancelStop();
    EngineOn();
  }
  if (fabs(_turn - _turnWanted)+fabs(_turn)+fabs(_turnSpeedWanted)+fabs(FutureVisualState()._turnBySpeed) > 0.05)
    CancelStop();
  
  if
  (
#if _VBS3 // change the threshold for the pilot brake
    (fabs(_thrustWanted)<0.1 && fabs(FutureVisualState()._modelSpeed.Z())<1.0) ||
#else
    fabs(_thrustWanted)<0.2 && fabs(FutureVisualState()._modelSpeed.Z())<4.0 ||
#endif
    _thrustWanted*FutureVisualState()._modelSpeed.Z()<0
  )
  {
    _pilotBrake=true;
#if _VBS3
    if(_thrustWanted * FutureVisualState()._modelSpeed.Z() < -0.1 && fabs(FutureVisualState()._modelSpeed.Z())<1 )
#else
    if(fabs(FutureVisualState()._modelSpeed.Z())>1)
#endif
       _thrustWanted=0;
  }
  else
    _pilotBrake = false;
}

RString Car::DiagText() const
{
  float dx, dz;
  Vector3 pos = FutureVisualState().Position();

  pos[1] = GLandscape->SurfaceYAboveWater(pos[0], pos[2], &dx, &dz);

  float slope = FutureVisualState().Direction().Z()*dz + FutureVisualState().Direction().X()*dx;
  char buf[256];
  sprintf(buf," Slope %.0f %% %s %.2f,(%.2f)",slope*100,_pilotBrake ? "B" : "E",_thrustWanted,_turnWanted);
  sprintf(buf+strlen(buf)," RF %.1f,%.1f", _reverseTimeLeft,_forwardTimeLeft);
  return base::DiagText()+(RString)buf;
}

#define DIAG_SPEED 0

#if _ENABLE_AI

void Car::AIPilot(AIBrain *unit, float deltaT)
{
  //if( unit ) SelectFireWeapon();
  float asz=fabs(FutureVisualState().ModelSpeed().Z());
  float slowTurn = 1-asz*(1.0/15);
  saturateMax(slowTurn,0);

  _turnIncreaseSpeed = (slowTurn*0.5)+0.8;
  _turnDecreaseSpeed = ((1-slowTurn)*0.8)+0.8;


  // TODO: limit AIPilot simulation rate (10 ps)
  Assert(unit);

  // move to given point
  // check goto/fire at command status

  Vector3Val speed = FutureVisualState().ModelSpeed();
  
  SteerInfo info;
  info.headChange=0;
  info.speedWanted=0;
  info.turnPredict=0;
  
  if( unit->GetState()==AIBrain::Stopping )
  {
    // special handling of stop state
    if (fabs(speed[2])<1.0f)
    {
      unit->OnStepCompleted();
    }
    info.speedWanted = 0.0f;
  }
  else if (unit->GetState()==AIBrain::Stopped)
    info.speedWanted = 0.0f;
  else
    LeaderPilot(info);
  // functions above may have attempted to start the engine
  // we may need to cut it off
  if (_waterDrown)
    _engineOff = true;

  float aHC = fabs(info.headChange);
  
  bool disableForward=false;
  if ((GetMoveMode() != VMMBackward && FutureVisualState().ModelSpeed().Z() < 10.0f) || (GetMoveMode() == VMMBackward && FutureVisualState().ModelSpeed().Z() > -10.0f))
  {
    // the intention of the code below was to stop reversing once the direction
    // this does not work well when we need to reverse because of collision
    if( !_reverseNeeded && aHC<(H_PI*3/16) && _reverseTimeLeft<1)
    {
      if( _reverseTimeLeft>0 )
      {
        CreateFreshPlan();
        _forwardTimeLeft=2;
      }
      _reverseTimeLeft=0;
    }
    else if( aHC>(H_PI*8/16) )
    {
      // we want to change direction completely, start by reversing
      _reverseTimeLeft=2;
      _reverseNeeded = false;
    }

    CollisionBuffer retVal;
    if (GetMoveMode() == VMMBackward)
      GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(),retVal,Landscape::FilterIgnoreOne(this),FutureVisualState().Position(),FutureVisualState().Position()-CollisionSize()*1.5*FutureVisualState().Direction(),0/*1.5*/, ObjIntersectGeom);
    else
      GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(),retVal,Landscape::FilterIgnoreOne(this),FutureVisualState().Position(),FutureVisualState().Position()+CollisionSize()*1.5*FutureVisualState().Direction(),0/*1.5*/, ObjIntersectGeom);

    disableForward=( retVal.Size()>0 );
  }

  // if collision is preventing us to move, do something
  if (_objectContact && fabs(FutureVisualState().ModelSpeed().Z())<0.1f)
  {
    // if not in the reverse-forward maneuver yet, start it
    if (_reverseTimeLeft<=0 && _forwardTimeLeft<=0)
    {
      disableForward = true;
      // how long do we want to reverse
      _reverseTimeLeft = 2.0f;
      _reverseNeeded = true;
    }
  }
  
  bool reverse = false;
  // before we stop reversing/forwarding, we should stop
  // otherwise we will start turning wheels while moving, resulting in opposite rotation
  bool brake = false;
  static float brakeTime = 0.4f;
  if (_forwardTimeLeft>0)
  {
    _forwardTimeLeft -= deltaT*(1-_turningWheels);
    if (_forwardTimeLeft<brakeTime)
      brake = true;
  }
  else if (_reverseTimeLeft>0 || disableForward)
  {
    // check if back is free
    CollisionBuffer retVal;
    if (GetMoveMode() == VMMBackward)
      GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(),retVal,Landscape::FilterIgnoreOne(this),FutureVisualState().Position(),FutureVisualState().Position()+CollisionSize()*2*FutureVisualState().Direction(),0/*1.5*/, ObjIntersectGeom);
    else
      GLandscape->ObjectCollisionLine(GetFutureVisualStateAge(),retVal,Landscape::FilterIgnoreOne(this),FutureVisualState().Position(),FutureVisualState().Position()-CollisionSize()*2*FutureVisualState().Direction(),0/*1.5*/, ObjIntersectGeom);
    if (retVal.Size()>0)
    {
      _reverseTimeLeft=0;
      _forwardTimeLeft=1;
    }
    else
    {
      _reverseTimeLeft-=deltaT*(1-_turningWheels);
      if (_reverseTimeLeft<brakeTime)
        brake = true;
      if (_reverseTimeLeft<=0)
      {
        CreateFreshPlan();
        _forwardTimeLeft=1;
      }
      reverse=true;
      if( (GetMoveMode() != VMMBackward && info.speedWanted>0) || (GetMoveMode() == VMMBackward && info.speedWanted<0) )
      {
        info.speedWanted=-info.speedWanted;
        info.headChange=AngleDifference(H_PI,info.headChange);
      }
    }
  }

#if DIAG_SPEED
  if (this==GWorld->CameraOn())
    LogF("Pilot speed %.1f",info.speedWanted);
#endif

  AvoidCollision(deltaT,info.speedWanted,info.headChange);

#if DIAG_SPEED
  if (this==GWorld->CameraOn())
    LogF("Avoid %.1f",info.speedWanted);
#endif

  float curHeading=atan2(FutureVisualState().Direction()[0],FutureVisualState().Direction()[2]);
  float wantedHeading=curHeading+info.headChange;

  // estimate inertial orientation change
  Matrix3Val orientation=FutureVisualState().Orientation();
  Matrix3Val derOrientation=_angVelocity.Tilda()*orientation;
  static float estTime = 0.5f;
  Matrix3Val estOrientation=orientation+derOrientation*estTime;
  Vector3Val estDirection=estOrientation.Direction();
  float estHeading=atan2(estDirection[0],estDirection[2]);

  info.headChange=AngleDifference(wantedHeading,estHeading);
  if (fabs(info.headChange) < H_PI / 18)
    info.headChange *= fabs(info.headChange) * (18/H_PI);

  
  {
    float aTP=fabs(info.turnPredict);
    if (aTP>H_PI/64)
    {
      // limit speed only when turning significantly
      //float maxSpeed=GetType()->GetMaxSpeedMs();
      float maxSpeed=GetType()->GetMaxSpeedMs();
      // even with very slow or very fast car use some normal brakes
      // note: actually in-turn speed is not much dependent on max speed
      // but we can assume faster vehicles have better turning radius
      saturate(maxSpeed,50/3.6,100/3.6);
      float limitSpeed=Interpolativ(aTP,H_PI/64,H_PI/8,maxSpeed,5);

#if _ENABLE_CHEATS
      if( CHECK_DIAG(DEPath) && this==GWorld->CameraOn() )
      {
        DIAG_MESSAGE(200,"Turn limit %.1f (%.3f, turn %.3f)",limitSpeed*3.6,info.headChange,info.turnPredict);
        //LogF("Turn limit %.1f (%.3f, turn %.3f)",limitSpeed,headChange,turnPredict);
      }
#endif
      saturate(info.speedWanted,-limitSpeed,+limitSpeed);
      saturate(info.speedWanted,-_limitSpeedForced,+_limitSpeedForced);
    }
  }

#if DIAG_SPEED
  if (this==GWorld->CameraOn())
    LogF("Turn %.1f",info.speedWanted);
#endif
  
  if (fabs(info.speedWanted)>0.5f && !_waterDrown)
    EngineOn();

  // some thrust is needed to keep speed
  float isSlow=1-fabs(speed.Z())*(1.0f/17);
  float maxTurn = 1-fabs(speed.Z())*(1.0f/25);
  saturate(isSlow,0.2f,1);
  saturate(maxTurn,0.05f,1);

  float isLevel=1-fabs(FutureVisualState().Direction()[1]*(1.0f/0.6f));
  saturate(isLevel,0.2f,1);
  saturateMax(isSlow,isLevel); // change thrust slowly on steep surfaces


  if (fabs(info.speedWanted)<0.1f && speed.SquareSize()<1)
    _thrustWanted=0;
  else
  {
    Vector3 relAccel=DirectionWorldToModel(FutureVisualState()._acceleration);
    float changeAccel=(info.speedWanted-speed.Z())*(1/0.5f)-relAccel.Z()*0.2f;
    changeAccel*=isSlow;
    float thrustOld=_thrust;
    float thrust=thrustOld+changeAccel*0.33f;
    Limit(thrust,-1,1);
    _thrustWanted=thrust;
  }

  _turnWanted=info.headChange*4;
  if (GetMoveMode() == VMMBackward)
    _turnWanted=-_turnWanted; //we have to turn the wheels in opposite direction when moving backwards to achieve given wanted heading

  _turnSpeedWanted = 0;
  _turnSpeedWantedLastActive = TIME_MIN;
  if (reverse)
    _turnWanted = -_turnWanted;
  // brake means we need to stop in preparation for direction change
  if (brake)
    _thrustWanted = 0;
  Limit(_turnWanted,-maxTurn,+maxTurn);
  
  // wait with acceleration after the wheels were turned
  //if (_reverseTimeLeft>0 || _forwardTimeLeft>0)
  // always allow trust applied in opposite direction to movement (i.e. braking)
  _turningWheels = 0;
  if (_thrustWanted*FutureVisualState().ModelSpeed().Z()>=0)
  {
    // we might want to check _reverseTimeLeft and _forwardTimeLeft here
    static float accelerateAfterTurned = 2.0f;
    float maxThrust = 1-fabs(_turnWanted-_turn)*accelerateAfterTurned;
    saturate(maxThrust,0,1);
    Limit(_thrustWanted,-maxThrust,maxThrust);
    // do not update _reverseTimeLeft / _forwardTimeLeft while doing this
    _turningWheels = 1-maxThrust;
  }
  
  // limit turn based on speed (to avoid slips)

  Limit(_thrustWanted,-1,1);

  if (fabs(speed[2])<5)
  { // may be switching from/to reverse
    if (fabs(_turnWanted-_turn)>0.6 || _turnWanted*_turn<0 && fabs(_turnWanted-_turn)>0.2)
      saturate(info.speedWanted,-0.6,+0.6);
  }

  _pilotBrake=fabs(info.speedWanted)<0.1 || fabs(info.speedWanted)<fabs(speed[2])-5;

  /*
  if( _pilotBrake )
    LogF("speed %.1f->%.1f",speed[2],speedWanted);
  */
}

#endif // _ENABLE_AI

Vector3 Car::SteerPoint(float spdTime, float costTime, Ref<PathAction> *action, OLink(Object) *building, Clearance *clearance, bool updateCost)
{
  AIBrain *unit = PilotUnit();
  if (!unit) return FutureVisualState().Position();
  Path &path = unit->GetPath();
  // no path - no steering
  if (path.Size() < 2) return FutureVisualState().Position() + FutureVisualState().Direction();

  // for cars, calculate steer point based on distances, not on costs
  // (cars are unable to turn on no distance)
  Vector3 relAccel = DirectionWorldToModel(FutureVisualState()._acceleration);
  // simulated distance
  float distance = FutureVisualState().ModelSpeed().Z() * spdTime + 0.5 * relAccel.Z() * spdTime * spdTime;
  // constant distance
  distance +=  (100.0f/3.6f) * costTime;

  // find the position on path, <distance> from the current position

  return path.PosAtDist(FutureVisualState().Position(), distance);
}

FieldCost CarType::GetTypeCost(OperItemType type, CombatMode mode, Clearance clearance) const
{
  // FIX: bushes are passable for cars, avoid them in Stealth mode
  static const float costs[] =
  {
    1.0, // OITNormal,
    1.0, 3.0, 3.0, //OITAvoidBush, OITAvoidTree, OITAvoid
    SET_UNACCESSIBLE,  // OITWater,
    SET_UNACCESSIBLE, // OITSpaceRoad
    0.5, //  OITRoad
    1.0, // OITSpaceBush
    1.5, // OITSpaceHardBush
    SET_UNACCESSIBLE, // OITSpaceTree
    SET_UNACCESSIBLE, // OITSpace
    0.5 // OITRoadForced
  };
  static const float costsStealth[] =
  {
    1.0, // OITNormal,
    1.0, 3.0, 3.0, //OITAvoidBush, OITAvoidTree, OITAvoid
    SET_UNACCESSIBLE,  // OITWater,
    SET_UNACCESSIBLE, // OITSpaceRoad
    0.5, //  OITRoad
    2.0, // OITSpaceBush
    10.0, // OITSpaceHardBush
    SET_UNACCESSIBLE, // OITSpaceTree
    SET_UNACCESSIBLE, // OITSpace
    0.5 // OITRoadForced
  };

  Assert(lenof(costs) == NOperItemType);
  Assert(lenof(costsStealth) == NOperItemType);

  // floating cars
  if (type == OITWater && _canFloat)
    // we avoid water somewhat more than its speed would dictate - we do not want to get wet
    return FieldCost(2,1);
  // light cars
  if (type == OITSpaceHardBush && (!_shape || _shape->Mass() < 3000))
    return FieldCost(SET_UNACCESSIBLE);
  return mode == CMSafe || mode == CMStealth ? FieldCost(costsStealth[type]) : FieldCost(costs[type]);
}

const float RoadFaster=2.0;

static const float ObjPenalty1[]={1.0,1.05,1.1,1.4};

static const float ObjPenalty2[]={1.0,1.20,1.5,2.0};
static const float ObjRoadPenalty2[]={1.0,1.10,1.2,1.4};

static const float RoadPenalty[4]={0.25f,0.5f,1.0f,1.2f};

float CarType::GetFieldCost(const GeographyInfo &info, CombatMode mode) const
{
  // road fields are expected to be faster
  // fields with objects will be passed through slower
  int nObj=info.u.howManyObjects;
  Assert( nObj<=3 );
  if (info.u.road)
  {
    int modeIndex = mode-CMSafe;
    saturate(modeIndex, 0, 3);
    return RoadPenalty[modeIndex]*ObjRoadPenalty2[nObj];
  }
  else
    return ObjPenalty2[nObj];
}

float CarType::GetBaseCost(const GeographyInfo &geogr, bool operative, bool includeGradient) const
{
  float cost=GetMinCost()*RoadFaster;
  // avoid any water
  // when finding strategic path, we want to consider any field with a road as accessible
  if (geogr.u.maxWaterDepth>0 && (!geogr.u.road || operative))
  {
    if (geogr.u.minWaterDepth>=2 || operative)
      return _canFloat ? 1.0f/2 : 1e30f; // in water - speed 2 m/s
    else
    {
      // assume fording - shallow water
      cost = _canFloat ? 1.0f/6 : 1.0f/0.5f; // in water - speed 6 m/s for floating, 0.5 m/s for other
      // with fording we need to pass through to slope handling
    }
  }
  // avoid forests
  if (geogr.u.full)
    return 1e30f;
  // penalty for objects
  int nObj = geogr.u.howManyObjects;
  Assert(nObj <= 3);
  cost *= ObjPenalty1[nObj];
  if (includeGradient)
  {
    // avoid steep hills
    // penalty for hills
    int grad = geogr.u.gradient;
    if (grad>=6)
      return 1e30f;
    //static const float gradPenalty[6]={1.0,1.05,1.1,1.5,2.0,3.0};
    static const float gradPenalty[6]={1.0f,1.02f,1.05f,1.1f,2.0f,3.0f};
    cost *= gradPenalty[grad];
  }
  return cost;
}

float CarType::GetGradientPenalty(float gradient) const
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

/// linear interpolation of short table into a longer one
/**
We control the values for 16 direction. Game calculates with 256 directions.

Normal range is 0..128, but sometimes larger value may make sense, like when paying debths

*/
class CarTurnCostTable
{
  static const float _turnTime8[17];
  static const float _turnAng8[17];
  float _turnTime128[257];
public:  
  float operator()(int index) const {return _turnTime128[index];}
  CarTurnCostTable()
  {
    for (int i = 0; i < lenof(_turnTime128); i++)
      // perform interpolation of source values
      _turnTime128[i] = Lint(_turnAng8, _turnTime8, 17, i);
  }
} CarTurnCost;

/**
function should be linear in range 0..45 deg (0..2),
so that direction oscillation is not too much expensive
*/

const float CarTurnCostTable::_turnTime8[17] =
{
  0,0.5f,1.0f,5.0f,10.0f,15.0f,20.0f,25.0f,30.0f,
  35.0f*9/8,40.0f*10/8,45.0f*11/8,50.0f*12/8,
  55.0f*13/8,60.0f*14/8,65.0f*15/8,70.0f*16/8,
};

const float CarTurnCostTable::_turnAng8[17] =
{
  0,1*128/8,2*128/8,3*128/8,4*128/8,
  5*128/8,6*128/8,7*128/8,8*128/8,
  9*128/8,10*128/8,11*128/8,12*128/8,
  13*128/8,14*128/8,15*128/8,16*128/8
};

float CarType::GetCostTurn(int difDir, int &debet, bool forceFullTurn) const
{
  // direction change can be anything -128..127
  // in sec
  // this is time needed to turn on one OperItemGrid
  // time to turn on one 40/16
  int realDir = difDir + debet;
  
  if (!forceFullTurn)
  {
    // new way - store and pay debet
    float cost = 0.0f;
    if ((difDir * debet) >= 0 && debet != 0)
    {
      // not oscillating direction - pay the debet
      int totalToPay = abs(debet);
      int toPay = intMin(totalToPay,_maxTurnOneStep);
      debet -= (debet > 0) ? toPay : -toPay;
      
      // ..... 
      cost += CarTurnCost(totalToPay)-CarTurnCost(totalToPay-toPay);
    }
    debet += difDir;
    const int maxAllowedDebet = 32; //45 deg   
    if (abs(debet) > maxAllowedDebet) // debet too high
    {
      int toPay = abs(debet) - maxAllowedDebet;
      debet -= (debet > 0) ? toPay : -toPay;
      cost += CarTurnCost(toPay + maxAllowedDebet) - CarTurnCost(maxAllowedDebet);
    }
    float preferOneDir = (realDir < 0) ? 0.8f : 1.0f;
    return cost * preferOneDir;
  }
  else
  {
    int aDir = abs(realDir);
    // we might consider _turnCoef as well
    float cost = CarTurnCost(aDir);
    debet = 0;
    // prefer turning in one direction
    if (difDir < 0)
      return cost * 0.8f;
    else
      return cost;
  }
}

float CarType::GetPathCost(const GeographyInfo &geogr, float dist, CombatMode mode) const
{
  // cost based only on distance
  float cost = GetMinCost();
  // avoid any water
  // penalty for objects
  int nObj = geogr.u.howManyObjects;
  Assert(nObj<=3);
  cost *= ObjRoadPenalty2[nObj];
  return cost * dist;
}

Matrix4 Car::InternalCameraTransform(Matrix4 &baseTr, CameraType camType) const
{
  //rotation using _turn not used here as it is already done in Car::InsideCamera
  return base::InternalCameraTransform(baseTr, camType);
}

Vector3 Car::LookIntoTurn(CameraType camType) const
{
  float turnEff = floatMinMax(RenderVisualState()._turnBySpeed,-MaxTurnBySpeed,+MaxTurnBySpeed);
  float angle = turnEff*TurnCursorSideCoef;
  // check if the cursor would be out of the screen
  float fov = GetCameraFOV();

  AspectSettings asp;
  GEngine->GetAspectSettings(asp);

  // how much do we need to rotate so that the cursor is on screen?
  float fovAngle = atan(fov*asp.leftFOV)*0.90f;
  float out = 0;
  if (fabs(angle)>fovAngle) out = angle-fovAngle*fSign(angle);
  return Matrix3(MRotationY,-out).Direction();

}

Vector3 Car::GetCameraDirection(CameraType camType) const
{
  return base::GetCameraDirection(camType);
}

float Car::GetEngineVol(float &freq) const
{
  freq = (_randomizer * 0.05f + 0.95f) * FutureVisualState()._rpm * 1.2f;
  return fabs(_thrust) * 0.5f + 0.5f;
}

float Car::GetEnvironVol(float &freq) const
{
  freq = 1.0f;
  return FutureVisualState()._speed.SquareSize() / Square(Type()->GetMaxSpeedMs());
}

bool Car::IsHitEnough() const
{
  if(!_allowCrewInImmobile)
  {
    const float enoughHit = 0.9f;
    float lWheelHit = floatMax(GetHit(Type()->_wheelLFHit), GetHit(Type()->_wheelLBHit));
    float rWheelHit = floatMax(GetHit(Type()->_wheelRFHit), GetHit(Type()->_wheelRBHit));
    float engineHit = GetHit(Type()->_engineHit);
    return (lWheelHit >= enoughHit) || (rWheelHit >= enoughHit) || (engineHit >= enoughHit);
  }
  else return false;
}

bool Car::IsCautious() const
{
  AIBrain *unit = PilotUnit();
  if (!unit)
    return false;
  CombatMode mode = unit->GetCombatMode();
  return mode == CMStealth || mode == CMCombat ||(mode == CMAware && Type()->HoldOffroadFormation());
}

bool Car::BlowHorn(const TurretContext &context, const WeaponModePtr &ptr)
{
  if (!_hornSound)
  {
    const MuzzleType *muzzle = ptr.GetMuzzleType();
    if (muzzle)
    {
      AbstractWave *sound = GSoundScene->OpenAndPlayOnce(muzzle->_sound.name, this, false, FutureVisualState().Position(), FutureVisualState().Speed(),
        muzzle->_sound.vol, muzzle->_sound.freq, muzzle->_sound.distance);
      if (sound)
      {
        GSoundScene->SimulateSpeedOfSound(sound);
        GSoundScene->AddSound(sound);
        _hornSound = sound;
        GetNetworkManager().PlaySound(muzzle->_sound.name, FutureVisualState().Position(), FutureVisualState().Speed(), muzzle->_sound.vol, muzzle->_sound.freq, _hornSound);
      }
    }
    return true;
  }
  return false;
}

void Car::Sound(bool inside, float deltaT)
{
  PROFILE_SCOPE_EX(snCar,sound);
  float gearVol = Type()->_gearSound.vol;
  const VisualState &vs = RenderVisualState();
  if (_doGearSound && !_gearSound)
  {
    _doGearSound = false;
    // check if the sound can be hard at all
    if (inside || GSoundScene->CanBeAudible(WaveEffect,vs.Position(),gearVol))
    {
      AbstractWave *sound = GSoundScene->OpenAndPlayOnce(Type()->_gearSound.name, this, true, vs.Position(), vs.Speed());
      _gearSound = sound;
      if (sound)
      {
        GSoundScene->SimulateSpeedOfSound(sound);
        GSoundScene->AddSound(sound);
      }
    }
  }

  if (_gearSound)
  {
    float obstruction, occlusion;
    GWorld->CheckSoundObstruction(this, true, obstruction, occlusion);
    _gearSound->SetVolume(gearVol); // volume, frequency
    _gearSound->SetObstruction(obstruction, occlusion, deltaT);
    _gearSound->SetPosition(vs.Position(), vs.Speed());
    _gearSound->Set3D(!inside);
  }
  if (_hornSound)
  {
    float obstruction, occlusion;
    GWorld->CheckSoundObstruction(this, false, obstruction, occlusion);
    _hornSound->SetPosition(vs.Position(), vs.Speed());
    _hornSound->SetObstruction(obstruction, occlusion, deltaT);
    _hornSound->Set3D(!inside);
  }
  if (vs._scudState >= 1 && vs._scudState < 2)
  {
    const SoundPars &sound = Type()->_scudSoundElevate;
    if (!_scudSound && sound.name.GetLength() > 0)
    {
      _scudSound = GSoundScene->OpenAndPlay(sound.name, this, false, vs.PositionModelToWorld(_scudPos), _scudSpeed, sound.vol, sound.freq, sound.distance);
      if (_scudSound)
        _scudSound->SetVolume(sound.vol, sound.freq);
    }
  }
  else if (vs._scudState >= 3 && vs._scudState < 4)
  {
    const SoundPars &sound = Type()->_scudSound;
    if (!_scudSound && sound.name.GetLength() > 0)
    {
      _scudSound = GSoundScene->OpenAndPlay(sound.name, this, false, vs.PositionModelToWorld(_scudPos), _scudSpeed, sound.vol, sound.freq, sound.distance);
      if (_scudSound)
        _scudSound->SetVolume(sound.vol, sound.freq);
    }
  }
  else _scudSound.Free();

  if (_scudSound)
    _scudSound->SetPosition(vs.PositionModelToWorld(_scudPos), _scudSpeed);

  base::Sound(inside,deltaT);
}

void Car::UnloadSound()
{
  base::UnloadSound();
  _scudSound.Free();
}

Matrix4 Car::InsideCamera(CameraType camType) const
{
  Matrix4 transf;
  if (camType == CamGunner && GetOpticsCamera(transf, camType))
    return transf;
  if (!GetProxyCamera(transf, camType))
  {
    Vector3 pos = Type()->_pilotPos;
    transf.SetTranslation(pos);
  }
  return transf;
}

bool Car::HasFlares(CameraType camType) const
{
  if (camType==CamGunner || camType==CamInternal)
    return false;
  return base::HasFlares(camType);
}

float CarVisualState::GetSCUD() const
{
  if (_scudState<1) return 0;
  if (_scudState<2) return _scudState-1;
  if (_scudState<3) return 1;
  return _scudState-2;
}

void Car::Draw(int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
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

void Car::PrepareProxiesForDrawing(Object *rootObj, int rootLevel, const AnimationContext &animContext,
  const PositionRender &transform, int level, ClipFlags clipFlags, SortObject *so)
{
  float dist2 = so->distance2;
  const VisualState &vs = RenderVisualState();
  for (int i=0; i<GetType()->GetProxyCountForLevel(level); i++)
  {
    const ProxyObjectTyped &proxy = GetType()->GetProxyForLevel(level,i);
    Object *obj = proxy.obj;
    const EntityType *type = obj->GetEntityType();
    if (type && !strcmp(type->_simName, "scud") && vs._scudState < 4)
    {
      Matrix4 proxyTransform = obj->RenderVisualState().Transform();
      SkeletonIndex boneIndex = Type()->GetProxyBoneForLevel(level,i);
      AnimateBoneMatrix(proxyTransform, vs, level, boneIndex);

      // smart clipping par of obj->Draw
      Matrix4Val pTransform = transform * proxyTransform;

      // LOD detection
      LODShape *pshape = NULL;
      // preload fire model, but until loaded, use the original one
      if (vs._scudState >= 3)
        pshape = Type()->_scudModelFire;
      else
      {
        pshape = Type()->_scudModel;
        // before actually using it, preload the fire model
        if (Type()->_scudModelFire && vs._scudState>=1.90f)
        {
          LODShape *preload = Type()->_scudModelFire;
          int level = GScene->LevelFromDistance2(preload, dist2, pTransform.Scale());
          if (level != LOD_INVISIBLE)
          {
            ShapeUsed shape = preload->Level(level);
            obj->PrepareTextures(NULL, preload, dist2, level, false);
          }
        }
      }
      if (pshape)
      {
        int level = GScene->LevelFromDistance2(pshape, dist2, pTransform.Scale());
        if (level != LOD_INVISIBLE)
        {
          Ref<Object> object = new Object(pshape, VISITOR_NO_ID);
          GScene->TempProxyForDrawing(object, rootObj, level, rootLevel, clipFlags, dist2, pTransform);
        }
      }
    }
  }
  base::PrepareProxiesForDrawing(rootObj, rootLevel, animContext, transform, level, clipFlags, so);
}

bool Car::GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const
{
  switch (action->GetType())
  {
    case ATScudLaunch:
    case ATScudStart:
    case ATScudCancel:
      // No parameters
      return true;
  }
  return base::GetActionParams(params, action, unit);
}

void Car::PerformAction(const Action *action, AIBrain *unit)
{
  switch (action->GetType())
  {
    case ATScudLaunch:
      // if we are recovering from a cancel, do not change state
      if (FutureVisualState()._scudState<1) FutureVisualState()._scudState = 1;
      _scudStateDir = +0.1;
      break;
    case ATScudStart:
      FutureVisualState()._scudState = 3.0f;
      _scudStateDir = +0.1;
      break;
    case ATScudCancel:
      if (FutureVisualState()._scudState>2)
        FutureVisualState()._scudState = 2;
      // cancel should be done during "launch" state, 1<=_scudState<=2
      _scudStateDir = -0.1;
      break;
    default:
      base::PerformAction(action, unit);
      break;
  }
}

void Car::GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit)
{
  base::GetActions(add, unit);
  if (!unit)
    return;
  if (unit == CommanderUnit() && unit->LSCanProcessActions())
  {
    if (Type()->_scudModel)
    {
      if (FutureVisualState()._scudState < 1.0)
      {
        UIAction action(new ActionBasic(ATScudLaunch, this));
        add(action);
      }
      else if (FutureVisualState()._scudState <= 2.0)
      {
        if (_scudStateDir>0)
        {
          UIAction action(new ActionBasic(ATScudCancel, this));
          add(action);
        }
        else
        {
          UIAction action(new ActionBasic(ATScudLaunch, this));
          add(action);
        }
      }
      if (FutureVisualState()._scudState >= 2.0 && FutureVisualState()._scudState < 3.0)
      {
        {
          UIAction action(new ActionBasic(ATScudStart, this));
          add(action);
        }
        {
          UIAction action(new ActionBasic(ATScudCancel, this));
          add(action);
        }
      }
    }
  }
}

float Car::GetHitForDisplay(int kind) const
{
  // see InGameUI::DrawTankDirection
  switch (kind)
  {
  case 0: return GetHitCont(Type()->_bodyHit);
  case 1: return GetHitCont(Type()->_engineHit);
  case 2:return GetHitCont(Type()->_fuelHit);
  case 3:
    {//if one side is destroyed, car is useless
      float hit = floatMax(
        floatMax(GetHitCont(Type()->_wheelLF2Hit), GetHitCont(Type()->_wheelLFHit)) + 
        floatMax(GetHitCont(Type()->_wheelLMHit), GetHitCont(Type()->_wheelLBHit)),  
        floatMax(GetHitCont(Type()->_wheelRF2Hit), GetHitCont(Type()->_wheelRFHit)) + 
        floatMax(GetHitCont(Type()->_wheelRMHit), GetHitCont(Type()->_wheelRBHit))
        )*0.5f;
      return hit;
    }
  default: return 0;
  }
}


#define SERIAL_DEF_VISUALSTATE(name, value) CHECK(ar.Serialize(#name, FutureVisualState()._##name, 1, value))

LSError Car::Serialize(ParamArchive &ar)
{
  SERIAL_BASE;
  
  if (!IS_UNIT_STATUS_BRANCH(ar.GetArVersion()))
  {
    SERIAL_DEF_VISUALSTATE(scudState, 0);
    SERIAL_DEF(thrustWanted, 0);
    SERIAL_DEF(thrust, 0);
    SERIAL_DEF(turnWanted, 0);
    SERIAL_DEF(turn, 0);
    SERIAL_DEF_VISUALSTATE(turnBySpeed, 0);
    SERIAL_DEF(turnSpeedWanted, 0);

    CHECK(::Serialize(ar, "scudPos", _scudPos, 1, VZero))
    CHECK(::Serialize(ar, "scudSpeed", _scudSpeed, 1, VZero))

    SERIAL_DEF(reverseNeeded, false);
    SERIAL_DEF(reverseTimeLeft, 0);
    SERIAL_DEF(forwardTimeLeft, 0);
  }
  return LSOK;
}

#undef SERIAL_DEF_VISUALSTATE


#define CAR_MSG_LIST(XX) \
  XX(UpdateGeneric, UpdateCar) \
  XX(UpdatePosition, UpdatePositionCar)

DEFINE_NETWORK_OBJECT(Car, base, CAR_MSG_LIST)

#define UPDATE_CAR_MSG(MessageName, XX) \
  XX(MessageName, RString, plateNumber, NDTString, RString, NCTNone, DEFVALUE(RString, "XXXXXXXX"), DOC_MSG("Plate number"), TRANSF, ET_NOT_EQUAL, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, scudState, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Scud launcher state"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR)

DECLARE_NET_INDICES_EX_ERR(UpdateCar, UpdateTankOrCar, UPDATE_CAR_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdateCar, UpdateTankOrCar, UPDATE_CAR_MSG, NoErrorInitialFunc)

#define UPDATE_POSITION_CAR_MSG(MessageName, XX) \
  XX(MessageName, float, rpmWanted, NDTFloat, float, NCTFloat0To2, DEFVALUE(float, 0), DOC_MSG("Wanted value of RPM"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, thrustWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted thrust"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, turnWanted, NDTFloat, float, NCTFloatM1ToP1, DEFVALUE(float, 0), DOC_MSG("Wanted turning angle"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR)

DECLARE_NET_INDICES_EX_ERR(UpdatePositionCar, UpdatePositionVehicle, UPDATE_POSITION_CAR_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdatePositionCar, UpdatePositionVehicle, UPDATE_POSITION_CAR_MSG, NoErrorInitialFunc)

NetworkMessageFormat &Car::CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format)
{
  switch (cls)
  {
  case NMCUpdateGeneric:
    base::CreateFormat(cls, format);
    UPDATE_CAR_MSG(UpdateCar, MSG_FORMAT_ERR)
    break;
  case NMCUpdatePosition:
    base::CreateFormat(cls, format);
    UPDATE_POSITION_CAR_MSG(UpdatePositionCar, MSG_FORMAT_ERR)
    break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError Car::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    TMCHECK(base::TransferMsg(ctx))
    {
      PREPARE_TRANSFER(UpdateCar)

      TRANSF(plateNumber)
      TRANSF_EX(scudState, FutureVisualState()._scudState)  // TODO: check whether FutureVS is correct
    }
    break;
  case NMCUpdatePosition:
    {
      PREPARE_TRANSFER(UpdatePositionCar)

      Matrix3 oldTrans = FutureVisualState().Orientation();
      TMCHECK(base::TransferMsg(ctx))
      StabilizeTurrets(oldTrans, FutureVisualState().Orientation());
      TRANSF(rpmWanted)
      TRANSF(thrustWanted)
      TRANSF(turnWanted)
      if (!ctx.IsSending() && _rpmWanted>0.5f && _thrustWanted>=0.2f && fabs(FutureVisualState().ModelSpeed().Z())>2.0f)
      {
        // once moving fast and still thrusting, we can be sure the car is no longer braking
        // this is done to reduce error caused by remote simulation using out of sync parameters
        _pilotBrake = false;
      }
    }
    break;
  default:
    return base::TransferMsg(ctx);
  }
  return TMOK;
}

float Car::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = 0;
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    error += base::CalculateError(ctx);
    {
      PREPARE_TRANSFER(UpdateCar)

      ICALCERR_NEQSTR(plateNumber, ERR_COEF_VALUE_MAJOR)
      error += ERR_COEF_VALUE_MAJOR * fabs(message->_scudState - FutureVisualState()._scudState);  // need to trampoline to currentVisualState
    }
    break;
  case NMCUpdatePosition:
    {
      error +=  base::CalculateError(ctx);

      PREPARE_TRANSFER(UpdatePositionCar)

      ICALCERR_ABSDIF(float, rpmWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, thrustWanted, ERR_COEF_VALUE_MAJOR)
      ICALCERR_ABSDIF(float, turnWanted, ERR_COEF_VALUE_MAJOR)
    }
    break;
  default:
    error += base::CalculateError(ctx);
    break;
  }
  return error;
}

void Car::CamIntExtSwitched()
{
  Person *player=GWorld->PlayerOn();
  AIBrain *unit=( player ? player->CommanderUnit() : NULL );
  if (!unit || unit->GetVehicle()!=this)
    return;
  player->ResetXRotWanted();
}

Vector3 Car::ExternalCameraPosition( CameraType camType ) const
{
  return (camType == CamGroup) ? Type()->_groupCameraPosition : Type()->_extCameraPosition;
}

bool Car::IsGunner( CameraType camType ) const
{
  return camType==CamGunner || camType==CamInternal || camType==CamExternal;
}
