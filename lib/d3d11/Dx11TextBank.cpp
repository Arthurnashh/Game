#include "../wpch.hpp"
#include "Dx11TextBank.hpp"
#include "Dx11Texture.hpp"
#include "../engine.hpp"
#include "../pactext.hpp"
#include <math.h>

namespace
{
const int Dx11MaxMipmaps = 12;

float Dx11SRGBToLinear(float value)
{
  if (value <= 0.04045f) return value / 12.92f;
  return (float)pow((value + 0.055f) / 1.055f, 2.4);
}

PacFormat Dx11DestinationFormat(PacFormat format)
{
  switch (format)
  {
  case PacARGB1555:
  case PacRGB565:
  case PacARGB4444:
  case PacAI88:
  case PacARGB8888:
  case PacDXT1:
  case PacDXT2:
  case PacDXT3:
  case PacDXT4:
  case PacDXT5:
    return format;
  case PacP8:
    // Expand the palette into a non-paletted format supported by DX11.
    return PacARGB1555;
  default:
    return format;
  }
}
}

class Dx11TextureAsset : public Texture
{
  friend class Dx11TextBank;
  typedef Texture base;

  mutable SRef<ITextureSource> _source;
  mutable bool _headersReady;
  mutable bool _failed;
  mutable CriticalSection _headersLock;
  mutable int _width;
  mutable int _height;
  mutable int _mipCount;
  mutable PacLevelMem _mips[Dx11MaxMipmaps];

  bool EnsureHeaders() const
  {
    ScopeLockSection lock(_headersLock);
    if (_headersReady) return true;
    if (_failed) return false;
    if (!_source)
    {
      ITextureSourceFactory *factory = SelectTextureSourceFactory(Name());
      if (factory)
      {
        LogF("DX11 texture header: %s asset=%p mip-buffer=%p", (const char *)Name(), this, _mips);
        SRef<ITextureSource> created = factory->Create(Name(), _mips, Dx11MaxMipmaps);
        _source = created;
      }
    }
    if (!_source)
    {
      _failed = true;
      return false;
    }

    const PacFormat format = _source->GetFormat();
    if (format == PacARGB4444 || format == PacAI88 || format == PacARGB8888)
      _source->ForceAlpha();
    const PacFormat dstFormat = Dx11DestinationFormat(format);
    const int sourceMipCount = _source->GetMipmapCount();
    if (sourceMipCount <= 0)
      RptF("DX11 texture source invalid after init: name=%s source=%p format=%d mipCount=%d m0=%dx%d",
        (const char *)Name(), _source.GetRef(), (int)format, sourceMipCount,
        _mips[0]._w, _mips[0]._h);
    _mipCount = 0;
    for (int i = 0; i < sourceMipCount && i < Dx11MaxMipmaps; ++i)
    {
      _mips[i].SetDestFormat(dstFormat, 8);
      // Procedural color textures are intentionally 1x1. They are used as
      // valid constant material stages (for example neutral normal/detail
      // maps), so do not discard the only mip merely because it is 1x1.
      if (_mips[i]._w <= 0 || _mips[i]._h <= 0) break;
      ++_mipCount;
    }
    if (_mipCount == 0)
    {
      _failed = true;
      return false;
    }
    _width = _mips[0]._w;
    _height = _mips[0]._h;
    _headersReady = true;
    return true;
  }

public:
  Dx11TextureAsset()
    : _headersReady(false), _failed(false), _width(0), _height(0), _mipCount(0)
  {
  }

  virtual void OnUnused() const
  {
    if (GetName().GetLength() > 0 && GEngine && GEngine->TextBank())
      static_cast<Dx11TextBank *>(GEngine->TextBank())->Remove(
        const_cast<Dx11TextureAsset *>(this));
    base::OnUnused();
  }

  virtual int AArea(int level = 0) const
  {
    return EnsureHeaders() ? AWidth(level) * AHeight(level) : 0;
  }
  virtual int AWidth(int level = 0) const
  {
    if (!EnsureHeaders()) return 0;
    if (level < 0 || level >= _mipCount) return 0;
    return _mips[level]._w;
  }
  virtual int AHeight(int level = 0) const
  {
    if (!EnsureHeaders()) return 0;
    if (level < 0 || level >= _mipCount) return 0;
    return _mips[level]._h;
  }
  virtual int ANMipmaps() const { return EnsureHeaders() ? _mipCount : 0; }
  virtual int BestMipmap() const { return 0; }
  virtual AbstractMipmapLevel &AMipmap(int level)
  {
    DoAssert(EnsureHeaders() && level >= 0 && level < _mipCount);
    return _mips[level >= 0 && level < Dx11MaxMipmaps ? level : 0];
  }
  virtual const AbstractMipmapLevel &AMipmap(int level) const
  {
    DoAssert(EnsureHeaders() && level >= 0 && level < _mipCount);
    return _mips[level >= 0 && level < Dx11MaxMipmaps ? level : 0];
  }
  virtual void ASetNMipmaps(int count)
  {
    if (count >= 0 && count <= Dx11MaxMipmaps) _mipCount = count;
  }
  virtual Color GetPixel(int, float, float) const { return HBlack; }
  virtual Color GetColor() const
  {
    return EnsureHeaders() && _source
      ? Color(safe_cast<Color>(_source->GetAverageColor())) *
        Color(safe_cast<Color>(_source->GetMaxColor()))
      : HBlack;
  }
  Color GetMaxColor() const
  {
    return EnsureHeaders() && _source
      ? Color(safe_cast<Color>(_source->GetMaxColor())) : HWhite;
  }
  virtual PacFormat GetFormat() const
  {
    return EnsureHeaders() && _source ? _source->GetFormat() : PacUnknown;
  }
  virtual bool IsTransparent() const
  {
    return EnsureHeaders() && _source && _source->IsTransparent();
  }
  virtual bool IsAlpha() const
  {
    return EnsureHeaders() && _source && _source->IsAlpha();
  }
  virtual bool IsAlphaNonOpaque() const
  {
    return EnsureHeaders() && _source && _source->IsAlphaNonOpaque();
  }
  virtual int GetClamp() const
  {
    return EnsureHeaders() && _source ? _source->GetClamp() : 0;
  }
  virtual void SetClamp(int clampFlags)
  {
    if (EnsureHeaders() && _source) _source->SetClamp(clampFlags);
  }
  virtual bool GetRect(int level, int x, int y, int width, int height,
    PacFormat format, void *data) const
  {
    if (!EnsureHeaders() || !_source || !data || level < 0 || level >= _mipCount)
      return false;
    const PacLevelMem &sourceMip = _mips[level];
    if (x < 0 || y < 0 || width <= 0 || height <= 0 ||
        x + width > sourceMip._w || y + height > sourceMip._h ||
        x != 0 || y != 0 || width != sourceMip._w || height != sourceMip._h)
      return false;
    PacLevelMem requestedMip = sourceMip;
    requestedMip._dFormat = format;
    const int dataSize = PacLevelMem::MipmapSize(format, width, height);
    if (dataSize <= 0) return false;
    requestedMip._pitch = dataSize / height;
    return _source->GetMipmapData(data, requestedMip, level, 0);
  }
  virtual bool RequestGetRect(int level, int, int, int, int) const
  {
    if (!EnsureHeaders() || !_source || level < 0 || level >= _mipCount)
      return false;
    return _source->RequestMipmapData(_mips, level, level,
      FileRequestPriority(10));
  }
  virtual QFileTime GetTimestamp() const
  {
    return EnsureHeaders() && _source ? _source->GetTimestamp() : 0;
  }
  virtual bool IsDynRangeCompressed() const
  {
    return EnsureHeaders() && _source && _source->GetMaxColor() != PackedWhite;
  }
  virtual bool VerifyChecksum(const MipInfo &) const { return true; }
  virtual TextureType Type() const
  {
    return EnsureHeaders() && _source ? _source->GetTextureType() : TT_Diffuse;
  }
  virtual TexFilter GetMaxFilter() const
  {
    return EnsureHeaders() && _source ? _source->GetMaxFilter() : TFAnizotropic16;
  }
  virtual int ProcessingThreadId() const { return -1; }
  virtual void RequestDone(RequestContext *, RequestResult) {}
};

Dx11TextBank::Dx11TextBank()
{
}

Dx11TextBank::~Dx11TextBank()
{
  ScopeLockSection lock(_texturesLock);
  ClearTexCache();
  DeleteAllAnimated();
  for (int i = 0; i < _textures.Size(); ++i)
  {
    Dx11TextureAsset *texture = _textures[i];
    if (texture) _textures[i] = NULL;
  }
}

int Dx11TextBank::Find(RStringB name) const
{
  ScopeLockSection lock(_texturesLock);
  for (int i = 0; i < _textures.Size(); ++i)
  {
    Dx11TextureAsset *texture = _textures[i];
    if (texture && texture->GetName() == name) return i;
  }
  return -1;
}

void Dx11TextBank::Remove(Dx11TextureAsset *texture)
{
  ScopeLockSection lock(_texturesLock);
  for (int i = 0; i < _textures.Size(); ++i)
  {
    if (_textures[i] == texture)
    {
      _textures[i] = NULL;
      return;
    }
  }
}

Ref<Texture> Dx11TextBank::CreateTexture(ITextureSource *source)
{
  if (!source) return NULL;
  Dx11TextureAsset *texture = new Dx11TextureAsset;
  texture->_source = source;
  if (!texture->EnsureHeaders())
  {
    delete texture;
    return NULL;
  }
  TextureSourcePacBase *pacSource = dynamic_cast<TextureSourcePacBase *>(source);
  if (pacSource && pacSource->GetName().GetLength() > 0)
    texture->SetName(pacSource->GetName());
  return texture;
}

Ref<Texture> Dx11TextBank::Load(RStringB name, bool caseSensitive)
{
  ScopeLockSection lock(_texturesLock);
  if (name.GetLength() <= 0) return NULL;
  int index = Find(name);
  if (index >= 0) return (Texture *)_textures[index];

  Ref<Texture> texture = LoadCached(name);
  if (!texture)
  {
    Dx11TextureAsset *asset = new Dx11TextureAsset;
    asset->SetName(name, caseSensitive);
    texture = asset;
  }
  Dx11TextureAsset *asset = dynamic_cast<Dx11TextureAsset *>(texture.GetRef());
  if (!asset)
  {
    // The inherited AbstractTextBank cache may contain a legacy Texture type.
    // Never reinterpret it as our DX11 asset (its private mip array has a
    // different layout and would pass an invalid output buffer to PAC parsing).
    texture = new Dx11TextureAsset;
    asset = static_cast<Dx11TextureAsset *>(texture.GetRef());
    asset->SetName(name, caseSensitive);
  }
  int freeIndex = _textures.FindKey(NULL);
  if (freeIndex < 0) freeIndex = _textures.Add();
  _textures[freeIndex] = asset;
  return texture;
}

MipInfo Dx11TextBank::UseMipmap(Texture *texture, int level, int levelTop, int)
{
  if (!texture) return MipInfo(NULL, 0);
  Dx11TextureAsset *asset = static_cast<Dx11TextureAsset *>(texture);
  const int mipCount = asset->ANMipmaps();
  if (mipCount <= 0) return MipInfo(NULL, 0);
  int selected = levelTop >= 0 ? levelTop : level;
  if (selected < 0) selected = 0;
  if (selected >= mipCount) selected = mipCount - 1;
  return MipInfo(texture, selected);
}

MipInfo Dx11TextBank::UseMipmapLoaded(Texture *texture)
{
  if (!texture) return MipInfo(NULL, 0);
  Dx11TextureAsset *asset = static_cast<Dx11TextureAsset *>(texture);
  return asset->ANMipmaps() > 0 ? MipInfo(texture, 0) : MipInfo(NULL, 0);
}

void Dx11TextBank::Compact()
{
}

void Dx11TextBank::GetTextureList(LLinkArray<Texture> &list)
{
  ScopeLockSection lock(_texturesLock);
  list.Realloc(_textures.Size());
  list.Resize(_textures.Size());
  for (int i = 0; i < _textures.Size(); ++i)
    list[i] = _textures[i] ? (Texture *)_textures[i].GetLink() : NULL;
}

void Dx11TextBank::FlushBank(QFBank *bank)
{
  ScopeLockSection lock(_texturesLock);
  FlushBankFromCache(bank);
}

HRESULT Dx11TextBank::CreateGpuTexture(ID3D11Device *device, Texture *texture,
  Dx11Texture *gpuTexture) const
{
  if (!device || !texture || !gpuTexture) return E_INVALIDARG;
  const Dx11TextureAsset *asset = dynamic_cast<const Dx11TextureAsset *>(texture);
  if (!asset)
  {
    RptF("DX11 texture upload received legacy Texture: %s object=%p",
      (const char *)texture->GetName(), texture);
    return E_INVALIDARG;
  }
  if (!asset->EnsureHeaders() || !asset->_source)
  {
    RptF("DX11 texture source unavailable: %s ready=%d source=%p failed=%d",
      (const char *)texture->GetName(), asset->_headersReady ? 1 : 0,
      asset->_source.GetRef(), asset->_failed ? 1 : 0);
    return E_FAIL;
  }
  return gpuTexture->InitializeFromSource(device, asset->_source,
    const_cast<PacLevelMem *>(asset->_mips), Dx11MaxMipmaps);
}

Color Dx11TextBank::GetMaxColor(Texture *texture) const
{
  const Dx11TextureAsset *asset = dynamic_cast<const Dx11TextureAsset *>(texture);
  if (!asset) return HWhite;
  Color maxColor = asset->GetMaxColor();
  if (IsSRGB(texture))
    return Color(Dx11SRGBToLinear(maxColor.R()),
      Dx11SRGBToLinear(maxColor.G()), Dx11SRGBToLinear(maxColor.B()),
      maxColor.A());
  return maxColor;
}

bool Dx11TextBank::IsSRGB(Texture *texture) const
{
  const Dx11TextureAsset *asset = dynamic_cast<const Dx11TextureAsset *>(texture);
  if (!asset) return false;
  const TextureType type = asset->Type();
  return type == TT_Diffuse || type == TT_Macro;
}
