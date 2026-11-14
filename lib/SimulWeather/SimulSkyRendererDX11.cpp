#include "../wpch.hpp"

#if _ARMA3_SIMUL_WEATHER

#include "SimulSkyRendererDX11.hpp"

SimulSkyRendererDX11::SimulSkyRendererDX11(Dx11DeviceAccess *engine,
  bool renderSky, bool useFadeTextures, bool useColourSky)
  : SimulSkyRendererBase(renderSky, useFadeTextures, useColourSky),
    _engine(engine), _textureFormat(DXGI_FORMAT_UNKNOWN), _bytesPerTexel(0)
{
}

SimulSkyRendererDX11::~SimulSkyRendererDX11()
{
}

bool SimulSkyRendererDX11::InitRenderer()
{
  if (!_engine || !_engine->D3D11Device()) return false;
  const DXGI_FORMAT formats[] = {
    DXGI_FORMAT_R16G16B16A16_FLOAT,
    DXGI_FORMAT_R32G32B32A32_FLOAT
  };
  for (int i = 0; i < lenof(formats); ++i)
  {
    UINT support = 0;
    HRESULT hr = _engine->D3D11Device()->CheckFormatSupport(formats[i],
      &support);
    if (SUCCEEDED(hr) &&
        (support & D3D11_FORMAT_SUPPORT_TEXTURE3D) &&
        (support & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE))
    {
      _textureFormat = formats[i];
      _bytesPerTexel = i == 0 ? 8 : 16;
      RptF("DX11 SimulWeather sky volumes use format %u", (unsigned)formats[i]);
      return true;
    }
  }
  RptF("DX11 SimulWeather: no filterable Texture3D float format is supported");
  return false;
}

void SimulSkyRendererDX11::DeInitRenderer()
{
  for (int i = 0; i < TextureCount; ++i)
  {
    _lossTextures[i].Free();
    _inscatterTextures[i].Free();
    _skyTextures[i].Free();
    _sunlightTextures[i].Free();
  }
  _textureFormat = DXGI_FORMAT_UNKNOWN;
  _bytesPerTexel = 0;
}

void SimulSkyRendererDX11::FillSkyTexture(int, int, int, int, const float *)
{
  // The current SimulWeather configuration uses the game's skybox and only
  // requests generated fade volumes from this provider.
}

bool SimulSkyRendererDX11::WriteFloat4Range(Ref<TextureSysGPUDX11> texture,
  int texelIndex, int numTexels, const float *values)
{
  if (!texture || !values || texelIndex < 0 || numTexels <= 0 ||
      !_bytesPerTexel)
    return false;
  Ref<TextureSysGPUDX11::LockedData> locked =
    TextureSysGPUDX11::GetLockedData(texture);
  if (!locked || !locked->CheckSize() || !locked->GetData()) return false;

  const size_t totalTexels = (size_t)texture->Width() * texture->Height() *
    texture->Depth();
  if ((size_t)texelIndex > totalTexels ||
      (size_t)numTexels > totalTexels - (size_t)texelIndex)
    return false;

  if (_textureFormat == DXGI_FORMAT_R16G16B16A16_FLOAT)
  {
    short *dst = (short *)locked->GetData() + (size_t)texelIndex * 4;
    for (int i = 0; i < numTexels * 4; ++i)
      dst[i] = simul::sky::TextureGenerator::ToFloat16(values[i]);
  }
  else if (_textureFormat == DXGI_FORMAT_R32G32B32A32_FLOAT)
  {
    float *dst = (float *)locked->GetData() + (size_t)texelIndex * 4;
    memcpy(dst, values, (size_t)numTexels * 4 * sizeof(float));
  }
  else return false;
  return true;
}

void SimulSkyRendererDX11::FillFadeTexturesSequentially(int textureIndex,
  int texelIndex, int numTexels, const float *lossValues,
  const float *inscatterValues)
{
  if (!GetInitialized() || !GetUseFadeTextures()) return;
  if (textureIndex < 0 || textureIndex >= TextureCount) return;
  if (!WriteFloat4Range(_lossTextures[textureIndex], texelIndex, numTexels,
        lossValues))
    RptF("DX11 SimulWeather: failed writing loss volume %d", textureIndex);
  if (!WriteFloat4Range(_inscatterTextures[textureIndex], texelIndex,
        numTexels, inscatterValues))
    RptF("DX11 SimulWeather: failed writing inscatter volume %d", textureIndex);
}

void SimulSkyRendererDX11::FillSunlightTexture(int, int, int, const float *)
{
  // Sky/sunlight volume generation is not requested while renderSky is false.
}

void SimulSkyRendererDX11::CycleTexturesForward()
{
  Swap(_skyTextures[0], _skyTextures[1]);
  Swap(_skyTextures[1], _skyTextures[2]);
  Swap(_lossTextures[0], _lossTextures[1]);
  Swap(_lossTextures[1], _lossTextures[2]);
  Swap(_inscatterTextures[0], _inscatterTextures[1]);
  Swap(_inscatterTextures[1], _inscatterTextures[2]);
  Swap(_sunlightTextures[0], _sunlightTextures[1]);
  Swap(_sunlightTextures[1], _sunlightTextures[2]);
}

void SimulSkyRendererDX11::CreateFadeTextures()
{
  if (!GetInitialized() || !GetUseFadeTextures() || !_engine ||
      fadeTexWidth <= 0 || fadeTexHeight <= 0 || numAltitudes <= 0)
    return;

  for (int i = 0; i < TextureCount; ++i)
  {
    _lossTextures[i] = new TextureSysGPUDX11(_engine, _bytesPerTexel);
    _inscatterTextures[i] = new TextureSysGPUDX11(_engine, _bytesPerTexel);
    HRESULT hr = _lossTextures[i]->CreateTextures(fadeTexWidth, fadeTexHeight,
      numAltitudes, _textureFormat);
    if (SUCCEEDED(hr))
      hr = _inscatterTextures[i]->CreateTextures(fadeTexWidth, fadeTexHeight,
        numAltitudes, _textureFormat);
    if (FAILED(hr))
    {
      RptF("DX11 SimulWeather: creating fade volumes failed (0x%08x)",
        (unsigned)hr);
      _lossTextures[i].Free();
      _inscatterTextures[i].Free();
      return;
    }
  }
}

void SimulSkyRendererDX11::CreateSkyTextures()
{
  if (!GetInitialized() || !GetRenderSky()) return;
  // Sky color volumes are not requested by the legacy game configuration.
}

void SimulSkyRendererDX11::CreateSunlightTextures()
{
  if (!GetInitialized() || !GetRenderSky()) return;
  // Sunlight volumes are not requested by the legacy game configuration.
}

void SimulSkyRendererDX11::GetLossAndInscatterTextures(
  Ref<TextureSysGPUDX11> &loss1, Ref<TextureSysGPUDX11> &loss2,
  Ref<TextureSysGPUDX11> &inscatter1, Ref<TextureSysGPUDX11> &inscatter2)
{
  loss1 = _lossTextures[0];
  loss2 = _lossTextures[1];
  inscatter1 = _inscatterTextures[0];
  inscatter2 = _inscatterTextures[1];
}

#endif
