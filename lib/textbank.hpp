#ifndef _TEXTBANK_HPP
#define _TEXTBANK_HPP

#include <Es/Strings/rString.hpp>
#include <Es/Types/lLinks.hpp>
#include "pactext.hpp"
#include <El/ParamFile/paramFileDecl.hpp>
#include <El/FileServer/fileServer.hpp>
#include <El/Color/colors.hpp>
#include <El/ActiveObject/activeObject.hpp>

class OggDecoder;

//#define MAX_MIPMAPS 10
// that would be enough for 256*256 texture
// last mipmap will have dimensions 4x4

/**
@file generic texture interface
*/



/// information about a probability of one clutter type
struct ClutterProbab
{
  /// random generator threshold
  float _probabilityThold;
  /// config name for the clutter class
  RStringB _name;

  /// optimized access - index into the landscape clutter bank
  /** Not valid after landscape is unloaded - needs to be recreated */
  mutable int _landIndex;
};

TypeIsMovableZeroed(ClutterProbab)

struct SurfaceCharacter: public AutoArray<ClutterProbab>
{
  /// check if some items may be created 
  bool IsEmpty() const {return Size()<=0;}
};

#define AbstractMipmapLevel PacLevelMem

#include <El/FreeOnDemand/memFreeReq.hpp>

const float MaxMipmapWanted = 1e3;

/// texture sampler can be used for CPU sampling of texture
/**
height-maps are first application of this
*/

class TextureSampler: public RefCount, private NoCopy
{
  // sampled textures are loaded in the memory, and should be therefore coarse
  enum {MaxMipmaps=6};
  SRef<ITextureSource> _src;
  PacLevelMem _mipmaps[MaxMipmaps];
  char *_data[MaxMipmaps];
  
  
  public:
  TextureSampler(const char *name);
  ~TextureSampler();
  
  /// pixel bilinear sampling - looped
  Color GetPixelBilinear(float u, float v, int level=0) const;
  
  /// pixel point sampling - looped
  Color GetPixelPoint(float u, float v, int level=0) const;
  
  private:
  static int MipmapSize( PacFormat format, int w, int h );
};

#if _ENABLE_REPORT  
  #define SINFO_RELOAD 1
#else
  #define SINFO_RELOAD 0
#endif

//////////////////////////////////////////////////////////////////////////

#ifndef DECL_ENUM_SUFRACE_SOUNDS
#define DECL_ENUM_SUFRACE_SOUNDS
DECL_ENUM(SufraceSound)
#endif

DEFINE_ENUM_BEG(SufraceSound)
SurfSGrassness, SurfSRockness, SurfSSandness, SurfSMudness, SurfSGravelness, SurfSAsphaltness
DEFINE_ENUM_END(SufraceSound)

template<> const EnumName *GetEnumNames(SufraceSound dummy);

//////////////////////////////////////////////////////////////////////////
enum VisiblePose
{
  VPStand,
  VPKneel,
  VPProne,
  VPDown,
  VPCount
};

/// physical and other simulation attributes for textures
struct SurfaceInfo: public RefCount, private NoCopy
{
  /// filename only mask for texture matching
  RStringB _name;
  /// full wild-card pattern mask for matching
  RStringB _pattern;
  /// name used for loading
  RStringB _entryName;
  /// ID used for scripting functions
  RStringB _surfaceType;
  /// surface bumpiness
  float _roughness;
  /// how much dust is created from the surface
  float _dustness;
  /// bullet speed reduction for 1 m of distance travelled through
  float _bulletPenetrability;
  /// surface thickness
  float _thickness;
  /// surface is water - used for floating
  bool _isWater;
  /// environment sound type
  RStringB _soundEnv;
  /// impact type
  RStringB _impact;
  /// ground clutter character
  SurfaceCharacter _character;
  /// height-map attached with the surface
  Ref<TextureSampler> _heightMap;
  /// sounds from which is surface composed
  AutoArray<float> _surfSounds;
  //! Audible factor
  float _audibility;
  //! visibility factor
  float _visibility[VPCount];

  /// factor used on roadways - how much do we want to reduce ground clutter on terrain at that position?
  float _terrainClutterFactor;
  
  /// hit sound identifier
  enum SoundHitType
  {
    SHDefault,
    SHGround,
    SHGroundHard,
    SHMan,
    SHArmor,
    SHIron,
    SHBuilding,
    SHFoliage,
    SHWood,
    SHGlass,
    SHGlassArmored,
    SHConcrete,
    SHPlastic,
    SHRubber,
    SHMetal,
    SHMetalPlate,
    NSoundHitTypes
  };
  SoundHitType _soundHit;

  #if SINFO_RELOAD
    /// time-stamp of the source file
    QFileTime _timestamp;
  #endif
  
  /// default instance - for returning by reference
  static SurfaceInfo _default;
  
  SurfaceInfo();
  ~SurfaceInfo();
  
  explicit SurfaceInfo(const char *name);
  explicit SurfaceInfo(ParamEntryPar entry);
  void Load(ParamEntryPar entry);
  void Init();
  
  float GetSurfSound(SufraceSound type) const;
};


#if !defined _WIN32
  // no alignment requirements on Xbox
  #define ALIGN_SLIST
  #define AssertSLISTAlign(entry)
#else
  // on PC SLIST_ENTRY must be aligned to MEMORY_ALLOCATION_ALIGNMENT
  #define ALIGN_SLIST __declspec(align(MEMORY_ALLOCATION_ALIGNMENT))
  #define AssertSLISTAlign(entry) ((intptr_t(entry)&(MEMORY_ALLOCATION_ALIGNMENT-1))==0)
#endif

/// class to build a list of textures which requested a loading
ALIGN_SLIST class SListControlled: public SLIST_ENTRY
{
  /// non-zero when we are currently in the list
  LONG _inList;

  public:
  SListControlled()
  {
    _inList = 0;
  }
  ~SListControlled()
  {
    DoAssert(_inList==0);
  }
  
  /// check if we were already added into the list in this frame
  bool AddIntoSList()
  {
    // always set we will be in the list now
    LONG was = InterlockedExchange(&_inList,1);
    // if was not there yet, report
    return was==0;
  }
  bool IsInSList() const {return _inList!=0;}
  
  void RemoveFromSList() {_inList = 0;}
};


namespace enf
{
  struct PhysicsMaterial;
}

/// texture interface - basic mean to access textures

class Texture: public RequestableObject, public LinkBidirWithFrameStore, public SListControlled
{
  // abstract class
  friend class AnimatedTexture;
  friend class AbstractTextBank;

private:
  AnimatedTexture *_inAnimation;
  /** surface info always exists as long as the bank exists */
#if _ENABLE_PHYSICS
  enf::PhysicsMaterial *_physMat;
#endif
  const SurfaceInfo *_info;
  RStringB _name;
  
protected:
  /// some textures (UI) always need to be loaded
  bool _someDataNeeded;
  /// some textures (secondary textures in multi-material) should not be replaced by average color
  bool _someDataWanted;

  /// what mipmap should be loaded and activated for this frame
  float _mipmapWanted;
  /// what mipmap should be loaded, but not activated (preload only)
  float _mipmapPreload;

  /// request textures by AOT/z2
  /** natural for 3D scene, compute mip level from this only once */
  float _wantedAOTPerZ2;
  /** similar to _wantedAOTPerZ2, but used for mipmap loading only, not to activate it */
  float _preloadAOTPerZ2;
  /// how many pixels represent the mipmap which is currently loaded
  /** This should be set during each UseMipmap */
  float _loadedTexDetail;
  
  /// maximum possible texture detail
  /** Set only during init or major change, like setting reset */
  float _maxTexDetail;
  
public:

  Texture();
  virtual ~Texture();

  virtual bool IgnoreLoadedLevels() const { return false;  };
  virtual OggDecoder* getDecoder() const {  return NULL;  };
  
  /// prepare mipmap based on distance and AOT
  void PrepareMipmapLevelScr(float z2, float scrAreaOTex, bool someDataWanted);
  
  /// preload mipmap based on distance and AOT
  void PreloadMipmapLevelScr(float z2, float scrAreaOTex, bool someDataWanted);

  /// preload mipmap based on distance and AOT, return if ready
  bool PreloadAndCheckMipmapLevel(float z2, float areaOTex, bool someDataWanted, float *quality=NULL);

  /// handle texture preloading
  bool PreloadTexture(float z2, float areaOTex, bool someDataWanted, float *quality=NULL);
  
  /// handle texture preparation
  void PrepareTextureScr(float z2, int special, float scrAreaOTex, bool someDataWanted=false)
  {
    // BestMipmap should already be handled earlies
    Assert(!(special&::BestMipmap));
    // if there are no headers yet, there is nothing to prepare
    if (!HeadersReady()) return;
    PrepareMipmapLevelScr(z2,scrAreaOTex,someDataWanted);
  }
  /// handle texture preparation
  void PreloadTextureScr(float z2, int special, float scrAreaOTex, bool someDataWanted=false)
  {
    // BestMipmap should already be handled earlies
    Assert(!(special&::BestMipmap));
    // if there are no headers yet, there is nothing to prepare
    if (!HeadersReady()) return;
    PreloadMipmapLevelScr(z2,scrAreaOTex,someDataWanted);
  }
  
  void ProcessAoTWanted();
  
  /// check if some AOT loading was requested
  bool CheckUseMipmapRequested() const {return _wantedAOTPerZ2>0;}

  float FindMipmapLevel(float areaOTexPerZ2);
  /// generic conversion from mip level to AOT space
  /**
  @return multiplication factor
  */
  static float AOTCoefFromMipmapLevel(float level)
  {
    const float negLog4 = -1.3862943611f; // log(4)
    return exp(level*negLog4);
  }
  /// AOT calculated for a given texture
  /**
  We know mipmap level, we want to know what areaOTex/z2 +  this corresponds to
  This is not a complete calculate  - mipbias and camera dependent parameters are not handled here
  */
  float TexDetailFromMipmapLevel(float level)
  {
    return AOTCoefFromMipmapLevel(level)*AArea();
  }
  
  bool PrepareMipmap0(bool dataNeeded);
  
  /// reset current frame usage data
  void ResetMipmap()
  {
	  _mipmapWanted = MaxMipmapWanted;
	  _mipmapPreload = MaxMipmapWanted;
	  _someDataNeeded = false;
	  _someDataWanted = false;
	  _wantedAOTPerZ2 = 0;
	  _preloadAOTPerZ2 = 0;
  }

  /// select given frame, prefer a loaded one, request the desired one
  Texture *GetAnimationLoaded( int i ) const;
  /// request a mipmap in the animation frame
  bool RequestAnimation(int i, int level) const;
  /// select given frame
  Texture *GetAnimation( int i ) const;
  int AnimationLength() const;
  bool IsAnimated() const {return _inAnimation!=NULL;}
  AnimatedTexture *GetAnimatedTexture() const {return _inAnimation;}

  /// with some implementations some textures may be located in slower memory (AGP vs. VRAM)
  virtual void SetTexturePerformance(int priority){}

  /// reload from underlying data if needed
  virtual void CheckForChangedFile() {}
  //! load all headers so any data-accessing function can be used
  virtual void LoadHeaders() {}
  /// check if texture headers are ready
  /**
  Note: header preload is started once the texture is created
  */
  virtual bool HeadersReady() const {return true;}

  /// texture usage type - ordered by assumed resolution
  enum UsageType {
    TexObject,
    TexLandscape,
    TexCockpit,
    TexUI,
    TexCustom,
    NUsageType
  };
  virtual void SetUsageType( UsageType type ) {}
  virtual void AdjustMaxSize(){}

  /// AWidth*AHeight
  virtual int AArea( int level=0 ) const = 0;
  virtual int AWidth( int level=0 ) const = 0;
  virtual int AHeight( int level=0 ) const = 0;
  /// get mipmap count
  virtual int ANMipmaps() const= 0;
  /// get index of the best supported by engine or HW
  virtual int BestMipmap() const = 0;
  virtual AbstractMipmapLevel &AMipmap( int level ) = 0;
  virtual const AbstractMipmapLevel &AMipmap( int level  ) const = 0;
  virtual void ASetNMipmaps( int n ) = 0;
  virtual Color GetPixel( int level, float u, float v ) const = 0;
  /// check if raw data are dyn. range compressed
  virtual bool IsDynRangeCompressed() const {return false;}
  /// get rectangle data
  /**
  Not all format conversions are possible. Check PacLevelMem::LoadPaaDXT, PacLevelMem::LoadPaa
  */
  virtual bool GetRect(
    int level, int x, int y, int w, int h, PacFormat format, void *data
  ) const
  {
    return false;
  }
  /// async request for GetRect
  virtual bool RequestGetRect(int level, int x, int y, int w, int h) const
  {
    return true;
  }
  
  virtual Color GetColor() const = 0;
  //! Returns format of the texture
  virtual PacFormat GetFormat() const = 0;
  /// needs alpha testing
  virtual bool IsTransparent() const = 0;
  /// needs alpha blending
  virtual bool IsAlpha() const = 0;
  /// needs alpha blending, alpha is low and texture is quite invisible
  virtual bool IsAlphaNonOpaque() const = 0;
  virtual QFileTime GetTimestamp() const {return 0;}

  //@{ UV clamping controls
  virtual int GetClamp() const = 0;
  virtual void SetClamp(int clampFlags) = 0;
  //@}

  virtual void SetMipmapRange( int min, int max ){}

  virtual bool VerifyChecksum( const MipInfo &mip ) const = 0; // verify consistency

  const char *Name() const {return _name;}
  const RStringB &GetName() const {return _name;}
  void SetName(RStringB name, bool caseSensitive = false);

  //TextureType Type() const {return _tt;}
  //! Method to return the texture type
  virtual TextureType Type() const = 0;

  /// max. filter which has sense for given texture  
  virtual TexFilter GetMaxFilter() const = 0;

  //@{ access to simulation attributes
  float Roughness() const {return _info->_roughness;}
  float Dustness() const {return _info->_dustness;}
  bool IsWater() const {return _info->_isWater;}

#if _ENABLE_PHYSICS
  enf::PhysicsMaterial* GetPhysMaterial() const {return _physMat;}
#endif
  const SurfaceInfo &GetSurfaceInfo() const {return *_info;}
  RStringB GetSoundEnv() const {return _info->_soundEnv;}
  const SurfaceCharacter &GetCharacter() const {return _info->_character;}
  //@}

  /// LinkBidirWithFrameStore implementation
  virtual size_t GetMemoryControlled() const;
  #if _DEBUG
  virtual void OnUsed() const;
  #endif
  /// RefCount implementation
  virtual void OnUnused() const;

  //! Binds the texture with the push-buffer to ensure we won't release it while it is being used
  virtual void BindWithPushBuffer() {}

#ifdef TRAP_FOR_0x8000
  virtual bool TrapFor0x8000(const char *str) const { (void *)str; return false; /*continue ForEach*/ }
#endif
};

// each texture bank knows type of its textures
// overloads function "Load" with different return type (if necessary")
// casts parameter of function UseMipmap to know type - this is potentially unsafe


class QFBank;

#include <El/FreeOnDemand/memFreeReq.hpp>

/// multiple interfaces instanced
//MemHandlerInstance(TexVidMem)
MemHandlerInstance(TexCache)

enum TexPriority
{
  TexPriorityVeryLow=10,
  TexPriorityLow=50,
  TexPriorityNormal=100,
  TexPriorityUILow=200,
  TexPriorityUIHigh=300,
  TexPriorityCritical=1000,
};

/// lock free concurrent structure allowing gathering items
/** Win32 SList version */
template <class Type>
class LockFreeGrowingList: private SListMT<Type>
{
  typedef SListMT<Type> base;
  public:
  LockFreeGrowingList()
  {
  }
  /// MT safe adding another element
  void Add(Type *data)
  {
    // TODO:MC: remove AddRef from here, 
    data->AddRef(); // make sure item is not destroyed while in the list
    AssertSLISTAlign((SLIST_ENTRY *)data);
    this->Push(data);
  }
  
  void GetAll(LockFreeGrowingList &tgt)
  {
    LockFreeGrowingList temp;
    GetAllReversed(temp);
    temp.GetAllReversed(tgt);
  }
  void GetAllReversed(LockFreeGrowingList &tgt) {base::GetAllReversed(tgt);}
  void GetAllReversed(LockFreeGrowingList &tgt, Type *skip){base::GetAllReversed(tgt,skip);}
  
  using base::CheckIntegrity;


  void Delete(Type *skip)
  {
    LockFreeGrowingList temp;
    // POP everything (in the reversed order), skipping the item we want to delete
    GetAllReversed(temp,skip);
    // and PUSH everything, reversing it one more time
    temp.GetAllReversed(*this);
  }
  
  Ref<Type> Get()
  {
    Ref<Type> ret;
    // a reference was held by the list, and we now take ownership of it, no need to call AddRef / Release
    ret.SetRef(base::Pop());
    return ret;
  }
  
  void Clear()
  {
    Type *entry = base::Flush();
    DoAssert(!entry);
    (void)entry;
  }
};

/// texture manager interface

class AbstractTextBank: public MHInstanceTexCache
{
  friend class Texture;

  protected:
  
  RefArray<SurfaceInfo> _surfaces;
  RefArray<AnimatedTexture> _animatedTextures;

  FramedMemoryControlledList<Texture> _cache;

  LockFreeGrowingList<Texture> _loadRequested;
  
  float _textureMipBias;
  float _textureMipBiasMin;
  float _textureMipBiasAOTCoef;
  
  public:
    
  AnimatedTexture *LoadAnimated(RStringB name, bool caseSensitive = false);
  void DeleteAnimated( AnimatedTexture *texture );
  void DeleteAllAnimated();

  virtual Ref<Texture> CreateTexture(ITextureSource *source) = 0;


  /// before loading from the file try searching cache
  Ref<Texture> LoadCached(RStringB name);

  /// if the texture is cached, flush it
  bool FlushCached(RStringB name);

  /// make sure no texture from the bank is cached
  void FlushBankFromCache(QFBank *bank);
  
  virtual void MakePermanent(Texture *tex){}
  
  virtual Ref<Texture> Load(RStringB text, bool caseSensitive = false) = 0;

  /// request - we want this mipmap
  /**
  @param priority lower value means texture is less important
  */
  virtual MipInfo UseMipmap(Texture *texture, int level, int levelTop, int priority=TexPriorityNormal) = 0; 
  /// get what we have
  virtual MipInfo UseMipmapLoaded(Texture *texture) = 0; // request - we want this some mipmap

  virtual void Compact() = 0;
  virtual void Preload() {}
  virtual void FlushTextures(bool deep, bool allowAutoDeep) {} // flush temporary data
  /// flush cached texture 
  virtual bool FlushTexture(const char *name) {return true;}

  /// get list of all textures
  virtual void GetTextureList(LLinkArray<Texture> &list) = 0;
  
  //@{ texture quality - value engine dependent
  virtual int GetTextureQuality() const {return 0;}
  virtual int GetMaxTextureQuality() const {return 2;}
  virtual void SetTextureQuality(int value) {}
  //@}
  
  virtual int GetTextureMemory() const {return 0;}
  virtual int GetMaxTextureMemory() const {return 2;}
  virtual void SetTextureMemory(int value) {}
  
  /// suspend texture until the variable is out of the scope
  class TextureSuspendScope: private ::NoCopy
  {
    Ref<Texture> _texture;
    AbstractTextBank *_bank;

    public:    
    TextureSuspendScope(AbstractTextBank *bank, const char *tex)
    :_bank(bank),_texture(bank->SuspendTexture(tex))
    {
    }
    
    ~TextureSuspendScope()
    {
      _bank->ResumeTexture(_texture);
    }
  };
  
  virtual Ref<Texture> SuspendTexture(const char *name) {return NULL;}
  virtual void ResumeTexture(Texture *tex) {}

  virtual void CheckForChangedFiles() {}

  virtual void AllocateTextureMemory() {}
  virtual void FlushBank(QFBank *bank) = 0;
  
  virtual void StartFrame();
  virtual void FinishFrame();
  virtual void PerformMipmapLoading();

  virtual bool VerifyChecksums() {return true;}

  static int AnimatedName( const char *name, char *prefix, char *postfix );
  static int AnimatedNumber( const char *name );

  float GetTextureMipBias() const {return _textureMipBias;}
  void SetTextureMipBias(float bias);
  float GetTextureMipBiasMin() const {return _textureMipBiasMin;}
  void SetTextureMipBiasMin(float bias) {_textureMipBiasMin=bias;}
  /// return AOT adjustment coef which corresponds to the current mip bias;
  float GetTextureMipBiasAOTCoef() const {return _textureMipBiasAOTCoef;}

  protected:
  /// full wildcard matching
  int FindFileName(const char *name) const;
  /// simple filename pattern matching ('?')
  int FindPath(const char *name) const;
  /// first check patterns, then names
  const SurfaceInfo &GetSurface(const char *name) const;

public:
  
#if _ENABLE_PHYSICS
	enf::PhysicsMaterial* GetPhysMaterial(const char *name) const;
#endif

  AbstractTextBank();
  /// called once file system is ready
  virtual void Init();
  
  virtual ~AbstractTextBank();

  /// surface property info
  SurfaceInfo *NewSurfaceInfo(const char *name);
  int NSurfaceInfos() const {return _surfaces.Size();}
  const SurfaceInfo *GetSurfaceInfo(int i) const {return _surfaces[i];}

  #if _ENABLE_CHEATS
    virtual RString GetStat(int statId, RString &statVal, RString &statVal2);
  #endif

  //@{ implementation of MemoryFreeOnDemandHelper
  virtual float PriorityTexCache() const;
  virtual RString GetDebugNameTexCache() const;
  virtual size_t MemoryControlledTexCache() const;
  virtual size_t MemoryControlledRecentTexCache() const;
  virtual void MemoryControlledFrameTexCache();
  virtual size_t FreeOneItemTexCache();
  //@} implementation of MemoryFreeOnDemandHelper
  virtual IMemoryFreeOnDemand *GetProgressFreeOnDemandInterface() {return NULL;}
  
  /// helper for Texture::OnUnused
  void OnUnused(Texture *tex);
  /// release all cached textures
  void ClearTexCache();
};

Texture *GlobPreloadTexture( RStringB name );
Ref<Texture> GlobLoadTexture(RStringB name, bool caseSensitive = false);
Ref<Texture> GlobLoadTextureUI(RStringB name);
AnimatedTexture *GlobLoadTextureAnimated(RStringB name, bool caseSensitive = false);

Ref<Texture> GlobLoadTexture(ITextureSource *source);

class AnimatedTexture: public RefArray<Texture>,public RefCount
{
  AbstractTextBank *_bank;

  public:
  AnimatedTexture( AbstractTextBank *bank );
  ~AnimatedTexture();

  void Remove( Texture *text );
  const char *Name() const; // name of first texture
  RStringB GetName() const; // name of first texture

  private:
  void operator = ( const AnimatedTexture &src );
  AnimatedTexture( const AnimatedTexture &src );
};

extern LLink<Texture> DefaultTexture;

#endif

