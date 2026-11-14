//-----------------------------------------------------------------------------
// File: enf_rendswapchain.cpp
//
// Desc: Render class, swap chain abstraction
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "enf_main.h"
#include "render/enf_irendmain.h"
#include "render/enf_renderresources.h"
#include "system/enf_profiler.h"

namespace enf
{
#ifdef ENF_DX9
//-----------------------------------------------------------------------------
IDirect3DSurface9* SwapChain::GetBackBuffer()
{
	IDirect3DSurface9* back;
	if(SUCCEEDED(m_pSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back)))
		return back;

	return NULL;
}

//-----------------------------------------------------------------------------
ERESULT SwapChain::Reset(void* window, uint width, uint height)
{
	if(width == NULL || height == NULL || window == NULL)
		return EE_ERROR;

	Free();
	m_hWindow = window;
	m_iWidth = width;
	m_iHeight = height;
	Allocate();
	return EE_OK;
}

//-----------------------------------------------------------------------------
void SwapChain::Free()
{
	int c;
	if(m_pDepthBuffer)
	{
		c = m_pDepthBuffer->Release();
		enf_assert(c == 0);
		m_pDepthBuffer = NULL;
	}
	if(m_pSwapChain)
	{
		c = m_pSwapChain->Release();
		enf_assert(c == 0);
		m_pSwapChain = NULL;
	}
}

//-----------------------------------------------------------------------------
ERESULT SwapChain::Allocate()
{
	D3DPRESENT_PARAMETERS params;

	params.BackBufferWidth = m_iWidth;
	params.BackBufferHeight = m_iHeight;
	params.BackBufferFormat = D3DFMT_A8R8G8B8;
	params.BackBufferCount = 1;
	params.MultiSampleType = D3DMULTISAMPLE_NONE;
	params.MultiSampleQuality = 0;
	params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	params.hDeviceWindow = (HWND)m_hWindow;
	params.Windowed = true;
	params.EnableAutoDepthStencil = false;
	params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
	params.Flags = 0;//D3DPRESENTFLAG_DEVICECLIP;
	params.FullScreen_RefreshRateInHz = 0;
	params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	HRESULT hr = g_pRender->GetDXDevice()->CreateAdditionalSwapChain(&params, &m_pSwapChain);

	hr = g_pRender->GetDXDevice()->CreateDepthStencilSurface(
		m_iWidth,
		m_iHeight,
		D3DFMT_D24S8,
		D3DMULTISAMPLE_NONE,
		0,
		true,
		&m_pDepthBuffer,
		NULL);

	return EE_OK;
}

//-----------------------------------------------------------------------------
SwapChain::SwapChain(IRenderer* dev): DeviceResource(dev)
{
	m_pDepthBuffer = NULL;
	m_pSwapChain = NULL;
}

//-----------------------------------------------------------------------------
void SwapChain::Present(const RRect* destrect, void* hwindow)
{
	RECT rect;
	if(destrect)
	{
		rect.left = destrect->Left;
		rect.top = destrect->Top;
		rect.right = destrect->Right;
		rect.bottom = destrect->Bottom;
	}
	ENF_PROFILE(PROFILE_PRESENT);

	//sometimes happen. We don't want to crash anyway
	if(m_pSwapChain)
		m_pSwapChain->Present(NULL, destrect ? &rect : NULL, (HWND)hwindow, NULL, 0);
}

//-----------------------------------------------------------------------------
SwapChain* IRenderer::CreateSwapChain(void* window, uint width, uint height)
{
	if(width == NULL || height == NULL || window == NULL)
		return NULL;

	SwapChain* sw = ENF_NEW SwapChain(this);
	sw->m_hWindow = window;
	sw->m_iWidth = width;
	sw->m_iHeight = height;
	sw->m_bManaged = false;

	AddResource(sw);

	sw->Allocate();
	return sw;
}
#endif
}