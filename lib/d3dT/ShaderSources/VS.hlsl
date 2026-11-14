#line 2 "W:\c\Poseidon\lib\d3dT\ShaderSources\VS.hlsl"

#include "VS.h"

// Make sure the skinned normal points at the eye (revert the normal if it points out of the camera)
// This is f.i. suitable for polyplane rendering
void AlignNormal(inout float3 skinnedNormal, in float4 skinnedPosition)
{
  if (EnableAlignNormal > 0.0f)
  {
    float3 camDirection = normalize(VSC_CameraPosition.xyz - skinnedPosition.xyz);
    if (dot(camDirection, skinnedNormal) < 0) skinnedNormal = -skinnedNormal;
  }
}

TransformOutput VTransform(float4 vPosition, float3 vNormal)
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

  // Make sure the skinned normal points at the eye
  AlignNormal(to.skinnedNormal, to.skinnedPosition);
  
  // Transformed position
  to.transformedPosition = float4(mul(to.skinnedPosition, VSC_ViewMatrix), 1);

  // Instance shadow
  to.alpha_X_X_instanceShadow = VSC_Alpha_X_X_LandShadowIntensity;

  // Project the position
  to.projectedPosition = mul(VSC_ProjMatrix, to.transformedPosition);
  
  // Return output
  return to;
}

TransformOutput VTransformInstanced(float4 vPosition, float3 vNormal, uint4 vBlendIndices)
{
  // Define output
  TransformOutput to;

  // Original position
  to.position = vPosition;

  // Skinned matrix
//   float4x3 vm = instancingItems_42[1/*vBlendIndices.w*/].view_matrix;
//   to.skinnedMatrix0 = vm._m00_m10_m20_m30;
//   to.skinnedMatrix1 = vm._m01_m11_m21_m31;
//   to.skinnedMatrix2 = vm._m02_m12_m22_m32;
  to.skinnedMatrix0 = instancingItems_42_New[vBlendIndices.w * 4 + 0];
  to.skinnedMatrix1 = instancingItems_42_New[vBlendIndices.w * 4 + 1];
  to.skinnedMatrix2 = instancingItems_42_New[vBlendIndices.w * 4 + 2];

  // Skinned position and normal
  to.skinnedPosition.x = dot(to.skinnedMatrix0, vPosition);
  to.skinnedPosition.y = dot(to.skinnedMatrix1, vPosition);
  to.skinnedPosition.z = dot(to.skinnedMatrix2, vPosition);
  to.skinnedPosition.w = 1;
  to.skinnedNormal.x = dot(to.skinnedMatrix0.xyz, vNormal);
  to.skinnedNormal.y = dot(to.skinnedMatrix1.xyz, vNormal);
  to.skinnedNormal.z = dot(to.skinnedMatrix2.xyz, vNormal);
  to.skinnedNormal = normalize(to.skinnedNormal);

  // Make sure the skinned normal points at the eye
  AlignNormal(to.skinnedNormal, to.skinnedPosition);

  // Transformed position
  to.transformedPosition = float4(mul(to.skinnedPosition, VSC_ViewMatrix), 1);

  // Instance shadow
  //to.alpha_X_X_instanceShadow = instancingItems_42[1/*vBlendIndices.w*/].alpha_X_X_shadow;
  to.alpha_X_X_instanceShadow = instancingItems_42_New[vBlendIndices.w * 4 + 3];

  // Project the position
  to.projectedPosition = mul(VSC_ProjMatrix, to.transformedPosition);
  
  // Return output
  return to;
}

TransformOutput VTransformSkinned(float4 vPosition, float3 vNormal, float4 vBlendWeights, uint4 vBlendIndices)
{
  // Define output
  TransformOutput to;

  // Original position
  to.position = vPosition;

  // Skinned matrix
  float2 fRestBlendWeight = float2(1 - dot(float4(1, 1, 1, 1), vBlendWeights), 0);
  to.skinnedMatrix0 = view_matrix_56[(vBlendIndices - D3DCOLORtoUBYTE4(VSC_MatrixOffset)).x]._m00_m10_m20_m30 * vBlendWeights.x + fRestBlendWeight.xyyy;
  to.skinnedMatrix1 = view_matrix_56[(vBlendIndices - D3DCOLORtoUBYTE4(VSC_MatrixOffset)).x]._m01_m11_m21_m31 * vBlendWeights.x + fRestBlendWeight.yxyy;
  to.skinnedMatrix2 = view_matrix_56[(vBlendIndices - D3DCOLORtoUBYTE4(VSC_MatrixOffset)).x]._m02_m12_m22_m32 * vBlendWeights.x + fRestBlendWeight.yyxy;
  to.skinnedMatrix0 = view_matrix_56[(vBlendIndices - D3DCOLORtoUBYTE4(VSC_MatrixOffset)).y]._m00_m10_m20_m30 * vBlendWeights.y + to.skinnedMatrix0;
  to.skinnedMatrix1 = view_matrix_56[(vBlendIndices - D3DCOLORtoUBYTE4(VSC_MatrixOffset)).y]._m01_m11_m21_m31 * vBlendWeights.y + to.skinnedMatrix1;
  to.skinnedMatrix2 = view_matrix_56[(vBlendIndices - D3DCOLORtoUBYTE4(VSC_MatrixOffset)).y]._m02_m12_m22_m32 * vBlendWeights.y + to.skinnedMatrix2;
  to.skinnedMatrix0 = view_matrix_56[(vBlendIndices - D3DCOLORtoUBYTE4(VSC_MatrixOffset)).z]._m00_m10_m20_m30 * vBlendWeights.z + to.skinnedMatrix0;
  to.skinnedMatrix1 = view_matrix_56[(vBlendIndices - D3DCOLORtoUBYTE4(VSC_MatrixOffset)).z]._m01_m11_m21_m31 * vBlendWeights.z + to.skinnedMatrix1;
  to.skinnedMatrix2 = view_matrix_56[(vBlendIndices - D3DCOLORtoUBYTE4(VSC_MatrixOffset)).z]._m02_m12_m22_m32 * vBlendWeights.z + to.skinnedMatrix2;
  to.skinnedMatrix0 = view_matrix_56[(vBlendIndices - D3DCOLORtoUBYTE4(VSC_MatrixOffset)).w]._m00_m10_m20_m30 * vBlendWeights.w + to.skinnedMatrix0;
  to.skinnedMatrix1 = view_matrix_56[(vBlendIndices - D3DCOLORtoUBYTE4(VSC_MatrixOffset)).w]._m01_m11_m21_m31 * vBlendWeights.w + to.skinnedMatrix1;
  to.skinnedMatrix2 = view_matrix_56[(vBlendIndices - D3DCOLORtoUBYTE4(VSC_MatrixOffset)).w]._m02_m12_m22_m32 * vBlendWeights.w + to.skinnedMatrix2;

  // Skinned position and normal
  to.skinnedPosition.x = dot(to.skinnedMatrix0, vPosition);
  to.skinnedPosition.y = dot(to.skinnedMatrix1, vPosition);
  to.skinnedPosition.z = dot(to.skinnedMatrix2, vPosition);
  to.skinnedPosition.w = 1;
  to.skinnedNormal.x = dot(to.skinnedMatrix0.xyz, vNormal);
  to.skinnedNormal.y = dot(to.skinnedMatrix1.xyz, vNormal);
  to.skinnedNormal.z = dot(to.skinnedMatrix2.xyz, vNormal);
  to.skinnedNormal = normalize(to.skinnedNormal);

  // Make sure the skinned normal points at the eye
  //AlignNormal(to.skinnedNormal, to.skinnedPosition); Disabled, because some skinned shaders produces too many instructions then

  // Transformed position
  to.transformedPosition = float4(mul(to.skinnedPosition, VSC_ViewMatrix), 1);

  // Instance shadow
  to.alpha_X_X_instanceShadow = VSC_Alpha_X_X_LandShadowIntensity;

  // Project the position
  to.projectedPosition = mul(VSC_ProjMatrix, to.transformedPosition);
  
  // Return output
  return to;
}

TransformOutput VertexTransform(float4 vPosition, float3 vNormal, float4 vBlendWeights, uint4 vBlendIndices,
                                bool skinning, bool instancing)
{
  if (skinning)
  {
    return VTransformSkinned(vPosition, vNormal, vBlendWeights, vBlendIndices);
  }
  else
  {
    if (instancing)
    {
      return VTransformInstanced(vPosition, vNormal, vBlendIndices);
    }
    else
    {
      return VTransform(vPosition, vNormal);
    }
  }
}

// Structure to hold shader output values
struct ShaderOut
{
  float4 color0;
  float4 texCoord1;
  float4 texCoord2;
  float4 texCoord3;
  int maxTexCoordCount;
};

ShaderOut VertexInitOutput()
{
  // Define output
  ShaderOut so;
  
  // Fill out values
  so.color0 = float4(0, 0, 0, 0);
  so.texCoord1 = float4(0, 0, 0, 0);
  so.texCoord2 = float4(0, 0, 0, 0);
  so.texCoord3 = float4(0, 0, 0, 0);
  so.maxTexCoordCount = -1; // This must be overwritten by the particular shader
  
  // Return output
  return so;
}

// Structure to hold shader input values (input values, despite the output values, can have meaningful names)
struct ShaderIn
{
  float3 s;
  float3 t;
};

// Main light enumeration
#define ML_None     0
#define ML_Sun      1
#define ML_Sky      2
#define ML_SetColor 3

void VLDirection(in TransformOutput to, inout AccomLights al, inout float4 lighting)
{
  // Base diffuse color
  float3 H = normalize(normalize(VSC_CameraPosition.xyz - to.skinnedPosition.xyz) - VSC_LDirectionTransformedDir.xyz);
  float lightDot = dot(to.skinnedNormal, -VSC_LDirectionTransformedDir.xyz);
  float4 lightingVector = lit(lightDot, dot(to.skinnedNormal, H), VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart.x);
  al.diffuse.xyz += lightingVector.y * VSC_LDirectionD;
  
  // calculate light reflected from the enviroment (from the opposite side) and the through light
  float reverseSide = max(-lightDot,0);
  //vDiffuse.xyz += reverseSide * VSC_LDirectionGround * VSC_LDirectionD;
  al.diffuse.xyz += reverseSide * VSC_LDirectionD.xyz * (VSC_LDirectionGround.xyz + VSC_AE.www);

  // 0.5 is here, because usual PS is simplier with this value to be halved
  al.specular.xyz += lightingVector.z * VSC_LDirectionS * 0.5;
  
  // this is used for grass - we know TEXCOORD1 is not used for anything else there
  lighting = (al.diffuse + al.specular) * al.specular.w + al.ambient;
}

void VLDirectionSky(in TransformOutput to, inout AccomLights al)
{
  float coef = dot(to.skinnedNormal, VSC_LDirectionTransformedDirSky.xyz);
  coef = pow(max(coef, 0), 6);
  al.ambient.xyz += VSC_LDirectionSunSkyColor.xyz * coef + VSC_LDirectionSkyColor.xyz * (1-coef);
}

void VLDirectionSetColor(inout AccomLights al)
{
  al.ambient = VSC_LDirectionSetColorColor;
}

void VAlphaShadow(in TransformOutput to, inout AccomLights al)
{

  float s = max(dot(to.skinnedNormal, -VSC_LDirectionTransformedDir.xyz), 0);
  al.diffuse.w = s;
}

//////////////////////////////////////////////////////////////////

void VFBasic( in ShaderIn si, in TransformOutput to, inout AccomLights al,
              in int mainLight, in bool alphaShadowEnabled, inout ShaderOut so)
{
  // Use some type of the main light (if not ML_None)
  if (mainLight == ML_Sun)
  {
    VLDirection(to, al, so.texCoord1);
  }
  else if (mainLight == ML_Sky)
  {
    VLDirectionSky(to, al);
  }
  else if (mainLight == ML_SetColor)
  {
    VLDirectionSetColor(al);
  }

  // Alpha shadow fragment (just for certain set of shaders (no NormalMap ones))
  if (alphaShadowEnabled)
  {
    VAlphaShadow(to, al);
  }
  
  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 2;
}

//////////////////////////////////////////////////////////////////

void VFNormalMap( in ShaderIn si, in TransformOutput to, inout AccomLights al,
                  in int mainLight, in bool alphaShadowEnabled, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal);

  // Calculate the ligth local
  so.color0.xyz = (lightLocal + 1) * 0.5;

  al.specular.xyz = lightLocal;

  // Calculate the halfway local
  so.texCoord3.xyz = halfwayLocal;

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 4;
}

//////////////////////////////////////////////////////////////////

void VFNormalMapDiffuse(in ShaderIn si, in TransformOutput to, inout AccomLights al,
                        in int mainLight, in bool alphaShadowEnabled, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal);

  // Calculate the ligth local
  so.color0.xyz = (lightLocal + 1) * 0.5;

  // Calculate the specular color
  float3 H = normalize(normalize(VSC_CameraPosition.xyz - to.skinnedPosition.xyz) - VSC_LDirectionTransformedDir.xyz);
  float4 lightingVector = lit(dot(to.skinnedNormal, -VSC_LDirectionTransformedDir.xyz), dot(to.skinnedNormal, H), VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart.x);
  // 0.5 is here, because usual PS is simplier with this value to be halved
  al.specular.xyz += lightingVector.z * VSC_LDirectionS * 0.5;

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 3;
}

//////////////////////////////////////////////////////////////////

void NormalMapThroughFunction(
  float3 skinnedMatrix0,
  float3 skinnedMatrix1,
  float3 skinnedMatrix2,
  in float3 lightLocal,
  in float4 vPosition,
  in float3 skinnedNormal,
  in float4 skinnedPosition,
  inout float4 vSpecular,
  inout float4 vAmbient,
  out float4 oDiffuseAtten_AmbientAtten,
  bool fadeFace)
{
  vSpecular.xyz = lightLocal;

  // calculate interestion of direction light ray with tree crown sphere
  // VSC_LDirectionTransformedDir - model or world space light direction
  
  // convert light into model space
  float3 lightVertex = normalize(
    skinnedMatrix0 * VSC_LDirectionTransformedDir.x +
    skinnedMatrix1 * VSC_LDirectionTransformedDir.y +
    skinnedMatrix2 * VSC_LDirectionTransformedDir.z
  );
  
  // VSC_TreeCrown[0] - crown sphere center
  // VSC_TreeCrown[1] - crown sphere size, xyz: 1/vector span, w: 1/sphere radius
  
  // position inside of unit sphere
  float3 x = (vPosition-VSC_TreeCrown[0])*VSC_TreeCrown[1];

  // calculate ambient attenuation
  oDiffuseAtten_AmbientAtten.w = sqrt(x.x*x.x+x.z*x.z);
  oDiffuseAtten_AmbientAtten.z = x.y * 0.5+0.5; // map to 0..1 range
  
  // ligth direction transformed into unit sphere space
  float3 l = lightVertex*VSC_TreeCrown[1];
  
  // distance traveled is:
  // s = ((X.L)+sqrt((X.L)*(X.L)+(L.L)-(L.L)(X.X))/(L.L)

  float xDotL = dot(x,l);
  float lDotL = dot(l,l);
  float xDotX = dot(x,x);
  
  float sDiscriminant = max(xDotL*xDotL + lDotL - lDotL*xDotX,0);
  
  float sizeInvLDotL = 1/lDotL * VSC_TreeCrown[1].w;
  float sSqrt = sqrt(sDiscriminant);

  float s = (xDotL + sSqrt)*sizeInvLDotL;
  float sMax = sSqrt*2*sizeInvLDotL;
  
  oDiffuseAtten_AmbientAtten.xy = min(s,sMax);
  //oDiffuseAtten_AmbientAtten.xy = skinnedMatrix0.xy;

  // disappear based on vertex normal
  if (fadeFace)
  {
    // Get the vertex to camera vector
    float3 vertex2Camera = VSC_CameraPosition.xyz - skinnedPosition.xyz;
    
    // Calculate the dot product
    //float facingCamera = dot(skinnedNormal, normalize(vertex2Camera));
    float degenerateFactor = saturate((dot(skinnedNormal, normalize(vertex2Camera))-0.1)*10);
    float facingCamera = saturate(dot(skinnedNormal, -VSC_CameraDirection.xyz) * 1.5f);
    
    // Omit faces with less than 2.56 degree
    //if (facingCamera > 0.001f)
    {
      // Modify the alpha
      vAmbient.w *= facingCamera*degenerateFactor;
    }
  }
}

void VFNormalMapThrough(in ShaderIn si, in TransformOutput to, inout AccomLights al,
                        in int mainLight, in bool alphaShadowEnabled, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal);

  // Perform the tree shader
  NormalMapThroughFunction(to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2,
    lightLocal, to.position, to.skinnedNormal, to.skinnedPosition,
    al.specular, al.ambient, so.texCoord1, true);

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 2;
}

void VFNormalMapThroughNoFade(in ShaderIn si, in TransformOutput to, inout AccomLights al,
                              in int mainLight, in bool alphaShadowEnabled, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal);

  // Perform the tree shader
  NormalMapThroughFunction(to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2,
    lightLocal, to.position, to.skinnedNormal, to.skinnedPosition,
    al.specular, al.ambient, so.texCoord1, false);

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 2;
}

void VFNormalMapSpecularThrough(in ShaderIn si, in TransformOutput to, inout AccomLights al,
                                in int mainLight, in bool alphaShadowEnabled, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal);

  // Set the halfway vector
  so.texCoord3.xyz = halfwayLocal;

  // Perform the tree shader
  NormalMapThroughFunction(to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2,
    lightLocal, to.position, to.skinnedNormal, to.skinnedPosition,
    al.specular, al.ambient, so.texCoord1, true);

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 2;
}

void VFNormalMapSpecularThroughNoFade(in ShaderIn si, in TransformOutput to, inout AccomLights al,
                                      in int mainLight, in bool alphaShadowEnabled, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal);

  // Set the halfway vector
  so.texCoord3.xyz = halfwayLocal;

  // Perform the tree shader
  NormalMapThroughFunction(to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2,
    lightLocal, to.position, to.skinnedNormal, to.skinnedPosition,
    al.specular, al.ambient, so.texCoord1, false);

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 2;
}

//////////////////////////////////////////////////////////////////

void VFBasicAS( in ShaderIn si, in TransformOutput to, inout AccomLights al,
                in int mainLight, in bool alphaShadowEnabled, inout ShaderOut so)
{
  // Use some type of the main light (if not ML_None)
  if (mainLight == ML_Sun)
  {
    VLDirection(to, al, so.texCoord1);
  }
  else if (mainLight == ML_Sky)
  {
    VLDirectionSky(to, al);
  }
  else if (mainLight == ML_SetColor)
  {
    VLDirectionSetColor(al);
  }

  // Alpha shadow fragment (just for certain set of shaders (no NormalMap ones))
  if (alphaShadowEnabled)
  {
    VAlphaShadow(to, al);
  }
  
  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 4;
}

//////////////////////////////////////////////////////////////////

void VFNormalMapAS( in ShaderIn si, in TransformOutput to, inout AccomLights al,
                    in int mainLight, in bool alphaShadowEnabled, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal);

  // Calculate the ligth local
  so.color0.xyz = (lightLocal + 1) * 0.5;

  al.specular.xyz = lightLocal;

  // Calculate the halfway local
  so.texCoord3.xyz = halfwayLocal;

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 6;
}

//////////////////////////////////////////////////////////////////

void VFNormalMapDiffuseAS(in ShaderIn si, in TransformOutput to, inout AccomLights al,
                          in int mainLight, in bool alphaShadowEnabled, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal);

  // Calculate the ligth local
  so.color0.xyz = (lightLocal + 1) * 0.5;

  // Calculate the specular color
  float3 H = normalize(normalize(VSC_CameraPosition.xyz - to.skinnedPosition.xyz) - VSC_LDirectionTransformedDir.xyz);
  float4 lightingVector = lit(dot(to.skinnedNormal, -VSC_LDirectionTransformedDir.xyz), dot(to.skinnedNormal, H), VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart.x);
  // 0.5 is here, because usual PS is simplier with this value to be halved
  al.specular.xyz += lightingVector.z * VSC_LDirectionS * 0.5;

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 5;
}

//////////////////////////////////////////////////////////////////

void VFGlass( in ShaderIn si, in TransformOutput to, inout AccomLights al,
              in int mainLight, in bool alphaShadowEnabled, inout ShaderOut so)
{
  // Use some type of the main light (if not ML_None)
  if (mainLight == ML_Sun)
  {
    VLDirection(to, al, so.texCoord1);
  }
  else if (mainLight == ML_Sky)
  {
    VLDirectionSky(to, al);
  }
  else if (mainLight == ML_SetColor)
  {
    VLDirectionSetColor(al);
  }

  // Alpha shadow fragment (just for certain set of shaders (no NormalMap ones))
  if (alphaShadowEnabled)
  {
    VAlphaShadow(to, al);
  }

  // Transform position to LWS
  so.texCoord1 = float4(mul(to.skinnedPosition, VSC_LWSMatrix), 1);
  
  // Transform normal to LWS
  so.texCoord2.x = dot(VSC_LWSMatrix._m00_m10_m20, to.skinnedNormal);
  so.texCoord2.y = dot(VSC_LWSMatrix._m01_m11_m21, to.skinnedNormal);
  so.texCoord2.z = dot(VSC_LWSMatrix._m02_m12_m22, to.skinnedNormal);
  so.texCoord2.w = 1;

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 1;
}

//////////////////////////////////////////////////////////////////

/*
  List of program defined macros:

    PRGDEFName,
    PRGDEFSTPresent,
    PRGDEFUVMapTex1,
    PRGDEFMainLight,
    PRGDEFRenderingMode,
    PRGDEFSkinning,
    PRGDEFInstancing)
*/

//! Macro to concatenate 2 strings
#define CAT(a,b) a##b

void VSShaderPool(
  in float4 vPosition                           : POSITION,
  in uint4   iNormal                            : NORMAL,
#if PRGDEFSkinning == 1
  in uint4 iBlendWeights                        : BLENDWEIGHT,
  in uint4 vBlendIndices                        : BLENDINDICES,
#endif
#if PRGDEFInstancing == 1
  in uint4 vBlendIndices                        : BLENDINDICES,
#endif
#if PRGDEFSTPresent == 1
  in uint4 iS                                   : TANGENT0,
  in uint4 iT                                   : TANGENT1,
#else
  in uint4 iSDiscard                            : TANGENT0,
  in uint4 iTDiscard                            : TANGENT1,
#endif
  in int2 iTexCoord0                            : TEXCOORD0,
#if PRGDEFUVMapTex1 == 1
  in int2 iTexCoord1                            : TEXCOORD1,
#else
  in int2 iTexCoord1                            : TEXCOORD0,
  in int2 iTexCoord1Discard                     : TEXCOORD1,
#endif
  out float4 oProjectedPosition                 : SV_POSITION,
#if PRGDEFRenderingMode == RMCommon
  out float4 oOutAmbient                        : TEXCOORD_AMBIENT,
  out float4 oOutSpecular                       : TEXCOORD_SPECULAR,
  out float4 oOutDiffuse                        : TEXCOORD_DIFFUSE_SI,
  out float4 oShadowMap                         : TEXCOORD5,
  out float4 oColor0                            : COLOR0,
  out float4 oShadowReduction_Fog               : COLOR1,
#elif PRGDEFRenderingMode == RMShadowBuffer
  out float4 oPosition                          : TEXCOORD7,
#elif PRGDEFRenderingMode == RMDepthMap
  out float4 oPosition                          : TEXCOORD7,
#elif PRGDEFRenderingMode == RMCrater
  out float4 oShadowReduction_Fog               : COLOR1,
  out float4 oPosition                          : TEXCOORD7,
#endif
  out float4 oTexCoord[4]                       : TEXCOORD0
)
{
  // Create vS and vT with empty values, if ST is not present
#if PRGDEFSTPresent != 1
  uint4 iS = uint4(0, 0, 0, 0);
  uint4 iT = uint4(0, 0, 0, 0);
#endif

  // Read floats from integers
  float3 vNormal = BYTETOFLOAT01(iNormal.xyz);
  float3 vS = BYTETOFLOAT01(iS.xyz);
  float3 vT = BYTETOFLOAT01(iT.xyz);
#if PRGDEFSkinning == 1
  float4 vBlendWeights = BYTETOFLOAT01(iBlendWeights);
#endif
  float3 vTexCoord0 = float3(iTexCoord0.xy, 1);
  float3 vTexCoord1 = float3(iTexCoord1.xy, 1);

  // Decompress compressed vectors
  DecompressVector(vS);
  DecompressVector(vT);
  DecompressVector(vNormal);
  
  // Do the vertex transformations
#if PRGDEFSkinning == 1
  TransformOutput to = VertexTransform(vPosition, vNormal, vBlendWeights, vBlendIndices, true, false);
#elif PRGDEFInstancing == 1
  TransformOutput to = VertexTransform(vPosition, vNormal, float4(0, 0, 0, 0), vBlendIndices, false, true);
#else
  TransformOutput to = VertexTransform(vPosition, vNormal, float4(0, 0, 0, 0), uint4(0, 0, 0, 0), false, false);
#endif
  oProjectedPosition = to.projectedPosition;

  // Zero the output texcoords to make sure all are set (most of it probably will be removed by optimizer)
  for (int i = 0; i < 4; i++) oTexCoord[i] = float4(0, 0, 0, 0);

  #if PRGDEFRenderingMode == RMCommon
  {
    AccomLights al;
    al.ambient = float4(0, 0, 0, 0);
    al.specular = float4(0, 0, 0, 0);
    al.diffuse = float4(0, 0, 0, 0);
    al.shadowReduction = float4(0, 0, 0, 1);
    VertexInitLights(to.alpha_X_X_instanceShadow, al);
    ShaderOut so = VertexInitOutput();
    ShaderIn si;
    si.s = vS;
    si.t = vT;
    CAT(VF,PRGDEFName)(si, to, al, PRGDEFMainLight, AlphaShadowEnabled > 0.0f, so);
    oColor0 = so.color0;
    oTexCoord[1] = so.texCoord1;
    oTexCoord[2] = so.texCoord2;
    oTexCoord[3] = so.texCoord3;
    
    // Apply fog
    float oFog;
    if (FogModeA > 0.0f)
    {
      if (FogModeB > 0.0f)
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
      if (FogModeB > 0.0f)
      {
        VFog(to, oFog);
      }
      else
      {
        VFogNone(oFog);
      }
    }

    // Initialize shadow variable designed for shadow receiving
    if (ShadowReceiverFlag > 0.0f)
    {
      VShadowReceiver(to, oShadowMap);
    }
    else
    {
      oShadowMap = 0;
    }

    if ((PRGDEFMainLight == ML_None) || (PRGDEFMainLight == ML_Sun))
    {
      VLPointSpotN(to, al);
    }
    VDoneLights(al, float4(1, 1, 1, 1), oOutAmbient, oOutSpecular, oOutDiffuse);
    oShadowReduction_Fog = al.shadowReduction;
    oShadowReduction_Fog.z = oFog;
    TexCoordTransform(vTexCoord0, vTexCoord1, so.maxTexCoordCount, oTexCoord);
  }
  #else
  {
    TexCoordTransform(vTexCoord0, vTexCoord1, 1, oTexCoord);
    #if PRGDEFRenderingMode == RMShadowBuffer
    {
      if (SBTechniqueDefault > 0.0f)
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
      oPosition.xyz = to.position.xyz;
      oPosition.w = max(dot(to.skinnedNormal, -VSC_LDirectionTransformedDir.xyz), 0);
    }
    #endif
  }
  #endif
}

