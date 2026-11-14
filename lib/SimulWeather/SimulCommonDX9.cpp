#include "../wpch.hpp"

#if _ARMA3_SIMUL_WEATHER

#include "SimulCommonDX9.hpp"
#include "../d3d9/engdd9.hpp"

//========================================================================
// TextureSysGPUDX9 ::LockedData implementation
//========================================================================
TextureSysGPUDX9::LockedData::LockedData(Ref<TextureSysGPUDX9> texture)
:_texture(texture)
{
  if (_texture)
  {
    _texture->EnterCriticalSection();
    _texture->Lock();
    _sysMemTex = _texture->GetCurrentSysMemTex();
    if (_sysMemTex)
      _data = _sysMemTex->GetLockedData();

  }
};

TextureSysGPUDX9::LockedData::~LockedData()
{
  if (_texture)
    _texture->LeaveCriticalSection();
};

bool TextureSysGPUDX9::LockedData::CheckSize()
{
  if (_sysMemTex)
    return _sysMemTex->CheckSize();
  else
    return false;
}

bool TextureSysGPUDX9::LockedData::IsPitchCorrespondingSize(int texelSizeInBytes)
{
  if (_sysMemTex)
    return _sysMemTex->IsPitchCorrespondingSize(texelSizeInBytes);
  else
    return false;
}

int TextureSysGPUDX9::LockedData::GetMappedTextureOffset(int texelIndex, int texelSizeInBytes) const
{
  if (_sysMemTex)
    return _sysMemTex->GetMappedTextureOffset(texelIndex, texelSizeInBytes);
  else
    return 0;
}

//========================================================================
// TextureSysGPUDX9::SysMemTex implementation
//========================================================================

TextureSysGPUDX9::SysMemTex::SysMemTex(int width, int height, int depth)
:_locked(false), _width(width), _height(height), _depth(depth)
{
  ResetLocked();
}

TextureSysGPUDX9::SysMemTex::~SysMemTex()
{
  Unlock();
}

void TextureSysGPUDX9::SysMemTex::Lock()
{
  if (_locked)
    return; //already locked

  Assert(GEngineDD);
  if (!GEngineDD)
    return;
  Assert(_texture);
  if (!_texture)
    return;

  HRESULT hr=_texture->LockBox(0,&_lockedBox,NULL,NULL);
  if(FAILED(hr))
  {
    GEngineDD->DDError9("SimulWeather - volume texture - LockBox failed: ", hr);
    return;
  }    
  _locked = true;
}

void TextureSysGPUDX9::SysMemTex::Unlock()
{
  if (!_locked)
    return; //not locked

  // reset locked status
  ResetLocked();

  Assert(GEngineDD);
  if (!GEngineDD)
    return;
  Assert(_texture);
  if (!_texture)
    return;

  HRESULT hr=_texture->UnlockBox(0);
  if(FAILED(hr))
  {
    GEngineDD->DDError9("SimulWeather - volume texture - UnlockBox failed: ", hr);
    return;
  }    
}

void TextureSysGPUDX9::SysMemTex::ResetLocked()
{
  _locked = false;
  _lockedBox.pBits = 0;
  _lockedBox.RowPitch = 0;
  _lockedBox.SlicePitch = 0;
}

int TextureSysGPUDX9::SysMemTex::GetMappedTextureOffset(int texelIndex, int texelSizeInBytes) const
{
  Assert(_locked);
  Assert(_width > 0 && _height > 0 && _depth > 0);

  int widthHeight = _width * _height;
  int depthSlice = texelIndex / widthHeight;
  int indexInDepthSlice = texelIndex % widthHeight;
  int x = indexInDepthSlice % _width;
  int y = indexInDepthSlice / _width;

  int offset = (
    depthSlice * _lockedBox.SlicePitch + 
    y * _lockedBox.RowPitch  + 
    x * texelSizeInBytes
    );

  Assert(offset < _depth * _lockedBox.SlicePitch);

  return offset;
}

/// returns if pitch of the mapped texture corresponds to its size
bool TextureSysGPUDX9::SysMemTex::IsPitchCorrespondingSize(int texelSizeInBytes)
{
  return _lockedBox.RowPitch == _width * texelSizeInBytes
    && _lockedBox.SlicePitch == _height * _lockedBox.RowPitch;
}

//========================================================================
// TextureSysGPUDX9 implementation
//========================================================================

TextureSysGPUDX9::TextureSysGPUDX9()
:_needsUpdate(true), _currentSysMemTex(0)
{
}

TextureSysGPUDX9::~TextureSysGPUDX9()
{
}
 
void TextureSysGPUDX9::Lock()
{
  _needsUpdate = true;
  Assert(_currentSysMemTex < NumSysMemTextures);
  if (_currentSysMemTex >= NumSysMemTextures)
    return;
  Assert(_sysMemTex[_currentSysMemTex]);
  if (!_sysMemTex[_currentSysMemTex])
    return;

  _sysMemTex[_currentSysMemTex]->Lock();
}

void TextureSysGPUDX9::Unlock()
{
  Assert(_currentSysMemTex < NumSysMemTextures);
  if (_currentSysMemTex >= NumSysMemTextures)
    return;
  Assert(_sysMemTex[_currentSysMemTex]);
  if (!_sysMemTex[_currentSysMemTex])
    return;
  _sysMemTex[_currentSysMemTex]->Unlock();
}

void TextureSysGPUDX9::UpdateGPUTex(bool forceUpdate)
{
  PROFILE_SCOPE_EX(updGPU, SW);

  if (!forceUpdate && !_needsUpdate)
    return; //no need to update
  
  _needsUpdate = false;

  Assert(GEngineDD);
  if (!GEngineDD)
    return;

  GEngineDD->UpdateSimulWeatherTexture(this, _currentSysMemTex);
  // advance current texture
  _currentSysMemTex = (_currentSysMemTex + 1) % NumSysMemTextures;
}

void TextureSysGPUDX9::UpdateGPUTexInternal(unsigned sysMemTextureIndex)
{
  // we need to protect this with critical section, so that our sys mem texture can not be updated during this function
  ScopeLockSection lock(_criticalSection);

  Assert(GEngineDD);
  if (!GEngineDD)
    return;
  Assert(sysMemTextureIndex < NumSysMemTextures);
  if (sysMemTextureIndex >= NumSysMemTextures)
    return;
  Assert(_sysMemTex[sysMemTextureIndex]);
  if (!_sysMemTex[sysMemTextureIndex])
    return;
  Assert(_sysMemTex[sysMemTextureIndex]->GetTexture());
  if (!_sysMemTex[sysMemTextureIndex]->GetTexture())
    return;
  Assert(_gpuTex);
  if (!_gpuTex)
    return;

  // unlock first
  _sysMemTex[sysMemTextureIndex]->Unlock();

  // add dirty region
  HRESULT hr=_sysMemTex[sysMemTextureIndex]->GetTexture()->AddDirtyBox(NULL);
  if(FAILED(hr))
  {
    GEngineDD->DDError9("SimulWeather - volume texture - AddDirtyBox failed: ", hr);
    return;
  }    

  Assert(!GEngineDD->GetDirect3DDevice().IsNull());
  if (GEngineDD->GetDirect3DDevice().IsNull())
    return;

  // update texture
  hr = GEngineDD->GetDirect3DDevice()->UpdateTexture(_sysMemTex[sysMemTextureIndex]->GetTexture(), _gpuTex);
  if(FAILED(hr))
  {
    GEngineDD->DDError9("SimulWeather - UpdateTexture failed: ", hr);
    return;
  }
}

ComRef<IDirect3DVolumeTexture9>& TextureSysGPUDX9::GetGPUTex()
{
  if (_gpuTex)
    UpdateGPUTex(); //we need to update GPU tex when user needs it

  return _gpuTex;
}

HRESULT TextureSysGPUDX9::CreateTextures(int width, int height, int depth, D3DFORMAT format)
{
  ScopeLockSection lock(_criticalSection);

  Assert(GEngineDD);
  if (!GEngineDD)
    return S_FALSE;

  Assert(!GEngineDD->GetDirect3DDevice().IsNull());
  if (GEngineDD->GetDirect3DDevice().IsNull())
    return S_FALSE;

  for (unsigned i=0; i<NumSysMemTextures; ++i)
  {
    _sysMemTex[i] = new SysMemTex(width, height, depth);

    HRESULT hr = GEngineDD->GetDirect3DDevice()->CreateVolumeTexture(width, height, depth, 1, 
      0, format, D3DPOOL_SYSTEMMEM, _sysMemTex[i]->GetTexture().Init(), NULL);

    if (FAILED(hr))
    {
      GEngineDD->DDError9("SimulWeather - Creating of system memory texture failed!", hr);
      return hr;
    }
  }

  /// create GPU texture
  HRESULT hr = GEngineDD->GetDirect3DDevice()->CreateVolumeTexture(width, height, depth, 1, 
    0, format, D3DPOOL_DEFAULT, _gpuTex.Init(), NULL);

  if (FAILED(hr))
  {
    GEngineDD->DDError9("SimulWeather - Creating of gpu texture failed!", hr);
    return hr;
  }

  return hr;
}

#endif //#if _ARMA3_SIMUL_WEATHER