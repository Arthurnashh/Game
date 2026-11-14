// Poseidon - vehicle class
// (C) 1997, Suma
#include "wpch.hpp"

#include "AI/ai.hpp"
#include "tankOrCar.hpp"
#include "scene.hpp"
#include "global.hpp"
#include "landscape.hpp"
#include "keyInput.hpp"
#include "Network/network.hpp"
#include <El/Common/randomGen.hpp>

#include "Shape/specLods.hpp"
#include "diagModes.hpp"

void TankOrCarType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par,shape);
  _canFloat = par>>"canFloat";

  ConstParamEntryPtr inputTurnCurve = par.FindEntry("inputTurnCurve");
  if (inputTurnCurve.NotNull())
    _inputTurnCurve.Load(*inputTurnCurve);

  _leftDust =  par >> "leftDustEffect";
  _rightDust =  par >> "rightDustEffect";
  _leftWater =  par >> "leftWaterEffect";
  _rightWater =  par >> "rightWaterEffect";

  _tracksSpeed = par >> "tracksSpeed";
}

void TankOrCarType::GetPathPrecision(float speedAtCost, float &safeDist, float &badDist) const
{
  // for big and hard vehicles the path precision required is usually much lower
  float massFactor = InterpolativC(_shape->Mass(),2000.0f,10000.0f,1.0f,3.0f);

  base::GetPathPrecision(speedAtCost,safeDist,badDist);

  safeDist *= massFactor;
  badDist *= massFactor;
}

static const char *GetSelectionName(ParamEntryPar entry)
{
  if (entry.IsArray())
  {
    RString name = entry[0];
    return name;
  }

  RString name = entry;
  return name;
}

static const char *GetSelectionAltName(ParamEntryPar entry)
{
  if (entry.IsArray())
  {
    RString name = entry[1];
    return name;
  }
  return NULL;
}

#define SELECTION_NAMES(entry) GetSelectionName(entry), GetSelectionAltName(entry)



TankOrCarVisualState::TankOrCarVisualState(TankOrCarType const& type)
:base(type),
_rpm(0),_phaseL(0),_phaseR(0)
{
// empty
}



void TankOrCarVisualState::Interpolate(Object *obj, ObjectVisualState const& t1state, float t, ObjectVisualState& interpolatedResult) const
{
  base::Interpolate(obj, t1state, t, interpolatedResult);

  TankOrCarVisualState const& s = static_cast<TankOrCarVisualState const&>(t1state);
  TankOrCarVisualState& res = static_cast<TankOrCarVisualState&>(interpolatedResult);

#define INTERPOLATE(x) res.x = x + t*(s.x - x)

  INTERPOLATE(_rpm);  // continuous
  INTERPOLATE(_phaseL);  // continuous
  INTERPOLATE(_phaseR);  // continuous

#undef INTERPOLATE
}



void TankOrCarType::InitShape()
{
  base::InitShape();

  const ParamEntry &par=*_par;

  // track animations - left / right track are switched in program
  _leftOffset.Init(_shape, (par >> "selectionRightOffset").operator RString());
  _rightOffset.Init(_shape, (par >> "selectionLeftOffset").operator RString());

  RString brakeLights = par>>"selectionBrakeLights";
  _brakeLights.Init(_shape,brakeLights,NULL);

#if _VBS2 // convoy trainer, indicators
  ParamEntryPtr entry = par.FindEntry("selectionLeftIndicator");
  _hasIndicators = bool(entry);

  if(_hasIndicators)
  {
    RString leftIndicator = par>>"selectionLeftIndicator"; 
    _leftIndicators.Init(_shape,leftIndicator,NULL);

    RString rightIndicator = par>>"selectionRightIndicator";
    _rightIndicators.Init(_shape,rightIndicator,NULL);

    RString reverseIndicator = par>>"selectionReverseLights";
    _reverseLights.Init(_shape,reverseIndicator,NULL);
  }
#endif

  // weapon directions and positions
  Point3 beg,end;
  beg=_shape->MemoryPoint(SELECTION_NAMES(par >> "memoryPointMissile"));
  end=_shape->MemoryPoint(SELECTION_NAMES(par >> "memoryPointMissileDir"));
  _missilePos=(beg+end)*0.5f;
  _missileDir=(beg-end);
  _missileDir.Normalize();

  GetValue(_gearSound, par>>"soundGear");

  if(par.FindEntry("memoryPointCM"))
  {
    int n = (par >> "memoryPointCM").GetSize();
    _cmPos.Resize(n);
    for (int i=0; i<n; i++)
    {
      RString cmPos = (par >> "memoryPointCM")[i];
      _cmPos[i] = _shape->MemoryPoint(cmPos);
    }
  }
  if (_cmPos.Size() == 0) _cmPos.Add(Vector3(0,0,0));

  if(par.FindEntry("memoryPointCMDir"))
  {
    int n = (par >> "memoryPointCMDir").GetSize();
    _cmDir.Resize(n);
    for (int i=0; i<n; i++)
    {
      RString cmDir = (par >> "memoryPointCMDir")[i];
      _cmDir[i] = _shape->MemoryPoint(cmDir);
    }
  }
  if (_cmDir.Size() == 0) _cmDir.Add(Vector3(1,0,0));

  _exhausts.Clear();
  ParamEntryVal array = par >> "Exhausts";
  for (int i=0; i<array.GetEntryCount(); i++)
  {
    ParamEntryVal cls = array.GetEntry(i);
    RString pos = cls >> "position";
    if (!_shape->MemoryPointExists(pos)) continue;
    Vector3 beg = _shape->MemoryPoint(pos);
    Vector3 end = _shape->MemoryPoint((cls >> "direction").operator RString());
    int index = _exhausts.Add();
    _exhausts[index].position = end;
    _exhausts[index].direction = (end - beg).Normalized();
    _exhausts[index].effect = cls >> "effect";
  }
  _exhausts.Compact();

  _engineHit = FindHitPoint("HitEngine");

  //memory points for car, tank or ship water effects leftWaterEffect and rightWaterEffect
  _lWaterPos=_shape->MemoryPoint((par >> "memoryPointsLeftWaterEffect").operator RString());
  _rWaterPos=_shape->MemoryPoint((par >> "memoryPointsRightWaterEffect").operator RString());
}

void TankOrCarType::InitShapeLevel(int level)
{
  base::InitShapeLevel(level);

  _leftOffset.InitLevel(_shape,level);
  _rightOffset.InitLevel(_shape,level);

  _brakeLights.InitLevel(_shape,level);

#if _VBS2 // convoy trainer, indicators
  if(_hasIndicators)
  {
    _leftIndicators.InitLevel(_shape,level);
    _rightIndicators.InitLevel(_shape,level);
    _reverseLights.InitLevel(_shape,level);
  }
#endif

  if (level==_shape->FindLevel(VIEW_PILOT))
  {
    ShapeUsed oShape=_shape->Level(level);
    oShape->MakeCockpit();
  }
  if (level==_shape->FindLevel(VIEW_CARGO))
  {
    ShapeUsed oShape=_shape->Level(level);
    oShape->MakeCockpit();
  }
}

void TankOrCarType::DeinitShapeLevel(int level)
{
  _brakeLights.DeinitLevel(_shape,level);

  _leftOffset.DeinitLevel(_shape,level);
  _rightOffset.DeinitLevel(_shape,level);

#if _VBS2 // convoy trainer, indicators
  if(_hasIndicators)
  {
    _leftIndicators.DeinitLevel(_shape,level);
    _rightIndicators.DeinitLevel(_shape,level);
    _reverseLights.DeinitLevel(_shape,level);
  }
#endif
  base::DeinitShapeLevel(level);
}

AnimationSource *TankOrCarType::CreateAnimationSource(const AnimationType *type, RStringB source)
{
  if (!strcmpi(source,"wheelL"))
    return _animSources.CreateAnimationSource(&TankOrCar::GetCtrlWheelLRot);
  if (!strcmpi(source,"wheelR"))
    return _animSources.CreateAnimationSource(&TankOrCar::GetCtrlWheelRRot);
  return base::CreateAnimationSource(type,source);
}

TankOrCar::TankOrCar(const EntityAIType *name, Person *driver, bool fullCreate)
:base(name,driver,fullCreate),_pilotBrake(false),
_fireDustTimeLeft(0),_rpmWanted(0),
_invFireDustTimeTotal(1),
_lastPilotBrake(Glob.time-60),_cursorY(0.67f)
#if _VBS2// convoy trainer, indicators
,
_leftIndicatorOn(false),
_leftIndicate(true),
_rightIndicatorOn(false),
_rightIndicate(true),
_hazardLightsOn(false)
#endif
{
  int n = Type()->_exhausts.Size();
  if (n > 0)
  {
    _exhausts.Realloc(n);
    _exhausts.Resize(n);
    for (int i=0; i<n; i++)
    {
      RString name = Type()->_exhausts[i].effect;
      _exhausts[i].Init(Pars >> name, this, EVarSet_Exhaust);
    }
  }
  _leftDust.Init(Pars >> Type()->_leftDust, this, EVarSet_Dust);
  _rightDust.Init(Pars >> Type()->_rightDust, this, EVarSet_Dust);
  _leftWater.Init(Pars >> Type()->_leftWater, this, EVarSet_Water);
  _rightWater.Init(Pars >> Type()->_rightWater, this, EVarSet_Water);

  _cmIndexToggle = 0;
}

#if _VBS2
#include "soldierOld.hpp"
#endif

bool TankOrCar::AnimateTexture(AnimationContext &animContext, int level, float phaseL, float phaseR)
{
  // animate tracks
  ShapeUsed shape=_shape->Level(level);
  if( shape.IsNull() ) return false;

  // use _phaseL to modify u,v mapping of the faces
  // we need some texture to be able to perform conversion to physical

  const TankOrCarType *type = Type();
  if (type->_tracksSpeed>0.00001 || type->_tracksSpeed<-0.00001)
  {
    type->_leftOffset.UVOffset(animContext, _shape, 0, fmod(-type->_tracksSpeed * phaseL, 1), level);
    type->_rightOffset.UVOffset(animContext, _shape, 0, fmod(-type->_tracksSpeed * phaseR, 1), level);
      }

  return false; // no alpha change
}

void TankOrCar::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  if( _shape->Level(level).IsNull() ) return;
  //float value = fabs(ModelSpeed()[2]);

#if _VBS2 // if the driver is using NVG, we don't want to use any lights
  Soldier* soldier = dyn_cast<Soldier>(Driver());
  bool LightsDisabled = soldier && 
    (
    (soldier->IsWearingNVG() && !soldier->IsNetworkPlayer() && soldier->Brain()->GetCombatMode() > CMAware)
    || (soldier->IsNetworkPlayer() && !IsPilotLight() && GScene->MainLight()->NightEffect() > 0.6)
    );

#endif
  // avoid flashing brake lights
  if (_pilotBrake && EngineIsOn())
    _lastPilotBrake = Glob.time;
  if( _lastPilotBrake>Glob.time-0.4
#if _VBS2
    && !LightsDisabled
#endif
    )
    Type()->_brakeLights.Unhide(animContext, _shape, level);
  else
    Type()->_brakeLights.Hide(animContext, _shape, level);

#if _VBS2 // convoy trainer, indicators
  if(Type()->_hasIndicators)
  {
    if( _leftIndicatorOn && !LightsDisabled)
    {
      if(_leftIndicate && !LightsDisabled)
        Type()->_leftIndicators.Unhide(animContext, _shape, level);			
      else
        Type()->_leftIndicators.Hide(animContext, _shape, level);

      if(_lastLeftIndicate < Glob.time - 0.4)
      {
        _lastLeftIndicate = Glob.time;
        _leftIndicate = !_leftIndicate;
      }
    }
    else
      Type()->_leftIndicators.Hide(animContext, _shape, level);

    if( _rightIndicatorOn && !LightsDisabled)
    {
      if(_rightIndicate && !LightsDisabled){
        Type()->_rightIndicators.Unhide(animContext, _shape, level);			
      }
      else
        Type()->_rightIndicators.Hide(animContext, _shape, level);

      if(_lastRightIndicate < Glob.time - 0.4)
      {
        _lastRightIndicate = Glob.time;
        _rightIndicate = !_rightIndicate;
      }
    }
    else
      Type()->_rightIndicators.Hide(animContext, _shape, level);

    if(EngineIsOn() && ModelSpeed()[2] < -0.1 && !LightsDisabled)
    {
      Type()->_reverseLights.Unhide(animContext, _shape, level);			
    }
    else
      Type()->_reverseLights.Hide(animContext, _shape, level);
  }
#endif
  AnimateTexture(animContext,level, animContext.GetVisualState().cast<TankOrCar>()._phaseL, animContext.GetVisualState().cast<TankOrCar>()._phaseR);  // Raw is used to bypass ProtectedVisualState check

  base::Animate(animContext, level, setEngineStuff, parentObject, dist2);
}

#if _VBS2 // convoy trainer, indicators
void TankOrCar::DoIndicators(bool indicateLeft,bool indicateRight,bool hazardLights)
{	
  if(!Type()->_hasIndicators)
    return;

  if (hazardLights)
  {
    _hazardLightsOn = !_hazardLightsOn;
    if (!_hazardLightsOn)
    {
      _leftIndicatorOn = false;
      _rightIndicatorOn = false;
      hazardLights = false;
    }
  }

  if (indicateLeft || hazardLights)
  {
    if (indicateLeft && _hazardLightsOn)
    {
      _hazardLightsOn = false;
      _leftIndicatorOn = false;
      _rightIndicatorOn = false;
    }

    // turn off right indicator
    if (!hazardLights)
      _rightIndicatorOn = false;		

    if (_leftIndicatorOn && !hazardLights)
      _leftIndicatorOn = false;
    else
    {
      _leftIndicatorOn = true;
      _leftIndicate = true;
      _lastLeftIndicate = Glob.time;
    }
  }

  if (indicateRight || hazardLights)
  {
    if (indicateRight && _hazardLightsOn)
    {
      _hazardLightsOn = false;
      _leftIndicatorOn = false;
      _rightIndicatorOn = false;
    }

    // turn off left indicator
    if (!hazardLights)
      _leftIndicatorOn = false;

    if (_rightIndicatorOn && !hazardLights)
      _rightIndicatorOn = false;
    else
    {
      _rightIndicatorOn = true;
      _rightIndicate = true;
      _lastRightIndicate = Glob.time;	
    }
  }
}
#endif
void TankOrCar::Deanimate(int level, bool setEngineStuff)
{
  if (_shape->Level(level).IsNull())
    return;
  base::Deanimate(level,setEngineStuff);
}

void TankOrCar::Simulate(float deltaT, SimulationImportance prec)
{
  if( _isDead )
  {
#if _VBS2
    _hazardLightsOn = false;
    _leftIndicatorOn = false;
    _rightIndicatorOn = false;
#endif
    IExplosion *smoke = GetSmoke();
    if (smoke)
    {
      float explosionDelay = GRandGen.Gauss(0.2f,0.5f,1.5f);
      Time explosionTime = Glob.time+explosionDelay;
      smoke->Explode(explosionTime);
    }
    NeverDestroy();
  }
  base::Simulate(deltaT,prec);
}

float TankOrCar::GetSteerAheadSimul() const
{
  float val = base::GetSteerAheadSimul();
  if (Type()->_canFloat && _waterContact && !_landContact)
    val *= 8.0f;
  return val;
}

float TankOrCar::GetSteerAheadPlan() const
{
  float val = base::GetSteerAheadPlan();
  if (Type()->_canFloat && _waterContact && !_landContact)
    val *= 8.0f;
  return val;
}

float TankOrCar::GetPrecision() const
{
  float val = base::GetPrecision();
  if (Type()->_canFloat && _waterContact)
    val *= 3.0f;
  return val;
}

void TankOrCar::SimulateExhaust(float deltaT, SimulationImportance prec)
{
  if( !EnableVisualEffects(prec) ) return;
  if (!EngineIsOn()) return;
  
  const VisualState &vs = FutureVisualState();
  // simulate exhaust smoke
  int n = _exhausts.Size();
  for (int i=0; i<n; i++)
  {
    // damage
    float damage=GetTotalDamage();

    // intensity
    float moreThrust=ThrustWanted()-Thrust();
    saturate(moreThrust,0,0.2);
    float intensity=Thrust()*0.3+moreThrust*3+0.1;
    if( _gearSound )
      // gear change smoke
      intensity=1;
    intensity+=damage*0.2;
    intensity*=1+damage;

    // Calculate smoke position in model space
    Vector3 exhaustPos = Type()->_exhausts[i].position;

    // Calculate smoke speed in world space
    Vector3 cSpeed;
    float exhaustSize = GetMass() * (1.0 / 16000);
    cSpeed = vs.DirectionModelToWorld(Type()->_exhausts[i].direction);
    cSpeed *= 9 * floatMin(1, 0.3 * exhaustSize);
    cSpeed += vs._speed;
    float exhaustValues[] = {intensity, damage, exhaustPos.X(), exhaustPos.Y(), exhaustPos.Z(), cSpeed.X(), cSpeed.Y(), cSpeed.Z()};
    _exhausts[i].Simulate(deltaT, prec, exhaustValues, lenof(exhaustValues));
  }
}


void TankOrCar::GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit)
{
  base::GetActions(add, unit);
  if (!unit) return;

  if (unit->GetVehicle() == this && CommanderUnit() == unit && !unit->IsInCargo())
  {
    // lights on / off
    if (unit->IsPlayer() && _lights.SomeLights())
    {
      if (IsPilotLight())
      {
        UIAction action(new ActionBasic(ATLightOff, this));
        add(action);
      }
      else
      {
        if (GScene->MainLight()->NightEffect() > 0)
        {
          UIAction action(new ActionBasic(ATLightOn, this));
          add(action);
        }
      }
    }
  }
}

#if _HELISIM_PRECISE_INTERACTION
void TankOrCar::GetDrawActions3D(UIActions &actions, AIBrain *unit)
{
  if (!unit) return;

  if (unit->GetVehicle() == this && CommanderUnit() == unit && !unit->IsInCargo())
  {
    // lights on / off
    if (unit->IsPlayer() && _reflectors.SomeLights())
    {
      if (IsPilotLight())
      {
        UIAction action(new ActionBasic(ATLightOff, this));
        actions.Add(action);
      }
      else
      {
        if (GScene->MainLight()->NightEffect() > 0)
        {
          UIAction action(new ActionBasic(ATLightOn, this));
          actions.Add(action);
        }
      }
    }
  }
  base::GetDrawActions3D(actions, unit);
}
#endif

void TankOrCar::GetAimActions(float &aimX, float &aimY) const
{
  aimX = GInput.GetActionWithCurve(InputInterface::CurveFovMouse, UACarAimRight, UACarAimLeft);
  aimY = GInput.GetActionWithCurve(InputInterface::CurveFovMouse, UACarAimUp, UACarAimDown);
}

#define TANK_OR_CAR_MSG_LIST(XX) \
  XX(UpdateGeneric, UpdateTankOrCar)

DEFINE_NETWORK_OBJECT(TankOrCar, base, TANK_OR_CAR_MSG_LIST)

DEFINE_NET_INDICES_EX_ERR(UpdateTankOrCar, UpdateTransport, UPDATE_TANK_OR_CAR_MSG, NoErrorInitialFunc)

NetworkMessageFormat &TankOrCar::CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format)
{
  switch (cls)
  {
  case NMCUpdateGeneric:
    base::CreateFormat(cls, format);
    UPDATE_TANK_OR_CAR_MSG(UpdateTankOrCar, MSG_FORMAT_ERR)
      break;
  default:
    base::CreateFormat(cls, format);
    break;
  }
  return format;
}

TMError TankOrCar::TransferMsg(NetworkMessageContext &ctx)
{
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    TMCHECK(base::TransferMsg(ctx))
    {
      PREPARE_TRANSFER(UpdateTankOrCar)
        UPDATE_TANK_OR_CAR_MSG(UpdateTankOrCar, MSG_TRANSFER_ERR)
    }
    break;
  default:
    return base::TransferMsg(ctx);
  }
  return TMOK;
}

float TankOrCar::CalculateError(NetworkMessageContextWithError &ctx)
{
  float error = 0;
  switch (ctx.GetClass())
  {
  case NMCUpdateGeneric:
    error += base::CalculateError(ctx);
    {
      PREPARE_TRANSFER(UpdateTankOrCar)

        ICALCERR_NEQ(bool, pilotBrake, ERR_COEF_MODE)

#if _VBS2
        ICALCERR_NEQ(bool, leftIndicatorOn, ERR_COEF_MODE)
        ICALCERR_NEQ(bool, rightIndicatorOn, ERR_COEF_MODE)
        ICALCERR_NEQ(bool, hazardLightsOn, ERR_COEF_MODE)
#endif

    }
    break;
  default:
    error += base::CalculateError(ctx);
    break;
  }
  return error;
}

bool TankOrCar::ProcessFireWeapon(const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine, EntityAI *target,
  const RemoteFireWeaponInfo *remoteInfo, RemoteFireWeaponInfo *localInfo, bool forceLock)
{
  const MagazineType *aInfo = magazine ? magazine->Type() : NULL;
  const AmmoType *ammo = ptr.GetAmmoType();
  TransportVisualState &vs = FutureVisualState();  // can't use the abstract TankOrCarVisualState
  if (!ammo)
    return false;
  if (localInfo)
  {
    localInfo->_position = vs.Position();
    localInfo->_visible=floatMax(ammo->visibleFire,ammo->audibleFire);
  }
  switch (ammo->_simulation)
  {
  case AmmoShotMissile:
    if (remoteInfo)
      return false;
    {
      Matrix4Val shootTrans = context._turretType ? GunTurretTransform(vs, *context._turretType) : M4Identity;
      Vector3 pos;
      if (context._turretType && context._turretType->_missileByTurret)
      {
        vs.DirectionModelToWorld(localInfo->_direction, shootTrans.Rotate(context._turretType->_missileDir));
        pos = shootTrans.FastTransform(context._turretType->_missilePos);
      }
      else
      {
        vs.DirectionModelToWorld(localInfo->_direction, shootTrans.Rotate(Type()->_missileDir));
        pos = shootTrans.FastTransform(Type()->_missilePos);
      }
      return FireMissile(context, ptr, pos, localInfo->_direction, shootTrans.Rotate(Vector3(0,0,aInfo->_initSpeed)), target, true, false, forceLock);
    }
    break;
  case AmmoShotShell:
    {
      Matrix4Val shootTrans = context._turretType ? GunTurretTransform(vs, *context._turretType) : M4Identity;
      Vector3Val pos = context._turretType ? context._turretType->GetTurretPos(shootTrans) : VZero;
      bool fired;
      Vector3 dir;
      if (remoteInfo)
      {
        dir = remoteInfo->_direction;
        fired = FireShell(context, ptr, pos, dir, target, false, true);
      }
      else
      {
        Vector3 aimtDirection =  context._turretType ? context._turretType->GetTurretDir(shootTrans) : VForward;
        vs.DirectionModelToWorld(dir, aimtDirection);
        if (GIsManual(context._gunner))
        {
          // get fov from the current camera settings
          //CameraType camType = GWorld->GetCameraType();
          //float fov = GScene->GetCamera()->Left();
          float fov = GetCameraFOV();
          // let vehicle adjust the weapon depending on fov
          dir = AdjustWeapon(context, ptr, fov, dir);
        }

        localInfo->_direction = dir;
        fired = FireShell(context, ptr, pos, dir, target, false, false);
      }
      // compute the momentum change, assume a projectile mass 0.25kg

      // make the recoil proportional to muzzle velocity - this is done to reduce recoil for grenade launchers
      float massEst = ammo->hit*(0.25f/13.0f); // assume hit 13 equals to 0.5 kg, hit 650 -> 12 kg
      float forceSize = massEst*aInfo->_initSpeed;
      Vector3 force=-dir*forceSize;
      Vector3 forcePos=vs.DirectionModelToWorld(pos-_shape->CenterOfMass());
      // simulated both remote and local - do not transfer
      AddImpulse(force,forcePos.CrossProduct(force));

      return fired;
    }
  case AmmoShotBullet:
  case AmmoShotSpread:
    {
      Matrix4Val shootTrans = context._turretType ? GunTurretTransform(vs,*context._turretType) : M4Identity;
      Vector3 pos;
      Vector3 dir;
      if (remoteInfo) dir = remoteInfo->_direction;
      if (ptr.GetWeaponType()->_shotFromTurret)
      {
        pos = context._turretType ? context._turretType->GetTurretPos(shootTrans) : VZero;
        if (!remoteInfo)
        {
          Vector3 aimtDirection = context._turretType ? context._turretType->GetTurretDir(shootTrans) : VForward;
          vs.DirectionModelToWorld(dir, aimtDirection);
          if (GIsManual(context._gunner))
          {
            // get fov from the current camera settings
            //CameraType camType = GWorld->GetCameraType();
            //float fov = GScene->GetCamera()->Left();
            float fov = GetCameraFOV();
            // let vehicle adjust the weapon depending on fov
            dir = AdjustWeapon(context, ptr, fov, dir);
          }
          localInfo->_direction = dir;
        }
      }
      else
      {
        DoAssert(context._turret);
        if (context._turretType && context._turret)
        {                
          const TurretType* turretType = context._turretType;
          Turret* turret = context._turret;
          DoAssert (turretType);
          DoAssert (turret);
          
          size_t pointsCnt = turretType->_gunPosArray.Size();         
          pos = pointsCnt>0 ? 
            shootTrans.FastTransform(
              turretType->_gunPosArray[turret->_gunPosIndexToogle]) :            
            shootTrans.FastTransform(turretType->_gunPos);          
            
          // update toggle
          if (pointsCnt>0)
            turret->_gunPosIndexToogle = 
              (++turret->_gunPosIndexToogle)%pointsCnt;            
        } 
        else
        {
          pos = shootTrans.FastTransform(VZero);
        }
                
        //pos = shootTrans.FastTransform(context._turretType ? context._turretType->_gunPos : VZero);
        
        
        if (!remoteInfo)
        {
          Vector3 aimtDirection =  shootTrans.Rotate(context._turretType ? context._turretType->_gunDir : VForward);
          vs.DirectionModelToWorld(dir , aimtDirection);
          if (GIsManual(context._gunner))
          {
            // get fov from the current camera settings
            //CameraType camType = GWorld->GetCameraType();
            //float fov = GScene->GetCamera()->Left();
            float fov = GetCameraFOV();
            // let vehicle adjust the weapon depending on fov
            dir = AdjustWeapon(context, ptr, fov, dir);
          }
          localInfo->_direction = dir;
        }
      }

#if _VBS3 //we only apply torque around X, Z
      // add (a little) recoil even when firing a machine gun
      // compute the momentum change, assume a bullet mass 150g
      float forceSize = 0.150f*aInfo->_initSpeed;
      Vector3 force = -dir*forceSize;
      Vector3 forcePos=DirectionModelToWorld(pos-_shape->CenterOfMass());
      // simulated both remote and local - do not transfer
      forcePos[0] = 0; forcePos[2] = 0; // only use vertical plane
      //      AddImpulse(force,forcePos.CrossProduct(force));
      AddImpulse(VZero,forcePos.CrossProduct(force));
#endif
      return FireMGun(context, ptr, pos, dir, target, false, remoteInfo!=NULL);
    }
    break;
  case AmmoNone:
    if (localInfo)
      localInfo->_direction = VForward;
    break;
  default:
    if (localInfo)
      localInfo->_direction = VForward;
    Fail("Unknown ammo used.");
    break;
  }
  return false;
}

bool TankOrCar::FireWeapon(const TurretContextV &context, const WeaponModePtr &ptr, TargetType *target, bool forceLock)
{
  if (GetNetworkManager().IsControlsPaused())
    return false;

  if (!ptr.IsValid())
    return false;

  const MuzzleType *muzzle = ptr.GetMuzzleType();
  if (muzzle && muzzle->_magazines.Size() <= 0)
    // weapon accepts no magazines - it is a horn?
    return BlowHorn(context, ptr);

  if (!IsFireEnabled(context))
    return false;
  if (!GetWeaponLoaded(*context._weapons, ptr))
    return false;
  if (context._turretType && context._gunner && context._gunner->Brain())
  {
    if (!context._turretType->GunnerMayFire(context))
      return false;
  }

  const Magazine *magazine = ptr.GetMagazine();
  if (!magazine)
    return false;

  RemoteFireWeaponInfo local;
  bool fired = ProcessFireWeapon(context, ptr, magazine, target, NULL, &local, forceLock);
  if (fired)
  {
    base::PostFireWeapon(context, ptr, target, local);
    return true;
  }
  return false;
}

bool TankOrCar::FireWeaponEffects(const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine, EntityAI *target, const RemoteFireWeaponInfo *remoteInfo)
{
  const AmmoType *ammo = ValidateFireWeaponWithAmmo(ptr, magazine);
  if (!ammo) return false;
  if (EnableVisualEffects(SimulateVisibleNear)) switch (ammo->_simulation)
  {
    case AmmoShotMissile:
    case AmmoNone:
      break;
    case AmmoShotShell:
      {
        context._weapons->_gunClouds.Start(1.5f);
        context._weapons->_gunFire.Start(0.1f,1,false);
        const float time=1;
        _fireDustTimeLeft=time;
        _invFireDustTimeTotal=1/time;
      }
      break;
    case AmmoShotBullet:
    case AmmoShotSpread:
      {
        if (ptr.GetWeaponType()->_shotFromTurret)
        {
          context._weapons->_gunClouds.Start(0.1f);

#if _VBS2 //also display light effects for ShotFromTurret Weapons
          float duration = 0.1f;
          float intensity = 0.012f;
          if (slot._weapon)
          {
            duration = slot._weapon->_fireLightDuration;
            intensity = slot._weapon->_fireLightIntensity;
          }
          if (duration > 0 && intensity > 0) context._weapons->_gunFire.Start(duration, intensity, true);
#endif

        }
        else
        {
          context._weapons->_mGunClouds.Start(0.1f);
          {
            float duration = 0.1f;
            float intensity = 0.012f;
            const WeaponType *weapon = ptr.GetWeaponType();
            if (weapon)
            {
              duration = weapon->_fireLightDuration;
              intensity = weapon->_fireLightIntensity;
            }
            if (duration > 0 && intensity > 0)
              context._weapons->_mGunFire.Start(duration, intensity, true);
          }
          context._weapons->_mGunFireUntilFrame = Glob.frameID+1;
          context._weapons->_mGunFireTime = Glob.time;
          int newPhase;
          while ((newPhase = toIntFloor(GRandGen.RandomValue() * 3)) == context._weapons->_mGunFirePhase) ;
          context._weapons->_mGunFirePhase = newPhase;
        }
      }
      break;
  }
  return base::FireWeaponEffects(context, ptr, magazine,target,remoteInfo);
}

bool TankOrCar::AimWeapon(const TurretContextEx &context, const WeaponModePtr &ptr, Vector3Par direction)
{
  WeaponModePtr selected = context._weapons->FindValidWeapon(ptr);
  if (!selected.IsValid()) return false;

  if (context._turretType && context._gunner)
  {
    if (!context._turretType->GunnerMayFire(context))
      return false;
  }
  context._weapons->SelectWeapon(this, selected);
  // move turret/gun accordingly to direction
  Vector3 relDir(VMultiply, context._parentTrans.InverseRotation(), direction);
  // calculate current gun direction
  // compensate for neutral gun position
  if (context._turret)
    context._turret->Aim(*context._turretVisualState, *context._turretType, relDir, context._parentTrans);
  return true;
}

float TankOrCar::GetAimed(const TurretContext &context, const WeaponModePtr &ptr, Target *target, bool exact, bool checkLockDelay) const
{
  if (!ptr.IsValid())
    return 0.0f;
  if (!target || !target->idExact)
    return 0.0f;

  const Magazine *magazine = ptr.GetMagazine();
  const MagazineType *aInfo = magazine ? magazine->Type() : NULL;
  const AmmoType *ammo = aInfo ? aInfo->_ammo : NULL;
  if (!ammo)
    // FIXME: really 1.0 (instead of 0.0)?
    return 1.0f;

  if (!CheckTargetKnown(context,target))
    return 0.0f;
  if (!context._gunner)
    return 0.0f;

  AIBrain *unit = context._gunner ? context._gunner->Brain() : NULL;
  if (!unit)
    return 0.0f;

  if (!exact && (CheckSuppressiveFire() < SuppressYes) && !target->AccurateEnoughForHit(ammo,unit))
    return 0.0f;

  Vector3 speed;
  Vector3 ap;
  float seenBefore;
  float visible = GetAimingPosition(ap, speed, seenBefore, exact, context, target, ammo);

  if (!exact)
  {
    // even when suppressive fire is allowed, we require a visibility
    // in such case the visibility returned from GetAimingPosition is against the last seen position
    if (visible < 0.3f)
      return 0.0f;
    if (CheckSuppressiveFire() < SuppressYes)
    {
      if (!target->SeenRecently(unit))
        return 0.0f;
    }
    else
    {
      float pauseWanted = SuppressiveFireInterval(seenBefore, context, GetSuppressiveFireBoost());
      // if pause was not long enough, wait some more
      if (pauseWanted>=FLT_MAX || context._weapons->_lastShotTime > Glob.time - pauseWanted)
        return 0.0f;
    }
  }

  if (ammo && (ammo->_simulation != AmmoShotMissile && ammo->_simulation != AmmoShotRocket) || ammo->maxControlRange<10)
  {
    return CheckAimed(ap, speed, context, ptr, target, exact, ammo)*visible;
  }
  else
  {
    Vector3 relPos = ap-FutureVisualState().PositionModelToWorld(GetWeaponPoint(FutureVisualState(), context, ptr));
    // check if target is in front of us
    if (relPos.SquareSize() <= Square(30))
      return 0.0f; // missile fire impossible
    // check if target position is withing missile lock cone
    Vector3 wDir = GetWeaponDirection(FutureVisualState(), context, ptr);
    float wepCos = relPos.CosAngle(wDir);
    float wepAimed = 1.0f - (1.0f - wepCos) * 30.0f;
    saturateMax(wepAimed, 0.0f);

    return visible * wepAimed;
  }

  //return 1.0; // default: assume always aimed
}

bool TankOrCar::CalculateAimWeaponPos(const TurretContext &context, const WeaponModePtr &ptr, Vector3 &pos, float &timeToLead, Target *target, bool exact) const
{
  timeToLead = 0.0f;
  WeaponModePtr selected = context._weapons->FindValidWeapon(ptr);
  if (!selected.IsValid()) return false;
  if (!CheckTargetKnown(context, target))
    return false;
  const Magazine *magazine = selected.GetMagazine();
  const MagazineType *aInfo = magazine ? magazine->Type() : NULL;
  const AmmoType *ammo = aInfo ? aInfo->_ammo : NULL;
  const WeaponModeType *mode = selected.GetWeaponModeType();

  Vector3 tgtSpd;
  Vector3 tgtPos;
  float seenBefore;
  GetAimingPosition(tgtPos, tgtSpd, seenBefore, exact, context, target, ammo);

  if (CHECK_DIAG(DECombat))
  {
    LODShape *forceArrow = GScene->ForceArrow();
    { // position decided by FSM
      Ref<Object> obj = new ObjectColored(forceArrow,VISITOR_NO_ID);

      float size = 0.3f;
      obj->SetPosition(tgtPos);
      obj->SetOrient(VUp,VForward);
      obj->SetPosition(obj->FutureVisualState().PositionModelToWorld(forceArrow->BoundingCenter()*size));
      obj->SetScale(size);
      obj->SetConstantColor(Color(0, 0.3f, 0));
      GScene->DrawObject(obj);
      GScene->DrawDiagText(GetDebugName()+" AimTgt", tgtPos + VUp * size, 5);
    }
  }

  // predict his and my movement
  // Vector3 weaponPos = context._turretType->GetTurretAimCenter(Type());
  float dist = tgtPos.Distance(FutureVisualState().Position());
  float time = aInfo ? dist * aInfo->_invInitSpeed : 0.0f;
  float fall = 0.0f;
  bool rockets = false;
  if (ammo)
  {
    if (ammo->_simulation!=AmmoShotBullet && ammo->_simulation!=AmmoShotSpread && ammo->_simulation!=AmmoShotMissile)
    {
      // we do not expect direct hit on a small target: hit ground instead
      if (target->idExact && target->idExact->GetShape() && target->idExact->GetShape()->GeometrySphere() < 1.5f)
        tgtPos[1] = GLandscape->SurfaceY(tgtPos[0], tgtPos[2]);
    }

    switch (ammo->_simulation)
    {
    case AmmoShotMissile: case AmmoShotRocket:
      if (ammo->maxControlRange<10)
      {
        time = PreviewRocketBallistics(tgtPos, FutureVisualState().Position(), aInfo, ammo, fall, mode);
        if (time>=FLT_MAX) return false;
        rockets = true;
      }
      else
      {
        time = 0.0f;
      }
      break;
    default:
      time = PreviewBallistics(tgtPos, FutureVisualState().Position(), aInfo, ammo, fall, mode);
      if (time>=FLT_MAX) return false;
      break;
    }
  }
  timeToLead = time;
  
  tgtPos[1] += fall; // consider ballistics
  if (aInfo)
  {
    // for rockets this speed correction aprox. is not working properly
    Vector3 speedEst = rockets? tgtSpd : tgtSpd - FutureVisualState().Speed();
    float maxSpeedEst = aInfo->_maxLeadSpeed;
    if (speedEst.SquareSize() > Square(maxSpeedEst))
      speedEst = speedEst.Normalized() * maxSpeedEst;
    float predTime = floatMax(time + 0.1f, 0.25f);
    tgtPos += speedEst * predTime;
  }

  pos = tgtPos;
  return true;
}

bool TankOrCar::CalculateAimWeapon(const TurretContext &context, const WeaponModePtr &ptr, Vector3 &dir, float &timeToLead, Target *target, bool exact ) const
{
  Vector3 tgtPos;
  if (!CalculateAimWeaponPos(context, ptr, tgtPos, timeToLead, target, exact))
    return false;
  WeaponModePtr selected = context._weapons->FindValidWeapon(ptr);
  if (!selected.IsValid()) return false;

  const AmmoType *ammo = selected.GetAmmoType();
  Vector3 weaponPos = VZero;
  if (context._turretType)
  {
    weaponPos = context._turretType->GetTurretAimCenter(Type());
    // predict his and my movement
    if (ammo) switch (ammo->_simulation)
    {
    case AmmoShotMissile:
      weaponPos = GunTurretTransform(FutureVisualState(), *context._turretType).FastTransform(Type()->_missilePos);
      break;
    case AmmoShotBullet:
    case AmmoShotSpread:
      if (!selected.GetWeaponType()->_shotFromTurret)
      {
        weaponPos = GunTurretTransform(FutureVisualState(), *context._turretType).FastTransform(context._turretType->_gunPos);
        break;
      }
      // continue - main gun position
    default:
      weaponPos = context._turretType->GetTurretPos(GunTurretTransform(FutureVisualState(), *context._turretType));
      break;
    }
  }
  Vector3 myPos = FutureVisualState().PositionModelToWorld(weaponPos);
  dir = tgtPos - myPos;
  return true;
}


bool TankOrCar::AimWeaponTgt(const TurretContextEx &context, const WeaponModePtr &ptr, Target *target)
{
  Vector3 dir;
  float timeToLead;

  bool exact = false;

  if (!CalculateAimWeapon(context, ptr, dir, timeToLead, target, exact))
    return false;  
  return AimWeapon(context, ptr, dir);
}

const SoundPars& TankOrCar::SelectBulletSoundPars(const TurretContext &context, const WeaponModePtr &ptr) const
{
  // fix bug when weapon was removed during no last bullet fall down
  if (context._weapons && ptr.IsValid())
  {
    // select falling bullets sound, samples pars are defined: [0 - 1/3] metallic surfaces,
    // [1/3 - 2/3] hard surface and rest as soft surface
    const MuzzleType *muzzle = ptr.GetMuzzleType();
    if (muzzle) 
    {
      // for guns placed on cars we used only metallic samples
      float rand = GRandGen.RandomValue() * 0.33333f;
      return muzzle->_bullets.SelectSound(rand);
    }
  }

  return base::SelectBulletSoundPars(context, ptr);
}

/*!
@param dir is world space
*/

Vector3 TankOrCar::GetEyeDirection(ObjectVisualState const& vs, const TurretContext &context) const
{
  if (!context._turretType)
    return vs.Direction();
  return vs.Transform().Rotate(GunTurretTransform(vs, *context._turretType).Direction());
}

Vector3 TankOrCar::GetWeaponDirection(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const
{
  if (!context._turretType)
    return vs.Direction();
  if (!ptr.IsValid())
  {
    Vector3 dir = context._turretType->GetTurretDir(GunTurretTransform(vs, *context._turretType));
    return vs.Transform().Rotate(dir);
  }
  const AmmoType *ammo = ptr.GetAmmoType();
  if (!ammo)
  {
    Vector3 dir = context._turretType->GetTurretDir(GunTurretTransform(vs, *context._turretType));
    return vs.Transform().Rotate(dir);
  }
  Vector3 dir;
  switch (ammo->_simulation)
  {
  case AmmoShotMissile:
    dir = GunTurretTransform(vs, *context._turretType).Rotate(Type()->_missileDir);
    break;
  case AmmoShotBullet:
  case AmmoShotSpread:
    dir = GunTurretTransform(vs, *context._turretType).Rotate(context._turretType->_gunDir);
    break;
  default:
    dir = context._turretType->GetTurretDir(GunTurretTransform(vs, *context._turretType));
    break;
  }
  return vs.Transform().Rotate(dir);
}

Vector3 TankOrCar::GetWeaponDirectionWanted(const TurretContextEx &context, const WeaponModePtr &ptr) const
{
  if (!context._turretType)
    return FutureVisualState().Direction();
  Matrix3 aim;
  if (context._turret->_gunStabilized)
    aim = context._turret->GetAimWantedWorld();
  else
    aim = context._parentTrans * context._turret->GetAimWantedRel();
  if (!ptr.IsValid())
    return context._turretType->GetTurretDir(aim);
  const AmmoType *ammo = ptr.GetAmmoType();
  if (!ammo)
    return context._turretType->GetTurretDir(aim);
  switch (ammo->_simulation)
  {
  case AmmoShotMissile:
    return aim * Type()->_missileDir;
  case AmmoShotBullet:
  case AmmoShotSpread:
    return aim * context._turretType->_gunDir;
  default:
    return context._turretType->GetTurretDir(aim);
  }
}

Vector3 TankOrCar::GetEyeDirectionWanted(ObjectVisualState const& vs, const TurretContextEx &context) const
{
  if (context._turret->_gunStabilized)
    return context._turret->GetAimWantedWorld().Direction();
  else
    return (context._parentTrans * context._turret->GetAimWantedRel()).Direction();
}

Vector3 TankOrCar::GetWeaponCenter(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const
{
  if (!context._turret)
    return VZero;
  return context._turret->GetCenter(Type(), *context._turretType);
}

float TankOrCar::FireInRange(const TurretContext &context, const WeaponModePtr &ptr, float &timeToAim, const Target &target) const
{
  timeToAim=0;
  if (ptr != context._weapons->ValidatedCurrentWeapon())
  {
    // switching between weapons can be expensive
    // assume switching mode takes at least 0.5 sec
    timeToAim = 0.5f;
  }
  Vector3 relDir=PositionWorldToModel(target.GetPosition());

  return FireAngleInRange(context, ptr, relDir);
}

void TankOrCar::FireAimingLimits(float &minAngle, float &maxAngle, const TurretContext &context, const WeaponModePtr &ptr) const
{
  // assuming here all tanks have a turret that can be fully turned around (in Y axis)
  // TODO: fix this, this is not true for some APCs or ships
  //const TankOrCar *type=Type();
  minAngle = -2*H_PI;
  maxAngle = +2*H_PI;
}

float TankOrCar::FireAngleInRange(const TurretContext &context, const WeaponModePtr &ptr, Vector3Par rel) const
{
  // assuming here all tanks have a turret that can be fully turned around (in Y axis)
  // TODO: fix this, this is not true for some APCs or ships
  float dist2 = rel.SquareSizeXZ();
  if (dist2 == 0.0f)
    return 0;
  float y2 = Square(rel.Y());
  // x>0: atan(x)<x
  // atan(y/sqrt(dist2)) < type->_maxGunElev
  // y/sqrt(dist2) < type->_maxGunElev
  // y < type->_maxGunElev * sqrt(dist2)
  // y2 < type->_maxGunElev^2 * dist2
  // we need to have correct signs
  const TurretType *type = context._turretType;
  if (!type)
    return base::FireAngleInRange(context, ptr, rel);

  Assert(type->_maxElev>=0);
  Assert(type->_minElev<=0);
  float ret=0;
  if (rel.Y()>=0)
  {
    // fire up, high angle, have to use tan
    if(y2>dist2*Square(tan(type->_maxElev + type->_neutralXRot)))
      return 0;
    ret = rel.Y()*InvSqrt(dist2)*(1/tan(type->_maxElev + type->_neutralXRot));
  }
  else
  {
    // fire down
    if (y2>dist2*Square(type->_minElev + type->_neutralXRot))
      return 0;
    ret = rel.Y()*InvSqrt(dist2)*(1/type->_minElev + type->_neutralXRot);
  }
  return floatMin(1,6-ret*6);
}

