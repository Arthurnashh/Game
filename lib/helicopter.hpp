#ifdef _MSC_VER
#pragma once
#endif

#ifndef _HELICOPTER_HPP
#define _HELICOPTER_HPP

#include "transport.hpp"
#include "planeOrHeli.hpp"
#include "shots.hpp"

#include "wounds.hpp"
#include "global.hpp"

#include "helicopterRTD.hpp"

#if  _HELISIM_ROTORLIB && !_DUMMY_ROTORLIB

enum HelicopterSystems
{
  AirspeedIndicator = 1,
  Barometer = 2,
  VerticalSpeed = 4,
  Attitude = 8,
  GMeter = 16,
  Skid = 32,
  AltitudeRadar = 64,
  TorqueMeter = 128,
  MainRotorRPM = 256,
  TailRotorRPM = 512
};

class FDMHelicpterConfig: public RotorLibFDM::GenHeli600::ConfigParams
{
  typedef RotorLibFDM::GenHeli600::ConfigParams base;
  friend HelicopterType;

  FDMHelicpterConfig():base() {};

  virtual bool	readParamStr( const std::string& parameterString , std::string& error )
  {
    LogF(" %s", parameterString.data());
    return base::readParamStr(parameterString, error);
  }

};
#endif

class HelicopterType;

class HelicopterVisualState : public TransportVisualState
{
  typedef TransportVisualState base;
  friend class Helicopter;
  friend class HelicopterAuto;
  friend class HelicopterRTD;

protected:
  float _wheelPhase, _wheelPhaseDelta;  // delta is for interpolation
  float _cyclicForward;
  float _cyclicAside;  /// main rotor thrust direction
  float _rotorPosition, _rotorPositionDelta;  // delta used for interpolation
  float _backRotorPosition;
  float _rotorDive;
  float _gearsUp;  /// actual gear position 0 means gears down, 1 gears up, negative gear damaged
  float _rotorSpeed, _mainRotor;  /// for RPM

public:
  HelicopterVisualState(HelicopterType const& type);
  virtual void Interpolate(Object *obj, ObjectVisualState const& t1state, float t, ObjectVisualState& interpolatedResult) const;
  virtual void Copy(ObjectVisualState const& src) {*this=static_cast_checked<const HelicopterVisualState &>(src);}
  virtual HelicopterVisualState *Clone() const {return new HelicopterVisualState(*this);}

  float GetGear(const HelicopterType* type) const;
  float GetRotorH(const HelicopterType* type) const;
  float GetRotorV(const HelicopterType* type) const;
  float GetRotorHDive(const HelicopterType* type) const;
  float GetRotorVDive(const HelicopterType* type) const;
  float GetCyclicFwd() const {return _cyclicForward;}
  float GetCyclicAside() const {return _cyclicAside;}
  float GetRPM() const { return 10 * _rotorSpeed * (1 - _mainRotor * 0.1); }
  float GetWheelRot() const { return _wheelPhase; }
};

#if _HELISIM_SOUNDS

struct StartStopSoundPars: public RefCount
{
  SoundPars _startStopPars[6];

  bool Load(ParamEntryPar cfg, const RString *pars, int count);

  __forceinline const SoundPars& GetPars(bool start, bool inside) const 
  { 
    return _startStopPars[(start ? 0 : 2) + (inside ? 0 : 1)];
  }

  __forceinline const SoundPars& GetDamagePars(bool inside) const
  {
    return _startStopPars[inside ? 4 : 5];
  }
};

struct ExternNoisePars: public RefCount
{
  SoundParsEx _pars;

public:
  bool Load(ParamEntryPar cfg);
  __forceinline const SoundParsEx& GetPars() const { return _pars; }
};

struct NoiseSoundPars: public RefCount
{
  // samples with probabilities
  RandomSound _randomSound;
  // expression used for sample starting
  ExpressionCode _expr;
  float _limitUp;
  float _limitDown;
  // time when new sound can be played
  float _timeDelay;

public:
  bool Load(ParamEntryPar cfg, const RString *pars, int count);
  __forceinline const SoundPars& GetPars(float probability) const { return _randomSound.SelectSound(probability); }
};

struct LandingSoundsPars: public RefCount
{
  RandomSound _intPars;
  RandomSound _extPars;

public:
  bool Load(ParamEntryPar cfg);
  __forceinline const SoundPars& GetPars(bool safe, float probability) const { return safe ? _intPars.SelectSound(probability) : _extPars.SelectSound(probability); }
};

class SoundHelper
{
public:
  //! 
  bool Switch(TransportSoundContex &tsc, const SoundPars& pars, RefAbstractWave &wave, AbstractSubmix *submix, bool loop = false);
  //!
  __forceinline AbstractWave* CreateWave(const SoundPars &pars, TransportSoundContex &ctx, AbstractSubmix *submix)
  {
    if (pars.name.GetLength() > 0 && pars.vol > 0.0f && pars.freq > 0.0f)
    {
      AbstractWave *wave = GSoundScene->OpenAndPlayOnce(
        pars.name, ctx._transport, ctx._inside, ctx._pos, ctx._speed, pars.vol, pars.freq, pars.distance);

      // include into submix
      if (wave)
        submix->Include(wave);

      return wave;
    }
    else
      return NULL;
  }
  //!
  __forceinline AbstractWave* CreateLoopWave(const SoundPars &pars, TransportSoundContex &ctx, AbstractSubmix *submix)
  {
    if (pars.name.GetLength() > 0 && pars.vol > 0.0f && pars.freq > 0.0f)
    {
      AbstractWave *wave = GSoundScene->OpenAndPlay(
        pars.name, ctx._transport, ctx._inside, ctx._pos, ctx._speed, pars.vol, pars.freq, pars.distance);

      // include into submix
      if (wave)
        submix->Include(wave);

      return wave;
    }
    else
      return NULL;
  }

  //! 
  __forceinline void DestroyWave(RefAbstractWave &wave, AbstractSubmix *submix)
  {
    if (wave)
    {
      // remove from submix
      if (submix)
        submix->Exclude(wave.GetRef());

      wave.Free();
    }
  }
};

// Switch sound between internal & external
class IntExtSoundHolder: public RefCount, SoundHelper
{
  typedef SoundHelper helper;

  const SoundPars &_intPars;
  const SoundPars &_extPars;
  Ref<AbstractSubmix> _submix;
  RefAbstractWave _wave;
  unsigned char _state;
  bool _inside;
  bool _loop;
  bool _allowSwitch;

public:
  IntExtSoundHolder(const SoundPars &intPars, const SoundPars &extPars, Ref<AbstractSubmix> &submix, bool allowSwitching = true, bool loop = false)
    : _intPars(intPars), _extPars(extPars), _submix(submix), _state(0),
    _inside(false), _allowSwitch(allowSwitching), _loop(loop){}
  ~IntExtSoundHolder() 
  {
    helper::DestroyWave(_wave, _submix.GetRef());    
    _submix.Free();
  }

  //!
  void Update(TransportSoundContex &tsc, float &vol, float &distance, int &count);
  //!
  __forceinline void DestroyWave() { _state |= 2; }
  //! 
  __forceinline void RequestWave() { _state |= 1; }

  __forceinline bool IsLoop() const { return _loop; }

private:
  __forceinline bool DestroyWanted() const { return (_state & 2) != 0; }  
  __forceinline bool CreateWanted() const { return (_state & 1) != 0; }  

  __forceinline void UnrequestWave() { _state &= ~1; }
};

class StartStopSound: public RefCount, SoundHelper
{
  typedef SoundHelper helper;

  unsigned char _state;
  bool _inside;
  const StartStopSoundPars &_pars;
  RefAbstractWave _startWave;
  RefAbstractWave _stopWave;
  Ref<AbstractSubmix> _submix;

public:
  StartStopSound(StartStopSoundPars &pars, Ref<AbstractSubmix> &submix): _state(0), _pars(pars), _submix(submix) { Initialize(); }
  ~StartStopSound() { DeInitialize(); }

  void Update(TransportSoundContex &ctx, float &distance, float &volume, int &count);
  void UnloadSound();

  __forceinline void StartWanted()  { _state |= 1; }
  __forceinline void StopWanted(bool damage)   { _state |= (damage ? (16+2) : 2); }

  __forceinline void UnmarkStarRunning()  { _state &= ~4; }
  __forceinline void UnmarkStopRunning()  { _state &= ~8; }

protected:
  __forceinline void UnmarkStartWanted()  { _state &= ~1; }
  __forceinline void UnmarkStopWanted()   { _state &= ~(16+2); }

  __forceinline bool IsStartWanted() const { return (_state & 1) != 0; }
  __forceinline bool IsStopWanted() const { return (_state & 2) != 0; }

  __forceinline void MarkStarRunning()  { _state |= 4; }
  __forceinline void MarkStopRunning()  { _state |= 8; }

  __forceinline bool IsStarRunning() const  { return (_state & 4) != 0; }
  __forceinline bool IsStopRunning() const  { return (_state & 8) != 0; }

  __forceinline bool IsStarCompleted() const  { return (_state & 4) == 0; }
  __forceinline bool IsStopCompleted() const  { return (_state & 8) == 0; }

private:
  // wave terminated callback
  void static WaveCallback(AbstractWave *wave, RefCount *context);

  __forceinline const SoundPars& SelectPars(bool start, bool inside) 
  { 
    return (((_state & 16) != 0) ? _pars.GetDamagePars(inside) : _pars.GetPars(start, inside)); 
  }
  void Initialize();
  void DeInitialize();
  void PlayStartSound(TransportSoundContex &ctx);
  void PlayStopSound(TransportSoundContex &ctx);
};

class NoisesSound: public RefCount
{
  const NoiseSoundPars &_pars;
  RefAbstractWave _wave;
  Time _pause;

public:
  NoisesSound(NoiseSoundPars &pars): _pars(pars), _pause(0) {}
  ~NoisesSound() { UnloadSound(); }

  void Update(TransportSoundContex &ctx);
  void UnloadSound();
};

class ExternNoise: public RefCount
{
  const ExternNoisePars &_pars;
  RefAbstractWave _wave;
  Ref<AbstractSubmix> _submix;

public:
  ExternNoise(ExternNoisePars &pars): _pars(pars), _submix(GSoundsys->CreateSubmix()) 
  {
    const SoundCone *cone = pars.GetPars()._coneUsed ? &pars.GetPars()._cone : NULL;
    if (_submix && cone)
    {
      _submix->SetDirSoundConePars(cone->_angleIn, cone->_angleOut, cone->_volIn, cone->_volOut);
      _submix->EnableDirectionalSound(true);
    }
  }
  ~ExternNoise() { UnloadSound(); }
  void Update(TransportSoundContex &ctx, Vector3Val front, Vector3Val top);
  __forceinline bool IsPlaying() const { return _wave.NotNull(); }
  __forceinline float AudibleDistance() const { return _pars.GetPars()._pars.distance; }

private:
  void UnloadSound()
  {
    if (_submix)
      _submix->Exclude(_wave.GetRef());

    _submix.Free();
    _wave.Free();
  }
};

class LandingSounds: public RefCount, SoundHelper
{
  typedef SoundHelper helper;

  const LandingSoundsPars &_pars;
  Ref<AbstractSubmix> _submix;
  RefAbstractWave _wave;
  bool _soundWanted;

public:
  LandingSounds(LandingSoundsPars &pars, Ref<AbstractSubmix> &submix)
    : _pars(pars), _submix(submix), _soundWanted(false) {}
  ~LandingSounds() { UnloadSound(); }

  void Update(TransportSoundContex &ctx, float &distance, float &volume, int &count);
  __forceinline void SoundWanted(bool wanted) { _soundWanted = wanted; }

private:
  void UnloadSound() { helper::DestroyWave(_wave, _submix.GetRef()); _submix.Free(); }
};

#endif

class HelicopterType: public TransportType
{
  typedef TransportType base;
  friend class Helicopter;
  friend class HelicopterVisualState;
  friend class HelicopterAuto;
  friend class HelicopterRTD;

#if  _HELISIM_ROTORLIB
public:
#if !_DUMMY_ROTORLIB
  //rtd configuration
  mutable FDMHelicpterConfig _configRTD;
#endif
  //XML path
  RString _configNameRTD;
  //rtd simulation center
  Vector3 _centerOffsetRTD;
  // rtd config is sloaded
  bool _loadedRTD;


  //battery properties
  float _maxBatteryCharge;
  float _starterBatteryDrain;
  float _electricityBatteryDrain;
  float _engineBatteryRecharge;

  // stress and damage
  float _maxTorque;
  float _maxMainRotorStress;
  float _maxTailRotorStress;
  float _maxHorizontalStabilizerLeftStress;
  float _maxHorizontalStabilizerRightStress;
  float _maxVerticalStabilizerStress;

  //auto hover correction
  Vector3 _autoHoverCorrection;
  //collective when flying heli is placed in world
  float _defaultCollective;

  float _maxEngineTemperature;
  float _hotStartTemperature;
  float _retreatBladeStallWarningSpeed;
  //throttle idle {max temperatre imcrease, change per second}
  //throttle full {max temperatre imcrease, change per second}
  //rotors RPM {max temperatre decrease, change per second}, effection is RPM / MAX_RPM
  //speed  {max temperatre decrease, change per second}, effection is SPEED / MAX_SPEED
  //temperatureEffects[] = {{800, 40},{1000, 60},{-300,50},{-200,50}};
  AutoArray<float> _temperatureProgressIdle;
  AutoArray<float> _temperatureProgressFull;
  AutoArray<float> _temperatureProgressOff;

  //collective based horizontal wing rotation (degrees)
  float _horizontalWingsAngleCollMin;
  float _horizontalWingsAngleCollMax;

  float _stressDamagePerSec; //time to break-up when stress is at 110%
  //equipped with APU    
  bool _APU;
#endif

#if _HELISIM_SOUNDS
  struct SoundParsExt
  {
    SoundPars _starterClickPars[2];
    SoundPars _idleThrottlePars[2];
    SoundPars _rotorDamagePars[2];

    Ref<StartStopSoundPars> _apuSoundPars;
    Ref<StartStopSoundPars> _starterSoundPars;
    Ref<StartStopSoundPars> _starterSoundPars1;
    Ref<NoiseSoundPars> _noiseSoundPars;
    Ref<LandingSoundsPars> _landingSoundPars;
    Ref<ExternNoisePars> _externNoisePars;

    AutoArray<SubmixPars> _submixPars;
  } _spe;
#endif

  SoundPars _soundLandingGearPars;

  protected:
  Vector3 _pilotPos; // neutral neck and head positions

  Vector3 _missileLPos,_missileRPos;
  Vector3 _rocketLPos,_rocketRPos;
  AutoArray<Vector3> _cmPos,_cmDir;
  
  // machine guns memory points
  AutoArray<Vector3> _gunPosArray;  
  AutoArray<Vector3> _gunDirArray;  

  AnimationSection _hRotorStill,_hRotorMove;
  AnimationSection _vRotorStill,_vRotorMove;

  bool _enableSweep;
	bool _gearRetracting;
  float _gearDownSpeed;
  float _gearUpSpeed;

  float _altFullForce;
  float _altNoForce;

  int _hullHit;
  int _engineHit;
  int _rotorHHit,_rotorVHit;
  int _avionicsHit;
  int _missiles;
  int _glassRHit;
  int _glassLHit;

#if _HELISIM_ROTORLIB
  int _hitBatteries;
  int _hitLight;
  int _hitHydraulics;
  int _hitTransmission;
  int _hitGear;
  int _hitFuel;
  int _hitHStabilizerL1;
  int _hitHStabilizerR1;
  int _hitVStabilizer1;
  int _hitTail;
  int _hitPitotTube;
  int _hitStaticPort;
  int _hitEngine2;
  int _hitEngine3;
  int _hitStarter1;
  int _hitStarter2;
  int _hitStarter3;
#endif

  WoundTextureSelections _glassDamageHalf;
  WoundTextureSelections _glassDamageFull;
 
  float _mainRotorSpeed;
  float _backRotorSpeed;

  // note: dive is used for Chinook.
  // Back rotor is actually second main rotor in this case
  float _minMainRotorDive;
  float _minBackRotorDive;
  float _maxMainRotorDive;
  float _maxBackRotorDive;
  float _neutralMainRotorDive;
  float _neutralBackRotorDive;

  /// lift based on speed
  AutoArray<float> _envelope;

  /// class names describing effect sources
  RString _dustEffect, _waterEffect;
  /// class name describing effect sources
  RString _damageEffect;

  // exhausts info
  AutoArray<ExhaustInfo> _exhausts;

  /// Try to get gunPos and dir from memoryPoints
  /** and store them into _gunPosArray and _gunDirArray
      at [index] place
  */
  bool ComputePosAndDirFromBegEnd(const RString &gunBeg,
                                  const RString &gunEnd, 
                                  int index);
      
  /// Try to load Memory points and build all turrets' positions and dirs
  void InitTurretsPosAndDir();
  /// Create at least one record in pos and dir arrays
  /**
    pos will be initialized to VZero, dir to VForward
  */  
  void InitDummyPosAndDir();
  
  public:
  HelicopterType( ParamEntryPar param );
  ~HelicopterType();
  
  virtual void Load(ParamEntryPar par, const char *shape);
  AnimationSource *CreateAnimationSource(const AnimationType *type, RStringB source);
      
  void InitShape();
  void DeinitShape();
  void InitShapeLevel(int level);
  void DeinitShapeLevel(int level);
  bool AbstractOnly() const {return false;}
  /// Get pos from _gunPosArray
  Vector3Val GetGunPos(int i) const;
  /// Get pos from _gunDirArray
  Vector3Val GetGunDir(int i) const;

  virtual Object* CreateObject(bool unused) const;
  virtual EntityVisualState* CreateVisualState() const { return new HelicopterVisualState(*this); }

  float GetFieldCost( const GeographyInfo &info, CombatMode mode ) const;
  float GetBaseCost(const GeographyInfo &info, bool operative, bool includeGradient) const;
  virtual float GetAirCost(float x, float z) const;
  float GetGradientPenalty(float gradient) const;
  float GetCostTurn(int difDir, int &debet, bool forceFullTurn) const;
  bool CanBeAirborne() const {return true;}
  virtual bool StrategicPlanningOnly() const {return true;}
  virtual bool CanUseRoads() const {return false;}
  virtual float GetStopDistance() const;

  Vector3 RotorUpForce( Vector3Val speed, float coef, float alt, float groundEff, bool diag ) const;

#if _HELISIM_IMPERIAL_SYSTEM
  virtual float ConvertDistance(float meters, int &stringIDS) const;
  virtual float ConvertSpeed(float metersPerSecond) const;
  virtual float ConvertHeight(float height) const;
#endif

#if _HELISIM_ROTORLIB
  void ReloadRTDConfig() const;
#endif
};


class Helicopter: public PlaneOrHeli
{
  typedef PlaneOrHeli base;

protected:
  bool _missileLRToggle;
  bool _rocketLRToggle;
  int _cmIndexToggle;
  int _gunPosIndexToogle;
    
	/// true means pilot wants gear down
	bool _pilotGear;
  /// randomize frequency to avoid interference between choppers
  float _rndFrequency;
  float _rotorSpeedWanted; // turning motor on/off

  // pilot can directly coordinate these parameters
  float _backRotor,_backRotorWanted;
  float _mainRotorWanted; // main rotor thrust
  float _cyclicForwardWanted;
  float _cyclicAsideWanted; // main rotor thrust direction

  float _rotorDiveWanted;

  Vector3 _lastAngVelocity; // helper for prediction
  Vector3 _turbulence;
  Time _lastTurbulenceTime;

  /// dust effects
  EffectsSourceRT _dustEffect, _waterEffect;
  /// damage effect
  EffectsSourceRT _damageEffect;

  //exhausts effect
  AutoArray<EffectsSourceRT> _exhausts;

  float GetGearPos() const;
  
public:
  Helicopter( const EntityAIType *name, Person *pilot, bool fullCreate = true);
  ~Helicopter();
  
  const HelicopterType *Type() const
  {
    return static_cast<const HelicopterType *>(GetType());
  }

  /// @{ Visual state
public:
  friend class HelicopterVisualState;
  HelicopterVisualState typedef VisualState;

  VisualState& FutureVisualState() { return static_cast<VisualState &>(base::FutureVisualState()); }
  VisualState const& FutureVisualState() const { return static_cast_checked<VisualState const &>(base::FutureVisualState());}
  VisualState const& CurrentVisualState() const { return static_cast_checked<VisualState const &>(base::CurrentVisualState());}
  const VisualState& RenderVisualState() const{ return static_cast<const VisualState &>(base::RenderVisualState()); }

  ProtectedVisualState<const VisualState> RenderVisualStateScope(bool ignoreParent=false) const {return ProtectedVisualState<const VisualState>(base::RenderVisualStateScope(ignoreParent));}
  ProtectedVisualState<VisualState> FutureVisualStateScope(bool ignoreParent=false) { return ProtectedVisualState<VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  ProtectedVisualState<const VisualState> FutureVisualStateScope(bool ignoreParent=false) const { return ProtectedVisualState<const VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  /// @}

  /// @{ Sound controllers (\sa Transport::Sound())
  virtual float GetRPM() const { return FutureVisualState().GetRPM(); }
  virtual float GetRenderVisualStateRPM() const { return RenderVisualState().GetRPM(); }
  virtual float GetRotorSpeed() const { return FutureVisualState()._rotorSpeed; }
  virtual float GetMainRotorThrust() const { return FutureVisualState()._mainRotor; }
  /// @}
  
  /// @{ Visual controllers (\sa Type()->CreateAnimationSource())
  virtual float GetCtrlRPM(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetRPM(); }
  float GetCtrlGear(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetGear(Type()); }  /// gear animation

  float GetCtrlRotorH(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetRotorH(Type()); }
  float GetCtrlRotorV(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetRotorV(Type()); }
  float GetCtrlRotorHDive(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetRotorHDive(Type()); }
  float GetCtrlRotorVDive(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetRotorVDive(Type()); }
  float virtual GetCtrlCyclicFwd(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetCyclicFwd(); }
  float virtual GetCtrlCyclicAside(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetCyclicAside(); }
  /// @}

  Vector3 GetFrontOrientation() const { return Vector3(0, -1, 0); }

  protected:
  float GetGlassBroken() const;

  void DamageAnimation(AnimationContext &animContext, int level, float dist2);
  void DamageDeanimation( int level );

  public:
  AnimationStyle IsAnimated( int level ) const; // appearance changed with Animate
  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
  void Deanimate( int level, bool setEngineStuff );

  const char **HUDMode(int &mask) const;

  virtual float GetFleeDamage() const;
  virtual bool IsAbleToMove() const;
  virtual bool IsPossibleToGetIn() const;
  virtual bool IsWorking() const;
  virtual bool EjectNeeded() const;

  //helicopter gears
  void virtual SetPilotGear(bool pilotGear) {_pilotGear = pilotGear;}
  __forceinline bool GetPilotGear() const {return _pilotGear;}


  /// how much flight time is estimated based on fuel tank damage and current fuel level
  float FuelTimeLeft() const;

  /// how fast is the fuel leaking (l/s)
  virtual float FuelLeakRate() const;

  void StopRotor();
  
  virtual bool RemoteStateSupported() const {return true;}

  /// amount of back rotor malfunctioning - 0 = OK, 1 = out of service
  float GetBackRotorMalfunction() const;
  void Simulate( float deltaT, SimulationImportance prec );
  void SimulateExhaust(float deltaT, SimulationImportance prec);
  //return change of thrust, used for exhaust
  float virtual GetMoreThrust();
  //check if person runs into tail rotor
  virtual void CheckPersonRotorCollision(Object *object) {};

  void Sound( bool inside, float deltaT );
  void UnloadSound();

  float GetEngineVol( float &freq ) const;
  float GetEnvironVol( float &freq ) const;

  const CursorTextureInfo *GetCursorTexture(Person *person);
  const CursorTextureInfo *GetCursorAimTexture(Person *person);

  float Rigid() const {return 0.3;} // how much energy is transfered in collision

  bool IsVirtual( CameraType camType ) const {return true;}
  //bool IsContinuous( CameraType camType ) const {return true;}
  bool HasFlares( CameraType camType ) const;

  float GetHitForDisplay(int kind) const;

  virtual Vector3 CameraPointRel(CameraType camType) const;
  virtual Vector3 CameraPosition() const;

  virtual bool EmittingWind(Vector3Par pos, float radius) const;
  virtual Vector3 WindEmit(Vector3Par pos, Vector3Par wind) const;

  float TrackingSpeed() const {return 100;}

  float GetCombatHeight() const {return 30;}
  float GetMinCombatHeight() const {return 30;}
  float GetMaxCombatHeight() const {return 100;}

  bool Airborne() const;

  bool HasHUD() const {return true;}
  const char *GetControllerScheme() const {return "Aircraft";}
  void GetAimActions(float &aimX, float &aimY) const override;

  UserAction GetManualFireAction() const;

  LSError Serialize(ParamArchive &ar);

  USE_CASTING(base)
};

class HelicopterAuto: public Helicopter
{
  typedef Helicopter base;

  protected:
  Vector3 _pilotSpeed;
  float _pilotHeading; // heading set by pilot
  float _pilotDive; // dive set by pilot

  float _forceDive; // dive necessary for aiming
  float _forceBank;
  //float _apDive,_apBank;
  float _pilotHeight;
  float _defPilotHeight; // set by scripting
  float _dirCompensate;  // how much we compensate for estimated change
  // esp. when landing

  //@{ mid-air collision avoidance - see AvoidCollision
	Time _pilotAvoidHigh;
	float _pilotAvoidHighHeight;
	Time _pilotAvoidLow;
	float _pilotAvoidLowHeight;
	//@}

  // helpers for keyboard pilot
  /// direct cyclic control
  bool _pilotCyclicDirect;
  /// keyboard helper activated
  bool _pilotHeightHelper;
  /// vertical speed autopilot
  bool _pilotVSpeedHelper;
  bool _pilotSpeedHelper; //!< speed autopilot activated
  bool _pilotDirHelper; //!< direction autopilot activated
  /// pilot height may be given as Above Ground or Above Sea.
  bool _pilotHeightAGL;

  bool _hoveringAutopilot;

  Time _analogueThrottleTime; //!< last time when user touched speed autopilot controls
  Time _thrustHelperTime; //!< last time when user touched throttle / brake controls
  Time _actualCollectiveHelperTime;

  float _lastForward; // last speed control value, used to track _thrustHelperTime
  float _lastThrotleDirect; //!< last throttle value, used to track _analogueThrottleTime

  //@{ recognize fast speed-up, fast brake
  bool _pressedForward,_pressedBack;
  bool _pressedUp,_pressedDown;
  //@}
  
#if _VBS3_AVRS
  // vector pointing to helicopter.
  // when completed a waypoint
  Vector3 _heliVector;
  // used to tell if the waypoints changed
  Vector3 _wPPos;
#endif

  
  /// keep pilot heading
  bool _pilotHeadingSet; 
  /// keep pilot dive
  bool _pilotDiveSet; 
  /// keep pilot bank
  bool _pilotBankSet;

  // non-helper interface (joystick)
  float _bankWanted,_diveWanted, _minAttackDive;

  AutopilotState _state;

  // basic helicopter tactics - perform sweep over target

  enum SweepState {SweepDisengage,SweepTurn,SweepEngage,SweepFire};
  SweepState _sweepState;
  Time _sweepDelay;
  LinkTarget _sweepTarget;
  Vector3 _sweepDir;
  
  Matrix3 _worldToRelative;
  
  Vector3 DirectionWorldToRelative(Vector3Par dir) const {return _worldToRelative*dir;}
  
  public:
  HelicopterAuto( const EntityAIType *name, Person *pilot, bool fullCreate = true );

  virtual void Move(Matrix4Par transform);
  virtual void Move(Vector3Par position);
  virtual void Init(Matrix4Par pos, bool init);
  
  virtual void Simulate( float deltaT, SimulationImportance prec );
  void AvoidGround( float minHeight );
  void AvoidCollision();

  
  virtual void GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit) override;
#if _HELISIM_PRECISE_INTERACTION
  void GetDrawActions3D(UIActions &actions, AIBrain *unit);
#endif
  virtual bool GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const override;
  virtual void PerformAction(const Action *action, AIBrain *unit) override;

  bool IsStopped() const;

  void DamageCrew( EntityAI *killer, float howMuch,RString ammo );
  void Eject(AIBrain *unit, Vector3Val diff = VZero);

  float MakeAirborne(bool playerIn);
  void SetFlyingHeight(float val);

  float GetFlyingHeight();
  float GetSpeedWanted() {return _pilotSpeed[2];} 

  virtual void EngineOn();
  virtual void EngineOff();
  void EngineOffAction();
  bool EngineIsOn() const;
  bool IRSignatureOn() const;

  virtual bool FireWeapon(const TurretContextV &context, const WeaponModePtr &ptr, TargetType *target, bool forceLock) override;
  virtual bool ProcessFireWeapon(
    const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine, EntityAI *target,
    const RemoteFireWeaponInfo *remoteInfo, RemoteFireWeaponInfo *localInfo, bool forceLock
  ) override;
  virtual bool FireWeaponEffects(const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine,EntityAI *target,
    const RemoteFireWeaponInfo *remoteInfo) override;

  float FireValidTime() const {return 45;}

  void SuspendedPilot(AIBrain *unit, float deltaT );
  void KeyboardPilot(AIBrain *unit, float deltaT );

  virtual void AutohoverOn();
  virtual void AutohoverOff();

  virtual bool GetAutoHover() {return _hoveringAutopilot;}

  void FakePilot( float deltaT );

  Vector3 GetStopPosition() const;

  virtual float GetFeatureSize() const {return !_landContact && !_objectContact && !GetStopped() ? 25.0f : 0;} // pretend we are a feature, so that object size limits are not applied to us

  void Autopilot(float deltaT, Vector3Par targetCur, Vector3Par targetEst, Vector3Par tgtSpeed, Vector3Par direction);
  
  void ResetAutopilot();
  void BrakingManeuver();

  // AI interface
  virtual bool AimWeapon(const TurretContextEx &context, const WeaponModePtr &ptr, Vector3Par direction) override;
  virtual bool AimWeaponTgt(const TurretContextEx &context, const WeaponModePtr &ptr, Target *target) override;
  virtual bool CalculateAimWeaponPos(const TurretContext &context, const WeaponModePtr &ptr, Vector3 &pos, float &timeToLead, Target *target, bool exact ) const override;
  virtual bool CalculateAimWeapon(const TurretContext &context, const WeaponModePtr &ptr, Vector3 &dir, float &timeToLead, Target *target, bool exact ) const override;

  virtual Vector3 GetWeaponPoint(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const override;
  virtual Vector3 GetWeaponCenter(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const override;
  virtual Vector3 GetWeaponDirection(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const override;
  virtual Vector3 GetWeaponManualControlDirection(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const override;
  virtual Vector3 GetWeaponManualControlPoint(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const override;
  virtual Vector3 GetWeaponDirectionWanted(const TurretContextEx &context, const WeaponModePtr &ptr) const override;
  virtual float GetAimed(const TurretContext &context, const WeaponModePtr &ptr, Target *target, bool exact = false, bool checkLockDelay = false) const override;

  virtual float FireInRange(const TurretContext &context, const WeaponModePtr &ptr, float &timeToAim, const Target &target) const override;
  virtual float FireAngleInRange(const TurretContext &context, const WeaponModePtr &ptr, Vector3Par rel) const override;

  void MoveWeapons( float deltaT );

#if _ENABLE_AI
  void AIPilot(AIBrain *unit, float deltaT );
#endif

  void DrawDiags();
  RString DiagText() const;

  LSError Serialize(ParamArchive &ar);

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat
    (
    NetworkMessageClass cls,
    NetworkMessageFormat &format
    );
  static HelicopterAuto *CreateObject(NetworkMessageContext &ctx);
  TMError TransferMsg(NetworkMessageContext &ctx);
  float CalculateError(NetworkMessageContextWithError &ctx);

#if _HELISIM_ROTORLIB
  virtual bool IsRTD() {return false;}
#endif
protected:
  void SwitchPilotHeightAGL(bool agl);

  USE_CASTING(base)
};

#if _HELISIM_ROTORLIB

struct EngineTemperature
{
public:
  AutoArray<float> _currentElementTemperature;
  float _temperature;

  EngineTemperature(bool airbourne = false);
  void Simulate(float deltaT, Ref<HelicopterRTD> helicopter, int engineIndex);
  void SetHot(Ref<HelicopterRTD> helicopter, int engineIndex);
  float GetTemperature() const;

  LSError Serialize(ParamArchive &ar);
};
TypeIsMovable(EngineTemperature);

//helicopter using RotorLib FMD
class HelicopterRTD: public HelicopterAuto
{
private:
  typedef HelicopterAuto base;

  // GenHeli600 is the helicopter FDM model we want to use
  GenHeli600Arma* _rtdHelicopter;

  //autohover controller
#if !_DUMMY_ROTORLIB
  FCSAutoHoverTOH* _autoHoverRTD;
#else  
  void* _autoHoverRTD;
#endif

  // age of the RTD state relative to FutureVisualState (it's newer, so it should be <= 0)
  float _rtdStateAge;

  //actual collective
  float _actualCollectiveRTD;
  bool _autoTrimEnabled;
  bool _autoTrimEnabledState;
  bool _controllsTrim;

  float _rudderWanted;
  float _collectiveWanted;

  //trimmed controls 
  float _cyclicSideRTDOffset;
  float _cyclicForwardRTDOffset;
  float _backRotorRTDOffset;
  Time _delayAfterTrimming;

  //start up values
  int _starter; //use one bit for each starter (is set to true, if battery of Apu is on)
  int _starterWanted;  //use one bit for each starter
  Vector3 _starterTime;

  float _APU;
  bool _batteryOn;
  float _batteryCharge;
  bool _isAutoStartUpEnabledRTD;

  //true if any of ground contacts is brakeable
  bool _isRTDGearBrakeAble;

  //APU starting
  float _APUStartingTime;
  float _APUwanted;
  float _APUchange;

  // Starter RPM - used for sound controller
  float _rpmStarter[3];
  float _rpmStarterOffTime[3];

  float _cyclicSideRTD;
  float _cyclicForwardRTD; 
  float _pedals;

  //weight
  float _fuselageWeight;
  float _crewWeight;
  float _customWeight;
  float _ammoWeight;
  Time _weightUpdateTime;

  //switch from player RTD simulation to AI arma simulation
  bool _isRTDSimulated;
  //we want to switch to RTD, but RTD heli is not ready
  bool _isRTDSimulatedWanted;
  //in some time change sim. even if RTD model is not ready
  Time _timeToForcedChange;

  Vector3 _centerOffsetRTD;
  //last tested position
  int _lastRotorColAngle;

  //animsources
  float _cyclicAsideAnimation, _cyclicForwardAnimation;

  AutoArray<float> _lastHit; //!< HitPoints (local damage), - state in last frame -> to reduce changes

  //enabled rotorlib simulation
  bool _enabledRTDSimulation;

#if _HELISIM_SOUNDS
  struct SoundsExt
  {
    // submix
    Ref<AbstractSubmix> _submix;
    // apu
    Ref<StartStopSound> _apuSound;
    // starter 1
    Ref<StartStopSound> _starterSound;
    // starter 2
    Ref<StartStopSound> _starterSound1;
    // noises
    Ref<NoisesSound> _noisesSound;
    // landing sounds
    Ref<LandingSounds> _landingSound;
    // single samples
    Ref<IntExtSoundHolder> _starterClickSound;
    Ref<IntExtSoundHolder> _idleThrottleSound;
    Ref<IntExtSoundHolder> _idleThrottleSound1;
    Ref<IntExtSoundHolder> _rotorDamageSound;

    // sound controllers
    RefArray<SoundsHolder> _sounds;

    // 
    Ref<ExternNoise> _externNoise;

    bool _gearUpDownSoundWanted;
    //
    RefAbstractWave _gearUpDownSound;

    void Destroy()
    {
      _starterClickSound.Free();
      _idleThrottleSound.Free();
      _idleThrottleSound1.Free();
      _rotorDamageSound.Free();
      _externNoise.Free();

      _apuSound.Free();
      _starterSound.Free();
      _starterSound1.Free();
      _noisesSound.Free();
      _landingSound.Free();

      _submix.Free();

      for (int i = 0; i < _sounds.Size(); i++)
        _sounds[i].Free();

      _sounds.Clear();

      _gearUpDownSound.Free();
    }

    void DestroyWave(RefAbstractWave &wave)
    {
      if (wave)
      {
        if (_submix) _submix->Exclude(wave);
        wave.Free();
      }
    }
  } _se;
#endif

  //temperatureEffects[] = {{800, 40},{1000, 60},{-300,50},{-200,50}};
  AutoArray<EngineTemperature> _enginesTemperature;

public:
  HelicopterRTD( const EntityAIType *name, Person *pilot, bool fullCreate = true );

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat
    (
    NetworkMessageClass cls,
    NetworkMessageFormat &format
    );
  static HelicopterRTD *CreateObject(NetworkMessageContext &ctx);
  TMError TransferMsg(NetworkMessageContext &ctx);
  float CalculateError(NetworkMessageContextWithError &ctx);

  //Simulate
  virtual void Init( Matrix4Par pos, bool init );
  virtual void Simulate(float deltaT, SimulationImportance prec);
  void SimulateDamage(float deltaT, SimulationImportance prec);
  void SimulateWeaponActivity( float deltaT, SimulationImportance prec );
  virtual void KeyboardPilot(AIBrain *unit, float deltaT );
  void OnSimulationChanged(bool changeToRTD);//arma/rtd simulation switch
  void DisableSimulation(bool disable);
  virtual void CheckPersonRotorCollision(Object *object);

  //Simulate RTD
  float StepSimulated(float step);  //to synchronize with RotorLib simulation step
  virtual bool IsRTD(){ return (_rtdHelicopter == NULL)? false:true;} //current use of RTD simulation
  __forceinline bool IsRTDSimulated(){ return  _isRTDSimulated;};
  bool IsRTDSimulationWanted();
  void ResetHelicopterRTD();  //reset RTD helicopter state

  //Controlers
  void SetActualCollective(float value); 
  __forceinline float GetCollective(ObjectVisualState const& unused) const {return _actualCollectiveRTD;};
  __forceinline float GetRudder(ObjectVisualState const& unused) const {return _rudderWanted;};
  __forceinline bool IsAutoStartUpEnabledRTD() {return _isAutoStartUpEnabledRTD;};
  __forceinline void EnableAutoStartUpRTD(bool isAutoStartUpEnabledRTD) {_isAutoStartUpEnabledRTD = isAutoStartUpEnabledRTD;}


  virtual float MakeAirborne(bool playerIn);

  LSError Serialize(ParamArchive &ar);

  // Engine
  bool EngineIsOn() const;
  bool EngineIsOn(int index) const;
  //only engine on
  virtual void EngineOn();
  //engine on and set wanted RPM
  virtual void EngineOn(bool quickStart);
  virtual void EngineOff();
  virtual void EngineOffAction();
  void Diag_quickStartRTDHeli();

 
  void virtual SetPilotGear(bool pilotGear) { _pilotGear = (Type()->_gearRetracting)? pilotGear : true; }

  //Wings
  ///set wings effectivity  
  __forceinline void SetWingForceScaleRTD(float scale, int index) 
  {
#if !_DUMMY_ROTORLIB
      if(_rtdHelicopter) _rtdHelicopter->SetWingForceScale(scale, index);
#endif
  }
  __forceinline void GetWingsPositionRTD(AutoArray<Vector3> &values) 
  {
#if !_DUMMY_ROTORLIB
    if(_rtdHelicopter) return _rtdHelicopter->GetWingsPosition(values);
#endif
  }

  __forceinline void GetWingsOrientationRTD(AutoArray<Vector3> &values) 
  {
#if !_DUMMY_ROTORLIB
    if(_rtdHelicopter) _rtdHelicopter->GetWingsOrientation(values);
#endif
  }

  ///force produced by wings
  void GetWingsForcesRTD(AutoArray<Vector3> &values);

  ///enable auto trim control
  bool SetAutoTrim(bool autoTrim);
  bool GetAutoTrim()const {return _autoTrimEnabledState;};
  Vector3 GetTrimOffsets();
  // auto hover
  bool SetAutoHover(bool autoHover);
  virtual void AutohoverOn();
  virtual void AutohoverOff();

  // set brakes
  __forceinline void SetWheelsBrakeRTD(float brakeValue, float index) 
  {
#if !_DUMMY_ROTORLIB
    if(_rtdHelicopter) _rtdHelicopter->SetWheelsBrake(brakeValue, index);
#endif
  }
  __forceinline void SetRotorBrake(float value) 
  {
#if !_DUMMY_ROTORLIB
    if(_rtdHelicopter) _rtdHelicopter->SetRotorBrake(value);
#endif
  }
  __forceinline float GetRotorBrake() 
  {
#if !_DUMMY_ROTORLIB
    if(_rtdHelicopter) return _rtdHelicopter->GetRotorBrake(); else return 0;
#else
    return 0;
#endif
  }


  //Engine
  ///number of engines
  int GetNumberOfEnginesRTD();
  ///working engines 
  void GetEnginesOnRTD(AutoArray<bool> &values);
  //engines throttle
  void SetThrottleRTD(float value, int index);
  float GetThrottleRTD(int engineIndex);
  void GetThrottleRTD(AutoArray<float> &values);
  //target RPM
  float GetEngineTargetRPM(int engineIndex);
  void GetEngineTargetRPM(AutoArray<float> &RPMs);
  //set engine rpm
  void SetEngineRpmRTD(float value, int index) {};
  void SetWantedEngineRPM(float wantedEngineRPM, float time, int index) 
  { 
#if !_DUMMY_ROTORLIB
    if(_rtdHelicopter) _rtdHelicopter->SetWantedEngineRPM(wantedEngineRPM, time, index); 
#endif
  }
  //temperature
  void GetEnginesTemperatureRTD(AutoArray<float> &values);

  void GetEnginesPowerRTD(AutoArray<float> &values);
  void GetEnginesTorqueRTD(AutoArray<float> &values);
  void DecreasetEngineTemperatureRTD();
  ///engines rpm
  float GetAverageRPMScaled();

  //since RTD does not have separate clutches or each engine, this command return RPM limeted by wanted RPM (so no RPM is gained from rotor) 
  void GetRPM(AutoArray<float> &values) 
  {
#if !_DUMMY_ROTORLIB
    if(_rtdHelicopter) _rtdHelicopter->GetEnginesRpm(values);
#endif
  }
  virtual float GetRPM() const { return GetRPM(0); }
  virtual float GetRPM(int idx) const;

  //Rotors
  Vector3 GetRotorsRpmRTD() ;//rotors rpm
  void GetRotorsForcesRTD(AutoArray<Vector3> &values); //force produced by rotors

  ~HelicopterRTD();
  //called from _rtdHelicopter destructor (needed for case when _rtdHelicopter is deleted from RTD world destruction)
  void RemoveRTDHelicopter() {_rtdHelicopter = NULL;};

  // copy transformations and inform arma, that object has moved
  void CopyTransformArmaToRTD();
  void CopyTransformRTDToArma(float t);
  void CopyRotationArmaToRTD();

  // overwritten object methods
  virtual void Move(Matrix4Par transform);
  virtual void Move(Vector3Par position);
  virtual void SetOrient(Matrix3 const& dir);
  virtual void SetDirectionAndUp( Vector3PPar dir, Vector3PPar up );
  virtual void SetOrientation( Matrix3P const& m );
  virtual void SetUpAndDirection( Vector3PPar up, Vector3PPar dir );

  virtual void Sound(bool inside, float deltaT);
  virtual void UnloadSound();
  virtual void SetSpeed(Vector3Par speed); // camera needs to know object speed


  //Actions
  virtual void GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit) override;
#if _HELISIM_PRECISE_INTERACTION
  void GetDrawActions3D(UIActions &actions, AIBrain *unit);
#endif
  virtual bool GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const override;
  virtual void PerformAction(const Action *action, AIBrain *unit) override;

  //Weight
  //at this moment only RTD helicopter is affected by weight
  void AddCrewWeight();
  void RemoveCrewWeight();
  void UpdateWeight();
  void GetWeight(AutoArray<float> &values);
  void SetCustomWeight(float weight) {_customWeight = weight; UpdateWeight();}

  //APU
  void SetAPU(float APU, float time);
  float GetAPU() const {return _APU;};

  //Battery
  void SetBatteryCharge(float value) {_batteryCharge = value;}
  float GetBatteryCharge() const {return _batteryCharge;} ;
  bool IsBatteryOn() const {return _batteryOn;};
  void SetBatteryOn(bool batteryOn);

  //Starter
  virtual float GetRPMStarter(int idx) const;
  int GetStarterWanted() { return _starterWanted;};
  bool GetStarter(int index) const { return (_starter & (1 << index)) == (1 << index); }
  bool GetStarter(int starter, int index) const { return (starter & (1 << index)) == (1 << index); }
  void UpdateStarter(); // value is based on battery / apu
  void SetStarterWanted(bool starter, int index);

  //helicopter damage
  virtual void SetTotalDamageHandleDead(float damage, bool setHitpoints) override;
  float FuelLeakRate() const;
  __forceinline float GetMainRotorDamage() {return (Type()->_rotorHHit >=0) ? _hit[Type()->_rotorHHit] : 0; };
  __forceinline float GetTailRotorDamage() {return (Type()->_rotorVHit >=0) ? _hit[Type()->_rotorVHit] : 0; };
  __forceinline float GetHydraulicsDamage() {return (Type()->_hitHydraulics >=0) ? _hit[Type()->_hitHydraulics] : 0; };
  __forceinline float GetBatteryDamage() {return (Type()->_hitBatteries >=0) ? _hit[Type()->_hitBatteries] : 0; };
  float GetEngineMaxDamage() const;
  float GetEngineRPMState() const;
  bool IsHotStart() const;
  float GetMaxEngineTemperatureScaled() const;
  virtual float GetMotorDamage() const;
  virtual float GetTransmissionDamage() const;
  float virtual GetDamageShakePower();

  //animation sources
  float GetAirspeedIndicatorRTD(ObjectVisualState const& vs) const;
  float GetAltBaroRTD(ObjectVisualState const& vs) const;
  float GetVertSpeedRTD(ObjectVisualState const& vs) const;
  float GetAttitudeBankRTD(ObjectVisualState const& vs) const;
  float GetAttitudeDiveRTD(ObjectVisualState const& vs) const;
  void GetAttitude(ObjectVisualState const& vs, float &pitch, float &bank);
  float GetAltRadarRTD(ObjectVisualState const& vs) const;
  float GetAccelerometer(ObjectVisualState const& vs) const;
  float GetTorqueRTD(ObjectVisualState const& vs) const;
  float GetThrottle1(ObjectVisualState const& unused) const;
  float GetThrottle2(ObjectVisualState const& unused) const;
  float GetThrottle3(ObjectVisualState const& unused) const;
  float GetSkidRTD(ObjectVisualState const& vs) const;
  float virtual GetCtrlCyclicFwd(ObjectVisualState const& vs) const;
  float virtual GetCtrlCyclicAside(ObjectVisualState const& vs) const;
  float GetCtrlWheelRot(ObjectVisualState const& vs) const;

  //animation sources - scaled to 0-1
  float GetBatteryChargeScaled(ObjectVisualState const& vs)  const;
  float GetTorqueRTDScaled(ObjectVisualState const& vs) const;
  float GetTorqueEng1RTDScaled(ObjectVisualState const& vs) const;
  float GetTorqueEng2RTDScaled(ObjectVisualState const& vs) const;
  float GetTorqueEng3RTDScaled(ObjectVisualState const& vs) const;
  float GetRpm1RTDScaled(ObjectVisualState const& vs) const;
  float GetRpm2RTDScaled(ObjectVisualState const& vs) const;
  float GetRpm3RTDScaled(ObjectVisualState const& vs) const;
  float GetTemp1RTDScaled(ObjectVisualState const& unused) const;
  float GetTemp2RTDScaled(ObjectVisualState const& unused) const;
  float GetTemp3RTDScaled(ObjectVisualState const& unused) const;
  float GetMainRotorRPMRTDScaled(ObjectVisualState const& unused) const;
  float GetTailRotorRPMRTDScaled(ObjectVisualState const& unused) const;
  //@}

  float GetRadarAltMeterPosition();
  /// returns true if object is touching ground
  bool IsLandContact();

  virtual float GetGMeterZ() const;

  virtual float Thrust() const;
  //return change of thrust, used for exhaust
  float GetMoreThrust() 
  {
#if !_DUMMY_ROTORLIB
    return (_rtdHelicopter)? GetAverageRPMScaled(): base::GetMoreThrust();
#else
    return base::GetMoreThrust();
#endif
  }

  //LIGHTS
  virtual bool IsPilotLight() const {return (base::IsPilotLight() && IsElectricity());}
  virtual bool IsCollisionLight() const {return (base::IsCollisionLight() && IsBatteryOn() && IsElectricity());}

  virtual bool IsElectricity() const {return (IsBatteryOn() && GetBatteryChargeScaled(RenderVisualState()) > 0.05);}

protected:
#if _HELISIM_SOUNDS
  __forceinline void PlayApuSound(bool enable)
  {
    if (_se._apuSound.IsNull()) return;

    if (enable)
      _se._apuSound->StartWanted();
    else
      _se._apuSound->StopWanted(false);
  }
  __forceinline void PlayStarterSound(bool play, int index)
  {
    PlayStarterClickSound(play, index);
    
    Ref<StartStopSound> starterSound;

    if (index == 0)
      starterSound = _se._starterSound;

    if (index == 1)
      starterSound = _se._starterSound1;

    if (starterSound.IsNull()) return;

    if (play)
      starterSound->StartWanted();
    else
    {
      const int starterIdx[] = { Type()->_hitStarter1, Type()->_hitStarter2, Type()->_hitStarter3 };
      starterSound->StopWanted(_hit[starterIdx[index]] > 0.1f);
    }
  }
  __forceinline void PlayIdleThrottleSound(int index)
  {
#if !_DUMMY_ROTORLIB
    if (_rtdHelicopter)
    {
      float engRpm = _rtdHelicopter->GetEnginesRpm(index)/_rtdHelicopter->GetEngineTargetRPM(index);
      bool starterOn = (_starter & index) == index;

      bool result = starterOn && (engRpm > 0.05f) && (engRpm < 0.4f);

      Ref<IntExtSoundHolder> &sound = (index == 0) ? _se._idleThrottleSound : _se._idleThrottleSound1;

      if (result && sound)
        sound->RequestWave();
    }
#endif
  }
  __forceinline void PlayStarterClickSound(bool play, int index)
  {
    if (_se._starterClickSound)
    {    
      DoAssert(_se._starterClickSound->IsLoop());

      if (play)
        _se._starterClickSound->RequestWave();
      else
        _se._starterClickSound->DestroyWave();
    }
  }
  __forceinline void PlayRotorDamageSound()
  {
    if (_se._rotorDamageSound)
    {
      DoAssert(!_se._rotorDamageSound->IsLoop());
      _se._rotorDamageSound->RequestWave();     
    }
  }
  //__forceinline void PlayEngineDamageSound(bool play)
  //{
  //  if (_se._engineDamageSound)
  //  {
  //    if (play)
  //      _se._engineDamageSound->RequestWave();
  //    else
  //      _se._engineDamageSound->DestroyWave();
  //  }
  //}
  __forceinline void PlayLandingSound(bool landingWanted)
  {
    if (landingWanted && _se._landingSound)
      _se._landingSound->SoundWanted(landingWanted);
  }
  __forceinline void PlayGearUpDownSound(bool up)
  {
    if (!_landContact)
      _se._gearUpDownSoundWanted = true;
  }
#endif
  USE_CASTING(base)
};

#endif

#endif

