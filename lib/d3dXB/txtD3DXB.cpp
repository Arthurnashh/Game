#include "../wpch.hpp"

//#define SurfaceInfoD3DXB @@

#if !_DISABLE_GUI || defined _XBOX

#include <El/common/perfLog.hpp>
#include <El/ParamFile/paramFile.hpp>

//#include "global.hpp"
//#include "progress.hpp"
#include "txtD3DXB.hpp"
#include "engDDXB.hpp"
#include "../timeManager.hpp"

#include <Es/Threads/multiSync.hpp>
#include "../fileServer.hpp"
#include <El/QStream/qbStream.hpp>
#include <El/common/randomGen.hpp>
#include <El/Common/perfProf.hpp>
#include "../keyInput.hpp"
#include "../dikCodes.h"
#include "../progress.hpp"
#include <Es/Algorithms/qsort.hpp>

#ifdef _XBOX
#include <xgraphics.h>
#endif

extern size_t GetMemoryUsedSize();

#define LOG_INTERESTING 0

#if LOG_INTERESTING
static bool InterestedIn(const char *name)
{
  static char interestedIn[64]="form_echl.paa";
  return strstr(name,interestedIn)!=NULL;
}
#else
static inline bool InterestedIn(const char *name) {return false;}
#endif


#ifdef _XBOX
inline int AlignVRAM( int x )
{
  //int align = 4*1024; // must be power of 2
  int align = 256; // must be power of 2
  int alignMask = align-1;
  return (x+alignMask)&~alignMask;
}
#else
inline int AlignVRAM( int x )
{
  int align = 256; // must be power of 2
  int alignMask = align-1;
  return (x+alignMask)&~alignMask;
}
#endif

static int MipmapSize( PacFormat format, int w, int h )
{
  switch (format)
  {
    case PacDXT1:
    return w*h/2;
    case PacDXT2: case PacDXT3: case PacDXT4: case PacDXT5:
    return w*h;
    case PacARGB8888:
    return w*h*4;
    default:
    return w*h*2; // assume 16-b textures
  }
  /*NOTREACHED*/
}


#define MIN_MIP_SIZE 4 // guarantee QWORD alignment (4 16-bit pixels are enough)

#ifdef _XBOX
#define REPORT_ALLOC 0 // verbosity level 0..30
#else
#define REPORT_ALLOC 0 // verbosity level 0..30
#endif

#define USE_MANAGE 0 // TODO: small/large texture bank, try manager

static PacFormat BasicFormat( const char *name )
{
  char *ext=strrchr(name,'.');
  if( ext && !strcmpi(ext,".paa") ) return PacARGB4444;
  else return PacARGB1555;
}


#ifdef _XBOX

#include <xmmintrin.h>
#define AUTO_STATIC_ARRAY_16(Type,name,size) \
  AUTO_STATIC_ARRAY_ALIGNED(Type,name,size,__m128)

static bool LoadMipmapSwizzled
(
  void *dta, PacLevelMem &mip,
  const ITextureSource *src, int i, int bpp
)
{
  // if data is not pre-swizzled, we have to swizzle it now
  AUTO_STATIC_ARRAY_16(char,work,256*256*2);
  Assert(mip._pitch > 0);
  //work.Realloc(mip._h*mip._pitch);
  work.Resize(mip._h*mip._pitch);
  bool ret = src->GetMipmapData(work.Data(),mip,i);
  if (ret)
  {
    // temp contains non-swizzled data
    XGSwizzleRect(
      work.Data(),0,NULL,dta,
      mip._w,mip._h,NULL,bpp
    );
  }
  return ret;
}

static bool LoadMipmap
(
  void *dta, PacLevelMem &mip,
  const ITextureSource *src, int i
)
{
  PROFILE_SCOPE(texLM);
  bool ret = false;
  int bpp = 2;
  switch (mip._dFormat)
  {
    case PacARGB8888:
      bpp = 4;
    case PacARGB4444:
    case PacARGB1555:
    case PacRGB565:
    case PacAI88:
      ret = LoadMipmapSwizzled(dta,mip,src,i,bpp);
      break;
    default:
      // DXT and other formats are pre-swizzled
      ret = src->GetMipmapData(dta,mip,i);
      break;
  }
  return ret;
}

#endif

/*!
Similiar to LoadLevelsSys
*/
bool TextureD3DXB::RequestLevelsSys( int levelMin )
{
  #ifndef _XBOX
  if (levelMin==_systemLoaded)
  {
    // no file access needed - we may proceed
    return true;
  }
  #endif

  // if there is no time left for loading, we pretend data are not ready
  if (GTimeManager.TimeLeft(TCLoadTexture)<=0) return false;
  // we do not care about swizzling now - we only need to be sure data are here
  return _src->RequestMipmapData(_mipmaps,levelMin,_nMipmaps);

}

/*!
\patch 1.12 Date 8/07/2001 by Ondra
- Fixed: Incorrect texture source (like JPG file not loaded) crashes.
\patch 1.48 Date 3/12/2002 by Ondra
- Fixed: Incorrect texture source (like JPG file not loaded) caused shutdown.
Warning message is shown instead.
*/

int TextureD3DXB::LoadLevelsSys(int levelMin, float mipImprovement)
{
  TextBankD3DXB *bank=static_cast<TextBankD3DXB *>(GEngine->TextBank());
  #ifndef _XBOX
  if (levelMin==_systemLoaded)
  {
    Assert(_sysSurface.GetSurface());
    // verify texture is really loaded correctly
    SysCacheUse(bank->_systemUsed);

    // dirty rects may be invalidated during Reset??
    HRESULT err=_sysSurface.GetSurface()->AddDirtyRect(NULL);
    if (err!=D3D_OK)
    {
      DDErrorXB("AddDirtyRect",err);
    }
    return 0;
  }
  #else
  if (levelMin>=_levelLoaded)
  {
    CacheUse(bank->_thisFrameWholeUsed);
    return 0;
  }
  #endif

  TimeManagerScope scope(TCLoadTexture);
  PROFILE_SCOPE(texLS);
  //LogF("Load %s:%d",cc_cast(GetName()),levelMin);
  ScopeInUseXB use(_inUse);

  int i;

  if (!_src)
  {
    RptF("No texture source for %s",Name());
    return -1;
  }

  // create sysmem mip-map chain

  TEXTUREDESCXB sysd;
  memset(&sysd,0,sizeof(sysd)); 
  
  PacFormat format=_mipmaps[levelMin].DstFormat();
  //GEngineDD->InitVRAMPixelFormat(sysd.ddpfPixelFormat,format,false);
  GEngineDD->InitVRAMPixelFormat(sysd.format,format,true);

  sysd.pool=D3DPOOL_SYSTEMMEM;
  sysd.nMipmaps = _nMipmaps-levelMin;

  #ifndef _XBOX
  ReleaseSystem(true);
  #else
  ReleaseMemory(true);
  #endif

  DoAssert(levelMin < _nMipmaps);
  PacLevelMem &topMip = _mipmaps[levelMin];

  // not present -> create it and load it
  #ifndef _XBOX
  SurfaceInfoD3DXB &systemSurf = _sysSurface;
  #else
  SurfaceInfoD3DXB &systemSurf = _surface;
  #endif
  sysd.w = topMip._w;
  sysd.h = topMip._h;
  //Assert(topMip._pitch > 0);
  sysd.pitch = topMip._pitch;

  bank->UseSystem(systemSurf,sysd,format,mipImprovement);
  
  if (!systemSurf.GetSurface())
  {
    // not enough memory - loading failed
    return -1;
  }

  //! Invoke the TLH's event
  TextureHasChanged();

  // load all mipmaps - coarse mipmaps are very small anyway
  // and it would help little not to load them
  for( i=levelMin; i<_nMipmaps; i++ )
  { // sysmem load
    PacLevelMem &mip = _mipmaps[i];

    int aLevel = i-levelMin;
    Assert( aLevel>=0 );

    // lock surface - we will load it
    HRESULT err;
    D3DLOCKED_RECT rect;

    #ifndef _XBOX
    SurfaceInfoD3DXB &surf = _sysSurface;
    #else
    SurfaceInfoD3DXB &surf = _surface;
    #endif
    {
      PROFILE_SCOPE(texLR);
      err=surf.GetSurface()->LockRect
      (
        aLevel,&rect,NULL,0
      );
    }

    // Assign the true value to pitch
    if (PacFormatIsDXT(mip._dFormat))
    {
      // Pitch is 4 lines together
      mip._pitch = rect.Pitch >>2;
    }
    else
    {
      mip._pitch = rect.Pitch;
    }
    bool ret = true;

    if( err!=D3D_OK ) DDErrorXB("Cannot lock mipmap",err);
    // assume normal pitch
    // load level
    #ifdef _XBOX
      ret = LoadMipmap(rect.pBits,mip,_src,i);
    #else
      ret = _src->GetMipmapData(rect.pBits,mip,i);
    #endif
    ADD_COUNTER(ldTxP,mip._w*mip._h);
    
    if (!ret)
    {
      // in case of error fill mipmap with average color
      PackedColor color = _src->GetAverageColor();
      mip.FillMipmap(rect.pBits,color);
    }

    err=surf.GetSurface()->UnlockRect(aLevel);
    if( err!=D3D_OK ) DDErrorXB("Cannot unlock mipmap",err);
    if( !ret ) WarningMessage("Cannot load mipmap %s",Name());

    //actMipmap->GetAttachedSurface(&caps,nextMipmap.Init());
    //actMipmap=nextMipmap;
  } // if( load mipmap level)

  // if we quick-filled only, we should not mark the texture as loaded
  #ifdef _XBOX
    CacheUse(bank->_thisFrameWholeUsed);
    //LogF("texture %s: whole used",Name());
    _levelLoaded=levelMin;
  #else
    SysCacheUse(bank->_systemUsed);
    _systemLoaded = levelMin;
  #endif

  return 0;
}

void TextureD3DXB::InitDDSD( TEXTUREDESCXB &ddsd, int levelMin, bool enableDXT )
{
  memset(&ddsd,0,sizeof(ddsd)); 

  ddsd.pool = D3DPOOL_DEFAULT;

  Assert(levelMin < _nMipmaps);
  PacFormat format=_mipmaps[levelMin].DstFormat();

  GEngineDD->InitVRAMPixelFormat(ddsd.format,format,enableDXT);

  ddsd.w = _mipmaps[levelMin]._w; 
  ddsd.h = _mipmaps[levelMin]._h; 
  Assert(_mipmaps[levelMin]._pitch > 0);
  ddsd.pitch = _mipmaps[levelMin]._pitch;
  ddsd.nMipmaps = _nMipmaps-levelMin;
}


#ifndef _XBOX
bool TextureD3DXB::CopyToVRAM( SurfaceInfoD3DXB &surface, int levelMin )
{
  // select best system surface
  Assert (_sysSurface.GetSurface());

  IDirect3DDevice8 *device=GEngineDD->GetDirect3DDevice();

  Assert(_sysSurface._w==surface._w);
  Assert(_sysSurface._h==surface._h);
  Assert(_sysSurface._format==surface._format);
  HRESULT err = device->UpdateTexture
  (
    _sysSurface.GetSurface(),surface.GetSurface()
  );

  #if _ENABLE_REPORT
  if (err!=D3D_OK)
  {
    DDErrorXB("Copy",err);
    // check pools of both surfaces
    D3DSURFACE_DESC ssd,tsd;
    _sysSurface.GetSurface()->GetLevelDesc(0,&ssd);
    surface.GetSurface()->GetLevelDesc(0,&tsd);
    LogF("Src pool %d, Tgt pool %d",ssd.Pool,tsd.Pool);
  }
  #endif

  return true;
}
#endif  


bool TextureD3DXB::ReduceTextureLevel(SurfaceInfoD3DXB &surf, int needed, bool noThrottling, float mipImprovement)
{
  if (needed>_smallLevel) needed = _smallLevel;
  if (_levelLoaded>=needed) return false;
  if (needed>=_nMipmaps) return false;
  ScopeInUseXB use(_inUse);
  
  TextBankD3DXB *bank=static_cast<TextBankD3DXB *>(GEngine->TextBank());

  // check if reducing makes sense, and to what degree
  // first level smaller or equal to _maxSmallTexturePixels is minimal used
  if (_levelLoaded>=_smallLevel)
  {
    // nowhere to reduce - texture is already very small
    return false;
  }

  //! Invoke the TLH's event
  TextureHasChanged();

  Assert(_surface.GetSurface());
  SurfaceInfoD3DXB surface;
  TEXTUREDESCXB desc;

  InitDDSD(desc,needed,true);

  PacFormat format=_mipmaps[needed].DstFormat();
  int totalSize=TotalSize(needed);

  // assume any mipbias improvement is acceptable now
  if (bank->CreateVRAMSurfaceSmart(surface,desc,format,totalSize,noThrottling,mipImprovement)<0)
  {
    return false;
  }
  IDirect3DDevice8 *device=GEngineDD->GetDirect3DDevice();
  #ifdef _XBOX
    // device member functions are static, and this variable is therefore not used
    (void)device;
  #endif
  // use CopyRects to copy from the large to the small surface
  IDirect3DTexture8 *srcTex = _surface.GetSurface();
  IDirect3DTexture8 *tgtTex = surface.GetSurface();
  #if _ENABLE_REPORT
    // we do not care if source surface is busy
    // - we are able to perform a non-blocking lock on it
    if (tgtTex->IsBusy())
    {
      LogF("Target texture busy");
    }
    if (surface.IsBusy())
    {
      LogF("Target surface busy");
    }
  #endif
  for (int i=needed; i<_nMipmaps; i++)
  {
    int srcLevel = i-_levelLoaded;
    int tgtLevel = i-needed;
    // get mipmap surface
    ComRef<IDirect3DSurface8> srcSurf;
    ComRef<IDirect3DSurface8> tgtSurf;
    srcTex->GetSurfaceLevel(srcLevel,srcSurf.Init());
    tgtTex->GetSurfaceLevel(tgtLevel,tgtSurf.Init());
    if (srcSurf.IsNull() || tgtSurf.IsNull())
    {
      Fail("Cannot access single surfaces");
      bank->AddReleased(surface);
      // some strange failure
      return false;
    }
    #if _ENABLE_REPORT
      if (tgtSurf->IsBusy())
      {
        LogF("Target %d busy",tgtLevel);
      }
    #endif
    //if ((i > 0) && (_mipmaps[i - 1].Size() < 64))
    {
      if (!CPUCopyRects(srcSurf, tgtSurf))
      {
        LogF("CPUCopyRects error");
        bank->AddReleased(surface);
        return false;
      }
    }
    /*
    else
    {
      HRESULT hr = device->CopyRects(srcSurf,NULL,0,tgtSurf,NULL);
      if (FAILED(hr))
      {
        DDErrorXB("CopyRects error",hr);
        bank->AddReleased(surface);
        return 0;
      }
    }
    */
  }
  // we can release source surface now
  // no memory is freed here, therefore no stats update is necessary
  surf = _surface;
  _surface = surface;
  _levelLoaded = needed;
  // quick fill status is not changed now
  return true;
}

bool TextureD3DXB::ReduceTextureLevel(int tgtLevel)
{
  SurfaceInfoD3DXB surf;
  // do not throttle - we need the small texture so that we can release a larger one
  // we will therefore grow over the boundary for only a very short time
  if (ReduceTextureLevel(surf,tgtLevel,true,0))
  {
    ReleaseMemory(surf);
    return true;
  }
  return false;
}
bool TextureD3DXB::ReduceTexture(SurfaceInfoD3DXB &surf, float mipImprovement)
{
  int needed = toIntFloor(LevelNeeded());
  // if texture is not needed, assume small version is needed
  if (needed>=_nMipmaps) needed = _nMipmaps-1;
  return ReduceTextureLevel(surf,needed,false,mipImprovement);
}

bool TextureD3DXB::ReduceTexture()
{
  SurfaceInfoD3DXB surf;
  // do not throttle - we need the small texture so that we can release a larger one
  // we will therefore grow over the boundary for only a very short time

  int needed = toIntFloor(LevelNeeded());
  // if texture is not needed, assume small version is needed
  if (needed>=_nMipmaps) needed = _nMipmaps-1;
  if (ReduceTextureLevel(surf,needed,true,0))
  {
    ReleaseMemory(surf);
    return true;
  }
  return false;
}

void ReportGRAMXB( const char *name)
{
  if (GEngineDD) GEngineDD->ReportGRAM(name);
}

void ReportTexturesXB( const char *name )
{
  EngineDDXB *engine = dynamic_cast<EngineDDXB *>(GEngine);
  if (!engine) return;
  engine->TextBankDD()->ReportTextures(name);
}



int TextureD3DXB::TotalSize( int levelMin ) const
{
  int totalSize=0;
  for( int i=levelMin; i<_nMipmaps; i++ )
  {
    const PacLevelMem &mip = _mipmaps[i];
    int size = MipmapSize(mip.DstFormat(),mip._w,mip._h);
    //int size=_mipmaps[i]._pitch*_mipmaps[i]._h;
    totalSize+=size;
  }
  return totalSize;
}

/*!
Similiar to LoadLevels
*/

bool TextureD3DXB::RequestLevels( int levelMin )
{
  // if no levels are needed, we can load nothing and still return success
  if (levelMin<0 || _levelLoaded<=levelMin) return true;

  return RequestLevelsSys(levelMin);
}

TextureD3DXB *TextureD3DXB::GetDefaultTexture() const
{
  TextureD3DXB *result;
  switch (Type())
  {
  case TT_Detail:
    result = GEngineDD->TextBankDD()->GetDefDetailTexture();
  	break;
  case TT_Normal:
    result = GEngineDD->TextBankDD()->GetDefNormalMapTexture();
    break;
  default:
    result = GEngineDD->TextBankDD()->GetWhiteTexture();
    break;
  }
  Assert(result);
  Assert(result->_permanent);
  return result;
}

int TextureD3DXB::LoadLevels(int levelMin, float mipImprovement)
{
  // if no levels are needed, we can load nothing and still return success
  if (levelMin<0) return 0;

  int ret=0;
  //ScopeLock<AbstractTextureLoader> lock(*_bank->_loader);
  // if the texture loaded contains min..max range, let it be...

  Assert (levelMin<_nMipmaps);
  Assert (levelMin>=0);

  // assume: there is some levelCur so that for every i, i>=levelCur
  // _mipmaps[i]._memorySurf is not NULL

  TextBankD3DXB *bank=static_cast<TextBankD3DXB *>(GEngineDD->TextBank());
  if( _levelLoaded>levelMin)
  {

    #ifndef _XBOX
    ReleaseMemory(true); // release all old mipmap data
    //levelCur=_nMipmaps; // all levels released

    #if REPORT_ALLOC>=20
    LogF
    (
      "Load %s (%d:%d x %d)",
      (const char *)Name(),
      levelMin,_mipmaps[levelMin]._w,_mipmaps[levelMin]._h
    );
    #endif

    ScopeInUseXB use(_inUse);
    // some mipmaps must be added to the texture
    // create new mipmaps in system memory

    // create whole mipmap structure with single call in target heap

    TEXTUREDESCXB ddsd;
    InitDDSD(ddsd,levelMin,true);

    // prepare all new levels
    // create and load mipmap (per-level)
    PacFormat format=_mipmaps[levelMin].DstFormat();
    int totalSize=TotalSize(levelMin);
    // create or reuse VRAM surface
    if( bank->CreateVRAMSurfaceSmart(_surface,ddsd,format,totalSize,mipImprovement)<0 )
    {
      return -1;
    }
    #endif

    // all VidMem levels created
    // level 0 pointer acquired
    int ret=LoadLevelsSys(levelMin,mipImprovement);
    #ifndef _XBOX
    if (ret>=0)
    {
      if (!CopyToVRAM(_surface,levelMin))
      {
        LogF("CopyToVRAM failed");
        ret=-1;
      }

      // we load more that was used before - it is surely needed whole
      CacheUse(bank->_thisFrameWholeUsed);
      //LogF("texture %s: whole used",Name());
      _levelLoaded=levelMin;
      
    }
    #else
      if (ret<0)
      {
        // loading failed (probably due to lack of VRAM)
        // caller will probably try to load a coarser mipmap
        return -1;
      }
    #endif

    // everything is loaded - lock and copy pointers

  }
  else
  {
    CacheUse(bank->_thisFrameWholeUsed);
  }
  if (ret<0)
  {
    ReleaseMemory(true);
  }
  return ret;
}

#ifdef _XBOX
bool SurfaceInfoD3DXB::IsBusy() const
{
  if (_surface.IsNull()) return false;
  if (_surface->IsBusy()) return true;
  if (GEngineDD->GetDirect3DDevice()->IsFencePending(_fence)) return true;
  for (int i=0; i<_nMipmaps; i++)
  {
    // get mipmap surface
    ComRef<IDirect3DSurface8> surf;
    _surface->GetSurfaceLevel(i,surf.Init());
    if (surf.NotNull() && surf->IsBusy())
    {
      return true;
    }
  }
  return false;
}
#endif

bool TextBankD3DXB::ReuseWithReduction
(
  D3DMipCacheRootXB &rootPart, D3DMipCacheRootXB &rootFull,
  SurfaceInfoD3DXB &surf, const TEXTUREDESCXB &ddsd, PacFormat format,
  bool enableReduction, float mipImprovement
)
{
  int w=ddsd.w;
  int h=ddsd.h;
  int nMipmaps=ddsd.nMipmaps;
  if (w*h<=_maxSmallTexturePixels)
  {
    // no need to try reducing - no reduction is possible for this size
    enableReduction = false;
  }
  for (HMipCacheD3DXB *last=rootPart.Last(); last; last=rootPart.Prev(last))
  {
    // release mip-map data
    TextureD3DXB *texture=last->texture;
    //cannot release this one
    if( texture->_inUse ) continue;

    const SurfaceInfoD3DXB &surface=texture->_surface;
    if( nMipmaps!=surface._nMipmaps ) continue;
    // match size
    if( surface._w!=w ) continue;
    if( surface._h!=h ) continue;
    // match pixel format
    // if equal - this texture is perfect match
    if( surface._format!=format ) continue;
    if (surface.IsBusy()) continue;
    // you cannot reduce so much that the texture needs to be loaded again
    
    // surface moved to reusable
    if (enableReduction && texture->ReduceTexture(surf,mipImprovement))
    {
      rootPart.Delete(last);
      rootFull.Insert(last);
      return true;
    }
    else
    {
      // note: during ReuseMemory last becomes invalid, as texture is removed
      // from the rootPart list and corresponding cache entry is deleted
      texture->ReuseMemory(surf);
      return true;
    }
  }
  return false;
}

bool TextBankD3DXB::Reuse(
  SurfaceInfoD3DXB &surf, const TEXTUREDESCXB &ddsd, PacFormat format,
  bool enableReduction, float mipImprovement
)
{
  PROFILE_SCOPE(texRe);
  // first try discarding previously used
  if (ReuseWithReduction(_previousUsed,_previousUsed,surf,ddsd,format,enableReduction,mipImprovement))
  {
    return true;
  }
  if (ReuseWithReduction(_lastFramePartialUsed,_lastFrameWholeUsed,surf,ddsd,format,enableReduction,mipImprovement))
  {
    return true;
  }
  if (ReuseWithReduction(_thisFramePartialUsed,_thisFrameWholeUsed,surf,ddsd,format,enableReduction,mipImprovement))
  {
    return true;
  }
  return false;
}

#ifndef _XBOX
void TextBankD3DXB::ReuseSys
(
  SurfaceInfoD3DXB &surf, const TEXTUREDESCXB &ddsd, PacFormat format
)
{
  int w=ddsd.w;
  int h=ddsd.h;
  int nMipmaps=ddsd.nMipmaps;
  for
  (
    HSysCacheD3DXB *last=_systemUsed.Last();
    last;
    last=_systemUsed.Prev(last)
  )
  {
    // check mip-map data
    TextureD3DXB *text = last->texture;
    const SurfaceInfoD3DXB &surface = text->_sysSurface;
    if( nMipmaps!=surface._nMipmaps ) continue;
    // match size
    if( surface._w!=w ) continue;
    if( surface._h!=h ) continue;
    // match pixel format
    // if equal - this texture is perfect match
    if( surface._format!=format ) continue;
    
    // surface moved to reusable
    text->ReuseSystem(surf);
    return;
  }
}
#endif

int TextBankD3DXB::FindSurface
(
  int w, int h, int nMipmaps, PacFormat format,
  const AutoArray<SurfaceInfoD3DXB> &array
) const
{
  PROFILE_SCOPE(texFS);
  //if( format==PacAI88 && !_engine->Can88() ) format=PacARGB4444;
  int i;
  for( i=0; i<array.Size(); i++ )
  {
    const SurfaceInfoD3DXB &surface=array[i];
    // match mipmap structure
    if( nMipmaps!=surface._nMipmaps ) continue;
    //const PacLevelMem &pMip=pattern->_mipmaps[0];
    // match size
    if( surface._w!=w ) continue;
    if( surface._h!=h ) continue;
    // match pixel format
    if( surface._format!=format ) continue;
    if (surface.IsBusy()) continue;
    return i;
  }
  return -1;
}

int TextBankD3DXB::DeleteLastReleased( int need, D3DSurfaceDestroyerXB *batch )
{
  #ifdef _XBOX
    batch = NULL;
  #endif
  // LRU is last
  // delete first 
  for (int i=0; i<_freeTextures.Size(); i++)
  {
    SurfaceInfoD3DXB &free=_freeTextures[i];
    if (free.IsBusy()) continue;
    int size=free.SizeUsed();

    _totalSlack -= free.Slack();
    _freeAllocated -= size;
    _totalAllocated -= size;

    ADD_COUNTER(tHeap,size);
    _thisFrameAlloc++;

    #if REPORT_ALLOC>=10
      if (!batch || REPORT_ALLOC>=15)
      {
        LogF
        (
          "VID Released %dx%dx%s (%d B) deleted",
          free._w,free._h,FormatName(free._format),size
        );
      }
    #endif

    if (batch) batch->Add(free);
    free.Free(true,batch!=NULL);
    _freeTextures.Delete(i);

    return size;
  }
  return 0;
}

#ifndef _XBOX
int TextBankD3DXB::DeleteLastSystem( int need, D3DSurfaceDestroyerXB *batch )
{
  // LRU is last
  // delete first 
  SurfaceInfoD3DXB &free=_freeSysTextures[0];
  int size=free.SizeUsed();
  _systemAllocated-=size;
  #if REPORT_ALLOC>=10
    if (!batch || REPORT_ALLOC>=15)
    {
      LogF
      (
        "SYS Released %dx%dx%s (%dB) deleted",
        free._w,free._h,FormatName(free._format),size
      );
    }
  #endif
  if (batch) batch->Add(free);
  //ADD_COUNTER(syRel,1);
  free.Free(true,batch!=NULL);

  _freeSysTextures.Delete(0);
  return size;
}
#endif

void TextBankD3DXB::AddReleased( SurfaceInfoD3DXB &surf )
{
  #if REPORT_ALLOC>=40
    LogF
    (
      "Add to released %dx%dx%s (%dB)",
      surf._w,surf._h,FormatName(surf._format),surf.Size()
    );
  #endif
  _freeTextures.Add(surf);
  _freeAllocated += surf.SizeUsed();
}

void TextBankD3DXB::UseReleased
(
  SurfaceInfoD3DXB &surf,
  const TEXTUREDESCXB &ddsd, PacFormat format
)
{
  int reuse=FindReleased(ddsd.w,ddsd.h,ddsd.nMipmaps,format);
  if( reuse<0 ) return;
  SurfaceInfoD3DXB &reused=_freeTextures[reuse];
  surf=reused;
  #if REPORT_ALLOC>=20
    LogF
    (
      "Released %dx%dx%s (%dB) reused.",
      reused._w,reused._h,FormatName(reused._format),reused.SizeUsed()
    );
  #endif
  _freeAllocated -= reused.SizeUsed();
  reused.Free(false);
  _freeTextures.Delete(reuse);
}

#ifndef _XBOX
int TextBankD3DXB::FindSystem
(
  int w, int h, int nMipmaps, PacFormat format
) const
{
  return FindSurface(w,h,nMipmaps,format,_freeSysTextures);
}
#endif

void TextBankD3DXB::UseSystem(
  SurfaceInfoD3DXB &surf,
  const TEXTUREDESCXB &ddsd, PacFormat format, float mipImprovement
)
{
  PROFILE_SCOPE(texUS);
  #ifndef _XBOX
  int nMipmaps = ddsd.nMipmaps;
  int index=FindSystem(ddsd.w,ddsd.h,nMipmaps,format);
  if( index>=0 )
  {
    surf=_freeSysTextures[index];
    _freeSysTextures.Delete(index);
    #if REPORT_ALLOC>=30
      LogF
      (
        "Used system %dx%dx%s (%dB)",
        surf._w,surf._h,FormatName(surf._format),surf.SizeUsed()
      );
    #endif
  }
  else
  #endif
  {
    #ifdef _XBOX
    if (_totalAllocated>=_limitAlocatedTextures*15/16)
    {
      if (ddsd.w*ddsd.h>_maxSmallTexturePixels)
      {
        if (Reuse(surf,ddsd,format,true,mipImprovement))
        {
          return;
        }
      }
    }

    #endif

    TEXTUREDESCXB ssss=ddsd;
    IDirect3DDevice8 *dDraw=_engine->GetDirect3DDevice();
    int size = surf.CalculateSize(dDraw,ssss,format);
    int sizeNeeded = AlignVRAM(size);
    #ifdef _XBOX
    ReserveMemory(sizeNeeded);
    if (_totalAllocated+sizeNeeded>_limitAlocatedTextures)
    {
      if (!ReserveMemoryUsingReduction(sizeNeeded,mipImprovement))
      {
        #if 1
        Log(
          "Sys surface creation throttled: %d+%d>%d",
          _totalAllocated,sizeNeeded,_limitAlocatedTextures
        );
        #endif
        return;
      }
    }

    Retry:
    #else
    ReserveSystem(sizeNeeded);
    #endif
    // create a new surface

    //ADD_COUNTER(syCre,1);
    //DoAssert(_totalAllocated+sizeNeeded<=_limitAlocatedTextures);
    HRESULT err=surf.CreateSurface(dDraw,ssss,format);
    if( err!=D3D_OK )
    {
      #ifdef _XBOX
        if (ForcedReserveMemory(sizeNeeded)) goto Retry;
        LogF("CreateSurface - out of memory (size %d)",sizeNeeded);
      #endif
      ErrorMessage("Cannot create system memory surface",err);
    }
    else
    {
      #ifndef _XBOX
      _systemAllocated += surf.SizeUsed();
      #else
      _totalAllocated += surf.SizeUsed();
      _totalSlack += surf.Slack();
      //DoAssert(_totalAllocated<=_limitAlocatedTextures);

      #endif
    }
  }
}

#ifndef _XBOX
void TextBankD3DXB::AddSystem( SurfaceInfoD3DXB &surf )
{
  // TODO: XBOX UMA texture reusing
  _freeSysTextures.Add(surf);
  surf.Free(false);
  while( _freeSysTextures.Size()>0 && _systemAllocated>_limitSystemTextures )
  {
    DeleteLastSystem(0);
  }
}
#endif

/*!
\patch 1.03 Date 7/12/2001 by Ondra
- Improved: more robust handling of max. texture size
*/
void TextureD3DXB::ASetNMipmaps( int n )
{
  LoadHeadersNV();
  if (n>_nMipmaps)
  {
    ErrF("Out of range ASetNMipmaps in %s",(const char *)GetName());
    n = _nMipmaps;
  }
  _nMipmaps=n;
  PacLevelMem &mip=_mipmaps[n-1];
  saturateMax(_maxSize,mip._w);
  saturateMax(_maxSize,mip._h);
  if (_largestUsed>_nMipmaps-1) _largestUsed=_nMipmaps-1;
  if (_smallLevel<_largestUsed) _smallLevel = _largestUsed;
}

void TextureD3DXB::MemoryReleased()
{
  // mark as released
  // delete from statistics

  if( _cache )
  {
    _cache->Delete();
    delete _cache;
    _cache=NULL;
  }

  _levelLoaded=_nMipmaps;
}

int TextureD3DXB::ReleaseMemory( SurfaceInfoD3DXB &surf, bool store, D3DSurfaceDestroyerXB *batch )
{
  int ret = 0;
  #ifdef _XBOX
    batch = NULL;
  #endif
  TextBankD3DXB *bank=static_cast<TextBankD3DXB *>(GEngine->TextBank());
  if( surf.GetSurface() )
  {
    //! Invoke the TLH's event
    TextureHasChanged();

    // we need to wait until the surface can be released
    GEngineDD->D3DTextureDestroyed(surf.GetSurface());
    // first of all: move data to reserve bank
    if( store )
    {
      // make sure surface is restored
      // surface might be lost?
      #if REPORT_ALLOC>=20
        LogF
        (
          "VID Large Add To Bank %dx%dx%s (%dB) - '%s'",
          _surface._w,_surface._h,FormatName(_surface._format),_surface.SizeUsed(),
          (const char *)Name()
        );
      #endif
      bank->AddReleased(surf);
    }
    else
    {
      int size = surf.SizeUsed();
      ret += size;
      bank->_totalAllocated -= size;
      bank->_totalSlack -= surf.Slack();

      bank->_thisFrameAlloc++;
      ADD_COUNTER(tHeap,size);
      #if REPORT_ALLOC>=10
        if (!batch || REPORT_ALLOC>=15)
        {
          LogF
          (
            "VID Large Release %dx%dx%s (%dB) - '%s', rest %d, %s",
            _surface._w,_surface._h,FormatName(_surface._format),_surface.SizeUsed(),
            (const char *)Name(),bank->_totalAllocated,
            batch ? "Batch" : ""
          );
        }
      #endif
      if (batch) batch->Add(surf);
    }
    surf.Free(!store,batch!=NULL);
  }
  return ret;
}

int TextureD3DXB::ReleaseMemory( bool store, D3DSurfaceDestroyerXB *batch )
{
  MemoryReleased();
  return ReleaseMemory(_surface,store,batch);
}

#ifndef _XBOX
int TextureD3DXB::ReleaseSystem( bool store, D3DSurfaceDestroyerXB *batch )
{
  int ret = 0;
  TextBankD3DXB *bank=static_cast<TextBankD3DXB *>(GEngine->TextBank());
  if( _sysSurface.GetSurface() )
  {
    if( store )
    {
      // move data to reserve bank
      #if REPORT_ALLOC>=30
        LogF
        (
          "SYS Add To Bank %dx%dx%s (%dB)",
          _sysSurface._w,_sysSurface._h,
          FormatName(_sysSurface._format),_sysSurface.SizeUsed()
        );
      #endif
      bank->AddSystem(_sysSurface);
      _sysSurface.Free(false);
    }
    else
    {
      #if REPORT_ALLOC>=15
        if (!batch || REPORT_ALLOC>=15)
        {
          LogF
          (
            "SYS Release %dx%dx%s (%dB), rest %d, %s",
            _sysSurface._w,_sysSurface._h,
            FormatName(_sysSurface._format),_sysSurface.SizeUsed(),
            bank->_systemAllocated,batch ? "Batch" : ""
          );
        }
      #endif
      bank->_systemAllocated -= _sysSurface.SizeUsed();
      ret += _sysSurface.SizeUsed();
      //ADD_COUNTER(syRel,1);
      if (batch) batch->Add(_sysSurface);
      _sysSurface.Free(true,batch!=NULL);
    }
  }
  SystemReleased();
  return ret;
}
#endif

void TextureD3DXB::ReuseMemory( SurfaceInfoD3DXB &surf )
{
  // reuse immediatelly
  if( _surface.GetSurface() )
  {
    TextureHasChanged();
    // unassign the texture from any stage
    GEngineDD->D3DTextureDestroyed(_surface.GetSurface());
    // first of all: move data to reserve bank

    surf=_surface;
    // allocated memory ammount not changed
    #if REPORT_ALLOC>=20
      LogF
      (
        "VID Directly reused %dx%dx%s (%dB) - '%s'",
        surf._w,surf._h,FormatName(surf._format),surf.SizeUsed(),Name()
      );
    #endif
    _surface.Free(false);
  }
  
  MemoryReleased();
}

static PacFormat DstFormat( PacFormat srcFormat, int dxt )
{
  // memory representation
  switch (srcFormat)
  {
    case PacARGB1555:
    case PacRGB565:
    case PacARGB4444:
      return srcFormat;
    case PacARGB8888:
      if (GEngineDD->Can8888()) return PacARGB8888;
      return PacARGB4444;
    case PacP8:
      return PacARGB1555;
    case PacAI88:
      if (GEngineDD->Can88()) return srcFormat;
      if (GEngineDD->Can8888()) return PacARGB8888;
      return PacARGB4444;
    case PacDXT1:
      if (GEngineDD->CanDXT(1)) return srcFormat;
      else return PacARGB1555;
    case PacDXT2:
      if (GEngineDD->CanDXT(2)) return srcFormat;
      else return PacARGB4444;
    case PacDXT3:
      if (GEngineDD->CanDXT(3)) return srcFormat;
      else return PacARGB4444;
    case PacDXT4:
      if (GEngineDD->CanDXT(4)) return srcFormat;
      else return PacARGB4444;
    case PacDXT5:
      if (GEngineDD->CanDXT(5)) return srcFormat;
      else return PacARGB4444;
    default:
      LogF("Unsupported source format %d",srcFormat);
      Fail("Unsupported source format");
      return srcFormat;
  }
}

#include <El/Debugging/debugTrap.hpp>

void TextureD3DXB::SetMipmapRange( int min, int max )
{
  LoadHeaders();
  if( min<0 ) min=0;
  if( max>_nMipmaps-1 ) max=_nMipmaps-1;
  if( min>max ) min=max;
  // disable mipmaps before min and after max
  _largestUsed = min;
  _nMipmaps = max+1;
  if (_smallLevel<_largestUsed) _smallLevel=_largestUsed;
}

#if _ENABLE_REPORT
void TextureD3DXB::OnUnused() const
{
  #if LOG_INTERESTING
    if (InterestedIn(Name()))
    {
      LogF("TextureD3DXB::OnUnused %s",cc_cast(Name()));
    }
  #endif
  base::OnUnused();
}
#endif

/*!
\patch_internal 1.05 Date 7/16/2001 by Ondra
- Fixed: Older alpha textures without alpha taggs were not recognized,
resulting in bad renderstates while drawing. Alpha artifacts were visible.
- bug introduced while implementing JPG support.
*/

int TextureD3DXB::Init(const char *name)
{
  SetName(name);
  #if LOG_INTERESTING
    if (InterestedIn(Name()))
    {
      LogF("TextureD3DXB::Init %s",cc_cast(Name()));
    }
  #endif

  _maxSize=INT_MAX; // no texture size limit

  // quick check if source does exist
  ITextureSourceFactory *factory = SelectTextureSourceFactory(name);
  if (!factory || !factory->Check(name))
  {
    WarningMessage("Cannot load texture %s.",(const char *)GetName());
    _nMipmaps=0;
    _initialized = true; // we will never be more initialized than we are now
    return -1;
  }

  // make a preload request for the texture header
  // it is very likely we will need the header very soon
  
  PreloadHeaders();
  return 0;
}

int TextureD3DXB::Init(ITextureSource *source)
{
  // no name - directly defined texture
  SetName("");

  _src = source;

  if (_src->Init(_mipmaps,MAX_MIPMAPS))
  {
    _initialized = true;
    DoInitSource();

    #if LOG_INTERESTING
      if (InterestedIn(Name()))
      {
        LogF("Init done %s",cc_cast(Name()));
      }
    #endif
  }
  #if LOG_INTERESTING
    if (InterestedIn(Name()))
    {
      LogF("Init not done %s",cc_cast(Name()));
    }
  #endif
  return 0;
}


void TextureD3DXB::PreloadHeaders()
{
  // if headers are already loaded, there is no need to load them once again
  if (_initialized) return;
  ITextureSourceFactory *factory = SelectTextureSourceFactory(Name());
  factory->PreInit(Name(),this);
  // even if true was returned, the request may sometimes still be pending
  // it may be processed during next file server processing
  // if data were ready, RequestDone should be called during PreInit
  #if LOG_INTERESTING
    if (InterestedIn(Name()))
    {
      LogF("PreInit %s",cc_cast(Name()));
    }
  #endif
}

void TextureD3DXB::RequestDone(RequestContext *context, RequestResult result)
{
  #if LOG_INTERESTING
    if (InterestedIn(Name()))
    {
      LogF("RequestDone %s",cc_cast(Name()));
    }
  #endif
  // note: initialization might be done in between,
  // because it was required by someone else
  //Log("%s: Headers loaded",(const char *)Name());
  if (!_initialized) DoLoadHeaders();
}

void TextureD3DXB::DoInitSource()
{
  if (_src)
  {

    PacFormat format=BasicFormat(GetName());

    bool isPaa = ( format==PacARGB4444 );

    TextBankD3DXB *bank=static_cast<TextBankD3DXB *>(GEngine->TextBank());


    //in.seekg(0,QIOS::beg);
    // .paa should start with format marker

    #if REPORT_ALLOC>=20
    LogF("Init texture %s",(const char *)Name());
    #endif
    format = _src->GetFormat();

    if
    (
      format==PacARGB4444 ||
      format==PacAI88 ||
      format==PacARGB8888
    )
    {
      if (!_src->IsAlpha())
      {
        _src->ForceAlpha();
      }
    }


    int dxt = GEngineDD->DXTSupport();
    PacFormat dFormat = DstFormat(format,dxt);

    // Xbox should support all formats we use
    Assert(dFormat==format || format==PacP8);
    
    if( !_src->IsTransparent() && _src->GetFormat()==PacARGB1555 )
    {
      if( bank->_engine->Can565() && !isPaa ) dFormat=PacRGB565;
    }

    _largestUsed = MAX_MIPMAPS;
    _smallLevel = MAX_MIPMAPS;
    int nMipmaps = _src->GetMipmapCount();

    Assert(nMipmaps <= MAX_MIPMAPS);

    int i;
    for( i=0; i<nMipmaps; i++ )
    {
      PacLevelMem &mip=_mipmaps[i];

      mip.SetDestFormat(dFormat,8);

      if (!mip.TooLarge(_maxSize))
      {
        if (_largestUsed>i) _largestUsed=i;
      }

      // If there is a DXT compression
      if (PacFormatIsDXT(dFormat))
      {
        // If the resolution is smaller than minimum
        if ((mip._w < MIN_MIP_SIZE) || (mip._h < MIN_MIP_SIZE))
        {
          LogF("Warning: Texture is DXT compressed, but its mipmap resolution is smaller than %d.", MIN_MIP_SIZE);
          break;
        }
      }
    }

    _nMipmaps=i;
    Assert(_nMipmaps <= MAX_MIPMAPS);
    
    int limitUse = bank->_maxSmallTexturePixels/4;
    int smallLevel;
    for( smallLevel=_nMipmaps-1; smallLevel>_largestUsed; smallLevel-- )
    {
      const PacLevelMem &mipTop = _mipmaps[smallLevel];
      if( mipTop._w*mipTop._h>=limitUse ) break;
    }
    _smallLevel = smallLevel;

    _levelLoaded=_nMipmaps;
    _levelNeededThisFrame=_levelNeededLastFrame=i; // no level loaded, no needed
  } 
  else
  {
    WarningMessage("Cannot load texture %s.",(const char *)GetName());
    _nMipmaps=0;
    _initialized = true; // we will never be more initialized than we are now
  }
 }

/*!
\patch_ 1.50 Date 4/15/2002 by Ondra
- Optimized: Texture loading is now postponed to "Get ready" screen.
*/

void TextureD3DXB::DoLoadHeaders()
{
  Assert (!_initialized);
  _initialized = true;

  #if LOG_INTERESTING
    if (InterestedIn(Name()))
    {
      LogF("DoLoadHeaders %s",cc_cast(Name()));
    }
  #endif
  
  // if max. size is not set yet, autodetect it
  if (_maxSize>=0x10000)
  {
    TextBankD3DXB *bank=static_cast<TextBankD3DXB *>(GEngine->TextBank());
    (void)bank;
    
    if( !CmpStartStr(Name(),"fonts\\") ) _maxSize = 1024;
    else if( !CmpStartStr(Name(),"merged\\") ) _maxSize = 2048;
    else if( bank->AnimatedNumber(Name())>=0 && IsAlpha() ) _maxSize = Glob.config.maxAnimText;
    else _maxSize=Glob.config.maxObjText;
    #ifdef _XBOX
      if (_maxSize>512) _maxSize = 512;
    #endif
  }

  ITextureSourceFactory *factory = SelectTextureSourceFactory(Name());
  _src = factory->Create(Name(),_mipmaps,MAX_MIPMAPS);

  DoInitSource();
}

void TextureD3DXB::DoUnloadHeaders()
{
  if (_initialized)
  {
    ReleaseMemory();
    #ifndef _XBOX
    ReleaseSystem();
    #endif
    _initialized = false;
    _src.Free();
    #if LOG_INTERESTING
      if (InterestedIn(Name()))
      {
        LogF("DoUnloadHeaders %s",cc_cast(Name()));
      }
    #endif
  }
}

void TextureD3DXB::LoadHeaders()
{
  if (_initialized) return;
  DoLoadHeaders();
}
Color TextureD3DXB::GetColor() const
{
  LoadHeadersNV();
  return _src ? Color(_src->GetAverageColor())*Color(_src->GetMaxColor()) : HBlack;
}
Color TextureD3DXB::GetMaxColor() const
{
  LoadHeadersNV();
  return _src ? _src->GetMaxColor() : HWhite;
}

Color TextureD3DXB::GetPixel( int level, float u, float v ) const
{

  LoadHeadersNV();

  // largest used not initalized until LoadHeadersNV
  // Reduce the level to the largest used texture
  level = max(level, _largestUsed);
  
  // if the texture loaded contains min..max range, let it be...

  if (!_src) return HWhite;

  PacLevelMem mip=_mipmaps[level];

  AutoArray<char> mem;
  int size = MipmapSize(mip._sFormat,mip._w,mip._h);
  mem.Realloc(size);
  mem.Resize(size);

  // load level
  if (_src->GetMipmapData(mem.Data(),mip,level))
  {
    return Color(mip.GetPixel(mem.Data(),u,v))*Color(_src->GetMaxColor());
  }
  return HWhite;
}

DEFINE_FAST_ALLOCATOR(TextureD3DXB);

bool TextureD3DXB::CPUCopyRects(IDirect3DSurface8 *srcSurf, IDirect3DSurface8 *destSurf)
{
  // Check the input parameters
  Assert(srcSurf != NULL);
  Assert(destSurf != NULL);

  // Get the surface description (to determine the size of the surface)
  D3DSURFACE_DESC sdDest;
  destSurf->GetDesc(&sdDest);

#if _DEBUG
  // Compare size of the source and destination surface
  D3DSURFACE_DESC sdSrc;
  destSurf->GetDesc(&sdSrc);
  Assert(sdDest.Size == sdSrc.Size);
#endif

  // Lock Surfaces
  D3DLOCKED_RECT lrSrc, lrDest;
  // D3DLOCK_NOOVERWRITE is used here to indicate the texture is not a render target
  if (FAILED(srcSurf->LockRect(&lrSrc, NULL, D3DLOCK_READONLY|D3DLOCK_NOOVERWRITE)))
  {
    LogF("Error: Locking of a source surface failed.");
    return false;
  }
  if (FAILED(destSurf->LockRect(&lrDest, NULL, 0)))
  {
    srcSurf->UnlockRect();
    LogF("Error: Locking of a destination surface failed.");
    return false;
  }

  // Copy surfaces
  memcpy(lrDest.pBits, lrSrc.pBits, sdDest.Size);

  // Unlock surfaces
  destSurf->UnlockRect();
  srcSurf->UnlockRect();
  return true;
}

TextureD3DXB::TextureD3DXB()
:_nMipmaps(0),
_levelLoaded(63),
_levelNeededThisFrame(0),_levelNeededLastFrame(0), // no level loaded, all needed
_cache(NULL),_inUse(0),
#ifndef _XBOX
_sysCache(NULL),
_systemLoaded(63),
#endif
_maxSize(256),
_initialized(false),_permanent(false)
{
}

TextureD3DXB::~TextureD3DXB()
{
  if (GEngine) GEngine->TextureDestroyed(this);
  bool store = !IsOutOfMemory();
  ReleaseMemory(store);
  #ifndef _XBOX
  ReleaseSystem(store);
  #endif
}

void TextureD3DXB::SetMaxSize( int size )
{
  if (_initialized)
  {
    // limit size by real texture size
    int maxSize = _mipmaps[0]._w;
    saturateMax(maxSize,_mipmaps[0]._h);
    saturateMin(size,maxSize);
    // set max. size
    _maxSize=size;
    // scan for correspondig mipmap level
    int nMipmaps = _src->GetMipmapCount();
    if (nMipmaps<=0) return;
    _largestUsed = MAX_MIPMAPS;
    for (int i=0; i<nMipmaps; i++)
    {
      Assert(i < MAX_MIPMAPS);
      const PacLevelMem &mip=_mipmaps[i];

      if (!mip.TooLarge(_maxSize))
      {
        if (_largestUsed>i) _largestUsed=i;
      }

      // do not load too small mip-maps
      if( mip._w<MIN_MIP_SIZE ) break;
      if( mip._h<MIN_MIP_SIZE ) break;
    }
    if(_largestUsed>_nMipmaps-1) _largestUsed=_nMipmaps-1;
    if (_smallLevel<_largestUsed) _smallLevel = _largestUsed;
  }
  else
  {
    _maxSize = size;
  }
}

D3DSurfaceDestroyerXB::D3DSurfaceDestroyerXB()
{
}

D3DSurfaceDestroyerXB::~D3DSurfaceDestroyerXB()
{
  DestroyAll();
}


static int CmpSurfById( const D3DSurfaceToDestroyXB *s1, const D3DSurfaceToDestroyXB *s2 )
{
  // move small ID to the end
  return s2->_id-s1->_id;
}

void D3DSurfaceDestroyerXB::Add(SurfaceInfoD3DXB &surf)
{
  #ifndef _XBOX
  if (surf.GetSurface())
  {
    D3DSurfaceToDestroyXB destroy;
    destroy._surface = surf.GetSurface();
    destroy._id = surf.GetCreationID();
    _surfaces.Add(destroy);
  }
  #else
  Fail("No batch destroy on Xbox");
  #endif
}

void D3DSurfaceDestroyerXB::DestroyAll()
{
  #ifndef _XBOX
  QSort(_surfaces.Data(),_surfaces.Size(),CmpSurfById);
  #if REPORT_ALLOC>=10
  for (int i=0; i<_surfaces.Size(); i++)
  {
    D3DSurfaceToDestroy &surf = _surfaces[i];
    LogF("  destroy %x, id %d",surf._surface,surf._id);
  }
  #endif

  _surfaces.Clear();
  #endif
}


int SurfaceInfoD3DXB::CalculateSize
(
  IDirect3DDevice8 *dDraw, const TEXTUREDESCXB &desc, PacFormat format,
  int totalSize
)
{
  if (totalSize>0) return totalSize;
  int nMipmaps = desc.nMipmaps;
  int cTotalSize=0;
  int size=MipmapSize(format,desc.w,desc.h);
  for( int level=nMipmaps; --level>=0; )
  {
    cTotalSize+=size;
    size>>=2;
  }
  return cTotalSize;
}

#ifndef _XBOX
int SurfaceInfoD3DXB::_nextId;
#endif

HRESULT SurfaceInfoD3DXB::CreateSurface
(
  IDirect3DDevice8 *dDraw, const TEXTUREDESCXB &desc, PacFormat format, int totalSize
)
{
  if (_surface)
  {
    Fail("Non-free surface created.");
  }

  // TODO: check Can88
  _w = desc.w;
  _h = desc.h;
  _nMipmaps = desc.nMipmaps;
  Debug:
  {
    #if _ENABLE_REPORT
    if( totalSize<0 )
    #endif
    {
      int cTotalSize=0;
      int size=MipmapSize(format,_w,_h); // assume 16-b textures
      for( int level=_nMipmaps; --level>=0; )
      {
        cTotalSize+=size;
        size>>=2;
      }
      if( totalSize<0 ) totalSize=cTotalSize;
      DoAssert( totalSize==cTotalSize );
    }
  }
  _format=format;
  _totalSize=totalSize;
  #ifndef _XBOX
  if (desc.pool!=D3DPOOL_SYSTEMMEM)
  #endif
  {
    _usedSize=AlignVRAM(totalSize);
  }

  #ifndef _XBOX
  HRESULT err=dDraw->CreateTexture
  (
    _w,_h,_nMipmaps,0,
    desc.format,desc.pool,
    _surface.Init()
  );
  #else
    // create a texture and allocate texture memory manually

    _surface = new IDirect3DTexture8;

    HRESULT err = D3D_OK;
    //Assert(desc.pitch > 0);
    XGSetTextureHeader( _w, _h, _nMipmaps, 0, desc.format, desc.pool, _surface, 0, 0/*desc.pitch*/);
    #if _ENABLE_REPORT
    if (totalSize==0)
    {
      LogF("%dx%d:%d, _usedSize %d, format %s",_w,_h,_nMipmaps,_usedSize,FormatName(format));
      goto Debug;
    }
    #endif
    _physicalMemory = AllocateTextureMemory(totalSize);
    if (_physicalMemory.IsNull())
    {
      err = E_OUTOFMEMORY;
      _surface.Free();
      //_physicalMemory.Free();
    }
    else
    {
      _surface->Register( _physicalMemory.GetAddress() );
    }
  #endif
  if (err!=D3D_OK)
  {
    LogF("Dimensions %d:%d,%d",_w,_h,_nMipmaps);
    DDErrorXB("Create Surface Error",err);
  }
  else
  { 
    #ifndef _XBOX
    D3DSURFACE_DESC desc;
    int totSize = 0;
    // check real surface size
    for (int i=0; i<_nMipmaps; i++)
    {
      int level = i;
      _surface->GetLevelDesc(level,&desc);
      totSize += desc.Size;
    }
    _usedSize = AlignVRAM(totSize);
    _id = _nextId++;
    #else
    _usedSize = AlignVRAM(totalSize);
    #endif

  }
  #if REPORT_ALLOC>=10
    if( err==D3D_OK )
    {
      const char *mem="VDA";
      if( desc.pool==D3DPOOL_SYSTEMMEM ) mem="SYS";
      else if( desc.pool==D3DPOOL_DEFAULT ) mem="VID";
      LogF("%s: Created %dx%dx%s (%dB)",mem,_w,_h,FormatName(format),SizeUsed());
    }
  #endif
  return err;
}

#ifdef _XBOX  
void SurfaceInfoD3DXB::Copy(const SurfaceInfoD3DXB &src)
{
  _surface = src._surface;
  _physicalMemory = src._physicalMemory, src._physicalMemory = PhysMemHeapItemXB();

  _totalSize = src._totalSize;
  _usedSize = src._usedSize;
  _w = src._w;
  _h = src._h;
  _nMipmaps = src._nMipmaps;
  _format = src._format;
  _fence = src._fence;

  // Disable the original fence
  src._fence = 0;
}
#endif

void SurfaceInfoD3DXB::Free(bool lastRef, int refValue)
{
  //GEngineDD->TextBankDD()->ReportTextures("* Free [");
  // before freeing surface free its associated memory
  #ifdef _XBOX
    if (lastRef)
    {
      // make sure surface is no longer assigned to any stage
      GEngineDD->D3DTextureDestroyed(_surface);
      if (_physicalMemory.NotNull())
      {
        // wait until GPU no longer uses the resource
        if (_surface->IsBusy())
        {
          Log("Waiting for GPU to release the resource");
          _surface->BlockUntilNotBusy();
          Verify( !_surface->IsBusy() ); // this should reset the Lock member?
        }
        // Wait while fence is pending
        if (GEngineDD->GetDirect3DDevice()->IsFencePending(_fence))
        {
          GEngineDD->GetDirect3DDevice()->BlockOnFence(_fence);
        }
        Assert(_surface->Lock==0);
        FreeTextureMemory(_physicalMemory);
        //_physicalMemory = NULL;
      }
      Assert((_surface->Common&0xffff)==1);
      Assert(_surface->Lock==0);
      _surface.Free();
    }
    else
    {
      Assert(_physicalMemory.IsNull());
      Assert(_surface.IsNull());
    }
  #else
    IDirect3DTexture8 *surf = _surface;
    int result = _surface.Free();
    if (lastRef)
    {
      #if REPORT_ALLOC>=10
      if (surf && refValue==0)
      {
        LogF("  destroy %x, id %d, size %d",surf,_id,_usedSize);
      }
      #endif
      #if 1 //ndef _XBOX
      if (result!=refValue)
      {
        RptF("Surf: Some references to %x left (%d!=%d)",surf,result,refValue);
      }
      #endif
    }
  #endif
  //GEngineDD->TextBankDD()->ReportTextures("* Free ]");
  _w=0;
  _h=0;
  _nMipmaps=0;
  _totalSize=0;
  _usedSize = 0;
}

int TextBankD3DXB::CreateVRAMSurface
(
  SurfaceInfoD3DXB &surface,
  const TEXTUREDESCXB &desc, PacFormat format, int totalSize
)
{
  // VRAM creation - mark it
  TEXTUREDESCXB ddsd=desc;

  IDirect3DDevice8 *dDraw=_engine->GetDirect3DDevice();

  bool retry = false;
  Retry:
  //ReportTextures("* Create [");

  HRESULT err=surface.CreateSurface(dDraw,ddsd,format,totalSize);

  //ReportTextures("* Create ]");
  if (retry)
  {
    if (err!=D3D_OK)
    {
      DDErrorXB("Create Surface Error",err);
    }
    else
    {
      LogF("Retry OK");
    }
  }

  if(err==E_OUTOFMEMORY)
  {
    // first try to deallocate some convential memory
    #ifndef _XBOX
    int rest = _freeTextureMemory-_reserveTextureMemory;
    if (rest>=totalSize)
    {
      // it seems we need more texture memory than expected
      _reserveTextureMemory+=rest-totalSize;
    }
    #endif
    if (_freeTextures.Size()<=0)
    {
      ReportTexturesXB("Forced Reserve report");
    }
    if( ForcedReserveMemory(totalSize) )
    {
      retry = true;
      goto Retry;
    }
    LogF("SYS: No space left %d",totalSize);
    return -1; // out of memory
  }
  else if(err==D3DERR_OUTOFVIDEOMEMORY )
  {
    // video memory error reported
    // first try to evict some video memory, than system memory
    Log("Out of video memory.");
    int rest = _freeTextureMemory-_reserveTextureMemory;
    if (rest>=totalSize)
    {
      // it seems we need more texture memory than expected
      _reserveTextureMemory+=rest-totalSize;
    }
    if( ForcedReserveMemory(totalSize) )
    {
      retry = true;
      goto Retry;
    }
    LogF("VID: No space left %d",totalSize);
    return -1; // out of memory
  }
  if( err ) DDErrorXB("Cannot create video memory mipmap.",err);
  Assert( err==D3D_OK );
  _totalAllocated += surface.SizeUsed();
  _totalSlack += surface.Slack();
  //DoAssert(_totalAllocated<=_limitAlocatedTextures);
  return 0;
}

/**
@param noThrottling memory usage can grow over limit temporarily
@param mipImprovement ignored when noThrottling
*/

int TextBankD3DXB::CreateVRAMSurfaceSmart(
  SurfaceInfoD3DXB &surface,
  const TEXTUREDESCXB &desc, PacFormat format, int totalSize, bool noThrottling, float mipImprovement
)
{
  UseReleased(surface,desc,format);

  if(!surface.GetSurface())
  {
    // do not try reusing until used nearly all video memory
    if( _totalAllocated>_limitAlocatedTextures*15/16)
    {
      // avoid hasty release of usefull data
      // if there are too many free textures, release some of them first
      // if is possible that they are in some format which is not very common
      if (_freeAllocated<_limitAlocatedTextures*1/16)
      {
        Reuse(surface,desc,format,true,mipImprovement);
      }
    }

    if (!surface.GetSurface())
    {
      // if nothing can be reused, we have to create something new
      int neededSize = AlignVRAM(totalSize);
      ReserveMemory(neededSize);
      
      // if we are not throtlling, we do not care about a little overgrowing the limits
      if (_totalAllocated+neededSize>_limitAlocatedTextures && !noThrottling)
      {
        if (!ReserveMemoryUsingReduction(neededSize,mipImprovement))
        {
          //LogF("CreateVRAMSurface throttled (%d,%d)",neededSize,_totalAllocated);
          return -1;
        }
      }

      if( CreateVRAMSurface(surface,desc,format,totalSize)<0 )
      {
        return -1;
      }

      _thisFrameAlloc++;

      ADD_COUNTER(tGRAM,totalSize);
    }
    // we created or reused a texture
  }

  Assert( surface.SizeExpected()==totalSize );
  #if _ENABLE_REPORT
    if (surface.IsBusy())
    {
      LogF("Reused surface busy");
    }
  #endif
    
  return 0;
}

bool TextureD3DXB::VerifyChecksum( const MipInfo &absMip ) const
{
  return true;
}

// constructor

TextBankD3DXB::TextBankD3DXB( EngineDDXB *engine, ParamEntryPar cfg, ParamEntryPar fcfg )
:_totalAllocated(0),_totalSlack(0),
#ifndef _XBOX
_systemAllocated(0),
#endif
_freeAllocated(0)
{
  //RegisterFreeOnDemandSystemMemory(this);

  //const int mem2MB=;
  //_limitAlocatedTextures=mem2MB; // this should be good estimation on any card
  //_limitSystemTextures=( Glob.config.heapSize*(1024*1024) );
  //_limitSystemTextures=2*1024*1024;
  #ifndef _XBOX
  _limitSystemTextures=1024*1024;
  #endif
  _reserveTextureMemory = 0;
  _engine=engine;

  // will set _limitAlocatedTextures and _freeTextureMemory

  _maxTextureMemory = INT_MAX;

  CheckTextureMemory();

  #ifndef _XBOX
  _maxTextureMemory = _freeTextureMemory;
  #endif

  // as bias is adjusted automatically, it does not matter much what we set here
  float bias = fcfg.ReadValue("textureMipBias",2.0f);
  SetTextureMipBias(bias);
  SetTextureMipBiasMin(bias);
  
  // set small texture limit based on free memory
  // assume there will be about 1000 loaded simultaneosly
  // we want to use about 1/8 of texture memory for small textures
  // assume 16-bit textures

  int limitPixels = _limitAlocatedTextures/(2*1024*8);
  int limitPixelsPow2 = 1;
  while (limitPixelsPow2+limitPixelsPow2<limitPixels)
  {
    limitPixelsPow2+=limitPixelsPow2;
  }

  _maxSmallTexturePixels = limitPixelsPow2;
  // on Xbox we have custom allocator capable of allocating 256B aligned blocks
  saturateMax(_maxSmallTexturePixels,16*16);
  LogF
  (
    "Max small texture pixels: %d : %d, %.0f",
    limitPixelsPow2,_maxSmallTexturePixels,
    sqrt(float(_maxSmallTexturePixels))
  );

  //PreCreateVRAM(512*1024,true);
}

TextBankD3DXB::~TextBankD3DXB()
{
  DeleteAllAnimated();
  _texture.RemoveNulls();
  _white.Free();
  _defNormMap.Free();
  _defDetailMap.Free();
  _permanent.Clear();
  _texture.RemoveNulls();
  Assert (_texture.Size()==0);
  _texture.Clear();
  //_ditherMap.Free();
  ClearTexCache();

  if (!IsOutOfMemory())
  {
    // use batch destroyed to achive fast texture destruction
    // by destroying in reverse order to creation order
    // this is possible only when we have enough memory to store the batch
    D3DSurfaceDestroyerXB batch;
    #ifndef _XBOX
    for( int i=0; i<_freeTextures.Size(); i++ )
    {
      batch.Add(_freeTextures[i]);
    }
    for( int i=0; i<_freeSysTextures.Size(); i++ )
    {
      batch.Add(_freeSysTextures[i]);
    }
    _freeSysTextures.Clear();
    #else
    for( int i=0; i<_freeTextures.Size(); i++ )
    {
      _freeTextures[i].Free(true);
    }
    #endif
    _freeAllocated = 0;
    _freeTextures.Clear();
    batch.DestroyAll();
  }
  else
  {
    _freeTextures.Clear();
    _freeAllocated = 0;
    #ifndef _XBOX
    _freeSysTextures.Clear();
    #endif
  }
}

void TextBankD3DXB::Compact()
{
  _texture.RemoveNulls();
}

void TextBankD3DXB::StopAll()
{
  //_loader->DeleteAll();
}

static void SurfaceName( char *buf, const SurfaceInfoD3DXB &surface )
{
  const char *name="???";
  switch( surface._format )
  {
    case PacP8: name="PacP8";break;
    case PacAI88: name="PacAI88";break;
    case PacRGB565: name="PacRGB565";break;
    case PacARGB1555: name="PacARGB1555";break;
    case PacARGB4444: name="PacARGB4444";break;
    case PacARGB8888: name="PacARGB8888";break;
    case PacDXT1: name="PacDXT1";break;
    case PacDXT2: name="PacDXT2";break;
    case PacDXT3: name="PacDXT3";break;
    case PacDXT4: name="PacDXT4";break;
    case PacDXT5: name="PacDXT5";break;
  }
  sprintf(buf,"\t%s,%d,%d,",name,surface._w,surface._h);
}

#include <El/Statistics/statistics.hpp>

bool TextBankD3DXB::VerifyChecksums()
{
  StatisticsByName stats;
  // print released textures statistics
  for( int i=0; i<_freeTextures.Size(); i++ )
  {
    const SurfaceInfoD3DXB &surface=_freeTextures[i];
    char name[80];
    SurfaceName(name,surface);
    stats.Count(name);
  }
  LogF("Unused texture surfaces");
  stats.Report();
  stats.Clear();

  // print used textures statistics
  LogF("Used texture surfaces");
  for( int i=0; i<_texture.Size(); i++ )
  {
    TextureD3DXB *texture=_texture[i];
    if( !texture ) continue;
    if( texture->_surface.GetSurface() )
    {
      char name[80];
      SurfaceName(name,texture->_surface);
      stats.Count(name);
    }
    
  }
  stats.Report();
  stats.Clear();

  /*
  int i;
  for( i=0; i<_texture.Size(); i++ ) 
  {
    Assert( _texture[i] );
    if( !_texture[i]->VerifyChecksum(NULL) )
    {
      Log("Texture %s checksum failed",_texture[i]->Name());
      return false;
    }
  }
  */
  return true;
}


int TextBankD3DXB::Find(RStringB name1)
{
  int i;
  //RString cName(name1);
  for( i=0; i<_texture.Size(); i++ )
  {
    TextureD3DXB *texture=_texture[i];
    if( texture )
    {
      if ((name1 != texture->GetName())) continue;
      return i;
    }
  }
  return -1;
}

int TextBankD3DXB::FindFree()
{
  int i;
  for( i=0; i<_texture.Size(); i++ )
  {
    TextureD3DXB *texture=_texture[i];
    if( !texture ) return i;
  }
  return _texture.Size();
}

TextureD3DXB *TextBankD3DXB::Copy( int from )
{
  if (from<0) return NULL;
  const TextureD3DXB *source=_texture[from];
  if (!source) return NULL;
  const char *sName = source->Name();

  int iFree=FindFree();
  Assert( iFree>=0 );
  TextureD3DXB *texture=new TextureD3DXB;
  
  if (texture->Init(sName))
  {
    //texture->Init("data\\default.pac");
    return NULL;
  }
  //texture->InitLoaded();
  _texture.Access(iFree);
  _texture[iFree]=texture;
  return texture; 
}

Ref<Texture> TextBankD3DXB::CreateTexture(ITextureSource *source)
{
  Ref<TextureD3DXB> texture = new TextureD3DXB;
  if (!texture) return NULL;
  if (texture->Init(source)) return NULL;
  return texture.GetRef();
}

size_t TextureD3DXB::GetMemoryControlled() const
{
  return sizeof(*this);
}

void TextureD3DXB::RegisterTextureLoadHandler(TextureLoadHandler *tlh)
{
  _textureLoadHandlers.Insert(tlh);
}

void TextureD3DXB::TextureHasChanged()
{
  for (TextureLoadHandler *tlh = _textureLoadHandlers.First(); tlh; tlh = _textureLoadHandlers.Next(tlh))
  {
    tlh->TextureHasChanged();
  }
}

void TextureD3DXB::BindWithPushBuffer()
{
  _surface.SetFence(GEngineDD->GetPBLock());
}

Ref<Texture> TextBankD3DXB::Load(RStringB name)
{
  // if the texture is cached, the link still exists 
  // and Find should be sucessfull
  int i=Find(name);
  if( i>=0 )
  {
    // make sure texture is cached
    Texture *tex = _texture[i];
    return tex;
  }
  int iFree = _texture.FindKey(NULL);
  if (iFree<0) iFree=_texture.Size();

  Ref<Texture> texture = LoadCached(name);
  if (!texture)
  {
    Ref<TextureD3DXB> tex=new TextureD3DXB;
    if (tex->Init(name))
    {
      // set name to empty to avoid inserting in the cache
      tex->SetName(RStringB());
      return NULL;
    }
    texture = tex;
  }
  else
  {
    // request may be canceled now - restart it if necessary
    TextureD3DXB *txt=static_cast<TextureD3DXB *>(texture.GetRef());
    txt->PreloadHeaders();
  }

  _texture.Access(iFree);
  TextureD3DXB *txt=static_cast<TextureD3DXB *>(texture.GetRef());
  _texture[iFree]=txt;
  
  #if LOG_INTERESTING
    if (InterestedIn(texture->Name()))
    {
      LogF("TextBankD3DXB::Load %s",cc_cast(texture->Name()));
    }
  #endif
  
  Assert(!texture->IsInList());
  
  return texture;
}

// maintain texture cache

void TextBankD3DXB::ReleaseAllTextures(bool releaseSysMem)
{
  LogF("Allocated before ReleaseAllTextures %d",_totalAllocated);
  if (_defDetailMap) _defDetailMap->ReleaseMemory();
  //if (_ditherMap) _ditherMap->ReleaseMemory();
  if (_defNormMap) _defNormMap->ReleaseMemory();
  if (_white) _white->ReleaseMemory();
  ReserveMemory(_previousUsed,0);
  ReserveMemory(_lastFramePartialUsed,0);
  ReserveMemory(_lastFrameWholeUsed,0);
  ReserveMemory(_thisFramePartialUsed,0);
  ReserveMemory(_thisFrameWholeUsed,0);
  LogF("Allocated after ReleaseAllTextures %d",_totalAllocated);
  #if !_RELEASE
  if (_totalAllocated>0)
  {
    // scan what is allocated
    for (int i=0; i<_texture.Size(); i++)
    {
      TextureD3DXB *texture = _texture[i];
      if (texture->GetBigSurface()) Fail((const char *)texture->Name());
    }
    if( _freeTextures.Size()>0 )
    {
      Fail("FreeTextures");
    }
  }
  #endif
  #ifndef _XBOX
  if (releaseSysMem)
  {
    //if (_ditherMap) _ditherMap->ReleaseSystem();
    if (_defDetailMap) _defDetailMap->ReleaseSystem();
    if (_defNormMap) _defNormMap->ReleaseSystem();
    if (_white) _white->ReleaseSystem();
    ReserveSystem(_systemUsed,0);
    if (_freeSysTextures.Size()>0)
    {
      Fail("Some free sys texture");
      _freeSysTextures.Clear();
    }
    if (_systemAllocated>0)
    {
      LogF("_systemAllocated %d",_systemAllocated);
    }
  }
  #endif
}


int TextBankD3DXB::CalculateMemoryInUse( D3DMipCacheRootXB &root)
{
  int total = 0;
  for (HMipCacheD3DXB *item=root.Start(); root.NotEnd(item); item=root.Advance(item))
  {
    TextureD3DXB *tex = item->texture;
    if (!tex->_inUse) continue;
    int size = tex->_surface.SizeUsed();
    total += size;
  }
  return total;
}

int TextBankD3DXB::CalculateMemoryInUseGPU( D3DMipCacheRootXB &root)
{
  int total = 0;
  for (HMipCacheD3DXB *item=root.Start(); root.NotEnd(item); item=root.Advance(item))
  {
    TextureD3DXB *tex = item->texture;
    if (!tex->_surface.IsBusy()) continue;
    int size = tex->_surface.SizeUsed();
    total += size;
  }
  return total;
}

int TextBankD3DXB::CalculateMemory( D3DMipCacheRootXB &root)
{
  int total = 0;
  for (HMipCacheD3DXB *item=root.Start(); root.NotEnd(item); item=root.Advance(item))
  {
    TextureD3DXB *tex = item->texture;
    int size = tex->_surface.SizeUsed();
    total += size;
  }
  return total;
}
int TextBankD3DXB::CalculatePartMemory( D3DMipCacheRootXB &root)
{
  int total = 0;
  for (HMipCacheD3DXB *item=root.Start(); root.NotEnd(item); item=root.Advance(item))
  {
    TextureD3DXB *tex = item->texture;
    int needed = toIntFloor(tex->LevelNeeded());
    int unused = needed-tex->_levelLoaded;
    if (unused<0) unused = 0;
    int size = tex->_surface.SizeUsed()>>(unused*2);
    total += size;
  }
  return total;
}

int TextBankD3DXB::ReserveMemory( D3DMipCacheRootXB &root, int limit )
{
  #define LOG_TIME 0
  #if LOG_TIME
  DWORD start = GlobalTickCount();
  #endif

  D3DSurfaceDestroyerXB batchDestroy;
  bool someReleased=false;
  int totalReleased = 0;
  while( _freeTextures.Size()>0 && _totalAllocated>limit )
  {
    // free more than one at once
    int need=_totalAllocated-limit;
    size_t released = DeleteLastReleased(need,&batchDestroy);
    if (released<=0) break;
    totalReleased += released;
    someReleased=true;
  }
  // release as many mip-map data as neccessary to get required memory
  // get last mip-map used
  HMipCacheD3DXB *last=root.Last();
  while( _totalAllocated>limit )
  {
    if( !last ) break;

    // release texture data
    TextureD3DXB *texture=last->texture;
    if( texture->_inUse )
    {
      last=root.Prev(last);
      continue;
    }

    // release texture data from texture heap
    Assert( texture->_cache==last );
    
    someReleased=true;
    totalReleased += texture->ReleaseMemory(false,&batchDestroy);
    last=root.Last();
  }
  if (someReleased)
  {
    batchDestroy.DestroyAll();
    #if LOG_TIME
    DWORD end = GlobalTickCount();
    LogF("Time to release: %d",end-start);
    #endif
  }
  else
  {
    //LogF("Nothing released: total %d, limit %d",_totalAllocated,limit);
    //LogF("  _freeTextures.Size() %d",_freeTextures.Size());
    // check if all textures are in use
  }
  return totalReleased;
}

void TextBankD3DXB::ReportTextures(const char *name)
{
  #if _ENABLE_PERFLOG
  // report all textures loaded into VRAM and sysRAM
  LogFile file;
  #ifndef _XBOX
  if (name && strchr(name,'.') && !strchr(name,'*') )
  {
    file.Open(name);
  }
  #else
  file.AttachToDebugger();
  #endif
  // report some global information
  {

    #ifndef _XBOX
      IDirect3DDevice8 *dDraw=_engine->GetDirect3DDevice();
      UINT dwFree = dDraw->GetAvailableTextureMem();
    #else
      UINT dwFree = 0;
    #endif

    file << " Max " << _maxTextureMemory << " Free " << (int)dwFree << "\n";
    file << "Allocated " << _totalAllocated << "\n";

    int overhead = _maxTextureMemory - dwFree - _totalAllocated;
    file << "Overhead " << overhead << "\n";
    LogF
    (
      "%s: Free %d, Overhead %d, Reserved %d",
      name,dwFree,overhead,_reserveTextureMemory
    );
    if (overhead>4000000)
    {
      // something wrong happened - very big memory overhead
      static bool once=true;
      if (once)
      {
        LogF("#### Overhead #####");
        once=false;
      }
    }
  }


  if (file.IsOpen())
  {
    int usedInBigTextures = 0;
    int usedInFreeTextures = 0;
    int usedInSysTextures = 0;
    for (int i=0; i<_texture.Size(); i++)
    {
      TextureD3DXB *texture = _texture[i];
      if (!texture) continue;
      if (texture->_surface.GetSurface() )
      {
        usedInBigTextures += texture->_surface.SizeUsed();
      }
      #ifndef _XBOX
      if (texture->_sysSurface.GetSurface())
      {
        usedInSysTextures+= texture->_sysSurface.SizeUsed();
      }
      #endif
    }
    for (int i=0; i<_freeTextures.Size(); i++)
    {
      SurfaceInfoD3DXB &free = _freeTextures[i];
      if (free.GetSurface()) usedInFreeTextures+=free.SizeUsed();
    }
    int usedInTextures = usedInBigTextures + usedInFreeTextures;
    file << "Used in big   textures: " << usedInBigTextures   << "\n";
    file << "Used in free  textures: " << usedInFreeTextures  << "\n";
    file << "Used (total)          : " << usedInTextures      << "\n";
    file << "Used in sys   textures: " << usedInSysTextures  << "\n";

    #if 1 //ndef _XBOX
    for (int i=0; i<_texture.Size(); i++)
    {
      TextureD3DXB *texture = _texture[i];
      if (!texture) continue;
      if (!texture->_initialized) continue;
      if (texture->_levelLoaded<texture->_nMipmaps )
      {
        file.PrintF("%-32s",texture->Name());
        file.PrintF(" mips: %d",texture->NMipmaps());
        file.PrintF(
          " loaded %d, needed %.2f",texture->_levelLoaded,texture->LevelNeeded()
        );
        const PacLevelMem &mip = texture->_mipmaps[texture->_levelLoaded];
        file.PrintF(" %dx%d, memUsed: %d",mip._w,mip._h,texture->TotalSize(texture->_levelLoaded));
        if (texture->GetSurface().IsBusy())
        {
          file.PrintF(" * Busy");
        }
        file << "\n";
      }
    }
    #endif
  }
  #endif
}

float TextBankD3DXB::PriorityTexVidMem() const
{
  return 4.0; // texture priority is very high, because textures are needed very often
}
RString TextBankD3DXB::GetDebugNameTexVidMem() const
{
  return "Tex Vid Mem";
}

size_t TextBankD3DXB::MemoryControlledTexVidMem() const
{
  #ifdef _XBOX
  return _totalAllocated;
  #else
  return _systemAllocated;
  #endif
}
size_t TextBankD3DXB::MemoryControlledRecentTexVidMem() const
{
  return 0;
}

void TextBankD3DXB::MemoryControlledFrameTexVidMem()
{
}

size_t TextBankD3DXB::FreeOneItemTexVidMem()
{
  // this function is called during on-progress aggressive allocation
  #ifdef _XBOX
    int startMem = CountPhysicalTextureMemory();
    int limitMem = startMem-16*1024;
    LimitPhysicalTextureMemory(limitMem);
    int endMem = CountPhysicalTextureMemory();
    return startMem-endMem;
  #else
    return ReserveSystem(16*1024);
  #endif
}


bool TextBankD3DXB::ReduceSomeTexture(D3DMipCacheRootXB &root, D3DMipCacheRootXB &whole)
{
  HMipCacheD3DXB *last=root.Last();
  while(last)
  {
    if( !last ) break;

    // release texture data
    TextureD3DXB *texture=last->texture;
    if( !texture->_inUse && !texture->GetSurface().IsBusy() )
    {
      if (texture->ReduceTexture())
      {
        root.Delete(last);
        whole.Insert(last);
        return true;
      }
      // how came? Texture is used only partially - we should be able to reduce it
      #if _DEBUG
        __asm nop;
      #endif
    }
    last=root.Prev(last);
  }
  return false;
}

void TextBankD3DXB::FreePhysicalTextureMemory(int toFree)
{
  int physAllocated = CountPhysicalTextureMemory();
  if (physAllocated<=0) return;
  int physAllocated0 = physAllocated;
  for(;;)
  {
    if (physAllocated0-toFree>=physAllocated) return;
    // estimate how much we need to discard
    int needToDiscard = physAllocated-(physAllocated0-toFree);
    int totalAllocatedWanted = _totalAllocated-needToDiscard;
    LimitTextureMemory(totalAllocatedWanted);
    physAllocated = CountPhysicalTextureMemory();
  }
}

int TextBankD3DXB::CountPhysicalTextureMemory() const
{
  return GetTextureMemoryAllocated();
}

bool TextBankD3DXB::LimitPhysicalTextureMemory(int limit)
{
  bool someReleased = false;
  for(;;)
  {
    int physAllocated = CountPhysicalTextureMemory();
    if (physAllocated<=limit) return someReleased;
    // estimate how much we need to discard
    int totalAllocatedWanted = _totalAllocated-(physAllocated-limit);
    if (LimitTextureMemory(totalAllocatedWanted))
    {
      someReleased = true;
    }
    else break;
  }
  return someReleased;
}

bool TextBankD3DXB::LimitTextureMemory(int totalAllocatedWanted)
{
  if (totalAllocatedWanted<0) totalAllocatedWanted = 0;
  bool someReleased = false;
  for(;;)
  {
    bool someReleasedNow = false;
    if (ReserveMemory(_previousUsed,totalAllocatedWanted))
    {
      someReleasedNow = true;
      someReleased = true;
    }
    if (_totalAllocated<=totalAllocatedWanted) return someReleased;
    if (LimitMemoryUsingReduction(totalAllocatedWanted))
    {
      someReleased = true;
      someReleasedNow = true;
    }
    if (_totalAllocated<=totalAllocatedWanted) return someReleased;
    if (!someReleasedNow) return someReleased;
  }
}

bool TextBankD3DXB::ReserveMemoryUsingReduction(int size, float mipImprovement)
{
  PROFILE_SCOPE(texRU);
  int maxAllocAllowed = _limitAlocatedTextures-AlignVRAM(size);
  if (LimitMemoryUsingReduction(maxAllocAllowed)) return true;
  if (LimitMemoryUsingMipBias(maxAllocAllowed,mipImprovement)) return true;
  return false;
}

void TextBankD3DXB::ChangeMipBias(D3DMipCacheRootXB &root, float change)
{
  for(HMipCacheD3DXB *item=root.Last();item; item=root.Prev(item))
  {
    TextureD3DXB *texture=item->texture;
    texture->_levelNeededLastFrame += change;
    texture->_levelNeededThisFrame += change;
  }
}

float TextBankD3DXB::FindWorstBias(D3DMipCacheRootXB &root, float &maxBias, TextureD3DXB *&maxTex) const
{
  for(HMipCacheD3DXB *item=root.Last();item; item=root.Prev(item))
  {
    // release texture data
    TextureD3DXB *texture=item->texture;
    if( texture->_inUse) continue;
    // if nothing is loaded, nothing can be reduced
    if (texture->_levelLoaded>=texture->_nMipmaps) continue;
    if (texture->_levelLoaded>=texture->_smallLevel) continue;
    //if (texture->GetHandle()->IsBusy()) continue;
    float bias = texture->LevelNeeded()-texture->_levelLoaded;
    if (maxBias<bias) maxBias = bias,maxTex = texture;
  }
  return maxBias;
}

/**
This function is rather drastic measure.
Regular maintenance should keep mipbias so that this function is almost never needed.
*/
bool TextBankD3DXB::LimitMemoryUsingMipBias(int maxAllocAllowed, float mipImprovement)
{
  bool ret = true;
  float origBias = GetTextureMipBias();
  #if _ENABLE_REPORT
    bool biasChanged = false;
    int thisUsed0 = CalculateMemory(_thisFrameWholeUsed);
    int lastUsed0 = CalculateMemory(_lastFrameWholeUsed);
    thisUsed0 += CalculatePartMemory(_thisFramePartialUsed);
    lastUsed0 += CalculatePartMemory(_lastFramePartialUsed);
    if (thisUsed0+lastUsed0<_limitAlocatedTextures*3/4)
    {
      int thisUsedGPU = CalculateMemoryInUseGPU(_thisFrameWholeUsed);
      thisUsedGPU += CalculateMemoryInUseGPU(_lastFrameWholeUsed);
      thisUsedGPU += CalculateMemoryInUseGPU(_thisFramePartialUsed);
      thisUsedGPU += CalculateMemoryInUseGPU(_lastFramePartialUsed);
      if (thisUsedGPU<_limitAlocatedTextures*7/8)
      {
        LogF(
          "Strange reduction - memory already OK (%d, GPU %d)",
          thisUsed0+lastUsed0,thisUsedGPU
        );
      }
    }
  #endif
  while(_totalAllocated>maxAllocAllowed)
  {
    // find a texture which has most space between what is loaded and what is needed
    // (or find the texture, which has loaded closest to what is needed and can be discarded first)
    // all textures should 
    float worstBias = -FLT_MAX;
    TextureD3DXB *tex = NULL;
    FindWorstBias(_lastFramePartialUsed,worstBias,tex);
    FindWorstBias(_lastFrameWholeUsed,worstBias,tex);
    FindWorstBias(_thisFramePartialUsed,worstBias,tex);
    FindWorstBias(_thisFrameWholeUsed,worstBias,tex);
    FindWorstBias(_previousUsed,worstBias,tex);
    if (!tex)
    {
      ret = false;
      break;
    }
    // if worstBias is positive and large enough, there is no need to change bias
    // all we need to do is reducing the texture
    if (worstBias>1)
    {
      // this probably will never happen, as such textures should be catched earlier
      int tgtLevel = toIntFloor(tex->_levelLoaded+worstBias);
      if (!tex->ReduceTextureLevel(tgtLevel))
      {
        // if we cannot reduce, we can evict the texture completely
        tex->ReleaseMemory();
      }
      continue;
    }
    // we need to reduce mipbias so that this texture is evicted
    // this means adjusting bias to that worstBias becomes at least 1
    // change it a little bit more than neccesary to increase chance
    // it will not be changed again very soon
    float oldBias = GetTextureMipBias();
    float newBias = floatMin(oldBias+(1-worstBias)+0.2f,5);
    if (newBias>oldBias)
    {
      // it has no sense to degrade mipbias so that even the newly loaded texture is degraded
      if (newBias>origBias+mipImprovement)
      {
        break;
      }
      float changeBias = newBias-oldBias;
      
      #if _ENABLE_REPORT
        biasChanged = true;
      #endif
      SetTextureMipBias(newBias);
      // change needed values for all textures
      ChangeMipBias(_lastFramePartialUsed,changeBias);
      ChangeMipBias(_lastFrameWholeUsed,changeBias);
      ChangeMipBias(_thisFramePartialUsed,changeBias);
      ChangeMipBias(_thisFrameWholeUsed,changeBias);
      ChangeMipBias(_previousUsed,changeBias);
    }
    
    int tgtLevel = tex->_levelLoaded+1;
    if (!tex->ReduceTextureLevel(tgtLevel))
    {
      // if we cannot reduce, we can evict the texture completely
      tex->ReleaseMemory();
    }
  }
  #if _ENABLE_REPORT
  if (biasChanged)
  {
    int thisUsed = CalculateMemory(_thisFrameWholeUsed);
    int lastUsed = CalculateMemory(_lastFrameWholeUsed);
    thisUsed += CalculatePartMemory(_thisFramePartialUsed);
    lastUsed += CalculatePartMemory(_lastFramePartialUsed);
    // we had to change bias - change it a little bit more
    LogF("Mip bias forced from %.3f to %.3f",origBias,GetTextureMipBias());
    LogF("  VRAM usage reduced %d->%d",thisUsed0+lastUsed0,thisUsed+lastUsed);
    if (thisUsed0+lastUsed0<_limitAlocatedTextures*3/4)
    {
      LogF("Strange reduction");
    }
  }
  #endif
  return ret;
}

bool TextBankD3DXB::LimitMemoryUsingReduction(int maxAllocAllowed)
{
  PROFILE_SCOPE(texLR);
  while(_totalAllocated>maxAllocAllowed)
  {
    bool reduced;
    // store is false - we need free memory during reduction
    reduced = ReduceSomeTexture(_lastFramePartialUsed,_lastFrameWholeUsed);
    if (reduced) continue;
    reduced = ReduceSomeTexture(_thisFramePartialUsed,_thisFrameWholeUsed);
    if (reduced) continue;
    return false;
  }
  return true;
}

int TextBankD3DXB::ReserveMemory( int size)
{
  PROFILE_SCOPE(texRM);
  // note: while reducing may free some memory, it needs some allocation to be done
  if (size>=INT_MAX)
  {
    // all memory will be released - reset and start again
    _reserveTextureMemory = 0;
  }
  // check size
  // never free this&last frame textures
  // try to keep some memory (1/64 of total space) free to avoid
  // out of memory conditions
  int maxAllocAllowed = _limitAlocatedTextures/16*63/4-AlignVRAM(size);
  int free = ReserveMemory(_previousUsed,maxAllocAllowed);
  return free;
}

// assume driver will align RAM to some reasonable number
// 256 is experimental value for GeForce
// use GetAvailableVidMem after allocating very small texture to check this
// consider: this could be done run-time

int TextBankD3DXB::ForcedReserveMemory( int size, bool enableSysFree )
{
  #ifdef _XBOX
  // first of all try balancing / releasing other stuff
  if (enableSysFree)
  {
    size_t freed = FreeOnDemandSystemMemory(size);
    if (freed!=0) return freed;
  }
  #endif
  const int minRelease=4*1024;
  if( size<minRelease ) size=minRelease; // force some minimal release
  int newLimit=_totalAllocated-AlignVRAM(size);

  LogF
  (
    "ForcedReserveMemory newLimit=%d _totalAllocated=%d",
    newLimit,_totalAllocated
  );

  int ret = ReserveMemory(_previousUsed,newLimit);
  if (ret<=0)
  {
    //LogF("Evicting last frame partially used texture");
    ret += ReserveMemory(_lastFramePartialUsed,newLimit);
    if (ret<=0) 
    {
      //LogF("Evicting last frame whole used texture");
      ret += ReserveMemory(_lastFrameWholeUsed,newLimit);

      if (ret<=0) 
      {
        LogF("Evicting this frame partially used texture");
        ret =+ ReserveMemory(_thisFramePartialUsed,newLimit);

        if (ret<=0) 
        {
          LogF("Evicting this frame whole used texture");
          ret += ReserveMemory(_thisFrameWholeUsed,newLimit);
        }
      }
    }
  }
  CheckTextureMemory();

  LogF
  (
    "  done %d _totalAllocated=%d",ret,_totalAllocated
  );

  #if REPORT_ALLOC>=10
  //int overhead=dwTotal-_limitAlocatedTextures;
  int freemem = FreeTextureMemory();

  LogF
  (
    "VID Forced %d, free %d, allocated %d, limit %d",
    size,freemem,_totalAllocated,_limitAlocatedTextures
  );
  #endif

  return ret;
}

#ifndef _XBOX
int TextBankD3DXB::ReserveSystem( D3DMipCacheRootXB &root, int limit )
{
  // release as many mip-map data as neccessary to get required memory
  D3DSurfaceDestroyerXB batchDestroy;
  D3DSurfaceDestroyerXB *batch = IsOutOfMemory() ? NULL : &batchDestroy;

  bool someReleased=false;
  int totalReleased = 0;
  #if REPORT_ALLOC>=10
    if (_systemAllocated>limit)
    {
      LogF("Allocated %d, limit %d",_systemAllocated,limit);
    }
  #endif
  while( _freeSysTextures.Size()>0 && _systemAllocated>limit )
  {
    // free more than one at once
    int need=_systemAllocated-limit;
    totalReleased += DeleteLastSystem(need,batch);
    someReleased=true;
  }
  // get last mip-map used
  HSysCacheD3DXB *last=root.Last();
  while( _systemAllocated>limit )
  {
    if( !last ) break;

    // release texture data
    TextureD3DXB *text = last->texture;

    // release texture data from texture heap
    Assert( text->_sysCache==last );
    
    someReleased=true;
    totalReleased += text->ReleaseSystem(false,batch);
    last=root.Last();
  }
  return totalReleased;
}

int TextBankD3DXB::ReserveSystem( int size )
{
  return ReserveSystem(_systemUsed,_limitSystemTextures-size);
}

int TextBankD3DXB::ForcedReserveSystem( int size )
{
  const int minRelease=4*1024;
  if( size<minRelease ) size=minRelease; // force some minimal release
  return ReserveSystem(_systemUsed,_systemAllocated-size);
}

void TextureD3DXB::SystemReleased()
{
  if( _sysCache )
  {
    _sysCache->Delete();
    delete _sysCache;
    _sysCache=NULL;
  }
  _systemLoaded = _nMipmaps;
}

void TextureD3DXB::ReuseSystem( SurfaceInfoD3DXB &surf )
{
  // reuse immediatelly
  Assert( _sysSurface.GetSurface() );
  if( _sysSurface.GetSurface() )
  {
    // first of all: move data to reserve bank

    surf=_sysSurface;
    // allocated memory ammount not changed
    #if REPORT_ALLOC>=20
      LogF
      (
        "SYS Directly reused %dx%dx%s (%dB)",
        surf._w,surf._h,FormatName(surf._format),surf.SizeUsed()
      );
    #endif
    _sysSurface.Free(false);
    if( _sysCache )
    {
      _sysCache->Delete();
      delete _sysCache;
      _sysCache=NULL;
    }
    _systemLoaded = _nMipmaps;
  }
}
#endif

void TextBankD3DXB::StartFrame()
{
  if (_engine->IsAbleToDrawCheckOnly())
  {
    InitDetailTextures();
    CheckTextureMemory();
  }
  _thisFrameAlloc=0; // VRAM allocations/deallocations (count)

#if _ENABLE_CHEATS
  if (GInput.GetCheat2ToDo(DIK_E))
  {
    ReportTexturesXB("textures.txt");
  }
#ifdef _XBOX
  if (GInput.GetCheatXToDo(CXTTexSample))
  {
    ReportTexturesXB("textures.txt");
  }
#endif
#endif
}

void TextBankD3DXB::FinishFrame()
{
  //LogF("******* FinishFrame");
  // perform dynamic mipbias adjustment now
  // basic criterion is - how much is used by _thisFrameWholeUsed
  int thisUsed = CalculateMemory(_thisFrameWholeUsed);
  int lastUsed = CalculateMemory(_lastFrameWholeUsed);
  thisUsed += CalculatePartMemory(_thisFramePartialUsed);
  lastUsed += CalculatePartMemory(_lastFramePartialUsed);
  // note: we should consited partially used textures as well
  if (thisUsed+lastUsed<_limitAlocatedTextures*1/2)
  {
    float bias = GetTextureMipBias();
    bias -= 0.004f;
    saturateMax(bias,GetTextureMipBiasMin());
    SetTextureMipBias(bias);
  }
  else if (thisUsed+lastUsed<_limitAlocatedTextures*3/4)
  {
    float bias = GetTextureMipBias();
    bias -= 0.001f;
    saturateMax(bias,GetTextureMipBiasMin());
    SetTextureMipBias(bias);
  }
  else if (thisUsed+lastUsed>=_limitAlocatedTextures*4/5)
  {
    float bias = GetTextureMipBias();
    bias += 0.04f;
    saturateMin(bias,5.0f);
    SetTextureMipBias(bias);
  }
  
  // texture that were only partialy used may be released
  // fully used textures must be kept
  // make history one frame older

  // make last and this frame textures history one frame older

  // verify previous textures are marked as unused
  // all this-frame textures must be in either 
  // _thisFramePartialUsed or _thisFrameWholeUsed

  for( HMipCacheD3DXB *tc=_thisFramePartialUsed.Last(); tc; tc=_thisFramePartialUsed.Prev(tc) )
  {
    TextureD3DXB *tex = tc->texture;

    //Assert (tex->_levelNeededThisFrame!=tex->_nMipmaps);

    tex->_levelNeededLastFrame = tex->_levelNeededThisFrame;
    tex->_levelNeededThisFrame = tex->_nMipmaps;
    tex->ResetMipmap();
  }
  for( HMipCacheD3DXB *tc=_thisFrameWholeUsed.Last(); tc; tc=_thisFrameWholeUsed.Prev(tc) )
  {
    TextureD3DXB *tex = tc->texture;

    //Assert (tex->_levelNeededThisFrame!=tex->_nMipmaps);

    tex->_levelNeededLastFrame = tex->_levelNeededThisFrame;
    tex->_levelNeededThisFrame = tex->_nMipmaps;
    tex->ResetMipmap();
  }
  // this will move to partial
  for( HMipCacheD3DXB *tc=_lastFramePartialUsed.Last(); tc; tc=_lastFramePartialUsed.Prev(tc) )
  {
    TextureD3DXB *tex = tc->texture;

    //Assert (tex->_levelNeededLastFrame!=tex->_nMipmaps);
    
    tex->_levelNeededLastFrame = tex->_levelNeededThisFrame;
    tex->_levelNeededThisFrame = tex->_nMipmaps;

    tex->ResetMipmap();
  }
  for( HMipCacheD3DXB *tc=_lastFrameWholeUsed.Last(); tc; tc=_lastFrameWholeUsed.Prev(tc) )
  {
    TextureD3DXB *tex = tc->texture;

    //Assert (tex->_levelNeededLastFrame!=tex->_nMipmaps);
    
    tex->_levelNeededLastFrame = tex->_levelNeededThisFrame;
    tex->_levelNeededThisFrame = tex->_nMipmaps;

    tex->ResetMipmap();
  }

  _previousUsed.Move(_lastFrameWholeUsed);
  Assert( _lastFrameWholeUsed.Last()==NULL );

  _lastFrameWholeUsed.Move(_thisFrameWholeUsed);
  Assert( _thisFrameWholeUsed.Last()==NULL );

  _previousUsed.Move(_lastFramePartialUsed);
  Assert( _lastFramePartialUsed.Last()==NULL );

  _lastFramePartialUsed.Move(_thisFramePartialUsed);
  Assert( _thisFramePartialUsed.Last()==NULL );
  // thisFrame lists are empty now
}

DEFINE_FAST_ALLOCATOR(HMipCacheD3DXB);

void TextureD3DXB::CacheUse( D3DMipCacheRootXB &list )
{
  // some textures are never released - we cannot mark them as cached
  if (_permanent)
  {
    return;
  }
  
  HMipCacheD3DXB *first;
  if( _cache ) _cache->Delete(),first=_cache;
  else first=new HMipCacheD3DXB;
  first->texture=this;
  list.Insert(first);
  _cache=first;
}

//DEFINE_FAST_ALLOCATOR(HSysCacheD3D);

#ifndef _XBOX
void TextureD3DXB::SysCacheUse( D3DSysCacheRootXB &list )
{
  HSysCacheD3DXB *first;
  if( _sysCache ) _sysCache->Delete(),first=_sysCache;
  else first=new HSysCacheD3DXB;
  first->texture=this;
  list.Insert(first);
  _sysCache=first;
}
#endif

#if _ENABLE_CHEATS
extern bool DisableTextures;
extern bool StopLoadingTextures;
#endif

// assume: max reasonable allocations per frame
const int MaxAllocationsPerFrame=32;

/*!
\patch 1.03 Date 7/12/2001 by Ondra
- Fixed: crash when too low heap memory
*/

MipInfo TextBankD3DXB::UseMipmap(
  Texture *absTexture, int level, int top
)
{
  if( !absTexture )
  {
    // with pixel shaders we need to override default black
    return MipInfo(NULL,0);
  }
  // level is the level we need
  // top is the level we would like to have
  // we are sure that texture is of type Texture
  // however this cast is potentially unsafe - take care
  TextureD3DXB *texture=static_cast<TextureD3DXB *>(absTexture);
  texture->LoadHeadersNV();

  Assert(texture->_inUse == 0);

  bool forceUse = level==top && level<INT_MAX || texture->_someDataNeeded;
  
  // if some level is explicity required, we may not apply bias
  // level is neccessary, top is wanted
  // as we are background loading, we can always load what is wanted
  level = top;
  
  const float defBias = GetTextureMipBias();
  
  //float levelExact = floatMin(level,texture->_mipmapNeeded);
  float levelExact = floatMin(level,texture->_mipmapWanted+defBias);
  
  saturateMin(level,toIntFloor(texture->_mipmapWanted+defBias));

  if (level<texture->_largestUsed) level=texture->_largestUsed;

  #if _ENABLE_CHEATS
  if (!forceUse && DisableTextures)
  {
    texture->ReleaseMemory(true);
  }
  #endif
  
  // never use mipmaps smaller than some limit

  if (level<texture->_nMipmaps)
  {
    // first level smaller or equal to _maxSmallTexturePixels is minimal used
    if (level>texture->_smallLevel) level = texture->_smallLevel;
  }

  if (forceUse)
  {
    if (level>texture->_nMipmaps-1) level = texture->_nMipmaps-1;
  }
  else
  {
    if (level>texture->_nMipmaps) level = texture->_nMipmaps;
  }
  if (level<0) level = 0;
  
  /*
  if (_thisFrameAlloc>MaxAllocationsPerFrame)
  {
    // we already copied a lot, try to use what we have
    if( texture->_levelLoaded<texture->_nMipmaps )
    {
      level = texture->_levelLoaded;
    }  
  }
  */

  if (texture->_levelNeededThisFrame>levelExact)
  {
    texture->_levelNeededThisFrame = levelExact;
    //LogF("texture %s: need %d",texture->Name(),level);
  }
    
  bool cacheMarked = false;
  if (texture->_levelLoaded>level)
  {
    // level is not present - you have to load something-> load top
    //LogF("loading %s: %d (%d), loaded %d",texture->Name(),top,level,texture->_levelLoaded);

    // if request is ready to be satisfied, load it
    #if _ENABLE_CHEATS
    bool loaded = ( DisableTextures || StopLoadingTextures || level>=texture->_nMipmaps ) ? false : texture->RequestLevels(level);
    #else
    bool loaded = level>=texture->_nMipmaps ? false : texture->RequestLevels(level);
    #endif
    // if there are no data yet, load (quick fill) something
    if (loaded || forceUse )
    {
      for(;;)
      {
        DoAssert(level<texture->_nMipmaps);
        // mipbias may be increased when there is not enough memory
        // if makes no sense to increase it so much the texture 
        
        float mipImprovement = texture->_levelLoaded-level;
        #if 0 // _PROFILE
          SectionTimeHandle st = StartSectionTime();
        #endif
        
        // check if source data are already present in the file cache
        int ret = texture->LoadLevels(level,mipImprovement);
        #if 0 // _PROFILE
          float secTime = GetSectionTime(st);
          if (secTime>=3e-3f && !forceUse)
          {
            LogF(
              "  %s: loading time %.2f, level %d",
              cc_cast(texture->GetName()),secTime*1000,level
            );
            void ReportCurrentFps();
            ReportCurrentFps();
          }
        #endif
        if( ret>=0 )
        {
          cacheMarked = true;
          break;
        }
        // if the next level is already loaded, we can break
        if( ++level>=texture->_levelLoaded ) break;
      }
    }
    else
    {
      level = texture->_nMipmaps;
    }
  }
  // if nobody marked the cache as used, we will mark it now
  if (!cacheMarked && texture->_cache)
  {

    // level already loaded
    Assert( texture->_cache );
    // check if it used fully or partially
    // if it has been used fully during last two frames, we consider is fully used
    if( toIntFloor(texture->LevelNeeded())<=texture->_levelLoaded )
    {
      texture->CacheUse(_thisFrameWholeUsed);
    }
    else
    {
      texture->CacheUse(_thisFramePartialUsed);
    }
    //Assert( mip->_level==level );
  }
    
  // if no level is loaded, average color will be used
  return MipInfo(texture,level); // given mip-map loaded
}

MipInfo TextBankD3DXB::UseMipmapLoaded(Texture *tex)
{
  if( !tex )
  {
    // with pixel shaders we need to override default black
    return MipInfo(NULL,0);
  }
  // level is the level we need
  // top is the level we would like to have
  // we are sure that texture is of type Texture
  // however this cast is potentially unsafe - take care
  TextureD3DXB *texture=static_cast<TextureD3DXB *>(tex);
  texture->LoadHeadersNV();
  return MipInfo(texture,texture->_levelLoaded);
}

/*
void TextBankD3DXB::ReleaseMipmap()
{
  //Assert( CheckConsistency() );
  //_loader->Unlock();
  //Assert( CheckConsistency() );
}
*/

struct SurfDesc
{
  PacFormat format;
  int w,h;
  int n;
};

static const SurfDesc PreloadDesc[]=
{
  // data based on real session statistics
  // TODO: make more samples on different scenes
  PacARGB1555,16,16,309,
  PacARGB4444,16,16,251,
  PacRGB565,16,16,228,
  PacARGB4444,8,16,122,
  PacRGB565,16,8, 93,
  PacRGB565,8,16, 78,
  PacARGB4444,16,8, 77,
  PacARGB4444,32,32, 73,
  PacRGB565,32,8, 58,
  PacARGB1555,8,16, 46,
  PacARGB4444,16,32, 42,
  PacARGB1555,16,8, 33,
  PacARGB4444,64,64, 26,
  PacRGB565,4,32, 21,
  PacRGB565,8,32, 16,
  PacRGB565,32,32, 14,
  PacRGB565,32,4, 13,
  PacARGB4444,8,32, 12,
  PacRGB565,128,128, 11,
  PacARGB1555,32,8,  8,
  PacRGB565,128,32,  8,
  PacRGB565,64,64,  7,
  PacARGB4444,32,8,  6,
  PacARGB1555,32,32,  6,
  PacARGB4444,32,4,  5,
  PacRGB565,256,64,  5,
  PacRGB565,32,16,  4,
  PacARGB4444,256,32,  4,
  PacARGB1555,128,128,  3,
  PacARGB1555,32,16,  3,
  PacRGB565,8,8,  2,
  PacARGB1555,64,64,  2,
  PacARGB4444,4,32,  2,
  PacARGB4444,128,128,  2,
  PacARGB4444,128,64,  2,
  PacRGB565,32,256,  2,
  PacRGB565,16,32,  2,
  PacRGB565,64,32,  2,
  PacRGB565,16,128,  2,
  PacRGB565,256,32,  2,
  PacARGB1555,8,32,  2,
  PacRGB565,64,128,  2,
  PacARGB1555,128,64,  1,
  PacRGB565,128,64,  1,
  PacARGB4444,32,64,  1,
  PacARGB4444,16,128,  1,
  PacARGB4444,64,128,  1,
  PacARGB1555,32,4,  1,
  PacARGB1555,4,32,  1,
  PacARGB4444,16,64,  1,
  PacRGB565,64,16,  1,
  PacARGB4444,64,16,  1,
  PacRGB565,32,128,  1,
  PacARGB4444,64,32,  1,
  PacRGB565,128,256,  5,
  PacARGB1555,256,128,  2,
  PacARGB4444,128,256,  1,
  PacRGB565,256,128,  9,
  PacARGB4444,256,128,  6,
  PacARGB4444,256,256,  5,
  PacRGB565,256,256, 21,
};

const int NPreloadDesc=sizeof(PreloadDesc)/sizeof(*PreloadDesc);

extern bool UseWindow;

static PacFormat SupportedFormat( PacFormat format )
{
  //if( format==PacRGB565 && !GEngineDD->Can565() ) return PacARGB1555;
  if( format==PacRGB565 ) return PacARGB1555;
  return format;
}

void TextBankD3DXB::MakePermanent(Texture *tex)
{
  TextureD3DXB *texNat = static_cast<TextureD3DXB *>(tex);
  _permanent.Add(texNat);
  DoAssert(texNat->_cache==NULL);
  texNat->_permanent = true;
  UseMipmap(tex,0,0);
}

void TextBankD3DXB::InitDetailTextures()
{
  if( !_white )
  {
    _white = static_cast<TextureD3DXB *>(Load("#(rgb,2,2,1)color(1,1,1,1)").GetRef());
    MakePermanent(_white);
  }
  if( !_defNormMap )
  {
    _defNormMap = static_cast<TextureD3DXB *>(Load("#(rgb,2,2,1)color(0.5,0.5,1,1)").GetRef());
    MakePermanent(_defNormMap);
  }
  if( !_defDetailMap )
  {
    _defDetailMap = static_cast<TextureD3DXB *>(Load("#(rgb,2,2,1)color(0.5,0.5,0.5,1)").GetRef());
    MakePermanent(_defDetailMap);
  }
  /*
  if (!_ditherMap)
  {
    _ditherMap=new TextureD3DXB;
    ITextureSource *source = CreateDitherTextureSource(16,128,256);
    _ditherMap->Init(source);
    _ditherMap->_permanent = true;
  }
  */

  UseMipmap(_white,0,0);
  UseMipmap(_defNormMap,0,0);
  UseMipmap(_defDetailMap,0,0);
  //UseMipmap(_ditherMap,0,0);
}

void TextBankD3DXB::PreCreateVRAM( int reserve, bool randomOrder )
{
}

void TextBankD3DXB::PreCreateSys()
{
}

#if _ENABLE_REPORT
void ReportGC(const char *text);
#endif

void TextBankD3DXB::AllocateTextureMemory()
{
  #ifdef _XBOX
  int moreVRamNeeded = _maxTextureMemory-_totalAllocated;
  if (moreVRamNeeded>0)
  {
    //LogF("More VRAM needed: %d",moreVRamNeeded);
    //#if _ENABLE_REPORT
    //ReportGC("Before alloc");
    //#endif
    // check how much texture memory is allocated
    FreeOnDemandGarbageCollect(moreVRamNeeded+1024*1024,256*1024);
    //#if _ENABLE_REPORT
    //ReportGC("After alloc");
    //#endif
  }
  #endif
}

void TextBankD3DXB::FlushTextures(bool deep)
{
  Compact();
  #ifndef _XBOX
  ReserveSystem(INT_MAX);
  #endif
  if (deep)
  {
    ReserveMemory(INT_MAX);
  }
}

void TextBankD3DXB::FlushBank(QFBank *bank)
{
  for (int i=0; i<_texture.Size(); i++)
  {
    TextureD3DXB *tex = _texture[i];
    if (!tex) continue;
    // check if texture is from given bank
    if (!bank->Contains(tex->GetName())) continue;
    _texture.Delete(i);
    i--;
  }
  // we need to flush the texture from the cache as well
  FlushBankFromCache(bank);
}

void TextBankD3DXB::PreCreateSurfaces()
{
  //PreCreateVRAM(512*1024,true);
}

static int CompareTextureFileOrder(const LLink<TextureD3DXB> *tl1, const LLink<TextureD3DXB> *tl2)
{
  TextureD3DXB *t1 = *tl1;
  TextureD3DXB *t2 = *tl2;
  return QFBankQueryFunctions::FileOrder(t1->GetName(),t2->GetName());
}

void DisplayTextureFileOrder(TextureD3DXB *t1)
{
  const char *n1 = t1->GetName();
  QFBank *b1 = QFBankQueryFunctions::AutoBank(t1->GetName());
  if (!b1) return;
  int o1 = b1->GetFileOrder(n1+strlen(b1->GetPrefix()));
  LogF("texture %s, order %d",n1,o1);
}

/*!
\patch 1.78 Date 7/23/2002 by Ondra
- Fixed: Random crash during program startup.
*/

void TextBankD3DXB::Preload()
{
  Compact();

  DWORD start = GlobalTickCount();
  // sort textures bank / by order in bank
  QSort(_texture.Data(),_texture.Size(),CompareTextureFileOrder);
  // start preloading all headers - in case default preloading did not work for any reason
  #if 1
  for (int i=0; i<_texture.Size(); i++)
  {
    TextureD3DXB *tex = _texture[i];
    if (!tex) continue;
    tex->PreloadHeaders();
  }
  GFileServer->SubmitRequests();
  #endif

  // wait until all headers are loaded, that is until all request are satisfied
  // load all headers
  for (int i=0; i<_texture.Size(); i++)
  {
    TextureD3DXB *tex = _texture[i];
    if (!tex) continue;
    tex->LoadHeadersNV();
    //DisplayTextureFileOrder(tex);
    ProgressRefresh();
  }
  DWORD end = GlobalTickCount();

  LogF("Preload %d textures - %d ms",_texture.Size(),end-start);
  
  #if 0
  ReportTexturesXB("* PreloadTextures [");
  // switch data - flush and preload

  // remove all currently loaded textures
  ReserveSystem(INT_MAX);

  int currentlyLeft = _limitAlocatedTextures - _totalAllocated;
  if (currentlyLeft<12*1024*1024 )
  {
    LogF("Preload: VRAM low - forced release");
    // release textures - we need space
    ReserveMemory(INT_MAX);
  }

  //PreCreateSys();

  CheckTextureMemory();
  int left = _limitAlocatedTextures;
  // each texture will have small surface loaded
  left -= _maxSmallTexturePixels*3*_texture.Size();
  // estimate how many the big textures can we afford
  left -= 256*256*8*3; // keep space for 8 big 16-b textures with mipmaps

  LogF("Free space %d KB",left/1024);
  saturateMin(left,8*1024*1024); // preload max. 8MB of large textures
  LogF("    loaded %d KB",left/1024);

  #define PROGRESS 1
  #if PROGRESS
  ProgressAdd(_texture.Size());

  ProgressRefresh();
  #endif

  LogF("Preloading %d textures",_texture.Size());
  // preload as many big textures as will fit into the video memory
  // estimate texture preloading

  Temp<int> levelsVram(_texture.Size());

  for( int i=0; i<_texture.Size(); i++ ) levelsVram[i]=0;
  for( int wantedSize=1; wantedSize<512*512*2; wantedSize<<=1 )
  {
    for( int i=0; i<_texture.Size(); i++ )
    {
      #if PROGRESS
      ProgressRefresh();
      #endif
      TextureD3DXB *texture=_texture[i];
      // estimate 
      for( int level=0; level<texture->_nMipmaps; level++ )
      {
        PacLevelMem *mip=&texture->_mipmaps[level];
        if (mip->TooLarge(texture->_maxSize)) continue;
        int pixels = mip->_w*mip->_h;
        if (pixels<_maxSmallTexturePixels) continue;
        int size = pixels*2;
        // check if DXT will be used
        // if (DXT) size/=4;
        if( size>wantedSize ) continue;
        // maybe it is already marked for loading
        if( (levelsVram[i]&(1<<level))==0 && left>=size )
        {
          // if it will fit, mark it for preloading
          left-=size;
          levelsVram[i]|=1<<level;
        }
      }
    }
  }
  
  CheckTextureMemory();
  /*
  _engine->ReportGRAM("before small");
  LogF(" VID left %d",left);
  for( int i=0; i<_texture.Size(); i++ )
  {
    TextureD3DXB *texture=_texture[i];
    texture->LoadSmall();
    #if PROGRESS
    ProgressAdvance(1);
    ProgressRefresh();
    #endif
  }
  CheckTextureMemory();
  */
  _engine->ReportGRAM("before large");
  for( int i=0; i<_texture.Size(); i++ )
  {
    TextureD3DXB *texture=_texture[i];
    #define DIAG_REUSE 0
    #if DIAG_REUSE
    LogF("%s",texture->Name());
    #endif
    // check which level should we load
    int maxLevel=1024;
    int levels = levelsVram[i];
    for (int level=0,mask=1; level<24; mask<<=1,level++ )
    {
      if (levels&mask) {maxLevel=level;break;}
    }
    // load maxLevel
    if (maxLevel<texture->NMipmaps())
    {
      // force loading, no limits
      _thisFrameCopied = 0;
      _thisFrameAlloc = 0;

      #if DIAG_REUSE
        if (texture->_levelLoaded>maxLevel)
        {
          LogF("  loading large %d",maxLevel);
        }
      #endif
      
      UseMipmap(texture,maxLevel,maxLevel);

      #if REPORT_ALLOC>=20
      LogF
      (
        "Preloaded %s:%d, Wanted %d",
        texture->Name(),
        texture->_levelLoaded,maxLevel
      );
      #endif
    }
    else
    {
      #if REPORT_ALLOC>=20
      LogF("Preloading %s - no level",texture->Name());
      #endif
    }

    #if PROGRESS
    ProgressAdvance(1);
    ProgressRefresh();
    #endif
  }

  ReportTexturesXB("flush.txt");

  CheckTextureMemory();

  LogF("After preloading - VID free %d B",FreeTextureMemory());

  //PreCreateVRAM();

  ReportTexturesXB("* PreloadTextures ]");
  // test - release all system memory surfaces
  ReserveSystem(INT_MAX);
  #endif
}

int TextBankD3DXB::FreeAGPMemory()
{
  Fail("Obsolete");
  return 0;
}

int MaxTexMemXB = 48*1024*1024;

int TextBankD3DXB::FreeTextureMemory()
{
  #ifdef _XBOX
    // check Xbox total memory
    int dwFree = _maxTextureMemory - _totalAllocated;
    if (dwFree<0) return 0;
    return dwFree;
  #else
    IDirect3DDevice8 *dDraw=_engine->GetDirect3DDevice();
    UINT dwFree = dDraw->GetAvailableTextureMem();
    int vramReported = dwFree+_totalAllocated;
    if (vramReported>MaxTexMemXB)
    {
      vramReported = MaxTexMemXB;
    }
    int ret = vramReported-_totalAllocated;
    if (ret<0) return 0;
    return ret;
  #endif
}

#if _ENABLE_CHEATS

static RString FormatByteSizeNoZero(size_t size)
{
  if (size) return FormatByteSize(size);
  return RString();
}
RString TextBankD3DXB::GetStat(int statId, RString &statVal, RString &statVal2)
{
  switch (statId)
  {
    case 0:
      statVal = FormatByteSizeNoZero(FreeTextureMemory());
      statVal2 = RString();
      return "Free VRAM";
      break;
    case 1:
      statVal = FormatByteSizeNoZero(_totalAllocated);
      statVal2 = RString();
      return "Alloc VRAM";
      break;
    case 2:
      statVal = FormatByteSizeNoZero(CalculateMemory(_thisFrameWholeUsed));
      statVal2 = FormatByteSizeNoZero(CalculatePartMemory(_thisFrameWholeUsed));
      return "This whole";
    case 3:
      statVal = FormatByteSizeNoZero(CalculateMemory(_lastFrameWholeUsed));
      statVal2 = FormatByteSizeNoZero(CalculatePartMemory(_lastFrameWholeUsed));
      return "Last whole";
    case 4:
      statVal = FormatByteSizeNoZero(CalculateMemory(_thisFramePartialUsed));
      statVal2 = FormatByteSizeNoZero(CalculatePartMemory(_thisFramePartialUsed));
      return "This part";
    case 5:
      statVal = FormatByteSizeNoZero(CalculateMemory(_lastFramePartialUsed));
      statVal2 = FormatByteSizeNoZero(CalculatePartMemory(_lastFramePartialUsed));
      return "Last part";
    case 6:
      statVal = FormatByteSizeNoZero(CalculateMemory(_previousUsed));
      statVal2 = RString();
      return "Prev";
    case 7:
    {
      statVal = FormatByteSizeNoZero(_freeAllocated);
      statVal2 = RString();
      return "Free";
    }
    case 8:
      statVal = FormatByteSizeNoZero(_totalSlack);
      statVal2 = RString();
      return "Slack";
    case 9:
      statVal = RString();
      statVal2 = RString();
      return " ";
    case 10:
      statVal = Format("%.3f",GetTextureMipBias());
      statVal2 = RString();
      return "Mip bias";
    case 11:
    {
      int totalUsage = CalculateMemory(_thisFrameWholeUsed);
      totalUsage += CalculateMemory(_lastFrameWholeUsed);
      totalUsage += CalculatePartMemory(_thisFramePartialUsed);
      totalUsage += CalculatePartMemory(_lastFramePartialUsed);
      statVal = FormatByteSizeNoZero(totalUsage);
      statVal2 = RString();
      return "Total load";
    }
    case 12:
    {
      int totalUsage = CalculateMemoryInUse(_thisFrameWholeUsed);
      totalUsage += CalculateMemoryInUse(_lastFrameWholeUsed);
      totalUsage += CalculateMemoryInUse(_thisFramePartialUsed);
      totalUsage += CalculateMemoryInUse(_lastFramePartialUsed);

      int totalUsageGPU = CalculateMemoryInUseGPU(_thisFrameWholeUsed);
      totalUsageGPU += CalculateMemoryInUseGPU(_lastFrameWholeUsed);
      totalUsageGPU += CalculateMemoryInUseGPU(_thisFramePartialUsed);
      totalUsageGPU += CalculateMemoryInUseGPU(_lastFramePartialUsed);

      statVal = FormatByteSizeNoZero(totalUsage);
      statVal2 = FormatByteSizeNoZero(totalUsageGPU);
      return "GPU used";
    }
  }
  statVal = RString();
  statVal2 = RString();
  return RString();
}
#endif

void TextBankD3DXB::CheckTextureMemory()
{
  #ifndef _XBOX
    _freeTextureMemory = FreeTextureMemory();
    _limitAlocatedTextures = _totalAllocated + _freeTextureMemory;
    _limitAlocatedTextures -= _reserveTextureMemory;
    if (_limitAlocatedTextures>_maxTextureMemory) _limitAlocatedTextures = _maxTextureMemory;
  #else
    _maxTextureMemory = Glob.config.heapSize*1024*1024;
    _limitAlocatedTextures = _maxTextureMemory;
    _freeTextureMemory = FreeTextureMemory();
  #endif
}

void TextBankD3DXB::ReloadAll()
{
  Log("Reload all textures.");
  // make sure all texture state is tested
}

#endif //!_DISABLE_GUI

