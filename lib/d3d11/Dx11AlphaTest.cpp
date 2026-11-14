#include "Dx11AlphaTest.hpp"
#include "Dx11StateTranslator.hpp"

Dx11AlphaTest::Dx11AlphaTest()
{
}

void Dx11AlphaTest::Reset()
{
  _buffer.Reset();
}

HRESULT Dx11AlphaTest::Initialize(ID3D11Device *device,
  ID3D11DeviceContext *context)
{
  if (!device || !context) return E_INVALIDARG;
  HRESULT hr = _buffer.Initialize(device, sizeof(Constants));
  if (FAILED(hr)) return hr;
  return Set(context, false, 8, 0);
}

HRESULT Dx11AlphaTest::Set(ID3D11DeviceContext *context, bool enabled,
  UINT sourceComparison, UINT sourceReference)
{
  D3D11_COMPARISON_FUNC comparison;
  if (!Dx11StateTranslator::Comparison(sourceComparison, comparison)) return E_INVALIDARG;
  if (sourceReference > 255) return E_INVALIDARG;

  Constants constants;
  constants.reference = static_cast<float>(sourceReference) / 255.0f;
  constants.comparison = static_cast<UINT>(comparison);
  constants.enabled = enabled ? 1u : 0u;
  constants.padding = 0;
  return _buffer.Update(context, &constants, sizeof(constants));
}

void Dx11AlphaTest::Bind(ID3D11DeviceContext *context, UINT slot) const
{
  _buffer.BindPS(context, slot);
}

const char *Dx11AlphaTest::HlslSource()
{
  static const char source[] = R"HLSL(
// Pixel shader model 4.0 exposes constant-buffer slots b0-b13. Keep this
// shared state in the first free slot after the engine's b0 constants.
cbuffer Dx11AlphaTestConstants : register(b1)
{
  float Dx11AlphaReference;
  uint Dx11AlphaComparison;
  uint Dx11AlphaEnabled;
  uint Dx11AlphaPadding;
};

bool Dx11AlphaTestPass(float alpha)
{
  if (Dx11AlphaEnabled == 0) return true;
  switch (Dx11AlphaComparison)
  {
  case 1: return false;
  case 2: return alpha <  Dx11AlphaReference;
  case 3: return alpha == Dx11AlphaReference;
  case 4: return alpha <= Dx11AlphaReference;
  case 5: return alpha >  Dx11AlphaReference;
  case 6: return alpha != Dx11AlphaReference;
  case 7: return alpha >= Dx11AlphaReference;
  case 8: return true;
  default: return false;
  }
}

void Dx11ApplyAlphaTest(float alpha)
{
  if (Dx11AlphaEnabled != 0)
    clip(Dx11AlphaTestPass(alpha) ? 1.0f : -1.0f);
}
)HLSL";
  return source;
}
