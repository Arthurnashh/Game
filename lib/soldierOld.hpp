#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SOLDIER_OLD_HPP
#define _SOLDIER_OLD_HPP

/*!
\file
Interface file for Man and Soldier classes
*/

#include "person.hpp"
#include "animation.hpp"
#include "AI/ai.hpp"
#include "head.hpp"
#include "heads.hpp"
#include "global.hpp"
#include "envTracker.hpp"

#if _ENABLE_HAND_IK
#include "IKengine.hpp"
#endif

#include "motion.hpp"

#include "manActs.hpp"

#include <Es/Memory/normalNew.hpp>

#include "Physics\enf_worldphysics.h"
#include "El/Math/math3d.hpp"

// 5 times per sec is enough
#define AI_MAX_SIM_TIME 0.2f 
#define AI_MAX_SIM_TIME_NEAR 0.1f 
// Player must be simulated more precisely
#define PLAYER_MAX_SIM_TIME 0.05

#define ALLOWED_VOL_GROW 0.9f

#if _ENABLE_CHEATS
// For DEBUG: Store info about last actions
struct SDebugActionsInfo
{
  struct SInfo
  {
    RString type;
    RString actionName;
    RString moveName;
    unsigned int time;
  };

  AutoArray<SInfo> lastActions;

  void AddInfo(RString actionName, RString moveName, RString type);
  RString GetDiagInfo(RString prefix);
};
#endif

class PlayActionHandlerContext: public ActionContextBase
{
  GameDataCode _handler;

protected:
  OLinkO(Man) _man;

public:
  PlayActionHandlerContext(){}
  PlayActionHandlerContext(Man *man):_man(man){}
  PlayActionHandlerContext(Man *man, const GameDataCode &handler):_man(man),_handler(handler){}
  /// called once given context is finished
  virtual void Do(Man *ai);
  //!Support for animhooks to script
  virtual void Event(int userType, const RStringB& param) override;

  virtual LSError Serialize(ParamArchive &ar);
};

class PlayActionContext: public PlayActionHandlerContext
{
  typedef PlayActionHandlerContext base;

  MoveIdExt _move;

public:

  PlayActionContext(Man *man, const MoveIdExt &move):base(man),_move(move){}
  PlayActionContext(Man *man, const MoveIdExt &move, const GameDataCode &handler):base(man,handler),_move(move){}

  virtual MoveId GetPlayMove() const override {return _move.id;}

  virtual LSError Serialize(ParamArchive &ar)
  {
    Fail("PlayActionContext Serialize not supported, MP only");
    return LSUnknownError;
  }
};

/// properties shared by all layers of man movement
class MoveInfoManCommon: public MoveInfoBase 
{
  typedef MoveInfoBase base;

  friend class Man;

  /// sometimes weapon firing is not possible
  bool _disableWeapons;
  /// mask of weapon types optics can be shown for
  int _enableOptics;
  /// when weapon is disabled for a long time, automatic activation on fire is possible
  bool _showWeaponAim;
  /// during the animation, the aiming is forced instead of turning
  bool _forceAim;
  bool _enableMissile;
  bool _enableBinocular;
  bool _enableFistStroke;
  bool _enableGunStroke;
  bool _showItemInHand;
  bool _showItemInRightHand;
  bool _showHandGun;

#if _ENABLE_HAND_IK
  // curves for IK blending 
  AutoArray<float> _leftHandIKCurve;
  AutoArray<float> _rightHandIKCurve;

  AutoArray<float> _leftLegIKCurve;
  AutoArray<float> _rightLegIKCurve;
#endif
  //! Flag to determine the trigger can be pulled during the move (just visual effect)
  /*!
    If the trigger will be really pulled depends also on _disableWeapons flag. This flag can
    only disable pulling in case weapon is enabled.
  */
  bool _canPullTrigger;

  /// strength of the head bob effect, eventually reducible by a player
  // valid values: <0,1> reducible by a player; <0,-1> ditto but in-reducible (for example in cut scenes)
  float _headBobStrength;
  /// appropriate head bob mode
  HeadBobModeEnum _headBobMode;

  //! transition states from armed to unarmed animation state should switch to unarmed in engine
  bool _forceUnarmed;

public:
  MoveInfoManCommon(MotionType *motion, ParamEntryPar entry);

  __forceinline bool WeaponsDisabled() const {return _disableWeapons;}  
  __forceinline int EnableOptics() const {return _enableOptics;}
  __forceinline bool ShowWeaponAim() const {return _showWeaponAim;}
  __forceinline bool ForceAim() const {return _forceAim;}
  __forceinline bool MissileEnabled() const {return _enableMissile;}
  __forceinline bool EnableBinocular() const {return _enableBinocular;}
  __forceinline bool EnableGunStroke() const {return _enableGunStroke;}
  __forceinline bool EnableFistStroke() const {return _enableFistStroke;}
  __forceinline bool ShowItemInHand() const {return _showItemInHand;}
  __forceinline bool ShowItemInRightHand() const {return _showItemInRightHand;}
  __forceinline bool ShowHandGun() const {return _showHandGun;}
  __forceinline bool CanPullTrigger() const {return _canPullTrigger;}
  __forceinline float HeadBobStrength() const {return _headBobStrength;}
  __forceinline HeadBobModeEnum HeadBobMode() const {return _headBobMode;}

  ActionContextBase *GetAutoContext() const override;

#if _ENABLE_HAND_IK
// find and calculate position in curve and return blend factor
  float LeftHandIK( float time ) const;
  float RightHandIK( float time ) const;

  float LeftLegIK( float time ) const;
  float RightLegIK( float time ) const;
#endif

  void LoadAsSubclass(MotionType *motion, ParamEntryPar entry, ParamEntryPar actionMaps) override;

  USE_FAST_ALLOCATOR
};

/// information about a single move state - extended properties for a Man
class MoveInfoMan: public MoveInfoManCommon 
{
  typedef MoveInfoManCommon base;

  friend class Man;
  
  /// sometimes weapon firing is not possible
  /// when weapon is disabled for a long time, automatic activation on fire is possible
  bool _disableWeaponsLong;
  bool _onLandBeg,_onLandEnd; // copy surface direction
  bool _enableAutoActions;
  bool _onLadder;
  /// direct control over the speed allowed - micro movements
  bool _enableDirectControl;
  //! can be blended with more steps, other than secondary
  bool _canBlendStep;

  float _duty;
  float _visibleSize;
  int _bodyPosition;
  float _aimPrecision;
  /// camera shake coef for firing
  float _camShakeFire;

  Ref<BlendAnimType> _aiming;
  Ref<BlendAnimType> _aimingBody;
  Ref<BlendAnimType> _leaning;
  Ref<BlendAnimType> _legs;
  Ref<BlendAnimType> _head;

  //! Particular move can have a leaning more or less disabled
  float _leaningFactorBeg, _leaningFactorEnd, _leaningFactorZeroPoint;

  float _limitGunMovement;

  AutoArray<MoveVariant> _variantsPlayer; // random variants
  AutoArray<MoveVariant> _variantsAI;
  float _variantAfterMin; // how long should we wait before playing variant
  float _variantAfterMid;
  float _variantAfterMax;

  //@{ collision detection info    
  Ref<CollisionCapsule> _collShape;	 //! shape with collision capsule
  Ref<CollisionCapsule> _collShapeSafe; //! shape with safe collision capsule
  // some moves may have bounding sphere enlarged
  float _boundingSphere;
  //@}
#if _ENABLE_HAND_IK
  /// type of weapon hands are attached to (rifle, launcher, handgun)
  int _weaponIK;
#endif
  /// whether animation subclasses are subject of inheritage (dafault is false)
  bool _subclassInherit;

public:
  MoveInfoMan(MotionType *motion, ParamEntryPar entry);
  
  static bool Preload(ParamEntryPar entry);

  __forceinline bool DisableWeaponsLong() const {return _disableWeaponsLong;}
  __forceinline bool EnableAutoActions() const {return _enableAutoActions;}
  __forceinline bool EnableDirectControl() const {return _enableDirectControl;}
  __forceinline bool OnLadder() const {return _onLadder;}
  __forceinline bool OnLandBeg() const {return _onLandBeg;}
  __forceinline bool OnLandEnd() const {return _onLandEnd;}
  float OnLand( float time ) const {return _onLandBeg*(1-time)+_onLandEnd*time;}
  __forceinline float GetDuty() const {return _duty;}
  bool CanBlendStep() const override {return _canBlendStep;}

  float GetStepLength() const {return _move->GetStepLength();}
  float GetStepLengthX() const {return _move->GetStepLengthX();}
	float GetStepSpeed() const;
	float GetStepSpeedX() const;


	struct SpeedRange
  {
    float minSpeed,maxSpeed;
    SpeedRange():minSpeed(+FLT_MAX),maxSpeed(-FLT_MAX){}
    SpeedRange(float s):minSpeed(s),maxSpeed(s){}
    SpeedRange(float mins, float maxs):minSpeed(mins),maxSpeed(maxs){}
  };

  SpeedRange GetStepSpeedRange() const;
  SpeedRange GetStepSpeedRangeX() const;

  const BlendAnimSelections &GetAiming() const {return *_aiming;}
  const BlendAnimSelections &GetAimingBody() const {return *_aimingBody;}
  const BlendAnimSelections &GetLeaning() const {return *_leaning;}
  const BlendAnimSelections &GetLegs() const {return *_legs;}
  const BlendAnimSelections &GetHead() const {return *_head;}

  float LeaningFactor(float time) const;

  __forceinline float GetLimitGunMovement() const {return _limitGunMovement;}

  __forceinline float GetVisibleSize() const {return _visibleSize;}
  __forceinline float GetAimPrecision() const {return _aimPrecision;}
  __forceinline float GetCamShakeFire() const {return _camShakeFire;}
  __forceinline int GetBodyPosition() const {return _bodyPosition;}


  const AutoArray<MoveVariant> GetVariantsPlayer() const {return _variantsPlayer;}
  const AutoArray<MoveVariant> GetVariantsAI() const {return _variantsAI;}
      
  MoveId RandomVariant( const AutoArray<MoveVariant> &vars, float rnd ) const;
  MoveId RandomVariantPlayer() const;
  MoveId RandomVariantAI() const;
  void LoadVariants(
    AutoArray<MoveVariant> &vars, AutoArray<MoveVariant> *defaultVars,
    MotionType *motion, ParamEntryPar entry, RString member
  ) const;

  float GetVariantAfter() const;

  /// Function replaces GeomtryLevel in pLODShapeTo by _collShape or _collShapeSafe.
  bool AnimateCollGeom(AnimationContext &animContext,  const Shape *shape,
    const Mapping& vertexMapping, const Mapping& geomCompMapping, bool bUseSafe = false) const;
  /// Function returns true if for animation is define safe collision capsule.
  bool HasCollCapsuleSafe() const {return _collShapeSafe.NotNull();}

  void InitAnimSubclasses(MotionType *motion, ParamEntryPar entry, ParamEntryPar actionMaps);
  void LoadAsSubclass(MotionType *motion, ParamEntryPar entry, ParamEntryPar actionMaps) override;

  USE_FAST_ALLOCATOR
};

/// information about a single move state - - extended properties for a Man gesture
class MoveInfoGestureMan : public MoveInfoManCommon 
{
  typedef MoveInfoManCommon base;

  friend class Man;
  
  Ref<BlendAnimType> _mask;

  public:
  MoveInfoGestureMan(MotionType *motion, ParamEntryPar entry);
  
  static bool Preload(ParamEntryPar entry);
  const BlendAnimSelections &GetMask() const {return *_mask;}

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

/// manager of MoveInfoGestureMan
class MovesTypeGestureMan : public MovesTypeBase
{
  typedef MovesTypeBase base;

protected:
  ActionMap::Index _actionIndices[ManActN];

public:
  MovesTypeGestureMan();
  ~MovesTypeGestureMan();

  const MoveInfoGestureMan *GetMoveInfo(MoveId move) const
  {
    return static_cast<const MoveInfoGestureMan *>(GetMoveInfoBase(move, NoSubclassIx));
  }

  //default GetPreloadFunc() will do

  virtual MoveInfoBase *NewMoveInfo(ParamEntryPar entry)
  {
    return new MoveInfoGestureMan(this, entry);
  }
};

/// manager of MoveInfoMan
class MovesTypeMan : public MovesTypeBase
{
  typedef MovesTypeBase base;

protected:
  ActionMap::Index _actionIndices[ManActN];

  /// we may need to reference additional layers
  Ref<MovesTypeGestureMan> _gestures;

public:
  MovesTypeMan();
  ~MovesTypeMan();

  virtual void Load(const MovesTypeName &name);

  const MoveInfoMan *GetMoveInfo(MoveId move, int subIx) const
  {
    return static_cast<const MoveInfoMan *>(GetMoveInfoBase(move, subIx));
  }

  MoveInfoPreloadFunc GetPreloadFunc() const override {return &MoveInfoMan::Preload;}

  MoveInfoBase *NewMoveInfo(ParamEntryPar entry) override {return new MoveInfoMan(this, entry);}

  using base::GetMoveId;
  virtual MoveId GetMoveId(MoveLayer layer, RStringB name ) const;

  using base::FindActionIndex;
  using base::GetMove;

  ActionMap::Index FindActionIndex(ManAction action) const {return _actionIndices[action];}

  MoveId GetDefaultMove( int upDegree ) const
  {
    const MoveIdExt &id = GetMove(upDegree, CAST_TO_ENUM(ManAction, ManActDefault));
    Assert(id.layer==MovePrimary);
    return id.id;
  }
  const MoveIdExt &GetMove(int upDegree, ManAction action) const {return base::GetMove(upDegree, _actionIndices[action]);}
  const MoveIdExt &GetMove(const ActionMap *map, ManAction action) const;

  MovesTypeGestureMan *GetGestures() const {return _gestures;}
};


/// shared implementation of BankTraits for MovesTypeMan and MovesTypeGestureMan
template <class MovesTypeX>
struct BankTraitsMan: public DefLinkBankTraits<MovesTypeX>
{
  // default name is character
  typedef const MovesTypeName &NameType;
  // default name comparison
  static int CompareNames( NameType n1, NameType n2 )
  {
    return strcmpi(n1.skeletonName,n2.skeletonName);
  }
};

/// shared implementation of BankTraitsExt for MovesTypeMan and MovesTypeGestureMan
template <class BankTraitsX>
struct BankTraitsManExt: public DefBankTraitsExt<BankTraitsX>
{
  typedef typename BankTraitsX::Type Type;
  typedef typename BankTraitsX::NameType NameType;
  
  /// create an object based on the name
  static Type *Create(NameType name)
  {
    Type *instance = new Type();
    instance->Load(name);
    return instance;
  }
};

template <>
struct BankTraits<MovesTypeMan>: public BankTraitsMan<MovesTypeMan> {};

template <>
struct BankTraitsExt< BankTraits<MovesTypeMan> >: public BankTraitsManExt< BankTraits<MovesTypeMan> > {};

template <>
struct BankTraits<MovesTypeGestureMan>: public BankTraitsMan<MovesTypeGestureMan> {};

template <>
struct BankTraitsExt< BankTraits<MovesTypeGestureMan> >: public BankTraitsManExt< BankTraits<MovesTypeGestureMan> > {};

/// manager of primary human motion
extern BankArray<MovesTypeMan> MovesTypesMan;
/// manager of human gestures
extern BankArray<MovesTypeGestureMan> MovesTypesGestureMan;

#include "wounds.hpp"

/// Define one variant of breath
struct ProbSound
{
  AutoArray<SoundProbab> _pars;
};
TypeIsMovableZeroed(ProbSound);

/// Define one group of breath
struct BreathSound
{
  AutoArray<ProbSound> _group;
};
TypeIsMovableZeroed(BreathSound);

/// Used for damage and hit sounds
struct SoundDamage: public SoundProbab 
{
  float _min;
  float _mid;
  float _max;
};

TypeIsMovableZeroed(SoundDamage);

/// Named group of sounds
struct SoundDamageEx
{
  RString _name;
  AutoArray<SoundDamage> _pars;
};

TypeIsMovableZeroed(SoundDamageEx);

class SoundHitDamage
{
  ProbSound _hitSounds;
  AutoArray<SoundDamageEx> _damageSounds;

public:
  void Load(ParamEntryPar entry);  
  const SoundPars* SelectHitSound(float probab) const;
  const SoundDamage* SelectDamageSound(float probab, const RString &name) const;
};

TypeIsMovableZeroed(SoundHitDamage);

class ManVisualState : public EntityVisualState
{
  typedef EntityVisualState base;
  friend class Man;
  friend class Soldier;

public:
  MovesVisualState _moves;
  MovesVisualState _gestures;
#if _ENABLE_PHYSICS
	AutoArray<Matrix4> _ragdollMx;
	AutoArray<SkeletonIndex> _ragdollIdx;
#endif
  float _gestureFactor;

  //@{ this is very often queried, but quite costly to calculate
  Vector3 _cameraPositionWorld; ///< world space head (camera) position
  Vector3 _aimingPositionWorld; ///< world space aiming position
  //Vector3 _headPositionWorld; ///< world space head position
  //@}

  Matrix4 _gunTrans; ///< gun aiming transformation - aiming using hands + weapon only
  Matrix4 _headTrans; ///< head movement transformation
  Matrix4 _lookTrans; ///< look transformation
  Matrix4 _gunBodyTrans; ///< gun aiming transformation - aiming using body
  Matrix4 _leaningTrans; ///< leaning transformation
  Matrix4 _legTrans; ///< legs slope adjustment transformation
  Matrix4 _shakingTrans;

  bool _gunTransIdent,_headTransIdent,_leaningTransIdent; ///< optimize matrix combination
  bool _shakingTransIdent;

  // camera in cockpit
  float _headXShiftWantedCont,_headYShiftWantedCont,_headZShiftWantedCont; //head position shift in cockpit
  float _headZRotWanted, _headZRotWantedCont; //head tilt left/right
  float _headXShiftWanted, _headYShiftWanted, _headZShiftWanted;

  bool _gettingInOut;

  void Interpolate(Man *man, const ManVisualState & t1state, float t, ManVisualState& interpolatedResult) const;
public:
  ManVisualState(const ManType & type);

  virtual void Copy(ObjectVisualState const& src) {*this=static_cast_checked<const ManVisualState &>(src);}
  virtual ManVisualState *Clone() const {return new ManVisualState(*this);}
  
  virtual void Interpolate(Object *obj, const ObjectVisualState & t1state, float t, ObjectVisualState& interpolatedResult) const;

  Vector3 HeadShift() const;
  float HeadRoll() const;
};


/// shared information about each soldier/man type
class ManType: public EntityAIType
{
  typedef EntityAIType base;
  friend class Man;
  friend class Soldier;
  friend class LightAI; //because members are protected
  friend class GrimaceEyes;

protected:

  /// some properties have no sense for entities implemented as "man", but being not a man (like animals)
  bool _isMan; 

  //! Trigger pulling animation
  /*!
    There are 2 states inside and together it covers three states:
    - not aiming, just default animation is being used, finger off the trigger
    - aiming, first frame is being used, finger on the trigger
    - firing, second frame is being used, finger pulling the trigger
  */
  Ref<AnimationRT> _pullTrigger;

  /// skeleton bone index of the head
  SkeletonIndex _headBone;
  /// skeleton bone index of the head - variant for cut-scenes
  SkeletonIndex _headBoneCutScene;

#if _ENABLE_NEWHEAD
  //!{ Eye bones
  SkeletonIndex _lEye;
  SkeletonIndex _rEye;
  //!}
  //!{ Eyelid bones
  SkeletonIndex _lEyelidUp;
  SkeletonIndex _rEyelidUp;
  SkeletonIndex _lEyelidDown;
  SkeletonIndex _rEyelidDown;
  //!}
  //!{ Pupils
  SkeletonIndex _lPupil;
  SkeletonIndex _rPupil;
  //!}
#endif

  int _stepLIndex,_stepRIndex; // index in memory LOD
  int _commonDamagePoint;      // index in hitpoints LOD

  float _minGunElev,_maxGunElev; // gun movement
  float _minGunTurn,_maxGunTurn;
  float _minGunTurnAI,_maxGunTurnAI;

  float _maxTurnAngularVelocity;
  float _costTurnCoef;
  float _minHeadTurnAI,_maxHeadTurnAI;
  
  float _minTriedrElev,_maxTriedrElev; // gun movement
  float _minTriedrTurn,_maxTriedrTurn;

  RandomSound _hitSound;
  SoundPars _addSound;
  AutoArray<SoundHitDamage> _damageHitSound; // variants of sounds
  AutoArray<BreathSound> _breathSound;
  RandomSound _vegSound; // sounds played when occure collision with vegetation

  bool _canHideBodies;
  bool _canDeactivateMines;

  /// there could be glasses shown for this model (disabled when glasses can collide with some other attachment)
  bool _glassesEnabled;

  bool _useInternalLODInVehicles;

  /// some units (Arma2 women) cannot crawl
  /** this needs to be reflected in path finding */
  bool _canCrawl;
  //! which name to use
  bool _woman;
  /// names of common units of given type (language, gender)
  RString _genericNames;
  //! model face type
  RString _faceType;
  /// List of enabled identities
  FindArrayRStringCI _identityTypes;
  
  float _sideStepSpeed;
  int _insideView;

  float _lyingLimitSpeedHiding;
  float _crouchProbabilityHiding;
  float _lyingLimitSpeedCombat;
  float _crouchProbabilityCombat;
  float _lyingLimitSpeedStealth;

  int _handsIndex; // index of corresponding proxy

  int _pilotPoint;
  int _aimingAxisPoint;
  int _leaningAxisPoint;
  int _headAxisPoint;

  int _aimPoint;
  /// index of a memory point used for grenade throwing
  int _handGrenadePoint;

  // used for the bow
  int _leftHandPoint;
  int _rightHandPoint;

  //! Point the external camera is looking at
  int _camTgt;

  Ref<MovesTypeMan> _moveType;
  
  Ref<MovesTypeGestureMan> _gestureType;
  

#if _ENABLE_HAND_IK
  /// bone for the item in hand
  SkeletonIndex _weaponBoneIndx;
  IKHandAnimation _leftArmIK;  ///< IK engine for arms... 
  IKHandAnimation _rightArmIK;
  IKLegsAnimation _leftLegIK;  ///< IK engine for legs... 
  IKLegsAnimation _rightLegIK;
#endif  

  Mapping _bottomGeomComp; // indexes of the bottom geomComp
  Mapping _vertexCollMapping;  // mapping from collision geometry vertex oder to pattern vertex order CollisionVertexPattern
  Mapping _geomCompCollMapping;// mapping from geom components oder to pattern order CollisionGeomCompPattern 

  //! Section that covers faces in rough LODs and hands - used to replace the texture with custom one
  AnimationSection _personality;

  //! Original texture - used in wounds animation
  Ref<Texture> _faceTextureOrig;

  AnimationSection _headHide; // hiding in vehicle interior

  WoundTextureSelections _headWound;
  WoundTextureSelections _bodyWound;
  WoundTextureSelections _lArmWound;
  WoundTextureSelections _rArmWound;
  WoundTextureSelections _lLegWound;
  WoundTextureSelections _rLegWound;

  int _headHit;
  int _bodyHit,_handsHit;
  int _legsHit;

  // parts of body drawn when no inventory item is in the slot
  Ref<InventoryItemType> _emptySlots[NInventorySlotId];

#if !_ENABLE_NEWHEAD
  HeadTypeOld _head;
#endif

  //!{ Bone names
#if _ENABLE_NEWHEAD
  RString _boneHead;
  RString _boneHeadCutScene;
  RString _boneLEye;
  RString _boneREye;
  RString _boneLEyelidUp;
  RString _boneREyelidUp;
  RString _boneLEyelidDown;
  RString _boneREyelidDown;
  RString _boneLPupil;
  RString _boneRPupil;
#endif
  //!}

  AutoArray<PPEffectType> _ppEffects;

#if _ENABLE_CONVERSATION
  /// conversation topics shared for all units of the same type
  Ref<const KBCenter> _kbCenter;
#endif

public:
#if _VBS3
  bool _viewPortCentered;
#endif
  float _poseAudibility[VPCount];
  float _poseVisibility[VPCount];

  ManType( ParamEntryPar param );
  virtual void Load(ParamEntryPar par, const char *shape);
  void LoadDamageHitSounds(ParamEntryPar base, const char *name);
  void LoadBreathSounds(ParamEntryPar base, const char *name);
  void LoadBreathVarinat(ParamEntryPar groupEntry, BreathSound &bSound);
  const SoundPars &GetBreathSound(int breathSelector, float selector) const;
  void InitShape();
  void DeinitShape();

  virtual Object* CreateObject(bool fullCreate) const;
  virtual EntityVisualState* CreateVisualState() const { return new ManVisualState(*this); }

  bool AbstractOnly() const {return false;}
  virtual bool IsPerson() const {return true;}

  #if _DEBUG
  void OnUnused();
  void OnUsed();
  #endif
  
  void InitShapeLevel(int level);
  void DeinitShapeLevel(int level);

  //const WeightInfo &GetWeights() const {return *_weightType;}
  AnimationRT *GetAnimation( MoveId move, int subIx ) const {return _moveType->GetAnimation(move, subIx);}
  ActionMap *GetActionMap( MoveId move, int subIx ) const;
  const MoveIdExt &GetMove(const ActionMap *map, ManAction act) const {return _moveType->GetMove(map, act);}
  const MoveInfoMan *GetMoveInfo( MoveId move, int subIx ) const {return _moveType->GetMoveInfo(move, subIx);}
  const MoveInfoGestureMan *GetMoveInfoGesture( MoveId move ) const {return _gestureType->GetMoveInfo(move);}
  const Mapping& GetBottomGeomComp() const {return _bottomGeomComp;}
  SkeletonIndex GetHeadBone(bool cutSceneMode=false) const {return cutSceneMode ? _headBoneCutScene : _headBone;}

  bool IsAnimal() const {return !_isMan;}

  virtual float GetMaxRemoteAngleCorrection() const;

  virtual bool ScanTargets() const {return _isMan;}
  virtual bool ScannedAsTarget() const {return _isMan;}

  float GetFieldCost(const GeographyInfo &info, CombatMode mode) const;
  float GetBaseCost(const GeographyInfo &geogr, bool operative, bool includeGradient) const;
  float GetGradientPenalty(float gradient) const;
  float GetCostModeFactor(CombatMode mode) const;
  
  /// check if given position is suitable for ejecting
  static bool CheckEject(Vector3Par pos, float radius);
  float GetCostTurn(int difDir, int &debet, bool forceFullTurn) const;
  FieldCost GetTypeCost(OperItemType type, CombatMode mode, Clearance clearance) const;
  // maximal angular velocity for turning in rad/sec
  float GetMaxTurnAngularVelocity() const {return _maxTurnAngularVelocity;}

  const Mapping& GetVertexCollMapping() const {return _vertexCollMapping;};
  const Mapping& GetGeomCompCollMapping() const {return _geomCompCollMapping;};

  static FSMEntityConditionFunc *CreateEntityConditionFunc(RString name);
  static FSMEntityActionFunc *CreateEntityActionInFunc(RString name);
  static FSMEntityActionFunc *CreateEntityActionOutFunc(RString name);
  virtual MapNameToConditionFunc *GetCreateEntityConditionFunc() const {return &CreateEntityConditionFunc;}
  virtual MapNameToActionFunc *GetCreateEntityActionInFunc() const {return &CreateEntityActionInFunc;}
  virtual MapNameToActionFunc *GetCreateEntityActionOutFunc() const {return &CreateEntityActionOutFunc;}

#if _ENABLE_CONVERSATION
  const KBCenter *GetKBCenter() const {return _kbCenter;}
#endif

  protected:
  /// Function finds mapping between this ManType collision geometry and pattern collision order   
  void FindCollisionMapping();
};

#include "smokes.hpp"

// transition is considered as animation state, transition factor is 
struct ManAnimState
{
  MoveId anim; // which animation
  float time; // time (0..1) - may be looped or clamped depending on anim
  //float transTime; // transition time (transition speed given by anim)
  //float transSpeed; // positive -> fade in, negative -> fade out
};

/// man stance indication
/**
When adding a new one, add also into ManPosEnum
*/
enum ManPos
{
  ManPosDead,
  ManPosWeapon, // special weapon - AT
  ManPosBinocLying,
  ManPosLyingNoWeapon,
  ManPosLying,
  ManPosHandGunLying,
  ManPosCrouch,
  ManPosHandGunCrouch,
  ManPosCombat,
  ManPosHandGunStand,
  ManPosStand, // moves with weapon on the back
  ManPosSwimming,
  ManPosNoWeapon, // civilian moves
  ManPosBinoc, // binocular position
  ManPosBinocStand, // binocular position (weapon on back)

  ManPosNormalMin = ManPosLyingNoWeapon,
  ManPosNormalMax = ManPosNoWeapon
};

#if _VBS2 // fatigue model
// constants for the aiming and weapon sway system as a function of the soldier's
// breathing and hence fatigue levels
#define VERTICAL_AIM_RANGE     0.01 // maximum vertical variation in sight's aim
#define HORIZONTAL_AIM_RANGE   0.04 // maximum horizontal variation in sight's aim
#define AIM_WANDER_SCALE	   1.5  // Scaling factor for the wander in the breath path as a function of weapon mass. Larger values mean more wander.
                                    // Value of zero means perfect figure-8 regardless of weapon mass.
#define MIN_BREATH_RATE        8.0  // minimum (resting) rate of breaths per minute.
#define MAX_BREATH_RATE        50.0 // maximum (full fatigue/exertion) rate of breaths per minute.
#define AIM_RANGE_SCALE_FACTOR 0.5  // scaling of horizontal and vertical variation in proportion to max breath rate
#define STANDING_AIM_SCALE     1.0  // scale factor for how much sight sways when standing
#define KNEELING_AIM_SCALE     0.5  // scale factor for how much sight sways when kneeling
#define LYING_AIM_SCALE        0.2  // scale factor for how much sight sways when lying
#define AIM_NOISE_RESET_CHANCE 0.1  // Chance, per second that the "noise" added to breath wander of site will be reset.
#define AIM_NOISE_RESET_MULTIPLIER 0.25  // When aim noise is reset, it is multiplied by this value. Set to 0 if a full reset required.
#define AIM_LIGHT_WEAPON    0.95   // Weight (in kg) of a weapon system considered light in terms of adding noise to its sway
#define AIM_HEAVY_WEAPON    14.2   // Weight (in kg) of a weapon system considered heavy in terms of adding noise to its sway  
// constants pertaining to the fatigue and exertion model.
enum BreathState
{
  BreathNormal,
  BreathSteady,
  BreathShakey,
  BreathGasp
};

enum FatiguePos
{
  PosStanding,
  PosKneeling,
  PosLying,
  PosSwimming
};

enum WeaponMass
{
  WeaponMassNormal,
  WeaponMassHeavy
};

#define NUM_ANAEROBIC_VALUES     10     // Anaerobic window of 10 values
#define ANAEROBIC_INTERVAL       1.0    // Each value accumulated on a 1 second basis --> window of 10 seconds
#define MAX_ANAEROBIC_ENERGY     70.0   // Amount of energy expenditure in 10 seconds that equates to maximum anaerobic activity

#define NUM_AEROBIC_VALUES       12     // Aerobic window of 12 values
#define AEROBIC_INTERVAL         5.0    // Each value accumulated on a 5 second basis --> window of 1-minute
#define MAX_AEROBIC_ENERGY       300.0  // Amount of energy expenditure in 1 minute that equates to maximum aerobic activity

#define MAX_DIST_1_TICK			10.0	// The maximum distance we can expect a soldier to travel between frames...even if 1 per second.
										// This takes care of soldiers hopping out of vehicles also.

// Posture change "energy" costs
#define STANDING2KNEELING        3.0
#define STANDING2LYING           5.0
#define KNEELING2LYING           3.0
#define KNEELING2STANDING        5.0
#define LYING2KNEELING           5.0
#define LYING2STANDING           10.0

#define UPRIGHT_MOVE_COST        1.0    // Walking/running energy scale factor for movement.
#define KNEELING_MOVE_COST       1.5    // Moving while kneeling is 1.5 as expensive as walking. A low cost because movement shows only start
                                        // and end kneeling.
#define CRAWLING_MOVE_COST       7.0    // Crawling is 7 times as expensive (in energy) as walking.
#define SWIMMING_MOVE_COST       4.0    // Swimming is 4 times as expensive (in energy) as walking.
#define ENERGY_COST_PER_KG       0.015  // Each kg carried increases energy cost of movement by 1.5%

#define NUM_BREATH_VALUES        60
#define BREATH_INTERVAL          1.0

#define MAX_TIME_TO_NEXT_HOLD    3      // Time soldier must wait before holding breath again
#define MAX_BREATH_HOLD_DURATION 10.0   // Maximum time a soldier can hold their breath (for purposes of firing)

#define BREATH_HOLD_ENERGY_COST 15.0 
#define SUPPRESSION_STRESS_COST 2.0	// The cost to fatigue of being stressed. Equivalent to 2 seconds at full anaerobic output.
#endif

typedef bool (MoveInfoMan::*TestEnable)() const;
typedef bool (MoveInfoManCommon::*TestGEnable)() const;
typedef float (MoveInfoMan::*TestValue)() const;
typedef float (MoveInfoMan::*TestValueTimed)(float time) const;
typedef float MoveInfoMan::*TestVar;

//! entity event enum defined using enum factory
#define COVER_STATE_ENUM(type,prefix,XX) \
  XX(type, prefix, No) /*not covering */ \
  XX(type, prefix, MovingInDirect) /* close, moving into the cover by maneuvring*/ \
  XX(type, prefix, InCover) /* in the cover*/ \
  XX(type, prefix, MovingOut) /* moving out of the cover*/ \

#ifndef DECL_ENUM_COVER_STATE
#define DECL_ENUM_COVER_STATE
DECL_ENUM(CoverState)
#endif
DECLARE_ENUM(CoverState,Cover,COVER_STATE_ENUM)

/// suitable cover height for a corner cover (prone, crouch, stand)
extern const float CoverHeightCorner[];

/// suitable cover height for an edge cover (prone, crouch)
extern const float CoverHeightEdge[];

class Building;

//! Man class - simulation of all men
/*!
  Man implements all functionality specific to men.
  Ideally all military related functions should be in Soldier,
  but it is not implemented that way and most such functions
  are also implemented in Man.
  \todo Either merge Man and Soldier or move military functionality to Soldier.
*/

typedef RefArray<Object> ObjectArray;

class RaiseWeaponActions 
{
public:
  bool  _toggleON;
  bool  _weaponHoldActive;  // weapon was raised last time using UATempRaiseWeapon
  DWORD _startTime, _raiseWeaponTimeout;

  RaiseWeaponActions()  { Init(); }
  void Init() { _weaponHoldActive=_toggleON=false; _startTime=0; _raiseWeaponTimeout=1000; }
  // returns true when RaiseWeapon is active, calls multiple GetAction
  bool CheckAndUpdate(InputInterface *input);
};

#if _ENABLE_PHYSICS
namespace enf
{
  class BulletRagdollController;
}
#endif

class Man: public Person
{
  typedef Person base;
protected:
	float _gunYRot,_gunXRot; ///< actual gun direction  
  float _gunYRotWanted,_gunXRotWanted; ///< wanted gun direction, auto-aim included
  float _gunXRotWantedNoTrembleHands, _gunYRotWantedNoTrembleHands; ///< wanted gun direction, hand tremble not included
  float _gunYRotIronSightBackup;
  float _gunXSpeed,_gunYSpeed;
  float _gunXRotDelay, _gunYRotDelay;

  //@{ remember to apply recoil during weapon simulation
  /// x/y recoil is angular velocity
  float _gunXRecoil,_gunYRecoil;
  /// z recoil is offset
  float _gunZRecoil;
  //@}
  
  /** Pan: discrete (keyboard, hat-switch), Cont: continuous (TrackIR) */
  float _headYRot,_headYRotWanted,_headYRotWantedCont,_headYRotWantedPan,_headYRotWantedAuto; //head: heading
  float _headXRot,_headXRotWanted,_headXRotWantedCont,_headXRotWantedPan,_headXRotWantedAuto; //head: dive
  bool  _headSmoothMove;
  float _lookYRot, _lookYRotWanted; //lookAround: heading
  float _lookXRot, _lookXRotWanted; //lookAround: dive

  float ComputeWeaponDynamics(float origin, float deltaT, float actValue, float moveDelta, float dynamicFactor, float dampingFactor, float minDampingSpeed);
protected:
#if _ENABLE_PHYSICS
public:
  enf::BulletRagdollController* _ragdollCtrl;
protected:
  unsigned int _ragdollMode;
#endif
  Object::Zoom _zoom;
  bool _zoomOutToggled;
  bool _zoomInToggled;
  /// viewPars backup. It only saves what viewPars were used (useful in vehicle)
  const ViewPars *_lookAroundViewPars;

  float _leanZRot; //! Actual leaning rotation
  float _leanZRotWanted; //! Wanted leaning rotation
  float _hitSkeletonRot;
  float _hitSkeletonRotWanted;
  bool _headHitOnly;
  Vector3 _hitSkeletonRotAxis;

  // periodically change between looking forward and looking at the target
  float _lookForwardTimeLeft;
  float _lookTargetTimeLeft;

  RaiseWeaponActions _raiseWeaponActions;

  void DiagCover(CoverVars *cover, RString text, ColorVal coverColor, ColorVal textColor);

  /// current relation of the soldier to the selected cover
  CoverState _coverState;

  /// describe cover which is currently in use
  Ref<CoverVars> _cover;
  
  /// sometimes we may be stopped in an "open cover"
  bool _inOpenCover;
  /// when a path leads into a cover, once the cover is entered, this flag is set for it
  signed char _coverEntered;

  /// FSM can instruct us to enter the cover
  int _leaveCoverSemaphore;
  
  Matrix4 _legTransBackUp; /**< Backup of _legTrans. Used in Simulate() function.                             
                            @see #_legTrans
                            @see #Simulate(float , SimulationImportance )
                            @see #CreatePotenctialState(Frame& , float& , float , SimulationImportance ) 
                            @see #UseOldState()
                            */

  Vector3 _sumaShakingVector;
  float _manScale; // TODO: use _scale of Object
    
  float _hideBody, _hideBodyWanted; ///< hide body state
  float _hideBodyBackUp; /**< Backup of _hideBody. Used in Simulate() function. 
                         @see #_hideBody
                         @see #Simulate(float , SimulationImportance )
                         @see #CreatePotenctialState(Frame& , float& , float , SimulationImportance ) 
                         @see #UseOldState()
                         */

  // parts of body drawn when no inventory item is in the slot
  Ref<InventoryItem> _emptySlots[NInventorySlotId];

  //! Function to return pulling state of the trigger (0 - no pulling, 1 - full pulling)
  virtual float GetTriggerPullingState() const {return 0.0f;}

  void RecalcGunTransform(); ///< calculate _gunTrans from gunXRot ...
  void RecalcLeaningTransform(); ///< calculate _leaningTrans from some leaning coefficient
  void RecalcAffectSkeletonTransform(); ///<calculate _affectSkeletonTrans
  void RecalcShakeTransform(Vector3Par v,float deltaT);

  Ref<WeaponType> _mGunFireWeapon;

  RefAbstractWave _soundStep;
  RefAbstractWave _soundBreath;
  RefAbstractWave _soundEnv;
  RefAbstractWave _soundEquip;
  RefAbstractWave _soundMunitPrimary;
  RefAbstractWave _soundMunitSecondary;
  RefAbstractWave _soundDamage;
  RefAbstractWave _vegSound;
  Time _damageTimeToLive;
  Time _corpseTime; ///< When we become a corpse (used for garbage collection of dead bodies)

  int _breathSelect;
  int _damageHitSelect;

  OLinkPermOArray(Entity) _pipeBombs;

  /// AI is allowed to touch of the bombs (used when AI planted them on its own)
  bool _pipeBombsAuto;

  /// sound steps triggered regularly by the animations
  bool _doSoundStep;
  bool _slideSoundRunning;
  bool _canMoveFast;
  char _doMark; // 1 = right mark, 2 = left mark (footsteps)

  bool _nvg;      ///< using Night Vision Goggles
  bool _hasNVG; ///< have Night Vision Goggles
  bool _flir;   // using Flir Thermal Vision
  bool _wasNVGActive; // remember nvg state
  bool _wasNVGStateSet; // nvg state was saved
  bool _useVisionModes; // avoid drawing nvg optics with gun optics contemporary
  bool _walkToggle; ///< Manual control - walk wanted
  bool _isWalking; ///< the state depends on UAWalkRunToggle. False for slow (running)
  bool _walkRunTempLocked; ///< after UAWalkRunToggle, the UAWalkRunTemp is locked, until action released
  bool _inBuilding; ///< AI control - last path planned in building
  Time _vegCollisionTime; // time of last vegetation collision
  bool _walkForced; ///< walk forced by script
  float _customRecoilCoefficient; /// scripted recoil coef.

  bool _leanLeftLocked; ///< after UALeanLeftToggle, the UALeanLeft is locked, until key released
  bool _leanLeftToggle; 
  bool _leanRightLocked; 
  bool _leanRightToggle;
  
  /// on some surface we want to stand even when sloped - like staircases
  bool _forceStand;
  /// current slope of the surface in the forward and aside direction
  float _landGradFwd, _landGradAside;
  float _slideSpeed;
  
  /// how much is the soldier holding breath right now?
  float _holdBreath;

  /// tired - breathing sound and max. speed are controlled by this
  float _tired;
  /// how much air has the unit available for holding breath
  float _breathAir;

  //! If the object is alive, then the _metabolismFactor is 1, if it dies, the value goes slowly to 0 (it lasts several hours to reach 0)
  float _metabolismFactor;

  RStringB _soundStepOverride;
  
  bool _freeFall; ///< true -- if man is in the free-fall mode
  Time _freeFallUntil; ///< time, until which man must be in free-fall mode.
  Time _freeFallStartTime; ///< TIME_MIN when free fall is not in progress

  bool _swimming; ///< true -- if man is under water enough in order swimming is preferable

  float _aimInaccuracyX,_aimInaccuracyY;
  Time _lastInaccuracyTime;
  float _aimInaccuracyDist;
  Time _lastInaccuracyDistTime;

  float _waterDepth; ///< how deep under water is man's landcontact
  float _waterDepthBackUp; /**< Backup of _waterDepth. Used in Simulate() function. 
                           @see #_waterDepth
                           @see #Simulate(float , SimulationImportance )
                           @see #CreatePotenctialState(Frame& , float& , float , SimulationImportance ) 
                           @see #UseOldState()
                           */


  Time _whenKilled; ///! helpers for suspending dead body after a while
  Time _lastMovementTime;
  Time _lastObjectContactTime;
  Time _whenScreamed;
  
  /// hit display time
  Time _whenHit;
  // hit display intensity
  float _howMuchHit;
  /// hit display direction (world space)
  Vector3 _hitDirection;
  
  //float _launchDelay;
  MoveId _stillMoveQueueEnd;
  Time _variantTime; ///< when we should switch variant

  /// use audio or game time as animation timer?
  /** used for audio/animation synchronization */
  bool _useAudioTimeForMoves;
  /// what audio time was already processed
  /**
  As audio time is no passed into the simulation, we need to derive deltas on our own
  */
  UITime _audioTimeProcessed;
  
  /// member controlling motion flow (primary movement channel)
  Moves _moves;
  /// member controlling motion flow (additional channel - gestures)
  Moves _gesture;

  /// target logical move for this queue, received from a remote object
  MoveId _targetMoveWanted;
  /// target logical move for this queue, not considering _forceMove
  MoveId _targetMove;


  //@{ Backup of current state, Used in Simulate() function.
  /**
  @see #_primaryMove  MotionPathItem _primaryMoveBackUp;
  @see #Simulate(float , SimulationImportance )                                    
  @see #CreatePotenctialState(Frame& , float& , float , SimulationImportance )                                     
  @see #ValidatePotentialStateThis()                                    
  */                                    
                                    
  MotionPathItem _primaryMoveBackUp;
  MotionPathItem _secondaryMoveBackUp;
  float _primaryFactorBackUp;
  float _primaryTimeBackUp;
  float _primaryTimeDBackUp;
  float _secondaryTimeBackUp;
  float _secondaryTimeDBackUp;
  bool _finishedBackUp;
  //@}

  float _landDX; ///< Dy/Dx of land (or roadway). Used in ValidatePotencialState, (valid only if _landcontact is true).
  float _landDZ; ///< Dy/Dz of land (or roadway). Used in ValidatePotencialState, (valid only if _landcontact is true).
  bool _landContactBackUp, _waterContactBackUp;
  Vector3 _landContactPos; ///< actual position of the landcontact in world coord.
  bool _objectContactNonStatic;
  bool _walkingOnObject;
  bool _purePersonContact; /**< true if in last called ValidatePotentialState all contact was with person.
                           @see #ValidatePotentialState(const Frame& , bool , float , Vector3& , bool , ObjectArray& );  
                           */
  //Any of these (_collideWith, _walkingOnObject,_objectContact) won't tell me that I'm walking inside building. Adding another one...
  Ref<Object> _objectLandContact;
  bool _lowPriorityContact;
  bool _highPriorityContact;
  bool _highPriorityContactEnd;
  bool _ignoreLowPriorituContact;

  RString _surfaceSoundBackUp;  /**< Backup of _surfaceSound. Used in Simulate() function. 
                                @see #_surfaceSound
                                @see #Simulate(float , SimulationImportance )
                                @see #CreatePotenctialState(Frame& , float& , float , SimulationImportance ) 
                                @see #UseOldState()
                                */ 
  int _bodyPosition;

  Vector3 _speedBackUp; /**< Backup of _speed. Used in Simulate() function. 
                        @see #_speed
                        @see #Simulate(float , SimulationImportance )
                        @see #CreatePotenctialState(Frame& , float& , float , SimulationImportance ) 
                        @see #UseOldState()
                        */
  Vector3 _angMomentumBackUp; /**< Backup of _angMomentum. Used in Simulate() function. 
                              @see #_angMomentum
                              @see #Simulate(float , SimulationImportance )
                              @see #CreatePotenctialState(Frame& , float& , float , SimulationImportance ) 
                              @see #UseOldState()
                              */

  Vector3 _lastRemotePos; /**< Last received position of remote player. Used in Simulate() function.
                          @see #Simulate(float , SimulationImportance )
                          */

#define INFINITY_VOLUME 1000000.0f    
  float _collVolume; /**< last collision volume. If collision volume grows between the frames, dammage is applied.
                     @see #Simulate(float , SimulationImportance )
                     @see #DammageOnCollVol(float)
                     */

  Time _staticCollisionUntil; ///< time, until which man must check collisions with static objects.

  bool _staticCollision;  ///< true man must check collisions with static objects.
  bool _staticCollisionBackUp; /**< Backup of _angMomentum. Used in Simulate() function. 
                               @see #_angMomentum
                               @see #Simulate(float , SimulationImportance )
                               @see #CreatePotenctialState(Frame& , float& , float , SimulationImportance ) 
                               @see #UseOldState()
                               */ 
  /** time, since which man is colliding with _collideWith
  @see #_collideWith
  */
  Time _collideSince;
  /// object currently colliding with man.
  OLink(Object) _collideWith;


  bool _showPrimaryWeapon;
  bool _showSecondaryWeapon;
  bool _showHead;
  
  //@{ variables used while climbing ladder
  OLinkO(Building) _ladderBuilding;
  int _ladderIndex;
  float _ladderPosition; ///< 0..1 - position on ladder, 0 = bottom
  float _ladderPositionLast;
  int _ladderAIDir; // 
  //@}

  int GetActUpDegree(const ManVisualState &vs) const {return _moves.GetActUpDegree(vs._moves, GetMovesType());} // helper function

  void SetPosWanted(ManPos pos);

  int _upDegreeStable; ///< stable upDegree (must last at least 10 sec)

  ManPos _posWanted; ///< delayed position change
  Time _posWantedTime; 
  Time _forceBinocUntil;

  const bool _safeSystem; ///< yes - collision system with save capsules will be used
  bool _useSafeCapsule;  ///< yes - use save capsule, if it exist for the current move. 
  bool _useSafeCapsuleBackUp; /**< Backup of _useSafeCapsule. Used in Simulate() function. 
                              @see #_useSafeCapsule
                              @see #Simulate(float , SimulationImportance )
                              @see #CreatePotenctialState(Frame& , float& , float , SimulationImportance ) 
                              @see #UseOldState()
                              */ 
  bool _autoActionProcessed; ///< auto action on _collideWith processed
  Time _autoActionTime; ///< time when auto action on _collideWith can be processed
  UIActionType _autoActionType; ///< action type of planned auto action

  bool ChangeMoveQueue(MotionPathItem item);
  bool ChangeMoveQueueSetStillEnd(MotionPathItem item);

  bool SetMoveQueue(MotionPathItem item, bool enableVariants);
  void RefreshMoveQueue(bool enableVariants); // select variants
  bool SimulateForcedMove(SimulationImportance prec);


  void GetRelSpeedRange( float &speedX, float &speedZ, float &minSpd, float &maxSpd );

  /// control the movement speed by controlling the animation speed
  float _walkSpeedWantedZ;
  /// sometimes we need separate control for X axis (sideways movement)
  float _walkSpeedWantedX;

  /// desired angular velocity
  float _turnWanted;

  /// sometimes AI or player needs to perform a fine control over the movement
  bool _walkSpeedFineControl;
  
  /// transitional walking period for player before we start running
  Time _walkUntil;
  
  /// Sometimes after get out from vehicle man was stuck by collision into each other.
  bool _afterGetOut;  // true after get out start till first collision
  bool _ignoreCollisionIfStuck; // if man was stuck ignore collision till it reach some free position...
  Time _stuckStarted; // stuck was noticed... 

  /// is drawing bow in preparation to shoot
  bool _allowBowDraw;
  bool _drawingBow;
  bool _pullingBowString;
  float _bowStringOffset;

  mutable SurroundTracker _surround;
  // TODO: move into EntityAIFull ?
  mutable LightTracker _lit;
  
  UnitPosition _unitPosCommanded;
  UnitPosition _unitPosScripted;
  UnitPosition _unitPosFSM;

  /// set of moves ... somehow related to capsule collisions
  typedef AutoArray<int, MemAllocLocal<int,128> > MoveSets;

  typedef AutoArray<Vector3, MemAllocLocal<Vector3,64,AllocAlign16> > Subset;

#if _ENABLE_PHYSICS
	public:
  //! Create ragdoll
	enf::BulletRagdollController* CreateRagdoll( void );
  unsigned int IsRagDollActive() const {return _ragdollMode;}
  //! Remove Physics
  void RemovePhysics();
  //! Create Physics
public: //docasne pro test
  void CreatePhysics(EPhysicsLayers layer) override;
  enf::BulletRagdollController* GetRagdoll() const{ return _ragdollCtrl;}
protected:
#endif


#if _ENABLE_NEWHEAD
  Ref<HeadType> _headType;
  Ref<GlassesType> _glassesType;
#endif

#if _ENABLE_NEWHEAD
  Head _head;
#else
  HeadOld _head;
#endif

  //! Material that is used on the Man's face and hands
  Ref<TexMaterial> _faceMaterial;
  //! Texture that is used on the Man's face and hands
  Ref<Texture> _faceTexture;
  //! Wound textures that are used on the Man's face and hands
  Ref<Texture> _faceTextureWounded[2];

  //@{ pass data from simulation to rendering
  /// depth of field focus distance
  float _renderDepthOfFieldDistance;
  //@}

  AutoArray<int> _ppeHndls;
 

  /// sound from the radio replicated to be audible in 3D
  Ref<AbstractWave> _speakingSound3D;

  RadioChannel _directSpeaking;
  // TODO: merge _speakingSound3D and _directSpeakingSound
  Link<AbstractWave> _directSpeakingSound;

#if _VBS3 //change speed for grenade throwing based on stance
  float GetAmmoInitSpeedFactor(const MagazineType *aInfo) const;
#endif

  // flash light staff
  Ref<LightReflector> _light;
  Time _lUpdateTime;
  Time _lSimTime;
  bool _lightWanted;
  bool _lightForced;  // force AI switch on gun light
  float _attenuationWanted;
  float _attenuation; 
  // ir stuff
  bool _irWanted;
  bool _irEnabled;

  /// was AddAttached called for the soldier?
  bool _isAttached;
  // grass cover - indicate drawing offset in -Y axis, used for "fake" distance hiding
  float _coverGrassHeight; 
  float _coverGrassHeightWanted;

  /// start time of fire action (used for grenades)
  Time _fireStartTime;
  bool _fireEnabled;

  FindArray< InitPtr<ProjectileDecalOnVehicle> > _attachedDecals;

public:
  Man(const EntityAIType *name, bool fullCreate=true);
  ~Man();

  const ManType *Type() const
  {
    return static_cast<const ManType *>(GetType());
  }
#if _ENABLE_NEWHEAD
  const HeadType *GetHeadType() const
  {
    return _headType;
  }
#endif


  /// @{ Visual state
  ManVisualState typedef VisualState;

  VisualState& FutureVisualState() { return static_cast<VisualState &>(base::FutureVisualState()); }
  VisualState const& FutureVisualState() const { return static_cast_checked<VisualState const &>(base::FutureVisualState());}
  VisualState const& CurrentVisualState() const { return static_cast_checked<VisualState const &>(base::CurrentVisualState());}
  const VisualState& RenderVisualState() const{ return static_cast<const VisualState &>(base::RenderVisualState()); }

  ProtectedVisualState<const VisualState> RenderVisualStateScope(bool ignoreParent=false) const {return ProtectedVisualState<const VisualState>(base::RenderVisualStateScope(ignoreParent));}
  ProtectedVisualState<VisualState> FutureVisualStateScope(bool ignoreParent=false) { return ProtectedVisualState<VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  ProtectedVisualState<const VisualState> FutureVisualStateScope(bool ignoreParent=false) const { return ProtectedVisualState<const VisualState>(base::FutureVisualStateScope(ignoreParent)); }
  /// @}

  //! Virtual method
  virtual DefTIMode GetDefTIMode() const {return TIMan;}
  virtual float GetMetabolismFactor() const {return _metabolismFactor;}
  const MovesTypeMan *GetMovesType() const {return Type()->_moveType;}
  const MovesTypeBase *GetGesturesType() const {return Type()->_gestureType;}

  bool AreGlassesEnabled() const {return Type()->_glassesEnabled;}
  
#if _ENABLE_HAND_IK // hand IK 
  void HandIK(const VisualState &vs, LODShape * lshape, int level, StaticArrayAuto<Matrix4>& matrix);
  void HandIKPilot(const ManVisualState &vs, LODShape * lshape, int level, StaticArrayAuto<Matrix4>& matrix, 
    SkeletonIndex leftHandBone, SkeletonIndex rightHandBone, SkeletonIndex leftLegBone,  SkeletonIndex rightLegBone,
    const Matrix4 &leftHandIK, const Matrix4 &rightHandIK, const Matrix4 &leftLegIK, const Matrix4 &rightLegIK);
#endif

#if _ENABLE_PHYSICS
	enf::BulletRagdollController* GetRagdollCtrl(){return _ragdollCtrl;};
#endif

  float GetCombatHeight() const {return 0;}
  int GetFaceAnimation() const {return _head.GetFaceAnimation();}
  void SetFaceAnimation(int phase) {_head.SetFaceAnimation(phase);}
  void SetFace(RString name, RString player = "");
  void SetGlasses(RString name);
  void SetMimic(RStringB name);

  virtual bool IsAnimal() const {return !Type()->_isMan;}

  virtual bool IsWoman() const {return Type()->_woman;}
  virtual RString GetGenericNames() const {return Type()->_genericNames;}
  virtual RString GetFaceType() const {return Type()->_faceType;}

  /// Check whether given identity type is in the list of enabled types
  virtual bool IsIdentityEnabled(RString identityType) const;

  PositionRender GetPositionRender();

  bool IsWalking() {return _isWalking;}

  void SetUnitRecoilCoefficient(float coefficient) { _customRecoilCoefficient = coefficient; };
  float GetUnitRecoilCoefficient() {return _customRecoilCoefficient; };

#if _ENABLE_NEWHEAD
  void SetHead(RString name);
  void SetHead(RString faceType, RString name);
  void RemoveHead();
  void RemoveGlasses();
#endif
  virtual Vector3 GetHUDOffset() const;

  bool AttachWaveReady(const char *wave);
  void AttachWave(AbstractWave *wave, float freq = 1.0f);
  void SetRandomLip(bool set = true); 
  float GetSpeaking() const;

  void HideBody() {_hideBodyWanted = 1;}
  void ScanEquipment(); ///< Check if we have night vision or other equipment (set _hasNVG flag)

  void NextVisionMode();
  void ApplyVisionMode();

#if _VBS3  
  bool IsWearingNVG() {return _nvg;} // isWearingNVG command
  int GetActUp()const {return GetActUpDegree();}
  MoveId GetCurrentMoveId() const {return _moves.GetPrimaryMove().id;}
  bool SetPublicMoveQueue(MotionPathItem item, bool enableVariants) {return SetMoveQueue(item,enableVariants);}
  VisionMode GetVisionMode() const;
  int GetTiModus() const;
  const ViewPars* GetViewPars(bool includeTurrets = true) const;
#endif
  const VehPosRelatives *GetVehPosRelatives() const;

#if _ENABLE_NEWHEAD
  const Head& GetHead() const {return _head;}
#else
  const HeadOld& GetHead() const {return _head;}
#endif

  virtual InventoryItem *ToDrawInSlot(InventorySlotId slot) override;
  virtual HeadObject *CreateHead() const override;

  /// How much the man is overloaded
  float GetEncumbrance() const;

  /// hide body compensation for network
  Vector3 HideOffset() const {return Vector3(0,-_hideBody,0);}

  virtual Vector3 GetCommonDamagePoint();

  void MinimalWeapons();

  virtual void OnWeaponAdded();
  virtual void OnWeaponRemoved();
  virtual void OnWeaponChanged();
  virtual void OnWeaponSelected();
  virtual void OnDanger();
  virtual bool CanReloadCurrentWeapon();

  virtual bool Throw(Magazine *item) override;

  void AddProjectileDecal(ProjectileDecalOnVehicle *decal);
  void RemoveProjectileDecal(ProjectileDecalOnVehicle *decal);
  int RemoveProjectileDecals(int startIdx = 0, int count = 999);

  void Init( Matrix4Par pos, bool init );

  RString DiagText() const;
  /// get movement stance degree
  int GetAutoUpDegree() const;
  /// get move id based on unit combat mode
  MoveId GetDefaultMove() const;
  /// get move id based on unit combat mode
  MoveId GetDefaultMove(ManAction action) const;
  /// get move id based on actual move
  const MoveIdExt &GetMove(ManAction action) const;
  /// get move id based on actual move
  const MoveIdExt &GetMove(RString action) const;

  Vector3 GetActionMoveShift(RString action) const;  
  RString GetCurrentMove() const;
  /// get a canonical form of the current move
  RString GetCurrentMoveEq() const;

  void SetTargetMove(MoveId id) {_targetMove = id;}

  float GetCursorInaccuracy() const;
  virtual float GetCursorSize(Person *person) const;
  virtual const CursorTextureInfo *GetCursorTexture(Person *person);
  virtual const CursorTextureInfo *GetCursorAimTexture(Person *person);

  virtual bool GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const override;
  virtual void PerformAction(const Action *action, AIBrain *unit) override;
  virtual void GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit) override;

  virtual bool IsWeaponLowered() {return false;}

  void TouchOffBombs();
  virtual bool HasPendingBomb() const;

  bool CanCancelAction() const;
  bool IsActionHighlighted(UIActionType type, EntityAI *target) const;

  bool Supply(EntityAIFull *vehicle, const Action *action);

  float GetWeaponCursorScale() const override;
  void ProcessFire(InputInterface *input);
  bool ProcessDrawBow(InputInterface *input);
  void SetDrawingBow(bool drawing) { _drawingBow = drawing; }
  bool IsDrawingBow() { return _drawingBow; }
  // the hand is holding the bow string
  float IsPullingBowString() { return _pullingBowString; }
  void SetPullingBowString(bool active) { _pullingBowString = active; }
  // distance between hands while drawing a bow
  float GetBowStringOffset() { return _bowStringOffset; }
  void SetBowStringOffset(float value) { _bowStringOffset = value; }
  // update the bow state
  void UpdateBow(float deltaT, SimulationImportance prec);
  void OnBowStateChange(BowState newState);

  virtual void RemovePipeBomb(Entity *pipeBomb) override {_pipeBombs.DeleteKey(pipeBomb);}
  virtual bool HasPipeBomb(const Entity *pipeBomb) const override {return _pipeBombs.FindKey(pipeBomb) >= 0;}
  virtual bool CanDeactivatePipeBombs() const override {return Type()->_canDeactivateMines && Type()->IsEngineer();}
  virtual bool CanDeactivateMines() const override {return Type()->_canDeactivateMines;}

  virtual void TakeItemToHands(InventoryItem *item) override;
  virtual void RemoveItemInHands() override;
#if _ENABLE_PHYSICS
  void RemoveItemInHandsThrow();
#endif

  void ProcessItemToHands();

  void ThrowAction(Magazine *item);
  void PutAction(Magazine *item);
  void TakeInHandsAction(InventoryItem *item);
  void PauseMove(MoveId moveId);
  void ContinueMove();
  void ResetItemInHands();
  void RemoveItemFromSlots(InventoryItem* item);
  void ThrowItemAction(InventoryItem *item, float power);
  void PlayForcedMove(const MoveIdExt &item, ActionContextBase *context);

  /// called when soldier has finished given move (with action connected)
  void ProcessUIAction(const Action *action);

  /// called when soldier has finished given move (with move function connected)
  void ProcessMoveFunction( ActionContextBase *context );

  void PreciseGetInAction(const RString action, const RString actionAfter, int doorIndex);
  void PreciseGetOutAction(const RString action);


  /// called while soldier is performing given move (with move function connected)
  void ProcessMoveFunctionProgress( ActionContextBase *context, float progress );

  /// search for special item of given type in the inventory
  virtual const WeaponType *GetSpecialItem(WeaponSimulation simulation) const;

  virtual bool IsNVReady() const;
  virtual bool IsNVEnabled() const;
  virtual bool IsNVWanted() const;
  virtual void SetNVWanted(bool set = true);
  virtual bool IsFlirWanted() const;
  virtual bool IsGunLightEnabled() const { return _lightWanted; }
  virtual float VisibleLights() const { return _lightWanted ? 1.0f : 0.0; }
  bool HasTiOptics() const;

  int GetProxyComplexity(int level, const Matrix4 &pos, float dist2) const;
  Object *GetProxy(
    VisualStateAge age, LODShape *&shape, Object *&parentObject,
    int level, Matrix4 &transform, const Matrix4 &parentPos, int i
  ) const;
  bool PreloadProxies(int level, float dist2, bool preloadTextures = false) const;
  bool PreloadInsideView() const;
  virtual float DensityRatio() const;

  bool NeedsPrepareProxiesForDrawing() const;
  void PrepareProxiesForDrawing(
    Object *rootObj, int rootLevel, const AnimationContext &animContext,
    const PositionRender &transform, int level, ClipFlags clipFlags, SortObject *so
  );

  virtual Vector3 AnimateProxyPoint(const Object *proxy, Vector3Par modelPos) override;

  bool CheckAndUpdateRaiseWeapon(InputInterface *input) { return _raiseWeaponActions.CheckAndUpdate(input); }
  void RaiseWeapon() 
  {
    if (!_raiseWeaponActions._weaponHoldActive)
    {
      _raiseWeaponActions._startTime = ::GetTickCount();
      _raiseWeaponActions._raiseWeaponTimeout = 14000; //14 seconds
    }
  }
  void ResetRaiseWeapon() 
  { 
    if (_raiseWeaponActions._startTime) RaiseWeapon();
  }

  //! Get possible animation subclass index
  int AnimSubIx() const { return _moves.AnimSubIx(); }
  void UpdateAnimSub();

private:
  struct ProxyInfo;
  /// helper for getting replaceable proxy info (weapons etc..)
  void GetProxyInfo(ProxyInfo &info, int level, int index) const
  {
    const ProxyObjectTyped &proxy = Type()->GetProxyForLevel(level, index);
    GetProxyInfo(info, proxy, false);
  }
  // set createUniqueObject to be sure info.object will not be shared between more proxies
  void GetProxyInfo(ProxyInfo &info, const ProxyObjectTyped &proxy, bool createUniqueObject) const;

  void ReleaseProxyInfo(ProxyInfo &info, bool rejected) const;
public:
  void DrawPeripheralVision(Person *person, CameraType camType);
  void DrawNVOptics();

#if _VBS3
  //! Virtual method
  virtual float GetTunnelVisionCoef() const {return _tunnelVisionSmooth;}
#endif

  void DrawCameraCockpit();
  
  virtual float ForceVerticalFlag() const;

  int PassNum( int lod );

  //! Determine whether volume shadows should be casted in pass3
  virtual bool CastPass3VolShadow(int level, bool zSpace, float &castShadowDiff, float &castShadowAmb) const;

  virtual void Draw(
    int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
    const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
    ) override;

  virtual void PrepareInsideRendering();
  
protected:
  /// rendering including 3D/2D transition effects
  void DoDraw(
    int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
    const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
  );

  /// rendering of man inside a vehicle
  void DrawAsProxy(
    int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
    const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
    );
  /// basic 3D model rendering
  void DrawBase(
    int cb, int level, ClipFlags clipFlags, const DrawParameters &dp,
    const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
  );
  
  /// check whether is legal to draw own model (and proxies) when using optics, also return the transition coef between 3D and 2D optics
  bool CanDrawModel(float &transition) const;

  // check if we should use the blur effect (depending on transition factor)
  bool DrawBlur(float transition) const;

  /// check focus distance for purpose of depth of field effect
  float GetFocusDistance(float transition);
  
  void IRLaserWanted(bool enable = false);
  bool HasWeaponIR(const WeaponModePtr &ptr) const;

  // flash light
  void CreateFlashLight(FlashLigthInfo *flInfo);
  void EnableFlashLight(bool enableFlashLight) { _lightWanted = enableFlashLight; }
  void DisableFlashLight();
  bool HasWeaponFlashLight(const WeaponModePtr &ptr) const;
  bool HasReflectorAttached() const;
  void SimulateFlashLight(float deltaT, Vector3Val begPos, Vector3Val dir);
  /// called whenever joined object status may need to be updated
  void UpdateJoinedObject();

public:
  #if ALPHA_SPLIT
  void DrawAlpha( int level, ClipFlags clipFlags, const FrameBase &pos );
  #endif
  AnimationStyle IsAnimated( int level ) const; // appearence changed with Animate
  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
  void Deanimate(int level, bool setEngineStuff);
  bool PrepareShapeDrawMatrices(Matrix4Array &matrices, const ObjectVisualState &vs, LODShape *shape, int level);
#if _ENABLE_PHYSICS
	void PushRagdollMx(Matrix4Array& matrices, const Shape *shapeL, LODShape *shape);
	//void SetBoneMatrix(Matrix4 mat, const ObjectVisualState &vs, SkeletonIndex boneIndex, RString name);
#endif

  /// gather all animation to blend
  void GetBlendAnims(const ManVisualState &vs, BlendAnims &blend) const;
  
  /// gather all animation to blend - neutral pose
  void GetBlendAnimsNeutral(const ManVisualState &vs, BlendAnims &blend) const;

	virtual float GetCollisionRadius(const ObjectVisualState &vs) const;
  virtual float ClippingInfo(const ObjectVisualState &vs, Vector3 *minMax, ClippingType clip) const;
  virtual float BoundingInfoUI (Vector3 &bCenter, Vector3 *minMax) const;
  
  void AnimatedBSphere(
    int level, Vector3 &bCenter, float &bRadius, Matrix4Par pos, float posMaxScale
  ) const;

  void BasicAnimation(AnimationContext &animContext, int level, bool setEngineStuff, float dist2);
  void BasicDeanimation(int level, bool setEngineStuff );

  void WoundsAnimation(AnimationContext &animContext, int level, float dist2);
  void WoundsDeanimation( int level );

  void LandSlope(bool &forceStand, float &gradFwd, float &gradAside) const;
  bool IsAbleToProne() const;
  bool IsAbleToGetOver() const;

  bool IsAbleToStand() const;
  void BreakLegs();
  bool IsAbleToFire(const TurretContext &context) const;

  float GetHandsHit() const {return GetHit(Type()->_handsHit);}

  virtual bool PreloadMuzzleFlash(const WeaponsState &weapons, const WeaponModePtr &ptr) const override;
  virtual bool PreloadView(Person *person, CameraType camType) const;
  virtual bool GetForceOptics(const Person *person, CameraType camType) const;
  virtual LODShape *GetOpticsModel(const Person *person) const;
	virtual bool GetEnableOptics(ObjectVisualState const& vs, const Person *person) const; 
  virtual bool HasOpticsTransitionFx(Person *person, CameraType camTypeNew, CameraType camTypeOld) const;
  virtual bool HasCameraTransitionFx(Person *person, CameraType camTypeNew, CameraType camTypeOld) const;
  virtual float CameraTransitionFxSpeed(Person *person, CameraType camTypeNew, CameraType camTypeOld) const;
  virtual void OnCameraTransition(Person *person, CameraType camTypeNew, CameraType camTypeOld, float newFactor) const;

  bool UseInternalLODInVehicles()	const;

  void ShowHead(bool show = true);
  void ShowWeapons(bool showPrimary = true, bool showSecondary = true);
  bool GetGunMatrix(const ManVisualState &vs, const TurretContext &context, const WeaponModePtr &ptr, Matrix4 &matrix) const;
  bool GetPalmMatrix(const ManVisualState &vs, Matrix4 &matrix) const;

protected:
  //!helper function to all Switch... motion related functions
  void SwitchMove(const MoveIdExt &actId, ActionContextBase *context, bool callFuncNow=true, const char *action=NULL);
  //!helper function to all Play... motion related functions
  bool PlayMove(const MoveIdExt &actId, ActionContextBase *context, bool callFuncNow=true, const char *action=NULL);
  //!helper function to all PlayNow... motion related functions
  bool PlayMoveNow(const MoveIdExt &actId, ActionContextBase *context, bool callFuncNow=true, const char *action=NULL);

  WeaponModePtr GetCurrentHandWeapon() const;

public:
  // implementation of functions from EntityAI interface
  virtual bool PlayMove(RStringB move, ActionContextBase *context=NULL);
  virtual bool PlayMoveNow(RStringB move, ActionContextBase *context=NULL);
  virtual void SwitchMove(RStringB move, ActionContextBase *context=NULL);

  //virtual void SwitchMove( RStringB move, ActionContextBase *context);

  //@{ implementation of EntityAI - motion
  virtual bool IsAction(RStringB action) const;
  virtual bool PlayAction(ManAction action, ActionContextBase *context=NULL, bool callFuncNow=true);
  virtual bool PlayAction(RString action, ActionContextBase *context=NULL, bool callFuncNow=true);
  virtual bool PlayActionNow(ManAction action, ActionContextBase *context=NULL, bool callFuncNow=true);
  virtual bool PlayActionNow(RString action, ActionContextBase *context=NULL, bool callFuncNow=true);
  virtual void SwitchAction(ManAction action, ActionContextBase *context=NULL);
  virtual void SwitchAction(RString action, ActionContextBase *context=NULL, bool callFuncNow=true);
  virtual float GetMajorAnimationTime() const;
  virtual void UseAudioTimeForMoves(bool toggle);
  //@}

  ActionContextBase *CreatePlayActionContext(RString action);
  ActionContextBase *CreatePlayActionContext(RString action, const GameDataCode &handler);
  ActionContextBase *CreatePlayMoveContext(RString move);
  ActionContextBase *CreatePlayMoveContext(RString move, const GameDataCode &handler);

  virtual bool PlayActionGlobal(RString action) override;
  virtual bool PlayActionGlobal(RString move, const GameDataCode &handler) override;
  virtual bool PlayActionNowGlobal(RString action) override;
  virtual bool PlayMoveGlobal(RStringB move) override;
  virtual bool PlayMoveGlobal(RStringB move,  const GameDataCode &handler) override;
  virtual bool PlayMoveNowGlobal(RStringB move) override;
  virtual void SwitchMoveGlobal(RStringB move) override;
  virtual void SwitchActionGlobal(RString action) override;

  virtual void PlayerAction(RString action) override;

  virtual void PlayerAction(RString action, const GameDataCode &handler) override;

  //@{ implementation of Person


  virtual void CreateActivityGetIn(Transport *veh, UIActionType pos, Turret *turret = NULL, int cargoIndex = -1);
  virtual void FreeFall(float seconds = 1.0f);

  // React to Get
  virtual void OnGetInFinished();
  virtual void OnGetOutFinished();

  virtual RadioChannel *GetRadio() {return &_directSpeaking;}
  virtual const RadioChannel *GetRadio() const {return &_directSpeaking;}
  virtual void SetDirectSpeaking(AbstractWave *wave) {_directSpeakingSound = wave;}
  virtual void SetSpeaking3D(AbstractWave *wave) {_speakingSound3D = wave;}
  //@}

  //@{ implement EntityAIFull - using cover
  virtual void StopPilot(AIBrain *unit, SteerInfo &info);

  virtual bool FindCover(CoverVars &coverVars, const OperCover &cover, Vector3Par tgtPos);
  virtual bool IsOccupyingCover(const CoverInfo &info) const;
  
  bool StartUsingCover(CoverVars *cover, CoverState state);
  void StopUsingCover();

  void ForceWalk(bool force) {_walkForced = force;};
  bool IsForcedWalk() {return _walkForced;};

  virtual bool CheckMovementNeeded() const;
  virtual bool PlanChangePossible() const;
  virtual Vector3 PositionToPlanFrom() const;

  virtual bool OnPathFoundToCover();

  virtual void LeaveCover();
  virtual CoverStyle IsInCover() const;
  virtual float GoingToCover() const;
  virtual bool CheckCoverEntered();
  virtual bool CheckCoverLeft();
  virtual bool CheckSupressiveFireRequired() const;
  virtual CombatMode GetCombatModeAutoDetected(const AIBrain *brain) const;

  void ReportMoving();
  void ReportCoverLeft();
  void SetCoverEntered() {_coverEntered=1;}
  
  /// check cover entered status, no reset
  bool CheckCoverEnteredPending() const {return _coverEntered>0;}
  //@}

  /// if we are close enough to cover, perform micro-movement
  void TrySwitchingToCover(CoverVars *cover);

  /// perform find manoeuvring to given spot while in cover
  void ManeuverInCover(CoverVars *cover, Vector3Par pos, Vector3Par aimDir, SteerInfo &info) const;
  /// set orientation based on cover and desired aiming direction
  float AlignInCover(float &posHeadingWanted, CoverVars *cover, Vector3Par aimDir, float &aligned) const;

  void MoveInCover(CoverVars *cover, Vector3Par aimDir, SteerInfo &info);


  // UIAction helper - activity implementation
  virtual ActionContextBase *CreateActionContextUIAction(const Action *action, ManAction moveAction) override;
  // Gear action helper - activity implementation
  ActionContextBase *CreateActionContextGear(const Action *action);
  

  virtual void PutDownEnd();

  bool CheckActionProcessing(UIActionType action, AIBrain *unit) const;
  void StartActionProcessing(Action *action, AIBrain *unit);

  // some EntityAI interface implementations
  bool IsActionInProgress(MoveFinishF action) const;
  const ActionContextBase *GetActionInProgress() const;
  bool IsLadderActionInProgress() const;
  bool EnableWeaponManipulation() const;
  bool EnableViewThroughOptics() const;

  template <class Type>
  void ForEachAction(Type &func) const
  {
    _moves.ForEachAction(func,FutureVisualState()._moves);
  }

  virtual bool DoReloadMagazine(WeaponsState &weapons, const WeaponModePtr &ptr) override;

  // more functions
  void ApplyAnimation( int level, RStringB move, float time );
  void ApplyDeanimation( int level );
  float GetAnimSpeed(RStringB move);

  Vector3 GetPilotPosition(CameraType camType) const;


  UnitPosition GetUnitPosition() const;
  void SetUnitPositionCommanded(UnitPosition status);
  void SetUnitPositionScripted(UnitPosition status);
  void SetUnitPositionFSM(UnitPosition status);

  void Destroy( EntityAI *owner, float overkill, float minExp, float maxExp );
  void HitBy(EntityAI *owner, float howMuch, RString ammo, bool wasDestroyed);
  void Scream(EntityAI *killer);
  void ShowDamage(int part, EntityAI *killer);

  /// Do dammage to the object.
  virtual void DoDamage(DoDamageResult &result, Vector3Par pos, float val, float valRange, RString ammo, Vector3Par originDir, bool ignoreSmall);

  void KilledBy( EntityAI *owner );
  void SetTotalDamageHandleDead(float damage, bool setHitpoints) override;
  void ReactToDamage();
  virtual void DirectHit(const HitInfo& hitInfo, const SkinnedPosition& skinning) override;

  //Time GetDestroyedTime() const;

  virtual Vector3 PredictVelocity() const;

  float NeedsAmbulance() const;
  float NeedsRepair() const;
  float NeedsRefuel() const;

  void ResetLauncher();
  const AmmoType *GetCurrentAmmoType() const;
  bool LauncherReady() const;
  /// missile launcher is selected
  bool LauncherSelected() const;
  /// missile launcher animation is needed
  bool LauncherAnimSelected() const;

  bool LauncherWanted() const;
  /// current weapon is a binocular
  bool BinocularSelected() const;
  /// current weapon used an animation like binocular
  bool BinocularAnimSelected() const;

  void Simulate( float deltaT, SimulationImportance prec );
  virtual void SimulateSimple(float deltaT, SimulationImportance prec);

  virtual bool RemoteStateSupported() const {return true;}

  float CheckMaxLean(float leanZRot, float xOffset,float zOffset, bool isAimbody = false);
  void BasicSimulationCore( float deltaT, SimulationImportance prec ); // also inside vehicle
  /// used when inside of the vehicle
  void BasicSimulation( float deltaT, SimulationImportance prec, float speedFactor );

  void Move(Matrix4Par transform);
  void Move(Vector3Par position);
  float GetLegPhase() const;
  float GetPrimaryMoveTimeLeft() const;
  RStringB GetPrimaryMoveName() const;

  bool SimulateAnimations
  (
    float &turn, float &moveX, float &moveZ, float deltaT,
    SimulationImportance prec
  );    

  virtual void CalcAttachedPosition(float deltaT);

  void ProcessMovesProgress(Moves &moves, const MovesVisualState &mvs);
  
  virtual void PlaceOnSurface(Matrix4 &trans);
  virtual Vector3 PlacingPoint() const;

  bool VerifyStructure() const;

  // perform actual catch/drop ladder
  void SimLadderMovement(float deltaT, SimulationImportance prec);

  void CatchLadder(Building *obj, int ladder, bool up);
  void DropLadder(Building *obj, int ladder);
  bool IsOnLadder(Building *obj, int ladder) const;
  bool IsOnAnyLadder() const;

  /// used both from vehicle and from direct soldier
  void UpdateSpeechPosition(bool inside, Vector3Par pos, Vector3Par speed, float deltaT);
  
  virtual void SoundInTransport(Transport *parent, Matrix4Par worldPos, float deltaT);
  virtual bool SoundInTransportNeeded() const;

  virtual const SoundPars& SelectBulletSoundPars(const TurretContext &context, const WeaponModePtr &ptr) const override;
  void Sound( bool inside, float deltaT );
  void UnloadSound();

  float Rigid() const;
  /// by default let deflection be controlled only by ammo
  float MaxDeflectionAngleCos() const;
  bool HasGeometry() const;
  //bool OcclusionFire() const {return true;} // occlusion for fire 
  bool OcclusionView() const {return false;}

  void RecalcPositions(const Frame &pos);
  void MoveWeapons(float deltaT, bool forceRecalcMatrix);
  void OnPositionChanged();

  virtual float FireAngleInRange(const TurretContext &context, const WeaponModePtr &ptr, Vector3Par rel) const override;

  virtual void FireAimingLimits(float &minAngle, float &maxAngle, const TurretContext &context, const WeaponModePtr &ptr) const override;

  /// head movement - crew (used for BasicSimulation)
  bool MoveHeadCrew(float deltaT);
  /// head movement
  bool MoveHead(float deltaT, float speedFactor);
  /// Looking around
  virtual void ProcessLookAround(float headingWanted, float diveWanted, const ViewPars *viewPars, float deltaT, bool forceLookAround=false, bool forceDiveAround=false) override;
  virtual void UpdateHeadLookAround(InputInterface *input, float deltaT, bool forceLookAround) override;
  virtual void ProcessLookAroundAuto(Vector3Par dir) override;


  virtual Zoom* GetZoom() {return &_zoom;};
  virtual void ToggleZoomIn() {_zoomInToggled = !_zoomInToggled;} //UAZoomInToggle helper method
  virtual bool IsZoomInToggled() {return _zoomInToggled;} //UAZoomInToggle status
  virtual void ResetZoomInToggle() {_zoomInToggled = false;}
  virtual void ToggleZoomOut() {_zoomOutToggled = !_zoomOutToggled;} //UAZoomOutToggle helper method
  virtual bool IsZoomOutToggled() {return _zoomOutToggled;} //UAZoomOutToggle status
  virtual void ResetZoomOutToggle() {_zoomOutToggled = false;}
  virtual float GetCameraFOV() const;
  virtual void UpdateHeadLookWanted(float heading, float dive);
  //@{ Person implementation

  virtual void GetLookWanted(float &x, float &y);
  virtual Vector3 GetCrewHeadDirection() const;
  virtual Vector3 GetCrewHeadDirectionWanted() const;
  virtual Vector3 GetCrewLookDirection(ObjectVisualState const &vs) const;
  virtual void ResetXRotWanted() {_lookXRotWanted = _headXRotWanted = 0.0f;}
  virtual void AimHeadCrew(Vector3Par dir);
  //@}
  void AimHead(Vector3Par direction);
  void AimHeadForward();
  void LimitHeadRot(float &xRotWanted, float &yRotWanted) const;
  void LimitLookRot(float &xRotWanted, float &yRotWanted) const;
  void LimitRot(float &xRotWanted, float &yRotWanted, bool head) const;
  void UpdateLeaning(InputInterface *input);

  void AimHeadAI(Vector3Par direction, Target *target, float deltaT);

  void AimWeaponAI(const TurretContext &context, const WeaponModePtr &ptr, Vector3Par direction, Target *target, float deltaT );
  void AimWeaponAI(const TurretContext &context, const WeaponModePtr &ptr, Target *target, float deltaT );
  virtual bool AimWeaponForceFire(const TurretContextEx &context, const WeaponModePtr &ptr) override;

  /// avoid aiming under the ground
  Vector3 ValidateAimDir(Vector3Par dir) const;

  virtual bool CalculateAimWeaponPos(const TurretContext &context, const WeaponModePtr &ptr, Vector3 &pos, float &timeToLead, Target *target, bool exact) const override;

  //! aim weapon
  virtual bool AimWeaponAdjusted(const TurretContextEx &context, const WeaponModePtr &ptr, Vector3Par direction) override;

  virtual bool AimWeapon(const TurretContextEx &context, const WeaponModePtr &ptr, Vector3Par direction) override;
  virtual bool AimWeaponTgt(const TurretContextEx &context, const WeaponModePtr &ptr, Target *target) override;

  void SimulateHUD(CameraType camType, float deltaT);
  void AimHeadAndWeapon(Vector3Par direction );
  virtual void AimWeaponManDir(const TurretContextEx &context, const WeaponModePtr &ptr, Vector3Par direction) override;
  virtual bool IsAutoAimEnabled(const TurretContext &context, const WeaponModePtr &ptr) const override;

  // Adjust the weapon elevation depending on fov
  // This function is used for manual units to adjust weapon depending on current field of view.
  // Example of such weapon is M21 sniper rifle.
  void AdjustWeapon(const WeaponModePtr &ptr, float fov, Vector3 &camDir);

#if _VBS3
  void ShowTrajectory();
  void DrawLaserPointer(VisionMode vm, int lightMode = -1);
#endif

  int GetWeaponIndex(const WeaponModePtr &ptr) const;

  /// helper for GetWeaponRelDirection and GetAimCursorDirection
  /**
  - when muzzleDir is given, the real direction of muzzle is returned
  - otherwise, the direction of camera in optics is used
  */
  Vector3 GetWeaponRelDirectionDetectGrenades( const ManVisualState &vs, const WeaponModePtr &ptr, Vector3Par grenadeDir, bool muzzleDir ) const;
  Vector3 GetWeaponRelDirection( const ManVisualState &vs, const WeaponModePtr &ptr, bool muzzleDir ) const;
  Matrix4 GetWeaponRelTransform(const ManVisualState &vs, const WeaponModePtr &ptr) const;
  
  Vector3 GetHeadCenterMoves(VisualState const& vs) const;
  Vector3 GetLeaningCenterMoves(VisualState const& vs) const;
  Vector3 GetWeaponCenterMoves(VisualState const& vs) const;

  virtual Vector3 GetWeaponDirection(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const override;
  virtual Vector3 GetWeaponDirectionWanted(const TurretContextEx &context, const WeaponModePtr &ptr) const override;
  
  virtual Vector3 GetAimCursorDirection( const TurretContext &context, const WeaponModePtr &ptr) const override;
  virtual Vector3 GetWeaponCenter(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const override;
  

  virtual Vector3 GetEyeDirection(ObjectVisualState const& vs, const TurretContext &context) const;
  virtual Vector3 GetEyeDirectionWanted(ObjectVisualState const& vs, const TurretContextEx &context) const;
  Vector3 GetEyeDirectionWantedNoAddCont(ObjectVisualState const& vs, const TurretContextV &context) const;
  Vector3 GetLookAroundDirection(ObjectVisualState const& vs, const TurretContext &context) const;
  
  void CleanUp();

  virtual Vector3 GetWeaponPoint(ObjectVisualState const& vs, const TurretContext &context, const WeaponModePtr &ptr) const override;
#if _VBS3
  Vector3 GetLaserPoint(const TurretContext &context, int weapon) const;
#endif
  virtual bool GetWeaponCartridgePos(const TurretContext &context, const WeaponModePtr &ptr, Matrix4 &pos, Vector3 &vel) const override;

  virtual float GetAimed(const TurretContext &context, const WeaponModePtr &ptr, Target *target, bool exact = false, bool checkLockDelay = false) const override;

  bool GetWeaponAim(TurretContext context, Vector3 &position, Vector3 &direction);

  // check current state of motion
  bool IsDead(const VisualState &vs) const;
  bool IsDown(const VisualState &vs) const;
  bool IsDown() const {return IsDown(FutureVisualState());}
  bool IsCrouch(const VisualState &vs) const;
  bool IsCrouch() const {return IsCrouch(FutureVisualState());}
  bool IsDead() const {return IsDead(FutureVisualState());}
  bool IsLaunchDown() const;
  float CorpseAge();

  bool IsBinocularInMove() const;
  bool IsHandGunInMove() const;
  bool IsPrimaryWeaponInMove() const;
  bool IsWeaponInMove() const;
  bool IsSwimmingInMove() const;
  bool IsInCollision() const { return _collideWith != NULL; }
  Object* WalkingOnObject() const { return _objectLandContact; }

  //! Flag to determine a handgun cannot be used at the moment (for example during running)
  bool DisableWeapons() const;
  bool EnableMissile(VisualState const& vs) const;

  void FireAttemptWhenNotPossible();

  bool EnableTest(const VisualState &vs, TestEnable func) const;

  /// similar to EnableTest, considers _gesture as well (boolean or)
  bool EnableTestWithGestureOr(TestGEnable func) const;
  
  /// similar to EnableTest, considers _gesture as well (boolean and)
  bool EnableTestWithGestureAnd(TestGEnable func) const;

  bool EnableTestMovesCommon(TestGEnable func) const;
  bool EnableTestGesturesCommon(TestGEnable func) const;


  template <class TestValueType, class ValType, class Combine>
  ValType ValueTestEx(const ManVisualState &vs, TestValueType func, ValType defValue, const Combine &combine) const
  {
    const MoveInfoMan *info=Type()->GetMoveInfo(_moves.GetPrimaryMove(vs._moves).id, AnimSubIx());
    ValType primValue = info ? (info->*func)() : defValue;
    float factor = _moves.GetPrimaryFactor(vs._moves);
    if (factor < Moves::_primaryFactor1)
    {
      const MoveInfoMan *sec=Type()->GetMoveInfo(_moves.GetSecondaryMove(vs._moves).id, AnimSubIx());
      ValType secValue = sec ? (sec->*func)() : defValue;
      primValue = combine(primValue,secValue,factor);
    }
    return primValue;
  }

  template <class TestValueType, class ValType, class Combine>
  ValType ValueTestGesturesEx(TestValueType func, ValType defValue, const Combine &combine) const
  {
    Assert(FutureVisualState()._gestureFactor>0 && Type()->_gestureType);
    const MovesVisualState &vs = FutureVisualState()._gestures;

    const MoveInfoMan *info=Type()->GetMoveInfo(_gesture.GetPrimaryMove(vs).id, AnimSubIx());
    ValType primValue = info ? (info->*func)() : defValue;
    float factor = _gesture.GetPrimaryFactor(vs);
    if (factor < Moves::_primaryFactor1)
    {
      const MoveInfoMan *sec=Type()->GetMoveInfo(_gesture.GetSecondaryMove(vs).id, AnimSubIx());
      ValType secValue = sec ? (sec->*func)() : defValue;
      primValue = combine(primValue,secValue,factor);
    }
    return primValue;
  }

  float ValueTest(const ManVisualState &vs, TestValue func, float defValue = 0) const;
  float ValueTest(const ManVisualState &vs, TestValueTimed func, float defValue = 0) const;
  float ValueTest(const ManVisualState &vs, TestVar var, float defValue = 0) const;

  #define MOVE_INFO_TEST(name) \
    bool name(const VisualState &vs) const {return EnableTest(vs,&MoveInfoMan::name);}
  #define MOVE_INFO_TEST_OR_GEST(name) \
    bool name() const {return EnableTestWithGestureOr(&MoveInfoManCommon::name);}
  #define MOVE_INFO_TEST_AND_GEST(name) \
    bool name() const {return EnableTestWithGestureAnd(&MoveInfoManCommon::name);}
  #define MOVE_INFO_VALUE(name) \
    float name(const VisualState &vs) const {return ValueTest(vs,&MoveInfoMan::name);}
  #define MOVE_INFO_VALUE_DEF(name,def) \
    float name(const VisualState &vs) const {return ValueTest(vs,&MoveInfoMan::name,def);}

  MOVE_INFO_TEST(ShowItemInHand)
  MOVE_INFO_TEST(ShowItemInRightHand)
  MOVE_INFO_TEST(ShowHandGun)
  MOVE_INFO_TEST(EnableFistStroke)
  MOVE_INFO_TEST(EnableGunStroke)
  MOVE_INFO_TEST(EnableBinocular)
  MOVE_INFO_TEST(EnableAutoActions)
  MOVE_INFO_TEST(EnableDirectControl)
  MOVE_INFO_TEST_OR_GEST(WeaponsDisabled)
  MOVE_INFO_TEST(DisableWeaponsLong)
  MOVE_INFO_TEST_AND_GEST(ShowWeaponAim)
  MOVE_INFO_TEST(ForceAim)
  //MOVE_INFO_VALUE(GetLeaningFactorAngle)
  MOVE_INFO_VALUE(GetAimPrecision)
  MOVE_INFO_VALUE(GetCamShakeFire)
  MOVE_INFO_VALUE(GetLimitGunMovement)
  MOVE_INFO_VALUE_DEF(GetVisibleSize,1)
  // more

  virtual bool EnableOptics(ObjectVisualState const& vs) const;

  typedef const BlendAnimSelections &(MoveInfoMan::*BlendAnimFunc)() const;

  const BlendAnimSelections &GetBlendAnim( const ManVisualState &vs, BlendAnimSelections &tgt, BlendAnimFunc func ) const;
  const BlendAnimSelections &GetAiming( const ManVisualState &vs, BlendAnimSelections &tgt ) const;
  const BlendAnimSelections &GetAimingBody( const ManVisualState &vs, BlendAnimSelections &tgt ) const;
  const BlendAnimSelections &GetLeaning( const ManVisualState &vs, BlendAnimSelections &tgt ) const;
  const BlendAnimSelections &GetLegs( const ManVisualState &vs, BlendAnimSelections &tgt ) const;
  const BlendAnimSelections &GetHead( const ManVisualState &vs, BlendAnimSelections &tgt ) const;

  const BlendAnimSelections *GetGestureMask(const ManVisualState &vs, BlendAnimSelections &tgt) const;
  
  //float GetLimitGunMovement() const;
  float VisibleMovement() const;
  float VisibleMovementNoSize() const;
  
  float Audible() const;
  virtual float GetHidden(float dist) const; // hidden by surroundings
  virtual float GetLit() const;

  float CollisionSize() const;
  float VisibleSize(ObjectVisualState const& vs) const;
  float AimingSize() const;
  
  Vector3 VisiblePosition(ObjectVisualState const& vs) const;

  using base::AimingPosition;
  using base::AimingPositionHeadShot;
  virtual Vector3 AimingPosition(ObjectVisualState const& vs, const AmmoType *ammo, Vector3Par aimFromDir) const override;
  virtual Vector3 AimingPositionHeadShot(ObjectVisualState const& vs, const AmmoType *ammo, Vector3Par aimFromDir) const override;
  virtual Vector3 CameraPosition() const;
  virtual Vector3 EyePosition(ObjectVisualState const& vs) const;
  virtual float DistanceFrom(const SensorPos &pos) const;
  virtual void GetSensorInfo(SensorPos &pos) const;

  Vector3 GetLocalAimingPosition() const;

  Vector3 CalculateAimingPosition(Matrix4Par pos)const;
  Vector3 CalculateCameraPosition(Matrix4Par pos)const;
  Vector3 CalculateHeadPosition(Matrix4Par pos)const;

  float GetIndirectHitFactor() const;

  /// ResetMovement implementation
  void ResetMovement(float speed, ActionMap::Index actionIndex, bool init); 
  void ResetMovement(float speed, int action = -1); 
  void ResetMovement(float speed, RString action); 

  void AnimateBoneMatrix( Matrix4 &mat, const ObjectVisualState &vs, int level, SkeletonIndex boneIndex) const;
  void BlendMatrix(Matrix4 &mat,const Matrix4 &trans,float factor) const;

  Vector3 COMPosition(ObjectVisualState const& vs) const;
  /// helper for AnimatePoint - apply other than movement animations
  Vector3 ApplyNotRTMAnimations(const ManVisualState &vs, Vector3Par src, int level, int selIndex) const;
  /// current position
  Vector3 AnimatePoint(const ObjectVisualState &vs, int level, int selIndex) const;
  virtual bool IsPointAnimated(int level, int index) const {return true;}
  /// average (neutral) position for current animations
  Vector3 AnimatePointNeutral(const ObjectVisualState &vs, int level, int selIndex ) const;
  Matrix4 InsideCamera( CameraType camType ) const;
  Matrix3 HeadBobOrientation(const VisualStateAge vsa, CameraType camType, bool setEyes) const;
  Vector3 GetCameraDirection(CameraType cam) const;
  Vector3 ExternalCameraPosition( CameraType camType ) const;
  Vector3 GetSpeakerPosition() const;

  bool HasFlares( CameraType camType ) const;

  float TrackingSpeed() const {return 150;}
  //float OutsideCameraDistance( CameraType camType ) const {return 15;}
  float OutsideCameraDistance( CameraType camType ) const {return 10;}

  float RifleInaccuracy() const; // max. rifle accuracy in current state

  bool IsVirtual( CameraType camType ) const;
  bool IsVirtualX( CameraType camType ) const;
  bool IsGunner( CameraType camType ) const;
  void OverrideCursor(Vector3 &dir) const;

  virtual float IsGrassFlatenner(Vector3Par pos, float radius, Vector3 &flattenPos) const;
  virtual float FlattenGrass(float &skewX, float &skewZ, Vector3Par pos, float radius) const;
  
  virtual bool AnimationMovesPosition() const {return true;}


  bool IsCommander( CameraType camType ) const;
  virtual bool ShowAim(const TurretContextV &context, const WeaponModePtr &ptr, CameraType camType, Person *person) const override;
  virtual bool ShowCursor(const TurretContextV &context, const WeaponModePtr &ptr, CameraType camType, Person *person) const override;

  int InsideLOD( CameraType camType ) const;
  void InitVirtual
  (
    CameraType camType, float &heading, float &dive, float &fov
  ) const;
  void LimitVirtual
  (
    CameraType camType, float &heading, float &dive, float &fov
  ) const;

  LSError Serialize(ParamArchive &ar);

  DECLARE_NETWORK_OBJECT
  static NetworkMessageFormat &CreateFormat
  (
    NetworkMessageClass cls,
    NetworkMessageFormat &format
  );
  TMError TransferMsg(NetworkMessageContext &ctx) override;
  bool RejectMsg(NetworkMessageClass cls, NetworkMessage *msg) override;
  float CalculateError(NetworkMessageContextWithError &ctx) override;

  virtual bool ChildUpdateByParent(const Entity *obj) const override;

  void UnselectLauncher();

  protected:
  /** @brief Function applies dammage on man according to collision volume. */
  void DammageOnCollVol(float fDammage);

  int HeavyWeaponsCount();

  /** @brief Function removes some weapons (heavy), as swimming with these is physically impossible. */
  void DropHeavyWeaponsWhileSwimming();
  
  /** @brief Function currently stops swimming (used when person enters the vehicle while swimming) */
  virtual void OnMovedFromLandscape() { _swimming=false; }

  /** @brief Function return true when man is swimming */
  virtual bool IsSwimming() const {return _swimming;}

  void ApplyTurn(VisualState &vs, float turn);

  /** @brief Function adjucts rotation part of  cMoveTrans according to land and turn.*/
  void SetleInPosition(ManVisualState& cMoveTrans, float turn, float deltaT,  bool changeTrans = true);

  /** @brief Function measures collison volume in moveTrans and if it is possible it tries to find better position.*/
  float CheckAndRepairPotencialState(ManVisualState& moveTrans, float turn, float simDeltaT, float fVoll = 0, bool reportContact = false);

  /** @brief Function creates new state. It moves man according to animation and setle him on land or roadway.*/
  bool CreatePotentialState(ManVisualState& cMoveTrans, float& turn, float deltaT, SimulationImportance prec);

  /** @brief Function creates new state. It moves man according to animation and setle him on land or roadway.*/
  void SetleInPositionFreeFall(ManVisualState& cMoveTrans);

  /** @brief Function measures collison volume of the man in moveTrans and if it is possible it tries to find better position.*/
  float CheckAndRepairPotencialStateFreeFall(ManVisualState &moveTrans, Vector3& cRepairMove, ObjectArray& cCollObjects, float deltaT, float fVoll = 0,bool reportContact = false);

  /** @brief Function creates new state. It advances animation and applies phys. forces on man in freefall mode.*/
  bool CreatePotentialStateFreeFall(ManVisualState& cMoveTrans, Vector3& friction, Vector3& torqueFriction, Vector3& repairMove, float deltaT, SimulationImportance prec);  

  /** @brief Function simulates man in the freefall mode.*/
  bool SimulateFreeFall(ManVisualState & moveTrans, float deltaT, SimulationImportance prec);

  /** @brief Function provides collision detection with the objects. Before test it rollback state according to backuped values.*/
  float ValidatePotentialStateThis();

  /** @brief Function provides collision detection with the objects.*/
  float ValidatePotentialState(
      const ManVisualState& moveTrans, 
      bool setUpContactInfo = false,      
      Vector3 * repairMove = NULL,
      bool reportContact = false,     
      ObjectArray * objArray = NULL);        

  /** @brief Function tests if capsule for move will colide.*/
  bool IsCollisionFreeMove(MoveId move);

  /** @brief Functions finds convex subset with minimal result vector length.*/
  void FindConvexSubSet(const CollisionBuffer& cCollisions, const MoveSets& cRepairMoveSets, 
      Vector3Par cActRepairMove, int iSet, Subset& pcActualSubSet, Vector3& cMinRepairMove, float& fMinRepairMoveSize);

  /** @brief Function rollbacks state to the initial values.*/
  void UseOldState();

  bool IsLookingAround() const;

  /** Function is called if man does not collide*/
  void OnNoCollide();

  /** Function is called if man collide with object... 
  @param withInfo - collision info about contact (if there are more contacts with one body only one is reported.*/
  void OnCollide(CollisionBuffer& collisions,  MoveSets& sets, int maxSet);

  void CancelAction();

  public:
  static void ClassCleanUp();

  USE_CASTING(base)

#if _ENABLE_CHEATS
    SDebugActionsInfo _debugLastActionsInfo;
#endif
};

//! Soldier class - simulation of all soldiers

class Soldier: public Man
{
private:
  typedef Man base;

  WeaponModePtr _lastCurrentWeapon;
  bool _freelook; // free look mode was active in last KeyboardPilot
  ///raise/lower weapon maintanance
  typedef enum {WeaponRaised, WeaponLower} WeaponRaisedState;
  WeaponRaisedState
    _weaponRaisedState,      //actual state of the gun (raised/lower)
    _weaponTempRaisedState,  //temporary state of the gun wanted
    _weaponGlobRaisedState;  //global state of the gun wanted
  bool _weaponTempRaisedActive; //true iff the UATempRaiseWeapon was active in the last frame

  //! Coefficient to determine the pulling state of the trigger (0 - no pulling)
  /*!
    Note that pulling state can be greater than 1, it will be saturated for displaying purposes
    (to wait a while during the full pulling).
  */
  float _triggerPullingCoef;
  //! Flag to determine there was some impulse to pull the trigger (it will be returned to false in the full pulling)
  bool _triggerPullingImpulse;

  bool IsAimingDeadzoneEnabled();

protected:
  //! Virtual method
  virtual float GetTriggerPullingState() const {float r = _triggerPullingCoef; saturate(r, 0.0f, 1.0f); return r;}
public:
  Soldier(const EntityAIType *name, bool fullCreate=true);
  ~Soldier();

  virtual float FireInRange(const TurretContext &context, const WeaponModePtr &ptr, float &timeToAim, const Target &target) const override;

  WeaponModePtr MissileIndex() const;

#if _ENABLE_AI
  void AIFire( float deltaT );
  void AIPilot(float deltaT, SimulationImportance prec);
  bool UseBackwardMovement(Vector3Par toMove, EntityAI *leaderVehicle) const;
  void DisabledPilot(float deltaT, SimulationImportance prec);
  bool ValidatePoseChange(ActionMap *map, ManAction action) const;
#endif

  void KeyboardPilot( float deltaT, SimulationImportance prec);
  void SuspendedPilot( float deltaT, SimulationImportance prec);
  void UpdateRaiseWeapon(InputInterface *input); //updates weapon state (raised,lower using UAToggleRaiseWeapon, UATempRaiseWeapon)

  virtual bool IsWeaponLowered() {return _weaponRaisedState==WeaponLower;}

  void FakePilot( float deltaT, SimulationImportance prec );
  virtual bool PrepareThrow(const TurretContext &context, const WeaponModePtr &ptr) override;
  virtual bool FireWeapon(const TurretContextV &context, const WeaponModePtr &ptr, TargetType *target, bool forceLock) override;
  virtual bool ProcessFireWeapon(
    const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine, EntityAI *target,
    const RemoteFireWeaponInfo *remoteInfo, RemoteFireWeaponInfo *localInfo, bool forceLock
  ) override;
  virtual bool FireWeaponEffects(const TurretContext &context, const WeaponModePtr &ptr, const Magazine *magazine,EntityAI *target,
    const RemoteFireWeaponInfo *remoteInfo) override;

  virtual void ApplyRecoil(float impulseAngX, float impulseAngY, float offsetZ);
  virtual float GetRecoilFactor() const;
  virtual float GetCamShakeFireCoef() const;
  virtual void OnAddImpulse(Vector3Par force, Vector3Par torque);
  virtual void OnStress(Vector3Par pos, float hit);
  virtual void OnNewTarget(Target * target, Time knownSince);
  virtual RStringB GetRecoilName(const WeaponModeType *mode) const;

  void SimulateAI(float deltaT, SimulationImportance prec);
  void Simulate(float deltaT, SimulationImportance prec);

  //@{ JoinedObject implementation
  virtual void UpdatePosition();
  //}

  LSError Serialize(ParamArchive &ar);

  void DrawDiags();
#if _ENABLE_PHYSICS
  void PhysDiagDraw();
#endif

  USE_CASTING(base)
};

#endif
