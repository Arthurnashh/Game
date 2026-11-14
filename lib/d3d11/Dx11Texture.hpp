#pragma once
#ifndef DAYZ_D3D11_TEXTURE_HPP
#define DAYZ_D3D11_TEXTURE_HPP

#include <Es/Common/win.h>
#include <d3d11.h>

enum PacFormat : int;
class ITextureSource;
class PacLevelMem;

// 2D shader texture with a view and sampler; mip levels can be streamed later.
class Dx11Texture
{
public:
  Dx11Texture();
  ~Dx11Texture();
  Dx11Texture(const Dx11Texture &) = delete;
  Dx11Texture &operator=(const Dx11Texture &) = delete;

  HRESULT Initialize(ID3D11Device *device, UINT width, UINT height,
    DXGI_FORMAT format, UINT mipLevels, const D3D11_SUBRESOURCE_DATA *mipData,
    const D3D11_SAMPLER_DESC *samplerDesc = NULL,
    DXGI_FORMAT resourceFormat = DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT srgbViewFormat = DXGI_FORMAT_UNKNOWN);
  HRESULT InitializeFromSource(ID3D11Device *device, const char *sourceName);
  HRESULT InitializeFromSource(ID3D11Device *device, ITextureSource *source,
    PacLevelMem *mipLevels, int maxMipLevels);
  HRESULT UpdateMip(ID3D11DeviceContext *context, UINT mipLevel,
    const void *data, UINT rowPitch, UINT depthPitch = 0);
  static bool MapPacFormat(PacFormat sourceFormat, DXGI_FORMAT &format);
  static bool ComputePacRowPitch(PacFormat sourceFormat, UINT width, UINT &rowPitch);
  void Bind(ID3D11DeviceContext *context, UINT slot, bool srgb = false) const;
  void Reset();

  ID3D11ShaderResourceView *ResourceView(bool srgb = false) const
  {
    return srgb && _srgbResourceView ? _srgbResourceView : _resourceView;
  }
  ID3D11SamplerState *Sampler() const { return _sampler; }

private:
  ID3D11Texture2D *_texture;
  ID3D11ShaderResourceView *_resourceView;
  ID3D11ShaderResourceView *_srgbResourceView;
  ID3D11SamplerState *_sampler;
  UINT _mipLevels;
};

#endif
