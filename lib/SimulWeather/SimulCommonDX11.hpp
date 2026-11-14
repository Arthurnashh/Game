#pragma once
#ifndef DAYZ_SIMUL_COMMON_DX11_HPP
#define DAYZ_SIMUL_COMMON_DX11_HPP

#if _ARMA3_SIMUL_WEATHER

#include "../d3d11/Dx11VolumeTexture.hpp"
#include "../d3d11/Dx11DeviceAccess.hpp"
#include <Es/Threads/multisync.hpp>
#include <vector>

// CPU update buffer plus D3D11 Texture3D for the weather simulation's
// incrementally generated cloud and illumination fields.
class TextureSysGPUDX11 : public RefCount, private NoCopy
{
public:
  class LockedData : public RefCount, private NoCopy
  {
  public:
    explicit LockedData(Ref<TextureSysGPUDX11> texture);
    ~LockedData();
    void *GetData() const { return _data; }
    bool CheckSize() const;
    bool IsPitchCorrespondingSize(int texelSizeInBytes) const;
    int GetMappedTextureOffset(int texelIndex, int texelSizeInBytes) const;
  private:
    Ref<TextureSysGPUDX11> _texture;
    void *_data;
  };

  TextureSysGPUDX11(Dx11DeviceAccess *engine, int bytesPerTexel = 4);
  ~TextureSysGPUDX11();

  HRESULT CreateTextures(int width, int height, int depth, DXGI_FORMAT format);
  Ref<LockedData> LockData() { return new LockedData(Ref<TextureSysGPUDX11>(this)); }
  static Ref<LockedData> GetLockedData(Ref<TextureSysGPUDX11> texture)
    { return new LockedData(texture); }

  ID3D11ShaderResourceView *GetResourceView();
  Dx11VolumeTexture *GetGPUTex();
  int Width() const { return _width; }
  int Height() const { return _height; }
  int Depth() const { return _depth; }

private:
  friend class LockedData;
  void EnterCriticalSection() { _criticalSection.Lock(); }
  void LeaveCriticalSection();
  void UploadIfDirty();

  Dx11DeviceAccess *_engine;
  int _bytesPerTexel;
  int _width, _height, _depth;
  bool _dirty;
  std::vector<unsigned char> _staging;
  Dx11VolumeTexture _gpuTexture;
  CriticalSection _criticalSection;
};

#endif
#endif
