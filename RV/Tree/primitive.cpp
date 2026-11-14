#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "primitive.h"

// Minimum angle acceptable to bend a matrix
#define BEND_MINANGLE 0.001f

namespace TreeEngine
{

  DWORD dwPrimitiveVertexDecl[] =
  {
      D3DVSD_STREAM(0),
      D3DVSD_REG(D3DVSDE_POSITION,  D3DVSDT_FLOAT3),      // position
      D3DVSD_REG(D3DVSDE_NORMAL,    D3DVSDT_FLOAT3),      // normal
      D3DVSD_REG(D3DVSDE_DIFFUSE,   D3DVSDT_D3DCOLOR),    // diffuse color
      D3DVSD_REG(D3DVSDE_TEXCOORD0, D3DVSDT_FLOAT2),      // texture coordinate
      D3DVSD_END()
  };

  Matrix3 CPrimitive::BendMatrix3DirectionByForce(Matrix3Par matrix,
                                                  Vector3Par forceDirection,
                                                  float forceIntensity) {

    Matrix3 NewMatrix;

    // Get the dot product
    float OneMinusCosAngle;
    OneMinusCosAngle = (1.0f - fabs(matrix.Direction().DotProduct(forceDirection)));

    // If there is any difference between direction and forcedirection
    if (OneMinusCosAngle > BEND_MINANGLE) {
      // Matrix for transformation from XY plane
      Matrix3 Rotation;
      Rotation.SetUpAndAside(matrix.Direction(), -forceDirection);
    
      // Matrix for transformation to XY plane
      Matrix3 InvRotation;
      InvRotation = Rotation.InverseRotation();
    
      // Rotation angle
      float Angle = OneMinusCosAngle * forceIntensity;

      // Matrix for rotation around Z axis
      Matrix3 RotateZ;
      RotateZ.SetRotationZ(Angle);

      // Create and return the new matrix
      NewMatrix = Rotation * RotateZ * InvRotation * matrix;
    }
    else {
      NewMatrix = matrix;
    }
    return NewMatrix;
  }

  Matrix3 CPrimitive::AlignMatrix3UpByForce(
                        Matrix3Par matrix,
                        Vector3Par forceDirection,
                        float forceIntensity) {
    Matrix3 NewMatrix;

    // Align force direction
    Vector3 FD;
    float CosAngle = matrix.DirectionUp().DotProduct(forceDirection);
    if (CosAngle > 0.0f) {
      FD = forceDirection;
    }
    else {
      FD = -forceDirection;
      CosAngle = -CosAngle;
    }

    // Get the dot product
    float OneMinusCosAngle;
    OneMinusCosAngle = (1.0f - CosAngle);

    // If there is any difference between direction and forcedirection
    if (OneMinusCosAngle > BEND_MINANGLE) {
      // Matrix for transformation from XY plane
      Matrix3 Rotation;
      Rotation.SetUpAndAside(matrix.DirectionUp(), -FD);
    
      // Matrix for transformation to XY plane
      Matrix3 InvRotation;
      InvRotation = Rotation.InverseRotation();
    
      // Rotation angle
      float Angle = OneMinusCosAngle * forceIntensity;

      // Matrix for rotation around Z axis
      Matrix3 RotateZ;
      RotateZ.SetRotationZ(Angle);

      // Create and return the new matrix
      NewMatrix = Rotation * RotateZ * InvRotation * matrix;
    }
    else {
      NewMatrix = matrix;
    }
    return NewMatrix;
  }

  void CPrimitive::DrawRhomb(CPrimitiveStream &PS,
                            int Stage,
                            D3DCOLOR Color,
                            Matrix4Par A,
                            float Width,
                            float Height,
                            const Array<const UVTransform> &uvTransform,                            
                            bool twosided,
                            int *pTAccu) {

    // Get the triangles count
    if (pTAccu) {
      *pTAccu += 4;
    }
    
    // Remember the first indexes
    unsigned long VIndex = PS.GetNewVertexIndex();

    // Insert rhode vertices and incices
    SPrimitiveVertex BVF;
    BVF._position = A.Position();
    BVF._normal = A.DirectionUp();
    BVF._diffuse = Color;
    BVF._u0 = uvTransform[0].GetU(0.0f,0.0f);
    BVF._v0 = uvTransform[0].GetV(0.0f,0.0f);
    PS.AddVertex(&BVF);
    PS.AddIndex(Stage, PS.GetNewVertexIndex() - 1);

    BVF._position = A.Position() + A.Direction() * Height * 0.5f - A.DirectionAside() * Width * 0.5f;
    BVF._normal = A.DirectionUp();
    BVF._diffuse = Color;
    BVF._u0 = uvTransform[0].GetU(1.0f,0.0f);
    BVF._v0 = uvTransform[0].GetV(1.0f,0.0f);
    PS.AddVertex(&BVF);
    PS.AddIndex(Stage, PS.GetNewVertexIndex() - 1);

    BVF._position = A.Position() + A.Direction() * Height;
    BVF._normal = A.DirectionUp();
    BVF._diffuse = Color;
    BVF._u0 = uvTransform[0].GetU(1.0f,1.0f);
    BVF._v0 = uvTransform[0].GetV(1.0f,1.0f);
    PS.AddVertex(&BVF);
    PS.AddIndex(Stage, PS.GetNewVertexIndex() - 1);

    PS.AddIndex(Stage, VIndex + 0);
    PS.AddIndex(Stage, VIndex + 2);

    BVF._position = A.Position() + A.Direction() * Height * 0.5f + A.DirectionAside() * Width * 0.5f;
    BVF._normal = A.DirectionUp();
    BVF._diffuse = Color;
    BVF._u0 = uvTransform[0].GetU(0.0f,1.0f);
    BVF._v0 = uvTransform[0].GetV(0.0f,1.0f);
    PS.AddVertex(&BVF);
    PS.AddIndex(Stage, PS.GetNewVertexIndex() - 1);

    if (twosided)
    {    
      // Second side
      VIndex = PS.GetNewVertexIndex();

      BVF._position = A.Position();
      BVF._normal = -A.DirectionUp();
      BVF._diffuse = Color;
      BVF._u0 = uvTransform[1].GetU(0.0f,0.0f);
      BVF._v0 = uvTransform[1].GetV(0.0f,0.0f);
      PS.AddVertex(&BVF);
      PS.AddIndex(Stage, PS.GetNewVertexIndex() - 1);

      BVF._position = A.Position() + A.Direction() * Height * 0.5f + A.DirectionAside() * Width * 0.5f;
      BVF._normal = -A.DirectionUp();
      BVF._diffuse = Color;
      BVF._u0 = uvTransform[1].GetU(0.0f,1.0f);
      BVF._v0 = uvTransform[1].GetV(0.0f,1.0f);
      PS.AddVertex(&BVF);
      PS.AddIndex(Stage, PS.GetNewVertexIndex() - 1);

      BVF._position = A.Position() + A.Direction() * Height;
      BVF._normal = -A.DirectionUp();
      BVF._diffuse = Color;
      BVF._u0 = uvTransform[1].GetU(1.0f,1.0f);
      BVF._v0 = uvTransform[1].GetV(1.0f,1.0f);
      PS.AddVertex(&BVF);
      PS.AddIndex(Stage, PS.GetNewVertexIndex() - 1);

      PS.AddIndex(Stage, VIndex + 0);
      PS.AddIndex(Stage, VIndex + 2);

      BVF._position = A.Position() + A.Direction() * Height * 0.5f - A.DirectionAside() * Width * 0.5f;
      BVF._normal = -A.DirectionUp();
      BVF._diffuse = Color;
      BVF._u0 = uvTransform[1].GetU(1.0f,0.0f);
      BVF._v0 = uvTransform[1].GetV(1.0f,0.0f);
      PS.AddVertex(&BVF);
      PS.AddIndex(Stage, PS.GetNewVertexIndex() - 1);
    }
  }

  int CPrimitive::DrawHeel(CPrimitiveStream &ps,
                           float TextureTilingCircumferenceCoef,
                           D3DCOLOR color,
                           Matrix4Par b,
                           float radiusB,
                           float textureB,
                           int sideNumB)
  {

    // Test input parameters
    if (sideNumB < 2)
    {
      LogF("Number of cone vertices is less than 2.");
      return -1;
    }

    unsigned long VBBIndex;

    // Generate vertices
    SPrimitiveVertex BV;
    int i;
    float dTextureU;
    float TextureU;
    Vector3 DirectionAsideMRadius;
    Vector3 DirectionUpMRadius;

    // B vertices

    // Delta of angle
    float dAngleB = (2.0f * H_PI) / sideNumB;

    // First angles are zero
    float AngleB = 0.0f;

    // Delta of texture coordinate
    dTextureU = 1.0f / sideNumB;

    // First texture coordinate is zero
    TextureU = 0.0f;

    // Remember index
    VBBIndex = ps.GetNewVertexIndex();

    // Generate B vertices
    DirectionAsideMRadius = b.DirectionAside() * radiusB;
    DirectionUpMRadius = b.DirectionUp() * radiusB;
    for (i = 0; i <= sideNumB; i++) {
      BV._position = b.Position() + (DirectionAsideMRadius * cos(AngleB)) + (DirectionUpMRadius * sin(AngleB));
      if (radiusB == 0.0f)
      {
        BV._normal = b.Direction();
      }
      else
      {
        BV._normal = (BV._position - b.Position()) / radiusB;
      }
      BV._diffuse = color;
      BV._u0 = TextureU * TextureTilingCircumferenceCoef;
      BV._v0 = textureB;
      ps.AddVertex(&BV);
      AngleB += dAngleB;
      TextureU += dTextureU;
    }

    return VBBIndex;
  }

  int CPrimitive::DrawCone(
        CPrimitiveStream &PS,
        int Stage,
        float TextureTilingCircumferenceCoef,
        D3DCOLOR Color,
        Matrix4Par A,
        float RadiusA,
        float TextureA,
        int SideNumA,
        Matrix4Par B,
        float RadiusB,
        float TextureB,
        int SideNumB,
        int *pTAccu)
  {

    // Test input parameters
    if (SideNumA < 2) {
      LogF("Number of A vertices is less than 2.");
      return -1;
    }
    if (SideNumB < 2) {
      LogF("Number of B vertices is less than 2.");
      return -1;
    }

    // Get the triangles count
    if (pTAccu) {
      *pTAccu += SideNumA + SideNumB;
    }

    unsigned long Index;
    unsigned long VBAIndex;
    unsigned long VBBIndex;

    // Generate vertices
    SPrimitiveVertex BV;
    int i;
    float dTextureU;
    float TextureU;
    Vector3 DirectionAsideMRadius;
    Vector3 DirectionUpMRadius;

    // A vertices

    // Delta of angle
    float dAngleA = (2.0f * H_PI) / SideNumA;

    // First angles are zero
    float AngleA = 0.0f;

    // Delta of texture coordinate
    dTextureU = 1.0f / SideNumA;

    // First texture coordinate is zero
    TextureU = 0.0f;

    // Remember index
    VBAIndex = PS.GetNewVertexIndex();

    // Generate A vertices
    DirectionAsideMRadius = A.DirectionAside() * RadiusA;
    DirectionUpMRadius = A.DirectionUp() * RadiusA;
    for (i = 0; i <= SideNumA; i++) {
      BV._position = A.Position() + (DirectionAsideMRadius * cos(AngleA)) + (DirectionUpMRadius * sin(AngleA));
      if (RadiusA == 0.0f)
      {
        BV._normal = -A.Direction();
      }
      else
      {
        BV._normal = (BV._position - A.Position()) / RadiusA;
      }
      BV._diffuse = Color;
      // FLY it is possible to move this multiplying outside the cycle
      BV._u0 = TextureU * TextureTilingCircumferenceCoef;
      BV._v0 = TextureA;
      PS.AddVertex(&BV);
      AngleA += dAngleA;
      TextureU += dTextureU;
    }

    // Generate closing triangles
    unsigned long nvi = PS.GetNewVertexIndex();
    AngleA = 0.0f;
    for (int i = 0; i <= SideNumA; i++)
    {
      BV._position = A.Position() + (DirectionAsideMRadius * cos(AngleA)) + (DirectionUpMRadius * sin(AngleA));
      BV._normal = -A.Direction();
      BV._diffuse = Color;
      BV._u0 = (cos(AngleA) + 1.0f) * 0.5f;
      BV._v0 = (sin(AngleA) + 1.0f) * 0.5f;
      PS.AddVertex(&BV);
      AngleA += dAngleA;
    }
    for (i = 1; i < (SideNumA - 1); i++) {
      PS.AddIndex(Stage, nvi + 0);
      PS.AddIndex(Stage, nvi + i + 1);
      PS.AddIndex(Stage, nvi + i);
    }

    // B vertices

    // Delta of angle
    float dAngleB = (2.0f * H_PI) / SideNumB;

    // First angles are zero
    float AngleB = 0.0f;

    // Delta of texture coordinate
    dTextureU = 1.0f / SideNumB;

    // First texture coordinate is zero
    TextureU = 0.0f;

    // Remember index
    VBBIndex = PS.GetNewVertexIndex();

    // Generate B vertices
    DirectionAsideMRadius = B.DirectionAside() * RadiusB;
    DirectionUpMRadius = B.DirectionUp() * RadiusB;
    for (i = 0; i <= SideNumB; i++) {
      BV._position = B.Position() + (DirectionAsideMRadius * cos(AngleB)) + (DirectionUpMRadius * sin(AngleB));
      if (RadiusB == 0.0f)
      {
        BV._normal = B.Direction();
      }
      else
      {
        BV._normal = (BV._position - B.Position()) / RadiusB;
      }
      BV._diffuse = Color;
      BV._u0 = TextureU * TextureTilingCircumferenceCoef;
      BV._v0 = TextureB;
      PS.AddVertex(&BV);
      AngleB += dAngleB;
      TextureU += dTextureU;
    }

  /*
    // Generate closing triangles
    unsigned long nvi = PS.GetNewVertexIndex();
    for (int i = 0; i <= SideNumA; i++)
    {
      BV._position = A.Position() + (DirectionAsideMRadius * cos(AngleA)) + (DirectionUpMRadius * sin(AngleA));
      BV._normal = (BV._position - A.Position()) * InvRadius;
      BV._diffuse = Color;
      // FLY it is possible to move this multiplying outside the cycle
      BV._u0 = TextureU * TextureTilingCircumferenceCoef;
      BV._v0 = TextureA;
      PS.AddVertex(&BV);
      AngleA += dAngleA;
      TextureU += dTextureU;


    }
  */

  /*
    for (i = 1; i < (SideNumA - 1); i++) {
      PS.AddIndex(Stage, 0);
      PS.AddIndex(Stage, i + 1);
      PS.AddIndex(Stage, i);
    }
  */

    // Generate triangles for the body

    // Initialize counters
    int Ai = 0;
    int Bi = 0;

    // Initialize angles
    AngleA = 0.0f;
    AngleB = 0.0f;

    // Load first indices
    unsigned long LastAIndex = VBAIndex;
    unsigned long LastBIndex = VBBIndex;

    // Cycle through inner triangles
    do {
      float NewAngleA = AngleA + dAngleA;
      float NewAngleB = AngleB + dAngleB;
      
      // Triangle NewAngleB, AngleB, AngleA
      if (fabs(NewAngleA - AngleB) > fabs(NewAngleB - AngleA)) {
        Index = LastBIndex + 1;
        PS.AddIndex(Stage, Index);
        PS.AddIndex(Stage, LastBIndex);
        PS.AddIndex(Stage, LastAIndex);
        LastBIndex = Index;
        AngleB += dAngleB;
        Bi++;
      }
      // Triangle NewAngleA, AngleB, AngleA
      else {
        Index = LastAIndex + 1;
        PS.AddIndex(Stage, Index);
        PS.AddIndex(Stage, LastBIndex);
        PS.AddIndex(Stage, LastAIndex);
        LastAIndex = Index;
        AngleA += dAngleA;
        Ai++;
      }
    } while ((Ai < SideNumA) || (Bi < SideNumB));

    return VBBIndex;
  }

  int CPrimitive::DrawConeShared(CPrimitiveStream &PS,
                                int Stage,
                                float TextureTilingCircumferenceCoef,
                                D3DCOLOR Color,
                                int SharedVertexIndex,
                                int SharedSidesCount,
                                bool isRoot,
                                Matrix4Par B,
                                float RadiusB,
                                float TextureB,
                                int SidesCountB,
                                int *pTAccu)
  {

    // Test input parameters
    if (SharedSidesCount < 1) {
      LogF("Number of shared sides is less than 1.");
      return -1;
    }
    if (SidesCountB < 1) {
      LogF("Number of B sides is less than 1.");
      return -1;
    }

    // Get the triangles count
    if (pTAccu) {
      *pTAccu += SharedSidesCount + SidesCountB;
    }

    unsigned long Index;
    unsigned long VBBIndex;

    // Generate vertices
    SPrimitiveVertex BV;
    int i;
    float dTextureU;
    float TextureU;
    Vector3 DirectionAsideMRadius;
    Vector3 DirectionUpMRadius;

    // B vertices

    // Delta of angle
    float dAngleB = (2.0f * H_PI) / SidesCountB;

    // First angles are zero
    float AngleB = 0.0f;

    if (isRoot)
    {
      // Delta of texture coordinate
      dTextureU = -1.0f / SidesCountB;

      // First texture coordinate is zero
      TextureU = 1.0f;
    }
    else
    {
      // Delta of texture coordinate
      dTextureU = 1.0f / SidesCountB;

      // First texture coordinate is zero
      TextureU = 0.0f;
    }

    // Remember index
    VBBIndex = PS.GetNewVertexIndex();

    // Generate B vertices
    DirectionAsideMRadius = B.DirectionAside() * RadiusB;
    DirectionUpMRadius = B.DirectionUp() * RadiusB;
    for (i = 0; i <= SidesCountB; i++) {
      BV._position = B.Position() + (DirectionAsideMRadius * cos(AngleB)) + (DirectionUpMRadius * sin(AngleB));
      if (RadiusB == 0.0f)
      {
        BV._normal = B.Direction();
      }
      else
      {
        BV._normal = (BV._position - B.Position()) / RadiusB;
      }
      BV._diffuse = Color;
      BV._u0 = TextureU * TextureTilingCircumferenceCoef;
      BV._v0 = TextureB;
      PS.AddVertex(&BV);
      AngleB += dAngleB;
      TextureU += dTextureU;
    }

    // Generate triangles

    // Delta of angle
    float dAngleA = (2.0f * H_PI) / SharedSidesCount;

    // Initialize counters
    int Ai = 0;
    int Bi = 0;

    // Initialize angles
    AngleB = 0.0f;
    float AngleA = 0.0f;

    // Load first indices
    unsigned long LastAIndex = SharedVertexIndex;
    unsigned long LastBIndex = VBBIndex;

    // Cycle through inner triangles
    do {
      float NewAngleA = AngleA + dAngleA;
      float NewAngleB = AngleB + dAngleB;
      
      // Triangle NewAngleB, AngleB, AngleA
      if (fabs(NewAngleA - AngleB) > fabs(NewAngleB - AngleA)) {
        Index = LastBIndex + 1;
        PS.AddIndex(Stage, Index);
        PS.AddIndex(Stage, LastBIndex);
        PS.AddIndex(Stage, LastAIndex);
        LastBIndex = Index;
        AngleB += dAngleB;
        Bi++;
      }
      // Triangle NewAngleA, AngleB, AngleA
      else {
        Index = LastAIndex + 1;
        PS.AddIndex(Stage, Index);
        PS.AddIndex(Stage, LastBIndex);
        PS.AddIndex(Stage, LastAIndex);
        LastAIndex = Index;
        AngleA += dAngleA;
        Ai++;
      }
    } while ((Ai < SharedSidesCount) || (Bi < SidesCountB));

    return VBBIndex;
  }

  // Generates spline until we've got sideNum number of sides
  void RefineSpline(Vector3 *&vertexPosition, Vector3 *&vertexPositionTemp, int sourceVertexCount, int desiredVertexCount)
  {
    for (int i = sourceVertexCount; i < desiredVertexCount; i++)
    {
      // Copy the first item
      vertexPositionTemp[0] = vertexPosition[0];

      // Get the half's of the next items
      for (int j = 1; j < i; j++)
      {
        vertexPositionTemp[j] = (vertexPosition[j - 1] + vertexPosition[j]) * 0.5f;
      }

      // Copy the last item
      vertexPositionTemp[i] = vertexPosition[i - 1];

      // Switch the source with the temp
      Vector3 *t = vertexPosition;
      vertexPosition = vertexPositionTemp;
      vertexPositionTemp = t;
    }
  }

  void FillOutVertexPositions(Vector3 *&vertexPosition, Vector3 *&vertexPositionTemp, int sideCount, Matrix4Par origin, float radius, float depth)
  {
    if (sideCount == 1)
    {
      vertexPosition[0] = origin.Position() + (origin.DirectionAside() * radius);
      vertexPosition[1] = origin.Position() - (origin.DirectionAside() * radius);
    }
    else if (sideCount == 2)
    {
      vertexPosition[0] = origin.Position() + (origin.DirectionAside() * radius);
      vertexPosition[1] = origin.Position() + (origin.DirectionUp() * depth);
      vertexPosition[2] = origin.Position() - (origin.DirectionAside() * radius);
    }
    else
    {
      vertexPosition[0] = origin.Position() + (origin.DirectionAside() * radius);
      vertexPosition[1] = origin.Position() + (origin.DirectionUp() * depth * 2.0f);
      vertexPosition[2] = origin.Position() - (origin.DirectionAside() * radius);
      RefineSpline(vertexPosition, vertexPositionTemp, 3, sideCount + 1);
    }
  }

  void GenerateFrontVertices(CPrimitiveStream &ps, Vector3 *vertexPosition, int sideCount, Matrix4Par origin, float textureTilingCircumferenceCoef, float textureV)
  {
    SPrimitiveVertex bv;
    for (int i = 0; i <= sideCount; i++)
    {
      bv._position = vertexPosition[i];
      int maxVertex = intMin(i + 1, sideCount);
      int minVertex = intMax(i - 1, 0);
      Vector3 newAside = (vertexPosition[maxVertex] - vertexPosition[minVertex]).Normalized();
      bv._normal = newAside.CrossProduct(origin.Direction());
      bv._diffuse = D3DCOLOR_XRGB(255, 255, 255);
      bv._u0 = ((float)i / sideCount) * textureTilingCircumferenceCoef * 0.5f;
      bv._v0 = textureV;
      ps.AddVertex(&bv);
    }
  }

  void GenerateBackVertices(CPrimitiveStream &ps, Vector3 *vertexPosition, int sideCount, Matrix4Par origin, float textureTilingCircumferenceCoef, float textureV)
  {
    SPrimitiveVertex bv;
    for (int i = sideCount; i >= 0; i--)
    {
      bv._position = vertexPosition[i];
      int maxVertex = intMin(i + 1, sideCount);
      int minVertex = intMax(i - 1, 0);
      Vector3 newAside = -(vertexPosition[maxVertex] - vertexPosition[minVertex]).Normalized();
      bv._normal = newAside.CrossProduct(origin.Direction());
      bv._diffuse = D3DCOLOR_XRGB(255, 255, 255);
      bv._u0 = 1.0f - ((float)i / sideCount) * textureTilingCircumferenceCoef * 0.5;
      bv._v0 = textureV;
      ps.AddVertex(&bv);
    }
  }

  void GenerateFaces(CPrimitiveStream &ps, int stage, unsigned long iA, int sideCountA, unsigned long iB, int sideCountB)
  {
    int aIndex = 0;
    int bIndex = 0;
    do
    {

      ps.AddIndex(stage, iB + bIndex);
      ps.AddIndex(stage, iA + aIndex);

      if (aIndex >= sideCountA)
      {
        bIndex++;
        ps.AddIndex(stage, iB + bIndex);
      }
      else if (bIndex >= sideCountB)
      {
        aIndex++;
        ps.AddIndex(stage, iA + aIndex);
      }
      else
      {
        const SPrimitiveVertex &vA = PRIMITIVE_VERTEX(ps, iA + aIndex);
        const SPrimitiveVertex &vANew = PRIMITIVE_VERTEX(ps, iA + aIndex + 1);
        const SPrimitiveVertex &vB = PRIMITIVE_VERTEX(ps, iB + bIndex);
        const SPrimitiveVertex &vBNew = PRIMITIVE_VERTEX(ps, iB + bIndex + 1);

        float aNewU = vB._u0 - vANew._u0;
        float aNewV = vB._v0 - vANew._v0;
        float bNewU = vA._u0 - vBNew._u0;
        float bNewV = vA._v0 - vBNew._v0;

        // If new B side is shorter than new A side, then consider B
        if ((aNewU * aNewU + aNewV * aNewV) > (bNewU * bNewU + bNewV * bNewV))
        {
          DoAssert(bIndex < sideCountB);
          // Consider the B vertex
          bIndex++;
          ps.AddIndex(stage, iB + bIndex);
        }
        else
        {
          // Consider the A vertex
          aIndex++;
          ps.AddIndex(stage, iA + aIndex);
        }
      }

    } while ((aIndex < sideCountA) || (bIndex < sideCountB));
  }

  #define MAX_GUTTER_TESSELATION 256

  int CPrimitive::DrawGutter(
    CPrimitiveStream &PS,
    int Stage,
    float TextureTilingCircumferenceCoef,
    Matrix4Par A,
    float RadiusA,
    float depthA,
    float TextureA,
    int SideNumA,
    Matrix4Par B,
    float RadiusB,
    float depthB,
    float TextureB,
    int SideNumB,
    int *pTAccu)
  {

    // Test input parameters
    if (SideNumA < 1)
    {
      LogF("Number of A sides is less than 1.");
      return -1;
    }
    if (SideNumA > MAX_GUTTER_TESSELATION)
    {
      LogF("Number of A sides is more than %d.", MAX_GUTTER_TESSELATION);
      return -1;
    }
    if (SideNumB < 1)
    {
      LogF("Number of B vertices is less than 1.");
      return -1;
    }
    if (SideNumB > MAX_GUTTER_TESSELATION)
    {
      LogF("Number of B vertices is more than %d.", MAX_GUTTER_TESSELATION);
      return -1;
    }

    // Get the triangles count
    if (pTAccu)
    {
      *pTAccu += (SideNumA + SideNumB) * 2;
    }

    // Generate vertices
    Vector3 vertexPosition0[MAX_GUTTER_TESSELATION];
    Vector3 vertexPosition1[MAX_GUTTER_TESSELATION];
    Vector3 *vertexPosition = vertexPosition0;
    Vector3 *vertexPositionTemp = vertexPosition1;

    // Generate vertices for A
    unsigned long VBAIndex = PS.GetNewVertexIndex();
    FillOutVertexPositions(vertexPosition, vertexPositionTemp, SideNumA, A, RadiusA, depthA);
    GenerateFrontVertices(PS, vertexPosition, SideNumA, A, TextureTilingCircumferenceCoef, TextureA);
    GenerateBackVertices(PS, vertexPosition, SideNumA, A, TextureTilingCircumferenceCoef, TextureA);

    // Generate vertices for B
    unsigned long VBBIndex = PS.GetNewVertexIndex();
    FillOutVertexPositions(vertexPosition, vertexPositionTemp, SideNumB, B, RadiusB, depthB);
    GenerateFrontVertices(PS, vertexPosition, SideNumB, B, TextureTilingCircumferenceCoef, TextureB);
    GenerateBackVertices(PS, vertexPosition, SideNumB, B, TextureTilingCircumferenceCoef, TextureB);

    // Generate front faces
    GenerateFaces(PS, Stage, VBAIndex, SideNumA, VBBIndex, SideNumB);

    // Generate back faces
    GenerateFaces(PS, Stage, VBAIndex + SideNumA + 1, SideNumA, VBBIndex + SideNumB + 1, SideNumB);

    return VBBIndex;
  }

  int CPrimitive::DrawGutterShared(
    CPrimitiveStream &ps,
    int stage,
    float TextureTilingCircumferenceCoef,
    int SharedVertexIndex,
    int SharedSideCount,
    Matrix4Par B,
    float RadiusB,
    float depthB,
    float TextureB,
    int SideNumB,
    bool extraEndPoint,
    int *pTAccu)
  {

    // Test input parameters
    if (SharedSideCount < 1)
    {
      LogF("Number of shared sides is less than 1.");
      return -1;
    }
    if (SharedSideCount > MAX_GUTTER_TESSELATION)
    {
      LogF("Number of shared vertices is more than %d.", MAX_GUTTER_TESSELATION);
      return -1;
    }
    if (SideNumB < 1)
    {
      LogF("Number of B sides is less than 1.");
      return -1;
    }
    if (SideNumB > MAX_GUTTER_TESSELATION)
    {
      LogF("Number of B sides is more than %d.", MAX_GUTTER_TESSELATION);
      return -1;
    }

    // Get the triangles count
    if (pTAccu)
    {
      *pTAccu += SharedSideCount * 2 + SideNumB * 2;
    }

    // Generate vertices
    Vector3 vertexPosition0[MAX_GUTTER_TESSELATION];
    Vector3 vertexPosition1[MAX_GUTTER_TESSELATION];
    Vector3 *vertexPosition = vertexPosition0;
    Vector3 *vertexPositionTemp = vertexPosition1;

    // Generate vertices for B
    unsigned long VBBIndex = ps.GetNewVertexIndex();
    FillOutVertexPositions(vertexPosition, vertexPositionTemp, SideNumB, B, RadiusB, depthB);
    GenerateFrontVertices(ps, vertexPosition, SideNumB, B, TextureTilingCircumferenceCoef, TextureB);
    GenerateBackVertices(ps, vertexPosition, SideNumB, B, TextureTilingCircumferenceCoef, TextureB);

    // Generate front faces
    GenerateFaces(ps, stage, SharedVertexIndex, SharedSideCount, VBBIndex, SideNumB);

    // If positions are not the same then we're connecting some different primitive and we must create the triangle
    if (PRIMITIVE_VERTEX(ps, SharedVertexIndex + SharedSideCount)._position != PRIMITIVE_VERTEX(ps, SharedVertexIndex + SharedSideCount + 1)._position)
    {
      if (pTAccu) *pTAccu++;

      ps.AddIndex(stage, VBBIndex + SideNumB);
      ps.AddIndex(stage, SharedVertexIndex + SharedSideCount);
      ps.AddIndex(stage, SharedVertexIndex + SharedSideCount + 1);
    }

    // Generate back faces
    GenerateFaces(ps, stage, SharedVertexIndex + SharedSideCount + 1, SharedSideCount, VBBIndex + SideNumB + 1, SideNumB);

    //if (PRIMITIVE_VERTEX(ps, SharedVertexIndex + SharedSideCount * 2 + 2 - 1)._position != PRIMITIVE_VERTEX(ps, SharedVertexIndex)._position)
    if (extraEndPoint)
    {
      if (pTAccu) *pTAccu++;

      // Create a triangle from the last new point and last 2 shared points
      ps.AddIndex(stage, VBBIndex + SideNumB * 2 + 2 - 1);
      ps.AddIndex(stage, SharedVertexIndex + SharedSideCount * 2 + 2 - 1);
      ps.AddIndex(stage, SharedVertexIndex + SharedSideCount * 2 + 2 - 0);
    }

    return VBBIndex;
  }

  void GenerateStarVertices(CPrimitiveStream &ps, Matrix4Par origin, float radius, float textureV, int plateNum)
  {
    SPrimitiveVertex bv;
    for (int i = 0; i < plateNum * 2; i++)
    {
      float angle = (float)i * 2.0f * H_PI / (plateNum * 2);
      float angleOrtho = angle - H_PI * 0.5f;
      bv._position = origin.Position() + origin.DirectionAside() * radius * cos(angle)  + origin.DirectionUp() * radius * sin(angle);
      bv._normal = origin.DirectionAside() * cos(angleOrtho) + origin.DirectionUp() * sin(angleOrtho);
      bv._diffuse = D3DCOLOR_XRGB(255, 255, 255);
      bv._u0 = ((plateNum & 1) || (i < plateNum)) ? i&1 : 1 - i&1;
      bv._v0 = textureV;
      ps.AddVertex(&bv);

      bv._normal = -bv._normal;
      ps.AddVertex(&bv);
    }
  }

  void GenerateStarFaces(CPrimitiveStream &ps, int stage, unsigned long aIndex, unsigned long bIndex, int plateNum)
  {
    for (int i = 0; i < plateNum; i++)
    {
      ps.AddIndex(stage, aIndex + i * 2 + 0);
      ps.AddIndex(stage, bIndex + i * 2 + 0);
      ps.AddIndex(stage, aIndex + (plateNum + i) * 2 + 1);

      ps.AddIndex(stage, bIndex + i * 2 + 0);
      ps.AddIndex(stage, bIndex + (plateNum + i) * 2 + 1);
      ps.AddIndex(stage, aIndex + (plateNum + i) * 2 + 1);

      ps.AddIndex(stage, aIndex + i * 2 + 1);
      ps.AddIndex(stage, aIndex + (plateNum + i) * 2 + 0);
      ps.AddIndex(stage, bIndex + i * 2 + 1);

      ps.AddIndex(stage, aIndex + (plateNum + i) * 2 + 0);
      ps.AddIndex(stage, bIndex + (plateNum + i) * 2 + 0);
      ps.AddIndex(stage, bIndex + i * 2 + 1);
    }
  }

  int CPrimitive::DrawStar(
    CPrimitiveStream &ps,
    int stage,
    Matrix4Par a,
    float radiusA,
    float textureA,
    Matrix4Par b,
    float radiusB,
    float textureB,
    int plateNum,
    int *pTAccu)
  {

    // Test input parameters
    if (plateNum < 1)
    {
      LogF("Number of plates is less than 1.");
      return -1;
    }

    // Get the triangles count
    if (pTAccu)
    {
      *pTAccu += plateNum * 4;
    }

    // Generate vertices
    unsigned long VBAIndex = ps.GetNewVertexIndex();
    GenerateStarVertices(ps, a, radiusA, textureA, plateNum);
    unsigned long VBBIndex = ps.GetNewVertexIndex();
    GenerateStarVertices(ps, b, radiusB, textureB, plateNum);

    // Generate faces
    GenerateStarFaces(ps, stage, VBAIndex, VBBIndex, plateNum);

    return VBBIndex;
  }

  int CPrimitive::DrawStarShared(
    CPrimitiveStream &ps,
    int stage,
    int SharedVertexIndex,
    int SharedVertexNumber,
    Matrix4Par b,
    float radiusB,
    float textureB,
    int plateNum,
    int *pTAccu)
  {

    // Test input parameters
    if (plateNum * 4 != SharedVertexNumber)
    {
      LogF("Number of shared vertices doesn't match the number of plates.");
      return -1;
    }

    // Get the triangles count
    if (pTAccu)
    {
      *pTAccu += plateNum * 4;
    }

    // Generate vertices
    unsigned long VBBIndex = ps.GetNewVertexIndex();
    GenerateStarVertices(ps, b, radiusB, textureB, plateNum);

    // Generate faces
    GenerateStarFaces(ps, stage, SharedVertexIndex, VBBIndex, plateNum);

    return VBBIndex;
  }

  void CPrimitive::DrawFunnel(
    CPrimitiveStream &ps,
    int stage,
    Matrix4Par origin,
    float radius,
    float depth,
    int sideNum,
    int *pTAccu,
    bool twotextures)
  {

    // Test input parameters
    if (sideNum < 3)
    {
      LogF("Number of sides is less than 3.");
      return;
    }

    // Get the triangles count
    if (pTAccu)
    {
      *pTAccu += sideNum * 2;
    }

    float uvmult = twotextures?1:2;
    float halfmult = 0.25f * uvmult;
    float offset = 0.5f * uvmult;

    // Generate vertices
    unsigned long vbIndex = ps.GetNewVertexIndex();

    SPrimitiveVertex bv;

    // Add the first 2 vertices - the center (front and back)
    bv._position = origin.Position();
    bv._normal = origin.Direction();
    bv._diffuse = D3DCOLOR_XRGB(255, 255, 255);
    bv._u0 = halfmult;
    bv._v0 = 0.5f;
    ps.AddVertex(&bv);
    bv._u0 += offset;
    bv._normal = -bv._normal;
    ps.AddVertex(&bv);

    // Add the circle
    Vector3 circleCenter = origin.Position() + origin.Direction() * depth;
    Vector3 vertexDir;

    // Add 2 vertices - the beginning of the circle
    bv._position = circleCenter + origin.DirectionAside() * radius * cos(0.0f)  + origin.DirectionUp() * radius * sin(0.0f);
    vertexDir = (bv._position - origin.Position()).Normalized();
    float t = origin.Direction() * vertexDir;
    bv._normal = (origin.Direction() - vertexDir * t).Normalized();
    bv._u0 = (cos(0.0f) + 1.0f) * halfmult;
    bv._v0 = (sin(0.0f) + 1.0f) * 0.5f;
    ps.AddVertex(&bv);
    bv._u0 += offset;
    bv._normal = -bv._normal;
    ps.AddVertex(&bv);

    // Add next vertex and create triangle
    for (int i = 1; i <= sideNum; i++)
    {
      // Create the 2 vertices
      float angle = (float)i * 2.0f * H_PI / sideNum;
      bv._position = circleCenter + origin.DirectionAside() * radius * cos(angle) + origin.DirectionUp() * radius * sin(angle);
      vertexDir = (bv._position - origin.Position()).Normalized();
      float t = origin.Direction() * vertexDir;
      bv._normal = (origin.Direction() - vertexDir * t).Normalized();
      bv._u0 = (cos(angle) + 1.0f) * halfmult;
      bv._v0 = (sin(angle) + 1.0f) * 0.5f;
      ps.AddVertex(&bv);
      bv._normal = -bv._normal;
      bv._u0 += offset;
      ps.AddVertex(&bv);

      // Create the 2 faces
      ps.AddIndex(stage, vbIndex + 0);
      ps.AddIndex(stage, vbIndex + 2 + (i - 1) * 2 + 0);
      ps.AddIndex(stage, vbIndex + 2 + (i - 0) * 2 + 0);
      ps.AddIndex(stage, vbIndex + 1);
      ps.AddIndex(stage, vbIndex + 2 + (i - 0) * 2 + 1);
      ps.AddIndex(stage, vbIndex + 2 + (i - 1) * 2 + 1);
    }
  }

  int CPrimitive::DrawBlock(ObjectData *o, int &bodyId, Matrix4Par origin, float width, float height)
  {
    if (height <= 0) return 0;

    // Prepare selection object
    Selection s(o);

    // Add points
    PosT *pPoint;
    pPoint = o->NewPoint();
    int pA = o->NPoints() - 1; pPoint->SetPoint(origin.Position() + origin.DirectionAside() * width + origin.DirectionUp() * width);
    pPoint = o->NewPoint();
    int pB = o->NPoints() - 1; pPoint->SetPoint(origin.Position() - origin.DirectionAside() * width + origin.DirectionUp() * width);
    pPoint = o->NewPoint();
    int pC = o->NPoints() - 1; pPoint->SetPoint(origin.Position() - origin.DirectionAside() * width - origin.DirectionUp() * width);
    pPoint = o->NewPoint();
    int pD = o->NPoints() - 1; pPoint->SetPoint(origin.Position() + origin.DirectionAside() * width - origin.DirectionUp() * width);

    pPoint = o->NewPoint();
    int pE = o->NPoints() - 1; pPoint->SetPoint(origin.Position() + origin.Direction() * height + origin.DirectionAside() * width + origin.DirectionUp() * width);
    pPoint = o->NewPoint();
    int pF = o->NPoints() - 1; pPoint->SetPoint(origin.Position() + origin.Direction() * height - origin.DirectionAside() * width + origin.DirectionUp() * width);
    pPoint = o->NewPoint();
    int pG = o->NPoints() - 1; pPoint->SetPoint(origin.Position() + origin.Direction() * height - origin.DirectionAside() * width - origin.DirectionUp() * width);
    pPoint = o->NewPoint();
    int pH = o->NPoints() - 1; pPoint->SetPoint(origin.Position() + origin.Direction() * height + origin.DirectionAside() * width - origin.DirectionUp() * width);

    // Add points to the selection
    s.PointSelect(pA); s.PointSelect(pB); s.PointSelect(pC); s.PointSelect(pD);
    s.PointSelect(pE); s.PointSelect(pF); s.PointSelect(pG); s.PointSelect(pH);
    
    // Add normal
    VecT *pNormal;
    pNormal = o->NewNormal();
    int na = o->NNormals() - 1; *pNormal = origin.DirectionAside();
    pNormal = o->NewNormal();
    int nA = o->NNormals() - 1; *pNormal = -origin.DirectionAside();
    pNormal = o->NewNormal();
    int nb = o->NNormals() - 1; *pNormal = origin.DirectionUp();
    pNormal = o->NewNormal();
    int nB = o->NNormals() - 1; *pNormal = -origin.DirectionUp();
    pNormal = o->NewNormal();
    int nc = o->NNormals() - 1; *pNormal = origin.Direction();
    pNormal = o->NewNormal();
    int nC = o->NNormals() - 1; *pNormal = -origin.Direction();

    // Add faces
    int oldFN = o->ReserveFaces(12);

    // dir
    o->Face(oldFN + 0).n = 3;
    o->Face(oldFN + 0).vs[0].point = pA;
    o->Face(oldFN + 0).vs[0].normal = nc;
    o->Face(oldFN + 0).vs[1].point = pB;
    o->Face(oldFN + 0).vs[1].normal = nc;
    o->Face(oldFN + 0).vs[2].point = pC;
    o->Face(oldFN + 0).vs[2].normal = nc;
    o->Face(oldFN + 1).n = 3;
    o->Face(oldFN + 1).vs[0].point = pA;
    o->Face(oldFN + 1).vs[0].normal = nc;
    o->Face(oldFN + 1).vs[1].point = pC;
    o->Face(oldFN + 1).vs[1].normal = nc;
    o->Face(oldFN + 1).vs[2].point = pD;
    o->Face(oldFN + 1).vs[2].normal = nc;

    // Dir
    o->Face(oldFN + 2).n = 3;
    o->Face(oldFN + 2).vs[0].point = pE;
    o->Face(oldFN + 2).vs[0].normal = nC;
    o->Face(oldFN + 2).vs[1].point = pG;
    o->Face(oldFN + 2).vs[1].normal = nC;
    o->Face(oldFN + 2).vs[2].point = pF;
    o->Face(oldFN + 2).vs[2].normal = nC;
    o->Face(oldFN + 3).n = 3;
    o->Face(oldFN + 3).vs[0].point = pE;
    o->Face(oldFN + 3).vs[0].normal = nC;
    o->Face(oldFN + 3).vs[1].point = pH;
    o->Face(oldFN + 3).vs[1].normal = nC;
    o->Face(oldFN + 3).vs[2].point = pG;
    o->Face(oldFN + 3).vs[2].normal = nC;

    // aside
    o->Face(oldFN + 4).n = 3;
    o->Face(oldFN + 4).vs[0].point = pB;
    o->Face(oldFN + 4).vs[0].normal = na;
    o->Face(oldFN + 4).vs[1].point = pF;
    o->Face(oldFN + 4).vs[1].normal = na;
    o->Face(oldFN + 4).vs[2].point = pG;
    o->Face(oldFN + 4).vs[2].normal = na;
    o->Face(oldFN + 5).n = 3;
    o->Face(oldFN + 5).vs[0].point = pB;
    o->Face(oldFN + 5).vs[0].normal = na;
    o->Face(oldFN + 5).vs[1].point = pG;
    o->Face(oldFN + 5).vs[1].normal = na;
    o->Face(oldFN + 5).vs[2].point = pC;
    o->Face(oldFN + 5).vs[2].normal = na;

    // Aside
    o->Face(oldFN + 6).n = 3;
    o->Face(oldFN + 6).vs[0].point = pA;
    o->Face(oldFN + 6).vs[0].normal = nA;
    o->Face(oldFN + 6).vs[1].point = pD;
    o->Face(oldFN + 6).vs[1].normal = nA;
    o->Face(oldFN + 6).vs[2].point = pH;
    o->Face(oldFN + 6).vs[2].normal = nA;
    o->Face(oldFN + 7).n = 3;
    o->Face(oldFN + 7).vs[0].point = pA;
    o->Face(oldFN + 7).vs[0].normal = nA;
    o->Face(oldFN + 7).vs[1].point = pH;
    o->Face(oldFN + 7).vs[1].normal = nA;
    o->Face(oldFN + 7).vs[2].point = pE;
    o->Face(oldFN + 7).vs[2].normal = nA;

    // up
    o->Face(oldFN + 8).n = 3;
    o->Face(oldFN + 8).vs[0].point = pD;
    o->Face(oldFN + 8).vs[0].normal = nb;
    o->Face(oldFN + 8).vs[1].point = pC;
    o->Face(oldFN + 8).vs[1].normal = nb;
    o->Face(oldFN + 8).vs[2].point = pG;
    o->Face(oldFN + 8).vs[2].normal = nb;
    o->Face(oldFN + 9).n = 3;
    o->Face(oldFN + 9).vs[0].point = pD;
    o->Face(oldFN + 9).vs[0].normal = nb;
    o->Face(oldFN + 9).vs[1].point = pG;
    o->Face(oldFN + 9).vs[1].normal = nb;
    o->Face(oldFN + 9).vs[2].point = pH;
    o->Face(oldFN + 9).vs[2].normal = nb;

    // Up
    o->Face(oldFN + 10).n = 3;
    o->Face(oldFN + 10).vs[0].point = pA;
    o->Face(oldFN + 10).vs[0].normal = nB;
    o->Face(oldFN + 10).vs[1].point = pE;
    o->Face(oldFN + 10).vs[1].normal = nB;
    o->Face(oldFN + 10).vs[2].point = pF;
    o->Face(oldFN + 10).vs[2].normal = nB;
    o->Face(oldFN + 11).n = 3;
    o->Face(oldFN + 11).vs[0].point = pA;
    o->Face(oldFN + 11).vs[0].normal = nB;
    o->Face(oldFN + 11).vs[1].point = pF;
    o->Face(oldFN + 11).vs[1].normal = nB;
    o->Face(oldFN + 11).vs[2].point = pB;
    o->Face(oldFN + 11).vs[2].normal = nB;

    // Add faces to the selection
    s.FaceSelect(oldFN + 0); s.FaceSelect(oldFN + 1); s.FaceSelect(oldFN + 2); s.FaceSelect(oldFN + 3);
    s.FaceSelect(oldFN + 4); s.FaceSelect(oldFN + 5); s.FaceSelect(oldFN + 6); s.FaceSelect(oldFN + 7);
    s.FaceSelect(oldFN + 8); s.FaceSelect(oldFN + 9); s.FaceSelect(oldFN + 10); s.FaceSelect(oldFN + 11);

    // Add sharp edges
    o->AddSharpEdge(pA, pB);
    o->AddSharpEdge(pB, pC);
    o->AddSharpEdge(pC, pD);
    o->AddSharpEdge(pD, pA);
    o->AddSharpEdge(pE, pF);
    o->AddSharpEdge(pF, pG);
    o->AddSharpEdge(pG, pH);
    o->AddSharpEdge(pH, pE);
    o->AddSharpEdge(pA, pE);
    o->AddSharpEdge(pB, pF);
    o->AddSharpEdge(pC, pG);
    o->AddSharpEdge(pD, pH);

    // Add named selection to the object
    char sName[16];
    sprintf(sName, "Component%02d", bodyId++);
    o->SaveNamedSel(sName, &s);

    return 12;
  }

  int CPrimitive::DrawSphere(ObjectData *o, int &bodyId, Matrix4Par origin, float sX, float sY, float sZ, int nParallels, int nMeridians)
  {
    // Return immediately in case the number of parallels/meridians doesn't form a volume
    if ((nParallels < 3) || (nMeridians < 3)) return 0;

    // Prepare selection object
    Selection s(o);

    // Add points and normals
    int iPoint = o->NPoints();
    int iNormal = o->NNormals();
    for (int j = 0; j < nParallels; j++)
      for (int i = 0; i < nMeridians + 1; i++)
      {
        // Get the Z and width
        float pAngle = ((float)j / (nParallels - 1)) * H_PI;
        float z = cos(pAngle);
        float w = sin(pAngle);

        // Get the X and Y
        float mAngle = ((float)i / nMeridians) * H_PI * 2.0f;
        float x = cos(mAngle);
        float y = sin(mAngle);

        // Add the point
        Vector3 p = origin.Position() + origin.DirectionAside() * x * sX * w + origin.DirectionUp() * y * sY * w + origin.Direction() * z * sZ;
        PosT *pPoint = o->NewPoint();
        pPoint->SetPoint(p);

        // Add point to the selection
        s.PointSelect(o->NPoints() - 1);

        // Add the normal
        Vector3 n = (origin.Position() - p).Normalized();
        VecT *pNormal = o->NewNormal();
        *pNormal = n;
      }
    
    // Add faces
    int oldFN = o->ReserveFaces(nMeridians * (nParallels - 1));
    for (int j = 0; j < nParallels - 1; j++)
      for (int i = 0; i < nMeridians; i++)
      {
        int fId = j * nMeridians + i;
        o->Face(oldFN + fId).n = 4;
        o->Face(oldFN + fId).vs[0].point = iPoint + j * (nMeridians + 1) + i;
        o->Face(oldFN + fId).vs[0].normal = iNormal + j * (nMeridians + 1) + i;

        o->Face(oldFN + fId).vs[1].point = iPoint + j * (nMeridians + 1) + i + 1;
        o->Face(oldFN + fId).vs[1].normal = iNormal + j * (nMeridians + 1) + i + 1;

        o->Face(oldFN + fId).vs[2].point = iPoint + (j + 1) * (nMeridians + 1) + i + 1;
        o->Face(oldFN + fId).vs[2].normal = iNormal + (j + 1) * (nMeridians + 1) + i + 1;

        o->Face(oldFN + fId).vs[3].point = iPoint + (j + 1) * (nMeridians + 1 ) + i;
        o->Face(oldFN + fId).vs[3].normal = iNormal + (j + 1) * (nMeridians + 1) + i;

        // Add face to the selection
        s.FaceSelect(oldFN + fId);
      }

    // Add named selection to the object
    char sName[16];
    sprintf(sName, "Component%02d", bodyId++);
    o->SaveNamedSel(sName, &s);

    return nMeridians * (nParallels - 1) * 2;
  }

  int CPrimitive::GetSphereTriangles(int nParallels, int nMeridians)
  {
    return nMeridians * (nParallels - 1) * 2;
  }

  void CPrimitive::SetOrigin(Matrix4Par origin) {
    _origin = origin;
  }

  void CPrimitive::ApplyForceToX(Vector3Par direction, float intensity) {
    saturate(intensity, 0.0f, 1.0f);
    Vector3 matVector = _origin.Orientation().DirectionAside();
    float dp = direction.DotProduct(matVector);
    saturate(dp, -1.0f, 1.0f);
    float angle = acos(dp) * intensity;
    Matrix3 matRot;
    matRot.SetRotationAxis(direction.CrossProduct(matVector), angle);
    _origin.SetOrientation(matRot.InverseRotation() * _origin.Orientation());
  }

  void CPrimitive::ApplyForceToY(Vector3Par direction, float intensity) {
    saturate(intensity, 0.0f, 1.0f);
    Vector3 matVector = _origin.Orientation().DirectionUp();
    float dp = direction.DotProduct(matVector);
    saturate(dp, -1.0f, 1.0f);
    float angle = acos(dp) * intensity;
    Matrix3 matRot;
    matRot.SetRotationAxis(direction.CrossProduct(matVector), angle);
    _origin.SetOrientation(matRot.InverseRotation() * _origin.Orientation());
  }

  void CPrimitive::ApplyForceToZ(Vector3Par direction, float intensity) {
    saturate(intensity, 0.0f, 1.0f);
    Vector3 matVector = _origin.Orientation().Direction();
    float dp = direction.DotProduct(matVector);
    saturate(dp, -1.0f, 1.0f);
    float angle = acos(dp) * intensity;
    Matrix3 matRot;
    matRot.SetRotationAxis(direction.CrossProduct(matVector), angle);
    _origin.SetOrientation(matRot.InverseRotation() * _origin.Orientation());
  }

  void CPrimitive::ApplyGravitropism(float intensity) {
    _origin.SetOrientation(BendMatrix3DirectionByForce(_origin.Orientation(), Vector3(0.0f, 1.0f, 0.0f), intensity));
  }

  void CPrimitive::ApplyGeotropism(float intensity) {
    _origin.SetOrientation(AlignMatrix3UpByForce(_origin.Orientation(), Vector3(0.0f, 1.0f, 0.0f), intensity));
  }

  void CPrimitive::ApplyDiatropism(Vector3Par diaOrigin, float intensity) {
    Vector3 ForceDirection = _origin.Position() - diaOrigin;
    if (ForceDirection != Vector3(0, 0, 0)) {
      _origin.SetOrientation(AlignMatrix3UpByForce(_origin.Orientation(), ForceDirection.Normalized(), intensity));
    }
  }

  void CPrimitive::ApplyDiatropismX(Vector3Par diaOrigin, float intensity) {
    Vector3 direction = _origin.Position() - diaOrigin;
    if (direction != Vector3(0, 0, 0)) {
      saturate(intensity, 0.0f, 1.0f);
      direction.Normalize();
      Vector3 matVector = _origin.Orientation().DirectionAside();
      float dp = direction.DotProduct(matVector);
      saturate(dp, -1.0f, 1.0f);
      float angle = acos(dp) * intensity;
      Matrix3 matRot;
      matRot.SetRotationAxis(direction.CrossProduct(matVector).Normalized(), angle);
      _origin.SetOrientation(matRot.InverseRotation() * _origin.Orientation());
    }
  }

  void CPrimitive::ApplyDiatropismY(Vector3Par diaOrigin, float intensity) {
    Vector3 direction = _origin.Position() - diaOrigin;
    if (direction != Vector3(0, 0, 0)) {
      saturate(intensity, 0.0f, 1.0f);
      direction.Normalize();
      Vector3 matVector = _origin.Orientation().DirectionUp();
      float dp = direction.DotProduct(matVector);
      saturate(dp, -1.0f, 1.0f);
      float angle = acos(dp) * intensity;
      Matrix3 matRot;
      matRot.SetRotationAxis(direction.CrossProduct(matVector).Normalized(), angle);
      _origin.SetOrientation(matRot.InverseRotation() * _origin.Orientation());
    }
  }

  void CPrimitive::ApplyDiatropismZ(Vector3Par diaOrigin, float intensity) {
    Vector3 direction = _origin.Position() - diaOrigin;
    if (direction != Vector3(0, 0, 0)) {
      saturate(intensity, 0.0f, 1.0f);
      direction.Normalize();
      Vector3 matVector = _origin.Orientation().Direction();
      float dp = direction.DotProduct(matVector);
      saturate(dp, -1.0f, 1.0f);
      float angle = acos(dp) * intensity;
      Matrix3 matRot;
      matRot.SetRotationAxis(direction.CrossProduct(matVector).Normalized(), angle);
      _origin.SetOrientation(matRot.InverseRotation() * _origin.Orientation());
    }
  }

  void CPrimitive::ApplyBending(float twistAngle, float bendAngle) {
    // Get rotation of Aside (according to Z)
    float rotAside = sin(twistAngle - H_PI * 0.5f) * bendAngle;
    // Get rotation of Up (according to Z)
    float rotUp = sin(twistAngle) * bendAngle;
    // Create matrix
    Matrix3 matrix;
    matrix.SetUpAndAside(Vector3(0, cos(rotUp), sin(rotUp)), Vector3(cos(rotAside), 0, sin(rotAside)));
    // Bend matrix
    _origin.SetOrientation(_origin.Orientation() * matrix);
  }

  void CPrimitive::ApplyRotationX(float angle) {
    Matrix4 mat;
    mat.SetRotationX(angle);
    _origin = _origin * mat;
  }

  void CPrimitive::ApplyRotationY(float angle) {
    Matrix4 mat;
    mat.SetRotationY(angle);
    _origin = _origin * mat;
  }

  void CPrimitive::ApplyRotationZ(float angle) {
    Matrix4 mat;
    mat.SetRotationZ(angle);
    _origin = _origin * mat;
  }

  void CPrimitive::ApplyShift(Vector3Par shiftVector) {
    _origin.SetPosition(_origin.Position()
      + _origin.DirectionAside() * shiftVector.X()
      + _origin.DirectionUp() * shiftVector.Y()
      + _origin.Direction() * shiftVector.Z());
  }

  void CPrimitive::SetPolyplaneType(int polyplaneType) {
    _polyplaneType = polyplaneType;
  }

  int CPrimitive::GetPolyplaneType() {
    return _polyplaneType;
  }

  // ------------------------------------------
  // Following code is here for script purposes

  DEFINE_FAST_ALLOCATOR(GameDataPrimitive)

  RString GameDataPrimitive::GetText() const
  {
	  if( _value==NULL ) return "<NULL-object>";
	  return "Primitive";
  }

  bool GameDataPrimitive::IsEqualTo(const GameData *data) const
  {
	  const GamePrimitiveType val1 = GetPrimitive();
	  const GamePrimitiveType val2 = static_cast<const GameDataPrimitive *>(data)->GetPrimitive();
	  return val1==val2;
  }

  #ifndef CHECK
  #define CHECK(command) {LSError err = command; if (err != 0) return err;}
  #endif

  LSError GameDataPrimitive::Serialize(ParamArchive &ar)
  {
	  CHECK(base::Serialize(ar));
	  return (LSError)0;
  }

  GameData *CreateGameDataPrimitive() {return new GameDataPrimitive();}

  CPrimitive *GetPrimitive(GameValuePar oper) {
	  if (oper.GetType() == GamePrimitive)
	  {
		  return static_cast<GameDataPrimitive *>(oper.GetData())->GetPrimitive();
	  }
    return NULL;
  }

  GameValue Primitive_SetOrigin(const GameState *state,
                                GameValuePar oper1,
                                GameValuePar oper2) {

    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

    if (!IsTypeMatrix4(oper2)) {
		  if (state) state->SetError(EvalGen);
		  return GameValue();
    }

    primitive->SetOrigin(ConvertGameValueToMatrix4(oper2));

    return GameValue();
  }

  GameValue Primitive_ApplyForceToX(const GameState *state,
                                    GameValuePar oper1,
                                    GameValuePar oper2) {

    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

	  const GameArrayType &array = oper2;
	  if (array.Size() != 2)
    {
		  if (state) state->SetError(EvalDim, array.Size(), 2);
		  return 0.0f;
    }
    if (!IsTypeVector3(array[0])) {
		  if (state) state->SetError(EvalGen);
		  return GameValue();
    }
	  if (array[1].GetType() != GameScalar)
    {
		  if (state) state->TypeError(GameScalar, array[1].GetType());
		  return 0.0f;
    }

    primitive->ApplyForceToX(ConvertGameValueToVector3(array[0]), array[1]);

    return GameValue();
  }

  GameValue Primitive_ApplyForceToY(const GameState *state,
                                    GameValuePar oper1,
                                    GameValuePar oper2) {

    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

	  const GameArrayType &array = oper2;
	  if (array.Size() != 2)
    {
		  if (state) state->SetError(EvalDim, array.Size(), 2);
		  return 0.0f;
    }
    if (!IsTypeVector3(array[0])) {
		  if (state) state->SetError(EvalGen);
		  return GameValue();
    }
	  if (array[1].GetType() != GameScalar)
    {
		  if (state) state->TypeError(GameScalar, array[1].GetType());
		  return 0.0f;
    }

    primitive->ApplyForceToY(ConvertGameValueToVector3(array[0]), array[1]);

    return GameValue();
  }

  GameValue Primitive_ApplyForceToZ(const GameState *state,
                                    GameValuePar oper1,
                                    GameValuePar oper2) {

    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

	  const GameArrayType &array = oper2;
	  if (array.Size() != 2)
    {
		  if (state) state->SetError(EvalDim, array.Size(), 2);
		  return 0.0f;
    }
    if (!IsTypeVector3(array[0])) {
		  if (state) state->SetError(EvalGen);
		  return GameValue();
    }
	  if (array[1].GetType() != GameScalar)
    {
		  if (state) state->TypeError(GameScalar, array[1].GetType());
		  return 0.0f;
    }

    primitive->ApplyForceToZ(ConvertGameValueToVector3(array[0]), array[1]);

    return GameValue();
  }

  GameValue Primitive_ApplyGravitropism(const GameState *state,
                                        GameValuePar oper1,
                                        GameValuePar oper2) {

    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

    primitive->ApplyGravitropism(oper2);

    return GameValue();
  }

  GameValue Primitive_ApplyGeotropism(const GameState *state,
                                      GameValuePar oper1,
                                      GameValuePar oper2) {

    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

    primitive->ApplyGeotropism(oper2);

    return GameValue();
  }

  GameValue Primitive_ApplyDiatropism(const GameState *state,
                                      GameValuePar oper1,
                                      GameValuePar oper2) {

    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

	  const GameArrayType &array = oper2;
	  if (array.Size() != 2)
    {
		  if (state) state->SetError(EvalDim, array.Size(), 2);
		  return 0.0f;
    }
    if (!IsTypeVector3(array[0])) {
		  if (state) state->SetError(EvalGen);
		  return GameValue();
    }
	  if (array[1].GetType() != GameScalar)
    {
		  if (state) state->TypeError(GameScalar, array[1].GetType());
		  return 0.0f;
    }

    primitive->ApplyDiatropism(ConvertGameValueToVector3(array[0]), array[1]);

    return GameValue();
  }

  GameValue Primitive_ApplyDiatropismX(const GameState *state,
                                      GameValuePar oper1,
                                      GameValuePar oper2) {

    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

	  const GameArrayType &array = oper2;
	  if (array.Size() != 2)
    {
		  if (state) state->SetError(EvalDim, array.Size(), 2);
		  return 0.0f;
    }
    if (!IsTypeVector3(array[0])) {
		  if (state) state->SetError(EvalGen);
		  return GameValue();
    }
	  if (array[1].GetType() != GameScalar)
    {
		  if (state) state->TypeError(GameScalar, array[1].GetType());
		  return 0.0f;
    }

    primitive->ApplyDiatropismX(ConvertGameValueToVector3(array[0]), array[1]);

    return GameValue();
  }

  GameValue Primitive_ApplyDiatropismY(const GameState *state,
                                      GameValuePar oper1,
                                      GameValuePar oper2) {

    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

	  const GameArrayType &array = oper2;
	  if (array.Size() != 2)
    {
		  if (state) state->SetError(EvalDim, array.Size(), 2);
		  return 0.0f;
    }
    if (!IsTypeVector3(array[0])) {
		  if (state) state->SetError(EvalGen);
		  return GameValue();
    }
	  if (array[1].GetType() != GameScalar)
    {
		  if (state) state->TypeError(GameScalar, array[1].GetType());
		  return 0.0f;
    }

    primitive->ApplyDiatropismY(ConvertGameValueToVector3(array[0]), array[1]);

    return GameValue();
  }

  GameValue Primitive_ApplyDiatropismZ(const GameState *state,
                                      GameValuePar oper1,
                                      GameValuePar oper2) {

    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

	  const GameArrayType &array = oper2;
	  if (array.Size() != 2)
    {
		  if (state) state->SetError(EvalDim, array.Size(), 2);
		  return 0.0f;
    }
    if (!IsTypeVector3(array[0])) {
		  if (state) state->SetError(EvalGen);
		  return GameValue();
    }
	  if (array[1].GetType() != GameScalar)
    {
		  if (state) state->TypeError(GameScalar, array[1].GetType());
		  return 0.0f;
    }

    primitive->ApplyDiatropismZ(ConvertGameValueToVector3(array[0]), array[1]);

    return GameValue();
  }

  GameValue Primitive_ApplyBending(const GameState *state,
                                  GameValuePar oper1,
                                  GameValuePar oper2) {

    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

	  const GameArrayType &array = oper2;
	  if (array.Size() != 2)
    {
		  if (state) state->SetError(EvalDim, array.Size(), 2);
		  return 0.0f;
    }
	  if (array[0].GetType() != GameScalar)
    {
		  if (state) state->TypeError(GameScalar, array[0].GetType());
		  return 0.0f;
    }
	  if (array[1].GetType() != GameScalar)
    {
		  if (state) state->TypeError(GameScalar, array[1].GetType());
		  return 0.0f;
    }

    primitive->ApplyBending(array[0], array[1]);

    return GameValue();
  }

  GameValue Primitive_ApplyRotationX(const GameState *state,
                                    GameValuePar oper1,
                                    GameValuePar oper2) {

    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

    primitive->ApplyRotationX(oper2);

    return GameValue();
  }

  GameValue Primitive_ApplyRotationY(const GameState *state,
                                    GameValuePar oper1,
                                    GameValuePar oper2) {

    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

    primitive->ApplyRotationY(oper2);

    return GameValue();
  }

  GameValue Primitive_ApplyRotationZ(const GameState *state,
                                    GameValuePar oper1,
                                    GameValuePar oper2) {

    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

    primitive->ApplyRotationZ(oper2);

    return GameValue();
  }

  GameValue Primitive_ApplyShift(const GameState *state,
                                GameValuePar oper1,
                                GameValuePar oper2) {

    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

    if (!IsTypeVector3(oper2)) {
		  if (state) state->SetError(EvalGen);
		  return GameValue();
    }

    primitive->ApplyShift(ConvertGameValueToVector3(oper2));

    return GameValue();
  }

  GameValue Primitive_SetPolyplaneType(const GameState *state,
                                      GameValuePar oper1,
                                      GameValuePar oper2) {

    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

    primitive->SetPolyplaneType(toInt(oper2));

    return GameValue();
  }

  GameValue Primitive_SetConeDetail(const GameState *state, GameValuePar oper1, GameValuePar oper2)
  {
    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {state->SetError(EvalGen); return GameValue();}
    CPrimitiveConeType *pcone=dynamic_cast<CPrimitiveConeType *>(primitive);
    if (!pcone) {state->SetError(EvalForeignError,"Not applicable at this component"); return GameValue();}
    pcone->SetConeParams(toLargeInt(oper2));
    return GameValue();
  }

  GameValue Primitive_SetConeDetailArr(const GameState *state, GameValuePar oper1, GameValuePar oper2)
  {
    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {state->SetError(EvalGen); return GameValue();}
    CPrimitiveConeType *pcone=dynamic_cast<CPrimitiveConeType *>(primitive);
    if (!pcone) {state->SetError(EvalForeignError,"Not applicable at this component"); return GameValue();}

    const GameArrayType &values=oper2;
    if (values.Size()<3) {state->SetError(EvalType); return GameValue();}

    for (int i=0;i<values.Size();i++) if (values[i].GetType()!=GameScalar) {state->SetError(EvalType); return GameValue();}

    switch (values.Size())
    {
      case 3: pcone->SetConeParams(toLargeInt(values[0]),toLargeInt(values[1]),values[2]);break;
      case 4: pcone->SetConeParams(toLargeInt(values[0]),toLargeInt(values[1]),values[2],values[3]);break;
      case 5: pcone->SetConeParams(toLargeInt(values[0]),toLargeInt(values[1]),values[2],values[3],values[4]);break;
      default: state->SetError(EvalType); 
    }
    
    return GameValue();
  }

  GameValue Primitive_SetTextureStage(const GameState *state, GameValuePar oper1, GameValuePar oper2)
  {
    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {state->SetError(EvalGen); return GameValue();}
    CPrimitiveWithTexture *ptex=dynamic_cast<CPrimitiveWithTexture *>(primitive);
    if (!ptex) {state->SetError(EvalForeignError,"Not applicable at this component"); return GameValue();}
    ptex->SetTextureStage(toInt(oper2));
    return GameValue();
  }

  GameValue Primitive_GetTextureStage(const GameState *state, GameValuePar oper1)
  {
    CPrimitive *primitive = GetPrimitive(oper1);
    if (!primitive) {state->SetError(EvalGen); return GameValue();}
    CPrimitiveWithTexture *ptex=dynamic_cast<CPrimitiveWithTexture *>(primitive);
    if (!ptex) {state->SetError(EvalForeignError,"Not applicable at this component"); return GameValue();}
    return (float)ptex->GetTextureStage();
  }


  GameData *CreatePrimitive(ParamArchive *ar) {
      return 0;
  }

#define Category ""
  TYPES_PRIMITIVE(DEFINE_TYPE,Category)


  void CPrimitive::RegisterCommands(GameState &gState)
  {
    GameState& state = gState;
    TYPES_PRIMITIVE(REGISTER_TYPE,Category)
    gState.NewOperator(GameOperator(GameNothing, "setOrigin",         function, Primitive_SetOrigin,          GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "applyForceToX",     function, Primitive_ApplyForceToX,      GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "applyForceToY",     function, Primitive_ApplyForceToY,      GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "applyForceToZ",     function, Primitive_ApplyForceToZ,      GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "applyGravitropism", function, Primitive_ApplyGravitropism,  GamePrimitive, GameScalar TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "applyGeotropism",   function, Primitive_ApplyGeotropism,    GamePrimitive, GameScalar TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "applyDiatropism",   function, Primitive_ApplyDiatropism,    GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "applyDiatropismX",  function, Primitive_ApplyDiatropismX,   GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "applyDiatropismY",  function, Primitive_ApplyDiatropismY,   GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "applyDiatropismZ",  function, Primitive_ApplyDiatropismZ,   GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "applyBending",      function, Primitive_ApplyBending,       GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "applyRotationX",    function, Primitive_ApplyRotationX,     GamePrimitive, GameScalar TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "applyRotationY",    function, Primitive_ApplyRotationY,     GamePrimitive, GameScalar TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "applyRotationZ",    function, Primitive_ApplyRotationZ,     GamePrimitive, GameScalar TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "applyShift",        function, Primitive_ApplyShift,         GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "setPolyplaneType",  function, Primitive_SetPolyplaneType,   GamePrimitive, GameScalar TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "setConeSidesCount", function, Primitive_SetConeDetail,      GamePrimitive, GameScalar TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "setConeSidesCount", function, Primitive_SetConeDetailArr,   GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "setTextureIndex",   function, Primitive_SetTextureStage,  GamePrimitive, GameScalar TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "setStage",          function, Primitive_SetTextureStage,  GamePrimitive, GameScalar TODO_OPERATOR_DOCUMENTATION));
    gState.NewFunction(GameFunction(GameNothing, "getTextureIndex",    Primitive_GetTextureStage,  GamePrimitive TODO_FUNCTION_DOCUMENTATION));
    gState.NewFunction(GameFunction(GameNothing, "getStage",           Primitive_GetTextureStage,  GamePrimitive TODO_FUNCTION_DOCUMENTATION));

  };

  // ------------------------------------------

}