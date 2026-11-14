#line 2 "W:\c\Poseidon\lib\d3dT\ShaderSources\VSSprite.hlsl"

#include "VS.h"

// struct SpriteInstancingItem
// {
//   float4 position;
//   float4 color;
//   float4 mapping_angle;
// };
// SpriteInstancingItem spriteInstancingItems[56] : register(REG(FREESPACE_START_REGISTER));

float4 spriteInstancingItems_New[56*3] : register(REG(FREESPACE_START_REGISTER));

void VSSprite(
  in uint4 i_UV_Index             : TEXCOORD,
  out float4 oProjectedPosition   : SV_POSITION,
  out float4 oOutAmbient          : TEXCOORD_AMBIENT,
  out float4 oOutSpecular         : TEXCOORD_SPECULAR,
  out float4 oOutDiffuse          : TEXCOORD_DIFFUSE_SI,
  out float4 oShadowMap           : TEXCOORD5,
  out float4 oDummyColor0         : COLOR0,
  out float4 oShadowReduction_Fog : COLOR1,
  out float4 vOutTexCoord01       : TEXCOORD0)
{
  // Do the vertex transformations
  TransformOutput to;
  float4 modulateColor;
  {
    float2 uv = BYTETOFLOAT01(i_UV_Index).xy;
    int index = i_UV_Index.z;
    to.position = spriteInstancingItems_New[index * 3 + 0];
    to.skinnedMatrix0 = float4(1, 0, 0, 0);
    to.skinnedMatrix1 = float4(0, 1, 0, 0);
    to.skinnedMatrix2 = float4(0, 0, 1, 0);
    modulateColor = spriteInstancingItems_New[index * 3 + 1];
    float4 mapping_angle = spriteInstancingItems_New[index * 3 + 2];
    to.skinnedPosition.xyz = to.position.xyz;
    to.skinnedPosition.w = 1;
    to.skinnedNormal = normalize(VSC_CameraPosition.xyz - to.skinnedPosition.xyz);
    float scale = to.position.w;

    // first of all calculate distance from camera, we need it for size limitation
    to.transformedPosition.z = mul(to.skinnedPosition, VSC_ViewMatrix).z;
    // calculate field of view as 1/sqrt(xScale*yScale)
    float fov = rsqrt(VSC_ProjMatrix._m00*VSC_ProjMatrix._m11);
    // limit screen space size to avoid extreme fillrate load when close
    float maxLocalPos = 0.5*fov*to.transformedPosition.z;

    // Make the limit much more benevolent if the particle is marked as no-limit (scale is negative)
    if (scale < 0) maxLocalPos = maxLocalPos * 1000.0f;

    float2 localPosition = min(abs(scale),maxLocalPos) * (uv.xy - 0.5);
    // animate (rotate) the particle
    float2 sc; sincos(mapping_angle.w, sc.x, sc.y);
    float2 localRotatedPosition;
    localRotatedPosition.x = dot(float2(sc.y, -sc.x), localPosition);
    localRotatedPosition.y = dot(sc, localPosition);
    // fill the rest of the transformedPosition now
    to.transformedPosition.xy = mul(to.skinnedPosition, VSC_ViewMatrix).xy+localRotatedPosition;
    to.transformedPosition.w = 1;
    // project into the homogenous view space
    to.projectedPosition = mul(VSC_ProjMatrix, to.transformedPosition);
    float2 uvScreen = float2(uv.x, 1 - uv.y);
    vOutTexCoord01.xy = uvScreen * mapping_angle.zz + mapping_angle.xy;

    // Instance shadow
    to.alpha_X_X_instanceShadow = VSC_Alpha_X_X_LandShadowIntensity;
  }
  oProjectedPosition = to.projectedPosition;

  // Initialize ligths
  AccomLights al;
  al.ambient = float4(0, 0, 0, 0);
  al.specular = float4(0, 0, 0, 0);
  al.diffuse = float4(0, 0, 0, 0);
  al.shadowReduction = float4(0, 0, 0, 1);
  VertexInitLights(to.alpha_X_X_instanceShadow, al);

  // Apply fog
  float oFog;
  if (FogModeA)
  {
    if (FogModeB)
    {
      VFogFogAlpha(to, al, oFog);
    }
    else
    {
      VFogAlpha(to, al, oFog);
    }
  }
  else
  {
    if (FogModeB)
    {
      VFog(to, oFog);
    }
    else
    {
      VFogNone(oFog);
    }
  }

  // Initialize shadow variable designed for shadow receiving
  if (ShadowReceiverFlag)
  {
    VShadowReceiver(to, oShadowMap);
  }
  else
  {
    oShadowMap = 0;
  }

  // Include P&S lights
  VLPointSpotN(to, al);

  // Write lights to output
  VDoneLights(al, modulateColor, oOutAmbient, oOutSpecular, oOutDiffuse);
  oShadowReduction_Fog = al.shadowReduction;
  oShadowReduction_Fog.z = oFog;
  
  // Dummy inicialization
  oDummyColor0.xyzw = float4(0, 0, 0, 1);
  vOutTexCoord01.zw = float2(0, 0);
}
