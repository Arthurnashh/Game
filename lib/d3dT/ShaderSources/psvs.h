#line 2 "W:\c\Poseidon\lib\d3dT\ShaderSources\psvs.h"

//! Register designed for retrieving the diffuse color from VS
#define TEXCOORDAMBIENT TEXCOORD6
#define TEXCOORDSPECULAR TEXCOORD7
#define TEXCOORDDIFFUSE TEXCOORD4

//! Register designed for retrieving the shadow map coordinates from VS
#define TEXCOORDSR TEXCOORD5

/// is fog implemented in pixel shader or post-pixel shader?

struct SDetail
{
  float4 position                   : SV_POSITION;
  half4 ambientColor                : TEXCOORDAMBIENT;
  half4 landShadow                  : TEXCOORDSPECULAR;
  half4 diffuseColor                : TEXCOORDDIFFUSE;
  float4 tShadowBuffer              : TEXCOORDSR;
  half4 dummy0                      : COLOR0;
  half4 tPSShadowReduction_Fog      : COLOR1;
  float4 tDiffuseMap_DetailMap      : TEXCOORD0;
};

struct SDetailSpecularAlpha
{
  float4 position                   : SV_POSITION;
  half4 ambientColor                : TEXCOORDAMBIENT;
  half4 specularColor_landShadow    : TEXCOORDSPECULAR;
  half4 diffuseColor                : TEXCOORDDIFFUSE;
  float4 tShadowBuffer              : TEXCOORDSR;
  half4 dummy0                      : COLOR0;
  half4 tPSShadowReduction_Fog      : COLOR1;
  float4 tDiffuseMap_DetailMap      : TEXCOORD0;
};

struct SNormal
{
  float4 position                 : SV_POSITION;
  half4 ambientColor              : TEXCOORDAMBIENT;
  half4 landShadow                : TEXCOORDSPECULAR;
  half4 diffuseColor              : TEXCOORDDIFFUSE;
  float4 tShadowBuffer            : TEXCOORDSR;
  half4 tColor0                   : COLOR0;
  half4 tPSShadowReduction_Fog    : COLOR1;
  float4 tDiffuseMap              : TEXCOORD0;
};

struct SGrass
{
  float4 position                 : SV_POSITION;
  half4 ambientColor              : TEXCOORDAMBIENT;
  half4 landShadow                : TEXCOORDSPECULAR;
  half4 diffuseColor              : TEXCOORDDIFFUSE;
  float4 tShadowBuffer            : TEXCOORDSR;
  half4 dummy0                    : COLOR0;
  half4 tPSShadowReduction_Fog    : COLOR1;
  float4 tDiffuseMap              : TEXCOORD0;
  // TEXCOORD1 normally used for texture uv
  half3 lightingColor             : TEXCOORD1;
};

struct SSpecularAlpha
{
  float4 position                   : SV_POSITION;
  half4 ambientColor                : TEXCOORDAMBIENT;
  half4 specularColor_landShadow    : TEXCOORDSPECULAR;
  half4 diffuseColor                : TEXCOORDDIFFUSE;
  float4 tShadowBuffer              : TEXCOORDSR;
  half4 dummy0                      : COLOR0;
  half4 tPSShadowReduction_Fog      : COLOR1;
  float4 tDiffuseMap                : TEXCOORD0;
};

struct SSpecularNormalMapGrass
{
  float4 position                   : SV_POSITION;
  half4 ambientColor                : TEXCOORDAMBIENT;
  half4 lightLocal_landShadow       : TEXCOORDSPECULAR;
  half4 dummyDiffuse                : TEXCOORDDIFFUSE;
  float4 tShadowBuffer              : TEXCOORDSR;
  half4 dummy0                      : COLOR0;
  half4 tPSShadowReduction_Fog      : COLOR1;
  float4 tDiffuseMap_NormalMap      : TEXCOORD0;
};

struct SSpecularNormalMapThrough
{
  float4 position                   : SV_POSITION;
  half4 ambientColor                : TEXCOORDAMBIENT;
  half4 lightLocal_landShadow       : TEXCOORDSPECULAR;
  half4 diffuseColor                : TEXCOORDDIFFUSE;
  float4 tShadowBuffer              : TEXCOORDSR;
  half4 dummy0                      : COLOR0;
  half4 tPSShadowReduction_Fog      : COLOR1;
  float4 tDiffuseMap_NormalMap      : TEXCOORD0;
  float4 tDiffuseAtten_AmbientAtten : TEXCOORD1;
};

struct SSpecularNormalMapSpecularThrough
{
  float4 position                   : SV_POSITION;
  half4 ambientColor                : TEXCOORDAMBIENT;
  half4 lightLocal_landShadow       : TEXCOORDSPECULAR;
  half4 diffuseColor                : TEXCOORDDIFFUSE;
  float4 tShadowBuffer              : TEXCOORDSR;
  half4 dummy0                      : COLOR0;
  half4 tPSShadowReduction_Fog      : COLOR1;
  float4 tDiffuseMap_NormalMap      : TEXCOORD0;
  float4 tDiffuseAtten_AmbientAtten : TEXCOORD1;
  float4 dummy1                     : TEXCOORD2;
  half4 tHalfway                    : TEXCOORD3;
};

struct SDetailMacroAS
{
  float4 position                   : SV_POSITION;
  half4 ambientColor                : TEXCOORDAMBIENT;
  half4 landShadow                  : TEXCOORDSPECULAR;
  half4 diffuseColor                : TEXCOORDDIFFUSE;
  float4 tShadowBuffer              : TEXCOORDSR;
  half4 dummy0                      : COLOR0;
  half4 tPSShadowReduction_Fog      : COLOR1;
  float4 tDiffuseMap_DetailMap      : TEXCOORD0;
  float4 tMacroMap_ShadowMap        : TEXCOORD1;
};

struct SDetailSpecularAlphaMacroAS
{
  float4 position                   : SV_POSITION;
  half4 ambientColor                : TEXCOORDAMBIENT;
  half4 specularColor_landShadow    : TEXCOORDSPECULAR;
  half4 diffuseColor                : TEXCOORDDIFFUSE;
  float4 tShadowBuffer              : TEXCOORDSR;
  half4 dummy0                      : COLOR0;
  half4 tPSShadowReduction_Fog      : COLOR1;
  float4 tDiffuseMap_DetailMap      : TEXCOORD0;
  float4 tMacroMap_ShadowMap        : TEXCOORD1;
};

struct SNormalMap
{
  float4 position                   : SV_POSITION;
  half4 ambientColor                : TEXCOORDAMBIENT;
  half4 landShadow                  : TEXCOORDSPECULAR;
  half4 dummyDiffiuse               : TEXCOORDDIFFUSE;
  float4 tShadowBuffer              : TEXCOORDSR;
  half4 tLightLocalAndFog           : COLOR0;
  half4 tPSShadowReduction_Fog      : COLOR1;
  float4 tDiffuseMap_NormalMap      : TEXCOORD0;
  float4 dummyCoord1                : TEXCOORD1;
  float4 dummyCoord2                : TEXCOORD2;
  half3 tHalfway                    : TEXCOORD3;
};

struct SNormalMapDetailMacroASSpecularMap
{
  float4 position                   : SV_POSITION;
  half4 ambientColor                : TEXCOORDAMBIENT;
  half4 landShadow                  : TEXCOORDSPECULAR;
  half4 dummyDiffiuse               : TEXCOORDDIFFUSE;
  float4 tShadowBuffer              : TEXCOORDSR;
  half4 tLightLocalAndFog           : COLOR0;
  half4 tPSShadowReduction_Fog      : COLOR1;
  float4 tDiffuseMap_NormalMap      : TEXCOORD0;
  float4 tDetailMap_MacroMap        : TEXCOORD1;
  float4 tShadowMap_SpecularMap     : TEXCOORD2;
  half3 tHalfway                    : TEXCOORD3;
};

struct SNormalMapDetailMacroASSpecularDIMap
{
  float4 position                   : SV_POSITION;
  half4 ambientColor                : TEXCOORDAMBIENT;
  half4 landShadow                  : TEXCOORDSPECULAR;
  half4 dummyDiffiuse               : TEXCOORDDIFFUSE;
  float4 tShadowBuffer              : TEXCOORDSR;
  half4 tLightLocalAndFog           : COLOR0;
  half4 tPSShadowReduction_Fog      : COLOR1;
  float4 tDiffuseMap_NormalMap      : TEXCOORD0;
  float4 tDetailMap_MacroMap        : TEXCOORD1;
  float4 tShadowMap_SpecularMap     : TEXCOORD2;
  half3 tHalfway                    : TEXCOORD3;
};

struct SNormalMapDetailSpecularMap
{
  float4 position                   : SV_POSITION;
  half4 ambientColor                : TEXCOORDAMBIENT;
  half4 landShadow                  : TEXCOORDSPECULAR;
  half4 dummyDiffiuse               : TEXCOORDDIFFUSE;
  float4 tShadowBuffer              : TEXCOORDSR;
  half4 tLightLocalAndFog           : COLOR0;
  half4 tPSShadowReduction_Fog      : COLOR1;
  float4 tDiffuseMap_NormalMap      : TEXCOORD0;
  float4 tDetailMap_SpecularMap     : TEXCOORD1;
  half3 dummyCoord2                 : TEXCOORD2;
  half3 tHalfway                    : TEXCOORD3;
};

struct SNormalMapDetailSpecularDIMap
{
  float4 position                   : SV_POSITION;
  half4 ambientColor                : TEXCOORDAMBIENT;
  half4 landShadow                  : TEXCOORDSPECULAR;
  half4 dummyDiffiuse               : TEXCOORDDIFFUSE;
  float4 tShadowBuffer              : TEXCOORDSR;
  half4 tLightLocalAndFog           : COLOR0;
  half4 tPSShadowReduction_Fog      : COLOR1;
  float4 tDiffuseMap_NormalMap      : TEXCOORD0;
  float4 tDetailMap_SpecularMap     : TEXCOORD1;
  half3 dummyCoord2                 : TEXCOORD2;
  half3 tHalfway                    : TEXCOORD3;
};

struct SNormalMapMacroAS
{
  float4 position                   : SV_POSITION;
  half4 ambientColor                : TEXCOORDAMBIENT;
  half4 landShadow                  : TEXCOORDSPECULAR;
  half4 dummyDiffiuse               : TEXCOORDDIFFUSE;
  float4 tShadowBuffer              : TEXCOORDSR;
  half4 tLightLocalAndFog           : COLOR0;
  half4 tPSShadowReduction_Fog      : COLOR1;
  float4 tDiffuseMap_NormalMap      : TEXCOORD0;
  float4 tMacroMap                  : TEXCOORD1;
  float4 tShadowMap                 : TEXCOORD2;
  half3 tHalfway                    : TEXCOORD3;
};

struct SNormalMapMacroASSpecularMap
{
  float4 position              : SV_POSITION;
  half4 ambientColor           : TEXCOORDAMBIENT;
  half4 landShadow             : TEXCOORDSPECULAR;
  half4 dummy0                 : TEXCOORDDIFFUSE;
  float4 tShadowBuffer         : TEXCOORDSR;
  half4 tLightLocalAndFog      : COLOR0;
  half4 tPSShadowReduction_Fog : COLOR1;
  float4 tDiffuseMap_NormalMap : TEXCOORD0;
  float4 tMacroMap_ShadowMap   : TEXCOORD1;
  float4 tSpecularMap          : TEXCOORD2;
  half3 tHalfway               : TEXCOORD3;
};

struct SNormalMapMacroASSpecularDIMap
{
  float4 position              : SV_POSITION;
  half4 ambientColor           : TEXCOORDAMBIENT;
  half4 landShadow             : TEXCOORDSPECULAR;
  half4 dummy0                 : TEXCOORDDIFFUSE;
  float4 tShadowBuffer         : TEXCOORDSR;
  half4 tLightLocalAndFog      : COLOR0;
  half4 tPSShadowReduction_Fog : COLOR1;
  float4 tDiffuseMap_NormalMap : TEXCOORD0;
  float4 tMacroMap_ShadowMap   : TEXCOORD1;
  float4 tSpecularMap          : TEXCOORD2;
  half3 tHalfway               : TEXCOORD3;
};

struct SNormalMapSpecularMap
{
  float4 position              : SV_POSITION;
  half4 ambientColor           : TEXCOORDAMBIENT;
  half4 landShadow             : TEXCOORDSPECULAR;
  half4 dummyDiffuse           : TEXCOORDDIFFUSE;
  float4 tShadowBuffer         : TEXCOORDSR;
  half4 tLightLocalAndFog      : COLOR0;
  half4 tPSShadowReduction_Fog : COLOR1;
  float4 tDiffuseMap_NormalMap : TEXCOORD0;
  float4 tSpecularMap          : TEXCOORD1;
  float4 dummyCoord2           : TEXCOORD2;
  half3 tHalfway               : TEXCOORD3;
};

struct SNormalMapSpecularDIMap
{
  float4 position              : SV_POSITION;
  half4 ambientColor           : TEXCOORDAMBIENT;
  half4 landShadow             : TEXCOORDSPECULAR;
  half4 dummy0                 : TEXCOORDDIFFUSE;
  float4 tShadowBuffer         : TEXCOORDSR;
  half4 tLightLocalAndFog      : COLOR0;
  half4 tPSShadowReduction_Fog : COLOR1;
  float4 tDiffuseMap_NormalMap : TEXCOORD0;
  float4 tSpecularMap          : TEXCOORD1;
  float4 dummy1                : TEXCOORD2;
  half3 tHalfway               : TEXCOORD3;
};

struct SSpecularNormalMapDiffuse
{
  float4 position                 : SV_POSITION;
  half4 ambientColor              : TEXCOORDAMBIENT;
  half4 specularColor_landShadow  : TEXCOORDSPECULAR;
  half4 dummy0                    : TEXCOORDDIFFUSE;
  float4 tShadowBuffer            : TEXCOORDSR;
  half4 tLightLocalAndFog         : COLOR0;
  half4 tPSShadowReduction_Fog    : COLOR1;
  float4 tColorMap_NormalMap      : TEXCOORD0;
  float4 tDetailMap               : TEXCOORD1;
  float4 dummyCoord2              : TEXCOORD2;
  float4 dummyCoord3              : TEXCOORD3;
};

struct SSpecularNormalMapDiffuseMacroAS
{
  float4 position                 : SV_POSITION;
  half4 ambientColor              : TEXCOORDAMBIENT;
  half4 specularColor_landShadow  : TEXCOORDSPECULAR;
  half4 dummyDiffuse              : TEXCOORDDIFFUSE;
  float4 tShadowBuffer            : TEXCOORDSR;
  half4 tLightLocalAndFog         : COLOR0;
  half4 tPSShadowReduction_Fog    : COLOR1;
  float4 tColorMap_NormalMap      : TEXCOORD0;
  float4 tDetailMap_MacroMap      : TEXCOORD1;
  float4 tShadowMap               : TEXCOORD2;
};

struct STerrain
{
  float4 position                : SV_POSITION;
  half4 ambientColor             : TEXCOORDAMBIENT; //6
  half4 specularColor            : TEXCOORDSPECULAR; //7
  float4 tShadowBuffer           : TEXCOORDSR; //5
  half4 tLightLocalAndFog        : COLOR0;
  half4 tPSShadowReduction_Fog   : COLOR1;
  float4 tSatAndMask_GrassMap    : TEXCOORD4;
  float4 tColorMap_NormalMap     : TEXCOORD0;
  float4 tDetailMap_SpecularMap  : TEXCOORD1;
  float4 grassAlpha              : TEXCOORD2;
  half4 tHalfway                 : TEXCOORD3;
};

struct SShore
{
  float4 position               : SV_POSITION;
  float4 tNormalMap1_2          : TEXCOORD1;
  float2 tNormalMap3            : TEXCOORD3;
  half4 ambientColor            : TEXCOORDAMBIENT;
  float4 tFresnelView           : TEXCOORD2;
  half3 specularHalfway         : COLOR0;
  half4 tPSShadowReduction_Fog  : COLOR1;
  float4 tHeight                : TEXCOORD0;
  float4 stn2lws0               : TEXCOORD4;
  float4 stn2lws1               : TEXCOORD5;
  float4 stn2lws2               : TEXCOORDSPECULAR;
};

struct SShoreWet
{
  float4 position               : SV_POSITION;
  float4 tNormalMap1_2          : TEXCOORD1;
  float2 tNormalMap3            : TEXCOORD3;
  half4 ambientColor            : TEXCOORDAMBIENT;
  float4 tFresnelView           : TEXCOORD2;
  half3 specularHalfway         : COLOR0;
  half4 tPSShadowReduction_Fog  : COLOR1;
  float4 tHeight                : TEXCOORD0;
};

struct SGlass
{
  float4 position                 : SV_POSITION;
  half4 ambientColor              : TEXCOORDAMBIENT;
  half4 specularColor_landShadow  : TEXCOORDSPECULAR;
  half4 diffuseColor              : TEXCOORDDIFFUSE;
  float4 tShadowBuffer            : TEXCOORDSR;
  half4 dummyColor0               : COLOR0;
  half4 tPSShadowReduction_Fog    : COLOR1;
  float4 tDiffuseMap              : TEXCOORD0;
  float4 lwsPosition              : TEXCOORD1;
  half4 lwsNormal                 : TEXCOORD2;
};

struct SCrater
{
  float2 tDiffuseMap        : TEXCOORD0;
  float4 originalPosition : TEXCOORDSPECULAR;
  float4 tShadowBuffer    : TEXCOORDSR;
  half4 tPSShadowReduction_Fog       : COLOR1;
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
