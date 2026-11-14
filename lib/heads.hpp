#ifdef _MSC_VER
#pragma once
#endif

#ifndef _HEADS_HPP
#define _HEADS_HPP

#if _ENABLE_NEWHEAD

#include <Es/Containers/bankInitArray.hpp>

#include "Shape/shape.hpp"
#include "paramFileExt.hpp"
#include "rtAnimation.hpp"

#ifndef DECL_ENUM_SIMULATION_IMPORTANCE
#define DECL_ENUM_SIMULATION_IMPORTANCE
DECL_ENUM(SimulationImportance)
#endif

class Head;
class HeadType;

//////////////////////////////////////////////////////////////////////////
//! Grimace ancestor
class Grimace : public RefCount
{
protected:
  //! Grimace name
  RStringB _name;
public:
  //! Constructor
  Grimace(RStringB name) {_name = name;}
  //! Grimace name
  RStringB Name() const {return _name;}
  //! Fills out the specified bone
  virtual bool GetBone(Matrix4 &m, const SkeletonIndex &si,  Head &head, const HeadType *headType, Matrix4Par headOrigin, Vector3Par eyeDirModel, float eyeAccom) const = 0;
};

//////////////////////////////////////////////////////////////////////////

class GrimaceRTM : public Grimace
{
private:
  //! Animation data
  Ref<AnimationRT> _rtm;

public:
  //! Constructor
  GrimaceRTM(RStringB name, const AnimationRTName &rtmName) : Grimace(name), _rtm(new AnimationRT(rtmName)) {}
  //! Virtual method
  virtual bool GetBone(Matrix4 &m, const SkeletonIndex &si,  Head &head, const HeadType *headType, Matrix4Par headOrigin, Vector3Par eyeDirModel, float eyeAccom) const;
};

//////////////////////////////////////////////////////////////////////////

struct GrimaceWeight
{
  float _weight;
  SkeletonIndex _bone;
};

TypeIsSimple(GrimaceWeight)

//! One grimace selection
struct GrimaceSelection
{
  //! Source grimace
  Ref<Grimace> _grimace;
  //! Selection weights (for each skeleton index)
  AutoArray<GrimaceWeight> _weights;
  /// convert a bone ID into an index into _weights
  AutoArray<int> _boneMap;
};
TypeIsMovable(GrimaceSelection)

//! Compound grimace
class GrimaceCompound : public Grimace
{
private:
  //! Source grimaces selections list
  AutoArray<GrimaceSelection> _selections;
public:
  //! Constructor
  GrimaceCompound(RStringB name, const ParamEntryVal &grimace, const Skeleton *skeleton, const RefArray<Grimace> &currentGrimaces);
  //! Virtual method
  virtual bool GetBone(Matrix4 &m, const SkeletonIndex &si,  Head &head, const HeadType *headType, Matrix4Par headOrigin, Vector3Par eyeDirModel, float eyeAccom) const;
};

//////////////////////////////////////////////////////////////////////////

//! Eye and eyelid grimace
class GrimaceEyes : public Grimace
{
private:
  // The maximum angle the eye is able to roll
  float _eyeMaxAngle;
  // The angle of the upper eyelid if the eye is open and it is in the binding position
  float _eyelidUpStartAngle;
  // The maximum angle the upper eyelid can be opened
  float _eyelidUpMaxAngle;
  // The angle of the lower eyelid if the eye is open and it is in the binding position
  float _eyelidDownStartAngle;
  // The maximum angle the lower eyelid can be opened
  float _eyelidDownMaxAngle;

public:
  //! Constructor
  GrimaceEyes(RStringB name, const ParamEntryVal &grimace, const Skeleton *skeleton);
  //! Virtual method
  virtual bool GetBone(Matrix4 &m, const SkeletonIndex &si,  Head &head, const HeadType *headType, Matrix4Par headOrigin, Vector3Par eyeDirModel, float eyeAccom) const;

private:
  void GetEyeBone(Matrix4 &m, Vector3Par eyePos, Matrix4Par headOrigin, Vector3Par eyeDirModel) const;
  void GetPupilBone(Matrix4 &m, Vector3Par eyePos, Vector3Par pupilPos, Matrix4Par headOrigin, Vector3Par eyeDirModel,const float eyeAccom) const;
  void GetLidBone(Matrix4 &m, Vector3Par eyePos, Matrix4Par headOrigin, Vector3Par eyeDirModel, const float startAngle, const float maxAngle, const float winkPhase, const int downOrUP = 1) const;
};

//////////////////////////////////////////////////////////////////////////

const int VizemNum = 8;

//! Lipsync grimace
class GrimaceLipsync : public Grimace
{
private:
  Ref<AnimationRT> _vizem[VizemNum];
public:
  //! Constructor
  GrimaceLipsync(RStringB name, const ParamEntryVal &grimace, Skeleton *skeleton);
  //! Virtual method
  virtual bool GetBone(Matrix4 &m, const SkeletonIndex &si,  Head &head, const HeadType *headType, Matrix4Par headOrigin, Vector3Par eyeDirModel, float eyeAccom) const;
};

//////////////////////////////////////////////////////////////////////////

struct LipsyncInfoItem
{
  float time;
  int phase;
};
TypeIsMovableZeroed(LipsyncInfoItem)

class LipSyncInfo
{
protected:
  AutoArray<LipsyncInfoItem> _items;
  int _current;
  float _freq;
  float _time;
  float _frame;
  float _invFrame;
public:
  enum Answer
  {
    Yes,
    No,
    Maybe
  };
  LipSyncInfo() {_current = 0; _time = 0; _frame = 0.11; _invFrame = 1.0 / _frame; _freq=1.0f;}
  bool AttachWave(AbstractWave *wave, Answer lipFile=Maybe, float freq = 1.0f);
  static bool AttachWaveReady(const char *wave, Answer lipFile=Maybe);
  float GetPhase(int & start,int & end) const;
  bool Simulate(float deltaT);
  float GetSpeed(){return _freq;};
  bool IsEmpty(){return (_items.Size()==0);};
};

//////////////////////////////////////////////////////////////////////////

#include <Es/Memory/normalNew.hpp>

//! Used for drawing of head (including proxies attached on it)
class HeadObject : public Object
{
  typedef Object base;

protected:
  const HeadType *_type;

public:
  //! Material that is used on the Man's face and hands
  Ref<TexMaterial> _faceMaterial;
  //! Texture that is used on the Man's face and hands
  Ref<Texture> _faceTexture;
  //! Wound textures that are used on the Man's face and hands
  Ref<Texture> _faceTextureWounded[2];
  //! Original texture - used in wounds animation
  Ref<Texture> _faceTextureOrig;
  //! Head wound level
  int _headWound;

  //! Constructor
  HeadObject(const HeadType *type, LODShape *shape, const CreateObjectId &id);

  //!{ Head animation and deanimation
  void Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2);
  void Deanimate(int level, bool setEngineStuff);
  //!}

  const HeadType *Type() const {return _type;}

  /// prevent head disappearing sooner then its parent
  virtual float GetFeatureSize() const {return 3.0f;}

  virtual int PassNum(int lod);
  virtual int GetAlphaLevel(int lod) const;

  virtual bool CanBeInstanced(int level, float distance2, const DrawParameters &dp) const;

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

//! Class to hold informations about a head (like shape) - this is shared by many persons
class HeadType : public RefCount, public LODShapeLoadHandler
{
protected:
  //! Proxies container
  ProxiesHolder _proxies;
public:
  //! Param file class of the head type
  ConstParamClassDeepPtr _parClass;
  //!{ CfgHeads parameters
  //! Name of the model
  RStringB _modelName;
  //!}
  //! Head model
  Ref<HeadObject> _model;
  //! Number to determine how many times is this head type actually used
  mutable int _shapeRef;
  //! Selection that covers the head wounds
  WoundTextureSelections _headWound;
  //! Section that covers faces - used to replace the texture with custom one
  AnimationSection _personality;
  //! Left eye position
  Vector3 _lEyePos;
  //! Right eye position
  Vector3 _rEyePos;
  //! Left pupil position
  Vector3 _lPupilPos;
  //! Right pupil position
  Vector3 _rPupilPos;
  //! Grimaces
  RefArray<Grimace> _grimaces;
  int _grimaceIndexHurt;
  int _grimaceIndexNeutral;
  int _grimaceIndexDead;
  int _grimaceIndexDanger;
  int _grimaceIndexAware;
  int _grimaceIndexSafe;
  int _grimaceIndexCombat;

  //! Constructor
  HeadType();
  //! Destructor
  ~HeadType() {DoAssert(_shapeRef==0);};
  //! Initialization
  void Init(const char *name);
  //! Returns name of the param file class (used in bank array)
  const RStringB &GetName() const {return _parClass->GetName();}
  //!{ Force loading / allow releasing
  void ShapeAddRef();
  void ShapeRelease();
  //!}
  //!{ Init / deinit LODShape
  void InitShape();
  void DeinitShape();
  //!}
  //!{ Init / deinit Shape specified
  void InitShapeLevel(int level);
  void DeinitShapeLevel(int level);
  //!}
  //!{ LODShapeLoadHandler
  virtual void LODShapeLoaded(LODShape *shape);
  virtual void LODShapeUnloaded(LODShape *shape);
  virtual void ShapeLoaded(LODShape *shape, int level);
  virtual void ShapeUnloaded(LODShape *shape, int level);
  //!}
  //! Sets the mimic matrices
  void PrepareGrimaceMatrices(Matrix4Array &matrices, Head &head, const HeadType *headType, const Shape *shape, Matrix4Par headOrigin, Vector3Par eyeDirModel, float eyeAccom, int grimaceDest, int grimaceSrc, float factor) const;
  //! Returns index of the grimace
  int GetGrimaceIndex(RStringB grimaceName) const;
  //! Proxies container
  const ProxiesHolder &GetProxies() const {return _proxies;}
};

class Head// : public Object
{

//  typedef Object base;
//public:
public:

  //!{ Mimic mode parameters
  int _srcGrimace;
  int _dstGrimace;
  int _newGrimace;
  int _forcedGrimace;
  float _factorGrimace; // 0 - full _srcGrimace, 1 - full _dstGrimace
  //!}

public:
  float _grimacePhase;
  //!{ Wink items
  float _winkPhase;
  float _forceWinkPhase;
  float _nextWink;
  //!}
  //! Information about the lip moving
  SRef<LipSyncInfo> _lipInfo;
  /// async test if lipsync exists
  struct FileExitsTest
  {
    RString _file;
    Future<QFileTime> _exists;
  } _lipExists;
  //!{ Random lip moving version
  bool _randomLip;
  int _actualRandomLip;
  int _wantedRandomLip;
  float _nextChangeRandomLip;
  float _speedRandomLip;
  //!}

  /// skeleton bone index of the head
  SkeletonIndex _headBone;
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

public:
  //! Constructor
  Head();
  //! Simulation
  void Simulate(float deltaT, SimulationImportance prec, bool dead);
  //! Returns forced wink phase
  int GetFaceAnimation() const {return toInt(_forceWinkPhase);}
  //! Sets the forced wink phase
  void SetFaceAnimation(int phase) {_forceWinkPhase = phase;}
  //! Plan the next grimace to be the specified one
  void SetGrimace(int grimaceIndex);
  //! Plan the next grimace to be the specified one
  void SetForcedGrimace(int grimaceIndex);
  //! Returns a wink phase
  float GetWinkPhase() const;
  //! Returns a grimace phase
  float GetGrimacePhase() const {return _grimacePhase;}

  //!{ Lipsync
  void AttachWave(AbstractWave *wave, float freq = 1.0f);
  bool AttachWaveReady(const char *wave);
  float GetLipPhase(int & start,int & end) const;
  void SetRandomLip(bool set = true);
  void NextRandomLip();
  //!}

  /// Dummy implementations
  void SetForceMimic(RStringB name) {}
};

typedef BankInitArray<HeadType> HeadTypeBank;

extern HeadTypeBank HeadTypes;

//! Class to store unique type of glasses - this is shared by many persons
class GlassesType : public RefCount, public LODShapeLoadHandler
{
protected:
  /// Unique identifier (name of the class if CfgGlasses)
  RStringB _name;
  /// Path to the model
  RStringB _modelName;
  //! Glasses model
  Ref<Object> _model;
  //! Number to determine how many times is this type actually used
  mutable int _shapeRef;

public:
  //! Constructor
  GlassesType() : _shapeRef(0) {}
  //! Destructor
  ~GlassesType() {DoAssert(_shapeRef == 0);}
  //! Initialization
  void Init(const char *name);
  //! Returns the unique identifier
  const RStringB &GetName() const {return _name;}
  //! Returns the model object
  Object *GetModel() const {return _model;}
  //!{ Force loading / allow releasing
  void ShapeAddRef();
  void ShapeRelease();
  //!}
  //!{ LODShapeLoadHandler
  virtual void LODShapeLoaded(LODShape *shape) {}
  virtual void LODShapeUnloaded(LODShape *shape) {}
  virtual void ShapeLoaded(LODShape *shape, int level) {}
  virtual void ShapeUnloaded(LODShape *shape, int level) {}
  //!}
};

typedef BankInitArray<GlassesType> GlassesTypeBank;

extern GlassesTypeBank GlassesTypes;

#endif

#endif

