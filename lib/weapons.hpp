#ifdef _MSC_VER
#pragma once
#endif

#ifndef _WEAPONS_HPP
#define _WEAPONS_HPP

#include "wpch.hpp"
#include "paramFileExt.hpp"
#include "vehicle.hpp"
#include "animation.hpp"
#include "camShake.hpp"

#if _VBS3 //added muzzle events
  #include <El/Enum/enumNames.hpp> 
  #include <El/Evaluator/expressImpl.hpp>
#endif

enum AmmoSimulation
{ // basic ammo types
  AmmoNone,
  AmmoShotShell,
  AmmoShotMissile,
  AmmoShotRocket,
  AmmoShotBullet,
  AmmoShotSpread,
  AmmoShotIlluminating,
  AmmoShotSmoke,
  AmmoShotTimeBomb,
  AmmoShotPipeBomb,
  AmmoShotMine,
  AmmoShotStroke
};

enum AmmoTargetType
{
  AmmoLockGroundTT = 0,
  AmmoLockAirGroundTT = 1,
  AmmoLockAirTT = 2
};

DEFINE_ENUM_BEG(BowState)
  BSIdle,
  BSDrawn,
  BSReleased,
  BSCanceled
  DEFINE_ENUM_END(BowState)

struct SoundProbab: public SoundPars
{
  float _probability;
};

TypeIsMovableZeroed(SoundProbab);

class RandomSound
{
  AutoArray<SoundProbab> _pars;

  public:
  void Load( ParamEntryPar base, const char *name );
  const SoundPars &SelectSound( float probab ) const;
  bool PreloadSounds() const;
};

// information about post effect used for optics
struct PPEffectType
{
  // 
  RString _effectType;
  int _priority;
  // parameters
  AutoArray<float> _pars;

  void Load(ParamEntryPar cfg);
};

TypeIsMovable(PPEffectType);

#if _HELISIM_RENDER_TO_TEXTURE

struct R2TCameraDescription
{
  enum R2TCameraPostEffect { PENormal, PENightVision, PEThermal, PEColors, PEMirror };
  enum R2TRenderQuality { QLow = 1, QNormal = 2, QHigh = 3 };

  Vector3 _pos;
  Vector3 _dir;
  // Position set as memory point
  RString _posName;
  int _posIndex;
  // Direction set as memory point
  RString _dirName; 
  int _dirIndex;
  float _fov;
  int _turretTypeIndex; // TurretType* cannot be there, index is set in TransportType::InitShape 
  mutable R2TCameraPostEffect _postEffect;
  // post effect params
  mutable AutoArray<float> _pePars;
  mutable bool _peParsUpdated;
  mutable R2TRenderQuality _renderQuality;

  R2TCameraDescription();
  void SetToDefault();
  void Load(ParamEntryPar cfg);
	LSError Serialize(ParamArchive &ar);
};
TypeIsSimpleZeroed(R2TCameraDescription);

struct R2TTgtDescription: public SerializeClass
{
  RString _name;
  AutoArray<R2TCameraDescription> _camDesc;
  ~R2TTgtDescription()
  {
    _camDesc.Clear();
  }

  bool Load(ParamEntryPar cfg);
  LSError Serialize(ParamArchive &ar);
};
TypeIsMovableZeroed(R2TTgtDescription);

class R2TTgtPars: public RefCount
{
  int _currentIndex;
  Matrix4 _camTransformation;
  const R2TTgtDescription &_tgtDesc;

public:
  R2TTgtPars(const R2TTgtDescription &tgtDesc): _tgtDesc(tgtDesc), _camTransformation(M4Identity), _currentIndex(-1) {}

  const R2TCameraDescription& Next();

  __forceinline bool IsValid() const { return _currentIndex != -1; }
  __forceinline void Reset() { _currentIndex = _tgtDesc._camDesc.Size() > 0 ? 0 : -1; }
  __forceinline Matrix4& Transform() { return _camTransformation; }
  __forceinline const RString& Name() const { return _tgtDesc._name; }
  __forceinline const AutoArray<R2TCameraDescription>& Descriptions() const { return _tgtDesc._camDesc; }
  __forceinline bool IsEmpty() const { return _tgtDesc._camDesc.Size() == 0; }
  __forceinline const R2TCameraDescription& Desc() const 
  {
    DoAssert(_currentIndex >= 0 && _currentIndex < _tgtDesc._camDesc.Size());     
    return _tgtDesc._camDesc[_currentIndex]; 
  }
  __forceinline void SetEffect(R2TCameraDescription::R2TCameraPostEffect r2tEffect, float *pars, int count) const 
  {
    R2TCameraDescription& desc = const_cast<R2TCameraDescription&>(Desc());

    desc._postEffect = r2tEffect;
    desc._pePars.Access(count);

    for (int i = 0; i < count; i++)
      desc._pePars[i] = pars[i];

    desc._peParsUpdated = true;    
  }
  __forceinline void SetQuality(R2TCameraDescription::R2TRenderQuality r2tQuality) const 
  {
    Desc()._renderQuality = r2tQuality;
  }
};
TypeIsMovableZeroed(R2TTgtPars);

#endif

struct HitEffectsInfo
{
  RString _name;
  RString _effects;
};
TypeIsMovableZeroed(HitEffectsInfo)

/// params for camera shake - used for reading values from config
class CameraShakePars
{
public:
  CameraShakePars():_isValid(false), _power(0.0f), _duration(0.0f), _frequency(0), _distance(0.0f), _minSpeed(0.0f) {}

  /// reads values from given cfg
  void Load(ParamEntryPar cfg);

  bool IsValid() const {return _isValid;}
  float GetPower() const {return _power;}
  float GetDuration() const {return _duration;}
  float GetFrequency() const {return _frequency;}
  float GetDistance() const {return _distance;}
  float GetMinSpeed() const {return _minSpeed;}

  /// returns if power is non zero
  bool HasPower() const
  {
    static const float epsilon = 0.0001f; // just a very small number for testing
    return _power > epsilon;
  }
  /// returns if distance is non zero
  bool HasDistance() const
  {
    static const float epsilon = 0.0001f; // just a very small number for testing
    return _distance > epsilon;
  }

private:
  /// if this is valid (it was read from cfg)
  bool _isValid;
  /// max power of shake (is decreased based on distance of player from shake)
  float _power;
  /// max duration of shake (in s) (is decreased based on distance of player from shake)
  float _duration;
  /// frequency of camera changes in shakes (number of changes per second) - if 0, then default value from CameraShakeManager is used
  float _frequency;
  /// max distance from player to produce the shake (in m)
  float _distance;
  /// min speed of vehicle to produce shake effect
  float _minSpeed;
};


#if _VBS3
#define AMMO_EVENT_ENUM(type,prefix,XX) \
  XX(type, prefix, AmmoHit) /*string:weapon,string:muzzle,string:mode,string:ammo*/ \
  XX(type, prefix, AmmoExplode) /*string:weapon,string:muzzle,string:mode,string:ammo*/ 

#ifndef DECL_ENUM_MUZZLE_EVENT_ENUM
# define DECL_ENUM_MUZZLE_EVENT_ENUM
DECL_ENUM(AmmoEvent)
#endif
DECLARE_ENUM(AmmoEvent,AE,AMMO_EVENT_ENUM)
#endif

/// ammunition information
class AmmoType: public EntityType
{
  typedef EntityType base;

#if _VBS3 //Ammo Destroyed event
  protected:
    RString _eventHandlers[NAmmoEvent];
  public:
    //! generic event handler with typical parameter sets
    void OnEvent(AmmoEvent event, const GameValue &pars) const;
    //! check if there is some event handler
    bool IsEventHandler(AmmoEvent event) const;
#endif

public:
  float hit,indirectHit,indirectHitRange;

  float maxControlRange; // missile parameters
  float maneuvrability;
  /// how much of the tracking is done using over-steering
  float trackOversteer;
  /// how much of the tracking is done using computed leading of the target
  float trackLead;
  
  float initTime; //!< time to ignition
  float thrustTime; //!< time of burning
  float thrust; //!< how strong the engine is
  float sideAirFriction; //!< air friction (used for missiles / bombs)
  float explosionTime;
  /// some shots (hand grenades) are fused until they travel a certain distance
  float fuseDistance;
  
  float cost;
  float maxSpeed; // max speed of missiles
  float simulationStep;
  bool irLock; // ir lock only
  bool laserLock; // laser lock only
  bool nvLock; // night vision marker lock only
  AmmoTargetType airLock; // air lock possible
  bool manualControl; // no control out of maxControlRange
  float explosive;
  float caliber;
  /// some bullets can be deflected after a hit
  float deflecting;
  /// slowdown coefficient for deflection (speed will be multiplied by this coef after the ammo deflects from some surface)
  float _deflectionSlowDown;
  float _timeToLive;
  float _airFriction;
  float _coefGravity;
  /// speed at which hit and indirectHit values are valid
  float _typicalSpeed2;
  //!Affect ragdoll skeleton (melee, shots etc.)
  float _affectSkeleton;

  Ref<Texture> _texture;
  AmmoSimulation _simulation;
  Ref<VehicleType> _cartridgeType; // what kind of cartridges to we use (effect)
  //! shape that should be used when drawing proxy on vehicle
  Ref<LODShape> _proxyShape;
  //! shape used for a projectile stuck in another vehicle
  Ref<LODShape> _decalShape;

  PackedColor _tracerColor;   //! tracer color with alpha
  PackedColor _tracerColorR;  //!< tracer color in realistic mode

  //defines proxi ID for missiles (proxy index == ammo count + maverickweaponIndexOffset)
  int maverickweaponIndexOffset;

  float audibleFire;
  float visibleFire;
  float visibleFireTime;
#if _VBS3
  float audibleFireTime;
#endif
  // samples group for specified group
  RandomSound _hit[SurfaceInfo::NSoundHitTypes];

  // fake bullets samples
  RandomSound _bulletFly;

  SoundPars _soundFly;
  SoundPars _soundEngine;
  //SoundPars _supersonicCrackNear;
  //SoundPars _supersonicCrackFar;
  
  RString _defaultMagazine;

  RString _explosionEffects;
  RString _craterEffects;
  AutoArray<HitEffectsInfo> _hitEffects;
  
  // smoke trails for rockets
  RString _effectsMissile;
  // smoke effects (only for smokeshell simulation)
  RString _effectsSmoke;
  // class of items to be spawned as decal at hit location
  RString _decalClass;

  //@{VBS style tracers
  float _tracerScale;
  float _tracerStartTime;
  float _tracerEndTime;
  bool _nvgOnly;
  //@}

  // whistling radius
  float _whistleDist;
  // 0 .. on fire, 1 .. on explosion, 2 .. both
  int _whistleOnFire;

  //@{Camera shake parameters
  /// on explosion
  CameraShakePars _camShakeExplode;
  /// on weapon fire (general)
  CameraShakePars _camShakeFire;
  /// effect on player, when he fires from the weapon with this ammo 
  CameraShakePars _camShakePlayerFire;
  /// effect on player, when he is hit with this ammo
  CameraShakePars _camShakeHit;
  //@}

  
  AmmoType(ParamEntryPar name);

  void InitShape();
  void DeinitShape();
  bool AbstractOnly() const { return false; }

  virtual void Load(ParamEntryPar par, const char *shape);

  ParamEntryVal ParClass() const {return *_par;}
  ParamEntryVal ParClass(const char *name) const { return (*_par)>>name; }

  RString GetHitEffects(RString name) const;

  const SoundPars& FakeBulletSound(float prob) const { return _bulletFly.SelectSound(prob); }
  float GetMass() const {return 0.01f;} // TODO: use config mass instead
  float GetInvMass() const {return 1/0.01f;} // TODO: use config mass instead

};

/// information about how explosion / damage caused by ammo
struct HitInfo
{
  float _hit;
  float _indirectHit;
  float _indirectHitRange;
  float _explosive;
#if _VBS2
  int _external; //enumeration is specified in hlabase. required to prevent sending hit events back to HLA
#endif
  /// some information is used from the ammo type
  const AmmoType *_ammoType;
  Vector3 _pos;
  Vector3 _surfNormal;
  Vector3 _inSpeed;
  Vector3 _outSpeed;

  HitInfo(const AmmoType *ammoType, Vector3Par pos, Vector3Par surfNormal, Vector3Par inSpeed, Vector3Par outSpeed=VZero);
};

typedef const HitInfo &HitInfoPar;

DECL_ENUM(ManAction)

class EntityPlainType;

struct CursorTextureSection
{
  float uMin;
  float vMin;
  float uMax;
  float vMax;
  float xOffset;
  float yOffset;
};
TypeIsSimple(CursorTextureSection)

struct CursorTextureInfo
{
  Ref<Texture> texture;
  float fade;
  PackedColor color;
  int shadow;
  AutoArray<CursorTextureSection> sections;

  CursorTextureInfo() {fade = 1.0f; color = PackedWhite; shadow = 2;}
};

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#if _VBS3 //moved struct definition from vehicleAI

  enum CanSee
  {
    CanSeeRadar=1,
    CanSeeEye=2,
    CanSeeOptics=4,
    CanSeeEar=8,
    CanSeeCompass=16,
    CanSeePeripheral=32,
    //CanSeeAll=~0
  };

  enum VisionMode
  {
    VMDv,
    VMNv,
    VMTi,
    VMInvalid
  };

  // camera parameters
  struct ViewPars
  {
    float _initAngleY,_minAngleY,_maxAngleY;
    float _initAngleX,_minAngleX,_maxAngleX;
    float _initFov,_minFov,_maxFov;

    void Load( ParamEntryPar cfg );
    void InitVirtual
      (
      CameraType camType, float &heading, float &dive, float &fov
      ) const;
    void LimitVirtual
      (
      CameraType camType, float &heading, float &dive, float &fov
      ) 
      const;

# if _VBS3
  protected:
    //! Nightvision
    bool _hasNV;
    //! Normal Day Vision
    bool _hasDV;
    //! Class was properly loaded
    bool _initialized;
  public:

    AutoArray <float> _discreteFov;
    AutoArray <int> _thermalModes;

    RString _opticsModel; //stores just the name, also indication for "VBS2 supported viewoptics class"
    PackedColor _opticsColor;

    //specify a CanSee per weapon
    int _canSee;
    void DiscretizeValue(float fov, float &prev, float &closest, float &next) const;
    bool DiscreteValues() const { return _discreteFov.Size() > 0;}
    int GetNextThermalMode(int oldThermalmode) const;

    bool HasNV() const {return _hasNV;}
    bool HasDV() const {return _hasDV;}
    bool Initialized() const {return _initialized;}

    bool HasTI() const {return _thermalModes.Size() > 0;}

    void NextVisionMode(VisionMode& mode, int &tiModeIndex) const;

    bool ApplyVisionMode(VisionMode& mode, int& tiModus) const;

    ViewPars():_initialized(false),_canSee(0),_hasNV(true),_hasDV(true){};
# endif
  };
#endif //!_VBS3 //moved class to weapon.hpp

#if !_VBS3
  #ifndef DECL_ENUM_VISION_MODE
  #define DECL_ENUM_VISION_MODE
    DECL_ENUM(VisionMode)
  #endif

  DEFINE_ENUM_BEG(VisionMode)
    VMNormal, VMNvg, VMTi, VMCount, VMInvalid
  DEFINE_ENUM_END(VisionMode)

  // camera parameters
  struct ViewPars
  {
    float _initAngleY,_minAngleY,_maxAngleY;
    float _initAngleX,_minAngleX,_maxAngleX;
    float _initFov,_minFov,_maxFov;
    float _minMoveX, _maxMoveX, _minMoveY, _maxMoveY, _minMoveZ, _maxMoveZ;

    AutoArray<int> _visionModes;
    AutoArray<int> _thermalModes;

    bool _directionStabilized;

    RString _opticsDisplayName;

    //optics models
    Ref<LODShape> _gunnerOpticsModel;

    float _continuous;

    __forceinline bool HasVisionModes() const { return _visionModes.Size() > 0; }
    __forceinline bool HasThermalModes() const { return _thermalModes.Size() > 0; }
    __forceinline int VisionModesCount() const { return _visionModes.Size(); }
    void NextVisionMode(VisionMode& mode, int &tiModeIndex) const;
    void GetInitVisionMode(VisionMode& mode, int &tiModeIndex) const;

    void Load(ParamEntryPar cfg);
    void InitVirtual(CameraType camType, float &heading, float &dive, float &fov) const;
    void LimitVirtual(CameraType camType, float &heading, float &dive, float &fov) const;
  };
  TypeIsMovable(ViewPars);

  struct VehPosRelatives
  {
    bool _showHeadPhones;
    void Load(ParamEntryPar cfg);
  };
  TypeIsMovable(VehPosRelatives)

#endif

/* Hold flash light params */
struct FlashLigthInfo: public RefCount
{
  Color _color;
  Color _colorAmbient;
  RString _positionMem;
  RString _directionMem;
  float _radius;
  Vector3 _position;
  Vector3 _direction;
  Vector3 _scale;
  float _angle;
	float _innerAngleRatio;

  bool Load(ParamEntryPar cls);
  void Init(const LODShape * shape);
};

struct OpticsCameraInfo
{
  Vector3 _cameraDir, _cameraPos;
  float _neutralXRot, _neutralYRot;
  OpticsCameraInfo()
  {
    _cameraPos = VZero;
    _cameraDir = VForward;
    _neutralXRot = 0;
    _neutralYRot = 0;
  }
};

/* Hold optics settings params */
struct OpticsInfo : public OpticsCameraInfo
{
  bool _useModelOptics;

  AutoArray<PPEffectType> _ppEffectType; 

  bool _opticsFlare;
  float _opticsDisablePeripherialVision;
  float _opticsZoomMin,_opticsZoomMax,_opticsZoomInit;
  float _distanceZoomMin,_distanceZoomMax;
  int _id;
  RString _memoryPointCamera;

  float GetNormalZoom() const
  {
    // never return value above than 1 if the weapon is capable of 1
    return _opticsZoomMax>1 ? floatMax(_opticsZoomMin,1) : _opticsZoomMax;
  }

  RString _opticsDisplayName;

  //vision modes
  AutoArray<int> _visionModes;
  AutoArray<int> _thermalModes;

  //discrete fov
  AutoArray<float> _discreteFov;
  AutoArray<float> _discreteDistance;
  int _discreteInitIndex;
  int _discreteDistanceInitIndex;

  /// compensation to aim with the _cameraDir, not with the base weapon direction

  OpticsInfo() : OpticsCameraInfo()
  {
    _id = -1;
    _discreteInitIndex = 0;
    _discreteDistanceInitIndex = 0;
  }

  __forceinline bool HasVisionModes() const { return _visionModes.Size() > 0; }
  __forceinline int VisionModesCount() const { return _visionModes.Size(); }
  void NextVisionMode(VisionMode& mode, int &tiModeIndex) const;
  void GetInitVisionMode(VisionMode& mode, int &tiModeIndex) const;
  bool HasVisionMode(VisionMode mode) const;
};
TypeIsMovable(OpticsInfo);

#define MaskSlotPrimary     0x00000001  // primary weapons
#define MaskSlotSecondary   0x00000004  // secondary weapons
#define MaskSlotBinocular   0x00003000  // binocular
#define MaskHardMounted     0x00010000  // hard mounted
#define MaskSlotHandGun     0x00000002  // hand gun
#define MaskSlotInventory   0x001E0000  // hand gun magazines

/// Weapon simulation (special item type) enum
#define WEAPON_SIMULATION_ENUM(type, prefix, XX) \
  XX(type, prefix, Weapon) /* regular weapon */ \
  XX(type, prefix, Binocular) /* binocular */ \
  XX(type, prefix, NVGoggles) /* night vision goggles */ \
  XX(type, prefix, CMLauncher) \

#ifndef DECL_ENUM_WEAPON_SIMULATION
#define DECL_ENUM_WEAPON_SIMULATION
DECL_ENUM(WeaponSimulation)
#endif

DECLARE_ENUM(WeaponSimulation, WS, WEAPON_SIMULATION_ENUM)

inline RString ExtractActionName(RStringB name)
{
  const char *prefix = "ManAct";
  if (strnicmp(name, prefix, strlen(prefix)) == 0) return (const char *)name + strlen(prefix);
  return name;
}

#endif

