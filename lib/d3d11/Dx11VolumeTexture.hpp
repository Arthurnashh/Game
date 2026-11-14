#pragma once
#ifndef DAYZ_D3D11_VOLUME_TEXTURE_HPP
#define DAYZ_D3D11_VOLUME_TEXTURE_HPP

#include <Es/Common/win.h>
#include <d3d11.h>

// D3D11 volume resource used by the weather simulation's 3D lookup fields.
class Dx11VolumeTexture
{
public:
  Dx11VolumeTexture();
  ~Dx11VolumeTexture();
  Dx11VolumeTexture(const Dx11VolumeTexture &) = delete;
  Dx11VolumeTexture &operator=(const Dx11VolumeTexture &) = delete;

  HRESULT Initialize(ID3D11Device *device, UINT width, UINT height, UINT depth,
    DXGI_FORMAT format, const void *initialData = NULL,
    UINT rowPitch = 0, UINT slicePitch = 0);
  HRESULT Update(ID3D11DeviceContext *context, const void *data,
    UINT rowPitch, UINT slicePitch);
  HRESULT UpdateRegion(ID3D11DeviceContext *context, const D3D11_BOX &region,
    const void *data, UINT rowPitch, UINT slicePitch);
  void Bind(ID3D11DeviceContext *context, UINT slot) const;
  void Reset();

  ID3D11Texture3D *Texture() const { return _texture; }
  ID3D11ShaderResourceView *ResourceView() const { return _resourceView; }
  UINT Width() const { return _width; }
  UINT Height() const { return _height; }
  UINT Depth() const { return _depth; }

private:
  ID3D11Texture3D *_texture;
  ID3D11ShaderResourceView *_resourceView;
  UINT _width, _height, _depth;
};

#endif
