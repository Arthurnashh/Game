#include "Dx11VolumeTexture.hpp"

Dx11VolumeTexture::Dx11VolumeTexture()
  : _texture(NULL), _resourceView(NULL), _width(0), _height(0), _depth(0)
{
}

Dx11VolumeTexture::~Dx11VolumeTexture()
{
  Reset();
}

void Dx11VolumeTexture::Reset()
{
  if (_resourceView) { _resourceView->Release(); _resourceView = NULL; }
  if (_texture) { _texture->Release(); _texture = NULL; }
  _width = _height = _depth = 0;
}

HRESULT Dx11VolumeTexture::Initialize(ID3D11Device *device, UINT width,
  UINT height, UINT depth, DXGI_FORMAT format, const void *initialData,
  UINT rowPitch, UINT slicePitch)
{
  if (!device || !width || !height || !depth ||
      format == DXGI_FORMAT_UNKNOWN ||
      (initialData && (!rowPitch || !slicePitch)) ||
      (!initialData && (rowPitch || slicePitch)))
    return E_INVALIDARG;

  D3D11_TEXTURE3D_DESC desc = {};
  desc.Width = width;
  desc.Height = height;
  desc.Depth = depth;
  desc.MipLevels = 1;
  desc.Format = format;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  D3D11_SUBRESOURCE_DATA data = {};
  data.pSysMem = initialData;
  data.SysMemPitch = rowPitch;
  data.SysMemSlicePitch = slicePitch;

  ID3D11Texture3D *texture = NULL;
  HRESULT hr = device->CreateTexture3D(&desc, initialData ? &data : NULL,
    &texture);
  if (FAILED(hr)) return hr;

  D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
  viewDesc.Format = format;
  viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
  viewDesc.Texture3D.MostDetailedMip = 0;
  viewDesc.Texture3D.MipLevels = 1;
  ID3D11ShaderResourceView *view = NULL;
  hr = device->CreateShaderResourceView(texture, &viewDesc, &view);
  if (FAILED(hr))
  {
    texture->Release();
    return hr;
  }

  Reset();
  _texture = texture;
  _resourceView = view;
  _width = width;
  _height = height;
  _depth = depth;
  return S_OK;
}

HRESULT Dx11VolumeTexture::Update(ID3D11DeviceContext *context,
  const void *data, UINT rowPitch, UINT slicePitch)
{
  if (!_width || !_height || !_depth) return E_FAIL;
  D3D11_BOX full = {0, 0, 0, _width, _height, _depth};
  return UpdateRegion(context, full, data, rowPitch, slicePitch);
}

HRESULT Dx11VolumeTexture::UpdateRegion(ID3D11DeviceContext *context,
  const D3D11_BOX &region, const void *data, UINT rowPitch, UINT slicePitch)
{
  if (!context || !_texture || !data || !rowPitch || !slicePitch ||
      region.left >= region.right || region.top >= region.bottom ||
      region.front >= region.back || region.right > _width ||
      region.bottom > _height || region.back > _depth)
    return E_INVALIDARG;
  context->UpdateSubresource(_texture, 0, &region, data, rowPitch, slicePitch);
  return S_OK;
}

void Dx11VolumeTexture::Bind(ID3D11DeviceContext *context, UINT slot) const
{
  if (context) context->PSSetShaderResources(slot, 1, &_resourceView);
}
