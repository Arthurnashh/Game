#pragma once
#ifndef DAYZ_D3D11_TEXT_BANK_HPP
#define DAYZ_D3D11_TEXT_BANK_HPP

#include <d3d11.h>
#include "../textbank.hpp"

class Dx11TextureAsset;
class Dx11Texture;

// Synchronous source and mip metadata bridge for the DX11 engine path.
// GPU residency is managed by Dx11EngineCore; this bank owns game Texture
// objects and their source descriptors.
class Dx11TextBank : public AbstractTextBank
{
  friend class Dx11TextureAsset;
  // The renderer can request the same asset from multiple worker threads.
  // Protect the bank's non-thread-safe LLinkArray and cache lookup/update.
  mutable CriticalSection _texturesLock;
  LLinkArray<Dx11TextureAsset> _textures;

  int Find(RStringB name) const;
  void Remove(Dx11TextureAsset *texture);

public:
  Dx11TextBank();
  virtual ~Dx11TextBank();

  virtual Ref<Texture> CreateTexture(ITextureSource *source);
  virtual Ref<Texture> Load(RStringB name, bool caseSensitive = false);
  virtual MipInfo UseMipmap(Texture *texture, int level, int levelTop,
    int priority = TexPriorityNormal);
  virtual MipInfo UseMipmapLoaded(Texture *texture);
  virtual void Compact();
  virtual void GetTextureList(LLinkArray<Texture> &list);
  virtual void FlushBank(QFBank *bank);
  HRESULT CreateGpuTexture(ID3D11Device *device, Texture *texture,
    Dx11Texture *gpuTexture) const;
  Color GetMaxColor(Texture *texture) const;
  bool IsSRGB(Texture *texture) const;
};

#endif
