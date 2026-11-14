#include "../wpch.hpp"
#if !defined _RESISTANCE_SERVER && _ENABLE_DX10
#include "postProcessT.hpp"
#include "engddT.hpp"
#include "../textbank.hpp"
#include "../diagModes.hpp"
#include <Es/Strings/bString.hpp>
#include "../Shape/shape.hpp"
#include <El/Math/mathStore.hpp>
#include "ShaderSources/common.h"

#if _ENABLE_COMPILED_SHADER_CACHE
#include "ShaderCompileCacheT.h"
#endif

EngineDDT *EngineDDT::PostProcess::_engine;

void EngineDDT::PostProcess::Prepare()
{
  _engine->FlushQueues();
  // we want all render states to be set as needed
  // postprocess coordinates are given in 3D, but there is no camera space
  _engine->SwitchTL(TLViewspace);

  // Set standard render states (it is possible to change it in every descendant, but
  // it must return it also back)
  // No alpha blending - we will do everything in the pixel shader
  // Use point filter
  EngineShapeProperties prop;
  _engine->DoPrepareTriangle(NULL,TexMaterialLODInfo(NULL, 0),0,NoZBuf|NoZWrite|ClampU|ClampV|FilterPoint,prop);
  _engine->DoShadowsStatesSettings(SS_Transparent,false);
}

bool EngineDDT::PostProcess::CompilePixelShader(
  const QOStrStream &hlsl, const char *psName, ComRef<ID3D10PixelShader> &ps,
  bool fullPrec
) const
{
  return false;
  // Direct3D 10 NEEDS UPDATE 
//   ComRef<ID3DXBuffer> shader;
//   ComRef<ID3DXBuffer> errorMsgs;
//   
//   HRESULT hr;
//   DWORD flags = fullPrec ? 0 : D3DXSHADER_PARTIALPRECISION;
// 
//   hr = D3DXCompileShader(hlsl.str(), hlsl.pcount(), NULL, &_engine->_hlslInclude, psName, "ps_3_0", flags, shader.Init(), errorMsgs.Init(), NULL);
//   if (FAILED(hr))
//   {
//     LogF("Error: %x in D3DXCompileShader (%s).", hr, psName);
//     if (errorMsgs.NotNull())
//     {
//       LogF("%s", (const char*)errorMsgs->GetBufferPointer());
//     }
//     return false;
//   };
//   if (FAILED(_engine->GetDirect3DDevice()->CreatePixelShader((const DWORD*)shader->GetBufferPointer(), ps.Init())))
//   {
//     LogF("Error: %s shader creation failed.",psName);
//     return false;
//   }
// 
//   return true;
}

bool EngineDDT::PostProcess::CompileVertexShader(
  const QOStrStream &hlsl, const char *vsName, ComRef<ID3D10VertexShader> &vs
) const
{
  return false;
  // Direct3D 10 NEEDS UPDATE 
//   ComRef<ID3DXBuffer> shader;
//   ComRef<ID3DXBuffer> errorMsgs;
//   DWORD flags = DebugVS ? D3DXSHADER_SKIPOPTIMIZATION|D3DXSHADER_DEBUG : 0;
//   
//   HRESULT hr;
//   hr = D3DXCompileShader(hlsl.str(), hlsl.pcount(), NULL, &_engine->_hlslInclude, vsName, "vs_3_0", flags, shader.Init(), errorMsgs.Init(), NULL);
//   if (FAILED(hr))
//   {
//     LogF("Error: %x in D3DXCompileShader (%s).", hr, vsName);
//     if (errorMsgs.NotNull())
//     {
//       LogF("%s", (const char*)errorMsgs->GetBufferPointer());
//     }
//     return false;
//   };
//   if (FAILED(_engine->GetDirect3DDevice()->CreateVertexShader((const DWORD*)shader->GetBufferPointer(), vs.Init())))
//   {
//     LogF("Error: %s shader creation failed.",vsName);
//     return false;
//   }
// 
//   return true;
}

//////////////////////////////////////////////////////////////////////////

void EngineDDT::PPSimple::Prepare()
{
  // Direct3D 10 NEEDS UPDATE 
//   base::Prepare();
// 
//   // Set the stream source
//   _engine->SetStreamSource(_vb, sizeof(Vertex), 4);
// 
//   // Set the vertex declaration
//   _engine->SetVertexDeclaration(_vd);
// 
//   // Set the vertex shader
//   _engine->SetVertexShader(_vs);
// 
//   // Set the pixel shader
//   _engine->SetPixelShader(_ps);
}

bool EngineDDT::PPSimple::Init(const QOStrStream &hlsl, const QOStrStream &vs, const char *vsName)
{
  return false;
  // Direct3D 10 NEEDS UPDATE 
//   if (!base::Init(hlsl,vs)) return false;
// 
//   // Create the VB
//   ComRef<IDirect3DVertexBuffer9Old> vb;
//   if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(Vertex) * 4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
//   {
//     LogF("Error: Vertex buffer creation failed.");
//     return false;
//   }
//   Vertex *v;
//   float offsetU = 1.0f / _engine->Width() * 0.5f; //See the Nearest-Point Sampling in the DX9 documentation
//   float offsetV = 1.0f / _engine->Height() * 0.5f;
//   vb->Lock(0, 0, (void**)&v, 0);
//   v[0].pos = Vector3(-0.5, -0.5, 0.5); v[0].t0.u = 0 + offsetU; v[0].t0.v = 1 + offsetV;
//   v[1].pos = Vector3(-0.5,  0.5, 0.5); v[1].t0.u = 0 + offsetU; v[1].t0.v = 0 + offsetV;
//   v[2].pos = Vector3( 0.5, -0.5, 0.5); v[2].t0.u = 1 + offsetU; v[2].t0.v = 1 + offsetV;
//   v[3].pos = Vector3( 0.5,  0.5, 0.5); v[3].t0.u = 1 + offsetU; v[3].t0.v = 0 + offsetV;
//   vb->Unlock();
// 
//   // Create the VD
//   const D3DVERTEXELEMENT9 vertexDecl[] =
//   {
//     {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
//     {0, 12, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
//     D3DDECL_END()
//   };
//   ComRef<ID3D10InputLayout> vd;
//   if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDecl, vd.Init())))
//   {
//     LogF("Error: Vertex declaration creation failed.");
//     return false;
//   }
// 
//   // note: vertex shader is stored in PS.hlsl for simple effects
//   if (!CompileVertexShader(hlsl,vsName,_vs))
//   {
//     return false;
//   }
// 
//   _vb = vb;
//   _vd = vd;
// 
//   return true;
}

bool EngineDDT::PPFinal::Init(const QOStrStream &hlsl, const QOStrStream &vs)
{
  if (!base::Init(hlsl,vs,"VSPostProcess")) return false;

  if (!CompilePixelShader(hlsl,"PSPostProcessCopy", _ps))
  {
    return false;
  }
  return true;
}

void EngineDDT::PPFinal::Do(bool isLast)
{
  // Direct3D 10 NEEDS UPDATE 
//   // Draw
//   if (_engine->_currRenderTargetSurface!=_engine->_backBuffer)
//   {
//     PROFILE_SCOPE_GRF_EX(ppFin,pp,SCOPE_COLOR_YELLOW);
// 
//     bool inScene = _engine->_d3dFrameOpen;
//     if (!inScene) _engine->D3DBeginScene();
//     // StretchRect not possible here?
//     // Prepare drawing
//     Prepare();
// 
//     // Set temporary render states
//     SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE_OLD, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
//     SetRenderStateTemp rsFOGENABLE(D3DRS_FOGENABLE_OLD, FALSE);
// 
//     ComRef<ID3D10Texture2D> rt = _engine->GetCurrRenderTargetAsTexture();
//     _engine->SwitchToBackBufferRenderTarget();
//     SetTextureTemp tRenderTarget(0, rt);
//     SetSamplerStateTemp tempNoSRGB(0, D3DSAMP_SRGBTEXTURE_OLD, FALSE);
//     
//     // Drawing
//     _engine->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
//     
//     if (!inScene) _engine->D3DEndScene();
//   }
//   _engine->SwitchTL(TLDisabled);
}

//////////////////////////////////////////////////////////////////////////

int EngineDDT::PPStencilShadowsPri::_nPlates[NQualities]={4,4,2};


void EngineDDT::PPStencilShadowsPri::Prepare()
{
  // Direct3D 10 NEEDS UPDATE 
//   base::Prepare();
// 
//   // Set the stream source
//   int quality = toInt(3-_pars[ParQuality]);
//   saturate(quality,0,2);
//   int nPlates = _nPlates[quality];
//   if (_engine->IsSBEnabled()) nPlates = 1; // In SB shadows disappearing is performed via SB technique
//   _engine->SetStreamSource(_vb[quality], sizeof(Vertex), 6*nPlates);
// 
//   // Set the vertex declaration
//   _engine->SetVertexDeclaration(_vd);
// 
//   // Set the vertex shader
//   _engine->SetVertexShader(_vs);
// 
//   // Set the pixel shader
//   _engine->SetPixelShader(_ps);
}

bool EngineDDT::PPStencilShadowsPri::Init(const QOStrStream &hlsl, const QOStrStream &vs)
{
  return false;
  // Direct3D 10 NEEDS UPDATE 
//   if (!base::Init(hlsl,vs)) return false;
// 
//   for (int quality = 0; quality<NQualities; quality++)
//   {
//     int nPlates = _nPlates[quality];
//     if (_engine->IsSBEnabled()) nPlates = 1; // In SB shadows disappearing is performed via SB technique
//     
//     // Create the VB
//     ComRef<IDirect3DVertexBuffer9Old> vb;
//     if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(Vertex) * 6 * nPlates, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, vb.Init(), NULL)))
//     {
//       LogF("Error: Vertex buffer creation failed.");
//       return false;
//     }
//     void *vl;
//     float offsetU = 1.0f / _engine->Width() * 0.5f; //See the Nearest-Point Sampling in the DX9 documentation
//     float offsetV = 1.0f / _engine->Height() * 0.5f;
// 
//     vb->Lock(0, 0, (void**)&vl, 0);
//     Vertex * v= (Vertex *)vl;
//     float invNPlates = 1.0f / nPlates;
//     for (int i=0; i<nPlates; i++)
//     {
//       float z = (i+1) * invNPlates;
// 
//       float aDist = (nPlates-i) * invNPlates;
//       aDist *= aDist; // Make the interval of <0.25..1> to be <0.0625..1>
// 
//       // shadow intensity computation
//       static float overallIntensity = 0.80f;
//       int a = toInt(255*aDist*overallIntensity);
// 
//       v[0].pos = Vector3(-1, -1, z);
//       v[0].t0.u = 0 + offsetU;
//       v[0].t0.v = 1 + offsetV;
//       v[0].color = PackedColorRGB(PackedBlack,a);
//       
//       v[1].pos = Vector3(-1,  1, z);
//       v[1].t0.u = 0 + offsetU;
//       v[1].t0.v = 0 + offsetV;
//       v[1].color = PackedColorRGB(PackedBlack,a);
//       
//       v[2].pos = Vector3( 1, -1, z);
//       v[2].t0.u = 1 + offsetU;
//       v[2].t0.v = 1 + offsetV;
//       v[2].color = PackedColorRGB(PackedBlack,a);
//       
//       v[3].pos = Vector3( 1, -1, z);
//       v[3].t0.u = 1 + offsetU;
//       v[3].t0.v = 1 + offsetV;
//       v[3].color = PackedColorRGB(PackedBlack,a);
//       
//       v[4].pos = Vector3(-1,  1, z);
//       v[4].t0.u = 0 + offsetU;
//       v[4].t0.v = 0 + offsetV;
//       v[4].color = PackedColorRGB(PackedBlack,a);
//       
//       v[5].pos = Vector3( 1,  1, z);
//       v[5].t0.u = 1 + offsetU;
//       v[5].t0.v = 0 + offsetV;
//       v[5].color = PackedColorRGB(PackedBlack,a);
//       
//       v+= 6;
//     }
//     
//     vb->Unlock();
//     _vb[quality] = vb;
//   }
// 
//   // Create the VD
//   const D3DVERTEXELEMENT9 vertexDecl[] =
//   {
//     {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
//     {0, 12, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
//     {0, 20, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0},
//     D3DDECL_END()
//   };
//   ComRef<ID3D10InputLayout> vd;
//   if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDecl, _vd.Init())))
//   {
//     LogF("Error: Vertex declaration creation failed.");
//     return false;
//   }
// 
//   // Create the shaders
//   if (!CompileVertexShader(vs, "VSPostProcessStencilShadows", _vs)) return false;
//   if (!CompilePixelShader(hlsl, "PSPostProcessStencilShadowsPri", _ps)) return false;
// 
//   for (int i=0; i<NParameters; i++)
//   {
//     _pars[i] = 0;
//   }
// 
//   return true;
}

void EngineDDT::PPStencilShadowsPri::SetParameters(const float *pars, int nPars)
{
  if (nPars!=NParameters)
  {
    Fail("Bad nPars");
    return;
  }
  for (int i=0; i<NParameters; i++)
  {
    _pars[i] = pars[i];
  }
}


void EngineDDT::PPStencilShadowsPri::Do(bool isLast)
{
  // Direct3D 10 NEEDS UPDATE 
//   PROFILE_SCOPE_GRF_EX(ppStc,pp,SCOPE_COLOR_YELLOW);
// 
//   // Prepare drawing
//   Prepare();
//   
//   // First do the copy (to make sure same data is both in source and destination)
//   ComRef<ID3D10Texture2D> rt = _engine->CreateRenderTargetCopyAsTexture();
//   _engine->SetTexture(0, rt);
//   _engine->D3DSetSamplerState(0, D3DSAMP_SRGBTEXTURE_OLD, _engine->_rtIsSRGB);
//   
//   // Draw
//   {
//     // Set temporary constants
//     SetPixelShaderConstantTemp psc6(6, D3DXVECTOR4(_engine->_shadowLA.R(),_engine->_shadowLA.G(), _engine->_shadowLA.B(),_engine->_shadowLA.A()));
//     SetPixelShaderConstantTemp psc7(7, D3DXVECTOR4(_engine->_shadowLD.R(),_engine->_shadowLD.G(), _engine->_shadowLD.B(),_engine->_shadowLD.A()));
// 
//     // Set temporary render states
//     SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE_OLD, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
//     SetRenderStateTemp rsFOGENABLE(D3DRS_FOGENABLE_OLD, FALSE);
//     SetRenderStateTemp rsSTENCILFUNC(D3DRS_STENCILFUNC_OLD, GetStencilFunc());
//     SetRenderStateTemp rsSTENCILMASK(D3DRS_STENCILMASK_OLD, GetStencilMask());
//     SetRenderStateTemp rsSTENCILWRITEMASK(D3DRS_STENCILWRITEMASK_OLD, STENCIL_COUNTER|STENCIL_PROJ_SHADOW);
//     SetRenderStateTemp rsSTENCILPASS(D3DRS_STENCILPASS_OLD, D3DSTENCILOP_ZERO);
//     SetRenderStateTemp rsSTENCILREF(D3DRS_STENCILREF_OLD, GetStencilRef());
//     SetRenderStateTemp rsZTEST(D3DRS_ZENABLE_OLD,TRUE);
//     SetRenderStateTemp rsZFUNC(D3DRS_ZFUNC_OLD,D3DCMP_GREATEREQUAL_OLD);
//     
//     float startRamp = _pars[ParRampStart];
//     float endRamp = _pars[ParRampEnd];
//     if (_shadowRampMulAddIndex>=0)
//     {
//       // set vertex shader constants
//       DoAssert(endRamp>=startRamp);
//       const float ramp[4] = {endRamp-startRamp, startRamp, 1, 1};
//       
//       _engine->SetVertexShaderConstantF(_shadowRampMulAddIndex,ramp,1,true);
//     }
// 
//     // enable layer selection in debugger
//     // TODO: layer selection based on quality
//     #if _ENABLE_CHEATS
//     static int selectPlate = -1;
//     #else
//     const int selectPlate = -1;
//     #endif
//     
//     int quality = toInt(3-_pars[ParQuality]);
//     saturate(quality,0,2);
//     int nPlates = _nPlates[quality];
//     if (_engine->IsSBEnabled()) nPlates = 1; // If SB shadows disappearing is performed via SB technique
//     if (selectPlate<0 || selectPlate>=nPlates)
//     {
//       if (endRamp<=startRamp)
//       {
//         // no ramp - draw only one layer (two triangles)
//         _engine->GetDirect3DDevice()->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);
//       }
//       else
//       {
//         _engine->GetDirect3DDevice()->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2*nPlates);
//       }
//     }
//     else
//     {
//       _engine->GetDirect3DDevice()->DrawPrimitive(D3DPT_TRIANGLELIST, 6*selectPlate, 2);
//     }
//   }
// 
//   _engine->SetTexture(0, NULL);
//   
//   //_engine->SwitchTL(true);
//   _engine->SwitchTL(TLDisabled);
}

//////////////////////////////////////////////////////////////////////////

static const int GBSizeDivisor = 4;

bool EngineDDT::PPGaussianBlur::Init(const QOStrStream &hlsl, const QOStrStream &vs)
{
  return false;
//   if (!base::Init(hlsl,vs)) return false;
// 
//   // Create custom edge vertex resources
//   {
//     // Create the VB
//     if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(VertexCustomEdge) * 4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, _vbCustomEdge.Init(), NULL)))
//     {
//       LogF("Error: Vertex buffer creation failed.");
//       return false;
//     }
//     void *data;
//     _vbCustomEdge->Lock(0, 0, &data, 0);
//     VertexCustomEdge *v = (VertexCustomEdge *)data;
//     v[0].pos = Vector3(-0.5, -0.5, 0.5); v[0].index = 0;
//     v[1].pos = Vector3(-0.5,  0.5, 0.5); v[1].index = 1;
//     v[2].pos = Vector3( 0.5, -0.5, 0.5); v[2].index = 2;
//     v[3].pos = Vector3( 0.5,  0.5, 0.5); v[3].index = 3;
//     _vbCustomEdge->Unlock();
// 
//     // Create the VD
//     const D3DVERTEXELEMENT9 vertexDeclCustomEdge[] =
//     {
//       {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
//       {0, 12, D3DDECLTYPE_FLOAT1,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
//       D3DDECL_END()
//     };
//     if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDeclCustomEdge, _vdCustomEdge.Init())))
//     {
//       LogF("Error: Vertex declaration creation failed.");
//       return false;
//     }
// 
//     // Create the VS
//     if (!CompileVertexShader(vs,"VSPostProcessCustomEdge4T", _vsCustomEdge))
//     {
//       return false;
//     }
//   }
// 
//   // Create pixel shaders
//   {
//     if (!CompilePixelShader(hlsl,"PSPostProcessGaussianBlurH", _psHorizontal, true)) return false;
//     if (!CompilePixelShader(hlsl,"PSPostProcessGaussianBlurV", _psBlur, true)) return false;
//   }
// 
//   // Create horizontal blur and final blur render targets
//   {
//     int fullWidth = _engine->Width();
//     int fullHeight = _engine->Height();
//     int partWidth = fullWidth / GBSizeDivisor;
//     int partHeight = fullHeight / GBSizeDivisor;
//     HRESULT err;
//     err = _engine->GetDirect3DDevice()->CreateTexture(partWidth, fullHeight, 1, D3DUSAGE_RENDERTARGET, _engine->_rtFormat, D3DPOOL_DEFAULT, _rtHorizontal.Init(), NULL);
//     if (err != D3D_OK) {_engine->DDError9("Error: Render target texture creation failed.", err); return false;}
//     err = _rtHorizontal->GetSurfaceLevel(0, _rtsHorizontal.Init());
//     if (err != D3D_OK) {_engine->DDError9("Error: Render target surface creation failed.", err); return false;}
//     err = _engine->GetDirect3DDevice()->CreateTexture(partWidth, partHeight, 1, D3DUSAGE_RENDERTARGET, _engine->_rtFormat, D3DPOOL_DEFAULT, _rtBlur.Init(), NULL);
//     if (err != D3D_OK) {_engine->DDError9("Error: Render target texture creation failed.", err); return false;}
//     err = _rtBlur->GetSurfaceLevel(0, _rtsBlur.Init());
//     if (err != D3D_OK) {_engine->DDError9("Error: Render target surface creation failed.", err); return false;}
//   }
// 
//   // Success
//   return true;
}

void EngineDDT::PPGaussianBlur::Do(bool isLast)
{
  // Direct3D 10 NEEDS UPDATE 
//   // Draw
//   PROFILE_SCOPE_GRF_EX(ppGSB,pp,SCOPE_COLOR_YELLOW);
// 
//   // If scene is not being rendered initialize scene
//   bool inScene = _engine->_d3dFrameOpen;
//   if (!inScene) _engine->D3DBeginScene();
// 
//   // Prepare drawing
//   Prepare();
// 
//   // Perform blur in 2 steps
//   {
//     // Protect VS constants from changing
//     PushVertexShaderConstant pushC0(0);
//     PushVertexShaderConstant pushC1(1);
//     PushVertexShaderConstant pushC2(2);
//     PushVertexShaderConstant pushC3(3);
//     PushVertexShaderConstant pushC4(4);
//     PushVertexShaderConstant pushC5(5);
//     PushVertexShaderConstant pushC6(6);
//     PushVertexShaderConstant pushC7(7);
//     PushVertexShaderConstant pushC8(8);
// 
//     // Set the vertex shader + stream source
//     _engine->SetStreamSource(_vbCustomEdge, sizeof(VertexCustomEdge), 4);
//     _engine->SetVertexDeclaration(_vdCustomEdge);
//     _engine->SetVertexShader(_vsCustomEdge);
// 
//     // Prepare the UV coordinates offset coefficients
//     static const float baseU[4] = {0, 0, 1, 1};
//     static const float baseV[4] = {1, 0, 1, 0};
//     int fullWidth = _engine->Width();
//     int fullHeight = _engine->Height();
//     int partWidth = fullWidth / GBSizeDivisor;
//     int partHeight = fullHeight / GBSizeDivisor;
// 
//     // No posOffset needed
//     const float posOffset[4] = {0, 0, 0, 0};
//     _engine->SetVertexShaderConstantF(8, posOffset, 1);
// 
//     // Remember the current render target
//     ComRef<ID3D10Texture2D> rt = _engine->CreateRenderTargetCopyAsTexture();
// 
//     // Set temporarily render target and depth stencil to horizontally shrunk texture
//     ComRef<IDirect3DSurface9> zeroDepth;
//     _engine->SetTemporaryRenderTarget(_rtHorizontal, _rtsHorizontal, zeroDepth);
// 
//     // Set temporary render states
//     SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE_OLD, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
//     SetRenderStateTemp rsFOGENABLE(D3DRS_FOGENABLE_OLD, FALSE);
// 
//     // Use SRGB conversion in texture read (SRGB on write (D3DRS_SRGBWRITEENABLE_OLD) is set by default)
//     SetSamplerStateTemp ssSRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE_OLD, _engine->_rtIsSRGB);
// 
//     // Use linear sampling and clamping for the texture on stage 0
//     SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER_OLD, D3DTEXF_LINEAR_OLD);
//     SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER_OLD, D3DTEXF_LINEAR_OLD);
//     SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU_OLD, D3DTADDRESS_CLAMP_OLD);
//     SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV_OLD, D3DTADDRESS_CLAMP_OLD);
// 
//     // First step - render horizontally shrunk texture
//     {
//       // Prepare the UV coordinates offsets
//       const float srcOffsetU = 0.5f / partWidth;
//       const float srcOffsetV = 0.5f / fullHeight;
//       float U[4][4];
//       float V[4][4];
//       for (int vertex = 0; vertex < 4; vertex++)
//       {
//         // Texture 0 - source image
//         U[vertex][0] = baseU[vertex] + srcOffsetU;
//         V[vertex][0] = baseV[vertex] + srcOffsetV;
//         // Rest
//         U[vertex][1] = U[vertex][2] = U[vertex][3] = 0;
//         V[vertex][1] = V[vertex][2] = V[vertex][3] = 0;
//       }
//       _engine->SetVertexShaderConstantF(0, U[0], 4);
//       _engine->SetVertexShaderConstantF(4, V[0], 4);
// 
//       // Set the pixel shader for final blur
//       _engine->SetPixelShader(_psHorizontal);
// 
//       // Set current render target as temporary texture (we know we don't render into it)
//       SetTextureTemp tTex0(0, rt);
// 
//       // Prepare PS constants
//       float invW = 1.0f / _engine->Width();
//       SetPixelShaderConstantTemp spc0(0, D3DXVECTOR4(invW, invW, invW, invW));
// 
//       // Drawing
//       _engine->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
//     }
// 
//     // Rewrite render target and depth stencil surfaces by the blurred ones
//     _engine->SetRenderTargetAndDepthStencil(_rtsBlur, _rtBlur, zeroDepth);
// 
//     // Second step - render finally blurred texture from the horizontally shrunked one
//     {
//       // Prepare the UV coordinates offsets
//       const float srcOffsetU = 0.5f / partWidth;
//       const float srcOffsetV = 0.5f / partHeight;
//       float U[4][4];
//       float V[4][4];
//       for (int vertex = 0; vertex < 4; vertex++)
//       {
//         // Texture 0 - source image
//         U[vertex][0] = baseU[vertex] + srcOffsetU;
//         V[vertex][0] = baseV[vertex] + srcOffsetV;
//         // Rest
//         U[vertex][1] = U[vertex][2] = U[vertex][3] = 0;
//         V[vertex][1] = V[vertex][2] = V[vertex][3] = 0;
//       }
//       _engine->SetVertexShaderConstantF(0, U[0], 4);
//       _engine->SetVertexShaderConstantF(4, V[0], 4);
// 
//       // Set the pixel shader for final blur
//       _engine->SetPixelShader(_psBlur);
// 
//       // Set current render target as temporary texture (we know we don't render into it)
//       SetTextureTemp tTex0(0, _rtHorizontal);
// 
//       // Prepare PS constants
//       float invH = 1.0f / _engine->Height();
//       SetPixelShaderConstantTemp spc0(0, D3DXVECTOR4(invH, invH, invH, invH));
// 
//       // Drawing
//       _engine->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
//     }
// 
//     // Restore the original render target and depth stencil buffer
//     _engine->RestoreOldRenderTarget();
//   }
// 
//   // If scene is not being rendered finish initialized scene
//   if (!inScene) _engine->D3DEndScene();
// 
//   // Restore usual rendering settings
//   _engine->SwitchTL(TLDisabled);
}

//////////////////////////////////////////////////////////////////////////

bool EngineDDT::PPDOF::Init(const QOStrStream &hlsl, const QOStrStream &vs, const Ref<PPGaussianBlur> &ppGaussianBlur)
{
  if (!base::Init(hlsl,vs,"VSPostProcess")) return false;
  if (!CompilePixelShader(hlsl,"PSPostProcessDOF", _ps, true)) return false;
  _ppGaussianBlur = ppGaussianBlur;
  return true;
}

void EngineDDT::PPDOF::Do(bool isLast)
{
//   // Draw
//   PROFILE_SCOPE_GRF_EX(ppDOF,pp,SCOPE_COLOR_YELLOW);
// 
//   // If scene is not being rendered initialize scene
//   bool inScene = _engine->_d3dFrameOpen;
//   if (!inScene) _engine->D3DBeginScene();
// 
//   // Prepare drawing
//   Prepare();
// 
//   // Set temporary render states
//   SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE_OLD, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
//   SetRenderStateTemp rsFOGENABLE(D3DRS_FOGENABLE_OLD, FALSE);
// 
//   // Stage 0 - base texture - do the copy (to make sure same data is both in source texture and destination render target)
//   ComRef<ID3D10Texture2D> rt = _engine->CreateRenderTargetCopyAsTexture();
//   SetTextureTemp tTex0(0, rt);
//   SetSamplerStateTemp ss0SRGBTEXTURE(0, D3DSAMP_SRGBTEXTURE_OLD, _engine->_rtIsSRGB);
//   SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER_OLD, D3DTEXF_LINEAR_OLD);
//   SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER_OLD, D3DTEXF_LINEAR_OLD);
//   SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU_OLD, D3DTADDRESS_CLAMP_OLD);
//   SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV_OLD, D3DTADDRESS_CLAMP_OLD);
// 
//   // Stage 1 - blurred texture
//   SetTextureTemp tTex1(1, _ppGaussianBlur->GetBlurredTexture());
//   SetSamplerStateTemp ss1SRGBTEXTURE(1, D3DSAMP_SRGBTEXTURE_OLD, _engine->_rtIsSRGB);
//   SetSamplerStateTemp ss1MINFILTER(1, D3DSAMP_MINFILTER_OLD, D3DTEXF_LINEAR_OLD);
//   SetSamplerStateTemp ss1MAGFILTER(1, D3DSAMP_MAGFILTER_OLD, D3DTEXF_LINEAR_OLD);
//   SetSamplerStateTemp ss1ADDRESSU(1, D3DSAMP_ADDRESSU_OLD, D3DTADDRESS_CLAMP_OLD);
//   SetSamplerStateTemp ss1ADDRESSV(1, D3DSAMP_ADDRESSV_OLD, D3DTADDRESS_CLAMP_OLD);
// 
//   // Stage 2 - depth map
//   SetTextureTemp tTex2(2, _engine->_renderTargetDB);
//   SetSamplerStateTemp ss2SRGBTEXTURE(2, D3DSAMP_SRGBTEXTURE_OLD, FALSE);
//   SetSamplerStateTemp ss2MINFILTER(2, D3DSAMP_MINFILTER_OLD, D3DTEXF_POINT_OLD);
//   SetSamplerStateTemp ss2MAGFILTER(2, D3DSAMP_MAGFILTER_OLD, D3DTEXF_POINT_OLD);
//   SetSamplerStateTemp ss2ADDRESSU(2, D3DSAMP_ADDRESSU_OLD, D3DTADDRESS_CLAMP_OLD);
//   SetSamplerStateTemp ss2ADDRESSV(2, D3DSAMP_ADDRESSV_OLD, D3DTADDRESS_CLAMP_OLD);
// 
//   // Prepare PS constants
//   // with zoomed camera / binoculars there is more blur
//   Matrix4Val proj = _engine->_proj;
//   
//   AspectSettings as;
//   GEngine->GetAspectSettings(as);
// 
//   /// zoom is default FOV / current FOV
//   static float adjustZoom = 0.5f;
//   float zoom = proj(0,0)/as.leftFOV*adjustZoom;
//   float blurFactor = zoom;
//   static float minBlurFactor = 1;
//   static float maxBlurFactor = 10;
//   saturate(blurFactor,minBlurFactor,maxBlurFactor);
//   static float minZoom = 1;
//   static float maxZoom = 2.5; // we want to avoid too large offsets
//   saturate(zoom,minZoom,maxZoom);
// 
//   const float blurRadiusInPixels = 3.0f*zoom;
//   
//   float invW = 1.0f / _engine->Width();
//   float invH = 1.0f / _engine->Height();
//   SetPixelShaderConstantTemp spc0(0, D3DXVECTOR4(invW * blurRadiusInPixels, invH * blurRadiusInPixels, blurFactor, 0));
//   float invFocalPlane = 1.0f / _engine->_focusDist;
//   SetPixelShaderConstantTemp spc1(1, D3DXVECTOR4(invFocalPlane, invFocalPlane, invFocalPlane, invFocalPlane));
// 
//   // Drawing
//   _engine->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
// 
//   // If scene is not being rendered finish initialized scene
//   if (!inScene) _engine->D3DEndScene();
// 
//   // Restore usual rendering settings
//   _engine->SwitchTL(TLDisabled);
}

//////////////////////////////////////////////////////////////////////////

bool EngineDDT::PPDistanceDOF::Init(
  const QOStrStream &hlsl, const QOStrStream &vs, PPGaussianBlur *ppGaussianBlur
)
{
  if (!grandBase::Init(hlsl,vs,"VSPostProcess")) return false;
  if (!CompilePixelShader(hlsl,"PSPostProcessDistanceDOF", _ps, true)) return false;
  _ppGaussianBlur = ppGaussianBlur;
  return true;
}

//////////////////////////////////////////////////////////////////////////

// Direct3D 10 NEEDS UPDATE 
// EngineDDT *EngineDDT::PPGlow::QueryManager::_engine;
// 
// EngineDDT::PPGlow::QueryManager::QueryManager()
// {
//   // Create the query objects
//   for (int i = 0; i < NItems; i++)
//   {
//     for (int b=0; b<NBrightQueries; b++)
//     {
//       if (FAILED(_engine->GetDirect3DDevice()->CreateQuery(D3DQUERYTYPE_OCCLUSION, _query[i][b].Init())))
//       {
//         LogF("Warning: Current HW doesn't support queries");
//         goto Break;
//       }
//     }
//   }
//   Break:
// 
//   // Initialize the startup values
//   ResetQueries();
// }
// 
// void EngineDDT::PPGlow::QueryManager::ResetQueries()
// {
//   // Initialize the startup values
//   _newQuery = 0;
//   _oldestQuery = 0;
//   for (int b=0; b<NBrightQueries; b++)
//   {
//     _mostRecentResult[b] = -1;
//   }
//   _queriesIssued = 0;
//   _currentResultIndex = 0;
// }
// 
// bool EngineDDT::PPGlow::QueryManager::Issue(const float *value, const float *alpha, float masterValue)
// {
//   if (_query[_newQuery][0].IsNull()) return false;
//   if (_queriesIssued>=NItems) return false;
//   PushPixelShaderConstant psc7(7);
//   for (int b=0; b<NBrightQueries; b++)
//   {
//     const float queryDot[4]={0,0,0,alpha[b]};
//     _engine->SetPixelShaderConstantF(7, queryDot,1);
//     _query[_newQuery][b]->Issue(D3DISSUE_BEGIN);
//     _value[_newQuery][b] = value[b];
//     _engine->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
//     _query[_newQuery][b]->Issue(D3DISSUE_END);
//   }
//   _masterValue[_newQuery] = masterValue;
//   _queriesIssued++;
//   DoAssert(_queriesIssued<=NItems);
//   _newQuery = Next(_newQuery);
//   return true;
// }
// 
// float EngineDDT::PPGlow::QueryManager::GetNumberOfPixelsDrawn(int *npd, float *value)
// {
//   #if _DEBUG
//   // we may want to introduce artificial latency when debugging
//   static int latency = 1;
//   #else
//   const int latency = 0;
//   #endif
//   while (_queriesIssued>latency)
//   {
//     // Get the newest available query result
//     // check from beginning to the last
//     // once the last one is done, all are done
//     Assert(_currentResultIndex<NBrightQueries);
//     DWORD nPixels;
//     if (_query[_oldestQuery][_currentResultIndex]->GetData(&nPixels, sizeof(DWORD), D3DGETDATA_FLUSH) != S_OK)
//     {
//       // if query is not ready, break and report what we already have
//       break;
//     }
//     _results[_currentResultIndex] = nPixels>0; // we know we are sampling 1x1 - either 1 or 0 can pass
//     _currentResultIndex++;
//     if (_currentResultIndex>=NBrightQueries)
//     {
//       for (int b=0; b<NBrightQueries; b++)
//       {
//         _mostRecentResult[b] = _results[b];
//         _mostRecentResultValue[b] = _value[_oldestQuery][b];
//       }
//       _mostRecentMasterValue = _masterValue[_oldestQuery];
//       // we picked up the result - report it
//       // we assume we will be able to pick all other results as well now
//       _queriesIssued--;
//       _oldestQuery = Next(_oldestQuery);
//       _currentResultIndex = 0;
//     }
//   }
//   for (int b=0; b<NBrightQueries; b++)
//   {
//     value[b] = _mostRecentResultValue[b];
//     npd[b] = _mostRecentResult[b];
//   }
//   return _mostRecentMasterValue;
// }
// 
// bool EngineDDT::PPGlow::Init(
//   const QOStrStream &hlsl, const QOStrStream &vs, PPGaussianBlur *ppGaussianBlur
// )
// {
//   if (!base::Init(hlsl,vs)) return false;
//   
//   _ppGaussianBlur = ppGaussianBlur;
// 
//   // Create the low resolution textures cascade
//   
//   // we need textures for 4x average decimation
//   // and then some for 2x avg/min/max decimation
//   // we want to start calculating min/max once the picture is coarse enough
//   const int startMinMaxAtSize = 80;
//   
//   int fullWidth = _engine->Width();
//   int fullHeight = _engine->Height();
//   
//   // decimation 2x uses point sampling and should therefore be very accurate
//   // we will round to nearest bigger pow2
//   int w = roundTo2PowerNCeil(fullWidth);
//   int h = roundTo2PowerNCeil(fullHeight);
// 
//   int maxSize = max(w,h);
//   int minSize = min(w,h);
//   
//   DXGI_FORMAT rtFormat = _engine->_downsampleFormat;
//   int level = 0;
//   // we need at least one 4x decimation step
//   do
//   {
//     DecimationPass &dec = _dec[level];
//     w = w>>2;
//     h = h>>2;
//     maxSize = maxSize>>2;
//     minSize = minSize>>2;
//     // create the supplementary RT
//     ComRef<ID3D10Texture2D> rtt;
//     int effW = max(w,1);
//     int effH = max(h,1);
//     LogF("Decimation 4x: %d,%d",effW,effH);
//     HRESULT err = _engine->GetDirect3DDevice()->CreateTexture(effW, effH, 1, D3DUSAGE_RENDERTARGET, rtFormat, D3DPOOL_DEFAULT, rtt.Init(), NULL);
//     if (FAILED(err))
//     {
//       _engine->DDError9("Post process Low resolution texture creation failed",err);
//       return false;
//     }
//     dec._rt = rtt;
//     dec._rt->GetSurfaceLevel(0, dec._rts.Init());
//     dec._w = effW;
//     dec._h = effH;
//     // proceed to next level
//     level++;
//   } while (minSize>startMinMaxAtSize);
//   _decimation4x = level;
//   // we need at least one 2x decimation step
//   do
//   {
//     DecimationPass &dec = _dec[level];
//     w = w>>1;
//     h = h>>1;
//     maxSize = maxSize>>1;
//     minSize = minSize>>1;
//     // create the supplementary RT
//     ComRef<ID3D10Texture2D> rtt;
//     // never create less then 1x1
//     int effW = max(w,1);
//     int effH = max(h,1);
//     LogF("Decimation 2x: %d,%d",effW,effH);
//     HRESULT err = _engine->GetDirect3DDevice()->CreateTexture(effW, effH, 1, D3DUSAGE_RENDERTARGET, rtFormat, D3DPOOL_DEFAULT, rtt.Init(), NULL);
//     if (FAILED(err))
//     {
//       _engine->DDError9("Post process Low resolution texture creation failed",err);
//       return false;
//     }
//     dec._rt = rtt;
//     dec._rt->GetSurfaceLevel(0, dec._rts.Init());
//     dec._w = effW;
//     dec._h = effH;
//     // proceed to next level
//     level++;
//   } while (maxSize>1);
//   _decimation2x = level;
// 
//   HRESULT err = _engine->GetDirect3DDevice()->CreateTexture(1, 1, 1, D3DUSAGE_RENDERTARGET, rtFormat, D3DPOOL_DEFAULT, _rtMeasure.Init(), NULL);
//   if (FAILED(err))
//   {
//     _engine->DDError9("Post process Low resolution texture creation failed",err);
//     return false;
//   }
//   _rtMeasure->GetSurfaceLevel(0, _rtsMeasure.Init());
//   
//   // stencil/depth buffer no longer needed here - we can disable it
//   if (_engine->_depthBufferRT)
//   {
//     HRESULT err = _engine->_d3DDevice->CreateDepthStencilSurface(
//       1,1,D3DFMT_D24S8,
//       D3DMULTISAMPLE_NONE,0,TRUE,_rtsMeasureDepth.Init(),NULL
//     );
//     if (FAILED(err))
//     {
//       _engine->DDError9("Measure depth buffer creation failed",err);
//       return false;
//     }
//   }
//   
//   _rtApertureCurrent = 0;
//   DXGI_FORMAT apFormat = _engine->_apertureFormat;
//   for (int i=0; i<2; i++)  
//   {
//     HRESULT err = _engine->GetDirect3DDevice()->CreateTexture(1, 1, 1, D3DUSAGE_RENDERTARGET, apFormat, D3DPOOL_DEFAULT, _rtAperture[i].Init(), NULL);
//     if (FAILED(err))
//     {
//       _engine->DDError9("Post process Low resolution texture creation failed",err);
//       return false;
//     }
//     _rtAperture[i]->GetSurfaceLevel(0, _rtsAperture[i].Init());
//     
//     // save RT
//     ComRef<IDirect3DSurface9> rt;
//     _engine->_d3DDevice->GetRenderTarget(0,rt.Init());
//     // clear the target to avoid uninitialized value
//     _engine->_d3DDevice->SetRenderTarget(0,_rtsAperture[i]);
//     _engine->_d3DDevice->Clear(0,NULL,D3DCLEAR_TARGET,0,0,0);
//     // restore RT
//     _engine->_d3DDevice->SetRenderTarget(0,rt);
//     
//   }
//   
//   #if _DEBUG
//   {
//     HRESULT err = _engine->GetDirect3DDevice()->CreateOffscreenPlainSurface(1, 1, apFormat, D3DPOOL_SYSTEMMEM, _rtsDebugAp.Init(), NULL);
//     if (FAILED(err))
//     {
//       _engine->DDError9("Post process Low resolution texture creation failed",err);
//       return false;
//     }
//     err = _engine->GetDirect3DDevice()->CreateOffscreenPlainSurface(1, 1, rtFormat, D3DPOOL_SYSTEMMEM, _rtsDebugDec.Init(), NULL);
//     if (FAILED(err))
//     {
//       _engine->DDError9("Post process Low resolution texture creation failed",err);
//       return false;
//     }
//   }
//   #endif
//   
//   
//   // stencil/depth buffer no longer needed here - we can disable it
//   /*
//   if (_engine->_depthBufferRT)
//   {
//     D3DSURFACE_DESC desc;
//     _rtRough[0]->GetLevelDesc(0,&desc);
//     int roughWidth = desc.Width;
//     int roughHeight = desc.Height;
//     if (
//       FAILED(
//         _engine->_d3DDevice->CreateDepthStencilSurface(
//           roughWidth,roughHeight,D3DFMT_D24S8,
//           D3DMULTISAMPLE_NONE,0,TRUE,_rtsRoughDepth.Init(),NULL
//         )
//       )
//     )
//     {
//       LogF("PPGlow: Cannot create depth buffer");
//       return false;
//     }
//   }
//   */
// 
//   {
//     // Create the VB + declaration
//     if (FAILED(_engine->GetDirect3DDevice()->CreateVertexBuffer(sizeof(VertexCustomEdge) * 4, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, _vbCustomEdge.Init(), NULL)))
//     {
//       LogF("Error: Vertex buffer creation failed.");
//       return false;
//     }
//     void *data;
//     _vbCustomEdge->Lock(0, 0, &data, 0);
//     VertexCustomEdge *v = (VertexCustomEdge *)data;
//     v[0].pos = Vector3(-0.5, -0.5, 0.5); v[0].index = 0;
//     v[1].pos = Vector3(-0.5,  0.5, 0.5); v[1].index = 1;
//     v[2].pos = Vector3( 0.5, -0.5, 0.5); v[2].index = 2;
//     v[3].pos = Vector3( 0.5,  0.5, 0.5); v[3].index = 3;
//     _vbCustomEdge->Unlock();
// 
//     // Create the VD of the blur vertex
//     const D3DVERTEXELEMENT9 vertexDeclCustomEdge[] =
//     {
//       {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
//       {0, 12, D3DDECLTYPE_FLOAT1,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
//       D3DDECL_END()
//     };
//     if (FAILED(_engine->GetDirect3DDevice()->CreateVertexDeclaration(vertexDeclCustomEdge, _vdCustomEdge.Init())))
//     {
//       LogF("Error: Vertex declaration creation failed.");
//       return false;
//     }
//   }
// 
//   // Create the shaders
//   if (!CompileVertexShader(vs,"VSPostProcessCustomEdge4T",_vsCustomEdge))
//   {
//     return false;
//   }
//   
//   if (!CompilePixelShader(hlsl,"PSPostProcessDownSampleLuminance",_psLuminance))
//   {
//     return false;
//   }
//   if (!CompilePixelShader(hlsl,"PSPostProcessDownSampleAvgLuminance",_psAvgLuminance))
//   {
//     return false;
//   }
//   if (!CompilePixelShader(hlsl,"PSPostProcessDownSampleMaxAvgMinLuminance",_psMaxAvgMinLuminance))
//   {
//     return false;
//   }
//   ComRef<ID3D10PixelShader> ppal;
//   if (!CompilePixelShader(hlsl,"PSPostProcessAssumedLuminance",ppal,true))
//   {
//     return false;
//   }
//   ComRef<ID3D10PixelShader> ppalDirect;
//   if (!CompilePixelShader(hlsl,"PSPostProcessAssumedLuminanceDirect",ppalDirect,false))
//   {
//     return false;
//   }
// 
//   if (_engine->_apertureFormat==D3DFMT_A8R8G8B8)
//   {
//     /**
//     if there is no float render target support,
//     we cannot calculate change on the GPU
//     */
//     _psAssumedLuminance = ppalDirect;
//   }
//   else
//   {
//     _psAssumedLuminance = ppal;
//   }
// 
//   if (!CompilePixelShader(hlsl,"PSPostProcessQuery",_psQuery))
//   {
//     return false;
//   }
// 
// 
//   if (!CompilePixelShader(hlsl,"PSPostProcessGlow",_psGlow))
//   {
//     return false;
//   };
// 
//   if (!CompilePixelShader(hlsl,"PSPostProcessGlowNight",_psGlowNight))
//   {
//     return false;
//   };
//   if (!CompilePixelShader(hlsl,"PSPostProcessNVG",_psGlowNVG))
//   {
//     return false;
//   };
// 
//   return true;
// }
// 
// void EngineDDT::PPGlow::SetParameters(const float *pars, int nPars)
// {
//   Assert(nPars==3);
//   _deltaT = pars[0];
//   _eyeAdaptMin = pars[1];
//   _eyeAdaptMax = pars[2];
// }
// 
// #if _DEBUG
// Color EngineDDT::PPGlow::GetContent(IDirect3DSurface9 *surf, IDirect3DSurface9 *temp, DXGI_FORMAT format)
// {
//   _engine->GetDirect3DDevice()->GetRenderTargetData(surf,temp);
// 
//   D3DLOCKED_RECT locked;
//   temp->LockRect(&locked,NULL,D3DLOCK_READONLY);
//   // assume ARGB8888 format
//   Color color;
//   switch (format)
//   {
//     case D3DFMT_A8R8G8B8:
//       {
//         PackedColor c = PackedColor(*(DWORD *)locked.pBits);
//         color = Color(c);
//       }
//       break;
//     case D3DFMT_R32F:
//       {
//         const float *c = (const float *)locked.pBits;
//         color = Color(c[0],0,0,1);
//       }
//       break;
//     case D3DFMT_G32R32F:
//       {
//         const float *c = (const float *)locked.pBits;
//         color = Color(c[0],c[1],0,1);
//       }
//       break;
//     case D3DFMT_A32B32G32R32F:
//       {
//         const float *c = (const float *)locked.pBits;
//         color = Color(c[0],c[1],c[2],c[3]);
//       }
//       break;
//     case D3DFMT_A16B16G16R16F:
//       {
//         const Float16b<10> *c = (const Float16b<10> *)locked.pBits;
//         color = Color(c[0],c[1],c[2],c[3]);
//       }
//       break;
//     case D3DFMT_G16R16F:
//       {
//         const Float16b<10> *c = (const Float16b<10> *)locked.pBits;
//         color = Color(c[0],c[1],0,1);
//       }
//       break;
//     case D3DFMT_R16F:
//       {
//         const Float16b<10> *c = (const Float16b<10> *)locked.pBits;
//         color = Color(c[0],0,0,1);
//       }
//       break;
//     case D3DFMT_L16:
//       {
//         const unsigned short *c = (const unsigned short *)locked.pBits;
//         float l = c[0]*(1.0f/0xffff);
//         color = Color(l,l,l,1);
//       }
//       break;
//     case D3DFMT_G16R16:
//       {
//         const unsigned short *c = (const unsigned short *)locked.pBits;
//         float r = c[0]*(1.0f/0xffff);
//         float g = c[1]*(1.0f/0xffff);
//         color = Color(r,g,0,1);
//       }
//       break;
//     default:
//       color = HBlack;
//       break;
//   }
//   temp->UnlockRect();
//   return color;
// }
// 
// static Color DecodeApertureChangeF(Color color)
// {
//   return Color(
//     DecodeApertureChange(color.R()),
//     DecodeApertureChange(color.G()),
//     DecodeApertureChange(color.B()),
//     DecodeApertureChange(color.A())
//   );
// }
// 
// Color EngineDDT::PPGlow::GetContentAp(IDirect3DSurface9 *surf)
// {
//   // aperture change is stored as logarithm
//   Color raw = GetContent(surf,_rtsDebugAp,_engine->_apertureFormat);
//   return DecodeApertureChangeF(raw);
// }
// Color EngineDDT::PPGlow::GetContentDec(IDirect3DSurface9 *surf)
// {
//   return GetContent(surf,_rtsDebugDec,D3DFMT_A8R8G8B8);
// }
// 
// #endif
// 
// // note: MS HDR uses sensitivity as follows:
// //(0.2125, 0.7154, 0.0721);
// //(0.2125, 0.7154, 0.0721);
// #define R_EYE_DARK 0.05f
// #define G_EYE_DARK 0.55f
// #define B_EYE_DARK 0.40f
// static const Color DaySens(R_EYE,G_EYE,B_EYE);
// 
// /*!
// \patch 5129 Date 2/20/2007 by Ondra
// - Improved: Improved night vision goggles rendering.
// */
// static const Color NVGSens(0.6,0.2,0.2);
// static const Color NightSens(R_EYE_DARK,G_EYE_DARK,B_EYE_DARK);
// 
// void EngineDDT::PPGlow::Do(bool isLast)
// {
//   static const float zero9[9*4] =
//   {
//     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//   };
//   
//   // glow should always be last - it performs SRGB conversions
//   DoAssert(isLast);
//   PROFILE_SCOPE_GRF_EX(ppGlw,pp,SCOPE_COLOR_YELLOW);
// 
//   // Prepare drawing
//   Prepare();
// 
//   ComRef<ID3D10Texture2D> origRt = _engine->_currRenderTarget;
//   ComRef<IDirect3DSurface9> origRts = _engine->_currRenderTargetSurface;
// 
//   // We will need a source as a texture
//   ComRef<ID3D10Texture2D> rt;
//   
//   // if possible, use previous render target directly for texturing
//   if (isLast && _engine->_currRenderTarget)
//   {
//     rt = _engine->GetCurrRenderTargetAsTexture();
//   }
//   else
//   {
//     rt = _engine->CreateRenderTargetCopyAsTexture();
//   }
//   
//   // depending on render target format it may or may not be SRGB
//   Assert(_engine->_linearSpace);
//   BOOL rtIsSRGB = _engine->_rtIsSRGB && _engine->_canReadSRGB;
//   // Set the vertex shader + stream source
//   _engine->SetStreamSource(_vbCustomEdge, sizeof(VertexCustomEdge), 4);
//   _engine->SetVertexDeclaration(_vdCustomEdge);
//   _engine->SetVertexShader(_vsCustomEdge);
// 
// 
//   {
//     PushVertexShaderConstant pushC0(0);
//     PushVertexShaderConstant pushC1(1);
//     PushVertexShaderConstant pushC2(2);
//     PushVertexShaderConstant pushC3(3);
//     PushVertexShaderConstant pushC4(4);
//     PushVertexShaderConstant pushC5(5);
//     PushVertexShaderConstant pushC6(6);
//     PushVertexShaderConstant pushC7(7);
//     PushVertexShaderConstant pushC8(8);
//     
//     // decide if decimated surfaces are SRGB or not
//     const bool decimatedSRGB = _engine->_downsampleFormat==D3DFMT_A8B8G8R8;
//     
//     // Decimate the original into rough texture and measure aperture
//     {
//       // Set temporary states
//       
//       SetRenderStateTemp rsSRGBSRGBWRITEENABLE(D3DRS_SRGBWRITEENABLE_OLD, decimatedSRGB);
//       SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE_OLD, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
//       // no alpha or fog done here
//       SetRenderStateTemp rsFOGENABLE(D3DRS_FOGENABLE_OLD, FALSE);
//       SetRenderStateTemp rsALPHATESTENABLE(D3DRS_ALPHATESTENABLE_OLD, FALSE);
//       SetRenderStateTemp rsALPHABLENDENABLE(D3DRS_ALPHABLENDENABLE_OLD, FALSE);
//       // by default assume bi-linear filtering
//       SetSamplerStateTemp ss0MINFILTER(0, D3DSAMP_MINFILTER_OLD, D3DTEXF_LINEAR_OLD);
//       SetSamplerStateTemp ss0MAGFILTER(0, D3DSAMP_MAGFILTER_OLD, D3DTEXF_LINEAR_OLD);
//       SetSamplerStateTemp ss1MINFILTER(1, D3DSAMP_MINFILTER_OLD, D3DTEXF_LINEAR_OLD);
//       SetSamplerStateTemp ss1MAGFILTER(1, D3DSAMP_MAGFILTER_OLD, D3DTEXF_LINEAR_OLD);
//       SetSamplerStateTemp ss2MINFILTER(2, D3DSAMP_MINFILTER_OLD, D3DTEXF_LINEAR_OLD);
//       SetSamplerStateTemp ss2MAGFILTER(2, D3DSAMP_MAGFILTER_OLD, D3DTEXF_LINEAR_OLD);
//       SetSamplerStateTemp ss3MINFILTER(3, D3DSAMP_MINFILTER_OLD, D3DTEXF_LINEAR_OLD);
//       SetSamplerStateTemp ss3MAGFILTER(3, D3DSAMP_MAGFILTER_OLD, D3DTEXF_LINEAR_OLD);
//       // tri-linear filtering never done 
//       SetSamplerStateTemp ss0MIPFILTER(0, D3DSAMP_MIPFILTER_OLD, D3DTEXF_POINT_OLD);
//       SetSamplerStateTemp ss1MIPFILTER(1, D3DSAMP_MIPFILTER_OLD, D3DTEXF_POINT_OLD);
//       SetSamplerStateTemp ss2MIPFILTER(2, D3DSAMP_MIPFILTER_OLD, D3DTEXF_POINT_OLD);
//       SetSamplerStateTemp ss3MIPFILTER(3, D3DSAMP_MIPFILTER_OLD, D3DTEXF_POINT_OLD);
//       // clamping all stages
//       SetSamplerStateTemp ss0ADDRESSU(0, D3DSAMP_ADDRESSU_OLD, D3DTADDRESS_CLAMP_OLD);
//       SetSamplerStateTemp ss0ADDRESSV(0, D3DSAMP_ADDRESSV_OLD, D3DTADDRESS_CLAMP_OLD);
//       SetSamplerStateTemp ss1ADDRESSU(1, D3DSAMP_ADDRESSU_OLD, D3DTADDRESS_CLAMP_OLD);
//       SetSamplerStateTemp ss1ADDRESSV(1, D3DSAMP_ADDRESSV_OLD, D3DTADDRESS_CLAMP_OLD);
//       SetSamplerStateTemp ss2ADDRESSU(2, D3DSAMP_ADDRESSU_OLD, D3DTADDRESS_CLAMP_OLD);
//       SetSamplerStateTemp ss2ADDRESSV(2, D3DSAMP_ADDRESSV_OLD, D3DTADDRESS_CLAMP_OLD);
//       SetSamplerStateTemp ss3ADDRESSU(3, D3DSAMP_ADDRESSU_OLD, D3DTADDRESS_CLAMP_OLD);
//       SetSamplerStateTemp ss3ADDRESSV(3, D3DSAMP_ADDRESSV_OLD, D3DTADDRESS_CLAMP_OLD);
// 
// 
//       
//       #if 0 //_ENABLE_CHEATS
//         D3DSURFACE_DESC desc;
//         rt->GetLevelDesc(0,&desc);
//         DoAssert(desc.Width==_engine->Width());
//         DoAssert(desc.Height==_engine->Height());
//       #endif
// 
//       {
//         PROFILE_SCOPE_GRF_EX(ppGLu,pp,SCOPE_COLOR_YELLOW);
//     
//         // perform initial luminance calculation + 4x decimation
//         EdgeParams pars;
//         pars.SetupBilinear(_dec[0]._w,_dec[0]._h,_engine->Width(),_engine->Height());
//         _engine->SetVertexShaderConstantF(0, pars.u[0], 9);
// 
//         // Set the pixel shader
//         _engine->SetPixelShader(_psLuminance);
// 
//         // Set the render target
//         _engine->SetRenderTarget(_dec[0]._rts, _dec[0]._rt, NULL);
// 
//         // set the source as 4 textures    
//         SetTextureTemp tTex0(0, rt);
//         SetTextureTemp tTex1(1, rt);
//         SetTextureTemp tTex2(2, rt);
//         SetTextureTemp tTex3(3, rt);
//         SetSamplerStateTemp tempSRGB0(0, D3DSAMP_SRGBTEXTURE_OLD, rtIsSRGB);
//         SetSamplerStateTemp tempSRGB1(1, D3DSAMP_SRGBTEXTURE_OLD, rtIsSRGB);
//         SetSamplerStateTemp tempSRGB2(2, D3DSAMP_SRGBTEXTURE_OLD, rtIsSRGB);
//         SetSamplerStateTemp tempSRGB3(3, D3DSAMP_SRGBTEXTURE_OLD, rtIsSRGB);
//         
//         _engine->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
//       }
//     
//       // source is always linear space since here
//       SetSamplerStateTemp tempSRGB0(0, D3DSAMP_SRGBTEXTURE_OLD, decimatedSRGB);
//       SetSamplerStateTemp tempSRGB1(1, D3DSAMP_SRGBTEXTURE_OLD, decimatedSRGB);
//       SetSamplerStateTemp tempSRGB2(2, D3DSAMP_SRGBTEXTURE_OLD, decimatedSRGB);
//       SetSamplerStateTemp tempSRGB3(3, D3DSAMP_SRGBTEXTURE_OLD, decimatedSRGB);
//       
//       { // perform 4x decimation + average calculation
//         PROFILE_SCOPE_GRF_EX(ppGAv,pp,SCOPE_COLOR_YELLOW);
//         for (int i=1; i<_decimation4x; i++)
//         {
//           // perform initial luminance calculation + 4x decimation
//           EdgeParams pars;
//           pars.SetupBilinear(_dec[i]._w,_dec[i]._h,_dec[i-1]._w,_dec[i-1]._h);
//           
//           _engine->SetVertexShaderConstantF(0, pars.u[0], 9);
// 
//           // Set the pixel shader
//           _engine->SetPixelShader(_psAvgLuminance);
// 
//           // Set the render target
//           _engine->SetRenderTarget(_dec[i]._rts, _dec[i]._rt, NULL);
// 
//           ID3D10Texture2D *tex = _dec[i-1]._rt;
//           // set the source as 4 textures    
//           SetTextureTemp tTex0(0, tex);
//           SetTextureTemp tTex1(1, tex);
//           SetTextureTemp tTex2(2, tex);
//           SetTextureTemp tTex3(3, tex);
// 
//           _engine->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
//         }
//       }
// 
//       { // perform 2x decimation + min/max/avg calculation
//         PROFILE_SCOPE_GRF_EX(ppGMM,pp,SCOPE_COLOR_YELLOW);
//         // note: we know other temp setting is done, which will return original values once done
//         for (int s=0; s<4; s++) 
//         {
//           _engine->D3DSetSamplerState(s, D3DSAMP_MINFILTER_OLD, D3DTEXF_POINT_OLD);
//           _engine->D3DSetSamplerState(s, D3DSAMP_MAGFILTER_OLD, D3DTEXF_POINT_OLD);
//         }
//         
//         // Set the pixel shader
//         _engine->SetPixelShader(_psMaxAvgMinLuminance);
//         for (int i=_decimation4x; i<_decimation2x; i++)
//         {
//           EdgeParams pars;
//           pars.SetupBilinear(_dec[i]._w,_dec[i]._h,_dec[i-1]._w,_dec[i-1]._h);
//           
//           _engine->SetVertexShaderConstantF(0, pars.u[0], 9);
// 
//           // Set the render target
//           _engine->SetRenderTarget(_dec[i]._rts, _dec[i]._rt, NULL);
// 
//           ID3D10Texture2D *tex = _dec[i-1]._rt;
//           // set the source as 4 textures    
//           SetTextureTemp tTex0(0, tex);
//           SetTextureTemp tTex1(1, tex);
//           SetTextureTemp tTex2(2, tex);
//           SetTextureTemp tTex3(3, tex);
//           
//           _engine->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
//         }
//         
//         // perform some calculations with 1x1 surfaces
//         // this is to adjust aperture
//         {
//           _engine->SetPixelShader(_psAssumedLuminance);
//           
//           Assert(_engine->_accomFactor>0);
//           // values in last frame were converted to absolute by multiplying by:
//           // _engine->_accomFactorPrev * lastFrameAp
//           // values in this frame are converted to absolute by multiplying by:
//           //  _engine->_accomFactor * thisFrameAp
//           // absAp = lastFrameAp * _engine->_accomFactorPrev
//           // absAp = thisFrameAp * _engine->_accomFactor
// 
//           // lastFrameAp * _engine->_accomFactorPrev/_engine->_accomFactor = thisFrameAp
//           
//           
//           float eyeAccomLastToThis = _engine->_accomFactorPrev/_engine->_accomFactor;
//           // this controls overall scene brightness
//           // we need to know where the user selected midpoint is
//           float desiredLuminance = _engine->GetPreHDRBrightness()*_engine->GetHDRFactor();
//           float invAccom=1/_engine->_accomFactor;
// 
//           // adaptation time depends on how we are adapting
//           // if we are reacting with iris, the adaptations is very fast
//           // if we are reacting with retina, it is much slower
//           // even when retina is adapted, if light level is raised, the iris will react
//           // this allows quite quick re-adaptation for higher light levels
//           // we expect something like:
//           const float irisUsedToDark = 1; // time to make change 2x
//           const float irisUsedToLight = 0.5; // time to make change 0.5x
//           
//           const float retinaUsedToDark = 20; // time to make change 2x
//           const float retinaUsedToLight = 1.0; // time to make change 0.5x
//           
//           // retina adaptation dynamic range is probably much smaller (100x?)
//           // when retina adaptation is important:
//           // 1) iris fully open
//           // 2) once rods get adapted, they go down slowly
//           // we do not model this, we assume under certain level retina acts
//           // and above it iris acts
//           // assume retina starts reacting once iris adaptation is 1.0
//           const float crossBeg = 1.0f;
//           const float crossEnd = 4.0f;
//           
//           float retinaFactor = InterpolativC(
//             _engine->_accomFactorPrev,crossBeg,crossEnd,0,1
//           );
//           
//           // time to make change 2x
//           const float getUsedToDark = retinaFactor*retinaUsedToDark+(1-retinaFactor)*irisUsedToDark;
//           // time to make change 0.5x
//           const float getUsedToLight = retinaFactor*retinaUsedToLight+(1-retinaFactor)*irisUsedToLight;
//           
//           const float log2 = 0.69314718056f;
//           
//           
//           float minRatio = exp(-_deltaT*(log2/getUsedToLight));
//           float maxRatio = exp(+_deltaT*(log2/getUsedToDark));
// 
//           // make sure the number still can be represented with float16
//           saturateMin(minRatio,0.999f);
//           saturateMax(maxRatio,1.001f);
//           
//           // we need the value to be representable in float16 (m10e6) format
//           float minAdapt = 1, maxAdapt = 1;
//           
//           if (!_engine->_accomFactorFixed)
//           {
//             minAdapt = _eyeAdaptMin*invAccom;
//             maxAdapt = _eyeAdaptMax*invAccom;
//           }
// 
//           const float pars[8]={
//             eyeAccomLastToThis,_deltaT,desiredLuminance,0,
//             minAdapt,maxAdapt,minRatio,maxRatio
//           };
//           
//           #if _ENABLE_CHEATS
//             if (CHECK_DIAG(DEHDR))
//             {
//               Log("Adapt. factor: last %g, this %g",_engine->_accomFactorPrev,_engine->_accomFactor);
//             }
//           #endif
// 
//           PushPixelShaderConstant psc8(8),psc(9);
//           _engine->SetPixelShaderConstantF(8, pars, 2);
//           
//           _engine->SetVertexShaderConstantF(0, zero9, 9);
// 
//           int oldAperture = _rtApertureCurrent;
//           // switch to next aperture texture
//           _rtApertureCurrent = _rtApertureCurrent==0;
//           // Set the render target
//           _engine->SetRenderTarget(_rtsAperture[_rtApertureCurrent], _rtAperture[_rtApertureCurrent], NULL);
// 
//           // 1x1 maxAvgMin as source
//           SetTextureTemp tTex0(0, _dec[_decimation2x-1]._rt);
//           // old aperture value as another source
//           SetTextureTemp tTex1(1, _rtAperture[oldAperture]);
//           SetTextureTemp tTex2(2, NULL);
//           SetTextureTemp tTex3(3, NULL);
//           
//           // render target is always linear
//           SetRenderStateTemp rsSRGBSRGBWRITEENABLE(D3DRS_SRGBWRITEENABLE_OLD, FALSE);
// 
//           // old aperture is always linear
//           // source down-sampled scene is SRGB
//           SetSamplerStateTemp tempSRGB0(0, D3DSAMP_SRGBTEXTURE_OLD, decimatedSRGB);
//           SetSamplerStateTemp tempSRGB1(1, D3DSAMP_SRGBTEXTURE_OLD, FALSE);
//           
//           _engine->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
// 
//           #if _DEBUG
//             if (CHECK_DIAG(DEHDR))
//             {
//               // lock textures and read debugging data from them
//               Color dec = GetContentDec(_dec[_decimation2x-1]._rts);
//               
//               Color oldAp = GetContentAp(_rtsAperture[oldAperture]);
//               Color newAp = GetContentAp(_rtsAperture[_rtApertureCurrent]);
//               Color oldApAbs = oldAp*_engine->_accomFactorPrev;
//               Color newApAbs = oldAp*_engine->_accomFactor;
//               Log(
//                 "Aperture: %g,%g,%g,%g -> %g,%g,%g,%g",
//                 oldAp.R(),oldAp.G(),oldAp.B(),oldAp.A(),
//                 newAp.R(),newAp.G(),newAp.B(),newAp.A()
//               );
//               Log(
//                 "Aperture (abs): %g,%g,%g,%g -> %g,%g,%g,%g",
//                 oldApAbs.R(),oldApAbs.G(),oldApAbs.B(),oldApAbs.A(),
//                 newApAbs.R(),newApAbs.G(),newApAbs.B(),newApAbs.A()
//               );
//               Log("Dec: %g,%g,%g,%g",dec.R(),dec.G(),dec.B(),dec.A());
//             }
//           #endif
//         }
//       }
//       
//       // note: linear space temp goes off scope here (cf. tempNoSRGB0)
//     }
// 
//     
//     // Measure the brightness of the image
//     {
//       // source is 1x1 image - UV mapping does not really matter
//       // Set the mapping coordinates for all 4 vertices
//       _engine->SetVertexShaderConstantF(0, zero9, 9);
// 
//       // Set the pixel shader
// 
//       
//       // average is quite likely to be around 0.5^2.4 ~ 0.18946457081 ~ 48/255
//       
//       // _engine->_lastKnownAvgIntensity is the average from the last frame
//       // _engine->_accomFactor is current exposure settings
//       
//       float histPars[NBrightQueries+1];
//       // CPU reads eyeAccom as determined by the GPU
//       // we are using log representation in the pixels shaders for great precision and range
//       
//       const float avgExpected = 1.0f;
//       const float invAvgExpected = 1.0f/avgExpected;
// 
//       // we want half of the samples to be under avgExpected and half above avgExpected
//       // sample position relative to the expected value
//       // last one is always 1 and it is always ignored
//       // we are not interested about very small changes
//       // on the other hand, GPU exposure control does big changes not very often
//       // the change we do not adjust does gradually build up into a bigger change
//       static const float histInit[NBrightQueries]=
//       {
//         0.1f,0.36f,0.50f,0.70f,0.80f,0.89f,0.93f,0.95f,
//         1.05f,1.07f,1.11f,1.20f,1.30f,1.45f,1.64f,1.99f,
//       };
//       for (int i=0; i<NBrightQueries; i++)
//       {
//         histPars[i] = histInit[i]*avgExpected;
//       }
//       // always ignored
//       histPars[NBrightQueries]=1;
//       
//       // we want most probes to be around the expected values
//       // if the result it too high, the system will avoid too fast adaptation anyway
//       
//       // Draw
//       // Set temporary render states
//       SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE_OLD, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
//       //SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE_OLD, 0);
//       SetRenderStateTemp rsFOGENABLE(D3DRS_FOGENABLE_OLD, FALSE);
//       SetRenderStateTemp rsALPHATESTENABLE(D3DRS_ALPHATESTENABLE_OLD, FALSE);
//       SetRenderStateTemp rsSRGBSRGBWRITEENABLE(D3DRS_SRGBWRITEENABLE_OLD, FALSE);
//       SetSamplerStateTemp tempNoSRGB0(0, D3DSAMP_SRGBTEXTURE_OLD, FALSE);
//       
//       // Set the render target
//       // note: we need depth buffer, otherwise query does not work at all
//       _engine->SetRenderTarget(_rtsMeasure, _rtMeasure, _rtsMeasureDepth);
//       // Set most decimated (1x1) as a texture
//       //_engine->SetTexture(0, _dec[_decimation2x-1]._rt);
//       _engine->SetTexture(0, _rtAperture[_rtApertureCurrent]);
//       
//       // query R only
//       _engine->SetPixelShader(_psQuery);
//       
//       // Drawing
//       float alphas[NBrightQueries];
//       float values[NBrightQueries];
//       for (int b = 0; b < NBrightQueries; b++)
//       {
//         alphas[b] = histPars[b];
//         values[b] = histPars[b]*invAvgExpected*_engine->_accomFactor;
//       }
//       _query.Issue(values,alphas,_engine->_accomFactor);
// 
//       // Build the histogram and check the intensity of the image
//       {
// 
//         // Each level includes all the following levels
//         struct HistogramItem
//         {
//           float minAlpha;
//           float pixels;
//         } histogram[NBrightQueries+2];
// 
//         bool valid = true;
//         int numberOfPixelsDrawn[NBrightQueries];
//         float value[NBrightQueries];
//         float masterValue = _query.GetNumberOfPixelsDrawn(numberOfPixelsDrawn,value);
//         (void)masterValue;
//         for (int b=0; b<NBrightQueries; b++)
//         {
//           histogram[b+1].minAlpha = value[b];
//           histogram[b + 1].pixels = numberOfPixelsDrawn[b];
//           if (numberOfPixelsDrawn[b] < 0) valid = false;
//         }
// 
//         if (valid)
//         {
//           float totalAlpha = 0;
//           // all pixels are over 0 limit
//           histogram[0].minAlpha = 0;
//           histogram[0].pixels = 1 * 1;
//           //histogram[NBrightQueries].maxAlpha = 255*eyeAccom;
//           // no pixels are over 1 limit
//           histogram[NBrightQueries + 1].minAlpha = histogram[NBrightQueries].minAlpha;
//           histogram[NBrightQueries + 1].pixels = 0;
//           float totalError = 0;
//           for (int b = 0; b < NBrightQueries + 1; b++)
//           {
//             float alphaMin = histogram[b].minAlpha;
//             float alphaMax = histogram[b+1].minAlpha;
//             // exclude brighter levels
//             float thisCount = histogram[b].pixels - histogram[b + 1].pixels;
//             float sumAlpha = thisCount * (alphaMin + alphaMax) * 0.5f;
//             float error = thisCount * (alphaMax - alphaMin) * 0.5f;
//             totalAlpha += sumAlpha;
//             totalError += error;
//           }
// 
//           //LogF("Avg %.3f", totalAlpha / ((float(MeasureWidth * MeasureHeight) * 255.0f) / 5.0f));
// 
//           #if _ENABLE_CHEATS && _ENABLE_REPORT && _ENABLE_PERFLOG
//             if (CHECK_DIAG(DEHDR))
//             {
//               DIAG_MESSAGE(
//                 500,Format(
//                   "Avg expected measured %.4f+(%.4f), %.4f..%.4f",
//                   totalAlpha,totalError,totalAlpha-totalError,totalAlpha+totalError
//                 )
//               );
//             }
//           #endif
//           #if _ENABLE_CHEATS
//             if (CHECK_DIAG(DEHDR))
//             {
//               Log("Measured %g (was for eyeAccom %g)",totalAlpha,masterValue);
//             }
//           #endif
//           // if the resulting value is negative, something went wrong
//           DoAssert(totalAlpha>=0);
//           /*
//           // we cannot use masterValue to detect changes, as due to latency this could cause
//           // two values alternating and never reaching a stable state
//           // if the value did not change significantly from the master value, use the master value
//           float newEyeAccom = masterValue;
//           if (
//             fabs(totalAlpha-masterValue)/totalAlpha>4e-2
//           )
//           {
//             newEyeAccom = totalAlpha;
//           }
//           _engine->_lastKnownAvgIntensity = newEyeAccom;
//           */
//           // if the value did not change significantly, do not change it
//           if (
//             _engine->_lastKnownAvgIntensity<=0 ||
//             fabs(totalAlpha-_engine->_lastKnownAvgIntensity)/totalAlpha>4e-2
//           )
//           {
//             _engine->_lastKnownAvgIntensity = totalAlpha;
//           }
//         }
//       }
//     }
// 
//     // Final step - blending of original and the glow
//     {
//       // TODO: use EdgeParams
//       //EdgeParams pars;
//       //pars.SetupPoint(_engine->Width(),_engine->Height(),_engine->Width(),_engine->Height());
//       static const float baseU[4] = {0, 0, 1, 1};
//       static const float baseV[4] = {1, 0, 1, 0};
//       // point sampling requires 0.5 offset to avoid numerical inaccuracy
//       /*
//       Using tgtOffset is more closely what Directly Mapping Texels to Pixels suggests to do
//       const float tgtOffsetU = -0.5f / _engine->Width();
//       const float tgtOffsetV = -0.5f / _engine->Height();
//       const float srcOffsetU = 0;
//       const float srcOffsetV = 0;
//       const float glowOffsetU = 0;
//       const float glowOffsetV = 0;
//       */
//       // previous version, well tuned
//       const float tgtOffsetU = 0;
//       const float tgtOffsetV = 0;
//       const float srcOffsetU = 0.5f / _engine->Width();
//       const float srcOffsetV = 0.5f / _engine->Height();
//       const float glowOffsetU = 0;//1.0f / RoughWidth * 0.5f;
//       const float glowOffsetV = 0;//1.0f / RoughHeight * 0.5f;
//       float U[4][4];
//       float V[4][4];
//       for (int vertex = 0; vertex < 4; vertex++)
//       {
//         // Texture 0 - source image
//         U[vertex][0] = baseU[vertex] + srcOffsetU;
//         V[vertex][0] = baseV[vertex] + srcOffsetV;
//         // Texture 1 - glow buffer
//         U[vertex][1] = baseU[vertex] + glowOffsetU;
//         V[vertex][1] = baseV[vertex] + glowOffsetV;
//         // Texture 2
//         U[vertex][2] = 0;
//         V[vertex][2] = 0;
//         // Texture 3
//         U[vertex][3] = 0;
//         V[vertex][3] = 0;
//       }
//       const float posOffset[4] = {tgtOffsetU,tgtOffsetV,0,0};
//       _engine->SetVertexShaderConstantF(0, U[0], 4);
//       _engine->SetVertexShaderConstantF(4, V[0], 4);
//       _engine->SetVertexShaderConstantF(8, posOffset, 1);
// 
// 
//       // note: stage0 is set to point sampling by default (see FilterPoint in EngineDDT::PostProcess::Prepare)
//       // Set temporary states
//       SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE_OLD, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
//       SetRenderStateTemp rsFOGENABLE(D3DRS_FOGENABLE_OLD, FALSE);
//       SetSamplerStateTemp ss1ADDRESSU(1, D3DSAMP_ADDRESSU_OLD, D3DTADDRESS_CLAMP_OLD);
//       SetSamplerStateTemp ss1ADDRESSV(1, D3DSAMP_ADDRESSV_OLD, D3DTADDRESS_CLAMP_OLD);
//       SetSamplerStateTemp ss1MINFILTER(1, D3DSAMP_MINFILTER_OLD, D3DTEXF_LINEAR_OLD);
//       SetSamplerStateTemp ss1MAGFILTER(1, D3DSAMP_MAGFILTER_OLD, D3DTEXF_LINEAR_OLD);
//       SetSamplerStateTemp ss2MINFILTER(2, D3DSAMP_MINFILTER_OLD, D3DTEXF_POINT_OLD);
//       SetSamplerStateTemp ss2MAGFILTER(2, D3DSAMP_MAGFILTER_OLD, D3DTEXF_POINT_OLD);
// 
//       if (isLast)
//       {
//         // set backbuffer as target
//         _engine->SwitchToBackBufferRenderTarget();
//       }
//       else
//       {
//         // Set the original render target as both target and texture 0
//         _engine->SetRenderTarget(origRts,origRt, NULL);
//       }
//       
//       float hdrMultiply = _engine->GetPostHDRBrightness()/_engine->GetHDRFactor();
//       #if _ENABLE_CHEATS && _ENABLE_REPORT && _ENABLE_PERFLOG
//         if (CHECK_DIAG(DEHDR))
//         {
//           DIAG_MESSAGE(100,Format("Post HDR multiply %.2f",hdrMultiply));
//         }
//       #endif
// 
//       SetTextureTemp tTex0(0, rt); // Set the original render target as texture 0
//       SetTextureTemp tTex1(1, _ppGaussianBlur->GetBlurredTexture()); // TODO: assign glow texture here
//       SetTextureTemp tTex2(2, _rtAperture[_rtApertureCurrent]); // aperture control texture
//       SetTextureTemp tTex3(3, _dec[_decimation2x-1]._rt); // 1x1 maxAvgMin
// 
//       SetSamplerStateTemp tempSRGB0(0, D3DSAMP_SRGBTEXTURE_OLD, rtIsSRGB);
//       SetSamplerStateTemp tempSRGB1(1, D3DSAMP_SRGBTEXTURE_OLD, rtIsSRGB);
//       SetSamplerStateTemp tempSRGB2(2, D3DSAMP_SRGBTEXTURE_OLD, FALSE);
//       SetSamplerStateTemp tempSRGB3(3, D3DSAMP_SRGBTEXTURE_OLD, decimatedSRGB);
// 
//       // compute r,g,b eye sensitivity
// 
//       if (_engine->_nightEye>1e-3f)
//       {
//         // Set the pixel shader
//         _engine->SetPixelShader(_psGlowNight);
// 
//         // Value designed for night blue shift
//         static float coneAdjust = 60.0f;
//         float lumScale = coneAdjust/_engine->_accomFactor;
// 
//         // Get cones
//         float cones = Cones();
// 
//         // Get Eye sens
//         Color eyeSens = EyeSens(cones);
//         
//         // Set the temporary shader constant
//         SetPixelShaderConstantTemp spc6(6, D3DXVECTOR4(eyeSens.R(), eyeSens.G(), eyeSens.B(), 1) );
//         SetPixelShaderConstantTemp spc7(7, D3DXVECTOR4(hdrMultiply,hdrMultiply,hdrMultiply,hdrMultiply));
//         SetPixelShaderConstantTemp spc8(8, D3DXVECTOR4(lumScale, cones, 0,0));
// 
//         // Drawing
//         _engine->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
//       }
//       else
//       {
//         // Set the pixel shader
//         //if (_engine->GetNightVision())
//         ColorVal sens = _engine->GetNightVision() ? NVGSens : DaySens;
//         // Set the temporary shader constant
//         _engine->SetPixelShader(_engine->GetNightVision() ? _psGlowNVG : _psGlow);
// 
//         // Set the temporary shader constant
//         // Drawing
//         SetPixelShaderConstantTemp spc6(6, D3DXVECTOR4(sens.R(), sens.G(), sens.B(), 1));
//         SetPixelShaderConstantTemp spc7(7, D3DXVECTOR4(hdrMultiply,hdrMultiply,hdrMultiply,hdrMultiply));
//         _engine->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
//       }
// 
//     }
// 
//     // unbind any textures to avoid any hanging states
//     _engine->SetTexture(0, NULL);
//     _engine->SetTexture(1, NULL);
//     _engine->SetTexture(2, NULL);
//     _engine->SetTexture(3, NULL);
//     
//     // remember properties of this frame
//     _engine->_accomFactorPrev = _engine->_accomFactor;
//   }
//   // Switching TL off
//   _engine->SwitchTL(TLDisabled);
// }
// 
// float EngineDDT::PPGlow::Cones() const
// {
//   // when adaptation is under 1, we assume iris is fully open
//   // under certain level light is received only by rods
//   // when accomFactor is high, we are receiving lower
// 
//   // subjectiveIntensity = realIntensity*_accomFactor
//   // realIntensity = subjectiveIntensity/_accomFactor;
//   static float coneFull = 0.5f;
//   static float coneNo = 1.1f;
//   float cones = InterpolativC(_engine->_accomFactor,coneFull,coneNo,1,-0.2);
// 
//   return cones;
// }
// 
// Color EngineDDT::PPGlow::EyeSens(float cones) const
// {
//   // make sure we calculate luminance taking rods/cell sensitivity into account
//   Color eyeSens = DaySens*cones+NightSens*(1-cones);
// 
//   return eyeSens;
// }

bool EngineDDT::PPFlareIntensity::Init(const QOStrStream &hlsl, const QOStrStream &vs)
{
  return false;
  // Direct3D 10 NEEDS UPDATE 
//   if (!base::Init(hlsl,vs)) return false;
// 
//   // Create horizontal blur and final blur render targets
//   for (int i = 0; i < RenderTargetNum; i++)
//   {
//     int size = 1 << (RenderTargetNum - i - 1);
//     HRESULT err;
//     err = _engine->GetDirect3DDevice()->CreateTexture(size, size, 1, D3DUSAGE_RENDERTARGET, _engine->_rtFormat, D3DPOOL_DEFAULT, _rtLightValues[i].Init(), NULL);
//     if (err != D3D_OK) {_engine->DDError9("Error: Render target texture creation failed.", err); return false;}
//     err = _rtLightValues[i]->GetSurfaceLevel(0, _rtsLightValues[i].Init());
//     if (err != D3D_OK) {_engine->DDError9("Error: Render target surface creation failed.", err); return false;}
//   }
// 
//   // Success
//   return true;
}

void EngineDDT::PPFlareIntensity::Do(int x, int y, int sizeX, int sizeY)
{
  // Direct3D 10 NEEDS UPDATE 
//   // Draw
//   PROFILE_SCOPE_GRF_EX(ppFlr,pp,SCOPE_COLOR_YELLOW);
// 
//   // If scene is not being rendered initialize scene
//   bool inScene = _engine->_d3dFrameOpen;
//   if (!inScene) _engine->D3DBeginScene();
// 
//   // Prepare drawing
//   Prepare();
// 
//   // Set temporary render states
//   SetRenderStateTemp rsCOLORWRITEENABLE(D3DRS_COLORWRITEENABLE_OLD, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
//   SetRenderStateTemp rsFOGENABLE(D3DRS_FOGENABLE_OLD, FALSE);
// 
//   // Copy original render target surface into small texture
//   RECT rect;
//   rect.left = x - sizeX / 2;
//   rect.right = x + sizeX / 2;
//   rect.top = y - sizeY / 2;
//   rect.bottom = y + sizeY / 2;
//   _engine->GetDirect3DDevice()->StretchRect(_engine->_currRenderTargetSurface, &rect, _rtsLightValues[0], NULL, D3DTEXF_LINEAR_OLD);
// 
//   // Downsize texture up to 1x1 texture
//   for (int i = 1; i < RenderTargetNum; i++)
//   {
//     _engine->GetDirect3DDevice()->StretchRect(_rtsLightValues[i - 1], NULL, _rtsLightValues[i], NULL, D3DTEXF_LINEAR_OLD);
//   }
// 
//   // Set light values texture to stage 15 (overwrite shadow buffer texture)
//   _engine->SetTexture(15, _rtLightValues[RenderTargetNum - 1]);
// 
//   // If scene is not being rendered finish initialized scene
//   if (!inScene) _engine->D3DEndScene();
// 
//   // Restore usual rendering settings
//   _engine->SwitchTL(TLDisabled);
}

#endif