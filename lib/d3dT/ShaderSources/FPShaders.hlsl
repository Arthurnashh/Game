#line 2 "W:\c\Poseidon\lib\d3dT\ShaderSources\FPShaders.hlsl"

#include "psvs.h"
#include "common.h"


//! Constant definition
#define VCS_HLSLDEFS(type,name,regnum) type name : register(c##regnum);
#define VCS_HLSLDEFA(type,name,regnum,dim) type name[dim] : register(c##regnum);
#define VCS_HLSLDEFI(type,name,regnum) type name : register(i##regnum);
VSC_LIST(VCS_HLSLDEFS,VCS_HLSLDEFA,VCS_HLSLDEFI)

// 
// //! Macro to create cX register format
// #define REG(regnum) c##regnum
// 

//! Register designed for passing the diffuse color and shadow intensity in alpha to PS
#define TEXCOORD_AMBIENT TEXCOORD6
#define TEXCOORD_SPECULAR TEXCOORD7
#define TEXCOORD_DIFFUSE_SI TEXCOORD4

// ===================================================================================
// NonTL drawing
// ===================================================================================

struct VS_OUTPUT_NONTL
{
  float4 Position             : SV_POSITION;
  float4 Ambient              : TEXCOORD_AMBIENT;
  float4 Specular             : TEXCOORD_SPECULAR;
  float4 Diffuse              : TEXCOORD_DIFFUSE_SI;
  float4 oShadowMap           : TEXCOORD5;
  //float4 tVector3AndFog       : COLOR0;
  float4 oColor0              : COLOR0;
  float4 oShadowReduction_Fog : COLOR1;
  float4 TexCoord01           : TEXCOORD0;
};

VS_OUTPUT_NONTL VSNonTL(
  in float4 vPosition   : POSITION,
  in uint4 vAmbient      : COLOR0,
  in uint4 vSpecular     : COLOR1,
  in float2 vTexCoord0  : TEXCOORD0,
  in float2 vTexCoord1  : TEXCOORD1
)
{
	VS_OUTPUT_NONTL output;
	output.Position.xyz = vPosition.xyz;
	output.Position.w = 1.0f;
	output.Position = output.Position / vPosition.w;
	
	output.Ambient = BYTETOFLOAT01_BGRA2RGBA(vAmbient) * VSC_Tex0MaxColor;
	output.Specular = BYTETOFLOAT01_BGRA2RGBA(vSpecular);
	
	output.Diffuse = float4(0, 0, 0, 0);
	//output.tVector3AndFog = float4(0, 0, 0, 1);
	output.oColor0 = float4(0, 0, 0, 1);
  output.oShadowReduction_Fog = float4(0, 0, 1, 1);
	output.TexCoord01.xy = vTexCoord0.xy;
	output.TexCoord01.zw = vTexCoord1.xy;
	output.oShadowMap = 0;
  return output;
}


// ===================================================================================
// Post process
// ===================================================================================

VSPP_OUTPUT_SHADOWS VSPostProcessStencilShadows(
  in float3 vPosition : POSITION,
  in float2 vTexCoord : TEXCOORD0,
  in float4 vColor    : COLOR0
)
{
	VSPP_OUTPUT_SHADOWS output;

  float z = vPosition.z*VSC_ShadowRampMulAdd.x+VSC_ShadowRampMulAdd.y;

  // Project the position
  float4 pointZ = float4(0,0,z,1);
  float4 pointProj = mul(VSC_ProjMatrix, pointZ);

  // combine the result
	output.Position.xy = vPosition.xy*pointProj.w;
	output.Position.zw = pointProj.zw;
	
	output.TexCoord = vTexCoord;
	output.Color = vColor;
  return output;
}


// register space shader with view, projection and texgen matrices
/*** it should do no harm - postprocess effects restore everything */

float4 PPEdgeU[4] : register(c0);
float4 PPEdgeV[4] : register(c4);
float4 PPEdgePosOffset : register(c8);

VSPP4T_OUTPUT VSPostProcessCustomEdge4T(
  in float3 vPosition  : POSITION,
  in float  vEdgeIndex : BLENDINDICES
)
{
	VSPP4T_OUTPUT output;
	output.Position.xyzw = vPosition.xyzz+PPEdgePosOffset;
	output.TexCoord0.x = PPEdgeU[vEdgeIndex].x;
	output.TexCoord0.y = PPEdgeV[vEdgeIndex].x;
	output.TexCoord1.x = PPEdgeU[vEdgeIndex].y;
	output.TexCoord1.y = PPEdgeV[vEdgeIndex].y;
	output.TexCoord2.x = PPEdgeU[vEdgeIndex].z;
	output.TexCoord2.y = PPEdgeV[vEdgeIndex].z;
	output.TexCoord3.x = PPEdgeU[vEdgeIndex].w;
	output.TexCoord3.y = PPEdgeV[vEdgeIndex].w;
  return output;
}
