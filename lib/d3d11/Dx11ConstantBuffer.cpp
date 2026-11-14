#include "Dx11ConstantBuffer.hpp"
#include <string.h>

Dx11ConstantBuffer::Dx11ConstantBuffer()
  : _buffer(NULL), _byteSize(0)
{
}

Dx11ConstantBuffer::~Dx11ConstantBuffer()
{
  Reset();
}

void Dx11ConstantBuffer::Reset()
{
  if (_buffer) { _buffer->Release(); _buffer = NULL; }
  _byteSize = 0;
}

HRESULT Dx11ConstantBuffer::Initialize(ID3D11Device *device, UINT byteSize)
{
  const UINT maxByteSize = D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
  if (!device || !byteSize || byteSize > maxByteSize) return E_INVALIDARG;

  const UINT alignedSize = (byteSize + 15u) & ~15u;
  D3D11_BUFFER_DESC desc = {};
  desc.ByteWidth = alignedSize;
  desc.Usage = D3D11_USAGE_DYNAMIC;
  desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

  ID3D11Buffer *buffer = NULL;
  HRESULT hr = device->CreateBuffer(&desc, NULL, &buffer);
  if (FAILED(hr)) return hr;

  Reset();
  _buffer = buffer;
  _byteSize = alignedSize;
  return S_OK;
}

HRESULT Dx11ConstantBuffer::Update(ID3D11DeviceContext *context,
  const void *data, UINT byteSize)
{
  if (!context || !_buffer || !data || !byteSize || byteSize > _byteSize)
    return E_INVALIDARG;

  D3D11_MAPPED_SUBRESOURCE mapped = {};
  HRESULT hr = context->Map(_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  if (FAILED(hr)) return hr;
  memcpy(mapped.pData, data, byteSize);
  if (byteSize < _byteSize)
    memset(static_cast<BYTE *>(mapped.pData) + byteSize, 0, _byteSize - byteSize);
  context->Unmap(_buffer, 0);
  return S_OK;
}

void Dx11ConstantBuffer::BindVS(ID3D11DeviceContext *context, UINT slot) const
{
  if (context && _buffer) context->VSSetConstantBuffers(slot, 1, &_buffer);
}

void Dx11ConstantBuffer::BindPS(ID3D11DeviceContext *context, UINT slot) const
{
  if (context && _buffer) context->PSSetConstantBuffers(slot, 1, &_buffer);
}
