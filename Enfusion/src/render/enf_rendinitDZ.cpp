//-----------------------------------------------------------------------------
// File: enf_rendinit.cpp
//
// Desc: Renderer device initialization ()
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#include "enf_main.h"
#include "render/enf_irendmain.h"

#ifdef ENF_DUMMYRENDER

//-----------------------------------------------------------------------------
enf::ERESULT enf::IRenderer::Create() 
{
	return EE_ERROR;
}

//-----------------------------------------------------------------------------
bool enf::IRenderer::Init(void* hwnd)
{
	return false;
}

#else

#include "lib/d3d9/engdd9.hpp"


//-----------------------------------------------------------------------------
enf::ERESULT enf::IRenderer::Create()
{
	m_Setup.m_bWindowed							= GEngineDD->_windowed;
	m_Setup.m_DepthBufferFormat			=	(enf::TextureFormat) GEngineDD->_presentPars.AutoDepthStencilFormat;
	m_Setup.m_BackBufferFormat			= (enf::TextureFormat) GEngineDD->_presentPars.BackBufferFormat; //_rtFormat is main RT format, currently 16 bit
	m_Setup.m_FSAAType							= (FSAAType) GEngineDD->_fsAAWanted;
	m_Setup.m_iBackBufferCount			= GEngineDD->_presentPars.BackBufferCount;

	m_Setup.m_iBackBufferHeight			=	GEngineDD->_presentPars.BackBufferHeight;
	m_Setup.m_iBackBufferWidth			=	GEngineDD->_presentPars.BackBufferWidth;
	m_Setup.m_iFlags								= GEngineDD->_presentPars.Flags;
	m_Setup.m_PresentationInterval	=	GEngineDD->_presentPars.PresentationInterval;
	m_Setup.m_SwapEffect						= GEngineDD->_presentPars.SwapEffect;
	m_Setup.m_iRefreshRateInHz			= GEngineDD->_refreshRate;

	m_iTargetWidth	=	m_Setup.m_iBackBufferWidth;
	m_iTargetHeight	= m_Setup.m_iBackBufferHeight;

	/*
	m_Viewport = GEngineDD->;

	m_iAdapter = 0;
	m_uiAvailMem = ;
	m_bATICrap = ;
*/
	Renderer::FindGPUInfo();

	return EE_OK;
}

//-----------------------------------------------------------------------------
bool enf::IRenderer::Init(void* hwnd)
{
	m_hWnd = hwnd;

	m_pD3D = GEngineDD->GetDirect3D();
	if (!m_pD3D)
		return false;

	m_pd3dDevice	= GEngineDD->GetDirect3DDevice()->GetRef();
	if (!m_pd3dDevice)
		return false;

	m_pd3dDevice->GetDeviceCaps(&m_d3dCaps);
	m_uiAvailMem = m_pd3dDevice->GetAvailableTextureMem();

	return true;
}




#endif