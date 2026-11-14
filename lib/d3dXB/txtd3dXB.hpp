#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TXTD3DXB_HPP
#define _TXTD3DXB_HPP

#if !_DISABLE_GUI || defined _XBOX

#include "../types.hpp"
#include <El/Math/math3d.hpp>
#include <El/ParamFile/paramFileDecl.hpp>

#include <Es/Memory/fastAlloc.hpp>
#include <Es/Containers/listBidir.hpp>
#include <El/Color/colors.hpp>

#include "../pactext.hpp"
#include "../textbank.hpp"

#include "d3dXBdefs.hpp"

#include <Es/Memory/normalNew.hpp>

class TextureD3DXB;
class HMipCacheD3DXB: public TLinkBidirD
{
  public:
  // only textures are stored in VRAM memory
  TextureD3DXB *texture;

  USE_FAST_ALLOCATOR;
};

typedef HMipCacheD3DXB HSysCacheD3DXB;

#include <Es/Memory/debugNew.hpp>

typedef TListBidir<HMipCacheD3DXB> D3DMipCacheRootXB;
typedef D3DMipCacheRootXB D3DSysCacheRootXB;

struct TEXTUREDESCXB
{
  int w,h,pitch;
  int nMipmaps;
  D3DFORMAT format;
  D3DPOOL pool;
};

#include "physHeap.hpp"

class SurfaceInfoD3DXB
{
  private:
  #ifdef _XBOX  
    mutable SRef<IDirect3DTexture8> _surface;
    mutable PhysMemHeapItemXB _physicalMemory;

    void Copy(const SurfaceInfoD3DXB &src);
  #else
    ComRef<IDirect3DTexture8> _surface;
    static int _nextId;
    int _id;
  #endif



  public:
  int _totalSize; // expected size for allocation
  int _usedSize; // actually allocated size
  int _w,_h;
  int _nMipmaps;
  PacFormat _format;

  //! Fence associated with the surface
  mutable DWORD _fence;


  int SizeExpected() const {return _totalSize;}
  int SizeUsed() const {return _usedSize;}
  int Slack() const {return _usedSize-_totalSize;}

  #ifdef _XBOX
  int GetCreationID() const {return 0;}
  #else
  int GetCreationID() const {return _id;}
  #endif

  public:
  #ifdef _XBOX  
  SurfaceInfoD3DXB()
  {
    _physicalMemory = PhysMemHeapItemXB();
    _fence = 0;
  }
  ~SurfaceInfoD3DXB()
  {
    Assert(_surface.IsNull());
    Assert(_physicalMemory.IsNull());
  }
  SurfaceInfoD3DXB(const SurfaceInfoD3DXB &src)
  {
    Copy(src);
  }
  void operator = (const SurfaceInfoD3DXB &src)
  {
    Assert(_surface.IsNull());
    Assert(_physicalMemory.IsNull());

    Copy(src);
  }

  #endif

  static int CalculateSize
  (
    IDirect3DDevice8 *dDraw, const TEXTUREDESCXB &desc, PacFormat format,
    int totalSize=-1
  );
  HRESULT CreateSurface
  (
    IDirect3DDevice8 *dDraw, const TEXTUREDESCXB &desc, PacFormat format,
    int totalSize=-1
  );
  void Free(bool lastRef, int refValue=0 );
  // TODO: use ComRef more consistently
  // add AddRef to ComRef contructor?
  // or use Ref for COM types
  #ifndef _XBOX
  //ComRef<IDirect3DTexture8> &GetSurface() {return _surface;}
  const ComRef<IDirect3DTexture8> &GetSurface() const {return _surface;}
  /// check if the texture or any of its mipmaps are still busy
  #else
  IDirect3DTexture8 *GetSurface() const {return _surface;}
  bool IsBusy() const;
  #endif
  //! Fence setting
  void SetFence(DWORD fence)
  {
    _fence = fence;
  }
};

TypeIsMovableZeroed(SurfaceInfoD3DXB);

struct D3DSurfaceToDestroyXB
{
  int _id;
  ComRef<IDirect3DTexture8> _surface;
};

TypeIsMovableZeroed(D3DSurfaceToDestroyXB);

class D3DSurfaceDestroyerXB
{
  #ifndef _XBOX
  // manage order of batch of destroyed surfaces
  // DX memory manager releases surfaces much faster
  // when released in reverse order of creation
  AutoArray<D3DSurfaceToDestroyXB> _surfaces;
  #endif
  public:
  D3DSurfaceDestroyerXB();
  ~D3DSurfaceDestroyerXB();
  void Add(SurfaceInfoD3DXB &surf);
  void DestroyAll();
};

//class MipmapLevelD3D: public PacLevelMem {};

#include <Es/Memory/normalNew.hpp>

#define ASSERT_INIT() Assert(_initialized);

#define MAX_MIPMAPS 10

//! single texture (D3D)

class TextureLoadHandler;

class TextureD3DXB: public Texture
{
  typedef Texture base;

  friend class TextBankD3DXB;
  friend class EngineDDXB;

private:
  //PacPalette _pal;

  SRef<ITextureSource> _src; //!< texture source provider

  bool _initialized;
  /// some textures cannot be released
  bool _permanent;

  int _maxSize; // landscape textures have different limit than object...

  // this information is same for all mipmaps, but is also copied in mipmap header
  int _nMipmaps;
  PacLevelMem _mipmaps[MAX_MIPMAPS];

  SurfaceInfoD3DXB _surface; // video memory copy
  HMipCacheD3DXB *_cache; // is allocated in heap?

  #ifndef _XBOX
  SurfaceInfoD3DXB _sysSurface; // system memory copy
  HSysCacheD3DXB *_sysCache; // is allocated in system heap?

  signed char _systemLoaded;  // which mipmap level loaded to _sysSurface
  #endif

  /// first level which is small enough to be used
  signed char _largestUsed;
  /// first level which is small enough to be considered atomic
  signed char _smallLevel;
  /// which mipmap level loaded to _surface
  signed char _levelLoaded;
  /// lock count - currently used for background operation - do not release
  signed char _inUse;

  //@{ levelNeeded already included mipbias
  float _levelNeededThisFrame; 
  float _levelNeededLastFrame;
  //@}

  //! Link list of load handlers associated with the texture
  TListBidir<TextureLoadHandler> _textureLoadHandlers;

  __forceinline TextureD3DHandle GetBigSurface() const {return _surface.GetSurface();}

  float LevelNeeded() const
  {
    return
    (
      _levelNeededThisFrame<_levelNeededLastFrame 
      ? _levelNeededThisFrame : _levelNeededLastFrame
    );
  }
  //! Copies surface by locking and unlocking
  static bool CPUCopyRects(IDirect3DSurface8 *srcSurf, IDirect3DSurface8 *destSurf);
  
public:
  TextureD3DXB();
  ~TextureD3DXB();
  
  //int LoadPalette( QIStream &in );
  //static int SkipPalette( QIStream &in ) {return PacPalette::Skip(in);} // load palette
  
  // handle single mip-map levels
  void InitDDSD( TEXTUREDESCXB &ddsd, int levelMin, bool enableDXT );
  //! copy texture from SYSMEM to DEFAULT
  #ifndef _XBOX
  bool CopyToVRAM( SurfaceInfoD3DXB &surface, int levelMin );
  #endif
  bool ReduceTextureLevel(int tgtLevel);
  //! discard levels that are not needed
  bool ReduceTexture();
  //! discard levels that are not needed, reuse original surface
  bool ReduceTexture(SurfaceInfoD3DXB &surf, float mipImprovement);
  bool ReduceTextureLevel(SurfaceInfoD3DXB &surf, int needed, bool noThrottling, float mipImprovement);

  int LoadLevels(int levelMin, float mipImprovement); // load to VRAM

  //! perform actual texture loading from the file
  int LoadLevelsSys(int levelMin, float mipImprovement); // load to sysmem

  //! request texture loading, return true if texture is ready
  bool RequestLevelsSys( int levelMin );
  //! request texture loading, return true if texture is ready
  bool RequestLevels( int levelMin );
  //! Returns permanent default texture - will always return non NULL value
  TextureD3DXB *GetDefaultTexture() const;

  __forceinline TextureD3DHandle GetHandle() const {return GetBigSurface();}
  __forceinline const SurfaceInfoD3DXB &GetSurface() const {return _surface;}

private:
  
  void MemoryReleased(); // mark as released

  int TotalSize( int levelMin ) const;

  void SysCacheUse( D3DSysCacheRootXB &list );

  void SystemReleased(); // mark as released
  void ReuseSystem( SurfaceInfoD3DXB &surf ); // reuse immediately

public: 
  

  int ReleaseMemory( SurfaceInfoD3DXB &surf, bool store=false, D3DSurfaceDestroyerXB *batch=NULL ); // move to reusable

  #ifndef _XBOX
  int ReleaseSystem( bool store=false, D3DSurfaceDestroyerXB *batch=NULL ); // move to reusable
  #endif
  int ReleaseMemory( bool store=false, D3DSurfaceDestroyerXB *batch=NULL ); // move to reusable
  void ReuseMemory( SurfaceInfoD3DXB &surf ); // reuse immediately

	QFileTime GetTimestamp() const {ASSERT_INIT();return _src ? _src->GetTimestamp() : 0;}
  bool IsAlpha() const {ASSERT_INIT();return _src && _src->IsAlpha();}
  bool IsAlphaNonOpaque() const {ASSERT_INIT();return _src && _src->IsAlphaNonOpaque();}
  int AMaxSize() const {return _maxSize;}
  void SetMaxSize( int size );

  virtual int GetClamp() const {ASSERT_INIT();return _src ? _src->GetClamp() : 0;}
  virtual void SetClamp(int clampFlags) {ASSERT_INIT();if (_src) _src->SetClamp(clampFlags);}

  bool VerifyChecksum( const MipInfo &mip ) const;
  
  //int Load( int level );
  
  void SetMipmapRange( int min, int max );

  //! do as little preparation as possible
  int Init(const char *name);

  /// initialize based on known source
  int Init(ITextureSource *source);

  virtual TextureType Type() const {ASSERT_INIT(); return _src ? _src->GetTextureType() : TT_Diffuse;}

private:
  /// initialize based on source (_src)
  void DoInitSource();
    
  //! do real preparation - is called before any real data is used
  void DoLoadHeaders();
  
public:
  /// check if headers are ready
  bool HeadersReady() const {return _initialized;}
  //! ask file server to preload headers from file
  void PreloadHeaders();

  /// handler when header preloading is done
  void RequestDone(RequestContext *context, RequestResult result);
  
  //! undo what was done by DoLoadHeaders or any actual loading
  void DoUnloadHeaders();
  
  //! load header if necessary (non-virtual)
  void LoadHeadersNV() const
  {
    if (_initialized) return;
    const_cast<TextureD3DXB *>(this)->DoLoadHeaders();
  }

  void LoadHeaders();

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

  //! Virtual method
  virtual PacFormat GetFormat() const {return _src ? _src->GetFormat() : PacUnknown;}
  
  bool IsTransparent() const {ASSERT_INIT();return _src && _src->IsTransparent();}

  Color GetPixel( int level, float u, float v ) const;
  Color GetColor() const;
  Color GetMaxColor() const;

  int Width( int level ) const {ASSERT_INIT();Assert(level < MAX_MIPMAPS);return _mipmaps[level]._w;}
  int Height( int level ) const {ASSERT_INIT();Assert(level < MAX_MIPMAPS);return _mipmaps[level]._h;}

  void CacheUse( D3DMipCacheRootXB &list );

  size_t GetMemoryControlled() const;
  
  __forceinline void AddUse() {_inUse++;}
  __forceinline void ReleaseUse() {_inUse--;}

  #if _ENABLE_REPORT
  virtual void OnUnused() const;
  #endif
  

  //! Registering of TLH
  void RegisterTextureLoadHandler(TextureLoadHandler *tlh);
  //! Invokes the TLH's event
  void TextureHasChanged();
  //! Fence setting
//  void SetFence(DWORD fence)
//  {
//    _surface.SetFence(fence);
//  }
  virtual void BindWithPushBuffer();

  USE_FAST_ALLOCATOR
};

//! object that can be used to scope lock textures
class ScopeUseTextureXB
{
  TextureD3DXB *_inUse;

  public:
  //! construct - enter lock
  ScopeUseTextureXB(TextureD3DXB *texture)
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
  ~ScopeUseTextureXB()
  {
    if (_inUse) _inUse->ReleaseUse();
  }
  //! copy - duplicate lock
  ScopeUseTextureXB(const ScopeUseTextureXB &src)
  :_inUse(src._inUse)
  {
    if (_inUse) _inUse->AddUse();
  }
  //! no assignement possible
  void operator =(const ScopeUseTextureXB &src)
  {
    if (src._inUse) src._inUse->AddUse();
    if (_inUse) _inUse->ReleaseUse();
    _inUse = src._inUse;
  }
};

//! object that can be used to scope lock textures
class ScopeInUseXB
{
  signed char &_inUse;

  public:
  //! construct - enter lock
  ScopeInUseXB(signed char &inUse)
  :_inUse(inUse)
  {
    _inUse++;
  }
  //! destruct - leave lock
  ~ScopeInUseXB()
  {
    _inUse--;
  }
  //! copy - duplicate lock
  ScopeInUseXB(const ScopeInUseXB &src)
  :_inUse(src._inUse)
  {
    _inUse++;
  }
  private:
  //! no assignement possible
  void operator =(const ScopeInUseXB &src);
};

#include <Es/Memory/debugNew.hpp>

#include <El/FreeOnDemand/memFreeReq.hpp>

//! texture manager (D3D)
class TextBankD3DXB:
  public AbstractTextBank, public MHInstanceTexVidMem
{
  typedef AbstractTextBank base;

  friend class TextureD3DXB;
  
  int _maxTextureMemory; // inital memory free for textures
  int _limitAlocatedTextures; // sum of currently allocated textures + free space - reserved space
  int _reserveTextureMemory; // we may know from experience some VRAM must be left
  int _freeTextureMemory; // updated every time CheckTextureMemory is called
  
  int _maxSmallTexturePixels; // how many pixels of each texture should always be loaded

  LLinkArray<TextureD3DXB> _texture;
  Ref<TextureD3DXB> _white; // required for pixel shader - NULL texture is black
  Ref<TextureD3DXB> _defNormMap; // default normal map
  Ref<TextureD3DXB> _defDetailMap; // default detail map
  //Ref<TextureD3DXB> _ditherMap; // dithering map
  RefArray<TextureD3DXB> _permanent; // texture loaded permanently
  
  #ifndef _XBOX
  int _limitSystemTextures;
  // system texture bank
  AutoArray<SurfaceInfoD3DXB> _freeSysTextures; // free system surfaces
  int _systemAllocated; //!< allocated in system heap (_freeSysTextures and _texture)

  D3DSysCacheRootXB _systemUsed; // keep this copy
  #endif

  AutoArray<SurfaceInfoD3DXB> _freeTextures; // free VRAM surfaces
  int _freeAllocated; //!< allocated in free vidmem heap (_freeTextures)
  int _totalAllocated; //!< allocated in vidmem heap (_freeTextures and _texture)
  int _totalSlack; //!< slack in vidmem heap (_freeTextures and _texture)

  EngineDDXB *_engine;

  D3DMipCacheRootXB _thisFrameWholeUsed;
  D3DMipCacheRootXB _lastFrameWholeUsed;
  D3DMipCacheRootXB _thisFramePartialUsed;
  D3DMipCacheRootXB _lastFramePartialUsed;
  D3DMipCacheRootXB _previousUsed;
  
  int _thisFrameAlloc; // VRAM allocations/deallocations (count)
  

  public:
  TextBankD3DXB(EngineDDXB *engine, ParamEntryPar cfg,ParamEntryPar fcfg);
  ~TextBankD3DXB();

  int FreeTextureMemory();

  //! get statistics for display / debugging purposed
  #if _ENABLE_CHEATS
    virtual RString GetStat(int statId, RString &statVal, RString &statVal2);
  #endif

private:

  int FreeAGPMemory();
  void CheckTextureMemory();
  
  void MakePermanent(Texture *tex);
    
  void PreCreateVRAM( int reserve, bool randomOrder ); // pre-create surfaces
  void PreCreateSys(); // pre-create surfaces

  void PreCreateSurfaces();

public:
  void InitDetailTextures();
  virtual int NTextures() const {return _texture.Size();}
  virtual Texture *GetTexture( int i ) const {return _texture[i];}

  void Compact();

  void ReloadAll(); // texture cache was destroyed - reload

  void Preload(); // initalize VRAM
  void FlushTextures(bool deep); // switch data - flush and preload
   /// make sure texture memory is available
  void AllocateTextureMemory();
  void FlushBank(QFBank *bank);

  protected:
  int FindFree();
  
  TextureD3DXB *Copy( int from );
  int Find(RStringB name1);
  
  int FindSurface
  (
    int w, int h, int nMipmaps, PacFormat format,
    const AutoArray<SurfaceInfoD3DXB> &array
  ) const;

  // find, create if not found
  int FindSystem
  (
    int w, int h, int nMipmaps, PacFormat format
  ) const;
  void UseSystem( SurfaceInfoD3DXB &surf, const TEXTUREDESCXB &ddsd, PacFormat format, float mipImprovement);
  void AddSystem( SurfaceInfoD3DXB &surf );
  int DeleteLastSystem( int need, D3DSurfaceDestroyerXB *batch=NULL );

  int FindReleased
  (
    int w, int h, int nMipmaps, PacFormat format
  ) const
  {
    return FindSurface(w,h,nMipmaps,format,_freeTextures);
  }
  void AddReleased( SurfaceInfoD3DXB &surf );
  void UseReleased
  (
    SurfaceInfoD3DXB &surf,
    const TEXTUREDESCXB &ddsd, PacFormat format
  );
  int DeleteLastReleased( int need, D3DSurfaceDestroyerXB *batch=NULL );

  bool ReuseWithReduction
  (
    D3DMipCacheRootXB &root, D3DMipCacheRootXB &rootFull,
    SurfaceInfoD3DXB &surf, const TEXTUREDESCXB &ddsd, PacFormat format,
    bool enableReduction, float mipImprovement
  );

  bool Reuse(
    SurfaceInfoD3DXB &surf, const TEXTUREDESCXB &ddsd, PacFormat format,
    bool enableReduction, float mipImprovement
  );
  void ReuseSys
  (
    SurfaceInfoD3DXB &surf, const TEXTUREDESCXB &ddsd, PacFormat format
  );

  public:
  Ref<Texture> CreateTexture(ITextureSource *source);

  Ref<Texture> Load(RStringB name);

  void StopAll(); // stop all background activity (before shutdown)

  void StartFrame();
  void FinishFrame();

  MipInfo UseMipmapLoaded(Texture *tex);
  MipInfo UseMipmap(Texture *texture, int level, int levelTop);
  TextureD3DXB *GetWhiteTexture() const {return _white;}
  TextureD3DXB *GetDefNormalMapTexture() const {return _defNormMap;}
  TextureD3DXB *GetDefDetailTexture() const {return _defDetailMap;}
  //TextureD3DXB *GetDitherTexture() const {return _ditherMap;}
  
  bool VerifyChecksums();

  /// calculate all memory allocated by given textures which is currently locked by CPU
  static int CalculateMemoryInUse( D3DMipCacheRootXB &root);
  /// calculate all memory allocated by given textures which is currently used by GPU
  static int CalculateMemoryInUseGPU( D3DMipCacheRootXB &root);
  /// calculate all memory allocated by given textures
  static int CalculateMemory( D3DMipCacheRootXB &root);
  /// calculate only memory in the textures which is needed
  static int CalculatePartMemory( D3DMipCacheRootXB &root);
  
  bool ReduceSomeTexture(D3DMipCacheRootXB &root, D3DMipCacheRootXB &whole);
  int ReserveMemory( D3DMipCacheRootXB &root, int limit );
  bool ReserveMemoryUsingReduction(int size, float mipImprovement);

  void ChangeMipBias(D3DMipCacheRootXB &root, float change);
  float FindWorstBias(D3DMipCacheRootXB &root, float &maxBias, TextureD3DXB *&maxTex) const;
  
  /// limit memory allocated by reducion partially used textures
  bool LimitMemoryUsingReduction(int maxAllocAllowed);
  /// limit memory allocated by decreasing mip bias
  bool LimitMemoryUsingMipBias(int maxAllocAllowed, float mipImprovement);

  void FreePhysicalTextureMemory(int toFree);
  int CountPhysicalTextureMemory() const;
  bool LimitPhysicalTextureMemory(int limit);
  bool LimitTextureMemory(int totalAllocatedWanted);

  int ReserveMemory(int size);
  int ForcedReserveMemory( int size, bool enableSysFree=true);

  void ReleaseAllTextures(bool releaseSysMem=false);

  //! create surface in VRAM - try reusing first
  int CreateVRAMSurfaceSmart(
    SurfaceInfoD3DXB &surface,
    const TEXTUREDESCXB &desc, PacFormat format, int totalSize, bool noThrottling,
    float mipImprovement
  );

  //! create surface in VRAM
  int CreateVRAMSurface
  (
    SurfaceInfoD3DXB &surface,
    const TEXTUREDESCXB &desc, PacFormat format, int totalSize
  );
  
  #ifndef _XBOX
  int ReserveSystem( D3DSysCacheRootXB &root, int limit );
  int ReserveSystem( int size );
  int ForcedReserveSystem( int size );
  #endif
  
  void ReportTextures( const char *name );

  //@{ implementation of MemoryFreeOnDemandHelper
  virtual float PriorityTexVidMem() const;
  virtual RString GetDebugNameTexVidMem() const;
  virtual size_t MemoryControlledTexVidMem() const;
  virtual size_t MemoryControlledRecentTexVidMem() const;
  virtual void MemoryControlledFrameTexVidMem();
  virtual size_t FreeOneItemTexVidMem();

  //@} implementation of MemoryFreeOnDemandHelper
  IMemoryFreeOnDemand *GetProgressFreeOnDemandInterface()
  {
    return safe_cast<MHInstanceTexVidMem *>(this);
  }
};

#endif // !_DISABLE_GUI

#endif

