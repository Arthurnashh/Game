#ifdef _MSC_VER
#pragma once
#endif

#ifndef _ANIMATION_HPP
#define _ANIMATION_HPP

#include "Shape/shape.hpp"
#include <El/Time/time.hpp>
#include <El/ParamFile/paramFileDecl.hpp>

//class Skeleton;
class SkeletonSource;

/// define basic animation properties
struct AnimationDef
{
  RStringB name;
  RStringB altName;
};

/// virtual functions used to enable various ways of initialization

class AnimationRotation;

struct AnimationRotationDef: public AnimationDef
{
  virtual void InitLevel(AnimationRotation &anim, LODShape *shape, int level) = 0;
  virtual bool BoneSupported(LODShape *shape, int level) const = 0;
  virtual ~AnimationRotationDef(){}

};

struct AnimationRotationAxisDef;
struct AnimationRotationOrientedAxisDef;
struct AnimationRotationAxisAlignedAxisDef;

class AnimationTranslation;

struct AnimationTranslationDef: public AnimationDef
{
  virtual void InitLevel(AnimationTranslation &anim, LODShape *shape, int level) = 0;
  virtual bool BoneSupported(LODShape *shape, int level) const = 0;
  virtual ~AnimationTranslationDef(){}
};

struct AnimationTranslationAxisDef;
struct AnimationTranslationOrientedAxisDef;
struct AnimationTranslationAxisAlignedAxisDef;

#if _ENABLE_REPORT
  #define CHECK_INIT_SHAPE 1
#else
  #define CHECK_INIT_SHAPE 0
#endif

/// one animation of any entity
/** Used for both skeletal (HW) and plain (SW) animations. */

class AnimationBase
{
  protected:
  SkeletonIndex _bone[MAX_LOD_LEVELS];
  bool _initDone;
  
  /// helper for oriented axis resolution
  bool FindAxis(
    Vector3 &beg, Vector3 &end, LODShape *shape, int level,
    bool inMemory, RString axis, RString altAxis
  ) const;
  
  void DoConstruct();
  int FindSelection(LODShape *shape, int level, const char *nameSel, const char *altName) const;
  int FindSelection(LODShape *shape, int level) const;
  void InitZero();
  
  public:
  AnimationBase();
  
  void Init(LODShape *shape, const char *nameSel, const char *altName );
  bool IsInitDone() const {return _initDone;}
  void Deinit(LODShape *shape);

  void InitLevel(LODShape *shape, int level, const AnimationDef &def);
  void DeinitLevel(LODShape *shape, int level);

  /// get the bone for the animation used in given level
  SkeletonIndex GetBone(int level) const {return _bone[level];}

  void RegisterLevel(LODShape *shape, int level);
  
  /// get definition structure
  virtual const AnimationDef *GetDef() const = 0;

  /// check if bone is supported by given LOD
  virtual SkeletonIndex BoneSupported(LODShape *shape, int level) const;

  virtual void SerializeBin(SerializeBinStream &f);
  virtual void SerializeBin(SerializeBinStream &f, int level);
};

class Animation: public AnimationBase
{
  typedef AnimationBase base;
  
  AnimationDef _def;
  
  public:
  void Init(LODShape *shape, const char *nameSel, const char *altName );
  void InitLevel(LODShape *shape, int level);

  virtual const AnimationDef *GetDef() const {return &_def;}
  virtual void Reverse(LODShape *shape);
};


// AnimationSection is used for animation based on faces
// hiding or changing texture are typical operations

class AnimationSectionBase
{
  protected:
  int _selection[MAX_LOD_LEVELS]; // named selection index
  bool _initDone;
  
  #if CHECK_INIT_SHAPE
    InitVal<int,0> _initCount[MAX_LOD_LEVELS];
  #endif
  
  void DoConstruct();
  void ScanIndex(LODShape *shape, int level, const char *nameSel, const char *altName );
  void InitZero();

  public:
  AnimationSectionBase();
  
  void CheckSection( LODShape *shape, int level, const char *nameSel, const char *altName );
  
  void Init(LODShape *shape, const char *nameSel, const char *altName);
  void Deinit(LODShape *shape);
  
  void InitLevel(LODShape *shape, int level, const AnimationDef &def);
  void DeinitLevel(LODShape *shape, int level);

  /// apply texture to the selection
  void SetTexture(AnimationContext &animContext, LODShape *shape, int level, Texture *texture ) const;
  /// apply material to the selection
  void SetMaterial(AnimationContext &animContext, LODShape *shape, int level, TexMaterial *mat ) const;

  /// get first texture used on selection
  Texture *GetTexture(LODShape *shape) const; 
  /// get first material used on selection
  TexMaterial *GetMaterial(LODShape *shape) const;

  void Hide(AnimationContext &animContext, LODShape *shape, int level ) const; // apply texture to all faces
  void Unhide(AnimationContext &animContext, LODShape *shape, int level ) const; // apply texture to all faces

  // no bones, def should not be queried
  virtual const AnimationDef *GetDef() const {Fail("No def");return NULL;}

  int GetSelection( int level ) const
  {
    #if CHECK_INIT_SHAPE
      DoAssert(_initCount[level]>0);
    #endif
    return _selection[level];
  }
};


class AnimationSection: public AnimationSectionBase
{
  typedef AnimationSectionBase base;
  AnimationDef _def;

  public:  
  void Init(LODShape *shape, const char *name, const char *altName);
#if _VBS3
  virtual const AnimationDef *GetDef() const {return &_def;}
#endif
  void InitLevel(LODShape *shape, int level)
  {
    base::InitLevel(shape,level,_def);
  }
};

TypeIsMovable(AnimationSection)

class TexMaterialUVChange;

class AnimationUV
{
protected:
  int _selection[MAX_LOD_LEVELS]; // selected mesh indices
  AnimationDef _def;

  void DoConstruct();

public:
  AnimationUV(){DoConstruct();}
  void Init( LODShape *shape, const char *nameSel );
  
  void InitLevel( LODShape *shape, int level);
  void DeinitLevel( LODShape *shape, int level);
  
  void UVOffset(AnimationContext &animContext, LODShape *shape, float offsetU, float offsetV, int level ) const;
  int GetSelection( int level ) const {return _selection[level];}
};

TypeIsMovable(AnimationUV)

class AnimationAnimatedTexture: public AnimationSectionBase
{
  typedef AnimationSectionBase base;
  
  // original textures by section
  Temp< Ref<Texture> > _animTexS[MAX_LOD_LEVELS];
  AnimationDef _def;

  void DoConstruct();

  public:
  AnimationAnimatedTexture(){DoConstruct();}
  
  void Init( LODShape *shape, const char *nameSel, const char *altNameSel );
  void Deinit(LODShape *shape);

  void InitLevel(LODShape *shape, int level)
  {
    InitLevel(shape,level,_def);
  }
  void InitLevel( LODShape *shape, int level, const AnimationDef &def);
  void DeinitLevel(LODShape *shape, int level);
  
  /// set phase, prefer such which is already loaded, but always request the desired one
  void SetPhaseLoadedOnly(AnimationContext &animContext, LODShape *shape, int level, int phase) const;
  /// set texture animation phase (direct - integer index)
  void SetPhase(AnimationContext &animContext, LODShape *shape, int level, int phase) const;
  /// load texture animation phase (direct - integer index)
  bool RequestPhase(LODShape *shape, int level, int phase) const;
  /// set texture animation phase (relative - float)
  void AnimateTexture(AnimationContext &animContext, LODShape *shape, int level, float anim) const;
  /// preload texture for any level (assume it does not matter)
  bool PreloadTexture(LODShape *shape, float z2) const;
  /// preload texture for given level
  bool PreloadTexture(LODShape *shape, int level, float z2) const;
};

TypeIsMovable(AnimationAnimatedTexture)

/// animate selection by rotating it around given axis
class AnimationRotation: public AnimationBase
{
  typedef AnimationBase base;

  Vector3 _center[MAX_LOD_LEVELS];
  Vector3 _direction[MAX_LOD_LEVELS];
  
  SRef<AnimationRotationDef> _def;

  public:
  AnimationRotation();
  void InitLevel(LODShape *shape, int level, const AnimationRotationAxisDef &def);
  void InitLevel(LODShape *shape, int level, const AnimationRotationOrientedAxisDef &def);
  void InitLevel(LODShape *shape, int level, const AnimationRotationAxisAlignedAxisDef &def);
  
  void InitLevel(LODShape *shape, int level);
  void DeinitLevel(LODShape *shape, int level);
  
  void Init(
    LODShape *shape, const char *name, const char *altName,
    const char *axis, const char *altAxis = NULL, bool inMemory = true
  );
  void InitOrientedAxis(
    LODShape *shape, const char *name, const char *begin, const char *end, bool inMemory = true
  );
  void InitAxisAlignedAxis(
    LODShape *shape, const char *nameD, const char *altNameD,
    const char *axisD, const char *altAxisD, bool inMemoryD
  );
  void Deinit(LODShape *shape);

  Matrix4 GetRotation(float angle, int level ) const
  {
    Matrix4 mat;
    GetRotation(mat,angle,level);
    return mat;
  }
  /// rotation around an axis specified in the animation
  void GetRotation( Matrix4 &mat, float angle, int level ) const;
  //@{ rotation around a axis-aligned axis specified in the animation
  void GetRotationX( Matrix4 &mat, float angle, int level ) const;
  void GetRotationY( Matrix4 &mat, float angle, int level ) const;
  void GetRotationZ( Matrix4 &mat, float angle, int level ) const;
  //@}
  Vector3Val GetCenter(int level) const;
  Vector3Val GetCenter() const;
  void SetCenter( Vector3Par center, Vector3Par direction );
  
  RString GetDebugName() const {return _def->name;}
  virtual const AnimationDef *GetDef() const {return _def;}
  virtual SkeletonIndex BoneSupported(LODShape *shape, int level) const;

  virtual void SerializeBin(SerializeBinStream &f);
  virtual void SerializeBin(SerializeBinStream &f, int level);
  virtual void Reverse(LODShape *shape);
};

/// animate selection by moving it in given direction
class AnimationTranslation: public AnimationBase
{
  typedef AnimationBase base;

  /// center of selection is sometimes interesting for translations as well
  Vector3 _center[MAX_LOD_LEVELS];
  /// translation direction
  Vector3 _direction[MAX_LOD_LEVELS];

  SRef<AnimationTranslationDef> _def;

public:
  AnimationTranslation();
  void InitLevel(LODShape *shape, int level, const AnimationTranslationAxisDef &def);
  void InitLevel(LODShape *shape, int level, const AnimationTranslationOrientedAxisDef &def);
  void InitLevel(LODShape *shape, int level, const AnimationTranslationAxisAlignedAxisDef &def);

  void InitLevel(LODShape *shape, int level);
  void DeinitLevel(LODShape *shape, int level);

  void Init(
    LODShape *shape, const char *name, const char *altName,
    const char *axis, const char *altAxis = NULL, bool inMemory = true
  );
  void InitOrientedAxis(
    LODShape *shape, const char *name, const char *begin, const char *end, bool inMemory = true
  );
  void InitAxisAlignedAxis(
    LODShape *shape, const char *name, const char *axisNameName, bool inMemory = true
  );
  void Deinit(LODShape *shape);

  void GetTranslationX(Matrix4 &mat, float offset, int level) const
  {
    mat.SetOrientation(M3Identity);
    mat.SetPosition(Vector3(offset,0,0));
  }
  void GetTranslationY(Matrix4 &mat, float offset, int level) const
  {
    mat.SetOrientation(M3Identity);
    mat.SetPosition(Vector3(0,offset,0));
  }
  void GetTranslationZ(Matrix4 &mat, float offset, int level) const
  {
    mat.SetOrientation(M3Identity);
    mat.SetPosition(Vector3(0,0,offset));
  }
  Matrix4 GetTranslation(float offset, int level) const
  {
    Matrix4 mat;
    GetTranslation(mat,offset,level);
    return mat;
  }
  void GetTranslation(Matrix4 &mat, float offset, int level) const;
  virtual const AnimationDef *GetDef() const {return _def;}
  virtual SkeletonIndex BoneSupported(LODShape *shape, int level) const;
  Vector3Val GetCenter() const;
  Vector3Val GetCenter(int level) const;

  virtual void SerializeBin(SerializeBinStream &f);
  virtual void SerializeBin(SerializeBinStream &f, int level);
  virtual void Reverse(LODShape *shape);
};





class ParamEntry;


/// user/config defined animation
/**
This is currently core of all animations in the game.
*/
class AnimationType : public RefCount
{
public:
  /// source value out of range handling
  enum AnimAddress
  {
    AnimClamp,AnimLoop,AnimMirror,
    NAnimAddress
  };

  /// typeid used for serialized animations
  enum TypeId
  {
    TIdRotation,TIdRotationX,TIdRotationY,TIdRotationZ,
    TIdTranslation,TIdTranslationX,TIdTranslationY,TIdTranslationZ,
    TIdDirect,TIdHide
  };


protected:
  /// name of the config class
  RStringB _name;
  /// controller binding
  RStringB _source;
  float _minPhase,_maxPhase;
  float _minValue,_maxValue;
  AnimAddress _address;
  
public:
  static AnimationType *CreateObject(SerializeBinStream &f);
  static AnimationType *CreateObject(ParamEntryPar cls, LODShape *shape);
  virtual void Init(ParamEntryPar cls, LODShape *shape);
  
  virtual void InitLevel(LODShape *shape, int level) = 0;
  virtual void DeinitLevel(LODShape *shape, int level) = 0;

  float GetPhase(float phase, float minResult, float maxResult) const;
  void MinMaxValue(float &minVal, float &maxVal) const;
  
  /// get controller binding
  const RStringB &GetSource() const {return _source;}
  const RStringB &GetName() const {return _name;}

  virtual size_t GetMemoryControlled() const = 0;
  virtual SkeletonIndex GetBone(int level) const = 0;
  virtual Matrix4 GetAnimMatrix(int level, float phase) const = 0;
  virtual Vector3Val GetCenter(int level) const {Fail("No center supported");return VZero;}
  virtual Vector3Val GetCenter() const {Fail("No center supported");return VZero;}
  
  /// binary save/load - common info
  virtual void SerializeBin(SerializeBinStream &f) = 0;
  /// binary save/load - per-level info
  virtual void SerializeBin(SerializeBinStream &f, int level) =0;
  /// serialized TypeId
  virtual TypeId GetType() const = 0;
  /// reverse shape - mirror X/Z, leave Y
  /** needed for LODShape::Reverse */
  virtual void Reverse(LODShape *shape) = 0;
};

/// rotation around arbitrary axis
class AnimationRotationType : public AnimationType
{
  typedef AnimationType base;
protected:
    
  AnimationRotation _animation;
  float _angle0;
  float _angle1;

public:
  virtual void Init(ParamEntryPar cls, LODShape *shape);

  virtual void InitLevel(LODShape *shape, int level);
  virtual void DeinitLevel(LODShape *shape, int level);
  
  SkeletonIndex GetBone(int level) const;
  Matrix4 GetAnimMatrix(int level, float phase) const;
  
  virtual Vector3Val GetCenter(int level) const;
  virtual Vector3Val GetCenter() const;
  virtual TypeId GetType() const {return TIdRotation;}
  virtual void SerializeBin(SerializeBinStream &f);
  virtual void SerializeBin(SerializeBinStream &f, int level);
  virtual void Reverse(LODShape *shape);
  virtual size_t GetMemoryControlled() const {return sizeof(*this);}

};

//// common base for AnimationRotationAxisAlignedType template
class AnimationRotationAxisAlignedBaseType: public AnimationRotationType
{
  typedef AnimationType base;

  public:
  virtual void Init(ParamEntryPar cls, LODShape *shape);
  virtual TypeId GetType() const = 0;
};

typedef void (AnimationRotation::*RotationSelect)(Matrix4 &mat, float angle, int level) const;

/// rotation around axis aligned around a major axis
template <RotationSelect getRotation, AnimationType::TypeId id>
class AnimationRotationAxisAlignedType: public AnimationRotationAxisAlignedBaseType
{
  typedef AnimationRotationAxisAlignedBaseType base;

  public:
  Matrix4 GetAnimMatrix(int level, float phase) const
  {
    float angle = GetPhase(phase, _angle0, _angle1);
    Matrix4 rot;
    (_animation.*getRotation)(rot,angle,level);
    return rot;
  }
  virtual TypeId GetType() const {return id;}
};

/// rotation around X-axis
class AnimationRotationXType:
  public AnimationRotationAxisAlignedType<&AnimationRotation::GetRotationX,AnimationType::TIdRotationX>
{};

/// rotation around Y-axis
class AnimationRotationYType:
  public AnimationRotationAxisAlignedType<&AnimationRotation::GetRotationY,AnimationType::TIdRotationY>
{};

/// rotation around Z-axis
class AnimationRotationZType:
  public AnimationRotationAxisAlignedType<&AnimationRotation::GetRotationZ,AnimationType::TIdRotationZ>
{};

/// translation animation
class AnimationTranslationType : public AnimationType
{
protected:
  typedef AnimationType base;

  AnimationTranslation _animation;
  float _offset0;
  float _offset1;

public:
  void Init(ParamEntryPar cls, LODShape *shape);

  virtual void InitLevel(LODShape *shape, int level);
  virtual void DeinitLevel(LODShape *shape, int level);

  SkeletonIndex GetBone(int level) const;
  Matrix4 GetAnimMatrix(int level, float phase) const;
  Vector3Val GetCenter() const;
  Vector3Val GetCenter(int level) const;
  virtual TypeId GetType() const {return TIdTranslation;}
  virtual void SerializeBin(SerializeBinStream &f);
  virtual void SerializeBin(SerializeBinStream &f, int level);
  virtual void Reverse(LODShape *shape);
  virtual size_t GetMemoryControlled() const {return sizeof(*this);}
};

//// common base for AnimationTranslationAxisAlignedType template
class AnimationTranslationAxisAlignedBaseType: public AnimationTranslationType
{
  typedef AnimationType base;

  public:
  virtual void Init(ParamEntryPar cls, LODShape *shape);
  virtual TypeId GetType() const = 0;
};

typedef void (AnimationTranslation::*TranslationSelect)(Matrix4 &mat, float offset, int level) const;

/// translation aligned with a major axis
template <TranslationSelect getTranslation, AnimationType::TypeId id>
class AnimationTranslationAxisAlignedType: public AnimationTranslationAxisAlignedBaseType
{
  typedef AnimationTranslationAxisAlignedBaseType base;

  public:
  Matrix4 GetAnimMatrix(int level, float phase) const
  {
    float offset = GetPhase(phase, _minValue, _maxValue);
    Matrix4 rot;
    (_animation.*getTranslation)(rot,offset,level);
    return rot;
  }
  virtual TypeId GetType() const {return id;}
};

/// translation - X-axis
class AnimationTranslationXType:
  public AnimationTranslationAxisAlignedType<&AnimationTranslation::GetTranslationX,AnimationType::TIdTranslationX>
{};

/// translation - Y-axis
class AnimationTranslationYType:
  public AnimationTranslationAxisAlignedType<&AnimationTranslation::GetTranslationY,AnimationType::TIdTranslationY>
{};

/// translation - Z-axis
class AnimationTranslationZType:
  public AnimationTranslationAxisAlignedType<&AnimationTranslation::GetTranslationZ,AnimationType::TIdTranslationZ>
{};

/// hold info about animations bound to a common skeleton
class AnimationHolder: public LODShapeLoadHandler
{
  class LoadAnimationFromEntry;
  
  RefArray<AnimationType> _animations;

  /// for each level map bones to animations
  AutoArray< AutoArray< AutoArray<int> > > _bonesToAnimations;

  /// used when returning values by reference
	static AnimationHolder _empty;

  void InitBonesToAnimations(LODShape *shape, int level);
  	
public:
  void Load(LODShape *shape, ParamEntryPar array);
  void Clear();
  void InitLevel(LODShape *shape, int level);
  void DeinitLevel(LODShape *shape, int level);
  
  //@{ array style enumeration
  int Size() const {return _animations.Size();}
  AnimationType *operator [] (int i) const {return _animations[i];}
  //@}

  /// find an animation with a given name
  int FindIndex(RString name) const;
  /// find which animations control given bone
  const AutoArray<int> &BoneToAnimation(int level, SkeletonIndex bone) const
  {
    return _bonesToAnimations[level][GetSkeletonIndex(bone)];
  }
  /// remove an animation
  void Remove(int index);


  //@{ LODShapeLoadHandler implementation
  virtual void LODShapeLoaded(LODShape *shape);
  virtual void LODShapeUnloaded(LODShape *shape);
  virtual void ShapeLoaded(LODShape *shape, int level);
  virtual void ShapeUnloaded(LODShape *shape, int level);
  //@}
  //@{ per-model (LODShape) serialization
	virtual void SerializeBin(SerializeBinStream &f);
	virtual size_t GetMemoryControlled() const;
	//@}
  //@{ per-level (Shape) serialization
	virtual void SerializeBin(SerializeBinStream &f, int level);
	virtual size_t GetMemoryControlled(int level) const;
	//@}
	
	
	static const AnimationHolder &Empty() {return _empty;}
	
};

#endif
