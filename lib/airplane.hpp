#ifdef _MSC_VER
#pragma once
#endif

#ifndef _AIRPLANE_HPP
#define _AIRPLANE_HPP

#include "transport.hpp"
#include "planeOrHeli.hpp"
#include "shots.hpp"
#include "global.hpp"


class AirplaneType;

class AirplaneVisualState : public TransportVisualState
{
  typedef TransportVisualState base;
  friend class Airplane;
  friend class AirplaneAuto;

protected:
  float _flaps;  /// actual flap position
  float _gearsUp;  /// actual gear position 0 means gears down, 1 gears up, negative gear damaged
  float _rotorPosition;
  float _elevator;
  float _rudder;
  float _thrustVector;  /// thrust vectoring, 0 = default, 1 = 90 degrees down
  float _aileron;
  float _brake;
  float _wheelPhase;  /// wheel position
  float _cabinPos;  /// 0 = cabin closed, 1 = open
  float _rpm;  // on/off

public:
  AirplaneVisualState(AirplaneType const& type);
  virtual void Interpolate(Object *obj, ObjectVisualState const& t1state, float t, ObjectVisualState& interpolatedResult) const;
  virtual void Copy(ObjectVisualState const& src) {*this=static_cast_checked<const AirplaneVisualState &>(src);}
  virtual AirplaneVisualState *Clone() const {return new AirplaneVisualState(*this);}

  float GetGearPos(const AirplaneType *type) const;
  float GetFlapPos() const { return _flaps; }
  float GetElevatorPos() const { return _elevator; }
  float GetRudderPos() const { return _rudder; }
  float GetNoseWheelPos(Vector3Par modelSpeed) const;
  float GetThrustVectorPos() const { return _thrustVector; }
  float GetRotorPos() const;
  float GetAileronTPos() const;
  float GetAileronBPos() const;
  float GetAOA(Vector3Par speed) const;
  float GetAileron() const { return _aileron; }
  float GetSpeedBrake() const { return _brake; }
  float GetWheelPos() const { return _wheelPhase; }
  float GetCabinOpen() const { return (1 - _cabinPos)*15; }
  float GetRPM() const { return _rpm; }
};


/// common properties of given airplane type
class AirplaneType: public TransportType
{
  typedef TransportType base;

  friend class Airplane;
  friend class AirplaneVisualState;
  friend class AirplaneAuto;

  protected:
  //@{ rockets position and direction
  Vector3 _rocketLPos,_rocketRPos;
  Vector3 _rocketDir;
  //@}

  //@{ cm position and direction
  AutoArray<Vector3> _cmPos,_cmDir;
  //@}
    
  //@{ gun position and direction
  Vector3 _gunPos,_gunDir; 
  //@}

  Vector3 _ejectSpeed;

  AnimationSection _rotorStill, _rotorMove;

  /// lift based on speed
  AutoArray<float> _envelope;

  float _altFullForce;
  float _altNoForce;

  Vector3 _lDust,_rDust;
  /// class names of dust effects
  RString _leftDust, _rightDust; 
  /// class name of damage effects
  RString _damageEffect;
  /// class names describing effect sources
  RString _dustEffect, _waterEffect;

  float _minGunTurn,_maxGunTurn; // gun movement
  float _minGunElev,_maxGunElev;
  bool _gearRetracting;
  bool _cabinOpening;
  /// has VTOL vectoring?
  /** 0: none, 1: VTOL, 2: STOVL, 3: VTOL, vectoring needed near ground (Mv22) */
  int _vtol;
  bool _hasFlaps;
  bool _hasAirbrake;
  // reflector located on the landing gear needs to be off when gear is up
  bool _lightOnGear;

  float _gearDownSpeed;
  float _gearUpSpeed;
  float _landingSpeed;
  float _takeOffSpeed;
  float _stallSpeed;
  float _aileronSensitivity;
  float _elevatorSensitivity;
  float _wheelSteeringSensitivity;
  float _landingAoa;
  float _flapsFrictionCoef;
  /// units will eject once the plane is damage too much
  float _ejectDamageLimit;

  int _hullHit;
  /// plane which describes land-contact when landing gear is down
  mutable bool _landContactPlaneValid;
  
  mutable Plane _landContactPlane;
  
  float MarshallDist() const;
  float ApproachDist() const;
  float FinalDist() const;

  public:
  AirplaneType( ParamEntryPar param );
  virtual void Load(ParamEntryPar par, const char *shape);
  
  AnimationSource *CreateAnimationSource(const AnimationType *type, RStringB source);
  
  void InitShape();
  void DeinitShape();
  void InitShapeLevel(int level);
  void DeinitShapeLevel(int level);
  bool AbstractOnly() const {return false;}

  virtual Object* CreateObject(bool unused) const;
  virtual EntityVisualState* CreateVisualState() const { return new AirplaneVisualState(*this); }

  float GetFieldCost( const GeographyInfo &info, CombatMode mode ) const;
  float GetBaseCost(const GeographyInfo &info, bool operative, bool includeGradient) const;
  virtual float GetAirCost(float x, float z) const;
  float GetGradientPenalty(float gradient) const;
  float GetCostTurn(int difDir, int &debet, bool forceFullTurn) const;
  bool CanBeAirborne() const {return true;}

  virtual bool StrategicPlanningOnly() const {return true;}
  virtual bool CanUseRoads() const {return false;}
  virtual float GetStopDistance() const;

  /// lift based on simulation parameters
  float UpForce(float speed, float aoa, float maxSpeed, float flaps, float groundEff) const;

#if _HELISIM_IMPERIAL_SYSTEM
  virtual float ConvertDistance(float meters, int &stringIDS) const;
  virtual float ConvertSpeed(float metersPerSecond) const;
  virtual float ConvertHeight(float height) const;
#endif
};


/// airplane vehicle class

class Airplane: public PlaneOrHeli
{
  typedef PlaneOrHeli base;

protected:
  
  RefAbstractWave _servoSound;
  
  /// true means pilot wants gear down
  bool _pilotGear;

  bool _rocketLRToggle;
  int _cmIndexToggle;
  bool _gearDamage;

  /// override auto detection - open cabin when need to get in
  bool _openCabin;
  /// VTOL mode switch
  /**
  In VTOL mode max. thrust is engaged and forward speed is controlled using vectoring
  */
  bool _vtolMode;

  int _pilotFlaps;

  float _pilotBrake;

  float _rndFrequency; // volume control - low down hero sound
  float _servoVol;

  float _gunYRot,_gunYRotWanted;
  float _gunXRot,_gunXRotWanted;
  float _gunXSpeed,_gunYSpeed;

  // pilot can directly coordinate these parameters
  float _thrust,_thrustWanted; // turning motor on/off
  float _rotorSpeed;

  float _elevatorWanted;
  float _elevatorTrim;
  float _rudderWanted;
  float _aileronWanted;

  float _thrustVectorWanted;
  
  /// angular speed - to make inertia
  float _wheelSpeed;
  
  /// dust effects
  EffectsSourceRT _leftDust,_rightDust;
  /// damage effect
  EffectsSourceRT _damageEffect;
  /// hovering dust effects
  EffectsSourceRT _dustEffect, _waterEffect;

  Vector3 _lastAngVelocity; // helper for prediction

  enum SweepState {SweepDisengage,SweepTurn,SweepEngage,SweepFire};
  SweepState _sweepState;
  Time _sweepDelay;
  LinkTarget _sweepTarget;
  Vector3 _sweepDir;

public:
  Airplane( const EntityAIType *name, Person *pilot, bool fullCreate = true );
  ~Airplane();
  
  const AirplaneType *Type() const
  {
    return static_cast<const AirplaneType *>(GetType());
  }

  /// @{ Visual state
  AirplaneVisualState typedef VisualState;

  VisualState& FutureVisualState() { return static_cast<VisualState &>(base::FutureVisualState()); }
  VisualState const& FutureVisualState() const { return static_cast_checked<VisualState const &>(base::FutureVisualState());}
  VisualState const& CurrentVisualState() const { return static_cast_checked<VisualState const &>(base::CurrentVisualState());}
  const VisualState& RenderVisualState() const{ return static_cast<const VisualState &>(base::RenderVisualState()); }

  ProtectedVisualState<const VisualState> RenderVisualStateScope(bool ignoreParent=false) const {return ProtectedVisualState<const VisualState>(base::RenderVisualStateScope(ignoreParent));}
  ProtectedVisualState<VisualState> FutureVisualStateScope(bool ignoreParent=false) { return ProtectedVisualState<VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  ProtectedVisualState<const VisualState> FutureVisualStateScope(bool ignoreParent=false) const { return ProtectedVisualState<const VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  /// @}

  virtual float GetGearPos() const { return FutureVisualState().GetGearPos(Type()); }
  virtual float GetRPM() const { return FutureVisualState().GetRPM(); }
  virtual float GetRenderVisualStateRPM() const { return RenderVisualState().GetRPM(); }

  /// @{ Visual controllers (\sa Type()->CreateAnimationSource())
  virtual float GetCtrlRPM(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetRPM(); }
  float GetCtrlGearPos(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetGearPos(Type()); }
  float GetCtrlFlapPos(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetFlapPos(); }
  float GetCtrlElevatorPos(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetElevatorPos(); }
  float GetCtrlRudderPos(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetRudderPos(); }
  float GetCtrlNoseWheelPos(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetNoseWheelPos(vs.cast<Airplane>().ModelSpeed()); }
  float GetCtrlThrustVectorPos(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetThrustVectorPos(); }
  float GetCtrlRotorPos(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetRotorPos(); }
  float GetCtrlAileronTPos(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetAileronTPos(); }
  float GetCtrlAileronBPos(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetAileronBPos(); }
  float GetCtrlAOA(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetAOA(vs.cast<Airplane>().ModelSpeed()); }
  float GetCtrlAileron(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetAileron(); }
  float GetCtrlSpeedBrake(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetSpeedBrake(); }
  float GetCtrlWheelPos(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetWheelPos(); }
  float GetCtrlCabinOpen(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetCabinOpen(); }
  /// @}

  float Thrust() const {return _thrust;}
 
  virtual const char **HUDMode(int &mask) const;

  AnimationStyle IsAnimated( int level ) const; // appearance changed with Animate
  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
  void Deanimate( int level, bool setEngineStuff );

  virtual float DetectStall() const;
  void PerformFF( FFEffects &effects );
  void Simulate( float deltaT, SimulationImportance prec );

  virtual bool RemoteStateSupported() const {return true;}

  float GetHitForDisplay(int kind) const;

  void Sound( bool inside, float deltaT );
  void UnloadSound();

  float GetEngineVol( float &freq ) const;
  float GetEnvironVol( float &freq ) const;

  bool IsVirtual( CameraType camType ) const {return true;}
  bool IsContinuous(CameraType camType ) const;
  float TrackingSpeed() const {return 200;}
  
  virtual bool EmittingWind(Vector3Par pos, float radius) const;
  virtual Vector3 WindEmit(Vector3Par pos, Vector3Par wind) const;

  bool Airborne() const;

  bool HasHUD() const {return true;}
  const char *GetControllerScheme() const {return "Aircraft";}
  void GetAimActions(float &aimX, float &aimY) const override;

  UserAction GetManualFireAction() const;

  virtual bool GetAutoHover() {return _vtolMode;}

  LSError Serialize(ParamArchive &ar);
  USE_CASTING(base)
};

enum AirplaneState
{
  /// taxi-in: move to start of runway
  TaxiIn,
  /// various AI plane states
  Takeoff,Flight,Marshall,Approach,Final,Landing,
  Touchdown,AfterTouchdown,WaveOff,
  /// landing autopilot - player assistance
  AutoLanding,
  /// taxi-off: move out of runway
  TaxiOff,
};

class AirplaneAuto: public Airplane
{
  typedef Airplane base;

  protected:
  // full autopilot parameters - used for AI
  float _pilotSpeed;
  /// vtol mode - climb/dive speed
  float _pilotVertSpeed;
  float _pilotSpeedX;
  float _pilotHeading;
  float _pilotHeight;
  float _defPilotHeight;

  // when you start avoiding, continue for some time
  Time _pilotAvoidHigh;
  float _pilotAvoidHighHeight;
  Time _pilotAvoidLow;
  float _pilotAvoidLowHeight;

  // fly by wire - used for manual keyboard control
  float _pilotBank;

  float _dirCompensate;  // how much we compensate for estimated change

  bool _pilotHelperBankDive; //!< helpers for keyboard pilot - bank autopilot
  bool _pilotHelperHeight; //!< helpers for keyboard pilot - height autopilot
  bool _pilotHelperDir; //!< used for mouse control and AI - heading autopilot
  bool _pilotHelperThrust; //!< used for mouse control and AI - speed autopilot
  
  Time _analogueThrottleTime; //!< last time when user touched speed autopilot controls
  Time _thrustHelperTime; //!< last time when user touched throttle / brake controls
  
  float _lastForward; // last speed control value, used to track _thrustHelperTime
  float _lastThrotleDirect; //!< last throttle value, used to track _analogueThrottleTime

  bool _pressedForward,_pressedBack; // recognize fast speed-up, fast brake

  float _forceDive; // dive necessary for aiming

  /// takeoff/landing pattern
  AirplaneState _planeState;
  
  /// hovering autopilot state
  AutopilotState _state;
  
  /// which airport index is the preferred one for this airplane
  int _homeAirport;
  /// one-time override for airport selection (cancelled on touchdown or when landing is cancelled)
  int _landAtAirport;

  int PreferredAirport() const {return _landAtAirport>=0 ? _landAtAirport : _homeAirport;}

  public:
  AirplaneAuto( const EntityAIType *name, Person *pilot, bool fullCreate = true );

  void Simulate( float deltaT, SimulationImportance prec );

  float EstimateAboveTerrain(float estT) const;

  void AvoidGround( float minHeight );

  void DrawDiags();

  bool IsStopped() const;
  bool CheckEject() const;
  void DamageCrew(EntityAI *killer, float howMuch, RString ammo);
  void SetTotalDamageHandleDead(float damage, bool setHitpoints) override;
  void Repair(float amount);
  void Eject(AIBrain *unit, Vector3Val diff = VZero);
  void Land();
  void CancelLand();
  virtual void LandAtAirport(int index);
  virtual void AssignToAirport(int index);
  virtual void CancelLanding();
  virtual void SetFlyingHeight(float val);
  /// used when the pilot wants to eject
  Vector3 FindRunawayPlace(AIBrain *unit);

  void EngineOn();
  void EngineOff();

  virtual bool LightsNeeded() const;

  virtual bool IsLightOn(ObjectVisualState const& vs) const;

  virtual float GetFeatureSize() const {return !_landContact && !_objectContact && !GetStopped() ? 25.0f : 0;} // pretend we are a feature, so that object size limits are not applied to us

  virtual bool PrepareGetOut(AIBrain *unit);
  virtual void FinishGetOut(AIBrain *unit);
  virtual bool PrepareGetIn(UIActionType pos, Turret *turret, int cargoIndex);
  virtual void FinishGetIn(UIActionType pos, Turret *turret, int cargoIndex);

  /// check if landing autopilot can be engaged
  bool CheckLandingAvailable(bool init) const;

  //@{ flaps/gear helpers
  void AutoFlaps(float factor=1.0f);
  void AutoGear(bool extend);
  //@}

  /// landing assistance autopilot
  AirplaneState LandingAutopilot(
    Vector3Par ilsPos, Vector3Par ilsDir, bool stol,
    float &changeHeading, float curHeading,
    float &diveWanted, bool &diveWantedSet, bool &setControls
  );
  /// fully automatic landing, including approach pattern
  void AutomaticLanding(
    float &changeHeading, float curHeading, float &changeHeadingRudder,
    float landingSpeed, bool &setControls, float &diveWanted, bool &diveWantedSet, bool &elevatorSet
  );

  // VTOL airplane position/landing autopilot
  void Autopilot(float deltaT, Vector3Par target, Vector3Par direction);
  void ResetAutopilot();
  
  virtual void GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit) override;
  virtual bool GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const override;
  virtual void PerformAction(const Action *action, AIBrain *unit) override;

  virtual bool FireWeapon(const TurretContextV &context, const WeaponModePtr &ptr, TargetType *target, bool forceLock) override;
  virtual bool ProcessFireWeapon(
    const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine, EntityAI *target,
    const RemoteFireWeaponInfo *remoteInfo, RemoteFireWeaponInfo *localInfo, bool forceLock
  ) override;
  virtual bool FireWeaponEffects(const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine,EntityAI *target,
    const RemoteFireWeaponInfo *remoteInfo) override;

  void KeyboardAny(AIBrain *unit, float deltaT);

  void KeyboardPilot(AIBrain *unit, float deltaT );
  void FakePilot( float deltaT );

  enum PathResult
  {
    PathFinished,
    PathAborted, // obstacle
    PathGoing,
  };

  PathResult PathAutopilot( const Vector3 *path, int nPath );

  /// taxi off the runway - after landing
  bool TaxiOffAutopilot();
  /// taxi in the runway - prepare for takeoff
  bool TaxiInAutopilot();

  float MakeAirborne(bool playerIn);
  float MakeAirborne(Vector3Par dir, bool playerIn);

  RString DiagText() const;

  // AI interface

  virtual bool CalculateAimWeaponPos(const TurretContext &context, const WeaponModePtr &ptr, Vector3 &pos, float &timeToLead, Target *target, bool exact) const override;
  virtual bool CalculateAimWeapon(const TurretContext &context, const WeaponModePtr &ptr, Vector3 &dir, float &timeToLead, Target *target, bool exact) const override;
  virtual bool AimWeapon(const TurretContextEx &context, const WeaponModePtr &ptr, Vector3Par direction) override;
  virtual bool AimWeaponTgt(const TurretContextEx &context, const WeaponModePtr &ptr, Target *target) override;
  Matrix4 GunTransform() const;
  virtual Vector3 GetWeaponDirection(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const override;
  virtual Vector3 GetWeaponManualControlDirection(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const override;
#if _VBS3
  virtual Vector3 GetWeaponDirectionWanted(const TurretContextEx &context, const WeaponModePtr &ptr) const override;
#endif
  virtual float GetAimed(const TurretContext &context, const WeaponModePtr &ptr, Target *target, bool exact = false, bool checkLockDelay = false) const override;

  virtual float FireInRange(const TurretContext &context, const WeaponModePtr &ptr, float &timeToAim, const Target &target) const override;

  void MoveWeapons( float deltaT );

#if _ENABLE_AI
  void AvoidCollision();
  void AvoidObstacles();
  void AIGunner(TurretContextEx &context, float deltaT);
  void AIPilot(AIBrain *unit, float deltaT );
#endif

  LSError Serialize(ParamArchive &ar);

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat
  (
    NetworkMessageClass cls,
    NetworkMessageFormat &format
  );
  TMError TransferMsg(NetworkMessageContext &ctx);
  float CalculateError(NetworkMessageContextWithError &ctx);
};

#endif

