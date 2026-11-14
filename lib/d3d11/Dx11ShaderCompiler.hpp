#pragma once
#ifndef DAYZ_D3D11_SHADER_COMPILER_HPP
#define DAYZ_D3D11_SHADER_COMPILER_HPP

#include <d3d11.h>
#include <d3dcompiler.h>

// Compiles one HLSL entry point and creates its native D3D11 shader.
// The caller owns the returned bytecode and diagnostics blobs and must Release them.
class Dx11ShaderCompiler
{
public:
  static HRESULT CompileVertexShader(ID3D11Device *device, const char *source,
    SIZE_T sourceLength, const char *entryPoint, ID3D11VertexShader **shader,
    ID3DBlob **bytecode);

  // Keep the basic call independent of legacy Direct3D 10 declarations that
  // may already be present in the engine precompiled header.
  static HRESULT CompileVertexShader(ID3D11Device *device, const char *source,
    SIZE_T sourceLength, const char *entryPoint, ID3D11VertexShader **shader,
    ID3DBlob **bytecode, ID3DBlob **diagnostics);

  static HRESULT CompileVertexShader(ID3D11Device *device, const char *source,
    SIZE_T sourceLength, const char *entryPoint, ID3D11VertexShader **shader,
    ID3DBlob **bytecode, ID3DBlob **diagnostics,
    const D3D_SHADER_MACRO *defines, ID3DInclude *include,
    const char *sourceName);

  static HRESULT CompilePixelShader(ID3D11Device *device, const char *source,
    SIZE_T sourceLength, const char *entryPoint, ID3D11PixelShader **shader,
    ID3DBlob **bytecode);

  static HRESULT CompilePixelShader(ID3D11Device *device, const char *source,
    SIZE_T sourceLength, const char *entryPoint, ID3D11PixelShader **shader,
    ID3DBlob **bytecode, ID3DBlob **diagnostics);

  static HRESULT CompilePixelShader(ID3D11Device *device, const char *source,
    SIZE_T sourceLength, const char *entryPoint, ID3D11PixelShader **shader,
    ID3DBlob **bytecode, ID3DBlob **diagnostics,
    const D3D_SHADER_MACRO *defines, ID3DInclude *include,
    const char *sourceName);

private:
  static HRESULT Compile(ID3D11Device *device, const char *source,
    SIZE_T sourceLength, const char *entryPoint, const char *profile,
    ID3DBlob **bytecode, ID3DBlob **diagnostics,
    const D3D_SHADER_MACRO *defines, ID3DInclude *include,
    const char *sourceName);
};

#endif
