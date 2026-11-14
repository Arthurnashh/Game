//! Register designed for retrieving the diffuse color from VS
#define TEXCOORDAMBIENT TEXCOORD6
#define TEXCOORDSPECULAR TEXCOORD7
#define TEXCOORDDIFFUSE TEXCOORD4

//! Register designed for retrieving the shadow map coordinates from VS
#define TEXCOORDSR TEXCOORD5

struct SDetail
{
  centroid half tPSFog                    : COLOR1;
  float4 tDiffuseMap_DetailMap    : TEXCOORD0;
  centroid float4 x_x_nDotU_nDotL          : TEXCOORD3;
  centroid float4 tShadowBuffer            : TEXCOORDSR;
  centroid half4 ambientColor              : TEXCOORDAMBIENT;
  centroid half4 landShadow                : TEXCOORDSPECULAR;
  float2 vPos                     : VPOS;
};

struct SDetailSpecularAlpha
{
  centroid half tPSFog                   : COLOR1;
  float4 tDiffuseMap_DetailMap   : TEXCOORD0;
  centroid float4 x_x_nDotU_nDotL         : TEXCOORD3;
  centroid float4 tShadowBuffer           : TEXCOORDSR;
  centroid half4 ambientColor             : TEXCOORDAMBIENT;
  centroid half4 specularColor_landShadow : TEXCOORDSPECULAR;
  float2 vPos                    : VPOS;
};

struct SNormal
{
  centroid half tPSFog                   : COLOR1;
  float4 tDiffuseMap             : TEXCOORD0;
  centroid float4 x_x_nDotU_nDotL         : TEXCOORD3;
  centroid float4 tShadowBuffer           : TEXCOORDSR;
  centroid half4 ambientColor             : TEXCOORDAMBIENT;
  centroid half4 landShadow               : TEXCOORDSPECULAR;
  float2 vPos                    : VPOS;
};

struct SGrass
{
  // TEXCOORD1 normally used for texture uv
  centroid half tPSFog                   : COLOR1;
  centroid float4 tDiffuseMap             : TEXCOORD0; // centroid does no harm and improves performance on X360
  centroid half4 lightingColor            : TEXCOORD1;
  centroid float4 x_x_nDotU_nDotL         : TEXCOORD3;
  centroid float4 tShadowBuffer           : TEXCOORDSR;
  centroid half4 ambientColor             : TEXCOORDAMBIENT;
  centroid half4 landShadow               : TEXCOORDSPECULAR;
  float2 vPos                    : VPOS;
};

struct SSpecularAlpha
{
  centroid half tPSFog                   : COLOR1;
  float4 tDiffuseMap             : TEXCOORD0;
  centroid float4 x_x_nDotU_nDotL         : TEXCOORD3;
  centroid float4 tShadowBuffer           : TEXCOORDSR;
  centroid half4 ambientColor             : TEXCOORDAMBIENT;
  centroid half4 specularColor_landShadow : TEXCOORDSPECULAR;
  float2 vPos                    : VPOS;
};

struct SSpecularNormalMapGrass
{
  centroid half tPSFog                  : COLOR1;
  float4 tDiffuseMap_NormalMap  : TEXCOORD0;
  centroid half4 tUpLocal                : TEXCOORD4;
  centroid float4 tShadowBuffer          : TEXCOORDSR;
  centroid half4 ambientColor            : TEXCOORDAMBIENT;
  centroid half4 lightLocal_landShadow   : TEXCOORDSPECULAR;
  float2 vPos                   : VPOS;
};

struct SSpecularNormalMapThrough
{
  centroid half tPSFog                      : COLOR1;
  float4 tDiffuseMap_NormalMap      : TEXCOORD0;
  centroid float4 tDiffuseAtten_AmbientAtten : TEXCOORD1;
  centroid half4 tUpLocal                    : TEXCOORD4;
  centroid float4 tShadowBuffer              : TEXCOORDSR;
  centroid half4 ambientColor                : TEXCOORDAMBIENT;
  centroid half4 lightLocal_landShadow       : TEXCOORDSPECULAR;
  float2 vPos                       : VPOS;
};

struct SSpecularNormalMapSpecularThrough
{
  centroid half tPSFog                      : COLOR1;
  float4 tDiffuseMap_NormalMap      : TEXCOORD0;
  centroid float4 tDiffuseAtten_AmbientAtten : TEXCOORD1;
  centroid half4 tHalfway                    : TEXCOORD3;
  centroid half4 tUpLocal                    : TEXCOORD4;
  centroid float4 tShadowBuffer              : TEXCOORDSR;
  centroid half4 ambientColor                : TEXCOORDAMBIENT;
  centroid half4 lightLocal_landShadow       : TEXCOORDSPECULAR;
  float2 vPos                       : VPOS;
};

struct STree /* TODO: do not mix centroid/center to avoid performance hit on X360 - news:g5fhf5$o7v$1@new-server.localdomain */
{
  half tPSFog                      : COLOR1;
  float4 tDiffuseMap_NormalMap      : TEXCOORD0;
  centroid float4 tShadowMap                 : TEXCOORD1;
  centroid half4 tHalfway                    : TEXCOORD3;
  centroid half4 tUpLocal                    : TEXCOORD4;
  centroid float4 tShadowBuffer              : TEXCOORDSR;
  centroid half4 ambientColor                : TEXCOORDAMBIENT;
  centroid half4 lightLocal_landShadow       : TEXCOORDSPECULAR;
  float2 vPos                       : VPOS;
};

struct STreeAdv // no centroid to avoid performance hit on X360 - news:g5fhf5$o7v$1@new-server.localdomain
{
  half4 col0                        : COLOR0;
  float4 tc0                        : TEXCOORD0;  // performance hit on X360 - news:g5fhf5$o7v$1@new-server.localdomain
  float4 tc1                        : TEXCOORD1;
  centroid float4 tc2                        : TEXCOORD2;
  centroid float4 tc3                        : TEXCOORD3;
  //
  centroid half tPSFog                      : COLOR1;
  //float4 tDiffuseMap_NormalMap      : TEXCOORD0;
  //float4 tShadowMap                 : TEXCOORD1;
  //half4 tHalfway                    : TEXCOORD3;
  centroid half4 tUpLocal                    : TEXCOORD4;
  centroid float4 tShadowBuffer              : TEXCOORDSR;
  centroid half4 ambientColor                : TEXCOORDAMBIENT;
  centroid half4 lightLocal_landShadow       : TEXCOORDSPECULAR;
  float2 vPos                       : VPOS;
};

// structure for data transfer between tree adv shader parts
struct SPFTreeAdv
{
  half4 tex0;
  half4 tex1;
  //
  half3 indirectLight; // hemispheric and bidir light
  half3 vertexLight; // sum of point and spot lights
  //half3 indirectSum; // sumVertLight + indirectLight
  half4 colorFinal; // rgb ...composed material color, a ...final alpha (input.ambientColor.a * tex0.a)
  //half3 colorFinal2;
  half3 directLight;
  half3 specularLight;
  half3 specularIndirect; // specularLightHemi.rgb + specularEnvLight
  half fogCoef; // input.tPSFog.w
  half diffuseShadowCoef;
  half shadow;
};

struct STreePRT
{
  centroid half tPSFog                      : COLOR1;
  float4 tDiffuseMap_prtMap         : TEXCOORD0; // performance hit on X360 - news:g5fhf5$o7v$1@new-server.localdomain
  centroid float4 tMacroMap                  : TEXCOORD1;
  centroid half4 tPRTOffsfet                 : TEXCOORD4;
  centroid float4 tShadowBuffer              : TEXCOORDSR;
  centroid half4 ambientColor                : TEXCOORDAMBIENT;
  centroid half4 lightLocal_landShadow       : TEXCOORDSPECULAR;
  float2 vPos                       : VPOS;
};

struct SDetailMacroAS
{
  centroid half tPSFog                 : COLOR1;
  float4 tDiffuseMap_DetailMap : TEXCOORD0;
  float4 tMacroMap_ShadowMap   : TEXCOORD1;
  centroid float4 x_x_nDotU_nDotL       : TEXCOORD3;
  centroid float4 tShadowBuffer         : TEXCOORDSR;
  centroid half4 ambientColor           : TEXCOORDAMBIENT;
  centroid half4 landShadow             : TEXCOORDSPECULAR;
  float2 vPos                  : VPOS;
};

struct SDetailSpecularAlphaMacroAS
{
  centroid half tPSFog                    : COLOR1;
  float4 tDiffuseMap_DetailMap    : TEXCOORD0;
  float4 tMacroMap_ShadowMap      : TEXCOORD1;
  centroid float4 x_x_nDotU_nDotL          : TEXCOORD3;
  centroid float4 tShadowBuffer            : TEXCOORDSR;
  centroid half4 ambientColor              : TEXCOORDAMBIENT;
  centroid half4 specularColor_landShadow  : TEXCOORDSPECULAR;
  float2 vPos                     : VPOS;
};

struct SNormalMap
{
  centroid half4 tLightLocal             : COLOR0;
  centroid half tPSFog                  : COLOR1;
  float4 tDiffuseMap_NormalMap  : TEXCOORD0;
  centroid half4 tHalfway                : TEXCOORD3;
  centroid half4 tUpLocal                : TEXCOORD4;
  centroid float4 tShadowBuffer          : TEXCOORDSR;
  centroid half4 ambientColor            : TEXCOORDAMBIENT;
  centroid half4 landShadow              : TEXCOORDSPECULAR;
  float2 vPos                   : VPOS;
};

struct SNormalMapDetailMacroASSpecularMap
{
  centroid half4 tLightLocal             : COLOR0;
  centroid half tPSFog                  : COLOR1;
  float4 tDiffuseMap_NormalMap  : TEXCOORD0;
  float4 tDetailMap_MacroMap    : TEXCOORD1;
  float4 tShadowMap_SpecularMap : TEXCOORD2;
  centroid half4 tHalfway                : TEXCOORD3;
  centroid half4 tUpLocal                : TEXCOORD4;
  centroid float4 tShadowBuffer          : TEXCOORDSR;
  centroid half4 ambientColor            : TEXCOORDAMBIENT;
  centroid half4 landShadow              : TEXCOORDSPECULAR;
  float2 vPos                   : VPOS;
};

#define SNormalMapDetailMacroASSpecularDIMap SNormalMapDetailMacroASSpecularMap

struct SNormalMapDetailSpecularMap
{
  centroid half4 tLightLocal             : COLOR0;
  centroid half tPSFog                  : COLOR1;
  float4 tDiffuseMap_NormalMap  : TEXCOORD0;
  float4 tDetailMap_SpecularMap : TEXCOORD1;
  centroid half4 tHalfway                : TEXCOORD3;
  centroid half4 tUpLocal                : TEXCOORD4;
  centroid float4 tShadowBuffer          : TEXCOORDSR;
  centroid half4 ambientColor            : TEXCOORDAMBIENT;
  centroid half4 landShadow              : TEXCOORDSPECULAR;
  float2 vPos                   : VPOS;
};

#define SNormalMapDetailSpecularDIMap SNormalMapDetailSpecularMap

struct SNormalMapMacroAS
{
  centroid half4 tLightLocal            : COLOR0;
  centroid half tPSFog                 : COLOR1;
  float4 tDiffuseMap_NormalMap : TEXCOORD0;
  float4 tMacroMap             : TEXCOORD1;
  float4 tShadowMap            : TEXCOORD2;
  centroid half4 tHalfway               : TEXCOORD3;
  centroid half4 tUpLocal               : TEXCOORD4;
  centroid float4 tShadowBuffer         : TEXCOORDSR;
  centroid half4 ambientColor           : TEXCOORDAMBIENT;
  centroid half4 landShadow             : TEXCOORDSPECULAR;
  float2 vPos                  : VPOS;
};

struct SNormalMapMacroASSpecularMap
{
  centroid float4 tShadowBuffer         : TEXCOORDSR;
  centroid half4 ambientColor           : TEXCOORDAMBIENT;
  centroid half4 landShadow             : TEXCOORDSPECULAR;
  centroid half4 tLightLocal            : COLOR0;
  centroid half tPSFog                 : COLOR1;
  centroid half4 tUpLocal               : TEXCOORD4;
  float4 tDiffuseMap_NormalMap : TEXCOORD0;
  float4 tMacroMap_ShadowMap   : TEXCOORD1;
  float4 tSpecularMap          : TEXCOORD2;
  centroid half4 tHalfway               : TEXCOORD3;
  float2 vPos                  : VPOS;
};

struct SNormalMapSpecularMap
{
  centroid half4 tLightLocal            : COLOR0;
  centroid half tPSFog                 : COLOR1;
  float4 tDiffuseMap_NormalMap : TEXCOORD0;
  float4 tSpecularMap          : TEXCOORD1;
  centroid half4 tHalfway               : TEXCOORD3;
  centroid half4 tUpLocal               : TEXCOORD4;
  centroid float4 tShadowBuffer         : TEXCOORDSR;
  centroid half4 ambientColor           : TEXCOORDAMBIENT;
  centroid half4 landShadow             : TEXCOORDSPECULAR;
  float2 vPos                  : VPOS;
};

#define SNormalMapMacroASSpecularDIMap SNormalMapMacroASSpecularMap
#define SNormalMapSpecularDIMap SNormalMapSpecularMap

struct SSpecularNormalMapDiffuse
{
  centroid half4 tLightLocal              : COLOR0;
  centroid half tPSFog                   : COLOR1;
  float4 tColorMap_NormalMap     : TEXCOORD0;
  float4 tDetailMap              : TEXCOORD1;
  centroid half4 tUpLocal                 : TEXCOORD4;
  centroid float4 tShadowBuffer           : TEXCOORDSR;
  centroid half4 ambientColor             : TEXCOORDAMBIENT;
  centroid half4 specularColor_landShadow : TEXCOORDSPECULAR;
  float2 vPos                    : VPOS;
};

struct SSpecularNormalMapDiffuseMacroAS
{
  centroid half4 tLightLocal              : COLOR0;
  centroid half tPSFog                   : COLOR1;
  float4 tColorMap_NormalMap     : TEXCOORD0;
  float4 tDetailMap_MacroMap     : TEXCOORD1;
  float4 tShadowMap              : TEXCOORD2;
  centroid half4 tUpLocal                 : TEXCOORD4;
  centroid float4 tShadowBuffer           : TEXCOORDSR;
  centroid half4 ambientColor             : TEXCOORDAMBIENT;
  centroid half4 specularColor_landShadow : TEXCOORDSPECULAR;
  float2 vPos                    : VPOS;
};

struct SSprite
{
  centroid half tPSFog                    : COLOR1;
  centroid float4 tShadowBuffer            : TEXCOORDSR;
  centroid half4 ambientColor              : TEXCOORDAMBIENT;
  centroid half4 specularColor_landShadow  : TEXCOORDSPECULAR;
  centroid half4 diffuseColor              : TEXCOORDDIFFUSE;
  float4 tDiffuseMap              : TEXCOORD0;
  centroid half4 modulateColor             : TEXCOORD2;
  centroid float4 tPosition                : TEXCOORD3;
  float2 vPos                     : VPOS;
};

struct STerrain
{
  centroid float4 tShadowBuffer           : TEXCOORDSR; //5
  centroid half4 ambientColor             : TEXCOORDAMBIENT; //6
  centroid half4 specularColor            : TEXCOORDSPECULAR; //7
  centroid half3 tLightLocal              : COLOR0;
  centroid half4 tPSFog                   : COLOR1;
  float4 tSatAndMask_GrassMap    : TEXCOORD4;
  float4 tColorMap_NormalMap     : TEXCOORD0;
  float4 tDetailMap_SpecularMap  : TEXCOORD1;
  centroid float4 grassAlpha              : TEXCOORD2;
  centroid half4 tEyeLocal                : TEXCOORD3;
  float2 vPos                    : VPOS;
};

/*
VertSShore and SShore are for VS and PS, respectively
*/
struct VertSShore
{
  centroid float4 VertexPos_SkyLerp       : TEXCOORD0;      
  centroid float4 Wave_X_X_Height_Fog     : TEXCOORD1;
  float4 tNormalMap1_2           : TEXCOORD2;
  float4 tNormalMap3             : TEXCOORD3;
  centroid float4 CoastColorProjZ         : TEXCOORD4;
};


struct SShore
{
  centroid float4 VertexPos_SkyLerp       : TEXCOORD0;      
  centroid float4 Wave_X_X_Height_Fog     : TEXCOORD1;
  float4 tNormalMap1_2           : TEXCOORD2;
  float4 tNormalMap3             : TEXCOORD3;
  centroid float4 CoastColorProjZ         : TEXCOORD4;
  
  float2 vPos                    : VPOS;    
};



//#define WATER_REAL_REFRACTION

/*
for water and shore, the multiplicator of color by coast
*/
#ifdef WATER_REAL_REFRACTION
  #define CoastColorFactor  0.05
#else
  #define CoastColorFactor  0.25
#endif

struct SWater
{
  centroid float4 VertexNormalFog         : TEXCOORD0;
  centroid float4 VertexPosSkyLerp        : TEXCOORD1;
  float4 tWaveMap                : TEXCOORD2;
  float4 tNormalMap1_2           : TEXCOORD3;
  float4 tNormalMap3_LERP        : TEXCOORD4;
  centroid float4 ColorProjZ              : TEXCOORD5;
  centroid float4 SkinnedPosition : COLOR0;
  
  #ifdef WATER_REAL_REFRACTION
    centroid float3 DeepColor               : TEXCOORD6;
  #endif
 
  //must be
  centroid float4 tShadowBuffer           : TEXCOORD7;
  float2 vPos                    : VPOS;
};



#define SShoreWet SShore

struct SGlass
{
  centroid half tPSFog                   : COLOR1;
  float4 tDiffuseMap             : TEXCOORD0;
  centroid float4 lwsPosition             : TEXCOORD1;
  centroid half4 lwsNormal                : TEXCOORD2;
  centroid float4 x_x_nDotU_nDotL         : TEXCOORD3;
  centroid float4 tShadowBuffer           : TEXCOORDSR;
  centroid half4 ambientColor             : TEXCOORDAMBIENT;
  centroid half4 specularColor_landShadow : TEXCOORDSPECULAR;
  float2 vPos                    : VPOS;
};

struct SCrater
{
  float2 tDiffuseMap            : TEXCOORD0;
  centroid float4 originalPosition       : TEXCOORDSPECULAR;
  centroid float4 tShadowBuffer          : TEXCOORDSR;
  centroid half tPSFog                  : COLOR1;
  float2 vPos                   : VPOS;
};

struct SSuper
{
  centroid half4 stn2lws0                : COLOR0;
  centroid half4 tPSFog                  : COLOR1;
  float4 tDiffuseMap_NormalMap  : TEXCOORD0;
  float4 tDetailMap_MacroMap    : TEXCOORD1;
  float4 tShadowMap_SpecularMap : TEXCOORD2;
  centroid half4 stn2lws1                : TEXCOORD3;
  centroid half4 stn2lws2                : TEXCOORD4;
  centroid float4 tShadowBuffer          : TEXCOORDSR;
  centroid half4 ambientColor            : TEXCOORDAMBIENT;
  centroid half4 landShadow              : TEXCOORDSPECULAR;
  float2 vPos                   : VPOS;
};

#define SSuperExt SSuper
#define SSuperAToC SSuper

struct SSkin
{
  centroid half4 stn2lws0                : COLOR0;
  centroid float4 tPSFog                  : COLOR1;
  float4 tDiffuseMap_NormalMap  : TEXCOORD0;
  float4 tDetailMap_MacroMap    : TEXCOORD1;
  float4 tShadowMap_SpecularMap : TEXCOORD2;
  centroid half4 stn2lws1                : TEXCOORD3;
  centroid half4 stn2lws2                : TEXCOORD4;
  centroid float4 tShadowBuffer          : TEXCOORDSR;
  centroid half4 ambientColor            : TEXCOORDAMBIENT;
  centroid half4 landShadow              : TEXCOORDSPECULAR;
  float2 vPos                   : VPOS;
};

struct SCalmWaterVs2Ps
{
   float4 pos : POSITION0;
   centroid float4 tc0 : TEXCOORD0;
   float4 tc1 : TEXCOORD1;
   float4 tc2 : TEXCOORD2;
   centroid float4 tc3 : TEXCOORD3;
   centroid float4 tc4 : TEXCOORD4;
   centroid float4 tc5 : TEXCOORD5;
   centroid float4 tc6 : TEXCOORD6;
   centroid float4 tc7 : TEXCOORD7;
};

// struct for data transfer between pixel shader fragments
struct SCalmWaterPFT
{
  half shadow; // shadow coef
  half2 hMapA01; // sampler2 .ga
};

// structure for data transfer from PFSkin1() to PFSkinFinColor()
struct SPFSkinOut
{
  
  half3 indirectLight; // hemispheric and bidir light
  //half3 indirectSum; // sumVertLight + indirectLight
  half4 colorFinal; // rgb ...composed material color, a ...final alpha (input.ambientColor.a * tex0.a)
  //half3 colorFinal2;
  half3 directLight;
  half3 specularLight;
  half3 specularIndirect; // specularLightHemi.rgb + specularEnvLight
  half fogCoef; // input.tPSFog
  half diffuseShadowCoef;
};

struct SMulti
{
  //centroid half4 tLightLocal            : COLOR0;
	centroid half4 tMatrix0PosX            : COLOR0;
  centroid half4 tPSFog                 : COLOR1;
  float4 tCO0_CO1              : TEXCOORD0;
  float4 tCO2_CO3              : TEXCOORD1;
  float4 tMSK_DT0              : TEXCOORD2;
  float4 tDT1_DT2              : TEXCOORD3;
  //centroid half4 tUpLocal               : TEXCOORD4;
	centroid half4 tMatrix1PosY           : TEXCOORD4;
  centroid float4 tShadowBuffer         : TEXCOORDSR;
  centroid half4 ambientColor           : TEXCOORDAMBIENT;
  //centroid half4 halfway_landShadow     : TEXCOORDSPECULAR;
	centroid half4 tMatrix2PosZ     : TEXCOORDSPECULAR;
  float2 vPos                  : VPOS;
};

struct VSPP_OUTPUT
{
  float4 Position   : POSITION;
  float2 TexCoord   : TEXCOORD;
};
struct VSPP_OUTPUT_SHADOWS
{
  float4 Position   : POSITION;
  float2 TexCoord   : TEXCOORD0;
  float4 Color      : COLOR0;
};
struct VSPP4T_OUTPUT
{
  float4 Position   : POSITION;
  float2 TexCoord0  : TEXCOORD0;
  float2 TexCoord1  : TEXCOORD1;
  float2 TexCoord2  : TEXCOORD2;
  float2 TexCoord3  : TEXCOORD3;
};

////////////////////////////////////////////////////////////////////////////////////////////////////

// vertex output / ps input of SimulWeather cloud rendering
struct SWCloudsVertexOutput
{
  float4 hPosition			: SV_POSITION;
  float2 texCoordsNoise		: TEXCOORD0;
  float2 layerFade_Fog				: TEXCOORD1;
  float4 texCoords			: TEXCOORD2;
  float3 wPosition			: TEXCOORD3;
  float3 texCoordLightning	: TEXCOORD4;
  float3 lightColour			: TEXCOORD5;
#if FADE_MODE==1
  float3 fade_texc			: TEXCOORD6;
#endif
#if FADE_MODE==0
  float3 loss					: TEXCOORD6;
  float3 inscatter			: TEXCOORD7;
#endif
};

////////////////////////////////////////////////////////////////////////////////////////////////////

//==================================================================================================
// sea wave function; computes 2 waves + 2 derivatives; ret.xy ..wave heigths, ret.zw ..deriv. output; p.xy = <-1;1>
half4 WaveFc1A( half2 p )
{
  return half4( p - p * p * p, 1.0h - 3.0h * p * p );
  //return half4( p - p * p * p * p * p, 1.0h - 5.0h * p * p * p * p );
}

// sea wave function; computes 4 waves; ret.xyzw ..wave heigths; p.xyzw = <-1;1>
half4 WaveFc1B( half4 p )
{
  return half4( p - p * p * p );
}
