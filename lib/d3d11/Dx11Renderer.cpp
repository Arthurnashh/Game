#include "../wpch.hpp"
#include "Dx11Renderer.hpp"
#include "Dx11ShaderCompiler.hpp"
#include "Dx11StateTranslator.hpp"
#include <string.h>
#include <string>

#define DX11_CHECK_STAGE(stage) \
  do { if (FAILED(hr)) { \
    RptF("DX11: %s failed (0x%08x)", stage, (unsigned)hr); \
    Reset(); return hr; \
  } } while (0)

Dx11Renderer::Dx11Renderer()
  : _vertexShader(NULL), _pixelShader(NULL), _rasterizerState(NULL),
    _depthStencilState(NULL), _blendState(NULL), _transformConstants(NULL),
    _worldViewProjection(), _hasTexture(false), _vertexShaderBytecode(NULL)
{
}

Dx11Renderer::~Dx11Renderer()
{
  Reset();
}

void Dx11Renderer::Reset()
{
  if (_blendState) { _blendState->Release(); _blendState = NULL; }
  if (_transformConstants) { _transformConstants->Release(); _transformConstants = NULL; }
  if (_depthStencilState) { _depthStencilState->Release(); _depthStencilState = NULL; }
  if (_rasterizerState) { _rasterizerState->Release(); _rasterizerState = NULL; }
  if (_pixelShader) { _pixelShader->Release(); _pixelShader = NULL; }
  if (_vertexShader) { _vertexShader->Release(); _vertexShader = NULL; }
  if (_vertexShaderBytecode) { _vertexShaderBytecode->Release(); _vertexShaderBytecode = NULL; }
  _tlRenderer.Reset();
  _alphaTest.Reset();
  _device.Reset();
}

HRESULT Dx11Renderer::Initialize(HWND window, UINT width, UINT height,
  const char *vertexShaderSource, const char *vertexEntryPoint,
  const char *pixelShaderSource, const char *pixelEntryPoint, bool windowed)
{
  if (!vertexShaderSource || !vertexEntryPoint || !pixelShaderSource || !pixelEntryPoint)
    return E_INVALIDARG;

  Reset();
  HRESULT hr = _device.Create(window, width, height, windowed);
  DX11_CHECK_STAGE("device/swap chain creation");

  hr = _tlRenderer.Initialize(_device.Device());
  DX11_CHECK_STAGE("TL renderer initialization");

  ID3DBlob *vertexDiagnostics = NULL;
  hr = Dx11ShaderCompiler::CompileVertexShader(_device.Device(), vertexShaderSource,
    strlen(vertexShaderSource), vertexEntryPoint, &_vertexShader,
    &_vertexShaderBytecode, &vertexDiagnostics);
  if (FAILED(hr) && vertexDiagnostics)
    RptF("DX11: vertex shader diagnostic: %s",
      (const char *)vertexDiagnostics->GetBufferPointer());
  if (vertexDiagnostics) vertexDiagnostics->Release();
  DX11_CHECK_STAGE("engine vertex shader compilation");

  ID3DBlob *pixelBytecode = NULL;
  ID3DBlob *pixelDiagnostics = NULL;
  std::string pixelSource(pixelShaderSource);
  pixelSource += Dx11AlphaTest::HlslSource();
  hr = Dx11ShaderCompiler::CompilePixelShader(_device.Device(), pixelSource.c_str(),
    pixelSource.size(), pixelEntryPoint, &_pixelShader,
    &pixelBytecode, &pixelDiagnostics);
  if (FAILED(hr) && pixelDiagnostics)
    RptF("DX11: pixel shader diagnostic: %s",
      (const char *)pixelDiagnostics->GetBufferPointer());
  if (pixelBytecode) pixelBytecode->Release();
  if (pixelDiagnostics) pixelDiagnostics->Release();
  DX11_CHECK_STAGE("engine pixel shader compilation");

  D3D11_RASTERIZER_DESC rasterizer;
  hr = Dx11StateTranslator::BuildRasterizer(3, rasterizer);
  DX11_CHECK_STAGE("rasterizer descriptor creation");
  hr = _device.Device()->CreateRasterizerState(&rasterizer, &_rasterizerState);
  DX11_CHECK_STAGE("rasterizer state creation");

  D3D11_DEPTH_STENCIL_DESC depthStencil;
  hr = Dx11StateTranslator::BuildDepthStencil(true, true, 4, false,
    0xff, 0xff, 1, 8, depthStencil);
  DX11_CHECK_STAGE("depth-stencil descriptor creation");
  hr = _device.Device()->CreateDepthStencilState(&depthStencil, &_depthStencilState);
  DX11_CHECK_STAGE("depth-stencil state creation");

  D3D11_BLEND_DESC blend;
  hr = Dx11StateTranslator::BuildBlend(false, 2, 1,
    D3D11_COLOR_WRITE_ENABLE_ALL, blend);
  DX11_CHECK_STAGE("blend descriptor creation");
  hr = _device.Device()->CreateBlendState(&blend, &_blendState);
  DX11_CHECK_STAGE("blend state creation");

  D3D11_BUFFER_DESC transformDesc = {};
  transformDesc.ByteWidth = sizeof(float) * 20;
  transformDesc.Usage = D3D11_USAGE_DYNAMIC;
  transformDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  transformDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  hr = _device.Device()->CreateBuffer(&transformDesc, NULL, &_transformConstants);
  DX11_CHECK_STAGE("transform constant buffer creation");
  const float identity[16] = {
    1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
  };
  hr = SetWorldViewProjection(identity);
  DX11_CHECK_STAGE("initial transform upload");

  hr = _alphaTest.Initialize(_device.Device(), _device.Context());
  DX11_CHECK_STAGE("alpha-test state initialization");

  return S_OK;
}

#undef DX11_CHECK_STAGE

HRESULT Dx11Renderer::Resize(UINT width, UINT height)
{
  return _device.Resize(width, height);
}

HRESULT Dx11Renderer::Clear(float red, float green, float blue, float alpha,
  bool clearColor, bool clearDepth, bool clearStencil)
{
  return _device.Clear(red, green, blue, alpha,
    clearColor, clearDepth, clearStencil);
}

HRESULT Dx11Renderer::BeginFrame(float red, float green, float blue, float alpha)
{
  return _device.BeginFrame(red, green, blue, alpha);
}

HRESULT Dx11Renderer::SetAlphaTest(bool enabled, UINT sourceComparison, UINT sourceReference)
{
  return _alphaTest.Set(_device.Context(), enabled, sourceComparison, sourceReference);
}

HRESULT Dx11Renderer::SetLinearColorOutput(bool enabled)
{
  return _device.SetLinearColorOutput(enabled);
}

HRESULT Dx11Renderer::DrawTLVertices(UINT viewportWidth, UINT viewportHeight,
  const TLVertex *vertices, UINT vertexCount, const Dx11Texture *texture,
  bool depthTest, bool depthWrite, float alphaTestReference, bool triangleList,
  const float *fogColor, bool useFog, bool alphaBlend,
  const Dx11Texture *texture1, const Dx11Texture *texture2, UINT materialMode,
  bool textureSRGB)
{
  return _tlRenderer.Draw(_device.Context(), viewportWidth, viewportHeight,
    vertices, vertexCount, texture, depthTest, depthWrite, alphaTestReference,
    triangleList, fogColor, useFog, alphaBlend, texture1, texture2,
    materialMode, textureSRGB);
}

HRESULT Dx11Renderer::DrawMaterialTLVertices(UINT viewportWidth,
  UINT viewportHeight, const Dx11MaterialTLVertex *vertices, UINT vertexCount,
  const Dx11Texture *const *textures, const UINT *stageTexGen,
  UINT stageCount, UINT materialMode, bool depthTest, bool depthWrite,
  float alphaTestReference, bool triangleList, const float *fogColor,
  const float *texture0MaxColor, const bool *textureSRGB,
  bool useFog, bool alphaBlend)
{
  return _tlRenderer.DrawMaterial(_device.Context(), viewportWidth,
    viewportHeight, vertices, vertexCount, textures, stageTexGen, stageCount,
    materialMode, depthTest, depthWrite, alphaTestReference, triangleList,
    fogColor, texture0MaxColor, textureSRGB, useFog, alphaBlend);
}

void Dx11Renderer::DrawIndexed(const Dx11Mesh &mesh, const Dx11Texture *texture)
{
  ID3D11DeviceContext *context = _device.Context();
  if (!context || !_vertexShader || !_pixelShader) return;

  context->VSSetShader(_vertexShader, NULL, 0);
  context->VSSetConstantBuffers(0, 1, &_transformConstants);
  context->PSSetConstantBuffers(0, 1, &_transformConstants);
  context->PSSetShader(_pixelShader, NULL, 0);
  _alphaTest.Bind(context);
  context->RSSetState(_rasterizerState);
  context->OMSetDepthStencilState(_depthStencilState, 0);
  const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  context->OMSetBlendState(_blendState, blendFactor, 0xffffffff);

  if (texture)
    texture->Bind(context, 0);
  else
  {
    ID3D11ShaderResourceView *nullView = NULL;
    ID3D11SamplerState *nullSampler = NULL;
    context->PSSetShaderResources(0, 1, &nullView);
    context->PSSetSamplers(0, 1, &nullSampler);
  }
  mesh.Draw(context);
}

HRESULT Dx11Renderer::DrawShapeSection(const Dx11ShapeMesh &mesh,
  UINT sectionIndex, const Dx11Texture *texture)
{
  ID3D11DeviceContext *context = _device.Context();
  if (!context || !_vertexShader || !_pixelShader) return E_FAIL;
  HRESULT constantsHr = UpdateTransformConstants(texture != NULL);
  if (FAILED(constantsHr)) return constantsHr;

  context->VSSetShader(_vertexShader, NULL, 0);
  context->VSSetConstantBuffers(0, 1, &_transformConstants);
  context->PSSetConstantBuffers(0, 1, &_transformConstants);
  context->PSSetShader(_pixelShader, NULL, 0);
  _alphaTest.Bind(context);
  context->RSSetState(_rasterizerState);
  context->OMSetDepthStencilState(_depthStencilState, 0);
  const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  context->OMSetBlendState(_blendState, blendFactor, 0xffffffff);
  if (texture)
    texture->Bind(context, 0);
  else
  {
    ID3D11ShaderResourceView *nullView = NULL;
    ID3D11SamplerState *nullSampler = NULL;
    context->PSSetShaderResources(0, 1, &nullView);
    context->PSSetSamplers(0, 1, &nullSampler);
  }
  return mesh.DrawSection(context, sectionIndex);
}

HRESULT Dx11Renderer::SetWorldViewProjection(const float matrixRowMajor[16])
{
  if (!matrixRowMajor) return E_INVALIDARG;
  memcpy(_worldViewProjection, matrixRowMajor, sizeof(_worldViewProjection));
  return UpdateTransformConstants(_hasTexture);
}

HRESULT Dx11Renderer::UpdateTransformConstants(bool hasTexture)
{
  if (!_device.Context() || !_transformConstants) return E_FAIL;
  struct TransformConstants
  {
    float worldViewProjection[16];
    UINT hasTexture;
    UINT padding[3];
  } constants = {};
  memcpy(constants.worldViewProjection, _worldViewProjection,
    sizeof(constants.worldViewProjection));
  constants.hasTexture = hasTexture ? 1 : 0;
  D3D11_MAPPED_SUBRESOURCE mapped = {};
  HRESULT hr = _device.Context()->Map(_transformConstants, 0,
    D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  if (FAILED(hr)) return hr;
  memcpy(mapped.pData, &constants, sizeof(constants));
  _device.Context()->Unmap(_transformConstants, 0);
  _hasTexture = hasTexture;
  return S_OK;
}

HRESULT Dx11Renderer::EndFrame(UINT syncInterval)
{
  return _device.EndFrame(syncInterval);
}
