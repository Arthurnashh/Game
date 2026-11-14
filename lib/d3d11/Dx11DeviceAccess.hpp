#pragma once
#ifndef DAYZ_D3D11_DEVICE_ACCESS_HPP
#define DAYZ_D3D11_DEVICE_ACCESS_HPP

#include <Es/Common/win.h>
#include <d3d11.h>

// Shared access to the active D3D11 device for game features that must work
// with either the standalone porting engine or the full EngineDD11 backend.
class Dx11DeviceAccess
{
public:
  virtual ~Dx11DeviceAccess() {}
  virtual ID3D11Device *D3D11Device() const = 0;
  virtual ID3D11DeviceContext *D3D11Context() const = 0;
};

#endif
