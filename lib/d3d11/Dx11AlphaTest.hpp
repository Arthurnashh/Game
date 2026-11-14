#pragma once
#ifndef DAYZ_D3D11_ALPHA_TEST_HPP
#define DAYZ_D3D11_ALPHA_TEST_HPP

#include "Dx11ConstantBuffer.hpp"

// Shader-side replacement for the fixed-function alpha test removed after D3D9.
class Dx11AlphaTest
{
public:
  Dx11AlphaTest();

  HRESULT Initialize(ID3D11Device *device, ID3D11DeviceContext *context);
  HRESULT Set(ID3D11DeviceContext *context, bool enabled,
    UINT sourceComparison, UINT sourceReference);
  void Bind(ID3D11DeviceContext *context, UINT slot = 1) const;
  void Reset();
  static const char *HlslSource();

private:
  struct Constants
  {
    float reference;
    UINT comparison;
    UINT enabled;
    UINT padding;
  };

  Dx11ConstantBuffer _buffer;
};

#endif
