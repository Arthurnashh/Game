#include "../wpch.hpp"

#if _ARMA3_SIMUL_WEATHER

#include "SimulCloudRendererDX9.hpp"
#include "../d3d9/engdd9.hpp"
#include "../d3d9/txtd3d9.hpp"
#include "../scene.hpp"
#include "../world.hpp"
#include "../depMake.hpp"
#include "SimulWeatherBase.hpp"



//=====================================================================================
// Implementation of SimulCloudRendererDX9
//=====================================================================================

const bool big_endian = false;
static unsigned bits[4]	={(unsigned)0x0F00, (unsigned)0x000F, (unsigned)0x00F0, (unsigned)0xF000};
static unsigned bits8[4]={(unsigned)0x00FF0000, (unsigned)0x000000FF, (unsigned)0x0000FF00, (unsigned)0xFF000000};

//-----------------------------------------------------------------------------
static void SetBits8()
{
  simul::clouds::TextureGenerator::SetBits(bits8[0],bits8[1],bits8[2],bits8[3],(unsigned)4,big_endian);
}

//-----------------------------------------------------------------------------
SimulCloudRendererDX9::SimulCloudRendererDX9(EngineDD9* engine): SimulCloudRendererBase(), _engine(engine), _textureFormat(D3DFMT_UNKNOWN)
{
  Assert(_engine);
  cam_pos.x = cam_pos.y = cam_pos.z = cam_pos.w = 0;

	_vertexBuffersGPU.Init(MaxVBVertices, sizeof(Vertex_t));
	_vertexBuffersCPU.Init(MaxVBVertices, sizeof(CPUFadeVertex_t));
}


//-----------------------------------------------------------------------------
void SimulCloudRendererDX9::FillCloudTextureSequentially(int texture_index,int texel_index,int num_texels,const unsigned *uint32_array)
{
  if(!num_texels)
    return;

  Assert(texture_index >= 0 && texture_index < TextureCount);
  if (texture_index < 0 || texture_index >= TextureCount)
    return;

  Ref<TextureSysGPUDX9> texture = _cloudTextures[texture_index];
  Assert(texture);
  if (!texture)
    return;

  /// fill texture 
  Ref<TextureSysGPUDX9::LockedData> lockedData = TextureSysGPUDX9::GetLockedData(texture);
  Assert(lockedData);
  if (!lockedData)
    return;

  Assert(lockedData->GetData());
  if (!lockedData->GetData())
    return;

  Assert(lockedData->CheckSize());
  if (!lockedData->CheckSize())
    return;

  unsigned *ptr=(unsigned *)(lockedData->GetData());
  const int texelSizeInBytes = 4;

  if (lockedData->IsPitchCorrespondingSize(texelSizeInBytes))
  {
    // perform fast copy
    ptr+=texel_index;
    memcpy(ptr,uint32_array,num_texels*sizeof(unsigned));
  }
  else
  {
    // perform slow copy (pitch does not correspond size)
    for (int i=0; i < num_texels; ++i)
    {
      int offset = lockedData->GetMappedTextureOffset(texel_index+i, texelSizeInBytes) / sizeof(unsigned);
      ptr[offset] = *uint32_array++;
    }
  }

  // we don't need to unlock sysmem texture and update GPU texture - it is done when the texture is needed

#if SIMUL_WEATHER_LOG_TEXTURE_FILLS
  LogF("SimulWeather - CloudRenderer - cloud texture %d filled.", texture_index);
#endif
}


//-----------------------------------------------------------------------------
void SimulCloudRendererDX9::CycleTexturesForward()
{
  Swap(_cloudTextures[0], _cloudTextures[1]);
  Swap(_cloudTextures[1], _cloudTextures[2]);
}


//-----------------------------------------------------------------------------
void SimulCloudRendererDX9::FillIlluminationSequentially(int source_index,int texel_index,int num_texels,const unsigned char *uchar8_array)
{
  if(!num_texels)
    return;
  Assert(_illuminationTexture);
  if (!_illuminationTexture)
    return;

  Ref<TextureSysGPUDX9::LockedData> lockedData = TextureSysGPUDX9::GetLockedData(_illuminationTexture);
  Assert(lockedData);
  if (!lockedData)
    return;

  Assert(lockedData->CheckSize());
  if (!lockedData->CheckSize())
    return;

  unsigned *ptr=(unsigned *)(lockedData->GetData());
  Assert(ptr);
  if (!ptr)
    return;

  unsigned offset[]={16,8,0,24};//*(source_index); // xyzw

  const int texelSizeInBytes = 4;

  if (lockedData->IsPitchCorrespondingSize(texelSizeInBytes))
  {
    // perform fast copy
    ptr+=texel_index;
    for(int i=0;i<num_texels;i++)
    {
      unsigned ui=(unsigned)(*uchar8_array);
      ui<<=offset[source_index];
      unsigned msk=255<<offset[source_index];
      msk=~msk;
      (*ptr)&=msk;
      (*ptr)|=ui;
      uchar8_array++;
      ptr++;
    }
  }
  else
  {
    // perform slow copy
    for(int i=0;i<num_texels;i++)
    {
      int texOffset = lockedData->GetMappedTextureOffset(texel_index+i, texelSizeInBytes) / sizeof(unsigned);

      unsigned ui=(unsigned)(*uchar8_array);
      ui<<=offset[source_index];
      unsigned msk=255<<offset[source_index];
      msk=~msk;
      ptr[texOffset] &=msk;
      ptr[texOffset] |=ui;
      uchar8_array++;
      ptr++;
    }
  }
  //memcpy(ptr,uint32_array,num_texels*sizeof(unsigned));
  
  // no need to unlock or update texture - it is done when the texture is needed

#if SIMUL_WEATHER_LOG_TEXTURE_FILLS
  LogF("SimulWeather - CloudRenderer - illumination texture filled.");
#endif
}


//-----------------------------------------------------------------------------
bool SimulCloudRendererDX9::InitRenderer()
{
  Assert(_engine);
  if (!_engine)
    return false;

  /// check supported texture format
  D3DFORMAT formats[] = 
  {
    D3DFMT_A8R8G8B8
  };

  D3DFORMAT textureFormat = D3DFMT_UNKNOWN;
  for (int i=0; i< lenof(formats); ++i)
  {
    HRESULT err = _engine->CheckDeviceFormat(0, D3DRTYPE_VOLUMETEXTURE, formats[i]);
    if (SUCCEEDED(err))
    {
      err = _engine->CheckDeviceFormat(0, D3DRTYPE_TEXTURE, formats[i]);
      if (SUCCEEDED(err))
      {
        textureFormat = formats[i];
        break;
      }
    }
  }

  if (D3DFMT_UNKNOWN == textureFormat)
  {
    // no appropriate texture format found
    Fail("No appropriate texture format for Simul Weather cloud renderer.")
      return false;
  }

  _textureFormat = textureFormat;

  if (!_noiseTextureFileName.IsEmpty())
    CreateNoiseTexture(_noiseTextureFileName);

  LogF("SimulWeather - CloudRenderer initialized.");
  return true;
}


//-----------------------------------------------------------------------------
void SimulCloudRendererDX9::DeInitRenderer()
{
  for (int i=0; i < TextureCount; ++i)
  {
    _cloudTextures[i].Free();
  }
  _illuminationTexture.Free();
  _noiseTexture.Free();
  _noiseTextureCreated.Free();
  _skyLossTexture1.Free();
  _skyLossTexture2.Free();
  _skyInscatterTexture1.Free();
  _skyInscatterTexture2.Free();

  _illuminationTexWidth = _illuminationTexHeight = _illuminationTexDepth = 0;
  cloud_tex_width_x = cloud_tex_length_y = cloud_tex_depth_z = 0;
  _textureFormat = D3DFMT_UNKNOWN;

  _vertexBuffersGPU.Clear();
  _vertexBuffersCPU.Clear();

  if (cloudKeyframer)
    cloudKeyframer->SetRenderCallback(NULL);

  LogF("SimulWeather - CloudRenderer deinitialized.");
}


//-----------------------------------------------------------------------------
bool SimulCloudRendererDX9::CreateNoiseTextureFromDXFile(RString fileName)
{
  if (fileName.IsEmpty())
    return false;

  Assert(_engine);
  if (!_engine)
    return false;

  Assert(!_engine->GetDirect3DDevice().IsNull());
  if (_engine->GetDirect3DDevice().IsNull())
    return false;

  // Open stream and load data
  QOStrStreamData textureSource(fileName);
  if (!textureSource.str() || !textureSource.pcount())
    return false;

  // get info about file in memory
  D3DXIMAGE_INFO info;
  HRESULT hr = D3DXGetImageInfoFromFileInMemory(textureSource.str(), textureSource.pcount(), &info);

  if (FAILED(hr))
  {
    _engine->DDError9("SimulWeather - CloudRenderer - D3DXGetImageInfoFromFileInMemory failed: ", hr);
    RptF("Cannot create noise texture from file %s, failed to get info from file.", cc_cast(fileName));
    return false;
  }

  // create texture
  ComRef<IDirect3DTexture9> noiseTex;

  hr = _engine->GetDirect3DDevice().CreateTexture(info.Width, info.Height, info.MipLevels, 0, 
    info.Format, D3DPOOL_DEFAULT, noiseTex.Init(), NULL);

  if (FAILED(hr) || !noiseTex)
  {
    _engine->DDError9("SimulWeather - CloudRenderer - CreateTexture for noise texture failed: ", hr);
    RptF("Cannot create noise texture from file %s, Width: %d, Height: %d, MipLevels: %d", cc_cast(fileName), info.Width, info.Height, info.MipLevels);
    return false;
  }

  // Get the surface
  ComRef<IDirect3DSurface9> noiseTextureSurface;
  hr = noiseTex->GetSurfaceLevel(0, noiseTextureSurface.Init());
  if (FAILED(hr))
  {
    _engine->DDError9("SimulWeather - CloudRenderer - GetSurfaceLevel for noise texture failed: ", hr);
    return false;
  }

  // Load the surface 
  hr = D3DXLoadSurfaceFromFileInMemory(noiseTextureSurface, NULL, NULL, textureSource.str(), textureSource.pcount(), NULL, D3DX_FILTER_NONE, 0, &info);
  if (FAILED(hr))
  {
    _engine->DDError9("SimulWeather - CloudRenderer - D3DXLoadSurfaceFromFileInMemory failed: ", hr);
    return false;
  }

  // all ok
  LogF("SimulWeather - CloudRenderer - noise texture created as DX texture from file \"%s\"", cc_cast(fileName));
  _noiseTextureCreated = noiseTex;
  return true;      
}


//-----------------------------------------------------------------------------
void SimulCloudRendererDX9::CreateNoiseTexture(RString fileName)
{
  _noiseTextureCreated.Free();
  _noiseTexture.Free();

  _noiseTextureFileName = fileName;

  Assert(_engine);
  if (!_engine)
    return;

  Assert(!_engine->GetDirect3DDevice().IsNull());
  if (_engine->GetDirect3DDevice().IsNull())
    return;

  if (!fileName.IsEmpty())
  {
    // check extension
    const char *ext = GetFileExt(cc_cast(fileName));

    if ((stricmp(ext, ".dds") == 0 || stricmp(ext, ".tga") == 0) && QFBankQueryFunctions::FileExists(cc_cast(fileName)))
    {
      // try to load as DX file
      if (CreateNoiseTextureFromDXFile(fileName))
        return; //ok, texture created
    }

    // try to load as ordinary texture
    _noiseTexture = GlobLoadTexture(GlobResolveTexture(fileName));
    if (!_noiseTexture)
    {
      RptF("Simul Weather - Cloud Renderer - failed to load texture for noise! (%s)", cc_cast(fileName));
    }
    else
    {
      //_noiseTexture->SetMipmapRange(0,0);
      //_engine->TextBank()->UseMipmap(_noiseTexture, 0, 0);
      _noiseTexture->SetUsageType(Texture::TexCustom); // no limit
      _noiseTexture->ASetNMipmaps(1);
      return; //ok, texture created
    }
  }
  else
  {
    RptF("SimulWeather - Cloud Renderer - noise texture file is not specified!\n");
  }
  
	#define _SIMUL_WEATHER_CREATE_NOISE_TEXTURE 1

	#if _SIMUL_WEATHER_CREATE_NOISE_TEXTURE 
		// create noise texture in system memory
		ComRef<IDirect3DTexture9> tmpTexture;
		HRESULT hr = _engine->GetDirect3DDevice().CreateTexture(noise_texture_size, noise_texture_size,1,  0, 
			_textureFormat, D3DPOOL_SYSTEMMEM, tmpTexture.Init(), NULL);

		if (FAILED(hr))
		{
			_engine->DDError9("SimulWeather - Cannot create noise texture in system memory: ", hr);
			return;
		}

		D3DLOCKED_RECT lockedRect={0};
		hr=tmpTexture->LockRect(0,&lockedRect,NULL,NULL);
		if(FAILED(hr) || !lockedRect.pBits)
		{
			_engine->DDError9("SimulWeather - Cannot lock noise texture in system memory: ", hr);
			return;
		}

		SetBits8();
		simul::clouds::TextureGenerator::Make2DNoiseTexture((unsigned char *)(lockedRect.pBits),
			noise_texture_size,noise_texture_frequency,texture_octaves,texture_persistence,0,0);

		hr=tmpTexture->UnlockRect(0);
		if(FAILED(hr))
		{
			_engine->DDError9("SimulWeather - Cannot unlock noise texture in system memory: ", hr);
			return;
		}

		/// create noise texture on GPU
		_noiseTexture.Free();
		hr = _engine->GetDirect3DDevice().CreateTexture(noise_texture_size, noise_texture_size,1, 0, 
			_textureFormat, D3DPOOL_DEFAULT, _noiseTextureCreated.Init(), NULL);

		if (FAILED(hr))
		{
			_engine->DDError9("SimulWeather - Cannot create noise texture on GPU: ", hr);
			return;
		}

		/// copy texture from system memory to GPU
		hr = _noiseTextureCreated->AddDirtyRect(NULL);
		if (FAILED(hr))
		{
			_engine->DDError9("SimulWeather - noise texture - AddDirtyRect failed: ", hr);
			return;
		}

		hr = _engine->GetDirect3DDevice()->UpdateTexture(tmpTexture, _noiseTextureCreated);
		if (FAILED(hr))
		{
			_engine->DDError9("SimulWeather - Cannot update noise texture on GPU: ", hr);
			return;
		}

		LogF("SimulWeather - CloudRenderer noise texture succesfully created.");
	#endif
}


//-----------------------------------------------------------------------------
void SimulCloudRendererDX9::CreateCloudTexture()
{
  for (int i=0; i<TextureCount; ++i)
  {
    _cloudTextures[i] = new TextureSysGPUDX9;

    HRESULT hr = _cloudTextures[i]->CreateTextures(cloud_tex_width_x,cloud_tex_length_y,cloud_tex_depth_z , _textureFormat );
    if (FAILED(hr))
    {
      _engine->DDError9("SimulWeather - Creating of cloud texture failed!", hr);
      return;
    }
  }

  LogF("SimulWeather - CloudRenderer cloud textures successfully created.");
}


//-----------------------------------------------------------------------------
void SimulCloudRendererDX9::CreateIlluminationTexture()
{
  _illuminationTexture = new TextureSysGPUDX9;

  HRESULT hr = _illuminationTexture->CreateTextures(_illuminationTexWidth, _illuminationTexHeight, _illuminationTexDepth, _textureFormat);

  if (FAILED(hr))
  {
    _engine->DDError9("SimulWeather - Creating of illumination texture failed!", hr);
    return;
  }

  /// fill system memory texture
  Ref<TextureSysGPUDX9::LockedData> lockedData = TextureSysGPUDX9::GetLockedData(_illuminationTexture);
  Assert(lockedData);
  if (!lockedData)
    return;

  void *data = lockedData->GetData();
  Assert(data);
  if (!data)
    return;

  memset(data,0,4*_illuminationTexWidth*_illuminationTexHeight*_illuminationTexDepth);

  LogF("SimulWeather - CloudRenderer - illumination textures successfully created.");
}


//-----------------------------------------------------------------------------
void SimulCloudRendererDX9::SetLossTextures(Ref<TextureSysGPUDX9>& t1,Ref<TextureSysGPUDX9>& t2)
{
  _skyLossTexture1=t1;
  _skyLossTexture2=t2;
}


//-----------------------------------------------------------------------------
void SimulCloudRendererDX9::SetInscatterTextures(Ref<TextureSysGPUDX9>& t1, Ref<TextureSysGPUDX9>& t2)
{
  _skyInscatterTexture1=t1;
  _skyInscatterTexture2=t2;
}


//-----------------------------------------------------------------------------
static inline void SetupD3DVector(D3DXVECTOR4& v, const simul::math::Vector3 &from)
{
  v[0] = from.x;
  v[1] = from.y;
  v[2] = from.z;
  v[3] = from.w;
}


//-----------------------------------------------------------------------------
static inline void SetupD3DVector(D3DXVECTOR4& v, const simul::sky::float4 &from)
{
  v[0] = from.x;
  v[1] = from.y;
  v[2] = from.z;
  v[3] = from.w;
}


//-----------------------------------------------------------------------------
static inline void SetupD3DVector(D3DXVECTOR4& v, const Vector3P& from)
{
  v[0] = from[0];
  v[1] = from[1];
  v[2] = from[2];
}


//-----------------------------------------------------------------------------
void SimulCloudRendererDX9::SetupShaderConstants(int cb, bool cubemap)
{
  Camera *camera = GScene->GetCamera();

  // setup matrices
  PositionRender modelToWorld;
  modelToWorld.position.SetIdentity();
  modelToWorld.camSpace = false;

  _engine->SetBias(cb, 0);
  _engine->SetupWorldViewProjCam(cb, modelToWorld);

  // VS parameters (see VSSimulWeatherClouds)
  static const int VSParsSize = 5;
  D3DXVECTOR4 VSPars[VSParsSize];

  D3DXVECTOR4& illuminationOrigin = VSPars[0]; //ok
  float& altitudeTexCoord = VSPars[0][3]; //ok
  D3DXVECTOR4& illuminationScales = VSPars[1]; //ok
  float& maxFadeDistanceMetres = VSPars[1][3]; //ok
  D3DXVECTOR4& fractalScaleVS = VSPars[2]; //ok
  D3DXVECTOR4& eyePosition = VSPars[3]; //ok
  D3DXVECTOR4& fogPars = VSPars[4]; //ok

  // PS parameters (see PSSimulWeatherCloudsWithLightning)
  static const int PSParsSize = 8;
  D3DXVECTOR4 PSPars[PSParsSize];

  float& hazeEccentricity = PSPars[0][0]; //ok
  float& interp = PSPars[0][1]; //ok
  float& cloudEccentricity = PSPars[0][2]; //ok
  float& alphaSharpness = PSPars[0][3]; //ok
  D3DXVECTOR4& mieRayleighRatio = PSPars[1]; //ok
  D3DXVECTOR4& fractalScalePS = PSPars[2]; //ok
  D3DXVECTOR4& lightResponse = PSPars[3]; //ok
  D3DXVECTOR4& lightningMultipliers = PSPars[4]; //ok
  D3DXVECTOR4& lightningColour  = PSPars[5]; //ok
  D3DXVECTOR4& skylightColour = PSPars[6]; //ok
  D3DXVECTOR4& lightDir = PSPars[7]; //ok
  float& fadeInterp = PSPars[7][3]; //ok

  static const float lightningEffectOnCloud=20.f;

  if (enable_lightning && lightningRenderInterface)
  {
    static float bb=.1f;
    simul::sky::float4 lightning_multipliers;
    lightningColour=lightningRenderInterface->GetLightningColour();
    for(unsigned i=0;i<4;i++)
    {
      if(i<lightningRenderInterface->GetNumLightSources())
        lightningMultipliers[i]=bb*lightningRenderInterface->GetLightSourceBrightness(i);
      else 
        lightningMultipliers[i]=0;
    }
    lightningColour.w=lightningEffectOnCloud;

    simul::math::Vector3 light_X1=lightningRenderInterface->GetIlluminationOrigin();
    simul::math::Vector3 light_DX=lightningRenderInterface->GetIlluminationScales();
    
    SetupD3DVector(illuminationOrigin, light_X1);
    SetupD3DVector(illuminationScales, light_DX);
  }

  altitudeTexCoord = altitude_tex_coord;
  maxFadeDistanceMetres = _maxFadeDistanceMetres;

  simul::sky::float4 fractalScales=(cubemap?0.f:1.f)*helper->GetFractalScales(cloudInterface);
  SetupD3DVector(fractalScaleVS, fractalScales);
  SetupD3DVector(fractalScalePS, fractalScales);

  SetupD3DVector(eyePosition, camera->Position());

  hazeEccentricity = skyInterface->GetMieEccentricity();

  interp = cloudKeyframer->GetInterpolation();

  cloudEccentricity = cloudInterface->GetMieAsymmetry();
  alphaSharpness = cloudInterface->GetAlphaSharpness();

  SetupD3DVector(mieRayleighRatio, skyInterface->GetMieRayleighRatio());

  simul::sky::float4 light_response(_directLightCoef*cloudInterface->GetLightResponse(),
    _indirectLightCoef*cloudInterface->GetSecondaryLightResponse(),
    0,
    0);

  simul::sky::float4 sun_dir=skyInterface->GetDirectionToLight();
  Swap(sun_dir.y,sun_dir.z); //Arma has y as vertical axis

  // Get the overall ambient light at this altitude, and multiply it by the cloud's ambient response.
  float altKM = camera->Position()[1] * 0.001f;
  simul::sky::float4 sky_light_colour=skyInterface->GetAmbientLight(altKM)*cloudInterface->GetAmbientLightResponse();

  SetupD3DVector(lightResponse, light_response);
  SetupD3DVector(skylightColour, sky_light_colour);
  SetupD3DVector(lightDir, sun_dir);

  fadeInterp = fade_interp;

  // setup fog params
  fogPars.x = _fogStart;
  fogPars.y = 1.0f;
  if (_fogEnd - _fogStart > 0)
  {
    fogPars.y = 1.0f/(_fogEnd - _fogStart);
  }

  // setup constants
  _engine->SetVertexShaderConstantF(cb, VSC_SimulWeatherPars, (float*)VSPars, VSParsSize);
  _engine->SetPixelShaderConstantF(cb, PSC_SimulWeatherPars, (float*)PSPars, PSParsSize);

  // setup cloud light modifier
  Color cloudColorModifier = _cloudLighting * _engine->GetAccomodateEye().Brightness() * _engine->_modColor.Brightness();
  _engine->SetPixelShaderConstantF(cb, PSC_AE, cloudColorModifier.GetArray(), 1);  
}


//-----------------------------------------------------------------------------
void SimulCloudRendererDX9::SetupTextures(int cb)
{
  for (int i=0; i<8; ++i)
  {
    // Disable any in-texture gamma-correction that might be lingering from some other bit of rendering:
    _engine->D3DSetSamplerState(cb, i, D3DSAMP_SRGBTEXTURE, 0);

    _engine->D3DSetSamplerState(cb, i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    _engine->D3DSetSamplerState(cb, i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    _engine->D3DSetSamplerState(cb, i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
  }

  // setup textures
  bool wrap = cloudInterface ? cloudInterface->GetWrap() : false;
  DWORD wrapMode = wrap? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;

  // GetGPUTex also automatically updates GPU texture, so it is ready when we want to render
  if (_cloudTextures[0])
    _engine->SetTexture(cb, 0,  _cloudTextures[0]->GetGPUTex());
  else
    _engine->SetTexture(cb, 0,  0);

  _engine->D3DSetSamplerState(cb, 0, D3DSAMP_ADDRESSU, wrapMode);
  _engine->D3DSetSamplerState(cb, 0, D3DSAMP_ADDRESSV, wrapMode);
  _engine->D3DSetSamplerState(cb, 0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);

  if (_cloudTextures[1])
    _engine->SetTexture(cb, 1, _cloudTextures[1]->GetGPUTex());
  else
    _engine->SetTexture(cb, 1, 0);

  _engine->D3DSetSamplerState(cb, 1, D3DSAMP_ADDRESSU, wrapMode);
  _engine->D3DSetSamplerState(cb, 1, D3DSAMP_ADDRESSV, wrapMode);
  _engine->D3DSetSamplerState(cb, 1, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);

  if (_noiseTexture)
  {
    _engine->TextBank()->UseMipmap(_noiseTexture, 0, 0);
    TextureD3D9* textureD9 = (TextureD3D9*)_noiseTexture.GetRef();
    _engine->SetTexture(cb, 2, textureD9->GetHandle());
  }
  else if (_noiseTextureCreated)
  {
    _engine->SetTexture(cb, 2, _noiseTextureCreated);
  }
  _engine->D3DSetSamplerState(cb, 2, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
  _engine->D3DSetSamplerState(cb, 2, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
  _engine->D3DSetSamplerState(cb, 2, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);

  if (_illuminationTexture)
    _engine->SetTexture(cb, 3, _illuminationTexture->GetGPUTex());
  else
    _engine->SetTexture(cb, 3, 0);

  _engine->D3DSetSamplerState(cb, 3, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
  _engine->D3DSetSamplerState(cb, 3, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
  _engine->D3DSetSamplerState(cb, 3, D3DSAMP_ADDRESSW, D3DTADDRESS_BORDER);

  if(fade_mode==FRAGMENT)
  {
    if (_skyLossTexture1)
      _engine->SetTexture(cb, 4, _skyLossTexture1->GetGPUTex());
    else
      _engine->SetTexture(cb, 4, 0);

    if (_skyLossTexture2)
      _engine->SetTexture(cb, 5, _skyLossTexture2->GetGPUTex());
    else
      _engine->SetTexture(cb, 5, 0);

    if (_skyInscatterTexture1)
      _engine->SetTexture(cb, 6, _skyInscatterTexture1->GetGPUTex());
    else
      _engine->SetTexture(cb, 6, 0);

    if (_skyInscatterTexture2)
      _engine->SetTexture(cb, 7, _skyInscatterTexture2->GetGPUTex());
    else
      _engine->SetTexture(cb, 7, 0);

    for (int i=4; i<8; ++i)
    {
      _engine->D3DSetSamplerState(cb, i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      _engine->D3DSetSamplerState(cb, i, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
      _engine->D3DSetSamplerState(cb, i, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
    }
  }      
}


//-----------------------------------------------------------------------------
void SimulCloudRendererDX9::SetupRenderStates(int cb)
{
  _engine->D3DSetRenderState(cb,  D3DRS_ZENABLE, TRUE);
  _engine->D3DSetRenderState(cb,  D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
  _engine->D3DSetRenderState(cb,  D3DRS_ZWRITEENABLE, FALSE);
  _engine->D3DSetRenderState(cb,  D3DRS_DEPTHBIAS, 0);
  float zero = 0.0f;
  DWORD dwZero = *(const DWORD*)&zero;
  _engine->D3DSetRenderState(cb, D3DRS_SLOPESCALEDEPTHBIAS, dwZero);

  _engine->D3DSetRenderState(cb, D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | 
    D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

  _engine->D3DSetRenderState(cb, D3DRS_ALPHABLENDENABLE, TRUE);
  _engine->D3DSetRenderState(cb, D3DRS_ALPHATESTENABLE, FALSE);
  _engine->D3DSetRenderState(cb, D3DRS_CULLMODE, D3DCULL_NONE);
  _engine->D3DSetRenderState(cb, D3DRS_FILLMODE, D3DFILL_SOLID);

  _engine->D3DSetRenderState(cb, D3DRS_SRCBLEND,D3DBLEND_ONE);
  _engine->D3DSetRenderState(cb, D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
  _engine->D3DSetRenderState(cb, D3DRS_BLENDOP, D3DBLENDOP_ADD);

  // blending for alpha:
  _engine->D3DSetRenderState(cb, D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
  _engine->D3DSetRenderState(cb, D3DRS_SRCBLENDALPHA,D3DBLEND_ZERO);
  _engine->D3DSetRenderState(cb, D3DRS_DESTBLENDALPHA,D3DBLEND_INVSRCALPHA);
  _engine->D3DSetRenderState(cb, D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);

  if (_engine->_useAToC)
    _engine->D3DEnableAlphaToCoverage(cb, false);
}


//-----------------------------------------------------------------------------
void SimulCloudRendererDX9::RestoreRenderStates(int cb)
{
  // we have to restore D3DRS_SEPARATEALPHABLENDENABLE
  _engine->D3DSetRenderState(cb, D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
}


//-----------------------------------------------------------------------------
void SimulCloudRendererDX9::SetupShaders(int cb)
{
	IDirect3DPixelShader9 *pixelShader = NULL;
	IDirect3DVertexShader9 *vertexShader = NULL;
	IDirect3DVertexDeclaration9 *vertexDeclaration = NULL;

	if (FRAGMENT == fade_mode)
	{
		vertexShader = _engine->_vertexShaderSpecial[VSSSimulWeatherClouds];
		vertexDeclaration = _engine->_vertexDeclD[VD_SimulWeatherClouds][VSDV_Basic];
	}
	else
	{
		vertexShader = _engine->_vertexShaderSpecial[VSSSimulWeatherCloudsCPU];
		vertexDeclaration = _engine->_vertexDeclD[VD_SimulWeatherCloudsCPU][VSDV_Basic];
	}

  if (enable_lightning && lightningRenderInterface)
  {
    if (FRAGMENT == fade_mode)
    {
      pixelShader = _engine->_pixelShader[PSSimulWeatherCloudsWithLightning][PSTBasic];
    }
    else
    {
      pixelShader = _engine->_pixelShader[PSSimulWeatherCloudsWithLightningCPU][PSTBasic];
    }
  }
  else
  {
    if (FRAGMENT == fade_mode)
    {
      pixelShader = _engine->_pixelShader[PSSimulWeatherClouds][PSTBasic];
    }
    else
    {
      pixelShader = _engine->_pixelShader[PSSimulWeatherCloudsCPU][PSTBasic];
    }
  }

  Assert(pixelShader);
  if (!pixelShader)
  {
    LogF("SimulWeather - CloudRenderer - no pixel shader found.");
  }
  Assert(vertexShader);
  if (!vertexShader)
  {
    LogF("SimulWeather - CloudRenderer - no vertex shader found.");
  }
  Assert(vertexDeclaration);
  if (!vertexDeclaration)
  {
    LogF("SimulWeather - CloudRenderer - no vertex declaration found.");
  }
  
  _engine->SetVertexShader(cb, vertexShader);
  _engine->SetPixelShader(cb, pixelShader);
  _engine->SetVertexDeclaration(cb, vertexDeclaration);
}


//-----------------------------------------------------------------------------
void SimulCloudRendererDX9::Render(int cb, bool cubemap)
{
	if (!GScene || !GScene->GetCamera() || !_engine || _engine->GetDirect3DDevice().IsNull())
		return;

	if (!cloudKeyframer || !cloudInterface || !skyInterface || !helper)
		return;

	if (_engine->ResetNeeded()) 
		return;

	enable_lightning = false;
	
  _engine->SwitchTL(cb,EngineDD9::TLEnabled);

  SetupShaderConstants(cb, cubemap);
  SetupTextures(cb);
  SetupRenderStates(cb);
  SetupShaders(cb);

  RenderGeometry(cb, cubemap);  

  RestoreRenderStates(cb);
}
 

//-----------------------------------------------------------------------------
void SimulCloudRendererDX9::RenderGeometryPrimitiveUP(int cb, bool cubemap, Camera* camera)
{
  void *data = 0;
  int vertexSize = 0;
  if(fade_mode!=CPU)
  {
    _vertices.Resize(MaxVertices);
    data = (void*)_vertices.Data();
    vertexSize = sizeof(Vertex_t);
  }
  else
  {
    _cpuFadeVertices.Resize(MaxVertices);
    data = (void*)_cpuFadeVertices.Data();
    vertexSize = sizeof(CPUFadeVertex_t);
  }

  int v = FillVertices(this, data, MaxVertices, vertexSize, *camera, camera->ProjectionNormal(), cubemap, fade_mode==CPU, enable_lightning);
  if(v>2)
  {
    Array<char> vertexData((char *)data,v*vertexSize);
    CALL_CB_ENGINE(_engine, cb, DrawPrimitiveUP,D3DPT_TRIANGLESTRIP, v-2, vertexData, vertexSize);
  }
}


//-----------------------------------------------------------------------------
void SimulCloudRendererDX9::RenderGeometryVertexBuffer(int cb, bool cubemap, bool async, Camera* camera)
{
  // protect buffer creation with critical section
  ScopeLockSection lock(_renderVBCritSection);

  PROFILE_SCOPE_EX(swRdGeomVB, SW);

  /// create data
  void *data = 0;
  int vertexSize = 0;
  if(fade_mode!=CPU)
  { 
    _vertices.Resize(MaxVertices);
    data = (void*)_vertices.Data();
    vertexSize = sizeof(Vertex_t);
  } 
  else 
  { 
    _cpuFadeVertices.Resize(MaxVertices);
    data = (void*)_cpuFadeVertices.Data();
    vertexSize = sizeof(CPUFadeVertex_t);
  }

  int numVertices = FillVertices(this, data, MaxVertices, vertexSize, *camera, camera->ProjectionNormal(), cubemap, fade_mode==CPU, enable_lightning);
  if (numVertices <= 2)
    return; //nothing to do...

  // get free vertex buffer
  Ref<UserVertexBuffer> buffer = GetFreeVertexBuffer(numVertices);
  if (!buffer)
    return; //no free buffer

  Assert(buffer->GetBuffer());
  if (!buffer->GetBuffer())
    return;

  // fill buffer
  int startPos = buffer->FillVertices(data, numVertices);

  if (startPos < 0 || startPos >= buffer->GetMaxCount())
    return; //something went wrong...

  // size really does not matter here
  _engine->SetStreamSource(cb, VertexStaticData9::BufferFuture(buffer->GetBuffer()),vertexSize, buffer->GetMaxCount()); 

  _engine->DrawPrimitive(cb, D3DPT_TRIANGLESTRIP, startPos, numVertices-2);
}


//-----------------------------------------------------------------------------
void SimulCloudRendererDX9::RenderGeometry(int cb, bool cubemap)
{
  PROFILE_SCOPE_EX(swRdGeom, SW);

	Camera* camera = GScene->GetCamera();
	if (!camera)
		return;

  switch (GetRenderMode())
  {
  case RMPrimitiveUP:
    RenderGeometryPrimitiveUP(cb, cubemap, camera);
    break;
  case RMVertexBuffer:
    RenderGeometryVertexBuffer(cb, cubemap, false, camera);
    break;
  case RMVertexBufferAsync:
    RenderGeometryVertexBuffer(cb, cubemap, true, camera);
    break;
  default:
    LogF("SimulWeather - CloudRenderer - invalid render mode for simul weather clouds: %d", (int)GetRenderMode());
    break;
  }
}


//-----------------------------------------------------------------------------
Ref<UserVertexBuffer> SimulCloudRendererDX9::GetFreeVertexBuffer(int numVertices)
{
	if (fade_mode == CPU)
		return _vertexBuffersCPU.GetVertexBuffer(numVertices);

	return _vertexBuffersGPU.GetVertexBuffer(numVertices);
}


//-----------------------------------------------------------------------------
void **SimulCloudRendererDX9::GetCloudTextures()
{
  for (int i=0; i<TextureCount; ++i)
  {
    if (_cloudTextures[i])
      _cloudTexturesPtrs[i] = (IDirect3DVolumeTexture9*)_cloudTextures[i]->GetGPUTexNoUpdate().GetRef();
    else
      _cloudTexturesPtrs[i] = 0;
  }

  return (void **)_cloudTexturesPtrs;
}

#endif //_ARMA3_SIMUL_WEATHER

