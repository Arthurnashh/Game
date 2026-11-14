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

// Main light enumeration - vertex shader values
#define VML_None     0
#define VML_Sun      1
#define VML_Sky      2
#define VML_SetColor 3


//! Limit of the fixed space (all constants with smaller indices are being fixed)
//! Constants bigger than this limit can be fixed too, but they must be set before every usage
#define NONFIXED_SPACE_BEGINNING  43
#define NFS_0 43
#define NFS_1 44
#define NFS_2 45

//! First register of the free constant space
#define FREESPACE_START_REGISTER   46
#define FREESPACE_START_REGISTER0  46
#define FREESPACE_START_REGISTER1  47
#define FREESPACE_START_REGISTER2  48
#define FREESPACE_START_REGISTER3  49
#define FREESPACE_START_REGISTER4  50
#define FREESPACE_START_REGISTER5  51
#define FREESPACE_START_REGISTER6  52
#define FREESPACE_START_REGISTER7  53
#define FREESPACE_START_REGISTER8  54
#define FREESPACE_START_REGISTER9  55
#define FREESPACE_START_REGISTER10 56

//! First register of the space designed for lights (note it is being shared with freespace)
//! 214 = 46+168 (see maxFreeConstants in EngineDD9::CreateShaderFragments)
//! 200 = 224 - MAX_LIGHTS * 4
#define LIGHTSPACE_START_REGISTER_VS	214
#define LIGHTSPACE_START_REGISTER_PS	200
#define LIGHTSPACE_LWS_MATRIX_PS			220

//! Maximum number of lights to be used in one batch
//! See MAX_LIGHTS
// prev 6. -> D3DXCompileShader PSTerrainSNX - 568 instruction slots used (54 texture, 514 arithmetic) 
// news:jnqnrj$olr$1@new-server.localdomain
#define MAX_LIGHTS 5

// CALM WATER defines
//! use parallax mapping
#define CALMWATER_PARALLAX
//! don't use "GetCurrRenderTargetAsTexture()"
//#define CALMWATER_SIMPLE_REFRACT
//! don't use shadow variants
//#define CALMWATER_NOSHADOWS
//! show undistort refraction only with blue tint (for debuging)
//#define CALMWATER_REFRACT_DEBUG

// TREE
// faster noise alpha for TreeAdv shader (use _CAN and _NO textures instead of _CA and _NON)
#define NEW_TREE_NOISE

// param arrays sizes
#define CALMWATER_VSPARS_START 172
#define CALMWATER_PARS_SIZE_PS1 12
#define CALMWATER_PARS_SIZE_PS2 16
#define CALMWATER_PARS_SIZE (CALMWATER_PARS_SIZE_PS1 + CALMWATER_PARS_SIZE_PS2)
// sea water param arrays sizes
#define SEAWATER_VSPARS_START 172
#define SEAWATER_VSPARS_SIZE 16
#define SEAWATER_PSPARS_SIZE_1 16

/// size of simul weather vertex shader parameters
#define SIMUL_WEATHER_VS_PARS_SIZE 5

//! Enable/disable blur on the SSSM
//#define SSSMBLUR

//!{ Vertex shader constants
#define VSC_LIST(XXS,XXA) \
  XXS(float4x3,  VSC_ViewMatrix,                                        c,0)     /*World View matrix*/\
  XXS(float4x4,  VSC_ProjMatrix,                                        c,3)     /*Projection matrix*/\
  XXS(float4,    VSC_CameraPosition,                                    c,7)     /*Position of the camera in model space*/\
  XXA(float4x2,  VSC_TexTransform,                                      c,8, 8)  /*Texture coordinates*/\
  XXS(float4,    VSC_AE,                                                c,24)    /*(_lightAmbient * _matAmbient + _matEmmisive) in case one directional light is present, (_matEmmisive) elsewhere. A_E.w == diffuse back coeficient*/\
  XXS(float4,    VSC_GE,                                                c,30)    /*Color of light reflected by ground and Emmisive color. Used by ordinary shaders, that means shader with hemispherical lighting*/\
  XXS(float4,    VSC_InstanceColor,                                     c,25)    /*Color (inc. alpha of given instance)*/\
  XXS(float4,    VSC_UDirectionTransformedDir,                          c,26)    /*Direction UP in model space*/\
  XXS(float4,    VSC_LDirectionTransformedDir,                          c,27)    /*Directional light parameters*/\
  XXS(float4,    VSC_LDirectionD,                                       c,28)    /*Directional light parameters*/\
  XXS(float4,    VSC_LDirectionS,                                       c,29)    /*Directional light parameters*/\
  XXS(float4,    VSC_LDirectionGround,                                  c,42)    /*Directional light parameters*/\
  XXS(float4,    VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart, c,31)    /*Specular power common for all lights & Alpha value & Fog end & 1/(fogEnd-fogStart)*/\
  XXS(float4,    VSC_Free_ExpFog_AFogEnd_RCPAFogEndMinusAFogStart,      c,34)    /*alpha fog - register shared with shadowVolumeLength*/\
  XXS(float,     VSC_MatrixOffset,                                      c,32)    /*Offset of the matrices in the global array*/\
  XXS(float,     VSC_BonesStride,                                       c,33)    /*Stride of bones (aligned number of bones in the shape) in the skinned instancing*/\
  XXS(float4,    VSC_ShadowVolumeLength,  				                      c,34)    /**/\
  XXS(float4x3,  VSC_ShadowmapMatrix,                                   c,35)    /*Matrix to transform from view space to shadowmap space*/\
  XXS(float4,    VSC_InstanceLandShadowIntensity,                       c,38)    /*Intensity of the land shadow of the given instance*/\
  XXS(float4,    VSC_CameraDirectionInvDepth,                           c,39)    /*Direction of the camera in model space and inversion of max linear depth*/\
  XXA(float4,    VSC_TexCoordType,                                      c,40, 2)  /*Type of texcoord identification*/\
  XXS(float4,    VSC_LAND_SHADOWGRID_GRID__1__GRIDd2, c,FREESPACE_START_REGISTER0)      /*Land shadow parameters*, not used with _VBS3_LAND_SHADOW*/\
  XXA(float4,    VSC_LAND_SHADOW,                     c,FREESPACE_START_REGISTER1, 144) /*Land shadow parameters*, not used with _VBS3_LAND_SHADOW*/\
  XXS(float4,    VSC_TerrainAlphaAdd,                                   c,NFS_0) /*Terrain parameters*/\
  XXS(float4,    VSC_TerrainAlphaMul,                                   c,NFS_1) /*Terrain parameters*/\
  XXS(float4,    VSC_TerrainLODPars,                                    c,33)    /*Terrain parameters*/\
  XXA(float4,    VSC_TreeCrown,                                         c,NFS_0, 2)     /*Tree crown parameters*/\
  XXS(float4x3,  VSC_LWSMatrix,                                         c,NFS_0) /*Local World Space matrix*/\
  XXS(float4,    VSC_Period_X_X_SeaLevel,             c,FREESPACE_START_REGISTER0) /*Shore parameters*/\
  XXS(float4,    VSC_WaveHeight,                      c,FREESPACE_START_REGISTER0)    /*Water parameters*/\
  XXS(float4,    VSC_WaveGrid,                        c,FREESPACE_START_REGISTER1)    /*Water parameters - {0.9999f/grid,waterSegSize,1,(waterSegSize-1)*0.5f+0.0001f};*/\
  XXS(float4,    VSC_WaterPeakWhite,                  c,FREESPACE_START_REGISTER2)    /*Water parameters - {mul peak, add peak, mul shore, add shore};*/\
  XXS(float4,    VSC_WaterSeaLevel,                   c,FREESPACE_START_REGISTER3)    /*Water parameters - {seaLevel, deep water mul, deep water add, deep water alpha};*/\
  XXA(float4,    VSC_WaterDepth,                      c,FREESPACE_START_REGISTER4, 9*9) /*Water parameters - Depth*/\
  XXS(int4,      VSC_PointLoopCount,                                    i,0)     /*holds number of point lights, it is being used for static looping*/\
  XXS(int4,      VSC_SpotLoopCount,                                     i,1)     /*holds number of spot lights, it is being used for static looping*/\
  XXS(float4,    VSC_ResolX_ResolY_FovX_FovY,         c,FREESPACE_START_REGISTER4) /*Resolution and FOV parameters (for VSPoint shader)*/\
  XXA(float4,    VSC_SimulWeatherPars,                c,FREESPACE_START_REGISTER0, SIMUL_WEATHER_VS_PARS_SIZE) /*Simul Weather vertex shader parameters*/\
  XXA(float4,    VSC_CalmWaterPars,                   c, CALMWATER_VSPARS_START, CALMWATER_PARS_SIZE ) /*calm water specific params*/\
  XXA(float4,    VSC_SeaWaterPars,                    c, SEAWATER_VSPARS_START, SEAWATER_VSPARS_SIZE ) /*sea water specific params*/\
  XXS(float4,    VSC_TreeAdvPars,                     c, NFS_0 )  /*advanced tree specific params*/\
  XXS(float4,    VSC_GodRaysCenterSize,               c, FREESPACE_START_REGISTER0 )  	/*godrays center*/\
  XXS(float4,    VSC_GodRaysTexOffset,                c, FREESPACE_START_REGISTER1 )  	/*godrays offset of textures*/\
  XXA(float4, 	 VSC_GodRaysCornerPoints, 						c, FREESPACE_START_REGISTER2, 4) 	/*corner points*/\
	XXS(float4, 	 CV_RAIN_REYE, 												c, FREESPACE_START_REGISTER0) 		/*rain*/\
	XXS(float4, 	 CV_RAIN_VERT, 												c, FREESPACE_START_REGISTER1) 		/*rain*/\
	XXS(float4, 	 CV_RAIN_LUM_TEX, 										c, FREESPACE_START_REGISTER2) 		/*rain*/\
	XXS(float4, 	 CV_RAIN_DIR, 												c, FREESPACE_START_REGISTER3) 		/*rain*/\
	XXS(float4, 	 CV_RAIN_LIGHTDIR,										c, FREESPACE_START_REGISTER4) 		/*rain*/\
	XXS(float4, 	 CV_RAIN_RDIR, 												c, FREESPACE_START_REGISTER5) 		/*rain*/\
	XXS(float4, 	 CV_RAIN_PARAMS,											c, FREESPACE_START_REGISTER6) 		/*rain*/\
	XXS(float4, 	 CV_RAIN_PARAMS2,											c, FREESPACE_START_REGISTER7)			/*rain*/\
	XXS(float4, 	 CV_RAIN_RAINMAPPOS,									c, FREESPACE_START_REGISTER8)			/*rain*/\
	XXS(float4, 	 CV_RAIN_RAINMAPSCALE,								c, FREESPACE_START_REGISTER9)			/*rain*/\
	XXS(float4, 	 CV_RAIN_EYEDIR,											c, FREESPACE_START_REGISTER10)		/*rain*/\
	XXS(float4, 	 VSC_RainLayer_Offset,								c, FREESPACE_START_REGISTER0) 		/*rain*/\
	XXA(float3, 	 VSC_RainLayer_Corners,								c, FREESPACE_START_REGISTER1, 4) 	/*rain*/\
	XXS(float4, 	 VSC_EnfShapeBias,										c, FREESPACE_START_REGISTER0)			/*debug shapes*/\
	XXS(float4x3,  VSC_EnfShapeWorldMatrix,             c, FREESPACE_START_REGISTER1) 		/*World matrix*/\
	XXS(float4x3,  VSC_EnfShapeViewMatrix,              c, FREESPACE_START_REGISTER4) 		/*View matrix*/\
	XXS(float4x4,  VSC_EnfShapeProjMatrix,              c, FREESPACE_START_REGISTER7) 		/*Projection matrix*/\
	XXA(float3, 	 VSC_SSAOCorners,											c, FREESPACE_START_REGISTER0, 4) 	/*ssao*/\
	XXS(float4, 	 VSC_SSAOPar1,												c, FREESPACE_START_REGISTER4) 		/*ssao*/\
	XXS(float4, 	 VSC_SSAOPar2,												c, FREESPACE_START_REGISTER5) 		/*ssao*/\
	XXS(float4, 	 VSC_SSAOPar3,												c, FREESPACE_START_REGISTER6) 		/*ssao*/\
	XXS(float4, 	 VSC_SSAOPar4,												c, FREESPACE_START_REGISTER7) 		/*ssao*/\
	
	

#define VSC_LDirectionTransformedDirSky VSC_LDirectionTransformedDir  /*Directional Sky light parameters*/
#define VSC_LDirectionSunSkyColor       VSC_LDirectionD               /*Directional Sky light parameters*/
#define VSC_LDirectionSkyColor          VSC_LDirectionS               /*Directional Sky light parameters*/
#define VSC_LDirectionSetColorColor     VSC_LDirectionTransformedDir  /*Directional setColor light parameters*/
#define VSC_ShadowRampMulAdd            VSC_LDirectionTransformedDir  /*used in postprocess effect - shared location with other constant above*/
//!}

//! Start PS constant register designed for craters
#define PSC_CRATER_START 27
//! Maximum number of craters
#define MAX_CRATERS (41-PSC_CRATER_START)

#define PSC_CRATERTEMP_START 42

/// size of simul weather vertex shader parameters
#define SIMUL_WEATHER_PS_PARS_SIZE 8

//!{ Pixel shader constants
#define PSC_LIST(XXS,XXA) \
  XXS(float4, PSC_AE,                             c,	0)  		/* Color of light reflected by air - formerly known as ambient color - and Emmisive color*/\
  XXS(float3, PSC_GE,                             c,	7)  		/* Color of light reflected by ground and Emmisive color. Used by ordinary shaders, that means shader with hemispherical lighting*/\
  XXS(float4, PSC_DForced,                        c,	9)  		/**/\
  XXS(float4, PSC_Diffuse,                        c,	1)  		/**/\
  XXS(float4, PSC_DiffuseBack,                    c,	2)  		/* Used by trees, trees don't use the bidirectional lighting */\
  XXS(float3, PSC_LDirectionGround,               c,	2)  		/* Used by ordinary shaders, they don't use "through" light calculation */\
  XXS(float4, PSC_Specular,                       c,	3)  		/**/\
  XXS(float3, PSC_GroundReflColor,                c,	4)  		/**/\
  XXS(float3, PSC_SkyReflColor,                   c,	5)  		/**/\
  XXS(float3, PSC_WaveColor,                      c,	7)  		/* Used by water, water doesn't use the hemispherical lighting */\
  XXS(float4, PSC_GlassEnvColor,                  c,	4)  		/**/\
  XXS(float4, PSC_GlassMatSpecular,               c,	5)  		/**/\
  XXS(float4, PSC_T0AvgColor,                     c,	6)  		/**/\
  XXS(float4, PSC_MaxColor,                       c,	8)  		/**/\
  XXS(float4, PSC_Shadow_Factor_ZHalf,            c,	10) 		/**/\
  XXS(float4, PSC_DepthClip,                      c,	11) 		/*W stores the value of the z-clipping plane for depth buffer rendering*/\
  XXS(float4, PSC_FogColor,                       c,	12) 		/*Fog color*/\
  XXS(float4, PSC_DitherCoef,                     c,	13) 		/*Dither coefficient for AlphaToCoverage - dependent on used FSAA level*/\
  XXS(float4, PSC_SBTSize_invSBTSize_X_AlphaMult, c,	14) 		/*Alpha multiplier - used for AlphaToCoverage to make the trees thickers*/\
  XXS(float4, PSC_WLight,                         c,	15) 		/*Main light direction in world coordinates*/\
  XXS(float4, PSC_HDRMultiply,                    c,	16) 		/*x: amount of glow*/\
  XXS(float4, PSC_InvW_InvH_MaxLineDepth_X,       c,	17) 		/*Inverse of backbuffers W and H, max linear depth*/\
  XXS(float4, PSC_X_X_X_AlphaRef,                 c,	18) 		/**/\
  XXA(float,  PSC_Layers,                         c,	19,	6) 	/*Toggle individual layers on/off */\
  XXS(float4, PSC_HTMin_HTMax_AFMax_MFMax,        c,	25) 		/*TI properties - _VBS3_TI*/\
  XXS(float4, PSC_MFact_TBody_X_HSDiffCoef,       c,	26) 		/*TI properties - _VBS3_TI*/\
  XXA(float4, PSC_Crater,                         c,	PSC_CRATER_START, MAX_CRATERS) /*Craters from this to 32*/\
  XXA(bool,   PSC_ShadingDetail,                  b,	0,	4) 	/*Video options Shading detail setting is Low, Normal, High, Very High (or better) */\
  XXA(float4, PSC_NoTexSizeLog2,                  c,	41,	2) 	/* Height maps size for individual layers */\
  XXA(float4, PSCGPU_TOffX_TOffY_X_X,             c,	44,	4) 	/*X360 specific - constant owned by the GPU - holding offest of a tile*/\
  XXA(float4, PSC_PRTConstantDirect,              c,	48,	3) 	/*Constants used by the TreePRT shaders - direct lights*/\
  XXA(float4, PSC_PRTConstantIndirect,            c,	51,	3) 	/*Constants used by the TreePRT shaders - indirect lights*/\
  XXS(float4, PSC_TreeAdjust,                     c,	54) 		/*Various parameters to adjust tree rendering*/\
  XXS(int4,   PSC_PointLoopCount,                 i,	0)    	/*holds number of point lights, it is being used for static looping*/\
  XXS(int4,   PSC_SpotLoopCount,                  i,	1)    	/*holds number of spot lights, it is being used for static looping*/\
  XXA(float4, PSC_SkinEffectPars,                 c,	48, 4 ) /*skin shader specific params*/\
  XXA(float4, PSC_CalmWaterPars1,                 c,	27, CALMWATER_PARS_SIZE_PS1 )  /*calm water specific params block1*/\
  XXA(float4, PSC_CalmWaterPars2,                 c,	48, CALMWATER_PARS_SIZE_PS2 )  /*calm water specific params block2*/\
  XXA(float4, PSC_SeaWaterPars1,                  c,	48, SEAWATER_PSPARS_SIZE_1 )  /*sea water specific params block1*/\
  XXA(float4, PSC_TreeAdvPars,                    c,	27, 5 ) /*tree adv specific*/\
  XXA(float4, PSC_CraterTemp,                     c,	PSC_CRATERTEMP_START, MAX_CRATERS) /*crater temperate*/\
  XXS(float4, PSC_TerrainSatNormDist,             c,	55) 		/*Distances fot blending of satellite normal texture with detail normal texture*/\
  XXS(float4, PSC_DiscretizeAlpha,                c,	56) 		/*If alpha should be discretized*/\
  XXS(float4, PSC_NightEmissiveCoef,              c,	57)\
  XXA(float4, PSC_SimulWeatherPars,               c,	58,SIMUL_WEATHER_PS_PARS_SIZE) /*Simul Weather pixel shader parameters*/\
	XXA(float4, PSC_HeatHazePars, 									c, 	27, 1)\
	
//!{ Special texture ID's
/*!
Several textures at the end of the stages are designed to keep textures across the whole
rendering process. Neither other textures, nor NULL can be put on their places during rendering.
*/
#define TEXID_FIRST 15        //! First stage dedicated to special textures
#define TEXID_SHADOWMAP 15    //! Shadow map texture
#define TEXID_TC 15           //! Temperature Tc table // _VBS3_TI
#define TEXID_TI 14           //! TI texture can share space with regular textures // _VBS3_TI
//!}

//! Value used in Point vertex shader for stars to influence the point size. Inverse value is used for other than star purposes (f.i. for point drawing) where 
//! exact size need to be specified
#define MAX_STAR_INTENSITY 0.0015

// Temperature range to be handled by our system
// _VBS3_TI
#define TempMin 0.0f
#define TempMax 60.0f
#define ContrastMax 30.0f //(TempMax - TempMin)*0.5
#define InvTempDiff 0.016666667 //1.0f / (TempMax - TempMin)

#define TIConversionDimensionW 256
#define TIConversionDimensionH 64

#define EncodeTemperatureToRGB(temperature, R, G, B) \
  float temperature03 = saturate(((temperature) - TempMin) * InvTempDiff) * 3.0f; \
  if (temperature03 < 1) \
  { \
  R = temperature03; \
  G = 0.0f; \
  B = 0.0f; \
  } \
  else if (temperature03 < 2) \
  { \
  R = 1.0f; \
  G = temperature03 - 1.0f; \
  B = 0.0f; \
  } \
  else \
  { \
  R = 1.0f; \
  G = 1.0f; \
  B = temperature03 - 2.0f; \
  }

	
	
#ifndef __cplusplus


//-----------------------------------------------------------------------------
/*!
smoothstep : return in interval 0..1 by smooth hermite interpolation
*/
//-----------------------------------------------------------------------------
float  enf_smoothstep(float x)	{	float x2 = x*x;	return 3*x2 - 2*x*x2; }
float2 enf_smoothstep(float2 x)	{	float2 x2 = x*x;	return 3*x2 - 2*x*x2; }
float3 enf_smoothstep(float3 x)	{	float3 x2 = x*x;	return 3*x2 - 2*x*x2; }
float4 enf_smoothstep(float4 x)	{	float4 x2 = x*x;	return 3*x2 - 2*x*x2; }
	
	
//! Structure to hold one P&S light stuff
struct SLPS
{
  float4 A;
  float4 B;
  float4 C;
  float4 D;
};

#define LPOINT_TransformedPos A
#define LPOINT_Atten          B
#define LPOINT_D              C
#define LPOINT_A              D

#define LSPOT_TransformedPos_CosPhiHalf   A
#define LSPOT_TransformedDir_CosThetaHalf B
#define LSPOT_D_InvCTHMCPH                C
#define LSPOT_A_Atten                     D
	
	

//-----------------------------------------------------------------------------------------------
/*!
return linear distance attenuation of point light and normalized light direction
*/	
//-----------------------------------------------------------------------------------------------
float PointLightDistAtt(float3 lightPos, float3 pixelPos, float invRadius, out float3 normLightDir)
{
	float3 pointToLightVector = lightPos - pixelPos;
	
	float dist = length(pointToLightVector);
	normLightDir = pointToLightVector/dist;
	
	return 	saturate(1 - invRadius * dist);
}
	
//-----------------------------------------------------------------------------------------------
/*!
contribution of light
*/	
//-----------------------------------------------------------------------------------------------
float LightLdotN(float3 skinnedNormal, float3 lightDir)
{
	//using enf_smoothstep makes more contrast in highlights and shadows but less contrast in middle part
	return saturate(dot(skinnedNormal, lightDir));
}
	
//-----------------------------------------------------------------------------------------------
/*!
point lighting
*/	
//-----------------------------------------------------------------------------------------------
float3 PointLighting(in float3 skinnedPosition, in float3 skinnedNormal, const SLPS light)
{
	float3 normLightDir;
	float att = PointLightDistAtt(light.LPOINT_TransformedPos.xyz, skinnedPosition.xyz, light.LPOINT_Atten.x, normLightDir);

	att *= att;
  return att*att*(LightLdotN(skinnedNormal, normLightDir) * light.LPOINT_D.xyz + light.LPOINT_A.xyz);
}


//-----------------------------------------------------------------------------------------------
/*!
complete lighting 
\param att							attenuation
\param lightDir					light direction
\param skinnedNormal		normal of point
\param light						structure describing current light
\param toWorldSpace			matrix to transform from object space to world space (as normal is)
\param useSpec					use specular contribution
\param eyeVec						eye vector for specular in world space
\param specPower				specular power
\param specCoef					specular coefficient
*/	
//-----------------------------------------------------------------------------------------------
float3 PointLightEq(float att, float spotAtt, float3 lightDir, float3 skinnedNormal, const SLPS light, float3x3 toWorldSpace, inout float3 specContrib, uniform bool useSpec, float3 eyeVec, float specPower, float specCoef)
{	
	float3 transLightDir = normalize(mul(lightDir, toWorldSpace));
	float diffContribution = LightLdotN(skinnedNormal, transLightDir);

	float diff = 1;
	if (useSpec)
	{
		float3 specHalfway = normalize(eyeVec + transLightDir);
		float specContribution = 2.0*specCoef*saturate(pow(dot(skinnedNormal, specHalfway), specPower)); //*2, somewhere is written that spacular is halved
	
		//test -> specular has linear attenuation to have specular more and farther visible		
		specContrib = att*spotAtt*specContribution*(light.LSPOT_D_InvCTHMCPH.xyz + light.LSPOT_A_Atten.xyz);
		//diff = saturate(1 - specCoef);
	}

	//distance attenuation is power 4 to be closer to reality
	att *= att;
	return diff*att*att*spotAtt*(diffContribution*light.LSPOT_D_InvCTHMCPH.xyz + light.LSPOT_A_Atten.xyz);
}


//-----------------------------------------------------------------------------------------------
/*!
point lighting with specular
\param skinnedPosition	position of pixel
\param skinnedNormal		normal from pixel
\param light						structure describing current light
\param toWorldSpace			matrix to transform from object space to world space (as normal is)
\param specContrib 			final specular
\param useSpec					use specular contribution
\param eyeVec						eye vector for specular in world space
\param specPower				specular power
\param specCoef					specular coefficient
*/	
//-----------------------------------------------------------------------------------------------
float3 PointLighting(in float3 skinnedPosition, in float3 skinnedNormal, const SLPS light, float3x3 toWorldSpace, out float3 specContrib, uniform bool useSpec, float3 eyeVec, float specPower, float specCoef)
{
	float3 normLightDir;
	float att = PointLightDistAtt(light.LPOINT_TransformedPos.xyz, skinnedPosition.xyz, light.LPOINT_Atten.x, normLightDir);

  return PointLightEq(att, 1, normLightDir, skinnedNormal, light, toWorldSpace, specContrib, useSpec, eyeVec, specPower, specCoef);
}



//-----------------------------------------------------------------------------------------------
/*!
spot lighting
\param skinnedPosition	position of pixel
\param skinnedNormal		normal from pixel
\param light						structure describing current light
*/	
//-----------------------------------------------------------------------------------------------
float3 SpotLighting(in float3 skinnedPosition, in float3 skinnedNormal, const SLPS light)
{
	float3 normLightDir;
	float att = PointLightDistAtt(light.LPOINT_TransformedPos.xyz, skinnedPosition.xyz, light.LSPOT_A_Atten.w, normLightDir);

  float rho = dot(-light.LSPOT_TransformedDir_CosThetaHalf.xyz, normLightDir);
	float spotAtt = saturate(rho*light.LSPOT_TransformedPos_CosPhiHalf.w + light.LSPOT_TransformedDir_CosThetaHalf.w);

	att *= att;
  return att*att*spotAtt*spotAtt*(LightLdotN(skinnedNormal, normLightDir) * light.LSPOT_D_InvCTHMCPH.xyz + light.LSPOT_A_Atten.xyz);
}

//-----------------------------------------------------------------------------------------------
/*!
spot lighting with specular 
\param skinnedPosition	position of pixel
\param skinnedNormal		normal from pixel
\param light						structure describing current light
\param toWorldSpace			matrix to transform from object space to world space (as normal is)
\param specContrib			final specular contribution
\param useSpec					use specular contribution
\param eyeVec						eye vector for specular in world space
\param specPower				specular power
\param specCoef					specular coefficient
\param 
*/	
//-----------------------------------------------------------------------------------------------
float3 SpotLighting(float3 skinnedPosition, float3 skinnedNormal, const SLPS light, float3x3 toWorldSpace, out float3 specContrib, uniform bool useSpec, float3 eyeVec, float specPower, float specCoef)
{
	float3 normLightDir;
	float att = PointLightDistAtt(light.LPOINT_TransformedPos.xyz, skinnedPosition.xyz, light.LSPOT_A_Atten.w, normLightDir);

	//spot light 
  float rho = dot(-light.LSPOT_TransformedDir_CosThetaHalf.xyz, normLightDir);
	float spotAtt = saturate(rho*light.LSPOT_TransformedPos_CosPhiHalf.w + light.LSPOT_TransformedDir_CosThetaHalf.w);

	return PointLightEq(att, spotAtt*spotAtt, normLightDir, skinnedNormal, light, toWorldSpace, specContrib, useSpec, eyeVec, specPower, specCoef);
}


//--------------------------------------------------------------
/*!
Get 2D line equation coefficients k, q of type y = kx + q, usable 
e.g. in shaders when need to blend between distances -> 
dist01 = saturate(dist*k + q)

No release checking is done, so y1 and y0 MUST not be the same

\param y1	value where y = 1
\param y0	value where y = 0
\param K		K coefficient
\param Q		K coefficient
*/
//--------------------------------------------------------------
void GetLineEq(float y1, float y0, out float K, out float Q)
{
	K = 1.0f/(y1 - y0);
	Q = -y0*K;
}


//--------------------------------------------------------------
/*!
return saturated linear equation f(x) for x, where f(y1) = 1 and
f(y0) = 0

\param x	
\param y1	value where y = 1
\param y0	value where y = 0
*/
//--------------------------------------------------------------
float LinearFunc(const float x, const float y1, const float y0)
{
	float K, Q;
	GetLineEq(y1, y0, K, Q);
	
	return saturate(K*x + Q);
}

float4 LinearFunc4(const float4 x, const float y1, const float y0)
{
	float K, Q;
	GetLineEq(y1, y0, K, Q);
	
	return saturate(K*x + Q);
}



//-----------------------------------------------------------------------------
/*!
smooth step
*/
//-----------------------------------------------------------------------------
half SmoothStep(half x)
{
  float x2 = x*x;	
  return 3*x2 - 2*x*x2; 
}


/*!
defines for fast debug output
*/
#define D1(x)	return half4(x, x, x, 1);
#define D2(x)	return half4(x, 0, 1);
#define D3(x)	return half4(x, 1);
#define D4(x)	return x;

#endif	
