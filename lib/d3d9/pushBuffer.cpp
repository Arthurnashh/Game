#include "../wpch.hpp"
#if !defined _RESISTANCE_SERVER
#include "pushBuffer.hpp"
#include "engdd9.hpp"
#include "txtD3D9.hpp"
#include "../keyInput.hpp"
#include "../dikCodes.h"
#include "../textbank.hpp"
#include "../Shape/material.hpp"
#include "../scene.hpp"
#include "../landImpl.hpp"
#include <El/Common/perfLog.hpp>

EngineDD9 *PBItem::_engine;

#define PB_SetVertexShaderConstantF(register,data,count) _engine->SetVertexShaderConstantF(register, data, count)
#define PB_SetVertexShaderConstantB(register,data,count) _engine->SetVertexShaderConstantB(register, data, count)
#define PB_SetVertexShaderConstantI(register,data,count) _engine->SetVertexShaderConstantI(register, data, count)
#define PB_SetRenderState(state,data) _engine->D3DSetRenderState(state,data)
#define PB_SetSamplerState(stage,state,value) _engine->D3DSetSamplerState(stage, state, value)
#define PB_SetPixelShader(ps) _engine->SetPixelShader(ps)
#define PB_SetPixelShaderConstantF(register,data,count) _engine->SetPixelShaderConstantF(register, data, count)
#define PB_SetPixelShaderConstantI(register,data,count) _engine->SetPixelShaderConstantI(register, data, count)
#define PB_SetPixelShaderConstantB(register,data,count) _engine->SetPixelShaderConstantB(register, data, count)
#define PB_SetVertexShader(vs) _engine->SetVertexShader(vs)
#define PB_SetVertexDeclaration(decl) _engine->SetVertexDeclaration(decl)

#if USE_PUSHBUFFERS
#if _ENABLE_CHEATS
static bool DummyPBLow = false;
#else
const bool DummyPBLow = false;
#endif
#else
const bool DummyPBLow = false;
#endif

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstant::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetVertexShaderConstantF(_startRegister, _data, 1);
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstant2::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetVertexShaderConstantF(_startRegister, (float*)_data, 2);
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstantB::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetVertexShaderConstantB(_startRegister, &_data, 1);
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstantB2::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetVertexShaderConstantB(_startRegister, _data, 2);
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstantI::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetVertexShaderConstantI(_startRegister, _data, 1);
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstantMLSun::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  // Setup directional light constants
  Color diffuse = _engine->_cbState._lightDiffuse * _matDiffuse;
  diffuse.SetA(_matDiffuse.A());
  PB_SetVertexShaderConstantF(VSC_LDirectionD, (float*)&diffuse, 1);
  Color specular = _engine->_cbState._lightSpecular * _matSpecular;
  PB_SetVertexShaderConstantF(VSC_LDirectionS, (float*)&specular, 1);
  Vector3 tDirection = _engine->TransformAndNormalizeVectorToObjectSpace(_engine->_cbState._direction);
  D3DXVECTOR4 direction(tDirection.X(), tDirection.Y(), tDirection.Z(), 1.0f);
  PB_SetVertexShaderConstantF(VSC_LDirectionTransformedDir, (float*)&direction, 1);
  Vector3 tDirectionUp = _engine->TransformAndNormalizeVectorToObjectSpace(VUp);
  D3DXVECTOR4 directionUp(tDirectionUp.X(), tDirectionUp.Y(), tDirectionUp.Z(), 1.0f);
  PB_SetVertexShaderConstantF(VSC_UDirectionTransformedDir, (float*)&directionUp, 1);

  // Get the scene main light
  LightSun *sun = GScene->MainLight();

  // Setup constant for ground color (for back reflection purposes)
  // See label GROUNDREFLECTION
  ColorVal groundColor = sun->GroundReflection() * diffuse;
  PB_SetVertexShaderConstantF(VSC_LDirectionGround, (float*)&groundColor, 1);
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstantMLNone::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  Color black = HBlack;
  PB_SetVertexShaderConstantF(VSC_LDirectionS, (float*)&black, 1);
  PB_SetVertexShaderConstantF(VSC_LDirectionD, (float*)&black, 1);
  //PB_SetVertexShaderConstantF(VSC_LDirectionGround, (float*)&black, 1);

  Vector3 tDirection = _engine->TransformAndNormalizeVectorToObjectSpace(_engine->_cbState._direction);
  D3DXVECTOR4 direction(tDirection.X(), tDirection.Y(), tDirection.Z(), 1.0f);
  PB_SetVertexShaderConstantF(VSC_LDirectionTransformedDir, (float*)&direction, 1);
  Vector3 tDirectionUp = _engine->TransformAndNormalizeVectorToObjectSpace(VUp);
  D3DXVECTOR4 directionUp(tDirectionUp.X(), tDirectionUp.Y(), tDirectionUp.Z(), 1.0f);
  PB_SetVertexShaderConstantF(VSC_UDirectionTransformedDir, (float*)&directionUp, 1);
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstantSAFR::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  float value[4]={
    _matPower, _matAmbient.A(), _engine->_cbState._fogEnd, 1.0f / (_engine->_cbState._fogEnd - _engine->_cbState._fogStart)
  };
  PB_SetVertexShaderConstantF(VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart, value, 1);
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstantLightValuesR::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;

  // AE
  Color newAE = _matEmmisive * _engine->GetAccomodateEye() + _engine->_cbState._lightAmbient * _matAmbient;

  // Set the diffuse back coefficient
  float backCoef = _matForcedDiffuse.A();
  newAE.SetA(backCoef);

  // G
  LightSun *sun = GScene->MainLight();
  Color newGE = _matEmmisive * _engine->GetAccomodateEye() + sun->GroundReflection() * _engine->_cbState._lightAmbient * _matAmbient;

  // Set the PS constants
  PB_SetVertexShaderConstantF(VSC_AE, (float*)&newAE, 1);
  PB_SetVertexShaderConstantF(VSC_GE, (float*)&newGE, 1);
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstantFFAR::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  float value[4]={
    0,_engine->_cbState._expFogCoef, _engine->_cbState._aFogEnd,
    _engine->_cbState._aFogEnd > _engine->_cbState._aFogStart ? 1.0f / (_engine->_cbState._aFogEnd - _engine->_cbState._aFogStart) : 1.0f
  };
  PB_SetVertexShaderConstantF(VSC_Free_ExpFog_AFogEnd_RCPAFogEndMinusAFogStart, value, 1);
}

//////////////////////////////////////////////////////////////////////////

void ConvertMatrixTransposed(PASSMATRIXTYPE &mat, Matrix4Val src);

void PBISetVertexShaderConstantLWSM::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;

  // Calculate the local world space matrix
  Matrix4 modelToLWS;
  {
    Matrix4 localSpace;
    localSpace.SetOrientation(M3Identity);
    localSpace.SetPosition(_engine->_sceneProps->GetCameraPosition());
    modelToLWS = localSpace.InverseRotation() * _engine->_cbState._world;
  }

  // Pass the LWS matrix to the shader
  {
    PASSMATRIXTYPE lwsMatrix;
    ConvertMatrixTransposed(lwsMatrix, modelToLWS);
    PB_SetVertexShaderConstantF(VSC_LWSMatrix, (float*)&lwsMatrix, 3);
  }
}

//////////////////////////////////////////////////////////////////////////

void PBISetRenderState::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetRenderState(_state, _data);
}

//////////////////////////////////////////////////////////////////////////

void PBISetSamplerState::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetSamplerState(_stage, _state, _value);
}

//////////////////////////////////////////////////////////////////////////

static inline float CombineAvgAndMaxColor(float ac, float mc)
{
  if (ac<=mc*1e-6) return 0;
  if (ac>=mc*10) return 10;
  float ret = ac/mc;
  Assert(_finite(ret));
  return ret;
}

/// avoid division zero by zero during color combining
static Color CombineAvgAndMaxColor(ColorVal avgColor, ColorVal maxColor)
{
  return Color(
    CombineAvgAndMaxColor(avgColor.R(),maxColor.R()),
    CombineAvgAndMaxColor(avgColor.G(),maxColor.G()),
    CombineAvgAndMaxColor(avgColor.B(),maxColor.B()),
    CombineAvgAndMaxColor(avgColor.A(),maxColor.A())
    );
}

void PBISetTexture0::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  DoAssert(_texture); // At least white texture ought to be set
  
  Color maxColor = _texture->GetMaxColor();
  const TextureD3D9 *loadedTexture;
  if (_texture->GetHandle())
  {
    loadedTexture = _texture;
  }
  else
  {
    maxColor = _texture->GetColor();
    loadedTexture = _texture->GetDefaultTexture();
  }
  _engine->SetTexture0Color(maxColor, CombineAvgAndMaxColor(_texture->GetColor(),maxColor),true);
  _engine->SetTexture(0, loadedTexture->GetHandle());
}

//////////////////////////////////////////////////////////////////////////

void PBISetTexture::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  TextureD3DHandle tHandle;
  if (_texture)
  {
    const TextureD3D9 *loadedTexture;
    if (_texture->GetHandle())
    {
      loadedTexture = _texture;
#if 0 // _ENABLE_CHEATS
      static bool macroOff = true;
      static bool ambientOff = false;
      static bool detailOff = false;
      switch (tex[i]->Type())
      {
      case TT_Macro:
        if (!macroOff) break;
        goto TexOff;
      case TT_Detail:
        if (!detailOff) break;
        goto TexOff;
      case TT_AmbientShadow: 
        if (!ambientOff) break;
        goto TexOff;
      TexOff:
        //case TT_Normal:
        //case TT_Detail:
        loadedTexture = _texture->GetDefaultTexture();
        break;
      }
#endif
    }
    else
    {
      loadedTexture = _texture->GetDefaultTexture();
    }
    tHandle = loadedTexture->GetHandle();
  }
  else
  {
    tHandle = NULL;
  }
  _engine->SetTexture(_stage, tHandle);
}

//////////////////////////////////////////////////////////////////////////

void PBISetTextureSB::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;

  if (
    // when no 3D, no shadow maps used
    _engine->_cbState._tlActive!=EngineDD9::TLEnabled ||
    // when doing z rendering, no shadow maps are used
    _engine->_renderingMode>RMCommon && _engine->_renderingMode!=RMCrater
  )
  {
    _engine->SetTexture(TEXID_SHADOWMAP, NULL, false);
    return;
  }
  // Determine whether we want to use the SSSM technique, or just the SB
  _engine->_cbState._doSSSM = _engine->_zPrimingDone && !_alphaPrimitives;

  if (_engine->_cbState._doSSSM)
  {
    // Set the SSSM as shadow map
    _engine->UseShadowMapSSSM();
  }
  else
  {
    // Set the SB as shadow map
    _engine->UseShadowMapSB();
  }
}

//////////////////////////////////////////////////////////////////////////

void PBISetPixelShader::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetPixelShader(_ps);
}

//////////////////////////////////////////////////////////////////////////

void PBISetPixelShaderConstant::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetPixelShaderConstantF(_startRegister, _data, 1);
}

//////////////////////////////////////////////////////////////////////////

void PBISetPixelShaderConstantI::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetPixelShaderConstantI(_startRegister, _data, 1);
}

//////////////////////////////////////////////////////////////////////////

PBISetPixelShaderConstantDBS::PBISetPixelShaderConstantDBS(ColorVal matDiffuse, ColorVal matForcedDiffuse, ColorVal matSpecular, float matPower, bool useGroundBidir, bool useNewSpecular)
{
  _matDiffuse = matDiffuse;
  _matForcedDiffuse = matForcedDiffuse;
  _matSpecular = matSpecular;
  _matPower = matPower;
  _useGroundBidir = useGroundBidir;

  // Calculate specular coefficient (make the specular reflection stronger), consider specular power
  if (useNewSpecular && (_matPower > 0))
  {
    static float b = 0.5f;
    static float c = 1000.0f;
    _specularCoef = c / Square(2.0f * acos(pow(b, 1.0f / _matPower)) * 360.0f/(2.0f*H_PI));
  }
  else
  {
    _specularCoef = 1.0f;
  }
}

void PBISetPixelShaderConstantDBS::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  // Prepare array of colors to use only one call later
  Color difBakSpec[3];
  Color &diffuse = difBakSpec[0];
  Color &diffuseBack = difBakSpec[1];
  Color &specular = difBakSpec[2];

  // Calculate diffuse color
  {
    // Combine light diffuse and material diffuse
    diffuse = _engine->_cbState._lightDiffuse * _matDiffuse;

    // Copy alpha from material diffuse alpha
    diffuse.SetA(_matDiffuse.A());
  }

  // Calculate diffuse back color
  // See label GROUNDREFLECTION
  if (_useGroundBidir)
  {
    LightSun *sun = GScene->MainLight();
    diffuseBack = sun->GroundReflection() * diffuse;
  }
  else
  {
    float backCoef = _matForcedDiffuse.A();
    diffuseBack = diffuse * backCoef;
    diffuseBack.SetA(backCoef);
  }

  // Saturate the result by SunObjectColor + SunHaloObjectColor (to reflect no more than them)
  LightSun *sun = GScene->MainLight();
  static float sunBrightnessCoef = 2.0f;
  float sunBrightness = (sun->SunObjectColor().Brightness() + sun->SunHaloObjectColor().Brightness()) * _engine->GetAccomodateEye().Brightness() * _engine->_modColor.Brightness() * sunBrightnessCoef;
  float specularCoefSat = min(_specularCoef, sunBrightness);

  // Calculate specular color (which includes specular light from the Sun only, not from the environment)
  /// multiply by 2 so that pixel shader can avoid _x2
  specular = _engine->_cbState._lightSpecular * _matSpecular * 2.0f * Color(specularCoefSat, specularCoefSat, specularCoefSat, 1.0f);
  specular.SetA(_matPower);

//   // In case we use the new specular calculation (like in case of super shader), modify the specular color
//   if (_matPower > 0)
//   {
//     static float b = 0.5f;
//     static float c = 1000.0f;
//     const float specCoef = c / Square(2.0f * acos(pow(b, 1.0f / _matPower)) * 360.0f/(2.0f*H_PI));
//     specular = Color(specular.R() * specCoef, specular.G() * specCoef, specular.B() * specCoef, specular.A());
//   }

  // Set the pixel shader constant
  PB_SetPixelShaderConstantF(PSC_Diffuse, (float*)difBakSpec, 3);
}

//////////////////////////////////////////////////////////////////////////

void PBISetPixelShaderConstantDBSZero::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  static const Color zeroes[3]={Color(0, 0, 0, 0), Color(0, 0, 0, 0), Color(0, 0, 0, 0)};
  PB_SetPixelShaderConstantF(PSC_Diffuse, (float*)zeroes, 3);
}

//////////////////////////////////////////////////////////////////////////

void PBISetPixelShaderConstant_A_DFORCED_E_Sun::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;

  // AE
  Color newAE = _matEmmisive * _engine->GetAccomodateEye() + _engine->_cbState._lightAmbient * _matAmbient;

  // Set the diffuse back coefficient
  float backCoef = _matForcedDiffuse.A();
  newAE.SetA(backCoef);

  // G
  LightSun *sun = GScene->MainLight();
  Color newGE = _matEmmisive * _engine->GetAccomodateEye() + sun->GroundReflection() * _engine->_cbState._lightAmbient * _matAmbient;

  // DForced
  Color newDForced = HBlack + _engine->_cbState._lightDiffuse * _matForcedDiffuse;

  // Set the PS constants
  PB_SetPixelShaderConstantF(PSC_AE, (float*)&newAE, 1);
  PB_SetPixelShaderConstantF(PSC_GE, (float*)&newGE, 1);
  PB_SetPixelShaderConstantF(PSC_DForced, (float*)&newDForced, 1);
}

//////////////////////////////////////////////////////////////////////////

void PBISetPixelShaderConstant_A_DFORCED_E_NoSun::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;

  // E
  Color newE = _matEmmisive * _engine->GetAccomodateEye();

  // Zero
  Color zero = Color(0, 0, 0, 0);

  // Set the PS constants
  PB_SetPixelShaderConstantF(PSC_AE, (float*)&newE, 1);
  PB_SetPixelShaderConstantF(PSC_GE, (float*)&newE, 1);
  PB_SetPixelShaderConstantF(PSC_DForced, (float*)&zero, 1);
}

PBISetPixelShaderConstantLayers::PBISetPixelShaderConstantLayers(int layerMask, const TexMaterial *mat)
{
  for (int i=0; i<lenof(_layers); i++) _layers[i] = (layerMask&(1<<i))!=0;
  for (int i=0; i<lenof(_heigthTexSizeLog2); i++)
  {
    // provide a default init in case no other value is available
    _heigthTexSizeLog2[i] = 0;
    int stage = LandMatStageLayers+LandMatStagePerLayer*i+LandMatNoInStage;
    if (stage<mat->_stageCount+1)
    {
      const Texture *noMap = mat->_stage[i]._tex;
      if (noMap)
      {
        // we are interested in texture "size"
        // the pixel shader is not caring about x / y separately, therefore we need to provide a single number
        float area = noMap->AWidth()*noMap->AHeight();
        const float log2 = 0.69314718056f;
        // compute log2 of sqrt(area)
        _heigthTexSizeLog2[i] = log(area)*(0.5f/log2);
      }
    }
  }
}

void PBISetPixelShaderConstantLayers::Do(const PBContext &ctx) const
{
  // Set the PS constants
  float layers[lenof(_layers)][4];
  for (int i=0; i<lenof(_layers); i++)
  {
    layers[i][0] = layers[i][1] = layers[i][2] = layers[i][3] = _layers[i];
  }
  PB_SetPixelShaderConstantF(PSC_Layers, layers[0], lenof(_layers));
  // pack 4 floats in one constant
  const int floatPerVector = 4;
  float texSize[(lenof(_heigthTexSizeLog2)+floatPerVector-1)/floatPerVector*floatPerVector];
  COMPILETIME_COMPARE_ISLESSOREQUAL(lenof(_heigthTexSizeLog2),lenof(texSize));
  for (int i=0; i<lenof(_heigthTexSizeLog2); i++) texSize[i] = _heigthTexSizeLog2[i];
  for (int i=lenof(_heigthTexSizeLog2); i<lenof(texSize); i++) texSize[i] = 0;
  PB_SetPixelShaderConstantF(PSC_NoTexSizeLog2, texSize, lenof(texSize)/floatPerVector);
}


//////////////////////////////////////////////////////////////////////////

void PBISetPixelShaderConstantWater::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
//  //PROFILE_DX_SCOPE(3dPiC);
//  // water pixel shader requires some special constants to be set
//  // fog color is quite good approximation of sky color
//  // we could also store the sky reflection color separately
//  float hdr = _engine->GetHDRFactor();
//  float skyColor[]={_engine->_fogColor.R()*hdr,_engine->_fogColor.G()*hdr,_engine->_fogColor.B()*hdr,0.5f};
//  float skyTopColor[]={_engine->_skyTopColor.R()*hdr,_engine->_skyTopColor.G()*hdr,_engine->_skyTopColor.B()*hdr,0.5f};
//  PB_SetPixelShaderConstantF(PSC_GroundReflColor,skyColor,1);
//  PB_SetPixelShaderConstantF(PSC_SkyReflColor,skyTopColor,1);

  // Environmental map color modificator
  Color sky, aroundSun;
  _engine->_sceneProps->GetSkyColor(sky, aroundSun);
  Color skyColor = sky * _engine->GetAccomodateEye() * _engine->_modColor * EnvMapScale;
  PB_SetPixelShaderConstantF(PSC_GlassEnvColor, (float*)&skyColor, 1);

  // Wave color
  LightSun *sun = GScene->MainLight();
  float hdr = _engine->GetHDRFactor();
  Color waveColor = _engine->GetAccomodateEye()*(sun->GetDiffuse()+sun->GetAmbient())*0.5f*hdr;
  waveColor.SaturateMinMax();
  static float waveA = 0.4;
  waveColor.SetA(-waveA);
  PB_SetPixelShaderConstantF(PSC_WaveColor, (float *)&waveColor, 1);
}

//////////////////////////////////////////////////////////////////////////

void PBISetPixelShaderConstantWaterSimple::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  //PROFILE_DX_SCOPE(3dPiC);
  LightSun *sun = GScene->MainLight();
  // water pixel shader requires some special constants to be set
  // fog color is quite good approximation of sky color
  Color lighting = sun->SimulateDiffuse(0.5)*_engine->GetAccomodateEye()*_engine->_modColor;
  float hdr = _engine->GetHDRFactor();
  Color groundLitColor=Color(0.15,0.2,0.1)*lighting;
  float groundColor[]={groundLitColor.R(),groundLitColor.G(),groundLitColor.B(),0.5f};
  float skyTopColor[]={_engine->_skyTopColor.R()*hdr,_engine->_skyTopColor.G()*hdr,_engine->_skyTopColor.B()*hdr,0.5f};
  PB_SetPixelShaderConstantF(PSC_GroundReflColor,groundColor,1);
  PB_SetPixelShaderConstantF(PSC_SkyReflColor,skyTopColor,1);
  Color waveColor = _engine->GetAccomodateEye()*(sun->GetDiffuse()+sun->GetAmbient())*0.5*_engine->_modColor;
  waveColor.SaturateMinMax();
  static float waveA = 0.4;
  waveColor.SetA(-waveA);
  PB_SetPixelShaderConstantF(PSC_WaveColor, (float *)&waveColor, 1);
}

//////////////////////////////////////////////////////////////////////////

void PBISetPixelShaderConstantGlass::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  // Environmental map color modificator
  LightSun *sun = GScene->MainLight();
  Color lighting = sun->SimulateDiffuse(0.5) * _engine->GetAccomodateEye() * _engine->_modColor * EnvMapScale;
  PB_SetPixelShaderConstantF(PSC_GlassEnvColor, (float*)&lighting, 1);

  // Material specular color
  PB_SetPixelShaderConstantF(PSC_GlassMatSpecular, (float*)&_matSpecular, 1);
}

//////////////////////////////////////////////////////////////////////////

void PBISetPixelShaderConstantSuper::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  
  // Environmental map color modificator
  LightSun *sun = GScene->MainLight();
  Color lighting = sun->SimulateDiffuse(0.5) * _engine->GetAccomodateEye() * _engine->_modColor * EnvMapScale;
  PB_SetPixelShaderConstantF(PSC_GlassEnvColor, (float*)&lighting, 1);

  // Material specular color
  PB_SetPixelShaderConstantF(PSC_GlassMatSpecular, (float*)&_matSpecular, 1);

  // Main light in world coordinates
  Vector3Val dir = _engine->_cbState._direction;
  D3DXVECTOR4 direction(-dir.X(), -dir.Y(), -dir.Z(), 1.0f);
  PB_SetPixelShaderConstantF(PSC_WLight, (float*)&direction, 1);
}

//////////////////////////////////////////////////////////////////////////

void PBISetPixelShaderConstantTreePRT::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;

  // Local base vectors for RGBA (RGB vector are 35 degrees from XZ plane and thus roughly 45 degrees from each other)
  Vector3 localVector[4];
  localVector[0] = Vector3(0.81915204429,  0.57357643635,  0);
  localVector[1] = Vector3(-0.40957602214, 0.57357643635,  0.70940647991);
  localVector[2] = Vector3(-0.40957602214, 0.57357643635,  -0.70940647991);
  localVector[3] = Vector3(0,              -1,             0);

  // Get the light and up direction in the object space
  Vector3 tDirection = _engine->TransformAndNormalizeVectorToObjectSpace(_engine->_cbState._direction);
  Vector3 tDirectionUp = _engine->TransformAndNormalizeVectorToObjectSpace(VUp);

  // Get required light values
  LightSun *sun = GScene->MainLight();
  Color diffuse = _engine->_cbState._lightDiffuse * _matDiffuse;
  Color ground = sun->GroundReflection() * diffuse;
  Color ae = _matEmmisive * _engine->GetAccomodateEye() + _engine->_cbState._lightAmbient * _matAmbient;
  Color ge = _matEmmisive * _engine->GetAccomodateEye() + sun->GroundReflection() * _engine->_cbState._lightAmbient * _matAmbient;

  // Calculate light values for the local vectors
  Color localLightValueDirect[4];
  Color localLightValueIndirect[4];
  for (int i = 0; i < 4; i++)
  {
    localLightValueDirect[i] = HBlack;
    localLightValueIndirect[i] = HBlack;

    // Include diffuse color
    localLightValueDirect[i] += diffuse * max(localVector[i].DotProduct(-tDirection), 0.0f);

    // Include diffuse back color
    localLightValueIndirect[i] += ground * max(-localVector[i].DotProduct(-tDirection), 0.0f);

    // Include ambient color
    float lerpFactor = (localVector[i].DotProduct(tDirectionUp) + 1.0f) * 0.5f;
    localLightValueIndirect[i] += ge * (1.0f-lerpFactor) + ae * lerpFactor;
  }

  // Set up the PRT constants
  Color prtC[3*2];
  prtC[0] = Color(localLightValueDirect[0].R(), localLightValueDirect[1].R(), localLightValueDirect[2].R(), localLightValueDirect[3].R());
  prtC[1] = Color(localLightValueDirect[0].G(), localLightValueDirect[1].G(), localLightValueDirect[2].G(), localLightValueDirect[3].G());
  prtC[2] = Color(localLightValueDirect[0].B(), localLightValueDirect[1].B(), localLightValueDirect[2].B(), localLightValueDirect[3].B());
  prtC[3] = Color(localLightValueIndirect[0].R(), localLightValueIndirect[1].R(), localLightValueIndirect[2].R(), localLightValueIndirect[3].R());
  prtC[4] = Color(localLightValueIndirect[0].G(), localLightValueIndirect[1].G(), localLightValueIndirect[2].G(), localLightValueIndirect[3].G());
  prtC[5] = Color(localLightValueIndirect[0].B(), localLightValueIndirect[1].B(), localLightValueIndirect[2].B(), localLightValueIndirect[3].B());
  PB_SetPixelShaderConstantF(PSC_PRTConstantDirect, (float*)prtC, 3*2); // PSC_PRTConstantIndirect is filled out as well
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShader::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetVertexShader(_vs);
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexDeclaration::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
#if DO_TEX_STATS>=2
  if (LogStatesOnce)
  {
    /*
    D3DVERTEXELEMENT9 decl[64];
    UINT numElements = lenof(decl);
    decl->GetDeclaration()
    LogF("SetVertexDeclaration %d",dtaV->VertexSize());
    */

    for (int i=0; i<VDCount; i++) for (int j=0; j<VSDVCount; j++)
    {
      if (_engine->_vertexDeclD[i][j]==_decl)
      {
        LogF("SetVertexDeclaration %d,%d",i,j);
        goto Break;
      }
    }
    LogF("SetVertexDeclaration ???");
    Break:;
  }
#endif

  PB_SetVertexDeclaration(_decl);
}

//////////////////////////////////////////////////////////////////////////

#if _ENABLE_CHEATS
extern int LimitTriCount;
extern int LimitDIPCount;
#else
const int LimitTriCount = -1;
const int LimitDIPCount = -1;
#endif

void PBIDrawIndexedPrimitive::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  // no queueing, direct drawing
  VertexBufferD3D9 *buf = static_cast<VertexBufferD3D9 *>(ctx._sMesh->GetVertexBuffer());

  DoAssert(buf->_sections.Size() >= _end);

#ifdef DONT_USE_DYNBUFFERS
  if (buf->_dynamic)
  {
    _engine->GetDirect3DDevice()->DrawPrimitiveUP(
      D3DPT_TRIANGLELIST,
      _engine->_dynVBuffer._geometry.Size() / 3,
      _engine->_dynVBuffer._geometry.Data(),
      sizeof(SVertex9)
    );
  }
  else
#endif
  {
    int beg = _beg;
    while (beg < _end)
    {
      // check which sections can be concatenated could be performed here

      // scan sections to provide reasonable values for vertex range
      int endDraw;
      const VBSectionInfo &siBeg = buf->_sections[beg];
      int begVertex = siBeg.begVertex, endVertex = siBeg.endVertex;
      for (endDraw = beg + 1; endDraw < _end; endDraw++)
      {
        const VBSectionInfo &siCur = buf->_sections[endDraw-1];
        const VBSectionInfo &siNxt = buf->_sections[endDraw];
#ifdef STRIPIZATION_ENABLED
        Assert(sMesh.GetFaceArrayType() != FATStripized);
#endif
        if (siCur.end!=siNxt.beg)  break;
        saturateMin(begVertex,siCur.begVertex);
        saturateMax(endVertex,siCur.endVertex);
      }
      Assert (endDraw>beg);
      Assert (endVertex>=begVertex);
      const VBSectionInfo &siEnd = buf->_sections[endDraw-1];

      // all attributes prepared


      //const ShapeSection &sec = sMesh.GetSection(section);
      // actualy draw all data
      // note: min index, max index is not used by HW T&L
      //LogF("TLStart");
      // get vbuffer size
      //PROFILE_DX_SCOPE(3drTL);
#if !_RELEASE
      if (endVertex + _engine->_cbState._vOffsetLast > _engine->_cbState._vBufferSize)
      {
        LogF("vrange %d..%d",begVertex,endVertex);
        LogF("offset %d",_engine->_cbState._vOffsetLast);
        LogF("vb size %d",_engine->_cbState._vBufferSize);
        Fail("Vertex out of range");
        return;
      }
#endif
      //DoAssert(_engine->_d3dFrameOpen);
      int nInstances = ctx._instancesCount > 0 ? ctx._instancesCount : 1;
      // note: begVertex and endVertex do not work when nInstances>1
#if DO_TEX_STATS
      if (LogStatesOnce)
      {
        LogF
          (
          "  %d: Draw TL .... %dx%d tris (%d..%d,%s)",
          DPrimIndex++,
          (siEnd.end-siBeg.beg)/3,nInstances,
          beg,end,
          (const char *)sMesh.GetSection(beg).GetDebugText()
          );
      }
#endif


      int triangleCount;

#if 0 // DO_TEX_STATS
      if (LogStatesOnce)
      {
        LogF("Render states used for this section:");
        for (int i = 0; i < _renderState.Size(); i++)
        {
          if (_renderState[i].value != 0xFFFFFFFF)
          {
            LogF("%d, %d", i, _renderState[i].value);
          }
        }
      }
#endif

#ifdef STRIPIZATION_ENABLED
      if ((sMesh.GetFaceArrayType() == FATStripized) && EnableStrips)
      {
        PROFILE_DX_SCOPE_DETAIL(3dDTS);
        triangleCount = (siEnd.end-siBeg.beg - 2) * nInstances + (nInstances - 1) * 4;
        // If there is odd number of triangles in one section, then add the even-maker triangles
        if ((siEnd.end-siBeg.beg) & 1)
        {
          triangleCount += (nInstances - 1) * 1;
        }
        ret =_d3DDevice->DrawIndexedPrimitive
          (
          D3DPT_TRIANGLESTRIP,
          _vOffsetLast,
          begVertex,endVertex-begVertex,
          siBeg.beg+_iOffset,triangleCount
          );
      }
      else
#endif
      {
#ifdef STRIP_VISUALISATION
#ifdef STRIPIZATION_ENABLED
        if (sMesh.GetFaceArrayType() == FATStripized)
        {
          const ShapeSection &begSection = sMesh.GetSection(beg);
          const ShapeSection &endSection = sMesh.GetSection(end - 1);
          Offset o = begSection.beg;
          Poly face = sMesh.Face(o);
          int triangleOffset = 0;
          int iteration = 0;
          while (o < endSection.end)
          {

            // Skip degenerated
            while ((o < endSection.end) &&
              ((face.GetVertex(0) == face.GetVertex(1)) || 
              (face.GetVertex(1) == face.GetVertex(2)) ||
              (face.GetVertex(2) == face.GetVertex(0))))
            {
              triangleOffset++;
              sMesh.NextFace(o);
              if (o < endSection.end) face = sMesh.Face(o);
              Assert(face.N() == 3);
            }

            // Get nondegenerated
            int triangleCount = 0;
            while ((o < endSection.end) &&
              ((face.GetVertex(0) != face.GetVertex(1)) &&
              (face.GetVertex(1) != face.GetVertex(2)) &&
              (face.GetVertex(2) != face.GetVertex(0))))
            {
              triangleCount++;
              sMesh.NextFace(o);
              if (o < endSection.end) face = sMesh.Face(o);
              Assert(face.N() == 3);
            }

            // Draw triangles
            if (triangleCount > 0)
            {
              float coefR = iteration * 6.841f - (int)(iteration * 6.841f);
              float coefG = iteration * 2.483f - (int)(iteration * 2.483f);
              float coefB = iteration * 9.672f - (int)(iteration * 9.672f);
              float maxColor[4] = {
                _texture0MaxColor.R() * coefR,
                _texture0MaxColor.G() * coefG,
                _texture0MaxColor.B() * coefB,
                _texture0MaxColor.A()
              };

              //          D3DSetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT);
              //          D3DSetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
              //          D3DSetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_POINT);
              {
                //PROFILE_DX_SCOPE(3dPiC);
                SetPixelShaderConstantF(PSC_MaxColor, maxColor, 1);
                //SetVertexShaderConstantF(VSC_Tex0MaxColor, maxColor, 1);
              }
              {
                PROFILE_DX_SCOPE_DETAIL(3dDTV);
                _d3DDevice->DrawIndexedPrimitive(
                  D3DPT_TRIANGLELIST,
                  _vOffsetLast,
                  begVertex,
                  endVertex-begVertex,
                  siBeg.beg+_iOffset+triangleOffset*3,
                  triangleCount
                  );
              }
              iteration++;
            }

            // Move the offset
            triangleOffset += triangleCount;
          }
        }
        else
#endif
#endif
        {
          PROFILE_DX_SCOPE_DETAIL(3dDIP);
          triangleCount = ((siEnd.end-siBeg.beg)/3) * nInstances;
          if (LimitTriCount>=0 && triangleCount>LimitTriCount)
          {
            triangleCount = LimitTriCount;
          }
          if ((LimitDIPCount < 0) || (_engine->_dipCount < LimitDIPCount))
          {
            int instanceOffset = ctx._sMesh->NVertex() * (nInstances - 1);

            // Draw primitives (in more steps if necessary)
            const int MaxPrimitiveCount = 65535;
            int drawnPrimitives = 0;
            do
            {
              int tCount = min(triangleCount - drawnPrimitives, MaxPrimitiveCount);
              _engine->GetDirect3DDevice()->DrawIndexedPrimitive(
                D3DPT_TRIANGLELIST,
                _engine->_cbState._vOffsetLast,
                begVertex,endVertex - begVertex + instanceOffset,
                siBeg.beg+_engine->_cbState._iOffset+drawnPrimitives*3, tCount
              );
              _engine->_dipCount++;
#if _ENABLE_PERFLOG
              ADD_COUNTER(dPrim,1);
#endif
              drawnPrimitives += tCount;
            } while (drawnPrimitives < triangleCount);

#if _ENABLE_PERFLOG
            ADD_COUNTER(tris,triangleCount);
#endif
          }
        }
      }
      //LogF("TLEnd");
      beg = endDraw;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

void PBISetBias::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  // Simulate z-bias using perspective matrix
  _engine->_cbState._bias = _bias;
  if (_engine->_cbState._tlActive==EngineDD9::TLEnabled)
  {
    // Setup projection matrix
    _engine->ProjectionChanged();
  }
}

//////////////////////////////////////////////////////////////////////////

PushBufferD3D9::PushBufferD3D9(int key) : PushBuffer(key)
{
}

void PushBufferD3D9::Draw(const PBContext &ctx) const
{
  for (int i = 0; i < _pbItemList.Size(); i++)
  {
    _pbItemList[i]->Do(ctx);
  }
}

size_t PushBufferD3D9::GetBufferSize() const
{
  size_t total = 0;

  for (int i = 0; i < _pbItemList.Size(); i++)
  {
    total += _pbItemList[i]->GetBufferSize();
  }

  return total;
}

#endif