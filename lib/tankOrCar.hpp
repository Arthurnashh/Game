#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TANKORCAR_HPP
#define _TANKORCAR_HPP

#include "tracks.hpp"
#include "gearBox.hpp"

#include "transport.hpp"
#include "shots.hpp"
#include "global.hpp"

// curve for input
#include "curve3D.hpp"



class TankOrCarType;

class TankOrCarVisualState : public TransportVisualState  // TODO: dampers
{
  typedef TransportVisualState base;
  friend class TankOrCar;

protected:
  float _rpm;
  float _phaseL, _phaseR; // texture animation

public:
  TankOrCarVisualState(TankOrCarType const& type);
  virtual void Interpolate(Object *obj, ObjectVisualState const& t1state, float t, ObjectVisualState& interpolatedResult) const;
  virtual void Copy(ObjectVisualState const& src) {*this=static_cast_checked<const TankOrCarVisualState &>(src);}
  virtual TankOrCarVisualState *Clone() const = 0;

  float GetRPM() const { return _rpm; }
  float GetCtrlWheelLRot() const { return fmod(_phaseR, 1); } //empiric fix: wheels rotated to wrong direction
  float GetCtrlWheelRRot() const { return fmod(_phaseL, 1); }
};


class TankOrCarType: public TransportType
{
  typedef TransportType base;
  friend class TankOrCar;
  friend class Car;
  friend class Tank;
  friend class TankWithAI;

protected:
  Vector3 _pilotPos; // camera position 
  AutoArray<ExhaustInfo> _exhausts;
  RString _leftDust, _rightDust;
  RString _leftWater, _rightWater; //water effect source
  Vector3 _lWaterPos, _rWaterPos; // water effect position

  SoundPars _gearSound;

  AnimationSection _brakeLights;

  float _waterSpeedFactor;

#if _VBS2 // convoy trainer, indicators
  bool _hasIndicators;
  AnimationSection _leftIndicators;
  AnimationSection _rightIndicators;
  AnimationSection _reverseLights;
#endif

  int _engineHit;
  int _fuelHit;

  /// how fast tracks are moving relative to wheels
  float _tracksSpeed;
  AnimationUV _leftOffset,_rightOffset;

  bool _canFloat; // is able to move in water

  ConfigCurve3D _inputTurnCurve; // curve that modifies turning 

  //@{ weapon handling
  Vector3 _missilePos,_missileDir; // missile position and direction
  //@}

  //@{ cm position and direction
  AutoArray<Vector3> _cmPos,_cmDir;
  //@}

public:
  TankOrCarType(ParamEntryPar param) :base(param),_pilotPos(VZero) {_engineHit = -1;  _tracksSpeed = 1.0f;}
  virtual void Load(ParamEntryPar par, const char *shape);
  void InitShape();
  void InitShapeLevel(int level);
  void DeinitShapeLevel(int level);
  void GetPathPrecision(float speedAtCost, float &safeDist, float &badDist) const;
  AnimationSource *CreateAnimationSource(const AnimationType *type, RStringB source);

  virtual EntityVisualState* CreateVisualState() const = 0;
};

#if _VBS2 // convoy trainer, indicators
#define UPDATE_TANK_OR_CAR_MSG(MessageName, XX) \
  XX(MessageName, bool, pilotBrake, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("State of pilot brake (on / off)"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, bool, leftIndicatorOn, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("State of left indicator (on / off)"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, bool, rightIndicatorOn, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("State of right indicator (on / off)"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, bool, hazardLightsOn, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("State of hazard lights (on / off)"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE)
#else
#define UPDATE_TANK_OR_CAR_MSG(MessageName, XX) \
  XX(MessageName, bool, pilotBrake, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("State of pilot brake (on / off)"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE)
#endif

DECLARE_NET_INDICES_EX_ERR(UpdateTankOrCar, UpdateTransport, UPDATE_TANK_OR_CAR_MSG)

TypeIsMovable(ExhaustSource)

class TankOrCar: public Transport
{
  typedef Transport base;

protected:
  float _rpmWanted; // smooth change of rpm
  GearBox _gearBox; // current gear (0..3)
  EffectsSourceRT _leftDust, _rightDust;
  EffectsSourceRT _leftWater, _rightWater;
  Link<AbstractWave> _gearSound;

  AutoArray<EffectsSourceRT> _exhausts;

//  Ref<LightReflectorOnVehicle> _leftLight;
//  Ref<LightReflectorOnVehicle> _rightLight; 

  bool _pilotBrake;
  Time _lastPilotBrake; // avoid flashing lights
  int _cmIndexToggle;

#if _VBS2 // convoy trainer, indicators
  bool _leftIndicatorOn;
  bool _leftIndicate;
	Time _lastLeftIndicate;

  bool _rightIndicatorOn;
  bool _rightIndicate;
  Time _lastRightIndicate;

  bool _hazardLightsOn;
#endif

  float _invFireDustTimeTotal;
  float _fireDustTimeLeft;
  
  /// screenspace driving cursor vertical position (0..1 in the 2D viewport coordinates)
  float _cursorY;

public:
  TankOrCar(const EntityAIType *name, Person *driver, bool fullCreate);

  const TankOrCarType *Type() const {return static_cast<const TankOrCarType *>(GetType());}

  /// @{ Visual state
public:
  TankOrCarVisualState typedef VisualState;
  friend class TankOrCarVisualState;

  VisualState& FutureVisualState() { return static_cast<VisualState &>(base::FutureVisualState()); }
  VisualState const& FutureVisualState() const { return static_cast_checked<VisualState const &>(base::FutureVisualState());}
  VisualState const& CurrentVisualState() const { return static_cast_checked<VisualState const &>(base::CurrentVisualState());}
  const VisualState& RenderVisualState() const{ return static_cast<const VisualState &>(base::RenderVisualState()); }

  ProtectedVisualState<const VisualState> RenderVisualStateScope(bool ignoreParent=false) const {return ProtectedVisualState<const VisualState>(base::RenderVisualStateScope(ignoreParent));}
  ProtectedVisualState<VisualState> FutureVisualStateScope(bool ignoreParent=false) { return ProtectedVisualState<VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  ProtectedVisualState<const VisualState> FutureVisualStateScope(bool ignoreParent=false) const { return ProtectedVisualState<const VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  /// @}

  /// @{ Visual controllers (\sa Type()->CreateAnimationSource())
  virtual float GetCtrlRPM(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetRPM(); }
  float GetCtrlWheelLRot(const ObjectVisualState &vs) const { return static_cast<const VisualState &>(vs).GetCtrlWheelLRot(); } //empiric fix: wheels rotated to wrong direction
  float GetCtrlWheelRRot(const ObjectVisualState &vs) const { return static_cast<const VisualState &>(vs).GetCtrlWheelRRot(); }
  /// @}

  virtual float GetRPM() const { return FutureVisualState().GetRPM(); }
  virtual float GetRenderVisualStateRPM() const { return RenderVisualState().GetRPM(); }

  virtual bool CanFloat() const {return Type()->_canFloat;}

  void Simulate(float deltaT, SimulationImportance prec);

  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
  void Deanimate(int level, bool setEngineStuff);
  bool AnimateTexture(AnimationContext &animContext, int level, float phaseL, float phaseR);

#if _VBS2 // convoy trainer, indicators
  void DoIndicators(bool indicateLeft,bool indicateRight,bool hazardLights);
#endif

  void SimulateExhaust(float deltaT, SimulationImportance prec);

  virtual float GetSteerAheadSimul() const;
  virtual float GetSteerAheadPlan() const;

  virtual float GetPrecision() const;

  //@{weapon manipulation
  // all tank weapons are linked
  // special case is guided missile, which is linked, but have weapon lock
  virtual bool CalculateAimWeaponPos(const TurretContext &context, const WeaponModePtr &ptr, Vector3 &pos, float &timeToLead, Target *target, bool exact) const override;
  virtual bool CalculateAimWeapon(const TurretContext &context, const WeaponModePtr &ptr, Vector3 &dir, float &timeToLead, Target *target, bool exact) const override;
  virtual float GetAimed(const TurretContext &context, const WeaponModePtr &ptr, Target *target, bool exact, bool checkLockDelay) const override;

  virtual bool AimWeapon(const TurretContextEx &context, const WeaponModePtr &ptr, Vector3Par direction) override;
  virtual bool AimWeaponTgt(const TurretContextEx &context, const WeaponModePtr &ptr, Target *target) override;

  virtual Vector3 GetWeaponDirection(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const override;
  virtual Vector3 GetWeaponCenter(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const override;

  Vector3 GetEyeDirection(ObjectVisualState const& vs, const TurretContext &context) const;
  virtual Vector3 GetWeaponDirectionWanted(const TurretContextEx &context, const WeaponModePtr &ptr) const override;
  Vector3 GetEyeDirectionWanted(ObjectVisualState const& vs, const TurretContextEx &context) const;
  Vector3 GetLookAroundDirection(ObjectVisualState const& vs, const TurretContext &context) const {return GetEyeDirection(vs, context);}

  virtual float FireAngleInRange(const TurretContext &context, const WeaponModePtr &ptr, Vector3Par rel) const override;
  virtual float FireInRange(const TurretContext &context, const WeaponModePtr &ptr, float &timeToAim, const Target &target) const override;
  virtual void FireAimingLimits(float &minAngle, float &maxAngle, const TurretContext &context, const WeaponModePtr &ptr) const override;

  virtual const SoundPars& SelectBulletSoundPars(const TurretContext &context, const WeaponModePtr &ptr) const override;

  virtual bool FireWeapon(const TurretContextV &context, const WeaponModePtr &ptr, TargetType *target, bool forceLock) override;
  virtual bool ProcessFireWeapon(const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine, EntityAI *target,
    const RemoteFireWeaponInfo *remoteInfo, RemoteFireWeaponInfo *localInfo, bool forceLock) override;
  virtual bool FireWeaponEffects(const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine, EntityAI *target,
    const RemoteFireWeaponInfo *remoteInfo) override;

  virtual bool BlowHorn(const TurretContext &context, const WeaponModePtr &ptr) {return false;} // by default there is no horn
  //@}

  //@{ used for exhaust simulation
  //virtual float Thrust() const;
  virtual float ThrustWanted() const = 0;
  //@}

  const char *GetControllerScheme() const {return "Ground";}
  virtual void GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit) override;
#if _HELISIM_PRECISE_INTERACTION
  void GetDrawActions3D(UIActions &actions, AIBrain *unit);
#endif
  void GetAimActions(float &aimX, float &aimY) const override;

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format);
  TMError TransferMsg(NetworkMessageContext &ctx);
  float CalculateError(NetworkMessageContextWithError &ctx);
};

#endif
