#include "../wpch.hpp"

#if _ARMA3_SIMUL_WEATHER

#include "SimulCommonDX11.hpp"

TextureSysGPUDX11::TextureSysGPUDX11(Dx11DeviceAccess *engine,
  int bytesPerTexel)
  : _engine(engine), _bytesPerTexel(bytesPerTexel), _width(0), _height(0),
    _depth(0), _dirty(false)
{
}

TextureSysGPUDX11::~TextureSysGPUDX11()
{
  _gpuTexture.Reset();
  _staging.clear();
}

HRESULT TextureSysGPUDX11::CreateTextures(int width, int height, int depth,
  DXGI_FORMAT format)
{
  if (!_engine || !_engine->D3D11Device() || width <= 0 || height <= 0 ||
      depth <= 0 || _bytesPerTexel <= 0)
    return E_INVALIDARG;

  const size_t w = (size_t)width;
  const size_t h = (size_t)height;
  const size_t d = (size_t)depth;
  const size_t bpp = (size_t)_bytesPerTexel;
  if (w > SIZE_MAX / h || w * h > SIZE_MAX / d ||
      w * h * d > SIZE_MAX / bpp)
    return E_INVALIDARG;

  std::vector<unsigned char> staging(w * h * d * bpp, 0);
  HRESULT hr = _gpuTexture.Initialize(_engine->D3D11Device(), (UINT)width,
    (UINT)height, (UINT)depth, format, &staging[0],
    (UINT)(w * bpp), (UINT)(w * h * bpp));
  if (FAILED(hr)) return hr;

  _staging.swap(staging);
  _width = width;
  _height = height;
  _depth = depth;
  _dirty = false;
  return S_OK;
}

TextureSysGPUDX11::LockedData::LockedData(Ref<TextureSysGPUDX11> texture)
  : _texture(texture), _data(NULL)
{
  if (!_texture) return;
  _texture->EnterCriticalSection();
  if (!_texture->_staging.empty())
  {
    _texture->_dirty = true;
    _data = &_texture->_staging[0];
  }
}

TextureSysGPUDX11::LockedData::~LockedData()
{
  if (_texture) _texture->LeaveCriticalSection();
}

void TextureSysGPUDX11::LeaveCriticalSection()
{
  _criticalSection.Unlock();
}

bool TextureSysGPUDX11::LockedData::CheckSize() const
{
  return _texture && !_texture->_staging.empty();
}

bool TextureSysGPUDX11::LockedData::IsPitchCorrespondingSize(
  int texelSizeInBytes) const
{
  return _texture && texelSizeInBytes == _texture->_bytesPerTexel;
}

int TextureSysGPUDX11::LockedData::GetMappedTextureOffset(int texelIndex,
  int texelSizeInBytes) const
{
  if (!_texture || texelIndex < 0 ||
      texelSizeInBytes != _texture->_bytesPerTexel)
    return 0;
  const size_t offset = (size_t)texelIndex * (size_t)texelSizeInBytes;
  if (offset >= _texture->_staging.size()) return 0;
  return (int)offset;
}

void TextureSysGPUDX11::UploadIfDirty()
{
  if (!_dirty || !_engine || !_engine->D3D11Context() || _staging.empty())
    return;
  const UINT rowPitch = (UINT)((size_t)_width * _bytesPerTexel);
  const UINT slicePitch = rowPitch * (UINT)_height;
  if (SUCCEEDED(_gpuTexture.Update(_engine->D3D11Context(), &_staging[0],
      rowPitch, slicePitch)))
    _dirty = false;
}

ID3D11ShaderResourceView *TextureSysGPUDX11::GetResourceView()
{
  _criticalSection.Lock();
  UploadIfDirty();
  ID3D11ShaderResourceView *view = _gpuTexture.ResourceView();
  _criticalSection.Unlock();
  return view;
}

Dx11VolumeTexture *TextureSysGPUDX11::GetGPUTex()
{
  _criticalSection.Lock();
  UploadIfDirty();
  _criticalSection.Unlock();
  return &_gpuTexture;
}

#endif
