#include "Dx11ShaderCompiler.hpp"
#include "Dx11ShaderCache.hpp"

HRESULT Dx11ShaderCompiler::CompileVertexShader(ID3D11Device *device,
  const char *source, SIZE_T sourceLength, const char *entryPoint,
  ID3D11VertexShader **shader, ID3DBlob **bytecode)
{
  return CompileVertexShader(device, source, sourceLength, entryPoint, shader,
    bytecode, NULL);
}

HRESULT Dx11ShaderCompiler::CompileVertexShader(ID3D11Device *device,
  const char *source, SIZE_T sourceLength, const char *entryPoint,
  ID3D11VertexShader **shader, ID3DBlob **bytecode,
  ID3DBlob **diagnostics)
{
  return CompileVertexShader(device, source, sourceLength, entryPoint, shader,
    bytecode, diagnostics, NULL, NULL, NULL);
}

HRESULT Dx11ShaderCompiler::CompilePixelShader(ID3D11Device *device,
  const char *source, SIZE_T sourceLength, const char *entryPoint,
  ID3D11PixelShader **shader, ID3DBlob **bytecode)
{
  return CompilePixelShader(device, source, sourceLength, entryPoint, shader,
    bytecode, NULL);
}

HRESULT Dx11ShaderCompiler::CompilePixelShader(ID3D11Device *device,
  const char *source, SIZE_T sourceLength, const char *entryPoint,
  ID3D11PixelShader **shader, ID3DBlob **bytecode,
  ID3DBlob **diagnostics)
{
  return CompilePixelShader(device, source, sourceLength, entryPoint, shader,
    bytecode, diagnostics, NULL, NULL, NULL);
}

HRESULT Dx11ShaderCompiler::Compile(ID3D11Device *device, const char *source,
  SIZE_T sourceLength, const char *entryPoint, const char *profile,
  ID3DBlob **bytecode, ID3DBlob **diagnostics,
  const D3D_SHADER_MACRO *defines, ID3DInclude *include,
  const char *sourceName)
{
  if (bytecode) *bytecode = NULL;
  if (diagnostics) *diagnostics = NULL;
  if (!device || !source || !entryPoint || !bytecode) return E_INVALIDARG;

  return Dx11ShaderCache::Compile(source, sourceLength, entryPoint, profile,
    D3D10_SHADER_ENABLE_STRICTNESS | D3D10_SHADER_OPTIMIZATION_LEVEL3,
    bytecode, diagnostics, defines, include, sourceName);
}

HRESULT Dx11ShaderCompiler::CompileVertexShader(ID3D11Device *device,
  const char *source, SIZE_T sourceLength, const char *entryPoint,
  ID3D11VertexShader **shader, ID3DBlob **bytecode, ID3DBlob **diagnostics,
  const D3D_SHADER_MACRO *defines, ID3DInclude *include,
  const char *sourceName)
{
  if (shader) *shader = NULL;
  if (bytecode) *bytecode = NULL;
  if (!shader || !bytecode) return E_INVALIDARG;

  ID3DBlob *compiled = NULL;
  HRESULT hr = Compile(device, source, sourceLength, entryPoint, "vs_4_0",
    &compiled, diagnostics, defines, include, sourceName);
  if (FAILED(hr)) return hr;

  hr = device->CreateVertexShader(compiled->GetBufferPointer(),
    compiled->GetBufferSize(), NULL, shader);
  if (SUCCEEDED(hr))
    *bytecode = compiled;
  else
    compiled->Release();
  return hr;
}

HRESULT Dx11ShaderCompiler::CompilePixelShader(ID3D11Device *device,
  const char *source, SIZE_T sourceLength, const char *entryPoint,
  ID3D11PixelShader **shader, ID3DBlob **bytecode, ID3DBlob **diagnostics,
  const D3D_SHADER_MACRO *defines, ID3DInclude *include,
  const char *sourceName)
{
  if (shader) *shader = NULL;
  if (bytecode) *bytecode = NULL;
  if (!shader || !bytecode) return E_INVALIDARG;

  ID3DBlob *compiled = NULL;
  HRESULT hr = Compile(device, source, sourceLength, entryPoint, "ps_4_0",
    &compiled, diagnostics, defines, include, sourceName);
  if (FAILED(hr)) return hr;

  hr = device->CreatePixelShader(compiled->GetBufferPointer(),
    compiled->GetBufferSize(), NULL, shader);
  if (SUCCEEDED(hr))
    *bytecode = compiled;
  else
    compiled->Release();
  return hr;
}
