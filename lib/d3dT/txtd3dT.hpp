#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TXTD3DT_HPP
#define _TXTD3DT_HPP

#if !_DISABLE_GUI && _ENABLE_DX10

#include "../types.hpp"
#include <El/Math/math3d.hpp>

#include <Es/Memory/fastAlloc.hpp>
#include <Es/Containers/cachelist.hpp>
#include <El/Color/colors.hpp>

#include "../pactext.hpp"
#include "../textbank.hpp"

#include "d3dDefsT.hpp"



#include <Es/Memory/normalNew.hpp>

class TextureD3DT;

/// item describing what element is held and can be evicted from the cache
class HMipCacheD3DT: public LinkBidirWithFrameStore
{
  public:
  /// which texture
  TextureD3DT *texture;
  /// which mipmap level
  /** Several mipmaps may be stored from the same texture */
  int level;

  size_t GetMemoryControlled() const;

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
struct FramedMemoryControlledListTraits<HMipCacheD3DT>: public FramedMemoryControlledListTraitsSRef<HMipCacheD3DT>
{
};

/**
track texture mipmaps suitable for discarding

*/

class D3DMipCacheRootT: public FramedMemoryControlledList<HMipCacheD3DT>
{
  typedef FramedMemoryControlledList<HMipCacheD3DT> base;
  
};

/** 
We do not provide any real memory to MemoryFreeOnDemandHelper, but we need to be connect to it
so that it calls our Frame function 
*/

class D3DMipCacheRootFreeOnDemandT: public MemoryFreeOnDemandHelper
{
  D3DMipCacheRootT &_list;
  
  public:
  D3DMipCacheRootFreeOnDemandT(D3DMipCacheRootT &list):_list(list){}
    
  //@{ MemoryFreeOnDemandHelper implementation
  virtual size_t FreeOneItem();
  virtual float Priority() const;
  virtual size_t MemoryControlled() const;
  virtual size_t MemoryControlledRecent() const;
  virtual RString GetDebugName() const;
  virtual void MemoryControlledFrame();
  //@}
  
};

//! Structure to hold brief texture information
struct TEXTUREDESC9
{
  // Width and height of the texture
  int w, h;
  //! Number of mipmaps
  int nMipmaps;
  //! Texture format
  DXGI_FORMAT format;
};

#if 0 // _ENABLE_REPORT //_DEBUG
  /// enable texture tracking using private data
  #define TRACK_TEXTURES 1
#endif

class EngineDDT;

class SurfaceInfoD3DT
{
private:
  //! Surface data (texture and view)
  SurfaceData _surface;

  static int _nextId;
  int _id;

public:
  int _totalSize; // expected size for allocation
  int _usedSize; // actually allocated size
  int _w,_h;
  int _nMipmaps;
  PacFormat _format;

  int SizeExpected() const {return _totalSize;}
  int SizeUsed() const {return _usedSize;}
  int Slack() const {return _usedSize-_totalSize;}

  int GetCreationID() const {return _id;}

public:
  
  #if TRACK_TEXTURES
  SurfaceInfoD3DT(){}
  SurfaceInfoD3DT(const SurfaceInfoD3DT &src);
  void operator = (const SurfaceInfoD3DT &src);
  #endif

  static int CalculateSize
  (
    ID3D10Device *dDraw, const TEXTUREDESC9 &desc, PacFormat format,
    int totalSize=-1
  );
  HRESULT CreateSurface
  (
    EngineDDT *engine, ID3D10Device *dDraw, const TEXTUREDESC9 &desc, PacFormat format, bool cpuAccess,
    int totalSize=-1
  );
  void Free(bool lastRef, int refValue=0 );
  // TODO: use ComRef more consistently
  // add AddRef to ComRef contructor?
  // or use Ref for COM types
  SurfaceData &GetSurface() {return _surface;}
  const SurfaceData &GetSurface() const {return _surface;}
};

TypeIsMovableZeroed(SurfaceInfoD3DT);

struct D3DSurfaceToDestroy
{
  int _id;
  SurfaceData _surface;
};

TypeIsMovableZeroed(D3DSurfaceToDestroy);

class D3DSurfaceDestroyerT
{
  // manage order of batch of destroyed surfaces
  // DX memory manager releases surfaces much faster
  // when released in reverse order of creation
  AutoArray<D3DSurfaceToDestroy> _surfaces;
public:
  D3DSurfaceDestroyerT();
  ~D3DSurfaceDestroyerT();
  void Add( const SurfaceInfoD3DT &surf );
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


//! single texture (D3D)

class TextureD3DT: public Texture //, public TextureLRUItem
{
  typedef Texture base;

  friend class TextBankD3DT;
  friend class EngineDDT;
  friend class HMipCacheD3DT;

private:
  //PacPalette _pal;

  SRef<ITextureSource> _src; //!< texture source provider

  bool _initialized;
  bool _permanent;
  
  /// performance requirements (priority)
  int _performance;

  //bool _isPaa;
  int _maxSize;
  /// landscape textures have different limit than object...
  UsageType _usageType;

  // this information is same for all mipmaps, but is also copied in mipmap header
  int _nMipmaps;
  //! List of mipmaps (the first one with index 0 is the biggest one)
  PacLevelMem _mipmaps[MAX_MIPMAPS];
  /// video memory copy
  SurfaceInfoD3DT _surfaces[MAX_MIPMAPS]; 

  /// is given level present in the cache?
  InitPtr<HMipCacheD3DT> _surfCache[MAX_MIPMAPS];

  /// First level which is small enough to be used
  signed char _largestUsed;
  /// First level which is small enough to be considered atomic (either we have at least this level, or the texture is not loaded at all)
  signed char _smallLevel;
  /// The finest level loaded
  signed char _levelLoaded;
  /// Is currently used for background operation - do not release
  signed char _inUse;
  /// levelNeeded already included mipbias
  float _levelNeededThisFrame;
  float _levelNeededLastFrame;


  __forceinline TextureD3DHandle GetBigSurface() const {return _levelLoaded < _nMipmaps ? _surfaces[_levelLoaded].GetSurface()._resview : NULL;}

  float LevelNeeded() const
  {
    return
    (
      _levelNeededThisFrame<_levelNeededLastFrame 
      ? _levelNeededThisFrame : _levelNeededLastFrame
    );
  }

public:
  //! Constructor
  TextureD3DT();
  //! Destructor
  ~TextureD3DT();
  
  //int LoadPalette( QIStream &in );
  //static int SkipPalette( QIStream &in ) {return PacPalette::Skip(in);} // load palette
  
  /// Returns description of the texture acquired from the level "levelMin"
  /*!
    \param ddsd Texture description to return
    \param levelMin level to acquire texture from
    \param enableDXT Flag to determine DXT2-5 can be used
  */
  void InitDDSD(TEXTUREDESC9 &ddsd, int levelMin, bool enableDXT);
  /// copy texture from SYSMEM to DEFAULT
  bool CopyToVRAM(SurfaceInfoD3DT &surface, int sysLevel, const SurfaceInfoD3DT &sys);
  /// discard levels that are not needed, reuse original surface
  bool ReduceTexture(SurfaceInfoD3DT &surf, float mipImprovement);

  bool ReduceTextureLevel(int tgtLevel);
  bool ReduceTextureLevel(SurfaceInfoD3DT &surf, int needed, float mipImprovement);

  int LoadLevels(int levelMin, float mipImprovement); // load to VRAM

  /// Performs actual texture loading from the file into system texture
  /*!
    \param levelMin Least level to be loaded
    \return 0 in case the function succeeded, -1 if failed
  */
  int LoadLevelsSys(SurfaceInfoD3DT &systemSurf, int levelMin, float mipImprovement);

  /// request texture loading, return true if texture is ready
  bool RequestLevelsSys(int levelMin, int priority);
  /// request texture loading, return true if texture is ready
  bool RequestLevels(int levelMin, int priority);
  /// Returns permanent default texture - will always return non NULL value
  TextureD3DT *GetDefaultTexture() const;

  __forceinline TextureD3DHandle GetHandle() const {return GetBigSurface();}
  __forceinline const SurfaceInfoD3DT &GetSurface() const {return _surfaces[_levelLoaded];}

  private:
  
  void MemoryReleased(int from, int to); // mark as released

  int TotalSize( int levelMin ) const;

public:

  /// verify internal object state is OK
  bool CheckIntegrity() const;
  
  int TotalVRAMUsed() const {return TotalSize(_levelLoaded);}

  static int ReleaseMemory( SurfaceInfoD3DT &surf, bool store=false, D3DSurfaceDestroyerT *batch=NULL ); // move to reusable

  int ReleaseSystem(SurfaceInfoD3DT &surface, bool store=false, D3DSurfaceDestroyerT *batch=NULL); // move to reusable
  int ReleaseMemory( bool store=false, D3DSurfaceDestroyerT *batch=NULL ); // move to reusable
  int ReleaseLevelMemory( int from, int to, bool store=false, D3DSurfaceDestroyerT *batch=NULL ); // move to reusable
  void ReuseMemory( SurfaceInfoD3DT &surf ); // reuse immediatelly

  virtual PacFormat GetFormat() const {return _src ? _src->GetFormat() : PacUnknown;}
  virtual bool IsAlpha() const {ASSERT_INIT();return _src && _src->IsAlpha();}
  virtual bool IsAlphaNonOpaque() const {ASSERT_INIT();return _src && _src->IsAlphaNonOpaque();}
  virtual bool IsTransparent() const {ASSERT_INIT();return _src && _src->IsTransparent();}
  QFileTime GetTimestamp() const {ASSERT_INIT();return _src ? _src->GetTimestamp() : 0;}

  virtual int GetClamp() const {ASSERT_INIT();return _src ? _src->GetClamp() : 0;}
  virtual void SetClamp(int clampFlags) {ASSERT_INIT();if (_src) _src->SetClamp(clampFlags);}
  
  /// adjust max. size based on usage type
  void AdjustMaxSize();
  void SetUsageType( UsageType type );
  void SetNMipmaps( int n);

  bool VerifyChecksum( const MipInfo &mip ) const;
  
  virtual TextureType Type() const {ASSERT_INIT(); return _src ? _src->GetTextureType() : TT_Diffuse;}

  virtual TexFilter GetMaxFilter() const {ASSERT_INIT();return _src ? _src->GetMaxFilter() : TFAnizotropic16;}

  //int Load( int level );
  
  void SetMipmapRange( int min, int max );

  /// do as little preparation as possible
  int Init(const char *name);

  /// initialize based on known source
  int Init(ITextureSource *source);

private:
  /// initialize based on source (_src)
  void DoInitSource();
    
  //! do real preparation - is called before any real data is used
  void DoLoadHeaders();

public:
  /// check if headers are ready
  bool HeadersReady() const {return _initialized;};
  //! ask file server to preload headers from file
  void PreloadHeaders();

  void RequestDone(RequestContext *context, RequestResult result);
  
  //! undo what was done by DoLoadHeaders or any actual loading
  void DoUnloadHeaders();
  
  //! load header if necessary (non-virtual)
  void LoadHeadersNV() const
  {
    if (_initialized) return;
    const_cast<TextureD3DT *>(this)->DoLoadHeaders();
  }

  void LoadHeaders();

  void CheckForChangedFile();

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

  /// 0 = default performance, 100, 200 = increased performance
  void SetTexturePerformance(int performance);

  Color GetColor() const;
  Color GetMaxColor() const;

  int Width( int level ) const {ASSERT_INIT();Assert(level < MAX_MIPMAPS);return _mipmaps[level]._w;}
  int Height( int level ) const {ASSERT_INIT();Assert(level < MAX_MIPMAPS);return _mipmaps[level]._h;}

  void CacheUse(TextBankD3DT &bank, int level);

  __forceinline void AddUse() {_inUse++;}
  __forceinline void ReleaseUse() {_inUse--;}

  USE_FAST_ALLOCATOR
};

//! object that can be used to scope lock textures
class ScopeUseTexture9
{
  TextureD3DT *_inUse;

  public:
  //! construct - enter lock
  ScopeUseTexture9(TextureD3DT *texture)
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

//! texture manager (D3D)

class TextBankD3DT:
  public RefCount, public AbstractTextBank, public MHInstanceTexVidMem
{
  typedef AbstractTextBank base;

	friend class TextureD3DT;
	
	int _maxTextureMemory; // initial memory free for textures
	/// sum of currently allocated textures + free space - reserved space
	/** this limit is for textures only */
	int _limitAlocatedTextures;
	/// common limit for textures + vertex buffers
	int _limitAlocatedVRAM;
	/// we may know from experience some VRAM must be left
	int _reserveTextureMemory;
	/// updated every time RecalculateTextureMemoryLimits is called
	int _freeTextureMemory; 

  /// what is the minimal reasonable texture size
  int _minTextureSize;

  LLinkArray<TextureD3DT> _texture;
  Ref<TextureD3DT> _white; // required for pixel shader - NULL texture is black
  Ref<TextureD3DT> _random; // small texture with random numbers in alpha
  Ref<TextureD3DT> _defNormMap; // default normal map
  Ref<TextureD3DT> _defDetailMap; // default detail map
  Ref<TextureD3DT> _defMacroMap; // default macro map
  Ref<TextureD3DT> _defSpecularMap; // default specular map
  RefArray<TextureD3DT> _permanent; // texture loaded permanently
  
  #if _ENABLE_CHEATS
  struct StressInfo
  {
    ComRef<ID3D10Texture2D> _tex;
    int _size;
    
    StressInfo(){}
    StressInfo(const ComRef<ID3D10Texture2D> &tex, int size)
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
  #endif

  
  int _limitSystemTextures;
  // system texture bank
  AutoArray<SurfaceInfoD3DT> _freeSysTextures; // free system surfaces

  AutoArray<SurfaceInfoD3DT> _freeTextures; // free VRAM surfaces
  int _freeAllocated; //!< allocated in free vidmem heap (_freeTextures)
  int _totalAllocated; //!< allocated in vidmem heap (_freeTextures and _texture)
  int _totalSlack; //!< slack in vidmem heap (_freeTextures and _texture)

  EngineDDT *_engine;
  int _systemAllocated; //!< allocated in system heap (_freeSysTextures and _texture)

  /// LRU list of mipmaps to be discarded  
  D3DMipCacheRootT _used;
  
  D3DMipCacheRootFreeOnDemandT _freeOnDemand;
  
  /// keep information about what texture is to be discarded
  
  /*
  TListBidir<TextureD3DT,TextureLRUItem> _recentlyUsed;
  */
  
  int _thisFrameAlloc; // VRAM allocations/deallocations (count)

  /// currently used texture quality  
  int _textureQuality;
  /// max. texture quality as currently possible based on video card capabilities
  int _maxTextureQuality;
  
  
  static const int _textureLimit[];
  static const float _textureMipBiasQuality[];
  static const int _textureMaxSize[][Texture::NUsageType];
  
public:
  //! Constructor
  /*!
    \param engine The engine
    \param cfg User parameters
    \param fcfg Flashpoint cfg
  */
  TextBankD3DT(EngineDDT *engine, ParamEntryPar cfg, ParamEntryPar fcfg);
  ~TextBankD3DT();

	/// free texture memory, considering user settings
	int FreeTextureMemory();
	/// free texture memory as reported by the device driver
	int FreeTextureMemoryReported();
	/// free memory as reported by DDraw7 interfaces
	void FreeTextureMemoryReportedByDDraw(int &local, int &nonlocal);
	
	void ResetMaxTextureQuality();

  //! get statistics for display / debugging purposed
  #if _ENABLE_CHEATS
    virtual RString GetStat(int statId, RString &statVal, RString &statVal2);
  #endif

private:

  void RecalculateTextureMemoryLimits();
  void MakePermanent(Texture *tex);
    
  void PreCreateVRAM( int reserve, bool randomOrder ); // pre-create surfaces
  void PreCreateSys(); // pre-create surfaces

  void PreCreateSurfaces();

public:
  void InitDetailTextures();
  void DoneDetailTextures();

  virtual int NTextures() const {return _texture.Size();}
  virtual Texture *GetTexture( int i ) const {return _texture[i];}

  void Compact();

  void ReloadAll(); // texture cache was destroyed - reload

  //@{ texture memory stress testing
  void ClearTexStress();
  void AddTexStress(int size, bool renderTarget=false);
  void SetStressMode(int mode);
  void StressTestFrame();
  //@}

  void Preload(); // initialize VRAM
  virtual bool FlushTexture(const char *name);
  virtual Ref<Texture> SuspendTexture(const char *name);
  virtual void ResumeTexture(Texture *tex);
  
  int MaxTextureSize(Texture::UsageType type);
  
  
  int GetMaxTextureQuality() const;
  int GetTextureQuality() const {return _textureQuality;}
  void SetTextureQuality(int value);
  
  void CheckForChangedFiles();
  
  void FlushTextures(bool deep); // switch data - flush and preload
  void FlushBank(QFBank *bank);

  protected:
  int FindFree();
  
  TextureD3DT *Copy( int from );
  int Find(RStringB name1);
  
  int FindSurface
  (
    int w, int h, int nMipmaps, PacFormat format,
    const AutoArray<SurfaceInfoD3DT> &array
  ) const;

  // find, create if not found
  int FindSystem
  (
    int w, int h, int nMipmaps, PacFormat format
  ) const;
  void UseSystem(SurfaceInfoD3DT &surf, const TEXTUREDESC9 &ddsd, PacFormat format, float mipImprovement);
  void AddSystem( SurfaceInfoD3DT &surf );
  int DeleteLastSystem( int need, D3DSurfaceDestroyerT *batch=NULL );

  int FindReleased
  (
    int w, int h, int nMipmaps, PacFormat format
  ) const
  {
    return FindSurface(w,h,nMipmaps,format,_freeTextures);
  }
  void AddReleased( SurfaceInfoD3DT &surf );
  void UseReleased
  (
    SurfaceInfoD3DT &surf,
    const TEXTUREDESC9 &ddsd, PacFormat format
  );
  int DeleteLastReleased( int need, D3DSurfaceDestroyerT *batch=NULL );

  bool Reuse
  (
    SurfaceInfoD3DT &surf, const TEXTUREDESC9 &ddsd, PacFormat format,
    bool enableReduction, float mipImprovement
  );

public:
  Ref<Texture> CreateTexture(ITextureSource *source);

  Ref<Texture> Load(RStringB name);

  void StopAll(); // stop all background activity (before shutdown)

  void StartFrame();
  void FinishFrame();
  void PerformMipmapLoading();

  /// use some of the loaded mipmaps, never load a new one
  MipInfo UseMipmapLoaded(Texture *tex);
  /// make sure given mipmaps are loaded
  MipInfo UseMipmap(Texture *texture, int level, int levelTop, int priority=TexPriorityNormal);
  TextureD3DT *GetWhiteTexture() const {return _white;}
  TextureD3DT *GetRandomTexture() const {return _random;}
  TextureD3DT *GetDefNormalMapTexture() const {return _defNormMap;}
  TextureD3DT *GetDefDetailTexture() const {return _defDetailMap;}
  TextureD3DT *GetDefMacroTexture() const {return _defMacroMap;}
  TextureD3DT *GetDefSpecularTexture() const {return _defSpecularMap;}

  bool VerifyChecksums();

  int ReserveMemoryLimit(int limit, int minAge);
  bool ReserveMemoryUsingReduction(int size, float mipImprovement);
  float VRAMDiscardMetrics() const;
  int VRAMDiscardCountAge(int minAge, int *totalSize=NULL) const;

  //! Change levelNeeded parameters in all textures of the cache specified
  /*!
    \param root Cache containing textures to change
    \param change Value to change the needed parameters by
  */
  void ChangeMipBias(D3DMipCacheRootT &root, float change);
  //! Find texture with the biggest difference between loaded and needed
  /*!
    \param root Cache containing textures to find in
    \param maxBias Returned maxBias value (the biggest difference)
    \param maxTex Returns texture associated with the maxBias value
    \return The same value as maxBias
  */
  float FindWorstBias(D3DMipCacheRootT &root, float &maxBias, TextureD3DT *&maxTex) const;
  //! Limit memory allocated by decreasing mip bias
  /*!
    This function is rather drastic measure.
    Regular maintenance should keep mipbias so that this function is almost never needed.
  */
  bool LimitMemoryUsingMipBias(int maxAllocAllowed, float mipImprovement);

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

  //! create surface in VRAM - try reusing first
  int CreateVRAMSurfaceSmart
  (
    SurfaceInfoD3DT &surface,
    const TEXTUREDESC9 &desc, PacFormat format, int totalSize, float mipImprovement
  );

  //! create surface in VRAM
  int CreateVRAMSurface
  (
    SurfaceInfoD3DT &surface,
    const TEXTUREDESC9 &desc, PacFormat format, int totalSize
  );
  
  int ReserveSystemLimit(int limit);
  int ReserveSystem( int size );
  int ForcedReserveSystem( int size );
  
  void ReportTextures( const char *name );

  // implementation of MemoryFreeOnDemandHelper
  virtual float PriorityTexVidMem() const;
  virtual RString GetDebugNameTexVidMem() const;
  virtual size_t MemoryControlledTexVidMem() const;
  virtual size_t MemoryControlledRecentTexVidMem() const;
  virtual void MemoryControlledFrameTexVidMem();
  
  virtual size_t FreeOneItemTexVidMem();
  
  public:
  size_t GetMaxVRAMAllocation() const;
  size_t GetVRamTexAllocation() const {return _totalAllocated;}
};

#endif // !_DISABLE_GUI

#endif

