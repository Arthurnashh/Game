#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TXTD3D9_HPP
#define _TXTD3D9_HPP

#if !_DISABLE_GUI

#include "../types.hpp"
#include <El/Math/math3d.hpp>

#include <Es/Memory/fastAlloc.hpp>
#include <Es/Containers/cachelist.hpp>
#include <es/Framework/appFrame.hpp>
#include <El/Color/colors.hpp>
#include <El/Multicore/multicore.hpp>

#include "../pactext.hpp"
#include "../textbank.hpp"
#include "../engine.hpp"

#include "d3d9defs.hpp"

#include <Es/Memory/normalNew.hpp>

class TextureD3D9;

/// item describing what element is held and can be evicted from the cache
class HMipCacheD3D9: public LinkBidirWithFrameStore
{
  public:
  /// which texture
  TextureD3D9 *texture;
  /// which mipmap level
  /** Several mipmaps may be stored from the same texture */
  int level;

  size_t GetMemoryControlled() const;

  #if _ENABLE_REPORT
  ~HMipCacheD3D9()
  {
    // destructor must never remove from the list - that would break thread safety
    DoAssert(!IsInList());
    DoAssert(GetStoredMemoryControlled()==0);
  }
  #endif
  
  USE_FAST_ALLOCATOR;
};

#include <Es/Memory/debugNew.hpp>

template <class Type>
struct FramedMemoryControlledListTraitsSRef
{
  static void AddRef(Type *item) {}
  static void Release(Type *item){delete item;}
};

template <>
struct FramedMemoryControlledListTraits<HMipCacheD3D9>: public FramedMemoryControlledListTraitsSRef<HMipCacheD3D9>
{
};

/**
track texture mipmaps suitable for discarding

*/

class D3DMipCacheRoot9: public FramedMemoryControlledList<HMipCacheD3D9>
{
  typedef FramedMemoryControlledList<HMipCacheD3D9> base;
  
};

class TextBankD3D9;
/** 
We do not provide any real memory to MemoryFreeOnDemandHelper, but we need to be connect to it
so that it calls our Frame function 
*/

class D3DMipCacheRoot9FreeOnDemand: public MemoryFreeOnDemandHelper
{
  TextBankD3D9 &_bank;
  
  public:
  D3DMipCacheRoot9FreeOnDemand(TextBankD3D9 &bank):_bank(bank){}
    
  //@{ MemoryFreeOnDemandHelper implementation
  virtual size_t FreeOneItem();
  virtual float Priority() const;
  virtual size_t MemoryControlled() const;
  virtual size_t MemoryControlledRecent() const;
  virtual RString GetDebugName() const;
  virtual void MemoryControlledFrame();
  //@}
  
};

struct TEXTUREDESC9
{
  int w,h;
  int nMipmaps;
  D3DFORMAT format;
  D3DPOOL pool;
  DWORD usage;
};

class EngineDD9;

#if ENCAPSULATE_TEXTURE_REF
  /// encapsulation of texture allocations for Xbox
  struct TextureHandle9: public RefCount
  {
    mutable ComRef<IDirect3DTexture9> _surface;
    
    TextureHandle9()
    {
    }
    /// assume a permanent surface
    TextureHandle9(const ComRef<IDirect3DTexture9> &surface)
    :_surface(surface)
    {
    }
    
    ~TextureHandle9();
  };
#endif


/// information about a surface allocated with D3D
class SurfaceInfoD3D9
{
  private:

  #if !ENCAPSULATE_TEXTURE_REF
    ComRef<TextureHandle9> _surface;
  #else
    Ref<TextureHandle9> _surface;
    
    void Copy(const SurfaceInfoD3D9 &src);
  #endif
  
  static int _nextId;
  int _id;
  
  public:
  int _totalSize; // expected size for allocation
  int _usedSize; // actually allocated size
  int _w,_h;
  int _nMipmaps;
  //PacFormat _format;
  D3DFORMAT _d3dFormat;

#if _HELISIM_RENDER_TO_TEXTURE
  bool _isR2T;
#endif

  int SizeExpected() const {return _totalSize;}
  int SizeUsed() const {return _usedSize;}
  int Slack() const {return _usedSize-_totalSize;}

  int GetCreationID() const {return _id;}
  
  public:
  
#if TRACK_TEXTURES
    SurfaceInfoD3D9(){}
    SurfaceInfoD3D9(const SurfaceInfoD3D9 &src);
    void operator = (const SurfaceInfoD3D9 &src);
#endif

  static int CalculateSize(
    MyD3DDevice &dDraw, const TEXTUREDESC9 &desc, PacFormat format,
    int totalSize=-1
  );
  HRESULT CreateSurface(
    EngineDD9 *engine, MyD3DDevice &dDraw, const TEXTUREDESC9 &desc, PacFormat format,
    int totalSize=-1
  );
  void Free(const char *name, bool lastRef, int refValue=0 );
  // TODO: use ComRef more consistently
  // add AddRef to ComRef contructor?
  // or use Ref for COM types
  #if ENCAPSULATE_TEXTURE_REF
  IDirect3DTexture9 *GetSurface() const {return _surface ? _surface->_surface.GetRef() : NULL;}
  TextureHandle9 *GetHandle() const {return _surface;}
  bool IsNull() const {return _surface==NULL;}
  #else
  IDirect3DTexture9 *GetSurface() const {return _surface;}
  IDirect3DTexture9 *GetHandle() const {return _surface;}
  bool IsNull() const {return _surface.IsNull();}
  #endif
};

TypeIsMovableZeroed(SurfaceInfoD3D9);

struct D3DSurfaceToDestroy
{
  int _id;
  ComRef<IDirect3DTexture9> _surface;
};

TypeIsMovableZeroed(D3DSurfaceToDestroy);

/* on PC there is no way to copy smaller mip-maps of the texture, therefore we keep separate chains for smaller mip-maps */
/* TODO:
But it would be possible to employ a different scheme. Smaller mipmaps could be loaded only in a system memory,
as one chain of separate surfaces. Whenever we would be downgrading, we would copy the sysmem into VIDMEM and
discard the biggest surface.

This would mean somewhat larger SYSMEM consumption, reduced VIDMEM, and reduced overall usage.
*/
# define SMALL_MIPS_READY 1

class D3DSurfaceDestroyer9
{
  // manage order of batch of destroyed surfaces
  // DX memory manager releases surfaces much faster
  // when released in reverse order of creation
  AutoArray<D3DSurfaceToDestroy> _surfaces;
  public:
  D3DSurfaceDestroyer9();
  ~D3DSurfaceDestroyer9();
  void Add( const SurfaceInfoD3D9 &surf );
  void DestroyAll();
};


//class MipmapLevelD3D: public PacLevelMem {};

#include <Es/Memory/normalNew.hpp>

#define ASSERT_INIT() Assert(_initialized);

#define MAX_MIPMAPS 12

/// class to build a list of textures to be discarded
class TextureLRUItem : public TLinkBidir<4>
{
};

struct HeapTraitsLoadingJob
{
  typedef LLink<TextureD3D9> Type;
  static bool IsLess(const Type &a, const Type &b);
};

//! single texture (D3D)

class TextureD3D9: public Texture //, public TextureLRUItem
{
  typedef Texture base;

  friend class TextBankD3D9;
  friend class EngineDD9;
  friend class HMipCacheD3D9;
  friend struct HeapTraitsLoadingJob;

private:
  //PacPalette _pal;

  SRef<ITextureSource> _src; //!< texture source provider

  bool _initialized;
  bool _permanent;
  bool _initializeRequested;
  
  /// performance requirements (priority)
  int _performance;
  int _maxSize;
  /// landscape textures have different limit than object...
  UsageType _usageType;

  // this information is same for all mipmaps, but is also copied in mipmap header
  int _nMipmaps;
  //! List of mipmaps (the first one with index 0 is the biggest one)
  PacLevelMem _mipmaps[MAX_MIPMAPS];
  /// video memory copy
  SurfaceInfoD3D9 _surfaces[MAX_MIPMAPS]; 

  class LoadLevelsJob;
  /// only one loading job may be active for a single texture
  SRef<LoadLevelsJob> _loadingJob;

#ifdef TRAP_FOR_0x8000
#if 1
  int _dummyInts[256];
#endif
#endif

  /// is given level present in the cache?
  InitPtr<HMipCacheD3D9> _surfCache[MAX_MIPMAPS];

  /// First level which is small enough to be used
  signed char _largestUsed;
  /// First level which is small enough to be considered atomic (either we have at least this level, or the texture is not loaded at all)
  signed char _smallLevel;
  /// The finest level loaded
  signed char _levelLoaded;
  /// Level which is marked in the cache as active and can be safely used for rendering
  /** note: this is not updated when texture is not used, and still points to the last assigned value */
  signed char _levelActive;
  /// Is currently used for background operation - do not release
  signed char _inUse;
  /// levelNeeded already included mipbias
  float _levelNeededThisFrame;
  float _levelNeededLastFrame;


  TextureHandle9 *GetBigSurface() const
  {
    #if SMALL_MIPS_READY
    // we should use only a mipmap which is known to be registered as used
    // otherwise we risk stall on release of the fine mipmap
    Assert(_levelActive>=_levelLoaded);
    if (_levelActive >= _nMipmaps) return NULL;
    Assert(_surfaces[_levelActive].GetSurface());
    return _surfaces[_levelActive].GetHandle();
    #else
    // when storing only one mipmap we have no choice and need to use the one we have
    // TODO: solve possible GPU stalls
    Assert(_levelActive>=_levelLoaded);
    if (_levelActive >= _nMipmaps || _levelLoaded>=_nMipmaps) return NULL;
    Assert(_surfaces[_levelLoaded].GetSurface());
    return _surfaces[_levelLoaded].GetHandle();
    #endif
  }

  float LevelNeeded() const
  {
    return
    (
      _levelNeededThisFrame<_levelNeededLastFrame 
      ? _levelNeededThisFrame : _levelNeededLastFrame
    );
  }

  /// Copies surface by locking and unlocking
  static bool CPUCopyRects(IDirect3DSurface9 *srcSurf, IDirect3DSurface9 *destSurf);
  
public:
  TextureD3D9();
  ~TextureD3D9();
  
  bool IgnoreLoadedLevels() const { return (_src!=NULL)  && _src->IgnoreLoadedLevels();  };
  OggDecoder* getDecoder() const {  return ( (_src!=NULL)  ? _src->getDecoder() : NULL ) ;  };

  void OnUnused() const;

  //int LoadPalette( QIStream &in );
  //static int SkipPalette( QIStream &in ) {return PacPalette::Skip(in);} // load palette
  
  /// Returns description of the texture acquired from the level "levelMin"
  /*!
    \param ddsd Texture description to return
    \param levelMin level to acquire texture from
    \param enableDXT Flag to determine DXT2-5 can be used
  */
  void InitDDSD(TEXTUREDESC9 &ddsd, int levelMin, bool enableDXT);
#if !defined(_X360SHADERGEN)
  /// copy texture from SYSMEM to DEFAULT
  static void CopyToVRAM(SurfaceInfoD3D9 &surface, const SurfaceInfoD3D9 &sys);
#endif
  /// discard levels that are not needed, reuse original surface
  bool ReduceTexture(SurfaceInfoD3D9 &surf, int mipImprovement);

  bool ReduceTextureLevel(int tgtLevel);
  bool ReduceTextureLevel(SurfaceInfoD3D9 &surf, int needed, int mipImprovement);

  int LoadLevels(int levelMin, bool ignoreLoadedLevels = false); // load to VRAM


  /// request texture loading, return true if texture is ready
  bool RequestLevelsSys(int levelMin, int priority);
  /// request texture loading, return true if texture is ready
  bool RequestLevels(int levelMin, int priority);
  /// Returns permanent default texture - will always return non NULL value
  TextureD3D9 *GetDefaultTexture() const;

  __forceinline TextureHandle9 *GetHandle() const {return GetBigSurface();}

  private:
  
  void MemoryReleased(int from, int to); // mark as released

  int TotalSize( int levelMin ) const;

public:

  /// verify internal object state is OK
  bool CheckIntegrity() const;
  
  int TotalVRAMUsed() const {return TotalSize(_levelLoaded);}

  int ReleaseMemory( SurfaceInfoD3D9 &surf, bool store=false, D3DSurfaceDestroyer9 *batch=NULL ); // move to reusable

  int ReleaseSystem(SurfaceInfoD3D9 &surface, bool store=false, D3DSurfaceDestroyer9 *batch=NULL); // move to reusable
  int ReleaseMemory( bool store=false, D3DSurfaceDestroyer9 *batch=NULL ); // move to reusable
  int ReleaseLevelMemory( int from, int to, bool store=false, D3DSurfaceDestroyer9 *batch=NULL ); // move to reusable
  void ReuseMemory( SurfaceInfoD3D9 &surf ); // reuse immediatelly

  virtual PacFormat GetFormat() const {return _src ? _src->GetFormat() : PacUnknown;}
  virtual bool IsAlpha() const {ASSERT_INIT();return _src && _src->IsAlpha();}
  virtual bool IsAlphaNonOpaque() const {ASSERT_INIT();return _src && _src->IsAlphaNonOpaque();}
  virtual bool IsTransparent() const {ASSERT_INIT();return _src && _src->IsTransparent();}
  QFileTime GetTimestamp() const {ASSERT_INIT();return _src ? _src->GetTimestamp() : 0;}

  virtual int GetClamp() const {return _src ? _src->GetClamp() : 0;}
  virtual void SetClamp(int clampFlags);
  
  /// adjust max. size based on usage type
  void AdjustMaxSize();
  void SetUsageType( UsageType type );
  void SetNMipmaps( int n);

  bool VerifyChecksum( const MipInfo &mip ) const;
  
  virtual TextureType Type() const;

  virtual TexFilter GetMaxFilter() const;

  //int Load( int level );
  
  void SetMipmapRange( int min, int max );

  /// do as little preparation as possible
  int Init(const char *name, bool caseSensitive = false);

  /// initialize based on known source
  int Init(ITextureSource *source);

private:
  /// initialize based on source (_src)
  void DoInitSource();
    
  //! do real preparation - is called before any real data is used
  void DoLoadHeaders(bool canIgnore=true);

  /// advertise the resource is ready
  void MarkInitialized()
  {
    // before we advertise the texture is initialized, make sure other threads can really see it
    MemoryPublish();
    _initialized = true;
    _initializeRequested = true;
  }


public:
  /// non-virtual implementation of HeadersReady to allow inlining
  bool HeadersReadyFast() const
  {
    bool ret = _initialized;
    // make sure all other writes really finish before we start reading
    MemorySubscribe();
    return ret;
  }
  /// check if headers are ready
  virtual bool HeadersReady() const
  {
    return HeadersReadyFast();
  }
  //! ask file server to preload headers from file
  virtual void PreloadHeaders();

  virtual void RequestDone(RequestContext *context, RequestResult result);
  virtual int ProcessingThreadId() const;
  
  #if DEBUG_PRELOADING
  virtual bool DebugMe() const;
  #endif
  
  //! undo what was done by DoLoadHeaders or any actual loading
  virtual void DoUnloadHeaders();
  
  //! load header if necessary (non-virtual)
  void LoadHeadersNV() const
  {
    if (HeadersReadyFast()) return;
    const_cast<TextureD3D9 *>(this)->DoLoadHeaders();
  }

  void LoadHeadersServantCallback() const
  {
    const_cast<TextureD3D9 *>(this)->DoLoadHeaders(false);
  }

  void LoadHeaders();

  void CheckForChangedFile();
  void FlushHandles();

  const PacLevelMem *Mipmap( int level ) const
  {
    ASSERT_INIT();
    Assert(level < MAX_MIPMAPS);
    return &_mipmaps[level];
  }

  const AbstractMipmapLevel &AMipmap( int level ) const
  {
    ASSERT_INIT();
    Assert(level < MAX_MIPMAPS);
    return _mipmaps[level];
  }
  AbstractMipmapLevel &AMipmap( int level )
  {
    ASSERT_INIT();
    Assert(level < MAX_MIPMAPS);
    return _mipmaps[level];
  }
  int NMipmaps() const
  {
    ASSERT_INIT();
    return _nMipmaps;
  }
  //const PacPalette &GetPalette() const {return _pal;}

  int ANMipmaps() const {ASSERT_INIT();return _nMipmaps;}
  int BestMipmap() const {ASSERT_INIT();return _largestUsed;}
  void ASetNMipmaps( int n ); // {_nMipmaps=n;}
  int AArea( int level=0)const {LoadHeadersNV();Assert(level < MAX_MIPMAPS);return _mipmaps[level]._w*_mipmaps[level]._h;}
  int AWidth( int level=0 ) const {LoadHeadersNV();Assert(level < MAX_MIPMAPS);return _mipmaps[level]._w;}
  int AHeight( int level=0 ) const {LoadHeadersNV();Assert(level < MAX_MIPMAPS);return _mipmaps[level]._h;}

  virtual Color GetPixel( int level, float u, float v ) const;
  virtual bool IsDynRangeCompressed() const {return _src && _src->GetMaxColor()!=PackedWhite;}
  virtual bool GetRect(
    int level, int x, int y, int w, int h, PacFormat format, void *data
  ) const;
  virtual bool RequestGetRect(int level, int x, int y, int w, int h) const;
  
  /// update _loadedTexDetail based on _levelLoaded
  void UpdateLoadedTexDetail(int levelLoaded);
  
  void UpdateMaxTexDetail();

  int LoadingJobFrame() const;

  /// 0 = default performance, 100, 200 = increased performance
  void SetTexturePerformance(int performance);

  Color GetColor() const;
  Color GetMaxColor() const;

  int Width( int level ) const {ASSERT_INIT();Assert(level < MAX_MIPMAPS);return _mipmaps[level]._w;}
  int Height( int level ) const {ASSERT_INIT();Assert(level < MAX_MIPMAPS);return _mipmaps[level]._h;}

  void CacheUse(TextBankD3D9 &bank, int level, int active);

  __forceinline void AddUse() {_inUse++;}
  __forceinline void ReleaseUse() {_inUse--;}

#ifdef TRAP_FOR_0x8000
  virtual bool TrapFor0x8000(const char *str) const;
#endif

  USE_FAST_ALLOCATOR
};

//! object that can be used to scope lock textures
class ScopeUseTexture9
{
  TextureD3D9 *_inUse;

  public:
  //! construct - enter lock
  ScopeUseTexture9(TextureD3D9 *texture)
  { 
    if (texture)
    {
      _inUse = texture;
      _inUse->AddUse();
    }
    else
    {
      _inUse = NULL;
    }
  }
  //! destruct - leave lock
  ~ScopeUseTexture9()
  {
    if (_inUse) _inUse->ReleaseUse();
  }
  //! copy - duplicate lock
  ScopeUseTexture9(const ScopeUseTexture9 &src)
  :_inUse(src._inUse)
  {
    if (_inUse) _inUse->AddUse();
  }
  //! no assignement possible
  void operator =(const ScopeUseTexture9 &src)
  {
    if (src._inUse) src._inUse->AddUse();
    if (_inUse) _inUse->ReleaseUse();
    _inUse = src._inUse;
  }
};

//! object that can be used to scope lock textures
class ScopeInUse9
{
  signed char &_inUse;

  public:
  //! construct - enter lock
  ScopeInUse9(signed char &inUse)
  :_inUse(inUse)
  {
    _inUse++;
  }
  //! destruct - leave lock
  ~ScopeInUse9()
  {
    _inUse--;
  }
  //! copy - duplicate lock
  ScopeInUse9(const ScopeInUse9 &src)
  :_inUse(src._inUse)
  {
    _inUse++;
  }
  private:
  //! no assignement possible
  void operator =(const ScopeInUse9 &src);
};

#include <Es/Memory/debugNew.hpp>

#include <El/FreeOnDemand/memFreeReq.hpp>

template <>
struct MapClassTraits< LLink<TextureD3D9> >: public DefMapClassTraits< LLink<TextureD3D9> >
{
  /// key type
  typedef RStringB KeyType;
  /// calculate hash value
  static unsigned int CalculateHashValue(KeyType key)
  {
    return CalculateStringHashValue(key);
  }

  /// compare keys, return zero when equal, non-zero otherwise (this is all hashmap needs)
  static int CmpKey(KeyType k1, KeyType k2)
  {
    return k1!=k2;
  }

  static int CoefExpand() {return DefCoefExpand;}

  /// get a key for given a item
  static KeyType GetKey(const LLink<TextureD3D9> &item)
  {
    if (item.IsNull()) return KeyType();
    return item->GetName();
  }

  struct DeleteNullTexture
  {
    bool operator () (const LLink<TextureD3D9> &tex) const
    {
      return tex.IsNull();
    }
  };
  
  /// cleanup zombies - handle NULL links
  static int CleanUp(LLinkArray<TextureD3D9> &array)
  {
    int oldCount = array.Size();
    // remove any targets with idExact = NULL
    array.ForEachWithDelete(DeleteNullTexture());
    //Assert(oldCount== array.Size());
    // return how many items did we remove
    return oldCount- array.Size();
  }
};

class TimeManager;

//! texture manager (D3D)

class TextBankD3D9: public RefCount, public AbstractTextBank
{
  typedef AbstractTextBank base;

  public:
  struct TextureLimits
  {
    /// maximum size
    int maxSize;
    /// number of mipmaps to skip
    int skipMips;
  };

  private:
	friend class TextureD3D9;
	
	int _maxTextureMemory; // initial memory free for textures
	/// common limit for textures + vertex buffers
	int _limitAlocatedVRAM;
	/// we may know from experience some VRAM must be left
	int _reserveTextureMemory;
	/// updated every time RecalculateTextureMemoryLimits is called
	int _freeTextureMemory; 

  /// what is the minimal reasonable texture size
  int _minTextureSize;

  typedef MapStringToClass< LLink<TextureD3D9>, LLinkArray<TextureD3D9> > TextureList;
  TextureList _texture;
  Ref<TextureD3D9> _white; ///< required for pixel shader - NULL texture is black
  Ref<TextureD3D9> _zero; ///< used as default for mask
  Ref<TextureD3D9> _black;  // required for Ti pixel shader when no Ti texture is defined
  Ref<TextureD3D9> _random; ///< small texture with random numbers in alpha
  Ref<TextureD3D9> _defNormMap; ///< default normal map
  Ref<TextureD3D9> _defDetailMap; ///< default detail map
  Ref<TextureD3D9> _defTI; ///< default TI map
  Ref<TextureD3D9> _defTIMan; ///< default TI map
  Ref<TextureD3D9> _defMacroMap; ///< default macro map
  Ref<TextureD3D9> _defSpecularMap; ///< default specular map
  Ref<TextureD3D9> _defDTSpecularMap; ///< default detail specular map

  template <class Op>
  void ForEachDefTexture(const Op &op)
  {
    op(_white);
    op(_zero);
    op(_black);
    op(_random);
    op(_defNormMap);
    op(_defDetailMap);
    op(_defTI);
    op(_defTIMan);
    op(_defMacroMap);
    op(_defSpecularMap);
    op(_defDTSpecularMap);
  }

  RefArray<TextureD3D9> _permanent; ///< texture loaded permanently
  
  CriticalSection _loadingJobsCS;
  HeapArray<LLink<TextureD3D9>, MemAllocD, HeapTraitsLoadingJob >_loadingJobs;

  #if _ENABLE_CHEATS
  struct StressInfo
  {
    ComRef<IDirect3DTexture9> _tex;
    int _size;
    
    StressInfo(){}
    StressInfo(const ComRef<IDirect3DTexture9> &tex, int size)
    :_tex(tex),_size(size)
    {
    }
    
    ClassIsMovableZeroed(StressInfo)
  };
  /// memory stress testing - surfaces doing nothing but eating the memory
  AutoArray<StressInfo> _stressTest;
  /// track total size of stress test surfaces
  int _stressTestTotalSize;
  /// stress testing can continuously create and destroy random sized textures
  /**
  Negative value means we prefer destroying textures
  Positive value means we prefer creating textures
  The bigger number, the bigger the change per frame is
  */
  int _texStressMode;
  
  /// virtual memory stress test
  AutoArray<void *> _stressTestVM;
  #endif

  
  int _limitSystemTextures;
  // system texture bank
  AutoArray<SurfaceInfoD3D9> _freeSysTextures; // free system surfaces

  AutoArray<SurfaceInfoD3D9> _freeTextures; // free VRAM surfaces
  int _freeAllocated; //!< allocated in free vidmem heap (_freeTextures)
  int _totalAllocated; //!< allocated in vidmem heap (_freeTextures and _texture)
  int _totalSlack; //!< slack in vidmem heap (_freeTextures and _texture)
  /// total amout of VRAM freed in this frame
  /** used in debugging to detect suspicious extensive freeing */
  int _freeVRAMTurnOver;
  
  /// is FinishFrame still expected to be called?
  bool _frameNotFinishedYet;
  
  EngineDD9 *_engine;
  int _systemAllocated; //!< allocated in system heap (_freeSysTextures and _texture)

  /// LRU list of mipmaps to be discarded  
  D3DMipCacheRoot9 _used;
  
  D3DMipCacheRoot9FreeOnDemand _freeOnDemand;
  
  /// keep information about what texture is to be discarded
  
  /*
  TListBidir<TextureD3D9,TextureLRUItem> _recentlyUsed;
  */
  
  int _thisFrameAlloc; // VRAM allocations/deallocations (count)

  /// currently used texture quality  
  int _textureQuality;
  /// max. texture quality as currently possible based on video card capabilities
  int _maxTextureQuality;
  
  /// currently used texture memory  
  int _textureVRAM;
  /// max. texture quality as currently possible based on video card capabilities
  int _maxTextureVRAM;
  
  /// protect texture memory allocation
  mutable CriticalSection _textureMemoryCS;
  /// ID of thread currently holding _textureMemoryCS
  mutable int _textureMemoryThread;
  /// ID of the thread used for texture management
  int _textureThread;
  
  
  
  TimeManager &GetTexTimeManager();

  /// check for changed files at the next frame was requested
  bool _checkForChangedFiles;
  bool _flushHandles;
  
  static const int _textureLimit[];
  static const float _textureMipBiasQuality[];
  static const TextureLimits _textureMaxSize[][Texture::NUsageType];
public:
  //! Constructor
  /*!
    \param engine The engine
    \param cfg User parameters
    \param fcfg Flashpoint cfg
  */
  TextBankD3D9(EngineDD9 *engine, ParamEntryPar cfg, ParamEntryPar fcfg);
  ~TextBankD3D9();

	/// free texture memory, considering user settings
	int FreeTextureMemory();
	/// free texture memory as reported by the device driver
	int FreeTextureMemoryReported();
	/// free memory as reported by DDraw7 interfaces
	void FreeTextureMemoryReportedByDDraw(int &local, int &nonlocal);
	
	void ResetMaxTextureQuality(bool complete);

  //! get statistics for display / debugging purposed
  #if _ENABLE_CHEATS
    virtual RString GetStat(int statId, RString &statVal, RString &statVal2);
  #endif

    void RecalculateTextureMemoryLimits();
    
private:

  void MakePermanent(Texture *tex);
    
  void PreCreateVRAM( int reserve, bool randomOrder ); // pre-create surfaces
  void PreCreateSys(); // pre-create surfaces

  void PreCreateSurfaces();

public:
  class StoreThreadId: private ::NoCopy
  {
    TextBankD3D9 &_bank;
    #if _ENABLE_REPORT
    int _prevVal;
    #endif
    
    public:
    StoreThreadId(TextBankD3D9 &bank):_bank(bank)
    {
      PROFILE_SCOPE_EX(txCSL,tex);
      bank._textureMemoryCS.Lock();
      #if _ENABLE_REPORT
      _prevVal = bank._textureMemoryThread;
      bank._textureMemoryThread = GetCurrentThreadId();
      #endif
    }
    ~StoreThreadId()
    {
      #if _ENABLE_REPORT
      _bank._textureMemoryThread = _prevVal;
      #endif
      _bank._textureMemoryCS.Unlock();
    }
  };
  bool CheckTextureMemoryAccess() const {return _textureMemoryThread==GetCurrentThreadId();}
  
  void D3DMipCacheRoot9MemoryControlledFrame();

  void InitDetailTextures();
  void DoneDetailTextures();

  void Compact();

  void ReloadAll(); // texture cache was destroyed - reload

  //@{ texture memory stress testing
  void ClearTexStress();
  void AddTexStress(int size, bool renderTarget=false);
  void SetStressMode(int mode);
  void StressTestFrame();
  //@}

  //@{ virtual memory stress testing
  void ClearVMStress();
  bool AddVMStress(int size);
  //@}
  
  void GetTextureList(LLinkArray<Texture> &list);
  void Preload(); // initialize VRAM
  virtual bool FlushTexture(const char *name);
  virtual Ref<Texture> SuspendTexture(const char *name);
  virtual void ResumeTexture(Texture *tex);
  
  TextureLimits MaxTextureSize(Texture::UsageType type);
  
  RString GetTextureDebugName(IDirect3DBaseTexture9 *handle) const;
  
  virtual int GetMaxTextureQuality() const;
  virtual int GetTextureQuality() const {return _textureQuality;}
  virtual void SetTextureQuality(int value);
  
  static int SelectTextureMemory(int vram);
  
  virtual int GetMaxTextureMemory() const;
  virtual int GetTextureMemory() const;
  virtual void SetTextureMemory(int value);
  virtual void CheckForChangedFiles();
  
  /// debugging  - explicit control over limits
  void SetTextureMemoryDiag(int value);
  
  void FlushHandles();
  
  void DoCheckForChangedFiles();
  void DoFlushHandles();
  
  void FlushTextures(bool deep, bool allowAutoDeep); // switch data - flush and preload
  bool CheckDeepFlushNeeded() const;
  void FlushBank(QFBank *bank);

  protected:
  LLink<TextureD3D9> Find(RStringB name1) const;
  
  int FindSurface(
    int w, int h, int nMipmaps, D3DFORMAT format,
    const AutoArray<SurfaceInfoD3D9> &array
  ) const;

  // find, create if not found
  int FindSystem(int w, int h, int nMipmaps, D3DFORMAT format) const;
  void UseSystem(SurfaceInfoD3D9 &surf, const TEXTUREDESC9 &ddsd, PacFormat format, int mipImprovement);
  void AddSystem( SurfaceInfoD3D9 &surf );
  int DeleteLastSystem( int need, D3DSurfaceDestroyer9 *batch=NULL );

  int FindReleased(int w, int h, int nMipmaps, D3DFORMAT format) const
  {
    return FindSurface(w,h,nMipmaps,format,_freeTextures);
  }
  void AddReleased(SurfaceInfoD3D9 &surf);
  void UseReleased(SurfaceInfoD3D9 &surf, const TEXTUREDESC9 &ddsd);
  int DeleteLastReleased(D3DSurfaceDestroyer9 *batch=NULL);

  bool Reuse(
    SurfaceInfoD3D9 &surf, const TEXTUREDESC9 &ddsd, PacFormat format,
    bool enableReduction, int mipImprovement
  );

public:
  Ref<Texture> CreateTexture(ITextureSource *source);

  Ref<Texture> Load(RStringB name, bool caseSensitive = false);

  void StopAll(); // stop all background activity (before shutdown)

  void RegisterVisualizeThread(int threadId);
  bool UsingVisualizeThread() const {return _textureThread!=0 && !CheckMainThread(_textureThread);}

  void StartFrame();

  void PerformAsyncMaintenance();
  
  void FinishFrame();

  bool CheckUsedIntegrity() const;

  bool FrameNotFinishedYet() const {return _frameNotFinishedYet;};
  void PerformMipmapLoading();

  /// use some of the loaded mipmaps, never load a new one
  MipInfo UseMipmapLoaded(Texture *tex);
  /// make sure given mipmaps are loaded
  MipInfo UseMipmap(Texture *texture, int level, int levelTop, int priority=TexPriorityNormal);
  TextureD3D9 *GetWhiteTexture() const {return _white;}
  TextureD3D9 *GetBlackTexture() const {return _black;}
  TextureD3D9 *GetRandomTexture() const {return _random;}
  TextureD3D9 *GetDefNormalMapTexture() const {return _defNormMap;}
  TextureD3D9 *GetDefDetailTexture() const {return _defDetailMap;}
  TextureD3D9 *GetDefTITexture() const {return _defTI;}
  TextureD3D9 *GetDefTIManTexture() const {return _defTIMan;}
  TextureD3D9 *GetDefMacroTexture() const {return _defMacroMap;}
  TextureD3D9 *GetDefSpecularTexture() const {return _defSpecularMap;}
  TextureD3D9 *GetDefDTSpecularTexture() const {return _defDTSpecularMap;}
  TextureD3D9 *GetDefMaskTexture() const {return _zero;}

  bool VerifyChecksums();

  #if !SMALL_MIPS_READY
    /**
    when we are not really releasing, we can only limit what memory is used in other than free textures
    otherwise we would release all textures to get into the limit
    */
    int TotalMemoryToLimit() const {return _totalAllocated-_freeAllocated;}
  #else
    int TotalMemoryToLimit() const {return _totalAllocated;}
  #endif

  /// handle freeing of _freeTextures
  void HandleRegularCleanup(int limit);
  int ReserveMemoryLimit(int limit, int minAge);
  float VRAMDiscardMetrics() const;
  int VRAMDiscardCountAge(int minAge, int *totalSize=NULL) const;

  //! Change levelNeeded parameters in all textures of the cache specified
  /*!
    \param root Cache containing textures to change
    \param change Value to change the needed parameters by
  */
  void ChangeMipBias(D3DMipCacheRoot9 &root, float change);
  //! Find texture with the biggest difference between loaded and needed
  /*!
    \param root Cache containing textures to find in
    \param maxBias Returned maxBias value (the biggest difference)
    \param maxTex Returns texture associated with the maxBias value
    \return The same value as maxBias
  */
  float FindWorstBias(D3DMipCacheRoot9 &root, float &maxBias, TextureD3D9 *&maxTex) const;
  //! Limit memory allocated by decreasing mip bias
  /*!
    This function is rather drastic measure.
    Regular maintenance should keep mipbias so that this function is almost never needed.
  */
  bool LimitMemoryUsingMipBias(int maxAllocAllowed, int mipImprovement);

  /// track texture allocation
  void TextureAllocated(int size, int overhead);

  /// track texture allocation
  void TextureFreed(int size, int overhead) {TextureAllocated(-size,-overhead);}

  /// make sure there is enough memory for "size" allocation
  int ReserveMemory(int size, int minAge);
  
  int ForcedReserveMemory( int size, bool enableSysFree=true);
  
  /// check current memory allocation, release as necessary, return how much was released
  int CheckMemoryAllocation();

  void ReleaseAllTextures(bool releaseSysMem=false);
  void ReleaseTexturesWithPerformance(int performance);

  void RemoveLoadingJob( TextureD3D9 *tex );
  void TrackLoadingJob( TextureD3D9 *tex );
  void UpdateLoadingJob( TextureD3D9 *tex );

  //! create surface in VRAM - try reusing first
  int CreateVRAMSurfaceSmart(
    SurfaceInfoD3D9 &surface,
    const TEXTUREDESC9 &desc, PacFormat format, int totalSize, int mipImprovement
  );

  //! create surface in VRAM
  int CreateVRAMSurface(
    SurfaceInfoD3D9 &surface,
    const TEXTUREDESC9 &desc, PacFormat format, int totalSize
  );
  
  int ReserveSystemLimit(int limit);
  int ReserveSystem( int size );
  int ForcedReserveSystem( int size );
  
  void ReportTextures9( const char *name );
  
  public:
  size_t GetMaxVRAMAllocation() const;
  size_t GetVRamTexAllocation() const {return _totalAllocated;}

  void OnVRAMChanged();
  void OnTexDetailChanged();
#ifdef TRAP_FOR_0x8000
  void TrapFor0x8000(const char *str);
#endif
};

#if 0
  #define PROTECT_TEXTURE_MEMORY_ACCESS_BANK(bank) AssertSameThread(_textureThread);
  #define CHECK_TEXTURE_MEMORY_ACCESS_BANK(bank) AssertSameThread((bank)._textureThread);
#else
  #define PROTECT_TEXTURE_MEMORY_ACCESS_BANK(bank) PROFILE_SCOPE_DETAIL_EX(txCSS,tex);TextBankD3D9::StoreThreadId idLock((bank));
  #define CHECK_TEXTURE_MEMORY_ACCESS_BANK(bank) //DoAssert((bank).CheckTextureMemoryAccess());
#endif
#define PROTECT_TEXTURE_MEMORY_ACCESS() PROTECT_TEXTURE_MEMORY_ACCESS_BANK(*this)
#define CHECK_TEXTURE_MEMORY_ACCESS() CHECK_TEXTURE_MEMORY_ACCESS_BANK(*this)



#endif // !_DISABLE_GUI

#endif

