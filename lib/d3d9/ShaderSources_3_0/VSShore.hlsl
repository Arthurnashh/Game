#include "VS.h"


VertSShore VSShore(
  in float4 vPosition                 : POSITION0,
  in float3 lod                       : POSITION1,
#if _VBS3_MORETERRAINLODS_SHADER
  in float4 lod36                     : POSITION2,
#endif
  in float3 vNormal                   : NORMAL,
  in float3 vS                        : TANGENT0,
  in float3 vT                        : TANGENT1,
  
  out float4 oProjectedPosition       : POSITION)
{
  VertSShore output = (VertSShore) 0;

  // Decompress compressed vectors
  DecompressVector(vS);
  DecompressVector(vT);
  DecompressVector(vNormal);

  // grass alpha not needed
  float2 grassAlphaOut;
#if _VBS3_MORETERRAINLODS_SHADER
  TransformOutput to = VTransformLodEx(vPosition, vNormal, lod, lod36, grassAlphaOut, false);
#else
  TransformOutput to = VTransformLodEx(vPosition, vNormal, lod, grassAlphaOut, false);
#endif
  oProjectedPosition = to.projectedPosition;

  // Initialize ligths
  AccomLights al = (AccomLights) 0;
  VertexInitLights(to.instanceColor.a,to.instanceShadow, al);

  // Shore
  float2 worldpos = vPosition.xz + VSC_CalmWaterPars[0].xz - VSC_CameraPosition.xz;
  ComputeTexCoords(worldpos, output.tNormalMap1_2.xy, output.tNormalMap1_2.zw, output.tNormalMap3.xy, VSC_CalmWaterPars[0].w);

  // Include P&S lights (note no DoneLights is needed here)
  float4 res = float4(VSC_AE.rgb, 0);
  VLPointSpotNAmbient(to, res);

  float3 locpos              = VSC_CameraPosition.xyz - to.skinnedPosition;
  output.VertexPos_SkyLerp   = float4(float3(-locpos.z, locpos.x, locpos.y), VSC_LDirectionD.w);
  
  #ifdef WATER_REAL_REFRACTION
    output.CoastColorProjZ.rgb = VSC_LDirectionD.rgb;
  #else
    output.CoastColorProjZ.rgb = VSC_LDirectionD.rgb*CoastColorFactor;
  #endif
  
  output.CoastColorProjZ.w   = to.projectedPosition.z*VSC_CameraDirectionInvDepth.w;
  
//  output.BasicColor          = res.rgb;
  
  //normal lerp
  float dist = length(locpos);
  const float cameraunit1 = 1.0/50.0;
  const float cameraunit2 = 1.0/30.0;
  float cameraheight      = abs(VSC_CameraPosition.y);
  float cameraheightmul1  = 1.0 + (cameraheight*cameraunit1);
  float cameraheightmul2  = 1.0 + (cameraheight*cameraunit2);
  
  output.tNormalMap3.z = 1.6*saturate(1 - GetDistanceLERP(dist, 300.0*cameraheightmul1, 1000.0*cameraheightmul1));
  output.tNormalMap3.w = 1.2*saturate(1 - GetDistanceLERP(dist, 15.0*cameraheightmul2, 35.0*cameraheightmul2));
 
  // Wave height and current height 
  const float per = 0.05235987755;
  float waveHeightA     = sin(to.skinnedPosition.x * per + VSC_Period_X_X_SeaLevel.x) + sin(to.skinnedPosition.z * per + VSC_Period_X_X_SeaLevel.x);
  float waveHeight01A   = waveHeightA * 0.25f + 0.5f;
  float waveHeightB     = sin(to.skinnedPosition.x * per + VSC_Period_X_X_SeaLevel.x + 2.9f) + sin(to.skinnedPosition.z * per + VSC_Period_X_X_SeaLevel.x + 1.5f);
  float waveHeight01B   = waveHeightB * 0.25f + 0.5f;

  float fog;
  VFog(to, fog);
  
  output.Wave_X_X_Height_Fog = float4(
    waveHeight01A,// - 0.3f, 
    waveHeight01B,// - 0.3f, 
    to.skinnedPosition.y - VSC_Period_X_X_SeaLevel.w,
    fog);
 
  return output;
}