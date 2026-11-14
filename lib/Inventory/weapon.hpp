#ifdef _MSC_VER
#pragma once
#endif

#ifndef _WEAPON_HPP
#define _WEAPON_HPP

#include "inventory.hpp"
// #include "../weapons.hpp"

class WeaponModeType : public RefCount
{
public:
  ConstParamClassDeepPtr _parClass;
  RStringB _displayName;      // used in InGameUI
  int _mult;                  // take this from ammo store (bullets in small burst)
  int _burst;
  //SoundPars _sound;           // shot sound
  RandomSound _beginSound;
  float _soundDuration;
  float _reloadTime;          // how long it takes to reload
  float _dispersion;          // dispersion
  /// default recoil
  RStringB _recoilName;
  /// recoil when prone
  RStringB _recoilProneName;
  float _aiRateOfFire; // AI rate of fire at given distance
  float _aiRateOfFireDistance;

  bool _soundContinuous;
  /// when burst is fired, only a first bullet is making a sound
  bool _soundBurst;
  bool _autoFire;             // full auto weapons
  bool _useAction;
  bool _showToPlayer;
  RString _useActionTitle;

  float minRange,minRangeProbab;
  float midRange,midRangeProbab;
  float maxRange,maxRangeProbab;

  float invMidRangeMinusMinRange; // precalculated inverse
  float invMidRangeMinusMaxRange;

  //multiplier of dispersion, when 1, there is  1 meter dispersion with every second of flight
  float artilleryDispersion;
  float artilleryCharge;

public:
  WeaponModeType();
  void Init(ParamEntryPar cls);
  const RStringB &GetName() const {return _parClass->GetName();}
  RStringB GetDisplayName() const {return _displayName;}
};

#if _VBS3
#define MUZZLE_EVENT_ENUM(type,prefix,XX) \
  XX(type, prefix, Fired) /*string:weapon,string:muzzle,string:mode,string:ammo*/ 

  #ifndef DECL_ENUM_MUZZLE_EVENT_ENUM
  # define DECL_ENUM_MUZZLE_EVENT_ENUM
  DECL_ENUM(MuzzleEvent)
  #endif
  DECLARE_ENUM(MuzzleEvent,ME,MUZZLE_EVENT_ENUM)
#endif

class MuzzleType : public RefCount, public LODShapeLoadHandler
{
#if _VBS3 //MuzzleEvent
protected:
  RString _eventHandlers[NMuzzleEvent];
public:
  //! generic event handler with typical parameter sets
  void OnEvent(MuzzleEvent event, const GameValue &pars);
  //! check if there is some event handler
  bool IsEventHandler(MuzzleEvent event) const;
  //! will show the trajectory
  bool _showTrajectory;

#endif

protected:
  CursorTextureInfo _cursorTexture;
  CursorTextureInfo _cursorAimTexture;
  // relative cursor size, 1 is normal
  float _cursorSize;

public:
  ConstParamClassDeepPtr _parClass;
  RStringB _displayName;      // used in InGameUI (when no magazine loaded)
  RString  _reloadAction;
  float _magazineReloadTime;
  SoundPars _sound;           // no shot sound
  SoundPars _reloadSound;
  SoundPars _reloadMagazineSound;
  RandomSound _bullets;
  float _soundDuration; 
  float _reloadSoundDuration; 
  float _reloadMagazineSoundDuration;
  float _aiDispersionCoefX;
  float _aiDispersionCoefY;

  // values for optics as attachments
  OpticsCameraInfo _opticsCameraInfo;

  /// shot spread when spread ammo used
  float _shotSpreadAngle;

  void GetDispersionCoefs(float &x, float &y, float invAimingAccuracy) const
  {
    // invAimingAccuracy is assumed to be in range 1..5
    float f = (invAimingAccuracy-1)*0.25f;
    x = f*_aiDispersionCoefX+1;
    y = f*_aiDispersionCoefY+1;
  }
  
  bool _soundContinuous; // weapon sound is continuous
  bool _enableAttack;
  bool _optics;
  bool _showEmpty;
  bool _autoReload;
  bool _backgroundReload;
  //bool _opticsFlare;
  /// 1 = force optics always, 2 = force optics for 1st person view only
  signed char _forceOptics;
  //float _opticsDisablePeripherialVision;
  //weapons also support ViewOptics
#if _VBS3 //weapons also support ViewOptics
  ViewPars  _viewOptics;
#endif

  bool _canAutoAim;
  bool _showAimCursorInternal;
  bool _showSwitchAction;
  
  int _canBeLocked;
  int _ballisticsComputer; // should ballistics computer (and eventually which one) be used?
  int _primary; // primary level

  /// what stance is needed to use the weapon
  bool _useAsBinocular;

//  float _opticsZoomMin,_opticsZoomMax,_opticsZoomInit;
//  float _distanceZoomMin,_distanceZoomMax;

  /// 2D optics model
  Ref<LODShape> _opticsModel;

  /// 3D optics enabled
  bool _opticsEnable;

  /// fire animation for 2D optics
  AnimationAnimatedTexture _animFire;

  /// camera position and direction
  //Vector3 _cameraPos, _cameraDir;
  //@{ position and direction of muzzle
  Vector3 _muzzlePos, _muzzleDir, _muzzleEnd;
  // infra red laser position and direction
  Vector3 _irLaserPos, _irLaserDir;

#if _VBS3
  //@{ position of a laser attachment
  Vector3 _laserPos;
#endif

  //@{ position and velocity of outgoing empty cartridge
  Vector3 _cartridgeOutPos, _cartridgeOutVel;
  int _cartridgeOutPosIndex, _cartridgeOutEndIndex;
  //@}


  RefArray<WeaponModeType> _modes;
  RefArray<MagazineType> _magazines;  // which ammo can be used
  Ref<MagazineType> _typicalMagazine; // for type (expected magazine)

  RString _magazineSlot; // name of the attachment slot for the magazine
  Ref<Texture> _magazineSlotIcon;

  //AutoArray<PPEffectType> _ppEffectType;
  // flash light params
  Ref<FlashLigthInfo> _gunLightInfo;
  //optics settings
  AutoArray<OpticsInfo> _opticsInfo;
  // has any optics Ti
  bool _hasTiVision;

  float _irDistance; // 0 - no IR

public:
  MuzzleType();
  virtual ~MuzzleType();

  void Init(ParamEntryPar cls, const WeaponType *weapon);
  static bool InitOpticModes(ParamEntryPar cls, const WeaponType *weapon, AutoArray<OpticsInfo> &out);
  
  void InitCameraParams(const ParamEntry &cls, LODShape *shape, OpticsCameraInfo &opticsInfo) const;

  virtual void InitShape(const WeaponType *weapon);
  virtual void InitShapeOpticsShape(const WeaponType *weapon);
  virtual void DeinitShape(const WeaponType *weapon);
  void InitShapeLevel(const WeaponType *weapon, int level);
  void DeinitShapeLevel(const WeaponType *weapon, int level);

  //@{ LODShapeLoadHandler
  virtual void LODShapeLoaded(LODShape *shape);
  virtual void LODShapeUnloaded(LODShape *shape);
  virtual void ShapeLoaded(LODShape *shape, int level);
  virtual void ShapeUnloaded(LODShape *shape, int level);
  //@} LODShapeLoadHandler

  bool CanUse(const MagazineType *type) const;

  const RStringB &GetName() const {return _parClass->GetName();}
  const RString &GetMagazineSlot() const {return _magazineSlot;}

  virtual float GetCursorSize() const {return _cursorSize;}
  virtual const CursorTextureInfo *GetCursorTexture() const {return &_cursorTexture;}
  virtual const CursorTextureInfo *GetCursorAimTexture() const {return &_cursorAimTexture;}

  __forceinline bool HasTiVision() const { return _hasTiVision; }
  __forceinline bool HasVisionModes(int opticsMode) const {return _opticsInfo[opticsMode].HasVisionModes();}
  __forceinline int VisionModesCount(int opticsMode) const { return _opticsInfo[opticsMode].VisionModesCount(); }
  void NextVisionMode(VisionMode& mode, int &tiModeIndex, int opticsMode) const;
  void GetInitVisionMode(VisionMode& mode, int &tiModeIndex, int opticsMode) const;
};
TypeIsMovable(MuzzleType)

/*!
\patch 1.22 Date 09/04/2001 by Jirka
- Added: Support for revolving drum for weapons.
*/
class WeaponType: public InventoryItemType
{
private:
  typedef InventoryItemType base;

public:
  int _scope;                 // public / protected for briefing
  RStringB _picName;          // used in briefing
  RStringB _description;      // description for overview / library
  
  /// enable to find special items (see WEAPON_SIMULATION_ENUM for the list)
  WeaponSimulation _simulation;

  Ref<Texture> _picture;      // used in group info (InGameUI)
  int _weaponType;            // slots occupied - see MaskSlot
  bool _shotFromTurret;
  bool _canDrop;              // can be dropped
  bool _bow;                  // is a type of bow, it needs to be drawn before shooting
  //! infantry weapon have different mass and can influence dexterity
  float _dexterity;
  //! used in MP game for decision which body to hide
  float _value;
  /// duration of light flash
  float _fireLightDuration; 
  /// intensity of light flash
  float _fireLightIntensity;
#if _VBS2 // fatigue model
  // make lightsources view mode dependent
  int _lightMode;
#endif

  RefArray<MuzzleType> _muzzles;

  mutable AnimationAnimatedTexture _animFire;

  /// resources for weapon optics info in HUD 
  AutoArray<RStringB> _weaponInfoTypes;

  //weapon mass (used for RTD helicopter Total weight)
  float _mass;

public:
  WeaponType(ParamEntryPar param);
  virtual void Load(ParamEntryPar par, const char *shape) override;

  const RStringB &GetName() const {return _par->GetName();}
  RStringB GetPictureName() const;
  Texture *GetPicture() const {return _picture;}

  WeaponSimulation GetSimulation() const {return _simulation;}

  RStringB GetDescription() const {return _description;}

  //! mass of weapon
  float GetMass() const {return _mass;}

  virtual AnimationSource *CreateAnimationSource(const AnimationType *type, RStringB source) override;

  virtual void InitShape() override;
  virtual void DeinitShape() override;
  virtual void InitShapeLevel(int level) override;
  virtual void DeinitShapeLevel(int level) override;

  __forceinline const AutoArray<RStringB> &GetWeaponInfoTypes() const {return _weaponInfoTypes;}

  virtual Texture *GetAttachmentIcon(int i) const;

  virtual Object* CreateObject(bool fullCreate) const override;
};

class VisionModePars : public SerializeClass
{
  friend struct Muzzle;
  friend struct WeaponsState;

private:
  VisionMode _visionMode; // current vision mode
  int _flirIndex;         // current flir mode index

public:
  virtual LSError Serialize(ParamArchive &ar) override
  {
    CHECK(ar.SerializeEnum("visionMode", _visionMode, 1, ENUM_CAST(VisionMode, VMNormal)))
    CHECK(ar.Serialize("flirIndex", _flirIndex, 1, -1))

    return LSOK;
  }
  virtual bool IsDefaultValue(ParamArchive &ar) const override {return _visionMode==VMNormal && _flirIndex==-1;}
  virtual void LoadDefaultValues(ParamArchive &ar) override {_visionMode = VMNormal;_flirIndex=-1;}
};

// State of the weapon's muzzle
struct Muzzle : public SerializeClass
{
  // magazine loaded to this muzzle
  Ref<Magazine> _magazine;
  // magazine prepared to be loaded
  Ref<Magazine> _magazineWanted;

  int _opticsMode;
  int _discreteIndex; //index of discrete optics
  int _discreteDistanceIndex;
  VisionModePars _visionPars;

  WeaponSounds _sound;

  /// used during reload driver by soldier animation
  float _reloadMagazineProgress;

  /// used while drawing the bow
  float _drawBowProgress;
  float _drawBowMax;

  /// used while shooting the bow
  float _releaseBowProgress;

  // index of the selected weapon mode
  int _currentMode;

  Muzzle() 
  {
    _opticsMode = 0; _currentMode = 0;
    _discreteIndex = 0; _discreteDistanceIndex = 0;
    _reloadMagazineProgress = 0;
    _drawBowProgress = 0;
    _drawBowMax = 0;
    _releaseBowProgress = 1;  //idle pos is at the end of progress
  }

  void Init(const MuzzleType *type);

  // SerializeClass implementation
  virtual bool IsDefaultValue(ParamArchive &ar) const override;
	virtual LSError Serialize(ParamArchive &ar) override;
	virtual void LoadDefaultValues(ParamArchive &ar) override;

  // network synchronization
  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format);
  TMError TransferMsg(NetworkMessageContext &ctx);
};
TypeIsMovable(Muzzle)

#define UPDATE_MUZZLE_MSG(MessageName, XX) \
  XX(MessageName, Ref<Magazine>, magazineWanted, NDTRef, NetworkId, NCTNone, DEFVALUENULL, DOC_MSG("Wanted loaded magazine"), TRANSF_REF) \
  XX(MessageName, Ref<Magazine>, magazine, NDTRef, NetworkId, NCTNone, DEFVALUENULL, DOC_MSG("Loaded magazine"), TRANSF_REF) \
  XX(MessageName, int, currentMode, NDTInteger, int, NCTSmallSigned, DEFVALUE(int, -1), DOC_MSG("Index of currently selected weapon mode"), TRANSF) \
  // TODO: check if less important members need synchronization: _reloadMagazineProgress, _opticsMode, _discreteIndex, _discreteDistanceIndex, _visionPars

DECLARE_NET_INDICES(UpdateMuzzle, UPDATE_MUZZLE_MSG)

// Weapon instance
class Weapon : public InventoryItem
{
private:
  typedef InventoryItem base;

protected:
  // phase for WeaponType::_animType animation
  int _phaseFire;
  //! Heat factor of the gun (note this class doesn't store it, it's lifetime is in scope Init-Clear)
  float _gunHeatFactor;

  // state of all muzzles
  AutoArray<Muzzle> _muzzles;
  // index of the selected muzzle
  int _currentMuzzle;

public:
  Weapon(const EntityAIType *type, bool fullCreate);

  const WeaponType *Type() const
  {
    return static_cast<const WeaponType *>(GetType());
  }

  // cast optimization
  virtual Weapon *GetWeapon() override {return this;}
  virtual const Weapon *GetWeapon() const override {return this;}

  virtual bool RemoveOwnedItem(InventoryItem *item) override;

  virtual float GetMetabolismFactor() const override {return _gunHeatFactor;}

  virtual bool PrepareShapeDrawMatrices(Matrix4Array &matrices, const ObjectVisualState &vs, LODShape *shape, int level) override;

  virtual bool CanBeInstanced(int level, float distance2, const DrawParameters &dp) const override;
  virtual AnimationStyle IsAnimated(int level) const override;
  virtual void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2) override;
  virtual void Deanimate(int level, bool setEngineStuff) override;

  // TODO: replace by own simulation
  void SetAnimPhase(int phaseFire);

  /// Weapon heating after fire
  void AddHeat();

  //! Simulation
  virtual void Simulate(float deltaT, SimulationImportance prec) override;

  virtual bool CanAutoShortcut() const override;
  virtual bool UseFromInventory(Person *owner) override;

  // animation controllers
  float GetRevolvingPos(const ObjectVisualState &vs) const;
  float GetReloadPos(const ObjectVisualState &vs) const;
  float GetMagazineReloadPos(const ObjectVisualState &vs) const;
  float GetDrawBowPos(const ObjectVisualState &vs) const;
  float IsEmpty(const ObjectVisualState &vs) const;
#if _VBS3 
  float GetAmmo() const;
#endif

  const OpticsInfo *GetAttachedOpticsInfo() const;
  ItemOptics *GetAttachedOptics();
  const float SumDispersionModifiers(const EntityAIFull *agent) const;
  const float SumDexterityModifiers(const EntityAIFull *agent) const;

  // use the _modelShape from the current magazine if needed
  void UpdateShape();

  Magazine *GetMagazine(int muzzle)
  {
    if (muzzle < 0 || muzzle >= _muzzles.Size()) return NULL;
    return _muzzles[muzzle]._magazine;
  }
  const Magazine *GetMagazine(int muzzle) const
  {
    if (muzzle < 0 || muzzle >= _muzzles.Size()) return NULL;
    DoAssert(muzzle < _muzzles.Size());
    return _muzzles[muzzle]._magazine;
  }
  int NMuzzles() const {return _muzzles.Size();}
  Muzzle &GetMuzzle(int muzzle)
  {
    DoAssert(muzzle >= 0 && muzzle < _muzzles.Size());
    return _muzzles[muzzle];
  }
  const Muzzle &GetMuzzle(int muzzle) const
  {
    DoAssert(muzzle >= 0 && muzzle < _muzzles.Size());
    return _muzzles[muzzle];
  }

  int GetCurrentMuzzle() const
  {
    return _currentMuzzle;
  }
  Muzzle &SetCurrentMuzzle(int muzzle) 
  {
    DoAssert(muzzle >= 0 && muzzle < _muzzles.Size());
    _currentMuzzle = muzzle;
    return _muzzles[muzzle];
  }

  // access to attachments
  virtual int NAttachments() const override;
  virtual const InventoryItem *GetAttachment(int i) const override;
  virtual InventoryItem *GetAttachment(int i) override;
  virtual InventoryItem *GetAttachment(RString slotName) override;
  virtual InventoryItem *FindAttachment(InventorySlotId slotId) const override;
  // check if attachment can be added to the weapon's slot
  virtual bool CanAddAttachment(const InventoryItem *item, int index) const override;
  // add attachment to the weapon's slot if slot is empty
  virtual bool AddAttachment(InventoryItem *item, int index) override;
  // check if attachment can be added to the weapon's slot
  virtual bool CanAddAttachment(const InventoryItem *item) const override;
  // add attachment to the weapon's slot if slot is empty
  virtual bool AddAttachment(InventoryItem *item) override;

  // serialization
  virtual LSError Serialize(ParamArchive &ar) override;
  static Weapon *CreateObject(ParamArchive &ar);

  // network synchronization
  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format);
  TMError TransferMsg(NetworkMessageContext &ctx);
  float CalculateError(NetworkMessageContextWithError &ctx);

  virtual void CreateSubobjects() override;
  virtual void UpdateSubobjects() override;

  // JSON serialization
  virtual void JSONSave(JSONValue &container, JSONAllocator &allocator) const override;
  virtual bool JSONLoad(JSONValue &container, Person *person) override;

  INHERIT_SOFTLINK(Weapon, base);
  USE_CASTING(base)
};

#define UPDATE_WEAPON_MSG(MessageName, XX) \
  XX(MessageName, AutoArray<Muzzle>, muzzles, NDTObjectArray, REF_MSG_ARRAY(NetworkMessageUpdateMuzzle), NCTNone, DEFVALUE_MSG(NMTUpdateMuzzle), DOC_MSG("List of muzzles"), TRANSF_ARRAY, ET_MUZZLES, 1) \
  XX(MessageName, int, currentMuzzle, NDTInteger, int, NCTSmallSigned, DEFVALUE(int, -1), DOC_MSG("Index of currently selected muzzle"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \

DECLARE_NET_INDICES_EX_ERR(UpdateWeapon, UpdateInventoryItem, UPDATE_WEAPON_MSG)

#endif // _WEAPON_HPP
