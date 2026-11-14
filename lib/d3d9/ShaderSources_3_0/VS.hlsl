#include "VS.h"

// Make sure the skinned normal points at the eye (revert the normal if it points out of the camera)
// This is f.i. suitable for polyplane rendering
bool AlignNormal(inout float3 skinnedNormal, in float4 skinnedPosition)
{
  bool normalAligned = false;
  if (EnableAlignNormal)
  {
    // no need to normalize - normalize does not change a sign
    float3 camDirection = VSC_CameraPosition.xyz - skinnedPosition.xyz;
    if (dot(camDirection, skinnedNormal) < 0)
    {
      skinnedNormal = -skinnedNormal;
      normalAligned = true;
    }
  }
  return normalAligned;
}

//-----------------------------------------------------------------------------------------------
/*!
prepare matrix converting from tangent space into local world space and into w coordinates put 
transformed position
\param tspace 		tangent space matrix with STN rows
\param skinnedPos	position 
\param row0				xyz matrix, w is x coord of transformed position
\param row1				xyz matrix, w is y coord of transformed position
\param row2				xyz matrix, w is z coord of transformed position
*/	
//-----------------------------------------------------------------------------------------------
void PrepareTSpace2LWorldMatrix(float3x3 tspace, float4 skinnedPos, out float4 row0, out float4 row1, out float4 row2)
{
	float3 lwsPosition = mul(skinnedPos, VSC_LWSMatrix);
	row0.w = lwsPosition.x;
	row1.w = lwsPosition.y;
	row2.w = lwsPosition.z;

	// We want to get STN2LWS matrix wich is 9 dots of the second and first matrix
	row0.x = dot(tspace[0], VSC_LWSMatrix._m00_m10_m20);
	row1.x = dot(tspace[0], VSC_LWSMatrix._m01_m11_m21);
	row2.x = dot(tspace[0], VSC_LWSMatrix._m02_m12_m22);
	row0.y = dot(tspace[1], VSC_LWSMatrix._m00_m10_m20);
	row1.y = dot(tspace[1], VSC_LWSMatrix._m01_m11_m21);
	row2.y = dot(tspace[1], VSC_LWSMatrix._m02_m12_m22);
	row0.z = dot(tspace[2], VSC_LWSMatrix._m00_m10_m20);
	row1.z = dot(tspace[2], VSC_LWSMatrix._m01_m11_m21);
	row2.z = dot(tspace[2], VSC_LWSMatrix._m02_m12_m22);
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
  to.normalAligned = AlignNormal(to.skinnedNormal, to.skinnedPosition);
  
  // Transformed position
  to.transformedPosition = float4(mul(to.skinnedPosition, VSC_ViewMatrix), 1);

  #if PRGDEFRenderingMode==RMShadowBuffer
  
  to.transformedPosition.xyz += VSC_LDirectionTransformedDirSky.xyz*VSC_ShadowVolumeLength.x;
  
  #endif
  
  // Instance shadow
  to.instanceColor = VSC_InstanceColor;
  to.instanceShadow = VSC_InstanceLandShadowIntensity.x;

  // Project the position
  to.projectedPosition = mul(VSC_ProjMatrix, to.transformedPosition);
  
  // Return output
  return to;
}

TransformOutput VTransformInstanced(float4 vPosition, float3 vNormal, float4 vBlendIndices)
{
  // Define output
  TransformOutput to;

  // Original position
  to.position = vPosition;

  // Skinned matrix
  float4x3 vm = instancingItems_33[D3DCOLORtoUBYTE4(vBlendIndices).w].view_matrix;
  to.skinnedMatrix0 = vm._m00_m10_m20_m30;
  to.skinnedMatrix1 = vm._m01_m11_m21_m31;
  to.skinnedMatrix2 = vm._m02_m12_m22_m32;

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
  to.normalAligned = AlignNormal(to.skinnedNormal, to.skinnedPosition);

  // Transformed position
  to.transformedPosition = float4(mul(to.skinnedPosition, VSC_ViewMatrix), 1);

  #if PRGDEFRenderingMode==RMShadowBuffer
  
  to.transformedPosition.xyz += VSC_LDirectionTransformedDirSky.xyz*VSC_ShadowVolumeLength.x;
  
  #endif

  // Instance shadow
  to.instanceColor = instancingItems_33[D3DCOLORtoUBYTE4(vBlendIndices).w].color;
  to.instanceShadow = instancingItems_33[D3DCOLORtoUBYTE4(vBlendIndices).w].shadow_x_x_x.x;
  //to.instanceShadow = instancingItems_33[D3DCOLORtoUBYTE4(vBlendIndices).w].shadow_x_x_x.w;
  //to.instanceShadow = 1;

  // Project the position
  to.projectedPosition = mul(VSC_ProjMatrix, to.transformedPosition);
  
  // Return output
  return to;
}

TransformOutput VTransformSkinned(float4 vPosition, float3 vNormal, float4 vBlendWeights, float4 vBlendIndices)
{
  // Define output
  TransformOutput to;

  // Original position
  to.position = vPosition;

  // Skinned matrix
  float2 fRestBlendWeight = float2(1 - dot(float4(1, 1, 1, 1), vBlendWeights), 0);
  to.skinnedMatrix0 = view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndices - VSC_MatrixOffset).x]._m00_m10_m20_m30 * D3DCOLORtoFLOAT4(vBlendWeights).x + fRestBlendWeight.xyyy;
  to.skinnedMatrix1 = view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndices - VSC_MatrixOffset).x]._m01_m11_m21_m31 * D3DCOLORtoFLOAT4(vBlendWeights).x + fRestBlendWeight.yxyy;
  to.skinnedMatrix2 = view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndices - VSC_MatrixOffset).x]._m02_m12_m22_m32 * D3DCOLORtoFLOAT4(vBlendWeights).x + fRestBlendWeight.yyxy;
  to.skinnedMatrix0 = view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndices - VSC_MatrixOffset).y]._m00_m10_m20_m30 * D3DCOLORtoFLOAT4(vBlendWeights).y + to.skinnedMatrix0;
  to.skinnedMatrix1 = view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndices - VSC_MatrixOffset).y]._m01_m11_m21_m31 * D3DCOLORtoFLOAT4(vBlendWeights).y + to.skinnedMatrix1;
  to.skinnedMatrix2 = view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndices - VSC_MatrixOffset).y]._m02_m12_m22_m32 * D3DCOLORtoFLOAT4(vBlendWeights).y + to.skinnedMatrix2;
  to.skinnedMatrix0 = view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndices - VSC_MatrixOffset).z]._m00_m10_m20_m30 * D3DCOLORtoFLOAT4(vBlendWeights).z + to.skinnedMatrix0;
  to.skinnedMatrix1 = view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndices - VSC_MatrixOffset).z]._m01_m11_m21_m31 * D3DCOLORtoFLOAT4(vBlendWeights).z + to.skinnedMatrix1;
  to.skinnedMatrix2 = view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndices - VSC_MatrixOffset).z]._m02_m12_m22_m32 * D3DCOLORtoFLOAT4(vBlendWeights).z + to.skinnedMatrix2;
  to.skinnedMatrix0 = view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndices - VSC_MatrixOffset).w]._m00_m10_m20_m30 * D3DCOLORtoFLOAT4(vBlendWeights).w + to.skinnedMatrix0;
  to.skinnedMatrix1 = view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndices - VSC_MatrixOffset).w]._m01_m11_m21_m31 * D3DCOLORtoFLOAT4(vBlendWeights).w + to.skinnedMatrix1;
  to.skinnedMatrix2 = view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndices - VSC_MatrixOffset).w]._m02_m12_m22_m32 * D3DCOLORtoFLOAT4(vBlendWeights).w + to.skinnedMatrix2;

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
  to.normalAligned = false;
  //AlignNormal(to.skinnedNormal, to.skinnedPosition); Disabled, because some skinned shaders produces too many instructions then

  // Transformed position
  to.transformedPosition = float4(mul(to.skinnedPosition, VSC_ViewMatrix), 1);

  #if PRGDEFRenderingMode==RMShadowBuffer
  
  to.transformedPosition.xyz += VSC_LDirectionTransformedDirSky.xyz*VSC_ShadowVolumeLength.x;
  
  #endif
  
  // Instance shadow
  to.instanceColor = VSC_InstanceColor;
  to.instanceShadow = VSC_InstanceLandShadowIntensity.x;

  // Project the position
  to.projectedPosition = mul(VSC_ProjMatrix, to.transformedPosition);
  
  // Return output
  return to;
}

TransformOutput VertexTransform(float4 vPosition, float3 vNormal, float4 vBlendWeights, float4 vBlendIndices,
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
  float4 texCoord4;
  int maxTexCoordCount;
  bool forceFogSimple;
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
  so.texCoord4 = float4(0, 0, 0, 0);
  so.maxTexCoordCount = -1; // This must be overwritten by the particular shader
  so.forceFogSimple = false;
  
  // Return output
  return so;
}

// Structure to hold shader input values (input values, despite the output values, can have meaningful names)
struct ShaderIn
{
  float3 s;
  float3 t;
};

void VLDirection(in TransformOutput to, inout AccomLights al, inout float4 lighting, inout float4 x_x_nDotU_nDotL)
{
  // Base diffuse color
  float3 H = normalize(normalize(VSC_CameraPosition.xyz - to.skinnedPosition.xyz) - VSC_LDirectionTransformedDir.xyz);
  float lightDot = dot(to.skinnedNormal, -VSC_LDirectionTransformedDir.xyz);
  float4 lightingVector = lit(lightDot, dot(to.skinnedNormal, H), VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart.x);
  //al.diffuse.xyz += lightingVector.y * VSC_LDirectionD;
  
  // calculate light reflected from the enviroment (from the opposite side) and the through light
  float reverseSide = max(-lightDot,0);
  //vDiffuse.xyz += reverseSide * VSC_LDirectionGround * VSC_LDirectionD;
  //al.diffuse.xyz += reverseSide * VSC_LDirectionD.xyz * (VSC_LDirectionGround.xyz + VSC_AE.www);

  al.specular.xyz += lightingVector.z * VSC_LDirectionS;
  
  // this is used for grass - we know TEXCOORD1 is not used for anything else there
  lighting = al.specular.wwww;

  // Value to pass to PS - diffuse and diffuse ground will be calculated there instead in VS
  x_x_nDotU_nDotL.z = dot(to.skinnedNormal, VSC_UDirectionTransformedDir.xyz);
  x_x_nDotU_nDotL.w = lightDot;
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

//////////////////////////////////////////////////////////////////

void VFBasic( in ShaderIn si, in TransformOutput to, inout AccomLights al, in int mainLight, inout ShaderOut so)
{
  // Use some type of the main light (if not VML_None)
  if (mainLight == VML_Sun)
  {
    VLDirection(to, al, so.texCoord1, so.texCoord3);
  }
  else if (mainLight == VML_Sky)
  {
    VLDirectionSky(to, al);
  }
  else if (mainLight == VML_SetColor)
  {
    VLDirectionSetColor(al);
  }

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 2;
}

void VFGrass( in ShaderIn si, inout TransformOutput to, inout AccomLights al, in int mainLight, inout ShaderOut so)
{
  // Force normal to point up (shader will be simplified then)
  to.skinnedNormal = float3(0, 1, 0);

  VLDirection(to, al, so.texCoord1, so.texCoord3);
  so.texCoord1.rgb = to.instanceColor.rgb;

  // Use some type of the main light (if not VML_None)
//   if (mainLight == VML_Sun)
//   {
//     VLDirection(to, al, so.texCoord1, so.texCoord3);
//     so.texCoord1.rgb = to.instanceColor.rgb;
//   }
//   else if (mainLight == VML_Sky)
//   {
//     VLDirectionSky(to, al);
//   }
//   else if (mainLight == VML_SetColor)
//   {
//     VLDirectionSetColor(al);
//   }

  // We know grass is using always the simple fog
  so.forceFogSimple = true;

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 2;
}

//////////////////////////////////////////////////////////////////

void VFNormalMap( in ShaderIn si, in TransformOutput to, inout AccomLights al,
                  in int mainLight, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  float3 upLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal, upLocal);

  // Calculate the ligth local
  so.color0.xyz = lightLocal;

  al.specular.xyz = lightLocal;

  // Calculate the halfway local
  so.texCoord3.xyz = halfwayLocal;

  // Store the up local
  so.texCoord4.xyz = upLocal;

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 4;
}

//////////////////////////////////////////////////////////////////

void VFNormalMapDiffuse(in ShaderIn si, in TransformOutput to, inout AccomLights al,
                        in int mainLight, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  float3 upLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal, upLocal);

  // Calculate the ligth local
  so.color0.xyz = lightLocal;

  // Store the up local
  so.texCoord4.xyz = upLocal;

  // Calculate the specular color
  float3 H = normalize(normalize(VSC_CameraPosition.xyz - to.skinnedPosition.xyz) - VSC_LDirectionTransformedDir.xyz);
  float4 lightingVector = lit(dot(to.skinnedNormal, -VSC_LDirectionTransformedDir.xyz), dot(to.skinnedNormal, H), VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart.x);
  al.specular.xyz += lightingVector.z * VSC_LDirectionS;

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
    float facingCamera = saturate(dot(skinnedNormal, -VSC_CameraDirectionInvDepth.xyz) * 1.5f);
    
    // Omit faces with less than 2.56 degree
    //if (facingCamera > 0.001f)
    {
      // Modify the alpha
      vAmbient.w *= facingCamera*degenerateFactor;
    }
  }
}

void VFNormalMapThrough(in ShaderIn si, in TransformOutput to, inout AccomLights al,
                        in int mainLight, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  float3 upLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal, upLocal);

  // Perform the tree shader
  NormalMapThroughFunction(to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2,
    lightLocal, to.position, to.skinnedNormal, to.skinnedPosition,
    al.specular, al.ambient, so.texCoord1, true);

  // Store the up local
  so.texCoord4.xyz = upLocal;

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 2;
}

void VFNormalMapThroughNoFade(in ShaderIn si, in TransformOutput to, inout AccomLights al,
                              in int mainLight, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  float3 upLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal, upLocal);

  // Perform the tree shader
  NormalMapThroughFunction(to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2,
    lightLocal, to.position, to.skinnedNormal, to.skinnedPosition,
    al.specular, al.ambient, so.texCoord1, false);

  // Store the up local
  so.texCoord4.xyz = upLocal;

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 2;
}

void VFNormalMapSpecularThrough(in ShaderIn si, in TransformOutput to, inout AccomLights al,
                                in int mainLight, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  float3 upLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal, upLocal);

  // Set the halfway vector
  so.texCoord3.xyz = halfwayLocal;

  // Perform the tree shader
  NormalMapThroughFunction(to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2,
    lightLocal, to.position, to.skinnedNormal, to.skinnedPosition,
    al.specular, al.ambient, so.texCoord1, true);

  // Store the up local
  so.texCoord4.xyz = upLocal;

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 2;
}

void VFNormalMapSpecularThroughNoFade(in ShaderIn si, in TransformOutput to, inout AccomLights al,
                                      in int mainLight, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  float3 upLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal, upLocal);

  // Set the halfway vector
  so.texCoord3.xyz = halfwayLocal;

  // Perform the tree shader
  NormalMapThroughFunction(to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2,
    lightLocal, to.position, to.skinnedNormal, to.skinnedPosition,
    al.specular, al.ambient, so.texCoord1, false);

  // Store the up local
  so.texCoord4.xyz = upLocal;

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 2;
}

void TreeFunction(
  float3 skinnedMatrix0,
  float3 skinnedMatrix1,
  float3 skinnedMatrix2,
  in float3 lightLocal,
  in float4 vPosition,
  in float3 skinnedNormal,
  in float4 skinnedPosition,
  inout float4 vSpecular,
  inout float4 vAmbient,
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

  // disappear based on vertex normal
  if (fadeFace)
  {
    // Get the vertex to camera vector
    float3 vertex2Camera = VSC_CameraPosition.xyz - skinnedPosition.xyz;
    
    // Calculate the dot product
    //float facingCamera = dot(skinnedNormal, normalize(vertex2Camera));
    float degenerateFactor = saturate((dot(skinnedNormal, normalize(vertex2Camera))-0.1)*10);
    float facingCamera = saturate(dot(skinnedNormal, -VSC_CameraDirectionInvDepth.xyz) * 1.5f);
    
    // Omit faces with less than 2.56 degree
    //if (facingCamera > 0.001f)
    {
      // Modify the alpha
      vAmbient.w *= facingCamera*degenerateFactor;
    }
  }
}

void VFTree(in ShaderIn si, in TransformOutput to, inout AccomLights al, in int mainLight, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  float3 upLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal, upLocal);

  // Set the halfway vector
  so.texCoord3.xyz = halfwayLocal;

  // Perform the tree shader
  TreeFunction(to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2,
    lightLocal, to.position, to.skinnedNormal, to.skinnedPosition,
    al.specular, al.ambient, true);

  // Store the up local
  so.texCoord4.xyz = upLocal;

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 3;
}

void VFTreeNoFade(in ShaderIn si, in TransformOutput to, inout AccomLights al, in int mainLight, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  float3 upLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal, upLocal);

  // Set the halfway vector
  so.texCoord3.xyz = halfwayLocal;

  // Perform the tree shader
  TreeFunction(to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2,
    lightLocal, to.position, to.skinnedNormal, to.skinnedPosition,
    al.specular, al.ambient, false);

  // Store the up local
  so.texCoord4.xyz = upLocal;

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 3;
}

void TreePRTFunction(
  float3 skinnedMatrix0,
  float3 skinnedMatrix1,
  float3 skinnedMatrix2,
  in float3 skinnedNormal,
  in float4 skinnedPosition,
  inout float4 vAmbient,
  bool fadeFace)
{
  // calculate interestion of direction light ray with tree crown sphere
  // VSC_LDirectionTransformedDir - model or world space light direction
  
  // convert light into model space
  float3 lightVertex = normalize(
    skinnedMatrix0 * VSC_LDirectionTransformedDir.x +
    skinnedMatrix1 * VSC_LDirectionTransformedDir.y +
    skinnedMatrix2 * VSC_LDirectionTransformedDir.z
  );

  // disappear based on vertex normal
  if (fadeFace)
  {
    // Get the vertex to camera vector
    float3 vertex2Camera = VSC_CameraPosition.xyz - skinnedPosition.xyz;
    
    // Calculate the dot product
    //float facingCamera = dot(skinnedNormal, normalize(vertex2Camera));
    float degenerateFactor = saturate((dot(skinnedNormal, normalize(vertex2Camera))-0.1)*10);
    float facingCamera = saturate(dot(skinnedNormal, -VSC_CameraDirectionInvDepth.xyz) * 1.5f);
    
    // Omit faces with less than 2.56 degree
    //if (facingCamera > 0.001f)
    {
      // Modify the alpha
      vAmbient.w *= facingCamera*degenerateFactor;
    }
  }
}

// Determine the polygon orientation and calculate the PRT map offset according to it
float2 PRTMapOffset(in bool normalAligned)
{
  if (normalAligned)
  {
    return float2(0, 0.5);
  }
  else
  {
    return float2(0, 0);
  }
}

void VFTreePRT(in ShaderIn si, in TransformOutput to, inout AccomLights al, in int mainLight, inout ShaderOut so)
{
  // Perform the tree shader
  TreePRTFunction(to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2,
    to.skinnedNormal, to.skinnedPosition,
    al.ambient, true);

  #ifndef _XBOX
  // Set the PRT map offset
  so.texCoord4.xy = PRTMapOffset(to.normalAligned).xy;
  #endif

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 3;
}

void VFTreePRTNoFade(in ShaderIn si, in TransformOutput to, inout AccomLights al, in int mainLight, inout ShaderOut so)
{
  // Perform the tree shader
  TreePRTFunction(to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2,
    to.skinnedNormal, to.skinnedPosition,
    al.ambient, false);

  #ifndef _XBOX
  // Set the PRT map offset
  so.texCoord4.xy = PRTMapOffset(to.normalAligned).xy;
  #endif

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 3;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// ADVANCED TREE
////////////////////////////////////////////////////////////////////////////////////////////////////

// FREE varyings:
//so.texCoord1.zw; // TEXCOORD1
//so.texCoord2.xyzw; // TEXCOORD2
//so.texCoord3.w; // TEXCOORD3
//so.texCoord4.w; // TEXCOORD4

////////////////////////////////////////////////////////////////////////////////////////////////////
void VFTreeAdv(in ShaderIn si, in TransformOutput to, inout AccomLights al, in int mainLight, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  float3 upLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal, upLocal);

  // Set the halfway vector
  so.texCoord3.xyz = halfwayLocal;

  // Perform the tree shader
  TreeFunction(to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2,
    lightLocal, to.position, to.skinnedNormal, to.skinnedPosition,
    al.specular, al.ambient, true);

  // Store the up local
  so.texCoord4.xyz = upLocal;

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 3;
  
  /*so.texCoord2.xyzw = half4( 1.0, 0.5, 1.0, 1.0 );
  so.texCoord1.zw = half2( 1.0, 0.0 );
  so.texCoord3.w = 0.5;*/
  
  half3 dirO = to.position.xyz - half3( 0.0, 0.0, 0.0 );
  dirO = normalize( dirO );
  
  //half3 dir = mul( dirO, (float3x3)VSC_ViewMatrix );
  //half3 dir = mul( (float3x3)VSC_ViewMatrix, dirO );
  half3 crownNorm;
  crownNorm.x = dot( to.skinnedMatrix0.xyz, dirO );
  crownNorm.y = dot( to.skinnedMatrix1.xyz, dirO );
  crownNorm.z = dot( to.skinnedMatrix2.xyz, dirO );
  
  half crownDot = dot( -VSC_LDirectionTransformedDir.xyz, crownNorm.xyz );
  
  //half crownTermA = saturate( 2.0 * crownDot + 1.0 );
  half crownTermA = saturate( VSC_TreeAdvPars.x * crownDot + VSC_TreeAdvPars.y );
  //half crownTermB = saturate( 10.0 * crownDot + 5.0 );
  
  //so.texCoord2.xyz = normalize( crownNorm );
  so.texCoord2.xyz = normalize( eyeLocal );
  
  so.texCoord1.z = crownTermA;
  //so.texCoord1.w = crownTermB;
  
//#ifdef NEW_TREE_NOISE
  al.ambient.a *= 2.0;
//#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void VFTreeAdvTrunk(in ShaderIn si, in TransformOutput to, inout AccomLights al, in int mainLight, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  float3 upLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal, upLocal);

  // Set the halfway vector
  so.texCoord3.xyz = halfwayLocal;

  // Perform the tree shader
  TreeFunction(to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2,
    lightLocal, to.position, to.skinnedNormal, to.skinnedPosition,
    al.specular, al.ambient, false);

  // Store the up local
  so.texCoord4.xyz = upLocal;

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 3;
 
  half3 dirO = to.position.xyz - half3( 0.0, 0.0, 0.0 );
  dirO = normalize( dirO );
  
  half3 crownNorm;
  crownNorm.x = dot( to.skinnedMatrix0.xyz, dirO );
  crownNorm.y = dot( to.skinnedMatrix1.xyz, dirO );
  crownNorm.z = dot( to.skinnedMatrix2.xyz, dirO );
  
  half crownDot = dot( -VSC_LDirectionTransformedDir.xyz, crownNorm.xyz );
  half crownTermA = saturate( VSC_TreeAdvPars.x * crownDot + VSC_TreeAdvPars.y );
  so.texCoord2.xyz = normalize( eyeLocal );
  so.texCoord1.z = crownTermA;
  al.ambient.a *= 2.0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/*void VFTreeAdvSimple(in ShaderIn si, in TransformOutput to, inout AccomLights al, in int mainLight, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  float3 upLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal, upLocal);

  // Set the halfway vector
  so.texCoord3.xyz = halfwayLocal;

  // Perform the tree shader
  TreeFunction(to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2,
    lightLocal, to.position, to.skinnedNormal, to.skinnedPosition,
    al.specular, al.ambient, true);

  // Store the up local
  so.texCoord4.xyz = upLocal;

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 3;
  
//#ifdef NEW_TREE_NOISE
  al.ambient.a *= 2.0;
//#endif
}*/

//////////////////////////////////////////////////////////////////

void VFBasicAS( in ShaderIn si, in TransformOutput to, inout AccomLights al,
                in int mainLight, inout ShaderOut so)
{
  // Use some type of the main light (if not VML_None)
  if (mainLight == VML_Sun)
  {
    VLDirection(to, al, so.texCoord1, so.texCoord3);
  }
  else if (mainLight == VML_Sky)
  {
    VLDirectionSky(to, al);
  }
  else if (mainLight == VML_SetColor)
  {
    VLDirectionSetColor(al);
  }

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 4;
}

//////////////////////////////////////////////////////////////////

void VFNormalMapAS( in ShaderIn si, in TransformOutput to, inout AccomLights al,
                    in int mainLight, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  float3 upLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal, upLocal);

  // Calculate the ligth local
  so.color0.xyz = lightLocal;

  al.specular.xyz = lightLocal;

  // Calculate the halfway local
  so.texCoord3.xyz = halfwayLocal;

  // Store the up local
  so.texCoord4.xyz = upLocal;

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 6;
}

//////////////////////////////////////////////////////////////////

void VFNormalMapDiffuseAS(in ShaderIn si, in TransformOutput to, inout AccomLights al,
                          in int mainLight, inout ShaderOut so)
{
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  float3 upLocal;
  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal, upLocal);

  // Calculate the ligth local
  so.color0.xyz = lightLocal;

  // Calculate the specular color
  float3 H = normalize(normalize(VSC_CameraPosition.xyz - to.skinnedPosition.xyz) - VSC_LDirectionTransformedDir.xyz);
  float4 lightingVector = lit(dot(to.skinnedNormal, -VSC_LDirectionTransformedDir.xyz), dot(to.skinnedNormal, H), VSC_SpecularPower_Alpha_FogEnd_RCPFogEndMinusFogStart.x);
  al.specular.xyz += lightingVector.z * VSC_LDirectionS;

  // Store the up local
  so.texCoord4.xyz = upLocal;

  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 5;
}

//////////////////////////////////////////////////////////////////

void VFGlass( in ShaderIn si, in TransformOutput to, inout AccomLights al,
              in int mainLight, inout ShaderOut so)
{
  // Use some type of the main light (if not VML_None)
  if (mainLight == VML_Sun)
  {
    VLDirection(to, al, so.texCoord1, so.texCoord3);
  }
  else if (mainLight == VML_Sky)
  {
    VLDirectionSky(to, al);
  }
  else if (mainLight == VML_SetColor)
  {
    VLDirectionSetColor(al);
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
void VFSuper( in ShaderIn si, in TransformOutput to, inout AccomLights al, in int mainLight, inout ShaderOut so)
{
	float3x3 tspace = GetModelSTN(si.s, si.t, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2);
	PrepareTSpace2LWorldMatrix(tspace, to.skinnedPosition, so.color0, so.texCoord3, so.texCoord4);

  al.specular.xyz = to.skinnedNormal.xyz;

  so.maxTexCoordCount = 6;
}


void VFSkin( in ShaderIn si, in TransformOutput to, inout AccomLights al, in int mainLight, inout ShaderOut so)
{
	float3x3 tspace = GetModelSTN(si.s, si.t, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2);
	PrepareTSpace2LWorldMatrix(tspace, to.skinnedPosition, so.color0, so.texCoord3, so.texCoord4);

  so.maxTexCoordCount = 6;
}

//////////////////////////////////////////////////////////////////

void VFMulti( in ShaderIn si, in TransformOutput to, inout AccomLights al,
              in int mainLight, inout ShaderOut so)
{
/*
  // Light and Halfway calculation
  float3 lightLocal;
  float3 halfwayLocal;
  float3 eyeLocal;
  float3 halfwayReflectLocal;
  float3 upLocal;
//  CalculateLocalLightAndHalfway(si.s, si.t, to.skinnedPosition, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2, lightLocal, halfwayLocal, eyeLocal, halfwayReflectLocal, upLocal);

//-----
  // Prepare the STN space matrix
  float3x3 tSpace = GetModelSTN(si.s, si.t, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2);

  lightLocal = mul(tSpace, -VSC_LDirectionTransformedDir.xyz);
	
	float3 camDirection = VSC_CameraPosition.xyz - to.skinnedPosition;
	eyeLocal = mul(tSpace, camDirection );
	
	halfwayLocal = normalize(normalize(eyeLocal) + lightLocal);

  // Calculate the local halfway reflect vector
  halfwayReflectLocal = mul(tSpace, normalize(normalize(camDirection) + float3(0, 1, 0)));

  // Calculate the local UP vector
  upLocal = mul(tSpace, VSC_UDirectionTransformedDir.xyz);
	
  // Calculate the ligth local
  so.color0.xyz = lightLocal;
  so.color0.w = to.skinnedNormal.x;

  // Store the up local
  so.texCoord4.xyz = upLocal;
  so.texCoord4.w = to.skinnedNormal.y;
	
  // Calculate the halfway local
  al.specular.xyz = halfwayLocal;
	al.specular.w = sign(to.skinnedNormal.z);
*/
	//---	
	float3x3 tspace = GetModelSTN(si.s, si.t, to.skinnedNormal, to.skinnedMatrix0, to.skinnedMatrix1, to.skinnedMatrix2);
	PrepareTSpace2LWorldMatrix(tspace, to.skinnedPosition, so.color0, so.texCoord4, al.specular);
	
  // Fill out the maximum number of texture coordinates
  so.maxTexCoordCount = 8;
}

//////////////////////////////////////////////////////////////////

/*
  List of program defined macros:

    PRGDEFName,
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
  in float3 vNormal                             : NORMAL,
  in float4 vBlendWeights                       : BLENDWEIGHT,
  in float4 vBlendIndices                       : BLENDINDICES,
  in float3 vS                                  : TANGENT0,
  in float3 vT                                  : TANGENT1,
  in float3 vTexCoord0                          : TEXCOORD0,
  in float3 vTexCoord1                          : PRGDEFUVMapTex1,
  ISOLATE out float4 oProjectedPosition                 : POSITION,
#if PRGDEFRenderingMode <= RMCommon
  out float4 oColor0                            : COLOR0,
  out float4 oOutFog                            : COLOR1,
  out float4 oTexCoord[4]                       : TEXCOORD0,
  out float4 oTexCoord4                         : TEXCOORD4,
  out float4 oShadowMap                         : TEXCOORD5,
  out float4 oOutAmbient                        : TEXCOORD_AMBIENT,
  out float4 oOutSpecular                       : TEXCOORD_SPECULAR
#elif PRGDEFRenderingMode == RMShadowBuffer
  out float4 oPosition                          : TEXCOORD7,
  out float2 oTexCoord0                         : TEXCOORD0
#elif PRGDEFRenderingMode == RMZPrime || PRGDEFRenderingMode == RMDPrime
  out float4 oPosition                          : TEXCOORD7,
  out float4 oAlpha                             : TEXCOORD_AMBIENT,
  out float4 oTexCoord0                         : TEXCOORD0
#elif PRGDEFRenderingMode == RMCrater
  out float oOutFog                            : COLOR1,
  out float4 oPosition                          : TEXCOORD7,
  out float2 oTexCoord0                         : TEXCOORD0
#endif
)
{
  // Decompress compressed vectors
  DecompressVector(vS);
  DecompressVector(vT);
  DecompressVector(vNormal);
  
  // Do the vertex transformations
  TransformOutput to = VertexTransform(vPosition, vNormal, vBlendWeights, vBlendIndices, PRGDEFSkinning, PRGDEFInstancing);
  oProjectedPosition = to.projectedPosition;

  // Zero the output texcoords to make sure all are set (most of it probably will be removed by optimizer)
  float4 texCoord[4];
  for (int i = 0; i < 4; i++) texCoord[i] = float4(0, 0, 0, 0);

  #if PRGDEFRenderingMode <= RMCommon
  {
    AccomLights al;
    al.ambient = float4(0, 0, 0, 0);
    al.specular = float4(0, 0, 0, 0);
    al.diffuse = float4(0, 0, 0, 0);
    VertexInitLights(to.instanceColor.a, to.instanceShadow, al);
    ShaderOut so = VertexInitOutput();
    ShaderIn si;
    si.s = vS;
    si.t = vT;
    CAT(VF,PRGDEFName)(si, to, al, PRGDEFMainLight, so);
    oColor0 = so.color0;
    oTexCoord4 = so.texCoord4;
    texCoord[1] = so.texCoord1;
    texCoord[2] = so.texCoord2;
    texCoord[3] = so.texCoord3;
    
    // Apply fog
    float oFog;
    if (so.forceFogSimple)
    {
      VFog(to, oFog);
    }
    else
    {
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

    if ((PRGDEFMainLight == VML_None) || (PRGDEFMainLight == VML_Sun))
    {
      VLPointSpotN(to, al);
    }
    VDoneLightsSimple(al, oOutAmbient, oOutSpecular);
    oOutFog = half4(oFog, to.skinnedPosition.xyz);
    TexCoordTransform(vTexCoord0, vTexCoord1, so.maxTexCoordCount, texCoord);

    // Copy the texture coordinates to output
    for (int i = 0; i < 4; i++) oTexCoord[i] = texCoord[i];
  }
  #else
  {
    #if PRGDEFRenderingMode == RMShadowBuffer
    {
      TexCoordTransform(vTexCoord0, vTexCoord1, 1, texCoord);
      if (SBTechniqueDefault)
      {
        oPosition = to.projectedPosition;
				oPosition.z *= VSC_CameraDirectionInvDepth.w;
      }
      else
      {
        oPosition = 0;
      }

      // Copy the texture coordinates to output
      oTexCoord0.xy = texCoord[0].xy;
    }
    #elif PRGDEFRenderingMode == RMZPrime || PRGDEFRenderingMode == RMDPrime
    {
      TexCoordTransform(vTexCoord0, vTexCoord1, 2, texCoord);
      // we emulate the all computations
      AccomLights al;
      al.ambient = float4(0, 0, 0, 0);
      al.specular = float4(0, 0, 0, 0);
      al.diffuse = float4(0, 0, 0, 0);
      VertexInitLights(to.instanceColor.a, to.instanceShadow, al);
      ShaderOut so = VertexInitOutput();
      ShaderIn si;
      si.s = vS;
      si.t = vT;
      CAT(VF,PRGDEFName)(si, to, al, PRGDEFMainLight, so);
      float4 oOutAmbient;
      float4 oOutSpecular;
      float4 oOutDiffuse;
      VDoneLights(al, oOutAmbient, oOutSpecular, oOutDiffuse);
      // but we are really interested in alpha only
      oAlpha = oOutAmbient.a;

      // Copy the texture coordinates to output
      oTexCoord0.xyzw = texCoord[0].xyzw;

      // Output position, especially Z is important, out is <0..shadowZ> -> <0..1>
      oPosition    = to.transformedPosition;
			oPosition.z *= VSC_CameraDirectionInvDepth.w;
    }
    #elif PRGDEFRenderingMode == RMCrater
    {
      TexCoordTransform(vTexCoord0, vTexCoord1, 1, texCoord);
      float oFog;
      VFog(to, oFog);
      oOutFog = oFog;
      oPosition.xyz = to.position.xyz;
      oPosition.w = max(dot(to.skinnedNormal, -VSC_LDirectionTransformedDir.xyz), 0);

      // Copy the texture coordinates to output
      oTexCoord0.xy = texCoord[0].xy;
    }
    #endif
  }
  #endif
}
