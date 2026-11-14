#pragma once
#ifndef DAYZ_SIMUL_SKY_RENDERER_DX11_HPP
#define DAYZ_SIMUL_SKY_RENDERER_DX11_HPP

#if _ARMA3_SIMUL_WEATHER

#include "SimulSkyRendererBase.hpp"
#include "SimulCommonDX11.hpp"

// D3D11 implementation of the Simul sky data provider. As in the legacy
// renderer, the game's established skybox remains responsible for drawing
// the visible sky; this class builds the volumetric fade fields consumed by
// the cloud renderer.
class SimulSkyRendererDX11 : public SimulSkyRendererBase
{
public:
  SimulSkyRendererDX11(Dx11DeviceAccess *engine, bool renderSky = false,
    bool useFadeTextures = true, bool useColourSky = false);
  virtual ~SimulSkyRendererDX11();

  virtual void FillSkyTexture(int altIndex, int textureIndex, int texelIndex,
    int numTexels, const float *float4Array);
  virtual void FillFadeTexturesSequentially(int textureIndex, int texelIndex,
    int numTexels, const float *lossFloat4Array,
    const float *inscatterFloat4Array);
  virtual void FillSunlightTexture(int textureIndex, int texelIndex,
    int numTexels, const float *float4Array);
  virtual void CycleTexturesForward();

  void GetLossAndInscatterTextures(Ref<TextureSysGPUDX11> &loss1,
    Ref<TextureSysGPUDX11> &loss2, Ref<TextureSysGPUDX11> &inscatter1,
    Ref<TextureSysGPUDX11> &inscatter2);

private:
  virtual bool InitRenderer();
  virtual void DeInitRenderer();
  virtual void CreateFadeTextures();
  virtual void CreateSkyTextures();
  virtual void CreateSunlightTextures();

  bool WriteFloat4Range(Ref<TextureSysGPUDX11> texture, int texelIndex,
    int numTexels, const float *values);

  Dx11DeviceAccess *_engine;
  DXGI_FORMAT _textureFormat;
  int _bytesPerTexel;
  Ref<TextureSysGPUDX11> _skyTextures[TextureCount];
  Ref<TextureSysGPUDX11> _sunlightTextures[TextureCount];
  Ref<TextureSysGPUDX11> _lossTextures[TextureCount];
  Ref<TextureSysGPUDX11> _inscatterTextures[TextureCount];
};

#endif
#endif
