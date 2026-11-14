#include "../wpch.hpp"

#if _ARMA3_SIMUL_WEATHER

#include "SimulSkyRendererDX9.hpp"
#include "../d3d9/engdd9.hpp"

SimulSkyRendererDX9::SimulSkyRendererDX9(EngineDD9* engine, bool renderSky, bool useFadeTextures, bool useColourSky)
:SimulSkyRendererBase(renderSky, useFadeTextures, useColourSky), _engine(engine)
, _textureFormat(D3DFMT_UNKNOWN)
{
  Assert(_engine);
}

SimulSkyRendererDX9::~SimulSkyRendererDX9()
{
}

bool SimulSkyRendererDX9::InitRenderer()
{
  Assert(_engine);
  if (!_engine)
    return false;

  /// check supported texture format
  D3DFORMAT formats[] = 
  {
    D3DFMT_A16B16G16R16F, 
    D3DFMT_A32B32G32R32F
  };

  _textureFormat = D3DFMT_UNKNOWN;
  for (int i=0; i< lenof(formats); ++i)
  {
    HRESULT err = _engine->CheckDeviceFormat(TextureUsage, D3DRTYPE_VOLUMETEXTURE, formats[i]);
    if (SUCCEEDED(err))
    {
      _textureFormat = formats[i];
      break;
    }
  }

  if (D3DFMT_UNKNOWN == _textureFormat)
  {
    // no appropriate texture format found
    Fail("No appropriate texture format for Simul Weather sky renderer.")
    return false;
  }  

  LogF("SimulWeather - SkyRenderer initialized.");
  return true;
}

void SimulSkyRendererDX9::DeInitRenderer()
{
  for(int i=0 ; i<TextureCount ; ++i)
  {
    _lossTextures[i].Free();
    _inscatterTextures[i].Free();
    _skyTextures[i].Free();
    _sunlightTextures[i].Free();
  }
  _textureFormat = D3DFMT_UNKNOWN;
  LogF("SimulWeather - SkyRenderer deinitialized.");
}

void SimulSkyRendererDX9::FillSkyTexture(int alt_index,int texture_index,int texel_index,int num_texels,const float *float4_array)
{
  if (!GetInitialized())
    return;
  if (!GetRenderSky())
    return;

  Assert(_engine);
  if (!_engine)
    return;

  // SIMUL WEATHER TODO
}

void SimulSkyRendererDX9::FillFadeTexturesSequentially(int texture_index,int texel_index,int num_texels,
                                  const float *loss_float4_array,
                                  const float *inscatter_float4_array)
{
  if (!GetInitialized())
    return;
  if (!GetUseFadeTextures())
    return;

  Assert(_engine);
  if (!_engine)
    return;

  Assert(texture_index >= 0 && texture_index < TextureCount);
  if (texture_index < 0 || texture_index >= TextureCount)
    return;

  for (int i=0; i<=1; ++i)
  {
    // texture we want to update
    Ref<TextureSysGPUDX9> tex = NULL;
    // array with update values
    const float *updateArray = NULL;

    switch (i)
    {
    case 0:
      tex = _lossTextures[texture_index];
      updateArray = loss_float4_array;
      break;
    case 1:
      tex = _inscatterTextures[texture_index];
      updateArray = inscatter_float4_array;
      break;
    default:
      Fail("Invalid index!");
      break;
    }
    if (!tex|| !updateArray)
      continue; //no texture to update

    Ref<TextureSysGPUDX9::LockedData> lockedData = TextureSysGPUDX9::GetLockedData(tex);
    Assert(lockedData);
    if (!lockedData)
      return;

    void *tex_ptr=lockedData->GetData();
    Assert(tex_ptr);
    if (!tex_ptr)
      continue;

    Assert(lockedData->CheckSize());
    if (!lockedData->CheckSize())
      return;

    // fill texture
    if(_textureFormat==D3DFMT_A16B16G16R16F)
    {
      // Convert the array of floats into float16 values for the texture.
      short *short_ptr=(short *)(tex_ptr);

      int texelSizeInBytes = 8;
      if (lockedData->IsPitchCorrespondingSize(texelSizeInBytes))
      {
        // perform fast copy
        short_ptr+=4*texel_index;
        for(int i=0;i<num_texels*4;i++)
          *short_ptr++=simul::sky::TextureGenerator::ToFloat16(*updateArray++);
      }
      else
      {
        for(int i=0;i<num_texels;i++)
        {
          int offset = lockedData->GetMappedTextureOffset(texel_index+i, texelSizeInBytes) / sizeof(short);
          short_ptr[offset] = simul::sky::TextureGenerator::ToFloat16(*updateArray++);
          short_ptr[offset+1] = simul::sky::TextureGenerator::ToFloat16(*updateArray++);
          short_ptr[offset+2] = simul::sky::TextureGenerator::ToFloat16(*updateArray++);
          short_ptr[offset+3] = simul::sky::TextureGenerator::ToFloat16(*updateArray++);
        }
      }
    }
    else
    {
      // Copy the array of floats into the texture.
      float *float_ptr=(float *)(tex_ptr);

      int texelSizeInBytes = 16;
      if (lockedData->IsPitchCorrespondingSize(texelSizeInBytes))
      {
        // perform fast copy
        float_ptr+=4*texel_index;
        for(int i=0;i<num_texels*4;i++)
          *float_ptr++=(*updateArray++);
      }
      else
      {
        for(int i=0;i<num_texels;i++)
        {
          int offset = lockedData->GetMappedTextureOffset(texel_index+i, texelSizeInBytes) / sizeof(float);
          float_ptr[offset] = simul::sky::TextureGenerator::ToFloat16(*updateArray++);
          float_ptr[offset+1] = simul::sky::TextureGenerator::ToFloat16(*updateArray++);
          float_ptr[offset+2] = simul::sky::TextureGenerator::ToFloat16(*updateArray++);
          float_ptr[offset+3] = simul::sky::TextureGenerator::ToFloat16(*updateArray++);
        }
      }
    }

    // no need to unlock and update (will be done when the texture is actually used)
      }

#if SIMUL_WEATHER_LOG_TEXTURE_FILLS
  LogF("SimulWeather - FillFadeTexturesSequentially done, texture index: %d.", texture_index);
#endif
}

void SimulSkyRendererDX9::FillSunlightTexture(int texture_index,int texel_index,int num_texels,const float *float4_array)
{
  if (!GetInitialized())
    return;
  if (!GetRenderSky())
    return;

  Assert(_engine);
  if (!_engine)
    return;

  // SIMUL WEATHER TODO
}

void SimulSkyRendererDX9::CycleTexturesForward()
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

void SimulSkyRendererDX9::CreateFadeTextures()
{
  if (!GetInitialized())
    return;
  if (!GetUseFadeTextures())
    return;

  Assert(_engine);
  if (!_engine)
    return;
  

  for(int i=0;i<TextureCount;++i)
  {
    _lossTextures[i] = new TextureSysGPUDX9;
    _inscatterTextures[i] = new TextureSysGPUDX9;

    HRESULT hr = _lossTextures[i]->CreateTextures(fadeTexWidth,fadeTexHeight, numAltitudes, _textureFormat );
    if (FAILED(hr))
    {
      _engine->DDError9("SimulWeather - Creating loss texture failed!", hr);
    }

    hr = _inscatterTextures[i]->CreateTextures(fadeTexWidth,fadeTexHeight, numAltitudes, _textureFormat );
    if (FAILED(hr))
    {
      _engine->DDError9("SimulWeather - Creating inscatter texture failed!", hr);
    }

  }

  LogF("SimulWeather - SkyRenderer - fade textures successfully created");
}

void SimulSkyRendererDX9::CreateSkyTextures()
{
  if (!GetInitialized())
    return;
  if (!GetRenderSky())
    return;

  Assert(_engine);
  if (!_engine)
    return;

  for (int i=0; i < TextureCount; ++i)
  {
    _skyTextures[i].Free();
  }
  // SIMUL WEATHER TODO
}

void SimulSkyRendererDX9::CreateSunlightTextures()
{
  if (!GetInitialized())
    return;
  if (!GetRenderSky())
    return;

  Assert(_engine);
  if (!_engine)
    return;

  for (int i=0; i < TextureCount; ++i)
  {
    _sunlightTextures[i].Free();
  }

  // SIMUL WEATHER TODO
}

void SimulSkyRendererDX9::GetLossAndInscatterTextures(Ref<TextureSysGPUDX9>& l1,Ref<TextureSysGPUDX9>&l2,
                                 Ref<TextureSysGPUDX9>&i1,Ref<TextureSysGPUDX9>&i2)
{
  // GetGPUTex also automatically updates GPU texture
  l1=_lossTextures[0];
  l2=_lossTextures[1];
  i1=_inscatterTextures[0];
  i2=_inscatterTextures[1];
}


#endif //_ARMA3_SIMUL_WEATHER
