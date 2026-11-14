#pragma once

#include <windows.h>

#define DX11_COMPAT_COMPILE_DEBUG 0x1
#define DX11_COMPAT_COMPILE_SKIP_OPTIMIZATION 0x2

// Opaque bridge used by Enfusion's legacy shader loader.  The implementation
// lives in a DX11-only translation unit so the old DirectX SDK headers and
// Windows SDK D3DCompiler headers never share one translation unit.
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
  void **errors);
