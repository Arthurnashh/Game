
#ifdef _MSC_VER
#pragma once
#endif

#ifndef _D3DXB_DEFS_HPP
#define _D3DXB_DEFS_HPP

#include <Es/common/win.h>
#ifndef _XBOX

	#include <d3d8.h>

  #include <Es/Memory/normalNew.hpp>

  #include <d3dx8.h>

  #include <Es/Memory/debugNew.hpp>

#else
  #include <XGraphics.h>
#endif


#if DIRECT3D_VERSION<0x0800
	#pragma message("DirectX 8 SDK required")
#endif

typedef IDirect3DTexture8 *TextureD3DHandle;

#endif