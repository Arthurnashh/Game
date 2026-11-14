#line 2 "W:\c\Poseidon\lib\d3dT\ShaderSources\VSShore.hlsl"

#include "VS.h"

void VSShore(
  in float4 vPosition                 : POSITION0,
  in float3 lod                       : POSITION1,
  in uint4 iNormal                    : NORMAL,
  in uint4 iS                         : TANGENT0,
  in uint4 iT                         : TANGENT1,
  out float4 oProjectedPosition       : SV_POSITION,
  out float4 vOutNormalMap1_2         : TEXCOORD1,
  out float4 vOutNormalMap3           : TEXCOORD3,
  out float4 vAmbientOut              : TEXCOORD_AMBIENT,
  out float4 vOutFresnelView          : TEXCOORD2,
  out float3 vOutSpecularHalfwayLocal : COLOR0,
  out float4 oShadowReduction_Fog     : COLOR1,
  out float4 oWaveHeight_X_X_Height   : TEXCOORD0,
  out float4 vOutSTN2LWS0             : TEXCOORD_DIFFUSE_SI,
  out float4 vOutSTN2LWS1             : TEXCOORD5,
  out float4 vOutSTN2LWS2             : TEXCOORD_SPECULAR)
{
  // Read floats from integers
  float3 vNormal = BYTETOFLOAT01(iNormal.xyz);
  float3 vS = BYTETOFLOAT01(iS.xyz);
  float3 vT = BYTETOFLOAT01(iT.xyz);

  // Decompress compressed vectors
  DecompressVector(vS);
  DecompressVector(vT);
  DecompressVector(vNormal);

  // grass alpha not needed
  float2 grassAlphaOut;
  TransformOutput to = VTransformLodEx(vPosition, vNormal, lod, grassAlphaOut, false);
  oProjectedPosition = to.projectedPosition;

  // Initialize ligths
  AccomLights al;
  al.ambient = float4(0, 0, 0, 0);
  al.specular = float4(0, 0, 0, 0);
  al.diffuse = float4(0, 0, 0, 0);
  al.shadowReduction = float4(0, 0, 0, 1);
  VertexInitLights(to.alpha_X_X_instanceShadow, al);

  // Shore
  {
    // Calculate 3 different texture coordinates
    {
      vOutNormalMap1_2.x = dot(float3(to.skinnedPosition.xz, 1), VSC_TexTransform[1]._m00_m10_m30);
      vOutNormalMap1_2.y = dot(float3(to.skinnedPosition.xz, 1), VSC_TexTransform[1]._m01_m11_m31);

      vOutNormalMap1_2.z = dot(float3(to.skinnedPosition.xz, 1), VSC_TexTransform[2]._m00_m10_m30) * 2.0f;
      vOutNormalMap1_2.w = dot(float3(to.skinnedPosition.xz, 1), VSC_TexTransform[2]._m01_m11_m31) * 2.0f;

      vOutNormalMap3.x = dot(float3(to.skinnedPosition.xz, 1), VSC_TexTransform[3]._m00_m10_m30) * 4.0f;
      vOutNormalMap3.y = dot(float3(to.skinnedPosition.xz, 1), VSC_TexTransform[3]._m01_m11_m31) * 4.0f;
      vOutNormalMap3.zw = float2(0,1);
    }

    // Remember the camera direction
    float3 camDirectionNN = VSC_CameraPosition.xyz - to.skinnedPosition;
    float3 camDirection = normalize(camDirectionNN);
    
    // Calculate local vectors
    float3 eyeLocal;
    float3 eyeLocalNN;
    float3 halfwayLocal;
    {
      // Calculate STN conversion matrix
      float3 modelS;
      float3 modelT;
      float3 modelN;
      {
        const float baseInclination = 0.0f;
      
        // Calculate STN vectors
        float3 skinnedN = ModifyWaterNormal(/*to.skinnedNormal*/float3(0, 1, 0), camDirection, baseInclination);
        float3 skinnedS;
        float3 skinnedT;
        {
          // Get S and T vectors based on normal  
          skinnedS = normalize(float3(1,0,0)-skinnedN*skinnedN.x);
          skinnedT = skinnedS.yzx*skinnedN.zxy - skinnedN.yzx*skinnedS.zxy;  // crossproduct
        }

        // Convert STN vectors to matrix
        modelS = -skinnedS;
        modelT = -skinnedT;
        modelN = to.skinnedNormal = skinnedN;
      }

      // Calculate the local eye vector
      eyeLocalNN.x = dot(modelS, camDirectionNN);
      eyeLocalNN.y = dot(modelT, camDirectionNN);
      eyeLocalNN.z = dot(modelN, camDirectionNN);
      eyeLocal = normalize(eyeLocalNN);

      // Calculate the local ligth
      float3 lightLocal;
      lightLocal.x = dot(modelS, -(float3)VSC_LDirectionTransformedDir);
      lightLocal.y = dot(modelT, -(float3)VSC_LDirectionTransformedDir);
      lightLocal.z = dot(modelN, -(float3)VSC_LDirectionTransformedDir);

      // Calculate the local halfway vector
      halfwayLocal = normalize(eyeLocal + lightLocal);

      // LWS
      {
        // Transform position to LWS
        float4 lwsPosition = float4(mul(to.skinnedPosition, VSC_LWSMatrix), VSC_LDirectionD.w);
        vOutSTN2LWS0.w = lwsPosition.x;
        vOutSTN2LWS1.w = lwsPosition.y;
        vOutSTN2LWS2.w = lwsPosition.z;
        vOutFresnelView.w = lwsPosition.w;
        
        // We want to get STN2LWS matrix wich is 9 dots of the second and first matrix
        vOutSTN2LWS0.x = dot(modelS, VSC_LWSMatrix._m00_m10_m20);
        vOutSTN2LWS1.x = dot(modelS, VSC_LWSMatrix._m01_m11_m21);
        vOutSTN2LWS2.x = dot(modelS, VSC_LWSMatrix._m02_m12_m22);
        vOutSTN2LWS0.y = dot(modelT, VSC_LWSMatrix._m00_m10_m20);
        vOutSTN2LWS1.y = dot(modelT, VSC_LWSMatrix._m01_m11_m21);
        vOutSTN2LWS2.y = dot(modelT, VSC_LWSMatrix._m02_m12_m22);
        vOutSTN2LWS0.z = dot(modelN, VSC_LWSMatrix._m00_m10_m20);
        vOutSTN2LWS1.z = dot(modelN, VSC_LWSMatrix._m01_m11_m21);
        vOutSTN2LWS2.z = dot(modelN, VSC_LWSMatrix._m02_m12_m22);
      }
    }

    // Write out fresnel view (that means vector from point to camera in local space (will be normalized in PS))
    vOutFresnelView.xyz = eyeLocalNN;

    // Write out the halfway vector
    vOutSpecularHalfwayLocal = (halfwayLocal + 1.0f) * 0.5f;
    
    // Write out ambient color (the same way like in water case - consider water is shallow)
    {
      // Calculate alpha factor based on viewing angle
      float viewA = GetWaterViewA(to.skinnedNormal, camDirection);
      float alpha = lerp(1.0f, VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart.y, viewA);
      //float alpha = 1;

      vAmbientOut = float4(VSC_AE.rgb + VSC_LDirectionD.rgb * saturate(dot(to.skinnedNormal, -VSC_LDirectionTransformedDir.xyz)), alpha);
    }

    // Wave height and current height 
    const float per = 0.05235987755;
    float waveHeightA = sin(to.skinnedPosition.x * per + VSC_Period_X_X_SeaLevel.x) + sin(to.skinnedPosition.z * per + VSC_Period_X_X_SeaLevel.x);
    float waveHeight01A = waveHeightA * 0.25f + 0.5f;
    float waveHeightB = sin(to.skinnedPosition.x * per + VSC_Period_X_X_SeaLevel.x + 2.9f) + sin(to.skinnedPosition.z * per + VSC_Period_X_X_SeaLevel.x + 1.5f);
    float waveHeight01B = waveHeightB * 0.25f + 0.5f;
    oWaveHeight_X_X_Height = float4(waveHeight01A - 0.3f, waveHeight01B - 0.3f, 0, to.skinnedPosition.y - VSC_Period_X_X_SeaLevel.w);
  }

  // Apply fog
  float oFog;
  VFog(to, oFog);

  // Include P&S lights (note no DoneLights is needed here)
  VLPointSpotNAmbient(to, vAmbientOut, al.shadowReduction);
  oShadowReduction_Fog = al.shadowReduction;
  oShadowReduction_Fog.z = oFog;
}