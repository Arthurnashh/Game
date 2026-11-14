#include "Dx11StateTranslator.hpp"
#include <float.h>

bool Dx11StateTranslator::Comparison(UINT value, D3D11_COMPARISON_FUNC &result)
{
  switch (value)
  {
  case 1: result = D3D11_COMPARISON_NEVER; return true;
  case 2: result = D3D11_COMPARISON_LESS; return true;
  case 3: result = D3D11_COMPARISON_EQUAL; return true;
  case 4: result = D3D11_COMPARISON_LESS_EQUAL; return true;
  case 5: result = D3D11_COMPARISON_GREATER; return true;
  case 6: result = D3D11_COMPARISON_NOT_EQUAL; return true;
  case 7: result = D3D11_COMPARISON_GREATER_EQUAL; return true;
  case 8: result = D3D11_COMPARISON_ALWAYS; return true;
  default: return false;
  }
}

bool Dx11StateTranslator::StencilOperation(UINT value, D3D11_STENCIL_OP &result)
{
  switch (value)
  {
  case 1: result = D3D11_STENCIL_OP_KEEP; return true;
  case 2: result = D3D11_STENCIL_OP_ZERO; return true;
  case 3: result = D3D11_STENCIL_OP_REPLACE; return true;
  case 4: result = D3D11_STENCIL_OP_INCR_SAT; return true;
  case 5: result = D3D11_STENCIL_OP_DECR_SAT; return true;
  case 6: result = D3D11_STENCIL_OP_INVERT; return true;
  case 7: result = D3D11_STENCIL_OP_INCR; return true;
  case 8: result = D3D11_STENCIL_OP_DECR; return true;
  default: return false;
  }
}

bool Dx11StateTranslator::BlendFactor(UINT value, D3D11_BLEND &result)
{
  switch (value)
  {
  case 1: result = D3D11_BLEND_ZERO; return true;
  case 2: result = D3D11_BLEND_ONE; return true;
  case 3: result = D3D11_BLEND_SRC_COLOR; return true;
  case 4: result = D3D11_BLEND_INV_SRC_COLOR; return true;
  case 5: result = D3D11_BLEND_SRC_ALPHA; return true;
  case 6: result = D3D11_BLEND_INV_SRC_ALPHA; return true;
  case 7: result = D3D11_BLEND_DEST_ALPHA; return true;
  case 8: result = D3D11_BLEND_INV_DEST_ALPHA; return true;
  case 9: result = D3D11_BLEND_DEST_COLOR; return true;
  case 10: result = D3D11_BLEND_INV_DEST_COLOR; return true;
  case 11: result = D3D11_BLEND_SRC_ALPHA_SAT; return true;
  case 14: result = D3D11_BLEND_BLEND_FACTOR; return true;
  case 15: result = D3D11_BLEND_INV_BLEND_FACTOR; return true;
  case 16: result = D3D11_BLEND_SRC1_COLOR; return true;
  case 17: result = D3D11_BLEND_INV_SRC1_COLOR; return true;
  case 18: result = D3D11_BLEND_SRC1_ALPHA; return true;
  case 19: result = D3D11_BLEND_INV_SRC1_ALPHA; return true;
  default: return false;
  }
}

bool Dx11StateTranslator::TextureAddress(UINT value, D3D11_TEXTURE_ADDRESS_MODE &result)
{
  switch (value)
  {
  case 1: result = D3D11_TEXTURE_ADDRESS_WRAP; return true;
  case 2: result = D3D11_TEXTURE_ADDRESS_MIRROR; return true;
  case 3: result = D3D11_TEXTURE_ADDRESS_CLAMP; return true;
  case 4: result = D3D11_TEXTURE_ADDRESS_BORDER; return true;
  case 5: result = D3D11_TEXTURE_ADDRESS_MIRROR_ONCE; return true;
  default: return false;
  }
}

bool Dx11StateTranslator::CullMode(UINT value, D3D11_CULL_MODE &result)
{
  switch (value)
  {
  case 1: result = D3D11_CULL_NONE; return true;
  case 2: result = D3D11_CULL_FRONT; return true;
  case 3: result = D3D11_CULL_BACK; return true;
  default: return false;
  }
}

bool Dx11StateTranslator::Filter(UINT minFilter, UINT magFilter, UINT mipFilter,
  D3D11_FILTER &result)
{
  // The legacy no-mip-filter value is zero; point mip sampling is the closest
  // D3D11 equivalent because D3D11 always selects a mip level.
  if (mipFilter == 0) mipFilter = 1;
  if (minFilter == 3 && magFilter == 3 && mipFilter == 3)
  {
    result = D3D11_FILTER_ANISOTROPIC;
    return true;
  }
  if ((minFilter != 1 && minFilter != 2) ||
      (magFilter != 1 && magFilter != 2) ||
      (mipFilter != 1 && mipFilter != 2))
    return false;

  const bool minLinear = minFilter == 2;
  const bool magLinear = magFilter == 2;
  const bool mipLinear = mipFilter == 2;
  if (!minLinear && !magLinear && !mipLinear) result = D3D11_FILTER_MIN_MAG_MIP_POINT;
  else if (!minLinear && !magLinear && mipLinear) result = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
  else if (!minLinear && magLinear && !mipLinear) result = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
  else if (!minLinear && magLinear && mipLinear) result = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
  else if (minLinear && !magLinear && !mipLinear) result = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
  else if (minLinear && !magLinear && mipLinear) result = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
  else if (minLinear && magLinear && !mipLinear) result = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
  else result = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  return true;
}

HRESULT Dx11StateTranslator::BuildDepthStencil(bool depthEnable, bool depthWrite,
  UINT depthFunction, bool stencilEnable, UINT stencilReadMask,
  UINT stencilWriteMask, UINT stencilPass, UINT stencilFunction,
  D3D11_DEPTH_STENCIL_DESC &result)
{
  D3D11_COMPARISON_FUNC depthFunc;
  D3D11_COMPARISON_FUNC stencilFunc;
  D3D11_STENCIL_OP passOp;
  if (stencilReadMask > 0xff || stencilWriteMask > 0xff) return E_INVALIDARG;
  if (!Comparison(depthFunction, depthFunc) || !Comparison(stencilFunction, stencilFunc) ||
      !StencilOperation(stencilPass, passOp))
    return E_INVALIDARG;

  result = D3D11_DEPTH_STENCIL_DESC();
  result.DepthEnable = depthEnable ? TRUE : FALSE;
  result.DepthWriteMask = depthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
  result.DepthFunc = depthFunc;
  result.StencilEnable = stencilEnable ? TRUE : FALSE;
  result.StencilReadMask = static_cast<UINT8>(stencilReadMask);
  result.StencilWriteMask = static_cast<UINT8>(stencilWriteMask);
  result.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  result.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
  result.FrontFace.StencilPassOp = passOp;
  result.FrontFace.StencilFunc = stencilFunc;
  result.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  result.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
  result.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
  result.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
  return S_OK;
}

HRESULT Dx11StateTranslator::BuildBlend(bool blendEnable, UINT sourceBlend,
  UINT destinationBlend, UINT colorWriteMask, D3D11_BLEND_DESC &result)
{
  D3D11_BLEND src;
  D3D11_BLEND dst;
  if (!BlendFactor(sourceBlend, src) || !BlendFactor(destinationBlend, dst))
    return E_INVALIDARG;

  result = D3D11_BLEND_DESC();
  D3D11_RENDER_TARGET_BLEND_DESC &target = result.RenderTarget[0];
  target.BlendEnable = blendEnable ? TRUE : FALSE;
  target.SrcBlend = src;
  target.DestBlend = dst;
  target.BlendOp = D3D11_BLEND_OP_ADD;
  target.SrcBlendAlpha = D3D11_BLEND_ONE;
  target.DestBlendAlpha = D3D11_BLEND_ZERO;
  target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
  target.RenderTargetWriteMask = static_cast<UINT8>(colorWriteMask & D3D11_COLOR_WRITE_ENABLE_ALL);
  return S_OK;
}

HRESULT Dx11StateTranslator::BuildRasterizer(UINT sourceCullMode,
  D3D11_RASTERIZER_DESC &result)
{
  D3D11_CULL_MODE cull;
  if (!CullMode(sourceCullMode, cull)) return E_INVALIDARG;

  result = D3D11_RASTERIZER_DESC();
  result.FillMode = D3D11_FILL_SOLID;
  result.CullMode = cull;
  result.FrontCounterClockwise = FALSE;
  result.DepthClipEnable = TRUE;
  return S_OK;
}

HRESULT Dx11StateTranslator::BuildSampler(UINT minFilter, UINT magFilter,
  UINT mipFilter, UINT addressU, UINT addressV, UINT maxAnisotropy,
  D3D11_SAMPLER_DESC &result)
{
  D3D11_FILTER filter;
  D3D11_TEXTURE_ADDRESS_MODE u;
  D3D11_TEXTURE_ADDRESS_MODE v;
  if (!Filter(minFilter, magFilter, mipFilter, filter) ||
      !TextureAddress(addressU, u) || !TextureAddress(addressV, v))
    return E_INVALIDARG;

  result = D3D11_SAMPLER_DESC();
  result.Filter = filter;
  result.AddressU = u;
  result.AddressV = v;
  result.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  result.MaxAnisotropy = maxAnisotropy < 1 ? 1 : (maxAnisotropy > 16 ? 16 : maxAnisotropy);
  result.ComparisonFunc = D3D11_COMPARISON_NEVER;
  result.BorderColor[0] = result.BorderColor[1] = result.BorderColor[2] = result.BorderColor[3] = 1.0f;
  result.MinLOD = -FLT_MAX;
  result.MaxLOD = FLT_MAX;
  return S_OK;
}
