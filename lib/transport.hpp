#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TRANSPORT_HPP
#define _TRANSPORT_HPP

/*!
\file
interface for Transport class
*/

#include "vehicleAI.hpp"
#include "hudDesc.hpp"
#include "speaker.hpp"
#include "person.hpp"
#include "landscape.hpp"
#include "camShake.hpp"
#include "global.hpp"

struct ManCargoItem
{
  Ref<Person> man;
  //float animTime;
  /// cargo position locked  (nobody can get in / move to it)
  bool _locked;

  ManCargoItem() : _locked(false) {}
  LSError Serialize(ParamArchive &ar);
};
TypeIsMovableZeroed(ManCargoItem)

//typedef RefArray<Person> ManCargo;
class ManCargo : public AutoArray<ManCargoItem>
{
  typedef AutoArray<ManCargoItem> base;
public:
  Person *operator [](int i) const
  {return base::operator [](i).man;}
  bool IsLocked(int i) const {return base::operator [](i)._locked;}
  void Lock(int i, bool lock) {base::operator [](i)._locked = lock;}
};

class AnimationSourceDoor : public AnimationSource
{
  typedef AnimationSource base;
protected:
  /// door identification
  int _index;

  float _animSpeed;

public:  
  AnimationSourceDoor(const RStringB &name,const int index, const float animPeriod) :base(name),_index(index),_animSpeed(1/animPeriod) {};
  float GetPhase(const ObjectVisualState &vs, const Animated *obj) const;
  void SetPhaseWanted(Animated *obj, float phase) {};
  int GetIndex() const {return _index;};
  float GetAnimSpeed() const {return _animSpeed;};
};

class DoorsHolder
{
  friend class DoorsState;  

  /// total count of doors
  AutoArray< InitPtr<AnimationSourceDoor> > _sources;

public:
  AnimationSource *CreateAnimationSource(ParamEntryPar cfg);

  InitPtr<AnimationSourceDoor> operator [] (int i) const {return _sources[i];}
  int Count() const {return _sources.Size();}

  int GetDoorIndex(const char *name) const;
};

struct DoorInfo
{
  float _pos;
  bool _open;
};
TypeIsSimple(DoorInfo);

class DoorsState
{
  Buffer<DoorInfo> _door;
 
public:
  void Init(const DoorsHolder &type);

  void Set(int index, float val) {_door[index]._pos=val;};

  //@{ array style access to door index  
  DoorInfo &operator [] (int i) {return _door[i];}
  DoorInfo operator [] (int i) const {return _door[i];}

  int Count() const {return _door.Length();}
  //@}

  void Interpolate(DoorsState const& t1state, float t, DoorsState& interpolatedResult) const;
};

#include "turret.hpp"
#include "AI/pathPlanner.hpp"
  
enum VehicleMoveMode
{
  VMMFormation,
  VMMMove,
  VMMBackward,
  VMMStop,
  VMMSlowForward,
  VMMForward,
  VMMFastForward,
};

enum VehicleTurnMode
{
  VTMNone,
  VTMLeft,
  VTMRight,
  VTMAbs,
};


typedef RStringB ManAnimationType;

/// proxy crew information for given level
struct LevelProxies
{
  ManProxy _driverProxy; // positioning of different crew members
  AutoArray<ManProxy> _cargoProxy;
  /// count proxies which should be enumerated as normal
  AutoArray<int> _normalProxy;
  
  void Clear();
};
TypeIsMovable(LevelProxies)

//////////////////////////////////////////////////////////////////////////

// used for directional sound, store cone info
struct SoundCone
{
  float _volIn;
  float _volOut;
  float _angleIn;
  float _angleOut;

  bool  operator == (const SoundCone &with) const
  {
    const float Delta = 0.05f;

    return (
      fabs(_volIn - with._volIn) < Delta &&
      fabs(_volOut - with._volOut) < Delta &&
      fabs(_angleIn - with._angleIn) < Delta &&
      fabs(_angleOut - with._angleOut) < Delta
      );
  }
};

TypeIsSimple(SoundCone)

// used for audio controllers
struct SoundParsEx
{
  SoundPars _pars;
  ExpressionCode _freqExpr;
  ExpressionCode _volExpr;
  SoundCone _cone;
  bool _coneUsed;
};

TypeIsMovable(SoundParsEx)

// used for audio events
struct SoundEventPars
{
  SoundPars _pars;
  ExpressionCode _expression;
  float _limit;
  bool _started;
  SoundCone _cone;
  bool _coneUsed;
};

TypeIsMovable(SoundEventPars)

struct SubmixPars: public AutoArray<SoundParsEx>
{
  typedef AutoArray<SoundParsEx> base;

  SubmixPars() {}
  ~SubmixPars() { base::Clear(); }
};

TypeIsMovable(SubmixPars)

struct SubmixEventPars
{
  AutoArray<SoundEventPars> _sEventPars;

  SubmixEventPars(): _sEventPars() {}
  ~SubmixEventPars() { _sEventPars.Clear(); }
};

TypeIsMovable(SubmixEventPars)

struct ExhaustInfo
{
  Vector3 position;
  Vector3 direction;
  RString effect;
};
TypeIsMovableZeroed(ExhaustInfo)

//////////////////////////////////////////////////////////////////////////

class TransportType;


class TransportVisualState : public EntityVisualState
{
  typedef EntityVisualState base;
  friend class Transport;

protected:
  float _fuel;
  float _driverHidden;
  TurretVisualStateArray _turretVisualStates;

  DoorsState _doors;

private:
  TransportVisualState();  // don't call the implicit ctor!

public:
  bool _IKIdent;
  Matrix4 _leftHandIK;
  Matrix4 _rightHandIK;
  Matrix4 _leftLegIK;
  Matrix4 _rightLegIK;

  explicit TransportVisualState(TransportType const& type);

  virtual void Interpolate(Object *obj, ObjectVisualState const& t1state, float t, ObjectVisualState& interpolatedResult) const;  // TODO
  virtual void Copy(ObjectVisualState const& src) { *this = static_cast_checked<const TransportVisualState &>(src); }
  virtual TransportVisualState *Clone() const {return new TransportVisualState(*this);}

  float GetAltRadar(Ref<LODShape> shape) const;
  float GetAltBaro() const;
  float GetSpeedHorizontal() const;
  float GetSpeedVertical() const;
  float GetDirection() const;
  float GetDriverHatchPos() const { return 1.0f - _driverHidden; }
  float GetHorizonBank() const;
  float GetHorizonDive() const;
  float GetFuelFraction(TransportType const* type) const;
  float GetFuel() const { return _fuel; }
  float GetGMeter() const;
  float GetGMeterX() const;
  float GetGMeterY() const;
  virtual float GetGMeterZ() const;

  bool IsDriverHidden() const {return _driverHidden > 0.1f;}

  TurretVisualState* GetTurret(const int *path, int size);
  TurretVisualState const* GetTurret(const int *path, int size) const;

  virtual float GetDamperState(int index) const { return 0.0f; }  // damper state for visual and sound controllers

  float GetDoorState(int index) const { return (index < _doors.Count()) ? _doors[index]._pos : 0.0f; };
  void  SetDoorState(int index, float val) { if (index < _doors.Count()) _doors.Set(index,val); };
  bool  GetDoorOpen(int index) { return (index < _doors.Count()) ? _doors[index]._open : false; };
  void  SetDoorOpen(int index, bool val) { if (index < _doors.Count()) _doors[index]._open = val; };
};

//////////////////////////////////////////////////////////////////////////

/// search the vehicle config for the turret with primary gunner flag set
bool HasPrimaryGunnerTurret(ParamEntryPar cls);
/// search the vehicle config for the turret with primary observer flag set
bool HasPrimaryObserverTurret(ParamEntryPar cls);

class TransportType: public EntityAIType
{
  friend class Transport;
  friend class TransportVisualState;
  friend class Man;

  typedef EntityAIType base;

protected:
  Buffer<LevelProxies> _proxies;
  RString _crew;

  /// list of MFDs
  AutoArray<MFDDesc> _mfd;  

  /// doors
  DoorsHolder _doors;  

  PlateInfos _squadTitles;

  //@{
  //! Action names corresponding to crew position
  RString _driverAction;
  RString _driverInAction; // actions inside vehicle (turn in)
  RString _driverGetInAction;
  RString _driverGetOutAction;
  AutoArray<RString> _cargoAction;
  AutoArray<RString> _cargoGetInAction;
  AutoArray<RString> _cargoGetOutAction;
  //@}

  GetInOutMode _preciseGetInOut;   
  AutoArray<GetInOutMode> _cargoPreciseGetInOut;


  //@{ shadows should be casted in some internal (cockpit) views, light may be attenuated
  bool _viewDriverShadow;
  bool _viewCargoShadow;
  float _viewDriverShadowDiff;
  float _viewCargoShadowDiff;
  float _viewDriverShadowAmb;
  float _viewCargoShadowAmb;
  //@}

  //@{ some crew position should be ejected when killed
  bool _ejectDeadDriver;
  bool _ejectDeadCargo;
  //@}

  //@{ weapon should be hidden for some crew positions
  bool _hideWeaponsDriver;
  bool _hideWeaponsCargo;
  //@}

  GetInPoints _driverGetInPos;
  // store get in points for each position separately
  AutoArray<GetInPoints> _cargoGetInPos;
  float _getInRadius;

#if _HELISIM_PRECISE_INTERACTION
  GetInPoints _driverGetInPosPrecise;
  AutoArray<GetInPoints> _cargoGetInPosPrecise;
#endif

  /// where driver can switch position
  int _driverCompartments;
  /// where cargo can switch position
  AutoArray<int> _cargoCompartments;

  int _driverDoors;
  AutoArray<int> _cargoDoors;

  float _insideSoundCoef;
  bool _outsideSoundFilter;
  //@{ vehicle body sound absorption characteristics
  float _occludeSoundsWhenIn;
  float _obstructSoundsWhenIn;
  //@}

  bool _hideProxyInCombat;
  bool _forceHideDriver;

#if _VBS3 //added optional parameter _viewDriverInExternal
  bool _viewDriverInExternal;
#endif


  bool _unloadInCombat;

  bool _hasDriver;
  bool _hasGunner;
  bool _hasObserver;
  // the driver commands the vehicle
  // CHANGED: before, the meaning was "driver commands more than gunner"
  bool _driverIsCommander;
  bool _driverForceOptics;

  /// crew is sitting always out or is vulnerable when inside
  bool _crewVulnerable;

  /// Manual Fire action is available for this vehicle
  bool _enableManualFire;

  // _viewDriver is member of EntityAIType
  ViewPars _viewCargo;
  ViewPars _viewOptics;
  AutoArray<VehPosRelatives> _cargoSpec;

  int _driverOpticsPos;

  Ref<LODShape> _driverOpticsModel;

  RefArray<EntityAIType> _typicalCargo;

  AutoArray<int> _cargoProxyIndexes; 

  PackedColor _driverOpticsColor;
	
  int _maxManCargo;

  AutoArray<SubmixPars> _submixPars;
  AutoArray<SubmixEventPars> _submixEventPars;
  
  // driver optics post effects
  AutoArray<PPEffectType> _opticsType;

  // camera shake that is caused by this transport passing near player
  CameraShakePars _camShake;
  // coefficient that is applied to all camera shakes, when the player is inside this vehicle
  float _camShakeCoef;

  float _wheelCircumference;

#if _HELISIM_RENDER_TO_TEXTURE
  AutoArray<R2TTgtDescription> _r2tTgtDesc;
#endif
#if _ENABLE_HAND_IK // hand IK 
  /// skeleton bone indexes to attach drivers hands
  RString _driverLeftHandAnimName,_driverRightHandAnimName;
  RString _driverLeftLegAnimName,_driverRightLegAnimName;
  SkeletonIndex _driverLeftHandIndex,_driverRightHandIndex;
  SkeletonIndex _driverLeftLegIndex,_driverRightLegIndex;

#endif

protected:
  void AddProxy(
    ManProxy &mProxy,
    const ProxyObjectTyped &proxy, const ProxyObject &proxyObj
  );

  // Load sound parameters
  void LoadSubmixGroups(ParamEntryPar par, RString className, AutoArray<SubmixPars> &submixPars);

  /// list of attached turrets types
  RefArray<TurretType> _turrets;

public:
  TransportType( ParamEntryPar param );
  virtual void Load(ParamEntryPar par, const char *shape);

  AnimationSource *CreateAnimationSource(const AnimationType *type, RStringB source);

  virtual AnimationSource *CreateAnimationSourcePar(const AnimationType *type, ParamEntryPar entry);

  void InitShape();
  void DeinitShape(); // before shape is unloaded
  virtual size_t GetMemoryControlled() const;

  void InitShapeLevel(int level);
  void DeinitShapeLevel(int level);

  virtual EntityVisualState* CreateVisualState() const {return new TransportVisualState(*this);}

  virtual bool ForEachTurret(ITurretTypeFunc &func) const;
  virtual bool HasTurret() const {return _turrets.Size() > 0;}
  virtual const TurretType *GetTurret(const int *path, int size) const;

  void DrawMFDs(Transport *veh, int cb, const PositionRender & pos) const;
    
  RString GetDriverAction() const {return _driverAction;}
  RString GetDriverInAction() const {return _driverInAction;}
  RString GetDriverGetInAction() const {return _driverGetInAction;}
  RString GetDriverGetOutAction() const {return _driverGetOutAction;}

  RString GetCargoAction(int pos) const {return _cargoAction[pos];}
  RString GetCargoGetInAction(int pos) const;
  RString GetCargoGetOutAction(int pos) const;

  GetInOutMode PreciseGetInOut() const {return _preciseGetInOut;}
  GetInOutMode CargoPreciseGetInOut(int pos) const;


  bool GetHideProxyInCombat() const {return _hideProxyInCombat;}
  
  bool GetUnloadInCombat() const {return _unloadInCombat;}

  __forceinline int NDriverGetInPos() const {return _driverGetInPos.Size();}
  int NCargoGetInPos(int cargoIndex) const;

  __forceinline GetInPointIndexVal GetDriverGetInPos(int i) const {return _driverGetInPos[i];}
  GetInPointIndexVal GetCargoGetInPos(int cargoIndex, int i) const;

#if _HELISIM_PRECISE_INTERACTION
  __forceinline int NDriverGetInPosPrecise() const {return _driverGetInPosPrecise.Size();}
  int NCargoGetInPosPrecise(int cargoIndex) const;
  __forceinline int NCargoSizePrecise() const {return _cargoGetInPosPrecise.Size();}
  __forceinline GetInPointIndexVal GetDriverGetInPosPrecise(int i) const {return _driverGetInPosPrecise[i];}
  GetInPointIndexVal GetCargoGetInPosPrecise(int cargoIndex, int i) const;
#endif

  __forceinline float GetGetInRadius() const {return _getInRadius;}

  __forceinline int GetMaxManCargo() const {return _maxManCargo;}

  bool HasDriver() const {return _hasDriver;}
  bool HasGunner() const {return _hasGunner;}
  bool HasObserver() const {return _hasObserver;}
  bool HasCargo() const {return _maxManCargo>0;}
  __forceinline bool DriverIsCommander() const {return _driverIsCommander;}
  bool CanManualFire() const {return _hasGunner && _enableManualFire;}

  virtual float GetMaxWeaponRange() const;

  RString GetCrew() const {return _crew;}
  const ViewPars *GetViewOptics() const {return &_viewOptics;}
  
  const LevelProxies *GetLevelProxies(int level) const {return (level >=0 && (size_t)level <_proxies.Length()) ? &_proxies[level] : NULL;}

  const CameraShakePars& GetCamShake() const {return _camShake;}
  float GetCamShakeCoef() const {return _camShakeCoef;}

  const DoorsHolder *GetDoors() const {return &_doors;};
};

typedef TransportType VehicleTransportType;

DECL_ENUM(LockState)

#define UPDATE_TRANSPORT_MSG(MessageName, XX) \
  XX(MessageName, Ref<Person>, driver, NDTRef, NetworkId,  NCTNone, DEFVALUENULL, DOC_MSG("Driver person"), TRANSF_REF, ET_NOT_EQUAL, ERR_COEF_STRUCTURE) \
  XX(MessageName, bool, driverLocked, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("Driver position is locked"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, OLinkPerm<Person>, effCommander, NDTRef, NetworkId,  NCTNone, DEFVALUENULL, DOC_MSG("Currently commanding person"), TRANSF_REF, ET_NOT_EQUAL, ERR_COEF_STRUCTURE) \
  XX(MessageName, RefArray<Person>, manCargo, NDTRefArray, AutoArray<NetworkId>, NCTNone, DEFVALUEREFARRAY, DOC_MSG("Men in cargo space"), TRANSF_REFS, ET_NOT_EQUAL_COUNT, 0.5 * ERR_COEF_STRUCTURE) \
  XX(MessageName, AutoArray<bool>, cargoLocked, NDTBoolArray, AutoArray<bool>, NCTNone, DEFVALUEBOOLARRAY, DOC_MSG("Locked cargo positions"), TRANSF, ET_NOT_EQUAL, 0.5 * ERR_COEF_MODE) \
  XX(MessageName, int, lock, NDTInteger, int, NCTSmallSigned, DEFVALUE(int, LSDefault), DOC_MSG("Lock state"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, float, driverHiddenWanted, NDTFloat, float, NCTFloat0To1, DEFVALUE(float, 0), DOC_MSG("Wanted position of (tank) driver - inside / outside"), TRANSF, ET_NOT_EQUAL, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, fuel, NDTFloat, float, NCTNone, DEFVALUE(float, 0), DOC_MSG("Current amount of fuel"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MINOR) \
  XX(MessageName, bool, engineOff, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("Engine is off / on"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, bool, manualFire, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("Commander has control over the main gun"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, bool, enableVisionMode, NDTBool, bool, NCTNone, DEFVALUE(bool, true), DOC_MSG("Vision modes are off / on"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, bool, allowCrewInImmobile, NDTBool, bool, NCTNone, DEFVALUE(bool, true), DOC_MSG("allowcrew to remain in immobile vehicle"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, Ref<Person>, copilot, NDTRef, NetworkId,  NCTNone, DEFVALUENULL, DOC_MSG("copilot person"), TRANSF_REF, ET_NOT_EQUAL, ERR_COEF_STRUCTURE) \
  XX(MessageName, bool, unLockedControls, NDTBool, bool, NCTNone, DEFVALUE(bool, true), DOC_MSG("enables copilot to take control over vehicle"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, bool, enableCopilots, NDTBool, bool, NCTNone, DEFVALUE(bool, true), DOC_MSG("enables copilot to take control over vehicle"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, StaticArrayAuto<bool>, doors, NDTBoolArray, AutoArray<bool>, NCTNone, DEFVALUEBOOLARRAY, DOC_MSG("Current state of doors"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, RefArray<Weapon>, weapons, NDTRefArray, AutoArray<NetworkId>, NCTNone, DEFVALUEREFARRAY, DOC_MSG("List of weapons"), TRANSF_REFS, ET_NOT_EQUAL_COUNT, ERR_COEF_MODE) \
  XX(MessageName, RefArray<Magazine>, magazines, NDTRefArray, AutoArray<NetworkId>, NCTNone, DEFVALUEREFARRAY, DOC_MSG("List of magazines"), TRANSF_REFS, ET_NOT_EQUAL_COUNT, ERR_COEF_MODE) \
  XX(MessageName, OLinkPermO(Weapon), currentWeapon, NDTRef, NetworkId, NCTNone, DEFVALUENULL, DOC_MSG("The currently selected weapon"), TRANSF_REF, ET_NOT_EQUAL, ERR_COEF_MODE) \

DECLARE_NET_INDICES_EX_ERR(UpdateTransport, UpdateVehicleAIFull, UPDATE_TRANSPORT_MSG)

#define DIAG_CREW 0

//////////////////////////////////////////////////////////////////////////

struct TransportSoundContex
{
  // vehicle
  Transport *_transport;
  // controllers array
  float *_ctrl;
  // controllers count
  int _ctrlCount;
  // listener inside
  bool _inside;
  // occlusion
  float _occlusion;
  // obstruction
  float _obstruction;
  // sound position
  Vector3 _pos;
  // sound speed
  Vector3 _speed;
  // simulation time step
  float _deltaT;
};

/// Used for start & stop sounds. Manage switching between samples.
class StartStopHolder: public RefCount
{
private:
  RefAbstractWave _wave;

  bool _engineState;
  bool _cameraState;
  bool _stateInitilized;
  bool _wavePlaying;
  bool _waveStop;

public:
  StartStopHolder(): RefCount(), _stateInitilized(false), _wavePlaying(false), _waveStop(false) {}
  ~StartStopHolder() { Unload(); }

public:
  // play sound
  void Sound(Transport *transport);
  // wave terminated callback
  void static WaveCallBack(AbstractWave *wave, RefCount *context);

  void SetEngineState(bool value) {_engineState = value;};

  // release wave
  void Unload();
private:
  // start playing from spec. position
  bool StartPlayFromPos(Transport *transport, const SoundPars &pars, bool inside, bool reverse);

};

// class hold waves included into submix
class AbstractSoundsHolder: public RefCount
{
protected:
  AbstractSubmix *_submix;
  AutoArray<RefAbstractWave> _waves;

public:
  AbstractSoundsHolder(int count): _submix(0) { Construct(count); }
  virtual ~AbstractSoundsHolder() { Destruct(); }

  FilterTypeEx SetFilter(FilterTypeEx fType);
  // define submix output voice
  void SetOutputVoiceSound(SubmixOutput sOutput);

protected:
  void SetDirectSound(SoundCone &cone, const Transport *transport);

private:
  void Construct(int count);
  void Destruct();
};

/* used for sound controllers - hold group of similar waves */
class SoundsHolder: public AbstractSoundsHolder
{
public:
  SoundsHolder(int count): AbstractSoundsHolder(count) {}

public:
  template<class Type>
  void UpdateSubmixWaves(const AutoArray<Type> &arr, const TransportSoundContex &context);
};

/* used for sound events - hold group of similar waves */
class SoundsEventHolder: public AbstractSoundsHolder
{
public:
  SoundsEventHolder(int count): AbstractSoundsHolder(count) {}

public:
  template<class Type>
  void UpdateSubmixWaves(const AutoArray<Type> &arr, const TransportSoundContex &context);
};

//////////////////////////////////////////////////////////////////////////

//! base class for all vehicles capable of hosting crew
/*!
  all common stuff related to transporting personnel or cargo 
  is implemented in this class  
*/

class Transport: public EntityAIFull, public IWeaponsHolder
{
  typedef EntityAIFull base;

  friend class GetGetInTurretActionsLeader;
  friend class SoundsHolder;
  friend class StartStopHolder;

public:
  enum LandingMode
  {
    LMNone,
    LMLand,
    LMGetIn,
    LMGetOut,
  };

protected:
  Vector3 _driverPos; // commander gave command (VZero->invalid)
  Vector3 _dirWanted;
  VehicleMoveMode _moveMode;
  VehicleTurnMode _turnMode;
  bool _fireEnabled;
  bool _showDmg;
  //! vehicle commander has direct control over the weapons
  bool _manualFire;
  //! vehicle engine is off
  bool _engineOff;
  /// driver position is locked (nobody can get in / move to it)
  bool _driverLocked;
  /// the simulation and drawing switched to wreck
  bool _isWreck;
  //! zooming
  Object::Zoom _zoom;

  /// weapons controlled by the driver
  WeaponsState _weaponsState;
  RefArray<Weapon> _weapons;              //!< Weapons
  RefArray<Magazine> _magazines;          //!< Magazines (reserve only)
  OLinkPermO(Weapon) _currentWeapon;      // selected weapon

  //force generated by sling load
  Vector3 _slingloadForce;
 
  /* surface info always exists as long as the bank exists */
  const SurfaceInfo *_ptrSurfInfo;
  Ref<SurfaceInfo> _surfaceInfo;
  // sound controllers
  RefArray<SoundsHolder> _sounds;
  // sound events
  RefArray<SoundsEventHolder> _soundsEvents;

  //! Coefficient to hold the engine temperature coefficient (0 - Off, 1 - fully hot)
  float _engineTemperatureCoef;
  //! Coefficient to hold the wheel temperature coefficient (0 - Off, 1 - fully hot)
  float _wheelTemperatureCoef;
  //! Coefficient to hold gun temperature coefficient, common factor for all turrets (0 - Off, 1 - fully hot)
  float _turretGunTemperatureCoef;
  //! Time when destroyed vehicle reach max heat up temperature 
  Time _destroyMaxHeatTime;

  //! when we collide with some hard object, we revert turret to neutral position
  //! so that we can get out of the crash
  Time _turretFrontUntil;

  LockState _lock;

  /// script initiated landing
  LandingMode _landing;

  float _azimutWanted;
  
  float _randomizer;

  //! Person on a place of the driver. It is not possible to set it directly, it must be done using GetIn or Swap functions that do many other things
  //! See _driverAssigned
  Ref<Person> _driver; // store driver handle (when in)

  // second pilot
  bool _unLockedControls;
  bool _enableCopilots;
  Ref<Person> _copilot;

  OLinkPerm<Person> _effCommander; // who is giving commands
  
  Link<AbstractWave> _doorSound;
  RefAbstractWave _dmgSound;
  
  RefAbstractWave _engineSound,_envSound;
  //AutoArray< RefAbstractWave > _soundWaves;
  // start & stop sounds
  Ref<StartStopHolder> _holder;

  float _driverHiddenWanted;

  ManCargo _manCargo; // transported soldiers

  // crash sound parameters
  enum CrashType {CrashNone,CrashLand,CrashWater,CrashObject,CrashTree,CrashBuilding,CrashArmor};
  CrashType _doCrash;
  float _crashVolume;
  // time of last crash sound (will not repeat for some time)
  Time _timeCrash;
  //float _crewDammage; // how much the crew is affected by dammage
  Time _getOutAfterDamage;

  Time _explosionTime; // vehicle is going to explode

  //units will eject from vehicle without wheels/tracks
  bool _allowCrewInImmobile;

  /// list of directly attached turrets (may have their own turrets)
  RefArray<Turret> _turrets;

  RadioChannel _radio;

  Time _showDmgValid;

  /// first contact detection - used for vehicle/ground or vehicle/object crashes
  float _maxContactSpeed;

  // Values for energy renormalization.
  float _oldEnergy;   //< Energy in the last simulation step,
  bool _validEnergy;  //< Was energy in last simulation step valid?
  
  // enable/disable VM
  bool _enableVisionMode;
  // current vision mode
  VisionMode _visionMode;
  // current ti mode
  int _tiIndex;

  // last time then the camera shake was performed
  Time _lastCamShakeTime;
  Time _lastCamDamageShakeTime;

#if _HELISIM_RENDER_TO_TEXTURE
  RefArray<R2TTgtPars> _r2tTgtPars;
#endif

  AutoArray<float> _soundControllers;

public:
  const TransportType *Type() const { return static_cast<const TransportType *>(GetType()); }

  /// @{ Visual state
public:
  TransportVisualState typedef VisualState;
  friend class TransportVisualState;

  
  VisualState& FutureVisualState() { return static_cast<VisualState &>(base::FutureVisualState()); }
  VisualState const& FutureVisualState() const { return static_cast_checked<VisualState const &>(base::FutureVisualState());}
  VisualState const& CurrentVisualState() const { return static_cast_checked<VisualState const &>(base::CurrentVisualState());}
  const VisualState& RenderVisualState() const{ return static_cast<const VisualState &>(base::RenderVisualState()); }

  ProtectedVisualState<const VisualState> RenderVisualStateScope(bool ignoreParent=false) const {return ProtectedVisualState<const VisualState>(base::RenderVisualStateScope(ignoreParent));}
  ProtectedVisualState<VisualState> FutureVisualStateScope(bool ignoreParent=false) { return ProtectedVisualState<VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  ProtectedVisualState<const VisualState> FutureVisualStateScope(bool ignoreParent=false) const { return ProtectedVisualState<const VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  /// @}

  WeaponsState &GetWeapons() {return _weaponsState;}

  //! consume some fuel (used during simulation), return false if tank is empty
  bool ConsumeFuel(float ammount);

public:
  Transport(const EntityAIType *name, Person *driver, bool fullCreate = true);

  virtual void CreateSubobjects() override;

  virtual WeaponsState *GetWeaponsState() override {return &_weaponsState;}

  // IWeaponsHolder implementation
  virtual void Compact() override;
  virtual bool ForEachWeapon(const std::function<bool(Weapon *weapon)> &func) override;
  virtual bool ForEachWeapon(const std::function<bool(const Weapon *weapon)> &func) const override;
  virtual bool AddWeapon(EntityAIFull *entity, Weapon *weapon) override;
  virtual bool RemoveWeapon(Weapon *weapon) override;
  virtual bool DeleteWeapon(RStringB name) override;
  virtual void DeleteWeapons(const std::function<bool(const Weapon *weapon)> &func) override;
  virtual void DeleteAllWeapons() override;
  virtual bool ForEachMagazine(const std::function<bool(Magazine *)> &func) override;
  virtual bool AddMagazine(EntityAIFull *entity, Magazine *magazine) override;
  virtual bool RemoveMagazine(Magazine *magazine) override;
  virtual bool DeleteMagazine(RStringB name) override;
  virtual void DeleteMagazines(const std::function<bool(const Magazine *magazine)> &func) override;
  virtual void DeleteAllMagazines() override;
  virtual WeaponModePtr GetCurrentWeapon() const override;
  virtual void SetCurrentWeapon(const WeaponModePtr &ptr) override;

  //! Set alive factor - typical use in cutscene
  void SetAliveFactor(float aliveFactor) {_engineTemperatureCoef = aliveFactor;}
  //! Set movement factor (wheels, trucks) - typical use in cutscene
  void SetMovementFactor(float movementFactor) {_wheelTemperatureCoef = movementFactor;}
  //! Set metabolism factor (main gun) - typical use in cutscene
  void SetMetabolismFactor(float metabolismFactor) {_turretGunTemperatureCoef = metabolismFactor;}
  //! Virtual method
  virtual float GetAliveFactor() const {return _engineTemperatureCoef;}
  //! Virtual method
  virtual float GetMovementFactor() const {return _wheelTemperatureCoef;}
  //! Virtual method
  virtual float GetMetabolismFactor() const {return _turretGunTemperatureCoef;}
  //! Select shadow lod
  virtual int GetPass3ShadowVolumeLevel(int pass, CameraType camType) const;

  bool HasTiOptics() const;
  /// Switch to next driver vision mode
  bool NextVisionMode();
  /// Check if  driver optics support vision modes
  bool HasVisionModes() const;
  int VisionModesCount() const;
  /// Get currently driver set mode and ti index
  VisionMode GetVisionModePars(int &flirIndex);
  /// Allow / deny VM use
  void EnableVisionModes(bool val);
  /// VM allowed
  bool VisionModesEnabled() const {return _enableVisionMode;}
  bool GetPostProcessEffects(const TurretContextV &context, AutoArray<PPEffectType> **ppEffects) const;
  void CleanUp();
  void Simulate( float deltaT, SimulationImportance prec );

  void SetAllowCrewInImmobile(bool allow);
  //at this moment only RTD helicopter is affected by weight
  virtual void AddCrewWeight() {};
  virtual void RemoveCrewWeight() {};
  //shaking caused by damage (damaged helicopter transmission)
  virtual float GetDamageShakePower() {return 0;};


  //force generated by sling load
  Vector3 GetSlingLoadForce() {return _slingloadForce;};
  void SetSlingLoadForce(Vector3 slingloadForce) {_slingloadForce  = slingloadForce;};
  void AddSlingLoadForce(Vector3 slingloadForce) {_slingloadForce += slingloadForce;};

#if _HELISIM_RENDER_TO_TEXTURE

  virtual bool HasR2TTarget() const { return _r2tTgtPars.Size() > 0; }
  virtual Matrix4Val GetCurrentR2TTargetMatrix(int index)
  {
    DoAssert(index >= 0 && index < _r2tTgtPars.Size());
    UpdateR2TCamera(index);
    return _r2tTgtPars[index]->Transform();
  }
  void UpdateR2TCamera(int index);  
  const RefArray<R2TTgtPars>& AccessR2TTargets() const { return _r2tTgtPars; }

#endif

protected:
  Vector3 WreckFriction(Vector3Par speed);
  void ObjectCollisionWithPerson(CollisionBuffer objColl, unsigned int fromIndx, unsigned int toIndx, float& appliedImpulseObj);
  /// Simplified physical simulation, used for the destroyed vehicle
  void SimulateWreck(float deltaT, SimulationImportance prec);

public:
  /// Check if the simulation is switched to wreck
  bool IsWreck() const {return _isWreck;}

  virtual void StabilizeTurrets(Matrix3Val oldTrans, Matrix3Val newTrans);

  /// make sure crew moves their heads as necessary
  void MoveCrewHead();

  virtual float GetFuel() const { return FutureVisualState().GetFuel(); }
  virtual float GetRenderVisualStateFuel() const { return RenderVisualState().GetFuel(); }
  void Refuel(float ammount);

  virtual void Repair(float amount=1.0f);

  // @{ sound controllers
  // revolutions per minute
  virtual float GetRPM() const {return 0.0f;}
  virtual float GetRPM(int idx) const {return 0.0f;}
  virtual float GetRenderVisualStateRPM() const {return 0.0f;}

#if _HELISIM_SOUNDS
  virtual float GetAPU() const {return 0.0f;}
  virtual bool IsBatteryOn() const {return false;}
  virtual float GetRPMStarter(int idx) const {return 0.0f;}
  virtual float GetMotorDamage() const { return 0.0f; }
  virtual float GetTransmissionDamage() const { return 0.0f; }
  virtual float GetRotorBrake() const { return 0.0f; }
#endif

  // random value from range [0, 1], used for small differences in vehicle frequencies
  virtual float GetRandomizer() const {return _randomizer;}
  // camera position - values are from range [0, 1], where inside = 0, outside = 1
  virtual float GetCamPos() const;
  // vehicle thrust
  virtual float Thrust() const {return 0.0f;}
  // engine state, discrete values 0 = engine OFF, 1 = engine ON
  virtual float GetEngineOn() const {return _engineOff ? 0.0f : 1.0f;}

  // main rotor rotor speed
  virtual float GetRotorSpeed() const {return 0.0f;}
  // main rotor thrust
  virtual float GetMainRotorThrust() const {return 0.0f;}
  // tank angular velocity
  virtual Vector3 GetAngularVelocity() const {return VZero;}
  // flaps position normally - 0, down 1x- 0.5, down 2x - 1
  virtual float GetFlapPos() const {return 0.0f;}
  // horizontal -1
  virtual float GetThrustVectorPos() const {return 0.0f;}
  // outside 0, inside 1
  virtual float GetGearPos() const {return 0.0f;}
  // rotor position
  virtual float GetRotorPos() const {return 0.0f;}
  // vehicle in water
  float GetWaterContact() const { return _waterContact; }
  // damper simulation access for sound
  float GetDamperState(int index) const { return RenderVisualState().GetDamperState(index); }
  // @}

  virtual float GetSpeed() const;

  virtual bool GetAutoHover() {return false;}
  
  // @{ surface characteristics
  float GetRoughness() const {return _ptrSurfInfo ? _ptrSurfInfo->_roughness : 0.0f;}
  float GetDustness() const {return _ptrSurfInfo ? _ptrSurfInfo->_dustness : 0.0f;}
  float GetRockness() const {return GetSurfaceSounds(SurfSRockness);}
  float GetSandness() const {return GetSurfaceSounds(SurfSSandness);}
  float GetGrassness() const {return GetSurfaceSounds(SurfSGrassness);}
  float GetMudness() const {return GetSurfaceSounds(SurfSMudness);} 
  float GetGravelness() const {return GetSurfaceSounds(SurfSGravelness);}
  float GetAsphaltness() const {return GetSurfaceSounds(SurfSAsphaltness);}

  // sounds surface
  virtual float GetSurfaceSounds(SufraceSound type) const {return _ptrSurfInfo ? _ptrSurfInfo->GetSurfSound(type) : 0.0f;}
  // @}

  // @{ directional sound params
  virtual Vector3 GetFrontOrientation() const {return -VForward;}
  Vector3 GetTopOrientation() const {return VUp;}
  // @}
  
  void SetSurfaceInfo(GroundCollisionBuffer &groundCollBuffer);
  // internal sound coefficient
  float GetInsideSoundCoeff() const { return (Type()->_insideSoundCoef); }
  // filtering surrounding sounds when camera is located inside vehicle
  bool EnableOutsideSoundFilter() const { return (Type()->_outsideSoundFilter); }
  /// check if the vehicle has a compass device
  bool GetEnableCompass(AIBrain *unit) const;

  float GetSpeedHorizontal() const { return RenderVisualState().GetSpeedHorizontal(); }
  float GetHorizonBank() const { return RenderVisualState().GetHorizonBank(); }  // also used in HudDesc
  float GetHorizonDive() const { return RenderVisualState().GetHorizonDive(); }  // also used in HudDesc
  virtual float GetGMeterZ() const { return RenderVisualState().GetGMeterZ(); }

  /// @{ Visual controllers (\sa Type()->CreateAnimationSource())
  float GetCtrlAltRadar(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetAltRadar(_shape); }
  float GetCtrlAltBaro(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetAltBaro(); }
  float GetCtrlSpeedHorizontal(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetSpeedHorizontal(); }
  float GetCtrlSpeedVertical(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetSpeedVertical(); }
  float GetCtrlDirection(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetDirection(); }
  float GetCtrlWaypointDirection(ObjectVisualState const& vs) const;
  float GetCtrlTurretDirection(ObjectVisualState const& vs) const;
  float GetCtrlHorizonBank(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetHorizonBank(); }
  float GetCtrlHorizonDive(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetHorizonDive(); }
  float GetCtrlFuel(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetFuelFraction(Type()); }
  float GetCtrlGMeter(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetGMeter(); }
  float GetCtrlGMeterX(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetGMeterX(); }
  float GetCtrlGMeterY(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetGMeterY(); }
  float GetCtrlGMeterZ(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetGMeterZ(); }
  float GetCtrlDriverHatchPos(ObjectVisualState const& vs) const { return static_cast_checked<VisualState const&>(vs).GetDriverHatchPos(); }

  // virtual controllers
  virtual float GetCtrlRPM(ObjectVisualState const& vs) const {return 0.0f;}  // overridden in TankOrCar, Airplane, Helicopter, Motorcycle, Ship
  virtual float GetCtrlDrivingWheelPos(ObjectVisualState const& vs) const {return 0.0f;}  // overridden in Car, Motorcycle, Ship
  // @}

  VehicleMoveMode GetMoveMode() const {return _moveMode;}
  VehicleTurnMode GetTurnMode() const {return _turnMode;}

  void PlaceOnSurface(Matrix4 &trans);

  virtual Vector3 GetCameraDirection( CameraType camType ) const;
  Matrix4 InternalCameraTransform(Matrix4 &base, CameraType camType) const;
  virtual Matrix4 InsideCamera( CameraType camType ) const;
  //! Determine what LOD to use for the vehicle player is in
  virtual int InsideLOD( CameraType camType ) const;
  //! Determine what LOD to use for the vehicle player is in and when not using optics
  virtual int InternalLOD(UIActionType position) const;

  Matrix4 TurretTransform(const TurretType &type) const;
  Matrix4 GunTurretTransform(ObjectVisualState const& vs, const TurretType &type) const;

  Vector3 GetEyeDirection(ObjectVisualState const& vs, const TurretContext &context) const;
  Vector3 GetEyeDirectionWanted(ObjectVisualState const& vs, const TurretContextEx &context) const;
  Vector3 GetLookAroundDirection(ObjectVisualState const& vs, const TurretContext &context) const;

  bool IsVirtual( CameraType camType ) const;
  bool IsVirtualX( CameraType camType ) const;

  virtual void PreloadGetIn(UIActionType position);

  /// prepare getting out - open doors, ...
  virtual bool PrepareGetOut(AIBrain *unit);

  /// finish getting out - close doors, ...
  virtual void FinishGetOut(AIBrain *unit){}

  void CloseDoor(AIBrain *unit);
  void OpenDoor(AIBrain *unit);
  void SetDoorOpen(int index, bool val);

  /// prepare getting in - open doors, ...
  virtual bool PrepareGetIn(UIActionType pos, Turret *turret, int cargoIndex);

  /// finish getting in - close doors, ...
  virtual void FinishGetIn(UIActionType pos, Turret *turret, int cargoIndex){}

  virtual bool IsGunner( CameraType camType ) const;

  virtual void LimitVirtual(CameraType camType, float &heading, float &dive, float &fov) const;
  virtual void InitVirtual(CameraType camType, float &heading, float &dive, float &fov) const;
  virtual void OverrideCursor(Vector3 &dir) const;

  void EngineOn();
  void EngineOff();
  virtual bool EngineIsOn() const {return !_engineOff;}
  virtual bool IRSignatureOn() const {return !_engineOff;}
  //! some transport types may have no engine or may have it always on
  bool EngineCanBeOff() const {return true;}

  void Init( Matrix4Par pos, bool init );
  void InitUnits();

  bool ValidateCrew(Person *crew, bool complex=true) const;
  bool Validate(bool complex=true) const;
  virtual void TrackTargets(TargetList &res, bool initialize, float trackTargetsPeriod[NTgtCategory]);
  virtual void TrackOneTarget(TargetList &res, Target *target, float trackTargetsPeriod);

  void SetTotalDamageHandleDead(float damage, bool setHitpoints) override;
  void ReactToDamage();

  // @{ locking of positions in the vehicle
  bool IsDriverLocked() const {return _driverLocked;}
  void LockDriver(bool lock) {_driverLocked = lock;}
  bool IsCargoLocked(int i) const {return _manCargo.IsLocked(i);}
  void LockCargo(int i, bool lock) {_manCargo.Lock(i, lock);}
  //@}

  /// check if current situation is suitable for ejection
  virtual bool CheckEject() const;

  //! eject given crew member
  bool EjectCrew(Person *man);
  //! eject given crew member if he is dead
  bool EjectIfDead(Person *man);
  //! eject given crew member if he is alive
  bool EjectIfAlive(Person *man);

  //! eject all alive persons in cargo 
  bool EjectIfAliveCargo();
  //! eject all dead persons in cargo 
  bool EjectIfDeadCargo();

  //! eject all crew member that are not fixed
  void EjectAllNotFixed();

  bool HasSomePilot() const; // false if no pilot - vehicle should brake

  /// enumerate all turrets
  virtual bool ForEachTurret(ITurretFunc &func) const;
  /// enumerate all turrets
  virtual bool ForEachTurret(EntityVisualState& vs, ITurretFuncV &func) const;
  /// enumerate all turrets
  virtual bool ForEachTurretEx(EntityVisualState& vs, ITurretFuncEx &func) const;
  /// enumerate all crew
  virtual bool ForEachCrew(ICrewFunc &func) const;

  virtual void ForEachChildren(const std::function<bool(const NetworkObject *)> &func) const override;

#if _VBS3 //Commander override
  bool FindOpticsTurret(const Person *gunner, TurretContext &context) const;
  bool FindControlledTurret(const Person *gunner, TurretContext &context) const;
#endif

  virtual bool FindTurret(const Person *gunner, TurretContext &context) const;
  virtual bool FindTurret(EntityVisualState& vs, const Person *gunner, TurretContextV &context) const;
  virtual bool FindTurret(const Turret *turret, TurretContext &context) const;
  virtual bool FindTurret(EntityVisualState& vs, const Turret *turret, TurretContextV &context) const;
  bool FindAssignedTurret(const AIBrain *brain, TurretContext &context) const;
  bool FindAssignedTurret(EntityVisualState& vs, const AIBrain *brain, TurretContextV &context) const;
  virtual bool GetPrimaryGunnerTurret(TurretContext &context) const;
  virtual bool GetPrimaryGunnerTurret(EntityVisualState &vs, TurretContextV &context) const;
  virtual bool GetPrimaryGunnerTurret(EntityVisualState &vs, TurretContextEx &context) const;
  virtual bool GetPrimaryObserverTurret(TurretContext &context) const;
  virtual bool GetPrimaryObserverTurret(EntityVisualState &vs, TurretContextV &context) const;
  virtual bool GetPrimaryObserverTurret(EntityVisualState &vs, TurretContextEx &context) const;

  bool GetDriverTurret(TurretContext &context) const;

  //! Count guided missiles
  void CountMissiles(MissileCounter &missiles) const;
  //! missile shape for proxy drawing
  LODShape *GetMissileShape(const int proxyIndex) const;

  virtual bool CanFireUsing(const TurretContext &context) const;

  virtual bool GetWeaponAim(TurretContext context, Vector3 &position, Vector3 &direction);


  void SimulateAI(float deltaT, SimulationImportance prec);  // false if no pilot
  void CrashDammage(float ammount, const Vector3 &pos=VZero, float range=0.0f);

  virtual void SelectWeaponCommander(AIBrain *unit, const TurretContext &context, const WeaponModePtr &ptr) override;

  void SimulateGunner(TurretContextEx &context, float deltaT, SimulationImportance prec);
  void SimulateObserver(TurretContextEx &context, float deltaT, SimulationImportance prec);

#if _ENABLE_AI
  virtual void AIObserver(AIBrain *unit, float deltaT);
  virtual void AICommander(AIBrain *unit, float deltaT);
  virtual void AIPilot(AIBrain *unit, float deltaT) {}
  virtual void AIGunner(TurretContextEx &context, float deltaT);
  /// helper function called from AIGunner
  virtual void CommandPrimaryGunner(AIBrain *unit, TurretContext &context);
#endif

  virtual void KeyboardPilot(AIBrain *unit, float deltaT) {}
  virtual void SuspendedPilot(AIBrain *unit, float deltaT) {}
  virtual void KeyboardAny(AIBrain *unit, float deltaT);
  
  // KeyboardCommander and KeyboardGunner are partially realized in InGameUI
  virtual void KeyboardObserver(TurretContextEx &context, float deltaT);
  virtual void KeyboardGunner(TurretContextEx &context, float deltaT);
  virtual void KeyboardLookAround(AIBrain *unit, float deltaT);

  virtual Vector3 AdjustWeapon(const TurretContext &context, const WeaponModePtr &ptr, float fov, Vector3 direction) const override;
  virtual bool GIsManual(const Person *gunner) const;

  virtual CombatMode GetCombatModeAutoDetected(const AIBrain *brain) const;
  
  void LookAround(float aimX, float aimY, float deltaT, const ViewPars *viewPars, AIBrain *unit, bool forceLookAround=false, bool forceDiveAround=false);
  // UpdateZoom gets LockTarget action and update zoom
  void UpdateZoom(AIBrain *unit);
  // get current zoom for the player (from transport or from appropriate turret)
  virtual float GetCameraFOV() const;
  Object::Zoom *GetZoom();
  Object::Zoom *GetZoom(const AIBrain *brain);

#if _VBS3 // Airborne() function moved up to class Transport
  virtual bool Airborne() const {return !_landContact;}
  //! Get weapon position (where the projectiles leave the weapon)
  virtual Vector3 GetWeaponPoint(const TurretContext &context, const WeaponModePtr &ptr) const override;
#endif

  virtual void SetObserverSpeed(const TurretContextEx &context, float speedX, float speedY, float deltaT);

  // set exact wanted direction of turret  
  virtual void SetObserverDir(const TurretContextEx &context, float x, float y);

  /// opportunity to perform stabilization after network update
  virtual void OnTurretUpdate(Turret *turret, Matrix3Par oldOrient, Matrix3Par newOrient){}
  virtual bool IsTurretLocked(const TurretContextV &context) const {return false;}
  bool IsGunnerLookingAround(const TurretContextV &context) const;

  /// get turret based on path in hierarchy
  Turret *GetTurret(const int *path, int size);
  /// get turret based on path in hierarchy
  const Turret *GetTurret(const int *path, int size) const;

  /// get path in hierarchy for given turret
  template <class Allocator>
  bool GetTurretPath(AutoArray<int, Allocator> &path, const Turret *turret) const
  {
    for (int i=0; i<_turrets.Size(); i++)
    {
      path.Resize(1);
      path[0] = i;
      if (_turrets[i]->GetTurretPath(path, turret)) return true;
    }
    return false;
  }

  void FindDriverPath(Vector3Par pos, bool forceReplan);
  void DriverPathPilot
  (
    AIBrain *unit, SteerInfo &info,
    float speedCoef, Vector3Par instantPos
  );

  virtual void FakePilot( float deltaT ) {};

  // prepare for drawing
  AnimationStyle IsAnimated( int level ) const; // appearance changed with Animate
  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
  void Deanimate( int level, bool setEngineStuff );
  
  virtual bool PreloadMuzzleFlash(const WeaponsState &weapons, const WeaponModePtr &ptr) const override;
  
  virtual void AnimateManProxyMatrix(ObjectVisualState const& vs, int level, const ManProxy &proxy, Matrix4 &proxyTransform) const;
  
  bool IsManualFire() const {return _manualFire;}

  /**
  0.5 decision value seems natural, but it causes closing hatches hitting
  the proxies which are still outside
  see also Turret::IsGunnerHidden 
  */
  bool IsPersonHidden(ObjectVisualState const& vs, Person *person) const;
  void HideDriver(float hide=1.0f) 
  {  
    _driverHiddenWanted = hide;
  }
  void HidePerson(Person *person, float hide=1.0f);

  float GetDriverHidden() const {return FutureVisualState()._driverHidden;}
  float GetDriverHiddenWanted() const {return _driverHiddenWanted;}

  int GetDriverCompartments() const {return Type()->_driverCompartments;}
  int GetCargoCompartments(int index) const;
  int GetPersonCompartments(Person *person) const;

  //@{
  //!Get action corresponding to given position in vehicle
  virtual RString DriverAction() const;
  virtual RString CargoAction(int position) const;

  virtual float DriverAnimSpeed() const;
  virtual float CargoAnimSpeed(int position) const;
  //@}

  virtual UserAction GetManualFireAction() const {return (UserAction)-1;}

  virtual LODShape *GetOpticsModel(const Person *person) const;
  virtual bool GetEnableOptics(ObjectVisualState const& vs, const Person *person) const { return false; }
  virtual PackedColor GetOpticsColor(const Person *person);
  virtual bool GetForceOptics(const Person *person, CameraType camType) const; 
  virtual bool PreloadView(Person *person, CameraType camType) const;

  virtual bool ShowAim(const TurretContextV &context, const WeaponModePtr &ptr, CameraType camType, Person *person) const override;
  virtual bool ShowCursor(const TurretContextV &context, const WeaponModePtr &ptr, CameraType camType, Person *person) const override;

  virtual void DrawDiags();
  void DrawCameraCockpit();

  bool NeedsPrepareProxiesForDrawing() const;
  void PrepareProxiesForDrawing(
    Object *rootObj, int rootLevel, const AnimationContext &animContext,
    const PositionRender &transform, int level, ClipFlags clipFlags, SortObject *so
    );
  /// prepare single crew member for drawing
  void PrepareCrewMemberForDrawing(
    Object *rootObj, int rootLevel, const AnimationContext &animContext, const PositionRender &transform, int level, ClipFlags clipFlags, SortObject *so,
    const ManProxy &proxy, Person *man, bool getInOutOnCenter=false);

  int GetCrewMemberComplexity(
    int level, const Matrix4 &pos, float dist2,
    const ManProxy &proxy, Person *man, bool getInOutOnCenter=false
  ) const;
  
  bool PreloadCrewMember(const ManProxy &proxy, Person *man, float dist2) const;
  bool PreloadProxies(int level, float dist2, bool preloadTextures = false) const;

  void RecalcIKTransform(int level,const Person *driver, const ManProxy &proxy,
    SkeletonIndex leftHandBone, SkeletonIndex rightHandBone, SkeletonIndex leftLegBone, SkeletonIndex rightLegBone,
    Matrix4 & leftHandIK, Matrix4 & rightHandIK, Matrix4 & leftLegIK, Matrix4 & rightLegIK) const;

  
  virtual int GetProxyComplexity(int level, const Matrix4 &pos, float dist2) const;
  Matrix4 FindMissilePos(int index, bool &found, int maverickweaponIndexOffset) const;

  int PassNum( int lod );

  //! Determine whether volume shadows should be casted in pass3
  virtual bool CastPass3VolShadow(int level, bool zSpace, float &castShadowDiff, float &castShadowAmb) const;

  //! Virtual method
  virtual void Draw(
    int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
    const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
  ) override;


  virtual int GetProxyCount(int level) const;
  virtual Object *GetProxy(
    VisualStateAge age, LODShape *&shape, Object *&parentObject,
    int level, Matrix4 &transform, const Matrix4 &parentPos, int i
  ) const;

  virtual Matrix4 ProxyWorldTransform(VisualStateAge age, const Object *obj) const;
  virtual Matrix4 ProxyInvWorldTransform(VisualStateAge age, const Object *obj) const;


  Texture *GetFlagTexture();

  /// helper for AimingPosition creation
  Vector3 AimingPositionCrew(ObjectVisualState const& vs, const AmmoType *ammo, Vector3Par aimFromDir) const;
  bool GetOpticsCamera(Matrix4 &transf, CameraType camType) const; // returns true if transf ok
  bool GetProxyCamera(Matrix4 &transf, CameraType camType) const; // returns true if transf ok

  virtual Vector3 AimingPosition(ObjectVisualState const& vs, const AmmoType *ammo=NULL, Vector3Par aimFromDir=VZero) const override;
  
  /// used to provide a way how to calculate camera position for external camera
  virtual Vector3 CameraPointRel(CameraType camType) const;

  void PerformFF( FFEffects &effects );

  void Sound( bool inside, float deltaT );
  void UnloadSound();
  static const RString* SoundControllers();
  static int SoundControllersCount();
  void EvaluateSoundControllers();

  bool ListenerIsInside(Person *listener) const;

  virtual void OccludeAndObstructSound(EntityAI *focus, float &occlusion, float &obstruction, ObstructType soundInside) const;
  virtual float GetEngineVol( float &freq ) const {freq=1;return 0;}
  virtual float GetEnvironVol( float &freq ) const {freq=1;return 0;}

  virtual float GetSoundsVol(float &freq, const SoundParsEx &soundPars, float* evalControllers, int length) const;
  virtual float EvaluateSoundEvent(const SoundEventPars &soundEPars, float* evalControllers, int length) const;
  const SoundPars& GetEngineSoundPars(bool inside);

  virtual Vector3 GetEnginePos() const; // sound source position in model coord
  virtual Vector3 GetEnvironPos() const; // sound source position in model coord
    
  AIBrain *DriverBrain() const;
  AIBrain *GunnerBrain() const;
  AIBrain *ObserverBrain() const;
  AIBrain *CommandingGunner() const;
  AIBrain *CopilotBrain() const;

  virtual AIBrain *PilotUnit() const;
  virtual AIBrain *GunnerUnit() const;
  virtual AIBrain *ObserverUnit() const;
  virtual AIBrain *CommanderUnit() const;
  virtual AIBrain *EffectiveGunnerUnit(const TurretContext &context) const;
  virtual AIBrain *EffectiveObserverUnit() const;
  virtual AIBrain *EffectivePilotingUnit() const;
  virtual AIBrain *CopilotUnit() const;

  TargetSide GetTargetSide() const;

  bool IsCrewed() const;

  TargetSide GetTargetSideAsIfAlive() const;

  float VisibleLights() const; // visibility

  virtual bool QIsManual() const;
  virtual bool QIsManualMovement() const;
  bool QIsManual(const AIBrain *unit) const;

  RadioChannel &GetRadio() {return _radio;}
  const RadioChannel &GetRadio() const {return _radio;}

  VehicleMoveMode GetMoveMode() {return _moveMode;}
  bool IsFireEnabled(const TurretContext &context);

//  bool IsLocked() const {return _locked;}
//  void SetLocked(bool lock) {_locked = lock;}
  LockState GetLock() const {return _lock;}
  void SetLock(LockState lock) {_lock = lock;}

  virtual bool StopAtStrategicPos() const;

#if _ENABLE_AI
  virtual void LeaderPilot(SteerInfo &info);
  void CommandPilot(SteerInfo &info);
  virtual bool ReverseLeftRightAIPilot() {return false;}
#endif

  private:
  float CalculateDriverPos(float &headChange, float minDist, Vector3 &instant);
  bool CanBeCommander(AIBrain *unit) const;

  public:
#if _VBS2 //Convoy trainer
    void Transport::SimulatePost( float deltaT, SimulationImportance prec );
#endif
  // void UpdatePath(Vector3Par dest);

  void ShowDamage(int part, EntityAI *killer);

  virtual void CommanderLoadMagazine(Magazine *magazine, RString weapon, RString muzzle) override;

  virtual void LandStarted(LandingMode landing);
  void LandFinished() {_landing = LMNone;}

  virtual bool IsStopped() const;
  bool CanCancelStop() const;

  GetInPoint GetUnitGetOutPos(AIBrain *unit) const;

  Person *Driver() const {return _driver;}
  Person *Observer() const;
  Person *Gunner() const;

  Person *Copilot() const {return  _copilot;} // second pilot
  void SetCopilot(Person *copilot);
  void SetUnlockedControls(bool unlocked) {_unLockedControls = unlocked;}
  void EnableCopilots(bool enable);
  bool IsCopilotsEnabled() {return _enableCopilots;}

  void DriverConstruct( Person *driver );
        
  virtual void SimulateImpulseDamage(DoDamageResult &result, EntityAI *owner, float impulse, Vector3Par modelPos);

#if _HELISIM_PRECISE_INTERACTION
  virtual GetInPoint GetDriverGetInPosPrecise(Person *person, Vector3Par pos) const;
  virtual GetInPoint GetCargoGetInPosPrecise(int cargoIndex, Person *person, Vector3Par pos) const;
  virtual GetInPoint GetTurretGetInPosPrecise(Turret *turret, Person *person, Vector3Par pos) const;
#endif

private:
  void GetInAny(Person *driver, bool sound, const char *name);
  void GetOutAny(const Matrix4 & outPos, Person *driver, bool sound, const char *name, RString getOutAction);

public:
  virtual GetInPoint GetDefaultGetInPos(Person *person, Vector3Par pos) const;
  virtual GetInPoint GetDriverGetInPos(Person *person, Vector3Par pos) const;
  virtual GetInPoint GetCommanderGetInPos(Person *person, Vector3Par pos) const;
  virtual GetInPoint GetGunnerGetInPos(Person *person, Vector3Par pos) const;
  virtual GetInPoint GetTurretGetInPos(Turret *turret, Person *person, Vector3Par pos) const;
  virtual GetInPoint GetCargoGetInPos(int cargoIndex, Person *person, Vector3Par pos) const;

protected:

  virtual GetInPoint GetDriverGetOutPos(Person *person) const;
  virtual GetInPoint GetCommanderGetOutPos(Person *person) const;
  virtual GetInPoint GetGunnerGetOutPos(Person *person) const;
  virtual GetInPoint GetCargoGetOutPos(Person *person) const;

  float GetGetInRadius() const {return Type()->GetGetInRadius();}

  /// helper function to return get in point in model coordinates
  GetInPoint AnimateGetInPoint(GetInPointIndexVal ptIndex) const;

  public:
  virtual GetInPoint GetTurretGetOutPos(Turret *turret, Person *person) const;

  virtual Vector3 GetStopPosition() const;

  virtual void GetInDriver( Person *driver, bool sound=true, bool isFastMoveIn = false);
  void GetOutDriver( const Matrix4 & outPos, bool eject=false, bool sound=true);
  bool QIsDriverIn() const {return _driver!=NULL;}

  void GetInGunner( Person *man, bool sound=true, bool isFastMoveIn = false );

  void GetOutGunner( const Matrix4 &outPos, bool sound=true);
  bool QIsGunnerIn() const {return Gunner() != NULL;}

  void GetInCommander( Person *man, bool sound=true, bool isFastMoveIn = false);
  void GetOutCommander( const Matrix4 &outPos, bool sound=true);
  bool QIsCommanderIn() const {return Observer() != NULL;}

  void GetInTurret(Turret *turret, Person *man, bool sound = true, bool isFastMoveIn = false);
  void GetOutTurret(TurretContext &context, const Matrix4 &outPos, bool sound = true);

  /// called when a person becomes a leader or the vehicle after get-in
  void ControlTakenBy(Person *man);

  /// check if crew can be attacked with small arms
  virtual bool CheckCrewVulnerable() const;

  virtual void GetInCargo( Person *driver, bool sound=true, int posIndex=-1, bool isFastMoveIn = false);
  void GetOutCargo( Person *driver, const Matrix4 &outPos, bool sound=true);
  const ManCargo &GetManCargo() const {return _manCargo;}
  int GetManCargoSize() const;
  int GetFreeManCargo() const;

  int GetMaxManCargo() const {return Type()->_maxManCargo;}
  float GetMaxFuelCargo() const {return Type()->_maxFuelCargo;}
  float GetMaxRepairCargo() const {return Type()->_maxRepairCargo;}
  float GetMaxAmmoCargo() const {return Type()->_maxAmmoCargo;}
  bool IsAttendant() const {return Type()->_attendant;}
  bool IsEngineer() const {return Type()->_engineer;}

  float GetFreeFuelCargo() const {return Type()->_maxFuelCargo-SUPPLY(this,GetFuelCargo,());}
  float GetFreeRepairCargo() const {return Type()->_maxRepairCargo-GetRepairCargo();}
  float GetFreeAmmoCargo() const {return Type()->_maxAmmoCargo-GetAmmoCargo();}

  virtual float GetExplosives() const; // how much explosives is in

  float NeedsLoadFuel() const;
  float NeedsLoadAmmo() const;
  float NeedsLoadRepair() const;

  // if there is no driver, you can get in
  virtual bool IsPossibleToGetIn() const;
  virtual bool IsWorking() const;
  virtual bool IsAbleToMove() const;
  virtual bool IsAbleToFire(const TurretContext &context) const;
  virtual bool IsAbleToFireAnyTurret() const;
  virtual bool EjectNeeded() const;

  virtual float CalculateTotalCost() const;
  
  virtual bool GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const override;
  virtual void PerformAction(const Action *action, AIBrain *unit) override;

  /// initiate process of getting out
  void StartGetOutActivity(AIBrain *unit, bool eject);

  virtual void GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit) override;
#if _HELISIM_PRECISE_INTERACTION
  void GetDrawActions3D(UIActions &actions, AIBrain *unit);
  void GetDrawGetInActions3D(UIActions &actions, AIBrain *unit);
#endif
  void GetGetInActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit, bool now);

#if _HELISIM_PRECISE_INTERACTION
  virtual bool CheckDriverGetInPositionPrecise(Person *person, float radius, float radiusView) const;
  virtual bool CheckCargoGetInPositionPrecise(int cargoIndex, Person *person,  float radius, float radiusView) const;
  virtual bool CheckTurretGetInPositionPrecise(Turret *turret, Person *person, float radius, float radiusView) const;
#endif

  /// is there any commander for the vehicle?
  Person *CommanderPerson() const;

  /// check if the unit can switch position to the driver
  bool CanMoveToDriver(AIBrain *unit, int currentCompartments) const;
  /// check if the unit can switch position to the turret
  bool CanMoveToTurret(TurretContextV &context, AIBrain *unit, int currentCompartments) const;
  /// check if the unit can switch position to the cargo
  bool CanMoveToCargo(int index, AIBrain *unit, int currentCompartments) const;

  /// check if the unit can switch position in the vehicle
  bool CanChangePosition(AIBrain *unit) const;

  void ChangePosition(const Action *action, Person *soldier, bool netAware = false, bool forced = false);

  bool QCanIBeIn( Person *who ) const;

  virtual bool QCanIGetIn( Person *who=NULL ) const; // TODO: remove who argument
  virtual bool QCanIGetInGunner( Person *who=NULL ) const; // TODO: remove who argument
  virtual bool QCanIGetInCommander( Person *who=NULL ) const; // TODO: remove who argument
  virtual bool QCanIGetInTurret(Turret *turret, Person *who = NULL) const; // TODO: remove who argument
  /// check if person can get in specified cargo position (-1 for any), return the index of free cargo position (-1 if none exist)
  virtual int QCanIGetInCargo(Person *who = NULL, int index = -1) const; // both cargo and driver will do
  virtual bool QCanIGetInAny( Person *who=NULL ) const; // both cargo and driver will do

  virtual void DamageCrew( EntityAI *killer, float howMuch, RString ammo );
  virtual void HitBy( EntityAI *owner, float howMuch, RString ammo, bool wasDestroyed );
  virtual void Destroy( EntityAI *killer, float overkill, float minExp, float maxExp  ); // overkill==1 means no overkill
  virtual void Eject( AIBrain *unit, Vector3Val diff = VZero); // eject is possible, diff - move person from getinpos...
  virtual void Land(); // start landing autopilot
  virtual void CancelLand(); // start landing autopilot

  virtual void GetAimActions(float &aimX, float &aimY) const {}

  float GetFovFactor() const;

  virtual LSError Serialize(ParamArchive &ar);

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat
  (
    NetworkMessageClass cls,
    NetworkMessageFormat &format
  );
  TMError TransferMsg(NetworkMessageContext &ctx);
  float CalculateError(NetworkMessageContextWithError &ctx);
  virtual bool ChildUpdateByParent(const Entity *obj) const override;

  void DestroyObject();

  virtual Time GetDestroyedTime() const;

  virtual void ResetStatus();

  USE_CASTING(base)
  INHERIT_SOFTLINK(Transport,base);

};

typedef Transport VehicleTransport;

#endif
