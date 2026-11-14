#include "../wpch.hpp"
#if !defined _RESISTANCE_SERVER && _ENABLE_DX10

#include "pushBufferT.hpp"
#include "engddT.hpp"
#include "txtD3DT.hpp"
#include "../keyInput.hpp"
#include "../dikCodes.h"
#include "../textbank.hpp"
#include "../Shape/material.hpp"
#include "../scene.hpp"
#include <El/Common/perfLog.hpp>

EngineDDT *PBItemT::_engine;

#define PB_SetVertexShaderConstantF(register,data,count) _engine->SetVertexShaderConstantF(register, data, count)
#define PB_SetVertexShaderConstantB(register,data,count) _engine->SetVertexShaderConstantB(register, data, count)
#define PB_SetVertexShaderConstantI(register,data,count) _engine->SetVertexShaderConstantI(register, data, count)
#define PB_SetRenderState(state,data) _engine->D3DSetRenderState(state,data)
#define PB_SetSamplerState(stage,state,value) _engine->D3DSetSamplerState(stage, state, value)
#define PB_SetPixelShader(ps) _engine->SetPixelShader(ps)
#define PB_SetPixelShaderConstantF(register,data,count) _engine->SetPixelShaderConstantF(register, data, count)
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

void PBISetVertexShaderConstantT::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetVertexShaderConstantF(_startRegister, _data, 1);
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstant2T::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetVertexShaderConstantF(_startRegister, (float*)_data, 2);
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstantBT::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetVertexShaderConstantB(_startRegister, &_data, 1);
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstantB2T::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetVertexShaderConstantB(_startRegister, _data, 2);
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstantIT::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetVertexShaderConstantI(_startRegister, _data, 1);
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstantMaxColorT::Do(const PBContext &ctx) const
{
#if _ENABLE_CHEATS && USE_PUSHBUFFERS
  if (GInput.GetCheat3ToDo(DIK_L))
  {
    DummyPBLow = !DummyPBLow;
    GlobalShowMessage(500, "DummyPBLow %s", DummyPBLow ? "On" : "Off");
  }
#endif
  if (DummyPBLow) return;
  if (_texture0MaxColorSeparate)
  {
    PB_SetVertexShaderConstantF(VSC_Tex0MaxColor, (float*)&HWhite, 1);
  }
  else
  {
    PB_SetVertexShaderConstantF(VSC_Tex0MaxColor, (float*)&_engine->_texture0MaxColor, 1);
  }
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstantMLSunT::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  // Setup directional light constants
  Color diffuse = _engine->_lightDiffuse * _matDiffuse;
  diffuse.SetA(_matDiffuse.A());
  PB_SetVertexShaderConstantF(VSC_LDirectionD, (float*)&diffuse, 1);
  Color specular = _engine->_lightSpecular * _matSpecular;
  PB_SetVertexShaderConstantF(VSC_LDirectionS, (float*)&specular, 1);
  Vector3 tDirection = _engine->TransformAndNormalizeVectorToObjectSpace(_engine->_direction);
  D3DXVECTOR4 direction(tDirection.X(), tDirection.Y(), tDirection.Z(), 1.0f);
  PB_SetVertexShaderConstantF(VSC_LDirectionTransformedDir, (float*)&direction, 1);

  // Get the scene main light
  LightSun *sun = GScene->MainLight();

  // Setup constant for ground color (for back reflection purposes)
  ColorVal groundColor = sun->GroundReflection();
  PB_SetVertexShaderConstantF(VSC_LDirectionGround, (float*)&groundColor, 1);

  // Set ambient, emmisive and dforced constants
  if (_vsIsAS)
  {
    Color zero = Color(0, 0, 0, 0);
    PB_SetVertexShaderConstantF(VSC_AE, (float*)&zero, 1);
    PB_SetVertexShaderConstantF(VSC_DForced, (float*)&zero, 1);
  }
  else
  {
    Color newA_E = _matEmmisive * _engine->GetAccomodateEye();
    Color newDForced = HBlack;

    // Add ambient color
    newA_E = newA_E + _engine->_lightAmbient * _matAmbient;

    // Set the diffuse back coefficient
    float backCoef = _matForcedDiffuse.A();
    newA_E.SetA(backCoef);

    // Add DForced color
    newDForced = newDForced + _engine->_lightDiffuse * _matForcedDiffuse;

    PB_SetVertexShaderConstantF(VSC_AE, (float*)&newA_E, 1);
    PB_SetVertexShaderConstantF(VSC_DForced, (float*)&newDForced, 1);
  }
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstantMLNoneT::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  Color black = HBlack;
  PB_SetVertexShaderConstantF(VSC_LDirectionS, (float*)&black, 1);
  PB_SetVertexShaderConstantF(VSC_LDirectionD, (float*)&black, 1);
  PB_SetVertexShaderConstantF(VSC_LDirectionGround, (float*)&black, 1);

  Vector3 tDirection = _engine->TransformAndNormalizeVectorToObjectSpace(_engine->_direction);
  D3DXVECTOR4 direction(tDirection.X(), tDirection.Y(), tDirection.Z(), 1.0f);
  PB_SetVertexShaderConstantF(VSC_LDirectionTransformedDir, (float*)&direction, 1);

  if (_vsIsAS)
  {
    Color zero = Color(0, 0, 0, 0);
    PB_SetVertexShaderConstantF(VSC_AE, (float*)&zero, 1);
    PB_SetVertexShaderConstantF(VSC_DForced, (float*)&zero, 1);
  }
  else
  {
    Color newA_E = _matEmmisive * _engine->GetAccomodateEye();
    Color newDForced = HBlack;
    PB_SetVertexShaderConstantF(VSC_AE, (float*)&newA_E, 1);
    PB_SetVertexShaderConstantF(VSC_DForced, (float*)&newDForced, 1);
  }
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstantSAFRT::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  float value[4]={
    _matPower, _matAmbient.A(), _engine->_fogEnd, 1.0f / (_engine->_fogEnd - _engine->_fogStart)
  };
  PB_SetVertexShaderConstantF(VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart, value, 1);
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexShaderConstantFFART::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  float value[4]={
    0,_engine->_expFogCoef, _engine->_aFogEnd,
    _engine->_aFogEnd > _engine->_aFogStart ? 1.0f / (_engine->_aFogEnd - _engine->_aFogStart) : 1.0f
  };
  PB_SetVertexShaderConstantF(VSC_Free_ExpFog_AFogEnd_RCPAFogEndMinusAFogStart, value, 1);
}

//////////////////////////////////////////////////////////////////////////

void ConvertMatrixTransposedT(D3DXMATRIX &mat, Matrix4Val src);

void PBISetVertexShaderConstantLWSMT::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;

  // Calculate the local world space matrix
  Matrix4 modelToLWS;
  {
    Matrix4 localSpace;
    localSpace.SetOrientation(M3Identity);
    localSpace.SetPosition(_engine->_sceneProps->GetCameraPosition());
    modelToLWS = localSpace.InverseRotation() * _engine->_world;
  }

  // Pass the LWS matrix to the shader
  {
    D3DXMATRIX lwsMatrix;
    ConvertMatrixTransposedT(lwsMatrix, modelToLWS);
    PB_SetVertexShaderConstantF(VSC_LWSMatrix, (float*)&lwsMatrix, 3);
  }
}

//////////////////////////////////////////////////////////////////////////

void PBISetRenderStateT::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetRenderState(_state, _data);
}

//////////////////////////////////////////////////////////////////////////

void PBISetSamplerStateT::Do(const PBContext &ctx) const
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

void PBISetTexture0T::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  DoAssert(_texture); // At least white texture ought to be set
  
  Color maxColor = _texture->GetMaxColor();
  const TextureD3DT *loadedTexture;
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

void PBISetTextureT::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  TextureD3DHandle tHandle;
  if (_texture)
  {
    const TextureD3DT *loadedTexture;
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

void PBISetPixelShaderT::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetPixelShader(_ps);
}

//////////////////////////////////////////////////////////////////////////

void PBISetPixelShaderConstantT::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetPixelShaderConstantF(_startRegister, _data, 1);
}

void PBISetPixelShaderConstantDBST::Do(const PBContext &ctx) const
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
    diffuse = _engine->_lightDiffuse * _matDiffuse;

    // If pixel shader is not the macro one, then include the _texture0MaxColor into diffuse
    if (!_texture0MaxColorSeparate)
    {
      diffuse = diffuse * _engine->_texture0MaxColor;
    }

    // Copy alpha from material diffuse alpha
    diffuse.SetA(_matDiffuse.A());
  }

  // Calculate diffuse back color
  {
    float backCoef = _matForcedDiffuse.A();
    diffuseBack = diffuse * backCoef;
    diffuseBack.SetA(backCoef);
  }

  // Calculate specular color
  /// multiply by 2 so that pixel shader can avoid _x2
  specular = _engine->_lightSpecular * _matSpecular * 2.0f;
  specular.SetA(_matPower);

  // Set the pixel shader constant
  PB_SetPixelShaderConstantF(PSC_Diffuse, (float*)difBakSpec, 3);
}

//////////////////////////////////////////////////////////////////////////

void PBISetPixelShaderConstantDBSZeroT::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  static const Color zeroes[3]={Color(0, 0, 0, 0), Color(0, 0, 0, 0), Color(0, 0, 0, 0)};
  PB_SetPixelShaderConstantF(PSC_Diffuse, (float*)zeroes, 3);
}

//////////////////////////////////////////////////////////////////////////

void PBISetPixelShaderConstant_A_DFORCED_E_SunT::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  // Emissive in the result
  Color newA_Dforced_E = _matEmmisive * _engine->GetAccomodateEye();

  // Add ambient and forced diffuse to the A_DForced_E
  Color ambient = _engine->_lightAmbient * _matAmbient + _engine->_lightDiffuse * _matForcedDiffuse;
  newA_Dforced_E = (newA_Dforced_E + ambient);

  // If pixel shader is not the macro one, then include the _texture0MaxColor into newA_Dforced_E
  if (!_texture0MaxColorSeparate)
  {
    newA_Dforced_E = newA_Dforced_E * _engine->_texture0MaxColor;
  }

  // Set the diffuse back coefficient
  float backCoef = _matForcedDiffuse.A();
  newA_Dforced_E.SetA(backCoef);

  PB_SetPixelShaderConstantF(PSC_ADForcedE, (float*)&newA_Dforced_E, 1);
}

//////////////////////////////////////////////////////////////////////////

void PBISetPixelShaderConstant_A_DFORCED_E_NoSunT::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  // Emissive in the result
  Color newA_Dforced_E = _matEmmisive * _engine->GetAccomodateEye();

  PB_SetPixelShaderConstantF(PSC_ADForcedE, (float*)&newA_Dforced_E, 1);
}

//////////////////////////////////////////////////////////////////////////

// Coefficient to scale by the content of an environmental map (to represent bigger range of values)
static float EnvMapScale = 1.0f;

void PBISetPixelShaderConstantWaterT::Do(const PBContext &ctx) const
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

void PBISetPixelShaderConstantWaterSimpleT::Do(const PBContext &ctx) const
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

void PBISetPixelShaderConstantGlassT::Do(const PBContext &ctx) const
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

void PBISetVertexShaderT::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  PB_SetVertexShader(_vs);
}

//////////////////////////////////////////////////////////////////////////

void PBISetVertexDeclarationT::Do(const PBContext &ctx) const
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
extern int LimitTriCountT;
extern int LimitDIPCountT;
#else
const int LimitTriCountT = -1;
const int LimitDIPCountT = -1;
#endif

void PBIDrawIndexedPrimitiveT::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  // no queueing, direct drawing
  VertexBufferD3DT *buf = static_cast<VertexBufferD3DT *>(ctx._sMesh->GetVertexBuffer());

  DoAssert(buf->_sections.Size() >= _end);

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
    if (endVertex + _engine->_vOffsetLast > _engine->_vBufferSize)
    {
      LogF("vrange %d..%d",begVertex,endVertex);
      LogF("offset %d",_engine->_vOffsetLast);
      LogF("vb size %d",_engine->_vBufferSize);
      Fail("Vertex out of range");
      return;
    }
#endif
    DoAssert(_engine->_d3dFrameOpen);
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

    {
      {
        PROFILE_DX_SCOPE_DETAIL(3dDIP);
        triangleCount = ((siEnd.end-siBeg.beg)/3) * nInstances;
        if (LimitTriCountT>=0 && triangleCount>LimitTriCountT)
        {
          triangleCount = LimitTriCountT;
        }
        if ((LimitDIPCountT < 0) || (_engine->_dipCount < LimitDIPCountT))
        {
          int instanceOffset = ctx._sMesh->NVertex() * (nInstances - 1);

          // Draw primitives (in more steps if necessary)
          const int MaxPrimitiveCount = 65535;
          int drawnPrimitives = 0;
          do
          {
            int tCount = min(triangleCount - drawnPrimitives, MaxPrimitiveCount);
            _engine->DrawIndexedPrimitive
              (
              D3DPT_TRIANGLELIST_OLD,
              _engine->_vOffsetLast,
              begVertex,endVertex - begVertex + instanceOffset,
              siBeg.beg+_engine->_iOffset+drawnPrimitives*3, tCount
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
    beg = endDraw;
  }
}

//////////////////////////////////////////////////////////////////////////

void PBISetBiasT::Do(const PBContext &ctx) const
{
  if (DummyPBLow) return;
  // Simulate z-bias using perspective matrix
  _engine->_bias = _bias;
  if (_engine->_tlActive==EngineDDT::TLEnabled)
  {
    // Setup projection matrix
    _engine->ProjectionChanged();
  }
}

//////////////////////////////////////////////////////////////////////////

PushBufferD3DT::PushBufferD3DT(int key) : PushBuffer(key)
{
}

void PushBufferD3DT::Draw(const PBContext &ctx) const
{
  for (int i = 0; i < _pbItemList.Size(); i++)
  {
    _pbItemList[i]->Do(ctx);
  }
}

size_t PushBufferD3DT::GetBufferSize() const
{
  size_t total = 0;

  for (int i = 0; i < _pbItemList.Size(); i++)
  {
    total += _pbItemList[i]->GetBufferSize();
  }

  return total;
}

#endif