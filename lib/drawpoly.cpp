// Horizont Shader - basic 2D/3D raster operations
// (C) 1997, SUMA
#include "wpch.hpp"

#include "Shape/poly.hpp"
#include "tlVertex.hpp"
#include "engine.hpp"
#include "global.hpp"
#include "Shape/material.hpp"
#include "keyInput.hpp"

// TODO: remove some includes
#include "scene.hpp"
#include "camera.hpp"

#if _DEBUG
  #define INTERESTED_IN_MIPMAPS 1
#endif

#if INTERESTED_IN_MIPMAPS
  static char InterestedInMipmaps[256]="";
  static float InterestedInMipLimit = -20;
#endif

#include <El/Common/perfProf.hpp>

/// find a mipmap level needed for given texture and rendering distance
/**
@param areaOTex/z2 is expected to be screen space area of one texel
*/
float Texture::FindMipmapLevel(float areaOTexPerZ2)
{
  // note: we cannot saturate z2 here, as it often contains UV scale 
  //saturateMax(z2,Square(0.1f));
  // estimate screen area from z and area

  // apply perspective on position and size
  // one screen pixel contains textArea/scrArea _texture pixels (average)
  int texHW=AArea();
  float invTexHW = 1.0f/texHW;

  // calculate continuous result
  // what we calculate can be viewed as -log(areaOTexPerZ2/hw)/log(4)
  const float negInvLog4 = -1/1.3862943611f; // log(4)
  float logLevel = areaOTexPerZ2*invTexHW>1e-10f ? log(areaOTexPerZ2*invTexHW)*negInvLog4 : 20;
  return logLevel+GEngine->TextBank()->GetTextureMipBias();
}

/**
Lock free MT safe implemenation of tracking maximum of scrAreaOTex/z2
*/
static inline bool TrackAOTPerZ2(float &wantedAOTPerZ2, float scrAreaOTex, float z2)
{
#ifdef _WIN32
  // atomic test + update of _wanted
  for(;;)
  {
    // note: using division would bring natural read atomicity.
    // SSE division latency is about 20-30 cycles, which is faster than _InterlockedCompareExchange64
    
    // make sure we read the old data only once (atomic)
    float oldWantedAOTPerZ2 = const_cast<volatile float &>(wantedAOTPerZ2);
    int oldWantedAOTPerZ2Raw = reinterpret_cast<int &>(oldWantedAOTPerZ2);
    // when somebody has already requested a finer mipmap, there is no need for any update - proceed
    // note: scrAreaOTex may be infinite here - this is valid scenario
    //if (scrAreaOTex/z2<=oldWantedAOTPerZ2) goto NotSet;
    // avoid division (both z2 and _wantedZ2 is non-negative)
    if (scrAreaOTex<=oldWantedAOTPerZ2*z2) return false;
    
    float newWantedAOTPerZ2 = z2>0 ? scrAreaOTex/z2 : FLT_MAX;
    int newWantedAOTPerZ2Raw = reinterpret_cast<int &>(newWantedAOTPerZ2);
    if (oldWantedAOTPerZ2Raw==_InterlockedCompareExchange((long *)&wantedAOTPerZ2,newWantedAOTPerZ2Raw,oldWantedAOTPerZ2Raw))
    {
      return true;
    }
  }
#else
  return false;
#endif
}


void Texture::PreloadMipmapLevelScr(float z2, float scrAreaOTex, bool someDataWanted)
{
  _someDataWanted |= someDataWanted;
  
  if (TrackAOTPerZ2(_preloadAOTPerZ2,scrAreaOTex,z2))
  {
    // when we made a request, we need to add ourselves into the list for processing
    if (AddIntoSList())
    {
      AbstractTextBank *bank = GEngine->TextBank();
      DoAssert(bank->_loadRequested.CheckIntegrity());
      bank->_loadRequested.Add(this);
      DoAssert(bank->_loadRequested.CheckIntegrity());
    }
  }
}

void Texture::PrepareMipmapLevelScr(float z2, float scrAreaOTex, bool someDataWanted)
{
  // note: we cannot saturate z2 here, as it often contains UV scale 
  // camera=*GScene->GetCamera();
  // scrAreaOTex = areaOTex*camera.GetAOTFactor();
  
  // we want to find maximum of scrAreaOTex/z2
  _someDataWanted |= someDataWanted;
  
  if (TrackAOTPerZ2(_wantedAOTPerZ2,scrAreaOTex,z2))
  {
    #if INTERESTED_IN_MIPMAPS
      if (*InterestedInMipmaps && strstr(GetName(),InterestedInMipmaps))
      {
        float level = z2>0 ? FindMipmapLevel(_wantedAOTPerZ2) : 0;
        LogF("%s: level %.2f, Set AOT=%g/z2=%g",cc_cast(GetName()),level,scrAreaOTex,z2);
      }
    #endif
    #if 0
    static char InterestedInAOT[256]="ca\\rocks2\\data\\r2_boulder1_co.paa";
    static float LimitAOT = 100;
    if (_wantedAOTPerZ2>LimitAOT && !strcmp(_name,InterestedInAOT))
    {
      __asm nop;
    }
    #endif
    // when we made a request, we need to add ourselves into the list for processing
    if (AddIntoSList())
    {
      AbstractTextBank *bank = GEngine->TextBank();
      DoAssert(bank->_loadRequested.CheckIntegrity());
      bank->_loadRequested.Add(this);
      DoAssert(bank->_loadRequested.CheckIntegrity());
    }
  }

  #if _ENABLE_REPORT
    static bool track = false;
    static float trackLimit = 50;
    if (track && z2>0 && z2<Square(trackLimit))
    {
      // FLT_MAX is used to indicate best possible level is loaded
      AbstractTextBank *bank = GEngine->TextBank();
      float biasAot = bank->GetTextureMipBiasAOTCoef();
      
      // decide based on only AOT/z2 only
      if (!(_loadedTexDetail>=_maxTexDetail || _loadedTexDetail*z2 >= scrAreaOTex*biasAot))
      {
        LogF("Not ready %s (%g)",cc_cast(GetName()),_loadedTexDetail);
        if (!(_loadedTexDetail>=_maxTexDetail || _loadedTexDetail*z2 >= scrAreaOTex*biasAot*0.25f))
        {
          LogF("Not ready - serious %s (%g)",cc_cast(GetName()),_loadedTexDetail);
        }
      }
    }
  #endif
}

/**
@param quality is there is any deficit, quantify how much will the quality suffer.
 Linear area is used for quantification (one mipmap worse than required is returned as 0.25)
 When 1 is returned, quality is good enough.
 The argument is cumulative - this function only lowers it.
*/

bool Texture::PreloadAndCheckMipmapLevel(float z2, float areaOTex, bool someDataWanted, float *quality)
{
  const Camera &camera=*GScene->GetCamera();
  float scrAreaOTex = areaOTex*camera.GetAOTFactor();
  // note: we cannot saturate z2 here, as it often contains UV scale 
  //saturateMax(z2,Square(0.1f));
  //if (!quality)
  {
    // when checking quality, the only intention is to check which LOD should be used
    // in such case we do not want to load anything at all, only check what is already loaded
    PreloadMipmapLevelScr(z2,scrAreaOTex,someDataWanted);
  }
  
  AbstractTextBank *bank = GEngine->TextBank();
  
  // FLT_MAX is used to indicate best possible level is loaded
  float biasAot = bank->GetTextureMipBiasAOTCoef();
  
  if (quality)
  {
    // avoid division as much as possible here
    //if (*quality>_loadedTexDetail*z2 / scrAreaOTex*biasAot)
    // depending on texture type, best mipmap handling may differ
    //TextureType
    if (*quality*scrAreaOTex*biasAot>_loadedTexDetail*z2 )
    {
      *quality = _loadedTexDetail*z2 / (scrAreaOTex*biasAot);
    }
  }
  // decide based on only AOT/z2 only
  if (_loadedTexDetail>=_maxTexDetail || _loadedTexDetail*z2 >= scrAreaOTex*biasAot)
  {
    return true;
  }
  #if _ENABLE_REPORT
  if (z2>0)
  {
    static bool track = false;
    if (track)
    {
      LogF("%s not loaded: %g (z=%g)",cc_cast(GetName()),_loadedTexDetail*z2 / (scrAreaOTex*biasAot),sqrt(z2));
    }
  }
  #endif
  return false;
}

bool Texture::PrepareMipmap0(bool dataNeeded)
{
  // TODO:MC: atomic update needed
  if (dataNeeded) _someDataNeeded = true;
  if (_mipmapWanted>0)
  {
    _mipmapWanted = 0;
  }
  if (AddIntoSList())
  {
    AbstractTextBank *bank = GEngine->TextBank();
    DoAssert(bank->_loadRequested.CheckIntegrity());
    bank->_loadRequested.Add(this);
    DoAssert(bank->_loadRequested.CheckIntegrity());
  }
  // check if best data are already loaded
  return _loadedTexDetail>=_maxTexDetail;
}

void Texture::ProcessAoTWanted()
{
  // avoid infinity here - we cannot provide better than what we have anyway
  const float maxDetailedRequest = -100.0f;
  if (_preloadAOTPerZ2>0)
  {
    float level = FindMipmapLevel(_preloadAOTPerZ2);
    if (level<maxDetailedRequest) level=maxDetailedRequest;
	  saturateMin(_mipmapPreload,level);
    _preloadAOTPerZ2 = 0;
  }
  if (_wantedAOTPerZ2>0)
  {
    float level = FindMipmapLevel(_wantedAOTPerZ2);
    #if INTERESTED_IN_MIPMAPS
      if (*InterestedInMipmaps && strstr(GetName(),InterestedInMipmaps) && level<InterestedInMipLimit)
      {
        LogF(
          "PreloadTexture %s - level %g, AOT/z2",
          cc_cast(GetName()),level,_wantedAOTPerZ2
        );
        // debugging opportunity
        FindMipmapLevel(_wantedAOTPerZ2);
      }
    #endif
    if (level<maxDetailedRequest) level=maxDetailedRequest;
	  saturateMin(_mipmapWanted,level);
    // we no longer need any AOT processing
    _wantedAOTPerZ2 = 0;
  }
}


bool Texture::PreloadTexture(float z2, float areaOTex, bool someDataWanted, float *quality)
{
  if (!HeadersReady())
  {
    if (quality) *quality = 0;
    return false;
  }
  // no explicit bias  - if any bias is desired, it can be implemented on higher level
  if (z2>0)
  {
    // no data needed, some data wanted
    return PreloadAndCheckMipmapLevel(z2,areaOTex,someDataWanted,quality);
  }
  else
  {
    // if preloading for distance zero, preload finest mipmap
    // no data needed, some data wanted
    if (PrepareMipmap0(false))
    {
      return true;
    }
    if (quality)
    {
      *quality = 0;
    }
    return false;
  }
}


void PolyProperties::Prepare(int cb, Texture *texture, const TexMaterial *mat, int special )
{
  Assert( texture!=(Texture *)-1 );
  AbstractTextBank *bank=GEngine->TextBank();
  
  // some triangles have mipmap level predefined (e.g. landscape)
  int level=INT_MAX,mipTop=INT_MAX;
  if( special&BestMipmap ) level=mipTop=0;

  MipInfo mip=bank->UseMipmap(texture,level,mipTop);

  // check current texture state

  GEngine->PrepareTriangle(cb,mip,mat,special);
}

MipInfo PolyProperties::LoadTextureMipmaps(int spec) const
{
  Assert( _texture!=(Texture *)-1 );
  AbstractTextBank *bank=GEngine->TextBank();
  if( (_special&BestMipmap)==0 )
  {
    return bank->UseMipmapLoaded(_texture);
  }
  else
  {
    MipInfo mip = bank->UseMipmap(_texture,0,0);
    if (_surfMat)
    {
      // load mipmaps for material as well
      for (int i = 0; i < _surfMat->_stageCount+1; i++)
      {
        Texture *matTex = _surfMat->_stage[i].GetTextureForDraw();
        if (matTex)
        bank->UseMipmap(matTex,0,0);
      }
    }
    return mip;
  }
}

void PolyProperties::PrepareTL(
  int cb, const TLMaterial &mat, int spec, const EngineShapeProperties &prop, const DrawParameters &dp, bool UVchanged, float offsetU, float offsetV
) const
{

  MipInfo mip = LoadTextureMipmaps(spec);

  // Force alpha and disable Z write if craters drawing
  int orFlags = (dp._cratersCount > 0) ? IsAlpha | NoZWrite : 0;

  if (_surfMat)
  {
    // modulate material by material given in section
    TLMaterial matmod;
    _surfMat->Combine(matmod,mat);
    TexMaterialLODInfo matInfo(_surfMat, UVchanged,offsetU,offsetV,dp._defTI);
    int useSpec = _special|(spec&NoBackfaceCull);
    GEngine->PrepareTriangleTL(cb,this, mip,matInfo,useSpec|orFlags,matmod,spec,prop);
  }
  else
  {
    // set material given by model material supplier
    TexMaterialLODInfo matInfo(_surfMat, UVchanged,offsetU,offsetV,dp._defTI);
    GEngine->PrepareTriangleTL(cb,this, mip,matInfo,_special|orFlags,mat,spec,prop);
  }

}
