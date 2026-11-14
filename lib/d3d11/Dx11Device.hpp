#pragma once
#ifndef DAYZ_D3D11_DEVICE_HPP
#define DAYZ_D3D11_DEVICE_HPP

#include <d3d11.h>
#include <dxgi.h>

class Dx11Device
{
public:
  Dx11Device();
  ~Dx11Device();
  Dx11Device(const Dx11Device &) = delete;
  Dx11Device &operator=(const Dx11Device &) = delete;

  HRESULT Create(HWND window, UINT width, UINT height, bool windowed = true,
    D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE);
  HRESULT Resize(UINT width, UINT height);
  HRESULT SetLinearColorOutput(bool enabled);
  HRESULT Clear(float red, float green, float blue, float alpha,
    bool clearColor, bool clearDepth, bool clearStencil);
  HRESULT BeginFrame(float red, float green, float blue, float alpha = 1.0f);
  HRESULT EndFrame(UINT syncInterval = 1);
  void Reset();

  ID3D11Device *Device() const { return _device; }
  ID3D11DeviceContext *Context() const { return _context; }
  IDXGISwapChain *SwapChain() const { return _swapChain; }
  ID3D11RenderTargetView *BackBufferView() const { return _backBufferView; }
  D3D_FEATURE_LEVEL FeatureLevel() const { return _featureLevel; }

private:
  HRESULT CreateBackBufferView();
  ID3D11Device *_device;
  ID3D11DeviceContext *_context;
  IDXGISwapChain *_swapChain;
  ID3D11RenderTargetView *_backBufferView;
  ID3D11RenderTargetView *_backBufferSRGBView;
  ID3D11Texture2D *_depthBuffer;
  ID3D11DepthStencilView *_depthStencilView;
  D3D_FEATURE_LEVEL _featureLevel;
};

#endif
