#include "../wpch.hpp"

#if !_DISABLE_GUI || defined _XBOX
#include "engddXB.hpp"

#include <El/common/perfLog.hpp>
#include <El/QStream/qbstream.hpp>
#include <Es/Strings/bstring.hpp>

#include "txtD3DXB.hpp"
#include "../Shape/poly.hpp"
#include "../tlVertex.hpp"
#include "../txtPreload.hpp"
#include "../Shape/shape.hpp"
#include "../Shape/material.hpp"
#include "../clip2D.hpp"
#include "../global.hpp"
#include "../diagModes.hpp"
#include <El/Common/perfProf.hpp>
#include <Es/Files/filenames.hpp>

// TODO: avoid scene here
#include "../scene.hpp"
#include "../landscape.hpp"
#include "../lights.hpp"
#include "../camera.hpp"
#include "../progress.hpp"
#include "../packFiles.hpp"


#ifdef _XBOX
#include "shaders/FPShadersNonTL.h"
#include "shaders/FPShadersNonTLTex4.h"
#include "shaders/FPShadersStencilVisualisation.h"
#include "shaders/FPShadersWater.h"
#include "shaders/FPShadersWaterSimple.h"
#include "shaders/FPShaders.h"
#include "shaders/FPShadersNormal.h"
#include "shaders/FPShadersNormalDiffuse.h"
#include "shaders/FPShadersNormalThrough.h"
#include "shaders/FPShadersGetPosNorm.h"
#include "shaders/FPShadersGetInst.h"
#include "shaders/FPShadersShadowVolume.h"
#include "shaders/FPShadersPoint.h"
#include "shaders/FPShadersInstancing.h"
#include "shaders/FPShadersInstancingEmissive.h"
#include "shaders/FPShadersInstancingModulate.h"
#include "shaders/FPShadersNormalInstancing.h"
#include "shaders/FPShadersNormalDiffuseInstancing.h"
#include "shaders/FPShadersNormalThroughInstancing.h"
#include "shaders/FPShadersShadowVolumeInstancing.h"
#include "shaders/FPShadersSpriteInstancing.h"
#include "shaders/FPShadersSkinning.h"
#include "shaders/FPShadersNormalSkinning.h"
#include "shaders/FPShadersNormalDiffuseSkinning.h"
#include "shaders/FPShadersNormalThroughSkinning.h"
#include "shaders/FPShadersShadowVolumeSkinning.h"
#include "shaders/FPShadersDirectional.h"
#include "shaders/FPShadersDirectionalADForced.h"
#include "shaders/FPShadersDirectionalNoSpec.h"
#include "shaders/FPShadersDirectionalSky.h"
#include "shaders/FPShadersDirectionalStars.h"
#include "shaders/FPShadersDirectionalSunObject.h"
#include "shaders/FPShadersDirectionalSunHaloObject.h"
#include "shaders/FPShadersDirectionalMoonObject.h"
#include "shaders/FPShadersDirectionalMoonHaloObject.h"
#include "shaders/FPShadersAlphaShadow.h"
#include "shaders/FPShadersFog.h"
#include "shaders/FPShadersFogAlpha.h"
#include "shaders/FPShadersFogNone.h"
#include "shaders/FPShadersLandShadow.h"
#include "shaders/FPShadersModShadow.h"
#include "shaders/FPShadersModInstShadow.h"
#include "shaders/FPShadersPoint0.h"
#include "shaders/FPShadersPoint1.h"
#include "shaders/FPShadersPoint2.h"
#include "shaders/FPShadersPoint3.h"
//#include "shaders/FPShadersPoint4.h"
//#include "shaders/FPShadersPoint5.h"
//#include "shaders/FPShadersPoint6.h"
//#include "shaders/FPShadersPoint7.h"
#include "shaders/FPShadersSpot0.h"
#include "shaders/FPShadersSpot1.h"
#include "shaders/FPShadersSpot2.h"
#include "shaders/FPShadersSpot3.h"
//#include "shaders/FPShadersSpot4.h"
//#include "shaders/FPShadersSpot5.h"
//#include "shaders/FPShadersSpot6.h"
//#include "shaders/FPShadersSpot7.h"
#include "shaders/FPShadersWriteLights.h"
#include "shaders/FPShadersWriteLightsColorMod.h"
#include "shaders/FPShadersWriteLightsAlphaMod.h"
#endif
#include "shaders/FPShadersDecl.h"

#ifndef _XBOX


#include <winuser.h>
#include <Dxerr8.h>
#if _DEBUG
#define NoSysLockFlag D3DLOCK_NOSYSLOCK
#else
// No sys lock is usually slower
#define NoSysLockFlag 0
#endif
#define NoClipUsageFlag D3DUSAGE_DONOTCLIP
#define DiscardLockFlag D3DLOCK_DISCARD
#else
#define NoSysLockFlag 0
#define NoClipUsageFlag 0
#define DiscardLockFlag 0
#define NO_FOG 0

#if _ENABLE_PERFLOG && _ENABLE_PERFLIB
#include <XbDm.h>
#include <D3d8perf.h>
#include <Xperf.h>
#pragma comment(lib,"xperf")
#endif
//#include <XGraphics.h>
#endif

#define PERF_PROF 0
/// enable pushbuffer loading from a file
#define LOAD_PB_FROM_DISK 0

#include <El/Common/perfProf.hpp>

#if PERF_PROF
#define PROFILE_DX_SCOPE(name) PROFILE_SCOPE(d3d)
//#define PROFILE_DX_SCOPE(name) PROFILE_SCOPE(name)
#else
#define PROFILE_DX_SCOPE(name)
#endif

#ifndef _XBOX
#define DX_CHECK(hr) \
  if (hr!=D3D_OK) \
    { \
    DXTRACE_ERR_NOMSGBOX("Check",hr); \
    Assert(false); \
    }

#define DX_CHECKN(n,hr) \
  if (hr!=D3D_OK) \
    { \
    DXTRACE_ERR_NOMSGBOX(n,hr); \
    Assert(false); \
    }
#else
#define DX_CHECK(hr) \
  if (hr!=D3D_OK) \
    { \
    Assert(false); \
    }

#define DX_CHECKN(n,hr) \
  if (hr!=D3D_OK) \
    { \
    Assert(false); \
    }
#endif

#if _DEBUG
#define WRITEONLYVB D3DUSAGE_WRITEONLY
#define WRITEONLYIB 0
#else
#define WRITEONLYVB D3DUSAGE_WRITEONLY
#define WRITEONLYIB D3DUSAGE_WRITEONLY
#endif

#define FAST_D3DRGBA(r, g, b, a) \
  (((toInt((a) * 255)) << 24) | ((toInt((r) * 255)) << 16) | \
  ((toInt((g) * 255)) << 8) | toInt((b) * 255)) 

#define FAST_D3DRGBA_SAT(r, g, b, a) \
  (min(toInt((a) * 255),255) << 24) | \
  (min(toInt((r) * 255),255) << 16) | \
  (min(toInt((g) * 255),255) << 8) | \
  (min(toInt((b) * 255),255))

#define FAST_D3DRGB(r, g, b) \
  (0xff000000 | ((toInt((r) * 255)) << 16) | \
  ((toInt((g) * 255)) << 8) | toInt((b) * 255)) 

#define FAST_D3DA(a) \
  ( (toInt((a) * 255)) << 24 ) 

#define FAST_D3DRGB565(r, g, b) \
  (0xff000000 | (((toInt((r) * 255)) >> 3) << (16 + 3)) | \
  ((toInt((g) * 255) >> 2) << (8 + 2)) | (toInt((b) * 255)) >> 3) << 3

#define CLIP_TO_565(c) (c&0xFFF8FCF8)

#define LOG_LOCKS 0

// Number of stencil plates
#define STENCILPLATE_NUMBER 8

//#define MYFVF (D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX2)
static const DWORD SVertexVSNonTLDecl[] =
{
  D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0, D3DVSDT_FLOAT4),
    D3DVSD_REG( 3, D3DVSDT_D3DCOLOR),
    D3DVSD_REG( 4, D3DVSDT_D3DCOLOR),
    D3DVSD_REG( 9, D3DVSDT_FLOAT2),
    D3DVSD_REG(10, D3DVSDT_FLOAT2),
    D3DVSD_END()
};

static const DWORD SVertexVSNonTL4TexDecl[] =
{
  D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0, D3DVSDT_FLOAT4),
    D3DVSD_REG( 3, D3DVSDT_D3DCOLOR),
    D3DVSD_REG( 4, D3DVSDT_D3DCOLOR),
    D3DVSD_REG( 9, D3DVSDT_FLOAT2),
    D3DVSD_REG(10, D3DVSDT_FLOAT2),
    D3DVSD_REG(11, D3DVSDT_FLOAT2),
    D3DVSD_REG(12, D3DVSDT_FLOAT2),
    D3DVSD_END()
};

static const DWORD SVertexVSTransformedDecl[] =
{
  D3DVSD_STREAM(0),
    D3DVSD_REG(0, D3DVSDT_FLOAT4),
    D3DVSD_END()
};


#ifdef _XBOX
/// max. vertex count for dynamic (CPU animated) objects
const int MaxShapeVertices = 2*1024;
/// size of vertex buffer allocation coalescing
#if CUSTOM_VB_ALLOCATOR
  /** zero disabled coalescing - we assume underlying allocator will handle it well */
  const int MaxStaticVertexBufferSizeB = 0;
#else
  const int MaxStaticVertexBufferSizeB = 16*1024-64;
#endif
#else
const int MaxShapeVertices = 32*1024;
const int MaxStaticVertexBufferSizeB = 128*1024;
#endif

//! Size of one vertex buffer item (in vertices)

// source vertex buffer format
//#define MYSFVF (D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1)

static const DWORD SVertexVSDeclBasic[] =
{
  D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0/*D3DVSDE_POSITION*/,  D3DVSDT_SHORT3 ),
    D3DVSD_REG( 3/*D3DVSDE_NORMAL*/,    D3DVSDT_NORMPACKED3 ),
    D3DVSD_REG( 7/*D3DVSDE_TEXCOORD0*/, D3DVSDT_SHORT2 ),
    D3DVSD_END()
};
static const DWORD SVertexVSDeclNormal[] =
{
  D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0/*D3DVSDE_POSITION*/,  D3DVSDT_SHORT3 ),
    D3DVSD_REG( 3/*D3DVSDE_NORMAL*/,    D3DVSDT_NORMPACKED3 ),
    D3DVSD_REG( 7/*D3DVSDE_TEXCOORD0*/, D3DVSDT_SHORT2 ),
    D3DVSD_REG( 10/*D3DVSDE_NORMAL*/,    D3DVSDT_NORMPACKED3 ),
    D3DVSD_REG( 11/*D3DVSDE_NORMAL*/,    D3DVSDT_NORMPACKED3 ),
    D3DVSD_END()
};
static const DWORD SVertexVSDeclNormalAlpha[] =
{
  D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0/*D3DVSDE_POSITION*/,  D3DVSDT_SHORT3 ),
    D3DVSD_REG( 3/*D3DVSDE_NORMAL*/,    D3DVSDT_NORMPACKED3 ),
    D3DVSD_REG( 7/*D3DVSDE_TEXCOORD0*/, D3DVSDT_SHORT2 ),
    D3DVSD_REG( 10/*D3DVSDE_NORMAL*/,    D3DVSDT_NORMPACKED3 ),
    D3DVSD_REG( 11/*D3DVSDE_NORMAL*/,    D3DVSDT_NORMPACKED3 ),
    D3DVSD_REG( 1/*alpha*/,              D3DVSDT_PBYTE1 ),
    D3DVSD_END()
};
static const DWORD SVertexVSDeclInstanced[] =
{
  D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0/*Position*/,              D3DVSDT_SHORT3 ),
    D3DVSD_REG( 3/*Normal vector*/,         D3DVSDT_NORMPACKED3 ),
    D3DVSD_REG( 7/*Texture coordinates*/,   D3DVSDT_SHORT2 ),
    D3DVSD_REG( 9/*Index of the instance*/, D3DVSDT_PBYTE1 ),
    D3DVSD_END()
};
static const DWORD SVertexVSDeclNormalInstanced[] =
{
  D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0 /*Position*/,               D3DVSDT_SHORT3 ),
    D3DVSD_REG( 3 /*Normal vector*/,          D3DVSDT_NORMPACKED3 ),
    D3DVSD_REG( 7 /*Texture coordinates*/,    D3DVSDT_SHORT2 ),
    D3DVSD_REG( 10/*S vector*/,               D3DVSDT_NORMPACKED3 ),
    D3DVSD_REG( 11/*T vector*/,               D3DVSDT_NORMPACKED3 ),
    D3DVSD_REG( 9 /*Index of the instance*/,  D3DVSDT_PBYTE1 ),
    D3DVSD_END()
};
static const DWORD SVertexVSDeclSpriteInstanced[] =
{
  D3DVSD_STREAM(0),
    D3DVSD_REG( 7 /*Texture coordinates*/,    D3DVSDT_PBYTE2 ),
    D3DVSD_REG( 9 /*Index of the instance*/,  D3DVSDT_PBYTE1 ),
    D3DVSD_END()
};
static const DWORD SVertexVSDeclSkinning[] =
{
  D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0/*D3DVSDE_POSITION*/,  D3DVSDT_SHORT3 ),
    D3DVSD_REG( 3/*D3DVSDE_NORMAL*/,    D3DVSDT_NORMPACKED3 ),
    D3DVSD_REG( 7/*D3DVSDE_TEXCOORD0*/, D3DVSDT_SHORT2 ),
    D3DVSD_REG( 8/*D3DVSDE_TEXCOORD0*/, D3DVSDT_D3DCOLOR ),
    D3DVSD_REG( 9/*D3DVSDE_TEXCOORD0*/, D3DVSDT_D3DCOLOR ),
    D3DVSD_END()
};
static const DWORD SVertexVSDeclNormalSkinning[] =
{
  D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0/*D3DVSDE_POSITION*/,  D3DVSDT_SHORT3 ),
    D3DVSD_REG( 3/*D3DVSDE_NORMAL*/,    D3DVSDT_NORMPACKED3 ),
    D3DVSD_REG( 7/*D3DVSDE_TEXCOORD0*/, D3DVSDT_SHORT2 ),
    D3DVSD_REG( 10/*D3DVSDE_NORMAL*/,   D3DVSDT_NORMPACKED3 ),
    D3DVSD_REG( 11/*D3DVSDE_NORMAL*/,   D3DVSDT_NORMPACKED3 ),
    D3DVSD_REG( 8/*D3DVSDE_TEXCOORD0*/, D3DVSDT_D3DCOLOR ),
    D3DVSD_REG( 9/*D3DVSDE_TEXCOORD0*/, D3DVSDT_D3DCOLOR ),
    D3DVSD_END()
};
static const DWORD SVertexVSDeclShadowVolume[] =
{
  D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0/*D3DVSDE_POSITION*/,  D3DVSDT_SHORT3 ),
    D3DVSD_REG( 3/*D3DVSDE_NORMAL*/,    D3DVSDT_NORMPACKED3 ),
    D3DVSD_END()
};
static const DWORD SVertexVSDeclShadowVolumeInstanced[] =
{
  D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0/*D3DVSDE_POSITION*/,      D3DVSDT_SHORT3 ),
    D3DVSD_REG( 3/*D3DVSDE_NORMAL*/,        D3DVSDT_NORMPACKED3 ),
    D3DVSD_REG( 9/*Index of the instance*/, D3DVSDT_PBYTE1 ),
    D3DVSD_END()
};
static const DWORD SVertexVSDeclShadowVolumeSkinning[] =
{
  D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0/*D3DVSDE_POSITION*/,  D3DVSDT_SHORT3 ),
    D3DVSD_REG( 8/*D3DVSDE_TEXCOORD0*/, D3DVSDT_D3DCOLOR ),
    D3DVSD_REG( 9/*D3DVSDE_TEXCOORD0*/, D3DVSDT_D3DCOLOR ),
    D3DVSD_REG( 1/*D3DVSDE_POSITION*/,  D3DVSDT_SHORT3 ),
    D3DVSD_REG( 2/*D3DVSDE_TEXCOORD0*/, D3DVSDT_D3DCOLOR ),
    D3DVSD_REG( 3/*D3DVSDE_TEXCOORD0*/, D3DVSDT_D3DCOLOR ),
    D3DVSD_REG( 4/*D3DVSDE_POSITION*/,  D3DVSDT_SHORT3 ),
    D3DVSD_REG( 5/*D3DVSDE_TEXCOORD0*/, D3DVSDT_D3DCOLOR ),
    D3DVSD_REG( 6/*D3DVSDE_TEXCOORD0*/, D3DVSDT_D3DCOLOR ),
    D3DVSD_END()
};
static const DWORD SVertexVSDeclPoint[] =
{
  D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0/*Position*/,            D3DVSDT_NORMPACKED3 ),
    D3DVSD_REG( 7/*Texture coordinates*/, D3DVSDT_PBYTE2 ),
    D3DVSD_REG( 8/*D3DVSDE_TEXCOORD0*/,   D3DVSDT_D3DCOLOR ),
    D3DVSD_END()
};

//static const D3DVERTEXATTRIBUTEFORMAT VAFXXX =
//{
//  {
//    /* 0*/{0, 0, D3DVSDT_NONE, 0, 0},
//    /* 1*/{0, 0, D3DVSDT_NONE, 0, 0},
//    /* 2*/{0, 0, D3DVSDT_NONE, 0, 0},
//    /* 3*/{0, 0, D3DVSDT_NONE, 0, 0},
//    /* 4*/{0, 0, D3DVSDT_NONE, 0, 0},
//    /* 5*/{0, 0, D3DVSDT_NONE, 0, 0},
//    /* 6*/{0, 0, D3DVSDT_NONE, 0, 0},
//    /* 7*/{0, 0, D3DVSDT_NONE, 0, 0},
//    /* 8*/{0, 0, D3DVSDT_NONE, 0, 0},
//    /* 9*/{0, 0, D3DVSDT_NONE, 0, 0},
//    /*10*/{0, 0, D3DVSDT_NONE, 0, 0},
//    /*11*/{0, 0, D3DVSDT_NONE, 0, 0},
//    /*12*/{0, 0, D3DVSDT_NONE, 0, 0},
//    /*13*/{0, 0, D3DVSDT_NONE, 0, 0},
//    /*14*/{0, 0, D3DVSDT_NONE, 0, 0},
//    /*15*/{0, 0, D3DVSDT_NONE, 0, 0},
//  }
//};

static D3DVERTEXATTRIBUTEFORMAT VAFTL =
{
  {
    /* 0*/{0, 0, D3DVSDT_FLOAT4, 0, 0},
    /* 1*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 2*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 3*/{0, sizeof(Vector3) + sizeof(float), D3DVSDT_D3DCOLOR, 0, 0},
    /* 4*/{0, sizeof(Vector3) + sizeof(float) + sizeof(PackedColor), D3DVSDT_D3DCOLOR, 0, 0},
    /* 5*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 6*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 7*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 8*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 9*/{0, sizeof(Vector3) + sizeof(float) + sizeof(PackedColor) + sizeof(PackedColor), D3DVSDT_FLOAT2, 0, 0},
    /*10*/{0, sizeof(Vector3) + sizeof(float) + sizeof(PackedColor) + sizeof(PackedColor) + sizeof(UVPair), D3DVSDT_FLOAT2, 0, 0},
    /*11*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*12*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*13*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*14*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*15*/{0, 0, D3DVSDT_NONE, 0, 0},
  }
};

static D3DVERTEXATTRIBUTEFORMAT VAFPoint =
{
  {
    /* 0*/{0, 0, D3DVSDT_NORMPACKED3, 0, 0},
    /* 1*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 2*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 3*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 4*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 5*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 6*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 7*/{0, sizeof(DWORD), D3DVSDT_PBYTE2, 0, 0},
    /* 8*/{0, sizeof(DWORD) + sizeof(BYTE) * 2, D3DVSDT_D3DCOLOR, 0, 0},
    /* 9*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*10*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*11*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*12*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*13*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*14*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*15*/{0, 0, D3DVSDT_NONE, 0, 0},
  }
};

static D3DVERTEXATTRIBUTEFORMAT VAFBasic =
{
  {
    /* 0*/{0, 0, D3DVSDT_SHORT3, 0, 0},
    /* 1*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 2*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 3*/{0, sizeof(Vector3Compressed), D3DVSDT_NORMPACKED3, 0, 0},
    /* 4*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 5*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 6*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 7*/{0, sizeof(Vector3Compressed) + sizeof(DWORD), D3DVSDT_SHORT2, 0, 0},
    /* 8*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 9*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*10*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*11*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*12*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*13*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*14*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*15*/{0, 0, D3DVSDT_NONE, 0, 0},
  }
};

static D3DVERTEXATTRIBUTEFORMAT VAFBasicSkinned =
{
  {
    /* 0*/{0, 0, D3DVSDT_SHORT3, 0, 0},
    /* 1*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 2*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 3*/{0, sizeof(Vector3Compressed), D3DVSDT_NORMPACKED3, 0, 0},
    /* 4*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 5*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 6*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 7*/{0, sizeof(Vector3Compressed) + sizeof(DWORD), D3DVSDT_SHORT2, 0, 0},
    /* 8*/{0, sizeof(Vector3Compressed) + sizeof(DWORD) + sizeof(UVPairCompressed), D3DVSDT_D3DCOLOR, 0, 0},
    /* 9*/{0, sizeof(Vector3Compressed) + sizeof(DWORD) + sizeof(UVPairCompressed) + sizeof(BYTE)*4, D3DVSDT_D3DCOLOR, 0, 0},
    /*10*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*11*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*12*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*13*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*14*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*15*/{0, 0, D3DVSDT_NONE, 0, 0},
  }
};

static D3DVERTEXATTRIBUTEFORMAT VAFBasicInstanced =
{
  {
    /* 0*/{0, 0, D3DVSDT_SHORT3, 0, 0},
    /* 1*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 2*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 3*/{0, sizeof(Vector3Compressed), D3DVSDT_NORMPACKED3, 0, 0},
    /* 4*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 5*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 6*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 7*/{0, sizeof(Vector3Compressed) + sizeof(DWORD), D3DVSDT_SHORT2, 0, 0},
    /* 8*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 9*/{0, sizeof(Vector3Compressed) + sizeof(DWORD) + sizeof(UVPairCompressed), D3DVSDT_PBYTE1, 0, 0},
    /*10*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*11*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*12*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*13*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*14*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*15*/{0, 0, D3DVSDT_NONE, 0, 0},
  }
};

static D3DVERTEXATTRIBUTEFORMAT VAFNormal =
{
  {
    /* 0*/{0, 0, D3DVSDT_SHORT3, 0, 0},
    /* 1*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 2*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 3*/{0, sizeof(Vector3Compressed), D3DVSDT_NORMPACKED3, 0, 0},
    /* 4*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 5*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 6*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 7*/{0, sizeof(Vector3Compressed) + sizeof(DWORD), D3DVSDT_SHORT2, 0, 0},
    /* 8*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 9*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*10*/{0, sizeof(Vector3Compressed) + sizeof(DWORD) + sizeof(UVPairCompressed), D3DVSDT_NORMPACKED3, 0, 0},
    /*11*/{0, sizeof(Vector3Compressed) + sizeof(DWORD) + sizeof(UVPairCompressed) + sizeof(DWORD), D3DVSDT_NORMPACKED3, 0, 0},
    /*12*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*13*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*14*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*15*/{0, 0, D3DVSDT_NONE, 0, 0},
  }
};

static D3DVERTEXATTRIBUTEFORMAT VAFNormalSkinned =
{
  {
    /* 0*/{0, 0, D3DVSDT_SHORT3, 0, 0},
    /* 1*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 2*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 3*/{0, sizeof(Vector3Compressed), D3DVSDT_NORMPACKED3, 0, 0},
    /* 4*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 5*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 6*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 7*/{0, sizeof(Vector3Compressed) + sizeof(DWORD), D3DVSDT_SHORT2, 0, 0},
    /* 8*/{0, sizeof(Vector3Compressed) + sizeof(DWORD) + sizeof(UVPairCompressed) + sizeof(DWORD) + sizeof(DWORD), D3DVSDT_D3DCOLOR, 0, 0},
    /* 9*/{0, sizeof(Vector3Compressed) + sizeof(DWORD) + sizeof(UVPairCompressed) + sizeof(DWORD) + sizeof(DWORD) + sizeof(BYTE)*4, D3DVSDT_D3DCOLOR, 0, 0},
    /*10*/{0, sizeof(Vector3Compressed) + sizeof(DWORD) + sizeof(UVPairCompressed), D3DVSDT_NORMPACKED3, 0, 0},
    /*11*/{0, sizeof(Vector3Compressed) + sizeof(DWORD) + sizeof(UVPairCompressed) + sizeof(DWORD), D3DVSDT_NORMPACKED3, 0, 0},
    /*12*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*13*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*14*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*15*/{0, 0, D3DVSDT_NONE, 0, 0},
  }
};

static D3DVERTEXATTRIBUTEFORMAT VAFNormalInstanced =
{
  {
    /* 0*/{0, 0, D3DVSDT_SHORT3, 0, 0},
    /* 1*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 2*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 3*/{0, sizeof(Vector3Compressed), D3DVSDT_NORMPACKED3, 0, 0},
    /* 4*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 5*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 6*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 7*/{0, sizeof(Vector3Compressed) + sizeof(DWORD), D3DVSDT_SHORT2, 0, 0},
    /* 8*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 9*/{0, sizeof(Vector3Compressed) + sizeof(DWORD) + sizeof(UVPairCompressed) + sizeof(DWORD) + sizeof(DWORD), D3DVSDT_PBYTE1, 0, 0},
    /*10*/{0, sizeof(Vector3Compressed) + sizeof(DWORD) + sizeof(UVPairCompressed), D3DVSDT_NORMPACKED3, 0, 0},
    /*11*/{0, sizeof(Vector3Compressed) + sizeof(DWORD) + sizeof(UVPairCompressed) + sizeof(DWORD), D3DVSDT_NORMPACKED3, 0, 0},
    /*12*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*13*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*14*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*15*/{0, 0, D3DVSDT_NONE, 0, 0},
  }
};

static D3DVERTEXATTRIBUTEFORMAT VAFSpriteInstanced =
{
  {
    /* 0*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 1*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 2*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 3*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 4*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 5*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 6*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 7*/{0, 0, D3DVSDT_PBYTE2, 0, 0},
    /* 8*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 9*/{0, sizeof(BYTE) * 2, D3DVSDT_PBYTE1, 0, 0},
    /*10*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*11*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*12*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*13*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*14*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*15*/{0, 0, D3DVSDT_NONE, 0, 0},
  }
};

static D3DVERTEXATTRIBUTEFORMAT VAFNormalAlpha =
{
  {
    /* 0*/{0, 0, D3DVSDT_SHORT3, 0, 0},
    /* 1*/{0, sizeof(Vector3Compressed) + sizeof(DWORD) + sizeof(UVPairCompressed) + sizeof(DWORD) + sizeof(DWORD), D3DVSDT_PBYTE1, 0, 0},
    /* 2*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 3*/{0, sizeof(Vector3Compressed), D3DVSDT_NORMPACKED3, 0, 0},
    /* 4*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 5*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 6*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 7*/{0, sizeof(Vector3Compressed) + sizeof(DWORD), D3DVSDT_SHORT2, 0, 0},
    /* 8*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 9*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*10*/{0, sizeof(Vector3Compressed) + sizeof(DWORD) + sizeof(UVPairCompressed), D3DVSDT_NORMPACKED3, 0, 0},
    /*11*/{0, sizeof(Vector3Compressed) + sizeof(DWORD) + sizeof(UVPairCompressed) + sizeof(DWORD), D3DVSDT_NORMPACKED3, 0, 0},
    /*12*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*13*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*14*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*15*/{0, 0, D3DVSDT_NONE, 0, 0},
  }
};

static D3DVERTEXATTRIBUTEFORMAT VAFShadowVolume =
{
  {
    /* 0*/{0, 0, D3DVSDT_SHORT3, 0, 0},
    /* 1*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 2*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 3*/{0, sizeof(Vector3Compressed), D3DVSDT_NORMPACKED3, 0, 0},
    /* 4*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 5*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 6*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 7*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 8*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 9*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*10*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*11*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*12*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*13*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*14*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*15*/{0, 0, D3DVSDT_NONE, 0, 0},
  }
};

static D3DVERTEXATTRIBUTEFORMAT VAFShadowVolumeInstanced =
{
  {
    /* 0*/{0, 0, D3DVSDT_SHORT3, 0, 0},
    /* 1*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 2*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 3*/{0, sizeof(Vector3Compressed), D3DVSDT_NORMPACKED3, 0, 0},
    /* 4*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 5*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 6*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 7*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 8*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 9*/{0, sizeof(Vector3Compressed) + sizeof(DWORD), D3DVSDT_PBYTE1, 0, 0},
    /*10*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*11*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*12*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*13*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*14*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*15*/{0, 0, D3DVSDT_NONE, 0, 0},
  }
};

static D3DVERTEXATTRIBUTEFORMAT VAFShadowVolumeSkinning =
{
  {
    /* 0*/{0, 0, D3DVSDT_SHORT3, 0, 0},
    /* 1*/{0, sizeof(Vector3Compressed) + sizeof(BYTE) * 4 * 2, D3DVSDT_SHORT3, 0, 0},
    /* 2*/{0, sizeof(Vector3Compressed) * 2 + sizeof(BYTE) * 4 * 2, D3DVSDT_D3DCOLOR, 0, 0},
    /* 3*/{0, sizeof(Vector3Compressed) * 2 + sizeof(BYTE) * 4 * 3, D3DVSDT_D3DCOLOR, 0, 0},
    /* 4*/{0, sizeof(Vector3Compressed) * 2 + sizeof(BYTE) * 4 * 4, D3DVSDT_SHORT3, 0, 0},
    /* 5*/{0, sizeof(Vector3Compressed) * 3 + sizeof(BYTE) * 4 * 4, D3DVSDT_D3DCOLOR, 0, 0},
    /* 6*/{0, sizeof(Vector3Compressed) * 3 + sizeof(BYTE) * 4 * 5, D3DVSDT_D3DCOLOR, 0, 0},
    /* 7*/{0, 0, D3DVSDT_NONE, 0, 0},
    /* 8*/{0, sizeof(Vector3Compressed), D3DVSDT_D3DCOLOR, 0, 0},
    /* 9*/{0, sizeof(Vector3Compressed) + sizeof(BYTE) * 4, D3DVSDT_D3DCOLOR, 0, 0},
    /*10*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*11*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*12*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*13*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*14*/{0, 0, D3DVSDT_NONE, 0, 0},
    /*15*/{0, 0, D3DVSDT_NONE, 0, 0},
  }
};

/** Indexed by [EVertexDecl value][basic,skinning,instancing] */
static const DWORD *VertexVSDecl[][VSDVCount] =
{
  //  Basic                       Skinning                            Instancing
  {   NULL,                       NULL,                               SVertexVSDeclSpriteInstanced},
  {   SVertexVSDeclPoint,         NULL,                               NULL},
  {   SVertexVSDeclBasic,         SVertexVSDeclSkinning,              SVertexVSDeclInstanced},
  {   SVertexVSDeclNormal,        SVertexVSDeclNormalSkinning,        SVertexVSDeclNormalInstanced},
  {   SVertexVSDeclBasic,         SVertexVSDeclSkinning,              SVertexVSDeclInstanced},
  {   SVertexVSDeclNormal,        SVertexVSDeclNormalSkinning,        SVertexVSDeclNormalInstanced},
  {   SVertexVSDeclNormalAlpha,   NULL,                               NULL},
  {   NULL,                       NULL,                               NULL},
  {   NULL,                       NULL,                               NULL},
  {   NULL,                       NULL,                               NULL},
  {   NULL,                       NULL,                               NULL},
  {   SVertexVSDeclShadowVolume,  SVertexVSDeclShadowVolumeSkinning,  SVertexVSDeclShadowVolumeInstanced},
  {   NULL,                       NULL,                               NULL},
};
COMPILETIME_COMPARE(lenof(VertexVSDecl), VDCount)

/** Declaration for SetVertexShaderInput */
static D3DVERTEXATTRIBUTEFORMAT *VAFVSDecl[][VSDVCount] =
{
  //  Basic                       Skinning                            Instancing
  {   NULL,                       NULL,                               &VAFSpriteInstanced},
  {   &VAFPoint,                  NULL,                               NULL},
  {   &VAFBasic,                  &VAFBasicSkinned,                   &VAFBasicInstanced},
  {   &VAFNormal,                 &VAFNormalSkinned,                  &VAFNormalInstanced},
  {   &VAFBasic,                  &VAFBasicSkinned,                   &VAFBasicInstanced},
  {   &VAFNormal,                 &VAFNormalSkinned,                  &VAFNormalInstanced},
  {   &VAFNormalAlpha,            NULL,                               NULL},
  {   NULL,                       NULL,                               NULL},
  {   NULL,                       NULL,                               NULL},
  {   NULL,                       NULL,                               NULL},
  {   NULL,                       NULL,                               NULL},
  {   &VAFShadowVolume,           &VAFShadowVolumeSkinning,           &VAFShadowVolumeInstanced},
  {   NULL,                       NULL,                               NULL},
};

namespace Unique1
{
  COMPILETIME_COMPARE(lenof(VAFVSDecl), VDCount)
};

#if 0
#define ValidateCoord(c)
#else
#define LOG_INVALID 0
inline void ValidateCoord( float &c, float max )
{
#if LOG_INVALID
  static int total=0;
  static int valid=0;
  total++;
#endif
#if 0
  const float minC=-2047,maxC=+2048;
  // coordinate must be within range -2047...2048
  if( c<minC ) c=minC;
  else if( c>maxC ) c=maxC;
#else
  if( c<0 ) c=0;
  else if( c>max ) c=max;
#endif

#if LOG_INVALID
  else valid++;
  if( total>=100000 )
  {
    if( valid<total ) LogF("Invalid %d of %d",total-valid,total);
    valid=0;
    total=0;
  }
#endif
}
#endif

#if _ENABLE_PERFLOG && _ENABLE_CHEATS && !defined _XBOX
#define DO_TEX_STATS 1
#endif

#if _ENABLE_PERFLOG 
#include "../keyInput.hpp"
#include "../dikCodes.h"
#define LOG_STATE_LEVEL 1
extern bool LogStatesOnce;
#else
#define LOG_STATE_LEVEL 0
#endif

#if DO_TEX_STATS
#include <El/Statistics/statistics.hpp>
#include "../keyInput.hpp"
#include "../dikCodes.h"
static StatisticsByName TexStats;
static bool EnableTexStats = false;
#endif

#if PIXEL_SHADERS
#include "shaders/psDecl.h"
#endif

extern int D3DAdapter;

#ifdef STRIPIZATION_ENABLED
#if defined _XBOX && _ENABLE_CHEATS
// Flag to determine wheter strips are enabled or not
bool EnableStrips = true;
#else
const bool EnableStrips = true;
#endif
#endif

//StatisticsByName vbStats;

static void ConvertMatrix( D3DMATRIX &mat, Matrix4Val src )
{
  mat._11 = src.DirectionAside()[0];
  mat._12 = src.DirectionAside()[1];
  mat._13 = src.DirectionAside()[2];

  mat._21 = src.DirectionUp()[0];
  mat._22 = src.DirectionUp()[1];
  mat._23 = src.DirectionUp()[2];

  mat._31 = src.Direction()[0];
  mat._32 = src.Direction()[1];
  mat._33 = src.Direction()[2];

  mat._41 = src.Position()[0];
  mat._42 = src.Position()[1];
  mat._43 = src.Position()[2];

  mat._14 = 0;
  mat._24 = 0;
  mat._34 = 0;
  mat._44 = 1;
}

static void ConvertMatrixTransposed( D3DMATRIX &mat, Matrix4Val src )
{
  mat._11 = src.DirectionAside()[0];
  mat._21 = src.DirectionAside()[1];
  mat._31 = src.DirectionAside()[2];
  mat._41 = 0;

  mat._12 = src.DirectionUp()[0];
  mat._22 = src.DirectionUp()[1];
  mat._32 = src.DirectionUp()[2];
  mat._42 = 0;

  mat._13 = src.Direction()[0];
  mat._23 = src.Direction()[1];
  mat._33 = src.Direction()[2];
  mat._43 = 0;

  mat._14 = src.Position()[0];
  mat._24 = src.Position()[1];
  mat._34 = src.Position()[2];
  mat._44 = 1;
}

static void ConvertMatrixTransposed3( FLOAT *mat, Matrix4Val src )
{
  // TODO: consider using SIMD to perform swizzling
  /* XDK code: XGMatrixTranspose transpose 4x4 SIMD matrix 

  mov         ecx,dword ptr [esp+8] 
  mov         eax,dword ptr [esp+4] 
  movaps      xmm0,xmmword ptr [ecx] 
  movaps      xmm2,xmmword ptr [ecx+10h] 
  movaps      xmm3,xmmword ptr [ecx+20h] 
  movaps      xmm5,xmmword ptr [ecx+30h] 
  movaps      xmm1,xmm0 
  movaps      xmm4,xmm3 
  unpcklps    xmm0,xmm2 
  unpckhps    xmm1,xmm2 
  unpcklps    xmm3,xmm5 
  unpckhps    xmm4,xmm5 
  movlps      qword ptr [eax],xmm0 
  movlps      qword ptr [eax+8],xmm3 
  movhps      qword ptr [eax+10h],xmm0 
  movhps      qword ptr [eax+18h],xmm3 
  movlps      qword ptr [eax+20h],xmm1 
  movlps      qword ptr [eax+28h],xmm4 
  movhps      qword ptr [eax+30h],xmm1 
  movhps      qword ptr [eax+38h],xmm4 
  */

  mat[0] = src.DirectionAside()[0];
  mat[1] = src.DirectionUp()[0];
  mat[2] = src.Direction()[0];
  mat[3] = src.Position()[0];

  mat[4] = src.DirectionAside()[1];
  mat[5] = src.DirectionUp()[1];
  mat[6] = src.Direction()[1];
  mat[7] = src.Position()[1];

  mat[8] = src.DirectionAside()[2];
  mat[9] = src.DirectionUp()[2];
  mat[10] = src.Direction()[2];
  mat[11] = src.Position()[2];
}


void EngineDDXB::GetZCoefs(float &zAdd, float &zMult)
{
  int zBias = _bias;
  /*
  if (_useDXTL)
  {
  zMult = 1.0f-zBias*1e-7f;
  zAdd = -zBias*1e-6f;
  }
  else
  */
  {
    zMult = 1.0f-zBias*1e-7f;
    zAdd = zBias*-2e-7f;
  }
}

static const float ZBiasEpsilon = 0.1f;

static void ConvertProjectionMatrix
(
 D3DMATRIX &mat, Matrix4Val src, float zBias
 )
{
  // note: D3D notation _11 is element (0,0) in C notation
  mat._11 = src(0,0);
  mat._12 = 0;
  mat._13 = 0;
  mat._14 = 0;

  mat._21 = 0;
  mat._22 = src(1,1);
  mat._23 = 0;
  mat._24 = 0;

  /*
  // note: D3D projection calculation
  w = 1/z
  z = src(2,2)+src.Pos(2)/z
  */

  float c = src(2,2);
  float d = src.Position()[2];
  if (fabs(zBias) > ZBiasEpsilon)
  {
    if (GEngine->GetTL() && GEngine->HasWBuffer() && GEngine->IsWBuffer())
    {
      // w-buffer adjustments based on near / far values
      // calculate near and far
      float n = -d/c;
      float f = d/(1-c);
      // adjust near and far depending on z-bias
      //float mult = 1.0f-zBias*1e-6f;
      float add = 0; //zBias*1e-6f;
      float mult = 1+zBias*1e-5f;
      //float add = 0;

      f *= mult;
      n *= mult;
      n += add;

      // set adjusted coefs
      c = f/(f-n);
      d = -c*n;
    }
    else
    {
      // values used for SW T&L
      //float zMult = 1.0f-zBias*1e-7f;
      //float zAdd = zBias*-2e-7f;
      float zMult = 1.0f-zBias*1e-7f;
      float zAdd = -zBias*1e-6f;

      c = src(2,2)*zMult+zAdd;
      d = src.Position()[2]*zMult;
    }
  }

  mat._31 = 0;
  mat._32 = 0;
  mat._33 = c;
  mat._34 = 1;

  mat._41 = 0;
  mat._42 = 0;
  mat._43 = d;
  mat._44 = 0;
  // source w is 1
  // result homogenous z: x*m13 + y*m23 + z*m33 + m43
  // result homogenous w:                 z
  // m13 and m23 is zero
  // result projected  z: m33 + m43/z

#if LOG_STATE_LEVEL
  if (LogStatesOnce)
  {
    LogF("Set z-bias %d",zBias);

    LogF("  now: z1 = %10g/z0 + %10g",mat._43,mat._33);
    LogF("  def: z1 = %10g/z0 + %10g",src.Position()[2],src.Direction()[2]);
  }
#endif

}


void LogTextureF( const char *t, const TextureD3DXB *x )
{
  LogF("Texture %s %s",t,x ? x->Name() : "--");
}

#if 0
#define LogTexture(t,x) LogTextureF(t,x)
#else
#define LogTexture(t,x)
#endif

bool EngineDDXB::CanZBias() const
{
  return false;
}

#if LOG_STATE_LEVEL
void LogSetProjXB(const D3DMATRIX &mat)
{
  if (LogStatesOnce)
  {
    LogF
      (
      "Set Projection %g,%g,%g,%g",
      mat._11,mat._22,mat._33,mat._43
      );
  }
  /*
  float c = mat._33;
  float d = mat._43;
  float n = -d/c;
  float f = d/(1-c);

  float Q = f/(f-n);
  float mQZn = -Q*n;
  LogF("Set c=%g (%g), d=%g (%g), n=%.2f, f=%.1f",c,Q,d,mQZn,n,f);
  */

}
#else

#define LogSetProjXB(mat)

#endif

void EngineDDXB::SetBias( int bias )
{
  // some devices support z-bias
  if (bias==_bias) return;
  _bias=bias;

  // Setup projection matrix
  ProjectionChanged();
}

void EngineDDXB::EnableReorderQueues( bool enableReorder )
{
  if (_enableReorder==enableReorder) return;
  _enableReorder = enableReorder;
  if (!_enableReorder)
  {
    FlushQueues();
  }
}
void EngineDDXB::FlushQueues()
{
  FlushAndFreeAllQueues(_queueNo);
  //FlushAndFreeAllQueues(_queueTL);
}

#ifdef _XBOX
#define NO_SET_TEX 0
#else
#define NO_SET_TEX 0
#endif


#define NO_SET_TSS 0

extern void ReleaseVBuffers();
extern void RestoreVBuffers();

void EngineDDXB::PreReset(bool hard)
{
  FreeAllQueues(_queueNo);

  _iBufferLast = NULL; // forget any VB
  _vBufferLast = NULL;

  SetTexture0Color(HWhite,HWhite,false);

  _hwInDefinedState = false;
  for (int i = 0; i < TEXTURE_STAGES; i++)
  {
    _d3DDevice->SetTexture( i,NULL );
    _lastHandle[i] = NULL;
    _lastTexture[i].Free();
  }

  // destroy all video memory surfaces
  _backBuffer.Free();
  _frontBuffer.Free();
  _postFxBuffer.Free();
  _brightBuffer.Free();

  //_zBuffer.Free();

  _textBank->ReleaseAllTextures(hard); // release all textures
  // release all vertex buffers
  ReleaseVBuffers();

#if PIXEL_SHADERS
  DeinitPixelShaders();
#endif

  DestroyVB();
  DestroyVBTL();

  Assert(_iBuffersAllocated==0);
  Assert(_vBuffersAllocated==0);

}

void EngineDDXB::PostReset()
{
  CreateVB();
  CreateVBTL();

  RestoreVBuffers();

  ClearStateCaches();

  Init3DState();
  // TODO: reset fog
}

bool EngineDDXB::Reset()
{
  bool inScene = _d3dFrameOpen;
  if (inScene) FinishDraw(1);

  PreReset(false);

  // Recreate all surfaces

  HRESULT ok = _d3DDevice->Reset(&_pp);
  if (ok!=D3D_OK)
  {
    LogF("Reset failed.");
    // wait for a while and try again
    Sleep(1000);
    HRESULT ok = _d3DDevice->Reset(&_pp);
    if (ok!=D3D_OK)
    {
      LogF("Reset failed after sleep");
      return false;
    }

  }

  PostReset();
  if (inScene) InitDraw();
  return true;
}

bool ParseUserParams(ParamFile &cfg);
void ParseFlashpointCfg(ParamFile &file);

bool EngineDDXB::ResetHard()
{
  bool inScene = _d3dFrameOpen;
  if (inScene) FinishDraw(1);
  PreReset(true);

  // destroy surfaces and device
  DestroySurfaces();

  Init3D();
  DoSetGamma();

  ParamFile cfg;
  ParseUserParams(cfg);
  ParamFile fcfg;
  ParseFlashpointCfg(fcfg);

  // direct-color mode required
  D3DConstruct(cfg,fcfg);

  PostReset();
  if (inScene) InitDraw();
  return true;
}

void EngineDDXB::SetWaterDepth(
  const WaterDepthQuad *water, int xRange, int zRange, float grid,
  float seaLevel,
  float shoreTop, float shoreBottom, float peakWaveTop, float peakWaveBottom
)
{
  // prepare constants for water "skinning"
  _skinningType = STNone; // this will reset any skinning currently set
  const int waterSegSize = 9;
  DoAssert(xRange==waterSegSize);
  DoAssert(zRange==waterSegSize);
  _d3DDevice->SetVertexShaderConstant(WATER_DEPTH,water,waterSegSize*waterSegSize);
  // add small eps. because we want to avoid truncating below 0
  // inv. grid size is less then 1/grid, because we want to avoid overflowing above seg. size
  //float gridVal[4]={0.9999f/grid,waterSegSize,1,(waterSegSize-1)*0.5f+0.0001f};
  float gridVal[4]={1.0f/grid,waterSegSize,waterSegSize-1,(waterSegSize-1)*0.5f};
  _d3DDevice->SetVertexShaderConstant(WATER_DEPTHGRID_GRID_1_GRIDd2,gridVal,1);
  float peakVal[4]={
    // peak visibility is: (level-peakWaveBottom)/(peakWaveTop-peakWaveBottom)
    // absolute - we must consider seaLevel here
    1.0/(peakWaveTop-peakWaveBottom), // mul. part
    -(peakWaveBottom+seaLevel)/(peakWaveTop-peakWaveBottom), // add. part
    // shore visibility is: (depth-shoreBottom)/(shoreTop-shoreBottom)
    1.0/(shoreTop-shoreBottom), // mul. part
    -shoreBottom/(shoreTop-shoreBottom), // add. part
  };
  float waterBright = peakWaveBottom;
  float waterDark = peakWaveBottom*5;
  float seaLevelVal[4]={
    seaLevel, // neutral sea level
    // water depth calculation
    1.0/(waterBright-waterDark), // mul. part
    -(waterDark+seaLevel)/(waterBright-waterDark), // add. part
    0.90 // alpha of deep water
  };
  _d3DDevice->SetVertexShaderConstant(WATER_PEAK_WHITE,peakVal,1);
  _d3DDevice->SetVertexShaderConstant(WATER_SEA_LEVEL,seaLevelVal,1);
}

void EngineDDXB::SetLandShadow(
  const float *shadow, int xRange, int zRange, float grid
)
{
  // prepare constants for water "skinning"
  _skinningType = STNone; // this will reset any skinning currently set
  const int landSegSize = 9;
  DoAssert(xRange==landSegSize);
  DoAssert(zRange==landSegSize);
  float landParams[landSegSize*landSegSize][4];
  memset(landParams,0,sizeof(landParams));
  // make sure each entry contains difference to following value as well
  for (int z=0; z<zRange-1; z++) for (int x=0; x<xRange-1; x++)
  {
    int i = z*zRange+x;
    landParams[i][0] = shadow[i];
    landParams[i][1] = shadow[i+1];
    landParams[i][2] = shadow[i+xRange];
    landParams[i][3] = shadow[i+xRange+1];
  }
  _d3DDevice->SetVertexShaderConstant(LAND_SHADOW,landParams,landSegSize*landSegSize);
  // add small eps. because we want to avoid truncating below 0
  // inv. grid size is less then 1/grid, because we want to avoid overflowing above seg. size
  float gridVal[4]={0.9999f/grid,landSegSize,1,(landSegSize-1)*0.5f+0.0001f};
  _d3DDevice->SetVertexShaderConstant(LAND_SHADOWGRID_GRID__1__GRIDd2,gridVal,1);
}

void EngineDDXB::SetSkinningType(ESkinningType skinningType, const Matrix4 *bones, int nBones)
{
  _skinningType = skinningType;

  if (_skinningType == STNone)
  {
    if (_currMinBoneIndex != 0)
    {
      // Setup the offset to the matrices
      static const float constants[4] = {0.5f, 765.0f, 0.0f, 0.0f};
      _d3DDevice->SetVertexShaderConstant(MATRICES_OFFSET_765, constants, 1);
      _currMinBoneIndex = 0;
    }
  }
  else
  {
    // If we can set bones directly
    if (nBones <= GetMaxBonesCount())
    {
      if (nBones>0)
      {
        FLOAT boneValues[MaxSkinningBones*3*4];
        for (int i = 0; i < nBones; i++)
        {
          ConvertMatrixTransposed3(&boneValues[i*3*4], bones[i]);
        }
        _d3DDevice->SetVertexShaderConstant(BONES_START, boneValues, nBones*3);
      }
      _bonesWereSetDirectly = true;
    }
    else
    {
      _bones.Resize(nBones); // Resize, but not shrink
      for (int i = 0; i < nBones; i++)
      {
        _bones[i] = bones[i];
      }
      _bonesWereSetDirectly = false;
    }
    _bonesHaveChanged = true;
  }
}

void EngineDDXB::SetTreeCrown(const Vector3 &min, const Vector3 &max)
{
  // scale using bounding box
  // this converts imaginary bounding ellipsoid to unit sphere
  Vector3Val span = max-min;
  Vector3Val radius = span*0.5f; // make the sphere a little bit smaller
  Vector3Val center = min+radius;
  // scale.InvSize is used to convert result in meters to 0..1
  const float sqrt3 = 1.7320508076f;
  // bounding sphere is probably a little bit too large
  const float resize = 1.1f;
  // some tree-like objects have empty geometry
  if (radius.X()>0 && radius.Y()>0 && radius.Z()>0)
  {
    float constantCenterRadius[8] = {
      center.X(),center.Y(),center.Z(),1,
      1/radius.X(),1/radius.Y(),1/radius.Z(),span.InvSize()*(sqrt3*resize)
    };
    _d3DDevice->SetVertexShaderConstant(TREE_CROWN_CENTER,constantCenterRadius,2);
  }
  else
  {
    float constantCenterRadius[8] = {
      center.X(),center.Y(),center.Z(),1,
      1e9,1e9,1e9,1e9
    };
    _d3DDevice->SetVertexShaderConstant(TREE_CROWN_CENTER,constantCenterRadius,2);
  }
}

void EngineDDXB::SetPosScaleAndOffset(const Vector3 &scale, const Vector3 &offset)
{
  const float invSHRTMAXMIN = 1.0f / (SHRT_MAX - SHRT_MIN);
  Vector3Val scaleI = scale * invSHRTMAXMIN;
  Vector3Val offsetI = offset - SHRT_MIN * invSHRTMAXMIN * scale;

  float constantsScaleOffset[8] = {
    scaleI.X(), scaleI.Y(), scaleI.Z(), 1.0f,
    offsetI.X(), offsetI.Y(), offsetI.Z(), 0.0f
  };
  _d3DDevice->SetVertexShaderConstant(POS_MUL, constantsScaleOffset, 2);
}

void EngineDDXB::SetTexGenScaleAndOffset(const UVPair &scale, const UVPair &offset)
{
  if (_newTexGenOffset!=offset || _newTexGenScale!=scale)
  {
    Assert(scale.u>0);
    Assert(scale.v>0);
    _newTexGenScale = scale;
    _newTexGenOffset = offset;
    _texGenScaleOrOffsetHasChanged = true;
  }
}

bool EngineDDXB::IsAbleToDrawCheckOnly()
{
#ifndef _XBOX
  if (_resetNeeded) return false;
  HRESULT lost = _d3DDevice->TestCooperativeLevel();
  if (lost==D3D_OK)
  {
    _resetNeeded = false;
  }
  else
  {
    _resetNeeded = true;
  }
  return !_resetNeeded;
#else
  return true;
#endif
}

AbilityToDraw EngineDDXB::IsAbleToDraw()
{
  if (!_d3DDevice) return ATDUnable;
#ifndef _XBOX
  HRESULT lost = _resetNeeded ? D3DERR_DEVICENOTRESET : _d3DDevice->TestCooperativeLevel();
  if (lost==D3DERR_DEVICENOTRESET)
  {
    if (Reset())
    {
      _resetNeeded = false;
      return ATDAbleReset;
    }
    else
    {
      _resetNeeded = true;
      return ATDUnable;
    }
  }
  else if (lost==D3D_OK)
  {
    _resetNeeded = false;
    return ATDAble;
  }
  else
  {
    _resetNeeded = true;
    return ATDUnable;
  }
#else
  return ATDAble;
#endif
}

void EngineDDXB::RecalcAccomodation()
{
  base::RecalcAccomodation();

  // Set VS constant
  Color accomodateEye = GetAccomodateEye();
  _d3DDevice->SetVertexShaderConstant(ACCOMODATE_EYE, (D3DXVECTOR4*)&accomodateEye, 1);

  //////////////////////////////////////////////////////////////////////////
  // Set the main light constants

  if (!GScene) return;

  LightSun *sun = GScene->MainLight();
  //Color filter = GetColorFilterEye();
  Color filter = GetAccomodateEye();
  static bool SkyHDREnabled = true;
  if (!SkyHDREnabled)
  {
    filter = GetColorFilterEye();
  }
  Color color;

  // ML_Sun
  Color sunAmbient = sun->GetAmbient() * GetAccomodateEye();
  Color sunDiffuse = sun->GetDiffuse() * GetAccomodateEye();
  _d3DDevice->SetVertexShaderConstant(LDIRECTION_A, (D3DXVECTOR4*)&sunAmbient, 1);
  _d3DDevice->SetVertexShaderConstant(LDIRECTION_D, (D3DXVECTOR4*)&sunDiffuse, 1);

  // ML_Sky
  // Note that we use the light direction from the ML_Stars section in the shader
  Color sunSkyColor = sun->SunSkyColorResult() * filter * _modColor;
  _d3DDevice->SetVertexShaderConstant(LDIRECTIONSKY_SUNSKY_COLOR, (D3DXVECTOR4*)&sunSkyColor, 1);
  Color skyColor = sun->SkyColorResult() * filter * _modColor;
  // fog is saturated at 1.0, and we need this saturated the same way
  skyColor.SaturateMinMax();
  _d3DDevice->SetVertexShaderConstant(LDIRECTIONSKY_SKY_COLOR, (D3DXVECTOR4*)&skyColor, 1);

  // ML_Stars

  // ML_SunObject
  color = sun->SunObjectColor() * filter * 0.5f;
  color.SetA(sun->SunObjectColor().A());
  _d3DDevice->SetVertexShaderConstant(LDIRECTION_SUNOBJECT_COLOR, (D3DXVECTOR4*)&color, 1);

  // ML_SunHaloObject
  color = sun->SunHaloObjectColor() * filter * 0.5f;
  color.SetA(sun->SunHaloObjectColor().A());
  _d3DDevice->SetVertexShaderConstant(LDIRECTION_SUNHALOOBJECT_COLOR, (D3DXVECTOR4*)&color, 1);

  // ML_MoonObject
  color = sun->MoonObjectColor() * filter * 0.5f;
  color.SetA(sun->MoonObjectColor().A());
  _d3DDevice->SetVertexShaderConstant(LDIRECTION_MOONOBJECT_COLOR, (D3DXVECTOR4*)&color, 1);

  // ML_MoonHaloObject
  color = sun->MoonHaloObjectColor() * filter * 0.5f;
  color.SetA(sun->MoonHaloObjectColor().A());
  _d3DDevice->SetVertexShaderConstant(LDIRECTION_MOONHALOOBJECT_COLOR, (D3DXVECTOR4*)&color, 1);

  // Counter monitoring VS constant traffic
#if _ENABLE_PERFLOG
  ADD_COUNTER(setLi, 16*8);
#endif
}

// 8b stencil layout
#define STENCIL_FULL 0xff
#define STENCIL_COUNTER 0x1f // 5 bits counter
#define STENCIL_PROJ_SHADOW 0x20 // 1 bit - projected shadow
#define STENCIL_SHADOW_LEVEL_MASK 0xc0 // 2 bit - shadow level counter
#define STENCIL_SHADOW_LEVEL_STEP 0x40



void EngineDDXB::EnableDepthOfField(float focusDist, float blur, bool farOnly)
{
  const float params[4]={blur,blur,blur,focusDist};
  SetPostFxParam(PFXDepthOfField,params);
  _depthOfFieldFarOnly = farOnly;
}

void EngineDDXB::InitDraw( bool clear, Color color )
{
  // check which buffers queued for deletion can be deleted
  int maxDelete;
  for (maxDelete=0; maxDelete<_statVBufferDelete.Size(); maxDelete++)
  {
    if (_statVBufferDelete[maxDelete]->IsBusy()) break;
  }
  // delete all of them at once
  _statVBufferDelete.DeleteAt(0,maxDelete);
  
  if( _d3dFrameOpen )
  {
    // end scene, but do not call present
    LogF("InitDraw done twice");
    return;
  }

  IsAbleToDrawCheckOnly();

  D3DRECT rect;
  rect.x1=0,rect.y1=0;
  rect.x2=_w,rect.y2=_h;
  int flags=D3DCLEAR_ZBUFFER;
  if (_hasStencilBuffer) flags|=D3DCLEAR_STENCIL;
  if( clear )
  {
    flags|=D3DCLEAR_TARGET;
  }

  D3DSetRenderState(D3DRS_STENCILWRITEMASK, STENCIL_FULL);
  //_aisValue = 0;
  for (int i=0; i<lenof(_postFxParam); i++)
  {
    _postFxParam[i][0] = _postFxParam[i][1] = 
    _postFxParam[i][2] = _postFxParam[i][3] = 0;
    _postFxEnabled[i] = false;
  }
  _depthOfFieldFarOnly = false;

  if (!_resetNeeded)
  {
    PROFILE_DX_SCOPE(3clr);
    // color needs to be halved - it will be double in HDR glow pass
    color = color * GetHDRFactor();
    PackedColor pc = PackedColor(color);
    if (_pixelSize==16) pc = PackedColor(CLIP_TO_565(pc));
    _d3DDevice->Clear(0, NULL, flags, pc, 1, 0);
  }

  _textBank->StartFrame();

  base::InitDraw();

#if LOG_LOCKS
  LogF("BeginScene");
#endif

#if _ENABLE_PERFLOG
  LogStatesOnce = false;
  if (GInput.GetCheat1ToDo(DIK_E))
  {
#if DO_TEX_STATS
    LogF("---------------------------------");
    LogF("Report texture switch totals");
    TexStats.Report();
    LogF("---------------------------------");
    TexStats.Clear();
    EnableTexStats = true;
#endif
    LogStatesOnce = true;
  }
#if DO_TEX_STATS
  if (EnableTexStats) TexStats.Count("** BeginScene **");
#endif
  if (LogStatesOnce)
  {
    LogF("---- BeginScene");
  }
#endif
  D3DBeginScene();

  _d3dFrameOpen = true;
  _shadowLevel = 0;
  _shadowLevelNeeded = 0;

  TLMaterial invalidMat;
  invalidMat.diffuse = Color(-1,-1,-1,-1);
  invalidMat.ambient = Color(-1,-1,-1,-1);
  invalidMat.forcedDiffuse = Color(-1,-1,-1,-1);
  invalidMat.emmisive = Color(-1,-1,-1,-1);
  invalidMat.specFlags = 0;

  LightList lights;
  DoSetMaterial(invalidMat,lights,0,false);
  // mark material as invalid
  _lastMat = (TexMaterial *)-1;
}

#ifdef _XBOX 
#define EXPLICIT_FENCE_LEVEL 0
//#define EXPLICIT_FENCE_LEVEL 100
#endif

/**
Caution: this actually copis current front buffer to all backbuffers.
*/
void EngineDDXB::BackBufferToAllBuffers()
{
  DoAssert(!_d3dFrameOpen);
  ComRef<IDirect3DSurface8> back;
  ComRef<IDirect3DSurface8> front;
  _d3DDevice->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,back.Init());
  _d3DDevice->GetBackBuffer(-1,D3DBACKBUFFER_TYPE_MONO,front.Init());
  _d3DDevice->CopyRects(front,NULL,0,back,NULL);
  for (int i=1; i<(int)_pp.BackBufferCount; i++)
  {
    ComRef<IDirect3DSurface8> backAdd;
    _d3DDevice->GetBackBuffer(i,D3DBACKBUFFER_TYPE_MONO,backAdd.Init());
    _d3DDevice->CopyRects(front,NULL,0,backAdd,NULL);
  }
}

/// total size of GPU-CPU communications buffer
//const int GPUPushBufferSize = 1824*1024;
const int GPUPushBufferSize = 1280*1024;
/// granularity of the buffer
//const int GPUKickOffSize = 64*1024;
const int GPUKickOffSize = 32*1024;

float EngineDDXB::GetPushBufferUsage() const
{
  int fences = _d3DDevice->GetPushDistance(D3DDISTANCE_FENCES_TOIDLE);
  int estimateSize = GPUKickOffSize*fences;
  return estimateSize/float(GPUPushBufferSize);
}

extern bool UseProgramDrawing;

float EngineDDXB::WaitBeforeFrameDrawing()
{
  if (UseProgramDrawing) return 0.0f;

  int fences = _d3DDevice->GetPushDistance(D3DDISTANCE_FENCES_TOIDLE);
  int nFences = GPUPushBufferSize/GPUKickOffSize;

  int limitFences = nFences/2;
  // if we are not over limit, we can continue immediately
  if (fences<=limitFences) return 0;
  int over = fences-limitFences;
  // we estimate 800 KB pushbuffer will take around 30 ms to become empty
  float timePerFence = 0.030f/(800*1024)*GPUKickOffSize;
  return over*timePerFence;
}

void EngineDDXB::FinishDraw(int wantedDurationVSync, bool allBackBuffers)
{
  if( _d3dFrameOpen )
  {
    {
      SCOPE_GRF("drwFD",SCOPE_COLOR_GRAY);
      base::FinishDraw(wantedDurationVSync,allBackBuffers);

      base::DrawFinishTexts();

      // we may draw any additional diag. info here
#if _ENABLE_REPORT
      if (ShowFps()>0)
      {
        float usage = GetPushBufferUsage();

        Color color(0.5,0.5,0,0.3);
        Rect2DFloat rect;
        rect.x = 0.5;
        rect.w = (0.95-rect.x)*usage;
        rect.h = 0.04;
        rect.y = 0.20;
        Rect2DAbs rect2D;
        GEngine->Convert(rect2D,rect);
        MipInfo mip = GEngine->TextBank()->UseMipmap(NULL, 0, 0);
        GEngine->Draw2D(mip,PackedColor(color),rect2D);
        Point2DAbs pos(rect2D.x,rect2D.y);

        Font *font=GEngine->GetDebugFont();

#ifdef _XBOX
        float textSize = 0.03;
#else
        float textSize = 0.02;
#endif

        PackedColor colorText(Color(0.2,0.2,0.2,0.8));
        TextDrawAttr attrText(textSize,font,colorText,true);

        GEngine->DrawTextF(pos,attrText,"%3d %%",toInt(usage*100));
      }
#endif

      // Print info
//      extern ShapeBank Shapes;
//      Shapes.ShowLastLODsSpace();

      {
        //PROFILE_SCOPE(drwFC);
        CloseAllQueues(_queueNo);
      }
#if EXPLICIT_FENCE_LEVEL>0
      _d3DDevice->InsertFence();
#endif
      _d3dFrameOpen=false;

      {
        //PROFILE_SCOPE(drwFD);
        DiscardVB();
      }

#if LOG_STATE_LEVEL
      if (LogStatesOnce)
      {
        LogF("---- EndScene");
      }
#endif


      // fill shadows (using stencil buffer)

#if LOG_LOCKS
      LogF("[[[ EndScene");
#endif
      D3DEndScene();
#if LOG_LOCKS
      LogF("]]] EndScene");
#endif


      {
        //PROFILE_SCOPE(drwFF);
        _textBank->FinishFrame();
      }

      // Clear the texture cache
      _hwInDefinedState = false;
      for (int i = 0; i < TEXTURE_STAGES; i++)
      {
        _d3DDevice->SetTexture(i,NULL);
        _lastHandle[i] = NULL;
        _lastTexture[i].Free();
      }

      if (allBackBuffers)
      {
        //PROFILE_SCOPE(drwFA);
        BackBufferToAllBuffers();
      }
      // make sure we are not waiting too much
      // it might be better to call BackToFront before BeginScene
      // to reduce latency we call it as soon as possible - it should be asynchronous anyway
    }
    {
      //PROFILE_SCOPE(drwFB);
      BackToFront(wantedDurationVSync);
    }
#if defined _XBOX && _ENABLE_PERFLOG && _ENABLE_PERFLIB
    D3DPERF_SetMarker(0,"BegFrame");
#endif

  }
}

void EngineDDXB::NextFrame()
{
  // swap frames - get ready for next frame
  // experimental - several BackToFront positions in code possible
#if defined _XBOX && _ENABLE_PERFLOG && _ENABLE_PERFLIB
  D3DPERF_SetMarker(0,"NextFrame");
#endif

#if defined STRIPIZATION_ENABLED && defined _XBOX && _ENABLE_CHEATS
  if (GInput.GetCheatXToDo(CXTStrips))
  {
    EnableStrips = !EnableStrips;
    GlobalShowMessage(500,"Strips %s",EnableStrips ? "On":"Off");
  }
#endif


  base::NextFrame();
}

bool EngineDDXB::InitDrawDone()
{
  return _d3dFrameOpen;
}

void EngineDDXB::D3DTextureDestroyed(TextureD3DHandle tex)
{
  for (int i = 0; i < TEXTURE_STAGES; i++)
  {
    if (_lastHandle[i]==tex)
    {
      if (_recording)
      {
        Fail("Texture destroyed while recording");
      }
      // even when recording we need to unbind the texture in case it is currently bound
      _hwInDefinedState = false;
      _d3DDevice->SetTexture(i, NULL);
      _lastHandle[i] = NULL;
      _lastTexture[i].Free();
      if (i == 0)
      {
        SetTexture0Color(HWhite,HWhite,true);
      }
    }
  }
}

void EngineDDXB::TextureDestroyed(Texture *tex)
{
}

void EngineDDXB::AddFragmentNames(char *s, const DWORD * const *fragments, int fragmentCount)
{
  for (int j = 0; j < fragmentCount; j++)
  {
    if (fragments[j] == _shaderWater) sprintf(s, "%s ShadersWater", s);
    else if (fragments[j] == _shaderWaterSimple) sprintf(s, "%s ShadersWaterSimple", s);
    else if (fragments[j] == _shaderGetPosNorm) sprintf(s, "%s ShadersGetPosNorm", s);
    else if (fragments[j] == _shaderGetInst) sprintf(s, "%s ShadersGetInst", s);
    else if (fragments[j] == _shader) sprintf(s, "%s Shaders", s);
    else if (fragments[j] == _shaderNormal) sprintf(s, "%s ShadersNormal", s);
    else if (fragments[j] == _shaderNormalDiffuse) sprintf(s, "%s ShadersNormalDiffuse", s);
    else if (fragments[j] == _shaderNormalThrough) sprintf(s, "%s ShadersNormalThrough", s);
    else if (fragments[j] == _shaderShadowVolume) sprintf(s, "%s ShadersShadowVolume", s);
    else if (fragments[j] == _shaderPointO) sprintf(s, "%s ShadersPoint", s);
    else if (fragments[j] == _shaderInstancing) sprintf(s, "%s ShadersInstancing", s);
    else if (fragments[j] == _shaderNormalInstancing) sprintf(s, "%s ShadersNormalInstancing", s);
    else if (fragments[j] == _shaderInstancingEmissive) sprintf(s, "%s ShadersInstancingEmissive", s);
    else if (fragments[j] == _shaderInstancingModulate) sprintf(s, "%s ShadersInstancingModulate", s);
    else if (fragments[j] == _shaderNormalThroughInstancing) sprintf(s, "%s ShadersNormalThroughInstancing", s);
    else if (fragments[j] == _shaderNormalDiffuseInstancing) sprintf(s, "%s ShadersNormalDiffuseInstancing", s);
    else if (fragments[j] == _shaderShadowVolumeInstancing) sprintf(s, "%s ShadersShadowVolumeInstancing", s);
    else if (fragments[j] == _shaderSpriteInstancing) sprintf(s, "%s ShadersSpriteInstancing", s);
    else if (fragments[j] == _shaderSkinning) sprintf(s, "%s ShadersSkinning", s);
    else if (fragments[j] == _shaderNormalSkinning) sprintf(s, "%s ShadersNormalSkinning", s);
    else if (fragments[j] == _shaderNormalDiffuseSkinning) sprintf(s, "%s ShadersNormalDiffuseSkinning", s);
    else if (fragments[j] == _shaderShadowVolumeSkinning) sprintf(s, "%s ShadersShadowVolumeSkinning", s);
    else if (fragments[j] == _shaderDirectional) sprintf(s, "%s ShadersDirectional", s);
    else if (fragments[j] == _shaderDirectionalNoSpec) sprintf(s, "%s ShadersDirectionalNoSpec", s);
    else if (fragments[j] == _shaderDirectionalADForced) sprintf(s, "%s ShadersDirectionalADForced", s);
    else if (fragments[j] == _shaderDirectionalSky) sprintf(s, "%s ShadersDirectionalSky", s);
    else if (fragments[j] == _shaderDirectionalStars) sprintf(s, "%s ShadersDirectionalStars", s);
    else if (fragments[j] == _shaderDirectionalSunObject) sprintf(s, "%s ShadersDirectionalSunObject", s);
    else if (fragments[j] == _shaderDirectionalSunHaloObject) sprintf(s, "%s ShadersDirectionalSunHaloObject", s);
    else if (fragments[j] == _shaderDirectionalMoonObject) sprintf(s, "%s ShadersDirectionalMoonObject", s);
    else if (fragments[j] == _shaderDirectionalMoonHaloObject) sprintf(s, "%s ShadersDirectionalMoonHaloObject", s);
    else if (fragments[j] == _shaderAlphaShadow) sprintf(s, "%s ShadersAlphaShadow", s);
    else if (fragments[j] == _shaderLandShadow) sprintf(s, "%s ShadersLandShadow", s);
    else if (fragments[j] == _shaderModShadow) sprintf(s, "%s ShadersModShadow", s);
    else if (fragments[j] == _shaderModInstShadow) sprintf(s, "%s ShadersModInstShadow", s);
    else if (fragments[j] == _shaderFog) sprintf(s, "%s ShadersFog", s);
    else if (fragments[j] == _shaderFogAlpha) sprintf(s, "%s ShadersFogAlpha", s);
    else if (fragments[j] == _shaderFogNone) sprintf(s, "%s ShadersFogNone", s);
    else if (fragments[j] == _shaderPoint[0]) sprintf(s, "%s ShadersPoint0", s);
    else if (fragments[j] == _shaderPoint[1]) sprintf(s, "%s ShadersPoint1", s);
    else if (fragments[j] == _shaderPoint[2]) sprintf(s, "%s ShadersPoint2", s);
    else if (fragments[j] == _shaderPoint[3]) sprintf(s, "%s ShadersPoint3", s);
    //        else if (fragments[j] == _shaderPoint[4]) sprintf(s, "%s Point[4]", s);
    //        else if (fragments[j] == _shaderPoint[5]) sprintf(s, "%s Point[5]", s);
    //        else if (fragments[j] == _shaderPoint[6]) sprintf(s, "%s Point[6]", s);
    //        else if (fragments[j] == _shaderPoint[7]) sprintf(s, "%s Point[7]", s);
    else if (fragments[j] == _shaderSpot[0]) sprintf(s, "%s ShadersSpot0", s);
    else if (fragments[j] == _shaderSpot[1]) sprintf(s, "%s ShadersSpot1", s);
    else if (fragments[j] == _shaderSpot[2]) sprintf(s, "%s ShadersSpot2", s);
    else if (fragments[j] == _shaderSpot[3]) sprintf(s, "%s ShadersSpot3", s);
    //        else if (fragments[j] == _shaderSpot[4]) sprintf(s, "%s Spot[4]", s);
    //        else if (fragments[j] == _shaderSpot[5]) sprintf(s, "%s Spot[5]", s);
    //        else if (fragments[j] == _shaderSpot[6]) sprintf(s, "%s Spot[6]", s);
    //        else if (fragments[j] == _shaderSpot[7]) sprintf(s, "%s Spot[7]", s);
    else if (fragments[j] == _shaderWriteLights) sprintf(s, "%s ShadersWriteLights", s);
    else if (fragments[j] == _shaderWriteLightsAlphaMod) sprintf(s, "%s ShadersWriteLightsAlphaMod", s);
    else if (fragments[j] == _shaderWriteLightsColorMod) sprintf(s, "%s ShadersWriteLightsColorMod", s);
    else sprintf(s, "%s UnknownFragment", s);
  }
}

#if SHADER_INFO_PRINTING

#include <Es/Algorithms/qsort.hpp>

static int CmpShaderInfo(const VSInfo *p1, const VSInfo *p2)
{
  if (p1->_usedCount > p2->_usedCount)
  {
    return -1;
  }
  else if (p1->_usedCount < p2->_usedCount)
  {
    return +1;
  }
  // make sure order is stable
  if (p1<p2) return -1;
  if (p1>p2) return +1;
  return 0;
}


#endif

void EngineDDXB::D3DEndScene()
{
  //_sBuffer = NULL;

  PROFILE_DX_SCOPE(3eScn);
  HRESULT err=_d3DDevice->EndScene();
  if( err!=D3D_OK ) DDErrorXB("Cannot end scene",err);

#if SHADER_INFO_PRINTING

  if (_showNotCachedShader || _printNotCachedShader)
  {
    // Sort the shader info array
    QSort(_vsInfo.Data(), _vsInfo.Size(), CmpShaderInfo);

    int notFoundShader = 0;
    for (int i = 0; i < _vsInfo.Size(); i++)
    {
      VSInfo &vsInfo = _vsInfo[i];
      
      // ignore unused shader
      if (vsInfo._usedCount<=0) continue;

      // If the shader is not designed to be optimized (f.i. it is optimized already), skip it
      if ((vsInfo._fragmentCount == 1) && (vsInfo._fragments[0] == _shaderShadowVolume)) continue;
      if ((vsInfo._fragmentCount == 1) && (vsInfo._fragments[0] == _shaderShadowVolumeSkinning)) continue;
      if ((vsInfo._fragmentCount == 1) && (vsInfo._fragments[0] == _shaderShadowVolumeInstancing)) continue;

      // Try to find it in the shader cache
      bool vsInfoFound = false;
      for (int i = 0; i < _shaderCacheId.Size(); i++)
      {
        ShaderCacheId &scid = _shaderCacheId[i];
        if (scid._fragmentsCount != vsInfo._fragmentCount) continue;
        int j;
        for (j = 0; j < scid._fragmentsCount; j++)
        {
          if (vsInfo._fragments[j] != scid._fragments[j]) break;
        }
        if (j < scid._fragmentsCount) continue;
        vsInfoFound = true;
        break;
      }

      if (!vsInfoFound)
      {
        for (int i = 0; i < _shaderCacheLightId.Size(); i++)
        {
          ShaderCacheId &scid = _shaderCacheLightId[i];
          if (scid._fragmentsCount != vsInfo._fragmentCount) continue;
          int j;
          for (j = 0; j < scid._fragmentsCount; j++)
          {
            if (vsInfo._fragments[j] != scid._fragments[j]) break;
          }
          if (j < scid._fragmentsCount) continue;
          vsInfoFound = true;
          break;
        }
      }

      // If not found, then report
      if (!vsInfoFound)
      {
        char s[2048];
        //sprintf(s, "%d: %d instructions (%d:%d) saved in ", i, (vsInfo._icBeforeOptimalization - vsInfo._icAfterOptimalization) * vsInfo._usedCount, vsInfo._icBeforeOptimalization, vsInfo._icAfterOptimalization);
        sprintf(s, "%d: t%d ", notFoundShader, vsInfo._usedCount);
        AddFragmentNames(s, vsInfo._fragments, vsInfo._fragmentCount);
        const int showCount = 8;
        static InitVal<int,-1> notCachedHandle[showCount];
        if (_showNotCachedShader && notFoundShader < showCount)
        {
		      GlobalDiagMessage(notCachedHandle[notFoundShader], 200, s);
        }
        if (_printNotCachedShader)
        {
          LogF("%s", s);
        }
        notFoundShader++;
      }
    }
    
    #if _PROFILE
    if (_printNotCachedShader)
    {
      // print all shader combinations, including a cached ones, into a file
      QOFStream f;
      f.open("u:\\vertexshaders.lst");
      for (int i = 0; i < _shaderCacheLightId.Size(); i++)
      {
        const ShaderCacheId &item = _shaderCacheLightId[i];
        char s[2048];
        sprintf(s,"L %d",i);
        AddFragmentNames(s, item._fragments, item._fragmentsCount);
        f << s << "\r\n";
      }
      for (int i = 0; i < _shaderCacheId.Size(); i++)
      {
        const ShaderCacheId &item = _shaderCacheId[i];
        char s[2048];
        sprintf(s,"N %d",i);
        AddFragmentNames(s, item._fragments, item._fragmentsCount);
        f << s << "\r\n";
      }
      f.close();
    }
    #endif
    _printNotCachedShader = false;
  }

  // Print the fragment optimalization results
//  if (_vsInfoLastCount != _vsInfo.Size())
//  {
//    for (int i = 0; i < _vsInfo.Size(); i++)
//    {
//      VSInfo &vsInfo = _vsInfo[i];
//      char s[2048];
//      sprintf(s, "%d instructions (%d:%d) saved in ", (vsInfo._icBeforeOptimalization - vsInfo._icAfterOptimalization) * vsInfo._usedCount, vsInfo._icBeforeOptimalization, vsInfo._icAfterOptimalization);
//      AddFragmentNames(s, vsInfo._fragments, vsInfo._fragmentCount);
//      LogF("%s", s);
//
//    }
//    _vsInfoLastCount = _vsInfo.Size();
//  }

  // Zero counter
  for (int i = 0; i < _vsInfo.Size(); i++)
  {
    _vsInfo[i]._usedCount = 0;
  }
#endif

}
void EngineDDXB::D3DBeginScene()
{
  LogTexture("XXX Start XXX",NULL);
  // check cooperative level (surfaces lost)
  PROFILE_DX_SCOPE(3bScn);
  HRESULT err=_d3DDevice->BeginScene();
  if( err!=D3D_OK )
  {
    DDErrorXB("Cannot begin scene",err);
  }

#if SHADER_INFO_PRINTING
  if (GInput.GetCheatXToDo(CXTNotCachedShader))
  {
    _showNotCachedShader = !_showNotCachedShader;
  }
  if (GInput.GetCheatXToDo(CXTNotCachedShaderPrint))
  {
    _printNotCachedShader = true;
  }
#endif

}

void EngineDDXB::PrepareMesh( int spec )
{
  // prepare internal variables
  SwitchTL(false);
  Matrix4Val proj = _sceneProps->GetCameraProjection();
  ChangeWDepth(proj(2,2),proj.Position()[2]);
  ChangeClipPlanes();
}


DEFINE_FAST_ALLOCATOR(IndexAllocXB);
//DEFINE_FAST_ALLOCATOR(VertexAlloc);

struct VBSectionInfo
{
  int beg,end;
  int begVertex,endVertex; // used vertex range
};

TypeIsSimple(VBSectionInfo);

/// d3d vertex buffer encapsulation
/**
Derived from TLinkBidirD so that it can be listed in LRU list
*/
class VertexBufferD3DXB: public VertexBuffer, public VertexBufferD3DXBLRUItem
{
  friend class EngineDDXB;
  friend class VertexStaticDataXB;
  friend class IndexStaticDataXB;
  friend class PushBufferXB;

private:

  bool _separate;
  bool _dynamic;

  AutoArray<VBSectionInfo> _sections;
  IndexAllocXB *_iAlloc;
  VertexAllocXB *_vAlloc;
  int _iOffset; // where are indices placed in shared static buffer
  int _vOffset; // vertex index offsetting done by D3D
  //! Maximum Pos values the vertex buffer contains (suitable for Pos compression)
  Vector3 _maxPos;
  //! Minimum Pos values the vertex buffer contains (suitable for Pos compression)
  Vector3 _minPos;
  //! Maximum UV values the vertex buffer contains (suitable for UV compression)
  UVPair _maxUV;
  //! Minimum UV values the vertex buffer contains (suitable for UV compression)
  UVPair _minUV;

  // following variables make sense only for "separate" vertex buffers
  ComRef<IDirect3DVertexBuffer8> _vBuffer;
  ComRef<IDirect3DIndexBuffer8> _iBuffer;
  int _iBufferSize;
  int _vBufferSize;

  // following are valid only for "shared" (!_separate)
  Link<VertexStaticDataXB> _sharedV;
  Link<IndexStaticDataXB> _sharedI;

protected:
  void CopyData( const Shape &src );
  void AllocateIndices(EngineDDXB *engine, int indices);
  void AllocateVertices(EngineDDXB *engine, int vertices);

public:
  VertexBufferD3DXB();
  ~VertexBufferD3DXB();


  //@{ implementation of VertexBufferD3DXBLRUItem
  virtual size_t GetBufferSize() const;
  //@}

  bool Init(EngineDDXB *engine, const Shape &src, VBType type, bool frequent);

  //! prepare buffer for rendering
  void Update(const Shape &src, const EngineShapeProperties &prop, VSDVersion vsdVersion);

  void Detach(const Shape &src);

  virtual void RemoveIndexBuffer();

  bool IsGeometryLoaded() const;

  double GetMemoryUsed() const {return GetBufferSize();}
  /// get number of indices in the buffer
  virtual int GetVBufferSize() const
  {
    return _vAlloc ? _vAlloc->Size() : _vBufferSize;
  };
  //@}
  virtual void BindWithPushBuffer()
  {
    if (_sharedV)
    {
      _sharedV->SetFence(GEngineDD->GetPBLock());
    }
  }

  virtual void OnSizeChanged()
  {
    GEngineDD->OnSizeChanged(this);
  }
};

VertexStaticDataXB::VertexStaticDataXB()
{
  //_vBufferSize = 0; // reserved sizes of resources
  //_vBufferUsed = 0; // used sizes
  _nShapes = 0;
  _fence = 0;
  _fenceReleased = 0;
}

VertexStaticDataXB::~VertexStaticDataXB()
{
  Dealloc();
}

void VertexStaticDataXB::Init(EngineDDXB *engine, int vertices, int vertexSize, IDirect3DDevice8 *dev)
{
  _engine = engine;
  _vertexSize = vertexSize;
  if (vertices>0)
  {
    int byteSize = vertices*vertexSize;
    #if !CUSTOM_VB_ALLOCATOR
      // round up to nearest 4 KB to avoid fragmentation as much as possible
      const int allocGranularity = 4*1024;
      
      byteSize = (byteSize+allocGranularity-1)&~(allocGranularity-1);
      
      Assert(byteSize/vertexSize>=vertices);
      vertices = byteSize/vertexSize;
    #endif
    

    RetryV:
    #if CUSTOM_VB_ALLOCATOR
      _mem = AllocatePhysicalMemory(byteSize);
      if (_mem.IsNull())
      {
        size_t freed = FreeOnDemandSystemMemory(byteSize);
        if (freed!=0) goto RetryV;
        LogF("Out of memory during v-buffer creation");
        return;
      }
      _vBuffer = new IDirect3DVertexBuffer8();
      _vBuffer->AddRef();
      _vBuffer->Register(_mem.GetAddress());
      
    #else
      RetryV:
      HRESULT err = dev->CreateVertexBuffer
      (
        byteSize,WRITEONLYVB,0,D3DPOOL_DEFAULT,
        _vBuffer.Init()
      );
      if (err!=D3D_OK)
      {
        if (err==D3DERR_OUTOFVIDEOMEMORY)
        {
          LogF("Out of VRAM during v-buffer creation");
          if (GEngineDD->TextBankDD()->ForcedReserveMemory(byteSize))
          {
            goto RetryV;
          }
        }
        else if (err=E_OUTOFMEMORY)
        {
          size_t freed = FreeOnDemandSystemMemory(byteSize);
          if (freed!=0) goto RetryV;
        }
        // debugging opportunity
        LogF("Error during v-buffer creation: error %x",err);
        size_t freed = FreeOnDemandSystemMemory(byteSize);
        if (freed!=0) goto RetryV;
        DDErrorXB("CreateVertexBuffer",err);
        return;
        // 
      }
    #endif
    // there might be a slight overhead (1 partial vertex), but we prefer this
    // because it will be easier to manage for the allocator
    _manager.Init(0,vertices,1);
    engine->AddVBuffersAllocated(_manager.Size()*vertexSize);

  }
}

#define V_OFFSET_CUSTOM 1


void VertexStaticDataXB::Dealloc()
{
  if (_nShapes>0)
  {
    LogF("VertexStaticDataXB:: %d shapes not removed",_nShapes);
  }
  if (_vBuffer)
  {
    _engine->AddVBuffersAllocated(-_manager.Size()*_vertexSize);
    if(GEngineDD->GetDirect3DDevice()->IsFencePending(_fence))
    {
      LogF("Busy vertex buffer released");
      GEngineDD->GetDirect3DDevice()->BlockOnFence(_fence);
    }

    #if CUSTOM_VB_ALLOCATOR
      FreePhysicalMemory(_mem);
      delete _vBuffer;
      _vBuffer = NULL;
    #else
      _vBuffer.Free();
    #endif
  }
}

#if defined _KNI
static inline void CopyNormal(Vector3P &tgt, Vector3Par src)
{
  tgt = Vector3P(-src.X(),-src.Y(),-src.Z());
}
#elif COMPRESSED_NORMALS
static __forceinline DWORD IntRange(float x, int mask)
{
  int i = toInt(x*-(mask/2));
#if 1 //_DEBUG
  if (i<-mask/2)
  {
    //LogF("Out of range");
    i = -mask/2;
  }
  else if (i>+mask/2)
  {
    //LogF("Out of range");
    i = +mask/2;
  }
#endif
  return i&mask;
}
static __forceinline void CopyNormal(DWORD &tgt, Vector3Par src)
{
  // note: make sure normals are not out of range
  tgt =
    (
    ( IntRange(src.Z(),0x3ff ) << 22L ) |
    ( IntRange(src.Y(),0x7ff ) << 11L ) |
    ( IntRange(src.X(),0x7ff ) <<  0L )
    );
}
#else
inline void CopyNormal(Vector3P &tgt, Vector3Par src)
{
  tgt = -src;
}

/*
LONG D3DVECTORtoSLONG( const D3DVECTOR &v )
{
LONG n = (LONG)
(
((LONG)(v.z * 511.0f) & 0x3ff) << 22) |
(LONG)(((LONG)(v.y * 1023.0f) & 0x7ff) << 11) |
(LONG)(((LONG)(v.x * 1023.0f) & 0x7ff)
);
return n;
}
*/
#endif

/*
static UVPair minUV = {FLT_MAX, FLT_MAX};
static UVPair maxUV = {FLT_MIN, FLT_MIN};
*/
static __forceinline void CopyUV(UVPairCompressed &tgt, const UVPair &src, const UVPair &srcMaxUV, const UVPair &srcMinUV, const UVPair &invUV)
{
  Assert(srcMaxUV.u != -1e6);
  Assert(srcMaxUV.v != -1e6);
  Assert(srcMinUV.u != 1e6);
  Assert(srcMinUV.v != 1e6);
  tgt.u = toInt(((src.u - srcMinUV.u) * invUV.u) * (SHRT_MAX - SHRT_MIN) + SHRT_MIN);
  tgt.v = toInt(((src.v - srcMinUV.v) * invUV.v) * (SHRT_MAX - SHRT_MIN) + SHRT_MIN);
}

static inline Vector3 CalcInvScale(Vector3Par srcMaxPos, Vector3Par srcMinPos)
{
  Vector3 scale = srcMaxPos-srcMinPos;
  saturateMax(scale[0],1e-6);
  saturateMax(scale[1],1e-6);
  saturateMax(scale[2],1e-6);
  return Vector3(1/scale[0],1/scale[1],1/scale[2]);
}

static inline UVPair CalcInvScale(const UVPair &srcMaxPos, const UVPair &srcMinPos)
{
  UVPair scale = {srcMaxPos.u-srcMinPos.u,srcMaxPos.v-srcMinPos.v};
  saturateMax(scale.u,1e-6);
  saturateMax(scale.v,1e-6);
  UVPair ret={1/scale.u,1/scale.v};
  return ret;
}

static __forceinline void CopyPos(
                                  Vector3Compressed &tgt, Vector3Par src, Vector3Par srcMaxPos, Vector3Par srcMinPos,
                                  Vector3Par invScale
                                  )
{
  Assert(srcMaxPos.X() != -1e6);
  Assert(srcMaxPos.Y() != -1e6);
  Assert(srcMaxPos.Z() != -1e6);
  Assert(srcMinPos.X() != 1e6);
  Assert(srcMinPos.Y() != 1e6);
  Assert(srcMinPos.Z() != 1e6);
  tgt.x = toInt(((src.X() - srcMinPos.X()) * invScale.X()) * (SHRT_MAX - SHRT_MIN) + SHRT_MIN);
  tgt.y = toInt(((src.Y() - srcMinPos.Y()) * invScale.Y()) * (SHRT_MAX - SHRT_MIN) + SHRT_MIN);
  tgt.z = toInt(((src.Z() - srcMinPos.Z()) * invScale.Z()) * (SHRT_MAX - SHRT_MIN) + SHRT_MIN);
}

/*!
\patch 1.24 Date 9/21/2001 by Ondra
- Fixed: Lighting of polygons by point lights was reversed on HW T&L.
\patch 1.32 Date 11/26/2001 by Ondra
- Fixed: Destroyed buildings did not collapse with HW T&L.
*/


bool VertexStaticDataXB::AddShape(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic)
{

  // when adding dynamic data, add only indices

  DWORD flags = NoSysLockFlag|D3DLOCK_NOOVERWRITE;

  if (_vertexSize != sizeof(SVertexXB)) return false;

  if (!dynamic)
  {
    VertexHeapXB::HeapItem *item = _manager.Alloc(src.NVertex());
    if (!item)
    {
      /*
      Log("No more space in vertex buffer - %d shapes",_nShapes);
      Log
      (
      "  used: V %d of %d",
      _manager.TotalBusy(),_manager.Size()
      );
      */
      return false; 
    }

    ShapeGeometryLock<> lock(&src);

    // Calculate the vertex buffer maximum and minimum Pos's
    src.CalculateMaxAndMinPos(vbuf._maxPos, vbuf._minPos);

    // Calculate the vertex buffer maximum and minimum UV's
    src.CalculateMaxAndMinUV(vbuf._maxUV, vbuf._minUV);

    vbuf._vAlloc = item;
    GEngineDD->ReportVBAlloc(vbuf._vAlloc->Size()*_vertexSize);
RetryV:
    BYTE *data = NULL;
    HRESULT err;
    {
      PROFILE_DX_SCOPE(3vbLS);
      err = _vBuffer->Lock
        (
        item->Memory()*sizeof(SVertexXB),src.NVertex()*sizeof(SVertexXB),
        &data,flags
        );
    }

    if (err!=D3D_OK || !data)
    {
      if (QFBank::FreeUnusedBanks(1024*1024)) goto RetryV;
#ifndef _XBOX
      ErrF("VB Lock failed, %s",DXGetErrorString8(err));
#else
      ErrF("VB Lock failed, %x",err);
#endif
      return false;
    }

    SVertexXB *sData = (SVertexXB *)data;
    const CondensedAutoArray<UVPair> *uv = &src.GetUVArray();
    const AutoArray<Vector3> *pos;
    const CondensedAutoArray<Vector3> *norm;
    if (src.OriginalPosValid())
    {
      pos = &src.GetOrigPosArray();
      norm = &src.GetOrigNormArray();
    }
    else
    {
      pos = &src.GetPosArray();
      norm = &src.GetNormArray();
    }
    Vector3 invScale = CalcInvScale(vbuf._maxPos, vbuf._minPos);
    UVPair invUV = CalcInvScale(vbuf._maxUV, vbuf._minUV);
    int nVertex = src.NVertex();
    for (int i = 0; i < nVertex; i++)
    {
      CopyPos(sData->pos, (*pos)[i], vbuf._maxPos, vbuf._minPos, invScale);
      CopyNormal(sData->norm, (*norm)[i]);
      CopyUV(sData->t0, (*uv)[i], vbuf._maxUV, vbuf._minUV, invUV);
      sData++;
    }
    _vBuffer->Unlock();

    _nShapes++;
  }
  else
  {
    vbuf._vAlloc = NULL;
  }

  return true;
}

bool VertexStaticDataXB::AddShapeNormal(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic)
{
  // when adding dynamic data, add only indices

  DWORD flags = NoSysLockFlag|D3DLOCK_NOOVERWRITE;

  if (_vertexSize != sizeof(SVertexNormalXB)) return false;

  if (!dynamic)
  {
    VertexHeapXB::HeapItem *item = _manager.Alloc(src.NVertex());
    if (!item)
    {
      return false; 
    }

    ShapeGeometryLock<> lock(&src);

    // Calculate the vertex buffer maximum and minimum Pos's
    src.CalculateMaxAndMinPos(vbuf._maxPos, vbuf._minPos);

    // Calculate the vertex buffer maximum and minimum UV's
    src.CalculateMaxAndMinUV(vbuf._maxUV, vbuf._minUV);

    vbuf._vAlloc = item;
    GEngineDD->ReportVBAlloc(vbuf._vAlloc->Size()*_vertexSize);
RetryV:
    BYTE *data = NULL;
    HRESULT err;
    {
      PROFILE_DX_SCOPE(3vbLS);
      err = _vBuffer->Lock
        (
        item->Memory()*sizeof(SVertexNormalXB),src.NVertex()*sizeof(SVertexNormalXB),
        &data,flags
        );
    }

    if (err!=D3D_OK || !data)
    {
      if (QFBank::FreeUnusedBanks(1024*1024)) goto RetryV;
#ifndef _XBOX
      ErrF("VB Lock failed, %s",DXGetErrorString8(err));
#else
      ErrF("VB Lock failed, %x",err);
#endif
      return false;
    }

    const CondensedAutoArray<UVPair> *uv = &src.GetUVArray();
    const AutoArray<Vector3> *pos;
    const CondensedAutoArray<Vector3> *norm;
    if (src.OriginalPosValid())
    {
      pos = &src.GetOrigPosArray();
      norm = &src.GetOrigNormArray();
    }
    else
    {
      pos = &src.GetPosArray();
      norm = &src.GetNormArray();
    }
    Vector3 invScale = CalcInvScale(vbuf._maxPos, vbuf._minPos);
    UVPair invUV = CalcInvScale(vbuf._maxUV, vbuf._minUV);

    if (src.IsSTPresent())
    {
      SVertexNormalXB *sData = (SVertexNormalXB *)data;
      const STPair *st = &src.ST(0);
      int nVertex = src.NVertex();
      for (int i = 0; i < nVertex; i++)
      {
        CopyPos(sData->pos, (*pos)[i], vbuf._maxPos, vbuf._minPos, invScale);
        CopyNormal(sData->norm,(*norm)[i]);
        CopyUV(sData->t0, (*uv)[i], vbuf._maxUV, vbuf._minUV, invUV);
        CopyNormal(sData->s, st->s);
        CopyNormal(sData->t, st->t);
        sData++;
        st++;
      }
    }
    else
    {
      // Copy Positions, normals and UVs
      SVertexNormalXB *sData = (SVertexNormalXB *)data;
      int nVertex = src.NVertex();
      for (int i = 0; i < nVertex; i++)
      {
        CopyPos(sData->pos, (*pos)[i], vbuf._maxPos, vbuf._minPos, invScale);
        CopyNormal(sData->norm,(*norm)[i]);
        CopyUV(sData->t0, (*uv)[i], vbuf._maxUV, vbuf._minUV, invUV);
        sData++;
      }

      // Count the ST coordinates from polygons
      const UVPair *uv = &src.UV(0);
      const Vector3 *pos;
      if (src.OriginalPosValid())
      {
        pos = &src.OrigPos(0);
      }
      else
      {
        pos = &src.Pos(0);
      }
      for (Offset o = src.BeginFaces(); o < src.EndFaces(); src.NextFace(o))
      {
        const Poly &poly = src.Face(o);
        if (poly.N() < 3) continue;

        // Detect and skip degenerated face
        if (poly.GetVertex(0) == poly.GetVertex(poly.N() - 1)) continue;
        bool degenerated = false;
        for (int i = 1; i < poly.N(); i++)
        {
          if (poly.GetVertex(i - 1) == poly.GetVertex(i))
          {
            degenerated = true;
            break;
          }
        }
        if (degenerated) continue;

        // Count ST coordinates for vertices of this face
        for (int i = 2; i < poly.N(); i++)
        {
          // Get triangle vertices

          VertexIndex v0 = poly.GetVertex(0);
          VertexIndex v1 = poly.GetVertex(i - 1);
          VertexIndex v2 = poly.GetVertex(i);
          SVertexNormalXB *a, *b, *c;
          const UVPair *aUV, *bUV, *cUV;
          const Vector3 *aPos, *bPos, *cPos;
          Vector3 s, t;
          bool generationFailed = false;

          // A vertex
          a = &((SVertexNormalXB*)data)[v0];
          b = &((SVertexNormalXB*)data)[v1];
          c = &((SVertexNormalXB*)data)[v2];
          aUV = &uv[v0];
          bUV = &uv[v1];
          cUV = &uv[v2];
          aPos = &pos[v0];
          bPos = &pos[v1];
          cPos = &pos[v2];
          if (!GenerateSTOld(src.Norm(poly.GetVertex(0)), *aPos, *bPos, *cPos, *aUV, *bUV, *cUV, s, t))
          {
            generationFailed = true;
          }
          CopyNormal(a->s, s); CopyNormal(a->t, t);

          // B vertex
          a = &((SVertexNormalXB*)data)[v1];
          b = &((SVertexNormalXB*)data)[v2];
          c = &((SVertexNormalXB*)data)[v0];
          aUV = &uv[v1];
          bUV = &uv[v2];
          cUV = &uv[v0];
          aPos = &pos[v1];
          bPos = &pos[v2];
          cPos = &pos[v0];
          if (!GenerateSTOld(src.Norm(poly.GetVertex(i - 1)), *aPos, *bPos, *cPos, *aUV, *bUV, *cUV, s, t))
          {
            generationFailed = true;
          }
          CopyNormal(a->s, s); CopyNormal(a->t, t);

          // C vertex
          a = &((SVertexNormalXB*)data)[v2];
          b = &((SVertexNormalXB*)data)[v0];
          c = &((SVertexNormalXB*)data)[v1];
          aUV = &uv[v2];
          bUV = &uv[v0];
          cUV = &uv[v1];
          aPos = &pos[v2];
          bPos = &pos[v0];
          cPos = &pos[v1];
          if (!GenerateSTOld(src.Norm(poly.GetVertex(i)), *aPos, *bPos, *cPos, *aUV, *bUV, *cUV, s, t))
          {
            generationFailed = true;
          }
          CopyNormal(a->s, s); CopyNormal(a->t, t);

          // Report error if occured
          if (generationFailed)
          {
            if (src.ExplicitPoints())
            {
              LogF(
                "%s: Error while trying to generate ST for points: %d, %d, %d",
                (const char *)src.GetDebugName(),
                src.VertexToPoint(poly.GetVertex(0)),
                src.VertexToPoint(poly.GetVertex(i-1)),
                src.VertexToPoint(poly.GetVertex(i))
                );
            }
            else
            {
              LogF(
                "%s: Error while trying to generate ST for vertices: %d, %d, %d",
                (const char *)src.GetDebugName(),
                poly.GetVertex(0),
                poly.GetVertex(i-1),
                poly.GetVertex(i)
                );
            }
          }
        }
      }
    }

    _vBuffer->Unlock();

    _nShapes++;
  }
  else
  {
    vbuf._vAlloc = NULL;
  }

  return true;
}

bool VertexStaticDataXB::AddShapeNormalAlpha(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic)
{
  // when adding dynamic data, add only indices

  DWORD flags = NoSysLockFlag|D3DLOCK_NOOVERWRITE;

  if (_vertexSize != sizeof(SVertexNormalAlphaXB)) return false;

  if (!dynamic)
  {
    VertexHeapXB::HeapItem *item = _manager.Alloc(src.NVertex());
    if (!item)
    {
      return false; 
    }

    ShapeGeometryLock<> lock(&src);

    // Calculate the vertex buffer maximum and minimum Pos's
    src.CalculateMaxAndMinPos(vbuf._maxPos, vbuf._minPos);

    // Calculate the vertex buffer maximum and minimum UV's
    src.CalculateMaxAndMinUV(vbuf._maxUV, vbuf._minUV);

    vbuf._vAlloc = item;
    GEngineDD->ReportVBAlloc(vbuf._vAlloc->Size()*_vertexSize);
RetryV:
    BYTE *data = NULL;
    HRESULT err;
    {
      PROFILE_DX_SCOPE(3vbLS);
      err = _vBuffer->Lock
        (
        item->Memory()*sizeof(SVertexNormalAlphaXB),src.NVertex()*sizeof(SVertexNormalAlphaXB),
        &data,flags
        );
    }

    if (err!=D3D_OK || !data)
    {
      if (QFBank::FreeUnusedBanks(1024*1024)) goto RetryV;
#ifndef _XBOX
      ErrF("VB Lock failed, %s",DXGetErrorString8(err));
#else
      ErrF("VB Lock failed, %x",err);
#endif
      return false;
    }

    SVertexNormalAlphaXB *sData = (SVertexNormalAlphaXB *)data;
    const CondensedAutoArray<UVPair> *uv = &src.GetUVArray();
    const AutoArray<Vector3> *pos;
    const CondensedAutoArray<Vector3> *norm;
    if (src.OriginalPosValid())
    {
      pos = &src.GetOrigPosArray();
      norm = &src.GetOrigNormArray();
    }
    else
    {
      pos = &src.GetPosArray();
      norm = &src.GetNormArray();
    }
    Vector3 invScale = CalcInvScale(vbuf._maxPos, vbuf._minPos);
    UVPair invUV = CalcInvScale(vbuf._maxUV, vbuf._minUV);
    const STPair *st = &src.ST(0);
    const unsigned char *a = src.GetAlphaArray().Data();
    int nVertex = src.NVertex();
    for (int i = 0; i < nVertex; i++)
    {
      CopyPos(sData->pos, (*pos)[i], vbuf._maxPos, vbuf._minPos, invScale);
      CopyNormal(sData->norm,(*norm)[i]);
      CopyUV(sData->t0, (*uv)[i], vbuf._maxUV, vbuf._minUV, invUV);
      CopyNormal(sData->s, st->s);
      CopyNormal(sData->t, st->t);
      sData->alpha = *a;
      sData++;
      st++;
      a++;
    }

    _vBuffer->Unlock();

    _nShapes++;
  }
  else
  {
    vbuf._vAlloc = NULL;
  }

  return true;
}

bool VertexStaticDataXB::AddShapeShadowVolume(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic)
{

  // when adding dynamic data, add only indices

  DWORD flags = NoSysLockFlag|D3DLOCK_NOOVERWRITE;

  if (_vertexSize != sizeof(SVertexShadowVolumeXB)) return false;

  if (!dynamic)
  {
    VertexHeapXB::HeapItem *item = _manager.Alloc(src.NVertex());
    if (!item)
    {
      return false; 
    }

    ShapeGeometryLock<> lock(&src);

    // Calculate the vertex buffer maximum and minimum Pos's
    src.CalculateMaxAndMinPos(vbuf._maxPos, vbuf._minPos);

    // Set the vertex buffer maximum and minimum UV's to reasonable values
    vbuf._maxUV.u = 1.0f;
    vbuf._maxUV.v = 1.0f;
    vbuf._minUV.u = -1.0f;
    vbuf._minUV.v = -1.0f;

    vbuf._vAlloc = item;
    GEngineDD->ReportVBAlloc(vbuf._vAlloc->Size()*_vertexSize);
RetryV:
    BYTE *data = NULL;
    HRESULT err;
    {
      PROFILE_DX_SCOPE(3vbLS);
      err = _vBuffer->Lock
        (
        item->Memory()*sizeof(SVertexShadowVolumeXB),src.NVertex()*sizeof(SVertexShadowVolumeXB),
        &data,flags
        );
    }

    if (err!=D3D_OK || !data)
    {
      if (QFBank::FreeUnusedBanks(1024*1024)) goto RetryV;
#ifndef _XBOX
      ErrF("VB Lock failed, %s",DXGetErrorString8(err));
#else
      ErrF("VB Lock failed, %x",err);
#endif
      return false;
    }

    SVertexShadowVolumeXB *sData = (SVertexShadowVolumeXB *)data;
    const AutoArray<Vector3> *pos;
    const CondensedAutoArray<Vector3> *norm;
    if (src.OriginalPosValid())
    {
      pos = &src.GetOrigPosArray();
      norm = &src.GetOrigNormArray();
    }
    else
    {
      pos = &src.GetPosArray();
      norm = &src.GetNormArray();
    }
    Vector3 invScale = CalcInvScale(vbuf._maxPos, vbuf._minPos);
    int nVertex = src.NVertex();
    for (int i = 0; i < nVertex; i++)
    {
      CopyPos(sData->pos, (*pos)[i], vbuf._maxPos, vbuf._minPos, invScale);
      CopyNormal(sData->norm,(*norm)[i]);
      sData++;
    }
    _vBuffer->Unlock();

    _nShapes++;
  }
  else
  {
    Fail("Dynamic not supported for shadow volumes");
    vbuf._vAlloc = NULL;
  }

  return true;
}

bool VertexStaticDataXB::AddShapePoint(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic)
{

  // when adding dynamic data, add only indices

  DWORD flags = NoSysLockFlag|D3DLOCK_NOOVERWRITE;

  if (_vertexSize != sizeof(SVertexPointXB)) return false;

  if (!dynamic)
  {
    VertexHeapXB::HeapItem *item = _manager.Alloc(src.NVertex() * 4);
    if (!item)
    {
      return false; 
    }

    vbuf._vAlloc = item;
    GEngineDD->ReportVBAlloc(vbuf._vAlloc->Size()*_vertexSize);
RetryV:
    BYTE *data = NULL;
    HRESULT err;
    {
      PROFILE_DX_SCOPE(3vbLS);
      err = _vBuffer->Lock
        (
        item->Memory()*sizeof(SVertexPointXB),src.NVertex() * 4 * sizeof(SVertexPointXB),
        &data,flags
        );
    }

    if (err!=D3D_OK || !data)
    {
      if (QFBank::FreeUnusedBanks(1024*1024)) goto RetryV;
#ifndef _XBOX
      ErrF("VB Lock failed, %s",DXGetErrorString8(err));
#else
      ErrF("VB Lock failed, %x",err);
#endif
      return false;
    }

    SVertexPointXB *sData = (SVertexPointXB *)data;
    const AutoArray<Vector3> *pos;
    if (src.OriginalPosValid())
    {
      pos = &src.GetOrigPosArray();
    }
    else
    {
      pos = &src.GetPosArray();
    }
    int nVertex = src.NVertex();
    for (int i = 0; i < nVertex; i++)
    {
      // Get the accomodation value
      Color accom = HWhite * 0.5f;

      // use different brightness for different stars
      ClipFlags clip=src.Clip(i);
      int user=clip&ClipUserMask;
      float brightness=user*(1.0f/MaxUserValue/ClipUserStep);

      // Get the packed color
      accom.SetA(brightness);
      PackedColor pc = PackedColor(accom);

      // Set the four vertices
      CopyNormal(sData->pos,(*pos)[i].Normalized());
      sData->t0u = 0;
      sData->t0v = 0;
      sData->color = pc;
      sData++;

      CopyNormal(sData->pos,(*pos)[i].Normalized());
      sData->t0u = 0;
      sData->t0v = 255;
      sData->color = pc;
      sData++;

      CopyNormal(sData->pos,(*pos)[i].Normalized());
      sData->t0u = 255;
      sData->t0v = 0;
      sData->color = pc;
      sData++;

      CopyNormal(sData->pos,(*pos)[i].Normalized());
      sData->t0u = 255;
      sData->t0v = 255;
      sData->color = pc;
      sData++;
    }

    /*
    const AutoArray<Vector3> *pos;
    const CondensedAutoArray<Vector3> *norm;
    if (src.OriginalPosValid())
    {
    pos = &src.GetOrigPosArray();
    norm = &src.GetOrigNormArray();
    }
    else
    {
    pos = &src.GetPosArray();
    norm = &src.GetNormArray();
    }
    Vector3 invScale = CalcInvScale(vbuf._maxPos, vbuf._minPos);
    for (int i=0; i<src.NVertex(); i++)
    {
    CopyPos(sData->pos, (*pos)[i], vbuf._maxPos, vbuf._minPos, invScale);
    CopyNormal(sData->norm,(*norm)[i]);
    sData++;
    }
    */

    _vBuffer->Unlock();

    _nShapes++;
  }
  else
  {
    Fail("Dynamic not supported for shadow volumes");
    vbuf._vAlloc = NULL;
  }

  return true;
}

bool VertexStaticDataXB::AddShapeInstanced(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, int instancesCount)
{

  // when adding dynamic data, add only indices

  DWORD flags = NoSysLockFlag|D3DLOCK_NOOVERWRITE;

  if (_vertexSize != sizeof(SVertexInstancedXB)) return false;

  if (!dynamic)
  {
    VertexHeapXB::HeapItem *item = _manager.Alloc(src.NVertex() * instancesCount);
    if (!item)
    {
      return false; 
    }

    ShapeGeometryLock<> lock(&src);

    // Calculate the vertex buffer maximum and minimum Pos's
    src.CalculateMaxAndMinPos(vbuf._maxPos, vbuf._minPos);

    // Calculate the vertex buffer maximum and minimum UV's
    src.CalculateMaxAndMinUV(vbuf._maxUV, vbuf._minUV);

    vbuf._vAlloc = item;
    GEngineDD->ReportVBAlloc(vbuf._vAlloc->Size()*_vertexSize);
RetryV:
    BYTE *data = NULL;
    HRESULT err;
    {
      PROFILE_DX_SCOPE(3vbLS);
      err = _vBuffer->Lock
        (
        item->Memory() * sizeof(SVertexInstancedXB), src.NVertex() * instancesCount * sizeof(SVertexInstancedXB),
        &data, flags
        );
    }

    if (err!=D3D_OK || !data)
    {
      if (QFBank::FreeUnusedBanks(1024*1024)) goto RetryV;
#ifndef _XBOX
      ErrF("VB Lock failed, %s",DXGetErrorString8(err));
#else
      ErrF("VB Lock failed, %x",err);
#endif
      return false;
    }

    SVertexInstancedXB *sData = (SVertexInstancedXB *)data;
    const CondensedAutoArray<UVPair> *uv = &src.GetUVArray();
    const AutoArray<Vector3> *pos;
    const CondensedAutoArray<Vector3> *norm;
    if (src.OriginalPosValid())
    {
      pos = &src.GetOrigPosArray();
      norm = &src.GetOrigNormArray();
    }
    else
    {
      pos = &src.GetPosArray();
      norm = &src.GetNormArray();
    }
    Vector3 invScale = CalcInvScale(vbuf._maxPos, vbuf._minPos);
    UVPair invUV = CalcInvScale(vbuf._maxUV, vbuf._minUV);
    for (int instance = 0; instance < instancesCount; instance++)
    {
      int nVertex = src.NVertex();
      for (int i = 0; i < nVertex; i++)
      {
        CopyPos(sData->pos, (*pos)[i], vbuf._maxPos, vbuf._minPos,invScale);
        CopyNormal(sData->norm, (*norm)[i]);
        CopyUV(sData->t0, (*uv)[i], vbuf._maxUV, vbuf._minUV, invUV);
        sData->index = instance;
        //sData->t1 = *uv;
        sData++;
      }
    }
    _vBuffer->Unlock();
    _nShapes++;
  }
  else
  {
    vbuf._vAlloc = NULL;
  }

  return true;
}

bool VertexStaticDataXB::AddShapeNormalInstanced(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, int instancesCount)
{

  // when adding dynamic data, add only indices

  DWORD flags = NoSysLockFlag|D3DLOCK_NOOVERWRITE;

  if (_vertexSize != sizeof(SVertexNormalInstancedXB)) return false;

  if (!dynamic)
  {
    VertexHeapXB::HeapItem *item = _manager.Alloc(src.NVertex() * instancesCount);
    if (!item)
    {
      return false; 
    }

    ShapeGeometryLock<> lock(&src);

    // Calculate the vertex buffer maximum and minimum Pos's
    src.CalculateMaxAndMinPos(vbuf._maxPos, vbuf._minPos);

    // Calculate the vertex buffer maximum and minimum UV's
    src.CalculateMaxAndMinUV(vbuf._maxUV, vbuf._minUV);

    vbuf._vAlloc = item;
    GEngineDD->ReportVBAlloc(vbuf._vAlloc->Size()*_vertexSize);
RetryV:
    BYTE *data = NULL;
    HRESULT err;
    {
      PROFILE_DX_SCOPE(3vbLS);
      err = _vBuffer->Lock
        (
        item->Memory()*sizeof(SVertexNormalInstancedXB),src.NVertex() * instancesCount * sizeof(SVertexNormalInstancedXB),
        &data,flags
        );
    }

    if (err!=D3D_OK || !data)
    {
      if (QFBank::FreeUnusedBanks(1024*1024)) goto RetryV;
#ifndef _XBOX
      ErrF("VB Lock failed, %s",DXGetErrorString8(err));
#else
      ErrF("VB Lock failed, %x",err);
#endif
      return false;
    }

    const CondensedAutoArray<UVPair> *uv = &src.GetUVArray();
    const AutoArray<Vector3> *pos;
    const CondensedAutoArray<Vector3> *norm;
    if (src.OriginalPosValid())
    {
      pos = &src.GetOrigPosArray();
      norm = &src.GetOrigNormArray();
    }
    else
    {
      pos = &src.GetPosArray();
      norm = &src.GetNormArray();
    }
    Vector3 invScale = CalcInvScale(vbuf._maxPos, vbuf._minPos);
    UVPair invUV = CalcInvScale(vbuf._maxUV, vbuf._minUV);

    if (src.IsSTPresent())
    {
      SVertexNormalInstancedXB *sData = (SVertexNormalInstancedXB*)data;
      int nVertex = src.NVertex();
      for (int instance = 0; instance < instancesCount; instance++)
      {
        const STPair *st = &src.ST(0);
        for (int i = 0; i < nVertex; i++)
        {
          CopyPos(sData->pos, (*pos)[i], vbuf._maxPos, vbuf._minPos,invScale);
          CopyNormal(sData->norm,(*norm)[i]);
          CopyUV(sData->t0, (*uv)[i], vbuf._maxUV, vbuf._minUV, invUV);
          CopyNormal(sData->s, st->s);
          CopyNormal(sData->t, st->t);
          sData->index = instance;
          sData++;
          st++;
        }
      }
    }
    else
    {
      Fail("ST coordinated should already be calculated");
      // Copy Positions, normals and UVs
      SVertexNormalInstancedXB *sData = (SVertexNormalInstancedXB *)data;
      int nVertex = src.NVertex();
      for (int i = 0; i < nVertex; i++)
      {
        CopyPos(sData->pos, (*pos)[i], vbuf._maxPos, vbuf._minPos,invScale);
        CopyNormal(sData->norm,(*norm)[i]);
        CopyUV(sData->t0, (*uv)[i], vbuf._maxUV, vbuf._minUV, invUV);
        sData++;
      }

      // Count the ST coordinates from polygons
      const UVPair *uv = &src.UV(0);
      const Vector3 *pos;
      if (src.OriginalPosValid())
      {
        pos = &src.OrigPos(0);
      }
      else
      {
        pos = &src.Pos(0);
      }
      for (Offset o = src.BeginFaces(); o < src.EndFaces(); src.NextFace(o))
      {
        const Poly &poly = src.Face(o);
        if (poly.N() < 3) continue;

        // Detect and skip degenerated face
        if (poly.GetVertex(0) == poly.GetVertex(poly.N() - 1)) continue;
        bool degenerated = false;
        for (int i = 1; i < poly.N(); i++)
        {
          if (poly.GetVertex(i - 1) == poly.GetVertex(i))
          {
            degenerated = true;
            break;
          }
        }
        if (degenerated) continue;

        // Count ST coordinates for vertices of this face
        for (int i = 2; i < poly.N(); i++)
        {
          // Get triangle vertices

          VertexIndex v0 = poly.GetVertex(0);
          VertexIndex v1 = poly.GetVertex(i - 1);
          VertexIndex v2 = poly.GetVertex(i);
          SVertexNormalInstancedXB *a, *b, *c;
          const UVPair *aUV, *bUV, *cUV;
          const Vector3 *aPos, *bPos, *cPos;
          Vector3 s, t;
          bool generationFailed = false;

          // A vertex
          a = &((SVertexNormalInstancedXB*)data)[v0];
          b = &((SVertexNormalInstancedXB*)data)[v1];
          c = &((SVertexNormalInstancedXB*)data)[v2];
          aUV = &uv[v0];
          bUV = &uv[v1];
          cUV = &uv[v2];
          aPos = &pos[v0];
          bPos = &pos[v1];
          cPos = &pos[v2];
          if (!GenerateSTOld(src.Norm(poly.GetVertex(0)), *aPos, *bPos, *cPos, *aUV, *bUV, *cUV, s, t))
          {
            generationFailed = true;
          }
          CopyNormal(a->s, s); CopyNormal(a->t, t);

          // B vertex
          a = &((SVertexNormalInstancedXB*)data)[v1];
          b = &((SVertexNormalInstancedXB*)data)[v2];
          c = &((SVertexNormalInstancedXB*)data)[v0];
          aUV = &uv[v1];
          bUV = &uv[v2];
          cUV = &uv[v0];
          aPos = &pos[v1];
          bPos = &pos[v2];
          cPos = &pos[v0];
          if (!GenerateSTOld(src.Norm(poly.GetVertex(i - 1)), *aPos, *bPos, *cPos, *aUV, *bUV, *cUV, s, t))
          {
            generationFailed = true;
          }
          CopyNormal(a->s, s); CopyNormal(a->t, t);

          // C vertex
          a = &((SVertexNormalInstancedXB*)data)[v2];
          b = &((SVertexNormalInstancedXB*)data)[v0];
          c = &((SVertexNormalInstancedXB*)data)[v1];
          aUV = &uv[v2];
          bUV = &uv[v0];
          cUV = &uv[v1];
          aPos = &pos[v2];
          bPos = &pos[v0];
          cPos = &pos[v1];
          if (!GenerateSTOld(src.Norm(poly.GetVertex(i)), *aPos, *bPos, *cPos, *aUV, *bUV, *cUV, s, t))
          {
            generationFailed = true;
          }
          CopyNormal(a->s, s); CopyNormal(a->t, t);

          // Report error if occured
          if (generationFailed)
          {
            if (src.ExplicitPoints())
            {
              LogF(
                "%s: Error while trying to generate ST for points: %d, %d, %d",
                (const char *)src.GetDebugName(),
                src.VertexToPoint(poly.GetVertex(0)),
                src.VertexToPoint(poly.GetVertex(i-1)),
                src.VertexToPoint(poly.GetVertex(i))
                );
            }
            else
            {
              LogF(
                "%s: Error while trying to generate ST for vertices: %d, %d, %d",
                (const char *)src.GetDebugName(),
                poly.GetVertex(0),
                poly.GetVertex(i-1),
                poly.GetVertex(i)
                );
            }
          }
        }
      }
      // Duplicate the first instance "instancesCount" times
      sData = (SVertexNormalInstancedXB *)data;
      for (int i = 1; i < instancesCount; i++)
      {
        memcpy(&sData[src.NVertex() * i], sData, sizeof(SVertexNormalInstancedXB) * src.NVertex());
      }

      // Fill out the proper instance index
      for (int i = 0; i < instancesCount; i++)
      {
        int nVertex = src.NVertex();
        for (int j = 0; j < nVertex; j++)
        {
          sData[i * nVertex + j].index = i;
        }
      }
    }


    _vBuffer->Unlock();

    _nShapes++;
  }
  else
  {
    vbuf._vAlloc = NULL;
  }

  return true;
}

bool VertexStaticDataXB::AddShapeShadowVolumeInstanced(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, int instancesCount)
{

  // when adding dynamic data, add only indices

  DWORD flags = NoSysLockFlag|D3DLOCK_NOOVERWRITE;

  if (_vertexSize != sizeof(SVertexShadowVolumeInstancedXB)) return false;

  if (!dynamic)
  {
    VertexHeapXB::HeapItem *item = _manager.Alloc(src.NVertex() * instancesCount);
    if (!item)
    {
      return false; 
    }

    ShapeGeometryLock<> lock(&src);

    // Calculate the vertex buffer maximum and minimum Pos's
    src.CalculateMaxAndMinPos(vbuf._maxPos, vbuf._minPos);

    // Set the vertex buffer maximum and minimum UV's to reasonable values
    vbuf._maxUV.u = 1.0f;
    vbuf._maxUV.v = 1.0f;
    vbuf._minUV.u = -1.0f;
    vbuf._minUV.v = -1.0f;

    vbuf._vAlloc = item;
    GEngineDD->ReportVBAlloc(vbuf._vAlloc->Size()*_vertexSize);
RetryV:
    BYTE *data = NULL;
    HRESULT err;
    {
      PROFILE_DX_SCOPE(3vbLS);
      err = _vBuffer->Lock
        (
        item->Memory()*sizeof(SVertexShadowVolumeInstancedXB),src.NVertex() * instancesCount * sizeof(SVertexShadowVolumeInstancedXB),
        &data,flags
        );
    }

    if (err!=D3D_OK || !data)
    {
      if (QFBank::FreeUnusedBanks(1024*1024)) goto RetryV;
#ifndef _XBOX
      ErrF("VB Lock failed, %s",DXGetErrorString8(err));
#else
      ErrF("VB Lock failed, %x",err);
#endif
      return false;
    }

    SVertexShadowVolumeInstancedXB *sData = (SVertexShadowVolumeInstancedXB *)data;
    const AutoArray<Vector3> *pos;
    const CondensedAutoArray<Vector3> *norm;
    if (src.OriginalPosValid())
    {
      pos = &src.GetOrigPosArray();
      norm = &src.GetOrigNormArray();
    }
    else
    {
      pos = &src.GetPosArray();
      norm = &src.GetNormArray();
    }
    Vector3 invScale = CalcInvScale(vbuf._maxPos, vbuf._minPos);
    for (int instance = 0; instance < instancesCount; instance++)
    {
      int nVertex = src.NVertex();
      for (int i = 0; i < nVertex; i++)
      {
        CopyPos(sData->pos, (*pos)[i], vbuf._maxPos, vbuf._minPos, invScale);
        CopyNormal(sData->norm, (*norm)[i]);
        sData->index = instance;
        sData++;
      }
    }

    _vBuffer->Unlock();

    _nShapes++;
  }
  else
  {
    Fail("Dynamic not supported for shadow volumes");
    vbuf._vAlloc = NULL;
  }

  return true;
}

bool VertexStaticDataXB::AddShapeSpriteInstanced(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, int instancesCount)
{

  // when adding dynamic data, add only indices

  DWORD flags = NoSysLockFlag|D3DLOCK_NOOVERWRITE;

  if (_vertexSize != sizeof(SVertexSpriteInstancedXB)) return false;

  if (!dynamic)
  {
    VertexHeapXB::HeapItem *item = _manager.Alloc(4 * instancesCount);
    if (!item)
    {
      return false; 
    }

    vbuf._vAlloc = item;
    GEngineDD->ReportVBAlloc(vbuf._vAlloc->Size()*_vertexSize);
RetryV:
    BYTE *data = NULL;
    HRESULT err;
    {
      PROFILE_DX_SCOPE(3vbLS);
      err = _vBuffer->Lock
        (
        item->Memory() * sizeof(SVertexSpriteInstancedXB), 4 * instancesCount * sizeof(SVertexSpriteInstancedXB),
        &data, flags
        );
    }

    if (err!=D3D_OK || !data)
    {
      if (QFBank::FreeUnusedBanks(1024*1024)) goto RetryV;
#ifndef _XBOX
      ErrF("VB Lock failed, %s",DXGetErrorString8(err));
#else
      ErrF("VB Lock failed, %x",err);
#endif
      return false;
    }

    SVertexSpriteInstancedXB *sData = (SVertexSpriteInstancedXB *)data;
    for (int instance = 0; instance < instancesCount; instance++)
    {
      sData[instance * 4 + 0].t0u = 0;
      sData[instance * 4 + 0].t0v = 0;
      sData[instance * 4 + 0].index = instance;

      sData[instance * 4 + 1].t0u = 0;
      sData[instance * 4 + 1].t0v = 255;
      sData[instance * 4 + 1].index = instance;

      sData[instance * 4 + 2].t0u = 255;
      sData[instance * 4 + 2].t0v = 0;
      sData[instance * 4 + 2].index = instance;

      sData[instance * 4 + 3].t0u = 255;
      sData[instance * 4 + 3].t0v = 255;
      sData[instance * 4 + 3].index = instance;
    }

    _vBuffer->Unlock();
    _nShapes++;
  }
  else
  {
    vbuf._vAlloc = NULL;
  }

  return true;

}

#include "../rtAnimation.hpp"

bool VertexStaticDataXB::AddShapeSkinned(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic)
{

  // when adding dynamic data, add only indices

  DWORD flags = NoSysLockFlag|D3DLOCK_NOOVERWRITE;

  if (_vertexSize != sizeof(SVertexSkinnedXB)) return false;

  if (!dynamic)
  {
    VertexHeapXB::HeapItem *item = _manager.Alloc(src.NVertex());
    if (!item)
    {
      /*
      Log("No more space in vertex buffer - %d shapes",_nShapes);
      Log
      (
      "  used: V %d of %d",
      _manager.TotalBusy(),_manager.Size()
      );
      */
      return false; 
    }

    ShapeGeometryLock<> lock(&src);

    // Calculate the vertex buffer maximum and minimum Pos's
    src.CalculateMaxAndMinPos(vbuf._maxPos, vbuf._minPos);

    // Calculate the vertex buffer maximum and minimum UV's
    src.CalculateMaxAndMinUV(vbuf._maxUV, vbuf._minUV);

    vbuf._vAlloc = item;
    GEngineDD->ReportVBAlloc(vbuf._vAlloc->Size()*_vertexSize);
RetryV:
    BYTE *data = NULL;
    HRESULT err;
    {
      PROFILE_DX_SCOPE(3vbLS);
      err = _vBuffer->Lock
        (
        item->Memory()*sizeof(SVertexSkinnedXB),src.NVertex()*sizeof(SVertexSkinnedXB),
        &data,flags
        );
    }

    if (err!=D3D_OK || !data)
    {
      if (QFBank::FreeUnusedBanks(1024*1024)) goto RetryV;
#ifndef _XBOX
      ErrF("VB Lock failed, %s",DXGetErrorString8(err));
#else
      ErrF("VB Lock failed, %x",err);
#endif
      return false;
    }

    SVertexSkinnedXB *sData = (SVertexSkinnedXB *)data;
    const CondensedAutoArray<UVPair> *uv = &src.GetUVArray();
    const AutoArray<Vector3> *pos;
    const CondensedAutoArray<Vector3> *norm;
    if (src.OriginalPosValid())
    {
      pos = &src.GetOrigPosArray();
      norm = &src.GetOrigNormArray();
    }
    else
    {
      pos = &src.GetPosArray();
      norm = &src.GetNormArray();
    }
    Vector3 invScale = CalcInvScale(vbuf._maxPos, vbuf._minPos);
    UVPair invUV = CalcInvScale(vbuf._maxUV, vbuf._minUV);
    const AutoArray<AnimationRTWeight> &a = src.GetVertexBoneRef();

    if (a.Size() > 0)
    {
      int nVertex = src.NVertex();
      for (int i = 0; i < nVertex; i++)
      {
        CopyPos(sData[i].pos, (*pos)[i], vbuf._maxPos, vbuf._minPos, invScale);
        CopyNormal(sData[i].norm,(*norm)[i]);
        int wsize = a[i].Size();
        // Copy indices and weights
        for (int w = 0; w < wsize; w++)
        {
          sData[i].indices[w] = GetSubSkeletonIndex(a[i][w].GetSel());
          sData[i].weights[w] = a[i][w].GetWeight8b();
        }
        // Zero remaining indices and weights
        for (int w = wsize; w < 4; w++)
        {
          sData[i].indices[w] = 0;
          sData[i].weights[w] = 0;
        }
        CopyUV(sData[i].t0, (*uv)[i], vbuf._maxUV, vbuf._minUV, invUV);
      }
    }
    else
    {
      RptF("Warning: Object is animated, but no skeleton is defined");
      int nVertex = src.NVertex();
      for (int i = 0; i < nVertex; i++)
      {
        CopyPos(sData[i].pos, (*pos)[i], vbuf._maxPos, vbuf._minPos, invScale);
        CopyNormal(sData[i].norm,(*norm)[i]);
        sData[i].indices[0] = 0;
        sData[i].weights[0] = 0;
        sData[i].indices[1] = sData[i].indices[2] = sData[i].indices[3] = 0;
        sData[i].weights[1] = sData[i].weights[2] = sData[i].weights[3] = 0;
        CopyUV(sData[i].t0, (*uv)[i], vbuf._maxUV, vbuf._minUV, invUV);
      }
    }

    _vBuffer->Unlock();

    _nShapes++;
  }
  else
  {
    vbuf._vAlloc = NULL;
  }

  return true;
}

bool VertexStaticDataXB::AddShapeNormalSkinned(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic)
{

  // when adding dynamic data, add only indices

  DWORD flags = NoSysLockFlag|D3DLOCK_NOOVERWRITE;

  if (_vertexSize != sizeof(SVertexNormalSkinnedXB)) return false;

  if (!dynamic)
  {
    VertexHeapXB::HeapItem *item = _manager.Alloc(src.NVertex());
    if (!item)
    {
      /*
      Log("No more space in vertex buffer - %d shapes",_nShapes);
      Log
      (
      "  used: V %d of %d",
      _manager.TotalBusy(),_manager.Size()
      );
      */
      return false; 
    }

    ShapeGeometryLock<> lock(&src);

    // Calculate the vertex buffer maximum and minimum Pos's
    src.CalculateMaxAndMinPos(vbuf._maxPos, vbuf._minPos);

    // Calculate the vertex buffer maximum and minimum UV's
    src.CalculateMaxAndMinUV(vbuf._maxUV, vbuf._minUV);

    vbuf._vAlloc = item;
    GEngineDD->ReportVBAlloc(vbuf._vAlloc->Size()*_vertexSize);
RetryV:
    BYTE *data = NULL;
    HRESULT err;
    {
      PROFILE_DX_SCOPE(3vbLS);
      err = _vBuffer->Lock
        (
        item->Memory()*sizeof(SVertexNormalSkinnedXB),src.NVertex()*sizeof(SVertexNormalSkinnedXB),
        &data,flags
        );
    }

    if (err!=D3D_OK || !data)
    {
      if (QFBank::FreeUnusedBanks(1024*1024)) goto RetryV;
#ifndef _XBOX
      ErrF("VB Lock failed, %s",DXGetErrorString8(err));
#else
      ErrF("VB Lock failed, %x",err);
#endif
      return false;
    }

    SVertexNormalSkinnedXB *sData = (SVertexNormalSkinnedXB *)data;
    const CondensedAutoArray<UVPair> *uv = &src.GetUVArray();
    const AutoArray<Vector3> *pos;
    const CondensedAutoArray<Vector3> *norm;
    if (src.OriginalPosValid())
    {
      pos = &src.GetOrigPosArray();
      norm = &src.GetOrigNormArray();
    }
    else
    {
      pos = &src.GetPosArray();
      norm = &src.GetNormArray();
    }
    Vector3 invScale = CalcInvScale(vbuf._maxPos, vbuf._minPos);
    UVPair invUV = CalcInvScale(vbuf._maxUV, vbuf._minUV);
    const AutoArray<AnimationRTWeight> &a = src.GetVertexBoneRef();

    if (src.IsSTPresent())
    {
      const STPair *st = &src.ST(0);
      if (a.Size() > 0)
      {
        int nVertex = src.NVertex();
        for (int i = 0; i < nVertex; i++)
        {
          CopyPos(sData[i].pos, (*pos)[i], vbuf._maxPos, vbuf._minPos, invScale);
          CopyNormal(sData[i].norm,(*norm)[i]);
          int wsize = a[i].Size();
          // Copy indices and weights
          for (int w = 0; w < wsize; w++)
          {
            sData[i].indices[w] = GetSubSkeletonIndex(a[i][w].GetSel());
            sData[i].weights[w] = a[i][w].GetWeight8b();
          }
          // Zero remaining indices and weights
          for (int w = wsize; w < 4; w++)
          {
            sData[i].indices[w] = 0;
            sData[i].weights[w] = 0;
          }
          CopyUV(sData[i].t0, (*uv)[i], vbuf._maxUV, vbuf._minUV, invUV);
          CopyNormal(sData[i].s, st->s);
          CopyNormal(sData[i].t, st->t);
          st++;
        }
      }
      else
      {
        RptF("Warning: Object is animated, but no skeleton is defined");
        int nVertex = src.NVertex();
        for (int i = 0; i < nVertex; i++)
        {
          CopyPos(sData[i].pos, (*pos)[i], vbuf._maxPos, vbuf._minPos, invScale);
          CopyNormal(sData[i].norm,(*norm)[i]);
          sData[i].indices[0] = 0;
          sData[i].weights[0] = 0;
          sData[i].indices[1] = sData[i].indices[2] = sData[i].indices[3] = 0;
          sData[i].weights[1] = sData[i].weights[2] = sData[i].weights[3] = 0;
          CopyUV(sData[i].t0, (*uv)[i], vbuf._maxUV, vbuf._minUV, invUV);
          CopyNormal(sData[i].s, st->s);
          CopyNormal(sData[i].t, st->t);
          st++;
        }
      }
    }
    else
    {
      // Copy Positions, normals and UVs
      if (a.Size() > 0)
      {
        int nVertex = src.NVertex();
        for (int i = 0; i < nVertex; i++)
        {
          CopyPos(sData[i].pos, (*pos)[i], vbuf._maxPos, vbuf._minPos, invScale);
          CopyNormal(sData[i].norm,(*norm)[i]);
          int wsize = a[i].Size();
          // Copy indices and weights
          for (int w = 0; w < wsize; w++)
          {
            sData[i].indices[w] = GetSubSkeletonIndex(a[i][w].GetSel());
            sData[i].weights[w] = a[i][w].GetWeight8b();
          }
          // Zero remaining indices and weights
          for (int w = wsize; w < 4; w++)
          {
            sData[i].indices[w] = 0;
            sData[i].weights[w] = 0;
          }
          CopyUV(sData[i].t0, (*uv)[i], vbuf._maxUV, vbuf._minUV, invUV);
        }
      }
      else
      {
        RptF("Warning: Object is animated, but no skeleton is defined");
        int nVertex = src.NVertex();
        for (int i = 0; i < nVertex; i++)
        {
          CopyPos(sData[i].pos, (*pos)[i], vbuf._maxPos, vbuf._minPos, invScale);
          CopyNormal(sData[i].norm,(*norm)[i]);
          sData[i].indices[0] = 0;
          sData[i].weights[0] = 0;
          sData[i].indices[1] = sData[i].indices[2] = sData[i].indices[3] = 0;
          sData[i].weights[1] = sData[i].weights[2] = sData[i].weights[3] = 0;
          CopyUV(sData[i].t0, (*uv)[i], vbuf._maxUV, vbuf._minUV, invUV);
        }
      }

      // Count the ST coordinates from polygons
      const UVPair *uv = &src.UV(0);
      const Vector3 *pos;
      if (src.OriginalPosValid())
      {
        pos = &src.OrigPos(0);
      }
      else
      {
        pos = &src.Pos(0);
      }
      for (Offset o = src.BeginFaces(); o < src.EndFaces(); src.NextFace(o))
      {
        const Poly &poly = src.Face(o);
        if (poly.N() < 3) continue;

        // Detect and skip degenerated face
        if (poly.GetVertex(0) == poly.GetVertex(poly.N() - 1)) continue;
        bool degenerated = false;
        for (int i = 1; i < poly.N(); i++)
        {
          if (poly.GetVertex(i - 1) == poly.GetVertex(i))
          {
            degenerated = true;
            break;
          }
        }
        if (degenerated) continue;

        // Count ST coordinates for vertices of this face
        for (int i = 2; i < poly.N(); i++)
        {
          // Get triangle vertices

          VertexIndex v0 = poly.GetVertex(0);
          VertexIndex v1 = poly.GetVertex(i - 1);
          VertexIndex v2 = poly.GetVertex(i);
          SVertexNormalSkinnedXB *a, *b, *c;
          const UVPair *aUV, *bUV, *cUV;
          const Vector3 *aPos, *bPos, *cPos;
          Vector3 s, t;
          bool generationFailed = false;

          // A vertex
          a = &((SVertexNormalSkinnedXB*)data)[v0];
          b = &((SVertexNormalSkinnedXB*)data)[v1];
          c = &((SVertexNormalSkinnedXB*)data)[v2];
          aUV = &uv[v0];
          bUV = &uv[v1];
          cUV = &uv[v2];
          aPos = &pos[v0];
          bPos = &pos[v1];
          cPos = &pos[v2];
          if (!GenerateSTOld(src.Norm(poly.GetVertex(0)), *aPos, *bPos, *cPos, *aUV, *bUV, *cUV, s, t))
          {
            generationFailed = true;
          }
          CopyNormal(a->s, s); CopyNormal(a->t, t);

          // B vertex
          a = &((SVertexNormalSkinnedXB*)data)[v1];
          b = &((SVertexNormalSkinnedXB*)data)[v2];
          c = &((SVertexNormalSkinnedXB*)data)[v0];
          aUV = &uv[v1];
          bUV = &uv[v2];
          cUV = &uv[v0];
          aPos = &pos[v1];
          bPos = &pos[v2];
          cPos = &pos[v0];
          if (!GenerateSTOld(src.Norm(poly.GetVertex(i - 1)), *aPos, *bPos, *cPos, *aUV, *bUV, *cUV, s, t))
          {
            generationFailed = true;
          }
          CopyNormal(a->s, s); CopyNormal(a->t, t);

          // C vertex
          a = &((SVertexNormalSkinnedXB*)data)[v2];
          b = &((SVertexNormalSkinnedXB*)data)[v0];
          c = &((SVertexNormalSkinnedXB*)data)[v1];
          aUV = &uv[v2];
          bUV = &uv[v0];
          cUV = &uv[v1];
          aPos = &pos[v2];
          bPos = &pos[v0];
          cPos = &pos[v1];
          if (!GenerateSTOld(src.Norm(poly.GetVertex(i)), *aPos, *bPos, *cPos, *aUV, *bUV, *cUV, s, t))
          {
            generationFailed = true;
          }
          CopyNormal(a->s, s); CopyNormal(a->t, t);

          // Report error if occured
          if (generationFailed)
          {
            if (src.ExplicitPoints())
            {
              LogF(
                "%s: Error while trying to generate ST for points: %d, %d, %d",
                (const char *)src.GetDebugName(),
                src.VertexToPoint(poly.GetVertex(0)),
                src.VertexToPoint(poly.GetVertex(i-1)),
                src.VertexToPoint(poly.GetVertex(i))
                );
            }
            else
            {
              LogF(
                "%s: Error while trying to generate ST for vertices: %d, %d, %d",
                (const char *)src.GetDebugName(),
                poly.GetVertex(0),
                poly.GetVertex(i-1),
                poly.GetVertex(i)
                );
            }
          }
        }
      }
    }

    _vBuffer->Unlock();

    _nShapes++;
  }
  else
  {
    vbuf._vAlloc = NULL;
  }

  return true;
}

void FillOutShadowVolumeVertexWithNeighbors(VertexIndex vertex, VertexIndex vertexA, VertexIndex vertexB, SVertexShadowVolumeSkinnedXB* sData, const Vector3 *pos, Vector3Par maxPos, Vector3Par minPos, Vector3 invScale)
{
  // Should we get a degenerated triangle, then skip it
  // (it will be filled another time by a non-degenerated neighbor)
  if ((pos[vertex] == pos[vertexA]) || (pos[vertexA] == pos[vertexB]) || (pos[vertexB] == pos[vertex])) return;

  CopyPos(sData[vertex].pos, pos[vertex], maxPos, minPos, invScale);
  CopyPos(sData[vertex].posA, pos[vertexA], maxPos, minPos, invScale);
  CopyPos(sData[vertex].posB, pos[vertexB], maxPos, minPos, invScale);

  // Vertex
  sData[vertex].indices[0] = sData[vertex].indices[1] = sData[vertex].indices[2] = sData[vertex].indices[3] = 0;
  sData[vertex].weights[0] = sData[vertex].weights[1] = sData[vertex].weights[2] = sData[vertex].weights[3] = 0;

  // VertexA
  sData[vertex].indicesA[0] = sData[vertex].indicesA[1] = sData[vertex].indicesA[2] = sData[vertex].indicesA[3] = 0;
  sData[vertex].weightsA[0] = sData[vertex].weightsA[1] = sData[vertex].weightsA[2] = sData[vertex].weightsA[3] = 0;

  // VertexB
  sData[vertex].indicesB[0] = sData[vertex].indicesB[1] = sData[vertex].indicesB[2] = sData[vertex].indicesB[3] = 0;
  sData[vertex].weightsB[0] = sData[vertex].weightsB[1] = sData[vertex].weightsB[2] = sData[vertex].weightsB[3] = 0;
}

bool VertexStaticDataXB::AddShapeShadowVolumeSkinned(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic)
{
  DWORD flags = NoSysLockFlag|D3DLOCK_NOOVERWRITE;

  if (_vertexSize != sizeof(SVertexShadowVolumeSkinnedXB)) return false;

  if (!dynamic)
  {
    VertexHeapXB::HeapItem *item = _manager.Alloc(src.NVertex());
    if (!item)
    {
      return false; 
    }

    ShapeGeometryLock<> lock(&src);

    // Calculate the vertex buffer maximum and minimum Pos's
    src.CalculateMaxAndMinPos(vbuf._maxPos, vbuf._minPos);

    // Set the vertex buffer maximum and minimum UV's to reasonable values
    vbuf._maxUV.u = 1.0f;
    vbuf._maxUV.v = 1.0f;
    vbuf._minUV.u = -1.0f;
    vbuf._minUV.v = -1.0f;

    vbuf._vAlloc = item;
    GEngineDD->ReportVBAlloc(vbuf._vAlloc->Size()*_vertexSize);
RetryV:
    BYTE *data = NULL;
    HRESULT err;
    {
      PROFILE_DX_SCOPE(3vbLS);
      err = _vBuffer->Lock
        (
        item->Memory()*sizeof(SVertexShadowVolumeSkinnedXB),src.NVertex()*sizeof(SVertexShadowVolumeSkinnedXB),
        &data,flags
        );
    }

    if (err!=D3D_OK || !data)
    {
      if (QFBank::FreeUnusedBanks(1024*1024)) goto RetryV;
#ifndef _XBOX
      ErrF("VB Lock failed, %s",DXGetErrorString8(err));
#else
      ErrF("VB Lock failed, %x",err);
#endif
      return false;
    }

    SVertexShadowVolumeSkinnedXB *sData = (SVertexShadowVolumeSkinnedXB *)data;
    const Vector3 *pos = &src.Pos(0);
    if (src.OriginalPosValid())
    {
      pos = &src.OrigPos(0);
    }

    const AutoArray<AnimationRTWeight> &a = src.GetVertexBoneRef();

    Vector3 invScale = CalcInvScale(vbuf._maxPos, vbuf._minPos);

    if (a.Size() > 0)
    {
      const AutoArray<VertexNeighborInfo> &neighborBoneRef = src.GetNeighborBoneRef();
      int nVertex = src.NVertex();
      for (int i = 0; i < nVertex; i++)
      {
        CopyPos(sData[i].pos, pos[i], vbuf._maxPos, vbuf._minPos, invScale);
        CopyPos(sData[i].posA, pos[neighborBoneRef[i]._posA], vbuf._maxPos, vbuf._minPos, invScale);
        CopyPos(sData[i].posB, pos[neighborBoneRef[i]._posB], vbuf._maxPos, vbuf._minPos, invScale);

        int wsize;

        // Original vertex
        wsize = a[i].Size();
        // Copy indices and weights
        for (int w = 0; w < wsize; w++)
        {
          sData[i].indices[w] = GetSubSkeletonIndex(a[i][w].GetSel());
          sData[i].weights[w] = a[i][w].GetWeight8b();
        }
        // Zero remaining indices and weights
        for (int w = wsize; w < 4; w++)
        {
          sData[i].indices[w] = 0;
          sData[i].weights[w] = 0;
        }

        // Vertex A
        wsize = neighborBoneRef[i]._rtwA.Size();
        // Copy indices and weights
        for (int w = 0; w < wsize; w++)
        {
          sData[i].indicesA[w] = GetSubSkeletonIndex(neighborBoneRef[i]._rtwA[w].GetSel());
          sData[i].weightsA[w] = neighborBoneRef[i]._rtwA[w].GetWeight8b();
        }
        // Zero remaining indices and weights
        for (int w = wsize; w < 4; w++)
        {
          sData[i].indicesA[w] = 0;
          sData[i].weightsA[w] = 0;
        }

        // Vertex B
        wsize = neighborBoneRef[i]._rtwB.Size();
        // Copy indices and weights
        for (int w = 0; w < wsize; w++)
        {
          sData[i].indicesB[w] = GetSubSkeletonIndex(neighborBoneRef[i]._rtwB[w].GetSel());
          sData[i].weightsB[w] = neighborBoneRef[i]._rtwB[w].GetWeight8b();
        }
        // Zero remaining indices and weights
        for (int w = wsize; w < 4; w++)
        {
          sData[i].indicesB[w] = 0;
          sData[i].weightsB[w] = 0;
        }
      }
    }
    else
    {
      RptF("Warning: Object is animated, but no skeleton is defined");
      // Go through all the sections
      for (int i = 0; i < src.NSections(); i++)
      {
        const ShapeSection &sec = src.GetSection(i);

        // Skip sections not designed for drawing
        if (sec.Special() & (IsHidden|IsHiddenProxy)) continue;

        // Go through all the faces in the section
        for (Offset o = sec.beg; o < sec.end; src.NextFace(o))
        {
          const Poly &face = src.Face(o);
          if (face.N() != 3)
          {
            LogF("Error: Shadow volume polygon doesn't have 3 vertices - it's vertices remained uninitialized");
            continue;
          }

          // Fill out three vertices
          FillOutShadowVolumeVertexWithNeighbors(face.GetVertex(0), face.GetVertex(1), face.GetVertex(2), sData, pos, vbuf._maxPos, vbuf._minPos, invScale);
          FillOutShadowVolumeVertexWithNeighbors(face.GetVertex(1), face.GetVertex(2), face.GetVertex(0), sData, pos, vbuf._maxPos, vbuf._minPos, invScale);
          FillOutShadowVolumeVertexWithNeighbors(face.GetVertex(2), face.GetVertex(0), face.GetVertex(1), sData, pos, vbuf._maxPos, vbuf._minPos, invScale);
        }
      }
    }

    _vBuffer->Unlock();

    _nShapes++;
  }
  else
  {
    Fail("Dynamic not supported for shadow volumes");
    vbuf._vAlloc = NULL;
  }

  return true;
}

void VertexStaticDataXB::RemoveShape(VertexHeapXB::HeapItem *vIndex)
{
#ifdef _ENABLE_REPORT
  if (!vIndex)
  {
    Fail("Nothing to remove");
    return;
  }
#endif
  GEngineDD->ReportVBDealloc(vIndex->Size()*_vertexSize);
  _nShapes--;
  // avoid adding into busy regions
  _fenceReleased = _fence;
  _manager.Free(vIndex);
#ifdef _ENABLE_REPORT
  if (_nShapes==0)
  {
    Assert(_manager.TotalBusy()==0);
  }
#endif
}

void EngineDDXB::DeleteVB(VertexStaticDataXB *vb)
{
  if (vb->_vBuffer==_vBufferLast)
  {
    _vBufferLast = NULL;
  }
  if (!vb->IsBusy())
  {
    Verify( _statVBuffer.DeleteKey(vb) );
  }
  else
  {
    _statVBufferDelete.Add(vb);
    Verify( _statVBuffer.DeleteKey(vb) );
    // TODO: queue for deletion
  }
}
void EngineDDXB::DeleteIB(IndexStaticDataXB *ib)
{
  // index buffer is never busy
  Verify( _statIBuffer.DeleteKey(ib) );
}

bool VertexStaticDataXB::IsBusy() const
{
  return (_vBuffer && _vBuffer->IsBusy()) || GEngineDD->GetDirect3DDevice()->IsFencePending(_fence);
}

bool VertexStaticDataXB::IsBusyForAdd() const
{
  return GEngineDD->GetDirect3DDevice()->IsFencePending(_fenceReleased)!=FALSE;
}

#define MaxVertexCount(type) (MaxStaticVertexBufferSizeB/sizeof(type))

//#define MaxVertexCount(type) (4*1024)

VertexStaticDataXB *EngineDDXB::AddShapeVertices(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate)
{
  vbuf._vAlloc = NULL;
  if (dynamic) return NULL;

  // Number of items in the new vertex buffer
  int bufferVertexCount;
  int maxVertexCount = MaxVertexCount(SVertexXB);
  // If separate, then create separated buffer
  if (separate || src.NVertex()>maxVertexCount)
  {
    bufferVertexCount = src.NVertex();
  }
  // If shared, then try to fit into existing buffer
  else
  {
    // Check if we will fit into some existing buffer
    for (int i=0; i<_statVBuffer.Size(); i++)
    {
      VertexStaticDataXB *sd = _statVBuffer[i];
      if (!sd->IsBusyForAdd() && sd->AddShape(src, vbuf, dynamic))
      {
        return sd;
      }
    }
    bufferVertexCount = maxVertexCount;
  }

  // Either we didn't fit into existing or we use a separate buffer, so create a new buffer
  VertexStaticDataXB *sd = new VertexStaticDataXB;
  sd->Init(this, bufferVertexCount, sizeof(SVertexXB), _d3DDevice);

  // If we failed to init vertex buffer for the static data, then free static data and return error
  if (!sd->_vBuffer)
  {
    delete sd;
    return NULL;
  }

  // Add shape into the new buffer
  _statVBuffer.Add(sd);
  if (sd->AddShape(src, vbuf, dynamic))
  {
    return sd;
  }
  return NULL;
}

VertexStaticDataXB *EngineDDXB::AddShapeVerticesNormal(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate)
{
  vbuf._vAlloc = NULL;
  if (dynamic) return NULL;

  // Number of items in the new vertex buffer
  int bufferVertexCount;
  int maxVertexCount = MaxVertexCount(SVertexNormalXB);
  // If separate, then create separated buffer
  if (separate || src.NVertex()>maxVertexCount)
  {
    bufferVertexCount = src.NVertex();
  }
  // If shared, then try to fit into existing buffer
  else
  {
    // Check if we will fit into some existing buffer
    for (int i=0; i<_statVBuffer.Size(); i++)
    {
      VertexStaticDataXB *sd = _statVBuffer[i];
      if (!sd->IsBusyForAdd() && sd->AddShapeNormal(src, vbuf, dynamic))
      {
        return sd;
      }
    }
    bufferVertexCount = maxVertexCount;
  }

  // Either we didn't fit into existing or we use a separate buffer, so create a new buffer
  VertexStaticDataXB *sd = new VertexStaticDataXB;
  sd->Init(this, bufferVertexCount, sizeof(SVertexNormalXB), _d3DDevice);

  // If we failed to init vertex buffer for the static data, then free static data and return error
  if (!sd->_vBuffer)
  {
    delete sd;
    return NULL;
  }

  // Add shape into the new buffer
  _statVBuffer.Add(sd);
  if (sd->AddShapeNormal(src, vbuf, dynamic))
  {
    return sd;
  }
  return NULL;
}

VertexStaticDataXB *EngineDDXB::AddShapeVerticesNormalAlpha(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate)
{
  vbuf._vAlloc = NULL;
  if (dynamic) return NULL;

  // Number of items in the new vertex buffer
  int bufferVertexCount;
  int maxVertexCount = MaxVertexCount(SVertexNormalAlphaXB);
  // If separate, then create separated buffer
  if (separate || src.NVertex()>maxVertexCount)
  {
    bufferVertexCount = src.NVertex();
  }
  // If shared, then try to fit into existing buffer
  else
  {
    // Check if we will fit into some existing buffer
    for (int i=0; i<_statVBuffer.Size(); i++)
    {
      VertexStaticDataXB *sd = _statVBuffer[i];
      if (!sd->IsBusyForAdd() && sd->AddShapeNormalAlpha(src, vbuf, dynamic))
      {
        return sd;
      }
    }
    bufferVertexCount = maxVertexCount;
  }

  // Either we didn't fit into existing or we use a separate buffer, so create a new buffer
  VertexStaticDataXB *sd = new VertexStaticDataXB;
  sd->Init(this, bufferVertexCount, sizeof(SVertexNormalAlphaXB), _d3DDevice);

  // If we failed to init vertex buffer for the static data, then free static data and return error
  if (!sd->_vBuffer)
  {
    delete sd;
    return NULL;
  }

  // Add shape into the new buffer
  _statVBuffer.Add(sd);
  if (sd->AddShapeNormalAlpha(src, vbuf, dynamic))
  {
    return sd;
  }
  return NULL;
}

VertexStaticDataXB *EngineDDXB::AddShapeVerticesShadowVolume(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate)
{
  vbuf._vAlloc = NULL;
  if (dynamic) return NULL;

  // Number of items in the new vertex buffer
  int bufferVertexCount;
  int maxVertexCount = MaxVertexCount(SVertexShadowVolumeXB);
  // If separate, then create separated buffer
  if (separate || src.NVertex()>maxVertexCount)
  {
    bufferVertexCount = src.NVertex();
  }
  // If shared, then try to fit into existing buffer
  else
  {
    // Check if we will fit into some existing buffer
    for (int i=0; i<_statVBuffer.Size(); i++)
    {
      VertexStaticDataXB *sd = _statVBuffer[i];
      if (!sd->IsBusyForAdd() && sd->AddShapeShadowVolume(src, vbuf, dynamic))
      {
        return sd;
      }
    }
    bufferVertexCount = maxVertexCount;
  }

  // Either we didn't fit into existing or we use a separate buffer, so create a new buffer
  VertexStaticDataXB *sd = new VertexStaticDataXB;
  sd->Init(this, bufferVertexCount, sizeof(SVertexShadowVolumeXB), _d3DDevice);

  // If we failed to init vertex buffer for the static data, then free static data and return error
  if (!sd->_vBuffer)
  {
    delete sd;
    return NULL;
  }

  // Add shape into the new buffer
  _statVBuffer.Add(sd);
  if (sd->AddShapeShadowVolume(src, vbuf, dynamic))
  {
    return sd;
  }
  return NULL;
}

VertexStaticDataXB *EngineDDXB::AddShapeVerticesPoint(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate)
{
  vbuf._vAlloc = NULL;
  if (dynamic) return NULL;

  // Number of items in the new vertex buffer
  int bufferVertexCount;
  int maxVertexCount = MaxVertexCount(SVertexPointXB);
  // If separate, then create separated buffer
  if (separate || (src.NVertex() * 4) > maxVertexCount)
  {
    bufferVertexCount = src.NVertex() * 4;
  }
  // If shared, then try to fit into existing buffer
  else
  {
    // Check if we will fit into some existing buffer
    for (int i=0; i<_statVBuffer.Size(); i++)
    {
      VertexStaticDataXB *sd = _statVBuffer[i];
      if (!sd->IsBusyForAdd() && sd->AddShapePoint(src, vbuf, dynamic))
      {
        return sd;
      }
    }
    bufferVertexCount = maxVertexCount;
  }

  // Either we didn't fit into existing or we use a separate buffer, so create a new buffer
  VertexStaticDataXB *sd = new VertexStaticDataXB;
  sd->Init(this, bufferVertexCount, sizeof(SVertexPointXB), _d3DDevice);

  // If we failed to init vertex buffer for the static data, then free static data and return error
  if (!sd->_vBuffer)
  {
    delete sd;
    return NULL;
  }

  // Add shape into the new buffer
  _statVBuffer.Add(sd);
  if (sd->AddShapePoint(src, vbuf, dynamic))
  {
    return sd;
  }
  return NULL;
}

VertexStaticDataXB *EngineDDXB::AddShapeVerticesInstanced(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate, int instancesCount)
{
  vbuf._vAlloc = NULL;
  if (dynamic) return NULL;

  // Number of items in the new vertex buffer
  int bufferVertexCount;
  int maxVertexCount = MaxVertexCount(SVertexInstancedXB);
  // If separate, then create separated buffer
  if (separate || src.NVertex()*instancesCount>maxVertexCount)
  {
    bufferVertexCount = src.NVertex() * instancesCount;
  }
  // If shared, then try to fit into existing buffer
  else
  {
    // Check if we will fit into some existing buffer
    for (int i = 0; i < _statVBuffer.Size(); i++)
    {
      VertexStaticDataXB *sd = _statVBuffer[i];
      if (!sd->IsBusyForAdd() && sd->AddShapeInstanced(src, vbuf, dynamic, instancesCount))
      {
        return sd;
      }
    }
    bufferVertexCount = maxVertexCount;
  }

  // Either we didn't fit into existing or we use a separate buffer, so create a new buffer
  VertexStaticDataXB *sd = new VertexStaticDataXB;
  sd->Init(this, bufferVertexCount, sizeof(SVertexInstancedXB), _d3DDevice);

  // If we failed to init vertex buffer for the static data, then free static data and return error
  if (!sd->_vBuffer)
  {
    delete sd;
    return NULL;
  }

  // Add shape into the new buffer
  _statVBuffer.Add(sd);
  if (sd->AddShapeInstanced(src, vbuf, dynamic, instancesCount))
  {
    return sd;
  }
  return NULL;
}

VertexStaticDataXB *EngineDDXB::AddShapeVerticesNormalInstanced(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate, int instancesCount)
{
  vbuf._vAlloc = NULL;
  if (dynamic) return NULL;

  // Number of items in the new vertex buffer
  int bufferVertexCount;
  int maxVertexCount = MaxVertexCount(SVertexNormalInstancedXB);
  // If separate, then create separated buffer
  if (separate || src.NVertex()*instancesCount>maxVertexCount)
  {
    bufferVertexCount = src.NVertex() * instancesCount;
  }
  // If shared, then try to fit into existing buffer
  else
  {
    // Check if we will fit into some existing buffer
    for (int i = 0; i < _statVBuffer.Size(); i++)
    {
      VertexStaticDataXB *sd = _statVBuffer[i];
      if (!sd->IsBusyForAdd() && sd->AddShapeNormalInstanced(src, vbuf, dynamic, instancesCount))
      {
        return sd;
      }
    }
    bufferVertexCount = maxVertexCount;
  }

  // Either we didn't fit into existing or we use a separate buffer, so create a new buffer
  VertexStaticDataXB *sd = new VertexStaticDataXB;
  sd->Init(this, bufferVertexCount, sizeof(SVertexNormalInstancedXB), _d3DDevice);

  // If we failed to init vertex buffer for the static data, then free static data and return error
  if (!sd->_vBuffer)
  {
    delete sd;
    return NULL;
  }

  // Add shape into the new buffer
  _statVBuffer.Add(sd);
  if (sd->AddShapeNormalInstanced(src, vbuf, dynamic, instancesCount))
  {
    return sd;
  }
  return NULL;
}

VertexStaticDataXB *EngineDDXB::AddShapeVerticesShadowVolumeInstanced(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate, int instancesCount)
{
  vbuf._vAlloc = NULL;
  if (dynamic) return NULL;

  // Number of items in the new vertex buffer
  int bufferVertexCount;
  int maxVertexCount = MaxVertexCount(SVertexShadowVolumeInstancedXB);
  // If separate, then create separated buffer
  if (separate || src.NVertex()*instancesCount>maxVertexCount)
  {
    bufferVertexCount = src.NVertex() * instancesCount;
  }
  // If shared, then try to fit into existing buffer
  else
  {
    // Check if we will fit into some existing buffer
    for (int i = 0; i < _statVBuffer.Size(); i++)
    {
      VertexStaticDataXB *sd = _statVBuffer[i];
      if (!sd->IsBusyForAdd() && sd->AddShapeShadowVolumeInstanced(src, vbuf, dynamic, instancesCount))
      {
        return sd;
      }
    }
    bufferVertexCount = maxVertexCount;
  }

  // Either we didn't fit into existing or we use a separate buffer, so create a new buffer
  VertexStaticDataXB *sd = new VertexStaticDataXB;
  sd->Init(this, bufferVertexCount, sizeof(SVertexShadowVolumeInstancedXB), _d3DDevice);

  // If we failed to init vertex buffer for the static data, then free static data and return error
  if (!sd->_vBuffer)
  {
    delete sd;
    return NULL;
  }

  // Add shape into the new buffer
  _statVBuffer.Add(sd);
  if (sd->AddShapeShadowVolumeInstanced(src, vbuf, dynamic, instancesCount))
  {
    return sd;
  }
  return NULL;
}

VertexStaticDataXB *EngineDDXB::AddShapeVerticesSpriteInstanced(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate, int instancesCount)
{
  vbuf._vAlloc = NULL;
  if (dynamic) return NULL;

  // Number of items in the new vertex buffer
  int bufferVertexCount;
  int maxVertexCount = MaxVertexCount(SVertexSpriteInstancedXB);
  // If separate, then create separated buffer
  if (separate || 4*instancesCount>maxVertexCount)
  {
    bufferVertexCount = 4 * instancesCount;
  }
  // If shared, then try to fit into existing buffer
  else
  {
    // Check if we will fit into some existing buffer
    for (int i = 0; i < _statVBuffer.Size(); i++)
    {
      VertexStaticDataXB *sd = _statVBuffer[i];
      if (!sd->IsBusyForAdd() && sd->AddShapeSpriteInstanced(src, vbuf, dynamic, instancesCount))
      {
        return sd;
      }
    }
    bufferVertexCount = maxVertexCount;
  }

  // Either we didn't fit into existing or we use a separate buffer, so create a new buffer
  VertexStaticDataXB *sd = new VertexStaticDataXB;
  sd->Init(this, bufferVertexCount, sizeof(SVertexSpriteInstancedXB), _d3DDevice);

  // If we failed to init vertex buffer for the static data, then free static data and return error
  if (!sd->_vBuffer)
  {
    delete sd;
    return NULL;
  }

  // Add shape into the new buffer
  _statVBuffer.Add(sd);
  if (sd->AddShapeSpriteInstanced(src, vbuf, dynamic, instancesCount))
  {
    return sd;
  }
  return NULL;
}

VertexStaticDataXB *EngineDDXB::AddShapeVerticesSkinned(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate)
{
  vbuf._vAlloc = NULL;
  if (dynamic) return NULL;

  // Number of items in the new vertex buffer
  int bufferVertexCount;
  int maxVertexCount = MaxVertexCount(SVertexSkinnedXB);
  // If separate, then create separated buffer
  if (separate || src.NVertex()>maxVertexCount)
  {
    bufferVertexCount = src.NVertex();
  }
  // If shared, then try to fit into existing buffer
  else
  {
    // Check if we will fit into some existing buffer
    for (int i=0; i<_statVBuffer.Size(); i++)
    {
      VertexStaticDataXB *sd = _statVBuffer[i];
      if (!sd->IsBusyForAdd() && sd->AddShapeSkinned(src, vbuf, dynamic))
      {
        return sd;
      }
    }
    bufferVertexCount = maxVertexCount;
  }

  // Either we didn't fit into existing or we use a separate buffer, so create a new buffer
  VertexStaticDataXB *sd = new VertexStaticDataXB;
  sd->Init(this, bufferVertexCount, sizeof(SVertexSkinnedXB), _d3DDevice);

  // If we failed to init vertex buffer for the static data, then free static data and return error
  if (!sd->_vBuffer)
  {
    delete sd;
    return NULL;
  }

  // Add shape into the new buffer
  _statVBuffer.Add(sd);
  if (sd->AddShapeSkinned(src, vbuf, dynamic))
  {
    return sd;
  }
  return NULL;
}

VertexStaticDataXB *EngineDDXB::AddShapeVerticesNormalSkinned(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate)
{
  vbuf._vAlloc = NULL;
  if (dynamic) return NULL;

  ShapeGeometryLock<> lock(&src);

  // Number of items in the new vertex buffer
  int bufferVertexCount;
  int maxVertexCount = MaxVertexCount(SVertexNormalSkinnedXB);
  // If separate, then create separated buffer
  if (separate || src.NVertex()>maxVertexCount)
  {
    bufferVertexCount = src.NVertex();
  }
  // If shared, then try to fit into existing buffer
  else
  {
    // Check if we will fit into some existing buffer
    for (int i=0; i<_statVBuffer.Size(); i++)
    {
      VertexStaticDataXB *sd = _statVBuffer[i];
      if (!sd->IsBusyForAdd() && sd->AddShapeNormalSkinned(src, vbuf, dynamic))
      {
        return sd;
      }
    }
    bufferVertexCount = maxVertexCount;
  }

  // Either we didn't fit into existing or we use a separate buffer, so create a new buffer
  VertexStaticDataXB *sd = new VertexStaticDataXB;
  sd->Init(this, bufferVertexCount, sizeof(SVertexNormalSkinnedXB), _d3DDevice);

  // If we failed to init vertex buffer for the static data, then free static data and return error
  if (!sd->_vBuffer)
  {
    delete sd;
    return NULL;
  }

  // Add shape into the new buffer
  _statVBuffer.Add(sd);
  if (sd->AddShapeNormalSkinned(src, vbuf, dynamic))
  {
    return sd;
  }
  return NULL;
}

VertexStaticDataXB *EngineDDXB::AddShapeVerticesShadowVolumeSkinned(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate)
{
  vbuf._vAlloc = NULL;
  if (dynamic) return NULL;

  ShapeGeometryLock<> lock(&src);

  // Number of items in the new vertex buffer
  int bufferVertexCount;
  int maxVertexCount = MaxVertexCount(SVertexShadowVolumeSkinnedXB);
  // If separate, then create separated buffer
  if (separate || src.NVertex()>maxVertexCount)
  {
    bufferVertexCount = src.NVertex();
  }
  // If shared, then try to fit into existing buffer
  else
  {
    // Check if we will fit into some existing buffer
    for (int i=0; i<_statVBuffer.Size(); i++)
    {
      VertexStaticDataXB *sd = _statVBuffer[i];
      if (!sd->IsBusyForAdd() && sd->AddShapeShadowVolumeSkinned(src, vbuf, dynamic))
      {
        return sd;
      }
    }
    bufferVertexCount = maxVertexCount;
  }

  // Either we didn't fit into existing or we use a separate buffer, so create a new buffer
  VertexStaticDataXB *sd = new VertexStaticDataXB;
  sd->Init(this, bufferVertexCount, sizeof(SVertexShadowVolumeSkinnedXB), _d3DDevice);

  // If we failed to init vertex buffer for the static data, then free static data and return error
  if (!sd->_vBuffer)
  {
    delete sd;
    return NULL;
  }

  // Add shape into the new buffer
  _statVBuffer.Add(sd);
  if (sd->AddShapeShadowVolumeSkinned(src, vbuf, dynamic))
  {
    return sd;
  }
  return NULL;
}

//////////////////////{{


IndexStaticDataXB::IndexStaticDataXB()
{
  _nShapes = 0;

}

IndexStaticDataXB::~IndexStaticDataXB()
{
  Dealloc();
}

static inline int AlignMemAlloc( int x )
{
  int sign = 1;
  if (x<0)
  {
    sign = -1;
    x = -x;
  }
  #if CUSTOM_VB_ALLOCATOR
    int align = 256; // custom allocator page size
  #else
    int align = 4*1024; // default physical memory page size
  #endif
  int alignMask = align-1;
  return ((x+alignMask)&~alignMask)*sign;
}

void EngineDDXB::AddIBuffersAllocated(int n)
{
  _iBuffersAllocated += n;
}
void EngineDDXB::AddVBuffersAllocated(int n)
{
  _vBuffersAllocated += n;
  _vBuffersUsed += AlignMemAlloc(n);
  #if _ENABLE_CHEATS
    // recover from inaccuracies
    DoAssert(_vBuffersUsed>=0);
    DoAssert(_vBuffersUsed>=_vBuffersAllocated);
    if (_vBuffersAllocated==0)
    {
      _vBuffersUsed = 0;
    }
    if (_vBuffersUsed<0) _vBuffersUsed = 0;
    if (_vBuffersUsed<_vBuffersAllocated) _vBuffersUsed = _vBuffersAllocated;
  #endif
}

inline void EngineDDXB::ReportVBAlloc(int size)
{
  _vBuffersRequested += size;
}
inline void EngineDDXB::ReportIBAlloc(int size)
{
  _iBuffersRequested += size;
}
inline void EngineDDXB::ReportVBDealloc(int size)
{
  _vBuffersRequested -= size;
}
inline void EngineDDXB::ReportIBDealloc(int size)
{
  _iBuffersRequested -= size;
}

void IndexStaticDataXB::Init(EngineDDXB *engine, int indices, IDirect3DDevice8 *dev)
{
  _engine = engine;
  _manager.Free();
  if (indices>0)
  {
    int byteSize = indices*sizeof(VertexIndex);
RetryI:
    HRESULT err = dev->CreateIndexBuffer
      (
      byteSize,WRITEONLYIB,D3DFMT_INDEX16,D3DPOOL_DEFAULT,
      _iBuffer.Init()
      );
    if (err!=D3D_OK)
    {
      if (err==D3DERR_OUTOFVIDEOMEMORY)
      {
        LogF("Out of VRAM during i-buffer creation");
        if (GEngineDD->TextBankDD()->ForcedReserveMemory(byteSize))
        {
          goto RetryI;
        }
      }
      else if (err=E_OUTOFMEMORY)
      {
        size_t freed = FreeOnDemandSystemMemory(byteSize);
        if (freed!=0) goto RetryI;
      }
      LogF("Error during i-buffer creation: error %x",err);
      // debugging opportunity
      size_t freed = FreeOnDemandSystemMemory(byteSize);
      if (freed!=0) goto RetryI;
      DDErrorXB("CreateIndexBuffer",err);
      return;
      // 
    }
    _manager.Init(0,indices,1);
    engine->AddIBuffersAllocated(_manager.Size()*sizeof(VertexIndex));
  }
}

void IndexStaticDataXB::Dealloc()
{
  if (_nShapes>0)
  {
    LogF("IndexStaticDataXB:: %d shapes not removed",_nShapes);
  }
  if (_iBuffer)
  {
    _engine->AddIBuffersAllocated(-_manager.Size()*sizeof(VertexIndex));
    _iBuffer.Free();
  }
}

int IndexStaticDataXB::MaxIndicesInOneSection(const Shape &src)
{
  int maxIndices = -1;
  for (int i = 0; i < src.NSections(); i++)
  {
    const ShapeSection &sec = src.GetSection(i);
    int indices = 0;
    for (Offset o = sec.beg; o < sec.end; src.NextFace(o))
    {
      const Poly &poly = src.Face(o);
      if (poly.N() < 3) continue;
      ///*
      if ((poly.N() == 3) &&
        ((poly.GetVertex(0) == poly.GetVertex(1)) || 
        (poly.GetVertex(1) == poly.GetVertex(2)) ||
        (poly.GetVertex(2) == poly.GetVertex(0)))) continue;
      //*/
      indices += (poly.N()-2)*3;
    }
    if (indices > maxIndices) maxIndices = indices;
  }
  return maxIndices;
}

int IndexStaticDataXB::IndicesNeededTotal(const Shape &src, int nInstances)
{
  int indices = 0;
  for (Offset o=src.BeginFaces(); o<src.EndFaces(); src.NextFace(o))
  {
    const Poly &poly = src.Face(o);
    if (poly.N() < 3) continue;
    if ((poly.N() == 3) &&
      ((poly.GetVertex(0) == poly.GetVertex(1)) || 
      (poly.GetVertex(1) == poly.GetVertex(2)) ||
      (poly.GetVertex(2) == poly.GetVertex(0)))) continue;
    indices += (poly.N()-2)*3;
  }
  return indices * nInstances;
}

int IndexStaticDataXB::IndicesNeededStrippedTotal(const Shape &src, int nInstances)
{
  int indices = 0;
  for (int i = 0; i < src.NSections(); i++)
  {
    const ShapeSection &sec = src.GetSection(i);
    Assert((OffsetDiff(sec.end, sec.beg) % PolyVerticesSize(3)) == 0);
    int trianglesCount = OffsetDiff(sec.end, sec.beg) / PolyVerticesSize(3);
    indices += (2 + trianglesCount) * nInstances + (nInstances - 1) * 2;
    if (trianglesCount & 1)
    {
      indices += (nInstances - 1) * 1;
    }
  }
  return indices;
}

/*!
\patch 1.24 Date 9/21/2001 by Ondra
- Fixed: Lighting of polygons by point lights was reversed on HW T&L.
\patch 1.32 Date 11/26/2001 by Ondra
- Fixed: Destroyed buildings did not collapse with HW T&L.
*/


bool IndexStaticDataXB::AddShape(const Shape &src, int vOffset, IndexAllocXB *&iIndex, int indicesTotal, bool dynamic, int nInstances)
{
  // let manager find some free space
  IndexHeapXB::HeapItem *item = _manager.Alloc(indicesTotal);
  if (!item)
  {
    return false;
  }

  iIndex = item;

  _nShapes++;

  GEngineDD->ReportIBAlloc(iIndex->Size()*sizeof(VertexIndex));
  // when adding dynamic data, add only indices

  if (indicesTotal > 0)
  {
RetryI:
    BYTE *data = NULL;
    HRESULT err;
    {
      PROFILE_DX_SCOPE(3ibLS);
      err = _iBuffer->Lock
        (
        iIndex->Memory()*sizeof(VertexIndex), indicesTotal * sizeof(VertexIndex),
        &data,NoSysLockFlag
        );
    }
    if (err!=D3D_OK || !data)
    {
      if (QFBank::FreeUnusedBanks(1024*1024)) goto RetryI;
#ifndef _XBOX
      ErrF("IB Lock failed, %s",DXGetErrorString8(err));
#else
      ErrF("IB Lock failed, %x",err);
#endif
      return false;
    }

    VertexIndex *iData = (VertexIndex *)data;

    // Go through all the sections
    for (int s = 0; s < src.NSections(); s++)
    {
      const ShapeSection &sec = src.GetSection(s);
      for (int instance=0; instance<nInstances; instance++)
      {
        int instanceOffset = src.NVertex()*instance;
        for (Offset o=sec.beg; o<sec.end; src.NextFace(o))
        {
          const Poly &poly = src.Face(o);
          if (poly.N()<3) continue;
          if ((poly.N() == 3) &&
            ((poly.GetVertex(0) == poly.GetVertex(1)) || 
            (poly.GetVertex(1) == poly.GetVertex(2)) ||
            (poly.GetVertex(2) == poly.GetVertex(0)))) continue;
          for (int i=2; i<poly.N(); i++)
          {
#if V_OFFSET_CUSTOM
            *iData++ = poly.GetVertex(0)+instanceOffset;
            *iData++ = poly.GetVertex(i-1)+instanceOffset;
            *iData++ = poly.GetVertex(i)+instanceOffset;
#else
            *iData++ = poly.GetVertex(0)+vOffset+instanceOffset;
            *iData++ = poly.GetVertex(i-1)+vOffset+instanceOffset;
            *iData++ = poly.GetVertex(i)+vOffset+instanceOffset;
#endif
          }
        }
      }
    }
    DoAssert(iData == &((VertexIndex *)data)[indicesTotal]);
    _iBuffer->Unlock();
  }
  return true;
}

bool IndexStaticDataXB::AddShapeStripped(const Shape &src, IndexAllocXB *&iIndex, int indicesTotal, bool dynamic, int nInstances)
{
  // let manager find some free space
  IndexHeapXB::HeapItem *item = _manager.Alloc(indicesTotal);
  if (!item)
  {
    return false;
  }

  iIndex = item;

  _nShapes++;

  GEngineDD->ReportIBAlloc(iIndex->Size()*sizeof(VertexIndex));
  // when adding dynamic data, add only indices

  if (indicesTotal>0)
  {
RetryI:
    BYTE *data = NULL;
    HRESULT err;
    {
      PROFILE_DX_SCOPE(3ibLS);
      err = _iBuffer->Lock
        (
        iIndex->Memory()*sizeof(VertexIndex), indicesTotal * sizeof(VertexIndex),
        &data,NoSysLockFlag
        );
    }
    if (err!=D3D_OK || !data)
    {
      if (QFBank::FreeUnusedBanks(1024*1024)) goto RetryI;
#ifndef _XBOX
      ErrF("IB Lock failed, %s",DXGetErrorString8(err));
#else
      ErrF("IB Lock failed, %x",err);
#endif
      return false;
    }

    VertexIndex *iData = (VertexIndex *)data;

    // Go through all the sections
    for (int i = 0; i < src.NSections(); i++)
    {
      const ShapeSection &sec = src.GetSection(i);
      Assert((OffsetDiff(sec.end, sec.beg) % PolyVerticesSize(3)) == 0);
      int trianglesCount = OffsetDiff(sec.end, sec.beg) / PolyVerticesSize(3);
      for (int instance = 0; instance < nInstances; instance++)
      {
        int instanceOffset = src.NVertex()*instance;

        // Add the first triangle
        Offset o = sec.beg;
        if (o < sec.end)
        {
          const Poly &poly = src.Face(o);
          Assert(poly.N() == 3);

          // If we are not in the first instance, then add the concatenation sequence
          if (instance > 0)
          {
            // If the previous instance has got odd number of triangles, then make it even
            if (trianglesCount & 1)
            {
              *iData = iData[-1]; iData++;
            }
            *iData = iData[-1]; iData++;
            *iData++ = poly.GetVertex(0)+instanceOffset;
          }
          *iData++ = poly.GetVertex(0)+instanceOffset;
          *iData++ = poly.GetVertex(1)+instanceOffset;
          *iData++ = poly.GetVertex(2)+instanceOffset;
        }

        // Add the rest of the triangles
        src.NextFace(o);
        while (true)
        {
          if (o >= sec.end) break;
          const Poly &polyA = src.Face(o);
          Assert(polyA.N() == 3);
          *iData++ = polyA.GetVertex(0)+instanceOffset;
          src.NextFace(o);

          if (o >= sec.end) break;
          const Poly &polyB = src.Face(o);
          Assert(polyB.N() == 3);
          *iData++ = polyB.GetVertex(2)+instanceOffset;
          src.NextFace(o);
        }
      }
    }
    DoAssert(iData == &((VertexIndex *)data)[indicesTotal]);
    _iBuffer->Unlock();
  }
  return true;
}

bool IndexStaticDataXB::AddShapeSprite(const Shape &src, IndexAllocXB *&iIndex, int indicesTotal, bool dynamic, int nInstances)
{
  // let manager find some free space
  IndexHeapXB::HeapItem *item = _manager.Alloc(indicesTotal);
  if (!item)
  {
    return false;
  }

  iIndex = item;

  _nShapes++;

  GEngineDD->ReportIBAlloc(iIndex->Size()*sizeof(VertexIndex));
  // when adding dynamic data, add only indices

  if (indicesTotal>0)
  {
RetryI:
    BYTE *data = NULL;
    HRESULT err;
    {
      PROFILE_DX_SCOPE(3ibLS);
      err = _iBuffer->Lock
        (
        iIndex->Memory()*sizeof(VertexIndex), indicesTotal * sizeof(VertexIndex),
        &data,NoSysLockFlag
        );
    }
    if (err!=D3D_OK || !data)
    {
      if (QFBank::FreeUnusedBanks(1024*1024)) goto RetryI;
#ifndef _XBOX
      ErrF("IB Lock failed, %s",DXGetErrorString8(err));
#else
      ErrF("IB Lock failed, %x",err);
#endif
      return false;
    }

    VertexIndex *iData = (VertexIndex *)data;
    for (int instance=0; instance<nInstances; instance++)
    {
      int instanceOffset = 4 * instance;
      *iData++ = 0 + instanceOffset;
      *iData++ = 1 + instanceOffset;
      *iData++ = 2 + instanceOffset;
      *iData++ = 2 + instanceOffset;
      *iData++ = 1 + instanceOffset;
      *iData++ = 3 + instanceOffset;
    }

    DoAssert(iData == &((VertexIndex *)data)[indicesTotal]);
    _iBuffer->Unlock();
  }
  return true;
}

bool IndexStaticDataXB::AddShapePoint(const Shape &src, IndexAllocXB *&iIndex, int indicesTotal, bool dynamic, int nInstances)
{
  // let manager find some free space
  IndexHeapXB::HeapItem *item = _manager.Alloc(indicesTotal);
  if (!item)
  {
    return false;
  }

  iIndex = item;

  _nShapes++;
  
  GEngineDD->ReportIBAlloc(iIndex->Size()*sizeof(VertexIndex));

  // when adding dynamic data, add only indices

  if (indicesTotal>0)
  {
RetryI:
    BYTE *data = NULL;
    HRESULT err;
    {
      PROFILE_DX_SCOPE(3ibLS);
      err = _iBuffer->Lock
        (
        iIndex->Memory()*sizeof(VertexIndex), indicesTotal * sizeof(VertexIndex),
        &data,NoSysLockFlag
        );
    }
    if (err!=D3D_OK || !data)
    {
      if (QFBank::FreeUnusedBanks(1024*1024)) goto RetryI;
#ifndef _XBOX
      ErrF("IB Lock failed, %s",DXGetErrorString8(err));
#else
      ErrF("IB Lock failed, %x",err);
#endif
      return false;
    }

    VertexIndex *iData = (VertexIndex *)data;
    int nVertex = src.NVertex();
    for (int i = 0; i < nVertex; i++)
    {
      int pointOffset = 4 * i;
      *iData++ = 0 + pointOffset;
      *iData++ = 2 + pointOffset;
      *iData++ = 1 + pointOffset;
      *iData++ = 1 + pointOffset;
      *iData++ = 2 + pointOffset;
      *iData++ = 3 + pointOffset;
    }
    DoAssert(iData == &((VertexIndex *)data)[indicesTotal]);
    _iBuffer->Unlock();
  }
  return true;
}

void IndexStaticDataXB::RemoveShape(IndexAllocXB *iIndex)
{
  GEngineDD->ReportIBDealloc(iIndex->Size()*sizeof(VertexIndex));
  _manager.Free(iIndex);
  _nShapes--;
#ifdef _ENABLE_REPORT
  if (_nShapes==0)
  {
    Assert(_manager.TotalBusy()==0);
  }
#endif
}

IndexStaticDataXB *EngineDDXB::AddShapeIndices(
  const Shape &src, int vOffset, IndexAllocXB *&iIndex, bool dynamic, int nInstances
  )
{
  iIndex = NULL;

  // Maximum number of indices
  const int maxIndices = 32*1024-64; // memory has some header - leave a space for it

  // Get the number of indices
  int indicesTotal;
#ifdef STRIPIZATION_ENABLED
  if ((src.GetFaceArrayType() == FATStripized) && EnableStrips)
  {
    indicesTotal = IndexStaticDataXB::IndicesNeededStrippedTotal(src, nInstances);
  }
  else
#endif
  {
    indicesTotal = IndexStaticDataXB::IndicesNeededTotal(src, nInstances);
  }

  // It's a nonsense to have zero indices
  Assert(indicesTotal > 0);

  // Number of items in the new index buffer
  int bufferIndexCount;

  // If separate, then create separated buffer
  if (indicesTotal > maxIndices)
  {
    bufferIndexCount = indicesTotal;
  }
  else
  {
    // Check if we will fit into some existing buffer
    for (int i=0; i<_statIBuffer.Size(); i++)
    {
      IndexStaticDataXB *sd = _statIBuffer[i];
      if (sd->IsBusyForAdd()) continue;
#ifdef STRIPIZATION_ENABLED
      if ((src.GetFaceArrayType() == FATStripized) && EnableStrips)
      {
        if (sd->AddShapeStripped(src,iIndex,indicesTotal,dynamic,nInstances))
        {
          return sd;
        }
      }
      else
#endif
      {
        if (sd->AddShape(src, vOffset, iIndex,indicesTotal,dynamic,nInstances))
        {
          return sd;
        }
      }
    }
    bufferIndexCount = maxIndices;
  }

  // Either we didn't fit into existing or we use a separate buffer, so create a new buffer
  IndexStaticDataXB *sd = new IndexStaticDataXB;
  sd->Init(this, bufferIndexCount, _d3DDevice);

  // If we failed to init index buffer for the static data, then free static data and return error
  if (!sd->_iBuffer)
  {
    delete sd;
    return NULL;
  }

  // Add shape into the new buffer
  _statIBuffer.Add(sd);

#ifdef STRIPIZATION_ENABLED
  if ((src.GetFaceArrayType() == FATStripized) && EnableStrips)
  {
    if (sd->AddShapeStripped(src,iIndex,indicesTotal,dynamic,nInstances))
    {
      return sd;
    }
  }
  else
#endif
  {
    if (sd->AddShape(src,vOffset,iIndex,indicesTotal,dynamic,nInstances))
    {
      return sd;
    }
  }
  return NULL;
}

IndexStaticDataXB *EngineDDXB::AddShapeIndicesSprite(
  const Shape &src, IndexAllocXB *&iIndex, bool dynamic, int nInstances
  )
{
  iIndex = NULL;

  // Maximum number of indices
  const int maxIndices = 32*1024-64; // memory has some header - leave a space for it

  // Get the number of indices
  int indicesTotal;
  indicesTotal = 6 * nInstances;

  // It's a nonsense to have zero indices
  Assert(indicesTotal > 0);

  // Number of items in the new index buffer
  int bufferIndexCount;

  // If separate, then create separated buffer
  if (indicesTotal > maxIndices)
  {
    bufferIndexCount = indicesTotal;
  }
  else
  {
    // Check if we will fit into some existing buffer
    for (int i=0; i<_statIBuffer.Size(); i++)
    {
      IndexStaticDataXB *sd = _statIBuffer[i];
      if (sd->IsBusyForAdd()) continue;
      if (sd->AddShapeSprite(src,iIndex,indicesTotal,dynamic,nInstances))
      {
        return sd;
      }
    }
    bufferIndexCount = maxIndices;
  }

  // Either we didn't fit into existing or we use a separate buffer, so create a new buffer
  IndexStaticDataXB *sd = new IndexStaticDataXB;
  sd->Init(this, bufferIndexCount, _d3DDevice);

  // If we failed to init index buffer for the static data, then free static data and return error
  if (!sd->_iBuffer)
  {
    delete sd;
    return NULL;
  }

  // Add shape into the new buffer
  _statIBuffer.Add(sd);

  if (sd->AddShapeSprite(src,iIndex,indicesTotal,dynamic,nInstances))
  {
    return sd;
  }
  return NULL;
}

IndexStaticDataXB *EngineDDXB::AddShapeIndicesPoint(
  const Shape &src, IndexAllocXB *&iIndex, bool dynamic, int nInstances
  )
{
  iIndex = NULL;

  // Maximum number of indices
  const int maxIndices = 32*1024-64; // memory has some header - leave a space for it

  // Get the number of indices
  int indicesTotal;
  indicesTotal = 6 * src.NVertex();

  // It's a nonsense to have zero indices
  Assert(indicesTotal > 0);

  // Number of items in the new index buffer
  int bufferIndexCount;

  // If separate, then create separated buffer
  if (indicesTotal > maxIndices)
  {
    bufferIndexCount = indicesTotal;
  }
  else
  {
    // Check if we will fit into some existing buffer
    for (int i=0; i<_statIBuffer.Size(); i++)
    {
      IndexStaticDataXB *sd = _statIBuffer[i];
      if (sd->IsBusyForAdd()) continue;
      if (sd->AddShapePoint(src,iIndex,indicesTotal,dynamic,nInstances))
      {
        return sd;
      }
    }
    bufferIndexCount = maxIndices;
  }

  // Either we didn't fit into existing or we use a separate buffer, so create a new buffer
  IndexStaticDataXB *sd = new IndexStaticDataXB;
  sd->Init(this, bufferIndexCount, _d3DDevice);

  // If we failed to init index buffer for the static data, then free static data and return error
  if (!sd->_iBuffer)
  {
    delete sd;
    return NULL;
  }

  // Add shape into the new buffer
  _statIBuffer.Add(sd);

  if (sd->AddShapePoint(src,iIndex,indicesTotal,dynamic,nInstances))
  {
    return sd;
  }
  return NULL;
}

///////////////////}}

class AnimatorEmpty: public IAnimator
{
  Color _color;
public:
  AnimatorEmpty(Color color=Color(1,1,1));
  virtual void DoTransform
    (
    TLVertexTable &dst,
    const Shape &src, const Matrix4 &posView,
    int from, int to
    ) const;
  // when Light is called TLVertexTable already contains
  virtual void DoLight
    (
    TLVertexTable &dst,
    const Shape &src, const Matrix4 &worldToModel, const LightList &lights,
    int spec, int material, int from, int to
    ) const;
  // get material with given index
  virtual void GetMaterial(TLMaterial &mat, int index) const;
};

AnimatorEmpty::AnimatorEmpty(Color color)
:_color(color)
{
}

void AnimatorEmpty::DoTransform
(
 TLVertexTable &dst,
 const Shape &src, const Matrix4 &posView,
 int from, int to
 ) const
{
  dst.DoTransformPoints(src,posView,from,to);
}
// when Light is called TLVertexTable already contains
void AnimatorEmpty::DoLight
(
 TLVertexTable &dst,
 const Shape &src, const Matrix4 &worldToModel, const LightList &lights,
 int spec, int material, int from, int to
 ) const 
{
  TLMaterial mat;
  GetMaterial(mat,material);
  mat.specFlags = spec;
  dst.DoMaterialLightingP
    (
    mat,worldToModel,lights,src,from,to
    );
}

void AnimatorEmpty::GetMaterial(TLMaterial &mat, int index) const
{
  // distrubute by predefined materials
  CreateMaterial(mat, _color, index);
}

#undef Verify

struct FixupCom : public TLinkBidirD
{
  ComRef<IDirect3DFixup8> _fixup;
};

//! Global manager of fixup resources
class FixupManager
{
private:
  TListBidir<FixupCom> _fixups;
public:
  FixupManager();
  ~FixupManager();
  //! Returns a fixup object of specified size
  ComRef<IDirect3DFixup8> GetFixup(DWORD size);
} GFixupManager;

FixupManager::FixupManager()
{
}

FixupManager::~FixupManager()
{
  while (!_fixups.Empty())
  {
    FixupCom *item = _fixups.Start();
    _fixups.Delete(item);
    delete item;
  }
}

ComRef<IDirect3DFixup8> FixupManager::GetFixup(DWORD size)
{
  PROFILE_SCOPE(fixRe);

  // Delete all not busy fixups
  FixupCom *item = _fixups.Start();
  while (_fixups.NotEnd(item))
  {
    if (!item->_fixup->IsBusy())
    {
      FixupCom *itemToDelete = item;
      item = _fixups.Advance(item);
      _fixups.Delete(itemToDelete);
      delete itemToDelete;
    }
    else
    {
      item = _fixups.Advance(item);
    }
  }

  // Create new fixup and add it into queue
  item = new FixupCom;
  _fixups.Add(item);
  HRESULT err;
  err = GEngineDD->GetDirect3DDevice()->CreateFixup(size, item->_fixup.Init());
  Assert(err == S_OK);
  return item->_fixup;
}

// standart day weights
//0.299f, 0.587f, 0.114f, 0.0f

#define EYE_NIGHT_R 0.15f
#define EYE_NIGHT_G 0.60f
#define EYE_NIGHT_B 0.25f

bool FixupEntry::SerializeBin(SerializeBinStream &f)
{
  f.TransferBinary(_pbOffset);
  return true;
}

FETextureHandle::FETextureHandle(DWORD pbOffset, TextureD3DXB *tex, PushBufferXB *pb) : FixupEntry(pbOffset)
{
  _tex = tex;
  _pb = pb;
  _tex->RegisterTextureLoadHandler(this);
  Reset();
}

FETextureHandle::FETextureHandle(PushBufferXB *pb)
{
  _pb = pb;
  Reset();
}

void FETextureHandle::TextureHasChanged()
{
  Reset();
  _pb->TextureHasChanged();
}

bool FETextureHandle::SerializeBin(SerializeBinStream &f)
{
  FixupEntry::SerializeBin(f);
  if (f.IsLoading())
  {
    // Load texture type
    TextureType tt;
    f.TransferBinary(tt);

    // Load texture name
    char name[1024];
    int length = f.LoadInt();
    DoAssert(length < 1024);
    f.Load(name, length);
    name[length] = 0;

    // Get and register the texture
    Ref<Texture> texture = GlobLoadTexture(name);
    DoAssert(texture.NotNull());
    TextureD3DXB *tex = static_cast<TextureD3DXB*>(texture.GetRef());
    _tex = tex;
    _tex->RegisterTextureLoadHandler(this);
  }
  else
  {
    // Save texture type
    TextureType tt = _tex->Type();
    f.TransferBinary(tt);

    // Save texture name
    int length = strlen(_tex->Name());
    DoAssert(length > 0);
    f.SaveInt(length);
    f.Save(_tex->Name(), length);
  }
  
  return true;
}

void FETexture::Apply(IDirect3DPushBuffer8 *pb)
{
  Assert(_tex);
  if (_tex->GetHandle() != NULL)
  {
    pb->SetTexture(_pbOffset, _stage, _tex->GetHandle());
  }
  else
  {
    pb->SetTexture(_pbOffset, _stage, _tex->GetDefaultTexture()->GetHandle());
  }
  _textureHasChanged = false;
}

void FETexture::StoreResources(RefArray<Texture> &usedTextures)
{
  usedTextures.AddUnique(_tex.GetRef());
}

bool FETexture::SerializeBin(SerializeBinStream &f)
{
  if (!FETextureHandle::SerializeBin(f)) return false;
  f.Transfer(_stage);
  return true;
}

void FEMaxColor::Apply(IDirect3DPushBuffer8 *pb)
{
  Assert(_tex);
  Color maxColor = _tex->GetMaxColor();
  if (!_tex->GetHandle())
  {
    maxColor = _tex->GetColor();
  }
  D3DCOLOR c = FAST_D3DRGBA(maxColor.R(), maxColor.G(), maxColor.B(), maxColor.A());
  pb->SetRenderState(_pbOffset, _rs, c);
  _textureHasChanged = false;
}

bool FEMaxColor::SerializeBin(SerializeBinStream &f)
{
  if (!FETextureHandle::SerializeBin(f)) return false;
  f.TransferBinary(_rs);
  return true;
}

static inline float CombineAvgAndMaxColor(float ac, float mc)
{
  if (ac<=mc*1e-6) return 0;
  if (ac>=mc*10) return 10;
  float ret = ac/mc;
  Assert(_finite(ret));
  return ret;
}

/// avoid division zero by zero during color combining
static Color CombineAvgAndMaxColor(ColorVal avgColor, ColorVal maxColor)
{
  return Color(
    CombineAvgAndMaxColor(avgColor.R(),maxColor.R()),
    CombineAvgAndMaxColor(avgColor.G(),maxColor.G()),
    CombineAvgAndMaxColor(avgColor.B(),maxColor.B()),
    CombineAvgAndMaxColor(avgColor.A(),maxColor.A())
  );
}

void FEAvgColor::Apply(IDirect3DPushBuffer8 *pb)
{
  Assert(_tex);
  Color maxColor = _tex->GetMaxColor();
  TextureD3DXB *loadedTexture;
  if (!_tex->GetHandle())
  {
    maxColor = _tex->GetColor();
    loadedTexture = _tex->GetDefaultTexture();
  }
  else
  {
    loadedTexture = _tex;
  }
  Color avgColor = CombineAvgAndMaxColor(loadedTexture->GetColor(),maxColor);
  D3DCOLOR c = FAST_D3DRGBA(avgColor.R(), avgColor.G(), avgColor.B(), avgColor.A());
  pb->SetRenderState(_pbOffset, _rs, c);
  _textureHasChanged = false;
}

bool FEAvgColor::SerializeBin(SerializeBinStream &f)
{
  if (!FETextureHandle::SerializeBin(f)) return false;
  f.TransferBinary(_rs);
  return true;
}

void FEDiffuse::Apply(IDirect3DPushBuffer8 *pb)
{
  Color diffuse;
  if (_mainLight == ML_Sun)
  {
    LightSun *sun = GScene->MainLight();
    diffuse = sun->GetDiffuse() * _matDiffuse * GEngine->GetAccomodateEye();
    diffuse.SetA(_matDiffuse.A());
    _currSunDiffuse = sun->GetDiffuse() * GEngine->GetAccomodateEye();
  }
  else
  {
    diffuse = Color(0, 0, 0, 0);
  }
  D3DCOLOR c = FAST_D3DRGBA_SAT(diffuse.R(), diffuse.G(), diffuse.B(), diffuse.A());
  pb->SetRenderState(_pbOffset, _rs, c);
}

bool FEDiffuse::HasChanged()
{
  if (_mainLight == ML_Sun)
  {
    LightSun *sun = GScene->MainLight();
    return _currSunDiffuse != (sun->GetDiffuse() * GEngine->GetAccomodateEye());
  }
  else
  {
    return false;
  }
}

bool FEDiffuse::SerializeBin(SerializeBinStream &f)
{
  if (!FixupEntry::SerializeBin(f)) return false;
  f.TransferBinary(_rs);
  f.TransferBinary(_mainLight);
  f.TransferBinary(_matDiffuse);
  return true;
}

void FEDiffuseBack::Apply(IDirect3DPushBuffer8 *pb)
{
  Color diffuseBack;
  if (_mainLight == ML_Sun)
  {
    LightSun *sun = GScene->MainLight();

    diffuseBack = sun->GetDiffuse() * _matDiffuse * GEngine->GetAccomodateEye() * _backCoef;
    diffuseBack.SetA(_backCoef);
    _currSunDiffuse = sun->GetDiffuse() * GEngine->GetAccomodateEye();
  }
  else
  {
    diffuseBack = Color(0, 0, 0, 0);
  }
  D3DCOLOR c = FAST_D3DRGBA_SAT(diffuseBack.R(), diffuseBack.G(), diffuseBack.B(), diffuseBack.A());
  pb->SetRenderState(_pbOffset, _rs, c);
}

bool FEDiffuseBack::HasChanged()
{
  if (_mainLight == ML_Sun)
  {
    LightSun *sun = GScene->MainLight();
    return _currSunDiffuse != (sun->GetDiffuse() * GEngine->GetAccomodateEye());
  }
  else
  {
    return false;
  }
}

bool FEDiffuseBack::SerializeBin(SerializeBinStream &f)
{
  if (!FixupEntry::SerializeBin(f)) return false;
  f.TransferBinary(_rs);
  f.TransferBinary(_mainLight);
  f.TransferBinary(_matDiffuse);
  return true;
}

void FESpecular::Apply(IDirect3DPushBuffer8 *pb)
{
  Color specular;
  if (_mainLight == ML_Sun)
  {
    LightSun *sun = GScene->MainLight();
    specular = sun->GetDiffuse() * _matSpecular * 2.0f * GEngine->GetAccomodateEye();
    _currSunDiffuse = sun->GetDiffuse() * GEngine->GetAccomodateEye();
  }
  else
  {
    specular = Color(0, 0, 0, 0);
  }
  D3DCOLOR c = FAST_D3DRGBA_SAT(specular.R(), specular.G(), specular.B(), specular.A());
  pb->SetRenderState(_pbOffset, _rs, c);
}

bool FESpecular::HasChanged()
{
  if (_mainLight == ML_Sun)
  {
    LightSun *sun = GScene->MainLight();
    return _currSunDiffuse != (sun->GetDiffuse() * GEngine->GetAccomodateEye());
  }
  else
  {
    return false;
  }
}

bool FESpecular::SerializeBin(SerializeBinStream &f)
{
  if (!FixupEntry::SerializeBin(f)) return false;
  f.TransferBinary(_rs);
  f.TransferBinary(_mainLight);
  f.TransferBinary(_matSpecular);
  return true;
}

void PushBufferXB::CalculateFixupSize()
{
  if (_fixupEntries.Size() == 0)
  {
    _fixupSize = 0;
    return;
  }

  HRESULT err;
  ComRef<IDirect3DFixup8> fuTemp;
  err = GEngineDD->GetDirect3DDevice()->CreateFixup(0, fuTemp.Init());
  Assert(err == S_OK);

  // Apply all the fixups
  _pb->BeginFixup(fuTemp, FALSE);
  for (int i = 0; i < _fixupEntries.Size(); i++)
  {
    _fixupEntries[i]->Apply(_pb.GetRef());
  }
  _pb->EndFixup();

  // Reset the cached values
  for (int i = 0; i < _fixupEntries.Size(); i++)
  {
    _fixupEntries[i]->Reset();
  }

  // Save the size of the fixup
  fuTemp->GetSize(&_fixupSize);
}

void PushBufferXB::ApplyFixups()
{
  if (_fixupEntries.Size() == 0)
  {
    return;
  }

  // Find the first changed fix-up (if any)
  int changedFixupIndex = 0;
#define DO_ALWAYS 0
#if !DO_ALWAYS
  for (; changedFixupIndex < _fixupEntries.Size(); changedFixupIndex++)
  {
    if (_fixupEntries[changedFixupIndex]->HasChanged())
    {
      break;
    }
  }
#endif

  // If something has changed then fix it up
  if (changedFixupIndex < _fixupEntries.Size())
  {
    ADD_COUNTER(fuNum,1);

    // Get the fixup
    _fuToSet = GFixupManager.GetFixup(_fixupSize);

    // Reset the fixup (wait for HW release)
    _fuToSet->Reset();

    // Begin
    _pb->BeginFixup(_fuToSet, FALSE);

    // Go through the rest of the FU entries and fix it up (if changed)
    for (int i = changedFixupIndex; i < _fixupEntries.Size(); i++)
    {
#if !DO_ALWAYS
      if (_fixupEntries[i]->HasChanged())
#endif
      {
        _fixupEntries[i]->Apply(_pb.GetRef());
      }
    }

    // End
    _pb->EndFixup();

    // Everything has been fix-upped right now
    _textureHasChanged = false;
    _nightEye = GEngineDD->GetNightEye();
    _sunDiffuse = GScene->MainLight()->GetDiffuse() * GEngine->GetAccomodateEye();
  }
}

#include <El/Statistics/statistics.hpp>

StatisticsByName pbSizeStats;

size_t PushBufferXB::GetMemoryControlled() const
{
  size_t total = 0;
  if (_pb.NotNull())
  {
    DWORD pbSize;
    _pb->GetSize(&pbSize);

    #if CUSTOM_PB_ALLOCATOR
      // this is the main reason for custom allocator
      // otherwise we are wasting a lot here, esp. for small pushbuffers
      const int allocGranularity = 256;
    #else
      // round up to nearest 4 KB
      const int allocGranularity = 4*1024;
    #endif
    pbSize = (pbSize+allocGranularity-1)&~(allocGranularity-1);
    
    total += pbSize;
    // note: fixup is never smaller than 4 KB (smallest allocation granularity)
    //const int oneFixupSize = 4*1024;
    //total += FU_NUM*oneFixupSize;
  }
  return total;
}

PushBufferXB::PushBufferXB()
{
}
PushBufferXB::~PushBufferXB()
{
  if (_pb.NotNull())
  {
    GEngineDD->DestroyHWPushBuffer(_pb);
  }
}


bool PushBufferXB::Init(Shape *shape, const LODShape *lodShape, RefArray<Texture> *usedTextures, int instancesCount)
{
#ifdef _DEBUG
#ifdef _XBOX
  //D3D__SingleStepPusher = TRUE;
#endif
#endif

  // Set the flag to default value
  _textureHasChanged = true;
  _nightEye = -1.0f;
  _sunDiffuse = Color(-1, -1, -1, -1);

  // Check the size of the VB to be large enough to hold the number of instances
  if (instancesCount > 0)
  {
    VertexBufferD3DXB *vb = static_cast<VertexBufferD3DXB *>(shape->GetVertexBuffer());
    int nInstancesInOneStep = vb->GetVBufferSize() / shape->NVertex();
    if (nInstancesInOneStep < instancesCount) return false;
  }

  // Create the pushbuffer
  const LightList empty;
  AnimatorEmpty ae;
  HRESULT err;
  const int pbTempSize = 1024*100;

  // assume pushbuffers are always rendered with HDR on
  float modulate = GEngineDD->GetHDRFactor();
  GEngineDD->SetModulateColor(Color(modulate, modulate, modulate));

  GEngineDD->SetHWToDefinedState();


  ESkinningType skinning = (lodShape && lodShape->GetAnimationType()==AnimTypeHardware) ? STPoint4 : STNone;
  GEngineDD->SetSkinningType(skinning);
  // vertex shader can be changed without having any effect on the pushbuffer
  //  GEngineDD->SetupSectionTL(
  //    shape->GetVertexDeclaration(),instancesCount>0,
  //    lodShape->GetAnimationType()==AnimTypeHardware ? STPoint4 : STNone,
  //    vsID,ml,fm
  //  );
  _fixupEntries.Clear();
  GEngineDD->_activePushBuffer = this;

  ComRef<IDirect3DPushBuffer8> pbTemp;
  err = GEngineDD->GetDirect3DDevice()->CreatePushBuffer(pbTempSize, false, pbTemp.Init());
  DoAssert(err == S_OK);

  err = GEngineDD->GetDirect3DDevice()->BeginPushBuffer(pbTemp);
  Assert(err == S_OK);

#if _ENABLE_PERFLOG
  COUNTER(tris);
  int startTris = COUNTER_VALUE(tris);
#endif

  GEngineDD->BeginRecording();

  GEngineDD->ClearStateCaches();
  GEngineDD->_vBufferLast = NULL;
  GEngineDD->_vBufferSize = 0;
  
  if (instancesCount > 0)
  {
    shape->DrawInstanced(
      SectionMaterialLODsArray(),empty, *lodShape, ClipAll, shape->Special(),
      NULL, instancesCount, 0, false, DrawParameters::_default
    );
  }
  else
  {
    shape->Draw(
      &ae, SectionMaterialLODsArray(), *lodShape, empty, ClipAll, shape->Special(), MIdentity,
      0,DrawParameters::_default
    );
  }
  GEngineDD->SetHWToDefinedState();

  GEngineDD->_activePushBuffer = NULL;

  GEngineDD->EndRecording();
  GEngine->SetSkinningType(STNone);

#if _ENABLE_PERFLOG
  _tris = COUNTER_VALUE(tris) - startTris;
#endif

  err = GEngineDD->GetDirect3DDevice()->EndPushBuffer();
  Assert(err == S_OK);

  // Compact the fixup entries array
  _fixupEntries.Compact();

  // Gather all the used textures
  if (usedTextures)
  {
    usedTextures->Clear();
    for (int i = 0; i < _fixupEntries.Size(); i++)
    {
      _fixupEntries[i]->StoreResources(*usedTextures);
    }
    usedTextures->Compact();
  }

  DWORD pbSize;
  pbTemp->GetSize(&pbSize);
#if _ENABLE_REPORT
  static DWORD maxPushBufferSize = 0;
  if (pbSize > pbTempSize)
  {
    if (lodShape)
    {
      RptF("Error: Push-buffer for model %s is too large - %d Bytes", lodShape->GetName().Data(), pbSize);
    }
    else
    {
      RptF("Error: Push-buffer for unknown model is too large - %d Bytes", pbSize);
    }
    _fixupEntries.Clear();
    if (usedTextures) usedTextures->Clear();
    return false;
  }
  if (maxPushBufferSize < pbSize)
  {
    maxPushBufferSize = pbSize;
    //LogF("New maximum push-buffer size is %d", maxPushBufferSize);
  }
#endif
  _pb = GEngineDD->CreateHWPushBuffer(pbSize);
  DoAssert(err == S_OK);

  // Mem copy PB
  memcpy((void*)_pb->Data, (void*)pbTemp->Data, pbSize);
  _pb->Size = pbSize; // ???

#ifdef _DEBUG

  //DWORD result = _pb->Verify(false);

#endif



  // Final clear
  GEngineDD->ClearStateCaches();

  // While recording PB in method SetVertexShaderInputDirect is old VB being released and therefore
  // is InsertFence called, but it is not possible to call InsertFence while recording.
  // With this no VB will be released while recording.
  GEngineDD->_vBufferLast = NULL;
  GEngineDD->_vBufferSize = 0;
  GEngineDD->GetDirect3DDevice()->SetVertexShaderInputDirect(NULL, 0, NULL);


  //////////////////////////////////////////////////////////////////////////

  // Retrieve the size of the future fixup
  CalculateFixupSize();

  return true;
}

void PushBufferXB::Prepare(const Shape &shape, int spec)
{
  {
    //PROFILE_SCOPE_GRF(fixup,SCOPE_COLOR_YELLOW);
    Color sunDiffuse = GScene->MainLight()->GetDiffuse() * GEngine->GetAccomodateEye();
    if (_textureHasChanged || (GEngineDD->GetNightEye() != _nightEye) || (sunDiffuse != _sunDiffuse))
    {
      ApplyFixups();
    }
  }
}

extern bool DrawPBStuff;

void PushBufferXB::Run(const Shape &shape, bool instancing)
{
  //PROFILE_SCOPE_GRF(pbDrw,SCOPE_COLOR_RED);


  //DWORD result = _pb->Verify(false);
  //Assert(result == S_OK);

  // Draw the object
#ifdef _DEBUG
#ifdef _XBOX
  //D3D__SingleStepPusher = TRUE;
#endif
#endif

  // Since we use PB we are sure the shape has got just one material,
  // so read it f.i. from the first section
  DoAssert(shape.NSections() > 0);
  const ShapeSection &sec = shape.GetSection(0);
  TexMaterial *mat = sec.GetMaterialExt();
  VertexShaderID firstVS;
  EMainLight firstML;
  EFogMode firstFM;
  bool landShadowEnabled = false;
  bool alphaInstancing = false;
  if (mat)
  {
    TexMaterial::Loaded m(mat);
    firstVS = m.GetVertexShaderID(0);
    firstML = m.GetMainLight();
    firstFM = m.GetFogMode();
    landShadowEnabled = m.GetRenderFlag(RFLandShadow);
    alphaInstancing = m.GetRenderFlag(RFAlphaInstancing);
  }
  else
  {
    firstVS = VSBasic;
    firstML = ML_Sun;
    firstFM = FM_Fog;
  }

  {
    //PROFILE_SCOPE_GRF(seSec,SCOPE_COLOR_GREEN);
    GEngineDD->SetupSectionTL(
      shape.GetVertexDeclaration(), instancing, GEngineDD->GetSkinningType(), firstVS, firstML, firstFM,
      GEngineDD->GetAlphaShadowEnabled(),landShadowEnabled,alphaInstancing
    );
  }

  {
    PROFILE_SCOPE_GRF(pbRun,SCOPE_COLOR_RED);

    // Make sure HW is in defined state
    DoAssert(GEngineDD->_hwInDefinedState);

    // D3DRS_STENCILREF needs to be set - better would be to split DoShadowsStatesSettings into 2 functions
    // - the object part and the shadow part. We must force the flag to determine we are still in defined state
    GEngineDD->D3DSetRenderState(D3DRS_STENCILREF, GEngineDD->GetStencilRefValue());
    GEngineDD->_hwInDefinedState = true;

    // Run the pushbuffer
    if (DrawPBStuff)
    {
      HRESULT err = GEngineDD->GetDirect3DDevice()->RunPushBuffer(_pb.GetRef(), _fuToSet);
      (void) err;
      Assert(err == S_OK);

      // pushbuffer changed VB source (SetVertexShaderInputDirect is recorded in it)
      // it is very unlikely anything else would share the same buffer and offset
      // set invalid data
      GEngineDD->_vBufferLast = NULL;
      GEngineDD->_vBufferSize = 0;
      GEngineDD->_vOffsetLast = 0;
      GEngineDD->_vsdVersionLast = VSDV_Basic;
      /* following code might be used to set valid data if wanted
      VertexBufferD3DXB *vb = static_cast<VertexBufferD3DXB *>(shape.GetVertexBuffer().GetRef());
      if (vb && vb->_sharedV)
      {
        VertexStaticDataXB *dtaV = vb->_sharedV;
        _vBufferLast = dtaV->_vBuffer;
        _vBufferSize = dtaV->Size();
        _vOffsetLast = vOffset;
      }
      */
    }
    _fuToSet = NULL;

    // Save fence (Note that faster way to get a fence is to
    // read the Lock member of _pb - kickPushBuffer is not performed then)
    //DWORD fence = GEngineDD->GetDirect3DDevice()->InsertFence();
    //DWORD fence = _pb->Lock;
    GEngineDD->SetPBLock(_pb->Lock);

#if _ENABLE_PERFLOG
    ADD_COUNTER(tris,_tris);
#endif
  }

}

bool PushBufferXB::SerializeBin(SerializeBinStream &f)
{
  // Transfer push buffer code
  if (f.IsLoading())
  {
    // Get size of the push buffer
    int pbSize = f.LoadInt();

    // Create and load the push buffer
    _pb = GEngineDD->CreateHWPushBuffer(pbSize);
    #if CUSTOM_PB_ALLOCATOR
      f.Load(_pb.mem.GetAddress(), pbSize);
    #else
      f.Load((void*)_pb->Data, pbSize);
    #endif
    _pb->Size = pbSize; // CreateHWPushBuffer does not set resource size
  }
  else
  {
    // Save size of the push buffer
    int pbSize = _pb->Size;
    f.SaveInt(pbSize);

    // Save the push buffer
    #if CUSTOM_PB_ALLOCATOR
      f.Save(_pb.mem.GetAddress(), pbSize);
    #else
      f.Save((void*)_pb->Data, pbSize);
    #endif
  }

//  VertexShaderID vsIDTemp;
//  EMainLight mlTemp;
//  EFogMode fmTemp;
//  f.TransferBinary(vsIDTemp);
//  f.TransferBinary(mlTemp);
//  f.TransferBinary(fmTemp);

  // Transfer fixup entries
  if (f.IsLoading())
  {
    int size = f.LoadInt();
    _fixupEntries.Realloc(size);
    _fixupEntries.Resize(size);
    for (int i = 0; i < size; i++)
    {
      Ref<FixupEntry> &item = _fixupEntries[i];
      FEType t;
      f.TransferBinary(t);
      switch (t)
      {
      case FET_Texture:     item = new FETexture(this);	  break;
      case FET_MaxColor:    item = new FEMaxColor(this);  break;
      case FET_AvgColor:    item = new FEAvgColor(this);  break;
      case FET_Diffuse:     item = new FEDiffuse();       break;
      case FET_DiffuseBack: item = new FEDiffuseBack();   break;
      case FET_Specular:    item = new FESpecular();      break;
      default: RptF("Error: Unknow type of fixup");
      }
      item->SerializeBin(f);
    }
  }
  else
  {
    // Save size of the array
    int size = _fixupEntries.Size();
    f.SaveInt(size);
    for (int i = 0; i < size; i++)
    {
      // Save type
      FEType t = _fixupEntries[i]->FixupEntryType();
      f.TransferBinary(t);

      // Save content
      _fixupEntries[i]->SerializeBin(f);
    }
  }

  // Fixup size
  f.TransferBinary(_fixupSize);

  // SetStreamSource command arguments
  f.TransferBinary(_setStreamSourceOffset);

  #if _ENABLE_PERFLOG
    f.TransferBinary(_tris);
  #else
    // no perflog means we are loading, as in superrelease build no PB saving is done
    int dummy = 0;
    f.TransferBinary(dummy);
  #endif
  // If loading then zero some values
  if (f.IsLoading())
  {
    _fuToSet = NULL;
    _textureHasChanged = true;
    _nightEye = -1.0f;
    _sunDiffuse = Color(-1, -1, -1, -1);
    GEngineDD->_activePushBuffer = NULL;
  }

  return true;
}

void PushBufferXB::VertexBufferHasChanged(Ref<VertexBuffer> &buffer, VBType type, EVertexDecl vertexDeclaration, bool instanced)
{

  // Apply the vertex buffer fixup
  _pb->BeginFixup(NULL, FALSE);
  VertexBufferD3DXB *vb = static_cast<VertexBufferD3DXB *>(buffer.GetRef());
  VertexStaticDataXB *dtaV = vb->_sharedV;
  D3DSTREAM_INPUT si;
  si.VertexBuffer = dtaV->_vBuffer;
  si.Stride = dtaV->VertexSize();
  si.Offset = vb->_vOffset * dtaV->VertexSize();
  VSDVersion vsdv;
  if (instanced)
  {
    vsdv = VSDV_Instancing;
  }
  else
  {
    if (type == VBStaticSkinned)
    {
      vsdv = VSDV_Skinning;
    }
    else
    {
      vsdv = VSDV_Basic;
    }
  }
  _pb->SetVertexShaderInputDirect(_setStreamSourceOffset, VAFVSDecl[vertexDeclaration][vsdv], 1, &si);
  _pb->EndFixup();
}

#ifdef NDEBUG
#define Verify( expr ) (expr)
#else
#define Verify( expr ) DoAssert(expr)
#endif

VertexDynamicDataXB::VertexDynamicDataXB()
{
  _actualVBIndex = 0;
  _vBufferSize = 0;
  _vBufferUsed = 0;
}

void VertexDynamicDataXB::Init(int size, IDirect3DDevice8 *dev)
{
  DWORD usage = WRITEONLYVB|D3DUSAGE_DYNAMIC;
  int byteSize = size*sizeof(SVertexXB);
  for (int i=0; i<NumDynamicVB3D; i++)
  {
RetryV:
    /*
    HRESULT err=dev->CreateVertexBuffer
    (
    byteSize,usage,MYSFVF,D3DPOOL_DEFAULT,
    _vBuffer[i].Init()
    );
    */
    HRESULT err=dev->CreateVertexBuffer
      (
      byteSize,usage,0,D3DPOOL_DEFAULT,
      _vBuffer[i].Init()
      );
    if (err!=D3D_OK)
    {
      if (err==D3DERR_OUTOFVIDEOMEMORY)
      {
        LogF("Out of VRAM during v-buffer creation");
        if (GEngineDD->TextBankDD()->ForcedReserveMemory(byteSize))
        {
          goto RetryV;
        }
      }
      DDErrorXB("CreateVertexBuffer",err);
      return;
      // 
    }
  }
  _actualVBIndex = 0;
  _vBufferSize = size;
  _vBufferUsed = 0;
}
void VertexDynamicDataXB::Dealloc()
{
  for (int i=0; i<NumDynamicVB3D; i++)
  {
    _vBuffer[i].Free();
  }
}

int VertexDynamicDataXB::AddVertices(const Shape &src)
{
  const int nVertex = src.NVertex();
  SVertexXB *sData;
  int vOffset = Lock(sData, nVertex);
  if (vOffset >= 0)
  {
    ShapeGeometryLock<> lock(&src);

    // Calculate the vertex buffer maximum and minimum Pos's
    Vector3 minPos, maxPos;
    src.CalculateMaxAndMinPos(maxPos, minPos, true);
    Vector3 scalePos = maxPos - minPos;
    Assert(maxPos.X() != -1e6);
    Assert(maxPos.Y() != -1e6);
    Assert(maxPos.Z() != -1e6);
    Assert(minPos.X() != 1e6);
    Assert(minPos.Y() != 1e6);
    Assert(minPos.Z() != 1e6);

    saturateMax(scalePos[0],1e-6f);
    saturateMax(scalePos[1],1e-6f);
    saturateMax(scalePos[2],1e-6f);

    GEngineDD->SetPosScaleAndOffset(scalePos, minPos);

    // Calculate the vertex buffer maximum and minimum UV's
    UVPair minUV, maxUV;
    src.CalculateMaxAndMinUV(maxUV, minUV);
    UVPair scale;
    scale.u = maxUV.u - minUV.u;
    scale.v = maxUV.v - minUV.v;
    saturateMax(scale.u,1e-6f);
    saturateMax(scale.v,1e-6f);
    Assert(maxUV.u != -1e6);
    Assert(maxUV.v != -1e6);
    Assert(minUV.u != 1e6);
    Assert(minUV.v != 1e6);
    GEngineDD->SetTexGenScaleAndOffset(scale, minUV);

    const CondensedAutoArray<UVPair> *uv = &src.GetUVArray();
    const AutoArray<Vector3> *pos = &src.GetPosArray();
    const CondensedAutoArray<Vector3> *norm = &src.GetNormArray();
    Vector3 invScale = CalcInvScale(maxPos, minPos);
    UVPair invUV = CalcInvScale(maxUV, minUV);
    int nVertex = src.NVertex();
    for (int i = 0; i < nVertex; i++)
    {
      CopyPos(sData->pos, (*pos)[i], maxPos, minPos, invScale);
      CopyNormal(sData->norm,(*norm)[i]);
      CopyUV(sData->t0, (*uv)[i], maxUV, minUV, invUV);
      sData++;
    }

    Unlock();
  }
  return vOffset;
}

int VertexDynamicDataXB::Lock(SVertexXB *&vertex, int n)
{
  DWORD flags = NoSysLockFlag;
  if (_vBufferUsed + n >= _vBufferSize)
  {
    // discard old - start a new buffer
#if CUSTOM_VB_DISCARD
    if (++_actualVBIndex >= NumDynamicVB3D)
    {
      _actualVBIndex = 0;
    }
#endif
    flags |= DiscardLockFlag;
    _vBufferUsed = 0;
    if (n > _vBufferSize)
    {
      ErrorMessage("Cannot lock vertices - v-buffer too small (%d<%d)", _vBufferSize, n);
      return -1;
    }   
  }
  else
  {
    // fit in current buffer
    flags |= D3DLOCK_NOOVERWRITE;
  }

  int size = sizeof(SVertexXB)*n;
  int offset = sizeof(SVertexXB)*_vBufferUsed;
Retry:
  BYTE *data = NULL;
  HRESULT err;
  {
    PROFILE_DX_SCOPE(3vbLD);
    err = _vBuffer[_actualVBIndex]->Lock(offset, size, &data, flags);
  }
  if (err!=D3D_OK || !data)
  {
    if (QFBank::FreeUnusedBanks(1024*1024)) goto Retry;
#ifndef _XBOX
    ErrF("VB Lock failed, %s",DXGetErrorString8(err));
#else
    ErrF("VB Lock failed, %x",err);
#endif
    return -1;
  }
  int ret = _vBufferUsed;
  _vBufferUsed += n;

  vertex = (SVertexXB *)data;

  return ret;
}

void VertexDynamicDataXB::Unlock()
{
  HRESULT err;
  err = _vBuffer[_actualVBIndex]->Unlock();
  if (err != D3D_OK)
  {
    DDErrorXB("Cannot unlock vertex buffer.", err);
  }
}

size_t VertexBufferD3DXB::GetBufferSize() const
{
  size_t total = 0;
  if (_vBuffer)
  {
    total += _vBufferSize*sizeof(SVertexXB);
  }
  if (_iBuffer)
  {
    total += _iBufferSize*sizeof(VertexIndex);
  }
  if (_sharedV)
  {
    total += _vAlloc->Size()*_sharedV->VertexSize();
  }
  if (_sharedI)
  {
    total += _iAlloc->Size()*sizeof(VertexIndex);
  }
  if (GetOwner())
  {
    total += GetOwner()->GetMemoryControlledByProgram();
  }
  Assert(total<10*1024*1024);
  return total;
}

VertexBufferD3DXB::VertexBufferD3DXB()
{
  _iOffset = 0;
  _vOffset = 0; // offsetting done by D3D
  _maxPos = Vector3(-1e6, -1e6, -1e6);
  _minPos = Vector3(1e6, 1e6, 1e6);
  _maxUV.u = -1e6;
  _maxUV.v = -1e6;
  _minUV.u = 1e6;
  _minUV.v = 1e6;

  _iAlloc = NULL;
  _vAlloc = NULL;
}

void EngineDDXB::BufferReleased(VertexBufferD3DXB *buf)
{
  _vBufferLRU.Delete(buf);
}

void EngineDDXB::OnSizeChanged(VertexBufferD3DXB *buf)
{
  _vBufferLRU.RefreshMemoryControlled(buf);
}

VertexBufferD3DXB::~VertexBufferD3DXB()
{
  if (IsInList())
  {
    GEngineDD->BufferReleased(this);
  }
  if (_sharedV)
  {
    _sharedV->RemoveShape(_vAlloc);
    if (_sharedV->IsEmpty())
    {
      GEngineDD->DeleteVB(_sharedV);
    }
  }
  if (_sharedI)
  {
    _sharedI->RemoveShape(_iAlloc);
    if (_sharedI->IsEmpty())
    {
      GEngineDD->DeleteIB(_sharedI);
    }
  }
#if 0
  if (_vBuffer)
  {
    char name[256];
    sprintf(name,"VB %x (%x)",_vBuffer,this);
    vbStats.Count(name,-1);

    //LogF("Destroyed VB %x (%x)",_vBuffer,this);
  }
#endif
  /*
  if (_iBuffer)
  {
  //LogF("Destroy IB %x",_iBuffer);
  }
  */
  if (_vBuffer)
  {
    GEngineDD->AddVBuffersAllocated(-_vBufferSize*sizeof(SVertexXB));
    _vBuffer.Free();
  }

  if (_iBuffer)
  {
    GEngineDD->AddIBuffersAllocated(-_iBufferSize*sizeof(VertexIndex));
    _iBuffer.Free();
  }
}

VertexBuffer *EngineDDXB::CreateVertexBuffer(const Shape &src, VBType type, bool frequent)
{
  PROFILE_SCOPE(xbCVB);
  if (!_useDXTL) return NULL;
  VertexBufferD3DXB *buffer = new VertexBufferD3DXB;
  if (buffer->Init(this, src, type, frequent))
  {
    // insert it into the LRU cache
    // as it is only created, and not used, we might insert it at the very end ???
    Assert(!buffer->IsInList());
    buffer->SetOwner(&src);
    _vBufferLRU.Add(buffer);
    return buffer;
  }
  else
  {
    delete buffer;
    return NULL;
  }
}

void VertexBufferD3DXB::Detach(const Shape &src)
{
  DoAssert(GetOwner()==NULL || &src==GetOwner());
  SetOwner(NULL);
}

void VertexBufferD3DXB::RemoveIndexBuffer()
{
  if (_sharedI)
  {
    _sharedI->RemoveShape(_iAlloc);
    if (_sharedI->IsEmpty())
    {
      GEngineDD->DeleteIB(_sharedI);
    }
    _sharedI = NULL;
  }
  if (_iBuffer)
  {
    GEngineDD->AddIBuffersAllocated(-_iBufferSize*sizeof(VertexIndex));
    _iBuffer.Free();
  }
}

int EngineDDXB::CompareBuffers(const Shape &s1, const Shape &s2)
{
  // check if there are some vertex buffers
  const VertexBufferD3DXB *b1 = static_cast<const VertexBufferD3DXB *>(s1.GetVertexBuffer());
  const VertexBufferD3DXB *b2 = static_cast<const VertexBufferD3DXB *>(s2.GetVertexBuffer());
  int dd;
  if (!b1 && !b2) return 0;
  if (!b1) return -1;
  if (!b2) return +1;
  dd = b1->_separate && b2->_separate;
  if (dd) return dd;
  dd = b1->_dynamic-b2->_dynamic;
  if (dd) return dd;
  dd = (char *)b1->_sharedV.GetTypeRef()-(char *)b2->_sharedV.GetTypeRef();
  if (dd) return dd;
  dd = (char *)b1->_sharedI.GetTypeRef()-(char *)b2->_sharedI.GetTypeRef();
  return dd;
}

struct SaveContext
{
  QFBank *bank;
};

const char PBPoolTempFolder[] = "D:\\_PBPool";

static void CreateAndSavePushBuffer(const FileInfoO &fi, const FileBankType *files, void *context)
{
  #if LOAD_PB_FROM_DISK
  //if (strcmpi(fi.name, "hous\\zidka04.p3d")) return;
  //if (strcmpi(fi.name, "hous\\benzina.p3d")) return;
  const char *ext = GetFileExt(GetFilenameExt(fi.name));
  if (!strcmpi(ext, ".p3d"))
  {
    // Get some memory
    FreeOnDemandGarbageCollect(4*1024*1024, 2*1024*1024);

    // Create the LODShape
    SaveContext *sc = (SaveContext *)context;
    RString sFileName = sc->bank->GetPrefix() + fi.name;
    Ref<LODShapeWithShadow> lodShape = new LODShapeWithShadow(sFileName);

    // Create the VB and PB
    int pbCount = 0;
    for (int i = 0; i < lodShape->NLevels(); i++)
    {
      if (lodShape->LevelUsesVBuffer(i))
      {
        ShapeUsed shape = lodShape->Level(i);
        shape->ConvertToVBuffer(VBStatic, lodShape, i);
        shape->CreatePushBuffer(VBStatic, lodShape, i);
        if (shape->ProgramExists()) pbCount++;
      }
    }

    // Create file only in case some LOD's have got the PB
    if (pbCount > 0)
    {
      // Create model directory
      BString<1024> folder;
      folder.StrNCpy(sFileName, GetFilenameExt(sFileName.Data()) - sFileName.Data());
      void CreatePath(RString path);
      CreatePath(PBPoolTempFolder + RString("\\") + RString(folder));

      // Create name of the file
      BString<1024> fileName;
      sprintf(fileName, "%s\\%s%s", PBPoolTempFolder, sc->bank->GetPrefix().Data(), fi.name.Data());
      int fnSize = GetFileExt(GetFilenameExt(fileName.cstr())) - fileName.cstr();
      fileName[fnSize] = 0;
      const char ex[] = ".pbx";
      strcat(fileName, ex);

      // Create file and save the header
      QOFStream qofs(fileName);
      SerializeBinStream f(&qofs);
      GEngineDD->SerializeBinPBHeader(f);

      // Save the LOD table tableSize, (LOD index, offset)... and remember fixups
      int lodOffsetAdress[MAX_LOD_LEVELS];
      f.SaveInt(pbCount);
      for (int i = 0; i < lodShape->NLevels(); i++)
      {
        if (lodShape->LevelUsesVBuffer(i))
        {
          ShapeUsed shape = lodShape->Level(i);
          if (shape->ProgramExists())
          {
            f.SaveInt(i);

            bool permanent = lodShape->IsPermanent(lodShape->Resolution(i)) || i==lodShape->FindSimplestLevel();
            f.SaveChar(permanent);
            
            lodOffsetAdress[i] = f.CreateFixUp();
          }
          else
          {
            lodOffsetAdress[i] = -1;
          }
        }
        else
        {
          lodOffsetAdress[i] = -1;
        }
      }

      // Save LODs, fix the offset up, release VBuffer and PB
      for (int i = 0; i < lodShape->NLevels(); i++)
      {
        if (lodShape->LevelUsesVBuffer(i))
        {
          ShapeUsed shape = lodShape->Level(i);
          if (shape->ProgramExists())
          {
            f.FixUp(lodOffsetAdress[i]);
            shape->SerializeBinPushBuffer(f);
          }
          shape->ReleaseVBuffer();
        }
      }
    }

    // Save push buffers for all the LODs


//    for (int i = 0; i < lodShape->NLevels(); i++)
//    {
//      if (lodShape->LevelUsesVBuffer(i))
//      {
//        ShapeUsed shape = lodShape->Level(i);
//        shape->ConvertToVBuffer(VBStatic, lodShape, i);
//        shape->SavePushBuffer(PBPoolTempFolder, lodShape, i);
//        shape->ReleaseVBuffer();
//      }
//    }
  }
  #endif
}

static void BankHasAnyP3D(const FileInfoO &fi, const FileBankType *files, void *context)
{
  //if (strcmpi(fi.name, "krater.p3d")) return;
  const char *ext = GetFileExt(GetFilenameExt(fi.name));
  if (!strcmpi(ext, ".p3d"))
  {
    bool *flag = (bool*)context;
    *flag = true;
  }
}

void EngineDDXB::CreateAllPushBuffers()
{
  #if LOAD_PB_FROM_DISK
  bool createPB;
  if (!QFBankQueryFunctions::AutoBank("PBPool\\"))
  {
#if _SUPER_RELEASE
    Fail("Error: File PBPool.pbo not found");
#endif
    // File PBPool doesn't exist
    createPB = true;
  }
  else
  {
#if _SUPER_RELEASE
    createPB = false;
#else
    // Go through all the banks and get the youngest one time
    QFileTime pbPoolTimeStamp = 0;
    QFileTime youngestBankTime = 0;
    for (int i = 0; i < GFileBanks.Size(); i++)
    {
      QFBank &bank = GFileBanks[i];
      bool bankHasAnyP3D = false;
      bank.ForEach(BankHasAnyP3D, &bankHasAnyP3D);
      if (bankHasAnyP3D)
      {
        QFileTime bankTime = bank.TimeStamp();
        if (bankTime > youngestBankTime) youngestBankTime = bankTime;
      }
      if (bank.GetPrefix() == RString("pbpool\\"))
      {
        pbPoolTimeStamp = bank.TimeStamp();
      }
    }
    if (youngestBankTime > pbPoolTimeStamp)
    {
      createPB = true;
    }
    else
    {
      createPB = false;
    }
#endif
  }

  // Create the PB if necessary
  if (createPB)
  {
    IProgressDisplay *CreateDisplayProgress(RString text, RString resource);
    // Fill out shader caches and save them into file
    LogF("PBPool not found - creating of push buffers");
    Ref<ProgressHandle> p = ProgressStart(
      CreateDisplayProgress("Generating push-buffers", "RscDisplayNotFreezeBig"),
      true // we want this to appear on DS as well
    );

    // Delete the temporary folder
    bool DeleteDirectoryStructure(const char *name, bool deleteDir);
    DeleteDirectoryStructure(PBPoolTempFolder, false);

    // Delete the PBPool.pbo file
    GFileBanks.Unlock(RString("pbpool\\"));
    QIFileFunctions::CleanUpFile("#:\\dta\\PBPool.pbo");

    // Create temporary folder
    ::CreateDirectory(PBPoolTempFolder, NULL);

    // Initialize textures like "white"
    _textBank->InitDetailTextures();

    // Go through all the banks and create push-buffer for all the *.p3d files
    for (int i = 0; i < GFileBanks.Size(); i++)
    {
      QFBank &bank = GFileBanks[i];
      SaveContext saveContext;
      saveContext.bank = &bank;
      bank.ForEach(CreateAndSavePushBuffer, &saveContext);
    }

    // Create the PBO from the temporary folder
    FileBankManager mgr;
    mgr.Create("D:\\dta\\PBPool.pbo", PBPoolTempFolder);

    ProgressFinish(p);
    p = ProgressStart(
      CreateDisplayProgress("Generated push-buffers, use putXPBPool.bat", "RscDisplayNotFreezeBig"),
      true // we want this to appear on DS as well
    );
    ErrorMessage("");
  }
  #endif
}


PushBuffer *EngineDDXB::CreatePushBuffer()
{
  return new PushBufferXB;
}

PushBufferHandle EngineDDXB::CreateHWPushBuffer(DWORD size)
{
  #if CUSTOM_PB_ALLOCATOR
    PushBufferHandle ret;
    ret.com = NULL;
    ret.mem = AllocatePhysicalMemory(size);
    if (ret.mem.IsNull())
    {
      return ret;
    }
    ret.com = new IDirect3DPushBuffer8();
    ret.com->AddRef();
    ret.com->Data = (DWORD)ret.mem.GetAddress();
    //ret.com->Register(ret.mem.GetAddress());
    return ret;
  #else
    ComRef<IDirect3DPushBuffer8> ret;
    // default D3D handling of the _pb resource
    HRESULT ok = _d3DDevice->CreatePushBuffer(size,false,ret.Init());
    if (FAILED(ok)) return NULL;
    return ret;
  #endif
}
void EngineDDXB::DestroyHWPushBuffer(PushBufferHandle &pb)
{
  #if CUSTOM_PB_ALLOCATOR
    if (pb->IsBusy())
    {
      LogF("PB released while busy");
    }
    pb->BlockUntilNotBusy();
    FreePhysicalMemory(pb.mem);
    delete pb.com;
    pb.com = NULL;
  #else
    pb.Free();
  #endif
}


class DCPushBuffer : public DrawCommand
{
private:
  /// 0 is noninstanced, 1 is instanced with 1 (1<<0) instance, 2 = 2 (1<<1), 3 = 4 (1<<2)
  Ref<PushBuffer> _pb[PBInstancesLogMax + 2];
  /// total memory used by this object
  int _memoryControlled;
  //! Textures used on the model this draw command represents
  RefArray<Texture> _usedTextures;
public:
  DCPushBuffer(Shape *shape, const LODShape *lodShape);
  //! Constructor used only before serialization
  DCPushBuffer()
  {
    _memoryControlled = -1;
  }
  virtual int GetAvailableInstances(int instances);
  virtual void Prepare(const Shape &shape, int spec);
  virtual void Finish(const Shape &shape);
  virtual void PrepareItem(const Shape &shape, int spec, int instancesCount);
  virtual void Do(const Shape &shape, int instancesCount);
  virtual void Display() {};
  virtual size_t GetMemoryControlled() const;
  virtual bool SerializeBin(SerializeBinStream &f);
  virtual void VertexBufferHasChanged(Ref<VertexBuffer> &buffer, VBType type, EVertexDecl vertexDeclaration);
};

class PLPushBuffer : public ProgramLoader
{
protected:
  QFBankHandle _handle;
public:
  PLPushBuffer(QFBankHandle &handle)
  {
    _handle = handle;
  }
  static RString GetFileName(RString lsName)
  {
    RString fileName = RString("PBPool\\") + lsName;
    int fnSize = GetFileExt(GetFilenameExt(fileName)) - fileName;
    fileName = RString(fileName, fnSize);
    fileName = fileName + ".pbx";
    return fileName;
  }
  static bool RequestHeader(FileRequestPriority prior, const char *name)
  {
    RString pbName = GetFileName(name);
    QIFStreamB in;
    in.AutoOpen(pbName, 0,
      sizeof(int)                         // File type
      + sizeof(int)                       // Version
      + sizeof(int)                       // Level count
      + MAX_LOD_LEVELS * sizeof(int) * 2  // Index and offset for each level
      );
    if (in.fail())
    {
      return true;
    }
    else
    {
      return in.PreRead(prior);
    }
  }
  virtual bool RequestProgram()
  {
    QIFStreamB fs;
    fs.open(_handle);
    // handle stores exact limits for the face array
    return fs.PreRead(FileRequestPriority(10));
  }
  virtual void LoadProgram(RefArray<DrawCommand> &program)
  {
    PROFILE_SCOPE(xbPBL);
    if (program.Size() > 0)
    {
      LogF("Warning: Program already loaded");
      return;
    }
    Ref<DrawCommand> pb = new DCPushBuffer();
    QIFStreamB fs;
    fs.open(_handle);
    SerializeBinStream f(&fs);
    pb->SerializeBin(f);
    program.Add(pb);
    program.Compact();
  }
  virtual void UnloadProgram(RefArray<DrawCommand> &program)
  {
    program.Clear();
  }
};

//////////////////////////////////////////////////////////////////////////

DCPushBuffer::DCPushBuffer(Shape *shape, const LODShape *lodShape)
{
  _memoryControlled = 0;
  for (int i = 0; i <= PBInstancesLogMax+1; i++)
  {
    //DWORD start = GlobalTickCount();
    if ((!lodShape || (lodShape->GetAnimationType() == AnimTypeHardware)) && (i > 0)) break;
    _pb[i] = GEngine->CreatePushBuffer();
    if (_pb[i].NotNull())
    {
      if (!_pb[i]->Init(shape, lodShape, i == 0 ? &_usedTextures : NULL, i>0 ? (1<<(i-1)) : 0))
      {
        _pb[i].Free();
        if (i <= 1)
        {
          Fail("Not a single instance allowed?");
        }
      }
      else
      {
        _memoryControlled += _pb[i]->GetMemoryControlled();
      }
    }
    //LogF("%d, %d", i>0?(1<<(i-1)):0, GlobalTickCount() - start);
  }
}

int DCPushBuffer::GetAvailableInstances(int instances)
{
  DoAssert(instances <= PBInstancesMax);
  Assert(instances>0);
  int log2Instances = log2Floor(instances);
  // we need to check if the number of instances is really supported

  while (log2Instances >= 0)
  {
    if (_pb[log2Instances+1].NotNull())
    {
      return 1<<log2Instances;
    }
    log2Instances--;
  }
  Fail("There must exists one instance in any case");
  return 1;
}

void DCPushBuffer::Prepare(const Shape &shape, int spec)
{
  {
    //PROFILE_SCOPE_GRF(seDef,SCOPE_COLOR_RED);
    GEngine->LazySetHWToDefinedState();
  }
  {
    //PROFILE_SCOPE_GRF(ldMip,SCOPE_COLOR_BLUE);
    for (int s=0; s<shape.NSections(); s++)
    {
      const ShapeSection &ss = shape.GetSection(s);
      int secSpecial = ss.Special();
      if (secSpecial&(IsHidden|IsHiddenProxy)) continue;
      ss.LoadTextureMipmaps(spec|secSpecial);
    }
  }
}

void DCPushBuffer::PrepareItem(const Shape &shape, int spec, int instancesCount)
{
  //ADD_COUNTER(dcPushBufPrep,1);

  // Prepare the pushbuffer
  int log2count = instancesCount> 0 ? log2Floor(instancesCount)+1 : 0;
  _pb[log2count]->Prepare(shape, spec);
}

void DCPushBuffer::Finish(const Shape &shape)
{
  //ADD_COUNTER(dcPushBufFinish,1);

  // Set fence to all used textures (note that faster way to get a fence is to
  // read the Lock member of _pb - kickPushBuffer is not performed then)
  for (int i = 0; i < _usedTextures.Size(); i++)
  {
    _usedTextures[i]->BindWithPushBuffer();
  }

  // Set fence to the used vertex buffer
  shape.GetVertexBuffer()->BindWithPushBuffer();
}

void DCPushBuffer::Do(const Shape &shape, int instancesCount)
{
  //ADD_COUNTER(dcPushBufDo,1);

  int log2count = instancesCount> 0 ? log2Floor(instancesCount)+1 : 0;
  // Run the pushbuffer
  _pb[log2count]->Run(shape, instancesCount == 0 ? false : true);
}

size_t DCPushBuffer::GetMemoryControlled() const
{
  return _memoryControlled;
}

bool DCPushBuffer::SerializeBin(SerializeBinStream &f)
{
  // Transfer _pb array
  _memoryControlled = 0;
  if (f.IsLoading())
  {
    for (int i = 0; i < PBInstancesLogMax + 2; i++)
    {
      _pb[i].Free();
    }
    int levels = f.LoadInt();
    DoAssert(levels > 0);
    for (int i = 0; i < levels; i++)
    {
      int index = f.LoadInt();
      _pb[index] = GEngine->CreatePushBuffer();
      DoAssert(_pb[index].NotNull());
      if (!_pb[index]->SerializeBin(f)) return false;
      _memoryControlled += _pb[index]->GetMemoryControlled();
    }
  }
  else
  {
    // Calculate number of pb's and save it
    int levels = 0;
    for (int i = 0; i < PBInstancesLogMax + 2; i++)
    {
      if (_pb[i].NotNull()) levels++;
    }
    DoAssert(levels > 0);
    f.SaveInt(levels);

    // Save each entry
    for (int i = 0; i < PBInstancesLogMax + 2; i++)
    {
      if (_pb[i].NotNull())
      {
        f.SaveInt(i);
        if (!_pb[i]->SerializeBin(f)) return false;
      }
    }
  }

  // TODO: remove from the file
  int dummy = _memoryControlled;
  f.Transfer(dummy);

  // Transfer used textures
  if (f.IsLoading())
  {
    // we ignore what is saved there
    int size = f.LoadInt();
    _usedTextures.Realloc(size);
    _usedTextures.Resize(size);
    for (int i = 0; i < size; i++)
    {
      // Load texture type
      TextureType tt;
      f.TransferBinary(tt);

      // Load texture name
      char name[1024];
      int length = f.LoadInt();
      DoAssert(length < 1024);
      f.Load(name, length);
      name[length] = 0;

      // Get the texture
      _usedTextures[i] = GlobLoadTexture(name);
    }
  }
  else
  {
    int size = _usedTextures.Size();
    f.SaveInt(size);
    for (int i = 0; i < size; i++)
    {
      // Save texture type
      TextureType tt = _usedTextures[i]->Type();
      f.TransferBinary(tt);

      // Save texture name
      int length = strlen(_usedTextures[i]->Name());
      f.SaveInt(length);
      f.Save(_usedTextures[i]->Name(), length);
    }
  }

  return true;
}

void DCPushBuffer::VertexBufferHasChanged(Ref<VertexBuffer> &buffer, VBType type, EVertexDecl vertexDeclaration)
{
  for (int i = 0; i <= PBInstancesLogMax+1; i++)
  {
    if (_pb[i].NotNull())
    {
      _pb[i]->VertexBufferHasChanged(buffer, type, vertexDeclaration, (i == 0) ? false : true);
    }
  }
}

DrawCommand *EngineDDXB::CreatePushBuffer(Shape *shape, const LODShape *lodShape)
{
  return new DCPushBuffer(shape,lodShape);
}


bool EngineDDXB::SerializeBinPBHeader(SerializeBinStream &f)
{
  // Check the file identifier
#ifdef _MSC_VER
  if (!f.Version('PBX '))
#else
  if (!f.Version(StrToInt(" XBP")))
#endif
  {
    f.SetError(f.EBadFileType);
    return false;
  }

  // Check the file version
  const int actVersion = 2;
  int version = actVersion;
  f.Transfer(version);
  if (version != actVersion)
  {
    f.SetError(f.EBadVersion);
    return false;
  }
  return true;
}

void EngineDDXB::CreateProgramLoaders(LODShape *ls)
{
  #if LOAD_PB_FROM_DISK
  if (!UseProgramDrawing) return;
  
  RString pbName = PLPushBuffer::GetFileName(ls->Name());
  QIFStreamB in;
  in.AutoOpen(pbName);
  if (!in.fail())
  {
    SerializeBinStream f(&in);
    if (SerializeBinPBHeader(f))
    {
      // Load header into structure
      int levelCount = f.LoadInt();
      struct
      {
        int index;
        int offset;
        bool permanent;
      } level[MAX_LOD_LEVELS];
      for (int i = 0; i < levelCount; i++)
      {
        level[i].index = f.LoadInt();
        level[i].permanent = f.LoadChar()!=0;
        level[i].offset = f.LoadInt();
      }

      // Create handles from the structure
      for (int i = 0; i < levelCount; i++)
      {
        int size;
        if (i < levelCount - 1)
        {
          size = level[i + 1].offset - level[i].offset;
        }
        else
        {
          size = INT_MAX;
        }
        Ref<ProgramLoader> programLoader = new PLPushBuffer(QFBankQueryFunctions::GetHandle(pbName, level[i].offset, size));
        const RefShapeRef &levelRef = ls->LevelRef(level[i].index);
        DoAssert(levelRef.NotNull());
        levelRef.SetProgramLoader(programLoader);
      }
    }
    else
    {
      SerializeBinStream::ErrorCode ec = f.GetError();
      switch (ec)
      {
      case SerializeBinStream::EBadFileType:
        RptF("Error: Bad type of file %s", pbName.Data());
        break;
      case SerializeBinStream::EBadVersion:
        RptF("Error: Bad version of file %s", pbName.Data());
        break;
      default:
        RptF("Error: Unknown error while reading header of file %s", pbName.Data());
      }
    }
  }
  #endif
}

bool EngineDDXB::PreloadProgramLoaderData(FileRequestPriority prior, const char *name)
{
  #if LOAD_PB_FROM_DISK
  return PLPushBuffer::RequestHeader(prior, name);
  #else
  return true;
  #endif
}

bool EngineDDXB::PreloadProgramLoaderPermanentLevels(FileRequestPriority prior, const char *name)
{
  #if LOAD_PB_FROM_DISK
  RString pbName = PLPushBuffer::GetFileName(name);
  QIFStreamB in;
  in.AutoOpen(pbName);
  if (!in.fail())
  {
    SerializeBinStream f(&in);
    if (SerializeBinPBHeader(f))
    {
      // Load header into structure
      int levelCount = f.LoadInt();
      struct
      {
        int index;
        int offset;
        bool permanent;
      } level[MAX_LOD_LEVELS];
      for (int i = 0; i < levelCount; i++)
      {
        level[i].index = f.LoadInt();
        level[i].permanent = f.LoadChar()!=0;
        level[i].offset = f.LoadInt();
      }

      bool ret = true;      
      // Create handles from the structure
      for (int i = 0; i < levelCount; i++)
      {
        int size;
        if (i < levelCount - 1)
        {
          size = level[i + 1].offset - level[i].offset;
        }
        else
        {
          size = INT_MAX;
        }

        if (level[i].permanent)
        {
          if (!in.PreRead(prior,level[i].offset, size)) ret = false;
        }
      }
      return ret;
    }
    else
    {
      SerializeBinStream::ErrorCode ec = f.GetError();
      switch (ec)
      {
      case SerializeBinStream::EBadFileType:
        RptF("Error: Bad type of file %s", pbName.Data());
        break;
      case SerializeBinStream::EBadVersion:
        RptF("Error: Bad version of file %s", pbName.Data());
        break;
      default:
        RptF("Error: Unknown error while reading header of file %s", pbName.Data());
      }
    }
  }
  #endif  
  return true;
}

void EngineDDXB::BeginRecording()
{
  _recording = true;
}

void EngineDDXB::EndRecording()
{
  _recording = false;
}

void VertexBufferD3DXB::AllocateIndices(EngineDDXB *engine, int indices)
{
  _iBufferSize = 0;
  Assert (!_iBuffer);
  _iBuffer.Free();
  IDirect3DDevice8 *device = engine->GetDirect3DDevice();
#ifdef _XBOX
  // device member functions are static, and this variable is therefore not used
  (void)device;
#endif

  if (indices>0)
  {

    int byteSize = indices*sizeof(VertexIndex);
RetryI:
    HRESULT err = device->CreateIndexBuffer
      (
      byteSize,WRITEONLYIB,D3DFMT_INDEX16,D3DPOOL_DEFAULT,
      _iBuffer.Init()
      );

    if (err!=D3D_OK)
    {
      if (err==D3DERR_OUTOFVIDEOMEMORY)
      {
        LogF("Out of VRAM during i-buffer creation");
        if (GEngineDD->TextBankDD()->ForcedReserveMemory(byteSize))
        {
          goto RetryI;
        }
      }
      if (err==E_OUTOFMEMORY)
      {
        size_t freed = FreeOnDemandSystemMemory(byteSize);
        if (freed!=0) goto RetryI;
      }
      DDErrorXB("CreateIndexBuffer",err);
      ErrorMessage("CreateIndexBuffer failed (%x)",err);
      return;
    }
    //LogF("Created IB %x",_iBuffer);
    engine->AddIBuffersAllocated(byteSize);

    DoAssert(_iBuffer);
  }
  _iBufferSize = indices;
}

void VertexBufferD3DXB::AllocateVertices(EngineDDXB *engine, int vertices)
{
  IDirect3DDevice8 *device = engine->GetDirect3DDevice();

#ifdef _XBOX
  // device member functions are static, and this variable is therefore not used
  (void)device;
#endif

  int byteSize = vertices*sizeof(SVertexXB);
RetryV:
  // only static data are stored in static buffer
  HRESULT err=device->CreateVertexBuffer
    (
    byteSize,WRITEONLYVB,0,D3DPOOL_DEFAULT,
    _vBuffer.Init()
    );

  if (err!=D3D_OK)
  {
    if (err==D3DERR_OUTOFVIDEOMEMORY)
    {
      LogF("Out of VRAM during v-buffer creation");
      if (GEngineDD->TextBankDD()->ForcedReserveMemory(byteSize))
      {
        goto RetryV;
      }
    }
    if (err==E_OUTOFMEMORY)
    {
#if _XBOX
      if (GEngineDD->TextBankDD()->ForcedReserveMemory(byteSize))
      {
        goto RetryV;
      }
#endif
      size_t freed = FreeOnDemandSystemMemory(byteSize);
      if (freed!=0) goto RetryV;
    }
    DDErrorXB("CreateVertexBuffer",err);
    return;
    // 
  }

  engine->AddVBuffersAllocated(byteSize);

#if 0
  LogF("Created VB %x (%x)",_vBuffer,this);
  char name[256];
  sprintf(name,"VB %x (%x)",_vBuffer,this);
  vbStats.Count(name,+1);
#endif
  _vBufferSize = vertices;

}

bool VertexBufferD3DXB::Init(EngineDDXB *engine, const Shape &src, VBType type, bool frequent)
{
  // note: when device is reset, all buffers need to be released and created again
  if (src.NVertex()<=0)
  {
    LogF("Empty vertices.");
    return false;
  }
  // geometry needs to be locked, as we need to get indices
  ShapeGeometryLock<> lock(&src);
  // currently only dynamic buffers are used 
  //dynamic = true;
  int nInstances = 1;
  // create and fill index buffer
  // note: index buffer is almost never changed
  switch (type)
  {
    /*
    case VBSmallDiscardable:
    _dynamic = true;
    _separate = true;
    break;
    */
  default:
    Fail("Unknown v-buffer type");
  case VBStatic:
#if 1
    {
      const int maxVertices = frequent ? 8000 : 2000;
      if (src.NVertex()<=maxVertices/2)
      {
        nInstances = maxVertices/src.NVertex();
      }
      if (UseProgramDrawing)
      {
        // it makes no sense to create more than pow2 instances
        nInstances = 1<<log2Floor(nInstances);
        // it makes no sense to create more than max. supported instances
        if (nInstances>PBInstancesMax) nInstances =PBInstancesMax;

      }
    }
#endif
  case VBStaticSkinned:
    _dynamic = false;
    _separate = false;
    break;
    //_dynamic = true;
    //_separate = false;
    //break;
  case VBBigDiscardable:
  case VBSmallDiscardable:
    // let autodetection check if the buffer is big or small
    _dynamic = false;
    _separate = false;
    break;
  case VBNone: case VBNotOptimized:
    Fail("Cannot generate vertex buffer of given type");
  case VBDynamic:
    _dynamic = true;
    _separate = false;
    break;
  case VBDynamicDiscardable:
    _dynamic = true;
    _separate = true;
    break;
  }
  _iOffset = 0;
  _vOffset = 0;

  if (_dynamic && (src.NVertex() > MaxShapeVertices))
  {
    LogF("Error: Number of vertices too large for the dynamic vertex buffer.");
    return false;
  }

  if (!src.CanBeInstanced())
  {
    nInstances = 1;
  }

  /*
  // Decide wheter to use instances
  if ((nInstances > 1) && (src.GetVertexDeclaration() != VD_Tex0) && (src.GetVertexDeclaration() != VD_Position_Tex0_Color))
  {
  int indices;
  #ifdef STRIPIZATION_ENABLED
  if ((src.GetFaceArrayType() == FATStripized) && EnableStrips)
  {
  Fail("Not working with stripes");
  }
  else
  #endif
  {
  indices = IndexStaticDataXB::MaxIndicesInOneSection(src);
  }
  // If there are too many indices then disable instances
  if (indices*2 > 1440) nInstances = 1;
  }
  */

  // Create the vertex buffer
  int vOffset = 0;
  if (!_separate)
  {
    if (type == VBStaticSkinned)
    {
      nInstances = 1;
      switch (src.GetVertexDeclaration())
      {
      case VD_Position_Normal_Tex0:
        _sharedV = GEngineDD->AddShapeVerticesSkinned(src, *this, _dynamic, false);
        break;
      case VD_Position_Normal_Tex0_ST:
      case VD_Position_Normal_Tex0_ST_Alpha:
        _sharedV = GEngineDD->AddShapeVerticesNormalSkinned(src, *this, _dynamic, false);
        break;
      case VD_ShadowVolume:
        _sharedV = GEngineDD->AddShapeVerticesShadowVolumeSkinned(src, *this, _dynamic, false);
        break;
      default:
        Fail("Unknown vertex declaration in the source shape");
      }
    }
    else
    {
      switch (src.GetVertexDeclaration())
      {
      case VD_Tex0:
        if (nInstances>EngineDDXB::MaxInstancesSprite) nInstances=EngineDDXB::MaxInstancesSprite;
        if (nInstances<=1)
        {
          Fail("Error: Non instanced sprite?");
        }
        else
        {
          _sharedV = GEngineDD->AddShapeVerticesSpriteInstanced(src, *this, _dynamic, type == VBBigDiscardable, nInstances);
        }
        break;
      case VD_Position_Tex0_Color:
        nInstances = 1;
        _sharedV = GEngineDD->AddShapeVerticesPoint(src, *this, _dynamic, type == VBBigDiscardable);
        break;
      case VD_Position_Normal_Tex0:
        if (nInstances>EngineDDXB::MaxInstances) nInstances=EngineDDXB::MaxInstances;
        if (nInstances<=1)
        {
          _sharedV = GEngineDD->AddShapeVertices(src, *this, _dynamic, type == VBBigDiscardable);
        }
        else
        {
          _sharedV = GEngineDD->AddShapeVerticesInstanced(src, *this, _dynamic, type == VBBigDiscardable, nInstances);
        }
        break;
      case VD_Position_Normal_Tex0_ST:
        if (nInstances>EngineDDXB::MaxInstances) nInstances=EngineDDXB::MaxInstances;
        if (nInstances <=1)
        {
          _sharedV = GEngineDD->AddShapeVerticesNormal(src, *this, _dynamic, type == VBBigDiscardable);
        }
        else
        {
          _sharedV = GEngineDD->AddShapeVerticesNormalInstanced(src, *this, _dynamic, type == VBBigDiscardable, nInstances);
        }
        break;
      case VD_Position_Normal_Tex0_ST_Alpha:
        nInstances = 1;
        _sharedV = GEngineDD->AddShapeVerticesNormalAlpha(src, *this, _dynamic, type == VBBigDiscardable);
        break;
      case VD_ShadowVolume:
        if (nInstances>EngineDDXB::MaxInstancesShadowVolume) nInstances=EngineDDXB::MaxInstancesShadowVolume;
        if (nInstances <= 1)
        {
          _sharedV = GEngineDD->AddShapeVerticesShadowVolume(src, *this, _dynamic, type == VBBigDiscardable);
        }
        else
        {
          _sharedV = GEngineDD->AddShapeVerticesShadowVolumeInstanced(src, *this, _dynamic, type == VBBigDiscardable, nInstances);
        }
        break;
      default:
        Fail("Unknown vertex declaration in the source shape");
      }
    }

    // Create the index buffer
    if (!_dynamic && !_sharedV)
    {
      // mark fall-back
      Fail("Vertex buffer not allocated");
      _separate = true;
    }
    else
    {
      // Finish in case of zero sections
      if (src.NSections() == 0)
      {
        return true;
      }

      vOffset = _vAlloc ? _vAlloc->Memory() : 0;
      if (src.GetVertexDeclaration() == VD_Tex0)
      {
        _sharedI = GEngineDD->AddShapeIndicesSprite(src,_iAlloc,_dynamic, nInstances);
      }
      else if (src.GetVertexDeclaration() == VD_Position_Tex0_Color)
      {
        Assert(nInstances == 1);
        _sharedI = GEngineDD->AddShapeIndicesPoint(src,_iAlloc,_dynamic, nInstances);
      }
      else
      {
        _sharedI = GEngineDD->AddShapeIndices(src, vOffset, _iAlloc, _dynamic, nInstances);
      }
      if (!_sharedI || !_iAlloc)
      {
        LogF("Shared index buffer not allocated");
        _separate = true;
        if (_sharedV)
        {
          _sharedV->RemoveShape(_vAlloc);
          _sharedV = NULL;
          _vAlloc = NULL;
        }
      }
      else
      {
        _iOffset = _iAlloc->Memory();
        _vOffset = vOffset;
      }
    }
  }
  if ( (_sharedV || _dynamic) && _sharedI)
  {

    // scan sections
    _sections.Realloc(src.NSections() * nInstances);
    _sections.Resize(src.NSections() * nInstances);

    int start = 0;

    // Go through all the sections
    for (int i = 0; i < src.NSections(); i++)
    {
      const ShapeSection &sec = src.GetSection(i);

      // Get the number of triangles
#ifdef STRIPIZATION_ENABLED
      int trianglesCount = 0;
      if ((src.GetFaceArrayType() == FATStripized) && EnableStrips)
      {
        Assert((OffsetDiff(sec.end, sec.beg) % PolyVerticesSize(3)) == 0);
        trianglesCount = OffsetDiff(sec.end, sec.beg) / PolyVerticesSize(3);
      }
#endif

      // Go through all the instances
      for (int instance = 0; instance < nInstances; instance++)
      {
        int minV = INT_MAX;
        int maxV = 0;
        int size = 0;

#ifdef STRIPIZATION_ENABLED
        if ((src.GetFaceArrayType() == FATStripized) && EnableStrips)
        {
          // Process first face
          Offset o = sec.beg;
          if (o < sec.end)
          {
            const Poly &face = src.Face(o);
            Assert(face.N() == 3);
            int vi;
            vi = face.GetVertex(0);
            saturateMin(minV,vi);
            saturateMax(maxV,vi);
            vi = face.GetVertex(1);
            saturateMin(minV,vi);
            saturateMax(maxV,vi);
            vi = face.GetVertex(2);
            saturateMin(minV,vi);
            saturateMax(maxV,vi);
          }

          // Process rest of the faces
          src.NextFace(o);
          while (true)
          {
            int vi;

            if (o >= sec.end) break;
            const Poly &faceA = src.Face(o);
            Assert(faceA.N() == 3);
            vi = faceA.GetVertex(0);
            saturateMin(minV,vi);
            saturateMax(maxV,vi);
            src.NextFace(o);

            if (o >= sec.end) break;
            const Poly &faceB = src.Face(o);
            Assert(faceB.N() == 3);
            vi = faceB.GetVertex(2);
            saturateMin(minV,vi);
            saturateMax(maxV,vi);
            src.NextFace(o);
          }

          // Add the size of the instance

          // Add standard number of indices
          size += 2 + trianglesCount;

          // All instances except the first one are concatenated with the previous using 2 indices
          if (instance > 0)
          {
            // In case there is odd number of triangles, then there is one triangle added
            if (trianglesCount & 1)
            {
              size ++;
            }
            size += 2;
          }
        }
        else
#endif
        {
          for (Offset o=sec.beg; o<sec.end; src.NextFace(o))
          {
            const Poly &face = src.Face(o);
            if (face.N() < 3) continue;
            ///*
            if  ((face.N() == 3) &&
              ((face.GetVertex(0) == face.GetVertex(1)) || 
              (face.GetVertex(1) == face.GetVertex(2)) ||
              (face.GetVertex(2) == face.GetVertex(0)))) continue;
            //*/

            // check vertex indices used
            for (int vv=0; vv<face.N(); vv++)
            {
              int vi = face.GetVertex(vv);
              saturateMin(minV,vi);
              saturateMax(maxV,vi);
            }
            size += (face.N()-2)*3;
          }
        }

        // record section info
        int dstSection = src.NSections()*instance;
        _sections[dstSection + i].beg = start;
        _sections[dstSection + i].end = start + size;
#if V_OFFSET_CUSTOM
        //Assert(minV != INT_MAX);
        if (minV <= maxV)
        {
          _sections[dstSection + i].begVertex = minV;
          _sections[dstSection + i].endVertex = maxV + 1;
        }
        else
        {
          _sections[dstSection + i].begVertex = 0;
          _sections[dstSection + i].endVertex = 0;
        }
#else
        if (minV <= maxV)
        {
          _sections[dstSection + i].begVertex = minV + vOffset;
          _sections[dstSection + i].endVertex = maxV + vOffset + 1;
        }
        else
        {
          _sections[dstSection + i].begVertex = vOffset;
          _sections[dstSection + i].endVertex = vOffset;
        }
#endif
        Assert (_sections[dstSection + i].endVertex >= _sections[dstSection + i].begVertex);
        // Move start to the next item
        start += size;
      }
    }
  }
  else // !_shared
  {
    // nor vertices not indices shared - sharing not desired or there is no space for sharing
    if (!_dynamic)
    {
      AllocateVertices(engine,src.NVertex());
      if (!_vBuffer)
      {
        // vertex buffer allocation failed - 
        return false;
      }
      CopyData(src);
    }

    DoAssert(nInstances==1); // instancing not supported here
    // calculate how many triangle we will need to render this buffer
    int indices = IndexStaticDataXB::IndicesNeededTotal(src, 1);

    if (indices>0)
    {
      AllocateIndices(engine,indices);

Retry:
      BYTE *data = NULL;
      HRESULT err;
      {
        PROFILE_DX_SCOPE(3ibLS);
        err = _iBuffer->Lock(0,indices*sizeof(VertexIndex),&data,NoSysLockFlag);
      }
      if (err!=D3D_OK || !data)
      {
        if (QFBank::FreeUnusedBanks(1024*1024)) goto Retry;
#ifndef _XBOX
        ErrF("IB Lock failed, %s",DXGetErrorString8(err));
#else
        ErrF("IB Lock failed, %x",err);
#endif
        return false;
      }

      VertexIndex *iData = (VertexIndex *)data;
      for (Offset o=src.BeginFaces(); o<src.EndFaces(); src.NextFace(o))
      {
        const Poly &poly = src.Face(o);
        if (poly.N() < 3) continue;
        if ((poly.N() == 3) &&
          ((poly.GetVertex(0) == poly.GetVertex(1)) || 
          (poly.GetVertex(1) == poly.GetVertex(2)) ||
          (poly.GetVertex(2) == poly.GetVertex(0)))) continue;
        for (int i=2; i<poly.N(); i++)
        {
          *iData++ = poly.GetVertex(0);
          *iData++ = poly.GetVertex(i-1);
          *iData++ = poly.GetVertex(i);
        }
      }
      DoAssert(iData == &((VertexIndex *)data)[indices]);
      _iBuffer->Unlock();

      // scan sections
      _sections.Realloc(src.NSections());
      _sections.Resize(src.NSections());
      int start = 0;
      for (int i=0; i<src.NSections(); i++)
      {
        const ShapeSection &sec = src.GetSection(i);
        // calculate how much indices is used for this section
        int size = 0;
        int minV = INT_MAX;
        int maxV = 0;
        for (Offset o=sec.beg; o<sec.end; src.NextFace(o))
        {
          const Poly &face = src.Face(o);
          if (face.N() < 3) continue;
          if ((face.N() == 3) &&
            ((face.GetVertex(0) == face.GetVertex(1)) || 
            (face.GetVertex(1) == face.GetVertex(2)) ||
            (face.GetVertex(2) == face.GetVertex(0)))) continue;
          size += (face.N()-2)*3;
          for (int vv=0; vv<face.N(); vv++)
          {
            int vi = face.GetVertex(vv);
            saturateMin(minV,vi);
            saturateMax(maxV,vi);
          }
        }
        // record section info
        _sections[i].beg = start;
        _sections[i].end = start+size;
        _sections[i].begVertex = minV;
        _sections[i].endVertex = maxV+1;
        start += size;
      }
    }
    _iOffset = 0;
    _vOffset = 0;
    // vertex offset is actually determined dynamically
    // after vertices are placed to dynamic vertex buffer
  }

  // index buffer remains constant - animation is done on vertices
  // or whole sections
  return true;

}

void VertexBufferD3DXB::CopyData( const Shape &src )
{
  Assert (_separate);
  Assert (!_dynamic);
  if (_vBufferSize<=0) return;
  // copy source data from the mesh
  //DWORD size;

  DWORD flags = NoSysLockFlag;

Retry:
  BYTE *data = NULL;
  HRESULT err;
  {
    PROFILE_DX_SCOPE(3vbLS);
    err = _vBuffer->Lock(0,_vBufferSize*sizeof(SVertexXB),&data,flags);
  }

  if (err!=D3D_OK || !data)
  {
    if (QFBank::FreeUnusedBanks(1024*1024)) goto Retry;
#ifndef _XBOX
    ErrF("VB Lock failed, %s",DXGetErrorString8(err));
#else
    ErrF("VB Lock failed, %x",err);
#endif
    return;
  }

  // Calculate the vertex buffer maximum and minimum Pos's
  src.CalculateMaxAndMinPos(_maxPos, _minPos);

  // Calculate the vertex buffer maximum and minimum UV's
  src.CalculateMaxAndMinUV(_maxUV, _minUV);

  SVertexXB *sData = (SVertexXB *)data;
  const CondensedAutoArray<UVPair> *uv = &src.GetUVArray();
  const AutoArray<Vector3> *pos = &src.GetPosArray();
  const CondensedAutoArray<Vector3> *norm = &src.GetNormArray();
  Vector3 invScale = CalcInvScale(_maxPos, _minPos);
  UVPair invUV = CalcInvScale(_maxUV, _minUV);

  int nVertex = src.NVertex();
  for (int i = 0; i < nVertex; i++)
  {
    CopyPos(sData->pos, (*pos)[i], _maxPos, _minPos, invScale);
    CopyNormal(sData->norm,(*norm)[i]);
    CopyUV(sData->t0, (*uv)[i], _maxUV, _minUV, invUV);
    sData++;
  }

  _vBuffer->Unlock();

}

bool VertexBufferD3DXB::IsGeometryLoaded() const
{
  // dynamic buffers do not contain any geometry information
  return !_dynamic;
}


void VertexBufferD3DXB::Update(const Shape &src, const EngineShapeProperties &prop, VSDVersion vsdVersion)
{
  if (!_dynamic)
  {
    // use static buffer as source
    GEngineDD->SetVSourceStatic(src, prop, *this, vsdVersion);
  }
  else
  {
    // update dynamic buffer - and use it as source
    GEngineDD->SetVSourceDynamic(src, prop, *this);
  }

  // note: no need to set indices / vertices when drawing same shape again
  // this can be quite common

}

int EngineDDXB::ShapePropertiesNeeded(const TexMaterial *mat) const
{
  switch (mat->GetVertexShaderID(0))
  {
    case VSNormalMapThrough:
      return ESPTreeCrown;
  }
  return 0;
}

void EngineDDXB::SetVSourceStatic(
  const Shape &src, const EngineShapeProperties &prop,
  const VertexBufferD3DXB &buf, VSDVersion vsdVersion
)
{
  PROFILE_DX_SCOPE(3vbS);

  bool pbPlaying = src.ProgramExists();

  if (_recording || !pbPlaying)
  {
    // Set Pos dynamic range constants
    Vector3 scalePos = buf._maxPos - buf._minPos;
    saturateMax(scalePos[0],1e-6f);
    saturateMax(scalePos[1],1e-6f);
    saturateMax(scalePos[2],1e-6f);
    SetPosScaleAndOffset(scalePos, buf._minPos);

    // Set UV dynamic range constants
    UVPair scale;
    scale.u = buf._maxUV.u - buf._minUV.u;
    scale.v = buf._maxUV.v - buf._minUV.v;
    saturateMax(scale.u,1e-6f);
    saturateMax(scale.v,1e-6f);
    SetTexGenScaleAndOffset(scale, buf._minUV);
    
    // if shape requires any special properties, set them
    if (src.GetShapePropertiesNeeded()&ESPTreeCrown)
    {
      Vector3 minmax[2];
      prop.GetTreeCrown(minmax);
      SetTreeCrown(minmax[0], minmax[1]);
    }
    
  }

  // Separate part
  if (!buf._sharedV)
  {
    DoAssert(!_activePushBuffer);
    if (_vBufferLast!=buf._vBuffer || _vOffsetLast!=0 || _vsdVersionLast!=VSDV_Basic)
    {
#if EXPLICIT_FENCE_LEVEL>30
      _d3DDevice->InsertFence();
#endif
      ADD_COUNTER(d3xVB,1);
      _hwInDefinedState = false;
      D3DSTREAM_INPUT si;
      si.VertexBuffer = buf._vBuffer;
      si.Stride = sizeof(SVertexXB);
      si.Offset = 0;
      _d3DDevice->SetVertexShaderInputDirect(VAFVSDecl[VD_Position_Normal_Tex0][VSDV_Basic], 1, &si);
      _vBufferLast = buf._vBuffer;
      _vBufferSize = buf._vBufferSize;
      _vOffsetLast = 0;
      _vsdVersionLast = VSDV_Basic;

#if LOG_STATE_LEVEL
      if (LogStatesOnce)
      {
        LogF("SetVBSourceStatic separate");
      }
#endif

    }
    if (_iBufferLast!=buf._iBuffer)
    {
      ADD_COUNTER(d3xIB,1);
      _d3DDevice->SetIndices(buf._iBuffer, 0);
      _iBufferLast = buf._iBuffer;

#if LOG_STATE_LEVEL
      if (LogStatesOnce)
      {
        LogF("SetIBSourceStatic separate");
      }
#endif
    }
    _iOffset = buf._iOffset;
  }
  // Shared part
  else
  {
#if V_OFFSET_CUSTOM
    int vOffset = buf._vOffset;
#else
    int vOffset = 0;
#endif
    VertexStaticDataXB *dtaV = buf._sharedV;
    IndexStaticDataXB *dtaI = buf._sharedI;
    if (dtaV && (_vBufferLast!=dtaV->_vBuffer || _vOffsetLast!=vOffset || _vsdVersionLast!=vsdVersion))
    {
#if EXPLICIT_FENCE_LEVEL>30
      _d3DDevice->InsertFence();
#endif
      ADD_COUNTER(d3xVB,1);

      // Set the VB declaration
      if (_recording)
      {
        // Remember values for fix-ups
        D3DSTREAM_INPUT si;
        si.VertexBuffer = dtaV->_vBuffer;
        si.Stride = dtaV->VertexSize();
        si.Offset = vOffset * dtaV->VertexSize();
        _d3DDevice->SetVertexShaderInputDirect(VAFVSDecl[src.GetVertexDeclaration()][vsdVersion], 1, &si);
      }
      else
      {
        if (!pbPlaying)
        {
          _hwInDefinedState = false;
          D3DSTREAM_INPUT si;
          si.VertexBuffer = dtaV->_vBuffer;
          si.Stride = dtaV->VertexSize();
          si.Offset = vOffset * dtaV->VertexSize();
          _d3DDevice->SetVertexShaderInputDirect(VAFVSDecl[src.GetVertexDeclaration()][vsdVersion], 1, &si);
        }
      }

      _vBufferLast = dtaV->_vBuffer;
      _vBufferSize = dtaV->Size();
      _vOffsetLast = vOffset;
      _vsdVersionLast = vsdVersion;

#if LOG_STATE_LEVEL
      if (LogStatesOnce)
      {
        LogF("SetVBSourceStatic shared");
      }
#endif
    }

    if (dtaI && _iBufferLast!=dtaI->_iBuffer)
    {
      ADD_COUNTER(d3xIB,1);
      _d3DDevice->SetIndices(dtaI->_iBuffer, 0);
      _iBufferLast = dtaI->_iBuffer;
#if LOG_STATE_LEVEL
      if (LogStatesOnce)
      {
        LogF("SetIBSourceStatic shared");
      }
#endif
    }
    _iOffset = buf._iOffset;
  }
}

void EngineDDXB::SetVSourceDynamic(
 const Shape &src, const EngineShapeProperties &prop, const VertexBufferD3DXB &buf
)
{
  PROFILE_DX_SCOPE(3vbD);
  int vOffset = _dynVBuffer.AddVertices(src);

  const ComRef<IDirect3DVertexBuffer8> &dbuf = _dynVBuffer.GetVBuffer();
  if (_vBufferLast!=dbuf || vOffset!=_vOffsetLast || _vsdVersionLast!=VSDV_Basic)
  {
#if EXPLICIT_FENCE_LEVEL>30
    _d3DDevice->InsertFence();
#endif
#if LOG_STATE_LEVEL
    if (LogStatesOnce)
    {
      LogF("SetVBSourceDynamic");
    }
#endif
    ADD_COUNTER(d3xVB,1);
    _hwInDefinedState = false;
    D3DSTREAM_INPUT si;
    si.VertexBuffer = dbuf;
    si.Stride = sizeof(SVertexXB);
    si.Offset = vOffset * sizeof(SVertexXB);
    _d3DDevice->SetVertexShaderInputDirect(VAFVSDecl[VD_Position_Normal_Tex0][VSDV_Basic], 1, &si);
    _vBufferLast = dbuf;
    _vBufferSize = _dynVBuffer._vBufferSize;
    _vOffsetLast = vOffset;
    _vsdVersionLast = VSDV_Basic;
  }

  if (buf._separate)
  {

    if (_iBufferLast!=buf._iBuffer)
    {

#if LOG_STATE_LEVEL
      if (LogStatesOnce)
      {
        LogF("SetIBSourceDynamic separate");
      }
#endif

      ADD_COUNTER(d3xIB,1);
      _d3DDevice->SetIndices(buf._iBuffer, 0);
      _iBufferLast = buf._iBuffer;
    }
    _iOffset = buf._iOffset;
  }
  else
  {
    IndexStaticDataXB *dtaI = buf._sharedI;
    if (dtaI && _iBufferLast!=dtaI->_iBuffer)
    {
#if LOG_STATE_LEVEL
      if (LogStatesOnce)
      {
        LogF("SetIBSourceDynamic shared");
      }
#endif

      ADD_COUNTER(d3xIB,1);
      _d3DDevice->SetIndices(dtaI->_iBuffer, 0);
      _iBufferLast = dtaI->_iBuffer;
    }
    _iOffset = buf._iOffset;
  }
}

#define SET_COLOR(d,s) d.r = s.R(),d.g = s.G(),d.b = s.B(),d.a = s.A()
#define SET_WHITE(d,av) d.r = d.g = d.b = 1, d.a = av
#define SET_BLACK(d,av) d.r = d.g = d.b = 0, d.a = av
#define SET_POS(d,s) d.x = s.X(),d.y = s.Y(),d.z = s.Z()

/*!
\patch 1.20 Date 8/16/2001 by Ondra
- Fixed: Night vision with HW T&L has no longer colored areas near light.
\patch_internal 1.24 Date 9/27/2001 by Ondra
- Fixed: HW T&L spotlight parameters adjusted.
\patch 1.24 Date 9/27/2001 by Ondra
- Fixed: Brightness factor was applied twice in HW T&L daytime lighting.
*/
static void ConvertLight
(
 D3DLIGHT8 &desc, const Light *light, float night,
 const TLMaterial &mat
 )
{
  LightDescription ldesc;
  light->GetDescription(ldesc, GEngine->GetAccomodateEye());
  // convert description to D3D format
  switch (ldesc.type)
  {
  default: desc.Type = D3DLIGHT_DIRECTIONAL; break;
  case LTPoint: desc.Type = D3DLIGHT_POINT; break;
  case LTSpotLight: desc.Type = D3DLIGHT_SPOT; break;
  }

  SET_POS(desc.Position,ldesc.pos);
  SET_POS(desc.Direction,ldesc.dir);
  //SET_COLOR(desc.Ambient,HBlack);
  SET_COLOR(desc.Diffuse,(ldesc.diffuse*night*mat.diffuse));
  //SET_COLOR(desc.Specular,HBlack);
  SET_COLOR(desc.Specular,(ldesc.diffuse*night*0.025f));
  desc.Falloff = 1;
  desc.Theta = ldesc.theta;
  desc.Phi = ldesc.phi;
  desc.Range = ldesc.startAtten*10;
  desc.Attenuation0 = 0;
  desc.Attenuation1 = 0;
  if (ldesc.type==LTSpotLight)
  {
    desc.Attenuation2 = 4.0f/Square(ldesc.startAtten);
    SET_COLOR(desc.Ambient,(ldesc.ambient*night*mat.ambient*0.3));
  }
  else
  {
    desc.Attenuation2 = 1.0f/Square(ldesc.startAtten);
    SET_COLOR(desc.Ambient,(ldesc.ambient*night*mat.ambient));
  }
}

bool EngineDDXB::IsWBuffer() const
{
  return _useWBuffer;
}

bool EngineDDXB::CanWBuffer() const
{
  return _canWBuffer && (_depthBpp<=16 || _canWBuffer32);
}

void EngineDDXB::SetWBuffer(bool val)
{
  bool change = false;
  if (_userEnabledWBuffer != val) change  = true;
  _userEnabledWBuffer = val;
  if (CanWBuffer())
  {
    if (_useWBuffer != val) change  = true;
    _useWBuffer = val;
    if (change) ResetHard();
  }
}

/*!
\patch 1.01 Date 6/18/2001 by Ondra
- Fixed: T&L lights in daytime
*/

void EngineDDXB::SetupLights(const TLMaterial &mat, const LightList &lights, int spec)
{
  if (!GScene)
  {
    // no scene - no lights
    return;
  }

  // Setup material color parameters
  _d3DDevice->SetVertexShaderConstant(MAT_E, (D3DXVECTOR4*)&mat.emmisive, 1);

  //  _d3DDevice->SetVertexData4f(V_MAT_A, 0.1, 0.1, 0.1, 0.1);
  //  _d3DDevice->SetVertexData4f(V_MAT_D, 0.1, 0.1, 0.1, 0.1);
  //  _d3DDevice->SetVertexData4f(V_MAT_D_FORCED, 0.1, 0.1, 0.1, 0.1);
  //  _d3DDevice->SetVertexData4f(V_MAT_S, 0.1, 0.1, 0.1, 0.1);

  _d3DDevice->SetVertexData4f(V_MAT_A, mat.ambient.R(), mat.ambient.G(), mat.ambient.B(), mat.ambient.A());
  _d3DDevice->SetVertexData4f(V_MAT_D, mat.diffuse.R(), mat.diffuse.G(), mat.diffuse.B(), mat.diffuse.A());
  _d3DDevice->SetVertexData4f(V_MAT_D_FORCED, mat.forcedDiffuse.R(), mat.forcedDiffuse.G(), mat.forcedDiffuse.B(), mat.forcedDiffuse.A());
  _d3DDevice->SetVertexData4f(V_MAT_S, mat.specular.R(), mat.specular.G(), mat.specular.B(), mat.specularPower);

#if _ENABLE_PERFLOG
  ADD_COUNTER(stLig, 16*6);
#endif


#ifdef _XBOX

  //  _night = GScene->MainLight()->NightEffect();
  //  if ((mat.specFlags|spec)&DisableSun) _night = 1;

  // Remember material values to use them later
  _matAmbient = mat.ambient;
  _matDiffuse = mat.diffuse;
  _matSpecular = mat.specular;
  _matForcedDiffuse = mat.forcedDiffuse;
  //_matValuesHasChanged = true;

#endif

}

void EngineDDXB::SetLights(const LightList &lights, int spec, bool fakeLightRelocation)
{
  //////////////////////////////////////////////////////////////////////////
  // Set the main light constants

  LightSun *sun = GScene->MainLight();

  // ML_Sun
  Vector3 tLightDirection = _invWorld.Orientation() * sun->LightDirection();
  Color groundColor = sun->GroundReflection();
  
  tLightDirection.Normalize();
  D3DXVECTOR4 lightDirection(tLightDirection.X(), tLightDirection.Y(), tLightDirection.Z(), 1.0f);
  _d3DDevice->SetVertexShaderConstant(LDIRECTION_TRANSFORMED_DIR, &lightDirection, 1);

  //Vector3 tShadowDirection = _invWorld.Orientation() * sun->LightDirection();
  // use "shadow direction" to avoid very long shadows
  Vector3 tShadowDirection = _invWorld.Orientation() * sun->ShadowDirection();
  tShadowDirection.Normalize();
  D3DXVECTOR4 shadowDirection(tShadowDirection.X(), tShadowDirection.Y(), tShadowDirection.Z(), 1.0f);
  _d3DDevice->SetVertexShaderConstant(SHADOWDIRECTION_TRANSFORMED_DIR, &shadowDirection, 1);
  
  D3DXVECTOR4 ground(groundColor.R(),groundColor.G(),groundColor.B(),groundColor.A());
  _d3DDevice->SetVertexShaderConstant(LDIRECTION_GROUND, &ground, 1);

  // ML_Sky

  // ML_Stars
  // calculate common alpha correction
  float alphaCorrection=
    (
    // overcast limitation
    // Moved to the VS (1.5 * mat.diffuse.A() - 0.5) *
    // daytime limitation
    sun->StarsVisibility() *
    // resolution correction
    (Width()*Height())*(1.0f/(640*480)) *
    // zoom correction
    GScene->GetCamera()->InvLeft()
    );
  Vector3 tSunDirection = _invWorld.Orientation() * sun->SunDirection();
  tSunDirection.Normalize();
  D3DXVECTOR4 sunDirection(tSunDirection.X(), tSunDirection.Y(), tSunDirection.Z(), alphaCorrection);
  _d3DDevice->SetVertexShaderConstant(LDIRECTIONSTARS_TRANSFORMED_SUN_DIR__ALPHA_CORRECTION, &sunDirection, 1);

  // ML_SunObject

  // ML_SunHaloObject

  // ML_MoonObject

  // ML_MoonHaloObject

#if _ENABLE_PERFLOG
  ADD_COUNTER(setLi, 16*2);
#endif

  //////////////////////////////////////////////////////////////////////////
  // Remember the P&S lights

  // we need to avoid _lightsHasChanged when there is no change, because doing so
  // forces shader relink, which is quite expensive
  int nLights = lights.Size();

  float night = GScene->MainLight()->NightEffect();
  if (spec&DisableSun) night = 1;

  // no lights during daytime
  if (night<=0) nLights = 0;
  // if there are or were some lights, we will relink
  // we could try checking if light count or configration has changed
  // but it is quite unlikely it did not
  if (_lights.Size()>0 || nLights>0)
  {
    saturateMin(nLights, maxPSLights);
    _lights.Resize(nLights);
    for (int i = 0; i < nLights; i++)
    {
      const Light *light = lights[i];
      _lights[i] = unconst_cast(light);
    }
    _lightsHasChanged = true;
  }

  //////////////////////////////////////////////////////////////////////////
  // Set the P&S lights constants

  for (int i = 0; i < _lights.Size(); i++)
  {
    LightDescription ldesc;
    _lights[i]->GetDescription(ldesc, GetAccomodateEye());
    if (ldesc.type == LTPoint)
    {
      DoAssert(i<maxPointLights);
      // Beginning in shader constans
      int lightConstantBase = POINTSPOT_BASE + i * POINTSPOT_COMMON_SIZE;

      // LPoint_TransformedPos
      Vector3 pointLightPosition = TransformAndNormalizePositionToObjectSpace(ldesc.pos);
      D3DXVECTOR4 transformedPos(pointLightPosition.X(), pointLightPosition.Y(), pointLightPosition.Z(), 1.0f);
      _d3DDevice->SetVertexShaderConstant(lightConstantBase + LPOINT_TRANSFORMED_POS, &transformedPos, 1);

      // LPoint_Atten
      D3DXVECTOR4 atten(Square(ldesc.startAtten), 0.0f, 0.0f, 0.0f);
      _d3DDevice->SetVertexShaderConstant(lightConstantBase + LPOINT_ATTEN, &atten, 1);

      // LPoint_D
      Color pointD = ldesc.diffuse * night;
      _d3DDevice->SetVertexShaderConstant(lightConstantBase + LPOINT_D, (D3DXVECTOR4*)&pointD, 1);

      // LPoint_A
      Color pointA = ldesc.ambient * night;
      _d3DDevice->SetVertexShaderConstant(lightConstantBase + LPOINT_A, (D3DXVECTOR4*)&pointA, 1);
    }
    else if (ldesc.type == LTSpotLight)
    {
      DoAssert(i<maxSpotLights);
      // Beginning in shader constans
      int lightConstantBase = POINTSPOT_BASE + i * POINTSPOT_COMMON_SIZE;

      // LSpot_TransformedPos
      Vector3 newPos;
      float newTheta;
      float newAtten;
      if (fakeLightRelocation)
      {
        const float relocDistance = 70.0f;
        const float frontEquivalence = 10.0f;

        // Move the light back and up (to keep the bottom side of the light cone)
        newPos = ldesc.pos + Vector3(0, sin(ldesc.phi * 0.5f) * relocDistance, 0) - ldesc.dir * relocDistance;

        // Calculate new theta (to keep the enlighted area several meters in front of the original spot position)
        newTheta = atan((tan(ldesc.theta) * frontEquivalence) / (relocDistance + frontEquivalence));

        // Affect the attenuation by the new light distance
        newAtten = Square(ldesc.startAtten + relocDistance) / 4.0f;
      }
      else
      {
        newPos = ldesc.pos;
        newTheta = ldesc.theta;
        newAtten = Square(ldesc.startAtten) / 4.0f;
      }
      Vector3 spotLightPos = TransformAndNormalizePositionToObjectSpace(newPos);
      D3DXVECTOR4 transformedPos(spotLightPos.X(), spotLightPos.Y(), spotLightPos.Z(), 1.0f);
      _d3DDevice->SetVertexShaderConstant(lightConstantBase + LSPOT_TRANSFORMED_POS, &transformedPos, 1);

      // LSpot_TransformedDir
      Vector3 spotLightDir = _invWorld.Orientation() * ldesc.dir;
      spotLightDir.Normalize();
      D3DXVECTOR4 transformedDir(spotLightDir.X(), spotLightDir.Y(), spotLightDir.Z(), 1.0f);
      _d3DDevice->SetVertexShaderConstant(lightConstantBase + LSPOT_TRANSFORMED_DIR, &transformedDir, 1);

      // LSpot_CosPhiHalf
      float cosPhiHalf = cos(ldesc.phi * 0.5f);
      // LSpot_CosThetaHalf
      float cosThetaHalf = cos(newTheta * 0.5f);
      // LSpot_InvCTHMCPH
      float invCTHMCPH = 1.0f / (cos(newTheta * 0.5f) - cos(ldesc.phi * 0.5f));
      // LSpot_Atten
      float atten = newAtten;
      D3DXVECTOR4 lSpotConstants(atten, cosPhiHalf, cosThetaHalf, invCTHMCPH);
      _d3DDevice->SetVertexShaderConstant(lightConstantBase + LSPOT__ATTEN__COS_PHI_HALF__COS_THETA_HALF__INV_CTHMCPH, &lSpotConstants, 1);

      // LSpot_D
      Color spotD = ldesc.diffuse * night;
      _d3DDevice->SetVertexShaderConstant(lightConstantBase + LSPOT_D, (D3DXVECTOR4*)&spotD, 1);

      // LSpot_A
      Color spotA = ldesc.ambient * night * 0.3f;
      _d3DDevice->SetVertexShaderConstant(lightConstantBase + LSPOT_A, (D3DXVECTOR4*)&spotA, 1);
    }
  }



}

inline float PlaneDistance2(const Plane &p1, const Plane &p2)
{
  float d = Square(p1.D()-p2.D());
  d += p1.Normal().Distance2(p2.Normal());
  return d;
}

void EngineDDXB::ChangeClipPlanes()
{
}

void EngineDDXB::ChangeWDepth(float matC, float matD)
{
  if (_matC==matC && _matD==matD) return;
  _matC = matC, _matD = matD;
  if (!_useWBuffer) return;


  if (_tlActive)
  {
    Fail("ChangeWDepth in not compatible with HW T&L");
    D3DMATRIX projMatrix;
    // set projection (based on camera)
    int bias = _bias;

    ConvertProjectionMatrix(projMatrix,_sceneProps->GetCameraProjection(),bias);

    // adjust z scale
    projMatrix._33 = matC;
    projMatrix._43 = matD;

    //_d3DDevice->SetTransform( D3DTS_PROJECTION , &projMatrix );
    LogSetProjXB(projMatrix);
  }
  else
  {

    D3DMATRIX mat;

    mat._11 = 1;
    mat._12 = 0;
    mat._13 = 0;
    mat._14 = 0;

    mat._21 = 0;
    mat._22 = 1;
    mat._23 = 0;
    mat._24 = 0;

    mat._31 = 0;
    mat._32 = 0;
    mat._33 = matC;
    mat._34 = 1;

    mat._41 = 0;
    mat._42 = 0;
    mat._43 = matD;
    mat._44 = 0;

    // we have to set projection matrix - D3D needs wNear a wFar from it

    //_d3DDevice->SetTransform( D3DTS_PROJECTION , &mat );
    LogSetProjXB(mat);
  }

}

void EngineDDXB::DoSwitchTL( bool active )
{
  FlushAndFreeAllQueues(_queueNo);


  //FlushAndFreeAllQueues(_queueTL);
  _tlActive = active;
  if (_recording) return;
  if (active)
  {
    PROFILE_DX_SCOPE(3eTL);
#if LOG_STATE_LEVEL
    if (LogStatesOnce)
    {
      LogF("SetVertexShader T&L");
    }
#endif

    // Set the vertex shader
    if (!_recording)
    {
      if (_shaderHandle != 0)
      {
        _d3DDevice->LoadVertexShader(_shaderHandle, 0);
        _d3DDevice->SelectVertexShader(NULL, 0);
        //_d3DDevice->SetVertexShader(_shaderHandle);
      }
    }

#ifndef _XBOX
    D3DSetRenderState(D3DRS_CLIPPING,TRUE);
#endif

  // Setup screenspace constants
  float screenspaceScale[4] = {_w/2.0f, -_h/2.0f, D3DZ_MAX_D24S8, 0.0f};
  _d3DDevice->SetVertexShaderConstant(SCREENSPACE_SCALE, screenspaceScale, 1);
  float screenspaceOffset[4] = {_w/2.0f + 0.53125f, _h/2 + 0.53125f, 0.0f , 0.0f};
  _d3DDevice->SetVertexShaderConstant(SCREENSPACE_OFFSET, screenspaceOffset, 1);

#ifdef _XBOX

    // Setup projection matrix
    ProjectionChanged();

#endif
  }
  else
  {
#if LOG_STATE_LEVEL
    if (LogStatesOnce)
    {
      LogF("SetVertexShader dummy");
    }
#endif
    PROFILE_DX_SCOPE(3dTL);

    _d3DDevice->LoadVertexShader(_shaderNonTLHandle, 0);
    _d3DDevice->SelectVertexShader(NULL, 0);
    
    // Setup screenspace constants
    float screenspaceNonTLScale[4] = {1.0f, 1.0f, D3DZ_MAX_D24S8, 1.0f};
    _d3DDevice->SetVertexShaderConstant(SCREENSPACE_SCALE, screenspaceNonTLScale, 1);
    float screenspaceNonTLOffset[4] = {0.53125f, 0.53125f, 0.0f , 5.42101e-020};
    _d3DDevice->SetVertexShaderConstant(SCREENSPACE_OFFSET, screenspaceNonTLOffset, 1);
    
    //_d3DDevice->SetVertexShader(_shaderNonTLHandle);
    //Log("Default non-T&L vertex shader %x selected",MYFVF);
    //D3DSetRenderState(D3DRS_LIGHTING,FALSE);
#ifndef _XBOX
    D3DSetRenderState(D3DRS_CLIPPING,FALSE);
#else
    //D3DSetRenderState(D3DRS_FOGTABLEMODE,D3DFOG_NONE);
#endif
    _iBufferLast = NULL; // forget any VB
    _vBufferLast = NULL;
  }

}

void EngineDDXB::DoSetMaterial(
  const TLMaterial &mat, const LightList &lights, int spec,
  bool forceSpecular

)
{
  _isWorldMatrixChanged = false;
  _materialSet = mat;
  _materialSetSpec = spec;
  // set corresponding render states
  // combine material with light


  PROFILE_DX_SCOPE(3mat);

  if (mat.specularPower>0 || forceSpecular)
  {
    D3DSetRenderState(D3DRS_SPECULARENABLE,TRUE);
    //D3DSetRenderState(D3DRS_LOCALVIEWER,TRUE);
    SelectPixelShaderSpecular(PSSSpecular);
  }
  else
  {
    D3DSetRenderState(D3DRS_SPECULARENABLE,FALSE);
    SelectPixelShaderSpecular(PSSNormal);
  }
#if LOG_STATE_LEVEL
  if (LogStatesOnce)
  {
    LogF
      (
      "Set material da %.2f, aa %.2f, em %.2f,%.2f,%.2f",
      mat.diffuse.A(),mat.ambient.A(),
      mat.emmisive.R(),mat.emmisive.G(),mat.emmisive.B()
      );
  }
#endif

  // adapt lights based on mat. forcedDiffuse

  SetupLights(mat,lights,spec);
}

//TypeIsSimple(Light *);

void EngineDDXB::ProjectionChanged()
{
  if (!_tlActive) return;

  if (_recording) return;

  // Set the projection matrix
  D3DXMATRIX projMatrix;
  int bias = _bias;
  Matrix4 proj = _sceneProps->GetCameraProjection();
  ConvertProjectionMatrix(projMatrix, proj, bias);
  D3DXMatrixTranspose(&projMatrix, &projMatrix);
  _d3DDevice->SetVertexShaderConstant(PROJ_MATRIX_0, &projMatrix, 4);
  LogSetProjXB(projMatrix);

  // Set the fog and shadow volume parameters
  Camera *camera = GScene->GetCamera();
  float wFogStart      = ( 0.2*(camera->ClipFarFog()-camera->ClipNear()) ) + camera->ClipNear();
  float wFogEnd        = camera->ClipFarFog();
  float fogAndShadowVolumeConstants[4] = {wFogEnd, 1.0f / (wFogEnd - wFogStart), 0.05f , 100.0f};
  _d3DDevice->SetVertexShaderConstant(FOGEND__RCP_FOGEND_MINUS_FOGSTART__SHADOWVOLUME_OFFSET__SHADOWVOLUME_SIZE, &fogAndShadowVolumeConstants, 1);
}

void EngineDDXB::EnableSunLight(EMainLight mainLight)
{
  if (mainLight == _mainLight) return;
  _mainLight = mainLight;

  if (!_tlActive) return;
  //_d3DDevice->LightEnable(0,_sunEnabled);

  _materialSetSpec = -1; // set invalid spec
  // set invalid material to force setting lights
  _materialSet.diffuse = Color(-1,-1,-1,-1);
  _materialSet.ambient = Color(-1,-1,-1,-1);
  _materialSet.forcedDiffuse = Color(-1,-1,-1,-1);
  _materialSet.emmisive = Color(-1,-1,-1,-1);
  _materialSet.specFlags = 0;

}

void EngineDDXB::SetFogMode(EFogMode fogMode)
{
  _fogMode = fogMode;
}

void EngineDDXB::BeginMeshTLInstanced(
  const Shape &sMesh, const EngineShapeProperties &prop, int spec, const LightList &lights, int bias,
  const DrawParameters &dp
)
{
  Assert(_visTestCount==0);
  PROFILE_SCOPE(xbBMI);
  // The world is identity (all transformations are done by bones)
  _world.SetIdentity();
  _invWorld.SetIdentity();
  _isWorldMatrixChanged = true;

#ifdef _XBOX
  // Setup position of the camera in model coordinates, keep the model to camera distance

  if (!_recording)
  {
    // Get the position of the camera
    Vector3 cameraPosition = _sceneProps->GetCameraPosition();

    // Transform and normalize the camera position
    Vector3 cameraObjectPosition = TransformAndNormalizePositionToObjectSpace(cameraPosition);

    D3DXVECTOR4 cpos(cameraObjectPosition.X(), cameraObjectPosition.Y(), cameraObjectPosition.Z(), 1.0f);
    _d3DDevice->SetVertexShaderConstant(CAMERA_POSITION, &cpos, 1);
  }
#endif

  // And the rest...
  SwitchTL(true);
  ChangeClipPlanes();

  //////////////////////////////////////////////////////////////////////////

  //SetBias(bias);

  //////////////////////////////////////////////////////////////////////////

  // update if necessary
  VertexBufferD3DXB *vb = static_cast<VertexBufferD3DXB *>(sMesh.GetVertexBuffer());

  if (vb->IsInList())
  {
    _vBufferLRU.Refresh(vb);
  }
  else
  {
    _vBufferLRU.Add(vb);
  }

  vb->SetOwner(&sMesh);

  // Update also sets the _posScale and _posOffset values
  vb->Update(sMesh, prop, VSDV_Instancing);

  //////////////////////////////////////////////////////////////////////////

  // Setup the worldView matrix
#ifdef _XBOX

  if (!_recording)
  {
    Camera *camera = GScene->GetCamera();
    Matrix4 view = camera->InverseScaled();
    D3DXMATRIX viewMatrix;
    ConvertMatrixTransposed(viewMatrix, view);
    _d3DDevice->SetVertexShaderConstant(WORLD_VIEW_MATRIX_0, &viewMatrix, 3);
  }

#endif

  // Set the lights
  if (!_recording)
  {
    SetLights(lights,spec);
  }
}

void EngineDDXB::SetupInstances(const ObjectInstanceInfo *instances, int nInstances)
{
  PROFILE_SCOPE(xbStI);
  Assert(nInstances > 0);
  Assert(nInstances <= MaxInstances);
  // Setup the instance bones
  FLOAT boneValues[MaxInstances*4*4];
  for (int i = 0; i < nInstances; i++)
  {
    ConvertMatrixTransposed3(&boneValues[i * 4 * 4], instances[i]._origin);
    ColorVal icolor = instances[i]._color;
    boneValues[i * 4 * 4 + 3 * 4 + 0] = icolor.R()*_modColor.R();
    boneValues[i * 4 * 4 + 3 * 4 + 1] = icolor.G()*_modColor.G();
    boneValues[i * 4 * 4 + 3 * 4 + 2] = icolor.B()*_modColor.B();
    boneValues[i * 4 * 4 + 3 * 4 + 3] = icolor.A();
  }
  _d3DDevice->SetVertexShaderConstant(BONES_START, boneValues, nInstances * 4);

#if _ENABLE_PERFLOG
  ADD_COUNTER(stIns, 16 * nInstances * 4);
#endif

}

void EngineDDXB::SetupInstancesSprite(const ObjectInstanceInfo *instances, int nInstances)
{
  Assert(nInstances > 0);
  Assert(nInstances <= MaxInstancesSprite);
  FLOAT boneValues[MaxInstancesSprite*2*4];
  for (int i = 0; i < nInstances; i++)
  {
    Color color = instances[i]._object->GetConstantColor();
    boneValues[i * 2 * 4 + 0 * 4 + 0] = instances[i]._object->Position().X();
    boneValues[i * 2 * 4 + 0 * 4 + 1] = instances[i]._object->Position().Y();
    boneValues[i * 2 * 4 + 0 * 4 + 2] = instances[i]._object->Position().Z();
    boneValues[i * 2 * 4 + 0 * 4 + 3] = instances[i]._object->Scale();
    boneValues[i * 2 * 4 + 1 * 4 + 0] = color.R();
    boneValues[i * 2 * 4 + 1 * 4 + 1] = color.G();
    boneValues[i * 2 * 4 + 1 * 4 + 2] = color.B();
    boneValues[i * 2 * 4 + 1 * 4 + 3] = color.A();
  }
  _d3DDevice->SetVertexShaderConstant(BONES_START, boneValues, nInstances * 2);
}

/**
@param useColors when true, color is used as emissive,
  when false, it is used to modulate all colors, and alpha is used to control shadows
*/

void EngineDDXB::DrawMeshTLInstanced(
  const Shape &sMesh, const SectionMaterialLODs &matLOD, const EngineShapeProperties &prop, int spec, const LightList &lights, int bias,
  const ObjectInstanceInfo *instances, int nInstances, float minDist2, bool useColors, const DrawParameters &dp
)
{
  DoAssert(nInstances>0);
  // Go through all instances in chunks
  VertexBufferD3DXB *vb = static_cast<VertexBufferD3DXB *>(sMesh.GetVertexBuffer());
  int nInstancesInOneStep = vb->GetVBufferSize() / sMesh.NVertex();

  // Either instances in one step must be > 1 (we don't create VB for smaller size), or 
  // we are recording (we need to create PB even for size 1)
  if ((nInstancesInOneStep > 1) || _recording)
  {
    // Set shader constants
    {
      PROFILE_SCOPE_GRF(xbDMI,SCOPE_COLOR_RED);
      BeginMeshTLInstanced(sMesh, prop, spec, lights, bias, dp);
    }

    // Either use a program to draw or draw it the usual way
    if (sMesh.ProgramExists())
    {
      PROFILE_SCOPE_GRF(drwPB,SCOPE_COLOR_RED);

      // Prepare
      sMesh.PrepareDrawUsingProgram(spec);

      //saturateMin(nInstancesInOneStep, 1);
      int drawnInstances = 0;
      int maxInstances = PBInstancesMax;
      int oldInstances = -1;
      while (drawnInstances < nInstances)
      {
        // Get number of instances
        int newInstances = min(nInstancesInOneStep, nInstances - drawnInstances);

        // Fit the number of instances to draw to possible number of instances
        saturateMin(newInstances, maxInstances);
        newInstances = sMesh.GetAvailableInstances(newInstances);

        // We will never in this cycle draw more than newInstances instances
        maxInstances = newInstances;

        // In case newInstances differs from old instances then call the prepare again,
        // because there we need to fixup the proper push-buffer entry
        if (oldInstances != newInstances)
        {
          PROFILE_SCOPE_GRF(prePB,SCOPE_COLOR_RED);
          sMesh.PrepareItemDrawUsingProgram(spec, newInstances);
          oldInstances = newInstances;
        }

        // Set shader constants
        Assert(!_recording);
        //if (!_recording) // we shouldn't reach this code when recording
        {
          if (instances[drawnInstances]._object)
          {
            PROFILE_SCOPE_GRF(xbDSO,SCOPE_COLOR_RED);
            instances[drawnInstances]._object->SetupInstances(&instances[drawnInstances], newInstances);
          }
          else
          {
            PROFILE_SCOPE_GRF(xbDSI,SCOPE_COLOR_RED);
            SetupInstances(&instances[drawnInstances], newInstances);
          }
        }

        // Draw
        {
          PROFILE_SCOPE_GRF(runPB,SCOPE_COLOR_RED);
          sMesh.DrawUsingProgram(newInstances);
        }

        // Update the number of drawn instances
        drawnInstances += newInstances;
      }

      // Finish
      {
        PROFILE_SCOPE_GRF(finPB,SCOPE_COLOR_RED);
        sMesh.FinishDrawUsingProgram();
      }
    }
    else
    {
      int drawnInstances = 0;
      while (drawnInstances < nInstances)
      {
        // Get number of instances
        int newInstances = min(nInstancesInOneStep, nInstances - drawnInstances);

        // Set shader constants
        if (!_recording)
        {
          if (instances[drawnInstances]._object)
          {
            PROFILE_SCOPE(xbDSO);
            instances[drawnInstances]._object->SetupInstances(&instances[drawnInstances], newInstances);
          }
          else
          {
            PROFILE_SCOPE(xbDSI);
            SetupInstances(&instances[drawnInstances], newInstances);
          }
        }

        // Go through the sections
        for (int i=0; i<sMesh.NSections(); i++)
        {
          const ShapeSection &sec = sMesh.GetSection(i);

          if (sec.Special()&(IsHidden|IsHiddenProxy)) continue;

          // flush section
          TLMaterial mat;
#if _ENABLE_PERFLOG
          if (LogStatesOnce)
          {
            LogF("Material %d",sec.GetMaterial());
          }
#endif

          int matIndex = sec.GetMaterial();
          // distribute by predefined materials
          CreateMaterial(mat, HWhite, matIndex);

          sec.PrepareTL(mat, 0, lights, spec, prop);
          PROFILE_SCOPE(xbIDS);
          DrawSectionTL(sMesh, i, i+1, bias, drawnInstances, newInstances);
        }

        // Update the number of drawn instances
        drawnInstances += newInstances;
      }
    }

    // Finish mesh
    PROFILE_SCOPE(xbIEM);
    EndMeshTL(sMesh);
  }
  else
  {
    PROFILE_SCOPE(xbDME);
    DoAssert(instances);
    Engine::DrawMeshTLInstanced(sMesh, matLOD, prop, spec, lights, bias, instances, nInstances, minDist2, useColors, dp);
    return;
  }
}

void EngineDDXB::SetupShadowVolumeInstances(const ObjectInstanceInfo *instances, int nInstances)
{
  // Setup the instance bones
  FLOAT boneValues[MaxInstancesShadowVolume * 3 * 4];
  for (int i = 0; i < nInstances; i++)
  {
    ConvertMatrixTransposed3(&boneValues[i * 3 * 4], instances[i]._origin);
  }
  _d3DDevice->SetVertexShaderConstant(BONES_START, boneValues, nInstances * 3);
}

void EngineDDXB::DrawShadowVolumeInstanced(LODShape *shape, int level,const ObjectInstanceInfo *instances, int instanceCount)
{
  ShapeUsed shadow = shape->Level(level);
  DoAssert(instanceCount > 0);

  // Return immediately in some cases
  if (instanceCount == 0) return;
  if (!GetTL()) return;
  if (shadow->GetVertexBuffer() == NULL) return;

  // Go through all instances in chunks
  VertexBufferD3DXB *vb = static_cast<VertexBufferD3DXB *>(shadow->GetVertexBuffer());
  int nInstancesInOneStep = vb->GetVBufferSize() / shadow->NVertex();

  if ((instanceCount <= 1) || (nInstancesInOneStep <= 1))
  {
    Engine::DrawShadowVolumeInstanced(shape,level, instances, instanceCount);
    return;
  }

  // Prepare shadow volume drawing
  BeginShadowVolume(100.0f);

  // Set shader constants
  int spec=IsShadow|IsAlphaFog;
  const LightList noLights;
  BeginMeshTLInstanced(*shadow, *shape, spec, noLights, ShadowBias, DrawParameters::_default);

  // Set shadow material
  TLMaterial shadowMat;
  shadowMat.diffuse = HBlack;
  shadowMat.ambient = HBlack;
  shadowMat.emmisive = HBlack;
  shadowMat.forcedDiffuse = HBlack;
  shadowMat.specFlags = 0;

  int drawnInstances;

  // ---------------
  // Draw back faces

  EngineShapeProperties prop;
  drawnInstances = 0;
  while (drawnInstances < instanceCount)
  {
    // Get number of instances
    int newInstances = min(nInstancesInOneStep, instanceCount - drawnInstances);

    // Set the instances
    SetupShadowVolumeInstances(&instances[drawnInstances], newInstances);

    // Go through all the sections
    for (int i = 0; i < shadow->NSections(); i++)
    {
      ShapeSection sec = shadow->GetSection(i);

      // Skip sections not designed for drawing
      if (sec.Special() & (IsHidden|IsHiddenProxy|NoShadow)) continue;

      // Draw section
      sec.OrSpecial(IsShadow | IsShadowVolume);
      sec.SetTexture(NULL);
      sec.PrepareTL(shadowMat, 0, noLights, 0, prop);
      DrawSectionTL(*shadow, i, i + 1, ShadowBias, drawnInstances, newInstances);
    }

    // Update the number of drawn instances
    drawnInstances += newInstances;
  }

  // ----------------
  // Draw front faces

  drawnInstances = 0;
  while (drawnInstances < instanceCount)
  {
    // Get number of instances
    int newInstances = min(nInstancesInOneStep, instanceCount - drawnInstances);

    // Set the instances
    SetupShadowVolumeInstances(&instances[drawnInstances], newInstances);

    // Go through all the sections
    for (int i = 0; i < shadow->NSections(); i++)
    {
      ShapeSection sec = shadow->GetSection(i);

      // Skip sections not designed for drawing
      if (sec.Special() & (IsHidden|IsHiddenProxy|NoShadow)) continue;

      // Draw section
      sec.OrSpecial(IsShadow | IsShadowVolume | ShadowVolumeFrontFaces);
      sec.SetTexture(NULL);
      sec.PrepareTL(shadowMat, 0, noLights, 0, prop);
      DrawSectionTL(*shadow, i, i + 1, ShadowBias, drawnInstances, newInstances);
    }

    // Update the number of drawn instances
    drawnInstances += newInstances;
  }

  // Finish the mesh drawing
  EndMeshTL(*shadow);

  // Set skinning to default value (FLY Better than this clean-up to PrepareShapeDraw would be better to set it before each drawing)
  SetSkinningType(STNone);
}

class Matrix4x4
{
public:
  float _o[4][4];
  //       c  r
  // columns corresponds to aside,up,dir,pos

  // Matrix3 operator has identical argument ordering: row column
  float &Set(int r, int c) {return _o[c][r];}

  float &operator () (int r, int c) {return _o[c][r];}
  const float &operator () (int r, int c) const {return _o[c][r];}

  void SetZero()
  {
    for (int i=0; i<4; i++)
    {
      _o[i][0]=_o[i][1]=_o[i][2]=_o[i][3] = 0;
    }
  }
  void SetIdentity()
  {
    SetZero();
    _o[0][0]=_o[1][1]=_o[2][2]=_o[3][3] = 1;
  }

  void SetNormal(const Matrix4 &m)
  {
    Set(0,0) = m(0,0);
    Set(1,0) = m(1,0);
    Set(2,0) = m(2,0);
    Set(3,0) = 0;

    Set(0,1) = m(0,1);
    Set(1,1) = m(1,1);
    Set(2,1) = m(2,1);
    Set(3,1) = 0;

    Set(0,2) = m(0,2);
    Set(1,2) = m(1,2);
    Set(2,2) = m(2,2);
    Set(3,2) = 0;

    Set(0,3) = m.Position()[0];
    Set(1,3) = m.Position()[1];
    Set(2,3) = m.Position()[2];
    Set(3,3) = 1;
  }
  void SetPerspective(const Matrix4 &m)
  {
    Set(0,0) = m(0,0);
    Set(1,0) = m(1,0);
    Set(2,0) = m(2,0);
    Set(3,0) = 0;

    Set(0,1) = m(0,1);
    Set(1,1) = m(1,1);
    Set(2,1) = m(2,1);
    Set(3,1) = 0;

    Set(0,2) = m(0,2);
    Set(1,2) = m(1,2);
    Set(2,2) = m(2,2);
    Set(3,2) = 1;

    Set(0,3) = m.Position()[0];
    Set(1,3) = m.Position()[1];
    Set(2,3) = m.Position()[2];;
    Set(3,3) = 0;
  }
  void SetMultiply(const Matrix4x4 &a, const Matrix4x4 &b)
  {
    for (int i=0; i<4; i++ ) for(int j=0; j<4; j++)
    {
      Set(i,j) =
        (
        a(i,0)*b(0,j)+
        a(i,1)*b(1,j)+
        a(i,2)*b(2,j)+
        a(i,3)*b(3,j)
        );
    }

  }
  void Transpose()
  {
    swap(Set(0,1),Set(1,0));swap(Set(0,2),Set(2,0));swap(Set(0,3),Set(3,0));
    swap(Set(1,2),Set(2,1));swap(Set(1,3),Set(3,1));
    swap(Set(2,3),Set(3,2));
  }
};

static const D3DMATRIX D3DMatIdentity=
{
  1,0,0, 0,
    0,1,0, 0,
    0,0,1, 0,
    0,0,0, 1,
};

static const D3DMATRIX D3DMatZero =
{
  0,0,0, 0,
    0,0,0, 0,
    0,0,0, 0,
    0,0,0, 0,
};

void EngineDDXB::BeginShadowVolume(float shadowVolumeSize)
{
  if (!_recording)
  {
    float constants[4] = {0.03f, shadowVolumeSize, 0.0f , 0.0f};
    _d3DDevice->SetVertexShaderConstant(SHADOW_OFFSET__SHADOW_SIZE, &constants, 1);
  }
}

void EngineDDXB::BeginInstanceTL(const Matrix4 &modelToWorld, const DrawParameters &dp)
{
  if (!_recording)
  {
    _world = modelToWorld;
    // FLY It might be better to get the inverse matrix in the parameter
    _invWorld = modelToWorld.InverseGeneral();
    _isWorldMatrixChanged = true;

    // Setup the worldView matrix
    Camera *camera = GScene->GetCamera();
    Matrix4 worldView = camera->InverseScaled() * modelToWorld;
    D3DXMATRIX worldViewMatrix;
    ConvertMatrixTransposed(worldViewMatrix, worldView);

    _d3DDevice->SetVertexShaderConstant(WORLD_VIEW_MATRIX_0, &worldViewMatrix, 3);
  }
}

void EngineDDXB::BeginMeshTL( const Shape &sMesh, const EngineShapeProperties &prop, int spec, const LightList &lights, int bias)
{
  Assert(_visTestCount==0);

#ifdef _XBOX
  if (!_recording)
  {
    // Setup position of the camera in model coordinates, keep the model to camera distance

    // Get the position of the camera
    Vector3 cameraPosition = _sceneProps->GetCameraPosition();

    // Transform and normalize the camera position
    Vector3 cameraObjectPosition = TransformAndNormalizePositionToObjectSpace(cameraPosition);

    D3DXVECTOR4 cpos(cameraObjectPosition.X(), cameraObjectPosition.Y(), cameraObjectPosition.Z(), 1.0f);
    _d3DDevice->SetVertexShaderConstant(CAMERA_POSITION, &cpos, 1);
  }
#endif

  // And the rest...
  SwitchTL(true);
  ChangeClipPlanes();

  //////////////////////////////////////////////////////////////////////////

  //SetBias(bias);

  //////////////////////////////////////////////////////////////////////////

  // update if necessary
  VertexBufferD3DXB *vb = static_cast<VertexBufferD3DXB *>(sMesh.GetVertexBuffer());

  if (vb->IsInList())
  {
    _vBufferLRU.Refresh(vb);
  }
  else
  {
    _vBufferLRU.Add(vb);
  }

  vb->SetOwner(&sMesh);

  // Update also sets the _posScale and _posOffset values
  vb->Update(sMesh, prop, _skinningType != STNone ? VSDV_Skinning : VSDV_Basic);


  //////////////////////////////////////////////////////////////////////////

  // Set the lights
  if (!_recording)
  {
    SetLights(lights, spec, dp._fakeLightRelocation);
  }
}

void EngineDDXB::EndMeshTL( const Shape &sMesh )
{
  //_sBuffer = NULL;
  // turn off all lights
  ClearLights();
  VertexBuffer *buffer = sMesh.GetVertexBuffer();
  if (buffer)
  {
    buffer->BindWithPushBuffer();
  }
}

int EngineDDXB::AddLight(Light *light)
{
  _lightsHasChanged = true;
  for(int i=0; i<_lights.Size(); i++)
  {
    if (!_lights[i])
    {
      _lights[i] = light;
      return i;
    }
  }
  return _lights.Add(light);
}

void EngineDDXB::ClearLights()
{
  if (_lights.Size()==0) return;
  _lightsHasChanged = true;
  for(int i=0; i<_lights.Size(); i++)
  {
    if (_lights[i])
    {
      //_d3DDevice->LightEnable(i+1,FALSE);
    }
  }
  _lights.Resize(0); // currently active lights
  if (_lights.MaxSize()>64) _lights.Clear();
}


void EngineDDXB::DiscardVB()
{
}

static __forceinline PackedColor ColorScale(PackedColor col) {return col;}

bool EngineDDXB::AddVertices( const TLVertex *v, int n )
{
  if( n<=0 ) return true;
  // keep meshbuffer allocated as long as possible
  // fill vertex buffer with vertices
  HRESULT err;

  BYTE *data = NULL;
  int size=sizeof(TLVertex)*n;

#if 1
  int lockOffset = 0;
  int lockFlags = 0;
  if (_queueNo._vertexBufferUsed+n<=MeshBufferLength && !_queueNo._firstVertex)
  {
#if LOG_LOCKS
    LogF("add D3DLOCK_NOOVERWRITE %d + %d",_queueNo._vertexBufferUsed,n);
#endif
    lockOffset = _queueNo._vertexBufferUsed*sizeof(TLVertex);
    lockFlags = NoSysLockFlag|D3DLOCK_NOOVERWRITE;
  }
  else
#endif
  {
    if (n>MeshBufferLength)
    {
      RptF("Vertex Buffer too small (%d<%d)",n,MeshBufferLength);
      return false;
    }
    _queueNo._firstVertex = false;

    // start a new vertex buffer
#if LOG_STATE_LEVEL
    if (LogStatesOnce)
    {
      LogF("Vertex buffer full");
    }
#endif

    //FlushAndFreeAllQueues(_queueNo);
    FlushAllQueues(_queueNo);
    // check if we will fit into actual vertex buffer
    // if not, flush old vertex buffer
#if LOG_LOCKS
    LogF("add D3DLOCK_DISCARD %d + %d",0,n);
#endif
    lockFlags = NoSysLockFlag|DiscardLockFlag;
#if CUSTOM_VB_DISCARD
    if (++_queueNo._actualVBIndex>=NumDynamicVB2D)
    {
      _queueNo._actualVBIndex = 0;
    }
#endif
    _queueNo._vertexBufferUsed = 0;
  }

  const ComRef<IDirect3DVertexBuffer8> &buf = _queueNo.GetVBuffer();
Retry:
  {
    PROFILE_DX_SCOPE(3vbLA);
    err=buf->Lock
      (
      lockOffset,size,&data,lockFlags
      );
  }
  if( err!=D3D_OK )
  {
    if (err!=D3D_OK || !data)
    {
      //if (QFBank::FreeUnusedBanks(1024*1024)) goto Retry;
      if (FreeOnDemandSystemMemory(1024*1024)) goto Retry;
#ifndef _XBOX
      ErrF("VB Lock failed, %s",DXGetErrorString8(err));
#else
      ErrF("VB Lock failed, %x",err);
#endif
      return false;
    }
  }
  else
  {

    if (data)
    {
      // memcpy not possible if we want to apply factor 0.5
      // if would be more efficient to apply it sooner,
      // but for this we probably need to change Engine interface
      if (UsingPixelShaders())
      {
        TLVertex *vData = (TLVertex *)data;
        for (int i=0; i<n; i++)
        {
          const TLVertex &s = v[i];
          TLVertex &d = vData[i];

          d.pos = s.pos;
          d.rhw = s.rhw;
          d.color = ColorScale(s.color);
          d.specular = ColorScale(s.specular);
          d.t0 = s.t0;
          d.t1 = s.t1;
        }
      }
      else
      {
        TLVertex *vData = (TLVertex *)data;
        for (int i=0; i<n; i++)
        {
          const TLVertex &s = v[i];
          TLVertex &d = vData[i];

          d.pos = s.pos;
          d.rhw = s.rhw;
          d.color = ColorScale(s.color);
          d.specular = s.specular;
          d.t0 = s.t0;
          d.t1 = s.t1;
        }
      }
      //memcpy(data,v,size);
    }
    err=buf->Unlock();
    if( err!=D3D_OK )  DDErrorXB("Cannot unlock vertex buffer.",err);
  }
  _queueNo._meshBase = _queueNo._vertexBufferUsed;
  _queueNo._meshSize = n;
  _queueNo._vertexBufferUsed += n;
  return true;
}

bool EngineDDXB::BeginMesh( TLVertexTable &mesh, int spec )
{
  SwitchTL(false);
  _mesh = &mesh;

  return AddVertices(mesh.VertexData(),mesh.NVertex());
}

void EngineDDXB::EndMesh( TLVertexTable &mesh )
{
  _mesh = NULL;
}

QueueXB::QueueXB()
{
  for (int i=0; i<MaxTriQueues; i++) _triUsed[i] = false;
  _usedCounter = 0;
  _vertexBufferUsed = 0;
  _indexBufferUsed = 0;
  _meshBase = 0;
  _meshSize = 0;
  _actTri = -1;
  _firstVertex = true;
  _firstIndex = true;
  _actualVBIndex=0;
  _actualIBIndex=0;

}

int QueueXB::Allocate
(
 TextureD3DXB *tex, int level, const TexMaterial *mat, int spec,
 int minI, int maxI, int tip
 )
{
  int index = -1;
  // slot shown by tip may (but need not) contain the setup we need
  if (tip>=minI && tip<maxI && _triUsed[tip])
  {
    TriQueueXB &triq = _tri[tip];
    if (tex==triq._texture && spec==triq._special) index = tip;
  }


  int free = -1;
  if (index<0)
  {
    for (int i=minI; i<maxI; i++)
    {
      if (_triUsed[i])
      {
        TriQueueXB &triq = _tri[i];
        if (tex!=triq._texture) continue;
        if (mat!=triq._material) continue;
        if (spec!=triq._special) continue;
        index = i;
      }
      else
      {
        if (free<0) free = i;
      }
    }
  }
  _usedCounter++;
  if (index>=0)
  {
    TriQueueXB &triq = _tri[index];
    // append to queue index
    saturateMin(triq._level,level);
    triq._lastUsed = _usedCounter;
    Assert (_triUsed[index]);
    /*
    if (LogStatesOnce)
    {
    LogF("Allocated old queue %d: %s",index,tex ? tex->Name() : "<NULL>");
    }
    */
    return index;
  }
  if (free>=0)
  {
    TriQueueXB &triq = _tri[free];
    //triq._queueBase = _meshBase;
    //triq._queueSize = 0;
    triq._special = spec;
    triq._texture = tex;
    triq._level = level;
    triq._material = mat;
    triq._lastUsed = _usedCounter;
    Assert (triq._triangleQueue.Size()==0);
    triq._triangleQueue.Resize(0);
    _triUsed[free] = true;
#if LOG_STATE_LEVEL
    if (LogStatesOnce)
    {
      LogF("Allocated new queue %d: %s",free,tex ? tex->Name() : "<NULL>");
    }
#endif
  }
  return free;
}

void QueueXB::Free( int i )
{
  Assert (_tri[i]._triangleQueue.Size()==0);
  Assert( _triUsed[i] );
  _triUsed[i] = false;
#if LOG_STATE_LEVEL
  if (LogStatesOnce)
  {
    LogF("Released queue %d",i);
  }
#endif
  _tri[i]._texture = NULL;
  _tri[i]._material = NULL;
}

void EngineDDXB::DoSwitchRenderMode(RenderMode mode)
{
#if LOG_STATE_LEVEL
  if (LogStatesOnce)
  {
    LogF("DoSwitchRenderMode %d",mode);
  }
#endif
  FlushAndFreeAllQueues(_queueNo);
  _renderMode = mode;
}

WORD *EngineDDXB::QueueAdd( QueueXB &queue, int n )
{
  Assert (queue._actTri>=0);
  Assert (queue._triUsed[queue._actTri]);
  TriQueueXB &triq = queue._tri[queue._actTri];
  if( triq._triangleQueue.Size()+n>TriQueueSize )
  {
    // flush but keep allocated
#if LOG_STATE_LEVEL
    if (LogStatesOnce)
    {
      LogF("QueueXB full %d",queue._actTri);
    }
#endif
    FlushQueue(queue,queue._actTri);
  }

  int index = triq._triangleQueue.Size();
  /*
  if (index==0)
  {
  // starting a new queue content
  triq._queueBase = queue._meshBase;
  triq._queueSize = 0;
  }
  */

  // HW might not be in defined state anymore (the flushing can occur at any time
  // - especially between setting the HW to defined state and running the PB)
  _hwInDefinedState = false;

  triq._triangleQueue.Resize(index+n);
  return triq._triangleQueue.Data()+index;
}

void EngineDDXB::QueueFan( const VertexIndex *ii, int n )
{
  int addN = (n-2)*3;
  //TriQueueXB &triq = _queueNo._tri[_queueNo._actTri];
  WORD *tgt = QueueAdd(_queueNo,addN);

  if (!tgt)
  {
    Fail("No target in QueueFan");
    return;
  }
  if (!ii)
  {
    Fail("No source in QueueFan");
    return;
  }

  int offset = _queueNo._meshBase;

  Assert( offset>=0 );
#if DIAG_QUEUE
  LogF
    (
    "QueueXB fan %d - meshBase %d, queueBase %d, offset %d",
    n,_queueNo._meshBase,triq._queueBase,offset
    );
  for (int i=0; i<n; i++)
  {
    LogF("  %d -> %d",ii[0],ii[0]+offset);
  }
#endif

  int oii0 = ii[0]+offset;
  int oiip = ii[1]+offset;

  //saturateMax(triq._queueSize,oii0+1);
  //saturateMax(triq._queueSize,oiip+1);

  // add all triangles
  /*
  if (LogStatesOnce)
  {
  LogF("  queue %d",n);
  for( int i=0; i<n; i++ )
  {
  LogF("    queue %d - %d",ii[0],ii[0]+offset);
  }
  }
  */
  // BUG: crash:
  // tgt (eax) == NULL, ii (ecx) == NULL
  // i (esi) ==2, n (edi) == 4
  // optimize calculations
  const VertexIndex *vi = ii+2;
  for( int i=2; i<n; i++ )
  {
    int oiin = (*vi++) + offset;
    tgt[0] = oii0;
    tgt[1] = oiip;
    tgt[2] = oiin;
    tgt += 3;
    //saturateMax(triq._queueSize,oiin+1);
    oiip = oiin;
  }
}

int EngineDDXB::AllocateQueue(QueueXB &queue, TextureD3DXB *tex, int level, const TexMaterial *mat, int spec)
{
  // scan if there is some queue with same attributes
  bool alpha = tex && tex->IsAlpha() || !_enableReorder;

  int minI = 0;
  int maxI = MaxTriQueues-1;
  if (alpha)
  {
    minI = MaxTriQueues-1, maxI = MaxTriQueues;
    // flush all other queues
    FlushAllQueues(queue,MaxTriQueues-1);
  }

  int index = queue.Allocate(tex,level,mat,spec,minI,maxI,queue._actTri);
  if (index>=0)
  {
    Assert(queue._triUsed[index]);
    return index;
  }
  // we must free some queue
  // hard to tell which, select random
  //int random = GRandGen.RandomValue()&MaxTriQueues;
  // find LRU
  int minUsed = INT_MAX;
  for (int i=minI; i<maxI; i++)
  {
    int used = queue._tri[i]._lastUsed;
    if (used<minUsed)
    {
      minUsed = used;
      index = i;
    }
  }
  if (index<0)
  { 
    Fail("LRU failed");
    index=0;
  }
  FlushAndFreeQueue(queue,index);

  index = queue.Allocate(tex,level,mat,spec,minI,maxI,index);

  Assert (index>=0); // some queues is free now (after FlushAndFreeQueue)

  Assert(queue._triUsed[index]);

  return index;
}

void EngineDDXB::FreeQueue(QueueXB &queue, int index)
{
  // queue is already flushed
  Assert(!queue._tri[index]._triangleQueue.Size());
  queue.Free(index);
}

void EngineDDXB::FlushAllQueues(QueueXB &queue, int skip)
{
#if LOG_STATE_LEVEL>=2
  if (LogStatesOnce)
  {
    LogF("FlushAllQueues");
  }
#endif
  for (int i=0; i<MaxTriQueues; i++) if (i!=skip)
  {
    if (queue._triUsed[i])
    {
      FlushQueue(queue,i);
    }
    Assert(queue._tri[i]._triangleQueue.Size()==0);
  }
}

void EngineDDXB::FreeAllQueues(QueueXB &queue)
{
#if LOG_STATE_LEVEL>=2
  if (LogStatesOnce)
  {
    LogF("FreeAllQueues");
  }
#endif
  for (int i=0; i<MaxTriQueues; i++)
  {
    if (queue._triUsed[i])
    {
      // simulate flushing - ignore rest of the queue
      queue._tri[i]._triangleQueue.Clear();
      FreeQueue(queue,i);
    }
    else
    {
      Assert(queue._tri[i]._triangleQueue.Size()==0);
    }
    Assert(!queue._triUsed[i]);
    Assert(queue._tri[i]._triangleQueue.Size()==0);
  }
}

void EngineDDXB::FlushAndFreeAllQueues(QueueXB &queue, bool nonEmptyOnly)
{
#if LOG_STATE_LEVEL>=2
  if (LogStatesOnce)
  {
    LogF("FlushAndFreeAllQueues");
  }
#endif
  for (int i=0; i<MaxTriQueues; i++)
  {
    if
      (
      queue._triUsed[i] &&
      (!nonEmptyOnly || queue._tri[i]._triangleQueue.Size()>0)
      )
    {
      FlushAndFreeQueue(queue,i);
    }
    else
    {
      Assert(queue._tri[i]._triangleQueue.Size()==0);
    }
    Assert(nonEmptyOnly || !queue._triUsed[i]);
    Assert(queue._tri[i]._triangleQueue.Size()==0);
  }
  //queue._actTri = -1; // some queue may be allocated
}

void EngineDDXB::CloseAllQueues(QueueXB &queue)
{
  FlushAndFreeAllQueues(queue);
  queue._usedCounter = 0; // reset counter to avoid overflow
  queue._firstVertex = true; // reset counter to avoid overflow
}

void EngineDDXB::FlushQueue(QueueXB &queue, int index)
{
  //SwitchTL(false);
  TriQueueXB &triq = queue._tri[index];
  int n = triq._triangleQueue.Size();
  if( n>0 )
  {
    if (index==MaxTriQueues-1)
    {
      FlushAllQueues(queue,index); // avoid recursion
    }
#if LOG_STATE_LEVEL
    if (LogStatesOnce)
    {
      LogF
        (
        "Flush queue %d:%s %x",
        index,triq._texture ? triq._texture->Name() : "<NULL>",triq._special
        );
    }
#endif
    EngineShapeProperties prop;
    DoPrepareTriangle(triq._texture,triq._material,triq._level,triq._special,prop);
    DoSetMaterial2D();
    DoSelectPixelShader();

#if DIAG_QUEUE
    LogF("Flushing tri queue: base %d, size %d",_queue._queueBase,_queue._queueSize);
#endif
    // set index stream / vertex buffer
    // queue._meshBuffer
    // triq._triangleQueue.Data(),triq._triangleQueue.Size(),

    HRESULT ret;

    // create index buffer
    int size = n*sizeof(VertexIndex);


    int indexOffset = 0;
    BYTE *data;
    DWORD flags = NoSysLockFlag;
    if (n+queue._indexBufferUsed<=IndexBufferLength && !queue._firstIndex)
    {
#if LOG_LOCKS
      LogF("Index add D3DLOCK_NOOVERWRITE %d + %d",queue._indexBufferUsed,n);
#endif
      // append data
      indexOffset = queue._indexBufferUsed;
      flags |= D3DLOCK_NOOVERWRITE;
    }
    else
    {
#if LOG_LOCKS
      LogF("Index add D3DLOCK_DISCARD %d + %d",0,n);
#endif
      queue._firstIndex = false;
      // reset index buffer
      indexOffset = 0;
      flags |= DiscardLockFlag;
#if CUSTOM_VB_DISCARD
      if (++_queueNo._actualIBIndex>=NumDynamicVB2D)
      {
        _queueNo._actualIBIndex = 0;
      }
#endif
    }

    const ComRef<IDirect3DIndexBuffer8> &ibuf = queue.GetIBuffer();
    const ComRef<IDirect3DVertexBuffer8> &vbuf = queue.GetVBuffer();
    ret = ibuf->Lock
      (
      indexOffset*sizeof(VertexIndex),size,&data,flags
      );
    queue._indexBufferUsed = indexOffset+n;
    if (ret==D3D_OK)
    {
      memcpy(data,triq._triangleQueue.Data(),size);
      ibuf->Unlock();
    }
    else
    {
      DDErrorXB("iBuffer->Lock",ret);
    }
    // fill index buffer
    if (_iBufferLast!=ibuf || _vBufferLast!=vbuf)
    {
      _iBufferLast = ibuf;
      _vBufferLast = vbuf;
      _vOffsetLast = 0;
      _vsdVersionLast = VSDV_Basic;
      ADD_COUNTER(d3xVB,1);
      ADD_COUNTER(d3xIB,1);
#if EXPLICIT_FENCE_LEVEL>40
      _d3DDevice->InsertFence();
#endif
      _d3DDevice->SetIndices(ibuf,0);
      _hwInDefinedState = false;
      D3DSTREAM_INPUT si;
      si.VertexBuffer = vbuf;
      si.Stride = sizeof(TLVertex);
      si.Offset = 0;
      _d3DDevice->SetVertexShaderInputDirect(&VAFTL, 1, &si);
      _vBufferSize = 0;
    }

    PROFILE_DX_SCOPE(3drIP);
    DoAssert(_d3dFrameOpen);
    ret =_d3DDevice->DrawIndexedPrimitive
      (
      D3DPT_TRIANGLELIST,
      0,queue._vertexBufferUsed,
      indexOffset,n/3
      );
    DX_CHECKN("DIP",ret);
#if LOG_STATE_LEVEL
    if (LogStatesOnce)
    {
      LogF("  Draw .... %d tris",triq._triangleQueue.Size()/3);
    }
#endif
    ADD_COUNTER(tris,n/3);
    ADD_COUNTER(dPrim,1);
    ADD_COUNTER(inds, n);
    //triq._queueSize = 0;
    triq._triangleQueue.Clear();
  }
  //triq._queueBase = queue._meshBase;
  //triq._queueSize = 0;
  Assert(!triq._triangleQueue.Size());
}

void EngineDDXB::FlushAndFreeQueue(QueueXB &queue, int index)
{
  FlushQueue(queue,index);
  FreeQueue(queue,index);
}

void EngineDDXB::Queue2DPoly( const TLVertex *v0, int n )
{

  int addN = (n-2)*3;

  Assert( _queueNo._actTri>=0 );
  Assert (_queueNo._triUsed[_queueNo._actTri]);
  WORD *tgt = QueueAdd(_queueNo,addN);

  //TriQueueXB &triq = _queueNo._tri[_queueNo._actTri];
  int offset = _queueNo._meshBase;
  Assert( offset>=0 );
  //saturateMax(triq._queueSize,0+offset+1);
  //saturateMax(triq._queueSize,1+offset+1);

  // add all triangles
  for( int i=2; i<n; i++ )
  {
    *tgt++ = 0+offset;
    *tgt++ = i-1+offset;
    *tgt++ = i+offset;
    //saturateMax(triq._queueSize,i+offset+1);
  }
}

#if _RELEASE
#define DO_COUNTERS 0
#else
#define DO_COUNTERS 0
#endif

#if DO_COUNTERS
struct OptimizeCounter
{
  const char *name;
  int done,skipped;
  int counter;
  OptimizeCounter( const char *n):name(n),done(0),skipped(0),counter(0){}
  ~OptimizeCounter(){ReportSingle();}

  void Skip( int i=1 ){skipped+=i,counter+=i;}
  void Perform( int i=1 ){done+=i,counter+=i;ReportEvery();}
  void ReportSingle()
  {
    LogF("%s: %d optimized to %d",name,skipped+done,done);
    skipped=0;
    done=0;
    counter=0;
  }
  void ReportEvery( int n=1000 )
  {
    if( counter>n ) ReportSingle();
  }
};
#endif

#if RS_DIAGS
HRESULT EngineDDXB::D3DSetTextureStageStateName
(
 DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD value, const char *name, bool optimize
 )
#else
HRESULT EngineDDXB::D3DSetTextureStageState
(
 DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD value, bool optimize
 )
#endif
{
#if DO_COUNTERS
  static OptimizeCounter opt("TextureStageState");
#endif
  // assume small values of state (in DX6 max. state was about 25)
  AutoArray<TextureStageStateInfo> &stageState=_textureStageState[stage];
  stageState.Access(state);
  TextureStageStateInfo &info=stageState[state];

  if( info.value==value && optimize )
  {
#if DO_COUNTERS
    opt.Skip();
#endif
    return D3D_OK;
  }

  info.value=value;
#if DO_COUNTERS
  opt.Perform();
#endif
#if LOG_STATE_LEVEL
  if (LogStatesOnce)
  {
#if RS_DIAGS
    LogF("SetTextureStageState %d,%s,%d",stage,name,value);
#else
    LogF("SetTextureStageState %d,%d,%d",stage,state,value);
#endif
  }
#endif
#if !NO_SET_TSS
  PROFILE_DX_SCOPE(3sTSS);
  _hwInDefinedState = false;
  HRESULT ret = _d3DDevice->SetTextureStageState(stage,state,value);
  DX_CHECKN("SetTSS",ret);
  return ret;
#else
  return 0;
#endif
}

static bool IsDifferent(const D3DMATRIX &a, const D3DMATRIX &b)
{
  for (int i=0; i<4; i++)
  {
    const float eps = 1e-10;
    if (fabs(a.m[i][0]-b.m[i][0])>eps) return true;
    if (fabs(a.m[i][1]-b.m[i][1])>eps) return true;
    if (fabs(a.m[i][2]-b.m[i][2])>eps) return true;
    if (fabs(a.m[i][3]-b.m[i][3])>eps) return true;
  }
  return false;
}

void EngineDDXB::DoEnableDetailTexGen(
  const D3DMATRIX *matrix0, const D3DMATRIX *matrix1, const D3DMATRIX *matrix2,
  bool matrix0UVCompressed, bool matrix1UVCompressed, bool matrix2UVCompressed, bool optimize 
)
{
  if (!matrix0) matrix0 = &D3DMatIdentity;
  if (!matrix1) matrix1 = &D3DMatIdentity;
  if (!matrix2) matrix2 = &D3DMatIdentity;
  const int nTexGenStages = 3;
  const D3DMATRIX *wanted[nTexGenStages]={matrix0,matrix1,matrix2};
  bool matrixCompressed[nTexGenStages] = {matrix0UVCompressed,matrix1UVCompressed,matrix2UVCompressed};
  
  static const int tgt[nTexGenStages]=
  {
    TEX_TRANSFORM_MATRIX0_0,TEX_TRANSFORM_MATRIX1_0,TEX_TRANSFORM_MATRIX2_0
  };

  // only uv source is currently supported
#if LOG_STATE_LEVEL
  if (LogStatesOnce)
  {
    LogF(
      "Set tex gen 0 %.1f,%.1f, %.1f,%.1f",matrix0->_11,matrix0->_22,matrix0->_31,matrix0->_32
    );
    LogF(
      "Set tex gen 1 %.1f,%.1f, %.1f,%.1f",matrix1->_11,matrix1->_22,matrix1->_31,matrix1->_32
    );
    LogF(
      "Set tex gen 2 %.1f,%.1f, %.1f,%.1f",matrix2->_11,matrix2->_22,matrix2->_31,matrix2->_32
    );
  }
#endif
  // if UV compression changes, force update
  if(_texGenScale!=_newTexGenScale ||  _texGenOffset!=_newTexGenOffset)
  {
    optimize = false;
  }
  _texGenScale=_newTexGenScale;
  _texGenOffset=_newTexGenOffset;
  Assert(_texGenScale.u>0);
  Assert(_texGenScale.v>0);
  _texGenScaleOrOffsetHasChanged = false;
  for (int stage=0; stage<nTexGenStages; stage++)
  {
    if (!optimize || IsDifferent(_texGenMatrix[stage],*wanted[stage]))
    {
      _texGenMatrix[stage] = *wanted[stage];
#ifdef _XBOX
      D3DXMATRIX tgm = *wanted[stage];
      Assert(_texGenScale.u > 0.0f);
      Assert(_texGenScale.v > 0.0f);
      if (matrixCompressed[stage])
      {
        D3DXMATRIX sao = D3DMatIdentity; // Scale And Offset matrix
        sao._11 = _texGenScale.u * ( 1.0f / (SHRT_MAX - SHRT_MIN) );
        sao._22 = _texGenScale.v * ( 1.0f / (SHRT_MAX - SHRT_MIN) );
        sao._41 = _texGenOffset.u - SHRT_MIN / float(SHRT_MAX - SHRT_MIN) * _texGenScale.u;
        sao._42 = _texGenOffset.v - SHRT_MIN / float(SHRT_MAX - SHRT_MIN) * _texGenScale.v;
        tgm = sao * tgm;
      }
      D3DXMatrixTranspose(&tgm, &tgm);
      _d3DDevice->SetVertexShaderConstant(tgt[stage], &tgm, 2);
#endif
    }
  }
}

void EngineDDXB::PrepareSingleTexDiffuseA(int spec)
{
  DoEnableDetailTexGen(NULL,NULL,NULL,true,true,true,true);
  // check what kind of texture we are using
  // if it is DXT transparent, use special shader to avoid artifacts
  if ((spec&(IsTransparent|IsAlpha))==IsTransparent)
  {
    // actually we are not sure if the texture is DXT
    // no othet transparent textures should however be used on Xbox
    SelectPixelShader(PSNormalDXTA);
  }
  else
  {
    SelectPixelShader(PSNormal);
  }
}

void EngineDDXB::PrepareDetailTex(int spec, const TexMaterial *mat, const EngineShapeProperties &prop)
{
  if (!IsMultitexturing())
  {
    PrepareSingleTexDiffuseA(spec);
    return;
  }
  if (mat)
  {
    TexMaterial::Loaded m(mat);
    D3DMATRIX matrix0, matrix1, matrix2;
    D3DMATRIX *pMatrix0 = NULL;
    D3DMATRIX *pMatrix1 = NULL;
    D3DMATRIX *pMatrix2 = NULL;
    bool matrix0UVCompressed = true;
    bool matrix1UVCompressed = true;
    bool matrix2UVCompressed = true;
    switch (mat->_stage[1]._uvSource)
    {
    case UVTexWaterAnim:
      {
        Matrix4 m0 = MIdentity;
        Matrix4 m1 = mat->_stage[1]._uvTransform;
        
        _sceneProps->GetSeaWaveMatrices(m0,m1,prop);

        ConvertMatrix(matrix0, m0);
        ConvertMatrix(matrix1, m1);
        
        matrix0UVCompressed = false; // matrix 0 (texture) translates directly from position
        matrix1UVCompressed = false; // matrix 1 (normal map) translates directly from position
        pMatrix0 = &matrix0;
        pMatrix1 = &matrix1;
      }
      break;
    case UVPos:
    case UVNorm:
      Fail("Unsupported UV source");
    case UVTex:
      {
        Matrix4 uvMatrix0;
        Matrix4 *puvMatrix0 = mat->GetUVMatrix0(uvMatrix0);
        if (puvMatrix0)
        {
          ConvertMatrix(matrix0, *puvMatrix0);
          matrix0._41 = matrix0._31;
          matrix0._42 = matrix0._32;
          matrix0._31 = 0.0f;
          matrix0._32 = 0.0f;
          pMatrix0 = &matrix0;
        }

        ConvertMatrix(matrix1, mat->_stage[1]._uvTransform);
        matrix1._41 = matrix1._31;
        matrix1._42 = matrix1._32;
        matrix1._31 = 0.0f;
        matrix1._32 = 0.0f;
        pMatrix1 = &matrix1;
      }
      break;
    case UVNone:
      matrix1UVCompressed = false;
      break;
    }

    switch (mat->_stage[2]._uvSource)
    {
    case UVTex:
      ConvertMatrix(matrix2, mat->_stage[2]._uvTransform);
      matrix2._41 = matrix2._31;
      matrix2._42 = matrix2._32;
      matrix2._31 = 0.0f;
      matrix2._32 = 0.0f;
      pMatrix2 = &matrix2;
      break; 
    case UVTexWaterAnim:
      {
        // x,z usually from 0 to LandRange
        float mw3 = fastFmod(Glob.time.toFloat()*0.02+sin(Glob.time.toFloat()*0.2)*0.02,2);
        float mw4 = fastFmod(Glob.time.toFloat()*0.01+sin(Glob.time.toFloat()*0.1)*0.01,2);

        ConvertMatrix(matrix2, mat->_stage[2]._uvTransform);
        matrix2._41 = matrix2._11*mw3;
        matrix2._42 = matrix2._22*mw4;
        matrix2._31 = 0.0f;
        matrix2._32 = 0.0f;
        pMatrix2 = &matrix2;
      }
      break;
    case UVNone:
      ConvertMatrix(matrix2, mat->_stage[2]._uvTransform);
      matrix2._41 = matrix2._31;
      matrix2._42 = matrix2._32;
      matrix2._31 = 0.0f;
      matrix2._32 = 0.0f;
      pMatrix2 = &matrix2;
      matrix2UVCompressed = false;
      break;
    }

    
    DoEnableDetailTexGen(
      pMatrix0, pMatrix1, pMatrix2, 
      matrix0UVCompressed, matrix1UVCompressed, matrix2UVCompressed, true
    );

    SelectPixelShader(m.GetPixelShaderID(0));
    return;
  }

  PrepareSingleTexDiffuseA(spec);
}

/** get value which should be written into the stencil buffer */
int EngineDDXB::GetStencilRefValue() const
{
  return _shadowLevel;
}

void EngineDDXB::SetPostFxEnabled(int value)
{
}

void EngineDDXB::SetPostFxParam(int fx, const float *param)
{
  if (fx>=NPFX) return;
  _postFxParam[fx][0] = param[0];
  _postFxParam[fx][1] = param[1];
  _postFxParam[fx][2] = param[2];
  _postFxParam[fx][3] = param[3];
  // if params are not set, effect is not enabled
  _postFxEnabled[fx] = true;
}

void EngineDDXB::SetShadowLevelNeeded( int value )
{
  saturate(value,0,3);
  int newValue = value*STENCIL_SHADOW_LEVEL_STEP;
  if (_shadowLevelNeeded!=newValue)
  {
    _shadowLevelNeeded = newValue;
    // set impossible value - force shadow state setting
    _currentShadowStateSettings = SS_None;
  }
}

void EngineDDXB::SetShadowLevel( int value )
{
  saturate(value,0,3);
  int newValue = value*STENCIL_SHADOW_LEVEL_STEP;
  // set impossible value - force shadow state setting
  if (_shadowLevel!=newValue)
  {
    _shadowLevel = newValue;
    // set impossible value - force shadow state setting
    _currentShadowStateSettings = SS_None;
  }
}

void EngineDDXB::DoShadowsStatesSettings(EShadowStateXB ss, bool optimize, bool noBackCull)
{
  _currentShadowStateSettings = ss;
  _backCullDisabled = noBackCull;
  switch(ss)
  {
  case SS_Transparent:
    D3DSetRenderStateOpt(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP, optimize);
    D3DSetRenderStateOpt(D3DRS_STENCILWRITEMASK, STENCIL_FULL, optimize);
    if (!_recording)
    {
      D3DSetRenderStateOpt(D3DRS_STENCILREF, _shadowLevel/* | _aisValue*/, optimize);
    }
    D3DSetRenderStateOpt(D3DRS_STENCILFUNC, D3DCMP_ALWAYS, optimize);
    D3DSetRenderStateOpt(D3DRS_CULLMODE, noBackCull ? D3DCULL_NONE : D3DCULL_CCW, optimize);
    break;
  case SS_Receiver:
    // when drawing object on top of shadow, reset the shadow
    D3DSetRenderStateOpt(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE, optimize);
    D3DSetRenderStateOpt(D3DRS_STENCILWRITEMASK, STENCIL_FULL, optimize);
    if (!_recording)
    {
      D3DSetRenderStateOpt(D3DRS_STENCILREF, _shadowLevel/* | _aisValue*/, optimize);
    }
    D3DSetRenderStateOpt(D3DRS_STENCILFUNC, D3DCMP_ALWAYS, optimize);
    D3DSetRenderStateOpt(D3DRS_CULLMODE, noBackCull ? D3DCULL_NONE : D3DCULL_CCW, optimize);
    break;
  case SS_Disabled:
    D3DSetRenderStateOpt(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE, optimize);
    D3DSetRenderStateOpt(D3DRS_STENCILWRITEMASK, STENCIL_FULL, optimize);
    DoAssert(!_recording);
    D3DSetRenderStateOpt(D3DRS_STENCILREF, _shadowLevelNeeded/* | _aisValue*/, optimize);
    D3DSetRenderStateOpt(D3DRS_STENCILFUNC, D3DCMP_ALWAYS, optimize);
    D3DSetRenderStateOpt(D3DRS_CULLMODE, noBackCull ? D3DCULL_NONE : D3DCULL_CCW, optimize);
    break;
  case SS_Ordinary:
    D3DSetRenderStateOpt(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE, optimize);
    DoAssert(!_recording);
    // no comparison - we use ref value only for replace
    // set STENCIL_COUNTER to make sure shadow is visualized
    // set STENCIL_PROJ_SHADOW so that shadow can be distinguished
    D3DSetRenderStateOpt(D3DRS_STENCILREF, STENCIL_PROJ_SHADOW+1, optimize);
    D3DSetRenderStateOpt(D3DRS_STENCILWRITEMASK, STENCIL_PROJ_SHADOW|STENCIL_COUNTER, optimize);
    D3DSetRenderStateOpt(D3DRS_STENCILFUNC, D3DCMP_ALWAYS, optimize);
    D3DSetRenderStateOpt(D3DRS_CULLMODE, noBackCull ? D3DCULL_NONE : D3DCULL_CCW, optimize);
    break;
  case SS_Volume_Front:
    D3DSetRenderStateOpt(D3DRS_STENCILPASS, D3DSTENCILOP_DECR, optimize);
    D3DSetRenderStateOpt(D3DRS_STENCILMASK, STENCIL_SHADOW_LEVEL_MASK, optimize);
    DoAssert(!_recording);
    D3DSetRenderStateOpt(D3DRS_STENCILREF, _shadowLevelNeeded, optimize);
    D3DSetRenderStateOpt(D3DRS_STENCILWRITEMASK, STENCIL_COUNTER, optimize);
    // if reference<=stencilbuffer value, pass
    D3DSetRenderStateOpt(D3DRS_STENCILFUNC, D3DCMP_LESSEQUAL, optimize);
    D3DSetRenderStateOpt(D3DRS_CULLMODE, D3DCULL_CCW, optimize);
    break;
  case SS_Volume_Back:
    D3DSetRenderStateOpt(D3DRS_STENCILPASS, D3DSTENCILOP_INCR, optimize);
    D3DSetRenderStateOpt(D3DRS_STENCILMASK, STENCIL_SHADOW_LEVEL_MASK, optimize);
    DoAssert(!_recording);
    D3DSetRenderStateOpt(D3DRS_STENCILREF, _shadowLevelNeeded, optimize);
    D3DSetRenderStateOpt(D3DRS_STENCILWRITEMASK, STENCIL_COUNTER, optimize);
    D3DSetRenderStateOpt(D3DRS_STENCILFUNC, D3DCMP_LESSEQUAL, optimize);
    D3DSetRenderStateOpt(D3DRS_CULLMODE, D3DCULL_CW, optimize);
    break;
  default:
    LogF("Error: Unknown shadow state");
  }
}

void EngineDDXB::SetMultiTexturingTextureDetail(int stageIndex, TextureD3DXB *tex)
{
  TextureD3DHandle tHandle = tex->GetHandle() ? tex->GetHandle() : tex->GetDefaultTexture()->GetHandle();
  if (_lastTexture[stageIndex] == tex) return;
#if !NO_SET_TEX
  if (_recording)
  {
    DWORD pbOffset;
    _d3DDevice->GetPushBufferOffset(&pbOffset);
    _activePushBuffer->_fixupEntries.Add(new FETexture(pbOffset, stageIndex, tex, _activePushBuffer));
    tHandle = _textBank->GetWhiteTexture()->GetHandle();
  }
  _hwInDefinedState = false;
  _d3DDevice->SetTexture(stageIndex, tHandle);
  _lastHandle[stageIndex] = tHandle;
  _lastTexture[stageIndex] = tex;
#endif
#if DO_TEX_STATS
  if (LogStatesOnce)
  {
    LogF("Texture stage %d change to %s", stageIndex, (const char *)tex ? tex->Name() : "<null>");
  }
#endif
}

void EngineDDXB::SetMultiTexturing(TextureD3DXB **tex, int count)
{
  _hwInDefinedState = false;
  // Set specified textures
  for (int i = 0; i < count; i++)
  {

    // Don't do anything in case the texture was already set
    if (_lastTexture[i + 1] == tex[i]) continue;

    // In case the texture hasn't been loaded modify the maxColor and use default texture
    TextureD3DXB *loadedTexture;
    if (tex[i])
    {
      if (!tex[i]->GetHandle())
      {
        loadedTexture = tex[i]->GetDefaultTexture();
      }
      else
      {
        loadedTexture = tex[i];
      }
    }
    else
    {
      loadedTexture = NULL;
    }


#if !NO_SET_TEX
    //ADD_COUNTER(pdcST,1);

    // Get the valid handle
    TextureD3DHandle tHandle = loadedTexture ? loadedTexture->GetHandle() : NULL;

    // Create the fix-up entry. If the texture is to be NULL we don't need to fix it up
    if (_recording && (loadedTexture != NULL))
    {
      DWORD pbOffset;
      _d3DDevice->GetPushBufferOffset(&pbOffset);
      _activePushBuffer->_fixupEntries.Add(new FETexture(pbOffset, i + 1, tex[i], _activePushBuffer));
      tHandle = _textBank->GetWhiteTexture()->GetHandle();
    }

    // Set the valid texture
    _d3DDevice->SetTexture(i + 1, tHandle);
    _lastHandle[i+1] = tHandle;
    _lastTexture[i + 1] = tex[i];
#endif
#if DO_TEX_STATS
    if (LogStatesOnce)
    {
      LogF("Texture stage %d change to %s", i + 1, (const char *)tex[i] ? tex[i]->Name() : "<null>");
    }
#endif

    // Remember the original texture
  }

  // Set the rest to NULL
  for (int i = count; i < TEXTURE_STAGES - 1; i++)
  {
    if (_lastTexture[i + 1].IsNull()) continue;
#if !NO_SET_TEX
    //ADD_COUNTER(pdcST,1);
    if (!_recording)
    {
      /// if there is some texture currently set, we do not mind
      /// if the texture is set but unused, nothign wrong will happen
      _d3DDevice->SetTexture(i + 1, NULL);
      _lastHandle[i+1] = NULL;
      _lastTexture[i + 1].Free();
    }
#endif
#if DO_TEX_STATS
    if (LogStatesOnce)
    {
      LogF("Texture stage %d change to <null>", i + 1);
    }
#endif
  }
}

Vector3 EngineDDXB::TransformAndNormalizePositionToObjectSpace(Vector3Par position)
{
  // Get the original distance
  float positionToModelDistance = position.Distance(_world.Position());

  // Transform the position
  Vector3 newPosition = _invWorld * position;

  // Normalize the position
  if (newPosition.SquareSize()>0)
  {
    float coef = positionToModelDistance * newPosition.InvSize();
    newPosition = newPosition * coef;
  }

  return newPosition;
}

void EngineDDXB::EnablePointSampling(int filter, bool optimize)
{
  switch (filter)
  {
  case FilterAnizotrop:
    D3DSetTextureStageStateOpt(0,D3DTSS_MINFILTER,D3DTEXF_ANISOTROPIC,optimize);
    D3DSetTextureStageStateOpt(0,D3DTSS_MAGFILTER,D3DTEXF_ANISOTROPIC,optimize); 
    D3DSetTextureStageStateOpt(0,D3DTSS_MIPFILTER,D3DTEXF_LINEAR,optimize);
    break;
  case FilterTrilinear:
    D3DSetTextureStageStateOpt(0,D3DTSS_MINFILTER,D3DTEXF_LINEAR,optimize);
    D3DSetTextureStageStateOpt(0,D3DTSS_MAGFILTER,D3DTEXF_LINEAR,optimize); 
    D3DSetTextureStageStateOpt(0,D3DTSS_MIPFILTER,D3DTEXF_LINEAR,optimize);
    break;
  case FilterLinear:
    D3DSetTextureStageStateOpt(0,D3DTSS_MINFILTER,D3DTEXF_LINEAR,optimize);
    D3DSetTextureStageStateOpt(0,D3DTSS_MAGFILTER,D3DTEXF_LINEAR,optimize); 
    D3DSetTextureStageStateOpt(0,D3DTSS_MIPFILTER,D3DTEXF_POINT,optimize);
    break;
    //case FilterPoint:
  default:
    D3DSetTextureStageStateOpt(0,D3DTSS_MINFILTER,D3DTEXF_POINT,optimize);
    D3DSetTextureStageStateOpt(0,D3DTSS_MAGFILTER,D3DTEXF_POINT,optimize);  
    D3DSetTextureStageStateOpt(0,D3DTSS_MIPFILTER,D3DTEXF_POINT,optimize);
    break;
  }
}

static float ColorDistance2(ColorVal c1, ColorVal c2)
{
  return Square(c1.R()-c2.R())+Square(c1.G()-c2.G())+Square(c1.B()-c2.B())+Square(c1.A()-c2.A());
}

void EngineDDXB::SetTexture0Color(ColorVal maxColor, ColorVal avgColor, bool optimize)
{
  _texture0MaxColor = maxColor;
  _texture0AvgColor = avgColor;
}

void EngineDDXB::SetHWToDefinedState()
{
  SwitchTL(true);

  // Call the prepareTL

  // Defined texture
  TextureD3DXB *tex = _textBank->GetWhiteTexture();
  MipInfo mip = _textBank->UseMipmap(tex, 0, 0);

  // Defined material
  _definedMat->SetStageTexture(1, tex);
  _definedMat->SetStageTexture(2, tex);
  _definedMat->SetStageTexture(3, tex);

  // Set defined texture scale and offset
  UVPair scale;
  scale.u = 1.0f;
  scale.v = 1.0f;
  UVPair offset;
  offset.u = 0.0f;
  offset.v = 0.0f;
  SetTexGenScaleAndOffset(scale, offset);

  // Defined lights
  LightList emptyLights;

  EngineShapeProperties prop;
  // The calling itself
  GEngine->PrepareTriangleTL(mip, TexMaterialLODInfo(_definedMat, 0), 0, _definedMatMod, emptyLights, 0, prop);

  // Setup constants like lights
  SetupNonFixedConstants();

  // Select the pixel shader
  DoSelectPixelShader();

  // Release the vertex and index buffer

  // Defined vertex buffer
  //_vBufferLast = NULL;
  //_vBufferSize = 0;
  //_d3DDevice->SetVertexShaderInputDirect(NULL, 0, NULL);

  // Defined index buffer
  _iBufferLast = NULL;
  _vOffsetLast = 0;
  _vsdVersionLast = VSDV_Basic;
  _d3DDevice->SetIndices(NULL, 0);

  // Set flag to determine we are in defined state
  _hwInDefinedState = true;
}

void EngineDDXB::SetTexture0(TextureD3DXB *tex)
{
  // Use white texture in case the input is NULL
  if (!tex)
  {
    tex = _textBank->GetWhiteTexture();
    DoAssert(tex);
  }

  // Don't do anything in case the texture was already set
  if (_lastTexture[0] != tex)
  {
    // Get max color
    Color maxColor = tex->GetMaxColor();

    // In case the texture hasn't been loaded the maxColor is the avg color (in max color space) and use default texture
    TextureD3DXB *loadedTexture;
    if (!tex->GetHandle())
    {
      maxColor = tex->GetColor();
      loadedTexture = tex->GetDefaultTexture();
    }
    else
    {
      loadedTexture = tex;
    }

    // Set the PS constants
    SetTexture0Color(maxColor, CombineAvgAndMaxColor(loadedTexture->GetColor(), maxColor), true);

    // Statistics
#if DO_TEX_STATS
    if (EnableTexStats) TexStats.Count(tex ? tex->Name() : "<NULL>");
#endif
#if LOG_STATE_LEVEL
    if (LogStatesOnce)
    {
      LogF("Switch %s",tex ? tex->Name() : "<NULL>");
    }
#endif
#if LOG_STATES
    LogF("Flush: texture change to %s",tex ? tex->Name() : "<NULL>");
#endif
    //LogTexture("Tri",tex);
#if !NO_SET_TEX

    // Get the valid handle
    TextureD3DHandle tHandle = loadedTexture->GetHandle();

    // Create fix-up entry in case we are recording
    if (_recording)
    {
      DWORD pbOffset;
      _d3DDevice->GetPushBufferOffset(&pbOffset);
      _activePushBuffer->_fixupEntries.Add(new FETexture(pbOffset, 0, tex, _activePushBuffer));
      tHandle = _textBank->GetWhiteTexture()->GetHandle();
    }

    DoAssert(tHandle==NULL || tHandle>(TextureD3DHandle)0x1000);

    // Set the valid texture
    _hwInDefinedState = false;
    HRESULT err=_d3DDevice->SetTexture(0, tHandle);
    _lastHandle[0] = tHandle;
    // Remember the original texture
    _lastTexture[0] = tex;
    ADD_COUNTER(tChng,1);
    if( err ) DDErrorXB("Cannot set texture",err);

#endif

  }
}

void EngineDDXB::SetTexture(TextureD3DXB *tex, const TexMaterial *mat, int specFlags)
{
  if (!tex && mat)
  {
    tex = static_cast<TextureD3DXB*>(mat->_stage[0]._tex.GetRef());
  }
  SetTexture0(tex);

  if (!mat)
  {
    SetMultiTexturing(NULL, 0);
  }
  else
  {
    Assert(TEXTURE_STAGES<=TexMaterial::NStages);
    TextureD3DXB *secTex[TEXTURE_STAGES-1];
    mat->Load();
    for (int i = 1; i < TEXTURE_STAGES; i++)
    {
      secTex[i-1] = static_cast<TextureD3DXB*>(mat->_stage[i]._tex.GetRef());
      
      int magFilter = mat->_stage[i]._filter>=TFLinear ? D3DTEXF_LINEAR : D3DTEXF_POINT;
      //int mipFilter = mat->_stage[i]._filter>=TFTrilinear ? D3DTEXF_LINEAR : D3DTEXF_POINT;
      D3DSetTextureStageState(i, D3DTSS_MINFILTER, magFilter);
      D3DSetTextureStageState(i, D3DTSS_MAGFILTER, magFilter);
      //D3DSetTextureStageState(i, D3DTSS_MIPFILTER, mipFilter);
    }
    SetMultiTexturing(secTex, TEXTURE_STAGES-1);

    Texture *tex1 = mat->_stage[1]._tex;
    if (tex1)
    {
      int clamp = tex1->GetClamp();
      D3DTEXTUREADDRESS addressU=( clamp&TexClampU ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP );    
      D3DTEXTUREADDRESS addressV=( clamp&TexClampV ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP );
      
      D3DSetTextureStageState(1,D3DTSS_ADDRESSU,addressU);
      D3DSetTextureStageState(1,D3DTSS_ADDRESSV,addressV);
    }
    Texture *tex2 = mat->_stage[2]._tex;
    if (tex2)
    {
      int clamp = tex2->GetClamp();
      D3DTEXTUREADDRESS addressU=( clamp&TexClampU ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP );    
      D3DTEXTUREADDRESS addressV=( clamp&TexClampV ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP );
      
      D3DSetTextureStageState(2,D3DTSS_ADDRESSU,addressU);
      D3DSetTextureStageState(2,D3DTSS_ADDRESSV,addressV);
    }
  }
}

void EngineDDXB::SetTextureDetail(int stageIndex, const TexMaterial *mat)
{
  Assert(mat != NULL);

  // Load material
  mat->Load();

  // Get the engine texture
  TextureD3DXB *secTex = static_cast<TextureD3DXB*>(mat->_stage[stageIndex]._tex.GetRef());

  // Set texture
  SetMultiTexturingTextureDetail(stageIndex, secTex);
}

#if RS_DIAGS
HRESULT EngineDDXB::D3DSetRenderStateName
(
 D3DRENDERSTATETYPE state, DWORD value, const char *name, bool optimize
 )
#else
HRESULT EngineDDXB::D3DSetRenderState
(
 D3DRENDERSTATETYPE state, DWORD value, bool optimize
 )
#endif
{
#if DO_COUNTERS
  static OptimizeCounter opt("RenderState");
#endif
  // assume small values of state (in DX6 max. state was about 40)
  _renderState.Access(state);
  RenderStateInfo &info=_renderState[state];

  if( info.value==value && optimize )
  {
#if DO_COUNTERS
    opt.Skip();
#endif
    return D3D_OK;
  }

  info.value=value;
#if DO_COUNTERS
  opt.Perform();
#endif
#if LOG_STATE_LEVEL
  if (LogStatesOnce)
  {
#if RS_DIAGS
    LogF("SetRenderState %s,%d",name,value);
#else
    LogF("SetRenderState %d,%d",state,value);
#endif
  }
#endif
#if !NO_SET_RS
  PROFILE_DX_SCOPE(3sRS);

  //PerfCounter
  /*
  if ((state >= 0) && (state < 256))
  {
  if (_perf[state] == NULL)
  {
  char name[256];
  sprintf(name, "renderState %d", state);
  _perf[state] = new PerfCounter(name, &GPerfProfilers);
  }
  (*_perf[state]) += 1;
  }
  */

  _hwInDefinedState = false;
  HRESULT err=_d3DDevice->SetRenderState(state,value);
  //HRESULT err=D3D_OK;
  DX_CHECKN("SetRS",err);
  return err;
#else
  return 0;
#endif
}

/*!
\patch 2.01 Date 12/16/2002 by Ondra
- Fixed: Water detail texture moving again.
*/

void EngineDDXB::DoPrepareTriangle(
  TextureD3DXB *tex, const TexMaterial *mat, int level, int spec, const EngineShapeProperties &prop
)
{
  int clamp = ((spec>>ClampLog)&ClampMask);
  if( _lastClamp!=clamp )
  {
    D3DTEXTUREADDRESS addressU=( clamp&TexClampU ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP );    
    D3DTEXTUREADDRESS addressV=( clamp&TexClampV ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP );
    
    D3DSetTextureStageState(0,D3DTSS_ADDRESSU,addressU);
    D3DSetTextureStageState(0,D3DTSS_ADDRESSV,addressV);
    
    _lastClamp = clamp;
  }

  int specFlags=spec&(
    NoZBuf|NoZWrite|NoAlphaWrite|NoStencilWrite|
    IsAlphaFog|
    IsShadow|IsShadowVolume|ShadowVolumeFrontFaces|
    IsAlpha|IsTransparent|
    DstBlendZero|DstBlendOne|FilterMask|IsHidden|
    NoColorWrite
  );

  if (
    _lastSpec!=specFlags || _lastMat!=mat || _texGenScaleOrOffsetHasChanged ||
    _currentShadowStateSettings==SS_None || prop.IsDynamic()
  )
  {
    // Remember values for future lazy set
    _lastSpec = specFlags;
    _lastMat = mat;

    // Set the always in shadow flag
//    int newAISValue = (mat && mat->GetRenderFlag(RFAlwaysInShadow)) ? 0x40 : 0;
//    if (_aisValue != newAISValue)
//    {
//      _aisValue = newAISValue;
//      _currentShadowStateSettings=SS_None;
//    }

    // Move flag from material to specFlags
    if (mat)
    {
      TexMaterial::Loaded m(mat);
      if (m.GetRenderFlag(RFNoZWrite)) specFlags |= NoZWrite;
      if (m.GetRenderFlag(RFNoColorWrite)) specFlags |= NoColorWrite;
      if (m.GetRenderFlag(RFNoAlphaWrite)) specFlags |= NoAlphaWrite;
    }
    
    EnablePointSampling(specFlags&FilterMask,true);
    if( specFlags&IsShadow )
    {
      D3DSetRenderState(D3DRS_COLORWRITEENABLE, 0);
      D3DSetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);

      if (specFlags&IsShadowVolume)
      {
        // Shadow volume
        D3DSetRenderState(D3DRS_MULTISAMPLEMODE, D3DMULTISAMPLEMODE_4X);
        D3DSetRenderState(D3DRS_OCCLUSIONCULLENABLE, FALSE);
        //D3DSetRenderState(D3DRS_MULTISAMPLEMODE, D3DMULTISAMPLEMODE_1X);
        D3DSetRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
        D3DSetRenderState(D3DRS_ZWRITEENABLE, FALSE);
      }
      else
      {
        D3DSetRenderState(D3DRS_MULTISAMPLEMODE, D3DMULTISAMPLEMODE_1X);
        D3DSetRenderState(D3DRS_OCCLUSIONCULLENABLE, TRUE);
        // Ordinary shadow
        D3DSetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
        if (!_hasStencilBuffer)
        {
          D3DSetRenderState(D3DRS_ZWRITEENABLE,TRUE);
        }
        else
        {
          D3DSetRenderState(D3DRS_ZWRITEENABLE,FALSE);
        }
      }
    }
    else
    {
      if (specFlags&IsHidden)
      {
        D3DSetRenderState(D3DRS_COLORWRITEENABLE, 0);
      }
      else if (specFlags&(NoZWrite|NoAlphaWrite))
      {
        if (specFlags&NoColorWrite)
        {
          D3DSetRenderState(D3DRS_COLORWRITEENABLE, 0);
        }
        else
        {
          D3DSetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
        }
      }
      else if (specFlags&NoColorWrite)
      {
        D3DSetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA);
      }
      else
      {
        D3DSetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
      }
      if( specFlags&NoZBuf )
      {
        D3DSetRenderState(D3DRS_MULTISAMPLEMODE, D3DMULTISAMPLEMODE_1X);
        D3DSetRenderState(D3DRS_OCCLUSIONCULLENABLE, TRUE);
        D3DSetRenderState(D3DRS_ZFUNC,D3DCMP_ALWAYS);
        D3DSetRenderState(D3DRS_ZWRITEENABLE,FALSE);
        D3DSetRenderState(D3DRS_ZENABLE,D3DZB_FALSE);
      }
      else if( specFlags&NoZWrite )
      {
        D3DSetRenderState(D3DRS_MULTISAMPLEMODE, D3DMULTISAMPLEMODE_1X);
        D3DSetRenderState(D3DRS_OCCLUSIONCULLENABLE, TRUE);
        // road
        //D3DSetRenderState(D3DRS_ZFUNC,D3DCMP_GREATEREQUAL);
        D3DSetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
        D3DSetRenderState(D3DRS_ZWRITEENABLE,FALSE);
        D3DSetRenderState(D3DRS_ZENABLE,D3DZB_TRUE);
      }
      else
      {
        D3DSetRenderState(D3DRS_MULTISAMPLEMODE, D3DMULTISAMPLEMODE_1X);
        D3DSetRenderState(D3DRS_OCCLUSIONCULLENABLE, TRUE);
        D3DSetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
        //D3DSetRenderState(D3DRS_ZFUNC,D3DCMP_GREATEREQUAL);
        D3DSetRenderState(D3DRS_ZWRITEENABLE,TRUE);
        D3DSetRenderState(D3DRS_ZENABLE,D3DZB_TRUE);
      }
    }

    if( specFlags&IsShadow )
    {

      // Select a proper pixel shader
      if (mat && _tlActive)
      {
        // secondary texture used in the material
        // this is possible only with HW T&L
        //DoAssert(_tlActive);
        PrepareDetailTex(spec,mat,prop);
      }
      else
      {
        // single texturing
        //PrepareSingleTexDiffuseA();
        DoEnableDetailTexGen(NULL, NULL, NULL,true, true, true, true);
        SelectPixelShader(PSWhiteAlpha);
      }

      D3DSetRenderState(D3DRS_SRCBLEND,D3DBLEND_ZERO);
      D3DSetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
      D3DSetRenderState(D3DRS_FOGENABLE,FALSE);
      int shadowI = toInt(_shadowFactor.A()*(255.0*7/16));
      saturate(shadowI,0,255);
      D3DSetRenderState(D3DRS_ALPHAREF,shadowI);
      D3DSetRenderState(D3DRS_ALPHATESTENABLE,TRUE);
      D3DSetTextureStageState(0,D3DTSS_ALPHAKILL,D3DTALPHAKILL_ENABLE);
      D3DSetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
      if (specFlags&IsShadowVolume)
      {
        if (specFlags&ShadowVolumeFrontFaces)
        {
          ShadowsStatesSettings(SS_Volume_Front,false);
        }
        else
        {
          ShadowsStatesSettings(SS_Volume_Back,false);
        }
      }
      else
      {
        ShadowsStatesSettings(SS_Ordinary,false);
      }
    }
    else
    {
      if (mat && _tlActive)
      {
        // secondary texture used in the material
        // this is possible only with HW T&L
        PrepareDetailTex(spec,mat,prop);
      }
      else
      {
        // single texturing
        PrepareSingleTexDiffuseA(spec);
      }

#if !NO_FOG
      if (mat)
      {
        if (mat->GetFogMode() == FM_Fog)
        {
          D3DSetRenderState(D3DRS_FOGENABLE,TRUE);
        }
        else
        {
          D3DSetRenderState(D3DRS_FOGENABLE,FALSE);
        }
      }
      else
      {
        if (specFlags&IsAlphaFog)
        {
          D3DSetRenderState(D3DRS_FOGENABLE,FALSE);
        }
        else
        {
          D3DSetRenderState(D3DRS_FOGENABLE,TRUE);
        }
      }
#endif

      if( specFlags&IsAlpha )
      {
        D3DSetRenderState(D3DRS_ALPHAREF,1);
        D3DSetTextureStageState(0,D3DTSS_ALPHAKILL,D3DTALPHAKILL_ENABLE);
        D3DSetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
        D3DSetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
        switch (specFlags&(DstBlendOne|DstBlendZero))
        {
        default:
        case 0:
          D3DSetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
          D3DSetRenderState(D3DRS_ALPHATESTENABLE,TRUE);
          break;
        case DstBlendOne:
          D3DSetRenderState(D3DRS_DESTBLEND,D3DBLEND_ONE);
          D3DSetRenderState(D3DRS_ALPHATESTENABLE,TRUE);
          break;
        case DstBlendZero:
          D3DSetRenderState(D3DRS_DESTBLEND,D3DBLEND_ZERO);
          D3DSetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
          break;
        }
      }
      else
      {
        // check if alpha testing is required
        D3DSetRenderState(D3DRS_ALPHAREF, 0x7F);
        if (specFlags&IsTransparent)
        {
          D3DSetRenderState(D3DRS_ALPHATESTENABLE,TRUE);
          D3DSetTextureStageState(0,D3DTSS_ALPHAKILL,D3DTALPHAKILL_ENABLE);
        }
        else
        {
          D3DSetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
          D3DSetTextureStageState(0,D3DTSS_ALPHAKILL,D3DTALPHAKILL_DISABLE);
        }
        D3DSetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
        D3DSetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
        D3DSetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
      }

      // IsHidden - we disable back face culling for visibility testing - we want whole cube to be tested
      ShadowsStatesSettings(
        specFlags&(NoZWrite|NoZBuf|NoAlphaWrite|NoStencilWrite) ? SS_Transparent : SS_Receiver,
        (specFlags&(NoBackfaceCull|IsShadowVolume))==NoBackfaceCull
      );
    }
  }

#if 1
  LogTexture("Tri",tex);
  SetTexture(tex,mat,spec);
  //HRESULT err=D3D_OK;
  //if( err ) DDErrorXB("Cannot set texture",err);
#endif

}

void EngineDDXB::DoPrepareTriangle( const MipInfo &absMip, const TexMaterial *mat, int specFlags, const EngineShapeProperties &prop )
{
  TextureD3DXB *tex = static_cast<TextureD3DXB *>(absMip._texture);
  int level = absMip._level;
  // allocate some queue
  DoPrepareTriangle(tex,mat,level,specFlags,prop);
}

void EngineDDXB::QueuePrepareTriangle( const MipInfo &absMip, const TexMaterial *mat, int specFlags )
{
  TextureD3DXB *tex = static_cast<TextureD3DXB *>(absMip._texture);
  int level = absMip._level;
  _queueNo._actTri = AllocateQueue(_queueNo,tex,level,mat,specFlags);
  Assert (_queueNo._triUsed[_queueNo._actTri]);
}

void EngineDDXB::PrepareTriangle
(
 const MipInfo &absMip, const TexMaterial *mat, int specFlags0
 )
{
  TextureD3DXB *tex = static_cast<TextureD3DXB *>(absMip._texture);
  SwitchRenderMode(RMTris);
  // allocate some queue
  SwitchTL(false);
  int level = absMip._level;
  _queueNo._actTri = AllocateQueue(_queueNo,tex,level,mat,specFlags0);
  Assert (_queueNo._triUsed[_queueNo._actTri]);
  _prepSpec = specFlags0;

}

void EngineDDXB::PrepareTriangleTL(
  const MipInfo &mip, const TexMaterialLODInfo &mat, int specFlags,
  const TLMaterial &tlMat, const LightList &lights,
  int spec, const EngineShapeProperties &prop
)
{
  Assert(_tlActive);

  bool forceSpecular = false;
  if (mat._mat)
  {
    TexMaterial::Loaded m(mat._mat);
    // Set the material shader
    _vertexShaderID = m.GetVertexShaderID(0);

    if ((m.GetMainLight() == ML_Sun) || (m.GetMainLight() == ML_None))
    {
      EnableSunLight(((spec&(DisableSun|SunPrecalculated))==0)?ML_Sun:ML_None);
    }
    else
    {
      EnableSunLight(m.GetMainLight());
    }

    // Set the material fog
    SetFogMode(m.GetFogMode());
    
    _landShadowEnabled = m.GetRenderFlag(RFLandShadow);
    _alphaInstancing = m.GetRenderFlag(RFAlphaInstancing);
    switch (m.GetPixelShaderID(0))
    {
      case PSNormalMap:
      case PSNormalMapThrough:
      case PSNormalMapDiffuse:
      case PSWater:
        forceSpecular = true;
        break;
    }
  }
  else
  {
    // Set the default shader
    _vertexShaderID = VSBasic;

    // Enable sunlight based on flags and the TexMaterial
    EnableSunLight(((spec&(DisableSun|SunPrecalculated))==0)?ML_Sun:ML_None);

    // Fog is usual without material
    SetFogMode(specFlags&IsAlphaFog ? FM_Alpha : FM_Fog);
    _landShadowEnabled = false;
    _alphaInstancing = false;
  }

  TextureD3DXB *tex = static_cast<TextureD3DXB *>(mip._texture);
  int level = mip._level;
  DoPrepareTriangle(tex,mat._mat,level,specFlags,prop);

  // make color information from the texture
  // to be a part of the material
  TLMaterial matMod = tlMat;
  //Color texColor(0.5f,0.5f,0.5f,1);
  /*
  if (tex)
  {
  // get texture color normalization factor
  if (tex->GetHandle()==NULL)
  {
  // get average texture color
  texColor = tex->GetColor();
  }
  }
  */
  DoAssert(_modColor.A() == 1);
  DoAssert(_modColor.G() == _modColor.R());
  DoAssert(_modColor.B() == _modColor.R());
  if (_modColor.R() == 1)
  {
  }
  else if (_modColor.R() == GetHDRFactor())
  {
  }
  else
  {
    Fail("Warning: _modColor contains a strange value");
  }
  matMod.ambient = matMod.ambient*_modColor;
  matMod.diffuse = matMod.diffuse*_modColor;
  matMod.forcedDiffuse = matMod.forcedDiffuse*_modColor;
  matMod.emmisive = matMod.emmisive*_modColor;
  if (UsingPixelShaders())
  {
    matMod.specular = matMod.specular*_modColor;
  }
  else
  {
    matMod.specular = matMod.specular;
  }

  //bool tlMatChanged = true;
  spec &= (DisableSun|SunPrecalculated);
  if (matMod==_materialSet && _materialSetSpec==spec && !_isWorldMatrixChanged && !_lightsHasChanged)
  {
  }
  else
  {
    DoSetMaterial(matMod,lights,spec,forceSpecular);
  }
}

/*!
\patch 1.33 Date 11/28/2001 by Ondra
- Fixed: D3D rendering queue bug that could cause polygons being dropped.
*/

void EngineDDXB::DrawDecal
(
 Vector3Par screen, float rhw, float sizeX, float sizeY, PackedColor color,
 const MipInfo &mip, int specFlags
 )
{

  float vx=screen.X();
  float vy=screen.Y();
  float z=screen.Z();

  float oow=rhw;

  // perform simple clipping
  float xBeg=vx-sizeX;
  float xEnd=vx+sizeX;
  float yBeg=vy-sizeY;
  float yEnd=vy+sizeY;
  float uBeg=0;
  float vBeg=0;
  float uEnd=1;
  float vEnd=1;

  if( xBeg<0 )
  {
    // -xBeg is out, side length is 2*sizeX
    uBeg=-xBeg/(2*sizeX);
    xBeg=0;
  }
  if( xEnd>_w )
  {
    // xEnd-_w is out, side length is 2*sizeX
    uEnd=1-(xEnd-_w)/(2*sizeX);
    xEnd=_w;
  }
  if( yBeg<0 )
  {
    // -yBeg is out, side length is 2*sizeY
    vBeg=-yBeg/(2*sizeY);
    yBeg=0;
  }
  if( yEnd>_h )
  {
    // yEnd-_h is out, side length is 2*sizeY
    vEnd=1-(yEnd-_h)/(2*sizeY);
    yEnd=_h;
  }

  if( xBeg>=xEnd || yBeg>=yEnd ) return;

  TLVertex v[4];

  // set vertex 0
  v[0].pos[0]=xBeg;
  v[0].pos[1]=yBeg;
  v[0].pos[2]=z;
  v[0].t0.u=uBeg;
  v[0].t0.v=vBeg;
  // set vertex 1
  v[1].pos[0]=xEnd;
  v[1].pos[1]=yBeg;
  v[1].pos[2]=z;
  v[1].t0.u=uEnd;
  v[1].t0.v=vBeg;
  // set vertex 2
  v[2].pos[0]=xEnd;
  v[2].pos[1]=yEnd;
  v[2].pos[2]=z;
  v[2].t0.u=uEnd;
  v[2].t0.v=vEnd;
  // set vertex 3
  v[3].pos[0]=xBeg;
  v[3].pos[1]=yEnd;
  v[3].pos[2]=z;
  v[3].t0.u=uBeg;
  v[3].t0.v=vEnd;


  // color scaled down - glow will scale it up again
  Color c = Color(color)*_modColor;
  color = PackedColor(c);
  
  if( specFlags&IsAlphaFog )
  {
    v[0].color=color;
    v[0].specular=PackedColor(0xff000000);
  }
  else
  {
    // use fog with z
    v[0].specular=PackedColor(0xff000000-(color&0xff000000));
    v[0].color=PackedColor(color|0xff000000);
  }

  int i;
  for( i=0; i<4; i++ )
  {
    v[i].rhw=oow;
    v[i].pos[2]=z;
    v[i].color=v[0].color;
    v[i].specular=v[0].specular;
  }

  SwitchRenderMode(RMTris);
  SwitchTL(false);

  AddVertices(v,4);

  // allocate some queue
  QueuePrepareTriangle(mip,NULL,specFlags);
  static const VertexIndex indices[4] = {0,1,2,3};
  // add vertices to vertex buffer
  // check active queue
  QueueFan(indices,4);
}

void EngineDDXB::DrawPolygon( const VertexIndex *ii, int n )
{
  QueueFan(ii,n);
}

void EngineDDXB::DrawSection
(
 const FaceArray &face, Offset beg, Offset end
 )
{
  if (_resetNeeded) return;
  for( Offset i=beg; i<end; face.Next(i) )
  {
    const Poly &f=face[i];
    QueueFan(f.GetVertexList(),f.N());
  }
}

static int SpliceShaders(
                         AutoArray<DWORD> &tokenArray, const DWORD **fragments, int fragmentsCount, bool optimize
                         )
{
  HRESULT hr;
  DWORD tokenArraySizeInBytes = tokenArray.Size() * sizeof(DWORD);
  if (SUCCEEDED(hr = XGSpliceVertexShaders(tokenArray.Data(), &tokenArraySizeInBytes, NULL, fragments, fragmentsCount, optimize)))
  {
    Assert(tokenArraySizeInBytes%sizeof(DWORD)==0);
    int dwordCount = (tokenArraySizeInBytes+sizeof(DWORD)-1)/ sizeof(DWORD);
    tokenArray.Realloc(dwordCount);
    tokenArray.Resize(dwordCount);
    tokenArraySizeInBytes = dwordCount*sizeof(DWORD);
    DWORD icInSplicedShader;
    if (FAILED(hr = XGSpliceVertexShaders(tokenArray.Data(), &tokenArraySizeInBytes, &icInSplicedShader, fragments, fragmentsCount, optimize)))
    {
      DDErrorXB("Failed to splice shaders", hr);
      RptF("Failed to splice shaders");
      return 0;
    }
    DoAssert(tokenArraySizeInBytes<=tokenArray.Size() * sizeof(DWORD));
    return icInSplicedShader;
  }
  RptF("Failed to splice shaders");
  return 0;
}

#if _DEBUG
// for shader debugging/development it is often good not to use VS cache
static bool UseVSCache = false;
#elif _PROFILE
static bool UseVSCache = true;
#else
static bool UseVSCache = true;
#endif

static bool EnableLights = true;

void EngineDDXB::InitVertexShaderCache()
{
  _shaderCacheLight.Realloc(_shaderCacheLightId.Size());
  _shaderCache.Realloc(_shaderCacheId.Size());
  // Fill out cache with lights
  for (int i = 0; i < _shaderCacheLightId.Size(); i++)
  {
    // Copy the beginning (omit the last fragment)
    const DWORD *fragments[16];
    // last fragment is _shaderWriteLights
    int fragmentsCount = _shaderCacheLightId[i]._fragmentsCount - 1;
    for (int j = 0; j < fragmentsCount; j++)
    {
      fragments[j] = _shaderCacheLightId[i]._fragments[j];
    }
    //DoAssert(_shaderCacheLightId[i]._fragments[fragmentsCount]==_shaderWriteLights);

    // Go through all the possible light combinations and create the shader
    for (int j = 0; j < SC_LIGHTCOMBINATIONS; j++)
    {
      // Get the fragment list
      int finalFragmentsCount = fragmentsCount;
      int items = log2Floor(j + 1);
      int lightMask = j - ((1 << items) - 1);
      for (int k = 0; k < items; k++)
      {
        if ((lightMask >> (items - k - 1)) & 1)
        {
          fragments[finalFragmentsCount++] = _shaderSpot[k];
        }
        else
        {
          fragments[finalFragmentsCount++] = _shaderPoint[k];
        }
      }
      fragments[finalFragmentsCount++] = _shaderCacheLightId[i]._fragments[fragmentsCount];

      // Create the shader and save it
      int instructions = SpliceShaders(
        _shaderCacheLight[i][j]._shader, fragments, finalFragmentsCount,
        true
        );
      if (instructions>MAX_INSTRUCTION_COUNT)
      {
        // the item can never be accessed - the shader is invalid
        _shaderCacheLight[i][j]._shader.Clear();
      }
      else
      {
        _shaderCacheLRU._fmcl.Add(&_shaderCacheLight[i][j]);
      }
    }
  }

  // Fill out basic cache
  for (int i = 0; i < _shaderCacheId.Size(); i++)
  {
    // Create the shader and save it
    int instructions = SpliceShaders(
      _shaderCache[i]._shader,
      _shaderCacheId[i]._fragments, _shaderCacheId[i]._fragmentsCount,
      true
      );
    DoAssert(instructions<=MAX_INSTRUCTION_COUNT);
    _shaderCacheLRU._fmcl.Add(&_shaderCache[i]);
  }
}

#define GET_FRAGMENT_INSTRUCTION_COUNT(handle) ((*handle) >> 16)

#define ADD_FRAGMENT(fragmentName) \
  fragments[fragmentsCount++] = fragmentName; \
  instructionCount += GET_FRAGMENT_INSTRUCTION_COUNT(fragmentName);

void EngineDDXB::DoSetupSectionTL(
  EVertexDecl decl, bool instancing, ESkinningType skinningType,
  VertexShaderID vsID, EMainLight mainLight, EFogMode fogMode,
  bool alphaShadowEnabled, bool landShadowEnabled, bool alphaInstancing,
  int triangleCount
)
{
  ADD_COUNTER(sScTL,1);
  const DWORD *fragments[16];
  int fragmentsCount = 0;
  int instructionCount = 0;

  // Get fragments to splice
  if (skinningType == STNone)
  {
    if (vsID == VSWater)
    {
      ADD_FRAGMENT(_shaderWater);
      // water configuration is fixed
      DoAssert(mainLight==ML_Sun);
      DoAssert(fogMode==FM_Fog);
      static const float valPI_0p5_PI2_0p25[]={
        H_PI,0.5,H_PI*2,0.25
      };
      static const float valSIN_TAYLOR_3579[]={
        -1.0/(3*2),1.0/(5*4*3*2),-1.0/(7*6*5*4*3*2),+1.0/(9*8*7*6*5*4*3*2)
      };
      static const float valCOS_TAYLOR_2468[]={
        -1.0/2,+1.0/(4*3*2),-1.0/(6*5*4*3*2),+1.0/(8*7*6*5*4*3*2)
      };

      _d3DDevice->SetVertexShaderConstant(WATER_PI_0p5_PI2_0p25,valPI_0p5_PI2_0p25,1);
      _d3DDevice->SetVertexShaderConstant(WATER_SIN_TAYLOR_3579,valSIN_TAYLOR_3579,1);
      _d3DDevice->SetVertexShaderConstant(WATER_COS_TAYLOR_2468,valCOS_TAYLOR_2468,1);

      float waveXScale,waveZScale,waveHeight;
      _sceneProps->GetSeaWavePars(waveXScale,waveZScale,waveHeight);
      const float valWAVE_HEIGHT_X_Y_XDER_YDER[]=
      {
        waveHeight,waveHeight,
        waveXScale*(2*H_PI)*waveHeight,waveZScale*(2*H_PI)*waveHeight
      };

      _d3DDevice->SetVertexShaderConstant(WATER_WAVE_HEIGHT_X_Y_XDER_YDER,valWAVE_HEIGHT_X_Y_XDER_YDER,1);

      // given minimal and maximal reflection angle cos, derive add and mul coefs
      const float minCosA = 0.17f; // cos 80 deg - full reflecion, no transparency
      const float maxCosA = 0.86f; // cos 30 deg - maximum transparency
      // x1 = x0 *M + A
      // x0 = minCosA -> x1 = 0
      // x0 = maxCosA -> x1 = 1
      // M = 1/(maxCosA-minCosA), A = -minCosA/(maxCosA-minCosA)
      static const float valWATER_REFLECT_ADD_MUL_u_u[]={
        -minCosA/(maxCosA-minCosA),1/(maxCosA-minCosA),0,0
      };
      _d3DDevice->SetVertexShaderConstant(WATER_REFLECT_ADD_MUL_u_u,valWATER_REFLECT_ADD_MUL_u_u,1);

      const float valWAVEGEN[]=
      {
        // wave phase offset  - due to looping, when multiplied by 8, must give integer
        2.0/8,3.0/8,
        // wave offset amplitude - arbitrary
        0.2,0.12
      };
      _d3DDevice->SetVertexShaderConstant(WATER_WAVEGEN,valWAVEGEN,1);
    }
    else if (vsID == VSWaterSimple)
    {
      ADD_FRAGMENT(_shaderWaterSimple);
      switch(mainLight)
      {
      case ML_None:
        break;
      case ML_Sun:
        ADD_FRAGMENT(_shaderDirectionalADForced);
        //ADD_FRAGMENT(_shaderDirectionalNoSpec);
        break;
      default:
        Fail("Incompatible type of main light regarding the water shader");
      }
      static const float valPI_0p5_PI2_0p25[]={
        H_PI,0.5,H_PI*2,0.25
      };
      static const float valSIN_TAYLOR_3579[]={
        -1.0/(3*2),1.0/(5*4*3*2),-1.0/(7*6*5*4*3*2),+1.0/(9*8*7*6*5*4*3*2)
      };
      static const float valCOS_TAYLOR_2468[]={
        -1.0/2,+1.0/(4*3*2),-1.0/(6*5*4*3*2),+1.0/(8*7*6*5*4*3*2)
      };

      _d3DDevice->SetVertexShaderConstant(WATER_PI_0p5_PI2_0p25,valPI_0p5_PI2_0p25,1);
      _d3DDevice->SetVertexShaderConstant(WATER_SIN_TAYLOR_3579,valSIN_TAYLOR_3579,1);
      _d3DDevice->SetVertexShaderConstant(WATER_COS_TAYLOR_2468,valCOS_TAYLOR_2468,1);

      const float waveXScale = 2.0f/50;
      const float waveZScale = 1.0f/50;
      // uv spans from 0 to LandSegmentGrid*LandGrid
      // 50 m corresponds to U=1 
      // first number is a number of waves per 1m grid
      float waveHeight = GLandscape->GetWaveHeight() * 0.5f * 0.02f;
      const float valWAVE_HEIGHT_X_Y_XDER_YDER[]=
      {
        waveHeight,waveHeight,
          waveXScale*(2*H_PI)*waveHeight,waveZScale*(2*H_PI)*waveHeight
      };

      _d3DDevice->SetVertexShaderConstant(WATER_WAVE_HEIGHT_X_Y_XDER_YDER,valWAVE_HEIGHT_X_Y_XDER_YDER,1);

      // given minimal and maximal reflection angle cos, derive add and mul coefs
      const float minCosA = 0.17f; // cos 80 deg - full reflecion, no transparency
      const float maxCosA = 0.86f; // cos 30 deg - maximum transparency
      // x1 = x0 *M + A
      // x0 = minCosA -> x1 = 0
      // x0 = maxCosA -> x1 = 1
      // M = 1/(maxCosA-minCosA), A = -minCosA/(maxCosA-minCosA)
      static const float valWATER_REFLECT_ADD_MUL_u_u[]={
        -minCosA/(maxCosA-minCosA),1/(maxCosA-minCosA),0,0
      };
      _d3DDevice->SetVertexShaderConstant(WATER_REFLECT_ADD_MUL_u_u,valWATER_REFLECT_ADD_MUL_u_u,1);

      const float valWAVEGEN[]=
      {
        // wave phase offset  - due to looping, when multiplied by 8, must give integer
        20.0/8,30.0/8,
        // wave offset amplitude - arbitrary
        0.2,0.12
      };
      _d3DDevice->SetVertexShaderConstant(WATER_WAVEGEN,valWAVEGEN,1);
    }
    else if (vsID == VSBasic || vsID==VSBasicAlpha)
    {
      if (!instancing)
      {
        ADD_FRAGMENT(_shaderGetPosNorm);
        if (!landShadowEnabled)
        {
          ADD_FRAGMENT(_shaderModShadow);
        }
        else
        {
          ADD_FRAGMENT(_shaderLandShadow);
        }
        ADD_FRAGMENT(_shader);
      }
      else
      {
        ADD_FRAGMENT(_shaderGetPosNorm);
        ADD_FRAGMENT(_shaderGetInst);
        if (!landShadowEnabled)
        {
          ADD_FRAGMENT(_shaderModInstShadow);
        }
        else
        {
          ADD_FRAGMENT(_shaderLandShadow);
        }
        ADD_FRAGMENT(_shaderInstancing);
        if (!alphaInstancing)
        {
          ADD_FRAGMENT(_shaderInstancingModulate);
        }
        else
        {
          ADD_FRAGMENT(_shaderInstancingEmissive);
        }
      }
      switch(mainLight)
      {
      case ML_None:
        break;
      case ML_Sun:
        ADD_FRAGMENT(_shaderDirectional);
        break;
      case ML_Sky:
        ADD_FRAGMENT(_shaderDirectionalSky);
        break;
      case ML_Stars:
        ADD_FRAGMENT(_shaderDirectionalStars);
        break;
      case ML_SunObject:
        ADD_FRAGMENT(_shaderDirectionalSunObject);
        break;
      case ML_SunHaloObject:
        ADD_FRAGMENT(_shaderDirectionalSunHaloObject);
        break;
      case ML_MoonObject:
        ADD_FRAGMENT(_shaderDirectionalMoonObject);
        break;
      case ML_MoonHaloObject:
        ADD_FRAGMENT(_shaderDirectionalMoonHaloObject);
        break;
      default:
        Fail("Unknown type of main light");
      }
    }
    else if (vsID == VSNormalMap || vsID == VSNormalMapAlpha)
    {
      if (!instancing)
      {
        ADD_FRAGMENT(_shaderGetPosNorm);
        if (!landShadowEnabled)
        {
          ADD_FRAGMENT(_shaderModShadow);
        }
        else
        {
          ADD_FRAGMENT(_shaderLandShadow);
        }
        ADD_FRAGMENT(_shaderNormal);
      }
      else
      {
        ADD_FRAGMENT(_shaderGetPosNorm);
        ADD_FRAGMENT(_shaderGetInst);
        if (!landShadowEnabled)
        {
          ADD_FRAGMENT(_shaderModInstShadow);
        }
        else
        {
          ADD_FRAGMENT(_shaderLandShadow);
        }
        ADD_FRAGMENT(_shaderNormalInstancing);
        if (!alphaInstancing)
        {
          ADD_FRAGMENT(_shaderInstancingModulate);
        }
        else
        {
          ADD_FRAGMENT(_shaderInstancingEmissive);
        }
      }
      switch(mainLight)
      {
      case ML_None:
        break;
      case ML_Sun:
        ADD_FRAGMENT(_shaderDirectionalADForced);
        break;
      default:
        Fail("Incompatible type of main light regarding the normal map shader");
      }
    }
    else if (vsID == VSNormalMapThrough)
    {
      if (!instancing)
      {
        ADD_FRAGMENT(_shaderGetPosNorm);
        if (!landShadowEnabled)
        {
          ADD_FRAGMENT(_shaderModShadow);
        }
        else
        {
          ADD_FRAGMENT(_shaderLandShadow);
        }
        ADD_FRAGMENT(_shaderNormalThrough);
        #if _DEBUG
          _d3DDevice->SetDebugMarker(211);
        #endif
      }
      else
      {
        ADD_FRAGMENT(_shaderGetPosNorm);
        ADD_FRAGMENT(_shaderGetInst);
        if (!landShadowEnabled)
        {
          ADD_FRAGMENT(_shaderModInstShadow);
        }
        else
        {
          ADD_FRAGMENT(_shaderLandShadow);
        }
        ADD_FRAGMENT(_shaderNormalThroughInstancing);
        if (!alphaInstancing)
        {
          ADD_FRAGMENT(_shaderInstancingModulate);
        }
        else
        {
          ADD_FRAGMENT(_shaderInstancingEmissive);
        }
        #if _DEBUG
        _d3DDevice->SetDebugMarker(212);
        #endif
      }
      switch(mainLight)
      {
      case ML_None:
        break;
      case ML_Sun:
        ADD_FRAGMENT(_shaderDirectionalADForced);
        break;
      default:
        Fail("Incompatible type of main light regarding the normal map shader");
      }
    }
    else if (vsID == VSNormalMapDiffuse || vsID == VSNormalMapDiffuseAlpha || vsID==VSTerrain  || vsID==VSTerrainAlpha)
    {
      if (!instancing)
      {
        ADD_FRAGMENT(_shaderGetPosNorm);
        if (!landShadowEnabled)
        {
          ADD_FRAGMENT(_shaderModShadow);
        }
        else
        {
          ADD_FRAGMENT(_shaderLandShadow);
        }
        ADD_FRAGMENT(_shaderNormalDiffuse);
      }
      else
      {
        ADD_FRAGMENT(_shaderGetPosNorm);
        ADD_FRAGMENT(_shaderGetInst);
        if (!landShadowEnabled)
        {
          ADD_FRAGMENT(_shaderModInstShadow);
        }
        else
        {
          ADD_FRAGMENT(_shaderLandShadow);
        }
        ADD_FRAGMENT(_shaderNormalDiffuseInstancing);
        if (!alphaInstancing)
        {
          ADD_FRAGMENT(_shaderInstancingModulate);
        }
        else
        {
          ADD_FRAGMENT(_shaderInstancingEmissive);
        }
      }
      switch(mainLight)
      {
      case ML_None:
        break;
      case ML_Sun:
        ADD_FRAGMENT(_shaderDirectionalADForced);
        break;
      default:
        Fail("Incompatible type of main light regarding the normal map shader");
      }
    }
    else if (vsID == VSShadowVolume)
    {
      if (!instancing)
      {
        ADD_FRAGMENT(_shaderShadowVolume);
      }
      else
      {
        ADD_FRAGMENT(_shaderShadowVolumeInstancing);
      }
    }
    else if (vsID == VSSprite)
    {
      if (!instancing)
      {
        Fail("Error: Non instanced sprite shader fragment?");
      }
      else
      {
        ADD_FRAGMENT(_shaderSpriteInstancing);
      }
      switch(mainLight)
      {
      case ML_None:
        break;
      case ML_Sun:
        ADD_FRAGMENT(_shaderDirectional);
        break;
      case ML_Sky:
        ADD_FRAGMENT(_shaderDirectionalSky);
        break;
      case ML_Stars:
        ADD_FRAGMENT(_shaderDirectionalStars);
        break;
      case ML_SunObject:
        ADD_FRAGMENT(_shaderDirectionalSunObject);
        break;
      case ML_SunHaloObject:
        ADD_FRAGMENT(_shaderDirectionalSunHaloObject);
        break;
      case ML_MoonObject:
        ADD_FRAGMENT(_shaderDirectionalMoonObject);
        break;
      case ML_MoonHaloObject:
        ADD_FRAGMENT(_shaderDirectionalMoonHaloObject);
        break;
      default:
        Fail("Unknown type of main light");
      }
    }
    else if (vsID == VSPoint)
    {
      if (!instancing)
      {
        ADD_FRAGMENT(_shaderPointO);
      }
      else
      {
        Fail("Error: Instanced point shader fragment?");
      }
      switch(mainLight)
      {
      case ML_None:
        break;
      case ML_Sun:
        ADD_FRAGMENT(_shaderDirectional);
        break;
      case ML_Sky:
        ADD_FRAGMENT(_shaderDirectionalSky);
        break;
      case ML_Stars:
        ADD_FRAGMENT(_shaderDirectionalStars);
        break;
      case ML_SunObject:
        ADD_FRAGMENT(_shaderDirectionalSunObject);
        break;
      case ML_SunHaloObject:
        ADD_FRAGMENT(_shaderDirectionalSunHaloObject);
        break;
      case ML_MoonObject:
        ADD_FRAGMENT(_shaderDirectionalMoonObject);
        break;
      case ML_MoonHaloObject:
        ADD_FRAGMENT(_shaderDirectionalMoonHaloObject);
        break;
      default:
        Fail("Unknown type of main light");
      }
      ADD_FRAGMENT(_shaderWriteLights);
    }
    else
    {
      Fail("Unknown type of vertex shader");
    }
  }
  else if (skinningType == STPoint4)
  {
    if (vsID == VSBasic || vsID==VSBasicAlpha)
    {
      ADD_FRAGMENT(_shaderModShadow);
      ADD_FRAGMENT(_shaderSkinning);
      switch(mainLight)
      {
      case ML_None:
        break;
      case ML_Sun:
        ADD_FRAGMENT(_shaderDirectional);
        break;
      case ML_Sky:
        ADD_FRAGMENT(_shaderDirectionalSky);
        break;
      case ML_Stars:
        ADD_FRAGMENT(_shaderDirectionalStars);
        break;
      case ML_SunObject:
        ADD_FRAGMENT(_shaderDirectionalSunObject);
        break;
      case ML_SunHaloObject:
        ADD_FRAGMENT(_shaderDirectionalSunHaloObject);
        break;
      case ML_MoonObject:
        ADD_FRAGMENT(_shaderDirectionalMoonObject);
        break;
      case ML_MoonHaloObject:
        ADD_FRAGMENT(_shaderDirectionalMoonHaloObject);
        break;
      default:
        Fail("Unknown type of main light");
      }
    }
    else if (vsID == VSNormalMap || vsID == VSNormalMapAlpha)
    {
      ADD_FRAGMENT(_shaderModShadow);
      ADD_FRAGMENT(_shaderNormalSkinning);
      switch(mainLight)
      {
      case ML_None:
        break;
      case ML_Sun:
        ADD_FRAGMENT(_shaderDirectionalADForced);
        break;
      default:
        Fail("Incompatible type of main light regarding the normal map shader");
      }
    }
    else if (vsID == VSNormalMapThrough)
    {
      ADD_FRAGMENT(_shaderModShadow);
      ADD_FRAGMENT(_shaderNormalThroughSkinning);
      switch(mainLight)
      {
      case ML_None:
        break;
      case ML_Sun:
        ADD_FRAGMENT(_shaderDirectionalADForced);
        break;
      default:
        Fail("Incompatible type of main light regarding the normal map shader");
      }
    }
    else if (vsID == VSNormalMapDiffuse || vsID == VSNormalMapDiffuseAlpha)
    {
      ADD_FRAGMENT(_shaderModShadow);
      ADD_FRAGMENT(_shaderNormalDiffuseSkinning);
      switch(mainLight)
      {
      case ML_None:
        break;
      case ML_Sun:
        ADD_FRAGMENT(_shaderDirectionalADForced);
        break;
      default:
        Fail("Incompatible type of main light regarding the normal map shader");
      }
    }
    else if (vsID == VSShadowVolume)
    {
      ADD_FRAGMENT(_shaderShadowVolumeSkinning);
    }
    else
    {
      Fail("Unknown type of skinned vertex shader");
    }
  }
  else
  {
    RptF("Unsupported skinning type");
  }

  if (vsID!=VSShadowVolume && vsID!=VSWater)
  {
    // Add fogMode fragment
    switch(fogMode)
    {
    case FM_None:
      ADD_FRAGMENT(_shaderFogNone);
      break;
    case FM_Fog:
      ADD_FRAGMENT(_shaderFog);
      break;
    case FM_Alpha:
      ADD_FRAGMENT(_shaderFogAlpha);
      break;
    default:
      Fail("Unknown type of main light");
    }
  }

  // Remember the start of lights
  int lightIndex = 0;
  int lightCount = 0;

  // Add P&S light fragments and set the constants
  if (
    vsID == VSBasic || vsID == VSBasicAlpha ||
    vsID == VSNormalMap || vsID == VSNormalMapAlpha ||
    vsID == VSNormalMapThrough ||
    vsID == VSNormalMapDiffuse || vsID == VSNormalMapDiffuseAlpha ||
    vsID == VSTerrain || vsID == VSTerrainAlpha ||
    // no light fragments for sea water
    vsID == VSWaterSimple ||
    vsID == VSSprite
  )
  {

    // Detect the usage of VS
    bool alphaVS = (
      (vsID==VSBasicAlpha) || (vsID==VSNormalMapAlpha) ||
      (vsID==VSNormalMapDiffuseAlpha) || (vsID==VSTerrainAlpha)
    );

    // Add the alpha shadow fragment (in case we don't use the alpha for something else)
    if ((!alphaVS) && CanUseNewShadows() && alphaShadowEnabled)
    {
      ADD_FRAGMENT(_shaderAlphaShadow);
    }

    // Get the write lights fragment (either alpha or usual)
    DWORD *writeFragment = alphaVS ? _shaderWriteLightsAlphaMod : _shaderWriteLights;
    if (vsID==VSSprite) writeFragment = _shaderWriteLightsColorMod;
    // Remember the index of the first light
    lightIndex = fragmentsCount;

    // Add lights until the VS is full
    if (EnableLights && ((mainLight == ML_Sun) || (mainLight == ML_None)))
    {
      for (int i = 0; i < _lights.Size(); i++)
      {
        LightDescription ldesc;
        _lights[i]->GetDescription(ldesc, GetAccomodateEye());

        if (i<maxPointLights && (ldesc.type == LTPoint) && (instructionCount + GET_FRAGMENT_INSTRUCTION_COUNT(_shaderPoint[0]) <= MAX_INSTRUCTION_COUNT - GET_FRAGMENT_INSTRUCTION_COUNT(writeFragment)))
        {
          ADD_FRAGMENT(_shaderPoint[i]);
        }
        else if (i<maxSpotLights && (ldesc.type == LTSpotLight) && (instructionCount + GET_FRAGMENT_INSTRUCTION_COUNT(_shaderSpot[0]) <= MAX_INSTRUCTION_COUNT - GET_FRAGMENT_INSTRUCTION_COUNT(writeFragment)))
        {
          ADD_FRAGMENT(_shaderSpot[i]);
        }
        else
        {
          // Either we found unknown type of light or (more likely) we didn't have the space to add the specified light
          break;
        }
      }
    }

    lightCount = fragmentsCount - lightIndex;
    ADD_FRAGMENT(writeFragment);
  }

  // Make sure we fit into instruction count
  Assert(instructionCount <= MAX_INSTRUCTION_COUNT);

#if SHADER_INFO_PRINTING
  if (_showNotCachedShader || _printNotCachedShader)
  {
    // In case we don't know the VS, remember it and the info about it
    bool vsInfoFound = false;
    for (int i = 0; i < _vsInfo.Size(); i++)
    {
      VSInfo &vsInfo = _vsInfo[i];
      if (vsInfo._fragmentCount != fragmentsCount) continue;
      int j;
      for (j = 0; j < fragmentsCount; j++)
      {
        if (vsInfo._fragments[j] != fragments[j]) break;
      }
      if (j < fragmentsCount) continue;
      vsInfo._usedCount += triangleCount;
      vsInfoFound = true;
      break;
    }
    if (!vsInfoFound)
    {
      VSInfo vsInfo;
      for (int j = 0; j < fragmentsCount; j++)
      {
        vsInfo._fragments[j] = fragments[j];
      }
      vsInfo._fragmentCount = fragmentsCount;

      // Get and remember unoptimized size in bytes

      #if 0
        vsInfo._icBeforeOptimalization = SpliceShaders(_shaderTokenArray,fragments,fragmentsCount,false);
        vsInfo._icAfterOptimalization = SpliceShaders(_shaderTokenArray,fragments,fragmentsCount,true);
      #else
        vsInfo._icBeforeOptimalization = vsInfo._icAfterOptimalization = 0;
      #endif

      vsInfo._usedCount = triangleCount;
      _vsInfo.Add(vsInfo);
    }
  }
#endif

#if defined _XBOX && _ENABLE_CHEATS
  if (GInput.GetCheatXToDo(CXTUseVSCache))
  {
    UseVSCache = !UseVSCache;
    GlobalShowMessage(500, "UseVSCache %s", UseVSCache ? "On":"Off");
  }
  if (GInput.GetCheatXToDo(CXTEnableLights))
  {
    EnableLights = !EnableLights;
    GlobalShowMessage(500, "EnableLights %s", EnableLights ? "On":"Off");
  }
#endif

  // Try to find the already spliced VS in the static cache
  //int vsCacheItemIndex = -1;
  AutoArray<DWORD> *shaderArrayPtr = NULL;
  if (UseVSCache)
  {
    bool cacheContainsShader = false;

    // Look up the cache with lights
    for (int i = 0; i < _shaderCacheLightId.Size(); i++)
    {
      // Test if the fragment without lights matches the cached fragment
      int fragmentIndex = 0;
      bool matches = true;
      for (int j = 0; j < _shaderCacheLightId[i]._fragmentsCount; j++)
      {
        if (_shaderCacheLightId[i]._fragments[j] != fragments[fragmentIndex])
        {
          matches = false;
          break;
        }

        // Move to next item in the fragment array
        fragmentIndex++;

        // In case we're in lights then skip them
        if (fragmentIndex == lightIndex)
        {
          fragmentIndex += lightCount;
        }

        // End in case we reached the end in the fragment array
        if (fragmentIndex > fragmentsCount)
        {
          matches = false;
          break;
        }
      }
      if (fragmentIndex < fragmentsCount) matches = false;

      // If the fragment matches the cached fragment identifier then remember it to use it later
      if (matches)
      {
        // Lookup the light combinations
        int lightMask = 0;
        for (int j = 0; j < lightCount; j++)
        {
          if (fragments[lightIndex + j] == _shaderSpot[j])
          {
            lightMask |= 1 << (lightCount - 1 - j);
          }
        }
        int lightCombinationNumber = (1 << lightCount) - 1 + lightMask;
        Assert(lightCombinationNumber < SC_LIGHTCOMBINATIONS);

        // If the shader hasn't been loaded yet then try to load it
        ShaderCacheItem &item = _shaderCacheLight[i][lightCombinationNumber];
        if (item._shader.Size() == 0)
        {
          // Create the filename
          char fileName[256];
          sprintf(fileName, "SCPool\\lci%d_%d.xvu", i, lightCombinationNumber);

          // Load the data
          QIFStreamB f;
          f.AutoOpen(fileName);
          if (f.fail())
          {
            RptF("Error: Failed to open shader %s", fileName);
          }
          else
          {
            if (f.PreRead())
            {
              // the buffer should be DWORD aligned
              Assert(f.rest()%sizeof(DWORD)==0);
              int size = (f.rest()+sizeof(DWORD)-1) / sizeof(DWORD);
              if (size>0)
              {
                item._shader.Realloc(size);
                item._shader.Resize(size);
                f.read(item._shader.Data(), size * sizeof(DWORD));
                _shaderCacheLRU._fmcl.Add(&item);
              }
            }
          }
        }

        // If item exists then remember it
        if (item._shader.Size() != 0)
        {
          shaderArrayPtr = &item._shader;
          _shaderCacheLRU._fmcl.Refresh(&item);
        }

        cacheContainsShader = true;

        // Cancel the search in any case
        break;
      }
    }

    // Look up the ordinary cache
    if (shaderArrayPtr == NULL)
    {
      for (int i = 0; i < _shaderCacheId.Size(); i++)
      {
        // Test if the fragment matches the cached fragment
        if (_shaderCacheId[i]._fragmentsCount != fragmentsCount) continue;
        int j;
        for (j = 0; j < fragmentsCount; j++)
        {
          if (_shaderCacheId[i]._fragments[j] != fragments[j]) break;
        }
        if (j < fragmentsCount) continue;

        // If the shader hasn't been loaded yet then try to load it
        if (_shaderCache[i]._shader.Size() == 0)
        {
          // Create the filename
          char fileName[256];
          sprintf(fileName, "SCPool\\bci%d.xvu", i);

          // Load the data
          QIFStreamB f;
          f.AutoOpen(fileName);
          if (f.fail())
          {
            RptF("Error: Failed to open shader %s", fileName);
          }
          else
          {
            if (f.PreRead())
            {
              // the buffer should be DWORD aligned
              Assert(f.rest()%sizeof(DWORD)==0);
              int size = (f.rest()+sizeof(DWORD)-1) / sizeof(DWORD);
              if (size>0)
              {
                _shaderCache[i]._shader.Realloc(size);
                _shaderCache[i]._shader.Resize(size);
                f.read(_shaderCache[i]._shader.Data(), size * sizeof(DWORD));
                _shaderCacheLRU._fmcl.Add(&_shaderCache[i]);
              }
            }
          }
        }

        // If item exists then remember it
        if (_shaderCache[i]._shader.Size() != 0)
        {
          shaderArrayPtr = &_shaderCache[i]._shader;
          _shaderCacheLRU._fmcl.Refresh(&_shaderCache[i]);
        }

        cacheContainsShader = true;

        // Cancel the search in any case
        break;
      }
    }

#if 0
    // Show the shader is not in a cache
    if (!cacheContainsShader)
    {
      char s[2048];
      sprintf(s, "Warning: VS not in cache - ");
      AddFragmentNames(s, fragments, fragmentsCount);
      LogF("%s", s);
    }
#endif
  }

  // Splice those fragments
  HRESULT hr;
  if (shaderArrayPtr == NULL)
  {
    // shader not in VS cache
    // we might want to gather some statistics here
    // if the shader is common, we might want to add it
    SpliceShaders(_shaderTokenArray,fragments, fragmentsCount, FALSE);
    shaderArrayPtr = &_shaderTokenArray;
    ADD_COUNTER(sScSp,1);
  }

  // Remember old shader handle to be able to delete it later
  DWORD oldShaderHandle = _shaderHandle;

  // Create the specified vertex shader
  const DWORD *declaration = NULL;
  if (skinningType == STNone)
  {
    if (instancing)
    {
      declaration = VertexVSDecl[decl][VSDV_Instancing];
    }
    else
    {
      declaration = VertexVSDecl[decl][VSDV_Basic];
    }
  }
  else if (skinningType == STPoint4)
  {
    if (instancing)
    {
      Fail("Skinning and instancing not supported together");
      RptF("Skinning and instancing not supported together");
    }
    else
    {
      declaration = VertexVSDecl[decl][VSDV_Skinning];
    }
  }
  else
  {
    Fail("Unsupported skinning type");
    RptF("Unsupported skinning type");
  }

  // Create the vertex shader - either from spliced shader or from the cache shader
  if (FAILED(hr = _d3DDevice->CreateVertexShader(declaration, shaderArrayPtr->Data(), &_shaderHandle, 0)))
  {
    DDErrorXB("Cannot create vertex shader",hr);
    RptF("Cannot create vertex shader");
    return;
  }

  ADD_COUNTER(sScCS,1);
  // Set the vertex shader
  _d3DDevice->LoadVertexShader(_shaderHandle, 0);
  _d3DDevice->SelectVertexShader(NULL, 0);
  //_d3DDevice->SetVertexShader(_shaderHandle);

  // Delete the previous shader
  if ((oldShaderHandle != 0) && (oldShaderHandle != _shaderHandle))
  {
    _d3DDevice->DeleteVertexShader(oldShaderHandle);
  }

  // Update new statuses
  _currSkinningType = skinningType;
  _currInstancing = instancing;
  _currMainLight = mainLight;
  _currFogMode = fogMode;
  _currVertexShaderID = vsID;
  _currAlphaShadowEnabled = alphaShadowEnabled;
  _currLandShadowEnabled = landShadowEnabled;
  _currAlphaInstancing = alphaInstancing;

  _lightsHasChanged = false;
}

void EngineDDXB::SetupPointSpotConstants()
{
}

void EngineDDXB::SetupPSConstants()
{
  // Setup constants which are not previously set.
  if (
    _vertexShaderID == VSNormalMap || _vertexShaderID == VSNormalMapAlpha ||
    _vertexShaderID == VSNormalMapThrough ||
    _vertexShaderID == VSNormalMapDiffuse || _vertexShaderID == VSNormalMapDiffuseAlpha ||
    _vertexShaderID == VSTerrain || _vertexShaderID == VSTerrainAlpha ||
    _vertexShaderID == VSWater || _vertexShaderID == VSWaterSimple ||
    _mainLight == ML_Sky
  )
  {
    LightSun *sun = GScene->MainLight();

    {
      //////////////////////////////////////////////////////////////////////////
      // Diffuse
      //_d3DDevice->SetPixelShaderConstant(PS_DIFFUSE, (float*)&_diffuse, 1);
      Color diffuse;
      if (_mainLight == ML_Sun)
      {
        diffuse = sun->GetDiffuse() * _matDiffuse * GetAccomodateEye();
        diffuse.SetA(_matDiffuse.A());
      }
      else if (_mainLight == ML_Sky)
      {
        diffuse = sun->SunSkyColorResult() * GetAccomodateEye() * _modColor;
      }
      else
      {
        diffuse = Color(0, 0, 0, 0);
      }
      PSConstRef &cRefDiffuse = _constDiffuse[_currPixelShaderSpecularSel][_currPixelShaderSel];
      for (int i = 0; i < cRefDiffuse._count; i++)
      {
        D3DRENDERSTATETYPE rsDiffuse = cRefDiffuse._rs[i];

        // Create fix-up entry in case we are recording
        if (_recording)
        {
          DWORD pbOffset;
          _d3DDevice->GetPushBufferOffset(&pbOffset);
          if ((_matDiffuse.R() > 0.9) && (_matDiffuse.G() < 0.1) && (_matDiffuse.B() < 0.1))
          {
            Fail("XXX");
          }
          _activePushBuffer->_fixupEntries.Add(new FEDiffuse(pbOffset, rsDiffuse, _mainLight, _matDiffuse));
        }

        // Set the PS constant
        D3DCOLOR c = FAST_D3DRGBA_SAT(diffuse.R(), diffuse.G(), diffuse.B(), diffuse.A());
        _d3DDevice->SetRenderState(rsDiffuse, c);
      }
    }

    {
      //////////////////////////////////////////////////////////////////////////
      // DiffuseBack
      //_d3DDevice->SetPixelShaderConstant(PS_DIFFUSE_BACK, (float*)&diffuseBack, 1);
      Color diffuseBack;
      if (_mainLight == ML_Sun)
      {
        float backCoef = _matForcedDiffuse.A();
        diffuseBack = sun->GetDiffuse() * _matDiffuse * GetAccomodateEye() * backCoef;
        diffuseBack.SetA(backCoef);
      }
      else if (_mainLight == ML_Sky)
      {
        Vector3 sunDirection = _invWorld.Orientation() * sun->SunDirection();
        sunDirection.Normalize();
        Vector3 sunDirBX2 = sunDirection * 0.5 + Vector3(0.5,0.5,0.5);
        diffuseBack = Color(sunDirBX2.X(),sunDirBX2.Y(),sunDirBX2.Z());
      }
      else
      {
        diffuseBack = Color(0, 0, 0, 0);
      }
      PSConstRef &cRefDiffuseBack = _constDiffuseBack[_currPixelShaderSpecularSel][_currPixelShaderSel];
      for (int i = 0; i < cRefDiffuseBack._count; i++)
      {
        D3DRENDERSTATETYPE rsDiffuseBack = cRefDiffuseBack._rs[i];

        // Create fix-up entry in case we are recording
        if (_recording)
        {
          DWORD pbOffset;
          _d3DDevice->GetPushBufferOffset(&pbOffset);
          _activePushBuffer->_fixupEntries.Add(new FEDiffuseBack(pbOffset, rsDiffuseBack, _mainLight, _matDiffuse, _matForcedDiffuse.A()));
        }

        // Set the PS constant
        D3DCOLOR c = FAST_D3DRGBA_SAT(diffuseBack.R(), diffuseBack.G(), diffuseBack.B(), diffuseBack.A());
        _d3DDevice->SetRenderState(rsDiffuseBack, c);
      }
    }

    {
      //////////////////////////////////////////////////////////////////////////
      // Specular
      //_d3DDevice->SetPixelShaderConstant(PS_SPECULAR, (float*)&specular, 1);
      Color specular;
      if (_mainLight == ML_Sun)
      {
        // Set the PS constant
        specular = sun->GetDiffuse() * _matSpecular * GetAccomodateEye();
      }
      else if (_mainLight == ML_Sky)
      {
        // Set the PS constant
        specular = sun->SkyColorResult() * GetAccomodateEye() * _modColor;
      }
      else
      {
        // Set the PS constant
        specular = Color(0, 0, 0, 0);
      }
      PSConstRef &cRefSpecular = _constSpecular[_currPixelShaderSpecularSel][_currPixelShaderSel];
      for (int i = 0; i < cRefSpecular._count; i++)
      {
        D3DRENDERSTATETYPE rsSpecular = cRefSpecular._rs[i];

        // Create fix-up entry in case we are recording
        if (_recording)
        {
          DWORD pbOffset;
          _d3DDevice->GetPushBufferOffset(&pbOffset);
          _activePushBuffer->_fixupEntries.Add(new FESpecular(pbOffset, rsSpecular, _mainLight, _matSpecular));
        }

        D3DCOLOR c = FAST_D3DRGBA_SAT(specular.R(), specular.G(), specular.B(), specular.A());
        _d3DDevice->SetRenderState(rsSpecular, c);
      }
    }
  }
}

void EngineDDXB::DrawSectionTL
(
 const Shape &sMesh, int beg, int end, int bias, int instancesOffset, int instancesCount
 )
{
  PROFILE_SCOPE_GRF(xbDST,0);
  if (_resetNeeded) return;

  _hwInDefinedState = false;

  // Set bias specified for this section
  SetBias(bias);

#ifdef _XBOX

  // ---------------------------------------------
  // Shader setting ------------------------------

  // Calculate number of triangles to draw
  int triangleCount = 0;
#if SHADER_INFO_PRINTING
  if (_showNotCachedShader || _printNotCachedShader)
  {
    VertexBufferD3DXB *buffer = static_cast<VertexBufferD3DXB *>(sMesh.GetVertexBuffer());
    int b = beg;
    while (b<end)
    {
      int endDraw;
      const VBSectionInfo &siBeg = buffer->_sections[b];
      int begVertex = siBeg.begVertex, endVertex = siBeg.endVertex;
      for (endDraw=b+1; endDraw<end; endDraw++)
      {
        const VBSectionInfo &siCur = buffer->_sections[endDraw-1];
        const VBSectionInfo &siNxt = buffer->_sections[endDraw];
        if (siCur.end!=siNxt.beg)  break;
        saturateMin(begVertex,siCur.begVertex);
        saturateMax(endVertex,siCur.endVertex);
      }
      Assert (endDraw>b);
      Assert (endVertex>=begVertex);
      const VBSectionInfo &siEnd = buffer->_sections[endDraw-1];
      int nInstances = instancesCount>0 ? instancesCount : 1;
      if (_vertexShaderID == VSPoint)
      {
        triangleCount += sMesh.NVertex() * 2;
      }
      else
      {
        triangleCount += ((siEnd.end-siBeg.beg)/3) * nInstances;
      }
      b = endDraw;
    }
  }
#endif

  bool instancing = instancesCount>=1;

  #if _ENABLE_REPORT
  static bool countVSChanges = false;
  if (countVSChanges)
  {
    // If the structure of the expected shader has changed, then link the shader again
    if (
      _currVertexShaderID != _vertexShaderID ||
      _lightsHasChanged ||
      _currSkinningType != _skinningType ||
      _currInstancing!=instancing ||
      _currMainLight != _mainLight ||
      _currFogMode != _fogMode ||
      _currAlphaShadowEnabled!= _alphaShadowEnabled ||
      _currLandShadowEnabled!=_landShadowEnabled ||
      _currAlphaInstancing!=_alphaInstancing
    )
    {
      ADD_COUNTER(scCd1,_currVertexShaderID != _vertexShaderID);
      ADD_COUNTER(scCd2,_lightsHasChanged);
      ADD_COUNTER(scCd3,_currSkinningType != _skinningType);
      ADD_COUNTER(scCd4,_currInstancing!=instancing);
      ADD_COUNTER(scCd5,_currMainLight != _mainLight);
      ADD_COUNTER(scCd6,_currFogMode != _fogMode);
      ADD_COUNTER(scCd6,_currAlphaShadowEnabled != _alphaShadowEnabled);
      ADD_COUNTER(scCd6,_currLandShadowEnabled != _landShadowEnabled);
      ADD_COUNTER(scCd6,_currAlphaInstancing != _alphaInstancing);
    }
  }
  #endif
  
  if (!_recording)
  {
    SetupSectionTL(
      sMesh.GetVertexDeclaration(), instancing, _skinningType, _vertexShaderID, _mainLight, _fogMode,
      _alphaShadowEnabled, _landShadowEnabled, _alphaInstancing, triangleCount
    );
  }


  // Setup constants like lights
  SetupNonFixedConstants();

  // Select the pixel shader
  DoSelectPixelShader();

  // Setup PS constants
  SetupPSConstants();

  // Setup bones constants
  if (_skinningType != STNone)
  {
    if (!_bonesWereSetDirectly)
    {
      DoAssert(!_recording);
      // Get minBoneIndex and bonesCount of this section
      // Note that it is OK to use just the "beg" section, because we're sure all other sections have the same parameters
      const ShapeSection &sec = sMesh.GetSection(beg);
      int minBoneIndex = sec._minBoneIndex;
      int bonesCount = sec._bonesCount;
      if (_bonesHaveChanged || (_currMinBoneIndex != minBoneIndex))
      {

        // Note that the section we're drawing can have zero bones
        if (bonesCount > 0)
        {
          // Setup required matrices
          FLOAT boneValues[MaxSkinningBones*3*4];
          for (int i = 0; i < bonesCount; i++)
          {
            ConvertMatrixTransposed3(&boneValues[i*3*4], _bones[minBoneIndex + i]);
          }
          _d3DDevice->SetVertexShaderConstant(BONES_START, boneValues, bonesCount*3);
        }

        // Setup the offset to the matrices
        float constants[4] = {0.5f - minBoneIndex * 3.0f, 765.0f, 0.0f, 0.0f};
        _d3DDevice->SetVertexShaderConstant(MATRICES_OFFSET_765, constants, 1);

        _bonesHaveChanged = false;
        _currMinBoneIndex = minBoneIndex;
      }
    }
  }

  // ---------------------------------------------
  // ---------------------------------------------

#endif

  // no queueing, direct drawing
  VertexBufferD3DXB *buf = static_cast<VertexBufferD3DXB *>(sMesh.GetVertexBuffer());

  DoAssert(buf->_sections.Size() >= end);

  while (beg<end)
  {
    // check which sections can be concatenated
    // scan sections to provide reasonable values for vertex range
    int endDraw;
    const VBSectionInfo &siBeg = buf->_sections[beg];
    int begVertex = siBeg.begVertex, endVertex = siBeg.endVertex;
    for (endDraw=beg+1; endDraw<end; endDraw++)
    {
      const VBSectionInfo &siCur = buf->_sections[endDraw-1];
      const VBSectionInfo &siNxt = buf->_sections[endDraw];
#ifdef STRIPIZATION_ENABLED
      Assert(sMesh.GetFaceArrayType() != FATStripized);
#endif
      if (siCur.end!=siNxt.beg)  break;
      saturateMin(begVertex,siCur.begVertex);
      saturateMax(endVertex,siCur.endVertex);
    }
    Assert (endDraw>beg);
    Assert (endVertex>=begVertex);
    const VBSectionInfo &siEnd = buf->_sections[endDraw-1];

    // all attributes prepared

#if LOG_STATE_LEVEL
    if (LogStatesOnce)
    {
      if (LogStatesOnce)
      {
        LogF
          (
          "  Draw TL .... %d tris (%d..%d,%s)",
          (siEnd.end-siBeg.beg)/3,
          beg,end,
          (const char *)sMesh.GetSection(beg).GetDebugText()
          );
      }
    }
#endif

    //const ShapeSection &sec = sMesh.GetSection(section);
    // actualy draw all data
    // note: min index, max index is not used by HW T&L
    //LogF("TLStart");
    // get vbuffer size
    PROFILE_DX_SCOPE(3drTL);
#if !_RELEASE
    if (endVertex+_vOffsetLast>_vBufferSize)
    {
      LogF("vrange %d..%d",begVertex,endVertex);
      LogF("offset %d",_vOffsetLast);
      LogF("vb size %d",_vBufferSize);
      Fail("Vertex out of range");
      return;
    }
#endif
    DoAssert(_d3dFrameOpen || _recording);
    int nInstances = instancesCount>0 ? instancesCount : 1;
    // note: begVertex and endVertex do not work when nInstances>1
    HRESULT ret;
    int triangleCount;
    int indicesCount;
#ifdef STRIPIZATION_ENABLED
    if ((sMesh.GetFaceArrayType() == FATStripized) && EnableStrips)
    {
      triangleCount = (siEnd.end-siBeg.beg - 2) * nInstances + (nInstances - 1) * 4;
      // If there is odd number of triangles in one section, then add the even-maker triangles
      if ((siEnd.end-siBeg.beg) & 1)
      {
        triangleCount += (nInstances - 1) * 1;
      }
      indicesCount = triangleCount + 2;
      ret =_d3DDevice->DrawIndexedPrimitive
        (
        D3DPT_TRIANGLESTRIP,
        begVertex,endVertex-begVertex,
        siBeg.beg+_iOffset, triangleCount
        );
    }
    else
#endif
    {
      //if ((sMesh.GetFaceArrayType() != FATStripized) || EnableStrips) return;
#ifdef _DEBUG
      /*
      if (_fogMode == FM_Alpha)
      {
      _d3DDevice->SetDebugMarker(334);
      }
      */
#endif

      if (_vertexShaderID == VSPoint)
      {
        triangleCount = sMesh.NVertex() * 2;
#ifdef _DEBUG
        /*
        _d3DDevice->SetDebugMarker(333);
        */
#endif
      }
      else
      {
        triangleCount = ((siEnd.end-siBeg.beg)/3) * nInstances;
      }
      indicesCount = triangleCount * 3;
      ret =_d3DDevice->DrawIndexedPrimitive
        (
        D3DPT_TRIANGLELIST,
        begVertex,endVertex-begVertex,
        siBeg.beg+_iOffset, triangleCount
        );
#if _DEBUG
      /*
      _d3DDevice->SetDebugMarker(0);
      */
#endif
    }
    //LogF("TLEnd");
#if _ENABLE_REPORT
    if (ret!=D3D_OK)
    {
      // log vertex indices from index buffer
      LogF("  Vertex offset %d",_vOffsetLast);
      LogF("  Vertex buffer supposed size %d",_vBufferSize);
      LogF
        (
        "  Vertex range %d..%d (count %d)",
        begVertex,endVertex,endVertex-begVertex
        );
#ifndef _XBOX
      D3DVERTEXBUFFER_DESC desc;
      _vBufferLast->GetDesc(&desc);
      LogF("  Vertex buffer actual size %d",desc.Size/sizeof(SVertexXB));
#endif
      int startIB = siBeg.beg+_iOffset;
      int lenIB = siEnd.end-siBeg.beg;
      LogF("  Index buffer range %d (%d)",startIB,lenIB);
#if !WRITEONLYIB
      BYTE *data;
      _iBufferLast->Lock
        (
        startIB*sizeof(VertexIndex),lenIB*sizeof(VertexIndex),
        &data,D3DLOCK_READONLY
        );
      if (data)
      {
        // read index data
        VertexIndex *iData = (VertexIndex *)data;
        for (int i=0; i<lenIB; i++)
        {
#if V_OFFSET_CUSTOM
          int ido = iData[i]+_vOffsetLast;
#else
          int ido = iData[i];
#endif
          if (ido>=endVertex)
          {
            LogF("  Overflow %4d : %5d",i,iData[i]);
          }
        }
        _iBufferLast->Unlock();
      }
#endif
#ifndef _XBOX
      DWORD sh;
      _d3DDevice->GetVertexShader(&sh);
      LogF("  VBuffer FVF %x, current shader %x",desc.FVF,sh);
#endif
      Fail("DIP failed.");
    }
#endif
    DX_CHECKN("DIP T&L",ret);
    ADD_COUNTER(dPrim,1);
    ADD_COUNTER(tris, triangleCount);
    ADD_COUNTER(inds, indicesCount);
    beg = endDraw;
  }
}

#include <Es/Memory/normalNew.hpp>
/// Xbox implementation of GPU Visibility Query
class XBVisibilityQuery: public IVisibilityQuery
{
  friend class EngineDDXB;

  enum QueryStatus
  {
    /// query was not started
    QueryNone,
    /// query was already started
    QueryStarted,
    /// we already have the result
    QueryDone
  };
  enum {NResults=4};
  DWORD _index[NResults];
  int _result[NResults];
  QueryStatus _status[NResults];
  int _toStart;

  /// check how many frames it takes between query and result
  int Latency() const;
public:
  XBVisibilityQuery();
  ~XBVisibilityQuery();
  virtual bool IsVisible() const;
  int VisiblePixels() const;

  void CheckResults(EngineDDXB *engine);

  USE_FAST_ALLOCATOR
};
#include <Es/Memory/debugNew.hpp>

DEFINE_FAST_ALLOCATOR(XBVisibilityQuery)

XBVisibilityQuery::XBVisibilityQuery()
{
  _toStart = 0;
  for (int i=0; i<NResults; i++)
  {
    _index[i] = 0;
    _status[i] = QueryNone;
    _result[i] = true;
  }
}

XBVisibilityQuery::~XBVisibilityQuery()
{
  for (int i=0; i<NResults; i++)
  {
    if (_status[i] == QueryStarted)
    {
      // terminate the query
      //LogF("Pending query destroyed");
      GEngineDD->_ids.Close(_index[i]);
    }
  }
}

bool XBVisibilityQuery::IsVisible() const
{
  unconst_cast(this)->CheckResults(GEngineDD);
#if _ENABLE_REPORT
  static bool AlwaysTrue = false;
  if (AlwaysTrue) return true;
#endif
#if 0
  // check how old the query result is
  const int maxAge = 2;
  int latency = Latency();
  if (latency>maxAge)
  {
    // if query result is too old, ignore it
    return true;
  }
#endif
  for (int i=0; i<NResults; i++)
  {
    if (_result[i]>0) return true;
  }
  return false;
}

int XBVisibilityQuery::VisiblePixels() const
{
  unconst_cast(this)->CheckResults(GEngineDD);
  // return most recent result we have
  int recent = _toStart;
  for (int i=0; i<NResults; i++)
  {
    if (--recent<0) recent = NResults-1;
    if (_status[recent]==QueryDone)
    {
      return _result[recent];
    }
  }
  // we do not know - query did not finish
  return -1;
}

int XBVisibilityQuery::Latency() const
{
  int latency = 0;
  int recent = _toStart;
  for (int i=0; i<NResults; i++)
  {
    if (--recent<0) recent = NResults-1;
    if (_status[recent]!=QueryStarted)
    {
      break;
    }
    latency++;
  }
  return latency;
}

void XBVisibilityQuery::CheckResults(EngineDDXB *engine)
{
  for (int i=0; i<NResults; i++)
  {
    if (_status[i]==QueryStarted)
    {
      // check the result
      UINT pixels = 0;
      int id = _index[i];
      HRESULT ret = engine->_d3DDevice->GetVisibilityTestResult(id,&pixels,NULL);
      if (ret==D3D_OK)
      {
        _result[i] = pixels;
        engine->_ids.Close(id);
        _status[i] = QueryDone;
        //LogF("Close (OK ) %d for %x:%d - %d",id,q,i,pixels);
      }
      else if (ret!=D3DERR_TESTINCOMPLETE)
      {
        _result[i] = INT_MAX; // we do not know how many pixels completed - assume all
        engine->_ids.Close(id);
        _status[i] = QueryDone;
        //LogF("Close (Err) %d for %x:%d",id,q,i);
      }
    }
  }
}

QueryId::QueryId()
{
  for (int i=0; i<MaxQueries; i++) _used[i] = false;
  _curUsed = 0;
}

int QueryId::Open()
{
  for (int i=0; i<MaxQueries-1; i++)
  {
    if (_curUsed>=MaxQueries) _curUsed=0;
    if (!_used[_curUsed])
    {
      int id = _curUsed++;
      _used[id] = true;
      return id;
    }
    _curUsed++;
  }
  // no valid query found
  return -1;
}
void QueryId::Close(int id)
{
  _used[id] = false;
}

void EngineDDXB::SetupModelSpaceRendering(const Matrix4 &trans)
{

  BeginInstanceTL(trans,DrawParameters::_default);
  // similar to BeginMeshTL {{
  // Setup position of the camera in model coordinates, keep the model to camera distance

  // Get the position of the camera
  Vector3 cameraPosition = _sceneProps->GetCameraPosition();

  // Transform and normalize the camera position
  Vector3 cameraObjectPosition = TransformAndNormalizePositionToObjectSpace(cameraPosition);

  D3DXVECTOR4 cpos(cameraObjectPosition.X(), cameraObjectPosition.Y(), cameraObjectPosition.Z(), 1.0f);
  _d3DDevice->SetVertexShaderConstant(CAMERA_POSITION, &cpos, 1);

}

void EngineDDXB::BeginVisibilityTesting()
{
  if (_visTestCount++!=0) return;

  // TODO: 4x AA should be possible //D3DSetRenderState(D3DRS_MULTISAMPLEMODE, D3DMULTISAMPLEMODE_4X);

  SwitchTL(true);

  // Set UV dynamic range constants
  UVPair min,scale;
  scale.u = 1;
  scale.v = 1;
  min.u = 0;
  min.v = 0;
  SetTexGenScaleAndOffset(scale, min);

  // And the rest...
  ChangeClipPlanes();

  SetBias(0);

  // Set the lights
  LightList lights;
  SetLights(lights,0);

  // similiar to BeginMeshTL }}

  _skinningType = STNone;

  TLMaterial mat;
  mat.Init();

#if _ENABLE_REPORT
  static bool ShowVisTests = false;
  int spec = ShowVisTests ? NoZWrite|NoBackfaceCull : NoZWrite|IsHidden|NoBackfaceCull;
#else
  const int spec = NoZWrite|IsHidden|NoBackfaceCull;
#endif
  EngineShapeProperties prop;
  PrepareTriangleTL(MipInfo(),TexMaterialLODInfo(GlobPreloadMaterial(EmptyMaterial), 0),spec,mat,lights,0,prop);

  _hwInDefinedState = false;

  D3DSTREAM_INPUT si;
  si.VertexBuffer = _vbMinMaxCube;
  si.Stride = sizeof(SVertexXB);
  si.Offset = 0;
  _d3DDevice->SetVertexShaderInputDirect(VAFVSDecl[VD_Position_Normal_Tex0][VSDV_Basic], 1, &si);

  SetupSectionTL(
    VD_Position_Normal_Tex0, false, _skinningType, _vertexShaderID, _mainLight, _fogMode,
    _alphaShadowEnabled, _landShadowEnabled, _alphaInstancing
  );

  SetupNonFixedConstants();
  DoSelectPixelShader();
}

void EngineDDXB::EndVisibilityTesting()
{
  if (--_visTestCount!=0) return;

  // remove vertex and index buffer settings
  _vBufferLast = NULL;
  _vBufferSize = 0;
  _hwInDefinedState = false;
  //_d3DDevice->SetVertexShaderInputDirect(NULL, 0, NULL);

  // Defined index buffer
  _iBufferLast = NULL;
  _vOffsetLast = 0;
  _vsdVersionLast = VSDV_Basic;
  _d3DDevice->SetIndices(NULL, 0);
}

IVisibilityQuery *EngineDDXB::BegPixelCounting(IVisibilityQuery *query)
{
  XBVisibilityQuery *q = static_cast<XBVisibilityQuery *>(query);
  if (!q) q = new XBVisibilityQuery;
  // check the result of all queries which are currently running
  q->CheckResults(this);

  // get a new query ID - skip all id's currently used
  _d3DDevice->BeginVisibilityTest();
    
  return q;
}

void EngineDDXB::EndPixelCounting(IVisibilityQuery *query)
{
  XBVisibilityQuery *q = static_cast<XBVisibilityQuery *>(query);
  if (!q) return;

  int id = _ids.Open();
  if (id>=0)
  {
    _d3DDevice->EndVisibilityTest(id);
    VisibilityQueryStarted(q,id);
  }
  else
  {
    // in case of any failure, we use 512 to indicate an unused query
    _d3DDevice->EndVisibilityTest(512);
  }
}

IVisibilityQuery *EngineDDXB::QueryVisibility(
  IVisibilityQuery *query, ClipFlags clipFlags, const Vector3 *minmax, const FrameBase &pos
)
{
  // if object is very near of the camera, do not check its visibility
  const float minDistTest = 20;
  if (pos.Position().Distance2(_sceneProps->GetCameraPosition())<Square(minDistTest))
  {
    return NULL;
  }
  // check if camera is inside of the minmax box
  Matrix4 invTransform = pos.GetInvTransform();
  Vector3 modelSpaceCam = invTransform.FastTransform(_sceneProps->GetCameraPosition());
  if (
    modelSpaceCam.X()>=minmax[0].X() && modelSpaceCam.X()<=minmax[1].X() &&
    modelSpaceCam.Y()>=minmax[0].Y() && modelSpaceCam.Y()<=minmax[1].Y() &&
    modelSpaceCam.Z()>=minmax[0].Z() && modelSpaceCam.Z()<=minmax[1].Z()
    )
  {
    return NULL;
  }
  XBVisibilityQuery *q = static_cast<XBVisibilityQuery *>(query);
  if (!q) q = new XBVisibilityQuery;
  // check the result of all query which are currently running
  q->CheckResults(this);

//  static int maxLatency = 2;
//  if (q->Latency()<maxLatency)
  {
    // get a new query ID - skip all id's currently used
    int id = _ids.Open();
    if (id>=0)
    {
      SCOPE_GRF("visib",SCOPE_COLOR_GRAY);


      // make sure visibility testing was set up
      Assert(_visTestCount>0);
      BeginVisibilityTesting();

      SetupModelSpaceRendering(pos);
      SetPosScaleAndOffset(minmax[1]-minmax[0], minmax[0]);

      _d3DDevice->BeginVisibilityTest();
      _d3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP,0,12);
      _d3DDevice->EndVisibilityTest(id);

      EndVisibilityTesting();

      VisibilityQueryStarted(q,id);
    }
  }
//  else
//  {
//    GlobalShowMessage(100,"Query latency high");
//  }
  return q;
}

void EngineDDXB::DrawStencilShadows(bool interior, int quality)
{
  if (CanUseNewShadows())
  {
    PostprocessShadowVisualization(interior);
    //PostprocessShadowVisualizationAIS();
  }
  else
  {
    Fail("Error: Only new 32 bit shadows are supported");
  }
}

void EngineDDXB::PreparePostTexturesDefault()
{
}
void EngineDDXB::CleanupPostTexturesDefault()
{
}

/// prepare depth of field blur
void EngineDDXB::PreparePostTexturesBlur()
{
  D3DSetTextureStageState(2, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
  D3DSetTextureStageState(2, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
  D3DSetTextureStageState(3, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
  D3DSetTextureStageState(3, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
  //int stage23filter = D3DTEXF_LINEAR;
  int stage23filter = D3DTEXF_LINEAR;
  D3DSetTextureStageState(2, D3DTSS_MINFILTER, stage23filter);
  D3DSetTextureStageState(2, D3DTSS_MAGFILTER, stage23filter);
  D3DSetTextureStageState(2, D3DTSS_MIPFILTER, D3DTEXF_POINT);
  D3DSetTextureStageState(3, D3DTSS_MINFILTER, stage23filter);
  D3DSetTextureStageState(3, D3DTSS_MAGFILTER, stage23filter);
  D3DSetTextureStageState(3, D3DTSS_MIPFILTER, D3DTEXF_POINT);
  _d3DDevice->SetTexture(2, &_backBufferTexture);
  _d3DDevice->SetTexture(3, &_backBufferTexture);
  _lastHandle[2] = &_backBufferTexture;
  _lastHandle[3] = &_backBufferTexture;
  _lastTexture[2] = NULL;
  _lastTexture[3] = NULL;
}

void EngineDDXB::CleanupPostTexturesBlur()
{
  _d3DDevice->SetTexture(2, NULL);
  _d3DDevice->SetTexture(3, NULL);
  _lastHandle[2] = NULL;
  _lastHandle[3] = NULL;

  for (int i=1; i<3; i++)
  {
    D3DSetTextureStageState(1, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    D3DSetTextureStageState(1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    D3DSetTextureStageState(1, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
  }

  D3DSetTextureStageState(2, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
  D3DSetTextureStageState(2, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
  D3DSetTextureStageState(3, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
  D3DSetTextureStageState(3, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
}

void EngineDDXB::VisibilityQueryResultKnown(XBVisibilityQuery *q, int pixels)
{
  int index = q->_toStart++;
  if (q->_toStart>=XBVisibilityQuery::NResults) q->_toStart = 0;
  if (q->_status[index]==XBVisibilityQuery::QueryStarted)
  {
    LogF("Query became obsolete too fast");
    _ids.Close(q->_index[index]);
    q->_result[index] = 1; // we do not know the result - we must consider it true
  }
  // query never started - it is immediately done
  q->_status[index] = XBVisibilityQuery::QueryDone;
  q->_index[index] = 0; // no ID needed
}

void EngineDDXB::VisibilityQueryStarted(XBVisibilityQuery *q, int id)
{
  int index = q->_toStart++;
  if (q->_toStart>=XBVisibilityQuery::NResults) q->_toStart = 0;
  if (q->_status[index]==XBVisibilityQuery::QueryStarted)
  {
    LogF("Query became obsolete too fast");
    _ids.Close(q->_index[index]);
    q->_result[index] = 1; // we do not know the result - we must consider it true
  }
  q->_status[index] = XBVisibilityQuery::QueryStarted;
  q->_index[index] = id;
}

float EngineDDXB::GetLastKnownAvgIntensity(float &accomFactor) const
{
  accomFactor = _lastKnownAvgIntensityAccomFactor;
  return _lastKnownAvgIntensity;
}
void EngineDDXB::ForgetAvgIntensity()
{
  _lastKnownAvgIntensity = -1;
  // reset any pending queries - they are now obsolete
  for (int i=0; i<NBrightQueries; i++)
  {
    _brightQuery[i] = NULL;
  }
}

float EngineDDXB::GetHDRFactor() const
{
  static float hdrFactor = 0.64f;
  return hdrFactor;
}

// we want at least 20% to be left for HDR rendering
const float MinHDRRange = 0.8f;

float EngineDDXB::GetPreHDRBrightness() const
{
  float maxUsrBrightness = MinHDRRange/GetHDRFactor();
  return floatMin(_usrBrightness,maxUsrBrightness);
}
float EngineDDXB::GetPostHDRBrightness() const
{
//  float maxUsrBrightness = 0.8f/GetHDRFactor();
//  if (_usrBrightness<=maxUsrBrightness)
//  {
//    return 1;
//  }
//  return _usrBrightness/maxUsrBrightness;
  // optimized form of the code above
  if (_usrBrightness*GetHDRFactor()<=MinHDRRange)
  {
    return 1;
  }
  return _usrBrightness*GetHDRFactor()/MinHDRRange;
}

void EngineDDXB::PreparePostTexturesGlow()
{
  PreparePostprocessWorkBuffer();

  // use visibility testing together with alpha testing to check overall brightness
  // by controlling the number of pixels rendered we may we may issue as many probes as we want
  // render target does not matter - we do not render anything
  // buffer is already down-sampled, each sample is very light, using only stage 1
  D3DSetTextureStageState(1, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
  D3DSetTextureStageState(1, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
  D3DSetTextureStageState(1, D3DTSS_MINFILTER, D3DTEXF_POINT);
  D3DSetTextureStageState(1, D3DTSS_MAGFILTER, D3DTEXF_POINT);
  D3DSetTextureStageState(1, D3DTSS_MIPFILTER, D3DTEXF_POINT);
  _d3DDevice->SetTexture(1, _brightBuffer);
  _lastHandle[1] = _brightBuffer;
  _lastTexture[1] = NULL;
  
  // disable any render target writes
  //D3DSetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
  D3DSetRenderState(D3DRS_COLORWRITEENABLE, 0);
  // z/stencil should already be disabled
  // set alpha testing - this is a core of this operation

  D3DSetRenderState(D3DRS_ALPHATESTENABLE,TRUE);
  D3DSetTextureStageState(0,D3DTSS_ALPHAKILL,D3DTALPHAKILL_DISABLE);

  const int xSamples = 40;
  const int ySamples = 30;
  
  for (int b=0; b<NBrightQueries; b++)
  {
    if (!_brightQuery[b]) _brightQuery[b] = new XBVisibilityQuery;
  }

  // each level included all following levels
  struct
  {
    int minAlpha,maxAlpha;
    float pixels;
  } histogram[NBrightQueries+2];
  
  // 
  //const int minAlpha = 0x10;
  //const int maxAlpha = 0xa0;
  
  static const struct {int thold; float coef;} histPars[NBrightQueries+1]=
  {
    {0x10,1.0},
    {0x20,1.0},
    {0x28,1.0},
    {0x30,1.0},
    {0x38,1.0},
    {0x40,1.0},
    {0x48,1.0},
    {0x50,1.0},
    {0x58,1.0},
    {0x60,1.0},
    {0x68,1.0},
    {0x70,1.2},
    {0x78,1.5},
    {0x80,2.0},
    {0x88,2.0},
    {0x90,3.0},
    {0xff,4.0}, // no query ever issued for this threshold
  };
  
  bool valid = true;
  for (int b=0; b<NBrightQueries; b++)
  {
    IVisibilityQuery *q = _brightQuery[b];
    histogram[b+1].minAlpha = histPars[b].thold;
    histogram[b+1].maxAlpha = histPars[b+1].thold;
    int pixelsVisible = q->VisiblePixels();;
    histogram[b+1].pixels = pixelsVisible;
    if (pixelsVisible<0) valid = false;
  }
  
  if (valid)
  {
    float totalAlpha = 0;
    // all pixels are over 0 limit
    histogram[0].minAlpha = 0;
    histogram[0].maxAlpha = histogram[1].minAlpha;
    histogram[0].pixels = xSamples*ySamples;
    histogram[NBrightQueries].maxAlpha = 1;
    // no pixels are over 1 limit
    histogram[NBrightQueries+1].minAlpha = 0xff;
    histogram[NBrightQueries+1].maxAlpha = 0xff;
    histogram[NBrightQueries+1].pixels = 0;
    for (int b=0; b<NBrightQueries+1; b++)
    {
      int alphaMin = histogram[b].minAlpha;
      int alphaMax = histogram[b].maxAlpha;
      // exclude brighter levels
      float thisCount = histogram[b].pixels - histogram[b+1].pixels;
      float sumAlpha = thisCount*((alphaMin+alphaMax)*0.5f)*histPars[b].coef;
      totalAlpha += sumAlpha;
    }
    // experimental: normal intensity is around 0.2
    _lastKnownAvgIntensity = totalAlpha/((float(xSamples*ySamples)*255)/5);
    _lastKnownAvgIntensityAccomFactor = _accomFactor;
    #if 0 // _PROFILE || _DEBUG
      DIAG_MESSAGE(500,Format("Avg %.3f, factor %.2f",_lastKnownAvgIntensity,_accomFactor));
    #endif
  }
  
  #if _ENABLE_CHEATS
    if (CHECK_DIAG(DEHDR))
    {
      BString<256> msg;
      strcpy(msg,"Hist ");
      for (int b=0; b<NBrightQueries; b++)
      {
        int alpha = histPars[b].thold;
        float visible = (histogram[b].pixels-histogram[b+1].pixels)/float(xSamples*ySamples);
        BString<64> buf;
        sprintf(buf,"%.2f:%.2f ",alpha/255.0,visible);
        strcat(msg,buf);
      }
      
      DIAG_MESSAGE(500,msg);
      LogF("%s, avg %.3f, accom %.3f",cc_cast(msg),_lastKnownAvgIntensity,_lastKnownAvgIntensityAccomFactor);
    }
  #endif

  {  
    SCOPE_GRF("brightness",SCOPE_COLOR_GREEN);
    
    float bright = GetPreHDRBrightness()*GetHDRFactor()*2;
    for (int b=0; b<NBrightQueries; b++)
    {
      XBVisibilityQuery *q = static_cast<XBVisibilityQuery *>(_brightQuery[b].GetRef());

      int alpha = toInt(histPars[b].thold*bright);
      if (alpha>=255)
      {
        // no need to issue query like this - we know the result
        VisibilityQueryResultKnown(q,xSamples*ySamples);
      }
      else
      {
        D3DSetRenderState(D3DRS_ALPHAREF,alpha);
            
        int id = _ids.Open();
        if (id>=0)
        {
          
          _d3DDevice->LoadVertexShader(_shaderNonTLTex4Handle, 0);
          _d3DDevice->SelectVertexShader(NULL, 0);
          //_d3DDevice->SetVertexShader(_shaderNonTLTex4Handle);
          _d3DDevice->SetPixelShader(_psPostBrightQuery);
          
          _d3DDevice->BeginVisibilityTest();
          _d3DDevice->Begin(D3DPT_QUADLIST);
          {
            // draw a single quad, input format is SVertexVSNonTLTex4Decl
            _d3DDevice->SetVertexDataColor(3,PackedWhite);
            _d3DDevice->SetVertexDataColor(4,PackedWhite);

            // size of work buffer
            const float wScaled = _w/16;
            const float hScaled = _h/16;
            const float posOffset = -0.5f;
            const float uvOffset = 0;

            _d3DDevice->SetVertexData2f(9 ,0+uvOffset  ,0+uvOffset  );
            _d3DDevice->SetVertexData2f(10,0+uvOffset  ,0+uvOffset  );
            _d3DDevice->SetVertexData2f(11,0+uvOffset  ,0+uvOffset+2);
            _d3DDevice->SetVertexData2f(12,0+uvOffset+2,0+uvOffset+2);
            _d3DDevice->SetVertexData4f(-1,posOffset,posOffset,0.5,1);

            _d3DDevice->SetVertexData2f(9 ,_w+uvOffset       ,0+uvOffset  );
            _d3DDevice->SetVertexData2f(10,wScaled+uvOffset  ,0+uvOffset  );
            _d3DDevice->SetVertexData2f(11,wScaled+uvOffset  ,0+uvOffset+2);
            _d3DDevice->SetVertexData2f(12,wScaled+uvOffset+2,0+uvOffset+2);
            _d3DDevice->SetVertexData4f(-1,posOffset+xSamples,posOffset,0.5,1);

            _d3DDevice->SetVertexData2f(9 ,wScaled+uvOffset  ,hScaled+uvOffset  );
            _d3DDevice->SetVertexData2f(10,wScaled+uvOffset  ,hScaled+uvOffset  );
            _d3DDevice->SetVertexData2f(11,wScaled+uvOffset  ,hScaled+uvOffset+2);
            _d3DDevice->SetVertexData2f(12,wScaled+uvOffset+2,hScaled+uvOffset+2);
            _d3DDevice->SetVertexData4f(-1,posOffset+xSamples,posOffset+ySamples,0.5,1);

            _d3DDevice->SetVertexData2f(9 ,0+uvOffset  ,hScaled+uvOffset  );
            _d3DDevice->SetVertexData2f(10,0+uvOffset  ,hScaled+uvOffset  );
            _d3DDevice->SetVertexData2f(11,0+uvOffset  ,hScaled+uvOffset+2);
            _d3DDevice->SetVertexData2f(12,0+uvOffset+2,hScaled+uvOffset+2);
            _d3DDevice->SetVertexData4f(-1,posOffset,posOffset+ySamples,0.5,1);
          }
          _d3DDevice->End();
          _d3DDevice->LoadVertexShader(_shaderNonTLHandle, 0);
          _d3DDevice->SelectVertexShader(NULL, 0);
          //_d3DDevice->SetVertexShader(_shaderNonTLHandle);

          _d3DDevice->EndVisibilityTest(id);

          VisibilityQueryStarted(q, id);
        }
      }
    }
  }

    
  
  // enable framebuffer writes again
  D3DSetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

  // use framebuffer as a texture 0
  D3DSetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
  D3DSetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
  D3DSetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_POINT);
  D3DSetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
  D3DSetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_POINT);
  _d3DDevice->SetTexture(0, &_backBufferTexture);
  _lastHandle[0] = &_backBufferTexture;
  _lastTexture[0] = NULL;
  
  // for any effects which follow map a glow buffer as a texture
  for (int i=1; i<4; i++)
  {
    D3DSetTextureStageState(i, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
    D3DSetTextureStageState(i, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
    D3DSetTextureStageState(i, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    D3DSetTextureStageState(i, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    D3DSetTextureStageState(i, D3DTSS_MIPFILTER, D3DTEXF_POINT);
    _d3DDevice->SetTexture(i, _postFxBuffer);
    _lastHandle[i] = _postFxBuffer;
    _lastTexture[i] = NULL;
  }

  D3DSetRenderState(D3DRS_ALPHAREF, 0x7F);
  D3DSetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
}

void EngineDDXB::CleanupPostTexturesGlow()
{
  for (int i=0; i<4; i++)
  {
    _d3DDevice->SetTexture(i, NULL);
    _lastHandle[i] = NULL;
    D3DSetTextureStageState(1, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    D3DSetTextureStageState(1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    D3DSetTextureStageState(1, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
  }

  D3DSetTextureStageState(1, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
  D3DSetTextureStageState(1, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
  D3DSetTextureStageState(2, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
  D3DSetTextureStageState(2, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
  D3DSetTextureStageState(3, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
  D3DSetTextureStageState(3, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
}

void EngineDDXB::PreparePostprocess()
{
  FlushQueues();
  SwitchTL(false);

  // create a backbuffer texture header
  {
    ComRef<IDirect3DSurface8> surface;
    _d3DDevice->GetRenderTarget(surface.Init());
    D3DSURFACE_DESC desc;
    surface->GetDesc(&desc);
    memset(&_backBufferTexture, 0, sizeof(_backBufferTexture));
    XGSetTextureHeader(
      desc.Width, desc.Height, 1, 0,
      desc.Format, 0, &_backBufferTexture, surface->Data,
      desc.Width * _pixelSize/8
      );
  }

  // no alpha blending - we will do everything in the pixel shader
  EngineShapeProperties prop;
  DoPrepareTriangle(NULL,NULL,0,NoZBuf|ClampU|ClampV|FilterPoint,prop);
  // we may need alpha write, though

  // set backbuffer texture  
  _d3DDevice->SetTexture(0, &_backBufferTexture);
  // reset texturing info - to force texture setting again
  _lastHandle[0] = &_backBufferTexture;
  _lastTexture[0] = NULL;

  _hwInDefinedState = false;
  _currPixelShaderSel = PSUninitialized;

}

bool EngineDDXB::PreparePostprocessDepth()
{
  ComRef<IDirect3DSurface8> surface;
  _d3DDevice->GetDepthStencilSurface(surface.Init());
  D3DSURFACE_DESC desc;
  surface->GetDesc(&desc);
  memset(&_depthBufferTexture, 0, sizeof(_depthBufferTexture));
  if (desc.Format!=D3DFMT_LIN_D24S8) return false;
  D3DFORMAT texFormat = D3DFMT_LIN_A8R8G8B8;
  XGSetTextureHeader(
    desc.Width, desc.Height, 1, 0,
    texFormat, 0, &_depthBufferTexture, surface->Data,
    desc.Width * 4
    );

  _d3DDevice->SetTexture(1, &_depthBufferTexture);
  _lastHandle[1] = &_depthBufferTexture;
  _lastTexture[1] = NULL;

  D3DSetTextureStageState(1, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
  D3DSetTextureStageState(1, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
  D3DSetTextureStageState(1, D3DTSS_MAGFILTER, D3DTEXF_POINT);
  D3DSetTextureStageState(1, D3DTSS_MINFILTER, D3DTEXF_POINT);
  D3DSetTextureStageState(1, D3DTSS_MIPFILTER, D3DTEXF_POINT);

  return true;
}

void EngineDDXB::CleanUpPostprocessDepth()
{
  _d3DDevice->SetTexture(1, NULL);
  _lastHandle[1] = NULL;

  memset(&_depthBufferTexture, 0, sizeof(_depthBufferTexture));

  D3DSetTextureStageState(1, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
  D3DSetTextureStageState(1, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
  D3DSetTextureStageState(1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
  D3DSetTextureStageState(1, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
  D3DSetTextureStageState(1, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);

}


void EngineDDXB::PreparePostprocessWorkBuffer()
{
  // we may issue 4 probes at a time (4 tex samples)
  // each probe may be 2x2 samples (bilinear interpolation)
  for (int i=0; i<4; i++)
  {
    D3DSetTextureStageState(i, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
    D3DSetTextureStageState(i, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
    D3DSetTextureStageState(i, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    D3DSetTextureStageState(i, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    D3DSetTextureStageState(i, D3DTSS_MIPFILTER, D3DTEXF_POINT);
    _d3DDevice->SetTexture(i, &_backBufferTexture);
    _lastHandle[i] = &_backBufferTexture;
    _lastTexture[i] = NULL;
  }

  ComRef<IDirect3DSurface8> currRender,currDepth;
  _d3DDevice->GetRenderTarget(currRender.Init());
  _d3DDevice->GetDepthStencilSurface(currDepth.Init());
  // do a first postprocess pass - copy everything from backbuffer to workbuffer
  ComRef<IDirect3DSurface8> postFxSurface;
  _postFxBuffer->GetSurfaceLevel(0,postFxSurface.Init());
  _d3DDevice->SetRenderTarget(postFxSurface,currDepth);
  // create a work buffer (back buffer scaled down 4x)
  _d3DDevice->SetPixelShader(_psPostCopy);

  float glowThold = GetPreHDRBrightness()*GetHDRFactor()*0.75f;

  // TODO: glow color sensitivity may depend on eye adaptation / night visions used
  float glowPars[4]={R_EYE,G_EYE,B_EYE,glowThold};
  _d3DDevice->SetPixelShaderConstant(POST_FX_PARAM,glowPars,1);
  // render from backbuffer to the helper buffer
  // no z/stencil op. needed or wanted
  D3DSetRenderState(D3DRS_STENCILENABLE,FALSE);
  // we want to get blurred image - use linear filter to get it
  for (int i=1; i<4; i++)
  {
    D3DSetTextureStageState(i, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    D3DSetTextureStageState(i, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    D3DSetTextureStageState(i, D3DTSS_MIPFILTER, D3DTEXF_POINT);
  }
  _d3DDevice->LoadVertexShader(_shaderNonTLTex4Handle, 0);
  _d3DDevice->SelectVertexShader(NULL, 0);
  _d3DDevice->Begin(D3DPT_QUADLIST);
  {
    // draw a single quad, input format is SVertexVSNonTLDecl
    _d3DDevice->SetVertexDataColor(3,PackedWhite);
    _d3DDevice->SetVertexDataColor(4,PackedWhite);

    const float posOffset = -0.5f;
    const float uvOffset = 0;
    _d3DDevice->SetVertexData2f(9 ,0+uvOffset  ,0+uvOffset  );
    _d3DDevice->SetVertexData2f(10,0+uvOffset+2,0+uvOffset  );
    _d3DDevice->SetVertexData2f(11,0+uvOffset  ,0+uvOffset+2);
    _d3DDevice->SetVertexData2f(12,0+uvOffset+2,0+uvOffset+2);
    _d3DDevice->SetVertexData4f(-1,posOffset,posOffset,0.5,1);

    _d3DDevice->SetVertexData2f(9 ,_w+uvOffset  ,0+uvOffset  );
    _d3DDevice->SetVertexData2f(10,_w+uvOffset+2,0+uvOffset  );
    _d3DDevice->SetVertexData2f(11,_w+uvOffset  ,0+uvOffset+2);
    _d3DDevice->SetVertexData2f(12,_w+uvOffset+2,0+uvOffset+2);
    _d3DDevice->SetVertexData4f(-1,posOffset+_w/4,posOffset,0.5,1);

    _d3DDevice->SetVertexData2f(9 ,_w+uvOffset  ,_h+uvOffset  );
    _d3DDevice->SetVertexData2f(10,_w+uvOffset+2,_h+uvOffset  );
    _d3DDevice->SetVertexData2f(11,_w+uvOffset  ,_h+uvOffset+2);
    _d3DDevice->SetVertexData2f(12,_w+uvOffset+2,_h+uvOffset+2);
    _d3DDevice->SetVertexData4f(-1,posOffset+_w/4,posOffset+_h/4,0.5,1);

    _d3DDevice->SetVertexData2f(9 ,0+uvOffset  ,_h+uvOffset  );
    _d3DDevice->SetVertexData2f(10,0+uvOffset+2,_h+uvOffset  );
    _d3DDevice->SetVertexData2f(11,0+uvOffset  ,_h+uvOffset+2);
    _d3DDevice->SetVertexData2f(12,0+uvOffset+2,_h+uvOffset+2);
    _d3DDevice->SetVertexData4f(-1,posOffset,posOffset+_h/4,0.5,1);
  }
  _d3DDevice->End();

  ComRef<IDirect3DSurface8> brightSurface;
  _brightBuffer->GetSurfaceLevel(0,brightSurface.Init());
  _d3DDevice->SetRenderTarget(brightSurface,currDepth);

  // scale down once more
  // we may issue 4 probes at a time (4 tex samples)
  // each probe may be 2x2 samples (bilinear interpolation)
  for (int i=0; i<4; i++)
  {
    D3DSetTextureStageState(i, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
    D3DSetTextureStageState(i, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
    D3DSetTextureStageState(i, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    D3DSetTextureStageState(i, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    D3DSetTextureStageState(i, D3DTSS_MIPFILTER, D3DTEXF_POINT);
    _d3DDevice->SetTexture(i, _postFxBuffer);
    _lastHandle[i] = _postFxBuffer;
    _lastTexture[i] = NULL;
  }
  
  _d3DDevice->LoadVertexShader(_shaderNonTLTex4Handle, 0);
  _d3DDevice->SelectVertexShader(NULL, 0);
  _d3DDevice->Begin(D3DPT_QUADLIST);
  {
    // draw a single quad, input format is SVertexVSNonTLDecl
    _d3DDevice->SetVertexDataColor(3,PackedWhite);
    _d3DDevice->SetVertexDataColor(4,PackedWhite);

    const float posOffset = -0.5f;
    const float uvOffset = 0;
    _d3DDevice->SetVertexData2f(9 ,0+uvOffset  ,0+uvOffset  );
    _d3DDevice->SetVertexData2f(10,0+uvOffset+2,0+uvOffset  );
    _d3DDevice->SetVertexData2f(11,0+uvOffset  ,0+uvOffset+2);
    _d3DDevice->SetVertexData2f(12,0+uvOffset+2,0+uvOffset+2);
    _d3DDevice->SetVertexData4f(-1,posOffset,posOffset,0.5,1);

    _d3DDevice->SetVertexData2f(9 ,_w/4+uvOffset  ,0+uvOffset  );
    _d3DDevice->SetVertexData2f(10,_w/4+uvOffset+2,0+uvOffset  );
    _d3DDevice->SetVertexData2f(11,_w/4+uvOffset  ,0+uvOffset+2);
    _d3DDevice->SetVertexData2f(12,_w/4+uvOffset+2,0+uvOffset+2);
    _d3DDevice->SetVertexData4f(-1,posOffset+_w/16,posOffset,0.5,1);

    _d3DDevice->SetVertexData2f(9 ,_w/4+uvOffset  ,_h/4+uvOffset  );
    _d3DDevice->SetVertexData2f(10,_w/4+uvOffset+2,_h/4+uvOffset  );
    _d3DDevice->SetVertexData2f(11,_w/4+uvOffset  ,_h/4+uvOffset+2);
    _d3DDevice->SetVertexData2f(12,_w/4+uvOffset+2,_h/4+uvOffset+2);
    _d3DDevice->SetVertexData4f(-1,posOffset+_w/16,posOffset+_h/16,0.5,1);

    _d3DDevice->SetVertexData2f(9 ,0+uvOffset  ,_h/4+uvOffset  );
    _d3DDevice->SetVertexData2f(10,0+uvOffset+2,_h/4+uvOffset  );
    _d3DDevice->SetVertexData2f(11,0+uvOffset  ,_h/4+uvOffset+2);
    _d3DDevice->SetVertexData2f(12,0+uvOffset+2,_h/4+uvOffset+2);
    _d3DDevice->SetVertexData4f(-1,posOffset,posOffset+_h/16,0.5,1);
  }
  _d3DDevice->End();

  
  _d3DDevice->LoadVertexShader(_shaderNonTLHandle, 0);
  _d3DDevice->SelectVertexShader(NULL, 0);
  D3DSetRenderState(D3DRS_STENCILENABLE,TRUE);


  
  _d3DDevice->SetRenderTarget(currRender,currDepth);
}

void EngineDDXB::CleanUpPostprocessWorkBuffer()
{
  _d3DDevice->SetTexture(2, NULL);
  _d3DDevice->SetTexture(3, NULL);
  _lastHandle[2] = NULL;
  _lastHandle[3] = NULL;

  for (int i=1; i<3; i++)
  {
    D3DSetTextureStageState(1, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    D3DSetTextureStageState(1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    D3DSetTextureStageState(1, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
  }

  D3DSetTextureStageState(2, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
  D3DSetTextureStageState(2, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
  D3DSetTextureStageState(3, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
  D3DSetTextureStageState(3, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
} 

void EngineDDXB::DoPostprocess(int nPixels, float scale, float zValue)
{
  _d3DDevice->Begin(D3DPT_QUADLIST);
  {
    // draw a single quad, input format is SVertexVSNonTLDecl
    _d3DDevice->SetVertexDataColor(3,PackedWhite);
    _d3DDevice->SetVertexDataColor(4,PackedWhite);

    const float posOffset = -0.5f;
    _d3DDevice->SetVertexData2f(9,0,0);
    _d3DDevice->SetVertexData2f(10,0,0);
    _d3DDevice->SetVertexData4f(-1,posOffset,posOffset,zValue,1);

    _d3DDevice->SetVertexData2f(9,_w,0);
    _d3DDevice->SetVertexData2f(10,_w,0);
    _d3DDevice->SetVertexData4f(-1,posOffset+_w,posOffset,zValue,1);

    _d3DDevice->SetVertexData2f(9,_w,_h);
    _d3DDevice->SetVertexData2f(10,_w,_h);
    _d3DDevice->SetVertexData4f(-1,posOffset+_w,posOffset+_h,zValue,1);

    _d3DDevice->SetVertexData2f(9,0,_h);
    _d3DDevice->SetVertexData2f(10,0,_h);
    _d3DDevice->SetVertexData4f(-1,posOffset,posOffset+_h,zValue,1);
  }
  _d3DDevice->End();
}

void EngineDDXB::DoPostprocessGlow(int nPixels, float scale, float zValue)
{
  _d3DDevice->LoadVertexShader(_shaderNonTLTex4Handle, 0);
  _d3DDevice->SelectVertexShader(NULL, 0);
  //_d3DDevice->SetVertexShader(_shaderNonTLTex4Handle);
  _d3DDevice->Begin(D3DPT_TRIANGLELIST);
  {
    // draw a single quad, input format is SVertexVSNonTLTex4Decl
    _d3DDevice->SetVertexDataColor(3,PackedWhite);
    _d3DDevice->SetVertexDataColor(4,PackedWhite);

    float wScaled = _w*scale*2;
    float hScaled = _h*scale*2;
    const float posOffset = -0.5f;
    const float uvOffset = 0;
    //int nPixelsY = 1;
    int nPixelsY = nPixels;
    _d3DDevice->SetVertexData2f(9,0,_h*2);
    _d3DDevice->SetVertexData2f(10,0+uvOffset,hScaled-nPixelsY+uvOffset);
    _d3DDevice->SetVertexData2f(11,0-nPixels+uvOffset,hScaled+uvOffset);
    _d3DDevice->SetVertexData2f(12,0+nPixels+uvOffset,hScaled+uvOffset);
    _d3DDevice->SetVertexData4f(-1,posOffset,posOffset+_h*2,zValue,1);

    _d3DDevice->SetVertexData2f(9,0,0);
    _d3DDevice->SetVertexData2f(10,0+uvOffset,0-nPixelsY+uvOffset);
    _d3DDevice->SetVertexData2f(11,0-nPixels+uvOffset,0+uvOffset);
    _d3DDevice->SetVertexData2f(12,0+nPixels+uvOffset,0+uvOffset);
    _d3DDevice->SetVertexData4f(-1,posOffset,posOffset,zValue,1);

    _d3DDevice->SetVertexData2f(9,_w*2,0);
    _d3DDevice->SetVertexData2f(10,wScaled+uvOffset,0-nPixelsY+uvOffset);
    _d3DDevice->SetVertexData2f(11,wScaled-nPixels+uvOffset,uvOffset);
    _d3DDevice->SetVertexData2f(12,wScaled+nPixels+uvOffset,0+uvOffset);
    _d3DDevice->SetVertexData4f(-1,posOffset+_w*2,posOffset,zValue,1);
  }
  _d3DDevice->End();
  _d3DDevice->LoadVertexShader(_shaderNonTLHandle, 0);
  _d3DDevice->SelectVertexShader(NULL, 0);
  //_d3DDevice->SetVertexShader(_shaderNonTLHandle);
}

/// depth of field blur
void EngineDDXB::DoPostprocessBlur(int nPixels, float scale)
{
  _d3DDevice->LoadVertexShader(_shaderNonTLTex4Handle, 0);
  _d3DDevice->SelectVertexShader(NULL, 0);
  //_d3DDevice->SetVertexShader(_shaderNonTLTex4Handle);
  _d3DDevice->Begin(D3DPT_TRIANGLELIST);
  {
    // draw a single quad, input format is SVertexVSNonTLTex4Decl
    _d3DDevice->SetVertexDataColor(3,PackedWhite);
    _d3DDevice->SetVertexDataColor(4,PackedWhite);

    float wScaled = _w*scale*2;
    float hScaled = _h*scale*2;
    const float posOffset = -0.5f;
    const float uvOffset1 = 1;
    const float uvOffset2 = nPixels + 1;
    _d3DDevice->SetVertexData2f(9,0,_h*2);
    _d3DDevice->SetVertexData2f(10,0,hScaled);
    _d3DDevice->SetVertexData2f(11,0+uvOffset1,hScaled+uvOffset2);
    _d3DDevice->SetVertexData2f(12,+uvOffset2,hScaled+uvOffset1);
    _d3DDevice->SetVertexData4f(-1,posOffset,posOffset+_h*2,0.5,1);

    _d3DDevice->SetVertexData2f(9,0,0);
    _d3DDevice->SetVertexData2f(10,0,0);
    _d3DDevice->SetVertexData2f(11,0+uvOffset1,uvOffset2);
    _d3DDevice->SetVertexData2f(12,uvOffset2,0+uvOffset1);
    _d3DDevice->SetVertexData4f(-1,posOffset,posOffset,0.5,1);

    _d3DDevice->SetVertexData2f(9,_w*2,0);
    _d3DDevice->SetVertexData2f(10,wScaled,0);
    _d3DDevice->SetVertexData2f(11,wScaled+uvOffset1,uvOffset2);
    _d3DDevice->SetVertexData2f(12,wScaled+uvOffset2,0+uvOffset1);
    _d3DDevice->SetVertexData4f(-1,posOffset+_w*2,posOffset,0.5,1);
  }
  _d3DDevice->End();
  _d3DDevice->LoadVertexShader(_shaderNonTLHandle, 0);
  _d3DDevice->SelectVertexShader(NULL, 0);
  //_d3DDevice->SetVertexShader(_shaderNonTLHandle);
}

void EngineDDXB::DoPostprocessWorkBuff()
{
  _d3DDevice->LoadVertexShader(_shaderNonTLTex4Handle, 0);
  _d3DDevice->SelectVertexShader(NULL, 0);
  //_d3DDevice->SetVertexShader(_shaderNonTLTex4Handle);
  _d3DDevice->Begin(D3DPT_QUADLIST);
  {
    // draw a single quad, input format is SVertexVSNonTLTex4Decl
    _d3DDevice->SetVertexDataColor(3,PackedWhite);
    _d3DDevice->SetVertexDataColor(4,PackedWhite);

    const float posOffset = -0.5f;
    _d3DDevice->SetVertexData2f(9,0,0);
    _d3DDevice->SetVertexData2f(10,0,0);
    _d3DDevice->SetVertexData2f(11,0,0);
    _d3DDevice->SetVertexData2f(12,0,0);
    _d3DDevice->SetVertexData4f(-1,posOffset,posOffset,0.5,1);

    _d3DDevice->SetVertexData2f(9,_w,0);
    _d3DDevice->SetVertexData2f(10,_w,0);
    _d3DDevice->SetVertexData2f(11,_w/4,0);
    _d3DDevice->SetVertexData2f(12,_w/4,0);
    _d3DDevice->SetVertexData4f(-1,posOffset+_w,posOffset,0.5,1);

    _d3DDevice->SetVertexData2f(9,_w,_h);
    _d3DDevice->SetVertexData2f(10,_w,_h);
    _d3DDevice->SetVertexData2f(11,_w/4,_h/4);
    _d3DDevice->SetVertexData2f(12,_w/4,_h/4);
    _d3DDevice->SetVertexData4f(-1,posOffset+_w,posOffset+_h,0.5,1);

    _d3DDevice->SetVertexData2f(9,0,_h);
    _d3DDevice->SetVertexData2f(10,0,_h);
    _d3DDevice->SetVertexData2f(11,0,_h/4);
    _d3DDevice->SetVertexData2f(12,0,_h/4);
    _d3DDevice->SetVertexData4f(-1,posOffset,posOffset+_h,0.5,1);
  }
  _d3DDevice->End();
  _d3DDevice->LoadVertexShader(_shaderNonTLHandle, 0);
  _d3DDevice->SelectVertexShader(NULL, 0);
  //_d3DDevice->SetVertexShader(_shaderNonTLHandle);
}

void EngineDDXB::CleanUpPostprocess()
{
  // reset state as much as possible
  _d3DDevice->SetTexture(0, NULL);
  _lastHandle[0] = NULL;

  memset(&_backBufferTexture, 0, sizeof(_backBufferTexture));
  // TL is still disabled, besides of texures 2D is fully set-up and ready
}

void EngineDDXB::PostprocessFx(
  int pixelShader, int fxPars, int nPixels, float scale,
  void (EngineDDXB::*doPostprocess)(int nPixels, float scale, float zValue),
  void (EngineDDXB::*prepareTextures)(),
  void (EngineDDXB::*cleanupTextures)()
)
{

  PreparePostprocess();
  (this->*prepareTextures)();
  _d3DDevice->SetPixelShader(pixelShader);
  if (fxPars>=0)
  {
    _d3DDevice->SetPixelShaderConstant(POST_FX_PARAM,_postFxParam[fxPars], 1);
  }

  (this->*doPostprocess)(nPixels,scale,0.5);
  CleanUpPostprocess();
  (this->*cleanupTextures)();
}

void EngineDDXB::PostprocessGlow()
{
  //if (!_postFxEnabled[PFXGlow] || _postFxParam[PFXGlow][3]<=0.001f) return;
  if (!_postFxEnabled[PFXGlow]) return;
  SCOPE_GRF("postGlow",SCOPE_COLOR_YELLOW);
  
  float postHDR = GetPostHDRBrightness();
  
  _postFxParam[PFXGlow][0] =       
  _postFxParam[PFXGlow][1] = 
  _postFxParam[PFXGlow][2] = postHDR*0.25f/GetHDRFactor(); // HDR multiplier
  _postFxParam[PFXGlow][3] = 0.5f; // glow intensity
  PostprocessFx(
    _psPostGlow,PFXGlow,1,0.25f,
    DoPostprocessGlow,PreparePostTexturesGlow,CleanupPostTexturesGlow
  );
  
}

void EngineDDXB::PostprocessNight()
{
  if (!_postFxEnabled[PFXNight] || _postFxParam[PFXNight][3]>=0.999f) return;
  SCOPE_GRF("postNight",SCOPE_COLOR_GRAY);
  PostprocessFx(_psPostNight,PFXNight,0,1);
}
void EngineDDXB::PostprocessDepthOfField()
{
  if (!_postFxEnabled[PFXDepthOfField]) return;

  // process only once
  _postFxEnabled[PFXDepthOfField] = false;

  SCOPE_GRF("postDOF",SCOPE_COLOR_GRAY);
  // process all pixels

  PreparePostprocess();

  if (PreparePostprocessDepth())
  {
    // k = - (D3DZ_MAX_D24S8*(q - q*cNear)/z0 -1)*16

    Camera *camera = GScene->GetCamera();
    float cNear = camera->ClipNear();
    float distance = floatMax(_postFxParam[PFXDepthOfField][3],cNear);
    float blur = _postFxParam[PFXDepthOfField][2];

    Matrix4Val project = _sceneProps->GetCameraProjection();

    //k = - ((q - q*cNear)/z0 -1)*16
    float zz = project(2,2)+project.Position()[2]*1/distance;
    float k = - (zz -1)*16;


    if (_depthOfFieldFarOnly)
    {
      const float fxParam[4] = {blur,blur,blur,k};
      
      _d3DDevice->SetPixelShader(_psPostDepthOfFieldFarOnly);
      _d3DDevice->SetPixelShaderConstant(POST_FX_PARAM,fxParam, 1);
    }
    else
    {
      const float fxParam[4] = {blur,blur,blur,k};
      _d3DDevice->SetPixelShader(_psPostDepthToA);
      _d3DDevice->SetPixelShaderConstant(POST_FX_PARAM,fxParam, 1);

      // helper pass - write depth result to alpha

      D3DSetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA);
      DoPostprocess(0,1);
      
      _d3DDevice->SetPixelShader(_psPostDepthOfField);
      _d3DDevice->SetPixelShaderConstant(POST_FX_PARAM,_postFxParam[PFXDepthOfField], 1);
    }

    // alpha will contain blur factor after this pass - this can be helpfull for PIX debugging
    D3DSetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
    

    PreparePostTexturesBlur();

    
    int nPixels;
    if (_depthOfFieldFarOnly)
    {
      // we use the effect only as a slight blur
      nPixels = 1;
    }
    else
    {
      // pixel offset should depend on k
      // k close to 0 means focus close to infinity
      // k close to 1 means focus close to zero
      const float maxK = 0;
      const float minK = 0.6f;
      const int maxOffset = 4;
      const int minOffset = 1;
      nPixels = maxOffset-toInt((k-maxK)/(minK-maxK)*(maxOffset-minOffset));
      if (nPixels<minOffset) nPixels = minOffset;
      if (nPixels>maxOffset) nPixels = maxOffset;
    }
    DoPostprocessBlur(nPixels,1);

    D3DSetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

    CleanUpPostprocessDepth();

    CleanupPostTexturesBlur();
    CleanUpPostprocess();
  }

  // set stenciling to a normal state again
  _currentShadowStateSettings = SS_None;
}

void EngineDDXB::Postprocess()
{
  FlushQueues();

  PostprocessGlow();
  PostprocessNight();
  PostprocessDepthOfField();
}

void EngineDDXB::PostprocessDepth()
{
  FlushQueues();

  // DOF fx makes sure it is executed only once
  PostprocessDepthOfField();
}

void EngineDDXB::PostprocessShadowVisualization(bool interior)
{
  SCOPE_GRF("postSV",SCOPE_COLOR_GRAY);

  PreparePostprocess();
  if (PreparePostprocessDepth())
  {
    _d3DDevice->SetPixelShader(_psPostShadowVisualisation);

    D3DSetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
    D3DSetRenderState(D3DRS_STENCILREF, 0);
    D3DSetRenderState(D3DRS_STENCILFUNC, D3DCMP_LESS);
    D3DSetRenderState(D3DRS_STENCILMASK, STENCIL_COUNTER);
    
    // we need to enable Z, otherwise fast path culling is not working at all
    D3DSetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
    D3DSetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    D3DSetRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);

    // we can use z-test to avoid areas which are out of shadow
    
    // Set the PS constants
    // Shadow factor
    float constants[4] = {
      _shadowFactor.R(),_shadowFactor.G(),_shadowFactor.B(),_shadowFactor.A()
    };
    _d3DDevice->SetPixelShaderConstant(POST_SHADOWVIS_PARAM, constants, 1);

    // Ramp mul and add coefficients
    Matrix4Val project = _sceneProps->GetCameraProjection();
    float backBorder = Glob.config.shadowsZ * 2.0f;
    float zz = project(2,2) + project.Position()[2] * 1.0f / backBorder;
    float tZ = (zz - (float)0xF00000/0xFFFFFF) * 16.0f;
    float mul = 1.0f;
    float add = tZ;
    float cmul[4] = {mul, mul, mul, mul};
    _d3DDevice->SetPixelShaderConstant(POST_SHADOWVIS_RAMPMUL, cmul, 1);
    float cadd[4] = {add, add, add, add};
    _d3DDevice->SetPixelShaderConstant(POST_SHADOWVIS_RAMPADD, cadd, 1);

    DoPostprocess(0,1,zz);
    CleanUpPostprocessDepth();
  }

  CleanUpPostprocess();

  // set stenciling to a normal state again
  _currentShadowStateSettings = SS_None;
  D3DSetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);

  D3DSetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
  D3DSetRenderState(D3DRS_ZWRITEENABLE, FALSE);
  D3DSetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
}

void EngineDDXB::PostprocessShadowVisualizationAIS()
{
  #if 0
  SCOPE_GRF("postSVAIS",SCOPE_COLOR_GRAY);

  PreparePostprocess();
  if (PreparePostprocessDepth())
  {
    _d3DDevice->SetPixelShader(_psPostShadowVisualisationAIS);

    D3DSetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
    D3DSetRenderState(D3DRS_STENCILREF, XXXXX);
    D3DSetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
    D3DSetRenderState(D3DRS_STENCILMASK, XXXXX);

    // Set the PS constants
    {
      // Shadow factor
      //float constants[4] = {0.0f, 0.0f, 0.0f, _shadowFactor * (1.0f/256.0f) * 0.75f};
      float constants[4] = {
        _shadowFactor.R(),_shadowFactor.G(),_shadowFactor.B(),_shadowFactor.A()
      };
      _d3DDevice->SetPixelShaderConstant(POST_SHADOWVIS_PARAM, constants, 1);

      // Ramp mul and add coefficients
      Matrix4Val project = _sceneProps->GetCameraProjection();
      float backBorder = Glob.config.shadowsZ * 2.0f;
      float zz = project(2,2) + project.Position()[2] * 1.0f / backBorder;
      float tZ = (zz - (float)0xF00000/0xFFFFFF) * 16.0f;
      float mul = 1.0f;
      float add = tZ;
      float cmul[4] = {mul, mul, mul, mul};
      _d3DDevice->SetPixelShaderConstant(POST_SHADOWVIS_RAMPMUL, cmul, 1);
      float cadd[4] = {add, add, add, add};
      _d3DDevice->SetPixelShaderConstant(POST_SHADOWVIS_RAMPADD, cadd, 1);
    }

    DoPostprocess(0,1);
    CleanUpPostprocessDepth();
  }

  CleanUpPostprocess();

  // set stenciling to a normal state again
  _currentShadowStateSettings = SS_None;
  D3DSetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
  #endif
}

bool EngineDDXB::CanUseNewShadows()
{
  return (_pixelSize == 32) ? true : false;
}

inline int FracAlpha( float a )
{
  int ia=toInt(a);
  saturate(ia,0,255);
  return ia;
}

void EngineDDXB::DrawPoints( int beg, int end )
{
  if (_resetNeeded) return;
  /*
  D3DPreparePoint();
  // TODO: queue point quads
  */
  //for( int i=beg; i<end; i++ ) QueuePoint(i);
  // now we are drawing quads directly
  for( int i=beg; i<end; i++ )
  {
    if (_mesh->Clip(i)&ClipAll) continue;

    const TLVertex &v = _mesh->GetVertex(i);
    PackedColor color=v.color;
    if( color.A8()<8 ) continue; // do not draw stars that are not visible

    // calculate alpha in TL, TR, BL, BR corners
    // v is used as TL corner
    // we have to simulate TR, BL, BR corners
    int xI = toIntFloor(v.pos[0]);
    int yI = toIntFloor(v.pos[1]);
    float xFrac = v.pos[0]-xI;
    float yFrac = v.pos[1]-yI;
    float ixFrac = 1-xFrac;
    float iyFrac = 1-yFrac;

    float a=color.A8();

    TLVertex vs[4];
    vs[0] = v; // TL
    vs[0].pos[0] = xI+0.5f;
    vs[0].pos[1] = yI+0.5f;
    vs[0].color = PackedColorRGB(color,FracAlpha(ixFrac*iyFrac*a));
    vs[0].specular = PackedColor(0);

    vs[1] = vs[0]; // TR
    vs[1].pos[0] = xI+2.5f;
    vs[1].color = PackedColorRGB(color,FracAlpha(xFrac*iyFrac*a));
    // 
    vs[2] = vs[1]; // BR
    vs[2].pos[1] = yI+2.5f;
    vs[2].color = PackedColorRGB(color,FracAlpha(xFrac*yFrac*a));

    vs[3] = vs[2]; // BL
    vs[3].pos[0] = vs[0].pos[0];
    vs[3].color = PackedColorRGB(color,FracAlpha(ixFrac*yFrac*a));

    static const VertexIndex indices[6]={0,1,2,3};

    AddVertices(vs,4);
    QueueFan(indices,4);
  }
}

void EngineDDXB::SetD3DFog( ColorVal fog )
{
  if( _d3DDevice )
  {
    const float scale = GetHDRFactor();
    D3DCOLOR fogColor=FAST_D3DRGBA_SAT(
      fog.R()*scale, fog.G()*scale, fog.B()*scale, 1.0f
    );
    if (_pixelSize==16) fogColor = CLIP_TO_565(fogColor);
    D3DSetRenderState(D3DRS_FOGCOLOR, fogColor);
  }
}

#if PIXEL_SHADERS
static const
#include "shaders/psDayNormal.h"
static const
#include "shaders/psDayNormalDXTA.h"
static const
#include "shaders/psDaySpecular.h"
static const
#include "shaders/psDayDetail.h"
static const
#include "shaders/psDayDetailSpecular.h"
static const
#include "shaders/psDaySpecularAlpha.h"
static const
#include "shaders/psDaySpecularNormalMap.h"
static const
#include "shaders/psDaySpecularNormalMapThrough.h"
static const
#include "shaders/psDayDetailSpecularAlpha.h"
static const
#include "shaders/psDayWater.h"
static const
#include "shaders/psDayWaterSimple.h"
static const
#include "shaders/psDaySpecularNormalMapDiffuse.h"
static const
#include "shaders/psDayReflect.h"
static const
#include "shaders/psDayReflectNoShadow.h"

static const
#include "shaders/psInterpolation.h"
static const
#include "shaders/psWhite.h"
static const
#include "shaders/psWhiteAlpha.h"
static const
#include "shaders/psStencilVisualisation.h"
static const
#include "shaders/psPostNight.h"
static const
#include "shaders/psPostGlow.h"
static const
#include "shaders/psPostBrightQuery.h"
static const
#include "shaders/psPostShadowVisualisation.h"
static const
#include "shaders/psPostShadowVisualisationAIS.h"
static const
#include "shaders/psPostDepthOfField.h"
static const
#include "shaders/psPostDepthOfFieldFarOnly.h"
static const
#include "shaders/psPostDepthToA.h"
static const
#include "shaders/psPostCopy.h"

static void FindConstantOffset(PSConstRef &cRef, DWORD mapping, int index, D3DRENDERSTATETYPE base)
{
  for (int x = 0; x < 8; x++)
  {
    int cIndex = (mapping >> (x*4)) & 0xf;
    if (cIndex == index)
    {
      DoAssert(cRef._count < MAX_CONSTANT_STAGES);
      cRef._rs[cRef._count++] = (D3DRENDERSTATETYPE)((int)base + x);
    }
  }
}

PSConstRef FindConstantRSType(D3DPIXELSHADERDEF *psCode, int index)
{
  PSConstRef cRef;
  FindConstantOffset(cRef, psCode->PSC0Mapping, index, D3DRS_PSCONSTANT0_0);
  FindConstantOffset(cRef, psCode->PSC1Mapping, index, D3DRS_PSCONSTANT1_0);
  {
    int cIndex = psCode->PSFinalCombinerConstants & 0xf;
    if (cIndex == index)
    {
      DoAssert(cRef._count < MAX_CONSTANT_STAGES);
      cRef._rs[cRef._count++] = D3DRS_PSFINALCOMBINERCONSTANT0;
    }
  }
  {
    int cIndex = (psCode->PSFinalCombinerConstants) >> 4 & 0xf;
    if (cIndex == index)
    {
      DoAssert(cRef._count < MAX_CONSTANT_STAGES);
      cRef._rs[cRef._count++] = D3DRS_PSFINALCOMBINERCONSTANT1;
    }
  }
  return cRef;
}

void EngineDDXB::InitPixelShaders()
{

  if (_pixelShaders<0x101) return;

  // variouse pixel shaders  experiments
  static const DWORD *const psList[NPixelShaderSpecular][NPixelShaderID]=
  {
    { // specular shaders
      dwPsDaySpecularAlphaPixelShader,
      dwPsDaySpecularAlphaPixelShader,
      dwPsDaySpecularNormalMapPixelShader,
      dwPsDaySpecularNormalMapThroughPixelShader,
      dwPsDaySpecularNormalMapThroughPixelShader,
      dwPsDaySpecularNormalMapDiffusePixelShader,
      dwPsDayDetailSpecularAlphaPixelShader,
      dwPsInterpolationPixelShader,
      dwPsDayWaterPixelShader,
      dwPsDayWaterSimplePixelShader,
      dwPsWhitePixelShader,
      dwPsWhiteAlphaPixelShader,
      dwPsDayReflectPixelShader,
      dwPsDayReflectNoShadowPixelShader,
      dwPsDaySpecularNormalMapDiffusePixelShader,
      dwPsDayDetailSpecularAlphaPixelShader,
      dwPsDaySpecularNormalMapPixelShader,
      dwPsDaySpecularNormalMapDiffusePixelShader,
    },
    { // non-specular shaders
      dwPsDayNormalPixelShader,
      dwPsDayNormalDXTAPixelShader,
      dwPsDaySpecularNormalMapPixelShader,
      dwPsDaySpecularNormalMapThroughPixelShader,
      dwPsDaySpecularNormalMapThroughPixelShader,
      dwPsDaySpecularNormalMapDiffusePixelShader,
      dwPsDayDetailPixelShader,
      dwPsInterpolationPixelShader,
      dwPsDayWaterPixelShader,
      dwPsDayWaterSimplePixelShader,
      dwPsWhitePixelShader,
      dwPsWhiteAlphaPixelShader,
      dwPsDayReflectPixelShader,
      dwPsDayReflectNoShadowPixelShader,
      dwPsDaySpecularNormalMapDiffusePixelShader,
      dwPsDayDetailPixelShader,
      dwPsDaySpecularNormalMapPixelShader,
      dwPsDaySpecularNormalMapDiffusePixelShader,
    },
  };

  for (int s=0; s<NPixelShaderSpecular; s++)
    for (int i=0; i<NPixelShaderID; i++)
    {
      // Get the pointer to the shader code
      const DWORD *psData = psList[s][i];

      // Find and remember the constant references
      D3DPIXELSHADERDEF *psCode = (D3DPIXELSHADERDEF*) psData;
      _constMaxColor[s][i]     = FindConstantRSType(psCode, T0_MAXCOLOR);
      _constAvgColor[s][i]     = FindConstantRSType(psCode, T0_AVGCOLOR);
      //_constRgbEyeCoef[s][i]   = FindConstantRSType(psCode, RGB_EYE_COEF);
      _constDiffuse[s][i]      = FindConstantRSType(psCode, PS_DIFFUSE);
      _constDiffuseBack[s][i]  = FindConstantRSType(psCode, PS_DIFFUSE_BACK);
      _constSpecular[s][i]     = FindConstantRSType(psCode, PS_SPECULAR);

#ifdef _XBOX
      {
        HRESULT hr = _d3DDevice->CreatePixelShader(
          (D3DPIXELSHADERDEF *)psData,&_pixelShader[s][i]
          );
          if (FAILED(hr))
          {
            Fail("Cannot create Xbox pixel shader");
            _pixelShader[s][i] = NULL;
          }
      }
#else
      if (psData) _d3DDevice->CreatePixelShader(psData,&_pixelShader[s][i]);
      else _pixelShader[s][i] = NULL;
#endif
    }

    // Set a basic pixel shader (for example on XBOX a PS must be set before setting of PS constant)
    SelectPixelShader(PSNormal);
    SelectPixelShaderSpecular(PSSNormal);
    DoSelectPixelShader();
    //DoSelectPixelShader(PSNormal,PSMDay,PSSNormal);
}


void EngineDDXB::DeinitPixelShaders()
{
  if (_pixelShaders<0x101) return;
  SelectPixelShader(PSNone);
  DoSelectPixelShader();
  //DoSelectPixelShader(PSNone,PSMDay,PSSSpecular);
  for (int s=0; s<NPixelShaderSpecular; s++)
    for (int i=0; i<NPixelShaderID; i++)
    {
      if (_pixelShader[s][i]) _d3DDevice->DeletePixelShader(_pixelShader[s][i]);
    }
}

void EngineDDXB::DoSelectPixelShader
(
 //PixelShaderID ps, PixelShaderMode mode, PixelShaderSpecular spec
 )
{
  Assert(_pixelShaderSel != PSUninitialized);
  Assert(_pixelShaderSpecularSel != PSSUninitialized);

  _hwInDefinedState = false;

  if ((_currPixelShaderSel != _pixelShaderSel) ||
    (_currPixelShaderSpecularSel != _pixelShaderSpecularSel))
  {
    if (_pixelShaderSel < PSNone)
    {
#if DO_TEX_STATS
      if (LogStatesOnce)
      {
        LogF("Set pixel shader %d,%d", _pixelShaderSpecularSel, _pixelShaderSel);
      }
#endif
      //_d3DDevice->SetPixelShader(_pixelShader[_pixelShaderSpecularSel][_pixelShaderSel]);
      // day shader is always used, night is done using postprocess
      _d3DDevice->SetPixelShader(_pixelShader[_pixelShaderSpecularSel][_pixelShaderSel]);
    }
    else
    {
#if DO_TEX_STATS
      if (LogStatesOnce)
      {
        LogF("Set pixel shader NULL");
      }
#endif
      _d3DDevice->SetPixelShader(NULL);
    }

    // Remember current settings
    _currPixelShaderSel = _pixelShaderSel;
    _currPixelShaderSpecularSel = _pixelShaderSpecularSel;
  }

  //////////////////////////////////////////////////////////////////////////
  // MaxColor
  //_d3DDevice->SetPixelShaderConstant(T0_MAXCOLOR,(float *)&_texture0MaxColor,1);
  PSConstRef &cRefMaxColor = _constMaxColor[_currPixelShaderSpecularSel][_currPixelShaderSel];
  for (int i = 0; i < cRefMaxColor._count; i++)
  {
    D3DRENDERSTATETYPE rsMaxColor = cRefMaxColor._rs[i];

    // Create fix-up entry in case we are recording
    if (_recording)
    {
      DWORD pbOffset;
      _d3DDevice->GetPushBufferOffset(&pbOffset);
      _activePushBuffer->_fixupEntries.Add(new FEMaxColor(pbOffset, rsMaxColor, _lastTexture[0], _activePushBuffer));
    }

    // Set the PS constant
    D3DCOLOR c = FAST_D3DRGBA(_texture0MaxColor.R(), _texture0MaxColor.G(), _texture0MaxColor.B(), _texture0MaxColor.A());
    _d3DDevice->SetRenderState(rsMaxColor, c);
  }

  //////////////////////////////////////////////////////////////////////////
  // AvgColor
  //_d3DDevice->SetPixelShaderConstant(T0_AVGCOLOR,(float *)&_texture0AvgColor,1);
  PSConstRef &cRefAvgColor = _constAvgColor[_currPixelShaderSpecularSel][_currPixelShaderSel];
  for (int i = 0; i < cRefAvgColor._count; i++)
  {
    D3DRENDERSTATETYPE rsAvgColor = cRefAvgColor._rs[i];

    // Create fix-up entry in case we are recording
    if (_recording)
    {
      DWORD pbOffset;
      _d3DDevice->GetPushBufferOffset(&pbOffset);
      _activePushBuffer->_fixupEntries.Add(new FEAvgColor(pbOffset, rsAvgColor, _lastTexture[0], _activePushBuffer));
    }

    // Set the PS constant
    D3DCOLOR c = FAST_D3DRGBA(_texture0AvgColor.R(), _texture0AvgColor.G(), _texture0AvgColor.B(), _texture0AvgColor.A());
    _d3DDevice->SetRenderState(rsAvgColor, c);
  }

  /*
  if (_pixelShaderSel==PSNormal && _pixelShaderSpecularSel==PSSSpecular)
  {
    float groundColor[]={0,0,0,0.1};
    // fog color is quite good approximation of sky color
    // we could also store the sky reflection color separatelly
    float skyColor[]={_fogColor.R(),_fogColor.G(),_fogColor.B(),0.5};
    _d3DDevice->SetPixelShaderConstant(PS_GROUND_REFL_COLOR,groundColor,1);
    _d3DDevice->SetPixelShaderConstant(PS_SKY_REFL_COLOR,skyColor,1);
  }
  */
  if (_pixelShaderSel==PSWater)
  {
    // water pixel shader requires some special constants to be set
    // not used for push-buffers
    Assert(!_recording);
    // fog color is quite good approximation of sky color
    // we could also store the sky reflection color separately
    float skyColor[]={_fogColor.R(),_fogColor.G(),_fogColor.B(),0.5f};
    float skyTopColor[]={_skyTopColor.R(),_skyTopColor.G(),_skyTopColor.B(),0.5f};
    _d3DDevice->SetPixelShaderConstant(PS_GROUND_REFL_COLOR,skyColor,1);
    _d3DDevice->SetPixelShaderConstant(PS_SKY_REFL_COLOR,skyTopColor,1);
    LightSun *sun = GScene->MainLight();
    Color waveColor = GetAccomodateEye()*(sun->GetDiffuse()+sun->GetAmbient())*0.5f;
    waveColor.SaturateMinMax();
    _d3DDevice->SetPixelShaderConstant(PS_WAVE_COLOR, (float *)&waveColor, 1);
  }
  else if (_pixelShaderSel==PSWaterSimple)
  {
    // water pixel shader requires some special constants to be set
    // not used for push-buffers
    LightSun *sun = GScene->MainLight();
    Assert(!_recording);
    // fog color is quite good approximation of sky color
    Color lighting = sun->SimulateDiffuse(0.5)*GEngine->GetAccomodateEye()*0.5;
    Color groundLitColor=Color(0.15,0.2,0.1)*lighting;
    float groundColor[]={groundLitColor.R(),groundLitColor.G(),groundLitColor.B(),0.5f};
    float skyTopColor[]={_skyTopColor.R(),_skyTopColor.G(),_skyTopColor.B(),0.5f};
    _d3DDevice->SetPixelShaderConstant(PS_GROUND_REFL_COLOR,groundColor,1);
    _d3DDevice->SetPixelShaderConstant(PS_SKY_REFL_COLOR,skyTopColor,1);
    Color waveColor = GetAccomodateEye()*(sun->GetDiffuse()+sun->GetAmbient())*0.5f;
    waveColor.SaturateMinMax();
    _d3DDevice->SetPixelShaderConstant(PS_WAVE_COLOR, (float *)&waveColor, 1);
  }
}

#endif

void EngineDDXB::EnableNightEye(float night)
{
  if (_nightVision) night = 0;
  // select normal or night pixel shader
  if (fabs(_nightEye-night)<0.01f) return;
  FlushQueues();
  _nightEye = night;
}

/*!
\patch 1.01 Date 6/25/2001 by Ondra.
- Fixed: Enabled guard band clipping.
- this solves some issues with clipping sun on nVidia cards
- it also increases performance on TnT family cards
\patch 1.16 Date 8/10/2001 by Ondra
- New: Pixel shader (1.0) used during night rendering.
*/

void EngineDDXB::Init3DState()
{
  _recording = false;



  //for (int i = 0; i < 256; i++) _perf[i] = NULL;

  HRESULT err;
  _matC = 0, _matD = 0; // w-buffer limits set
  _bias = 0;
  _clipANearEnabled = false,_clipAFarEnabled = false;


  _dxtFormats = 0;
  D3DDEVICE_CREATION_PARAMETERS cpars;
  _d3DDevice->GetCreationParameters(&cpars);

  UINT adapter = cpars.AdapterOrdinal;

  _d3DDevice->GetRenderTarget(_backBuffer.Init());

  D3DSURFACE_DESC bdesc;
  err=_backBuffer->GetDesc(&bdesc);
  if (err != D3D_OK) {DDErrorXB("GetSurfaceDesc failed.",err);return;}

  // create a buffer used in postprocess effects
#if 1
  {
    int workBuffW = bdesc.Width/4;
    const int linAlignment = 64; // 128 or 64 needed?  
    int alignW = (linAlignment-workBuffW)&(linAlignment-1);
    workBuffW += alignW;
    _d3DDevice->CreateTexture(
      workBuffW,bdesc.Height/4,1,
      D3DUSAGE_RENDERTARGET,bdesc.Format,D3DPOOL_DEFAULT,_postFxBuffer.Init()
    );
    if (!_postFxBuffer)
    {
      RptF("Post fx buffer creation failed");
    }

    int brightBuffW = workBuffW/4;
    int alignB = (linAlignment-workBuffW)&(linAlignment-1);
    brightBuffW += alignB;

    _d3DDevice->CreateTexture(
      brightBuffW,bdesc.Height/16,1,
      D3DUSAGE_RENDERTARGET,bdesc.Format,D3DPOOL_DEFAULT,_brightBuffer.Init()
    );
    
  }
#endif

  D3DFORMAT bbFormat = bdesc.Format;

  D3DDEVTYPE devType = D3DDEVTYPE_HAL;
#if 1 //ndef _XBOX
  err = _direct3D->CheckDeviceFormat
    (
    adapter,devType,bbFormat,0,D3DRTYPE_TEXTURE,D3DFMT_DXT1
    );
  if (err==D3D_OK) _dxtFormats |= 1<<1;
  err = _direct3D->CheckDeviceFormat
    (
    adapter,devType,bbFormat,0,D3DRTYPE_TEXTURE,D3DFMT_DXT2
    );
  if (err==D3D_OK) _dxtFormats |= 1<<2;
  err = _direct3D->CheckDeviceFormat
    (
    adapter,devType,bbFormat,0,D3DRTYPE_TEXTURE,D3DFMT_DXT3
    );
  if (err==D3D_OK) _dxtFormats |= 1<<3;
  err = _direct3D->CheckDeviceFormat
    (
    adapter,devType,bbFormat,0,D3DRTYPE_TEXTURE,D3DFMT_DXT4
    );
  if (err==D3D_OK) _dxtFormats |= 1<<4;
  err = _direct3D->CheckDeviceFormat
    (
    adapter,devType,bbFormat,0,D3DRTYPE_TEXTURE,D3DFMT_DXT5
    );
  if (err==D3D_OK) _dxtFormats |= 1<<5;

  LogF("DXT support %x",_dxtFormats);
#endif

  err = _direct3D->CheckDeviceFormat
    (
    adapter,devType,bbFormat,0,D3DRTYPE_TEXTURE,D3DFMT_A8R8G8B8
    );
  _can8888 = ( err==D3D_OK );
  if (_can8888) LogF("Can 8888 textures");
  err = _direct3D->CheckDeviceFormat
    (
    adapter,devType,bbFormat,0,D3DRTYPE_TEXTURE,D3DFMT_A8L8
    );
  _can88 = ( err==D3D_OK );
  if (_can88) LogF("Can 88 textures");
  // note: 1555 and 4444 support required
  err = _direct3D->CheckDeviceFormat
    (
    adapter,devType,bbFormat,0,D3DRTYPE_TEXTURE,tex_A1R5G5B5
    );
  if (err!=D3D_OK)
  {
    ErrorMessage("Texture format ARGB1555 required.");
  }
  err = _direct3D->CheckDeviceFormat
    (
    adapter,devType,bbFormat,0,D3DRTYPE_TEXTURE,D3DFMT_A4R4G4B4
    );
  if (err!=D3D_OK)
  {
    ErrorMessage("Texture format ARGB4444 required.");
  }

  D3DCAPS8 caps;
  _d3DDevice->GetDeviceCaps(&caps);
  if( caps.DevCaps&D3DDEVCAPS_TEXTUREVIDEOMEMORY )
  {
    _texLoc = TexLocalVidMem;
  }
  else if( caps.DevCaps&D3DDEVCAPS_TEXTURENONLOCALVIDMEM )
  {
    _texLoc = TexNonlocalVidMem;
  }
  else
  {
    _texLoc = TexSysMem;
  }

  _useHWTL = (caps.DevCaps&D3DDEVCAPS_HWTRANSFORMANDLIGHT)!=0;


  _w=bdesc.Width;
  _h=bdesc.Height;

  _minGuardX = 0; // guard band clipping properties
  _maxGuardX = _w;
  _minGuardY = 0;
  _maxGuardY = _h;

  _nightEye = 0;

  // check actual guard band clipping capabilities

  _pixelSize = 32;
  switch (bdesc.Format)
  {
  case D3DFMT_X1R5G5B5:
  case D3DFMT_R5G6B5:
  case D3DFMT_A1R5G5B5:
#ifdef _XBOX
  case D3DFMT_LIN_X1R5G5B5:
  case D3DFMT_LIN_R5G6B5:
  case D3DFMT_LIN_A1R5G5B5:
#endif
    _pixelSize = 16;
    break;
  } 

  _depthBpp = 32;

  D3DSURFACE_DESC ddesc;
  ComRef<IDirect3DSurface8> zBuffer;
  _d3DDevice->GetDepthStencilSurface(zBuffer.Init());
  if (zBuffer)
  {
    zBuffer->GetDesc(&ddesc);
    switch (ddesc.Format)
    {
#ifndef _XBOX
  case D3DFMT_D16_LOCKABLE:
  case D3DFMT_D15S1:
#endif
  case D3DFMT_D16:
    _depthBpp = 16;
    break;
    } 

    #if 0
      // experiment: turn z-buffer compression off
      D3DTILE depthtile;
      _d3DDevice->GetTile(1,&depthtile);
      _d3DDevice->SetTile(1,0);
      depthtile.Flags&=~D3DTILE_FLAGS_ZCOMPRESS;
      _d3DDevice->SetTile(1,&depthtile);
    #endif

  }
  LogF("Bpp %d, depth %d",_pixelSize,_depthBpp);

#if defined(_XBOX) && _ENABLE_REPORT
  {
    DWORD size = zBuffer->Size;
    DWORD height = ((size&D3DSIZE_HEIGHT_MASK)>>D3DSIZE_HEIGHT_SHIFT)+1;
    int pitchBlocks = (size&D3DSIZE_PITCH_MASK)>>D3DSIZE_PITCH_SHIFT;
    int pitch = (pitchBlocks+1)*D3DTEXTURE_PITCH_ALIGNMENT;
    LogF("Z-buffer pitch %d, size %d",pitch,pitch*height);
  }
#endif

  // fix: enabled guard band clipping
#if 1
  float minGuardX = caps.GuardBandLeft;
  float maxGuardX = caps.GuardBandRight;
  float minGuardY = caps.GuardBandTop;
  float maxGuardY = caps.GuardBandBottom;
  float maxBand = 1024*4;
  saturate(minGuardX,-maxBand,0);
  saturate(minGuardY,-maxBand,0);
  saturate(maxGuardX,_w,_w+maxBand);
  saturate(maxGuardY,_h,_h+maxBand);
  _minGuardX = toInt(minGuardX), _maxGuardX = toInt(maxGuardX);
  _minGuardY = toInt(minGuardY), _maxGuardY = toInt(maxGuardY);

  LogF
    (
    "Guard band x %d to %d, y %d to %d",
    _minGuardX,_maxGuardX,_minGuardY,_maxGuardY
    );
#endif

  if( caps.TextureCaps&D3DPTEXTURECAPS_SQUAREONLY )
  {
    ErrorMessage("Only square texture supported by HW.");
  }
  //_vidMemTextures=false; // AGP uses single heap model - n
  // for soft and MMX avoid bilinear/trilinear filtering

  // now make a Viewport
  // Setup the viewport for a reasonable viewing area
  D3DVIEWPORT8 viewData;
  memset(&viewData,0,sizeof(viewData));
  viewData.X = 0;
  viewData.Y = 0;
  //viewData.dwWidth  = _wWork;
  //viewData.dwHeight = _hWork;
  viewData.Width  = _w;
  viewData.Height = _h;
  viewData.MinZ = 0.0f;
  viewData.MaxZ = 1.0f;

  err = _d3DDevice->SetViewport(&viewData);
  if( err!=D3D_OK ) DDErrorXB("Cannot set 3D Viewport.",err);

  // Set default render state
  D3DSetRenderStateOpt(D3DRS_ZFUNC,D3DCMP_LESSEQUAL,false);
  D3DSetRenderStateOpt(D3DRS_ZWRITEENABLE,TRUE,false);
  if (_canWBuffer && (_depthBpp<=16 || _canWBuffer32) && _userEnabledWBuffer)
  {
    D3DSetRenderStateOpt(D3DRS_ZENABLE,D3DZB_USEW,false);
    LogF("Using w-buffer");
    _useWBuffer = true;
  }
  else
  {
    D3DSetRenderStateOpt(D3DRS_ZENABLE,D3DZB_TRUE,false);
    _useWBuffer = false;
  }

  D3DSetRenderStateOpt(D3DRS_SHADEMODE,D3DSHADE_GOURAUD,false);
  D3DSetRenderStateOpt(D3DRS_CULLMODE,D3DCULL_CCW,false);

#ifndef _XBOX
  D3DSetRenderStateOpt(D3DRS_CLIPPING,FALSE,false);
#endif
  //D3DSetRenderStateOpt(D3DRS_LIGHTING,FALSE,false);

  // on 16-bit devices use dithering
  D3DSetRenderStateOpt(D3DRS_DITHERENABLE,_pixelSize<=16,false);
#ifdef _XBOX
  // improve DXT1 decompression
  D3DSetRenderStateOpt(D3DRS_DXT1NOISEENABLE,true,false);
#endif

  D3DSetRenderStateOpt(D3DRS_ALPHATESTENABLE,FALSE,false);
  D3DSetTextureStageStateOpt(0,D3DTSS_ALPHAKILL,D3DTALPHAKILL_DISABLE,false);
  D3DSetRenderStateOpt(D3DRS_ALPHAFUNC,D3DCMP_GREATER,false);
  D3DSetRenderStateOpt(D3DRS_ALPHAREF,0x1,false);

  D3DSetRenderStateOpt(D3DRS_ALPHABLENDENABLE,TRUE,false);
  D3DSetRenderStateOpt(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA,false);
  D3DSetRenderStateOpt(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA,false);


  _lastClamp = 0;

  D3DCOLOR fogColor=FAST_D3DRGBA(_fogColor.R(),_fogColor.G(),_fogColor.B(),1);

#if !NO_FOG     
  D3DSetRenderStateOpt(D3DRS_FOGENABLE,TRUE,false);
#endif
  D3DSetRenderStateOpt(D3DRS_FOGCOLOR,fogColor,false);

  // to do: we should not use bilinear filtering on devices
  // which do not support clamping


  // try to use trilinear interpolation if possible
  // always use bilinear interpolation
  /*
  err=D3DSetTextureStageStateOpt(0,D3DTSS_MINFILTER,D3DTEXF_LINEAR,false);
  if( err!=D3D_OK ) DDErrorXB("D3DTSS_MINFILTER",err);
  err=D3DSetTextureStageStateOpt(0,D3DTSS_MAGFILTER,D3DTEXF_LINEAR,false);
  if( err!=D3D_OK ) DDErrorXB("D3DTSS_MAGFILTER",err);
  D3DSetTextureStageStateOpt(0,D3DTSS_MIPFILTER,D3DTEXF_POINT,false);
  */
  //D3DSetTextureStageStateOpt(0,D3DTSS_MAXANISOTROPY,4,false);

  // setup using fixed-function pipeline

  //D3DSetTextureStageStateOpt( 0, D3DTSS_TEXCOORDINDEX, 0,false);

  for (int i = 0; i < TEXTURE_STAGES; i++)
  {
    D3DSetTextureStageStateOpt(i, D3DTSS_TEXCOORDINDEX, i,false);
    D3DSetTextureStageStateOpt(i, D3DTSS_MAGFILTER, D3DTEXF_LINEAR,false);
    D3DSetTextureStageStateOpt(i, D3DTSS_MINFILTER, D3DTEXF_LINEAR,false);
    D3DSetTextureStageStateOpt(i, D3DTSS_MIPFILTER, D3DTEXF_LINEAR,false);
    D3DSetTextureStageStateOpt(i, D3DTSS_MAXANISOTROPY, 2, false);
  }

  // Setting the adressing mode
  D3DSetTextureStageStateOpt(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP,false);
  D3DSetTextureStageStateOpt(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP,false);
  D3DSetTextureStageStateOpt(1, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP,false);
  D3DSetTextureStageStateOpt(1, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP,false);
  D3DSetTextureStageStateOpt(2, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP,false);
  D3DSetTextureStageStateOpt(2, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP,false);
  D3DSetTextureStageStateOpt(3, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP,false);
  D3DSetTextureStageStateOpt(3, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP,false);


  /*
  D3DSetTextureStageStateOpt( 2, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP,false);
  D3DSetTextureStageStateOpt( 2, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP,false);
  D3DSetTextureStageStateOpt( 2, D3DTSS_TEXCOORDINDEX, 2,false);
  D3DSetTextureStageStateOpt( 2, D3DTSS_MAGFILTER, D3DTEXF_LINEAR,false);
  D3DSetTextureStageStateOpt( 2, D3DTSS_MINFILTER, D3DTEXF_LINEAR,false);
  D3DSetTextureStageStateOpt( 2, D3DTSS_MIPFILTER, D3DTEXF_LINEAR,false);
  D3DSetTextureStageStateOpt( 2, D3DTSS_MAXANISOTROPY, 2, false);
  */

  // Set initial value for pixel shader
  _pixelShaderSel = _currPixelShaderSel = PSUninitialized;
  _pixelShaderSpecularSel = _currPixelShaderSpecularSel = PSSUninitialized;

#if PIXEL_SHADERS
  InitPixelShaders();
#endif

  SetTexture0Color(HWhite,HWhite,false);

  D3DSetRenderStateOpt(D3DRS_STENCILENABLE,TRUE,false);
  D3DSetRenderStateOpt(D3DRS_STENCILREF,0,false);
#ifdef _XBOX
  D3DSetRenderStateOpt(D3DRS_ROPZREAD,TRUE,false);
  D3DSetRenderStateOpt(D3DRS_DONOTCULLUNCOMPRESSED,FALSE,false);
#endif

  _shadowLevelNeeded = 0;
  _shadowLevel = 0;
  //_aisValue = 0;
  

  DoShadowsStatesSettings(SS_Disabled, false);

#if VERTEX_SHADERS
  InitVertexShaders();
#endif

  _texGenMatrix[0] = D3DMatZero;
  _texGenMatrix[1] = D3DMatZero;
  _texGenMatrix[2] = D3DMatZero;
  _texGenScale.u = 1.0f;
  _texGenScale.v = 1.0f;
  _texGenOffset.u = 0.0f;
  _texGenOffset.v = 0.0f;
  _newTexGenScale.u = 1.0f;
  _newTexGenScale.v = 1.0f;
  _newTexGenOffset.u = 0.0f;
  _newTexGenOffset.v = 0.0f;
  _texGenScaleOrOffsetHasChanged = true;
  DoEnableDetailTexGen(NULL,NULL,NULL,true, true, true, false);

  // HW is not in defined state at the beginning
  _hwInDefinedState = false;

  // Defined material
  _definedMat = new TexMaterial;
  TLMaterial mat;
  mat.Init();
  _definedMat->Combine(_definedMatMod, mat);

  // Last items
  _lastSpec = 0;
  _prepSpec = 0;
  for (int i = 0; i < TEXTURE_STAGES; i++)
  {
    _d3DDevice->SetTexture( i,NULL );
    _lastHandle[i] = NULL;
    _lastTexture[i].Free();

  }
  _lastMat = NULL;

  _mainLight = ML_Sun;
  _currMainLight = ML_Sun;

  _fogMode = FM_Fog;
  _currFogMode = FM_Fog;

  _lightsHasChanged = true;

  //_matValuesHasChanged = true;

  _d3DDevice->LoadVertexShader(_shaderNonTLHandle, 0);
  _d3DDevice->SelectVertexShader(_shaderNonTLHandle, 0);

  DoSwitchTL(false);

  _materialSet.ambient = HBlack;
  _materialSet.diffuse = HBlack;
  _materialSet.forcedDiffuse = HBlack;
  _materialSet.emmisive = HBlack;
  _materialSet.specFlags = 0;
  _materialSetSpec = 0;

  // Initialize with some nonsense color
  _modColor = Color(1, 0, 0, 1);

  _isWorldMatrixChanged = true;
  _world.SetIdentity();
  _invWorld.SetIdentity();

  _currSkinningType = STUninitialized;
  _currInstancing = false;
  //_currSunEnabled = true;
  _bonesHaveChanged = true;
  _bonesWereSetDirectly = false; // Actually it doesn't matter what will be here
  _currMinBoneIndex = -1;

  //EnableAlphaShadow(true);
  _alphaShadowEnabled = true;
  _landShadowEnabled = false;
  _alphaInstancing = false;
  
  _currAlphaShadowEnabled = true;
  _currLandShadowEnabled = false;
  _currAlphaInstancing = false;

  // Set the VS constants
  static const float constants[4] = {0.0f, 1.0f, 255.0f * 3.0f , 0.5f};
  _d3DDevice->SetVertexShaderConstant(CONSTANTS_0_1_765_0d5, constants, 1);
  static const float constants2[4] = {0.01f, LIGHT_SATURATION, 255.0f * 4.0f , 0.5f};
  _d3DDevice->SetVertexShaderConstant(CONSTANTS_SVOffset_lightsat_1020_0d5, constants2, 1);
  static const float constants3[4] = {0.5f, 765.0f, 0.0f, 0.0f};
  _d3DDevice->SetVertexShaderConstant(MATRICES_OFFSET_765, constants3, 1);


  /*
  // Setup Taylor's constants
  float tc_1_3_5_7[4] = {1.0f/H_PI,
  1.0f/(2.0f*3.0f*H_PI),
  3.0f/(2.0f*4.0f*5.0f*H_PI),
  3.0f*5.0/(2.0f*4.0f*6.0f*7.0f*H_PI)};
  _d3DDevice->SetVertexShaderConstant(TAYLOR_1_3_5_7, tc_1_3_5_7, 1);
  float tc_9_11_13_15[4] = {3.0f*5.0f*7.0f/(2.0f*4.0f*6.0f*8.0f*9.0f*H_PI),
  3.0f*5.0f*7.0f*9.0f/(2.0f*4.0f*6.0f*8.0f*10.0f*11.0f*H_PI),
  3.0f*5.0f*7.0f*9.0f*11.0f/(2.0f*4.0f*6.0f*8.0f*10.0f*12.0f*13.0f*H_PI),
  3.0f*5.0f*7.0f*9.0f*11.0f*13.0f/(2.0f*4.0f*6.0f*8.0f*10.0f*12.0f*14.0f*15.0f*H_PI)};
  _d3DDevice->SetVertexShaderConstant(TAYLOR_9_11_13_15, tc_9_11_13_15, 1);
  */


}

#ifdef _XBOX
#define ENABLE_DXTO 1 // enable for opaque / transparent
#else
#define ENABLE_DXTO 1 // enable for opaque / transparent
#endif

#define ENABLE_DXTA 0 // enable for alpha

#define CAN_DXT(i) ( (_dxtFormats&(1<<(i)))!=0 )
void EngineDDXB::InitVRAMPixelFormat( D3DFORMAT &pf, PacFormat format, bool enableDXT )
{
  switch( format )
  {
  case PacDXT1:
#if ENABLE_DXTO
    if (CAN_DXT(1))
    {
      pf = D3DFMT_DXT1;
    }
    else
#endif
    {
      pf = tex_A1R5G5B5;
    }
    break;
  case PacDXT2:
    if( enableDXT && CAN_DXT(2) ) pf = D3DFMT_DXT2;
    else pf = tex_A4R4G4B4;
    break;
  case PacDXT3:
    if( enableDXT && CAN_DXT(3) ) pf = D3DFMT_DXT3;
    else pf = tex_A4R4G4B4;
    break;
  case PacDXT4:
    if( enableDXT && CAN_DXT(4) ) pf = D3DFMT_DXT4;
    else pf = tex_A4R4G4B4;
    break;
  case PacDXT5:
    if( enableDXT && CAN_DXT(5) ) pf = D3DFMT_DXT5;
    else pf = tex_A4R4G4B4;
    break;
  case PacARGB1555:
    pf = tex_A1R5G5B5;
    break;
  case PacRGB565:
    pf = tex_R5G6B5;
    break;
  case PacAI88:
    if (Can88())
    {
      // TODO: correct AI format
      pf = tex_A8L8;
      break;
    }
    // note: previous case may fall through
  case PacARGB8888:
    if (Can8888())
    {
      pf = tex_A8R8G8B8;
      break;
    }
    // note: previous case may fall through
  case PacARGB4444:
#if ENABLE_DXTA
    if( enableDXT && CAN_DXT(2) )
    {
      pf = D3D_FMTDXT2;
    }
    else if( CAN_DXT(3) )
    {
      pf = D3D_FMTDXT3;
    }
    else
#endif
    {
      pf = tex_A4R4G4B4;
    }
    break;
  case PacP8:
    pf = D3DFMT_P8;
    Fail("Palette textures obsolete");
    break;
  default:
    ErrorMessage("Texture has bad pixel format (VRAM).");
    break;
  }
}

#include <El/Debugging/debugTrap.hpp>


static const char *SurfaceFormatName(int format)
{
#define FN(x) case D3DFMT_##x: return #x
  switch (format)
  {
    FN(UNKNOWN);

#ifndef _XBOX
    FN(R8G8B8);
    FN(R3G3B2);
    FN(A8P8);
    FN(A4L4);
    FN(W11V11U10);
    FN(X4R4G4B4);
    FN(A8R3G3B2);
    FN(X8L8V8U8);
    FN(D32);
    FN(D15S1);
    FN(D24X8);
    FN(D24X4S4);

    // following formats map to some existing format
    FN(DXT3);
    FN(DXT5);
    FN(D16_LOCKABLE);
#endif
    FN(A8R8G8B8);
    FN(X8R8G8B8);
    FN(R5G6B5);
    FN(X1R5G5B5);
    FN(A1R5G5B5);
    FN(A4R4G4B4);
    FN(A8);

    FN(P8);

    FN(L8);
    FN(A8L8);

    FN(V8U8);
    FN(L6V5U5);
    FN(Q8W8V8U8);
    FN(V16U16);

    FN(UYVY);
    FN(YUY2);
    FN(DXT1);
    FN(DXT2);
    FN(DXT4);

    FN(D24S8);
    FN(D16);

#ifdef _XBOX
    FN(LIN_A1R5G5B5);
    FN(LIN_A4R4G4B4);
    FN(LIN_A8      );
    FN(LIN_A8B8G8R8);
    FN(LIN_A8R8G8B8);
    FN(LIN_B8G8R8A8);
    FN(LIN_G8B8    );
    FN(LIN_R4G4B4A4);
    FN(LIN_R5G5B5A1);
    FN(LIN_R5G6B5  );
    FN(LIN_R6G5B5  );
    FN(LIN_R8B8    );
    FN(LIN_R8G8B8A8);
    FN(LIN_X1R5G5B5);
    FN(LIN_X8R8G8B8);

    FN(LIN_A8L8);
    FN(LIN_AL8 );
    FN(LIN_L16 );
    FN(LIN_L8  );
#endif
  }
  return "OTHER";
}

typedef IDirect3D8 *WINAPI Direct3DCreate8F(UINT SDKVersion);

RString EngineDDXB::GetDebugName() const
{
#ifdef _XBOX
  RString ret = "Xbox ";
  if ( _useDXTL) ret = ret + "HW";
  return ret;
#else
  RString ret = "Xbox ";
  if ( _useDXTL) ret = ret + "HW T&L ";

  if (!_d3DDevice)
  {
    ret = ret + ", Device: <null>";
  }
  else
  {
    D3DDEVICE_CREATION_PARAMETERS cp;
    _d3DDevice->GetCreationParameters(&cp);
    D3DADAPTER_IDENTIFIER8 id;
    _direct3D->GetAdapterIdentifier(cp.AdapterOrdinal,D3DENUM_NO_WHQL_LEVEL,&id);
    char driverVersion[256];
    sprintf
      (
      driverVersion,"%d.%d.%d.%d",
      HIWORD(id.DriverVersion.HighPart),LOWORD(id.DriverVersion.HighPart),
      HIWORD(id.DriverVersion.LowPart),LOWORD(id.DriverVersion.LowPart)
      );
    ret = ret + ", Device: " + RString(id.Description);
    ret = ret + ", Driver:" + RString(id.Driver) + RString(" ") + RString(driverVersion);
  }

  return ret;
#endif
}

void EngineDDXB::CreateD3D()
{
  if (_direct3D) return;
#ifndef _XBOX
  HMODULE lib = LoadLibrary("d3d8.dll");
  if (!lib)
  {
    ErrorMessage("DX8 required.");
    return;
  }

  Direct3DCreate8F *create = (Direct3DCreate8F *)GetProcAddress
    (
    lib,"Direct3DCreate8"
    );
  if (!create)
  {
    ErrorMessage("Direct3DCreate8 required.");
    return;
  }
  _direct3D = create(D3D_SDK_VERSION);

  // we know DX 8.1 features are not used
  // if 8.1 creating will fail, try creating 8.0
#define D3D_SDK_VERSION_80 120
  if (!_direct3D)
  {
    _direct3D = create(D3D_SDK_VERSION_80);
  }

#else
  _direct3D = Direct3DCreate8(D3D_SDK_VERSION);
#endif

  if (!_direct3D)
  {
    ErrorMessage("DX8 SDK version %d required.",D3D_SDK_VERSION);
    return;
  }
}

void EngineDDXB::DestroyD3D()
{
  _direct3D.Free();
}

#ifndef _XBOX

#if WINVER<0x500

typedef struct tagMONITORINFO {  
  DWORD  cbSize; 
  RECT   rcMonitor; 
  RECT   rcWork; 
  DWORD  dwFlags; 
} MONITORINFO, *LPMONITORINFO; 

#define SM_XVIRTUALSCREEN       76
#define SM_YVIRTUALSCREEN       77
#define SM_CXVIRTUALSCREEN      78
#define SM_CYVIRTUALSCREEN      79


#endif

typedef BOOL WINAPI GetMonitorInfoT(HMONITOR mon, LPMONITORINFO mi);

#endif

void EngineDDXB::SearchMonitor(int &x, int &y, int &w, int &h)
{
#ifndef _XBOX
  MONITORINFO mi;
  int adapter = D3DAdapter;
  bool monFound = false;
  if (adapter>=0)
  {
    HMONITOR mon = _direct3D->GetAdapterMonitor(adapter);
    // if running in windowed mode on multimon window, search all monitors
    HMODULE lib = LoadLibrary("user32.dll");
    if (lib)
    {
      GetMonitorInfoT *getmon = (GetMonitorInfoT *)GetProcAddress(lib,"GetMonitorInfoA");
      if (getmon)
      {
        memset(&mi,0,sizeof(mi));
        mi.cbSize = sizeof(mi);
        getmon(mon,&mi);
        // we have monitor information, adjust window position
        int maxW = mi.rcWork.right-mi.rcWork.left;
        int maxH = mi.rcWork.bottom-mi.rcWork.top;
        if (w>maxW) w=maxW;
        if (h>maxH) h=maxH;
        if (x>mi.rcWork.right-w) x = mi.rcWork.right-w;
        if (y>mi.rcWork.bottom-h) y = mi.rcWork.bottom-h;
        if (x<mi.rcWork.left) x = mi.rcWork.left;
        if (y<mi.rcWork.top) y = mi.rcWork.top;
        monFound = true;
      }
      FreeLibrary(lib);
    }
  }

  if (!monFound)
  {
    int minX = GetSystemMetrics(SM_XVIRTUALSCREEN), minY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int maxW = GetSystemMetrics(SM_CXVIRTUALSCREEN), maxH = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if (w>maxW) w=maxW;
    if (h>maxH) h=maxH;
    if (x>minX+maxW-w) x = minX+maxW-w;
    if (y>minY+maxH-h) y = minY+maxH-h;
    if (x<minX) x = minX;
    if (y<minY) y = minY;
  }
#endif
}

void EngineDDXB::FindMode(int adapter)
{
  // prepare presentation parameters based on _w, _h, _pixelSize
  // first select backbuffer format
  D3DDISPLAYMODE bestDisplayMode;
  int bestCost = INT_MAX;
  bestDisplayMode.Format = D3DFMT_X8R8G8B8;
  bestDisplayMode.RefreshRate = D3DPRESENT_RATE_DEFAULT;
  bestDisplayMode.Width = _w;
  bestDisplayMode.Height = _h;
  if (_windowed)
  {
    _direct3D->GetAdapterDisplayMode(adapter,&bestDisplayMode);
    // check 
  }
  else
  {
    for (int mode=0; mode<(int)_direct3D->GetAdapterModeCount(adapter); mode++)
    {
      D3DDISPLAYMODE displayMode;
      _direct3D->EnumAdapterModes(adapter,mode,&displayMode);

      int bpp = 16;
      int cost = 0;
#if 0 //def _XBOX
      LogF
        (
        "Display Format %dx%d (%d) %s supported",
        displayMode.Width,displayMode.Height,displayMode.RefreshRate,
        SurfaceFormatName(displayMode.Format)
        );
#endif
      switch (displayMode.Format)
      {
      case D3DFMT_X8R8G8B8: cost += 0; bpp = 32; break;
      case D3DFMT_R5G6B5: cost += 100; bpp = 16; break;
      case D3DFMT_X1R5G5B5: case D3DFMT_A1R5G5B5: cost += 110; bpp = 16; break;

      case D3DFMT_A8R8G8B8: cost += 180; bpp = 32; break;
#ifndef _XBOX
      case D3DFMT_R8G8B8: cost += 200; bpp = 24; break;
#endif
#ifdef _XBOX

      case D3DFMT_LIN_X8R8G8B8: cost += 1180; bpp = 32; break;
      case D3DFMT_LIN_A8R8G8B8: cost += 1000; bpp = 32; break;

      case D3DFMT_LIN_R5G6B5: cost += 1110; bpp = 16; break;
      case D3DFMT_LIN_X1R5G5B5: cost += 1110; bpp = 16; break;
      case D3DFMT_LIN_A1R5G5B5: cost += 1100; bpp = 16; break;
#endif

      default: cost += 500; break; // unknown format - last resort
      }
      cost +=
        (
        abs(_w*_h-displayMode.Width*displayMode.Height)*10 + 
        abs(_w-displayMode.Width)*10 + 
        abs(_h-displayMode.Height)*10 +
        abs(_pixelSize-bpp)*200
        );
      if (_refreshRate>0)
      {
        cost += abs(_refreshRate-displayMode.RefreshRate);
      }
      else
      {
        cost += abs(75-displayMode.RefreshRate); // higher refresh rate is always better
      }
      if (cost<bestCost)
      {
        bestCost = cost;
        bestDisplayMode = displayMode;
      }
    }
    LogF
      (
      "mode %dx%d (%d Hz) (%s)",
      bestDisplayMode.Width,bestDisplayMode.Height,
      bestDisplayMode.RefreshRate,
      SurfaceFormatName(bestDisplayMode.Format)
      );
  }


  // select suitable depthbuffer formats
#if 0
  D3DFORMAT depthFormat = D3DFMT_D24S8;
#else

  // FLY If you choose other than D3DFMT_D24S8 format to be default on XBOX, then don't forget to change the screenspaceScale 3th constant
  D3DFORMAT depthFormat = D3DFMT_D16;
  const static D3DFORMAT depthFormats[]=
  {
#if 1 //!_DEBUG
    D3DFMT_D24S8,
#ifndef _XBOX
      D3DFMT_D24X4S4,
      D3DFMT_D15S1, // stencil formats
      D3DFMT_D32,
      D3DFMT_D24X8,
      D3DFMT_D16_LOCKABLE,
#endif
#endif
      D3DFMT_D16 // normal depth formats
  };

  D3DDEVTYPE devType = D3DDEVTYPE_HAL;

  D3DCAPS8 caps;
  _direct3D->GetDeviceCaps(adapter,devType,&caps);
  if (caps.StencilCaps)
  {
    LogF("Stencil caps %d",caps.StencilCaps);
  }

#ifndef _SUPER_RELEASE
  for (int di=0; di<sizeof(depthFormats)/sizeof(*depthFormats); di++)
  {
    D3DFORMAT format = depthFormats[di];

    HRESULT ok = _direct3D->CheckDeviceFormat(
      adapter,devType,bestDisplayMode.Format,
      D3DUSAGE_DEPTHSTENCIL,D3DRTYPE_SURFACE,
      format
    );
    if (ok==D3D_OK)
    {
      LogF("Depth %s supported",SurfaceFormatName(format));
      ok = _direct3D->CheckDepthStencilMatch(
        adapter,devType,
        bestDisplayMode.Format,bestDisplayMode.Format,
        format
      );
      if (ok!=D3D_OK)
      {
        LogF(
          "DepthStencilMatch check (%s/%s) failed",
          SurfaceFormatName(bestDisplayMode.Format),SurfaceFormatName(format)
        );
      }
    }
  }
#endif
  for (int di=0; di<sizeof(depthFormats)/sizeof(*depthFormats); di++)
  {
    D3DFORMAT format = depthFormats[di];

    HRESULT ok = _direct3D->CheckDeviceFormat(
      adapter,devType,bestDisplayMode.Format,
      D3DUSAGE_DEPTHSTENCIL,D3DRTYPE_SURFACE,
      format
    );
    if (ok==D3D_OK)
    {
      if (format==D3DFMT_D16_LOCKABLE)
      {
        LogF("Lockable z-buffer support present");
      }
      ok = _direct3D->CheckDepthStencilMatch
        (
        adapter,devType,
        bestDisplayMode.Format,bestDisplayMode.Format,
        format
        );
      if (ok==D3D_OK)
      {
        LogF
          (
          "Display Format %s/%s",
          SurfaceFormatName(bestDisplayMode.Format),
          SurfaceFormatName(format)
          );
        depthFormat = format;
        break;
      }
    }
  }
#endif

  switch (depthFormat)
  {
  case D3DFMT_D24S8:
#ifndef _XBOX
  case D3DFMT_D24X4S4: case D3DFMT_D15S1:
#endif
    _hasStencilBuffer = true;
    break;
  default:
    _hasStencilBuffer = false;
    break;
  }



  if (_windowed)
  {
    _pp.BackBufferWidth = _w;
    _pp.BackBufferHeight = _h;
    _pp.FullScreen_RefreshRateInHz = 0;
    _refreshRate = 0;
  }
  else
  {
    _pp.BackBufferWidth = bestDisplayMode.Width;
    _pp.BackBufferHeight = bestDisplayMode.Height;
    _pp.FullScreen_RefreshRateInHz = bestDisplayMode.RefreshRate;
    _w = bestDisplayMode.Width;
    _h = bestDisplayMode.Height;
    _refreshRate = bestDisplayMode.RefreshRate;
    _vSyncInterval = 733000000/_refreshRate;
  }


  _pp.BackBufferFormat = bestDisplayMode.Format;

  //_pp.BackBufferCount = 3;
#ifdef _XBOX
  _pp.BackBufferCount = 1;
#else
  _pp.BackBufferCount = 2;
#endif
  _pp.MultiSampleType = D3DMULTISAMPLE_NONE;
  _pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  _pp.hDeviceWindow = _hwndApp;
  _pp.Windowed = _windowed;
  _pp.EnableAutoDepthStencil = true;
  _pp.AutoDepthStencilFormat = depthFormat;
  _pp.Flags = 0;

  DWORD videoFlags = XGetVideoFlags();
  _pp.Flags = 0;
  if (videoFlags & XC_VIDEO_FLAGS_HDTV_480p)
  {
    _pp.Flags |= D3DPRESENTFLAG_PROGRESSIVE;
  }

  if (!_pp.Windowed)
  {
    // wait for first v-sync
#ifdef _XBOX
    if (_pp.FullScreen_RefreshRateInHz>47)
    {
      _pp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_THREE;
      _defPresentationInterval = _presentationInterval = 3;
    }
    else if (_pp.FullScreen_RefreshRateInHz>25)
    {
      _pp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_TWO;
      _defPresentationInterval = _presentationInterval = 2;
    }
    else
    {
      _pp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
      _defPresentationInterval = _presentationInterval = 2;
    }
#else
    _pp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    _defPresentationInterval = _presentationInterval = 1;
#endif
#if 0 //_PROFILE
    _pp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    _defPresentationInterval = _presentationInterval = 1;
#endif
    _pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
  }
  else
  {
    _pp.FullScreen_PresentationInterval = 0;
  }
}

#ifdef _XBOX
HRESULT ConvertVertexShaderTokensToXBOX(DWORD *dwShader, XGBuffer **shader, XGBuffer **error)
{
  int size = 0;
  // D3D8 shader token terminator is 0x0000ffff
  // count terminator as well
  for (size = 1; dwShader[size-1]!=0x0000ffff; size++)
  {
  }
  DWORD type;
  HRESULT hr = XGAssembleShader(
    "no",dwShader,size*sizeof(*dwShader),
    SASM_INPUT_VERTEXSHADER_TOKENS|SASM_INPUT_NONXBOX_TOKENS|SASM_SKIPVALIDATION|SASM_DONOTOPTIMIZE,
    NULL,shader,error,
    NULL,NULL,NULL,&type
    );
  return hr;
}
#endif

#define LOG_SWAP 0

template <class Type, int size>
class RecentData
{
  int _currentItem;
  int _currentItemId;
  Type _data[size];
  bool _dataSet[size];

public:
  void Reset(int id)
  {
    _currentItemId = id;
    _currentItem = 0;
    for (int i=0; i<size; i++)
    {
      _dataSet[i] = false;
    }
  }
  RecentData(int id=0)
  {
    Reset(id);
  }
  void Set(int id, const Type &value)
  {
    int pos = id - _currentItemId;
    // we cannot add data which is too old
    if (pos<0)
    {
      if (pos<=-size)
      {
        LogF("Data too old - ignored (%d, recent %d)",id,_currentItemId);
        return;
      }
      // setting data in past
      // each item should be set only once?
      int index = _currentItem+pos;
      if (index<0) index += size;
      Assert(index>=0 && index<size);
      Assert( !_dataSet[index]);
      _data[index] = value;
      _dataSet[index] = true;
      return;
    }
    if (pos>_currentItem+1)
    {
      // adding too much into the future - reset needed
      LogF("Data too new - reset (%d, recent %d)",id,_currentItemId);
      Reset(id);
    }
    // extending data about one item
    _data[_currentItem] = value;
    _dataSet[_currentItem] = true;
    if (++_currentItem>=size) _currentItem = 0;
    _currentItemId = id+1;
  }
  void AdjustSince(int id, const Type &add)
  {
    int pos = id - _currentItemId;
    if (pos>=0)
    {
      Fail("Adjust can be done only on past data");
      return;
    }
    int index = _currentItem+pos;
    if (index<0) index += size;
    Assert(index>=0 && index<size);
    for (int i=index; i!=_currentItem; )
    {
      Assert( _dataSet[i]);
      _data[i] += add;

      i++;
      if (i==size) i=0;
    }

  }
  bool isKnown(int id) const
  {
    int pos = id - _currentItemId;
    if (pos>=0) return false;
    if (pos<=-size) return false;

    int index = _currentItem+pos;
    if (index<0) index += size;
    Assert(index>=0 && index<size);
    return _dataSet[index];
  }
  Type Get(int id) const
  {
    int pos = id - _currentItemId;
    if (pos>=0) return Type();
    if (pos<=-size) return Type();

    int index = _currentItem+pos;
    if (index<0) index += size;
    Assert(index>=0 && index<size);
    Assert(_dataSet[index]);
    return _data[index];
  }
};

#ifdef _XBOX

const int SwapsInHistory = 32;
RecentData<int,SwapsInHistory> _frameDisplayStartVSync; /// real value
RecentData<int,SwapsInHistory> _frameDisplayStartWantedVSync; /// wanted value
RecentData<float,SwapsInHistory> _frameSimStartVSync; // real value when CPU started working

int EngineDDXB::_frameDurationVSync[TrackFrames];
int EngineDDXB::_curFrame = 0;
int EngineDDXB::_lastSwapVSync = 0;

int EngineDDXB::_lastVSync = 0;
int EngineDDXB::_lastVSyncTime = 0;
/// time in rdtsc between vsyncs
int EngineDDXB::_vSyncInterval = 733000000/60;;

int EngineDDXB::_lastSwapId;
int EngineDDXB::_lastKnownError=0;

#if _ENABLE_CHEATS
extern bool FpsCap;
#endif

#pragma warning(disable:4035)
/// fast and simple version
static inline __int64 GetCPUCyclesRdtsc()
{
  __asm
  {
    rdtsc
  }
  // return value in edx:eax
}

void __cdecl EngineDDXB::VBlankCallback(D3DVBLANKDATA *pData)
{
  _lastVSync = pData->VBlank;
  __int64 time = GetCPUCyclesRdtsc();
  // 32b should be enough for 100 sec with 733MHz CPU
  _lastVSyncTime = time;
  /*
  __int64 time = GetCPUCycles();
  int timeMs = int(time>>12)/179;
  #if _ENABLE_CHEATS
  if (FpsCap)
  {
  char buf[64];
  char iBuf[32];
  itoa(timeMs,iBuf,10);
  strcpy(buf,iBuf);
  strcat(buf,": VBlank ");

  itoa(pData->Swap,iBuf,10);
  strcat(buf,iBuf);
  strcat(buf," - ");

  itoa(pData->VBlank,iBuf,10);
  strcat(buf,iBuf);
  strcat(buf," ");

  if (pData->Flags&D3DVBLANK_SWAPMISSED)
  {
  strcat(buf,"VB missed");
  }
  else if (pData->Flags&D3DVBLANK_SWAPDONE)
  {
  strcat(buf,"VB Swap");
  }
  strcat(buf,"\n");
  OutputDebugString(buf);
  }
  #endif
  */
}

void EngineDDXB::BackToFront(int wantedDurationVsync)
{

  int lastVSync = _lastSwapId>0 ? _frameDisplayStartWantedVSync.Get(_lastSwapId) : 0;
  _lastSwapId++;
  _frameDisplayStartWantedVSync.Set(_lastSwapId,lastVSync+wantedDurationVsync);
  float time = GetTimeVSync();
#if _ENABLE_REPORT
  if (FpsCap)
  {
    LogF("Queued swap %d (%.3f)",_lastSwapId,GetTimeVSync());
  }
#endif

  _frameSimStartVSync.Set(_lastSwapId,time);

  _d3DDevice->Present(NULL,NULL,NULL,NULL);
}

#if _ENABLE_CHEATS
extern bool FpsCap;
#endif

#if _ENABLE_REPORT
static void __cdecl GPUIdleProfCallback(DWORD flags)
{
  PROFILE_SCOPE(GPUid);
  const int before = 1;
  while (IDirect3DDevice8::GetPushDistance(D3DDISTANCE_FENCES_TOWAIT)>before)
  {
  }
}

static void __cdecl GPUIdleCallback(DWORD flags)
{
  IDirect3DDevice8 *device = GEngineDD->GetDirect3DDevice();
  (void)device; // device functions are static on Xbox
  PROFILE_SCOPE(GPUid);
  DWORD start = GlobalTickCount();
  const int before = 1;
  while (device->GetPushDistance(D3DDISTANCE_FENCES_TOWAIT)>before)
  {
  }
  DWORD duration = GlobalTickCount()-start;
  if (duration>10)
  {
    if (flags&D3DWAIT_PRESENT)
    {
      LogF("Idle on present: %d ms",duration);
    }
    if (flags&D3DWAIT_BLOCKUNTILIDLE)
    {
      LogF("Idle on BlockUntilIdle: %d ms",duration);
    }
    if (flags&D3DWAIT_BLOCKONFENCE)
    {
      LogF("Idle on BlockOnFence: %d ms",duration);
    }
    if (flags&D3DWAIT_PUSHBUFFER)
    {
      LogF("Idle on pushbuffer: %d ms",duration);
    }
    if (flags&D3DWAIT_OBJECTLOCK)
    {
      LogF("Idle on object lock: %d ms",duration);
    }
    if (flags&D3DWAIT_BUSYWAIT_FLAG)
    {
      LogF("Idle on BusyWait flag: %d ms",duration);
    }
  }
}
#endif

void __cdecl EngineDDXB::SwapCallback(D3DSWAPDATA *pData)
{
  int duration = pData->SwapVBlank-_lastSwapVSync;
  _lastSwapVSync = pData->SwapVBlank;
  //_vSyncInterval = pData->TimeBetweenSwapVBlanks;
  _frameDurationVSync[_curFrame] = duration;
  if (++_curFrame>=TrackFrames) _curFrame = 0;

  // verification only
  int expected = _frameDisplayStartWantedVSync.Get(pData->Swap);
  int delta = pData->SwapVBlank-expected;
  _frameDisplayStartWantedVSync.AdjustSince(pData->Swap,delta);
  _lastKnownError += delta;
  _frameDisplayStartVSync.Set(pData->Swap,pData->SwapVBlank);
#if _ENABLE_CHEATS
  //#if LOG_SWAP
  if (FpsCap)
  {
    char buf[128];
    char iBuf[32];
    strcpy(buf,"Swap ");

    itoa(pData->Swap,iBuf,10);
    strcat(buf,iBuf);
    strcat(buf," - ");

    itoa(pData->SwapVBlank,iBuf,10);
    strcat(buf,iBuf);
    strcat(buf," - ");

    strcat(buf,"(err ");
    int refresh = GEngineDD->_refreshRate;
    if (refresh > 0)
    {
      int err = 1000*delta/refresh;
      itoa(err,iBuf,10);
      strcat(buf,iBuf);
      strcat(buf," ms)");
    }
    else
    {
      strcat(buf," invalid)");
    }

    strcat(buf,"\n");
    OutputDebugString(buf);
  }
#endif
}

void EngineDDXB::ResetTimingHistory(int ms)
{
  base::ResetTimingHistory(ms);
  // conver ms to vsync
  //float vsyncTime = 1.0f/RefreshRate();
  //float wantedTime = 0.001f*ms;
  //int numVsync = toInt(wantedTime/vsyncTime);
  // optimized:
  int numVsync = toInt(ms*0.001f*RefreshRate());

  for (int i=0; i<TrackFrames; i++)
  {
    _frameDurationVSync[i] = numVsync;
  }
}

void EngineDDXB::ResetTimingVSync()
{
  // most of all reset cummulative error - there is no need to keep time continuity
  _lastKnownError = 0;
}


bool EngineDDXB::CanVSyncTime() const
{
  return _refreshRate>0;
}

float EngineDDXB::GetTimeVSync() const
{
  int sinceLastVsync =  GetCPUCyclesRdtsc()-_lastVSyncTime;
  if (sinceLastVsync<=0) return _lastVSync;
  if (sinceLastVsync>=_vSyncInterval) return _lastVSync+1;
  return _lastVSync+float(sinceLastVsync)/_vSyncInterval;
}

float EngineDDXB::GetAvgFrameDurationVsync() const
{
  int sum = 0;
  for (int i=0; i<TrackFrames; i++)
  {
    sum += _frameDurationVSync[i];
  }
  return float(sum)/TrackFrames;
}
int EngineDDXB::GetMaxFrameDurationVsync() const
{
  int max = 0;
  for (int i=0; i<TrackFrames; i++)
  {
    if (_frameDurationVSync[i]>max) max = _frameDurationVSync[i];
  }
  return max;
}

int EngineDDXB::GetPresentationIntervalVSync() const
{
  return _presentationInterval;
}

int EngineDDXB::GetKnownErrorVsync() const
{
  return _lastKnownError;
}

int EngineDDXB::GetLastFrameEndVSync() const
{
  return _frameDisplayStartWantedVSync.Get(_lastSwapId);
}


#endif

/*!
\patch 1.21 Date 8/23/2001 by Ondra
- Improved: Added more information to message "Cannot create 3D device"
- Improved: W-buffer used on nVidia cards to impove z-buffer precision
in 16b modes.
\patch 1.30 Date 11/02/2001 by Ondra
- New: Ground multitexturing can be turned off in Video options.
This can be used to resolve compatibility issues
with some graphics adapters ("white ground problem").
\patch 1.34 Date 12/06/2001 by Ondra
- Fixed: W-buffer support was broken.
- New: W-buffer support enabled on nVidia cards in both 16b and 32b modes.
\patch 1.44 Date 2/13/2002 by Ondra
- Fixed: When no display adapter is selected in preferences
and no adapter is recognized, select first one.
This solves error message "Cannot create 3D device: Adapter -1"
*/

void EngineDDXB::Init3D()
{
  memset(&_pp,0,sizeof(_pp));

  // try to create device (from best to worst)
  HRESULT err;

  int adapter = D3DAdapter;
#ifndef _XBOX
  if (adapter<0 || adapter>=(int)_direct3D->GetAdapterCount())
  {
    // autodetect adapter

    int maxLevel = 0;
    for (int ad=0; ad<(int)_direct3D->GetAdapterCount(); ad++)
    {
      D3DADAPTER_IDENTIFIER8 id;
      _direct3D->GetAdapterIdentifier(ad,D3DENUM_NO_WHQL_LEVEL,&id);
      static const struct {const char *text;int level;} AdLevel[]=
      {
        "GeForce3",400,
          "Gladiac 920",400,
          "Voodoo5",300,
          "G850",270,
          "G800",250,
          "GeForce2",250,
          "Voodoo4",200,
          "GeForce",150,
          "KYRO",140,
          "G450",120,
          "TnT2",115, "TNT2",115,
          "TnT",110, "TNT",110,
          "Voodoo3",100,
          "G400",50,
          "Savage4",30,
          "Voodoo2",25,
          "740",20,
          "Voodoo",10,
      };

      for (int i=0; i<sizeof(AdLevel)/sizeof(*AdLevel); i++)
      {
        if (strstr(id.Description,AdLevel[i].text))
        {
          int level = AdLevel[i].level;
          LogF
            (
            "%s: Recognized as %s - %d",
            id.Description,AdLevel[i].text,level
            );
          if (level>maxLevel)
          {
            maxLevel = level;
            adapter = ad;
          }
          break;
        }
      }

    }
    if (adapter<0)
    {
      LogF("No adapter recognized by name");
      // if no adapter is recognized, select the first one
      adapter = 0;
    }
  }
#endif

  FindMode(adapter);

  D3DDEVTYPE devType = D3DDEVTYPE_HAL;
  //D3DDEVTYPE devType = D3DDEVTYPE_REF;

  D3DCAPS8 caps;
  _direct3D->GetDeviceCaps(adapter,devType,&caps);

  DWORD behavior = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

#if !VERTEX_SHADERS || _RELEASE
  if (caps.DevCaps&D3DDEVCAPS_HWTRANSFORMANDLIGHT)
  {
    behavior = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    if (caps.DevCaps&D3DDEVCAPS_PUREDEVICE)
    {
#ifndef _DEBUG
      // in debug we do not create pure device so that we can use vertex debugger
      behavior |= D3DCREATE_PUREDEVICE;
#endif
      LogF("T&L HW detected (Pure)");
    }
    else
    {
      LogF("T&L HW detected");
    }
  }
#endif

  _direct3D->SetPushBufferSize(GPUPushBufferSize,GPUKickOffSize);
  err = _direct3D->CreateDevice
    (
    adapter,devType,_hwndApp,behavior,
    &_pp,_d3DDevice.Init()
    );

  if (_d3DDevice)
  {
    /*
    int dwFree = _d3DDevice->GetAvailableTextureMem();
    if (dwFree<16*1024*1024)
    {
    _pp.BackBufferCount = 1;
    err = D3DERR_OUTOFVIDEOMEMORY;
    _d3DDevice.Free();
    LogF("Not enough memory for tripple buffering");
    LogF("Falling back to double buffering");
    }
    */
  }
  else
  {
    _pp.BackBufferCount = 1;
  }
  if (err!=D3D_OK || !_d3DDevice)
  {
    /*
    if (!pp.Windowed && pp.BackBufferCount<=2)
    {
    // wait for first v-sync
    pp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    LogF("Switching to double buffering");
    }
    */
    // TODO: check VP type
    // number of back buffer may be adjusted - try again
    err = _direct3D->CreateDevice
      (
      adapter,devType,_hwndApp,behavior,
      &_pp,_d3DDevice.Init()
      );
  }
  if (err!=D3D_OK || !_d3DDevice)
  {
    DDErrorXB("Cannot create 3D device",err);
    RptF
      (
      "Resolution failed: %dx%dx (%d Hz)",
      _pp.BackBufferWidth,_pp.BackBufferHeight,_pp.FullScreen_RefreshRateInHz
      );
#ifndef _XBOX
    D3DADAPTER_IDENTIFIER8 id;
    _direct3D->GetAdapterIdentifier(adapter,D3DENUM_NO_WHQL_LEVEL,&id);
    ErrorMessage
      (
      "Cannot create 3D device:\n"
      "  Adapter %d (%s) %s\n"
      "  Resolution %dx%d, format %s/%s, refresh %d Hz.\n"
      "  Error %s",
      (int)adapter,(const char *)id.Description,
      _pp.Windowed ? "Windowed" : "Fullscreen",
      _pp.BackBufferWidth,_pp.BackBufferHeight,
      SurfaceFormatName(_pp.BackBufferFormat),
      SurfaceFormatName(_pp.AutoDepthStencilFormat),
      _pp.FullScreen_RefreshRateInHz,
      DXGetErrorString8(err)
      );
#endif
  }
  LogF("Backbuffers: %d",_pp.BackBufferCount);
  /*
  if (!_hasStencilBuffer)
  {
  // no stencil buffer - we need z-bias badly
  _canZBias = (caps.RasterCaps&D3DPRASTERCAPS_ZBIAS)!=0;
  if (_canZBias)
  {
  LogF("Can z-bias");
  }
  }
  */
#ifdef _XBOX
  _d3DDevice->SetSwapCallback(SwapCallback);
  _d3DDevice->SetVerticalBlankCallback(VBlankCallback);
#if _ENABLE_REPORT
  _d3DDevice->SetWaitCallback(GPUIdleProfCallback);
#endif

  // note: persistent surface is not reference counted
  IDirect3DSurface8 *persist;
  _d3DDevice->GetPersistedSurface(&persist);
  if (persist)
  {
    DWORD size = persist->Size;
    DWORD height = ((size&D3DSIZE_HEIGHT_MASK)>>D3DSIZE_HEIGHT_SHIFT)+1;
    int pitchBlocks = (size&D3DSIZE_PITCH_MASK)>>D3DSIZE_PITCH_SHIFT;
    int pitch = (pitchBlocks+1)*D3DTEXTURE_PITCH_ALIGNMENT;
    LogF("Persistent pitch %d, size %d",pitch,pitch*height);
  }
  else
  {
    // no persisted surface - clear and swap all backbuffers
    for (int i=0; i<(int)_pp.BackBufferCount; i++)
    {
      _d3DDevice->Clear(0,NULL,D3DCLEAR_STENCIL|D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0,1,0);
      BackToFront(1);
    }
    // clear current backbuffer
    _d3DDevice->Clear(0,NULL,D3DCLEAR_STENCIL|D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0,1,0);
  }

#endif


#ifndef _XBOX
  _canWBuffer = (caps.RasterCaps&D3DPRASTERCAPS_WBUFFER)!=0;
#else
  _canWBuffer = false;
#endif
  _canWBuffer32 = _canWBuffer;

#if PIXEL_SHADERS
  _vertexShaders = 0; // pixel shader version
  _pixelShaders = 0;
  _usePixelShaders = false;
  _vertexShaders = caps.VertexShaderVersion&0xffff;
  _pixelShaders = caps.PixelShaderVersion&0xffff;
  if (_vertexShaders)
  {
    LogF("Vertex shaders version %d.%d",_vertexShaders>>8,_vertexShaders&0xff);
  }
  if (_pixelShaders)
  {
    LogF("Pixel shaders version %d.%d",_pixelShaders>>8,_pixelShaders&0xff);
  }
  if (_pixelShaders>=0x101)
  {
    _usePixelShaders = true;
  }
#endif
  if (_vertexShaders<0x100 || _pixelShaders<0x101)
  {
    ErrorMessage("Vertex and pixel shaders required");
    return;
  }

  if (_canWBuffer)
  {
    LogF("Can w-buffer");
  }

#ifdef _XBOX

  HRESULT hr;
  // Set constant mode to allow negative constants and to stop generating values for c-37 and c-38
  if (FAILED(hr = _d3DDevice->SetShaderConstantMode(D3DSCM_192CONSTANTS | D3DSCM_NORESERVEDCONSTANTS)))
  {
    DDErrorXB("Cannot set the constant mode", hr);
    RptF("Cannot set the constant mode");
  }

#if SHADER_INFO_PRINTING
  _vsInfoLastCount = 0;
#endif

  _shaderWater = dwFPShadersWaterVertexShader;
  _shaderWaterSimple = dwFPShadersWaterSimpleVertexShader;
  _shader = dwFPShadersVertexShader;
  _shaderGetPosNorm = dwFPShadersGetPosNormVertexShader;
  _shaderGetInst = dwFPShadersGetInstVertexShader;
  _shaderModShadow = dwFPShadersModShadowVertexShader;
  _shaderModInstShadow = dwFPShadersModInstShadowVertexShader;
  _shaderLandShadow = dwFPShadersLandShadowVertexShader;
  _shaderNormal = dwFPShadersNormalVertexShader;
  _shaderNormalThrough = dwFPShadersNormalThroughVertexShader;
  _shaderNormalDiffuse = dwFPShadersNormalDiffuseVertexShader;
  _shaderShadowVolume = dwFPShadersShadowVolumeVertexShader;
  _shaderPointO = dwFPShadersPointVertexShader;
  _shaderInstancing = dwFPShadersInstancingVertexShader;
  _shaderNormalInstancing = dwFPShadersNormalInstancingVertexShader;
  _shaderInstancingModulate = dwFPShadersInstancingModulateVertexShader;
  _shaderInstancingEmissive = dwFPShadersInstancingEmissiveVertexShader; 
  _shaderNormalThroughInstancing = dwFPShadersNormalThroughInstancingVertexShader;
  _shaderNormalDiffuseInstancing = dwFPShadersNormalDiffuseInstancingVertexShader;
  _shaderShadowVolumeInstancing = dwFPShadersShadowVolumeInstancingVertexShader;
  _shaderSpriteInstancing = dwFPShadersSpriteInstancingVertexShader;
  _shaderSkinning = dwFPShadersSkinningVertexShader;
  _shaderNormalSkinning = dwFPShadersNormalSkinningVertexShader;
  _shaderNormalDiffuseSkinning = dwFPShadersNormalDiffuseSkinningVertexShader;
  _shaderNormalThroughSkinning = dwFPShadersNormalThroughSkinningVertexShader;
  _shaderShadowVolumeSkinning = dwFPShadersShadowVolumeSkinningVertexShader;
  _shaderDirectional = dwFPShadersDirectionalVertexShader;
  _shaderDirectionalADForced = dwFPShadersDirectionalADForcedVertexShader;
  _shaderDirectionalNoSpec = dwFPShadersDirectionalNoSpecVertexShader;
  _shaderDirectionalSky = dwFPShadersDirectionalSkyVertexShader;
  _shaderDirectionalStars = dwFPShadersDirectionalStarsVertexShader;
  _shaderDirectionalSunObject = dwFPShadersDirectionalSunObjectVertexShader;
  _shaderDirectionalSunHaloObject = dwFPShadersDirectionalSunHaloObjectVertexShader;
  _shaderDirectionalMoonObject = dwFPShadersDirectionalMoonObjectVertexShader;
  _shaderDirectionalMoonHaloObject = dwFPShadersDirectionalMoonHaloObjectVertexShader;
  _shaderAlphaShadow = dwFPShadersAlphaShadowVertexShader;
  _shaderFog = dwFPShadersFogVertexShader;
  _shaderFogAlpha = dwFPShadersFogAlphaVertexShader;
  _shaderFogNone = dwFPShadersFogNoneVertexShader;
  _shaderPoint[0] = dwFPShadersPoint0VertexShader;
  _shaderPoint[1] = dwFPShadersPoint1VertexShader;
  _shaderPoint[2] = dwFPShadersPoint2VertexShader;
  _shaderPoint[3] = dwFPShadersPoint3VertexShader;
  //  _shaderPoint[4] = dwFPShadersPoint4VertexShader;
  //  _shaderPoint[5] = dwFPShadersPoint5VertexShader;
  //  _shaderPoint[6] = dwFPShadersPoint6VertexShader;
  //  _shaderPoint[7] = dwFPShadersPoint7VertexShader;
  _shaderSpot[0] = dwFPShadersSpot0VertexShader;
  _shaderSpot[1] = dwFPShadersSpot1VertexShader;
  _shaderSpot[2] = dwFPShadersSpot2VertexShader;
  _shaderSpot[3] = dwFPShadersSpot3VertexShader;
  //  _shaderSpot[4] = dwFPShadersSpot4VertexShader;
  //  _shaderSpot[5] = dwFPShadersSpot5VertexShader;
  //  _shaderSpot[6] = dwFPShadersSpot6VertexShader;
  //  _shaderSpot[7] = dwFPShadersSpot7VertexShader;
  _shaderWriteLights = dwFPShadersWriteLightsVertexShader;
  _shaderWriteLightsColorMod = dwFPShadersWriteLightsColorModVertexShader;
  _shaderWriteLightsAlphaMod = dwFPShadersWriteLightsAlphaModVertexShader;

  _shaderHandle = 0;

  //ComRef<XGBuffer> error;

  err = _d3DDevice->CreateVertexShader(SVertexVSNonTLDecl, dwFPShadersNonTLVertexShader, &_shaderNonTLHandle, 0);
  if (err!=D3D_OK)
  {
    _shaderNonTLHandle = 0;
    DDErrorXB("Cannot create vertex shader", err);
    RptF("Cannot create vertex shader");
  }
  err = _d3DDevice->CreateVertexShader(SVertexVSNonTL4TexDecl, dwFPShadersNonTLTex4VertexShader, &_shaderNonTLTex4Handle, 0);
  if (err!=D3D_OK)
  {
    _shaderNonTLTex4Handle = 0;
    DDErrorXB("Cannot create vertex shader", err);
    RptF("Cannot create vertex shader");
  }

  // Stencil visualization stuff
  err = _d3DDevice->CreateVertexShader(SVertexVSTransformedDecl, dwFPShadersStencilVisualisationVertexShader, &_shaderStencilVisualisationHandle, 0);
  if (err!=D3D_OK)
  {
    _shaderStencilVisualisationHandle = 0;
    DDErrorXB("Cannot create vertex shader", err);
    RptF("Cannot create vertex shader");
  }

  // Fill out shader caches identifiers
  {
    //////////////////////////////////////////////////////////////////////////
    // Frequent
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderGetInst);
      c.Add(_shaderModInstShadow);
      c.Add(_shaderInstancing);
      c.Add(_shaderInstancingEmissive);
      c.Add(_shaderFogAlpha);
      c.Add(_shaderAlphaShadow);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderModShadow);
      c.Add(_shaderNormal);
      c.Add(_shaderDirectionalADForced);
      c.Add(_shaderFog);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderModShadow);
      c.Add(_shaderNormal);
      c.Add(_shaderDirectionalADForced);
      c.Add(_shaderFog);
      c.Add(_shaderAlphaShadow);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderModShadow);
      c.Add(_shader);
      c.Add(_shaderDirectional);
      c.Add(_shaderFog);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderModShadow);
      c.Add(_shader);
      c.Add(_shaderDirectional);
      c.Add(_shaderFog);
      c.Add(_shaderAlphaShadow);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderGetInst);
      c.Add(_shaderModInstShadow);
      c.Add(_shaderNormalInstancing);
      c.Add(_shaderInstancingModulate);
      c.Add(_shaderDirectionalADForced);
      c.Add(_shaderFog);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderGetInst);
      c.Add(_shaderModInstShadow);
      c.Add(_shaderNormalInstancing);
      c.Add(_shaderInstancingModulate);
      c.Add(_shaderDirectionalADForced);
      c.Add(_shaderFog);
      c.Add(_shaderAlphaShadow);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderModShadow);
      c.Add(_shaderNormal);
      c.Add(_shaderDirectionalADForced);
      c.Add(_shaderFog);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderModShadow);
      c.Add(_shader);
      c.Add(_shaderDirectional);
      c.Add(_shaderFog);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderGetInst);
      c.Add(_shaderModInstShadow);
      c.Add(_shaderInstancing);
      c.Add(_shaderInstancingModulate);
      c.Add(_shaderDirectional);
      c.Add(_shaderFog);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderModShadow);
      c.Add(_shaderSkinning);
      c.Add(_shaderDirectional);
      c.Add(_shaderFog);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderGetInst);
      c.Add(_shaderModInstShadow);
      c.Add(_shaderInstancing);
      c.Add(_shaderInstancingModulate);
      c.Add(_shaderDirectional);
      c.Add(_shaderFog);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderGetInst);
      c.Add(_shaderModInstShadow);
      c.Add(_shaderInstancing);
      c.Add(_shaderInstancingModulate);
      c.Add(_shaderDirectional);
      c.Add(_shaderFog);
      c.Add(_shaderAlphaShadow);
      c.Add(_shaderWriteLights);
    }
//    {
//      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
//      c.Add(_shaderGetPosNorm);
//      c.Add(_shaderLandShadow);
//      c.Add(_shaderNormalDiffuse);
//      c.Add(_shaderDirectionalADForced);
//      c.Add(_shaderFog);
//      c.Add(_shaderAlphaShadow);
//      c.Add(_shaderWriteLights);
//    }
//    {
//      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
//      c.Add(_shaderGetPosNorm);
//      c.Add(_shaderLandShadow);
//      c.Add(_shaderNormalDiffuse);
//      c.Add(_shaderDirectionalADForced);
//      c.Add(_shaderFog);
//      c.Add(_shaderWriteLightsAlphaMod);
//    }
//    {
//      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
//      c.Add(_shaderGetPosNorm);
//      c.Add(_shaderLandShadow);
//      c.Add(_shader);
//      c.Add(_shaderDirectional);
//      c.Add(_shaderFog);
//      c.Add(_shaderAlphaShadow);
//      c.Add(_shaderWriteLights);
//    }

    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderGetInst);
      c.Add(_shaderModInstShadow);
      c.Add(_shaderNormalThroughInstancing);
      c.Add(_shaderInstancingModulate);
      c.Add(_shaderDirectionalADForced);
      c.Add(_shaderFog);
      c.Add(_shaderAlphaShadow);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderModShadow);
      c.Add(_shaderNormalThrough);
      c.Add(_shaderDirectionalADForced);
      c.Add(_shaderFog);
      c.Add(_shaderAlphaShadow);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderGetInst);
      c.Add(_shaderModInstShadow);
      c.Add(_shaderInstancing);
      c.Add(_shaderInstancingEmissive);
      c.Add(_shaderFogAlpha);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderModShadow);
      c.Add(_shaderNormalDiffuseSkinning);
      c.Add(_shaderDirectionalADForced);
      c.Add(_shaderFog);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderModShadow);
      c.Add(_shaderNormalSkinning);
      c.Add(_shaderDirectionalADForced);
      c.Add(_shaderFog);
      c.Add(_shaderWriteLights);
    }

    // Land shaders
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderLandShadow);
      c.Add(_shaderNormalDiffuse);
      c.Add(_shaderDirectionalADForced);
      c.Add(_shaderFog);
      c.Add(_shaderAlphaShadow);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderLandShadow);
      c.Add(_shaderNormalDiffuse);
      c.Add(_shaderDirectionalADForced);
      c.Add(_shaderFog);
      c.Add(_shaderWriteLightsAlphaMod);
    }
//    { // CHOPT Wrong shader after optimization in combination with lights spot, spot, point, point
//      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
//      c.Add(_shaderGetPosNorm);
//      c.Add(_shaderLandShadow);
//      c.Add(_shader);
//      c.Add(_shaderDirectional);
//      c.Add(_shaderFog);
//      c.Add(_shaderAlphaShadow);
//      c.Add(_shaderWriteLights);
//    }

    // Medium frequent shaders
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderModShadow);
      c.Add(_shaderNormalThrough);
      c.Add(_shaderDirectionalADForced);
      c.Add(_shaderFog);
      c.Add(_shaderWriteLights);
    }

    //////////////////////////////////////////////////////////////////////////
    // Old
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderGetInst);
      c.Add(_shaderModInstShadow);
      c.Add(_shaderInstancing);
      c.Add(_shaderInstancingModulate);
      c.Add(_shaderDirectional);
      c.Add(_shaderFogAlpha);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderModShadow);
      c.Add(_shaderSkinning);
      c.Add(_shaderDirectional);
      c.Add(_shaderFog);
      c.Add(_shaderWriteLights);
    }

    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderModShadow);
      c.Add(_shaderNormalDiffuseSkinning);
      c.Add(_shaderDirectionalADForced);
      c.Add(_shaderFog);
      c.Add(_shaderAlphaShadow);
      c.Add(_shaderWriteLights);
    }

    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderModShadow);
      c.Add(_shaderNormalSkinning);
      c.Add(_shaderDirectionalADForced);
      c.Add(_shaderFog);
      c.Add(_shaderAlphaShadow);
      c.Add(_shaderWriteLights);
    }

    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderModShadow);
      c.Add(_shaderSkinning);
      c.Add(_shaderDirectional);
      c.Add(_shaderFog);
      c.Add(_shaderAlphaShadow);
      c.Add(_shaderWriteLights);
    }

    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderGetInst);
      c.Add(_shaderModInstShadow);
      c.Add(_shaderInstancing);
      c.Add(_shaderInstancingEmissive);
      c.Add(_shaderDirectional);
      c.Add(_shaderFog);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderGetInst);
      c.Add(_shaderModInstShadow);
      c.Add(_shaderNormalInstancing);
      c.Add(_shaderInstancingEmissive);
      c.Add(_shaderDirectionalADForced);
      c.Add(_shaderFog);
      c.Add(_shaderWriteLights);
    }

    //////////////////////////////////////////////////////////////////////////
    // Existing
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderModShadow);
      c.Add(_shader);
      c.Add(_shaderDirectional);
      c.Add(_shaderFogNone);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderModShadow);
      c.Add(_shader);
      c.Add(_shaderDirectional);
      c.Add(_shaderFogAlpha);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderModShadow);
      c.Add(_shader);
      c.Add(_shaderDirectional);
      c.Add(_shaderFogNone);
      c.Add(_shaderAlphaShadow);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderModShadow);
      c.Add(_shader);
      c.Add(_shaderDirectional);
      c.Add(_shaderFogAlpha);
      c.Add(_shaderAlphaShadow);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheLightId.Append();
      c.Add(_shaderSpriteInstancing);
      c.Add(_shaderDirectional);
      c.Add(_shaderFogAlpha);
      c.Add(_shaderWriteLightsColorMod);
    }

    //////////////////////////////////////////////////////////////////////////
    // Shaders with no lights    
    {
      ShaderCacheId::Creator c = _shaderCacheId.Append();
      c.Add(_shaderPointO);
      c.Add(_shaderDirectionalStars);
      c.Add(_shaderWriteLights);
      c.Add(_shaderFogNone);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderModShadow);
      c.Add(_shader);
      c.Add(_shaderDirectionalSky);
      c.Add(_shaderFogNone);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderModShadow);
      c.Add(_shader);
      c.Add(_shaderDirectionalMoonHaloObject);
      c.Add(_shaderFogNone);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderModShadow);
      c.Add(_shader);
      c.Add(_shaderDirectionalMoonObject);
      c.Add(_shaderFogNone);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderModShadow);
      c.Add(_shader);
      c.Add(_shaderDirectionalSunHaloObject);
      c.Add(_shaderFogNone);
      c.Add(_shaderWriteLights);
    }
    {
      ShaderCacheId::Creator c = _shaderCacheId.Append();
      c.Add(_shaderGetPosNorm);
      c.Add(_shaderModShadow);
      c.Add(_shader);
      c.Add(_shaderDirectionalSunObject);
      c.Add(_shaderFogNone);
      c.Add(_shaderWriteLights);
    }

  }

  // data driver - avoid repeating code
  struct PSInfo
  {
    const DWORD *src;
    DWORD &tgt;
  };
  const PSInfo psList[]=
  {
    {dwPsStencilVisualisationPixelShader,_psStencilVisualisation},
    {dwPsPostNightPixelShader,_psPostNight},
    {dwPsPostGlowPixelShader,_psPostGlow},
    {dwPsPostBrightQueryPixelShader,_psPostBrightQuery},    
    {dwPsPostDepthOfFieldPixelShader,_psPostDepthOfField},
    {dwPsPostDepthOfFieldFarOnlyPixelShader,_psPostDepthOfFieldFarOnly},
    {dwPsPostDepthToAPixelShader,_psPostDepthToA},
    {dwPsPostCopyPixelShader,_psPostCopy},
    {dwPsPostShadowVisualisationPixelShader,_psPostShadowVisualisation},
    {dwPsPostShadowVisualisationAISPixelShader,_psPostShadowVisualisationAIS},
  };
  for (int i=0; i<lenof(psList); i++)
  {
    err = _d3DDevice->CreatePixelShader((D3DPIXELSHADERDEF*) psList[i].src, &psList[i].tgt);
    if (FAILED(err))
    {
      psList[i].tgt = NULL;
      RptF("Cannot create pixel shader");
    }
  }

  err = _d3DDevice->CreateVertexBuffer(STENCILPLATE_NUMBER * 6 * sizeof(D3DXVECTOR4), WRITEONLYVB, 0, D3DPOOL_DEFAULT, _vbStencilVisualisation.Init());
  if (FAILED(err))
  {
    RptF("Cannot create vertex buffer");
  }

  D3DXVECTOR4* v;
  _vbStencilVisualisation->Lock(0, 0, (byte**)&v, 0);
  for (int i = 0; i < STENCILPLATE_NUMBER; i++)
  {
    float d = i * (1.0f / (STENCILPLATE_NUMBER-1));
    float di = i * (1.0f / STENCILPLATE_NUMBER);
    // 3-th is used for distance computation
    // 4-th is used for plate shadow intensity computation
    v[0] = D3DXVECTOR4( 0, _h, d, 1-di);
    v[1] = D3DXVECTOR4( 0,  0, d, 1-di);
    v[2] = D3DXVECTOR4(_w, _h, d, 1-di);
    v[3] = D3DXVECTOR4(_w, _h, d, 1-di);
    v[4] = D3DXVECTOR4( 0,  0, d, 1-di);
    v[5] = D3DXVECTOR4(_w,  0, d, 1-di);
    v += 6;
  }
  _vbStencilVisualisation->Unlock();

  err = _d3DDevice->CreateVertexBuffer(14*sizeof(SVertexXB), WRITEONLYVB, 0, D3DPOOL_DEFAULT, _vbMinMaxCube.Init());
  if (SUCCEEDED(err))
  {
    SVertexXB *vertices;
    _vbMinMaxCube->Lock(0, 0, (byte**)&vertices, 0);

    // single strip cube rendering
    const static Vector3 cubeVertices[14]=
    {
      Vector3(0,1,1),Vector3(1,1,1),
        Vector3(0,1,0),Vector3(1,1,0),
        Vector3(1,0,0),Vector3(1,1,1),
        Vector3(1,0,1),Vector3(0,1,1),
        Vector3(0,0,1),Vector3(0,1,0),
        Vector3(0,0,0),Vector3(1,0,0),
        Vector3(0,0,1),Vector3(1,0,1)
    };

    for (int i=0; i<14; i++)
    {
      SVertexXB &v = vertices[i];
      Vector3Val pos = cubeVertices[i];
      CopyPos(v.pos,pos,Vector3(1,1,1),Vector3(0,0,0),Vector3(1,1,1));
      CopyNormal(v.norm,VUp);
      v.t0.u = 0;
      v.t0.v = 0;
    }
    _vbMinMaxCube->Unlock();

  }

#endif

  // check if detail texturing can be enabled
  // generic test
  _canDetailTex = true;
  if (caps.MaxTextureBlendStages<2 || caps.MaxSimultaneousTextures<2)
  {
    _canDetailTex = false;
    LogF("Not enough texture stages for detail texturing");
  }
  else
  {
    // check for required blending ops

    //if (caps.PixelShaderVersion<0x101)
    {
      const int requiredOps = D3DTEXOPCAPS_MODULATE|D3DTEXOPCAPS_MODULATE2X|D3DTEXOPCAPS_MODULATE4X;
      if ((caps.TextureOpCaps&requiredOps)!=requiredOps)
      {
        _canDetailTex = false;
        LogF("Texture ops not supported");
      }
    }

  }

  if (!_canDetailTex)
  {
    Engine::SetMultitexturing(false);
  }

  // test for some known adapters
  // (based on driver name?)



  _useDXTL = true;

  // patch: detection of known device capabilities
#ifndef _XBOX
  const int vendor_nVidia = 0x10de;
  const int vendor_3Dfx = 0x121a;
  D3DADAPTER_IDENTIFIER8 id;
  _direct3D->GetAdapterIdentifier(adapter,D3DENUM_NO_WHQL_LEVEL,&id);

  if (id.VendorId==vendor_3Dfx)
  {
    const int voodoo3 = 5;
    if (id.DeviceId==voodoo3)
    {
      LogF("Voodoo3 detected - detail texture disabled");
      _canDetailTex = false;
    }
  }

  // on nVidia cards do not use w-buffer, it is broken
  // do not use z-bias with HW T&L cards
  if (id.VendorId==vendor_nVidia || _useDXTL)
#endif
  {
    //_canWBuffer32 = false;
  }
#ifdef _XBOX
  D3DDISPLAYMODE dmode;
  _d3DDevice->GetDisplayMode(&dmode);
  LogF
    (
    "Format %s:%dx%d",
    SurfaceFormatName(dmode.Format),dmode.Width,dmode.Height
    );
#endif


  // check actual device capabilities
  Init3DState();

#if defined _XBOX && _ENABLE_PERFLOG && _ENABLE_PERFLIB
  D3DPERF_StartPerfProfile();
#endif
}

#if defined _XBOX && _ENABLE_PERFLOG && _ENABLE_PERFLIB
const int CPUCyclesMs = 733000;
void WINAPI MyDumpCounterCycleInfo(
                                   D3DPERF_PerformanceCounters perfctr, ULONGLONG cycles)
{
  //const char *name = FindEnumName(perfctr);
  D3DPERF *perf = D3DPERF_GetStatistics();
  D3DPERFNAMES *perfNames = D3DPERF_GetNames();
  LogF(
    "(#%Lu) %s %I64u cycles (%Lums)",
    perf->m_PerformanceCounters[perfctr].Count,
    perfNames->m_PerformanceCounterNames[perfctr],
    cycles, ULONGLONG(cycles / CPUCyclesMs)
    );
  //if (cycles>=CPUCyclesMs*4)
  //{
  //  LogF("WARN: Long wait");
  //}
}
#endif


//D3DWAITCALLBACK

void EngineDDXB::EnablePerfCap(bool perfCap)
{
#if defined _XBOX 
  if (perfCap)
  {
#if _ENABLE_REPORT
    _d3DDevice->SetWaitCallback(GPUIdleCallback);
#endif
#if _ENABLE_PERFLOG && _ENABLE_PERFLIB
#if 0
    D3DPERF *perf = D3DPERF_GetStatistics();
    perf->m_dwDumpFPSInfoMask = D3DPERF_DUMP_FPS_WAITINFO;
    perf->m_DumpWaitCycleTimesThreshold = CPUCyclesMs*10;
    perf->m_pfnCycleThresholdHandler = MyDumpCounterCycleInfo;
#endif
    // erase previous slow frame info
#if _FASTCAP
    bool DeleteDirectoryStructure(const char *name, bool deleteDir =true);
    DeleteDirectoryStructure("u:\\fastcap",false);
    CreateDirectory("u:\\fastcap",NULL);
#elif _PROFILE
    /*
    DeleteDirectoryStructure("u:\\cpx",false);
    CreateDirectory("u:\\cpx",NULL);
    XPERFCPXSTATE cpxState;
    cpxState.Threshold = 2000000;
    cpxState.SamplingMethod = XPERF_SAMPLING_TIMER;
    XPerfSetCPXState(&cpxState);
    */
#endif
#endif
  }
  else
  {
#if _ENABLE_REPORT
    _d3DDevice->SetWaitCallback(GPUIdleProfCallback);
#endif
#if 0 //_ENABLE_PERFLOG && _ENABLE_PERFLIB
    D3DPERF_SetShowFrameRateInterval(0);
    D3DPERF *perf = D3DPERF_GetStatistics();
    perf->m_DumpWaitCycleTimesThreshold = INT_MAX;
#endif
  }
#endif
}

void EngineDDXB::SetDebugMarker(int id)
{
  _d3DDevice->SetDebugMarker(id);
}

int EngineDDXB::ProfileBeginGraphScope(unsigned int color,const char *name) const
{
  #if _ENABLE_PERFLIB
  // name is limited to 31 chars
  const int maxName = 31;
  int len = strlen(name);
  if (len>maxName) name += len-maxName;
  return D3DPERF_BeginEvent(color,name);
  #else
  return 0;
  #endif
}
int EngineDDXB::ProfileEndGraphScope() const
{
  #if _ENABLE_PERFLIB
  return D3DPERF_EndEvent();
  #else
  return 0;
  #endif
}

void EngineDDXB::DoPerfCap(bool perfCap, bool slowFrame)
{
#if defined _XBOX && _ENABLE_PERFLOG && _ENABLE_PERFLIB
  if (perfCap)
  {
    static int counter = 1;
    if (slowFrame)
    {
#if _FASTCAP
      // save profiling info
      LogF("Stored in u:\\fastcap\\slowframe_%02d.xpv", counter);
      DmStopProfiling();
#elif _PROFILE
      /*
      XPerfEndCapture();
      */
#endif
      // advance to the next file
      if (++counter>=100) counter = 1;
      // the frame was slow - analyse it
      //D3DPERF_DumpFrameRateInfo();
    }
    D3DPERF_Reset();
#if _FASTCAP
    BString<256> name;
    sprintf(name,"u:\\fastcap\\slowframe_%02d.xpv",counter);
    DmStartProfiling(name,0);
#elif _PROFILE
    /*
    BString<256> name;
    sprintf(name,"u:\\cpx\\slowframe_%02d.cpx",counter);
    XPerfBeginCapture(name,XPERF_USECPX);
    */
#endif
  }
#endif
}

void EngineDDXB::SetShadowIntensity(float intensity)
{
  const float modulate[4] = {intensity,intensity,intensity,intensity};
  _d3DDevice->SetVertexShaderConstant(SHADOW_MODULATE_DIFFUSE,modulate, 1);
}

void EngineDDXB::SetShadowFactor(ColorVal shadowFactor)
{
  base::SetShadowFactor(shadowFactor);

  if (CanUseNewShadows())
  {
    // This is not a proper place to set these constants
    const float constants[4] = {-2.0f / Glob.config.shadowsZ, 2.0f, -16.0f / 256.0f, 1.0f};
    const float modulate[4] = {1,1,1,1};
    _d3DDevice->SetVertexShaderConstant(SHADOW_MUL__ADD__INTENSITY, constants, 1);
    _d3DDevice->SetVertexShaderConstant(SHADOW_MODULATE_DIFFUSE,modulate, 1);
  }
}

void EngineDDXB::Done3D()
{
  _vbStencilVisualisation.Free();
  _vbMinMaxCube.Free();
  _d3DDevice->SetShaderConstantMode(0);
  _d3DDevice->SetVertexShaderInputDirect(NULL, 0, NULL);
  _d3DDevice->SetVertexShader(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX2);
  _d3DDevice->DeleteVertexShader(_shaderHandle);
  _d3DDevice->DeleteVertexShader(_shaderNonTLHandle);
  _d3DDevice->DeleteVertexShader(_shaderNonTLTex4Handle);
  _d3DDevice->DeleteVertexShader(_shaderStencilVisualisationHandle);
  _d3DDevice->DeletePixelShader(_psStencilVisualisation);
  _d3DDevice->DeletePixelShader(_psPostNight);
  _d3DDevice->DeletePixelShader(_psPostGlow);
  _d3DDevice->DeletePixelShader(_psPostBrightQuery);
  _d3DDevice->DeletePixelShader(_psPostShadowVisualisation);
  _d3DDevice->DeletePixelShader(_psPostShadowVisualisationAIS);
  _d3DDevice->DeletePixelShader(_psPostDepthOfField);
  _d3DDevice->DeletePixelShader(_psPostDepthToA);
  _d3DDevice->DeletePixelShader(_psPostCopy);
}

AbstractTextBank *EngineDDXB::TextBank() {return _textBank;}

void EngineDDXB::CreateTextBank(ParamEntryPar cfg,ParamEntryPar fcfg)
{
  if( !_textBank )
  {
    _textBank=new TextBankD3DXB(this,cfg,fcfg);
    if( !_textBank )
    {
      ErrorMessage("Cannot create texture bank.");
      return;
    }
  }
}


void EngineDDXB::FlushMemory(bool deep)
{
  if (deep)
  {
    // flush all vertex buffers and pushbuffers to avoid any fragmentation
    FreeAll();
  }
  base::FlushMemory(deep);
}

void EngineDDXB::D3DDrawTexts()
{
}

//#define SPEC_2D (NoZBuf|IsAlpha|ClampU|ClampV|IsAlphaFog)

inline bool EngineDDXB::ClipDraw2D
(
 float &xBeg, float &yBeg, float &xEnd, float &yEnd,
 float &uBeg, float &vBeg, float &uEnd, float &vEnd,
 const Draw2DParsExt &pars, const Rect2DAbs &rect, const Rect2DAbs &clip
 )
{
  // perform simple clipping
  xBeg=rect.x /*-0.5f*/;
  xEnd=xBeg+rect.w;
  yBeg=rect.y /*-0.5f*/;
  yEnd=yBeg+rect.h;

  uBeg=0;
  vBeg=0;
  uEnd=1;
  vEnd=1;

  // veirfy mapping is linear
  //Assert( fabs(pars.vTL+uEnd*(pars.vTR-pars.vTL)+vEnd*(pars.vBL-pars.vTL)-pars.vBR)<1e-10);
  //Assert( fabs(pars.uTL+uEnd*(pars.uTR-pars.uTL)+vEnd*(pars.uBL-pars.uTL)-pars.uBR)<1e-10);

  float xc=floatMax(clip.x,0);
  float yc=floatMax(clip.y,0);
  float xec=floatMin(clip.x+clip.w,_w);
  float yec=floatMin(clip.y+clip.h,_h);

  bool clipped = false;
  if( xBeg<xc )
  {
    // -xBeg is out, side length is 2*sizeX
    uBeg=(xc-xBeg)/rect.w;
    xBeg=xc;
    clipped = true;
  }
  if( xEnd>xec )
  {
    // xEnd-_w is out, side length is 2*sizeX
    uEnd=1-(xEnd-xec)/rect.w;
    xEnd=xec;
    clipped = true;
  }
  if( yBeg<yc )
  {
    // -yBeg is out, side length is 2*sizeY
    vBeg=(yc-yBeg)/rect.h;
    yBeg=yc;
    clipped = true;
  }
  if( yEnd>yec )
  {
    // yEnd-_h is out, side length is 2*sizeY
    vEnd=1-(yEnd-yec)/rect.h;
    yEnd=yec;
    clipped = true;
  }
  return clipped;
}

void EngineDDXB::Draw2D
(
 const Draw2DParsExt &pars, const Rect2DAbs &rect, const Rect2DAbs &clip
 )
{
  //Draw2DPars pars = _pars;
  Assert( pars.mip.IsOK() );
  if( !pars.mip.IsOK() ) return;

  if (_resetNeeded) return;

  Assert( fabs(pars.vTR+pars.vBL-pars.vTL-pars.vBR)<1e-6);
  Assert( fabs(pars.uTR+pars.uBL-pars.uTL-pars.uBR)<1e-6);

  // perform simple clipping
  float xBeg,yBeg,xEnd,yEnd,uBeg,vBeg,uEnd,vEnd;
  bool clipped = ClipDraw2D(xBeg,yBeg,xEnd,yEnd,uBeg,vBeg,uEnd,vEnd,pars,rect,clip);

  if( xBeg>=xEnd || yBeg>=yEnd ) return;

  // note: colors should be "clipped" as well
  TLVertex pos[4];
  pos[0].rhw=1;
  pos[0].color=pars.colorTL;
  pos[0].specular=PackedColor(0xff000000);
  pos[0].pos[2]=0.5;

  pos[1].rhw=1;
  pos[1].color=pars.colorTR;
  pos[1].specular=PackedColor(0xff000000);
  pos[1].pos[2]=0.5;

  pos[2].rhw=1;
  pos[2].color=pars.colorBR;
  pos[2].specular=PackedColor(0xff000000);
  pos[2].pos[2]=0.5;

  pos[3].rhw=1;
  pos[3].color=pars.colorBL;
  pos[3].specular=PackedColor(0xff000000);
  pos[3].pos[2]=0.5;

  if (!clipped)
  {
    // optimized common version
    pos[0].t0.u = pars.uTL;
    pos[1].t0.u = pars.uTR;
    pos[3].t0.u = pars.uBL;
    pos[2].t0.u = pars.uBR;

    pos[0].t0.v = pars.vTL;
    pos[1].t0.v = pars.vTR;
    pos[3].t0.v = pars.vBL;
    pos[2].t0.v = pars.vBR;
  }
  else
  {
    pos[0].t0.u = pars.uTL+uBeg*(pars.uTR-pars.uTL)+vBeg*(pars.uBL-pars.uTL);
    pos[1].t0.u = pars.uTL+uEnd*(pars.uTR-pars.uTL)+vBeg*(pars.uBL-pars.uTL);
    pos[3].t0.u = pars.uTL+uBeg*(pars.uTR-pars.uTL)+vEnd*(pars.uBL-pars.uTL);
    pos[2].t0.u = pars.uTL+uEnd*(pars.uTR-pars.uTL)+vEnd*(pars.uBL-pars.uTL);

    pos[0].t0.v = pars.vTL+uBeg*(pars.vTR-pars.vTL)+vBeg*(pars.vBL-pars.vTL);
    pos[1].t0.v = pars.vTL+uEnd*(pars.vTR-pars.vTL)+vBeg*(pars.vBL-pars.vTL);
    pos[3].t0.v = pars.vTL+uBeg*(pars.vTR-pars.vTL)+vEnd*(pars.vBL-pars.vTL);
    pos[2].t0.v = pars.vTL+uEnd*(pars.vTR-pars.vTL)+vEnd*(pars.vBL-pars.vTL);
  }



  pos[0].pos[0]=xBeg,pos[0].pos[1]=yBeg;
  pos[1].pos[0]=xEnd,pos[1].pos[1]=yBeg;
  pos[2].pos[0]=xEnd,pos[2].pos[1]=yEnd;
  pos[3].pos[0]=xBeg,pos[3].pos[1]=yEnd;

  SwitchRenderMode(RM2DTris);
  SwitchTL(false);

  AddVertices(pos,4); // note: may flush all queues

  QueuePrepareTriangle(pars.mip,NULL,pars.spec);

  Queue2DPoly(pos,4);
}

void EngineDDXB::Draw2D
(
 const Draw2DPars &pars, const Rect2DAbs &rect, const Rect2DAbs &clip
 )
{
  // TODO: special case optimization
  Draw2D(Draw2DParsExt(pars),rect,clip);
}
void EngineDDXB::Draw2D
(
 const Draw2DParsNoTex &pars, const Rect2DAbs &rect, const Rect2DAbs &clip
 )
{
  // TODO: special case optimization
  Draw2D(Draw2DParsExt(pars),rect,clip);
}

void EngineDDXB::DrawLinePrepare()
{
  // use line texture
  Texture *tex = GPreloadedTextures.New(TextureLine);
  const MipInfo &mip = _textBank->UseMipmap(tex,1,1);
  int specFlags = NoZBuf|IsAlpha|ClampU|ClampV|IsAlphaFog;

  DrawPolyPrepare(mip,specFlags);
}

void EngineDDXB::DrawLineDo
(
 const Line2DAbs &line,
 PackedColor c0, PackedColor c1,
 const Rect2DAbs &clip
 )
{
  float x0 = line.beg.x;
  float y0 = line.beg.y;
  float x1 = line.end.x;
  float y1 = line.end.y;

  // convert line to poly;
  float dx = x1-x0;
  float dy = y1-y0;
  float dSize2 = dx*dx+dy*dy;
  float invDSize = dSize2 >0 ? InvSqrt(dSize2) : 1;

  // direction perpendicular dx, dy
  // TODO: use color alpha as width
  // 2D line drawing
  float pdx = +dy*invDSize, pdy = -dx*invDSize;
  float w = 3.0f;
  x0 -= pdx*(w*0.5);
  x1 -= pdx*(w*0.5);
  y0 -= pdy*(w*0.5);
  y1 -= pdy*(w*0.5);
  float x0Side = x0+pdx*w, y0Side = y0+pdy*w;
  float x1Side = x1+pdx*w, y1Side = y1+pdy*w;


  Vertex2DAbs vertices[4];
  float off = 0;
  //float off = 0.5f;
  vertices[0].x = x0-off;
  vertices[0].y = y0-off;
  vertices[0].u = 0;
  vertices[0].v = 0.25;
  vertices[0].color = c0;

  vertices[1].x = x0Side-off;
  vertices[1].y = y0Side-off;
  vertices[1].u = 0;
  vertices[1].v = 1;
  vertices[1].color = c0;

  vertices[3].x = x1-off;
  vertices[3].y = y1-off;
  vertices[3].u = 0.1;
  vertices[3].v = 0.25;
  vertices[3].color = c1;

  vertices[2].x = x1Side-off;
  vertices[2].y = y1Side-off;
  vertices[2].u = 0.1;
  vertices[2].v = 1;
  vertices[2].color = c1;

  DrawPolyDo(vertices,4,clip);
}

void EngineDDXB::DrawPolyPrepare(const MipInfo &mip, int specFlags)
{
  if (_resetNeeded) return;

  SwitchRenderMode(RM2DTris);
  SwitchTL(false);
  //FlushAllQueues(_queueNo);
  //AddVertices(gv,n); // note: may flush all queues
  QueuePrepareTriangle(mip,NULL,specFlags);
}

void EngineDDXB::DrawPolyDo
(
 const Vertex2DPixel *vertices, int n, const Rect2DPixel &clipRect
 )
{
  if (_resetNeeded) return;

  const int maxN=32;

  // reject poly if fully outside or invalid
  ClipFlags orClip=0;
  ClipFlags andClip=ClipAll;
  ClipFlags clipV[maxN];
  for( int i=0; i<n; i++ )
  {
    const Vertex2DPixel &vs=vertices[i];
    float x=vs.x;
    float y=vs.y;
    ClipFlags clip=0;
    if( x<clipRect.x ) clip|=ClipLeft;
    else if( x>clipRect.x+clipRect.w ) clip|=ClipRight;
    if( y<clipRect.y ) clip|=ClipTop;
    else if( y>clipRect.y+clipRect.h ) clip|=ClipBottom;
    clipV[i]=clip;
    orClip|=clip;
    andClip&=clip;
  }
  if( andClip ) return;
  // 2D clipping (with orClip flags)
  TLVertex gv[maxN];
  float x2d = Left2D();
  float y2d = Top2D();
  if( orClip )
  {
    Vertex2DPixel clippedVertices1[maxN]; // temporay buffer to keep clipped result
    Vertex2DPixel clippedVertices2[maxN]; // temporay buffer to keep clipped result

    Vertex2DPixel *free=clippedVertices1;
    Vertex2DPixel *used=clippedVertices2;
    // perform clipping
    for( int i=0; i<n; i++ ) used[i]=vertices[i];
    if( orClip&ClipTop ) n=Clip2D(clipRect,free,used,n,InsideTopPixel,InterpolateVertexPixel),swap(free,used);
    if( orClip&ClipBottom ) n=Clip2D(clipRect,free,used,n,InsideBottomPixel,InterpolateVertexPixel),swap(free,used);
    if( orClip&ClipLeft ) n=Clip2D(clipRect,free,used,n,InsideLeftPixel,InterpolateVertexPixel),swap(free,used);
    if( orClip&ClipRight ) n=Clip2D(clipRect,free,used,n,InsideRightPixel,InterpolateVertexPixel),swap(free,used);
    // use result
    if (n<3) return; // nothing to draw
    vertices=used;

    if( n>maxN )
    {
      n=maxN;
      Fail("Poly: Too much vertices");
    }

    for( int i=0; i<n; i++ )
    {
      TLVertex *v=&gv[i];
      const Vertex2DPixel &vs=vertices[i];

      v->pos[0]=vs.x+x2d;
      v->pos[1]=vs.y+y2d;
      v->pos[2]=vs.z;
      v->rhw=vs.w;
      v->color=vs.color;
      v->specular=PackedColor(0xff000000);
      //v->fog=0;
      v->t0.u=vs.u;
      v->t0.v=vs.v;
      // tmu1vtx?
    }
  }
  else
  {
    if( n>maxN )
    {
      n=maxN;
      Fail("Poly: Too much vertices");
    }

    for( int i=0; i<n; i++ )
    {
      TLVertex *v=&gv[i];
      const Vertex2DPixel &vs=vertices[i];

      v->pos[0]=vs.x+x2d;
      v->pos[1]=vs.y+y2d;
      v->pos[2]=vs.z;
      v->rhw=vs.w;
      v->color=vs.color;
      v->specular=PackedColor(0xff000000);
      //v->fog=0;
      v->t0.u=vs.u;
      v->t0.v=vs.v;
      // tmu1vtx?
    }
  }


  AddVertices(gv,n); // note: may flush all queues - but it must not free them - can be dangerous?
  Queue2DPoly(gv,n);
}

void EngineDDXB::DrawPolyDo
(
 const Vertex2DAbs *vertices, int n, const Rect2DAbs &clipRect
 )
{
  if (_resetNeeded) return;

  const int maxN=32;

  // reject poly if fully outside or invalid
  ClipFlags orClip=0;
  ClipFlags andClip=ClipAll;
  ClipFlags clipV[maxN];
  for( int i=0; i<n; i++ )
  {
    const Vertex2DAbs &vs=vertices[i];
    float x=vs.x;
    float y=vs.y;
    ClipFlags clip=0;
    if( x<clipRect.x ) clip|=ClipLeft;
    else if( x>clipRect.x+clipRect.w ) clip|=ClipRight;
    if( y<clipRect.y ) clip|=ClipTop;
    else if( y>clipRect.y+clipRect.h ) clip|=ClipBottom;
    clipV[i]=clip;
    orClip|=clip;
    andClip&=clip;
  }
  if( andClip ) return;
  // 2D clipping (with orClip flags)
  TLVertex gv[maxN];
  if( orClip )
  {
    Vertex2DAbs clippedVertices1[maxN]; // temporay buffer to keep clipped result
    Vertex2DAbs clippedVertices2[maxN]; // temporay buffer to keep clipped result

    Vertex2DAbs *free=clippedVertices1;
    Vertex2DAbs *used=clippedVertices2;
    // perform clipping
    for( int i=0; i<n; i++ ) used[i]=vertices[i];
    if( orClip&ClipTop ) n=Clip2D(clipRect,free,used,n,InsideTopAbs,InterpolateVertexAbs),swap(free,used);
    if( orClip&ClipBottom ) n=Clip2D(clipRect,free,used,n,InsideBottomAbs,InterpolateVertexAbs),swap(free,used);
    if( orClip&ClipLeft ) n=Clip2D(clipRect,free,used,n,InsideLeftAbs,InterpolateVertexAbs),swap(free,used);
    if( orClip&ClipRight ) n=Clip2D(clipRect,free,used,n,InsideRightAbs,InterpolateVertexAbs),swap(free,used);
    // use result
    if (n<3) return; // nothing to draw
    vertices=used;

    if( n>maxN )
    {
      n=maxN;
      Fail("Poly: Too much vertices");
    }


    for( int i=0; i<n; i++ )
    {
      TLVertex *v=&gv[i];
      const Vertex2DAbs &vs=vertices[i];

      v->pos[0]=vs.x, v->pos[1]=vs.y, v->pos[2]=vs.z;
      v->rhw=vs.w;
      v->color=vs.color;
      v->specular=PackedColor(0xff000000);
      v->t0.u=vs.u;
      v->t0.v=vs.v;
    }
  }
  else
  {
    if( n>maxN )
    {
      n=maxN;
      Fail("Poly: Too much vertices");
    }


    for( int i=0; i<n; i++ )
    {
      TLVertex *v=&gv[i];
      const Vertex2DAbs &vs=vertices[i];

      v->pos[0]=vs.x, v->pos[1]=vs.y, v->pos[2]=vs.z;
      v->rhw=vs.w;
      v->color=vs.color;
      v->specular=PackedColor(0xff000000);
      v->t0.u=vs.u;
      v->t0.v=vs.v;
    }
  }


  AddVertices(gv,n); // note: may flush all queues, but it cannot free them
  Queue2DPoly(gv,n);
}

void EngineDDXB::DrawPoly3D(Matrix4Par space, const MipInfo &mip, const Vertex2DFloat *vertices, int nVertices, const Rect2DFloat &clip, int specFlags, int matType)
{
  BeginInstanceTL(space,DrawParameters::_default);
  ChangeClipPlanes();
  SetBias(0);

  // Clip the input polygon
  const int maxN = 32;
  Vertex2DFloat clippedVertices1[maxN]; // temporay buffer to keep clipped result
  Vertex2DFloat clippedVertices2[maxN]; // temporay buffer to keep clipped result
  Vertex2DFloat *free=clippedVertices1;
  Vertex2DFloat *used=clippedVertices2;
  for (int i = 0; i < nVertices; i++) used[i] = vertices[i];
  int n = nVertices;
  n = Clip2D(clip, free, used, n, InsideTopFloat, InterpolateVertexFloat); swap(free,used);
  n = Clip2D(clip, free, used, n, InsideBottomFloat, InterpolateVertexFloat); swap(free,used);
  n = Clip2D(clip, free, used, n, InsideLeftFloat, InterpolateVertexFloat); swap(free,used);
  n = Clip2D(clip, free, used, n, InsideRightFloat, InterpolateVertexFloat); swap(free,used);
  if (n < 3) return; // nothing to draw
  vertices = used;
  if (n > maxN)
  {
    n = maxN;
    Fail("Poly: Too much vertices");
  }
  nVertices = n;

  // Lock the vertex buffer
  SVertexXB *vertex;
  int vOffset = _dynVBuffer.Lock(vertex, nVertices);
  if (vOffset >= 0)
  {

    // Scale and offset uv pair
    UVPair uv1; uv1.u = 1; uv1.v = 1;
    UVPair uv0; uv0.u = 0; uv0.v = 0;

    // Fill out the vertex buffer with data
    const Vector3 maxPos(10, 10, 10);
    const Vector3 minPos(-10, -10, -10);
    const Vector3 invScale = CalcInvScale(maxPos, minPos);
    const Vector3 scalePos = maxPos - minPos;
    for (int i = 0; i < nVertices; i++)
    {
      const Vertex2DFloat &vs = vertices[i];
      Vector3 pos(vs.x, 0.0f, vs.y);
      CopyPos(vertex->pos, pos, maxPos, minPos, invScale);
      CopyNormal(vertex->norm, Vector3(0, 1, 0));
      UVPair uv; uv.u = vs.u; uv.v = vs.v;
      CopyUV(vertex->t0, uv, uv1, uv0, uv1);
      vertex++;
    }
    _dynVBuffer.Unlock();

    // Set pos and texgen scale and offset
    GEngineDD->SetPosScaleAndOffset(scalePos, minPos);
    GEngineDD->SetTexGenScaleAndOffset(uv1, uv0);


    // HW is not in defined state
    _hwInDefinedState = false;

    // Set the vertex buffer
    const ComRef<IDirect3DVertexBuffer8> &dbuf = _dynVBuffer.GetVBuffer();
    D3DSTREAM_INPUT si;
    si.VertexBuffer = dbuf;
    si.Stride = sizeof(SVertexXB);
    si.Offset = vOffset * sizeof(SVertexXB);
    _d3DDevice->SetVertexShaderInputDirect(VAFVSDecl[VD_Position_Normal_Tex0][VSDV_Basic], 1, &si);
    _vBufferLast = dbuf;
    _vBufferSize = _dynVBuffer._vBufferSize;
    _vOffsetLast = vOffset;
    _vsdVersionLast = VSDV_Basic;

    // Prepare triangle
    LightList emptyLightList;
    TLMaterial mat;
    CreateMaterial(mat, vertices[0].color, matType);
    EngineShapeProperties prop;
    PrepareTriangleTL(mip, TexMaterialLODInfo(NULL, 0), specFlags, mat, emptyLightList, 0, prop);

    // Draw
    SetupSectionTL(VD_Position_Normal_Tex0, false, STNone, VSBasic, ML_Sun, FM_None, false, false, false, nVertices - 2);
    SetupNonFixedConstants();
    DoSelectPixelShader();
    SetupPSConstants();
    HRESULT ret =_d3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, nVertices - 2);
    DoAssert(ret == D3D_OK);
  }
}

void EngineDDXB::DrawLine( int beg, int end )
{
  const TLVertex &v0 = _mesh->GetVertex(beg);
  const TLVertex &v1 = _mesh->GetVertex(end);

  float x0 = v0.pos.X();
  float y0 = v0.pos.Y();
  float x1 = v1.pos.X();
  float y1 = v1.pos.Y();

  float z0 = v0.pos.Z();
  float z1 = v1.pos.Z();
  float w0 = v0.rhw;
  float w1 = v1.rhw;

  // use line texture
  Texture *tex = GPreloadedTextures.New(TextureLine);
  const MipInfo &mip = _textBank->UseMipmap(tex,1,1);

  // convert line to poly;
  int specFlags = NoZWrite|IsAlpha|ClampU|ClampV|IsAlphaFog;
  float dx = x1-x0;
  float dy = y1-y0;
  float dSize2 = dx*dx+dy*dy;
  float invDSize = dSize2 >0 ? InvSqrt(dSize2) : 1;

  float dSize = dSize2*invDSize;

  // direction perpendicular dx, dy
  // TODO: use color alpha as width
  // 2D line drawing
  float pdx = +dy*invDSize, pdy = -dx*invDSize;
  float w = 3.0f;
  x0 -= pdx*(w*0.5);
  x1 -= pdx*(w*0.5);
  y0 -= pdy*(w*0.5);
  y1 -= pdy*(w*0.5);
  float x0Side = x0+pdx*w, y0Side = y0+pdy*w;
  float x1Side = x1+pdx*w, y1Side = y1+pdy*w;

  Vertex2DAbs vertices[4];
  float off = 0.0f;
  //float off = 0.5f;
  vertices[0].x = x0-off;
  vertices[0].y = y0-off;
  vertices[0].z = z0;
  vertices[0].w = w0;
  vertices[0].u = 0;
  vertices[0].v = 0.25;
  vertices[0].color = v0.color;

  vertices[1].x = x0Side-off;
  vertices[1].y = y0Side-off;
  vertices[1].z = z0;
  vertices[1].w = w0;
  vertices[1].u = 0;
  vertices[1].v = 1;
  vertices[1].color = v0.color;

  vertices[2].x = x1Side-off;
  vertices[2].y = y1Side-off;
  vertices[2].z = z1;
  vertices[2].w = w1;
  vertices[2].u = dSize;
  vertices[2].v = 1;
  vertices[2].color = v1.color;

  vertices[3].x = x1-off;
  vertices[3].y = y1-off;
  vertices[3].z = z1;
  vertices[3].w = w1;
  vertices[3].u = dSize;
  vertices[3].v = 0.25;
  vertices[3].color = v1.color;

  Rect2DAbs clip(0,0,_w,_h);

  DrawPoly(mip,vertices,4,clip,specFlags);
}

void EngineDDXB::CreateVB()
{
  ReportGRAM("Before CreateVertexBuffer");
  for (int i=0; i<NumDynamicVB2D; i++)
  {
    HRESULT err=_d3DDevice->CreateVertexBuffer
      (
      MeshBufferLength*sizeof(TLVertex),
      NoClipUsageFlag|D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY,
      0/*MYFVF*/,
      D3DPOOL_DEFAULT,
      _queueNo._meshBuffer[i].Init()
      );
    if( err!=D3D_OK || !_queueNo._meshBuffer)
    {
      DDErrorXB("Cannot create vertex buffer.",err);
      return;
    }
    err = _d3DDevice->CreateIndexBuffer
      (
      IndexBufferLength*sizeof(VertexIndex),
      D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC|NoClipUsageFlag,
      D3DFMT_INDEX16,
      D3DPOOL_DEFAULT,
      _queueNo._indexBuffer[i].Init()
      );
    if (err!=D3D_OK || !_queueNo._indexBuffer)
    {
      DDErrorXB("CreateIndexBuffer",err);
      return;
    }
  }
  ReportGRAM("After CreateVertexBuffer");
  _queueNo._vertexBufferUsed = 0;
  _queueNo._indexBufferUsed = 0;
  _queueNo._actualIBIndex = 0;
  _queueNo._actualVBIndex = 0;
} 

void EngineDDXB::DestroyVB()
{
  for (int i=0; i<NumDynamicVB2D; i++)
  {
    _queueNo._meshBuffer[i].Free();
    _queueNo._indexBuffer[i].Free();
  }
}

void EngineDDXB::CreateVBTL()
{
  _dynVBuffer.Init(MaxShapeVertices,_d3DDevice);
} 

void EngineDDXB::DestroyVBTL()
{
  //_queueTL._meshBuffer.Free();
  //_queueTL._indexBuffer.Free();
  _dynVBuffer.Dealloc();
  _statVBuffer.Clear();
  _statIBuffer.Clear();
}

void EngineDDXB::D3DConstruct(ParamEntryPar cfg,ParamEntryPar fcfg)
{
  _d3dFrameOpen=false;

  static StaticStorage<WORD> TriangleQueueStorageNo[MaxTriQueues];
  for (int i=0; i<MaxTriQueues; i++)
  {
    TriQueueXB &triqNo = _queueNo._tri[i];
    triqNo._triangleQueue.SetStorage(TriangleQueueStorageNo[i].Init(TriQueueSize));
  }

  CreateVB();
  CreateVBTL();

  CreateTextBank(cfg,fcfg);
}

void EngineDDXB::D3DDestruct()
{
  _fonts.Clear();
  if( _textBank ) delete _textBank,_textBank=NULL;

#if PIXEL_SHADERS
  DeinitPixelShaders();
#endif
#if VERTEX_SHADERS
  DeinitVertexShaders();
#endif
  DestroyVBTL();
  DestroyVB();
}

void EngineDDXB::ClearStateCaches()
{
  _pixelShaderSel = _currPixelShaderSel = PSUninitialized;
  _pixelShaderSpecularSel = _currPixelShaderSpecularSel = PSSUninitialized;

  _texGenMatrix[0] = D3DMatZero;
  _texGenMatrix[1] = D3DMatZero;
  _texGenMatrix[2] = D3DMatZero;
  _texGenScale.u = -1.0f;
  _texGenScale.v = -1.0f;
  _texGenOffset.u = -1.0f;
  _texGenOffset.v = -1.0f;
  _texGenScaleOrOffsetHasChanged = true;


  _isWorldMatrixChanged = true;

  _renderState.Clear();
  int i;
  for( i=0; i<MaxStages; i++ )
  {
    _textureStageState[i].Clear();
  }

  _hwInDefinedState = false;
  for (int i = 0; i < TEXTURE_STAGES; i++)
  {
    _d3DDevice->SetTexture(i,NULL);
    _lastHandle[i] = NULL;
    _lastTexture[i].Free();
  }

  _lastSpec = 0;
  _prepSpec = 0;
  _lastMat = NULL;
  _lightsHasChanged = true;


}

void EngineDDXB::Restore()
{
  ClearStateCaches();
}

bool EngineDDXB::CanRestore()
{
  return false;
  //return _frontBuffer->IsLost()==D3D_OK;
}

// implementation of MemoryFreeOnDemandHelper
float EngineDDXB::Priority() const
{
  return UseProgramDrawing ? 1.8f : 1.2f;
}

size_t EngineDDXB::MemoryControlledRecent() const
{
  return _vBufferLRU.MemoryControlledRecent();
}

size_t EngineDDXB::MemoryControlled() const
{
  return _vBufferLRU.MemoryControlled();
}

size_t EngineDDXB::FreeOneItem()
{
  VertexBufferD3DXBLRUItem *last = _vBufferLRU.First();
  if (!last) return 0;
  // free vertex buffer that was not used for longest time
  Shape *shape = unconst_cast(last->GetOwner());
  // if buffer is not owned, it should not be listed here
  size_t estSize = last->GetStoredMemoryControlled();
  //size_t estSize = shape->NVertex()*sizeof(SVertexXB);
  // release v-buffer, but shape knows it should create it again if needed
  shape->ReleaseVBuffer();
  // we do not know how much we really released
  // we give some estimation
  return estSize;
}

void EngineDDXB::MemoryControlledFrame()
{
  _vBufferLRU.Frame();
}

#if _ENABLE_CHEATS

static RString FormatByteSizeNoZero(size_t size)
{
  if (size) return FormatByteSize(size);
  return RString();
}

RString EngineDDXB::GetStat(int statId, RString &statVal, RString &statVal2)
{
  switch (statId)
  {
    case 0:
      statVal = FormatByteSizeNoZero(_iBuffersAllocated+_vBuffersAllocated);
      statVal2 = FormatByteSizeNoZero(_vBufferLRU.MemoryControlled());
      return "Buffers";
    case 1:
      statVal = FormatByteSizeNoZero(_iBuffersAllocated+_vBuffersAllocated);
      statVal2 = FormatByteSizeNoZero(_iBuffersAllocated+_vBuffersUsed);
      return "Buf - A/Use";
    case 2:
      statVal = FormatByteSizeNoZero(_iBuffersAllocated+_vBuffersAllocated);
      statVal2 = FormatByteSizeNoZero(_vBuffersUsed-_vBuffersAllocated);
      return "Buf - A/Ovh";
    case 3:
      statVal = FormatByteSizeNoZero(_iBuffersRequested+_vBuffersRequested);
      statVal2 = FormatByteSizeNoZero(_iBuffersAllocated+_vBuffersAllocated-_iBuffersRequested-_vBuffersRequested);
      return "Buf - R/Ovh";
    case 4:
      statVal = FormatByteSizeNoZero(GetPhysicalMemoryRequested());
      statVal2 = FormatByteSizeNoZero(GetPhysicalMemoryAllocated()-GetPhysicalMemoryRequested());
      return "Phy - A/Ovh";
  }
  statVal = RString();
  statVal2 = RString();
  return RString();
}
#endif

void EngineDDXB::Screenshot(RString filename)
{
}

#include "../engineDll.hpp"

Engine *CreateEngineD3DXB( CreateEnginePars &pars )
{
  //DestroyEngine();
  //InitEngine(hInst,hPrev,sw,w,h);

#ifndef _XBOX
  HideCursor=true;
  if (!pars.UseWindow) EnableDesktopCursor(false);
#else
  HideCursor=false;
#endif

  if( pars.UseWindow )
  {
    pars.NoRedrawWindow = false;
    HideCursor = true;
    //ShowCursor(FALSE);
    return new EngineDDXB
      (
      pars.hInst,pars.hPrev,pars.sw,
      pars.winX,pars.winY,pars.winW,pars.winH,true,pars.bpp
      );
  }
  else
  {
    pars.NoRedrawWindow=true;
    return new EngineDDXB
      (
      pars.hInst,pars.hPrev,pars.sw,
      0,0,pars.w,pars.h,false,pars.bpp
      );
  }
}


//template Ref<VertexStaticDataXB>;
#endif
