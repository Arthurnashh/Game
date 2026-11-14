#ifdef _MSC_VER
#pragma once
#endif

#ifndef _WOUNDS_HPP
#define _WOUNDS_HPP

#include "Shape/shape.hpp"

struct WoundTexPair
{
	Ref<Texture> _healthy;
	Ref<Texture> _wounded[2];
};

struct WoundMatPair
{
	Ref<TexMaterial> _healthy;
	Ref<TexMaterial> _wounded[2];
};

TypeIsMovableZeroed(WoundTexPair)
TypeIsMovableZeroed(WoundMatPair)

/// list of textures to be replaced with another texture
class WoundInfo: public RefCount
{
  friend class WoundTextureSelections;
  
  AutoArray<WoundTexPair> _tex;
  AutoArray<WoundMatPair> _mat;
  
	public:
	void Load(LODShape *shape, ParamEntryVal cfg);
	void Unload();
	bool IsEmpty() const {return _tex.Size()==0 && _mat.Size()==0;}
};

/// replace textures on wounded soldier/damaged vehicle
template <class Type>
class WoundTextureSelection
{
	SectionSelection _sections;
	Ref<Type> _tex[2];
	Ref<Type> _oldTex;

	void SetTexture(AnimationContext &animContext, const Shape *shape, Type *tex, float dist2) const;

	public:
  WoundTextureSelection() {};
	void Init(const AutoArray<int> &sections, Type *tex1, Type *tex2, Type *oldTex)
  {
	  _sections.SetSections(sections);
	  _tex[0] = tex1;
	  _tex[1] = tex2;
	  _oldTex = oldTex;
  }

	/// override wounded texture for some particular healthy texture
	void Apply(AnimationContext &animContext, const Shape *shape, Type *tex, float dist2) const
  {
	  SetTexture(animContext, shape, tex, dist2);
  }

  void Apply(AnimationContext &animContext, const Shape *shape, int woundLevel, float dist2) const
  {
    Assert(woundLevel >= 0 && woundLevel < lenof(_tex));
	  SetTexture(animContext, shape, _tex[woundLevel], dist2);
  }

	Type *GetOrigTexture() const {return _oldTex;}

  ClassIsMovable(WoundTextureSelection)
};

typedef WoundTextureSelection<Texture> WoundTextureSelectionTex;
typedef WoundTextureSelection<TexMaterial> WoundTextureSelectionMat ;

class WoundTextureSelections;
/// definition needed for WoundTextureSelections
/** similar to AnimationRotationDef */

struct WoundTextureSelectionsDef
{
  virtual void InitLevel(WoundTextureSelections &anim, LODShape *shape, int level) = 0;
  virtual ~WoundTextureSelectionsDef(){}
};

struct WoundTextureSelectionsBasicDef;
struct WoundTextureSelectionsNameDef;

typedef AutoArray<WoundTextureSelectionTex> WoundTextureSelectionArray;
typedef AutoArray<WoundTextureSelectionMat> WoundMaterialSelectionArray;


#if _ENABLE_REPORT
  #define CHECK_INIT_SHAPE_WOUND 1
#else
  #define CHECK_INIT_SHAPE_WOUND 0
#endif


/// wounding by replacing textures
class WoundTextureSelections
{
	Buffer<WoundTextureSelectionArray> _selectionTex;
	Buffer<WoundMaterialSelectionArray> _selectionMat;
	SRef<WoundTextureSelectionsDef> _def;
	#if CHECK_INIT_SHAPE_WOUND
	  InitVal<int,0> _initCount[MAX_LOD_LEVELS];
	#endif
	
	public:
	// construct from Animation and WoundInfo
	WoundTextureSelections();
	void Init(LODShape *shape, Ref<WoundInfo> info);
	void Init(
		LODShape *shape, Ref<WoundInfo> info, const char *name, const char *altName
	);
	
	bool IsDefined() const {return _def.NotNull();}
	void InitLevel(LODShape *shape, int level)
	{
	  _def->InitLevel(*this,shape,level);
	}

  void InitLevelBasic( LODShape *shape, int level, const WoundTextureSelectionsBasicDef &def);
  void InitLevelName( LODShape *shape, int level, const WoundTextureSelectionsNameDef &def);
	void DeinitLevel(LODShape *shape, int level);
	
	void Unload();

	void Apply(AnimationContext &animContext, LODShape *shape, int level, int woundLevel, float dist2) const;
	void ApplyModified(
		AnimationContext &animContext, LODShape *shape, int level, int woundLevel,
		Texture *orig, Texture *origWounded, float dist2
	) const; // override wounded texture for some texture
};

TypeIsMovable(WoundTextureSelections)

#endif
