#pragma once
#ifndef DAYZ_D3D11_SHADER_CACHE_HPP
#define DAYZ_D3D11_SHADER_CACHE_HPP

#include <d3dcompiler.h>

// Persistent bytecode cache for the D3D11 runtime compiler. Cache entries are
// validated against the exact source, entry point, profile, flags, and compiler
// version; a corrupt or stale entry is treated as a cache miss.
class Dx11ShaderCache
{
public:
  static HRESULT Compile(const char *source, SIZE_T sourceLength,
    const char *entryPoint, const char *profile, UINT flags,
    ID3DBlob **bytecode, ID3DBlob **diagnostics = NULL,
    const D3D_SHADER_MACRO *defines = NULL, ID3DInclude *include = NULL,
    const char *sourceName = NULL);
};

#endif
