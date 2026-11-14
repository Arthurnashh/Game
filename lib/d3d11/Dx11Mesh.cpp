#include "Dx11Mesh.hpp"
#include <limits.h>
#include <string.h>

Dx11Mesh::Dx11Mesh()
  : _vertexBuffer(NULL), _indexBuffer(NULL), _inputLayout(NULL),
    _vertexStride(0), _vertexCapacity(0), _vertexWriteCursor(0),
    _vertexByteWidth(0), _indexByteWidth(0),
    _indexCount(0), _indexFormat(DXGI_FORMAT_UNKNOWN),
    _dynamicVertices(false), _dynamicIndices(false),
    _mappedVertexContext(NULL), _vertexMapped(false)
{
}

Dx11Mesh::~Dx11Mesh()
{
  Reset();
}

void Dx11Mesh::Reset()
{
  if (_vertexMapped && _mappedVertexContext && _vertexBuffer)
    _mappedVertexContext->Unmap(_vertexBuffer, 0);
  _vertexMapped = false;
  if (_mappedVertexContext)
  {
    _mappedVertexContext->Release();
    _mappedVertexContext = NULL;
  }
  if (_inputLayout) { _inputLayout->Release(); _inputLayout = NULL; }
  if (_indexBuffer) { _indexBuffer->Release(); _indexBuffer = NULL; }
  if (_vertexBuffer) { _vertexBuffer->Release(); _vertexBuffer = NULL; }
  _vertexStride = 0;
  _vertexCapacity = 0;
  _vertexWriteCursor = 0;
  _vertexByteWidth = 0;
  _indexByteWidth = 0;
  _indexCount = 0;
  _indexFormat = DXGI_FORMAT_UNKNOWN;
  _dynamicVertices = false;
  _dynamicIndices = false;
}

HRESULT Dx11Mesh::InitializeVertexStream(ID3D11Device *device,
  UINT vertexCapacity, UINT vertexStride)
{
  if (!device || !vertexCapacity || !vertexStride ||
      vertexCapacity > UINT_MAX / vertexStride)
    return E_INVALIDARG;

  D3D11_BUFFER_DESC desc = {};
  desc.ByteWidth = vertexCapacity * vertexStride;
  desc.Usage = D3D11_USAGE_DYNAMIC;
  desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

  ID3D11Buffer *buffer = NULL;
  HRESULT hr = device->CreateBuffer(&desc, NULL, &buffer);
  if (FAILED(hr)) return hr;

  Reset();
  _vertexBuffer = buffer;
  _vertexStride = vertexStride;
  _vertexCapacity = vertexCapacity;
  _vertexByteWidth = desc.ByteWidth;
  _dynamicVertices = true;
  return S_OK;
}

HRESULT Dx11Mesh::LockVertices(ID3D11DeviceContext *context,
  UINT vertexCount, UINT &firstVertex, void **data)
{
  firstVertex = 0;
  if (data) *data = NULL;
  if (!context || !data || !vertexCount || !_vertexBuffer ||
      !_dynamicVertices || !_vertexStride || _vertexMapped ||
      vertexCount > _vertexCapacity)
    return E_INVALIDARG;

  const bool discard = _vertexWriteCursor == 0 ||
    vertexCount > _vertexCapacity - _vertexWriteCursor;
  const UINT writeOffset = discard ? 0 : _vertexWriteCursor;
  const D3D11_MAP mapType = discard
    ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
  D3D11_MAPPED_SUBRESOURCE mapped = {};
  HRESULT hr = context->Map(_vertexBuffer, 0, mapType, 0, &mapped);
  if (FAILED(hr)) return hr;

  firstVertex = writeOffset;
  *data = static_cast<unsigned char *>(mapped.pData) +
    (size_t)writeOffset * _vertexStride;
  _vertexWriteCursor = writeOffset + vertexCount;
  _mappedVertexContext = context;
  _mappedVertexContext->AddRef();
  _vertexMapped = true;
  return S_OK;
}

HRESULT Dx11Mesh::UnlockVertices(ID3D11DeviceContext *context)
{
  if (!context || !_vertexMapped || context != _mappedVertexContext ||
      !_vertexBuffer)
    return E_INVALIDARG;
  context->Unmap(_vertexBuffer, 0);
  _vertexMapped = false;
  _mappedVertexContext->Release();
  _mappedVertexContext = NULL;
  return S_OK;
}

HRESULT Dx11Mesh::Initialize(ID3D11Device *device, const void *vertices,
  UINT vertexCount, UINT vertexStride, const void *indices, UINT indexCount,
  DXGI_FORMAT indexFormat, const D3D11_INPUT_ELEMENT_DESC *inputElements,
  UINT inputElementCount, ID3DBlob *vertexShaderBytecode,
  bool dynamicVertices, bool dynamicIndices)
{
  if (!device || !vertices || !vertexCount || !vertexStride || !indices ||
      !indexCount || !inputElements || !inputElementCount || !vertexShaderBytecode)
    return E_INVALIDARG;
  if (vertexCount > UINT_MAX / vertexStride) return E_INVALIDARG;

  UINT indexStride;
  if (indexFormat == DXGI_FORMAT_R16_UINT)
    indexStride = sizeof(USHORT);
  else if (indexFormat == DXGI_FORMAT_R32_UINT)
    indexStride = sizeof(UINT);
  else
    return E_INVALIDARG;
  if (indexCount > UINT_MAX / indexStride) return E_INVALIDARG;

  D3D11_BUFFER_DESC vertexDesc = {};
  vertexDesc.ByteWidth = vertexCount * vertexStride;
  vertexDesc.Usage = dynamicVertices ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE;
  vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  vertexDesc.CPUAccessFlags = dynamicVertices ? D3D11_CPU_ACCESS_WRITE : 0;

  D3D11_SUBRESOURCE_DATA vertexData = {};
  vertexData.pSysMem = vertices;

  ID3D11Buffer *vertexBuffer = NULL;
  HRESULT hr = device->CreateBuffer(&vertexDesc, &vertexData, &vertexBuffer);
  if (FAILED(hr)) return hr;

  D3D11_BUFFER_DESC indexDesc = {};
  indexDesc.ByteWidth = indexCount * indexStride;
  indexDesc.Usage = dynamicIndices ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE;
  indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
  indexDesc.CPUAccessFlags = dynamicIndices ? D3D11_CPU_ACCESS_WRITE : 0;

  D3D11_SUBRESOURCE_DATA indexData = {};
  indexData.pSysMem = indices;

  ID3D11Buffer *indexBuffer = NULL;
  hr = device->CreateBuffer(&indexDesc, &indexData, &indexBuffer);
  if (FAILED(hr))
  {
    vertexBuffer->Release();
    return hr;
  }

  ID3D11InputLayout *inputLayout = NULL;
  hr = device->CreateInputLayout(inputElements, inputElementCount,
    vertexShaderBytecode->GetBufferPointer(), vertexShaderBytecode->GetBufferSize(),
    &inputLayout);
  if (FAILED(hr))
  {
    indexBuffer->Release();
    vertexBuffer->Release();
    return hr;
  }

  Reset();
  _vertexBuffer = vertexBuffer;
  _indexBuffer = indexBuffer;
  _inputLayout = inputLayout;
  _vertexStride = vertexStride;
  _vertexCapacity = vertexCount;
  _vertexWriteCursor = 0;
  _vertexByteWidth = vertexCount * vertexStride;
  _indexByteWidth = indexCount * indexStride;
  _indexCount = indexCount;
  _indexFormat = indexFormat;
  _dynamicVertices = dynamicVertices;
  _dynamicIndices = dynamicIndices;
  return S_OK;
}

HRESULT Dx11Mesh::UpdateVertices(ID3D11DeviceContext *context,
  const void *vertices, UINT byteCount)
{
  if (!context || !vertices || !byteCount || !_vertexBuffer || !_dynamicVertices ||
      _vertexMapped || byteCount != _vertexByteWidth)
    return E_INVALIDARG;

  D3D11_MAPPED_SUBRESOURCE mapped = {};
  HRESULT hr = context->Map(_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  if (FAILED(hr)) return hr;
  memcpy(mapped.pData, vertices, byteCount);
  context->Unmap(_vertexBuffer, 0);
  _vertexWriteCursor = 0;
  return S_OK;
}

HRESULT Dx11Mesh::UpdateIndices(ID3D11DeviceContext *context,
  const void *indices, UINT byteCount)
{
  if (!context || !indices || !byteCount || !_indexBuffer || !_dynamicIndices ||
      byteCount != _indexByteWidth)
    return E_INVALIDARG;

  D3D11_MAPPED_SUBRESOURCE mapped = {};
  HRESULT hr = context->Map(_indexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  if (FAILED(hr)) return hr;
  memcpy(mapped.pData, indices, byteCount);
  context->Unmap(_indexBuffer, 0);
  return S_OK;
}

void Dx11Mesh::Draw(ID3D11DeviceContext *context) const
{
  DrawIndexedRange(context, 0, _indexCount);
}

HRESULT Dx11Mesh::DrawIndexedRange(ID3D11DeviceContext *context, UINT firstIndex,
  UINT indexCount) const
{
  if (!context || !_vertexBuffer || !_indexBuffer || !_inputLayout || !indexCount ||
      firstIndex > _indexCount || indexCount > _indexCount - firstIndex)
    return E_INVALIDARG;

  const UINT offset = 0;
  context->IASetInputLayout(_inputLayout);
  context->IASetVertexBuffers(0, 1, &_vertexBuffer, &_vertexStride, &offset);
  context->IASetIndexBuffer(_indexBuffer, _indexFormat, 0);
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context->DrawIndexed(indexCount, firstIndex, 0);
  return S_OK;
}
