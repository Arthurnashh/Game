#include "VS.h"

//! Function to return brightness of the color
float Brightness(float3 color)
{
  return dot(color, float3(0.299, 0.587, 0.114));
}

void VSPoint(
  in float4 vPosition             : POSITION0,
  in float3 vTexCoord0            : TEXCOORD0,
  in float4 vColor                : COLOR0,
  out float4 oProjectedPosition   : POSITION,
  out float oOutFog              : COLOR1,
  out float4 oShadowMap           : TEXCOORD5,
  out float4 oOutAmbient          : TEXCOORD_AMBIENT,
  out float4 oOutSpecular         : TEXCOORD_SPECULAR,
  out float4 oOutDiffuse          : TEXCOORD_DIFFUSE_SI,
  out float2 vOutTexCoord01       : TEXCOORD0)
{

  float brightness = Brightness(VSC_AE.xyz);
  float3 normalizedE = VSC_AE.xyz / brightness;

//   float invSize = 1.0f / length(VSC_AE.xyz);
//   float3 normalizedE = VSC_AE * invSize;
//   float intensityE


  // Get the instance alpha (that will infuence the point size only)
  float instanceAlpha = VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart.y * VSC_InstanceColor.a;

  // Intensity, alias percentage of screen surface (if FOV is 1) the point covers
  //const float intensity = 0.001f;
  float intensity = vColor.a * MAX_STAR_INTENSITY * instanceAlpha * brightness;

  // Field of view constants (half screen size to screen distance)
  const float2 fov = float2(VSC_ResolX_ResolY_FovX_FovY.z, VSC_ResolX_ResolY_FovX_FovY.w);

  // Inverse of screen resolution
  const float2 resol = float2(VSC_ResolX_ResolY_FovX_FovY.x, VSC_ResolX_ResolY_FovX_FovY.y);
  const float2 invResol = 1.0f / resol;

  // Limit of point size for both dimensions X and Y. If one dimension is smaller than this, intensity is going to change instead of size
  const float pixelSizeLimit = 3.0f;

  float screenSize = sqrt(intensity/100.0f);
  float pixelIntensity = 1.0f; // Intensity of the pixels to be drawn - unless some point dimension is smaller than pixelSizeLimit, the intensity remains 1

  float maxScreenSize = 0.006;
  if (screenSize>maxScreenSize)
  {
    float oversize = max(screenSize*screenSize/(maxScreenSize*maxScreenSize),1);
    screenSize = maxScreenSize;
    
    // instead of growing above certain limit, increase intensity withing a range HDR can process
    float maxHDR = 3;
    
    pixelIntensity = min(maxHDR,pixelIntensity*oversize);
    
    intensity /= pixelIntensity;
  }
  
  // Calculate point size and pixel intensity coefficient
  float2 pointSizePix = screenSize * fov * resol; // Calculate point size in pixels according to intensity
  
  {
    // Limit X size and influence the intensity
    if (pointSizePix.x < pixelSizeLimit)
    {
      pixelIntensity *= pointSizePix.x / pixelSizeLimit;
      pointSizePix.x = pixelSizeLimit;
    }

    // Limit Y size and influence the intensity
    if (pointSizePix.y < pixelSizeLimit)
    {
      pixelIntensity *= pointSizePix.y / pixelSizeLimit;
      pointSizePix.y = pixelSizeLimit;
    }
  }

  // Convert point size from pixels to <0,1> interval
  float2 pointSize = pointSizePix * invResol;





  // Do the vertex transformations
  TransformOutput to;
  {
    // Original position
    to.position = vPosition;

    // Skinned matrix
    to.skinnedMatrix0 = float4(1, 0, 0, 0);
    to.skinnedMatrix1 = float4(0, 1, 0, 0);
    to.skinnedMatrix2 = float4(0, 0, 1, 0);

    // Skinned position and normal
    to.skinnedPosition = vPosition;
    to.skinnedNormal = float3(0, 1, 0); // Unused

    // Transformed position
    to.transformedPosition = float4(mul(to.skinnedPosition, VSC_ViewMatrix), 1);

    // Instance shadow
    to.instanceColor = VSC_InstanceColor;
    to.instanceShadow = VSC_InstanceLandShadowIntensity.x;

    // Project the position
    to.projectedPosition = mul(VSC_ProjMatrix, to.transformedPosition);
    to.projectedPosition.xy += (vTexCoord0.xy - 0.5f) * 2.0f * pointSize * to.projectedPosition.w;

    // Normal orientation was not changed
    to.normalAligned = false;
  }
  oProjectedPosition = to.projectedPosition;

  // Initialize ligths
  AccomLights al;
  al.ambient.xyz = normalizedE;
  al.ambient.w = 1.0f;
  al.specular = float4(0, 0, 0, 1);
  al.diffuse = float4(0, 0, 0, 1);

  // Reduce the emmisive light
  al.ambient *= pixelIntensity;

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
  oShadowMap = 0;

  // Write lights to output
  VDoneLights(al, oOutAmbient, oOutSpecular, oOutDiffuse);
  oOutFog = oFog;

  // Texture coordinates
  float2 uvScreen = float2(vTexCoord0.x, 1 - vTexCoord0.y);
  vOutTexCoord01.xy = uvScreen;
}
