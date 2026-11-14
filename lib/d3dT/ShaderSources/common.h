#line 2 "W:/c/Poseidon/lib/d3dT/ShaderSources/common.h"

/*!
  aperture change is typically very small
  We want to encode it so that we are able to represent it with low precision (8b or 10b)

  We need to encode:
  - very small value, like exp(+_deltaT*(log2/getUsedToDark))
  - fps 100: exp(0.01f*(log2/getUsedToDark)) ~= 1.0013872
  - fps 1000: exp(0.001f*(log2/getUsedToDark)) ~= 1.0001386

  - quite large value: 1.05
  - log(1.05)~=0.04879
  - ln(1.0001386)~=0.0001386
*/
#define EncodeApertureChange(x) (log(x)*(2.5e3/255)+128.0/255)
#define DecodeApertureChange(encoded) (exp(((encoded)-128.0/255)*(255/2.5e3)))

//! Limit of the fixed space (all constants with smaller indices are being fixed)
//! Constants bigger than this limit can be fixed too, but they must be set before every usage
#define NONFIXED_SPACE_BEGINNING  42
#define NFS_0 42
#define NFS_1 43
#define NFS_2 44

//! First register of the free constant space
#define FREESPACE_START_REGISTER  45
#define FREESPACE_START_REGISTER0 45
#define FREESPACE_START_REGISTER1 46
#define FREESPACE_START_REGISTER2 47
#define FREESPACE_START_REGISTER3 48
#define FREESPACE_START_REGISTER4 49

//! First register of the space designed for lights (note it is being shared with freespace)
//! 213 = 45+168 (see maxFreeConstants in EngineDD9::CreateShaderFragments)
#define LIGHTSPACE_START_REGISTER 213

//! Maximum number of lights to be used in one batch
//! See MAX_LIGHTS
#define MAX_LIGHTS 6

//!{ Vertex shader constants
#define VSC_LIST(XXS,XXA,XXI) \
  XXS(float4x3,  VSC_ViewMatrix,                                        0)     /*World View matrix*/\
  XXS(float4x4,  VSC_ProjMatrix,                                        3)     /*Projection matrix*/\
  XXS(float4,    VSC_CameraPosition,                                    7)     /*Position of the camera in model space*/\
  XXA(float4x2,  VSC_TexTransform,                                      8, 8)  /*Texture coordinates*/\
  XXS(float4,    VSC_AE,                                                24)    /*(_lightAmbient * _matAmbient + _matEmmisive) in case one directional light is present, (_matEmmisive) elsewhere. A_E.w == diffuse back coeficient*/\
  XXS(float4,    VSC_DForced,                                           25)    /*_lightDiffuse * _matForcedDiffuse*/\
  XXS(float4,    VSC_Tex0MaxColor,                                      26)    /**/\
  XXS(float4,    VSC_LDirectionTransformedDir,                          27)    /*Directional light parameters*/\
  XXS(float4,    VSC_LDirectionD,                                       28)    /*Directional light parameters*/\
  XXS(float4,    VSC_LDirectionS,                                       29)    /*Directional light parameters*/\
  XXS(float4,    VSC_LDirectionGround,                                  30)    /*Directional light parameters*/\
  XXS(float4,    VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart, 31)    /*Specular power common for all lights & Alpha value & Fog end & 1/(fogEnd-fogStart)*/\
  XXS(float4,    VSC_Free_ExpFog_AFogEnd_RCPAFogEndMinusAFogStart,      34)    /*alpha fog - register shared with shadowVolumeLength*/\
  XXS(float,     VSC_MatrixOffset,                                      32)    /*Offset of the matrices in the global array*/\
  XXS(float,     VSC_BonesStride,                                       33)    /*Stride of bones (aligned number of bones in the shape) in the skinned instancing*/\
  XXS(float,     VSC_EnableAlignNormal,                                 33)    /*Flag to determine whether normals should be aligned or not*/\
  XXS(float,     VSC_ShadowVolumeLength,                                34)    /**/\
  XXS(float4x3,  VSC_ShadowmapMatrix,                                   35)    /*Matrix to transform from view space to shadowmap space*/\
  XXS(float4,    VSC_Alpha_X_X_LandShadowIntensity,                     38)    /*Intensity of the land shadow of the given instance*/\
  XXS(float4,    VSC_CameraDirection,                                   39)    /*Direction of the camera in model space*/\
  XXA(float4,    VSC_TexCoordType,                                      40, 2)  /*Type of texcoord identification*/\
  XXS(float4,    VSC_LAND_SHADOWGRID_GRID__1__GRIDd2, FREESPACE_START_REGISTER0)      /*Land shadow parameters*/\
  XXA(float4,    VSC_LAND_SHADOW,                     FREESPACE_START_REGISTER1, 144) /*Land shadow parameters*/\
  XXS(float4,    VSC_TerrainAlphaAdd,                                   NFS_0) /*Terrain parameters*/\
  XXS(float4,    VSC_TerrainAlphaMul,                                   NFS_1) /*Terrain parameters*/\
  XXS(float4,    VSC_TerrainLODPars,                                    33)    /*Terrain parameters*/\
  XXA(float4,    VSC_TreeCrown,                                         NFS_0, 2)     /*Tree crown parameters*/\
  XXS(float4x3,  VSC_LWSMatrix,                                         NFS_0) /*Local World Space matrix*/\
  XXS(float4,    VSC_Period_X_X_SeaLevel,             FREESPACE_START_REGISTER0) /*Shore parameters*/\
  XXS(float4,    VSC_WaveHeight,                      FREESPACE_START_REGISTER0)    /*Water parameters*/\
  XXS(float4,    VSC_WaveGrid,                        FREESPACE_START_REGISTER1)    /*Water parameters - {0.9999f/grid,waterSegSize,1,(waterSegSize-1)*0.5f+0.0001f};*/\
  XXS(float4,    VSC_WaterPeakWhite,                  FREESPACE_START_REGISTER2)    /*Water parameters - {mul peak, add peak, mul shore, add shore};*/\
  XXS(float4,    VSC_WaterSeaLevel,                   FREESPACE_START_REGISTER3)    /*Water parameters - {seaLevel, deep water mul, deep water add, deep water alpha};*/\
  XXA(float4,    VSC_WaterDepth,                      FREESPACE_START_REGISTER4, 9*9) /*Water parameters - Depth*/\

//   XXI(int4,      VSC_PointLoopCount,                                    0)     /*holds number of point lights, it is being used for static looping*/\
//   XXI(int4,      VSC_SpotLoopCount,                                     1)     /*holds number of spot lights, it is being used for static looping*/\

#define VSC_LDirectionTransformedDirSky VSC_LDirectionTransformedDir  /*Directional Sky light parameters*/
#define VSC_LDirectionSunSkyColor       VSC_LDirectionD               /*Directional Sky light parameters*/
#define VSC_LDirectionSkyColor          VSC_LDirectionS               /*Directional Sky light parameters*/
#define VSC_LDirectionSetColorColor     VSC_LDirectionTransformedDir  /*Directional setColor light parameters*/
#define VSC_ShadowRampMulAdd            VSC_LDirectionTransformedDir  /*used in postprocess effect - shared location with other constant above*/
//!}

//! Start PS constant register designed for craters
#define PSC_CRATER_START 18

//! Maximum number of craters
#define MAX_CRATERS (32-PSC_CRATER_START)

//!{ Pixel shader constants
#define PSC_LIST(XXS,XXA) \
  XXS(half4, PSC_ADForcedE,          0)  /**/\
  XXS(half4, PSC_Diffuse,            1)  /**/\
  XXS(half4, PSC_DiffuseBack,        2)  /**/\
  XXS(half4, PSC_Specular,           3)  /**/\
  XXS(half3, PSC_GroundReflColor,    4)  /**/\
  XXS(half3, PSC_SkyReflColor,       5)  /**/\
  XXS(half3, PSC_WaveColor,          7)  /**/\
  XXS(half4, PSC_GlassEnvColor,      4)  /**/\
  XXS(half4, PSC_GlassMatSpecular,   5)  /**/\
  XXS(half4, PSC_T0AvgColor,         6)  /**/\
  XXS(half4, PSC_MaxColor,           8)  /**/\
  XXS(half4, PSC_Shadow_Factor_ZHalf,10) /**/\
  XXS(half4, PSC_DepthClip,          11) /*W stores the value of the z-clipping plane for depth buffer rendering*/\
  XXS(half4, PSC_FogColor,           12) /*Fog color*/\
  XXA(half4, PSC_Crater,             PSC_CRATER_START, MAX_CRATERS) /*Craters from this to 32*/\

//!{ Special texture ID's
/*!
Several textures at the end of the stages are designed to keep textures across the whole
rendering process. Neither other textures, nor NULL can be put on their places during rendering.
*/
#define TEXID_FIRST 15        //! First stage dedicated to special textures
#define TEXID_SHADOWMAP 15    //! Shadow map texture
//!}

//! Conversion from int (or int4 f.i.) to float in interval <0, 1> (corresponds to impicit conversion under DX9 from D3DCOLOR in VB to float in VS)
#define BYTETOFLOAT01(x) (x * 1.0f/255.0f)

//! Conversion from int (or int4 f.i.) to float in interval <0, 1> together with D3DCOLOR swizzling
#define BYTETOFLOAT01_BGRA2RGBA(color) float4(color.z * 1.0f/255.0f, color.y * 1.0f/255.0f, color.x * 1.0f/255.0f, color.a * 1.0f/255.0f)

//! Index of the first boolean value in the vertex shader constant array and number of booleans
#define BOOLSTART 256
#define NBOOLCONST 10

//! Index of the first integer value in the vertex shader constant array and number of integers
#define INTSTART (BOOLSTART+NBOOLCONST)
#define NINTCONST 2
