#include "../wpch.hpp"
#include "Dx11Texture.hpp"
#include "../pactext.hpp"
#include <limits.h>
#include <vector>

Dx11Texture::Dx11Texture()
  : _texture(NULL), _resourceView(NULL), _srgbResourceView(NULL),
    _sampler(NULL), _mipLevels(0)
{
}

Dx11Texture::~Dx11Texture()
{
  Reset();
}

void Dx11Texture::Reset()
{
  if (_sampler) { _sampler->Release(); _sampler = NULL; }
  if (_srgbResourceView) { _srgbResourceView->Release(); _srgbResourceView = NULL; }
  if (_resourceView) { _resourceView->Release(); _resourceView = NULL; }
  if (_texture) { _texture->Release(); _texture = NULL; }
  _mipLevels = 0;
}

HRESULT Dx11Texture::Initialize(ID3D11Device *device, UINT width, UINT height,
  DXGI_FORMAT format, UINT mipLevels, const D3D11_SUBRESOURCE_DATA *mipData,
  const D3D11_SAMPLER_DESC *samplerDesc, DXGI_FORMAT resourceFormat,
  DXGI_FORMAT srgbViewFormat)
{
  if (!device || !width || !height || format == DXGI_FORMAT_UNKNOWN ||
      !mipLevels || !mipData)
    return E_INVALIDARG;

  D3D11_TEXTURE2D_DESC textureDesc = {};
  textureDesc.Width = width;
  textureDesc.Height = height;
  textureDesc.MipLevels = mipLevels;
  textureDesc.ArraySize = 1;
  textureDesc.Format = resourceFormat == DXGI_FORMAT_UNKNOWN
    ? format : resourceFormat;
  textureDesc.SampleDesc.Count = 1;
  textureDesc.Usage = D3D11_USAGE_DEFAULT;
  textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

  ID3D11Texture2D *texture = NULL;
  HRESULT hr = device->CreateTexture2D(&textureDesc, mipData, &texture);
  if (FAILED(hr))
    RptF("DX11 texture CreateTexture2D failed: %ux%u format=%u mips=%u pitch=%u hr=0x%08x",
      width, height, (unsigned)format, mipLevels,
      mipData ? mipData[0].SysMemPitch : 0, (unsigned)hr);
  if (FAILED(hr)) return hr;

  D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
  viewDesc.Format = format;
  viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  viewDesc.Texture2D.MostDetailedMip = 0;
  viewDesc.Texture2D.MipLevels = mipLevels;

  ID3D11ShaderResourceView *resourceView = NULL;
  hr = device->CreateShaderResourceView(texture, &viewDesc, &resourceView);
  if (FAILED(hr))
  {
    RptF("DX11 texture CreateShaderResourceView failed: format=%u mips=%u hr=0x%08x",
      (unsigned)format, mipLevels, (unsigned)hr);
    texture->Release();
    return hr;
  }

  ID3D11ShaderResourceView *srgbResourceView = NULL;
  if (srgbViewFormat != DXGI_FORMAT_UNKNOWN)
  {
    viewDesc.Format = srgbViewFormat;
    hr = device->CreateShaderResourceView(texture, &viewDesc,
      &srgbResourceView);
    if (FAILED(hr))
    {
      RptF("DX11 texture optional sRGB view unavailable: format=%u hr=0x%08x",
        (unsigned)srgbViewFormat, (unsigned)hr);
      hr = S_OK;
    }
  }

  D3D11_SAMPLER_DESC defaultSampler = {};
  if (samplerDesc)
    defaultSampler = *samplerDesc;
  else
  {
    defaultSampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    defaultSampler.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    defaultSampler.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    defaultSampler.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    defaultSampler.ComparisonFunc = D3D11_COMPARISON_NEVER;
    defaultSampler.MaxAnisotropy = 1;
    defaultSampler.MinLOD = 0.0f;
    defaultSampler.MaxLOD = 3.402823466e+38f;
  }

  ID3D11SamplerState *sampler = NULL;
  hr = device->CreateSamplerState(&defaultSampler, &sampler);
  if (FAILED(hr))
  {
    if (srgbResourceView) srgbResourceView->Release();
    resourceView->Release();
    texture->Release();
    return hr;
  }

  Reset();
  _texture = texture;
  _resourceView = resourceView;
  _srgbResourceView = srgbResourceView;
  _sampler = sampler;
  _mipLevels = mipLevels;
  return S_OK;
}

HRESULT Dx11Texture::UpdateMip(ID3D11DeviceContext *context, UINT mipLevel,
  const void *data, UINT rowPitch, UINT depthPitch)
{
  if (!context || !_texture || !data || !rowPitch || mipLevel >= _mipLevels)
    return E_INVALIDARG;

  context->UpdateSubresource(_texture, mipLevel, NULL, data, rowPitch, depthPitch);
  return S_OK;
}

HRESULT Dx11Texture::InitializeFromSource(ID3D11Device *device,
  const char *sourceName)
{
  if (!device || !sourceName) return E_INVALIDARG;
  const int maxMipLevels = 12; // shared PAC/PAA format limit
  PacLevelMem mipLevels[maxMipLevels];
  ITextureSourceFactory *factory = SelectTextureSourceFactory(sourceName);
  if (!factory) return E_FAIL;
  SRef<ITextureSource> source = factory->Create(sourceName, mipLevels, maxMipLevels);
  if (!source) return E_FAIL;
  return InitializeFromSource(device, source, mipLevels, maxMipLevels);
}

HRESULT Dx11Texture::InitializeFromSource(ID3D11Device *device,
  ITextureSource *source, PacLevelMem *mipLevels, int maxMipLevels)
{
  if (!device || !source || !mipLevels || maxMipLevels <= 0)
  {
    RptF("DX11 texture init invalid arguments: device=%p source=%p mips=%p max=%d",
      device, source, mipLevels, maxMipLevels);
    return E_INVALIDARG;
  }
  const int sourceMipCount = source->GetMipmapCount();
  if (sourceMipCount <= 0)
  {
    RptF("DX11 texture init has no source mips: source=%p mip-array=%p max=%d format=%d count=%d",
      source, mipLevels, maxMipLevels, (int)source->GetFormat(), sourceMipCount);
    return E_INVALIDARG;
  }

  PacFormat uploadFormat = source->GetFormat();
  // The legacy PAC palette path only decodes to these 16-bit formats.
  if (uploadFormat == PacP8) uploadFormat = PacARGB1555;
  // Expand PAA 4444 in the shared PAC decoder for portable DXGI upload.
  if (uploadFormat == PacARGB4444) uploadFormat = PacARGB8888;
  // D3D9's A8L8 sampler expands AI88 as luminance in RGB and alpha in A.
  // DXGI_FORMAT_R8G8_UNORM exposes those bytes as (L, A, 0, 1), which the
  // ordinary RGBA material shader cannot consume directly. Expand through
  // the PAC decoder so the D3D11 shader receives the same RGBA values.
  if (uploadFormat == PacAI88) uploadFormat = PacARGB8888;
  DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;
  if (!MapPacFormat(uploadFormat, dxgiFormat))
  {
    RptF("DX11 texture format unsupported: pac=%d", (int)uploadFormat);
    return E_NOTIMPL;
  }

  int mipCount = 0;
  for (; mipCount < sourceMipCount && mipCount < maxMipLevels; ++mipCount)
  {
    PacLevelMem &mip = mipLevels[mipCount];
    if (mip._w <= 0 || mip._h <= 0)
    {
      RptF("DX11 texture invalid mip dimensions: level=%d dims=%dx%d format=%d",
        mipCount, mip._w, mip._h, (int)uploadFormat);
      break;
    }
    if (PacFormatIsDXT(uploadFormat) && (mip._w < 4 || mip._h < 4)) break;
    mip.SetDestFormat(uploadFormat, 8);
  }
  if (!mipCount)
  {
    RptF("DX11 texture has no uploadable mips: format=%d sourceCount=%d max=%d",
      (int)uploadFormat, sourceMipCount, maxMipLevels);
    return E_INVALIDARG;
  }

  const UINT width = (UINT)mipLevels[0]._w;
  const UINT height = (UINT)mipLevels[0]._h;
  std::vector<std::vector<unsigned char> > storage(mipCount);
  std::vector<D3D11_SUBRESOURCE_DATA> subresources(mipCount);
  for (int i = 0; i < mipCount; ++i)
  {
    PacLevelMem &mip = mipLevels[i];
    UINT rowPitch = 0;
    if (!ComputePacRowPitch(uploadFormat, (UINT)mip._w, rowPitch))
    {
      RptF("DX11 texture row pitch failed: level=%d format=%d width=%d",
        i, (int)uploadFormat, mip._w);
      return E_NOTIMPL;
    }
    const bool compressed = PacFormatIsDXT(uploadFormat);
    const UINT rowCount = compressed ? ((UINT)mip._h + 3) / 4 : (UINT)mip._h;
    if (!rowCount || rowPitch > UINT_MAX / rowCount) return E_INVALIDARG;
    const UINT dataSize = rowPitch * rowCount;
    const UINT pacPitch = compressed ? rowPitch / 4 : rowPitch;
    if (!pacPitch || pacPitch > SHRT_MAX) return E_INVALIDARG;
    mip._pitch = (short)pacPitch;
    storage[i].resize(dataSize);
    const int textureSize = i == 0 ? intMax(mip._w, mip._h) : 0;
    if (!source->GetMipmapData(&storage[i][0], mip, i, textureSize))
    {
      RptF("DX11 texture mip read failed: level=%d dims=%dx%d format=%d",
        i, mip._w, mip._h, (int)uploadFormat);
      return E_FAIL;
    }
    subresources[i].pSysMem = &storage[i][0];
    subresources[i].SysMemPitch = rowPitch;
    subresources[i].SysMemSlicePitch = dataSize;
  }

  D3D11_SAMPLER_DESC sampler = {};
  sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  const int clamp = source->GetClamp();
  sampler.AddressU = (clamp & TexClampU) ? D3D11_TEXTURE_ADDRESS_CLAMP : D3D11_TEXTURE_ADDRESS_WRAP;
  sampler.AddressV = (clamp & TexClampV) ? D3D11_TEXTURE_ADDRESS_CLAMP : D3D11_TEXTURE_ADDRESS_WRAP;
  sampler.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
  sampler.ComparisonFunc = D3D11_COMPARISON_NEVER;
  sampler.MinLOD = 0.0f;
  sampler.MaxLOD = 3.402823466e+38f;
  sampler.MaxAnisotropy = 1;

  DXGI_FORMAT resourceFormat = DXGI_FORMAT_UNKNOWN;
  DXGI_FORMAT srgbViewFormat = DXGI_FORMAT_UNKNOWN;
  const TextureType textureType = source->GetTextureType();
  if (textureType == TT_Diffuse || textureType == TT_Macro)
  {
    switch (dxgiFormat)
    {
    case DXGI_FORMAT_B8G8R8A8_UNORM:
      resourceFormat = DXGI_FORMAT_B8G8R8A8_TYPELESS;
      srgbViewFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
      break;
    case DXGI_FORMAT_BC1_UNORM:
      resourceFormat = DXGI_FORMAT_BC1_TYPELESS;
      srgbViewFormat = DXGI_FORMAT_BC1_UNORM_SRGB;
      break;
    case DXGI_FORMAT_BC2_UNORM:
      resourceFormat = DXGI_FORMAT_BC2_TYPELESS;
      srgbViewFormat = DXGI_FORMAT_BC2_UNORM_SRGB;
      break;
    case DXGI_FORMAT_BC3_UNORM:
      resourceFormat = DXGI_FORMAT_BC3_TYPELESS;
      srgbViewFormat = DXGI_FORMAT_BC3_UNORM_SRGB;
      break;
    default:
      break;
    }
  }

  return Initialize(device, width, height, dxgiFormat, (UINT)mipCount,
    &subresources[0], &sampler, resourceFormat, srgbViewFormat);
}

bool Dx11Texture::MapPacFormat(PacFormat sourceFormat, DXGI_FORMAT &format)
{
  switch (sourceFormat)
  {
  case PacAI88:     format = DXGI_FORMAT_R8G8_UNORM; return true;
  case PacRGB565:   format = DXGI_FORMAT_B5G6R5_UNORM; return true;
  case PacARGB1555: format = DXGI_FORMAT_B5G5R5A1_UNORM; return true;
  case PacARGB4444: format = DXGI_FORMAT_B4G4R4A4_UNORM; return true;
  case PacARGB8888: format = DXGI_FORMAT_B8G8R8A8_UNORM; return true;
  case PacDXT1:     format = DXGI_FORMAT_BC1_UNORM; return true;
  case PacDXT2:
  case PacDXT3:     format = DXGI_FORMAT_BC2_UNORM; return true;
  case PacDXT4:
  case PacDXT5:     format = DXGI_FORMAT_BC3_UNORM; return true;
  default:
    format = DXGI_FORMAT_UNKNOWN;
    return false; // P8 requires palette expansion before upload.
  }
}

bool Dx11Texture::ComputePacRowPitch(PacFormat sourceFormat, UINT width,
  UINT &rowPitch)
{
  if (!width) return false;
  UINT bytesPerPixel = 0;
  switch (sourceFormat)
  {
  case PacAI88:
  case PacRGB565:
  case PacARGB1555:
  case PacARGB4444:
    bytesPerPixel = 2;
    break;
  case PacARGB8888:
    bytesPerPixel = 4;
    break;
  case PacP8:
    bytesPerPixel = 1;
    break;
  case PacDXT1:
  {
    const UINT blocks = width < 4 ? 1 : width / 4 + (width % 4 != 0);
    if (blocks > UINT_MAX / 8) return false;
    rowPitch = blocks * 8;
    return true;
  }
  case PacDXT2:
  case PacDXT3:
  case PacDXT4:
  case PacDXT5:
  {
    const UINT blocks = width < 4 ? 1 : width / 4 + (width % 4 != 0);
    if (blocks > UINT_MAX / 16) return false;
    rowPitch = blocks * 16;
    return true;
  }
  default:
    return false;
  }
  if (width > UINT_MAX / bytesPerPixel) return false;
  rowPitch = width * bytesPerPixel;
  return true;
}

void Dx11Texture::Bind(ID3D11DeviceContext *context, UINT slot, bool srgb) const
{
  ID3D11ShaderResourceView *view = ResourceView(srgb);
  if (!context || !view || !_sampler) return;
  context->PSSetShaderResources(slot, 1, &view);
  context->PSSetSamplers(slot, 1, &_sampler);
}
