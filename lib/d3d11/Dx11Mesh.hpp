#pragma once
#ifndef DAYZ_D3D11_MESH_HPP
#define DAYZ_D3D11_MESH_HPP

#include <d3d11.h>
#include <d3dcompiler.h>

// Indexed triangle mesh. Shaders and material state are bound by the caller.
class Dx11Mesh
{
public:
  Dx11Mesh();
  ~Dx11Mesh();
  Dx11Mesh(const Dx11Mesh &) = delete;
  Dx11Mesh &operator=(const Dx11Mesh &) = delete;

  HRESULT Initialize(ID3D11Device *device, const void *vertices, UINT vertexCount,
    UINT vertexStride, const void *indices, UINT indexCount, DXGI_FORMAT indexFormat,
    const D3D11_INPUT_ELEMENT_DESC *inputElements, UINT inputElementCount,
    ID3DBlob *vertexShaderBytecode, bool dynamicVertices = false,
    bool dynamicIndices = false);
  // Initializes a vertex-only dynamic stream. LockVertices appends with
  // WRITE_NO_OVERWRITE and wraps with WRITE_DISCARD when the ring is full.
  HRESULT InitializeVertexStream(ID3D11Device *device, UINT vertexCapacity,
    UINT vertexStride);
  HRESULT LockVertices(ID3D11DeviceContext *context, UINT vertexCount,
    UINT &firstVertex, void **data);
  HRESULT UnlockVertices(ID3D11DeviceContext *context);
  HRESULT UpdateVertices(ID3D11DeviceContext *context, const void *vertices,
    UINT byteCount);
  HRESULT UpdateIndices(ID3D11DeviceContext *context, const void *indices,
    UINT byteCount);
  void Draw(ID3D11DeviceContext *context) const;
  HRESULT DrawIndexedRange(ID3D11DeviceContext *context, UINT firstIndex,
    UINT indexCount) const;
  void Reset();

  UINT IndexCount() const { return _indexCount; }
  DXGI_FORMAT IndexFormat() const { return _indexFormat; }
  ID3D11Buffer *VertexBuffer() const { return _vertexBuffer; }
  UINT VertexStride() const { return _vertexStride; }
  UINT VertexCapacity() const { return _vertexCapacity; }

private:
  ID3D11Buffer *_vertexBuffer;
  ID3D11Buffer *_indexBuffer;
  ID3D11InputLayout *_inputLayout;
  UINT _vertexStride;
  UINT _vertexCapacity;
  UINT _vertexWriteCursor;
  UINT _vertexByteWidth;
  UINT _indexByteWidth;
  UINT _indexCount;
  DXGI_FORMAT _indexFormat;
  bool _dynamicVertices;
  bool _dynamicIndices;
  ID3D11DeviceContext *_mappedVertexContext;
  bool _vertexMapped;
};

#endif
