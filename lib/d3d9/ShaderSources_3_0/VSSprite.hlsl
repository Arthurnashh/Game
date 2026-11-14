#include "VS.h"

struct SpriteInstancingItem
{
  float4 position;
  float4 color;
  float4 mapping_angle;
};

SpriteInstancingItem spriteInstancingItems[56] : register(REG(FREESPACE_START_REGISTER));

void VSSprite(
  in float4 v_UV_Index            : TEXCOORD,
  out float4 oProjectedPosition   : POSITION,
  out float oOutFog              : COLOR1,
  out float4 oShadowMap           : TEXCOORD5,
  out float4 oOutAmbient          : TEXCOORD_AMBIENT,
  out float4 oOutSpecular         : TEXCOORD_SPECULAR,
  out float4 oOutDiffuse          : TEXCOORD_DIFFUSE_SI,
  out float4 vOutTexCoord01       : TEXCOORD0,
  out float4 vOutModulateColor    : TEXCOORD2,
  out float4 oPosition            : TEXCOORD3)
{
  // Do the vertex transformations
  TransformOutput to;
  float4 modulateColor;
  float particleDiameter;
  {
    float2 uv = D3DCOLORtoFLOAT4(v_UV_Index).xy;
    int index = D3DCOLORtoUBYTE4(v_UV_Index).z;
    to.position = spriteInstancingItems[index].position;
    to.skinnedMatrix0 = float4(1, 0, 0, 0);
    to.skinnedMatrix1 = float4(0, 1, 0, 0);
    to.skinnedMatrix2 = float4(0, 0, 1, 0);
    modulateColor = spriteInstancingItems[index].color;
    float4 mapping_angle = spriteInstancingItems[index].mapping_angle;
    to.skinnedPosition.xyz = to.position.xyz;
    to.skinnedPosition.w = 1;
    to.skinnedNormal = normalize(VSC_CameraPosition.xyz - to.skinnedPosition.xyz);
    float scale = to.position.w;

    // Calculate diameter of the particle (no size limitation)
    particleDiameter = abs(scale);

    // first of all calculate distance from camera, we need it for size limitation
    to.transformedPosition.z = mul(to.skinnedPosition, VSC_ViewMatrix).z;

    // Make the particle Z to be closer by the particle radius to the camera (consider the particle is of a sphere shape - it is
    // not correct for the particle to completely disappear when it's center reaches the some occlusion plane)
    // Note it is necessary to do that prior the size limit, because this modification influences final size (by perspective) and particle
    // size control wouldn't work as expected
    to.transformedPosition.z -= particleDiameter * 0.5f;

    // calculate field of view as 1/sqrt(xScale*yScale)
    float fov = rsqrt(VSC_ProjMatrix._m00*VSC_ProjMatrix._m11);
    // limit screen space size to avoid extreme fillrate load when close
    float maxLocalPos = 0.75*fov*to.transformedPosition.z;

    // Make the limit much more benevolent if the particle is marked as no-limit (scale is negative)
    if (scale < 0) maxLocalPos = maxLocalPos * 1000.0f;

    float2 localPosition = min(particleDiameter, maxLocalPos) * (uv.xy - 0.5);
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
    vOutTexCoord01.zw = float2(0, 0);

    // Instance shadow
    to.instanceColor = VSC_InstanceColor;
    to.instanceShadow = VSC_InstanceLandShadowIntensity.x;

    // Normal orientation was not changed
    to.normalAligned = false;
  }
  oProjectedPosition = to.projectedPosition;

  // Calculate mapping coordinates to screen space depth map (out of projectedPosition) and Z value to pass to PSSprite
  oPosition.xy = (to.projectedPosition.xy / to.projectedPosition.w + 1.0f) * 0.5f;  // Calculate mapping coordinates of the depth buffer
  oPosition.y = 1.0f - oPosition.y;                                                 // Revert Y axis
  oPosition.z = to.transformedPosition.z+particleDiameter;                          // Pass the Z value to PS
  oPosition.w = particleDiameter;                                                   // Pass the particle diameter

  // Initialize ligths
  AccomLights al;
  al.ambient = float4(0, 0, 0, 0);
  al.specular = float4(0, 0, 0, 0);
  al.diffuse = float4(0, 0, 0, 0);
  VertexInitLights(to.instanceColor.a,to.instanceShadow, al);

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
  VDoneLights(al, oOutAmbient, oOutSpecular, oOutDiffuse);
  vOutModulateColor = modulateColor;
  oOutFog = oFog;
}
