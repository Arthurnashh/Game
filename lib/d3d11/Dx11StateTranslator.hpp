#pragma once
#ifndef DAYZ_D3D11_STATE_TRANSLATOR_HPP
#define DAYZ_D3D11_STATE_TRANSLATOR_HPP

#include <d3d11.h>

// Converts the integer state values stored by the legacy DayZ material/render
// pipeline into native D3D11 descriptors. No Direct3D 9 headers are required.
class Dx11StateTranslator
{
public:
  static bool Comparison(UINT sourceValue, D3D11_COMPARISON_FUNC &result);
  static bool StencilOperation(UINT sourceValue, D3D11_STENCIL_OP &result);
  static bool BlendFactor(UINT sourceValue, D3D11_BLEND &result);
  static bool TextureAddress(UINT sourceValue, D3D11_TEXTURE_ADDRESS_MODE &result);
  static bool CullMode(UINT sourceValue, D3D11_CULL_MODE &result);
  static bool Filter(UINT minFilter, UINT magFilter, UINT mipFilter,
    D3D11_FILTER &result);

  static HRESULT BuildDepthStencil(bool depthEnable, bool depthWrite,
    UINT depthFunction, bool stencilEnable, UINT stencilReadMask,
    UINT stencilWriteMask, UINT stencilPass, UINT stencilFunction,
    D3D11_DEPTH_STENCIL_DESC &result);
  static HRESULT BuildBlend(bool blendEnable, UINT sourceBlend,
    UINT destinationBlend, UINT colorWriteMask, D3D11_BLEND_DESC &result);
  static HRESULT BuildRasterizer(UINT sourceCullMode, D3D11_RASTERIZER_DESC &result);
  static HRESULT BuildSampler(UINT minFilter, UINT magFilter, UINT mipFilter,
    UINT addressU, UINT addressV, UINT maxAnisotropy,
    D3D11_SAMPLER_DESC &result);
};

#endif
