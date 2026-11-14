#pragma once
#ifndef DAYZ_D3D11_CONSTANT_BUFFER_HPP
#define DAYZ_D3D11_CONSTANT_BUFFER_HPP

#include <d3d11.h>

// Dynamic constant buffer with D3D11's required 16-byte size alignment.
class Dx11ConstantBuffer
{
public:
  Dx11ConstantBuffer();
  ~Dx11ConstantBuffer();
  Dx11ConstantBuffer(const Dx11ConstantBuffer &) = delete;
  Dx11ConstantBuffer &operator=(const Dx11ConstantBuffer &) = delete;

  HRESULT Initialize(ID3D11Device *device, UINT byteSize);
  HRESULT Update(ID3D11DeviceContext *context, const void *data, UINT byteSize);
  void BindVS(ID3D11DeviceContext *context, UINT slot) const;
  void BindPS(ID3D11DeviceContext *context, UINT slot) const;
  void Reset();

  UINT ByteSize() const { return _byteSize; }

private:
  ID3D11Buffer *_buffer;
  UINT _byteSize;
};

#endif
