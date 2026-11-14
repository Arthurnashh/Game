
#ifdef _MSC_VER
#pragma once
#endif

#ifndef _D3D9_DEFS_HPP
#define _D3D9_DEFS_HPP

#include <Es/common/win.h>

#ifdef _DEBUG
#ifndef D3D_DEBUG_INFO
#define D3D_DEBUG_INFO
#endif
#endif

#include <d3d9.h>

#include <Es/Memory/normalNew.hpp>

#include <d3dx9.h>

#include <Es/Memory/debugNew.hpp>

#if DIRECT3D_VERSION<0x0900
# pragma message("DirectX 9 SDK required")
#endif

# define ENCAPSULATE_TEXTURE_REF 0 // native D3D style Ref counting used on PC

#if ENCAPSULATE_TEXTURE_REF
  struct TextureHandle9;
#else
  typedef IDirect3DTexture9 TextureHandle9;
#endif


// #undef SearchPath
#undef DrawText
#undef GetObject
#undef GetMessage
#undef min
#undef max
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

#if _ENABLE_PERFLOG && _PROFILE
  #define PERF_PROF 1
#else
  #define PERF_PROF 0
#endif

#include <El/Common/perfProf.hpp>

#if PERF_PROF

  //#define PROFILE_DX_SCOPE(name) PROFILE_SCOPE(d3d)
  //#define PROFILE_DX_SCOPE_DETAIL(name) PROFILE_SCOPE_DETAIL(d3d)
  #define PROFILE_DX_SCOPE(name) PROFILE_SCOPE_EX(name,d3d)
  #define PROFILE_DX_SCOPE_DETAIL(name) PROFILE_SCOPE_DETAIL_EX(name,d3d)

#else
  #define PROFILE_DX_SCOPE(name)
  #define PROFILE_DX_SCOPE_DETAIL(name)
#endif

class MyD3DDevice;

/// used for VRAM/AGP checks
#include <ddraw.h>


#if defined(_X360SHADERGEN)

// simulate some undefined flags on Xbox
#define D3DMULTISAMPLE_NONMASKABLE D3DMULTISAMPLE_NONE // TODOX360: D3DMULTISAMPLE_2_SAMPLES // D3DMULTISAMPLE_4_SAMPLES
#define D3DCREATE_SOFTWARE_VERTEXPROCESSING 0
#define D3DUSAGE_DYNAMIC 0
// TODOX360: eliminate states which do not exists
#define D3DRS_FOGENABLE ((D3DRENDERSTATETYPE)-1)
#define D3DRS_SRGBWRITEENABLE ((D3DRENDERSTATETYPE)-2)
#define D3DRS_FOGTABLEMODE ((D3DRENDERSTATETYPE)-3)
#define D3DRS_FOGCOLOR ((D3DRENDERSTATETYPE)-4)
#define D3DSAMP_SRGBTEXTURE ((D3DSAMPLERSTATETYPE)-1)
//#define D3DRS_FOGENABLE D3DRS_FORCE_DWORD

#endif

// on PC we want to compile shaders with a debug information
// there is a different mechanism for this on Xbox (updb), therefore we do not need it there
/*
debug level 0 - no debug info, full optimization
debug level 1 - debug info, full optimization
debug level 2 - debug info + no optimization
*/

#if (_DEBUG || _PROFILE)
  #if _DEBUG
    #define SHADER_DBG_LEVEL 1
  #else
    #define SHADER_DBG_LEVEL 1
  #endif
#else
  #define SHADER_DBG_LEVEL 0
#endif


#endif