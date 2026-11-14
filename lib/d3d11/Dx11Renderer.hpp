#pragma once
#ifndef DAYZ_D3D11_RENDERER_HPP
#define DAYZ_D3D11_RENDERER_HPP

#include <Es/Common/win.h>
#include "Dx11Device.hpp"
#include "Dx11Mesh.hpp"
#include "Dx11Texture.hpp"
#include "Dx11AlphaTest.hpp"
#include "Dx11TLRenderer.hpp"
#include "Dx11ShapeMesh.hpp"
#include <d3dcompiler.h>

// Minimal indexed-mesh renderer. The caller supplies engine-specific shaders,
// vertex formats, and per-frame/per-object constants through Context().
class Dx11Renderer
{
public:
  Dx11Renderer();
  ~Dx11Renderer();
  Dx11Renderer(const Dx11Renderer &) = delete;
  Dx11Renderer &operator=(const Dx11Renderer &) = delete;

  HRESULT Initialize(HWND window, UINT width, UINT height,
    const char *vertexShaderSource, const char *vertexEntryPoint,
    const char *pixelShaderSource, const char *pixelEntryPoint,
    bool windowed = true);
  HRESULT Resize(UINT width, UINT height);
  HRESULT Clear(float red, float green, float blue, float alpha,
    bool clearColor, bool clearDepth, bool clearStencil);
  HRESULT BeginFrame(float red, float green, float blue, float alpha = 1.0f);
  HRESULT SetLinearColorOutput(bool enabled);
  HRESULT SetAlphaTest(bool enabled, UINT sourceComparison, UINT sourceReference);
  HRESULT DrawTLVertices(UINT viewportWidth, UINT viewportHeight,
    const TLVertex *vertices, UINT vertexCount, const Dx11Texture *texture = NULL,
    bool depthTest = false, bool depthWrite = true,
    float alphaTestReference = 0.0f, bool triangleList = false,
    const float *fogColor = NULL, bool useFog = false,
    bool alphaBlend = true, const Dx11Texture *texture1 = NULL,
    const Dx11Texture *texture2 = NULL, UINT materialMode = 0,
    bool textureSRGB = false);
  HRESULT DrawMaterialTLVertices(UINT viewportWidth, UINT viewportHeight,
    const Dx11MaterialTLVertex *vertices, UINT vertexCount,
    const Dx11Texture *const *textures, const UINT *stageTexGen,
    UINT stageCount, UINT materialMode, bool depthTest, bool depthWrite,
    float alphaTestReference, bool triangleList, const float *fogColor,
    const float *texture0MaxColor, const bool *textureSRGB,
    bool useFog, bool alphaBlend);
  void DrawIndexed(const Dx11Mesh &mesh, const Dx11Texture *texture = NULL);
  HRESULT DrawShapeSection(const Dx11ShapeMesh &mesh, UINT sectionIndex,
    const Dx11Texture *texture = NULL);
  HRESULT SetWorldViewProjection(const float matrixRowMajor[16]);
  HRESULT EndFrame(UINT syncInterval = 1);
  void Reset();

  ID3D11Device *Device() const { return _device.Device(); }
  ID3D11DeviceContext *Context() const { return _device.Context(); }
  ID3DBlob *VertexShaderBytecode() const { return _vertexShaderBytecode; }

private:
  Dx11Device _device;
  ID3D11VertexShader *_vertexShader;
  ID3D11PixelShader *_pixelShader;
  ID3D11RasterizerState *_rasterizerState;
  ID3D11DepthStencilState *_depthStencilState;
  ID3D11BlendState *_blendState;
  ID3D11Buffer *_transformConstants;
  float _worldViewProjection[16];
  bool _hasTexture;
  ID3DBlob *_vertexShaderBytecode;
  Dx11AlphaTest _alphaTest;
  Dx11TLRenderer _tlRenderer;

  HRESULT UpdateTransformConstants(bool hasTexture);
};

#endif
