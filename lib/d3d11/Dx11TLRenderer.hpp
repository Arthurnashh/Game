#pragma once
#ifndef DAYZ_D3D11_TL_RENDERER_HPP
#define DAYZ_D3D11_TL_RENDERER_HPP

#include <Es/Common/win.h>
#include <d3d11.h>

class TLVertex;
class Dx11Texture;

// Expanded DX11-only layout used when a model material needs the independent
// UV generators and texture stages stored in its RVMAT. The legacy TLVertex
// layout remains untouched for D3D9 and the 2D UI path.
struct Dx11MaterialTLVertex
{
  float position[3];
  float rhw;
  UINT color;
  UINT specular;
  float uv[8][2];
};

// D3D11 submission path for the engine's already transformed TLVertex data.
// Polygons are expanded to triangle lists because D3D11 has no triangle-fan
// primitive topology.
class Dx11TLRenderer
{
public:
  Dx11TLRenderer();
  ~Dx11TLRenderer();
  Dx11TLRenderer(const Dx11TLRenderer &) = delete;
  Dx11TLRenderer &operator=(const Dx11TLRenderer &) = delete;

  HRESULT Initialize(ID3D11Device *device);
  HRESULT Draw(ID3D11DeviceContext *context, UINT viewportWidth,
    UINT viewportHeight, const TLVertex *vertices, UINT vertexCount,
    const Dx11Texture *texture = NULL, bool depthTest = false,
    bool depthWrite = true, float alphaTestReference = 0.0f,
    bool triangleList = false, const float *fogColor = NULL,
    bool useFog = false, bool alphaBlend = true,
    const Dx11Texture *texture1 = NULL, const Dx11Texture *texture2 = NULL,
    UINT materialMode = 0, bool textureSRGB = false);
  HRESULT DrawMaterial(ID3D11DeviceContext *context, UINT viewportWidth,
    UINT viewportHeight, const Dx11MaterialTLVertex *vertices,
    UINT vertexCount, const Dx11Texture *const *textures,
    const UINT *stageTexGen, UINT stageCount, UINT materialMode,
    bool depthTest, bool depthWrite, float alphaTestReference,
    bool triangleList, const float *fogColor,
    const float *texture0MaxColor, const bool *textureSRGB,
    bool useFog, bool alphaBlend);
  void Reset();

private:
  HRESULT EnsureBuffers(ID3D11Device *device, UINT vertexCount, UINT indexCount);
  HRESULT EnsureMaterialBuffers(ID3D11Device *device, UINT vertexCount,
    UINT indexCount);

  ID3D11VertexShader *_vertexShader;
  ID3D11PixelShader *_pixelShader;
  ID3D11InputLayout *_inputLayout;
  ID3D11VertexShader *_materialVertexShader;
  ID3D11PixelShader *_materialPixelShader;
  ID3D11InputLayout *_materialInputLayout;
  ID3D11Buffer *_constants;
  ID3D11Buffer *_vertices;
  ID3D11Buffer *_materialVertices;
  ID3D11Buffer *_materialConstants;
  ID3D11Buffer *_indices;
  ID3D11BlendState *_blendState;
  ID3D11BlendState *_opaqueBlendState;
  ID3D11DepthStencilState *_depthState;
  ID3D11DepthStencilState *_depthState3D;
  ID3D11DepthStencilState *_depthState3DNoWrite;
  ID3D11RasterizerState *_rasterizerState;
  UINT _vertexCapacity;
  UINT _materialVertexCapacity;
  UINT _indexCapacity;
};

#endif
