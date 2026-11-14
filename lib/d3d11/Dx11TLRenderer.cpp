#include "Dx11TLRenderer.hpp"
#include "Dx11ShaderCompiler.hpp"
#include "Dx11Texture.hpp"
#include "../tlVertex.hpp"
#include <limits.h>
#include <stddef.h>
#include <string.h>

namespace
{
struct TLConstants
{
  float viewportWidth;
  float viewportHeight;
  UINT hasTexture;
  UINT padding;
  float alphaTestReference;
  UINT useFog;
  UINT materialMode;
  UINT padding2;
  float fogColor[4];
};

struct MaterialTLConstants
{
  float viewportWidth;
  float viewportHeight;
  UINT stageCount;
  UINT materialMode;
  float alphaTestReference;
  UINT useFog;
  UINT padding[2];
  float fogColor[4];
  float texture0MaxColor[4];
  UINT textureMask[4];
  UINT stageTexGen[16];
};

const char *TLVertexShader =
  "cbuffer TLConstants : register(b0) { float width; float height; uint hasTexture; uint pad; float alphaTestReference; uint useFog; uint2 pad2; float4 fogColor; };\n"
  "struct VSIn { float3 position : POSITION; float rhw : RHW; float4 color : COLOR0; float2 uv : TEXCOORD0; float4 specular : COLOR1; };\n"
  "struct VSOut { float4 position : SV_Position; float4 color : COLOR0; float2 uv : TEXCOORD0; float fogTransmittance : TEXCOORD1; };\n"
  "VSOut main(VSIn i) { VSOut o; float safeRhw = abs(i.rhw) < 1.0e-20 ? (i.rhw < 0.0 ? -1.0e-20 : 1.0e-20) : i.rhw; float w = 1.0 / safeRhw;\n"
  "float2 ndc = float2(i.position.x * (2.0 / width) - 1.0, 1.0 - i.position.y * (2.0 / height));\n"
  "o.position = float4(ndc * w, i.position.z * w, w); o.color = i.color.bgra; o.uv = i.uv; o.fogTransmittance = i.specular.a; return o; }\n";

const char *TLPixelShader =
  "cbuffer TLConstants : register(b0) { float width; float height; uint hasTexture; uint pad; float alphaTestReference; uint useFog; uint materialMode; uint pad2; float4 fogColor; };\n"
  "Texture2D tex0 : register(t0); Texture2D tex1 : register(t1); Texture2D tex2 : register(t2);\n"
  "SamplerState samp0 : register(s0); SamplerState samp1 : register(s1); SamplerState samp2 : register(s2);\n"
  "struct PSIn { float4 position : SV_Position; float4 color : COLOR0; float2 uv : TEXCOORD0; float fogTransmittance : TEXCOORD1; };\n"
  "float4 main(PSIn i) : SV_Target { float4 c = i.color; if (hasTexture != 0) c *= tex0.Sample(samp0, i.uv); "
  "if (materialMode == 1) c.rgb *= tex2.Sample(samp2, i.uv).rgb * 4.5947; "
  "if (alphaTestReference > 0.0 && c.a < alphaTestReference) discard; if (useFog != 0) c.rgb = lerp(fogColor.rgb, c.rgb, saturate(i.fogTransmittance)); return c; }\n";

const char *MaterialTLVertexShader =
  "cbuffer MaterialTLConstants : register(b0) { float width; float height; uint stageCount; uint materialMode; float alphaTestReference; uint useFog; uint2 padding; float4 fogColor; float4 texture0MaxColor; uint4 textureMask; uint4 stageTexGen[4]; };\n"
  "struct VSIn { float3 position : POSITION; float rhw : RHW; float4 color : COLOR0; float4 specular : COLOR1; "
  "float2 uv0 : TEXCOORD0; float2 uv1 : TEXCOORD1; float2 uv2 : TEXCOORD2; float2 uv3 : TEXCOORD3; "
  "float2 uv4 : TEXCOORD4; float2 uv5 : TEXCOORD5; float2 uv6 : TEXCOORD6; float2 uv7 : TEXCOORD7; };\n"
  "struct VSOut { float4 position : SV_Position; float4 color : COLOR0; float fogTransmittance : TEXCOORD8; "
  "float2 uv0 : TEXCOORD0; float2 uv1 : TEXCOORD1; float2 uv2 : TEXCOORD2; float2 uv3 : TEXCOORD3; "
  "float2 uv4 : TEXCOORD4; float2 uv5 : TEXCOORD5; float2 uv6 : TEXCOORD6; float2 uv7 : TEXCOORD7; };\n"
  "VSOut main(VSIn i) { VSOut o; float safeRhw = abs(i.rhw) < 1.0e-20 ? (i.rhw < 0.0 ? -1.0e-20 : 1.0e-20) : i.rhw; float w = 1.0 / safeRhw; "
  "float2 ndc = float2(i.position.x * (2.0 / width) - 1.0, 1.0 - i.position.y * (2.0 / height)); "
  "o.position = float4(ndc * w, i.position.z * w, w); o.color = i.color.bgra; o.fogTransmittance = i.specular.a; "
  "o.uv0=i.uv0; o.uv1=i.uv1; o.uv2=i.uv2; o.uv3=i.uv3; o.uv4=i.uv4; o.uv5=i.uv5; o.uv6=i.uv6; o.uv7=i.uv7; return o; }\n";

const char *MaterialTLPixelShader =
  "cbuffer MaterialTLConstants : register(b0) { float width; float height; uint stageCount; uint materialMode; float alphaTestReference; uint useFog; uint2 padding; float4 fogColor; float4 texture0MaxColor; uint4 textureMask; uint4 stageTexGen[4]; };\n"
  "Texture2D tex0 : register(t0); Texture2D tex1 : register(t1); Texture2D tex2 : register(t2); Texture2D tex3 : register(t3); Texture2D tex4 : register(t4); "
  "Texture2D tex5 : register(t5); Texture2D tex6 : register(t6); Texture2D tex7 : register(t7); Texture2D tex8 : register(t8); Texture2D tex9 : register(t9); "
  "Texture2D tex10 : register(t10); Texture2D tex11 : register(t11); Texture2D tex12 : register(t12); Texture2D tex13 : register(t13); Texture2D tex14 : register(t14);\n"
  "SamplerState samp0 : register(s0); SamplerState samp1 : register(s1); SamplerState samp2 : register(s2); SamplerState samp3 : register(s3); SamplerState samp4 : register(s4); "
  "SamplerState samp5 : register(s5); SamplerState samp6 : register(s6); SamplerState samp7 : register(s7); SamplerState samp8 : register(s8); SamplerState samp9 : register(s9); "
  "SamplerState samp10 : register(s10); SamplerState samp11 : register(s11); SamplerState samp12 : register(s12); SamplerState samp13 : register(s13); SamplerState samp14 : register(s14);\n"
  "struct PSIn { float4 position : SV_Position; float4 color : COLOR0; float fogTransmittance : TEXCOORD8; "
  "float2 uv0 : TEXCOORD0; float2 uv1 : TEXCOORD1; float2 uv2 : TEXCOORD2; float2 uv3 : TEXCOORD3; "
  "float2 uv4 : TEXCOORD4; float2 uv5 : TEXCOORD5; float2 uv6 : TEXCOORD6; float2 uv7 : TEXCOORD7; };\n"
  "bool HasStage(uint stage) { return (textureMask[stage >> 2] & (1u << (stage & 3))) != 0; }\n"
  "float2 TexGenUV(PSIn i, uint gen) { if(gen==1) return i.uv1; if(gen==2) return i.uv2; if(gen==3) return i.uv3; if(gen==4) return i.uv4; if(gen==5) return i.uv5; if(gen==6) return i.uv6; if(gen==7) return i.uv7; return i.uv0; }\n"
  "uint StageTexGen(uint stage) { return stageTexGen[stage >> 2][stage & 3]; }\n"
  "float4 SampleStage(PSIn i, uint stage) { float2 uv=TexGenUV(i,StageTexGen(stage)); "
  "if(stage==0) return tex0.Sample(samp0,uv); if(stage==1) return tex1.Sample(samp1,uv); if(stage==2) return tex2.Sample(samp2,uv); if(stage==3) return tex3.Sample(samp3,uv); if(stage==4) return tex4.Sample(samp4,uv); "
  "if(stage==5) return tex5.Sample(samp5,uv); if(stage==6) return tex6.Sample(samp6,uv); if(stage==7) return tex7.Sample(samp7,uv); if(stage==8) return tex8.Sample(samp8,uv); if(stage==9) return tex9.Sample(samp9,uv); "
  "if(stage==10) return tex10.Sample(samp10,uv); if(stage==11) return tex11.Sample(samp11,uv); if(stage==12) return tex12.Sample(samp12,uv); if(stage==13) return tex13.Sample(samp13,uv); return tex14.Sample(samp14,uv); }\n"
  "float4 SampleStageLod(PSIn i, uint stage, float lod) { float2 uv=TexGenUV(i,StageTexGen(stage)); "
  "if(stage==0) return tex0.SampleLevel(samp0,uv,lod); if(stage==1) return tex1.SampleLevel(samp1,uv,lod); if(stage==2) return tex2.SampleLevel(samp2,uv,lod); if(stage==3) return tex3.SampleLevel(samp3,uv,lod); if(stage==4) return tex4.SampleLevel(samp4,uv,lod); "
  "if(stage==5) return tex5.SampleLevel(samp5,uv,lod); if(stage==6) return tex6.SampleLevel(samp6,uv,lod); if(stage==7) return tex7.SampleLevel(samp7,uv,lod); if(stage==8) return tex8.SampleLevel(samp8,uv,lod); if(stage==9) return tex9.SampleLevel(samp9,uv,lod); "
  "if(stage==10) return tex10.SampleLevel(samp10,uv,lod); if(stage==11) return tex11.SampleLevel(samp11,uv,lod); if(stage==12) return tex12.SampleLevel(samp12,uv,lod); if(stage==13) return tex13.SampleLevel(samp13,uv,lod); return tex14.SampleLevel(samp14,uv,lod); }\n"
  "float4 main(PSIn i) : SV_Target { float4 base=HasStage(0)?SampleStage(i,0)*texture0MaxColor:float4(1,1,1,1); float3 rgb=base.rgb; "
  "if((materialMode==1 || materialMode==4 || materialMode==5) && HasStage(2)) rgb *= SampleStage(i,2).rgb * 4.5947; "
  "else if(materialMode==2) { float3 detail=HasStage(2)?SampleStage(i,2).rgb:float3(0.5,0.5,0.5); float4 macro=HasStage(3)?SampleStage(i,3):float4(0,0,0,0); rgb=lerp(rgb,macro.rgb,macro.a)*detail*2.0; } "
  "else if(materialMode==3) { float3 mask=HasStage(4)?SampleStage(i,4).rgb:float3(0,0,0); if(HasStage(1)) rgb=lerp(rgb,SampleStage(i,1).rgb,mask.r); if(HasStage(2)) rgb=lerp(rgb,SampleStage(i,2).rgb,mask.g); if(HasStage(3)) rgb=lerp(rgb,SampleStage(i,3).rgb,mask.b); "
  // PSMulti stages 5..8 are SMDI data maps (specular/metal/gloss), not
  // diffuse detail textures. The legacy shader uses them for lighting only;
  // multiplying diffuse RGB by their red channel creates strong false tints.
  "if(HasStage(9)) { float4 macro=SampleStage(i,9); float3 average=SampleStageLod(i,0,20).rgb*texture0MaxColor.rgb; if(HasStage(1)) average=lerp(average,SampleStageLod(i,1,20).rgb,mask.r); if(HasStage(2)) average=lerp(average,SampleStageLod(i,2,20).rgb,mask.g); if(HasStage(3)) average=lerp(average,SampleStageLod(i,3,20).rgb,mask.b); float3 adjust=clamp(macro.rgb/max(average,float3(0.0001,0.0001,0.0001)),float3(0,0,0),float3(2,2,2)); rgb=lerp(rgb,rgb*adjust,macro.a); } } "
  "else if(materialMode==6) { float3 detail=HasStage(1)?SampleStage(i,1).rgb:float3(0.5,0.5,0.5); rgb*=detail*2.0; } "
  "else if(materialMode==7) { float3 detail=HasStage(1)?SampleStage(i,1).rgb:float3(0.5,0.5,0.5); float4 macro=HasStage(2)?SampleStage(i,2):float4(0,0,0,0); rgb=lerp(rgb,macro.rgb,macro.a)*detail*2.0; } "
  "else if(materialMode==10) { float3 detail=HasStage(2)?SampleStage(i,2).rgb:float3(0.5,0.5,0.5); float4 macro=HasStage(3)?SampleStage(i,3):float4(0,0,0,0); rgb=lerp(rgb,macro.rgb,macro.a)*detail*2.0; } "
  "else if(materialMode==11) { float4 macro=HasStage(2)?SampleStage(i,2):float4(0,0,0,0); rgb=lerp(rgb,macro.rgb,macro.a); } "
  "else if(materialMode==14) { float3 detail=HasStage(2)?SampleStage(i,2).rgb:float3(0.5,0.5,0.5); rgb*=detail*2.0; } "
  "else if((materialMode==8 || materialMode==12) && HasStage(2)) rgb*=SampleStage(i,2).rgb; "
  "float alpha=base.a*i.color.a; if((materialMode==1 || materialMode==8) && HasStage(1)) alpha+=SampleStage(i,1).a-0.5; if(materialMode==1 || materialMode==8 || materialMode==9) alpha=alpha>0.5?1.0:0.0; if(materialMode==5) alpha=1.0; "
  "float4 c=float4(rgb*i.color.rgb,alpha); if(alphaTestReference>0.0 && c.a<alphaTestReference) discard; if(useFog!=0) c.rgb=lerp(fogColor.rgb,c.rgb,saturate(i.fogTransmittance)); return c; }\n";

UINT GrowCapacity(UINT required)
{
  UINT capacity = 32;
  while (capacity < required && capacity <= 0x7fffffffu) capacity <<= 1;
  return capacity < required ? required : capacity;
}
}

Dx11TLRenderer::Dx11TLRenderer()
  : _vertexShader(NULL), _pixelShader(NULL), _inputLayout(NULL),
    _materialVertexShader(NULL), _materialPixelShader(NULL),
    _materialInputLayout(NULL), _constants(NULL), _vertices(NULL),
    _materialVertices(NULL),
    _materialConstants(NULL), _indices(NULL), _blendState(NULL),
    _opaqueBlendState(NULL),
    _depthState(NULL),
    _depthState3D(NULL), _depthState3DNoWrite(NULL), _rasterizerState(NULL),
    _vertexCapacity(0), _materialVertexCapacity(0), _indexCapacity(0)
{
}

Dx11TLRenderer::~Dx11TLRenderer()
{
  Reset();
}

void Dx11TLRenderer::Reset()
{
  if (_rasterizerState) { _rasterizerState->Release(); _rasterizerState = NULL; }
  if (_materialConstants) { _materialConstants->Release(); _materialConstants = NULL; }
  if (_materialVertices) { _materialVertices->Release(); _materialVertices = NULL; }
  if (_materialInputLayout) { _materialInputLayout->Release(); _materialInputLayout = NULL; }
  if (_materialPixelShader) { _materialPixelShader->Release(); _materialPixelShader = NULL; }
  if (_materialVertexShader) { _materialVertexShader->Release(); _materialVertexShader = NULL; }
  if (_depthState3DNoWrite) { _depthState3DNoWrite->Release(); _depthState3DNoWrite = NULL; }
  if (_depthState3D) { _depthState3D->Release(); _depthState3D = NULL; }
  if (_depthState) { _depthState->Release(); _depthState = NULL; }
  if (_opaqueBlendState) { _opaqueBlendState->Release(); _opaqueBlendState = NULL; }
  if (_blendState) { _blendState->Release(); _blendState = NULL; }
  if (_indices) { _indices->Release(); _indices = NULL; }
  if (_vertices) { _vertices->Release(); _vertices = NULL; }
  if (_constants) { _constants->Release(); _constants = NULL; }
  if (_inputLayout) { _inputLayout->Release(); _inputLayout = NULL; }
  if (_pixelShader) { _pixelShader->Release(); _pixelShader = NULL; }
  if (_vertexShader) { _vertexShader->Release(); _vertexShader = NULL; }
  _vertexCapacity = 0;
  _materialVertexCapacity = 0;
  _indexCapacity = 0;
}

HRESULT Dx11TLRenderer::Initialize(ID3D11Device *device)
{
  if (!device) return E_INVALIDARG;
  Reset();

  ID3DBlob *vsCode = NULL;
  HRESULT hr = Dx11ShaderCompiler::CompileVertexShader(device, TLVertexShader,
    strlen(TLVertexShader), "main", &_vertexShader, &vsCode);
  if (FAILED(hr)) return hr;

  const D3D11_INPUT_ELEMENT_DESC elements[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, (UINT)offsetof(TLVertex, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "RHW", 0, DXGI_FORMAT_R32_FLOAT, 0, (UINT)offsetof(TLVertex, rhw), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)offsetof(TLVertex, color), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, (UINT)offsetof(TLVertex, t0), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR", 1, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)offsetof(TLVertex, specular), D3D11_INPUT_PER_VERTEX_DATA, 0 }
  };
  hr = device->CreateInputLayout(elements, ARRAYSIZE(elements), vsCode->GetBufferPointer(),
    vsCode->GetBufferSize(), &_inputLayout);
  vsCode->Release();
  if (FAILED(hr)) { Reset(); return hr; }

  ID3DBlob *psCode = NULL;
  hr = Dx11ShaderCompiler::CompilePixelShader(device, TLPixelShader,
    strlen(TLPixelShader), "main", &_pixelShader, &psCode);
  if (FAILED(hr)) { Reset(); return hr; }
  psCode->Release();

  ID3DBlob *materialVsCode = NULL;
  hr = Dx11ShaderCompiler::CompileVertexShader(device, MaterialTLVertexShader,
    strlen(MaterialTLVertexShader), "main", &_materialVertexShader,
    &materialVsCode);
  if (FAILED(hr)) { Reset(); return hr; }
  D3D11_INPUT_ELEMENT_DESC materialElements[12] = {};
  materialElements[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
    (UINT)offsetof(Dx11MaterialTLVertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 };
  materialElements[1] = { "RHW", 0, DXGI_FORMAT_R32_FLOAT, 0,
    (UINT)offsetof(Dx11MaterialTLVertex, rhw), D3D11_INPUT_PER_VERTEX_DATA, 0 };
  materialElements[2] = { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0,
    (UINT)offsetof(Dx11MaterialTLVertex, color), D3D11_INPUT_PER_VERTEX_DATA, 0 };
  materialElements[3] = { "COLOR", 1, DXGI_FORMAT_R8G8B8A8_UNORM, 0,
    (UINT)offsetof(Dx11MaterialTLVertex, specular), D3D11_INPUT_PER_VERTEX_DATA, 0 };
  for (UINT uv = 0; uv < 8; ++uv)
    materialElements[4 + uv] = { "TEXCOORD", uv, DXGI_FORMAT_R32G32_FLOAT, 0,
      (UINT)offsetof(Dx11MaterialTLVertex, uv) + uv * sizeof(float) * 2,
      D3D11_INPUT_PER_VERTEX_DATA, 0 };
  hr = device->CreateInputLayout(materialElements, ARRAYSIZE(materialElements),
    materialVsCode->GetBufferPointer(), materialVsCode->GetBufferSize(),
    &_materialInputLayout);
  materialVsCode->Release();
  if (FAILED(hr)) { Reset(); return hr; }

  ID3DBlob *materialPsCode = NULL;
  hr = Dx11ShaderCompiler::CompilePixelShader(device, MaterialTLPixelShader,
    strlen(MaterialTLPixelShader), "main", &_materialPixelShader,
    &materialPsCode);
  if (FAILED(hr)) { Reset(); return hr; }
  materialPsCode->Release();

  D3D11_BUFFER_DESC constantDesc = {};
  constantDesc.ByteWidth = sizeof(TLConstants);
  constantDesc.Usage = D3D11_USAGE_DYNAMIC;
  constantDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  constantDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  hr = device->CreateBuffer(&constantDesc, NULL, &_constants);
  if (FAILED(hr)) { Reset(); return hr; }

  constantDesc.ByteWidth = sizeof(MaterialTLConstants);
  hr = device->CreateBuffer(&constantDesc, NULL, &_materialConstants);
  if (FAILED(hr)) { Reset(); return hr; }

  D3D11_BLEND_DESC blend = {};
  D3D11_RENDER_TARGET_BLEND_DESC &rt = blend.RenderTarget[0];
  rt.BlendEnable = TRUE;
  rt.SrcBlend = D3D11_BLEND_SRC_ALPHA;
  rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
  rt.BlendOp = D3D11_BLEND_OP_ADD;
  rt.SrcBlendAlpha = D3D11_BLEND_ONE;
  rt.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
  rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
  rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  hr = device->CreateBlendState(&blend, &_blendState);
  if (FAILED(hr)) { Reset(); return hr; }

  // Opaque materials must ignore texture alpha. The previous path blended
  // every world section, which turned opaque terrain/building faces into
  // translucent, overlapping purple slabs whenever their source alpha was
  // not fully opaque.
  rt.BlendEnable = FALSE;
  hr = device->CreateBlendState(&blend, &_opaqueBlendState);
  if (FAILED(hr)) { Reset(); return hr; }

  D3D11_DEPTH_STENCIL_DESC depth = {};
  depth.DepthEnable = FALSE;
  depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
  depth.DepthFunc = D3D11_COMPARISON_ALWAYS;
  depth.StencilEnable = FALSE;
  depth.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
  depth.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
  hr = device->CreateDepthStencilState(&depth, &_depthState);
  if (FAILED(hr)) { Reset(); return hr; }

  depth.DepthEnable = TRUE;
  depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
  depth.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
  hr = device->CreateDepthStencilState(&depth, &_depthState3D);
  if (FAILED(hr)) { Reset(); return hr; }

  depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
  hr = device->CreateDepthStencilState(&depth, &_depthState3DNoWrite);
  if (FAILED(hr)) { Reset(); return hr; }

  D3D11_RASTERIZER_DESC rasterizer = {};
  rasterizer.FillMode = D3D11_FILL_SOLID;
  rasterizer.CullMode = D3D11_CULL_NONE;
  rasterizer.DepthClipEnable = TRUE;
  hr = device->CreateRasterizerState(&rasterizer, &_rasterizerState);
  if (FAILED(hr)) { Reset(); return hr; }
  return S_OK;
}

HRESULT Dx11TLRenderer::EnsureBuffers(ID3D11Device *device, UINT vertexCount,
  UINT indexCount)
{
  if (!device || !vertexCount || !indexCount ||
      vertexCount > UINT_MAX / sizeof(TLVertex) ||
      indexCount > UINT_MAX / sizeof(USHORT)) return E_INVALIDARG;

  if (vertexCount > _vertexCapacity)
  {
    UINT capacity = GrowCapacity(vertexCount);
    if (capacity > UINT_MAX / sizeof(TLVertex)) return E_INVALIDARG;
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = capacity * sizeof(TLVertex);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ID3D11Buffer *buffer = NULL;
    HRESULT hr = device->CreateBuffer(&desc, NULL, &buffer);
    if (FAILED(hr)) return hr;
    if (_vertices) _vertices->Release();
    _vertices = buffer;
    _vertexCapacity = capacity;
  }

  if (indexCount > _indexCapacity)
  {
    UINT capacity = GrowCapacity(indexCount);
    if (capacity > UINT_MAX / sizeof(USHORT)) return E_INVALIDARG;
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = capacity * sizeof(USHORT);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ID3D11Buffer *buffer = NULL;
    HRESULT hr = device->CreateBuffer(&desc, NULL, &buffer);
    if (FAILED(hr)) return hr;
    if (_indices) _indices->Release();
    _indices = buffer;
    _indexCapacity = capacity;
  }
  return S_OK;
}

HRESULT Dx11TLRenderer::EnsureMaterialBuffers(ID3D11Device *device,
  UINT vertexCount, UINT indexCount)
{
  if (!device || !vertexCount || !indexCount ||
      vertexCount > UINT_MAX / sizeof(Dx11MaterialTLVertex) ||
      indexCount > UINT_MAX / sizeof(USHORT)) return E_INVALIDARG;

  if (vertexCount > _materialVertexCapacity)
  {
    UINT capacity = GrowCapacity(vertexCount);
    if (capacity > UINT_MAX / sizeof(Dx11MaterialTLVertex)) return E_INVALIDARG;
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = capacity * sizeof(Dx11MaterialTLVertex);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ID3D11Buffer *buffer = NULL;
    HRESULT hr = device->CreateBuffer(&desc, NULL, &buffer);
    if (FAILED(hr)) return hr;
    if (_materialVertices) _materialVertices->Release();
    _materialVertices = buffer;
    _materialVertexCapacity = capacity;
  }

  if (indexCount > _indexCapacity)
  {
    UINT capacity = GrowCapacity(indexCount);
    if (capacity > UINT_MAX / sizeof(USHORT)) return E_INVALIDARG;
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = capacity * sizeof(USHORT);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ID3D11Buffer *buffer = NULL;
    HRESULT hr = device->CreateBuffer(&desc, NULL, &buffer);
    if (FAILED(hr)) return hr;
    if (_indices) _indices->Release();
    _indices = buffer;
    _indexCapacity = capacity;
  }
  return S_OK;
}

HRESULT Dx11TLRenderer::Draw(ID3D11DeviceContext *context, UINT viewportWidth,
  UINT viewportHeight, const TLVertex *vertices, UINT vertexCount,
  const Dx11Texture *texture, bool depthTest, bool depthWrite,
  float alphaTestReference, bool triangleList, const float *fogColor,
  bool useFog, bool alphaBlend, const Dx11Texture *texture1,
  const Dx11Texture *texture2, UINT materialMode, bool textureSRGB)
{
  if (!context || !viewportWidth || !viewportHeight || !vertices || vertexCount < 3 ||
      vertexCount > 65535 || vertexCount - 2 > UINT_MAX / 3 || !_vertexShader ||
      !_pixelShader || !_inputLayout || !_constants || !_blendState ||
      !_opaqueBlendState || !_depthState ||
      !_depthState3D || !_depthState3DNoWrite || !_rasterizerState)
    return E_INVALIDARG;
  if (triangleList && vertexCount % 3 != 0) return E_INVALIDARG;

  const UINT indexCount = triangleList ? vertexCount : (vertexCount - 2) * 3;
  ID3D11Device *device = NULL;
  context->GetDevice(&device);
  if (!device) return E_FAIL;
  HRESULT hr = EnsureBuffers(device, vertexCount, indexCount);
  device->Release();
  if (FAILED(hr)) return hr;

  D3D11_MAPPED_SUBRESOURCE mapped = {};
  hr = context->Map(_vertices, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  if (FAILED(hr)) return hr;
  memcpy(mapped.pData, vertices, vertexCount * sizeof(TLVertex));
  context->Unmap(_vertices, 0);

  USHORT *indices = NULL;
  hr = context->Map(_indices, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  if (FAILED(hr)) return hr;
  indices = (USHORT *)mapped.pData;
  if (triangleList)
  {
    for (UINT i = 0; i < vertexCount; ++i)
      indices[i] = (USHORT)i;
  }
  else
  {
    for (UINT i = 0; i < vertexCount - 2; ++i)
    {
      indices[i * 3 + 0] = 0;
      indices[i * 3 + 1] = (USHORT)(i + 1);
      indices[i * 3 + 2] = (USHORT)(i + 2);
    }
  }
  context->Unmap(_indices, 0);

  TLConstants constants = {};
  constants.viewportWidth = (float)viewportWidth;
  constants.viewportHeight = (float)viewportHeight;
  constants.hasTexture = texture ? 1 : 0;
  constants.alphaTestReference = alphaTestReference;
  constants.useFog = useFog ? 1 : 0;
  constants.materialMode = materialMode;
  if (fogColor) memcpy(constants.fogColor, fogColor, sizeof(constants.fogColor));
  hr = context->Map(_constants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  if (FAILED(hr)) return hr;
  memcpy(mapped.pData, &constants, sizeof(constants));
  context->Unmap(_constants, 0);

  context->VSSetShader(_vertexShader, NULL, 0);
  context->PSSetShader(_pixelShader, NULL, 0);
  context->IASetInputLayout(_inputLayout);
  const UINT stride = sizeof(TLVertex), offset = 0;
  context->IASetVertexBuffers(0, 1, &_vertices, &stride, &offset);
  context->IASetIndexBuffer(_indices, DXGI_FORMAT_R16_UINT, 0);
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context->VSSetConstantBuffers(0, 1, &_constants);
  context->PSSetConstantBuffers(0, 1, &_constants);
  if (texture)
    texture->Bind(context, 0, textureSRGB);
  else
  {
    ID3D11ShaderResourceView *nullView = NULL;
    ID3D11SamplerState *nullSampler = NULL;
    context->PSSetShaderResources(0, 1, &nullView);
    context->PSSetSamplers(0, 1, &nullSampler);
  }
  if (texture1)
    texture1->Bind(context, 1);
  else
  {
    ID3D11ShaderResourceView *nullView = NULL;
    ID3D11SamplerState *nullSampler = NULL;
    context->PSSetShaderResources(1, 1, &nullView);
    context->PSSetSamplers(1, 1, &nullSampler);
  }
  if (texture2)
    texture2->Bind(context, 2);
  else
  {
    ID3D11ShaderResourceView *nullView = NULL;
    ID3D11SamplerState *nullSampler = NULL;
    context->PSSetShaderResources(2, 1, &nullView);
    context->PSSetSamplers(2, 1, &nullSampler);
  }
  const float blendFactor[4] = {0, 0, 0, 0};
  context->OMSetBlendState(alphaBlend ? _blendState : _opaqueBlendState,
    blendFactor, 0xffffffff);
  ID3D11DepthStencilState *depthState = !depthTest
    ? _depthState : (depthWrite ? _depthState3D : _depthState3DNoWrite);
  context->OMSetDepthStencilState(depthState, 0);
  context->RSSetState(_rasterizerState);
  context->DrawIndexed(indexCount, 0, 0);
  return S_OK;
}

HRESULT Dx11TLRenderer::DrawMaterial(ID3D11DeviceContext *context,
  UINT viewportWidth, UINT viewportHeight,
  const Dx11MaterialTLVertex *vertices, UINT vertexCount,
  const Dx11Texture *const *textures, const UINT *stageTexGen,
  UINT stageCount, UINT materialMode, bool depthTest, bool depthWrite,
  float alphaTestReference, bool triangleList, const float *fogColor,
  const float *texture0MaxColor, const bool *textureSRGB,
  bool useFog, bool alphaBlend)
{
  if (!context || !viewportWidth || !viewportHeight || !vertices ||
      vertexCount < 3 || vertexCount > 65535 || !textures || !stageTexGen ||
      !stageCount || stageCount > 15 || vertexCount - 2 > UINT_MAX / 3 ||
      !_materialVertexShader || !_materialPixelShader ||
      !_materialInputLayout || !_materialConstants || !_blendState ||
      !_opaqueBlendState || !_depthState || !_depthState3D ||
      !_depthState3DNoWrite || !_rasterizerState)
    return E_INVALIDARG;
  if (triangleList && vertexCount % 3 != 0) return E_INVALIDARG;

  const UINT indexCount = triangleList ? vertexCount : (vertexCount - 2) * 3;
  ID3D11Device *device = NULL;
  context->GetDevice(&device);
  if (!device) return E_FAIL;
  HRESULT hr = EnsureMaterialBuffers(device, vertexCount, indexCount);
  device->Release();
  if (FAILED(hr)) return hr;

  D3D11_MAPPED_SUBRESOURCE mapped = {};
  hr = context->Map(_materialVertices, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  if (FAILED(hr)) return hr;
  memcpy(mapped.pData, vertices, vertexCount * sizeof(Dx11MaterialTLVertex));
  context->Unmap(_materialVertices, 0);

  hr = context->Map(_indices, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  if (FAILED(hr)) return hr;
  USHORT *indices = (USHORT *)mapped.pData;
  if (triangleList)
  {
    for (UINT i = 0; i < vertexCount; ++i)
      indices[i] = (USHORT)i;
  }
  else
  {
    for (UINT i = 0; i < vertexCount - 2; ++i)
    {
      indices[i * 3 + 0] = 0;
      indices[i * 3 + 1] = (USHORT)(i + 1);
      indices[i * 3 + 2] = (USHORT)(i + 2);
    }
  }
  context->Unmap(_indices, 0);

  MaterialTLConstants constants = {};
  constants.viewportWidth = (float)viewportWidth;
  constants.viewportHeight = (float)viewportHeight;
  constants.stageCount = stageCount;
  constants.materialMode = materialMode;
  constants.alphaTestReference = alphaTestReference;
  constants.useFog = useFog ? 1 : 0;
  if (fogColor) memcpy(constants.fogColor, fogColor, sizeof(constants.fogColor));
  if (texture0MaxColor)
    memcpy(constants.texture0MaxColor, texture0MaxColor,
      sizeof(constants.texture0MaxColor));
  else
  {
    constants.texture0MaxColor[0] = 1.0f;
    constants.texture0MaxColor[1] = 1.0f;
    constants.texture0MaxColor[2] = 1.0f;
    constants.texture0MaxColor[3] = 1.0f;
  }
  for (UINT stage = 0; stage < stageCount; ++stage)
  {
    const UINT texGen = stageTexGen[stage] < 8 ? stageTexGen[stage] : 0;
    constants.stageTexGen[stage] = texGen;
    if (textures[stage])
      constants.textureMask[stage >> 2] |= 1u << (stage & 3);
  }
  hr = context->Map(_materialConstants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  if (FAILED(hr)) return hr;
  memcpy(mapped.pData, &constants, sizeof(constants));
  context->Unmap(_materialConstants, 0);

  context->VSSetShader(_materialVertexShader, NULL, 0);
  context->PSSetShader(_materialPixelShader, NULL, 0);
  context->IASetInputLayout(_materialInputLayout);
  const UINT stride = sizeof(Dx11MaterialTLVertex), offset = 0;
  context->IASetVertexBuffers(0, 1, &_materialVertices, &stride, &offset);
  context->IASetIndexBuffer(_indices, DXGI_FORMAT_R16_UINT, 0);
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context->VSSetConstantBuffers(0, 1, &_materialConstants);
  context->PSSetConstantBuffers(0, 1, &_materialConstants);

  ID3D11ShaderResourceView *views[15] = {};
  ID3D11SamplerState *samplers[15] = {};
  for (UINT stage = 0; stage < stageCount; ++stage)
  {
    if (!textures[stage]) continue;
    views[stage] = textures[stage]->ResourceView(
      textureSRGB && textureSRGB[stage]);
    samplers[stage] = textures[stage]->Sampler();
  }
  context->PSSetShaderResources(0, ARRAYSIZE(views), views);
  context->PSSetSamplers(0, ARRAYSIZE(samplers), samplers);

  const float blendFactor[4] = {0, 0, 0, 0};
  context->OMSetBlendState(alphaBlend ? _blendState : _opaqueBlendState,
    blendFactor, 0xffffffff);
  ID3D11DepthStencilState *depthState = !depthTest
    ? _depthState : (depthWrite ? _depthState3D : _depthState3DNoWrite);
  context->OMSetDepthStencilState(depthState, 0);
  context->RSSetState(_rasterizerState);
  context->DrawIndexed(indexCount, 0, 0);
  return S_OK;
}
