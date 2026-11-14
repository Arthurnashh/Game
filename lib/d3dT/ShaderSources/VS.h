#line 2 "W:\c\Poseidon\lib\d3dT\ShaderSources\VS.h"

#include "common.h"

//! Constant definition
#define VCS_HLSLDEFS(type,name,regnum)      type name       : register(c##regnum);
#define VCS_HLSLDEFA(type,name,regnum,dim)  type name[dim]  : register(c##regnum);
#define VCS_HLSLDEFI(type,name,regnum)      type name       : register(i##regnum);
VSC_LIST(VCS_HLSLDEFS,VCS_HLSLDEFA,VCS_HLSLDEFI)

// See INTSTART
float VSC_PointLoopCount  : register(c266);
float VSC_SpotLoopCount   : register(c267);

//! Macro to create cX register format
#define REG(regnum) c##regnum

//!{ Skinning and instancing arrays
float4x3 view_matrix_56[56] : register(REG(FREESPACE_START_REGISTER));
// struct InstancingItem
// {
//   float4x3 view_matrix;
//   float4 alpha_X_X_shadow;
// };
// InstancingItem instancingItems_42[42] : register(REG(FREESPACE_START_REGISTER));
float4 instancingItems_42_New[42*4] : register(REG(FREESPACE_START_REGISTER));
//!}

//!{ P&S Lights arrays

//! Structure to hold one P&S light stuff
// struct SLPS
// {
//   float4 A;
//   float4 B;
//   float4 C;
//   float4 D;
// };
// 
// #define LPOINT_TransformedPos A
// #define LPOINT_Atten          B
// #define LPOINT_D              C
// #define LPOINT_A              D
// 
// #define LSPOT_TransformedPos_CosPhiHalf   A
// #define LSPOT_TransformedDir_CosThetaHalf B
// #define LSPOT_D_InvCTHMCPH                C
// #define LSPOT_A_Atten                     D
// 
// //! P&S lights array
// SLPS LPSData[MAX_LIGHTS] : register(REG(LIGHTSPACE_START_REGISTER));
#define LPOINT_TransformedPos 0
#define LPOINT_Atten          1
#define LPOINT_D              2
#define LPOINT_A              3
#define LSPOT_TransformedPos_CosPhiHalf   0
#define LSPOT_TransformedDir_CosThetaHalf 1
#define LSPOT_D_InvCTHMCPH                2
#define LSPOT_A_Atten                     3
float4 LPSData_New[MAX_LIGHTS * 4] : register(REG(LIGHTSPACE_START_REGISTER));

//!}

//! Register designed for passing the diffuse color and shadow intensity in alpha to PS
#define TEXCOORD_AMBIENT TEXCOORD6
#define TEXCOORD_SPECULAR TEXCOORD7
#define TEXCOORD_DIFFUSE_SI TEXCOORD4

// An alternative to the intrinsic function D3DCOLORtoUBYTE4, but result values are from range <0, 1>
float4 D3DCOLORtoFLOAT4(float4 value)
{
  return float4(value.z, value.y, value.x, value.w);
}

float4 Multiply4x3OneVector(float4 a, float4 b0, float4 b1, float4 b2)
{
  return a.x * b0 + a.y * b1 + a.z * b2 + a.w * float4(0, 0, 0, 1);
}

void DecompressVector(inout float3 v)
{
  v = v * 2.0f - 1.0f;
}

void CalculateLocalLightAndHalfway(
                                   float3 S, float3 T, float3 skinnedPosition, float3 skinnedNormal, float3 sm0, float3 sm1, float3 sm2,
                                   out float3 lightLocal, out float3 halfwayLocal, out float3 eyeLocal, out float3 halfwayReflectLocal
                                   )
{
  // Prepare the STN space matrix
  float3 modelS;
  modelS.x = dot(-S, sm0);
  modelS.y = dot(-S, sm1);
  modelS.z = dot(-S, sm2);
  modelS = normalize(modelS);
  float3 modelT;
  modelT.x = dot(-T, sm0);
  modelT.y = dot(-T, sm1);
  modelT.z = dot(-T, sm2);
  modelT = normalize(modelT);
  float3 modelN = skinnedNormal;

  // Calculate the local ligth
  lightLocal.x = dot(modelS, -(float3)VSC_LDirectionTransformedDir);
  lightLocal.y = dot(modelT, -(float3)VSC_LDirectionTransformedDir);
  lightLocal.z = dot(modelN, -(float3)VSC_LDirectionTransformedDir);

  // Calculate the local eye vector  
  float3 camDirection = normalize(VSC_CameraPosition.xyz - skinnedPosition);
  eyeLocal.x = dot(modelS, camDirection);
  eyeLocal.y = dot(modelT, camDirection);
  eyeLocal.z = dot(modelN, camDirection);

  // Calculate the local halfway vector  
  halfwayLocal = normalize(eyeLocal + lightLocal);

  // Calculate the local halfway reflect vector
  float3 up = {0, 1, 0};
  float3 halfwayReflect = normalize(camDirection + up);
  halfwayReflectLocal.x = dot(modelS, halfwayReflect);
  halfwayReflectLocal.y = dot(modelT, halfwayReflect);
  halfwayReflectLocal.z = dot(modelN, halfwayReflect);
}

float Haze(float dist)
{
  // ln(1/512)/maxDist  ... maxDist = 10000
  //float hazeCoef = -0.00062383;
  float haze = exp(dist*VSC_Free_ExpFog_AFogEnd_RCPAFogEndMinusAFogStart.y);
  return haze;
}

void TexCoordTransform(in float3 vTexCoord0,
                       in float3 vTexCoord1,
                       in int count,
                       inout float4 vOutTexCoord[4])
{
  // Zero all texture coordinates - this should be removed by optimizer in case of even count of texcoords
  //for (int i = 0; i < 4; i++) vOutTexCoord[i] = float4(0, 0, 0, 0);

  // Cycle for all required texcoords
  for (uint i = 0; i < count; i++)
  {
    // Get the texture coordinate XY indices within XYZW vector
    bool useWZ;
    if ((i%2) == 0)
    {
      useWZ = false;
    }
    else
    {
      useWZ = true;
    };

    // Get the out vector index (each one vOutTexCoord holds 2 texture coordinates)
    int halfFloor = i / 2;

    // Get the type of the texcoord
    float tid = VSC_TexCoordType[i / 4][i % 4];

    // Perform one of the transformations
    if (tid < 0.5f)
    {
      vTexCoord0.z = 1;
      float u = dot(vTexCoord0, VSC_TexTransform[i]._m00_m10_m30);
      float v = dot(vTexCoord0, VSC_TexTransform[i]._m01_m11_m31);
      if (useWZ)
      {
        vOutTexCoord[halfFloor].w = u;
        vOutTexCoord[halfFloor].z = v;
      }
      else
      {
        vOutTexCoord[halfFloor].x = u;
        vOutTexCoord[halfFloor].y = v;
      }
    }
    else
    {
      vTexCoord1.z = 1;
      float u = dot(vTexCoord1, VSC_TexTransform[i]._m00_m10_m30);
      float v = dot(vTexCoord1, VSC_TexTransform[i]._m01_m11_m31);
      if (useWZ)
      {
        vOutTexCoord[halfFloor].w = u;
        vOutTexCoord[halfFloor].z = v;
      }
      else
      {
        vOutTexCoord[halfFloor].x = u;
        vOutTexCoord[halfFloor].y = v;
      }
    }
  }
}

// Structure to hold outputs from transform functions
struct TransformOutput
{
  float4 position;
  float4 skinnedMatrix0;
  float4 skinnedMatrix1;
  float4 skinnedMatrix2;
  float4 skinnedPosition;
  float3 skinnedNormal;
  float4 transformedPosition;
  float4 alpha_X_X_instanceShadow;
  float4 projectedPosition;
};

// Structure to hold light values to be passed to PS in the end
struct AccomLights
{
  float4 ambient;
  float4 specular;
  float4 diffuse;
  float4 shadowReduction;
};

void VertexInitLights(in float4 alpha_X_X_instanceShadow, inout AccomLights al)
{
  // Fill out values
  al.ambient.xyz = VSC_AE.xyz;
  al.ambient.w = VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart.y * alpha_X_X_instanceShadow.x;
  al.specular = float4(0, 0, 0, alpha_X_X_instanceShadow.w);
  al.diffuse = float4(VSC_DForced.xyz, 1);
}

void VDoneLights(in AccomLights al, in float4 modulate, out float4 vOutAmbient, out float4 vOutSpecular, out float4 vOutDiffuse)
{
  // Copy the colors to output
  vOutAmbient = al.ambient * modulate * VSC_Tex0MaxColor;
  vOutSpecular = al.specular;
  vOutDiffuse = float4(al.diffuse.rgb * modulate.rgb * VSC_Tex0MaxColor.rgb, al.diffuse.a);
}

void VDoneLightsTerrain(in AccomLights al, in float3 bedColor, out float4 vOutAmbient, out float4 vOutSpecular)
{
  // Copy the colors to output
  vOutAmbient = float4(al.ambient.rgb * VSC_Tex0MaxColor.rgb * bedColor, al.ambient.a * VSC_Tex0MaxColor.a);
  vOutSpecular = al.specular;
}

// on some HW/targets we may use dedicated fogging unit
void VFog(in TransformOutput to, out float vFog)
{
  float d = sqrt(dot(to.transformedPosition.xyz, to.transformedPosition.xyz));
  float haze = Haze(d);
  vFog = haze*saturate((VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart.z - d) * VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart.w);
}

void VFogAlpha(in TransformOutput to, inout AccomLights al, out float vFog)
{
  float d = sqrt(dot(to.transformedPosition.xyz, to.transformedPosition.xyz));
  al.ambient.w = saturate(al.ambient.w * (VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart.z - d) * VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart.w);
  vFog = 1;
}

void VFogFogAlpha(in TransformOutput to, inout AccomLights al, out float vFog)
{
  float d = sqrt(dot(to.transformedPosition.xyz, to.transformedPosition.xyz));
  float haze = Haze(d);
  //float haze = d/1000;
  vFog = haze*saturate((VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart.z - d) * VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart.w);
  al.ambient.w = saturate(al.ambient.w * (VSC_Free_ExpFog_AFogEnd_RCPAFogEndMinusAFogStart.z - d) * VSC_Free_ExpFog_AFogEnd_RCPAFogEndMinusAFogStart.w);
}

void VFogNone(out float vFog)
{
  vFog = 1;
}

// Rendering model enumeration (note, that this encapsulates rendering mode and crater rendering of the engine together)
#define RMCommon        0
#define RMShadowBuffer  1
#define RMDepthMap      2
#define RMCrater        3 // Care must be taken when changing this number (see ::DoSetupSectionTL function)

// Boolean registers mapping
/*
FogMode:
A  B
0, 0 - FM_None
0, 1 - FM_Fog
1, 0 - FM_Alpha
1, 1 - FM_FogAlpha
*/
// See BOOLSTART
float FogModeA           : register(c256);
float FogModeB           : register(c257);
float AlphaShadowEnabled : register(c259);
float ShadowReceiverFlag : register(c260);
float SBTechniqueDefault : register(c261);
float EnableAlignNormal  : register(c262);

void VShadowReceiver(in TransformOutput to, out float4 oShadowMap)
{
  // Write shadow map UV transformation
  oShadowMap.xyz = mul(to.transformedPosition, VSC_ShadowmapMatrix);
  oShadowMap.w = to.transformedPosition.z;
}

#define SHCOLORCOEF 8
void VLPointSpotNAmbient(in TransformOutput to, inout float4 ambient, inout float4 shadowReduction)
{
  // Color accumulator
  float3 sumColor = float3(0, 0, 0);

  // Point lights
  for (int i = 0; i < VSC_PointLoopCount; i++)
  {
    float3 pointToLightVector = LPSData_New[i*4 + LPOINT_TransformedPos].xyz - to.skinnedPosition.xyz;
    float lightSquareDistance = dot(pointToLightVector, pointToLightVector);
    float invLightDistance = rsqrt(lightSquareDistance);
    float3 lightDirection = pointToLightVector * invLightDistance;
    float attenuation = LPSData_New[i*4 + LPOINT_Atten].x * invLightDistance * invLightDistance;
    sumColor += (max(0, dot(to.skinnedNormal, lightDirection)) * LPSData_New[i*4 + LPOINT_D].xyz + LPSData_New[i*4 + LPOINT_A].xyz) * min(0.3, attenuation);
  }

  // Spot lights
  for (int ii = 0; ii < VSC_SpotLoopCount; ii++)
  {
    int i = MAX_LIGHTS - ii - 1;
    float3 pointToLightVector = LPSData_New[i*4 + LSPOT_TransformedPos_CosPhiHalf].xyz - to.skinnedPosition.xyz;
    float lightSquareDistance = dot(pointToLightVector, pointToLightVector);
    float invLightDistance = rsqrt(lightSquareDistance);
    float3 lightDirection = pointToLightVector * invLightDistance;
    float rho = dot(-LPSData_New[i*4 + LSPOT_TransformedDir_CosThetaHalf].xyz, lightDirection);
    float spotlightFactor = (rho > LPSData_New[i*4 + LSPOT_TransformedDir_CosThetaHalf].w) ? 1 : (rho <= LPSData_New[i*4 + LSPOT_TransformedPos_CosPhiHalf].w) ? 0 : (rho - LPSData_New[i*4 + LSPOT_TransformedPos_CosPhiHalf].w) * LPSData_New[i*4 + LSPOT_D_InvCTHMCPH].w;
    float attenuation = LPSData_New[i*4 + LSPOT_A_Atten].w * invLightDistance * invLightDistance;
    sumColor += (max(0, dot(to.skinnedNormal, lightDirection)) * LPSData_New[i*4 + LSPOT_D_InvCTHMCPH].xyz + LPSData_New[i*4 + LSPOT_A_Atten].xyz) * min(0.3, attenuation * spotlightFactor);
  }

  // Increment the ambient light
  ambient.xyz += sumColor;

  // Calculate the shadow disappear coefficient (pass lightIntensity for precise PS calculation,
  // pass max(...) for fast PS calculation (f.i. for trees))
  float lightIntensity = dot(sumColor, float3(0.33, 0.33, 0.33));
  shadowReduction = float4(lightIntensity, lightIntensity, lightIntensity, max(1 - lightIntensity * SHCOLORCOEF, 0));
}

void VLPointSpotN(in TransformOutput to, inout AccomLights al)
{
  VLPointSpotNAmbient(to, al.ambient, al.shadowReduction);
}

TransformOutput VTransformLodEx(in float4 vPosition, in float3 vNormal,
                                in float3 lod,
                                out float2 grassAlphaOut,
                                uniform bool grass)
{
  // Define output
  TransformOutput to;

  // Original position
  to.position = vPosition;

  // Skinned matrix
  to.skinnedMatrix0 = float4(1, 0, 0, 0);
  to.skinnedMatrix1 = float4(0, 1, 0, 0);
  to.skinnedMatrix2 = float4(0, 0, 1, 0);

  // Skinned position and normal
  to.skinnedPosition = vPosition;
  to.skinnedNormal = vNormal;

  { // adjust vertex position based on terrain LOD
    float3 lodPos = to.skinnedPosition;
    //float3 relPos = VSC_CameraPosition.xyz-skinnedPosition.xyz;
    float2 relPos = VSC_CameraPosition.xz-to.skinnedPosition.xz;
    float distance2 = dot(relPos,relPos);

    float k = VSC_TerrainLODPars.x;
    float maxLod = VSC_TerrainLODPars.y;

    float logDistance = log2(distance2);
    float lodWanted = maxLod + k + logDistance*0.5;

    lodWanted = min(maxLod,max(lodWanted,0));

    // note: same code could handle 4 values, however we need the z component for other purposes
    // Sahrani does not support 4 lods anyway

    // array of 3 lod values
    float3 yArray = float3(to.skinnedPosition.y,lod.xy);


    // calculate selector coefficents for all four values
    // for each of the 4 points calculate distance from its index
    // weight is max(1-distance,0)
    float3 select = max(1-abs(lodWanted-float3(0,1,2)),0);

    // lod.z contains height of the grass level

    // first we can try is simply adding it
    // we need distance based attenuation
    // the distance should be matched to ground clutter rendering

    to.skinnedPosition.y = dot(yArray,select);

    if (grass)
    {
      float grassCoef = min(1,(sqrt(distance2)-15)*(1.0/15));
      // note: it is possible to include following line in the dot product above
      // however real gain is unlikely (mad will be used here) and this is much better readable
      // we subtract a small epsilon to be sure we end under the ground when close
      //skinnedPosition.y += lod.z*grassCoef - (1-grassCoef)*0.1;
      to.skinnedPosition.y += lod.z*grassCoef;
    }
  }

  // Transformed position
  to.transformedPosition = float4(mul(to.skinnedPosition, VSC_ViewMatrix), 1);

  if (grass)
  {
    float d = sqrt(dot(to.transformedPosition.xyz, to.transformedPosition.xyz));

    float3 grassAlpha = saturate((VSC_TerrainAlphaAdd - d) * VSC_TerrainAlphaMul);

    grassAlphaOut.x = grassAlpha.y*(1-grassAlpha.x); // disappering (both close and far)
    grassAlphaOut.y = grassAlpha.z; // color lerp (detail, satellite)
  }
  else
  {
    // not used, but compiler requires us to assign a value
    grassAlphaOut = 0;
  }

  // Instance shadow
  to.alpha_X_X_instanceShadow = float4(1, 1, 1, 1);

  // Project the position
  to.projectedPosition = mul(VSC_ProjMatrix, to.transformedPosition);

  // Return output
  return to;
}

// Modify water normal - see https://wiki.bistudio.com/index.php/Analysis_-_Kreslen%C3%AD_vody_%28Water_Rendering%29#Modifikace_norm.C3.A1ly
float3 ModifyWaterNormal(float3 normal, float3 camDirection, float modifCoef)
{
  float cosViewAngle = max(dot(float3(0, 1, 0), camDirection), 0);
  float normalLeanCoef = (log(0.01 + cosViewAngle) - log(0.01)) * (1 - cosViewAngle) * 0.2;
  return normalize(normal + camDirection * normalLeanCoef * modifCoef);
}

// Fresnel reflection is fully performed in PS. Here we're doing something similar:
// We influence the transparency of the shape according to viewing angle (if we're looking
// in a sharp angle, the water is not transparent)
float GetWaterViewA(float3 normal, float3 camDirection)
{
  // calculate alpha factor based on viewing angle
  float viewAngle = dot(camDirection, normal);

  const float minCosA = 0.17f; // cos 80 deg - full reflection, no transparency
  const float maxCosA = 0.86f; // cos 30 deg - maximum transparency

  // x1 = x0 *M + A
  // x0 = minCosA -> x1 = 0
  // x0 = maxCosA -> x1 = 1
  const float waterReflectAdd = -minCosA/(maxCosA-minCosA);
  const float waterReflectMul = 1/(maxCosA-minCosA);

  // Calculate the alpha
  return saturate(viewAngle * waterReflectMul + waterReflectAdd);
}
