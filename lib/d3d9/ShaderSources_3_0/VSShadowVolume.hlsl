#include "VS.h"

#define SHADOWVOLUME_OFFSET 0.01

void VSShadowVolume(in  float4  vPosition:          POSITION,
                    in  float3  vNormal:            NORMAL,
                    out float4  vProjectedPosition: POSITION)
{
  // Decompress compressed vectors  
  DecompressVector(vNormal);

  // Calculate new position
  float a;
  if (dot(vNormal, VSC_LDirectionTransformedDir.xyz) >= 0)
  {
    a = VSC_ShadowVolumeLength.x;
  }
  else
  {
    a = SHADOWVOLUME_OFFSET;
  }
  float4 newPosition = vPosition + float4(a, a, a, 0) * VSC_LDirectionTransformedDir;
  
  // Transform the position into view space
  float4 tPosition = float4(mul(newPosition, VSC_ViewMatrix), 1);

  // Project the position  
  vProjectedPosition = mul(VSC_ProjMatrix, tPosition);
}

void VSShadowVolumeSkinned(in  float4  vPosition:          POSITION,
                           in  float4  vBlendWeights:      BLENDWEIGHT,
                           in  float4  vBlendIndices:      BLENDINDICES,
                           in  float4  vPositionA:         POSITION1,
                           in  float4  vBlendWeightsA:     BLENDWEIGHT1,
                           in  float4  vBlendIndicesA:     BLENDINDICES1,
                           in  float4  vPositionB:         POSITION2,
                           in  float4  vBlendWeightsB:     BLENDWEIGHT2,
                           in  float4  vBlendIndicesB:     BLENDINDICES2,
                           out float4  vProjectedPosition: POSITION)
{
  float fRestBlendWeight = 1 - dot(float4(1, 1, 1, 1), vBlendWeights);
  float4 skinnedPosition =
    float4(mul(vPosition, view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndices - VSC_MatrixOffset).x]), 1) * D3DCOLORtoFLOAT4(vBlendWeights).x +
    float4(mul(vPosition, view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndices - VSC_MatrixOffset).y]), 1) * D3DCOLORtoFLOAT4(vBlendWeights).y +
    float4(mul(vPosition, view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndices - VSC_MatrixOffset).z]), 1) * D3DCOLORtoFLOAT4(vBlendWeights).z +
    float4(mul(vPosition, view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndices - VSC_MatrixOffset).w]), 1) * D3DCOLORtoFLOAT4(vBlendWeights).w +
    vPosition * fRestBlendWeight;
  float fRestBlendWeightA = 1 - dot(float4(1, 1, 1, 1), vBlendWeightsA);
  float4 skinnedPositionA =
    float4(mul(vPositionA, view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndicesA - VSC_MatrixOffset).x]), 1) * D3DCOLORtoFLOAT4(vBlendWeightsA).x +
    float4(mul(vPositionA, view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndicesA - VSC_MatrixOffset).y]), 1) * D3DCOLORtoFLOAT4(vBlendWeightsA).y +
    float4(mul(vPositionA, view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndicesA - VSC_MatrixOffset).z]), 1) * D3DCOLORtoFLOAT4(vBlendWeightsA).z +
    float4(mul(vPositionA, view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndicesA - VSC_MatrixOffset).w]), 1) * D3DCOLORtoFLOAT4(vBlendWeightsA).w +
    vPositionA * fRestBlendWeightA;
  float fRestBlendWeightB = 1 - dot(float4(1, 1, 1, 1), vBlendWeightsB);
  float4 skinnedPositionB =
    float4(mul(vPositionB, view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndicesB - VSC_MatrixOffset).x]), 1) * D3DCOLORtoFLOAT4(vBlendWeightsB).x +
    float4(mul(vPositionB, view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndicesB - VSC_MatrixOffset).y]), 1) * D3DCOLORtoFLOAT4(vBlendWeightsB).y +
    float4(mul(vPositionB, view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndicesB - VSC_MatrixOffset).z]), 1) * D3DCOLORtoFLOAT4(vBlendWeightsB).z +
    float4(mul(vPositionB, view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndicesB - VSC_MatrixOffset).w]), 1) * D3DCOLORtoFLOAT4(vBlendWeightsB).w +
    vPositionB * fRestBlendWeightB;
  float3 vectorA = skinnedPositionA.xyz - skinnedPosition.xyz;
  float3 vectorB = skinnedPositionB.xyz - skinnedPosition.xyz;
  float3 n;
  n.x = vectorA.y * vectorB.z - vectorA.z * vectorB.y;
  n.y = vectorA.z * vectorB.x - vectorA.x * vectorB.z;
  n.z = vectorA.x * vectorB.y - vectorA.y * vectorB.x;
  float a;
  if (dot(n, VSC_LDirectionTransformedDir.xyz) >= 0)
  {
    a = VSC_ShadowVolumeLength.x;
  }
  else
  {
    a = SHADOWVOLUME_OFFSET;
  }
  float4 newSkinnedPosition = skinnedPosition + float4(a, a, a, 0) * VSC_LDirectionTransformedDir;
  float4 tPosition = float4(mul(newSkinnedPosition, VSC_ViewMatrix), 1);
  vProjectedPosition = mul(VSC_ProjMatrix, tPosition);
}

void VSShadowVolumeInstanced(in  float4  vPosition:          POSITION,
                             in  float3  vNormal:            NORMAL,
                             in  float4  vBlendIndices:      BLENDINDICES,
                             out float4  vProjectedPosition: POSITION)
{
  DecompressVector(vNormal);
  float4x3 vm = view_matrix_56[D3DCOLORtoUBYTE4(vBlendIndices).w];
  float4 skinnedMatrix0 = vm._m00_m10_m20_m30;
  float4 skinnedMatrix1 = vm._m01_m11_m21_m31;
  float4 skinnedMatrix2 = vm._m02_m12_m22_m32;
  float4 skinnedPosition;
  skinnedPosition.x = dot(skinnedMatrix0, vPosition);
  skinnedPosition.y = dot(skinnedMatrix1, vPosition);
  skinnedPosition.z = dot(skinnedMatrix2, vPosition);
  skinnedPosition.w = 1;
  float3 skinnedNormal;
  skinnedNormal.x = dot(skinnedMatrix0.xyz, vNormal);
  skinnedNormal.y = dot(skinnedMatrix1.xyz, vNormal);
  skinnedNormal.z = dot(skinnedMatrix2.xyz, vNormal);
  float a;
  if (dot(skinnedNormal, VSC_LDirectionTransformedDir.xyz) >= 0)
  {
    a = VSC_ShadowVolumeLength.x;
  }
  else
  {
    a = SHADOWVOLUME_OFFSET;
  }
  float4 newSkinnedPosition = skinnedPosition + float4(a, a, a, 0) * VSC_LDirectionTransformedDir;
  float4 tPosition = float4(mul(newSkinnedPosition, VSC_ViewMatrix), 1);
  vProjectedPosition = mul(VSC_ProjMatrix, tPosition);
}

