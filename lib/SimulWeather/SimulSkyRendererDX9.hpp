#ifdef _MSC_VER
#pragma once
#endif

#ifndef _ARMA3_SIMULSKYRENDERERDX9_HPP
#define _ARMA3_SIMULSKYRENDERERDX9_HPP

#if _ARMA3_SIMUL_WEATHER

#include "SimulSkyRendererBase.hpp"
#include "SimulCommonDX9.hpp"

class EngineDD9;

/** DX9 simul weather sky renderer 
Now we use sky renderer only for generating textures used for cloud fading, not for actual sky rendering
(we still use old Arma skybox). That's why we don't actually implement rendering functions.
*/
class SimulSkyRendererDX9: public SimulSkyRendererBase
{
public:
  /**
  \param engine pointer to rendering engine
  \param renderSky if we want to render the sky (otherwise old Arma skybox is used) 
  \param useFadeTextures if we want to use fade textures (used for cloud distance fading)
  \param useColourSky if we want to use coulour sky instead of normal sky
  */
  SimulSkyRendererDX9(EngineDD9* engine, bool renderSky = false, bool useFadeTextures = true, bool useColourSky = false);
  virtual ~SimulSkyRendererDX9();

  //@{ Implementation of SkyTexturesCallback
  virtual void FillSkyTexture(int alt_index,int texture_index,int texel_index,int num_texels,const float *float4_array);
  virtual void FillFadeTexturesSequentially(int texture_index,int texel_index,int num_texels,
    const float *loss_float4_array,
    const float *inscatter_float4_array);
  virtual void FillSunlightTexture(int texture_index,int texel_index,int num_texels,const float *float4_array);
  virtual void CycleTexturesForward();
  //@}

  /// gets textures used for distance fading
  void GetLossAndInscatterTextures(Ref<TextureSysGPUDX9>& l1,Ref<TextureSysGPUDX9>&l2,
    Ref<TextureSysGPUDX9>&i1,Ref<TextureSysGPUDX9>&i2);


private:
  //@{ Implementation of SimulSkyRendererBase
  /// initializes renderer - creates all resources,..., returns if succeeded
  virtual bool InitRenderer();
  /// deinitializes renderer - destroys all resources,...
  virtual void DeInitRenderer();
  /// creates textures used for fading
  virtual void CreateFadeTextures();
  /// creates textures used for sky rendering
  virtual void CreateSkyTextures();
  /// creates sunlight textures
  virtual void CreateSunlightTextures();
  //@}

  /// pool where the textures are created
  static const D3DPOOL TexturePool = D3DPOOL_DEFAULT;
  /// usage of textures
  static const DWORD TextureUsage = 0;

  /// format of all textures
  D3DFORMAT _textureFormat;

  // Three sky textures - 2 to interpolate and one to fill
  Ref<TextureSysGPUDX9> _skyTextures[TextureCount];
  // Three sunlight textures.
  Ref<TextureSysGPUDX9> _sunlightTextures[TextureCount];

  /// textures used for distance fading
  Ref<TextureSysGPUDX9> _lossTextures[TextureCount];
  Ref<TextureSysGPUDX9> _inscatterTextures[TextureCount]; 

  /// pointer to engine
  EngineDD9 *_engine;

};


#endif //_ARMA3_SIMUL_WEATHER
#endif //_ARMA3_SIMULSKYRENDERERDX9_HPP