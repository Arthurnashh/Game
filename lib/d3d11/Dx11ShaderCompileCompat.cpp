#include <windows.h>
#include <d3dcompiler.h>
#include "Dx11LegacyCompat.hpp"

HRESULT CompileShaderD3D11Compat(
  LPCSTR source,
  SIZE_T sourceLength,
  LPCSTR sourceName,
  const void *macros,
  void *includeHandler,
  LPCSTR entryPoint,
  LPCSTR target,
  UINT compileFlags,
  UINT effectFlags,
  void **compiledShader,
  void **errors)
{
  // D3DXMACRO has the same two-pointer layout as D3D_SHADER_MACRO, and the
  // adapter exposes ID3DInclude's two callback slots with the same ABI.
  UINT nativeFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
  if (compileFlags & DX11_COMPAT_COMPILE_DEBUG)
    nativeFlags |= D3DCOMPILE_DEBUG;
  if (compileFlags & DX11_COMPAT_COMPILE_SKIP_OPTIMIZATION)
  {
    nativeFlags &= ~D3DCOMPILE_OPTIMIZATION_LEVEL3;
    nativeFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
  }
  (void)effectFlags;
  return D3DCompile(
    source,
    sourceLength,
    sourceName,
    static_cast<const D3D_SHADER_MACRO *>(macros),
    static_cast<ID3DInclude *>(includeHandler),
    entryPoint,
    target,
    nativeFlags,
    0,
    reinterpret_cast<ID3DBlob **>(compiledShader),
    reinterpret_cast<ID3DBlob **>(errors));
}
