#line 2 "W:\c\Poseidon\lib\d3dT\ShaderSources\VSTerrain.hlsl"

#include "VS.h"

void VSTerrain(
  in float4 vPosition             : POSITION0,
  in float3 lod                   : POSITION1,
  in uint4 iNormal                : NORMAL,
  in uint4 iS                     : TANGENT0,
  in uint4 iT                     : TANGENT1,
  in int2 iTexCoord0              : TEXCOORD0,
  out float4 oProjectedPosition   : SV_POSITION,
#if PRGDEFRenderingMode == RMCommon
  out float4 oOutAmbient          : TEXCOORD_AMBIENT,
  out float4 oOutSpecular         : TEXCOORD_SPECULAR,
  out float4 oShadowMap           : TEXCOORD5,
  out float4 oLightLocal          : COLOR0,
  out float4 oShadowReduction_Fog : COLOR1,
  out float4 oSatAndMask          : TEXCOORD4,
#elif PRGDEFRenderingMode == RMShadowBuffer
  out float4 oPosition            : TEXCOORD7,
#elif PRGDEFRenderingMode == RMDepthMap
  out float4 oPosition            : TEXCOORD7,
#elif PRGDEFRenderingMode == RMCrater
  out float4 oShadowReduction_Fog : COLOR1,
  out float4 oPosition            : TEXCOORD7,
#endif
  out float4 oTexCoord[4]         : TEXCOORD0)
{
  // Read floats from integers
  float3 vNormal = BYTETOFLOAT01(iNormal.xyz);
  float3 vS = BYTETOFLOAT01(iS.xyz);
  float3 vT = BYTETOFLOAT01(iT.xyz);
  float3 vTexCoord0 = float3(iTexCoord0.xy, 1);

  // Decompress compressed vectors
  DecompressVector(vS);
  DecompressVector(vT);
  DecompressVector(vNormal);

  // grass alpha not needed
  float2 grassAlphaOut;
  TransformOutput to = VTransformLodEx(vPosition, vNormal, lod, grassAlphaOut, false);
  oProjectedPosition = to.projectedPosition;

  // Zero the output texcoords to make sure all are set (most of it probably will be removed by optimizer)
  for (int i = 0; i < 4; i++) oTexCoord[i] = float4(0, 0, 0, 0);

  #if PRGDEFRenderingMode == RMCommon
  {
    // Initialize ligths
    AccomLights al;
    al.ambient = float4(0, 0, 0, 0);
    al.specular = float4(0, 0, 0, 0);
    al.diffuse = float4(0, 0, 0, 0);
    al.shadowReduction = float4(0, 0, 0, 1);
    VertexInitLights(to.alpha_X_X_instanceShadow, al);

    // Terrain
    float3 bedColor;
    {
      // Calculate land shadow
      {
        // Calculate coordinate in grid
        float2 coordinate = to.skinnedPosition.xz * VSC_LAND_SHADOWGRID_GRID__1__GRIDd2.xx + VSC_LAND_SHADOWGRID_GRID__1__GRIDd2.ww;
        float4 weights;
        weights.xy = frac(coordinate);

        // Remove the fractional part from y - this needs to be done for y, as it is multiplied
        coordinate.xy -= weights.xy;

        // Calculate the index to VSC_LAND_SHADOW array
        float2 satXY = max(0, min(coordinate.xy, VSC_LAND_SHADOWGRID_GRID__1__GRIDd2.y - 1.0f));
        float shadowFragmentIndex = satXY.y * VSC_LAND_SHADOWGRID_GRID__1__GRIDd2.y + satXY.x;

        // weights will contain: xFrac, zFrac, 1-xFrac, 1-zFrac
        weights.zw = 1 - weights.xy;

        // (1-xFrac)*(1-zFrac), xFrac*(1-zFrac), (1-xFrac)*zFrac, xFrac*zFrac
        weights = weights.zxzx * weights.wwyy;

        // Bilinear interpolation between the four corners
        al.specular.w = dot(VSC_LAND_SHADOW[shadowFragmentIndex], weights);
        // darken vertices with negative Y
        // such vertices are under the sea
        // we do this partly here, partly in DoneLightsTerrain
        //vSpecular.w = max(min(-skinnedPosition.y*(1.0/10),1),vSpecular.w);
      }

      float shallow = saturate(1+to.skinnedPosition.y*(1.0/10));

      // compute sed bed color change - 1 for deep water, 0 for shallow water
      float deep = saturate(-to.skinnedPosition.y*(1.0/15));
      bedColor = lerp(1,float3(0,0.05,0.15),deep);

      // Light and Halfway calculation
      float3 lightLocal;
      float3 halfwayLocal;
      float3 eyeLocal;
      float3 halfwayReflectLocal;
      CalculateLocalLightAndHalfway(vS, vT, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal);

      // Calculate the ligth local
      // by modifying the light vector we affect lighting results
      oLightLocal.xyz = (lightLocal*shallow + 1) * 0.5;
      oLightLocal.w = 1;
      
      // Set the halfway vector
      // TODO: separate VS when specular is needed
      oTexCoord[3].xyz = halfwayLocal;
      
      // Calculate world texcoord for adressing the sattelite map and the layer mask
      vPosition.w = 1;
      oSatAndMask.x = dot(vPosition, VSC_TexTransform[4]._m00_m10_m20_m30);
      oSatAndMask.y = dot(vPosition, VSC_TexTransform[4]._m01_m11_m21_m31);
      oSatAndMask.w = dot(vPosition, VSC_TexTransform[1]._m00_m10_m20_m30);
      oSatAndMask.z = dot(vPosition, VSC_TexTransform[1]._m01_m11_m21_m31);
    }

    // Apply fog
    float oFog;
    VFogFogAlpha(to, al, oFog);

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
    VDoneLightsTerrain(al, bedColor, oOutAmbient, oOutSpecular);
    oShadowReduction_Fog = al.shadowReduction;
    oShadowReduction_Fog.z = oFog;

    // Texture coordinates transformation
    TexCoordTransform(vTexCoord0, vTexCoord0, 5, oTexCoord);
  }
  #else
  {
    TexCoordTransform(vTexCoord0, vTexCoord0, 1, oTexCoord);
    #if PRGDEFRenderingMode == RMShadowBuffer
    {
      if (SBTechniqueDefault)
      {
        oPosition = to.projectedPosition;
      }
      else
      {
        oPosition = 0;
      }
    }
    #elif PRGDEFRenderingMode == RMDepthMap
    {
      oPosition = to.transformedPosition;
    }
    #elif PRGDEFRenderingMode == RMCrater
    {
      float oFog;
      VFog(to, oFog);
      oShadowReduction_Fog = float4(0, 0, oFog, 1);
      oPosition = to.position;
    }
   #endif
  }
  #endif
}