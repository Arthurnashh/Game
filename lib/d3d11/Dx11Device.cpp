#include "../wpch.hpp"
#include "Dx11Device.hpp"
#include <string.h>

Dx11Device::Dx11Device()
  : _device(NULL), _context(NULL), _swapChain(NULL), _backBufferView(NULL),
    _backBufferSRGBView(NULL),
    _depthBuffer(NULL), _depthStencilView(NULL),
    _featureLevel(static_cast<D3D_FEATURE_LEVEL>(0))
{
}

Dx11Device::~Dx11Device()
{
  Reset();
}

void Dx11Device::Reset()
{
  if (_context) _context->ClearState();
  if (_depthStencilView) { _depthStencilView->Release(); _depthStencilView = NULL; }
  if (_depthBuffer) { _depthBuffer->Release(); _depthBuffer = NULL; }
  if (_backBufferSRGBView) { _backBufferSRGBView->Release(); _backBufferSRGBView = NULL; }
  if (_backBufferView) { _backBufferView->Release(); _backBufferView = NULL; }
  if (_swapChain) { _swapChain->Release(); _swapChain = NULL; }
  if (_context) { _context->Release(); _context = NULL; }
  if (_device) { _device->Release(); _device = NULL; }
  _featureLevel = static_cast<D3D_FEATURE_LEVEL>(0);
}

HRESULT Dx11Device::Create(HWND window, UINT width, UINT height, bool windowed,
  D3D_DRIVER_TYPE driverType)
{
  if (!window || !width || !height) return E_INVALIDARG;
  Reset();

  DXGI_SWAP_CHAIN_DESC swapDesc = {};
  swapDesc.BufferDesc.Width = width;
  swapDesc.BufferDesc.Height = height;
  swapDesc.BufferDesc.RefreshRate.Numerator = 0;
  swapDesc.BufferDesc.RefreshRate.Denominator = 1;
  swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapDesc.SampleDesc.Count = 1;
  swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapDesc.BufferCount = 2;
  swapDesc.OutputWindow = window;
  swapDesc.Windowed = windowed ? TRUE : FALSE;
  swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  const D3D_FEATURE_LEVEL levels[] = {
    D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
  };
  // Optional diagnostic switch for separating engine faults from hardware
  // driver faults. Normal launches continue to use the requested driver.
  const bool forceWarp = strstr(GetCommandLineA(), "-dx11warp") != NULL;
  const D3D_DRIVER_TYPE selectedDriver = forceWarp ? D3D_DRIVER_TYPE_WARP : driverType;
  if (forceWarp) RptF("DX11 diagnostic: using WARP software rasterizer");
  HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, selectedDriver, NULL,
    D3D11_CREATE_DEVICE_BGRA_SUPPORT, levels, ARRAYSIZE(levels),
    D3D11_SDK_VERSION, &swapDesc, &_swapChain, &_device, &_featureLevel, &_context);

  if (FAILED(hr))
  {
    RptF("DX11: D3D11CreateDeviceAndSwapChain failed (0x%08x)", (unsigned)hr);
    Reset();
    return hr;
  }

  hr = CreateBackBufferView();
  if (FAILED(hr))
  {
    RptF("DX11: back buffer/depth view creation failed (0x%08x)", (unsigned)hr);
    Reset();
  }
  else
  {
    RptF("DX11: device and swap chain ready, feature level 0x%04x",
      (unsigned)_featureLevel);
  }
  return hr;
}

HRESULT Dx11Device::CreateBackBufferView()
{
  ID3D11Texture2D *backBuffer = NULL;
  HRESULT hr = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
    reinterpret_cast<void **>(&backBuffer));
  if (SUCCEEDED(hr))
  {
    D3D11_TEXTURE2D_DESC backBufferDesc = {};
    backBuffer->GetDesc(&backBufferDesc);
    hr = _device->CreateRenderTargetView(backBuffer, NULL, &_backBufferView);
    if (SUCCEEDED(hr) && backBufferDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM)
    {
      D3D11_RENDER_TARGET_VIEW_DESC srgbViewDesc = {};
      srgbViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
      srgbViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
      srgbViewDesc.Texture2D.MipSlice = 0;
      HRESULT srgbHr = _device->CreateRenderTargetView(backBuffer,
        &srgbViewDesc, &_backBufferSRGBView);
      if (FAILED(srgbHr))
        RptF("DX11 optional sRGB back-buffer view unavailable: hr=0x%08x",
          (unsigned)srgbHr);
    }
    D3D11_TEXTURE2D_DESC depthDesc = backBufferDesc;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.ArraySize = 1;
    depthDesc.MipLevels = 1;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthDesc.CPUAccessFlags = 0;
    depthDesc.MiscFlags = 0;
    if (SUCCEEDED(hr)) hr = _device->CreateTexture2D(&depthDesc, NULL, &_depthBuffer);
    if (SUCCEEDED(hr)) hr = _device->CreateDepthStencilView(_depthBuffer, NULL, &_depthStencilView);
    backBuffer->Release();
    if (FAILED(hr)) return hr;

    _context->OMSetRenderTargets(1, &_backBufferView, _depthStencilView);
    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(backBufferDesc.Width);
    viewport.Height = static_cast<float>(backBufferDesc.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    _context->RSSetViewports(1, &viewport);
  }
  return hr;
}

HRESULT Dx11Device::Resize(UINT width, UINT height)
{
  if (!_swapChain || !width || !height) return E_INVALIDARG;
  _context->OMSetRenderTargets(0, NULL, NULL);
  if (_depthStencilView) { _depthStencilView->Release(); _depthStencilView = NULL; }
  if (_depthBuffer) { _depthBuffer->Release(); _depthBuffer = NULL; }
  if (_backBufferSRGBView) { _backBufferSRGBView->Release(); _backBufferSRGBView = NULL; }
  if (_backBufferView) { _backBufferView->Release(); _backBufferView = NULL; }
  HRESULT hr = _swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
  if (SUCCEEDED(hr)) hr = CreateBackBufferView();
  return hr;
}

HRESULT Dx11Device::SetLinearColorOutput(bool enabled)
{
  if (!_context || !_backBufferView || !_depthStencilView) return E_FAIL;
  ID3D11RenderTargetView *view = enabled && _backBufferSRGBView
    ? _backBufferSRGBView : _backBufferView;
  _context->OMSetRenderTargets(1, &view, _depthStencilView);
  return S_OK;
}

HRESULT Dx11Device::BeginFrame(float red, float green, float blue, float alpha)
{
  if (!_context || !_backBufferView || !_depthStencilView) return E_FAIL;
  _context->OMSetRenderTargets(1, &_backBufferView, _depthStencilView);
  return Clear(red, green, blue, alpha, true, true, true);
}

HRESULT Dx11Device::Clear(float red, float green, float blue, float alpha,
  bool clearColor, bool clearDepth, bool clearStencil)
{
  if (!_context || !_backBufferView || !_depthStencilView) return E_FAIL;
  if (clearColor)
  {
    const float color[4] = { red, green, blue, alpha };
    _context->ClearRenderTargetView(_backBufferView, color);
  }
  UINT clearFlags = 0;
  if (clearDepth) clearFlags |= D3D11_CLEAR_DEPTH;
  if (clearStencil) clearFlags |= D3D11_CLEAR_STENCIL;
  if (clearFlags)
    _context->ClearDepthStencilView(_depthStencilView, clearFlags, 1.0f, 0);
  return S_OK;
}

HRESULT Dx11Device::EndFrame(UINT syncInterval)
{
  return _swapChain ? _swapChain->Present(syncInterval, 0) : E_FAIL;
}
