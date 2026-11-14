#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TURRET_HPP
#define _TURRET_HPP

#include "smokes.hpp"
#include "proxyWeapon.hpp"

/// description of position 
struct GetInPoint
{
  Vector3 _pos;
  Vector3 _dir;
};
TypeIsSimpleZeroed(GetInPoint);
typedef const GetInPoint& GetInPointVal;

/// description of position in the vehicle type (not animated)
struct GetInPointIndex
{
  int _pos;
  int _dir;
};
TypeIsSimpleZeroed(GetInPointIndex);
typedef const GetInPointIndex& GetInPointIndexVal;

typedef AutoArray<GetInPointIndex> GetInPoints;
TypeIsMovable(GetInPoints)

struct ManProxy
{
  Matrix4 transform;
  //int selection; // copied over from proxyObject
  /// raw proxy in the model
  const ProxyObject *_proxy;

  ManProxy();
  bool Present() const {return _proxy!=NULL;}
};

TypeIsMovable(ManProxy)

/// Animation source for turret body
class AnimationSourceTurretBody : public AnimationSource
{
protected:
  /// handle of turret - each member is index in _turrets, starting in Transport(Type)
  /** turret location is described in turret hierarchy - the handle is a path */
  AutoArray<int> _path;

public:
  AnimationSourceTurretBody(RStringB name, int *data, int size) : AnimationSource(name), _path(data, size) {}
  float GetPhase(const ObjectVisualState &vs, const Animated *obj) const;
  void SetPhaseWanted(Animated *obj, float phase) {}
};

/// Animation source for turret gun
class AnimationSourceTurretGun : public AnimationSource
{
protected:
  /// handle of turret - each member is index in _turrets, starting in Transport(Type)
  AutoArray<int> _path;

public:
  AnimationSourceTurretGun(RStringB name, int *data, int size) : AnimationSource(name), _path(data, size) {}
  float GetPhase(const ObjectVisualState &vs, const Animated *obj) const;
  void SetPhaseWanted(Animated *obj, float phase) {}
};

/// Animation source for turret hatch
class AnimationSourceTurretHatch : public AnimationSource
{
protected:
  /// handle of turret - each member is index in _turrets, starting in Transport(Type)
  AutoArray<int> _path;

public:
  AnimationSourceTurretHatch(RStringB name, int *data, int size) : AnimationSource(name), _path(data, size) {}
  float GetPhase(const ObjectVisualState &vs, const Animated *obj) const;
  void SetPhaseWanted(Animated *obj, float phase) {}
};

enum StabilizedInAxes {SAxisNone,SAxisX,SAxisY,SAxisBoth};


//! turret type (shared info for all turrets)
class TurretType : public RefCount
{
  public:
  friend class Turret;

  /// definition config entry
  ConstParamClassDeepPtr _par;

  float _minElev,_maxElev,_initElev; // gun movement
  float _maxXRotSpeed, _maxYRotSpeed; // gun movement 
  float _minTurn,_maxTurn,_initTurn; // turret movement
  SoundPars _servoSound;

  /// animation responsible for body turning
  int _animBody;
  /// animation responsible for gun turning
  int _animGun;

  /// disable or enable stabilization in X or Y axis 
  StabilizedInAxes _stabilizedInAxes;
  
  /// gun position (neutral - rotation needs to be applied to get current position)
  Vector3 _pos0;
  /// gun direction (neutral - rotation needs to be applied to get current position)
  Vector3 _dir0;

  //@{ machine gun position and direction
  Vector3 _gunPos, _gunDir;
  /// machine guns memory points
  AutoArray<Vector3> _gunPosArray;      
  //@}

  //@{ turret specific missile position/direction
  Vector3 _missilePos, _missileDir;
  //@}
  /// when true, missile firing position is defined by the turret
  bool _missileByTurret;
  
  //when true, unit in turret has action eject
  bool _canEject;

  float _neutralXRot; // initial turret position (in model)
  float _neutralYRot; // initial turret position (in model)

  /// list of attached turrets types
  RefArray<TurretType> _turrets;
  /// weapons
  WeaponsType _weapons;

  // gunner related properties
  //@{
  /// identification of position
  RString _gunnerName;

  /// Action names corresponding to crew position
  RString _gunnerAction;
  RString _gunnerInAction;
  RString _gunnerGetInAction;
  RString _gunnerGetOutAction;

  Buffer<ManProxy> _gunnerProxy;

  int _gunnerDoor;

  // for observer with reflector
  bool _turretFollowFreeLook;

  /// list of positions where gunner can get into vehicle
  GetInPoints _gunnerGetInPos;
#if _HELISIM_PRECISE_INTERACTION
  GetInPoints _gunnerGetInPosPrecise;
#endif

  AutoArray<ViewPars> _viewGunner;
  AutoArray<ViewPars> _viewOptics;
  VehPosRelatives _turretSpec;

  //! Lights grouped in aggregation chunks
  ObjectTypeLights _lights;

#if _ENABLE_HAND_IK 
  SkeletonIndex _gunnerLeftHandIndex,_gunnerRightHandIndex;
  SkeletonIndex _gunnerLeftLegIndex,_gunnerRightLegIndex;
#endif

#if _VBS3
  ViewPars _viewOutOptics;
#endif

  /// gunner optics parameters
  int _gunnerOpticsPos;
  PackedColor _gunnerOpticsColor;
  /// out gunner optics parameters
  int _gunnerOutOpticsPos;
  PackedColor _gunnerOutOpticsColor;

  AutoArray<float> _discreteDistance;
  int _discreteDistanceInitIndex;

  /// switch vehicle engine on when this turret moved
  bool _startEngine;

  int _lodTurnedIn;
  int _lodTurnedOut;
#if _VBS3
  RString _overrideTurretName;
# if _VBS3_LASE_LEAD
  // Turret has a laser rangefinder
  bool _canLase;
# endif
#endif

  bool _gunnerUsesPilotView;
  bool _gunnerForceOptics;
  bool _gunnerOpticsShowCursor;
  bool _gunnerOutForceOptics;
  bool _gunnerOutOpticsShowCursor;
  /// gunner proxy should be casting a shadow
  bool _castGunnerShadow;
  //@{ see TransportType._viewDriverShadow etc.
  bool _viewGunnerShadow;
  bool _hideWeaponsGunner;
  float _viewGunnerShadowDiff;
  float _viewGunnerShadowAmb;
  //@}
  bool _ejectDeadGunner;
  /// gunner can hide when forceHideGunner set or hideProxyInCombat set and combat mode present
  bool _canHideGunner;
  /**
  In many armored vehicles the gunner cannot turn out at all
  */
  bool _forceHideGunner;
  bool _outGunnerMayFire, _outGunnerFireAlsoInInternalCamera ;
  bool _inGunnerMayFire, _inGunnerFireAlsoInInternalCamera;
  /**
  For some vehicles we need the gunner to be rendered in the external view
  even when the gunner is hidden
  */
  bool _viewGunnerInExternal;

  /// where gunner can switch position
  int _gunnerCompartments;

  /// primary gunner turret is controlled by the vehicle commander when manual fire is on
  bool _primaryGunner;
  /// primary observer turret is used for "where the vehicle looks"
  bool _primaryObserver;
  /// the turret is accessible through get in
  bool _hasGunner;
  /// how much the gunner in this turret command the vehicle (person with the highest value commands the vehicle)
  float _commanding;

  /// lock this turret in forward direction when driver is out
  bool _lockWhenDriverOut;
  /// lock this turret in forward direction when vehicle is in move and speedc is bigger that _lockWhenVehicleSpeed
  float _lockWhenVehicleSpeed;
  //@}

  /// Hit points related to the turret
  int _turretHit, _gunHit;

  //if true, unit can take control over vehicle
  bool _isCopilot;

  /// names of animation sources related to the turret
  RString _animationSourceBody, _animationSourceGun, _animationSourceHatch;

  /// gunner proxy identification
  CrewPosition _proxyType;
  int _proxyIndex;

  GetInOutMode _preciseGetInOut;

  bool _allowTabLock;
  ShowAllTargets _showAllTargets;

  AutoArray<PPEffectType> _opticsType;
  AutoArray<PPEffectType> _opticsOutType;

  /// resources for weapon optics info in HUD 
  AutoArray<RStringB> _turretInfoTypes;

  //////////////////////////////////////////////////////////////////////////
  //
  // Old interface (fixed turrets)
  //

  TurretType();
  void Load(ParamEntryPar cfg);

  //! get axis for rotation around Y axis (horizontal rotation)
  Vector3 GetYAxis(const EntityType *entType) const;
  //! get axis for rotation around X axis (vertical rotation)
  Vector3 GetXAxis(const EntityType *entType) const;
  
  //! get turret "center" position - center of rotation for aiming purposes
  Vector3 GetTurretAimCenter(const EntityType *entType) const;

  //! get actual turret position based on known turret animation matrix
  Vector3 GetTurretPos(const Matrix4 &trans) const {return trans.FastTransform(_pos0);}
  //! get actual turret direction based on known turret animation matrix
  Vector3 GetTurretDir(const Matrix4 &trans) const {return trans.Rotate(_dir0);}
  //! get actual turret direction based on known turret animation matrix
  Vector3 GetTurretDir(const Matrix3 &trans) const {return trans * _dir0;}

  //! combine _inGunnerMayFire and _inGunnerFireOnlyWithOptics
  bool InGunnerMayFire(const Person *gunner) const;
  //! combine _outGunnerMayFire and _outGunnerFireOnlyWithOptics
  bool OutGunnerMayFire(const Person *gunner) const;
  //! combine InGunnerMayFire, OutGunnerMayFire and IsGunnerHidden
  bool GunnerMayFire(const TurretContextV &context) const;

  SkeletonIndex GetBodyBone(const EntityType *entType, int level) const;
  SkeletonIndex GetGunBone(const EntityType *entType, int level) const;

  __forceinline int NGunnerGetInPos() const {return _gunnerGetInPos.Size();}
  __forceinline GetInPointIndexVal GetGunnerGetInPos(int i) const {return _gunnerGetInPos[i];}

#if _HELISIM_PRECISE_INTERACTION
  __forceinline int NGunnerGetInPosPrecise() const {return _gunnerGetInPosPrecise.Size();}
  __forceinline GetInPointIndexVal GetGunnerGetInPosPrecise(int i) const {return _gunnerGetInPosPrecise[i];}
#endif

  __forceinline const AutoArray<RStringB> &GetTurretInfoTypes() const {return _turretInfoTypes;}
  /// name of the movement action to get in
  RString GetGunnerGetInAction() const {return _gunnerGetInAction;}
  /// name of the movement action to get out
  RString GetGunnerGetOutAction() const {return _gunnerGetOutAction;}

  GetInOutMode GunnerPreciseGetInOut() const {return _preciseGetInOut;}

  //////////////////////////////////////////////////////////////////////////
  //
  // New interface (dynamic turrets)
  //

  TurretType(ParamEntryPar cfg);

  /// called when model is loaded
  void Init(LODShape *shape, EntityAIType *type);
  /// called when model is unloaded
  void Deinit(LODShape *shape, EntityType *type);
  /// called when model level is loaded
  void InitLevel(LODShape *shape, int level, EntityType *type);
  /// called when model level is unloaded
  void DeinitLevel(LODShape *shape, int level, EntityType *type);

  /// enumerate all turret types
  bool ForEachTurret(const EntityAIType *entityType, TurretPath &path, ITurretTypeFunc &func) const;
  /// search for turret with given path
  const TurretType *GetTurret(const int *path, int size) const;

  /// creates an animation source related to this turret
  template <class Allocator>
  AnimationSource *CreateAnimationSource(RStringB source, AutoArray<int, Allocator> &array)
  {
    int n = array.Size();
    if (strcmpi(source, _animationSourceBody) == 0)
      return new AnimationSourceTurretBody(source, array.Data(), n);
    if (strcmpi(source, _animationSourceGun) == 0)
      return new AnimationSourceTurretGun(source, array.Data(), n);
    if (strcmpi(source, _animationSourceHatch) == 0)
      return new AnimationSourceTurretHatch(source, array.Data(), n);
    for (int i=0; i<_turrets.Size(); i++)
    {
      array.Resize(n + 1);
      array[n] = i;
      AnimationSource *result = _turrets[i]->CreateAnimationSource(source, array);
      if (result) return result;
    }

    return NULL;
  }
};


#define CREATE_TURRET_MSG(MessageName, XX) \
  XX(MessageName, OLink(Entity), vehicle, NDTRef, NetworkId, NCTNone, DEFVALUENULL, DOC_MSG("Owner vehicle"), TRANSF_REF) \
  XX(MessageName, AutoArray<int>, path, NDTIntArray, AutoArray<int>, NCTSmallUnsigned, DEFVALUEINTARRAY, DOC_MSG("Path identifying turret in the vehicle"), TRANSF)

DECLARE_NET_INDICES_EX(CreateTurret, NetworkObject, CREATE_TURRET_MSG)

#if _VBS3
  #define UPDATE_TURRET_MSG(MessageName, XX) \
    XX(MessageName, Ref<Person>, gunner, NDTRef, NetworkId,  NCTNone, DEFVALUENULL, DOC_MSG("Gunner person"), TRANSF_REF, ET_NOT_EQUAL, ERR_COEF_STRUCTURE) \
    XX(MessageName, bool, locked, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("Gunner position is locked"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
    XX(MessageName, float, crewHiddenWanted, NDTFloat, float, NCTFloat0To1, DEFVALUE(float, 0), DOC_MSG("Wanted position of crew - inside / outside"), TRANSF, ET_NOT_EQUAL, ERR_COEF_VALUE_MAJOR) \
    XX(MessageName, RefArray<Weapon>, weapons, NDTRefArray, AutoArray<NetworkId>, NCTNone, DEFVALUEREFARRAY, DOC_MSG("List of weapons"), TRANSF_REFS, ET_NOT_EQUAL_COUNT, ERR_COEF_MODE) \
    XX(MessageName, RefArray<Magazine>, magazines, NDTRefArray, AutoArray<NetworkId>, NCTNone, DEFVALUEREFARRAY, DOC_MSG("List of magazines"), TRANSF_REFS, ET_NOT_EQUAL_COUNT, ERR_COEF_MODE) \
    XX(MessageName, Ref<Turret>, overriddenByTurret, NDTRef, NetworkId,  NCTNone, DEFVALUENULL, DOC_MSG("Overriding Turret"), TRANSF_REF, ET_NOT_EQUAL, ERR_COEF_STRUCTURE)
#else //Arma2
  #define UPDATE_TURRET_MSG(MessageName, XX) \
    XX(MessageName, Ref<Person>, gunner, NDTRef, NetworkId,  NCTNone, DEFVALUENULL, DOC_MSG("Gunner person"), TRANSF_REF, ET_NOT_EQUAL, ERR_COEF_STRUCTURE) \
    XX(MessageName, bool, locked, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("Gunner position is locked"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
    XX(MessageName, float, crewHiddenWanted, NDTFloat, float, NCTFloat0To1, DEFVALUE(float, 0), DOC_MSG("Wanted position of crew - inside / outside"), TRANSF, ET_NOT_EQUAL, ERR_COEF_VALUE_MAJOR) \
    XX(MessageName, RefArray<Weapon>, weapons, NDTRefArray, AutoArray<NetworkId>, NCTNone, DEFVALUEREFARRAY, DOC_MSG("List of weapons"), TRANSF_REFS, ET_NOT_EQUAL_COUNT, ERR_COEF_MODE) \
    XX(MessageName, RefArray<Magazine>, magazines, NDTRefArray, AutoArray<NetworkId>, NCTNone, DEFVALUEREFARRAY, DOC_MSG("List of magazines"), TRANSF_REFS, ET_NOT_EQUAL_COUNT, ERR_COEF_MODE) \
    XX(MessageName, OLinkPermO(Weapon), currentWeapon, NDTRef, NetworkId, NCTNone, DEFVALUENULL, DOC_MSG("The currently selected weapon"), TRANSF_REF, ET_NOT_EQUAL, ERR_COEF_MODE) \
    XX(MessageName, bool, searchLightOn, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("Search lights are on / off"), TRANSF, ET_NOT_EQUAL, ERR_COEF_VALUE_MAJOR)
#endif
DECLARE_NET_INDICES_EX_ERR(UpdateTurret, NetworkObject, UPDATE_TURRET_MSG)

#define UPDATE_POSITION_TURRET_MSG(MessageName, XX) \
  XX(MessageName, bool, gunStabilized, NDTBool, bool, NCTNone, DEFVALUE(bool, true), DOC_MSG("Gun is stabilized"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, float, yRotWanted, NDTFloat, float, NCTFloatAngle, DEFVALUE(float, 0), DOC_MSG("Wanted rotation in y axis"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR) \
  XX(MessageName, float, xRotWanted, NDTFloat, float, NCTFloatAngle, DEFVALUE(float, 0), DOC_MSG("Wanted rotation in x axis"), TRANSF, ET_ABS_DIF, ERR_COEF_VALUE_MAJOR)

DECLARE_NET_INDICES_EX_ERR(UpdatePositionTurret, NetworkObject, UPDATE_POSITION_TURRET_MSG)

class Entity;

typedef AutoArray<int, MemAllocLocal<int, 16> > TurretPath;

// turret/gun hierarchy
// each turret/gun has center of rotation
// relative to his parent

/// turret - used for rotating weapons on tanks, helis and other vehicles
/**
Some member values are sometimes absolute angles.
ToRelative() and ToAbsolute() can do the conversions when necessary.
*/

struct VisionSettings
{
  int _tiIndex;
  VisionMode _visionMode;

  LSError Serialize(ParamArchive &ar)
  {
    CHECK(ar.Serialize("tiIndex", _tiIndex, 1, 0))
    if(ar.IsSaving())
    {
      int vision = (int)_visionMode;
      CHECK(ar.Serialize("visionMode",vision, 1, 0))
    }
    else
    {
      int vision = 0;
      CHECK(ar.Serialize("visionMode", vision, 1, 0))
      _visionMode = (VisionMode)vision;
    }
    return LSOK;
  }
};
TypeIsSimple(VisionSettings)

class TurretVisualState;
class TransportType;

/// perform deep copy on construction or assignment
class TurretVisualStateArray: public RefArray<TurretVisualState>
{
public:
  TurretVisualStateArray(){}
  TurretVisualStateArray(const TurretVisualStateArray &src);
  void operator = (const TurretVisualStateArray &src);
};

class TurretVisualState : public RefCount
{
  friend class Transport;
  friend class TransportVisualState;
  friend class Turret;
  friend class HideGunnerFunc;

  //@{ actual values are always in local space
  float _xRot,_yRot;
  //@}
  /// gunner is hidden
  float _crewHidden;
  /// list of attached turret visual states (mirrors Turret::_turrets)
  TurretVisualStateArray _turretVisualStates;

  // need deep copies
public:
  bool _IKIdent;
  Matrix4 _leftHandIK;
  Matrix4 _rightHandIK;
  Matrix4 _leftLegIK;
  Matrix4 _rightLegIK;

  TurretVisualState(TurretType const& type, TransportType const& ownerType);

  float GetGunnerHidden() const { return _crewHidden; }
  bool IsGunnerHidden() const { return _crewHidden > 0.1; }

  Matrix3 TurretOrientation() const { return Matrix3(MRotationY,_yRot); }
  Matrix3 GunOrientation() const { return Matrix3(MRotationY,_xRot); }
  Matrix4 TurretTransform(const EntityType *entType, const TurretType &type) const;
  Matrix4 GunTransform(const EntityType *entType, const TurretType &type) const;

  void Init(const TurretType &type) { _xRot = type._initElev; _yRot = type._initTurn; }

  TurretVisualState* GetTurret(const int *path, int size);
  TurretVisualState const* GetTurret(const int *path, int size) const;

  void Interpolate(TurretVisualState const& s1, float t, TurretVisualState& res) const;
};

class Turret : public NetworkObject, public IWeaponsHolder
{
  friend class Transport; // needed for Transport::ChangePosition

protected:
  NetworkId _networkId;
  bool _local;	// local / remote in network game

  /// gunner position locked  (nobody can get in / move to it)
  bool _locked;

  OLinkNO(Transport) _owner;

  /// list of attached turrets
  RefArray<Turret> _turrets;

  /// owned weapons
  WeaponsState _weaponsState;
  RefArray<Weapon> _weapons;              //!< Weapons
  RefArray<Magazine> _magazines;          //!< Magazines (reserve only)
  OLinkPermO(Weapon) _currentWeapon; // selected weapon

  /// gunner controlling this turret (is inside)
  Ref<Person> _gunner;
  /// gunner assigned to this turret
  OLinkPermNO(AIBrain) _gunnerAssigned;

  // current vision mode
  VisionMode _visionMode;
  // current ti mode
  int _tiIndex;

  //
  bool _searchLightOn;

  AutoArray<VisionSettings> _oldviewModes;
  AutoArray<VisionSettings> _oldGunnerModes;

#if _VBS3
protected:
  TurretType _type;
  VisionMode _visionMode;
  VisionMode _visionOutMode;
  //index of TI mode stored in viewoptics
  int _tiModeIndex;
  int _tiModeOutIndex;
  Vector3 _lockOnPos;
  OLinkNO(Object) _lockOnObj;
public:
  const Transport* GetOwner() const {return _owner.GetLink();}

  //@{ if active the gunner cannot control the turret
  bool _disableGunnerInput;
  //@{ turret that can be overridden
  OLinkPermNO(Turret) _overrideTurret;

  OLinkNO(Turret) _overriddenByTurret;

  bool _showOverrideOptics;

  bool NextOverrideOptics() {_showOverrideOptics = !_showOverrideOptics; return _showOverrideOptics;}

  bool IsOverridden() {return !_overriddenByTurret.IsNull();}
  bool IsOverriding() {return _overrideTurret && _overrideTurret->_overriddenByTurret == this;}

  void SetOverrideTurretLink();

  DWORD _overrideStartTime;
  bool UpdateOverrideStatus();

  bool GunnerIsOverridingOtherTurret();
  void StartOverride();
  void StopOverride();

  //switches to next vision mode, doesn't apply it
  VisionMode NextVisionMode();
  VisionMode GetVisionMode() const;

  //returns true if it requires NV
  bool ApplyVisionMode();
  int GetTiModus();

  //based on if the gunner is in or out
  const ViewPars* GetCurrentViewPars() const;

#if _VBS3_LASE_LEAD
  //-1 means not lased
  enum LLStatus
  {
    LLError,
    LLAdjusting,
    LLReady
  };
  LLStatus GetLaseStatus();

  int _laseDistance;
  //stores the weapon which last lased
  int _laserWeaponAimed;
  float _laseOffsetX, _laseOffsetXWanted;
  float _laseOffsetY;
  void Lase(Vector3& pos, Vector3& aimDir, Vector3 camDir);
  void ClearFCS();
#endif //_VBS3_LASE_LEAD

#endif //_VBS3
  
public:
  //@{ if turret is stabilized, wanted values are always in absolute coordinate system
  float _xRotWanted,_yRotWanted;
  //@}

  // gun heat factor
  float _heatFactor;

  float _xSpeed;
  float _ySpeed;
  //! zooming
  Object::Zoom _zoom;

  RefAbstractWave _servoSound;
  float _servoVol;
  bool _gunStabilized; // compensate any orientation changes
  Matrix3 _stabilizeTo; //matrix of previous orientation

  /// gunner wants to be hidden
  float _crewHiddenWanted;

  int _gunPosIndexToogle;

  int _currentViewGunnerMode;
  int _currentViewOpticsMode;

  //used for camera locked on target
  Vector3 _lastLockedPosition;
  bool _lockOnGround;

  //elevation
  int _distanceIndex;

  //! Each item of the array contains group of aggregated lights
  ObjectLights _lights;


  // used only for serialization
  Turret();
  Turret(const TurretType &type, Transport *owner);
  ~Turret();

  void Init(const TurretType &type, Matrix3Par parent);

  const Transport* GetOwner() const {return _owner.GetLink();}

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

  void Animate(const TurretType &type, AnimationContext &animContext, LODShape *shape, int level, bool setEngineStuff, Object *parentObject);
  void Deanimate(const TurretType &type, LODShape *shape, int level, bool setEngineStuff);

  float GetGunnerHiddenWanted() const {return _crewHiddenWanted;}
  /** see also Transport::IsDriverHidden */
  void HideGunner(float hide = 1.0);
  
  // @{ visual state controllers
  float GetCtrlXRot(const TurretVisualState& vs) const { return vs._xRot; }
  float GetCtrlYRot(const TurretVisualState& vs) const { return vs._yRot; }
  float GetCtrlGunnerHatchPos(const TurretVisualState& vs) const {return 1.0f - vs.GetGunnerHidden();}
  // @}

  void ZeroingUp(const TurretType *type);
  void ZeroingDown(const TurretType *type);
  // @{ locking of positions in the vehicle
  bool IsGunnerLocked() const {return _locked;}
  void LockGunner(bool lock) {_locked = lock;}
  //@}
  bool SomeLights() {return _lights.SomeLights();}

  void NextOpticsMode(Person *person, const TurretType *type);
  void PreviousOpticsMode(Person *person, const TurretType *type);
  bool CheckVisionMode(VisionMode oldVMode, int tiIndex, const TurretType *type, ViewPars view);
  
  WeaponsState &GetWeapons() {return _weaponsState;}

  /// get turret based on path in hierarchy
  const Turret *GetTurret(const int *path, int size) const;
  /// get turret based on path in hierarchy
  Turret *GetTurret(const int *path, int size);

  /// get path in hierarchy for given turret
  template <class Allocator>
  bool GetTurretPath(AutoArray<int, Allocator> &path, const Turret *turret) const
  {
    if (this == turret) return true;
    int n = path.Size();
    for (int i=0; i<_turrets.Size(); i++)
    {
      path.Resize(n + 1);
      path[n] = i;
      if (_turrets[i]->GetTurretPath(path, turret)) return true;
    }
    return false;
  }

  Person *Gunner() const {return _gunner;}
  void SetGunner(Person *gunner) {_gunner = gunner;}

  AIBrain *GetGunnerAssigned() const {return _gunnerAssigned;}
  void AssignGunner(AIBrain *unit) {_gunnerAssigned = unit;}

  void UnloadSound();
  void Sound(
    Transport *parent,
    const TurretType &type, bool inside, float deltaT,
    FrameBase &pos, Vector3Val speed // parent position
  );

  float GunnerAnimSpeed() const {return 1;}

  /// simulate weapon movement
  bool MoveWeapons(TurretVisualState& vs, const TurretType &type, AIBrain *unit, float deltaT, Matrix3Par parent);
  
  void Stop(const TurretType &type, Matrix3Par parent);
  void GunBroken(const TurretType &type, Matrix3Par parent);
  void TurretBroken(TurretVisualState& vs, const TurretType &type, Matrix3Par parent);
  /// lock turret and gun pointing forward
  void LockForward(const TurretType &type);

  /// turret stabilization - enabled controls to work independent on hull rotation
  void Stabilize(
    TurretVisualState& vs,
    const Entity *obj,
    const TurretType &type, Matrix3Val oldTrans, Matrix3Val newTrans
  );
  /// aim, coordinates given in absolute space
  bool AimWorld(TurretVisualState& vs, const TurretType &type, Vector3Val relDir);
#if _VBS3
  /// Variant used for scripted call
  bool AimWorld(TurretVisualState& vs, float azimuth, float elevation, bool forceSet = false);
  /// Ability to lock on the turret
  void SetLockOn(Vector3 pos) {_lockOnPos = pos; _lockOnObj = NULL;}
  void SetLockOn(Object* obj) {_lockOnObj = obj; _lockOnPos = VZero;}
#endif
  /// aim, coordinates given in relative space
  bool Aim(TurretVisualState& vs, const TurretType &type, Vector3Val dir, Matrix3Par parent);
  void SetSpeed(TurretVisualState& vs, const TurretType &type, float speedX, float speedY, float deltaT, Matrix3Par parent);
  Matrix3 GetAimWantedWorld(Matrix3Par parent) const;
  Matrix3 GetAimWantedRel() const;
  Matrix3 GetAimWantedWorld() const;
  void SetStabilization(bool stabilized, Matrix3Par parent);

  bool IsLightOn() const {return _searchLightOn;}
  void SetLightOn(bool lightOn) { _searchLightOn = lightOn;}

  /// returns if hatch position changed
  bool SimulateHatch(TurretVisualState &vs, float deltaT, SimulationImportance prec);

  static Turret *CreateObject(ParamArchive &ar);
  LSError Serialize(ParamArchive &ar);
  static Turret *LoadRef(ParamArchive &ar);
  LSError SaveRef(ParamArchive &ar);

  virtual void SetNetworkId(NetworkId &id) {_networkId = id;}
  virtual NetworkId GetNetworkId() const {return _networkId;}
  virtual Vector3 GetCurrentPosition() const;
  virtual bool IsLocal() const {return _local;}
  virtual void SetLocal(bool local = true) {_local = local;}
  virtual void DestroyObject();
  
  //! Get debugging name (debugging only)
  virtual RString GetDebugName() const;
  //! Get name of owner vehicle (debugging only)
  const char *GetVehicleName() const;

  DECLARE_NETWORK_OBJECT
  static Turret *CreateObject(NetworkMessageContext &ctx);

  static NetworkMessageFormat &CreateFormat(
    NetworkMessageClass cls,
    NetworkMessageFormat &format
  );
  TMError TransferMsg(NetworkMessageContext &ctx);
  float CalculateError(NetworkMessageContextWithError &ctx);

  void CreateSubobjects(Transport *owner, TurretPath &path);

  Vector3 GetCenter(const EntityType *entType, const TurretType &type) const;

  //////////////////////////////////////////////////////////////////////////
  //
  // New interface (dynamic turrets)
  //

  /// enumerate all turrets
  bool ForEachTurret(Transport *vehicle, const TurretType *type, ITurretFunc &func) const;
  /// enumerate all turrets
  bool ForEachTurret(TurretVisualState& vs, Transport *vehicle, const TurretType *type, ITurretFuncV &func) const;
  /// enumerate all turrets
  bool ForEachTurretEx(TurretVisualState& vs, Transport *vehicle, const TurretType *type, Matrix3Val parentTrans, ITurretFuncEx &func) const;

  /// Simulation of visual effects
  void SimulateEffects(TurretVisualState &vs, const Transport *obj, const TurretType &type, float deltaT);

  /// Simulation of turret (returns if turret moved)
  bool Simulate(TurretVisualState& vs, const Transport *obj, const TurretType &type, float deltaT, SimulationImportance prec, Matrix3Val parentTrans);

  /// name of the movement action gunner need to be animated currently 
  RString GunnerAction(TurretVisualState &vs, const TurretType &type) const;

private:
  // helper for Stabilize
  void StabilizeXY(
    const TurretType &type,
    float &xRot, float &yRot, Matrix3Val oldTrans, Matrix3Val newTrans
  );
  /// absolute to relative conversion - used for xRot/yRot pairs
  void ToRelative(float &xr, float &yr, float x, float y, Matrix3Par parent);
  /// relative to absolute conversion - used for xRot/yRot pairs
  void ToAbsolute(float &x, float &y, float xr, float yr, Matrix3Par parent);

  Object::Zoom *GetZoom() {return &_zoom;}

public:
  /// switch to next vision mode 
  bool NextVisionMode(const TurretType *type);
  /// get currently used vision mode
  VisionMode GetVisionModePars(int &tiIndex) const;
  bool HasVisionModes(const TurretType *type) const;
  bool HasThermalModes(const TurretType *type) const;
  int VisionModesCount(const TurretType *type) const;

  const ViewPars* GetCurrentViewPars(const TurretType *type, bool allTurrets = false) const;
};

#endif
