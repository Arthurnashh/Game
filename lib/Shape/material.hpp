#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MATERIAL_HPP
#define _MATERIAL_HPP

#include "../textbank.hpp"
//#include "tlVertex.hpp"
#include "../engine.hpp"
#include <El/Enum/enumNames.hpp>
#include <El/ParamFile/paramFileDecl.hpp>
#include <El/ParamArchive/serializeClass.hpp>
#include <Es/Containers/bankArray.hpp>

//! Render flags
/*!
  Note that it is forbidden to remove any items from the current set - the binarized form
  would be invalid then
*/
#define RENDERFLAG_ENUM(type,prefix,XX) \
  XX(type, prefix, AlwaysInShadow) \
  XX(type, prefix, NoZWrite) \
  XX(type, prefix, LandShadow) \
  XX(type, prefix, Dummy0) \
  XX(type, prefix, NoColorWrite) \
  XX(type, prefix, NoAlphaWrite) \
  XX(type, prefix, AddBlend) \
  XX(type, prefix, AlphaTest32) \
  XX(type, prefix, AlphaTest64) \
  XX(type, prefix, AlphaTest128) \
  XX(type, prefix, Road) \
  XX(type, prefix, NoTiWrite) \

#ifndef DECL_ENUM_RENDER_FLAG
#define DECL_ENUM_RENDER_FLAG
DECL_ENUM(RenderFlag)
#endif
DECLARE_ENUM(RenderFlag,RF,RENDERFLAG_ENUM)

class ParamEntry;
class ParamArchive;

/// information about one particular texture stage
class TexStageInfo: public SerializeClass
{
  friend class TexMaterial;
  
  public:

  /// texture map file
  RefR<Texture> _tex;

  /// gets texture that is used for drawing (including replacement for env texture if needed)
  Texture* GetTextureForDraw() const;

  /// max. allowed texture filtering style
  TexFilter _filter;
  /// index of texgen used
  int _texGen;
#if _ARMA3_ENV_MAPS
  // if we should use world env map instead of our texture
  bool _useWorldEnvMap;
  // save version where the env maps were introduced
  static const int WorldEnvMapSaveVersion = 11;
#endif
  TexStageInfo();

  //! check if default values are used
  bool IsDefaultValue(ParamArchive &ar) const;
  //! load with default values
  void LoadDefaultValues(ParamArchive &ar);
  //! serialize all values into current class
  LSError Serialize(ParamArchive &ar);
  /// fast serialization
  bool SerializeBin(SerializeBinStream &f, int version);
  //!{ Operators
  bool operator== (const TexStageInfo &sec) const
  {
    return (
      // _filter==sec._filter &&
      _tex==sec._tex
#if _ARMA3_ENV_MAPS
      && _useWorldEnvMap == sec._useWorldEnvMap
#endif
    );
  }
  bool operator!= (const TexStageInfo &sec) const {return !operator==(sec);}
  //!}
};

/// information about one particular texture stage
class TexGenInfo: public SerializeClass
{
  friend class TexMaterial;
  
  public:
  /// texture coord source
  UVSource _uvSource;
  //! texture coordinate transform
  Matrix4 _uvTransform;

  TexGenInfo();

  /// calculate UV scale based on tex-coord source and matrix
  /**
  @return negative number (-1) in case it has no relation to the UV mapping
  */  
  float GetUVScale() const;
  /// get UV scale assuming 2D input will be used as a source
  float GetUVScale2D() const
  {
    return fabs(_uvTransform.Orientation().Determinant2x2());
  }
  /// get UV scale assuming 3D input will be used as a source
  float GetUVScale3D() const
  {
    // calculate volume change
    float volScale = fabs(_uvTransform.Orientation().Determinant());
    // assume area change is volume change ^(2/3)
    return powf(volScale,2.0f/3);
  }
  //! check if default values are used
  bool IsDefaultValue(ParamArchive &ar) const;
  //! load with default values
  void LoadDefaultValues(ParamArchive &ar);
  //! serialize all values into current class
  LSError Serialize(ParamArchive &ar);
  /// fast serialization
  bool SerializeBin(SerializeBinStream &f, int version);
  //!{ Operators
  bool operator== (const TexGenInfo &sec) const
  {
    return _uvSource==sec._uvSource && _uvTransform==sec._uvTransform;
  }
  bool operator!= (const TexGenInfo &sec) const {return !operator==(sec);}
  //!}
};

#if (_ENABLE_REPORT || _ENABLE_BULDOZER)  
  #define TEXMAT_RELOAD 1
#else
  #define TEXMAT_RELOAD 0
#endif

//////////////////////////////////////////////////////////////////////////

//!{ Declaration, default definition and real definition for particular descendants
#define TEXMATERIAL_INTERFACE_WATER(XXX) \
  XXX(virtual Color GetWaveColor() const,{return HWhite;},{return _color;}) \
  XXX(virtual float GetWaveSpeed() const,{return 0;},{return _speed;})
//!}

//! List of all possible descendants
#define TEXMATERIAL_ALL_INTERFACES \
    TEXMATERIAL_INTERFACE_WATER(DEFINE_DEF_PARS) 

//! Default and real definitions
//#define DECLARE_PARS(declaration,def_definition,impl_definition) declaration;
#define DEFINE_DEF_PARS(declaration,def_definition,impl_definition) declaration def_definition
#define DEFINE_IMPL_PARS(declaration,def_definition,impl_definition) declaration impl_definition

//////////////////////////////////////////////////////////////////////////

struct TexMaterialLOD
{
  //! Coefficient to show the importance of the specified texture for this LOD
  /*!
    If this LOD is to be used, the texture must be stretched on less than _pixelsOverTexLimit pixels
  */
  float _pixelsOverTexLimit;
  //! Engine specific pixel shader ID
  PixelShaderID _pixelShaderID;
  //! Vertex shader identification
  VertexShaderID _vertexShaderID;
  bool operator== (const TexMaterialLOD &sec) const
  {
    return (
      _pixelsOverTexLimit==sec._pixelsOverTexLimit &&
      _pixelShaderID==sec._pixelShaderID &&
      _vertexShaderID==sec._vertexShaderID
    );
  }
  bool operator!= (const TexMaterialLOD &sec) const {return !operator==(sec);}
};

//////////////////////////////////////////////////////////////////////////

class TexMaterialTypeModelSpace;
class TexMaterialTypeSimpleTerrain;
class TexMaterialTypeGrassTerrain;
class TexMaterialTypeRoad;
class TexMaterialTypeNormal;

/// texture material type interface
class TexMaterialType: public RefCount
{
  public:
  //@{ use double virtuality - will be reversing left and right side
  virtual bool Compare(const TexMaterialTypeSimpleTerrain &type) const {return false;}
  virtual bool Compare(const TexMaterialTypeModelSpace &type) const {return false;}
  virtual bool Compare(const TexMaterialTypeRoad &type) const {return false;}
  virtual bool Compare(const TexMaterialTypeNormal &type) const {return false;}
  virtual bool Compare(const TexMaterialTypeGrassTerrain &type) const {return false;}
  
  virtual bool operator == (const TexMaterialType &type) const = 0;
  bool operator !=(const TexMaterialType &type) const {return !operator ==(type);}
  //@}
  
  /// perform the transformation
  /**
  This should do anything needed to convert the material after the loading from 
  the file (note: may call Transform in a chained manner)
  */
  virtual void Transform(TexMaterial *mat) = 0;
};

/// no modification
class TexMaterialTypeNormal: public TexMaterialType
{
  
  public:
  virtual bool Compare(const TexMaterialTypeNormal &type) const {return true;}

  virtual bool operator ==(const TexMaterialType &type) const {return type.Compare(*this);}
  virtual void Transform(TexMaterial *mat) {}
};

/// any derived material
class TexMaterialTypeDerived: public TexMaterialType
{
  Ref<TexMaterialType> _base;

  protected:
  TexMaterialTypeDerived(TexMaterialType *basis):_base(basis){}
  bool CompareBase(const TexMaterialTypeDerived &type) const {return *_base==*type._base;}
};

/// convert material to model space

class TexMaterialTypeModelSpace: public TexMaterialTypeDerived
{
  typedef TexMaterialTypeDerived base;
  
  Vector3 _offset;
  
  public:
  TexMaterialTypeModelSpace(TexMaterialType *baseMat, Vector3Par offset)
  :base(baseMat),_offset(offset){}
  
  virtual bool Compare(const TexMaterialTypeModelSpace &type) const
  {
    return _offset==type._offset && CompareBase(type);
  }

  virtual bool operator ==(const TexMaterialType &type) const {return type.Compare(*this);}
  virtual void Transform(TexMaterial *mat);
};

/// material for distance terrain rendering
class TexMaterialTypeSimpleTerrain: public TexMaterialTypeModelSpace
{
  typedef TexMaterialTypeModelSpace base;
  
  public:
  
  TexMaterialTypeSimpleTerrain(TexMaterialType *baseMat, Vector3Par offset)
  :base(baseMat,offset){}
  
  /// prevent inheriting, as we can not be considerer equal to base
  virtual bool Compare(const TexMaterialTypeModelSpace &type) const
  {
    return false;
  }
  virtual bool Compare(const TexMaterialTypeSimpleTerrain &type) const
  {
    /// conditions for equality are exactly the same as with the base
    return base::Compare(type);
  }
  
  virtual bool operator ==(const TexMaterialType &type) const {return type.Compare(*this);}
  virtual void Transform(TexMaterial *mat);
};

/// material for terrain grass rendering
class TexMaterialTypeGrassTerrain: public TexMaterialTypeModelSpace
{
  typedef TexMaterialTypeModelSpace base;
  
  public:
  
  TexMaterialTypeGrassTerrain(TexMaterialType *baseMat, Vector3Par offset)
  :base(baseMat,offset){}
  
  /// prevent inheriting, as we can not be considerer equal to base
  virtual bool Compare(const TexMaterialTypeModelSpace &type) const
  {
    return false;
  }
  virtual bool Compare(const TexMaterialTypeGrassTerrain &type) const
  {
    /// conditions for equality are exactly the same as with the base
    return base::Compare(type);
  }
  
  virtual bool operator ==(const TexMaterialType &type) const {return type.Compare(*this);}
  virtual void Transform(TexMaterial *mat);
};

/// material for road rendering
/**
Roads share many properties with terrain, but pixel shader is much simpler.
*/
class TexMaterialTypeRoad: public TexMaterialTypeDerived
{
  typedef TexMaterialTypeDerived base;
  
  public:
  explicit TexMaterialTypeRoad(TexMaterialType *baseMat)
  :base(baseMat){}
  
  virtual bool Compare(const TexMaterialTypeRoad &type) const
  {
    return CompareBase(type);
  }

  virtual bool operator ==(const TexMaterialType &type) const {return type.Compare(*this);}
  virtual void Transform(TexMaterial *mat);
};


/// material identification
struct TexMaterialName
{
  /// name of the basic material to load from
  RStringB _name;
  /// type may provide some derivation/transformation to be performed on the material
  Ref<TexMaterialType> _type;
  
  TexMaterialName():_type(_defaultType){}
  TexMaterialName(const RStringB &name, TexMaterialType *type=_defaultType)
  :_name(name),_type(type){}
  TexMaterialName(const TexMaterialName &name)
  :_name(name._name),_type(name._type){}
  TexMaterialName(const TexMaterialName &name, TexMaterialType *type)
  :_name(name._name),_type(type){}
  
  operator const char *() const {return _name;}
  operator RStringB() const {return _name;}

  //@{ permanent ids to common types
  static Ref<TexMaterialType> _defaultType;
  static Ref<TexMaterialType> _roadType;
  //@} 
};

#if _ENABLE_PHYSICS
  namespace enf
  {
    struct PhysicsMaterial;
  }
#endif
//! general surface material properties
/*!
contains texture and other material properties.
Primary use for this structure is in ShapeSection
*/
class TexMaterial: public RequestableObject, public LinkBidirWithFrameStore
{
public:
  //! material name
  TexMaterialName _name;

  /// some materials may be background loading 
  /** during loading the material is marked as partially loaded - some functions do not require full load */
  //SizedEnum<LoadState> _loaded;
  enum {NotLoaded, Loading, FullyLoaded} _loaded;
  
  #if TEXMAT_RELOAD
    QFileTime _timestamp; //! time-stamp of the source file
    QFileTime GetTimestamp() const {return _timestamp;}
  #endif
  #if _ENABLE_REPORT
    /// file which we are loaded from
    RStringB _filename;
  #endif

private:
  //! Type of the main light
  EMainLight _mainLight;
  //! Type of the fog
  EFogMode _fogMode;
  //! Flags to determine the way of drawing
  typedef PackedBoolAutoArrayTT< FixedArray<PackedBoolArray,1> > RenderFlagsType;
  RenderFlagsType _renderFlags;
  
  /// integer date (meaning specific for each material type)
  int _customInt;

  /// some materials may be multi-passed
  /**
  This is only for binarization purposes. During rendering the passes are already unrolled.
  */
  Ref<TexMaterial> _nextPass;

  //!{ Material LOD parameters
  bool _lodLimitsInObjectPixels;
  TexMaterialLOD _lod;
  //!}

public: 
  
  enum {NStages=15};
  enum {NTexGens=8};
  //! Info about all supported stages
  TexStageInfo _stage[NStages];
  TexGenInfo _texGen[NTexGens];

  //! Stage dedicated to TI texture
  TexStageInfo _stageTI; // _VBS3_TI

  /// AutodetectAnisotropy needs to be called only once
  bool _anisotropyDetected;
  //! Number of used stages, not including stage 0
  int _stageCount;
  //! Number of used texgen, including stage 0
  int _nTexGen;
  //@{
  /*!
  \name material lighting properties
  */
  //! emmisive color - used for shining polygons
  Color _emmisive;
  //! ambient color factor
  Color _ambient;
  //! diffuse color factor
  Color _diffuse;
  //! forced diffuse color factor - used to simulate half or fully lighted polys
  Color _forcedDiffuse;
  //! specular color factor
  Color _specular;
  // specular power - higher value means sharper highlight
  // value zero means no highlight
  float _specularPower;
  //@}
  /// physical and other common surface properties
  Ref<SurfaceInfo> _surfaceInfo;
#if _ENABLE_PHYSICS
  enf::PhysicsMaterial* _physMat;
#endif
  /// base class to provide some type safety
  class CustomData: public RefCount
  {
    public:
    /// allow destruction using CustomData interface only
    virtual ~CustomData(){}
  };
  /// one custom data item may be attached
  /** used for Landscape ground clutter maps*/
  mutable Ref<CustomData> _customData;
  
  CustomData *GetCustomData() const {return _customData;}
  
  int GetCustomInt() const {return _customInt;}
  void SetCustomInt(int val) {_customInt = val;}

  void SetCustomData(CustomData *val) {_customData = val;}

#if _ENABLE_PHYSICS
  enf::PhysicsMaterial* GetPhysMaterial() const {Load(); return _physMat;}
#endif  
  const SurfaceInfo *GetSurfaceInfo() const {Load();return _surfaceInfo;}
  bool LodLimitsInObjectPixels() const {Load(); return _lodLimitsInObjectPixels;}
  PixelShaderID GetPixelShaderID() const {Load(); return _lod._pixelShaderID;}
  VertexShaderID GetVertexShaderID() const {Load(); return _lod._vertexShaderID;}
  
  PixelShaderID GetPixelShaderIDLoaded() const {Assert(_loaded>NotLoaded); return _lod._pixelShaderID;}
  VertexShaderID GetVertexShaderIDLoaded() const {Assert(_loaded>NotLoaded); return _lod._vertexShaderID;}
  
  EMainLight GetMainLight() const {Load();return _mainLight;}
  EFogMode GetFogMode() const {Load();return _fogMode;}
  bool GetRenderFlag(RenderFlag rf) const {Load();return _renderFlags.Get(rf);}
  bool SomeRenderFlags() const {Load();return !_renderFlags.IsEmpty();}

  void SetPixelShaderID(PixelShaderID id) {_lod._pixelShaderID = id;}
  void SetVertexShaderID(VertexShaderID id) {_lod._vertexShaderID = id;}
  void SetMainLight(EMainLight ml) {_mainLight = ml;}
  void SetFogMode(EFogMode fm) {_fogMode = fm;}
  //void SetRenderFlags(PackedBoolAutoArray rf) {_renderFlags = rf;}
  void SetRenderFlag(RenderFlag rf, bool value) {_renderFlags.Set(rf, value);}
  void SetStageTexture(int stage, Texture *texture)
  {
    Assert(stage < NStages);
    saturateMax(_stageCount, stage);
    _stage[stage]._tex = texture;
    _anisotropyDetected = false;
  }
  void SetAmbient(ColorVal ambient)
  {
    _ambient = ambient;
  }

  //! Function returns the vertex declaration
  /*!
    The vertex declaration depends on vertex shader and number of UV sets so far
  */
  int GetVertexDeclaration() const;
  //! Function to determine pixel shader uses alpha discrete result
  bool IsDiscretized() const;

  //! create default material
  void Init();

  TexMaterial();
  ~TexMaterial();

  //! create material from CfgMaterials
  explicit TexMaterial(const TexMaterialName &name);
  //! create material from a config entry
  explicit TexMaterial(ParamEntryPar cfg);
  /// create embedded material
  TexMaterial(const TexMaterialName &name, SerializeBinStream &f);
  /// create a material copy with a different name
  TexMaterial(const TexMaterialName &name, const TexMaterial &src);
  //! load material from a config entry
  void Load(ParamEntryPar cfg);
  //! combine this material with TLMaterial
  void CombineDirect(TLMaterial &dst, const TLMaterial &src);
  //! combine this material with TLMaterial omit src if needed
  void Combine(TLMaterial &dst, const TLMaterial &src);
  //! get name (necessary for BankArray)
  const TexMaterialName &GetName() const {return _name;}

  //! reload any used files (textures) if necessary
  void CheckForChangedFiles();

  //! reload if necessary
  void Reload(bool force=false);
  /// helper for construction
  void LoadFromName(const TexMaterialName &name);
  #if _DEBUG
  virtual void OnUsed() const;
  #endif
  /// RefCount implementation
  virtual void OnUnused() const;
  /// implementation of LinkBidirWithFrameStore
  size_t GetMemoryControlled() const;
  /// called when item is taken from cache and reused
  void ReuseCached();
  
  /// multi-pass support for binarization
  TexMaterial *NextPass() const {Load();return _nextPass;}
  //@{ RequestableObject implementation
  virtual int ProcessingThreadId() const;
  virtual void RequestDone(RequestContext *context, RequestResult result);
  //@}
  
  /// check if data are ready
  bool IsReady() const {MemorySubscribe();return int(_loaded)>=FullyLoaded;}
  /// make sure data are loaded before used
  void Load() const
  {
    MemorySubscribe();
    if (int(_loaded)<FullyLoaded) unconst_cast(this)->DoLoad();
  }

  //! Function to return the index of UV stage on specified texgen stage
  int GetUVStage(const TexGenInfo &texGen) const
  {
    return texGen._uvSource==UVTex1 ? 1 : 0;
  }
  //! Function to return the index of UV stage on specified texture stage
  int GetUVStage(int stage) const
  {
    return GetUVStage(GetTexGen(stage));
  }
  /// access texGen based on stage index
  const TexGenInfo &GetTexGen(int stage) const {return _texGen[_stage[stage]._texGen];}
  //! Function returns approximation of the scale of texture on a particular stage
  /**
    @param stage number of texture stage
    Function returns negative number (-1) in case it has no relation to the UV mapping
  */
  float GetUVScale(int stage) const
  {
    return GetTexGen(stage).GetUVScale();
  }

  int GetTexGenIndex(int stage) const {return _stage[stage]._texGen;}

  /// preload data for given detail level 
  bool PreloadData(const AreaOverTex &areaOTex, float z2, float *quality=NULL) const;

  /// make sure data are ready for rendering at given detail level 
  void PrepareData(const AreaOverTex &areaOTex, float z2) const;

  //! Update textures after the material is loaded
  void UpdateTexturesAfterLoading();
  
  /// fast serialization
  bool SerializeBin(SerializeBinStream &f);
  
  /// if the material is already loaded, we need to skip it
  void SkipBin(SerializeBinStream &f, RStringB filename);

  /// do auto-detection only once
  void AutodetectAnisotropy() const
  {
    if (!_anisotropyDetected) unconst_cast(this)->DoAutodetectAnisotropy();
  }
  
  /// perform any adjustments (e.g. backward compatibility handling) after the material has been loaded
  void AdjustAfterLoad();

  /// some stages may require to detect anisotropy based on textures/pixel shader
  void DoAutodetectAnisotropy();

    
  //! load material from a config entry using ParamArchive interface
  LSError Serialize(ParamArchive &ar);

  #if TEXMAT_RELOAD
  ///Reloads material from memory. 
  /**
  @param data pointer to memory block contain file in paramFile format
  @param size size of memory block
  @return true, if material has been reloaded
  */
  bool ReloadFromMemory(const void *data, size_t size); 
  #endif

  //! Define all descendant interface methods
  TEXMATERIAL_ALL_INTERFACES
  
protected:
  /// load data - may be blocking when used outside of RequestDone
  void DoLoad();
  /// start background loading
  void RequestData();
  
  /// check if all expected entries are filled
  void Validate();


public:
  /// safe acceleration of TexMaterial access - avoid multiple Load calls
  /**
  Load is called only once per scope, result in shorter code with less branches
  */
  class Loaded
  {
    const TexMaterial *_mat;
    
    public:
    explicit Loaded(const TexMaterial *mat)
    :_mat(mat)
    {
      if (mat) mat->Load();
    }
    
    PixelShaderID GetPixelShaderID() const {return _mat->_lod._pixelShaderID;}
    VertexShaderID GetVertexShaderID() const {return _mat->_lod._vertexShaderID;}
    EMainLight GetMainLight() const {return _mat->_mainLight;}
    EFogMode GetFogMode() const {return _mat->_fogMode;}
    bool SomeRenderFlags() const {return !_mat->_renderFlags.IsEmpty();}
    bool GetRenderFlag(RenderFlag rf) const {return _mat->_renderFlags.Test(rf)!=0;}
    const SurfaceInfo *GetSurfaceInfo() const {return _mat->_surfaceInfo;}
#if _ENABLE_PHYSICS
    enf::PhysicsMaterial* GetPhysMaterial() const {return _mat->_physMat;}
#endif
  };
};

class TexMaterialWater: public TexMaterial
{
  Color _color;
  float _speed;
public:
  TEXMATERIAL_INTERFACE_WATER(DEFINE_IMPL_PARS)
};


/// material bank

template<>
struct BankTraits<TexMaterial>: public DefLLinkBankTraits<TexMaterial>
{
	typedef const TexMaterialName &NameType;
  // default name comparison
	static int CompareNames( NameType n1, NameType n2 )
	{
		if (n1._name!=n2._name) return 1;
		return (*n1._type)!=(*n2._type);
	}

  static void ReuseCachedItem(TexMaterial *item)
  {
    item->ReuseCached();
  }

  struct TexMaterialHashTraits: public DefMapClassTraits< LLink<TexMaterial> >
  {
    //! key type
    typedef NameType KeyType;
    //! calculate hash value
    static unsigned int CalculateHashValue(KeyType key)
    {
      return CalculateStringHashValue(key._name);
    }

    //! compare keys, return negative when k1<k2, positive when k1>k2, zero when equal
    static int CmpKey(KeyType k1, KeyType k2)
    {
      return CompareNames(k1,k2);
    }
    static KeyType GetKey(const TexMaterial *item)
    {
      return item->GetName();
    }
  };
  /// replace default container
  typedef MapStringToClass<
    LLink<TexMaterial>, LLinkArray<TexMaterial>,
    TexMaterialHashTraits
  > ContainerType;
};

template <>
struct BankTraitsExt< BankTraits<TexMaterial> >
{
  typedef BankTraits<TexMaterial> Traits;
  typedef Traits::Type Type;
  typedef Traits::NameType NameType;
  typedef Traits::ContainerType ContainerType;
  /// get a name
  static NameType GetName(const Type *item) {return item->GetName();}
  /// perform a search
  static Type *Find(const ContainerType &container, NameType name)
  {
    const LLink<Type> &item = container.Get(name);
    if (container.IsNull(item))
    {
      return NULL;
    }
    return item;
  }
  /// add item
  static void AddItem(ContainerType &container, Type *item)
  {
    // adding NULL into the hashmap could cause a crash
    Assert(item);
    container.Add(item);
  }
  static bool DeleteItem(ContainerType &container, NameType name)
  {
    return container.Remove(name);
  }
  /// create an object based on the name
  static Type *Create(NameType name);
};

#include <El/FreeOnDemand/memFreeReq.hpp>

//! material bank

/*!
Result of TexMaterialBank::New or TextureToMaterial should be assigned
to Ref<TexMaterialBank>.
*/

class TexMaterialBank: public BankArrayCached<TexMaterial>
{
  typedef BankArrayCached<TexMaterial> base;
  
  public:
  typedef BankArrayTraits::ContainerType ContainerType;
  
  TexMaterialBank();
  ~TexMaterialBank();

  //! handling of old-style texture-driven materials
  TexMaterial *TextureToMaterial(Texture *tex);

  /// verification there are no problems in the internal structures
  bool CheckIntegrity() const;
  
  void Reload();

  //@{ implementation of MemoryFreeOnDemandHelper
  virtual float Priority() const;
  virtual RString GetDebugName() const;
  //@} implementation of MemoryFreeOnDemandHelper

  /// called when item is moving from the container into the cache
  void OnUnused(TexMaterial *tex);
  
  /// we need to be able to insert materials externally
  TexMaterial *Find(const TexMaterialName &name) {return base::Find(name);}
  /// embedded materials provide a quick way to load objects
  TexMaterial *NewEmbedded(const RStringB &name, SerializeBinStream &f, RStringB filename);
  /// engine sometimes needs a derived material (e.g. local coordinates version of world space material)
  Ref<TexMaterial> NewDerived(const TexMaterial *basic, TexMaterialType *type);
  
  template <class Functor>
  bool ForEach(Functor func) const
  {
    bool ret = ForEachF(func);
    if (ret) return ret;
    
    for (TexMaterial *item = _cache.Last(); item; item=_cache.Prev(item))
    {
      bool ret = func(item,NULL);
      if (ret) return ret;
    }
    return false;
  }
};

extern TexMaterialBank GTexMaterialBank;

#endif
