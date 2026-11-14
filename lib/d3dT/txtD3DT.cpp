#include "../wpch.hpp"

//#define SurfaceInfoD3DT @@

#if !_DISABLE_GUI && _ENABLE_DX10

#include <El/common/perfLog.hpp>
#include <El/ParamFile/paramFile.hpp>

//#include "global.hpp"
//#include "progress.hpp"
#include "txtD3DT.hpp"
#include "engDDT.hpp"
#include "../diagModes.hpp"

#include <Es/Threads/multiSync.hpp>
#include <El/FileServer/fileServer.hpp>
#include <El/QStream/qbStream.hpp>
#include <El/common/randomGen.hpp>
#include "../keyInput.hpp"
#include "../dikCodes.h"
#include "../progress.hpp"
#include "../timeManager.hpp"
#include <Es/Algorithms/qsort.hpp>

// {5926683A-2CF2-43df-A8A1-688108C9F922}
static const GUID PrivateDataGUID = 
{ 0x5926683a, 0x2cf2, 0x43df, { 0xa8, 0xa1, 0x68, 0x81, 0x8, 0xc9, 0xf9, 0x22 } };

#define MIN_MIP_SIZE 4 // guarantee QWORD alignment (4 16-bit pixels are enough)

#define REPORT_ALLOC 0 // verbosity level 0..30

#define USE_MANAGE 0 // TODO: small/large texture bank, try manager

static PacFormat BasicFormat( const char *name )
{
  const char *ext=strrchr(name,'.');
  if( ext && !strcmpi(ext,".paa") ) return PacARGB4444;
  else return PacARGB1555;
}

/*!
Similar to LoadLevelsSys
*/
bool TextureD3DT::RequestLevelsSys(int levelMin, int priority)
{
  // if there is no time left for loading, we pretend data are not ready
  if (priority<=TexPriorityNormal && GTimeManager.TimeLeft(TCLoadTexture)<=0) return false;

  // FIX: Avoid Crash
  if (!_src) return false;

  PROFILE_SCOPE_EX(texRq,tex);
  // we do not care about swizzling now - we only need to be sure data are here
  FileRequestPriority prior;
  // map texture priority to file request priority
  if (priority>=TexPriorityCritical) prior = FileRequestPriority(0);
  //else if (priority>=TexPriorityUIHigh) prior = FileRequestPriority(2);
  else if (priority>=TexPriorityUILow) prior = FileRequestPriority(5);
  else if (priority>=TexPriorityNormal) prior = FileRequestPriority(10);
  else if (priority>=TexPriorityLow) prior = FileRequestPriority(100);
  else prior = FileRequestPriority(500);
  return _src->RequestMipmapData(_mipmaps,levelMin,_nMipmaps,prior);

}

/*!
\patch 1.12 Date 8/07/2001 by Ondra
- Fixed: Incorrect texture source (like JPG file not loaded) crashes.
\patch 1.48 Date 3/12/2002 by Ondra
- Fixed: Incorrect texture source (like JPG file not loaded) caused shutdown.
Warning message is shown instead.
*/

int TextureD3DT::LoadLevelsSys(SurfaceInfoD3DT &systemSurf, int levelMin, float mipImprovement)
{
  TextBankD3DT *bank=static_cast<TextBankD3DT *>(GEngine->TextBank());

  ScopeInUse9 use(_inUse);

  int i;

  if (!_src)
  {
    RptF("No texture source for %s",Name());
    return -1;
  }

  PROFILE_SCOPE_EX(texLS,tex);

  // create sysmem mip-map chain

  TEXTUREDESC9 sysd;
  memset(&sysd,0,sizeof(sysd)); 
  
  Assert(levelMin < MAX_MIPMAPS);
  PacFormat format=_mipmaps[levelMin].DstFormat();
  //GEngineDD->InitVRAMPixelFormat(sysd.ddpfPixelFormat,format,false);
  GEngineDD->InitVRAMPixelFormat(sysd.format,format,true);

  //sysd.pool=D3DPOOL_SYSTEMMEM;
  sysd.nMipmaps = _nMipmaps-levelMin;

  Assert(levelMin < MAX_MIPMAPS);
  PacLevelMem &topMip = _mipmaps[levelMin];

  // not present -> create it and load it
  sysd.w = topMip._w;
  sysd.h = topMip._h;

  bank->UseSystem(systemSurf,sysd,format,mipImprovement);
  
  if (!systemSurf.GetSurface()._texture)
  {
    // not enough memory - loading failed
    return -1;
  }

  // load all mipmaps - coarse mipmaps are very small anyway
  // and it would help little not to load them
  for(i = levelMin; i < _nMipmaps; i++)
  { // sysmem load
    Assert(i < MAX_MIPMAPS);
    PacLevelMem &mip = _mipmaps[i];

    int aLevel = i-levelMin;
    Assert( aLevel>=0 );

    // lock surface - we will load it
    HRESULT err;
    D3D10_MAPPED_TEXTURE2D rect;

    {
      PROFILE_DX_SCOPE(3dLkR);
      err = systemSurf.GetSurface()._texture->Map(aLevel, D3D10_MAP_WRITE, 0, &rect);
    }

    // Assign the true value to pitch
    if (PacFormatIsDXT(mip._dFormat))
    {
      // Pitch is 4 lines together
      mip._pitch = rect.RowPitch >> 2;
    }
    else
    {
      mip._pitch = rect.RowPitch;
    }

    bool ret = true;

    if( err!=D3D_OK ) GEngineDD->DDError9("Cannot lock mipmap",err);
    // assume normal pitch
    // load level
    ret = _src->GetMipmapData(rect.pData, mip, i);
    ADD_COUNTER(ldTxP,mip._w*mip._h);
    
    if (!ret)
    {
      // in case of error fill mipmap with average color
      PackedColor color = _src->GetAverageColor();
      mip.FillMipmap(rect.pData, color);
    }

    // Do the channel permutation according to format
    if (format == PacARGB8888)
    {
      // Do the ARGB->RGBA conversion
      DWORD *data = (DWORD *)rect.pData;
      for (int y=0; y<mip._h; y++)
      {
        DWORD *line = data;
        for (int x=0; x<mip._w; x++)
        {
          DWORD argb = *line;
          DWORD a = (argb>>24)&0xff;
          DWORD r = (argb>>16)&0xff;
          DWORD g = (argb>>8)&0xff;
          DWORD b = argb&0xff;
          *line++ = (a << 24) | (b << 16) | (g << 8) | (r); // Consider the little endian here
        }
        Assert(mip._pitch > 0);
        data = (DWORD *)((char *)data+mip._pitch);
      }
    }

    systemSurf.GetSurface()._texture->Unmap(aLevel);
    if( !ret ) WarningMessage("Cannot load mipmap %s",Name());

    //actMipmap->GetAttachedSurface(&caps,nextMipmap.Init());
    //actMipmap=nextMipmap;
  } // if( load mipmap level)

  return 0;
}

/**
We use 200 for grass (ground clutter) and 100 for trees
*/
void TextureD3DT::SetTexturePerformance(int performance)
{
  _performance = performance;
}

void TextureD3DT::UpdateMaxTexDetail()
{
  if (_nMipmaps>0)
  {
    Assert(_largestUsed<_nMipmaps);
    _maxTexDetail = _mipmaps[_largestUsed]._w*_mipmaps[_largestUsed]._h;
  }
  else
  {
    _maxTexDetail = 1;
  }
}
void TextureD3DT::UpdateLoadedTexDetail(int levelLoaded)
{
  // Update _levelLoaded
  _levelLoaded = levelLoaded;
  DoAssert(_levelLoaded<=_smallLevel || _levelLoaded>=_nMipmaps)

  if (_levelLoaded<=_largestUsed)
  {
    // best mipmap sometimes required special handling
    TextureType type = _src->GetTextureType();
    switch (type)
    {
      case TT_Diffuse: case TT_Diffuse_Linear:
        _loadedTexDetail = _maxTexDetail;
        break;
      default:
        // for textures other than primary texture
        // we assume infinite precision for the best mipmap
        // this is to make sure secondary stage do not drive overall assume precision
        // down when fully loaded (Cf. Shape::PreloadTexturesLeft)
        _loadedTexDetail = FLT_MAX;
        break;
    }
    return;
  }
  // we know _levelLoaded is integer - acquiring TexDetail from it is very easy
  // it is the mipmap resolution
  const int missingLods = _levelLoaded-_largestUsed;
  Assert(missingLods<=MAX_MIPMAPS);
  const int nom = 1<<(2*MAX_MIPMAPS-2*missingLods);
  const int denom = 1<<(2*MAX_MIPMAPS);
  const float factor = nom*(1.0f/denom);
  _loadedTexDetail = _maxTexDetail*factor;
  //_loadedTexDetail = _levelLoaded>_largestUsed ? TexDetailFromMipmapLevel(_levelLoaded) : FLT_MAX;
}


void TextureD3DT::InitDDSD( TEXTUREDESC9 &ddsd, int levelMin, bool enableDXT )
{
  memset(&ddsd,0,sizeof(ddsd)); 

  Assert(levelMin < MAX_MIPMAPS);
  PacFormat format=_mipmaps[levelMin].DstFormat();

  GEngineDD->InitVRAMPixelFormat(ddsd.format,format,enableDXT);

  ddsd.w = _mipmaps[levelMin]._w; 
  ddsd.h = _mipmaps[levelMin]._h; 
  ddsd.nMipmaps = _nMipmaps-levelMin;
}

bool TextureD3DT::CopyToVRAM(SurfaceInfoD3DT &surface, int sysLevel, const SurfaceInfoD3DT &sys)
{
  PROFILE_SCOPE_EX(texUT,tex);
  // select best system surface
  DoAssert(sys.GetSurface()._texture);
  DoAssert((sys._w>>sysLevel)==surface._w);
  DoAssert((sys._h>>sysLevel)==surface._h);
  DoAssert(sys._format==surface._format);
  DoAssert(sys._nMipmaps - sysLevel == surface._nMipmaps);

  // Copy all mipmaps (subresources)
  ID3D10Device *device=GEngineDD->GetDirect3DDevice();
  //device->CopyResource(surface.GetSurface(), sys.GetSurface());
  for (int i = 0; i < surface._nMipmaps; i++)
  {
    device->CopySubresourceRegion(surface.GetSurface()._texture, i, 0, 0, 0, sys.GetSurface()._texture, i + sysLevel, NULL);
  }

  return true;
}

bool TextureD3DT::ReduceTexture(SurfaceInfoD3DT &surf, float mipImprovement)
{
  int needed = toIntFloor(LevelNeeded());
  // if texture is not needed, assume small version is needed
  if (needed>=_nMipmaps) needed = _nMipmaps-1;
  return ReduceTextureLevel(surf, needed, mipImprovement);
}

bool TextureD3DT::ReduceTextureLevel(int tgtLevel)
{
  SurfaceInfoD3DT surf;
  // do not throttle - we need the small texture so that we can release a larger one
  // we will therefore grow over the boundary for only a very short time
  if (ReduceTextureLevel(surf, tgtLevel, 0))
  {
    ReleaseMemory(surf);
    return true;
  }
  return false;
}

bool TextureD3DT::ReduceTextureLevel(SurfaceInfoD3DT &surf, int needed, float mipImprovement)
{
  if (needed > _smallLevel) needed = _smallLevel;
  if (_levelLoaded >= needed) return false;
  if (needed >= _nMipmaps) return false;

  ScopeInUse9 use(_inUse);

  // Return value will contain the actual level
  Assert(_surfaces[_levelLoaded].GetSurface()._texture);
  surf = _surfaces[_levelLoaded];

  // Zero the returned texture
  _surfaces[_levelLoaded].Free(false); // False, because the surface is shared with surf variable

  // Free the rest of disposed textures
  for (int i = _levelLoaded + 1; i < needed; i++)
  {
    Assert(_surfaces[i].GetSurface()._texture);
    ReleaseMemory(_surfaces[i]);
  }

  MemoryReleased(_levelLoaded,needed);

  // Update the level loaded variable and quit
  UpdateLoadedTexDetail(needed);

  return true;
}

void ReportGRAMX( const char *name)
{
  if (GEngineDD) GEngineDD->ReportGRAM(name);
}

void ReportTexturesX()
{
  EngineDDT *engine = dynamic_cast<EngineDDT *>(GEngine);
  if (!engine) return;
  engine->TextBankDD()->ReportTextures(NULL);
}

int TextureD3DT::TotalSize( int levelMin ) const
{
  int totalSize=0;
  for( int i=levelMin; i<_nMipmaps; i++ )
  {
    Assert(i < MAX_MIPMAPS);
    const PacLevelMem &mip = _mipmaps[i];
    int size = PacLevelMem::MipmapSize(mip.DstFormat(),mip._w,mip._h);
    //int size=_mipmaps[i]._pitch*_mipmaps[i]._h;
    totalSize+=size;
  }
  return totalSize;
}

/*!
Similar to LoadLevels
*/

bool TextureD3DT::RequestLevels(int levelMin, int priority)
{
  // if no levels are needed, we can load nothing and still return success
  if (levelMin<0 || _levelLoaded<=levelMin) return true;

  return RequestLevelsSys(levelMin,priority);
}


#if TRACK_TEXTURES
  static RString CreatingSurface;
  static char InterestedIn[256]="ca\\plants\\data\\hrusen2";

  static bool InterestedInResource(IDirect3DResource9 *res, const char *log)
  {
    char buf[1024];
    DWORD size = sizeof(buf)-1;
    buf[sizeof(buf)-1] = 0;
    HRESULT hr = res->GetPrivateData(PrivateDataGUID,buf,&size);
    
    if (hr==D3D_OK && strstr(buf, InterestedIn))
    {
      LogF("@@@ %s %s",log,buf);
      return true;
    }
    return false;
  }

#endif

TextureD3DT *TextureD3DT::GetDefaultTexture() const
{
  #if 0 // TRACK_TEXTURES
    if (strstr(GetName(),InterestedIn))
    {
      LogF("@@@ Default used for %s",cc_cast(GetName()));
    }
  #endif
  TextureD3DT *result;
  switch (Type())
  {
  case TT_Specular:
    result = GEngineDD->TextBankDD()->GetDefSpecularTexture();
    break;
  case TT_DTSpecular:
    result = GEngineDD->TextBankDD()->GetDefDTSpecularTexture();
    break;
  case TT_Macro:
    result = GEngineDD->TextBankDD()->GetDefMacroTexture();
    break;
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

#if 0 //_ENABLE_REPORT
  #define INTERESTED_IN_MIPMAPS 1
#endif

#if INTERESTED_IN_MIPMAPS
  static char InterestedInMipmaps[256]="trnka_";
#endif


int TextureD3DT::LoadLevels(int levelMin, float mipImprovement)
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
  TextBankD3DT *bank=static_cast<TextBankD3DT *>(GEngineDD->TextBank());
  if( _levelLoaded>levelMin)
  {
    PROFILE_SCOPE_EX(texLd,*);

    TimeManagerScope scope(TCLoadTexture);

    // no need to release old data - we will be happy to reuse them
    //ReleaseMemory(true); // release all old mipmap data
    //levelCur=_nMipmaps; // all levels released

#if REPORT_ALLOC>=20
    Assert(levelMin < MAX_MIPMAPS);
    LogF
    (
      "Load %s (%d:%d x %d)",
      (const char *)Name(),
      levelMin,_mipmaps[levelMin]._w,_mipmaps[levelMin]._h
    );
#endif

    // Lock the texture
    ScopeInUse9 use(_inUse);

    // Load texture in the system memory
    SurfaceInfoD3DT systemSurf;
    int ret = LoadLevelsSys(systemSurf,levelMin, mipImprovement);

    // Create whole texture chain and load the data
    if (ret >= 0)
    {
      PROFILE_SCOPE_EX(texCV,tex);

      Assert(CheckIntegrity());
      
      Assert(levelMin < MAX_MIPMAPS);
      //Assert(_levelLoaded < _nMipmaps);
      int to = _levelLoaded;
      if (to>_smallLevel+1) to = _smallLevel+1;
      for (int i = levelMin; i < to; i++)
      {

        // Get the texture description
        TEXTUREDESC9 ddsd;
        InitDDSD(ddsd, i, true);

        // Create or reuse VRAM surface
        PacFormat format = _mipmaps[i].DstFormat();
        int totalSize=TotalSize(i);
        #if TRACK_TEXTURES
          CreatingSurface = GetName();
        #endif
        int created = bank->CreateVRAMSurfaceSmart(_surfaces[i],ddsd,format,totalSize,mipImprovement);
        #if TRACK_TEXTURES
          CreatingSurface = RString();
        #endif
        if ( created< 0)
        {
          RptF("Error: Failed to create surface texture");
          return -1;
        }

        // Copy the texture to VRAM
        if (!CopyToVRAM(_surfaces[i], i - levelMin, systemSurf))
        {
          LogF("Error: CopyToVRAM failed");
          ret = -1;
        }
      }

      //LogF("texture %s: whole used",Name());
      UpdateLoadedTexDetail(levelMin);
      
      //Assert(CheckIntegrity());
      
      ReleaseSystem(systemSurf,true);
    }
  }

  if (ret < 0)
  {
    ReleaseMemory(true);
  }

  return ret;
}

bool TextBankD3DT::Reuse(
  SurfaceInfoD3DT &surf, const TEXTUREDESC9 &ddsd, PacFormat format,
  bool enableReduction, float mipImprovement
)
{
  PROFILE_SCOPE_EX(texRu,tex);
  // first try discarding previously used
  int w=ddsd.w;
  int h=ddsd.h;
  int nMipmaps=ddsd.nMipmaps;
  if (w*h<_minTextureSize*2)
  {
    // no need to try reducing - no reduction is possible for this size
    enableReduction = false;
  }
  HMipCacheD3DT *last=_used.First();
  if (!last) return false;
  // min. age dependent on max. age - do not reuse if much older textures exist
  int maxFrameId = FreeOnDemandFrameID()-(FreeOnDemandFrameID()-last->GetFrameNum())/2;
  for (; last; last=_used.Next(last))
  {
    // if the texture is recent, all after it are more recent
    if (last->GetFrameNum()>maxFrameId) break;
    // release mip-map data
    TextureD3DT *texture=last->texture;
    //cannot release this one
    if( texture->_inUse ) continue;

    const SurfaceInfoD3DT &surface=texture->_surfaces[texture->_levelLoaded];
    if( nMipmaps!=surface._nMipmaps ) continue;
    // match size
    if( surface._w!=w ) continue;
    if( surface._h!=h ) continue;
    // match pixel format
    // if equal - this texture is perfect match
    if( surface._format!=format ) continue;
    
    #if 0 // _ENABLE_REPORT
    if (last->GetFrameNum()>FreeOnDemandFrameID()-500)
    {
      LogF(
        "Reused %s:%d (age %d)",cc_cast(texture->GetName()),last->level,
        FreeOnDemandFrameID()-last->GetFrameNum()
      );
    }
    #endif
    // surface moved to reusable
    if (enableReduction && texture->ReduceTexture(surf, mipImprovement))
    {
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

int TextBankD3DT::FindSurface
(
  int w, int h, int nMipmaps, PacFormat format,
  const AutoArray<SurfaceInfoD3DT> &array
) const
{
  PROFILE_SCOPE_EX(texFn,tex);
  int i;
  for( i=0; i<array.Size(); i++ )
  {
    const SurfaceInfoD3DT &surface=array[i];
    // match mipmap structure
    if( nMipmaps!=surface._nMipmaps ) continue;
    //const PacLevelMem &pMip=pattern->_mipmaps[0];
    // match size
    if( surface._w!=w ) continue;
    if( surface._h!=h ) continue;
    // match pixel format
    if( surface._format==format )
    {
      /*
      // this texture is perfect match
      if( surface._texture==texture )
      {
        LogF("Texture %dx%d (%s) reused",w,h,(const char *)texture->Name());
      */
      return i;
    }
  }
  return -1;
}

int TextBankD3DT::DeleteLastReleased( int need, D3DSurfaceDestroyerT *batch )
{
  // LRU is last
  // delete first 
  SurfaceInfoD3DT &free=_freeTextures[0];
  int size=free.SizeUsed();

  _totalSlack -= free.Slack();
  _freeAllocated -= size;
  TextureFreed(size,free.Slack());

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
  _freeTextures.Delete(0);

  return size;
}

int TextBankD3DT::DeleteLastSystem( int need, D3DSurfaceDestroyerT *batch )
{
  PROFILE_SCOPE_EX(texDS,tex);
  // LRU is last
  // delete first 
  SurfaceInfoD3DT &free=_freeSysTextures[0];
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

void TextBankD3DT::AddReleased( SurfaceInfoD3DT &surf )
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

void TextBankD3DT::UseReleased
(
  SurfaceInfoD3DT &surf,
  const TEXTUREDESC9 &ddsd, PacFormat format
)
{
  Assert(surf.GetSurface()._texture.IsNull());
  int reuse=FindReleased(ddsd.w,ddsd.h,ddsd.nMipmaps,format);
  if( reuse<0 ) return;
  SurfaceInfoD3DT &reused=_freeTextures[reuse];
  surf=reused;
  #if REPORT_ALLOC>=20
    LogF
    (
      "Released %dx%dx%s (%dB) reused.",
      reused._w,reused._h,FormatName(reused._format),reused.SizeUsed()
    );
  #endif
  Assert(ddsd.nMipmaps==surf._nMipmaps);
  _freeAllocated -= reused.SizeUsed();
  reused.Free(false);
  _freeTextures.Delete(reuse);
}

int TextBankD3DT::FindSystem
(
  int w, int h, int nMipmaps, PacFormat format
) const
{
  PROFILE_SCOPE_EX(texFS,tex);
  return FindSurface(w,h,nMipmaps,format,_freeSysTextures);
}

/*!
\patch 5142 Date 3/20/2007 by Ondra
- Fixed: On "Cannot create system memory surface: 8007000e" error, some memory is flushed to allow game continuing.
*/
void TextBankD3DT::UseSystem
(
  SurfaceInfoD3DT &surf,
  const TEXTUREDESC9 &ddsd, PacFormat format, float mipImprovement
)
{
  PROFILE_SCOPE_EX(txUsS,tex);
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
  {
    /**/

    TEXTUREDESC9 ssss=ddsd;
    ID3D10Device *dDraw=_engine->GetDirect3DDevice();
    int size = surf.CalculateSize(dDraw,ssss,format);
    int sizeNeeded = _engine->AlignVRAMTex(size);
    ReserveSystem(sizeNeeded);
    // create a new surface

    Retry:
    //ADD_COUNTER(syCre,1);
    //DoAssert(_totalAllocated+sizeNeeded<=_limitAlocatedTextures);
    HRESULT err=surf.CreateSurface(_engine,dDraw,ssss,format,true);
    if( err!=D3D_OK )
    {
      if (ForcedReserveSystem(sizeNeeded)) goto Retry;
      RptF("Forced memory flush (%d system memory surface creation failed)",sizeNeeded);
      void ReportMemoryStatus(int level);
      //ReportMemoryStatus(true);
      if (FreeOnDemandSystemMemoryLowLevel(sizeNeeded)) goto Retry;
      if (ForcedReserveMemory(sizeNeeded)) goto Retry;
      ReportMemoryStatus(2);
      ErrorMessage(
        "Cannot create system memory surface %s,%dx%d (size %d B)\nError code %x",
        cc_cast(FormatName(format)),ssss.w,ssss.h,sizeNeeded,err
      );
    }
    else
    {
      _systemAllocated += surf.SizeUsed();
    }
  }
}

void TextBankD3DT::AddSystem( SurfaceInfoD3DT &surf )
{
  // TODO: XBOX UMA texture reusing
  _freeSysTextures.Add(surf);
  surf.Free(false);
  while( _freeSysTextures.Size()>0 && _systemAllocated>_limitSystemTextures )
  {
    DeleteLastSystem(0);
  }
}

void TextureD3DT::SetNMipmaps( int n )
{
  int discard = n;
  if (discard<_levelLoaded) discard = _levelLoaded;
  if (discard<_nMipmaps)
  {
    ReleaseLevelMemory(discard,_nMipmaps);
  }
  _nMipmaps=n;
  if (_levelLoaded>_nMipmaps)
  {
    UpdateLoadedTexDetail(_nMipmaps);
  }
  if (_smallLevel>_nMipmaps-1)
  {
    _smallLevel = _nMipmaps-1;
  }
  Assert(_nMipmaps>=MAX_MIPMAPS || _surfaces[_nMipmaps].GetSurface()._texture.IsNull());
  Assert((n-1) < MAX_MIPMAPS);
}

/*!
\patch 1.03 Date 7/12/2001 by Ondra
- Improved: more robust handling of max. texture size
*/
void TextureD3DT::ASetNMipmaps( int n )
{
  LoadHeadersNV();
  // never attempt to set more mipmaps then you can
  // assume max. size was already set
  saturateMin(n,_nMipmaps-_largestUsed);
  SetNMipmaps(_largestUsed+n);
}

void TextureD3DT::MemoryReleased(int from, int to)
{
  Assert(from>=_levelLoaded);
  Assert(to<=_nMipmaps);

  // levels smaller than _smallLevel can never be released individually
  Assert(from<=_smallLevel || from>=_nMipmaps);
  // mark as released
  // delete from statistics
  TextBankD3DT *bank=static_cast<TextBankD3DT *>(GEngine->TextBank());
  for (int level=from; level<to; level++)
  {
    if( _surfCache[level] )
    {
      Assert(level<=_smallLevel);
      #if TRACK_TEXTURES
        if (strstr(GetName(),InterestedIn))
        {
          LogF(
            "@@@ Surface released %s:%d, age %d",cc_cast(GetName()),level,
            FreeOnDemandFrameID()-_surfCache[level]->GetFrameNum()
          );
        }
        if (GetBigSurface())
        {
          InterestedInResource(GetBigSurface(),"MemoryReleased");
        }
      #endif
      bank->_used.Delete(_surfCache[level]);
      _surfCache[level]=NULL;
    }
  }

  // when releasing affects level loaded, set it
  if (from<=_levelLoaded && to>_levelLoaded)
  {
    UpdateLoadedTexDetail((to <= _smallLevel) ? to : _nMipmaps);
  }
}

void TextBankD3DT::TextureAllocated(int size, int overhead)
{
  _totalAllocated += size;
  _engine->AddLocalVRAMAllocated(size+overhead);
  _freeTextureMemory -= size+overhead;
}

int TextureD3DT::ReleaseMemory( SurfaceInfoD3DT &surf, bool store, D3DSurfaceDestroyerT *batch )
{
  int ret = 0;
  TextBankD3DT *bank=static_cast<TextBankD3DT *>(GEngine->TextBank());
  if (surf.GetSurface()._texture)
  {
    // we need to wait until the surface can be released
    GEngineDD->D3DTextureDestroyed(surf.GetSurface()._resview);
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
      bank->TextureFreed(size,surf.Slack());

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

bool TextureD3DT::CheckIntegrity() const
{
  bool ret = true;
  for (int i=0; i<_levelLoaded; i++)
  {
    if (_surfaces[i].GetSurface()._texture.NotNull())
    {
      LogF("%s:%d surface present, not tracked by _levelLoaded",cc_cast(GetName()),i);
      ret = false;
    }
  }
  for (int i=_levelLoaded; i<_smallLevel; i++)
  {
    if (_surfaces[i].GetSurface()._texture.IsNull())
    {
      LogF("%s:%d surface not present, tracked by _levelLoaded",cc_cast(GetName()),i);
      ret = false;
    }
  }
  if (!_permanent)
  {
    for (int i=0; i<_nMipmaps; i++)
    {
      if (_surfaces[i].GetSurface()._texture.NotNull())
      {
        if(i<=_smallLevel && !_surfCache[i])
        {
          LogF("%s:%d missing cache entry",cc_cast(GetName()),i);
          ret = false;
        }
      }
    }
  }
  return ret;
}

int TextureD3DT::ReleaseLevelMemory( int from, int to, bool store, D3DSurfaceDestroyerT *batch)
{
  Assert(CheckIntegrity());
  Assert(from>=_levelLoaded && from<=_nMipmaps);
  Assert(to>=_levelLoaded && to<=_nMipmaps);
  int size = 0;
  for (int i = from; i < to; i++)
  {
    #if TRACK_TEXTURES
      if (_surfaces[i].GetSurface().NotNull() && strstr(GetName(),InterestedIn))
      {
        LogF("@@@ ReleaseMemory %s: %d (%dx%d)",cc_cast(GetName()),i,_surfaces[i]._w,_surfaces[i]._h);
      }
    #endif
    size += ReleaseMemory(_surfaces[i], store, batch);
  }
  MemoryReleased(from,to);
  // Assert(CheckIntegrity());
  return size;
}

int TextureD3DT::ReleaseMemory( bool store, D3DSurfaceDestroyerT *batch )
{
  return ReleaseLevelMemory(_levelLoaded,_nMipmaps,store,batch);
}

int TextureD3DT::ReleaseSystem(SurfaceInfoD3DT &surface, bool store, D3DSurfaceDestroyerT *batch)
{
  int ret = 0;
  TextBankD3DT *bank=static_cast<TextBankD3DT *>(GEngine->TextBank());
  if (surface.GetSurface()._texture)
  {
    if( store )
    {
      // move data to reserve bank
      #if REPORT_ALLOC>=30
        LogF
        (
          "SYS Add To Bank %dx%dx%s (%dB)",
          surface._w,surface._h,
          FormatName(surface._format),surface.SizeUsed()
        );
      #endif
      bank->AddSystem(surface);
      surface.Free(false);
    }
    else
    {
      #if REPORT_ALLOC>=15
        if (!batch || REPORT_ALLOC>=15)
        {
          LogF
          (
            "SYS Release %dx%dx%s (%dB), rest %d, %s",
            surface._w,surface._h,
            FormatName(surface._format),surface.SizeUsed(),
            bank->_systemAllocated,batch ? "Batch" : ""
          );
        }
      #endif
      bank->_systemAllocated -= surface.SizeUsed();
      ret += surface.SizeUsed();
      //ADD_COUNTER(syRel,1);
      if (batch) batch->Add(surface);
      surface.Free(true,batch!=NULL);
    }
  }
  return ret;
}

void TextureD3DT::ReuseMemory( SurfaceInfoD3DT &surf )
{
  // reuse immediately
  if (_surfaces[_levelLoaded].GetSurface()._texture)
  {
    // first of all: move data to reserve bank

    surf = _surfaces[_levelLoaded];
    // allocated memory amount not changed
    #if REPORT_ALLOC>=20
      LogF
      (
        "VID Directly reused %dx%dx%s (%dB) - '%s'",
        surf._w,surf._h,FormatName(surf._format),surf.SizeUsed(),Name()
      );
    #endif
    _surfaces[_levelLoaded].Free(false);
  }
  MemoryReleased(_levelLoaded,_levelLoaded+1);
}

static PacFormat DstFormat( PacFormat srcFormat, int dxt )
{
  // memory representation
  switch (srcFormat)
  {
    case PacARGB1555:
    case PacRGB565:
    case PacARGB4444:
      if (GEngineDD->Can8888()) return PacARGB8888;
      return PacARGB4444;
    case PacARGB8888:
      if (GEngineDD->Can8888()) return PacARGB8888;
      return PacARGB4444;
    case PacP8:
      if (GEngineDD->Can8888()) return PacARGB8888;
      return PacARGB4444;
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
      // TODO: fail
      return srcFormat;
  }
}

#include <El/Debugging/debugTrap.hpp>

void TextureD3DT::SetMipmapRange( int min, int max )
{
  LoadHeaders();
  // if there is some surface, it is not usable now,
  // because it already contains mipmaps
  ReleaseMemory(true);
  if( min<0 ) min=0;
  if( max>_nMipmaps-1 ) max=_nMipmaps-1;
  if( min>max ) min=max;
  // disable mipmaps before min and after max
  _largestUsed = min;
  SetNMipmaps(max+1);
  
  if (_smallLevel<_largestUsed) _smallLevel=_largestUsed;
  UpdateMaxTexDetail();
}

/*!
\patch_internal 1.05 Date 7/16/2001 by Ondra
- Fixed: Older alpha textures without alpha taggs were not recognized,
resulting in bad renderstates while drawing. Alpha artifacts were visible.
- bug introduced while implementing JPG support.
*/

int TextureD3DT::Init(const char *name)
{
  SetName(name);

  _maxSize=INT_MAX; // no texture size limit
  _usageType = TexObject;

  // quick check if source does exist
  ITextureSourceFactory *factory = SelectTextureSourceFactory(name);
  if (!factory || !factory->Check(name))
  {
    ErrorMessage(EMMissingData, "Cannot load texture %s.",(const char *)GetName());
    _nMipmaps=0;
    _levelLoaded = _smallLevel = _largestUsed = _nMipmaps;
    return -1;
  }
  PreloadHeaders();
  //DoInit();
  return 0;
}

int TextureD3DT::Init(ITextureSource *source)
{
  // no name - directly defined texture
  SetName("");

  _src = source;

  if (_src->Init(_mipmaps,MAX_MIPMAPS))
  {
    _initialized = true;
    DoInitSource();
  }
  else
  {
    LogF("No init");
  }
  return 0;
}

void TextureD3DT::PreloadHeaders()
{
  // if headers are already loaded, there is no need to load them once again
  if (_initialized) return;
  ITextureSourceFactory *factory = SelectTextureSourceFactory(Name());
  if (factory->PreInit(Name(),this))
  {
    LoadHeaders();
  }
}

void TextureD3DT::RequestDone(RequestContext *context, RequestResult result)
{
  #if _DEBUG
    if (strstr(GetName(),"xfonts\\helveticanarrowb22-03"))
    {
      __asm nop; // break opportunity
    }
    else if (strstr(GetName(),"xfonts\\helveticanarrowb22-"))
    {
      __asm nop; // break opportunity
    }
    else if (strstr(GetName(),"xfonts\\"))
    {
      __asm nop; // break opportunity
    }
  #endif
  // note: initialization might be done in between,
  // because it was required by someone else
  //Log("%s: Headers loaded",(const char *)Name());
  // we cannot ignore canceled requests, as nobody would re-submit them
  //if (!_initialized && result!=RRCanceled)
  if (!_initialized)
  {
    DoLoadHeaders();
  }
}

/*!
\patch_ 1.50 Date 4/15/2002 by Ondra
- Optimized: Texture loading is now postponed to "Get ready" screen.
*/

void TextureD3DT::DoLoadHeaders()
{
  Assert (!_initialized);
  _initialized = true;

  // if max. size is not set yet, auto-detect it
  if (_maxSize>=0x10000)
  {
    TextBankD3DT *bank=static_cast<TextBankD3DT *>(GEngine->TextBank());
    _maxSize = bank->MaxTextureSize(_usageType);
  }
  saturateMin(_maxSize,GEngineDD->MaxTextureSize());

  ITextureSourceFactory *factory = SelectTextureSourceFactory(Name());
  _src = factory ? factory->Create(Name(),_mipmaps,MAX_MIPMAPS) : NULL;

  DoInitSource();
}

void TextureD3DT::DoInitSource()
{
  if (_src)
  {
    PacFormat format=BasicFormat(GetName());
    
    bool isPaa = ( format==PacARGB4444 );
    TextBankD3DT *bank=static_cast<TextBankD3DT *>(GEngine->TextBank());

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
      _src->ForceAlpha();
    }


    int dxt = GEngineDD->DXTSupport();
    PacFormat dFormat = DstFormat(format,dxt);

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
      Assert(i < MAX_MIPMAPS);
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

    int smallLevel;
    // find first mipmap large enough to be useful
    for( smallLevel=_nMipmaps-1; smallLevel>_largestUsed; smallLevel-- )
    {
      const PacLevelMem &mipTop = _mipmaps[smallLevel];
      int size = mipTop.MipmapSize(mipTop._dFormat,mipTop._w,mipTop._h);
      // 4/3 is used to calculate mipmap requirements
      // 2/3 is how much we require to be really used
      //if( size*4/3>=bank->_minTextureSize*2/3 ) break;
      // optimized to:
      if( size*2>=bank->_minTextureSize ) break;
    }
    _smallLevel = smallLevel;

    _levelNeededThisFrame=_levelNeededLastFrame=i; // no level loaded, no needed
    UpdateMaxTexDetail();
    UpdateLoadedTexDetail(_nMipmaps);

    return;
  } 
  
  ErrorMessage(EMMissingData, "Cannot load texture %s.",(const char *)GetName());
  _nMipmaps=0;
  _levelLoaded = _smallLevel = _largestUsed = _nMipmaps;
  return;
}

void TextureD3DT::DoUnloadHeaders()
{
  if (_initialized)
  {
    ReleaseMemory();
    _initialized = false;
    _src.Free();
  }
}

void TextureD3DT::CheckForChangedFile()
{
  RStringB name = GetName();
  // procedural or unnamed texture cannot (and need not to) be reloaded
  if (name[0]==0 || name[0]=='#') return;
  // if the texture is not ready, there is no need to reload it
  if (!HeadersReady()) return;
  QFileTime fileTime = QFBankQueryFunctions::TimeStamp(name);
  if (fileTime>GetTimestamp())
  {
    // we need to reload as much as possible
    DoUnloadHeaders();
    LoadHeaders();
  }
}

void TextureD3DT::LoadHeaders()
{
  if (_initialized) return;
  DoLoadHeaders();
}
Color TextureD3DT::GetColor() const
{
  LoadHeadersNV();
  return _src ? Color(_src->GetAverageColor())*Color(_src->GetMaxColor()) : HBlack;
}
Color TextureD3DT::GetMaxColor() const
{
  LoadHeadersNV();
  return _src ? _src->GetMaxColor() : HWhite;
}

Color TextureD3DT::GetPixel( int level, float u, float v ) const
{
  LoadHeadersNV();
  // Reduce the level to the largest used texture
  level = max(level, _largestUsed);

  if (!_src) return HWhite;

  PacLevelMem mip = _mipmaps[level];

  AutoArray<char> mem;
  int dataSize = PacLevelMem::MipmapSize(mip._dFormat,mip._w,mip._h);
  mem.Realloc(dataSize);
  mem.Resize(dataSize);

  // the pitch stored in the texture is determined by HW
  // we want compact layout here
  mip._pitch = dataSize/mip._h;

  // get data from the texture source
  _src->GetMipmapData(mem.Data(),mip,level);

  Color col=Color(mip.GetPixel(mem.Data(),u,v))*Color(_src->GetMaxColor());
  return col;
}

bool TextureD3DT::GetRect(
  int level, int x, int y, int w, int h, PacFormat format, void *data
) const
{
  Assert(_initialized);

  // open texture file
  if (!_src) return false;

  PacLevelMem mip=_mipmaps[level];

  // only whole surface is currently supported
  DoAssert(x==0);
  DoAssert(y==0);
  DoAssert(w==mip._w);
  DoAssert(h==mip._h);
  // override default destination format
  mip._dFormat = format;

  int dataSize = PacLevelMem::MipmapSize(format,mip._w,mip._h);
  
  mip._pitch = dataSize/mip._h;
  
  _src->GetMipmapData(data,mip,level);

  return true;
  
}

bool TextureD3DT::RequestGetRect(int level, int x, int y, int w, int h) const
{
  if (!_initialized) return false;
  // open texture file
  if (!_src) return false;

  // only whole surface is currently supported
  FileRequestPriority prior = FileRequestPriority(10);
  return _src->RequestMipmapData(_mipmaps,level,level,prior);
}



DEFINE_FAST_ALLOCATOR(TextureD3DT);

TextureD3DT::TextureD3DT()
:_nMipmaps(0),
_levelLoaded(0),_largestUsed(0),_smallLevel(0),
_levelNeededThisFrame(0),_levelNeededLastFrame(0), // no level loaded, all needed
_inUse(0),
_maxSize(256),_usageType(TexObject),
_initialized(false),_permanent(false),_performance(0)
{
}

TextureD3DT::~TextureD3DT()
{
  if (GEngine) GEngine->TextureDestroyed(this);
  bool store = !IsOutOfMemory();
  ReleaseMemory(store);
}

void TextureD3DT::SetUsageType( UsageType type )
{
  UsageType oldType = _usageType;
  // if the same texture is used multiple times, highest usage type should apply
  if (type>_usageType) _usageType = type;
  // depending on usage type determine the max size
  if (oldType!=_usageType)
  {
    AdjustMaxSize();
  }
}

void TextureD3DT::AdjustMaxSize()
{
  TextBankD3DT *bank=static_cast<TextBankD3DT *>(GEngine->TextBank());
  int size = bank->MaxTextureSize(_usageType);
  if (_initialized)
  {
    // limit size by real texture size
    int maxSize = _mipmaps[0]._w;
    saturateMax(maxSize,_mipmaps[0]._h);
    saturateMin(size,maxSize);
    // set max. size
    _maxSize = size;
    saturateMin(_maxSize,GEngineDD->MaxTextureSize());
    // scan for corresponding mipmap level
    int nMipmaps = _src ? _src->GetMipmapCount() : 0;
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
    if (_largestUsed > _nMipmaps-1) _largestUsed = _nMipmaps-1;
    if (_smallLevel < _largestUsed) _smallLevel = _largestUsed;
    UpdateMaxTexDetail();
    // make sure levels up to _largestUsed are not loaded
    if (_levelLoaded<_largestUsed)
    {
      ReduceTextureLevel(_largestUsed);
      DoAssert(_levelLoaded>=_largestUsed);
    }
  }
  else
  {
    _maxSize = size;
    saturateMin(_maxSize,GEngineDD->MaxTextureSize());
  }
}

D3DSurfaceDestroyerT::D3DSurfaceDestroyerT()
{
}

D3DSurfaceDestroyerT::~D3DSurfaceDestroyerT()
{
  DestroyAll();
}


static int CmpSurfById( const D3DSurfaceToDestroy *s1, const D3DSurfaceToDestroy *s2 )
{
  // move small ID to the end
  return s2->_id-s1->_id;
}

void D3DSurfaceDestroyerT::Add(const SurfaceInfoD3DT &surf)
{
  if (surf.GetSurface()._texture)
  {
    D3DSurfaceToDestroy destroy;
    destroy._surface = surf.GetSurface();
    destroy._id = surf.GetCreationID();
    _surfaces.Add(destroy);
  }
}

void D3DSurfaceDestroyerT::DestroyAll()
{
  {
    PROFILE_SCOPE_EX(txDAS,tex);
    QSort(_surfaces.Data(),_surfaces.Size(),CmpSurfById);
  }
  #if TRACK_TEXTURES
    for (int i=0; i<_surfaces.Size(); i++)
    {
      D3DSurfaceToDestroy &surf = _surfaces[i];
      InterestedInResource(surf._surface,"Destroy");
    }
  #endif
  #if REPORT_ALLOC>=10
    for (int i=0; i<_surfaces.Size(); i++)
    {
      D3DSurfaceToDestroy &surf = _surfaces[i];
      LogF("  destroy %x, id %d",surf._surface,surf._id);
    }
  #endif

  PROFILE_SCOPE_EX(txDAC,tex);
  _surfaces.Clear();
}


int SurfaceInfoD3DT::CalculateSize
(
  ID3D10Device *dDraw, const TEXTUREDESC9 &desc, PacFormat format,
  int totalSize
)
{
  if (totalSize>0) return totalSize;
  int nMipmaps = desc.nMipmaps;
  int cTotalSize=0;
  int size=PacLevelMem::MipmapSize(format,desc.w,desc.h);
  for( int level=nMipmaps; --level>=0; )
  {
    cTotalSize+=size;
    size>>=2;
  }
  return cTotalSize;
}

int SurfaceInfoD3DT::_nextId;

HRESULT SurfaceInfoD3DT::CreateSurface(
  EngineDDT *engine, ID3D10Device *dDraw, const TEXTUREDESC9 &desc, PacFormat format, bool cpuAccess, int totalSize
)
{
  // Direct3D 10 NEEDS UPDATE 
  if (_surface._texture)
  {
    Fail("Non-free surface created.");
  }

  // TODO: check Can88
  _w = desc.w;
  _h = desc.h;
  _nMipmaps = desc.nMipmaps;
  {
    int cTotalSize=0;
    int size=PacLevelMem::MipmapSize(format,_w,_h); // assume 16-b textures
    for( int level=_nMipmaps; --level>=0; )
    {
      cTotalSize+=size;
      size>>=2;
    }
    if( totalSize<0 ) totalSize=cTotalSize;
    Assert( totalSize==cTotalSize );
  }
  _format=format;
  _totalSize=totalSize;

  EngineDDT::CheckMemTypeScope scope(engine, !cpuAccess ? "VIDtex" : "SYStex", totalSize);
  if (!cpuAccess)
  {
    _usedSize=engine->AlignVRAMTex(totalSize);
  }

  PROFILE_SCOPE_EX(texCT,tex);

  // Create the texture
  D3D10_TEXTURE2D_DESC td;
  td.Width = _w;
  td.Height = _h;
  td.MipLevels = _nMipmaps;
  td.ArraySize = 1;
  td.Format = desc.format;
  td.SampleDesc.Count = 1;
  td.SampleDesc.Quality = 0;
  td.Usage = cpuAccess ? D3D10_USAGE_STAGING : D3D10_USAGE_DEFAULT;
  td.BindFlags = cpuAccess ? 0 : D3D10_BIND_SHADER_RESOURCE;
  td.CPUAccessFlags = cpuAccess ? (D3D10_CPU_ACCESS_WRITE | D3D10_CPU_ACCESS_READ) : 0;
  td.MiscFlags = 0;
  HRESULT err = dDraw->CreateTexture2D(&td, NULL, _surface._texture.Init());
  if (err!=D3D_OK)
  {
    Fail("Error: Texture creation failed");
    LogF("Dimensions %d:%d,%d",_w,_h,_nMipmaps);
    GEngineDD->DDError9("Error: Create Surface Error",err);
  }
  else
  {
    // Texture was created, now create the resource view
    if (!cpuAccess)
    {
      D3D10_SHADER_RESOURCE_VIEW_DESC srv;
      ZeroMemory(&srv, sizeof(srv));
      srv.Format = td.Format;
      srv.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
      srv.Texture2D.MipLevels = td.MipLevels;
      err = dDraw->CreateShaderResourceView(_surface._texture, &srv, _surface._resview.Init());
      if (err!=D3D_OK)
      {
        Fail("Error: Texture resource view creation failed");
        GEngineDD->DDError9("Error: Create Texture resource view error", err);
      }
    }

    #if TRACK_TEXTURES
      BString<256> text;
      if (desc.pool!=D3DPOOL_SYSTEMMEM)
      {
        sprintf(
          text,"TEX_ALLOC %s: %d (%dx%d,%d)",
          cc_cast(CreatingSurface),totalSize,_w,_h,_nMipmaps
        );
        if (strstr(CreatingSurface,InterestedIn))
        {
          LogF("@@@ CreateTexture %s",cc_cast(text));
        }
      }
      else
      {
        sprintf(text,"TEX_SYS   %s: %d",cc_cast(CreatingSurface),totalSize);
      }
      _surface->SetPrivateData(PrivateDataGUID,text.cstr(),strlen(text.cstr())+1,0);
    #endif
    _usedSize = engine->AlignVRAMTex(totalSize);
    _id = _nextId++;
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

#if TRACK_TEXTURES

SurfaceInfoD3DT::SurfaceInfoD3DT(const SurfaceInfoD3DT &src)
:_surface(src._surface)
{
  if (src._surface)
  {
    InterestedInResource(src._surface,"Copy construct");
  }
  
	_id = src._id;
	_totalSize = src._totalSize;
	_usedSize = src._usedSize;
	_w = src._w;
	_h = src._h;
	_nMipmaps = src._nMipmaps;
	_format = src._format;
}

void SurfaceInfoD3DT::operator = (const SurfaceInfoD3DT &src)
{
  if (src._surface)
  {
    InterestedInResource(src._surface,"Copy");
  }
  
  _surface = src._surface;
	_id = src._id;
	_totalSize = src._totalSize;
	_usedSize = src._usedSize;
	_w = src._w;
	_h = src._h;
	_nMipmaps = src._nMipmaps;
	_format = src._format;
}
#endif

void SurfaceInfoD3DT::Free(bool lastRef, int refValue)
{
  //GEngineDD->TextBankDD()->ReportTextures("* Free [");
  // before freeing surface free its associated memory
  PROFILE_DX_SCOPE(3dSFr);
  SurfaceData &surf = _surface;
  #if TRACK_TEXTURES
    if (lastRef && refValue==0)
    {
      InterestedInResource(_surface,"Free last ref to");

      BString<256> desc;
      sprintf(desc,"TEX_FREE  0");
      _surface->SetPrivateData(PrivateDataGUID,desc.cstr(),strlen(desc.cstr())+1,0);
    }
    else
    {
      if (_surface)
      {
        InterestedInResource(_surface,"Free ref to");
      }
    }
  #endif
  
  _surface._resview.Free();
  int result = _surface._texture.Free();
  if (lastRef)
  {
    #if REPORT_ALLOC>=10
    if (surf && refValue==0)
    {
      LogF("  destroy %x, id %d, size %d",surf,_id,_usedSize);
    }
    #endif
    #if 1
    if (result!=refValue)
    {
      RptF("Surf: Some references to %x left (%d!=%d)",surf,result,refValue);
    }
    #endif
  }
  //GEngineDD->TextBankDD()->ReportTextures("* Free ]");
  _w=0;
  _h=0;
  _nMipmaps=0;
  _totalSize=0;
  _usedSize = 0;
}

/*!
  \patch 5090 Date 11/28/2006 by Flyman
  - Fixed: Fixed bug in texture management. All textures could have been thrown away under certain conditions
*/
int TextBankD3DT::CreateVRAMSurface(
  SurfaceInfoD3DT &surface,
  const TEXTUREDESC9 &desc, PacFormat format, int totalSize
)
{
  TEXTUREDESC9 ddsd = desc;
  ID3D10Device *dDraw = _engine->GetDirect3DDevice();
  HRESULT err = surface.CreateSurface(_engine, dDraw, ddsd, format, false, totalSize);
  DoAssert(SUCCEEDED(err));
  return 0;

  // Direct3D 10 NEEDS UPDATE 
//   // VRAM creation - mark it
//   TEXTUREDESC9 ddsd=desc;
// 
//   ID3D10Device *dDraw=_engine->GetDirect3DDevice();
// 
//   bool retry = false;
//   Retry:
//   //ReportTextures("* Create [");
//   HRESULT err=surface.CreateSurface(_engine,dDraw,ddsd,format,false,totalSize);
// 
//   //ReportTextures("* Create ]");
//   if (retry)
//   {
//     if (err!=D3D_OK)
//     {
//       _engine->DDError9("Create Surface Error",err);
//     }
//     else
//     {
//       LogF("Retry OK");
//     }
//   }
// 
//   if(err==E_OUTOFMEMORY)
//   {
//     // first try to deallocate some conventional memory
//     int reserved = ForcedReserveMemory(totalSize);
// 
//     // Calculate how much memory we need to reserve in this case
//     _freeTextureMemory = FreeTextureMemory();
//     int reserve = _freeTextureMemory - reserved - totalSize;
// 
//     // Update the reserve (to omit the same situation later)
//     if (reserve>_reserveTextureMemory)
//     {
//       // it seems we need more texture memory than expected
//       _reserveTextureMemory = reserve;
// 
//       // Recalculate the _limitAlocatedTextures used to invoke mipbias setting
//       RecalculateTextureMemoryLimits();
//     }
// 
//     if (_freeTextures.Size()<=0)
//     {
//       ReportTextures("Forced Reserve report");
//     }
//     if( reserved )
//     {
//       retry = true;
//       goto Retry;
//     }
//     LogF("SYS: No space left %d",totalSize);
//     return -1; // out of memory
//   }
//   else if(err==D3DERR_OUTOFVIDEOMEMORY )
//   {
//     // video memory error reported
//     // first try to evict some video memory, than system memory
//     Log("Out of video memory.");
// 
// // Alternative implementation not depending on _freeTextureMemory calculation
// //     // If we success to free some memory, retry creating the texture
// //     if (ForcedReserveMemory(totalSize))
// //     {
// //       retry = true;
// //       goto Retry;
// //     }
// // 
// //     // Update the amount of memory reserved for fragmentation (_reserveTextureMemory) and
// //     // recalculate the _limitAlocatedTextures used to invoke mipbias setting. We know that
// //     // prior next call the ReserveMemoryUsingReduction can be called now, so incrementing
// //     // of _reserveTextureMemory should stop one day.
// //     _reserveTextureMemory += totalSize;
// //    _freeTextureMemory = FreeTextureMemory();
// //     RecalculateTextureMemoryLimits();
// // 
// //     // Return error
// //     LogF("VID: No space left %d",totalSize);
// //     return -1; // out of memory
// 
//     // Now we know we failed to get the memory (can be caused by fragmentation)
//     // So first check out how much memory can we get
//     int reserved = ForcedReserveMemory(totalSize);
// 
//     // Calculate how much memory we need to reserve in this case
//     _freeTextureMemory = FreeTextureMemory();
//     _engine->CheckLocalNonlocalTextureMemory();
//     int reserve = _freeTextureMemory - reserved - totalSize;
// 
//     // Update the reserve (to omit the same situation later)
//     if (reserve>_reserveTextureMemory)
//     {
//       // it seems we need more texture memory than expected
//       _reserveTextureMemory = reserve;
// 
//       // Recalculate the _limitAlocatedTextures used to invoke mipbias setting
//       RecalculateTextureMemoryLimits();
//     }
//     if (reserved>0)
//     {
//       retry = true;
//       goto Retry;
//     }
//     LogF("VID: No space left %d",totalSize);
//     return -1; // out of memory
//   }
//   if( err ) _engine->DDError9("Cannot create video memory mipmap.",err);
//   Assert( err==D3D_OK );
//   TextureAllocated(surface.SizeUsed(),surface.Slack());
//   _totalSlack += surface.Slack();
//   return 0;
}

int TextBankD3DT::CreateVRAMSurfaceSmart
(
  SurfaceInfoD3DT &surface,
  const TEXTUREDESC9 &desc, PacFormat format, int totalSize, float mipImprovement
)
{
  PROFILE_SCOPE_EX(txCVS,tex);

  if( CreateVRAMSurface(surface,desc,format,totalSize)<0 )
  {
    return -1;
  }
  Assert(surface._nMipmaps==desc.nMipmaps);
  return 0;

  // Direct3D 10 NEEDS UPDATE 
//   UseReleased(surface,desc,format);
// 
//   Assert(!surface.GetSurface() || surface._nMipmaps==desc.nMipmaps);
//   
//   if(!surface.GetSurface())
//   {
//     // do not try reusing until used nearly all video memory
//     if (_totalAllocated>_limitAlocatedTextures/16*15)
//     {
//       // avoid hasty release of useful data
//       // if there are too many free textures, release some of them first
//       // if is possible that they are in some format which is not very common
//       if (_freeAllocated<_limitAlocatedTextures/16*2)
//       {
//         Reuse(surface, desc, format, true, mipImprovement);
//         Assert(!surface.GetSurface() || surface._nMipmaps==desc.nMipmaps);
//       }
//     }
// 
//     if (!surface.GetSurface())
//     {
//       // if nothing can be reused, we have to create something new
//       int neededSize = _engine->AlignVRAMTex(totalSize);
//       ReserveMemory(neededSize,2);
// 
//       if (_totalAllocated+neededSize>_limitAlocatedTextures)
//       {
//         if (!ReserveMemoryUsingReduction(neededSize, mipImprovement))
//         {
//           LogF("CreateVRAMSurface throttled (%d,%d)",neededSize,_totalAllocated);
//           return -1;
//         }
//       }
// 
//       if( CreateVRAMSurface(surface,desc,format,totalSize)<0 )
//       {
//         return -1;
//       }
//       Assert(surface._nMipmaps==desc.nMipmaps);
// 
//       _thisFrameAlloc++;
// 
//       ADD_COUNTER(tGRAM,totalSize);
//     }
//     // we created or reused a texture
//   }
// 
//   Assert(surface._nMipmaps==desc.nMipmaps);
//   Assert(surface.SizeExpected()==totalSize);
//     
//   return 0;
}

bool TextureD3DT::VerifyChecksum( const MipInfo &absMip ) const
{
  return true;
}

// constructor

TextBankD3DT::TextBankD3DT(EngineDDT *engine, ParamEntryPar cfg, ParamEntryPar fcfg)
:_totalAllocated(0),_totalSlack(0),
_systemAllocated(0),
_freeAllocated(0),
_freeOnDemand(_used)
{
  //RegisterFreeOnDemandSystemMemory(this);
  RegisterFreeOnDemandMemory(&_freeOnDemand);
  #if _ENABLE_CHEATS
    _texStressMode = 0;
    _stressTestTotalSize = 0;
  #endif

  // to avoid frequent system surface creation we maintain a pool of used surfaces
  _limitSystemTextures = 8*1024*1024;
  _reserveTextureMemory = 0;
  _engine=engine;


  _maxTextureMemory = INT_MAX;

  _maxTextureQuality = 2;
  _textureQuality = 1;
  
  // will set _limitAlocatedTextures and _freeTextureMemory
  _freeTextureMemory = FreeTextureMemory();
  RecalculateTextureMemoryLimits();

  _maxTextureMemory = _freeTextureMemory;

  ResetMaxTextureQuality();
  
  // set small texture limit based on texture allocator granularity
  _minTextureSize = _engine->GetTexAlignment();
  // on Xbox we have custom allocator capable of allocating 256B aligned blocks
  // we assume PC video memory allocation is about the same
  // if the allocator forces us to use more than 256 KB, it is broken anyway
  // 8800
  saturate(_minTextureSize,16*16,512*512);
  LogF("Min texture size %d B",_minTextureSize);

  //PreCreateVRAM(512*1024,true);
}

TextBankD3DT::~TextBankD3DT()
{
  DeleteAllAnimated();

  _white.Free();
  _random.Free();
  _defNormMap.Free();
  _defDetailMap.Free();
  _defMacroMap.Free();
  _defSpecularMap.Free();
  _permanent.Clear();
  _texture.RemoveNulls();
  Assert (_texture.Size()==0);
  _texture.Clear();
  //_ditherMap.Free();
  ClearTexCache();
  
  if (!IsOutOfMemory())
  {
    // use batch destroyed to achieve fast texture destruction
    // by destroying in reverse order to creation order
    // this is possible only when we have enough memory to store the batch
    D3DSurfaceDestroyerT batch;
    for( int i=0; i<_freeTextures.Size(); i++ )
    {
      batch.Add(_freeTextures[i]);
    }
    for( int i=0; i<_freeSysTextures.Size(); i++ )
    {
      batch.Add(_freeSysTextures[i]);
    }
    _freeSysTextures.Clear();
    _freeAllocated = 0;
    _freeTextures.Clear();
    batch.DestroyAll();
  }
  else
  {
    _freeTextures.Clear();
    _freeAllocated = 0;
    _freeSysTextures.Clear();
  }
}

void TextBankD3DT::Compact()
{
  _texture.RemoveNulls();
}

void TextBankD3DT::StopAll()
{
  //_loader->DeleteAll();
}

static void SurfaceName( RString &buf, const SurfaceInfoD3DT &surface )
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
  buf = Format("\t%s,%d,%d,",name,surface._w,surface._h);
}

#include <El/Statistics/statistics.hpp>

bool TextBankD3DT::VerifyChecksums()
{
  StatisticsByName stats;
  // print released textures statistics
  for( int i=0; i<_freeTextures.Size(); i++ )
  {
    const SurfaceInfoD3DT &surface=_freeTextures[i];
    RString name;
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
    TextureD3DT *texture=_texture[i];
    if( !texture ) continue;
    if( texture->_surfaces[texture->_levelLoaded].GetSurface()._texture )
    {
      RString name;
      SurfaceName(name,texture->_surfaces[texture->_levelLoaded]);
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


int TextBankD3DT::Find(RStringB name1)
{
  int i;
  //RString cName(name1);
  for( i=0; i<_texture.Size(); i++ )
  {
    TextureD3DT *texture=_texture[i];
    if( texture )
    {
      if (name1 != texture->GetName()) continue;
      return i;
    }
  }
  return -1;
}

int TextBankD3DT::FindFree()
{
  int i;
  for( i=0; i<_texture.Size(); i++ )
  {
    TextureD3DT *texture=_texture[i];
    if( !texture ) return i;
  }
  return _texture.Size();
}

TextureD3DT *TextBankD3DT::Copy( int from )
{
  if (from<0) return NULL;
  const TextureD3DT *source=_texture[from];
  if (!source) return NULL;
  const char *sName = source->Name();

  int iFree=FindFree();
  Assert( iFree>=0 );
  TextureD3DT *texture=new TextureD3DT;
  
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

Ref<Texture> TextBankD3DT::CreateTexture(ITextureSource *source)
{
  Ref<TextureD3DT> texture = new TextureD3DT;
  if (!texture) return NULL;
  if (texture->Init(source)) return NULL;
  return texture.GetRef();
}

Ref<Texture> TextBankD3DT::Load(RStringB name)
{
  // Try to find the created texture. If found then return it
  int i = Find(name);
  if (i >= 0) return _texture[i].GetLink();

#if _DEBUG
  if (strstr(name,"xfonts\\helveticanarrowb22-03"))
  {
    __asm nop; // break opportunity
  }
  else if (strstr(name,"xfonts\\helveticanarrowb22-"))
  {
    __asm nop; // break opportunity
  }
  else if (strstr(name,"xfonts\\"))
  {
    __asm nop; // break opportunity
  }
#endif
  
  // Find first free place to put the new texture to
  int iFree = _texture.FindKey(NULL);
  if (iFree < 0) iFree = _texture.Size();

  // Try searching cache, if not found then load from file
  Ref<Texture> texture = LoadCached(name);
  if (!texture)
  {
    // Create the texture
    Ref<TextureD3DT> tex = new TextureD3DT;
    if (tex->Init(name))
    {
      // set name to empty to avoid inserting in the cache
      tex->SetName(RStringB());
      return NULL;
    }
    texture = tex;
  }

  _texture.Access(iFree);
  TextureD3DT *txt=static_cast<TextureD3DT *>(texture.GetRef());
  _texture[iFree]=txt;
  Assert(!txt->_initialized || txt->_nMipmaps>0);
  
  #if _DEBUG
    if (strstr(name,"xfonts\\helveticanarrowb22-03"))
    {
      __asm nop; // break opportunity
    }
    else if (strstr(name,"xfonts\\helveticanarrowb22-"))
    {
      __asm nop; // break opportunity
    }
    else if (strstr(name,"xfonts\\"))
    {
      __asm nop; // break opportunity
    }
  #endif
  
  // request may be canceled now - restart it if necessary
  txt->PreloadHeaders();
  
  return txt;
}

// maintain texture cache

void TextBankD3DT::ReleaseAllTextures(bool releaseSysMem)
{
  LogF("Allocated before ReleaseAllTextures %d",_totalAllocated);
  if (_defSpecularMap) _defSpecularMap->ReleaseMemory();
  if (_defMacroMap) _defMacroMap->ReleaseMemory();
  if (_defDetailMap) _defDetailMap->ReleaseMemory();
  if (_defNormMap) _defNormMap->ReleaseMemory();
  if (_random) _random->ReleaseMemory();
  if (_white) _white->ReleaseMemory();
  // release memory for all permanent textures
  for (int i=0; i<_permanent.Size(); i++)
  {
    _permanent[i]->ReleaseMemory();
  }
  ReserveMemoryLimit(0,0);
  LogF("Allocated after ReleaseAllTextures %d",_totalAllocated);
  #if _ENABLE_REPORT
  if (_totalAllocated>0)
  {
    DoAssert(_used.First()==NULL);
    Fail("Flush did not flush everything");
    // scan what is allocated
    for (int i=0; i<_texture.Size(); i++)
    {
      TextureD3DT *texture = _texture[i];
      if (texture && texture->GetBigSurface())
      {
        RptF("  allocated: %s",cc_cast(texture->Name()));
      }
    }
    // check cached textures as well
    for (Texture *tex = _cache.First(); tex; tex=_cache.Next(tex))
    {
      TextureD3DT *texture = static_cast<TextureD3DT *>(tex);
      if (texture && texture->GetBigSurface())
      {
        RptF("  cache allocated: %s",cc_cast(texture->Name()));
      }
    }
    if( _freeTextures.Size()>0 )
    {
      RptF("FreeTextures: %d",_freeTextures.Size());
    }
    ReportTextures(NULL);
  }
  #endif
  if (releaseSysMem)
  {
    ReserveSystem(0);
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
  #if _ENABLE_CHEATS
  _stressTest.Clear();
  _stressTestTotalSize=0;
  #endif
  
}

void TextBankD3DT::ReleaseTexturesWithPerformance(int performance)
{
    // scan what is allocated
  D3DSurfaceDestroyerT batch;
  for (int i=0; i<_texture.Size(); i++)
  {
    TextureD3DT *texture = _texture[i];
    if (texture && texture->GetBigSurface())
    {
      if (texture->_performance==performance)
      {
        texture->ReleaseMemory(false,&batch);
        LogF("Flushed %s",cc_cast(texture->GetName()));
      }
    }
  }
  // check cached textures as well
  for (Texture *tex = _cache.First(); tex; tex=_cache.Next(tex))
  {
    TextureD3DT *texture = static_cast<TextureD3DT *>(tex);
    if (texture && texture->GetBigSurface())
    {
      if (texture->_performance==performance)
      {
        texture->ReleaseMemory(false,&batch);
        LogF("Flushed %s",cc_cast(texture->GetName()));
      }
    }
  }
  if( _freeTextures.Size()>0 )
  {
    for( int i=0; i<_freeTextures.Size(); i++ )
    {
      batch.Add(_freeTextures[i]);
    }
  }
}

int TextBankD3DT::ReserveMemoryLimit(int limit, int minAge)
{
  #define LOG_TIME 0
  #if LOG_TIME
  DWORD start = GlobalTickCount();
  #endif

  D3DSurfaceDestroyerT batchDestroy;
  bool someReleased=false;
  int totalReleased = 0;
  while( _freeTextures.Size()>0 && _totalAllocated>limit )
  {
    // free more than one at once
    int need=_totalAllocated-limit;
    totalReleased += DeleteLastReleased(need,&batchDestroy);
    someReleased=true;
  }
  // release as many mip-map data as necessary to get required memory
  // get last mip-map used
  HMipCacheD3DT *last=_used.First();
  while( _totalAllocated>limit )
  {
    if( !last ) break;

    if (minAge>FreeOnDemandFrameID()-last->GetFrameNum()) break;

    // release texture data
    TextureD3DT *texture=last->texture;
    if( texture->_inUse )
    {
      last=_used.Next(last);
      continue;
    }
    int level = last->level;
    // release texture data from texture heap
    Assert(level>=0);
    Assert(level>=texture->_levelLoaded);
    Assert(level<texture->_nMipmaps);
    Assert( texture->_surfCache[level]==last );
    
    #if _ENABLE_REPORT
      static int maxAgeReport = 10;
      if (last->GetFrameNum()+maxAgeReport>=FreeOnDemandFrameID() && limit>0)
      {
        LogF(
          "Releasing recent texture %s:%d - age %d",
          cc_cast(texture->GetName()),level,
          FreeOnDemandFrameID()-last->GetFrameNum()
        );
      }
    #endif
    
    someReleased=true;
    // we need to handle small mipmaps as atomic
    int to = level+1;
    if (to>texture->_smallLevel) to = texture->_nMipmaps;
    DoAssert(texture->_levelLoaded<texture->_nMipmaps);
    totalReleased += texture->ReleaseLevelMemory(texture->_levelLoaded,to,false,&batchDestroy);
    // make sure this one is no longer here
    Assert(texture->_surfCache[level]==NULL);
    // if there is any surface, it needs to be tracked
    #if _DEBUG
      Assert(texture->CheckIntegrity());
      for (int i=0; i<texture->_nMipmaps; i++)
      {
        if (texture->_surfaces[i].GetSurface()._texture.NotNull())
        {
          Assert(i>=to);
        }
      }
    #endif
    if (_used.First()==last)
    {
      RptF("Texture %s:%d..%d not released",cc_cast(texture->GetName()),texture->_levelLoaded,to);
      Fail("Infinite loop prevented in ReserveMemoryLimit");
      break;
    }
    last=_used.First();
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

static inline int CmpTexMemUsed(const InitPtr<TextureD3DT> *t1,  const InitPtr<TextureD3DT> *t2)
{
  const TextureD3DT *tt1 = *t1;
  const TextureD3DT *tt2 = *t2;
  int d= tt2->TotalVRAMUsed()-tt1->TotalVRAMUsed();
  if (d) return d;
  return strcmp(tt1->GetName(),tt2->GetName());
}

void TextBankD3DT::ReportTextures( const char *name)
{
  Fail("DX10 TODO");
  // Direct3D 10 NEEDS UPDATE 
//   #if _ENABLE_PERFLOG
//   // report all textures loaded into VRAM and sysRAM
//   LogFile file;
//   if (name && strchr(name,'.') && !strchr(name,'*') )
//   {
//     file.Open(name);
//   }
//   else
//   {
//     file.AttachToDebugger();
//   }
//   // report some global information
//   {
//     ID3D10Device *dDraw=_engine->GetDirect3DDevice();
//     UINT dwFree = dDraw->GetAvailableTextureMem();
// 
//     file << " Max " << _maxTextureMemory << " Free " << (int)dwFree << "\n";
//     file << "Allocated " << _totalAllocated << "\n";
//     
//     int overhead = _maxTextureMemory - dwFree - _totalAllocated;
//     file << "Overhead " << overhead << "\n";
//     if (overhead>4000000)
//     {
//       // something wrong happened - very big memory overhead
//       static bool once=true;
//       if (once)
//       {
//         LogF("#### Overhead #####");
//         once=false;
//       }
//     }
//   }
// 
// 
//   if (file.IsOpen())
//   {
//     int usedInBigTextures = 0;
//     int usedInFreeTextures = 0;
//     for (int i=0; i<_texture.Size(); i++)
//     {
//       TextureD3DT *texture = _texture[i];
//       if (!texture) continue;
//       for (int i = texture->_levelLoaded; i < texture->_nMipmaps; i++)
//       {
//         if (texture->_surfaces[i].GetSurface())
//         {
//           usedInBigTextures += texture->_surfaces[i].SizeUsed();
//         }
//       }
//     }
//     for (int i=0; i<_freeTextures.Size(); i++)
//     {
//       SurfaceInfoD3DT &free = _freeTextures[i];
//       if (free.GetSurface()) usedInFreeTextures+=free.SizeUsed();
//     }
//     int usedInTextures = usedInBigTextures + usedInFreeTextures;
//     file << "Used in big   textures: " << usedInBigTextures   << "\n";
//     file << "Used in free  textures: " << usedInFreeTextures  << "\n";
//     file << "Used (total)          : " << usedInTextures      << "\n";
// 
//     AutoArray< InitPtr<TextureD3DT> > list;
//     
//     list.Realloc(_texture.Size());
//     
//     for (int i=0; i<_texture.Size(); i++)
//     {
//       TextureD3DT *texture = _texture[i];
//       if (!texture) continue;
//       if (!texture->_initialized) continue;
//       // report only loaded textures
//       if (texture->_levelLoaded<texture->_nMipmaps )
//       {
//         list.Add(texture);
//       }
//     }
//     // scan cache as well
//     for (Texture *tex = _cache.First(); tex; tex=_cache.Next(tex))
//     {
//       TextureD3DT *texture = static_cast<TextureD3DT *>(tex);
//       if (!texture) continue;
//       if (!texture->_initialized) continue;
//       // report only loaded textures
//       if (texture->_levelLoaded<texture->_nMipmaps )
//       {
//         list.Add(texture);
//       }
//     }
// 
// 
//     QSort(list,CmpTexMemUsed);
//     
//     for (int i=0; i<list.Size(); i++)
//     {
//       TextureD3DT *texture = list[i];
//       file.PrintF("%-32s",texture->Name());
//       file.PrintF(" mips: %d",texture->NMipmaps());
//       file.PrintF(
//         " loaded %d, needed %.2f",texture->_levelLoaded,texture->LevelNeeded()
//       );
//       const PacLevelMem &mip = texture->_mipmaps[texture->_levelLoaded];
//       file.PrintF(" %dx%d, memUsed: %d",mip._w,mip._h,texture->TotalSize(texture->_levelLoaded));
//       file << "\n";
//     }
//     
//   }
//   #endif
}

float TextBankD3DT::PriorityTexVidMem() const
{
  return 4.0; // texture priority is very high, because textures are needed very often
}
RString TextBankD3DT::GetDebugNameTexVidMem() const
{
  return "Tex Vid Mem";
}

size_t TextBankD3DT::MemoryControlledTexVidMem() const
{
  return _systemAllocated;
}
size_t TextBankD3DT::MemoryControlledRecentTexVidMem() const
{
  return 0;
}
size_t TextBankD3DT::FreeOneItemTexVidMem()
{
  size_t ret = 0;
  ret = ReserveSystem(16*1024);
  return ret;
}

void TextBankD3DT::MemoryControlledFrameTexVidMem()
{
}

bool TextBankD3DT::ReserveMemoryUsingReduction(int size, float mipImprovement)
{
  int maxAllocAllowed = _limitAlocatedTextures-_engine->AlignVRAMTex(size);
  //if (LimitMemoryUsingReduction(maxAllocAllowed)) return true;
  if (LimitMemoryUsingMipBias(maxAllocAllowed,mipImprovement)) return true;
  return false;
}

void TextBankD3DT::ChangeMipBias(D3DMipCacheRootT &root, float change)
{
  for (HMipCacheD3DT *item = root.Last(); item; item = root.Prev(item))
  {
    TextureD3DT *texture=item->texture;
    texture->_levelNeededLastFrame += change;
    texture->_levelNeededThisFrame += change;
  }
}

float TextBankD3DT::FindWorstBias(D3DMipCacheRootT &root, float &maxBias, TextureD3DT *&maxTex) const
{
  for (HMipCacheD3DT *item = root.Last(); item; item = root.Prev(item))
  {
    // release texture data
    TextureD3DT *texture = item->texture;
    if (texture->_inUse) continue;
    // if nothing is loaded, nothing can be reduced
    if (texture->_levelLoaded >= texture->_nMipmaps) continue;
    // if the very minimum is loaded, nothing can be reduced
    if (texture->_levelLoaded >= texture->_smallLevel) continue;
    float bias = texture->LevelNeeded() - texture->_levelLoaded;
    if (maxBias < bias) maxBias = bias, maxTex = texture;
  }
  return maxBias;
}

bool TextBankD3DT::LimitMemoryUsingMipBias(int maxAllocAllowed, float mipImprovement)
{
  bool ret = true;
  float origBias = GetTextureMipBias();
#if _ENABLE_REPORT
  bool biasChanged = false;
  int thisUsed0 = _used.MemoryControlled();
#endif
  while(_totalAllocated > maxAllocAllowed)
  {
    // find a texture which has most space between what is loaded and what is needed
    // (or find the texture, which has loaded closest to what is needed and can be discarded first)
    // all textures should 
    float worstBias = -FLT_MAX;
    TextureD3DT *tex = NULL;
    FindWorstBias(_used,worstBias,tex);
    if (!tex)
    {
      ret = false;
      break;
    }
    // if worstBias is positive and large enough, there is no need to change bias
    // all we need to do is reducing the texture
    if (worstBias > 1)
    {
      // this probably will never happen, as such textures should be caught earlier
      int tgtLevel = toIntFloor(tex->_levelLoaded + worstBias);
      if (!tex->ReduceTextureLevel(tgtLevel))
      {
        // if we cannot reduce, we can evict the texture completely
        tex->ReleaseMemory();
      }
      continue;
    }
    // we need to reduce mipbias so that this texture is evicted
    // this means adjusting bias to that worstBias becomes at least 1
    // change it a little bit more than necessary to increase chance
    // it will not be changed again very soon
    float oldBias = GetTextureMipBias();
    float newBias = floatMin(oldBias + (1 - worstBias) + 0.2f, 5);
    if (newBias > oldBias)
    {
      // it has no sense to degrade mipbias so that even the newly loaded texture is degraded
      if (newBias > origBias + mipImprovement)
      {
        break;
      }
      float changeBias = newBias - oldBias;

#if _ENABLE_REPORT
      biasChanged = true;
#endif
      SetTextureMipBias(newBias);
      // change needed values for all textures
      ChangeMipBias(_used,changeBias);
    }

    int tgtLevel = tex->_levelLoaded + 1;
    if (!tex->ReduceTextureLevel(tgtLevel))
    {
      // if we cannot reduce, we can evict the texture completely
      tex->ReleaseMemory();
    }
  }
#if _ENABLE_REPORT
  if (biasChanged)
  {
    int thisUsed = _used.MemoryControlled();
    // we had to change bias - change it a little bit more
    LogF("Mip bias forced from %.3f to %.3f",origBias,GetTextureMipBias());
    LogF("  VRAM usage reduced %d->%d",thisUsed0,thisUsed);
    if (thisUsed0<_limitAlocatedTextures/4*3)
    {
      LogF("Strange reduction");
    }
  }
#endif
  return ret;
}

float TextBankD3DT::VRAMDiscardMetrics() const
{
  HMipCacheD3DT *item = _used.First();
  if (!item) return NULL;
  // check topmost entry age
  //TextureD3DT *tex = item->texture;
  return FreeOnDemandFrameID()-item->GetFrameNum();
}

int TextBankD3DT::VRAMDiscardCountAge(int minAge, int *totalSize) const
{
  int count = 0;
  int size = 0;
  
  for ( HMipCacheD3DT *last=_used.First(); last; last = _used.Next(last))
  {
    if (minAge>FreeOnDemandFrameID()-last->GetFrameNum()) break;

    // release texture data
    TextureD3DT *texture=last->texture;
    
    count++;

    size += texture->_surfaces[last->level].SizeUsed();
  }
  if (totalSize) *totalSize = size;
  return count;
}

int TextBankD3DT::ReserveMemory(int size, int minAge)
{
  PROFILE_SCOPE_EX(txRsM,tex);
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
  int maxVRAMAllowed = _limitAlocatedVRAM/16*63/4-_engine->AlignVRAMTex(size);
  if (maxVRAMAllowed<0) maxVRAMAllowed = 0;
    
    
  int free = 0;
  for(;;)
  {
    if (_totalAllocated+(int)_engine->UsedVRAM()<=maxVRAMAllowed) break;
    
    float bMetrics = _engine->VRAMDiscardMetrics();
    float tMetrics = VRAMDiscardMetrics();
  
    int freeStep = 0;  
    if (bMetrics>=tMetrics)
    {
      freeStep = _engine->ThrottleVRAMAllocation(_engine->UsedVRAM()-1,minAge);
    }
    else
    {
      freeStep = ReserveMemoryLimit(_totalAllocated-1,minAge);
    }
    if (freeStep<=0) break;
    free += freeStep;
  }

  return free + CheckMemoryAllocation();
}

int TextBankD3DT::CheckMemoryAllocation()
{
  // on some HW there is no use for local/nonlocal prediction
  if (!_engine->UseLocalPrediction()) return 0;
  // we want to make sure textures are not overfilling the local memory
  // if we see there is no local memory left and at the same time there are some very old textures
  // we should definitely consider evicting some of them
  const int safetyMargin = 4*1024*1024 + (_engine->GetLocalVramAmount()>>5);
  int free = 0;
  if (_engine->GetFreeLocalVramAmount()<safetyMargin)
  {
    int toFree = safetyMargin-_engine->GetFreeLocalVramAmount();
    // we want to evict some, however we do not want to evict anything recent
    const int evictTime = 15; // evict time in seconds
    int minAge = evictTime*50; // assume 50 fps
    if (_engine->GetDynamicVBNonLocal())
    {
      // do not evict vertex buffers - it would not help anyway
      // compute what the new limit should be?
      int loweredLimit = _totalAllocated-toFree;
      // the limit should never go below zero
      if (loweredLimit<0) loweredLimit = 0;
      // if there is no item older than minAge, ReserveMemoryLimit does nothing
      free = ReserveMemoryLimit(_totalAllocated-toFree,minAge);
    }
    else
    {
      while (toFree>free)
      {
        int freeStep = 0;
        float bMetrics = _engine->VRAMDiscardMetrics();
        float tMetrics = VRAMDiscardMetrics();
        if (bMetrics>=tMetrics)
        {
          freeStep = _engine->ThrottleVRAMAllocation(_engine->UsedVRAM()-1,minAge);
        }
        else
        {
          freeStep = ReserveMemoryLimit(_totalAllocated-1,minAge);
        }
        if (freeStep<=0) break;
        free += freeStep;
      }
    }
#if _ENABLE_CHEATS
    LogF("Local VRAM flush: %d KB, requested %d KB",(free+1023)/1024,(toFree+1023)/1024);
#endif
    if (free<toFree)
    {
      DIAG_MESSAGE(1000,"VRAM overflow detected");
    }
  }
  return free;
}

// assume driver will align RAM to some reasonable number
// 256 is experimental value for GeForce
// use GetAvailableVidMem after allocating very small texture to check this
// consider: this could be done run-time

// ATI seems to be using 4096 alignment

int TextBankD3DT::ForcedReserveMemory( int size, bool enableSysFree )
{
  const int minRelease=4*1024;
  if( size<minRelease ) size=minRelease; // force some minimal release
  int ret = ReserveMemory(size,0);
  
  LogF(
    "ForcedReserveMemory _totalAllocated=%d",_totalAllocated
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

int TextBankD3DT::ReserveSystemLimit(int limit)
{
  PROFILE_SCOPE_EX(texRS,tex);
  // release as many mip-map data as neccessary to get required memory
  D3DSurfaceDestroyerT batchDestroy;
  D3DSurfaceDestroyerT *batch = IsOutOfMemory() ? NULL : &batchDestroy;

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
  return totalReleased;
}

int TextBankD3DT::ReserveSystem( int size )
{
  return ReserveSystemLimit(_limitSystemTextures-size);
}

int TextBankD3DT::ForcedReserveSystem( int size )
{
  const int minRelease=4*1024;
  if( size<minRelease ) size=minRelease; // force some minimal release
  return ReserveSystemLimit(_systemAllocated-size);
}

void TextBankD3DT::StartFrame()
{
  if (_engine->IsAbleToDrawCheckOnly())
  {
    InitDetailTextures();
    // do not check texture memory each frame - this function can be quite slow
    //_freeTextureMemory = FreeTextureMemory();
    //RecalculateTextureMemoryLimits();

    StressTestFrame();
  }
  _thisFrameAlloc=0; // VRAM allocations/deallocations (count)

#if _ENABLE_CHEATS
  if (GInput.GetCheat3ToDo(DIK_X))
  {
    ReserveMemory(INT_MAX,0);
  }
  if (GInput.GetCheat2ToDo(DIK_E))
  {
    ::ReportTexturesX();
  }
#endif
}

void TextBankD3DT::PerformMipmapLoading()
{
  // load mipmaps for all textures which requested it
  // track what textures are still not loaded as needed
  // TODO: load textures with most missing information first
  #if _ENABLE_CHEATS
    float maxMissingLevels = 0;
    TextureD3DT *maxMissing = NULL;
    InitVal<int,0> missingMips[MAX_MIPMAPS];
  #endif
  PROFILE_SCOPE_EX(txMLo,*)
  for(;;)
  {
    TextureD3DT *req = static_cast<TextureD3DT *>(_loadRequested.First());
    if (!req) break;
    MipInfo loaded = UseMipmap(req,INT_MAX,INT_MAX);
    // check what was requested
    // compare with what was loaded

  #if _ENABLE_CHEATS
    //req->_levelNeededThisFrame;
    float levelsMissing = loaded._level-floatMax(req->_mipmapWanted,req->_largestUsed);
    if (levelsMissing>0)
    {
      int numMissing = toInt(levelsMissing);
      saturate(numMissing,0,MAX_MIPMAPS-1);
      missingMips[numMissing]++;
      
      if (levelsMissing>maxMissingLevels)
      {
        maxMissingLevels = levelsMissing;
        maxMissing = req;
      }
    }
  #endif

    _loadRequested.Delete(req);
  }
  
  #if _ENABLE_CHEATS
  if (CHECK_DIAG(DEStreaming))
  {
    if (maxMissingLevels>0)
    {
      DIAG_MESSAGE(200,Format("Missing %s:%.2f",cc_cast(maxMissing->GetName()),maxMissingLevels));
      BString<256> histogram;
      for (int i=0; i<MAX_MIPMAPS; i++)
      {
        if (missingMips[i]>0)
        {
          sprintf(histogram+strlen(histogram),"%d: %d ",i,missingMips[i]);
        }
      }
      if (histogram[0]!=0)
      {
        DIAG_MESSAGE(200,Format("Missing levels %s",cc_cast(histogram)));
      }
    }
  }
  #endif
}

void TextBankD3DT::FinishFrame()
{
  // handle texture lists
  PROFILE_SCOPE_EX(txHLi,*)
  
  //LogF("******* FinishFrame");
  // perform dynamic mip-bias adjustment now
  // basic criterion is - how much is used by _thisFrameWholeUsed
  // TODO: avoid CalculateMemory, calculate incrementally
  int thisUsed = _used.MemoryControlledRecent();
  // note: we should consisted partially used textures as well
  if (thisUsed<_limitAlocatedTextures/2*1)
  {
    float bias = GetTextureMipBias();
    bias -= 0.004f;
    saturateMax(bias,GetTextureMipBiasMin());
    SetTextureMipBias(bias);
  }
  else if (thisUsed<_limitAlocatedTextures/4*3)
  {
    float bias = GetTextureMipBias();
    bias -= 0.001f;
    saturateMax(bias,GetTextureMipBiasMin());
    SetTextureMipBias(bias);
  }
  else if (thisUsed>=_limitAlocatedTextures/8*7)
  {
    float bias = GetTextureMipBias();
    bias += 0.04f;
    saturateMin(bias,5.0f);
    SetTextureMipBias(bias);
  }

  // make last and this frame textures history one frame older

  // we might verify previous textures are marked as unused

  int frameId = FreeOnDemandFrameID();
  for( HMipCacheD3DT *tc=_used.Last(); tc; tc=_used.Prev(tc) )
  {
    // no need to traverse textures which are not recently used
    if (frameId-tc->GetFrameNum()>2) break;
    TextureD3DT *tex = tc->texture;
    tex->_levelNeededLastFrame = tex->_levelNeededThisFrame;
    tex->_levelNeededThisFrame = tex->_mipmapWanted;
  }
  // the loop was separated to make sure _mipmapWanted is still valid
  // even if the same texture is traversed several times (several mipmaps)
  for( HMipCacheD3DT *tc=_used.Last(); tc; tc=_used.Prev(tc) )
  {
    // no need to traverse textures which are not recently used
    if (frameId-tc->GetFrameNum()>2) break;
    TextureD3DT *tex = tc->texture;
    tex->ResetMipmap();
  }

}

DEFINE_FAST_ALLOCATOR(HMipCacheD3DT);

size_t HMipCacheD3DT::GetMemoryControlled() const
{
  Assert(level>=0);
  Assert(level>=texture->_levelLoaded);
  Assert(level<texture->_nMipmaps);
  return texture->_surfaces[level].SizeUsed();
}


size_t D3DMipCacheRootFreeOnDemandT::FreeOneItem(){return NULL;}
float D3DMipCacheRootFreeOnDemandT::Priority() const {return 1.0f;}
size_t D3DMipCacheRootFreeOnDemandT::MemoryControlled() const {return 0;}
size_t D3DMipCacheRootFreeOnDemandT::MemoryControlledRecent() const {return 0;}
RString D3DMipCacheRootFreeOnDemandT::GetDebugName() const {return "";}
void D3DMipCacheRootFreeOnDemandT::MemoryControlledFrame()
{
  // this is the reason why we implement MemoryFreeOnDemandHelper
  _list.Frame();
}

void TextureD3DT::CacheUse(TextBankD3DT &bank, int level)
{
  if (_permanent || _nMipmaps<=0)
  {
    return;
  }

  // we insert more detailed mipmaps first - this way they should be also discarded first
  Assert(level<=_smallLevel || level>=_nMipmaps) ;
  Assert(_smallLevel<_nMipmaps || _nMipmaps==0);
  for (int i=level; i<=_smallLevel; i++)
  {
    if (_surfCache[i])
    {
      HMipCacheD3DT *first = _surfCache[i];
      bank._used.Refresh(first);
    }
    else
    {
      HMipCacheD3DT *first=new HMipCacheD3DT;
      first->texture = this;
      first->level = i;
      bank._used.Add(first);
      _surfCache[i] = first;
    }
    #if TRACK_TEXTURES
      if (strstr(GetName(),InterestedIn))
      {
        LogF("@@@ Surface cached %s:%d",cc_cast(GetName()),i);
      }
    #endif
  }

}

#if _ENABLE_CHEATS
extern bool DisableTextures;
extern bool StopLoadingTextures;
#endif

// assume: max reasonable allocations per frame
const int MaxAllocationsPerFrame=32;

MipInfo TextBankD3DT::UseMipmapLoaded(Texture *tex)
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
  TextureD3DT *texture=static_cast<TextureD3DT *>(tex);
  texture->LoadHeadersNV();
  return MipInfo(texture,texture->_levelLoaded);
}

/*!
\patch 1.03 Date 7/12/2001 by Ondra
- Fixed: crash when too low heap memory
\patch 5090 Date 11/24/2006 by Flyman
- Fixed: crash when out of videomemory
\patch 5090 Date 11/28/2006 by Flyman
- Changed: normal maps are not used when using "shading quality" "very low"
*/
MipInfo TextBankD3DT::UseMipmap(Texture *absTexture, int level, int top, int priority)
{
  if( !absTexture )
  {
    // with pixel shaders we need to override default black
    return MipInfo(NULL,0);
  }
  if (absTexture->LinkBidirWithFrameStore::IsInList())
  {
    // if the texture is cached, do not load it
    // cached texture can never be used on anything
    Assert(absTexture->RefCounter()<=0);
    return MipInfo(NULL,0);
  }
  #if _ENABLE_REPORT
    if(absTexture->RefCounter()<=0)
    {
      static int maxMsg = 30;
      if (maxMsg>0)
      {
        RptF(
          "Texture %s not loaded properly when used (RefCount<=0)",
          cc_cast(absTexture->GetName())
        );
        maxMsg--;
      }
    }
  #endif
  
  // level is the level we need
  // top is the level we would like to have
  // we are sure that texture is of type Texture
  // however this cast is potentially unsafe - take care
  TextureD3DT *texture=static_cast<TextureD3DT *>(absTexture);
  texture->LoadHeadersNV();

  Assert(texture->_inUse == 0);

  bool forceUse = level==top && level<INT_MAX || texture->_someDataNeeded;
  
  // if some level is explicitly required, we may not apply bias
  // level is necessary, top is wanted
  // as we are background loading, we can always load what is wanted
  level = top;

  // Determine whether we wish content of the texture to be loaded (f.i. we don't want to load normal maps
  // in low-end settings)
  // See label NNMLOAD
  bool loadContent;
  if (forceUse)
  {
    loadContent = true;
  }
  else
  {
    loadContent = _engine->TextureShouldBeUsed(texture);
  }

  // some info may be stored in the wanted slots
  texture->ProcessAoTWanted();
  
  // no bias here - it was already applied during FindMipmapLevel
  float levelExact = floatMin(level,texture->_mipmapWanted);

  // subtract a small value to make sure 0 is not handled as 1
  const float eps = 0.0001;
  saturateMin(level, toIntFloor(texture->_mipmapWanted-eps));

  if (level < texture->_largestUsed) level = texture->_largestUsed;

  #if _ENABLE_CHEATS
  if (!forceUse && DisableTextures)
  {
    texture->ReleaseMemory(true);
  }
  #endif

  // If the content is not to be loaded, we don't need the texture
  if (!loadContent)
  {
    texture->ReleaseMemory(true);
  }
  
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
  DoAssert(level >= 0);
  
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

  Assert(texture->CheckIntegrity());

  if (texture->_levelLoaded>level && !_engine->ResetNeeded())
  {
//     #if INTERESTED_IN_MIPMAPS
//       if (strstr(texture->GetName(),InterestedInMipmaps))
//       {
//         LogF("%s: Loading level %d (%.2f)",cc_cast(texture->GetName()),level,levelExact);
//       }
//     #endif
    
    // level is not present - you have to load something-> load top

    // if request is ready to be satisfied, load it
    // avoid overloading the request manager
    #if _ENABLE_CHEATS
    bool loaded = ( DisableTextures || StopLoadingTextures || (!loadContent) ) ? false : texture->RequestLevels(level,priority);
    #else
    bool loaded = (!loadContent) ? false : texture->RequestLevels(level,priority);
    #endif
    // if there are no data yet, load (quick fill) something
    if (loaded || forceUse )
    {
#if INTERESTED_IN_MIPMAPS
      if (strstr(texture->GetName(),InterestedInMipmaps))
      {
        LogF("%s: Loading level %d (%.2f)",cc_cast(texture->GetName()),level,levelExact);
      }
#endif
      for(;;)
      {
        DoAssert(level<texture->_nMipmaps);

        float mipImprovement = texture->_levelLoaded - level;

        // check if source data are already present in the file cache
        int ret=texture->LoadLevels(level, mipImprovement);
        if( ret>=0 )
        {
          break;
        }
        LogF("Out of VID: Try next level");
        level++;

        // Finish if we exceeded the smallLevel (at least _smallLevel we need to be loaded)
        if ((level > texture->_smallLevel) || (level >= texture->_nMipmaps))
        {
          level = texture->_nMipmaps; // Mark level it was not created
          break;
        }
      }
    }
    else
    {
      // return what is loaded
      level = texture->_levelLoaded;
      if (level>texture->_nMipmaps) level = texture->_nMipmaps;
    }
  }
  // if nobody marked the cache as used (that means nobody moved the item to the cache start), do it now
  texture->CacheUse(*this,level);
    
  Assert(texture->CheckIntegrity());
  
  // if no level is loaded, average color will be used
  return MipInfo(texture,level); // given mip-map loaded
}

/*
void TextBankD3DT::ReleaseMipmap()
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

void TextBankD3DT::MakePermanent(Texture *tex)
{
  // caution - we need to take care of permanent during Reset
  TextureD3DT *texNat = static_cast<TextureD3DT *>(tex);
  _permanent.Add(texNat);
  for (int i=0; i<texNat->_nMipmaps; i++)
  {
    DoAssert(texNat->_surfCache[i]==NULL);
  }
  texNat->_permanent = true;
  UseMipmap(tex,0,0);
}

void TextBankD3DT::InitDetailTextures()
{
  if( !_white )
  {
    _white=new TextureD3DT;
    ITextureSource *source = CreateColorTextureSource(HWhite);
    _white->Init(source);
    _white->_permanent = true;
  }

  if( !_random )
  {
    _random=new TextureD3DT;
    ITextureSource *source = CreateRandomTestTextureSource();
    _random->Init(source);
    _random->_permanent = true;
  }

  if( !_defNormMap )
  {
    _defNormMap=new TextureD3DT;
    Color neutral(0.5f,0.5f,1);
    ITextureSource *source = CreateColorTextureSource(neutral);
    _defNormMap->Init(source);
    _defNormMap->_permanent = true;
  }

  if( !_defDetailMap )
  {
    _defDetailMap=new TextureD3DT;
    Color neutral(0.5,0.5,0.5,0.5f);
    ITextureSource *source = CreateColorTextureSource(neutral);
    _defDetailMap->Init(source);
    _defDetailMap->_permanent = true;
  }

  if( !_defMacroMap )
  {
    _defMacroMap=new TextureD3DT;
    Color neutral(0.0,0.0,0.0,0.0f);
    ITextureSource *source = CreateColorTextureSource(neutral);
    _defMacroMap->Init(source);
    _defMacroMap->_permanent = true;
  }

  if( !_defSpecularMap )
  {
    _defSpecularMap=new TextureD3DT;
    Color neutral(1.0,0.0,0.0,1.0f);
    ITextureSource *source = CreateColorTextureSource(neutral);
    _defSpecularMap->Init(source);
    _defSpecularMap->_permanent = true;
  }

  UseMipmap(_white,0,0);
  UseMipmap(_random,0,0);
  UseMipmap(_defNormMap,0,0);
  UseMipmap(_defDetailMap,0,0);
  UseMipmap(_defMacroMap,0,0);
  UseMipmap(_defSpecularMap,0,0);

  for (int i=0; i<_permanent.Size(); i++)
  {
    UseMipmap(_permanent[i],0,0);
  }
}

void TextBankD3DT::DoneDetailTextures()
{
//  _white.Free();
//  _random.Free();
//  _defNormMap.Free();
//  _defDetailMap.Free();
//  _defMacroMap.Free();
//  _defSpecularMap.Free();
}

void TextBankD3DT::PreCreateVRAM( int reserve, bool randomOrder )
{
}

void TextBankD3DT::PreCreateSys()
{
}

bool TextBankD3DT::FlushTexture(const char *name)
{
  // check if the texture is cached
  RStringB fastName = name;
  bool ok = true;
  FlushCached(fastName);
  // another texture with the same name but different type may be loaded
  // check if the texture is loaded
  for (int i=0; i<_texture.Size(); i++)
  {
    TextureD3DT *tex = _texture[i];
    if (!tex) continue;
    if (fastName==tex->GetName())
    {
      // texture is loaded and someone is keeping the reference for it - cannot be flushed
      // but we may at least unload it from the memory
      tex->ReleaseMemory(true);
      ok = false;
    }
  }
  return ok;
}

Ref<Texture> TextBankD3DT::SuspendTexture(const char *name)
{
  // check if the texture is cached
  RStringB fastName = name;
  FlushCached(fastName);
  // another texture with the same name but different type may be loaded
  // check if the texture is loaded
  for (int i=0; i<_texture.Size(); i++)
  {
    TextureD3DT *tex = _texture[i];
    if (!tex) continue;
    if (fastName==tex->GetName())
    {
      // texture is loaded and someone is keeping the reference for it - cannot be flushed
      // but we may at least unload it from the memory
      tex->DoUnloadHeaders();
      return tex;
    }
  }
  return NULL;
  
}
void TextBankD3DT::ResumeTexture(Texture *tex)
{
  if (tex) tex->LoadHeaders();
}

const float TextBankD3DT::_textureMipBiasQuality[]=
{
  5.0f,
  3.7f,
  2.0f,
  1.5f,
  1.0f
};

/// We expect around 50 MB may be needed for fragmentation overhead

const int vbVRAMNeeded = 50*1024*1024;

/**
Assume there is VRAM + AGP. We want to avoid using all VRAM,
because some memory is needed for framebuffers + render targets.
In case there will be slight overflow, we will use AGP, but this is much slower
*/

const int TextBankD3DT::_textureLimit[]=
{
  /// 128+0 MB VRAM, 8 MB VRAM needed for FB + RT
  120*1024*1024,
  /// 128+32 MB VRAM, 16 MB VRAM needed for FB + RT
  144*1024*1024,
  /// 256+0 MB VRAM, 26 MB VRAM needed for FB + RT
  230*1024*1024,
  /// 256+128 MB VRAM, 32 MB VRAM needed for FB + RT
  352*1024*1024,
  /// 512+0 MB VRAM, 42 MB VRAM needed for FB + RT
  470*1024*1024
};

const int TextBankD3DT::_textureMaxSize[][Texture::NUsageType]=
{
  //Obj,Land,Cock,UI,Custom(Sky)
  {512,512,1024,2048,1024}, // very low
  {512,512,1024,2048,1024}, // low
  {1024,1024,2048,2048,2048}, // normal
  {4096,4096,4096,4096,4096}, // high
  {4096,4096,4096,4096,4096}, // very high
};

int TextBankD3DT::GetMaxTextureQuality() const
{
  return _maxTextureQuality;
}

void TextBankD3DT::SetTextureQuality(int value)
{
  saturate(value,-1,lenof(_textureLimit)-2);
  
  _textureQuality = value;
  _textureMipBiasMin = _textureMipBiasQuality[value+1];
  SetTextureMipBias(_textureMipBiasMin);
  _engine->SetLowEndQuality();
  
  _maxTextureMemory = _textureLimit[value+1];

  _freeTextureMemory = FreeTextureMemory();
  RecalculateTextureMemoryLimits();
  _engine->CheckLocalNonlocalTextureMemory();
  
  // resize all textures as needed
  for (int i=0; i<_texture.Size(); i++)
  {
    Texture *tex = _texture[i];
    if (tex) tex->AdjustMaxSize();
  }
  for (Texture *tex = _cache.First(); tex; tex=_cache.Next(tex))
  {
    tex->AdjustMaxSize();
  }

  // check if vertex buffers should be static based on VRAM/AGP stats
  
  int vramNeeded = _maxTextureMemory + vbVRAMNeeded;
  
  _engine->SetVBuffersStatic(_engine->GetLocalVramAmount() > vramNeeded);
}

int TextBankD3DT::MaxTextureSize(Texture::UsageType type)
{
  int index = _textureQuality+1;
  saturate(index,0,lenof(_textureMaxSize)-1);
  return _textureMaxSize[index][type];
}

void TextBankD3DT::FlushTextures(bool deep)
{
  Compact();
  if (deep)
  {
    // release all textures and vertex buffers
    // this prevents fragmentation building up
    ReserveMemory(INT_MAX,0);
  }
  // check current status of the local/nonlocal memory
  _engine->CheckLocalNonlocalTextureMemory();
}

void TextBankD3DT::CheckForChangedFiles()
{
  // if any source file has changed, reload it
  // TODO: scan cache as well
  for (int i=0; i<_texture.Size(); i++)
  {
    TextureD3DT *tex =_texture[i];
    if (!tex) continue;
    tex->CheckForChangedFile();
  }
}

void TextBankD3DT::FlushBank(QFBank *bank)
{
  for (int i=0; i<_texture.Size(); i++)
  {
    TextureD3DT *tex = _texture[i];
    if (!tex) continue;
    // check if texture is from given bank
    if (!bank->Contains(tex->GetName())) continue;
    _texture.Delete(i);
    i--;
  }
  // we need to flush the texture from the cache as well
  FlushBankFromCache(bank);
}

void TextBankD3DT::PreCreateSurfaces()
{
  //PreCreateVRAM(512*1024,true);
}

static int CompareTextureFileOrder(const LLink<TextureD3DT> *tl1, const LLink<TextureD3DT> *tl2)
{
  TextureD3DT *t1 = *tl1;
  TextureD3DT *t2 = *tl2;
  return QFBankQueryFunctions::FileOrder(t1->GetName(),t2->GetName());
}

#if _ENABLE_CHEATS
void DisplayTextureFileOrder(TextureD3DT *t1)
{
  const char *n1 = t1->GetName();
  QFBank *b1 = QFBankQueryFunctions::AutoBank(t1->GetName());
  if (!b1) return;
  int o1 = b1->GetFileOrder(n1+strlen(b1->GetPrefix()));
  LogF("texture %s, order %d",n1,o1);
}
#endif

/*!
\patch 1.78 Date 7/23/2002 by Ondra
- Fixed: Random crash during program startup.
*/

void TextBankD3DT::Preload()
{
  Compact();

  DWORD start = GlobalTickCount();
  // sort textures bank / by order in bank
  QSort(_texture.Data(),_texture.Size(),CompareTextureFileOrder);
  // start preloading all headers
  for (int i=0; i<_texture.Size(); i++)
  {
    TextureD3DT *tex = _texture[i];
    if (!tex) continue;
    tex->PreloadHeaders();
  }
  GFileServer->SubmitRequests();

  // load all headers
  for (int i=0; i<_texture.Size(); i++)
  {
    TextureD3DT *tex = _texture[i];
    if (!tex) continue;
    tex->LoadHeadersNV();
    //DisplayTextureFileOrder(tex);
    ProgressRefresh();
  }
  LogF("Preload %d textures - %d ms",_texture.Size(),GlobalTickCount()-start);
}

size_t TextBankD3DT::GetMaxVRAMAllocation() const
{
  return _limitAlocatedVRAM;
}


void TextBankD3DT::ResetMaxTextureQuality()
{
  // check how much texture we have and how much can we allocate
  int vramReported = FreeTextureMemoryReported()+_totalAllocated+_engine->UsedVRAM();
  // check if we can fit withing given limits
  // we know we can always fit into current quality
  // we can use additional local/nonlocal information as a hint
  
  int q;
  for (q=1; q<lenof(_textureLimit); q++)
  {
    int limit = _textureLimit[q];
    if (limit>vramReported && q>_textureQuality+1)
    {
      break;
    }
  }
  _maxTextureQuality = q-2;
  
}


int TextBankD3DT::FreeTextureMemory()
{
  int vramReported = FreeTextureMemoryReported();
  int vramAssumed = _maxTextureMemory - _totalAllocated - _engine->UsedVRAM();
  return intMin(vramReported,vramAssumed);
}

void TextBankD3DT::FreeTextureMemoryReportedByDDraw(int &local, int &nonlocal)
{
  local = nonlocal = 0;
}

int TextBankD3DT::FreeTextureMemoryReported()
{
  return 0;
  // Direct3D 10 NEEDS UPDATE 
//   PROFILE_SCOPE_EX(txRFM,tex);
//   ID3D10Device *dDraw=_engine->GetDirect3DDevice();
//   return dDraw->GetAvailableTextureMem();
}

#if _ENABLE_CHEATS
static RString FormatByteSizeNoZero(size_t size)
{
  if (size) return FormatByteSize(size);
  return RString();
}
RString TextBankD3DT::GetStat(int statId, RString &statVal, RString &statVal2)
{
  int id = 0;
  if (statId==id++)
  {
    statVal = FormatByteSizeNoZero(intMax(0,FreeTextureMemory()));
    statVal2 = FormatByteSizeNoZero(FreeTextureMemoryReported());
    return "Free VRAM Game/DX9" ;
  }
  else if (statId==id++)
  {
    int local,nonlocal;
    FreeTextureMemoryReportedByDDraw(local,nonlocal);
    statVal = FormatByteSizeNoZero(local);
    statVal2 = FormatByteSizeNoZero(nonlocal);
    return "DX7 Free VRAM/AGP";
  }
  else if (statId==id++)
  {
    // note: based on experiments:
    //   on nVidia dynamic buffers go into non-local
    //   on ATI dynamic buffers go into local
    // it is always safer to assume everything goes into local
    // and be proven wrong once remeasured
    int local = GEngineDD->GetFreeLocalVramAmount();
    int nonlocal = GEngineDD->GetFreeNonlocalVramAmount();
    if (local<0)
    {
      statVal = RString("-")+FormatByteSizeNoZero(-local);
    }
    else
    {
      statVal = FormatByteSizeNoZero(local);
    }
    statVal2 = FormatByteSizeNoZero(intMax(0,nonlocal));
    return "Est. free VRAM/AGP";
  }
  else if (statId==id++)
  {
    statVal = FormatByteSizeNoZero(_totalAllocated);
    statVal2 = FormatByteSizeNoZero(_totalAllocated+_engine->UsedVRAM());
    return "Alloc Tex/Tex+VBuf";
  }
  else if (statId==id++)
  {
    statVal = FormatByteSizeNoZero(_used.MemoryControlled());
    statVal2 = FormatByteSizeNoZero(_used.MemoryControlledRecent());
    return "VRAM Used/recent";
  }
  if (statId==id++)
  {
    statVal = FormatByteSizeNoZero(_freeAllocated);
    statVal2 = RString();
    return "Free";
  }
  else if (statId==id++)
  {
    statVal = FormatByteSizeNoZero(_totalSlack);
    statVal2 = RString();
    return "Slack";
  }
  else if (statId==id++)
  {
    statVal = FormatByteSizeNoZero(_stressTestTotalSize);
    statVal2 = RString();
    return _stressTestTotalSize>0 ? "Stress" : " ";
  }
  else if (statId==id++)
  {
    statVal = RString();
    statVal2 = RString();
    return " ";
  }
  if (statId==id++)
  {
    statVal = Format("%.3f",GetTextureMipBias());
    statVal2 = Format("%.3f",GetTextureMipBiasMin());
    return "Mip bias cur/min";
  }
  else if (statId==id++)
  {
    statVal = Format("T %g",VRAMDiscardMetrics());
    statVal2 = Format("V %g",_engine->VRAMDiscardMetrics());
    return "Age T/VB";
  }
  else if (statId==id++)
  {
    int size;
    statVal = Format("%d",VRAMDiscardCountAge(10,&size));
    statVal2 = FormatByteSize(size);
    return "T Age>10 #/mem";
  }
  else if (statId==id++)
  {
    int size;
    statVal = Format("%d",VRAMDiscardCountAge(50,&size));
    statVal2 = FormatByteSize(size);
    return "T Age>50 #/mem";
  }
  else if (statId==id++)
  {
    int size;
    statVal = Format("%d",_engine->VRAMDiscardCountAge(10,&size));
    statVal2 = FormatByteSize(size);
    return "V Age>10 #/mem";
  }
  else if (statId==id++)
  {
    int size;
    statVal = Format("%d",_engine->VRAMDiscardCountAge(50,&size));
    statVal2 = FormatByteSize(size);
    return "V Age>50 #/mem";
  }
  statVal = RString();
  statVal2 = RString();
  return RString();
}
#endif

void TextBankD3DT::RecalculateTextureMemoryLimits()
{
  _limitAlocatedVRAM = _freeTextureMemory + _totalAllocated + _engine->UsedVRAM();
  // if below certain limit, something very wrong has happened
  // do not degrade any more, let other systems handle it
  const int minTexVRAM = 32*1024*1024;
  if (_limitAlocatedVRAM<minTexVRAM) _limitAlocatedVRAM = minTexVRAM;
  // based on how many VRAM there is, decide how much we need for VB
  float wantedForVB = Interpolativ(
    _limitAlocatedVRAM,
    110*1024*1024,470*1024*1024,
    64*1024*1024,170*1024*1024
  );
  int neededForVB = toLargeInt(wantedForVB);
  _limitAlocatedTextures = _limitAlocatedVRAM - neededForVB - _reserveTextureMemory;
}

void TextBankD3DT::ClearTexStress()
{
  #if _ENABLE_CHEATS
  // release them all
  _stressTest.Clear();
  _stressTestTotalSize = 0;
  #endif
}

void TextBankD3DT::AddTexStress(int size, bool renderTarget)
{
  Fail("DX10 TODO");
  // Direct3D 10 NEEDS UPDATE 
//   #if _ENABLE_CHEATS
//   ComRef<ID3D10Texture2D> tex;
// 
//   
//   HRESULT err;
//   int pixelSize = 1;
//   if (!renderTarget)
//   {
//     err = _engine->GetDirect3DDevice()->CreateTexture(size, size, 1, 0, D3DFMT_DXT5, D3DPOOL_DEFAULT, tex.Init(), NULL);
//   }
//   else
//   {
//     pixelSize = 4;
//     err = _engine->GetDirect3DDevice()->CreateTexture(size, size, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, tex.Init(), NULL);
//   }
//   if (SUCCEEDED(err))
//   {
//     _stressTest.Add(StressInfo(tex,size*size*pixelSize));
//     _stressTestTotalSize += size*size*pixelSize;
//     //LogF("Total stress: %s",cc_cast(FormatByteSize(_stressTestTotalSize)));
//   }
//   else
//   {
//     _engine->DDError9("Cannot create stress surface",err);
//   }
//   #endif
}

void TextBankD3DT::SetStressMode(int mode)
{
  #if _ENABLE_CHEATS
  _texStressMode = mode;
  #endif
}

void TextBankD3DT::StressTestFrame()
{
  // Direct3D 10 NEEDS UPDATE 
//   #if _ENABLE_CHEATS
//   if (_texStressMode==0) return;
//   // if stress mode positive, create a few textures
//   // if stress mode negative, destroy a few textures
//   int countCreat = _texStressMode>0 ? _texStressMode : -_texStressMode/10;
//   int countDestr = _texStressMode<0 ? -_texStressMode : _texStressMode/10;
//   saturateMax(countCreat,4);
//   saturateMax(countDestr,4);
//   for (int i=0; i<countDestr && _stressTest.Size()>0; i++)
//   {
//     // destroy a random one
//     int index = rand()%_stressTest.Size();
//     _stressTestTotalSize -= _stressTest[index]._size;
//     _stressTest.Delete(index);
//   }
//   for (int i=0; i<countCreat; i++)
//   {
//     // create a few random sized textures
//     int sizeLogX = rand()%9;
//     int sizeLogY = rand()%9;
//     int sizeX = 4<<sizeLogX;
//     int sizeY = 4<<sizeLogY;
//     
//     ComRef<ID3D10Texture2D> tex;
// 
//     HRESULT err = _engine->GetDirect3DDevice()->CreateTexture(sizeX, sizeY, 1, 0, D3DFMT_DXT5, D3DPOOL_DEFAULT, tex.Init(), NULL);
//     if (SUCCEEDED(err))
//     {
//       _stressTest.Add(StressInfo(tex,sizeX*sizeY));
//       _stressTestTotalSize += sizeX*sizeY;
//     }
//     else
//     {
//       _engine->DDError9("Cannot create stress surface",err);
//     }
//   }
//   #endif
}



void TextBankD3DT::ReloadAll()
{
  Log("Reload all textures.");
  // make sure all texture state is tested
}

#endif //!_DISABLE_GUI

