#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "Polyplane.h"
#include "component.h"
#include "TextureFactory.h"


namespace TreeEngine
{
  #define CLEAR_COLOR 0x00000000
  //#define CLEAR_NORMAL 0xFF8080FF
  #define CLEAR_NORMAL 0xFF000000

//  HWND CPolyplane::PolyplanePreviewWindow=0;

  DWORD dwPolyplaneDecl[] =
  {
      D3DVSD_STREAM(0),
      D3DVSD_REG(D3DVSDE_POSITION,  D3DVSDT_FLOAT3),      // position
      D3DVSD_REG(D3DVSDE_NORMAL,    D3DVSDT_FLOAT3),      // normal
      D3DVSD_REG(D3DVSDE_TEXCOORD0, D3DVSDT_FLOAT2),      // texture coordinate
      D3DVSD_REG(D3DVSDE_TEXCOORD1, D3DVSDT_FLOAT3),      // U
      D3DVSD_REG(D3DVSDE_TEXCOORD2, D3DVSDT_FLOAT3),      // V
      D3DVSD_END()
  };

  DWORD dwPolyplaneNormalDecl[] =
  {
      D3DVSD_STREAM(0),
      D3DVSD_REG(D3DVSDE_POSITION,  D3DVSDT_FLOAT3),      // position
      D3DVSD_REG(D3DVSDE_NORMAL,    D3DVSDT_FLOAT3),      // normal
      D3DVSD_END()
  };

  void CPolyplane::RenderPlane(      
    CPrimitiveStream &psBranch,
    CPrimitiveStream &psPolyplane,
    ComRef<IDirect3DTexture8> &colorMap,
    ComRef<IDirect3DTexture8> &normalMap,
    const CPointList &pointList,
    Matrix4Par Camera,
    float Width,
    float Height,
    const DrawParameters &dp,
    CPointList *outPolygon)
  {
    _factory->RenderPlane(psBranch,psPolyplane,colorMap,normalMap,Camera,Width,Height,_textureSize,_textureSize,dp);

    // Create the output polygon as a convex 2D hull (the hull will have coordinates in <0, 1> to
    // correspond to textures (UV's can be used directly from it, Positions must be stretched
    // and shifted)). The points must be in clockwise order in space where positive directions are
    // to right and up.
    if (outPolygon)
    {
      // Convert points to camera space into <0, 1> range in X and Y dimensions
      Matrix4 scale(MScale, 1.0f / Width, 1.0f / Height, 1.0f);
      Matrix4 translation(MTranslation, Vector3(0.5f, 0.5f, 0.0f));
      CPointList camera01PointList(pointList, translation * scale * Camera.InverseScaled());

      // Find the convex hull with the specified maximum of vertices
      outPolygon->InitWithConvexHull2D(camera01PointList, dp._polyplaneHullVertices);

//       // Initialize output with the square as a first 4-point solution
//       outPolygon->Clear();
//       outPolygon->AddPoint(Vector3(0, 0, 0));
//       outPolygon->AddPoint(Vector3(0, 1, 0));
//       outPolygon->AddPoint(Vector3(1, 1, 0));
//       outPolygon->AddPoint(Vector3(1, 0, 0));

//       // Add testing points
//       //outPolygon->AddPoint(Vector3(0.5, 1, 0));
//       outPolygon->AddPoint(Vector3(0.3, 1, 0));
//       outPolygon->AddPoint(Vector3(0.9, 1, 0));
// 
//       outPolygon->AddPoint(Vector3(1, 0.5, 0));
//       outPolygon->AddPoint(Vector3(0.5, 0, 0));
//       outPolygon->AddPoint(Vector3(0, 0.5, 0));
    }
  }

  void CPolyplane::DrawPlane(
                    int Stage,
                    CPrimitiveStream &PSTriplane,
                    ComRef<IDirect3DTexture8> &colorMap,
                    ComRef<IDirect3DTexture8> &normalMap,
                    Matrix4Par Origin,
                    float Width,
                    float Height,
                    float dirOffset,
                    const CPointList *polygon)
  {
    SPolyplaneVertex Vertex;
    Vector3 Direction = Origin.Direction() * dirOffset * Width;
    if (polygon)
    {
      // First side
      {
        unsigned long originalVertexIndex = PSTriplane.GetNewVertexIndex();
        unsigned long vertexIndexM2 = 0;
        unsigned long vertexIndexM1 = 0;
        for (int i = 0; i < polygon->NPoints(); i++)
        {
          // Get the current point
          const Vector3 &point = polygon->GetPoint(i);

          // Update the last 2 vertex indices
          vertexIndexM2 = vertexIndexM1;
          vertexIndexM1 = PSTriplane.GetNewVertexIndex();

          // Add new vertex
          Vertex.Position = Origin.Position()
            + Origin.DirectionAside() * Width * (point.X() - 0.5f)
            + Origin.DirectionUp() * Height * (point.Y() - 0.5f)
            + Direction;
          Vertex.Normal = -Origin.Direction();
          Vertex.U1 = point.X();
          Vertex.V1 = 1.0f - point.Y();
          Vertex.S = -Origin.DirectionAside();
          Vertex.T = -Origin.DirectionUp();
          PSTriplane.AddVertex(&Vertex);

          // Add face
          if (i >= 2)
          {
            PSTriplane.AddIndex(Stage, originalVertexIndex);
            PSTriplane.AddIndex(Stage, vertexIndexM2);
            PSTriplane.AddIndex(Stage, vertexIndexM1);
          }
        }
      }

      // Second side
      {
        unsigned long originalVertexIndex = PSTriplane.GetNewVertexIndex();
        unsigned long vertexIndexM2 = 0;
        unsigned long vertexIndexM1 = 0;
        for (int i = 0; i < polygon->NPoints(); i++)
        {
          // Get the current point
          const Vector3 &point = polygon->GetPoint((polygon->NPoints() - i) % polygon->NPoints());

          // Update the last 2 vertex indices
          vertexIndexM2 = vertexIndexM1;
          vertexIndexM1 = PSTriplane.GetNewVertexIndex();

          // Add new vertex
          Vertex.Position = Origin.Position()
            + Origin.DirectionAside() * Width * (point.X() - 0.5f)
            + Origin.DirectionUp() * Height * (point.Y() - 0.5f)
            + Direction;
          Vertex.Normal = Origin.Direction();
          Vertex.U1 = point.X();
          Vertex.V1 = 1.0f - point.Y();
          Vertex.S = -Origin.DirectionAside();
          Vertex.T = -Origin.DirectionUp();
          PSTriplane.AddVertex(&Vertex);

          // Add face
          if (i >= 2)
          {
            PSTriplane.AddIndex(Stage, originalVertexIndex);
            PSTriplane.AddIndex(Stage, vertexIndexM2);
            PSTriplane.AddIndex(Stage, vertexIndexM1);
          }
        }
      }
    }
    else
    {
      Vector3 HalfDirectionAside = Origin.DirectionAside() * 0.5f * Width;
      Vector3 HalfDirectionUp = Origin.DirectionUp() * 0.5f * Height;

      // First side
      unsigned long VIndex = PSTriplane.GetNewVertexIndex();

      Vertex.Position = Origin.Position() - HalfDirectionAside + HalfDirectionUp + Direction;
      Vertex.Normal = -Origin.Direction();
      Vertex.U1 = 0.0f;
      Vertex.V1 = 0.0f;
      Vertex.S = -Origin.DirectionAside();
      Vertex.T = -Origin.DirectionUp();
      PSTriplane.AddVertex(&Vertex);
      PSTriplane.AddIndex(Stage, PSTriplane.GetNewVertexIndex() - 1);

      Vertex.Position = Origin.Position() + HalfDirectionAside + HalfDirectionUp + Direction;
      Vertex.Normal = -Origin.Direction();
      Vertex.U1 = 1.0f;
      Vertex.V1 = 0.0f;
      Vertex.S = -Origin.DirectionAside();
      Vertex.T = -Origin.DirectionUp();
      PSTriplane.AddVertex(&Vertex);
      PSTriplane.AddIndex(Stage, PSTriplane.GetNewVertexIndex() - 1);

      Vertex.Position = Origin.Position() + HalfDirectionAside - HalfDirectionUp - Direction;
      Vertex.Normal = -Origin.Direction();
      Vertex.U1 = 1.0f;
      Vertex.V1 = 1.0f;
      Vertex.S = -Origin.DirectionAside();
      Vertex.T = -Origin.DirectionUp();
      PSTriplane.AddVertex(&Vertex);
      PSTriplane.AddIndex(Stage, PSTriplane.GetNewVertexIndex() - 1);

      PSTriplane.AddIndex(Stage, VIndex + 0);
      PSTriplane.AddIndex(Stage, VIndex + 2);

      Vertex.Position = Origin.Position() - HalfDirectionAside - HalfDirectionUp - Direction;
      Vertex.Normal = -Origin.Direction();
      Vertex.U1 = 0.0f;
      Vertex.V1 = 1.0f;
      Vertex.S = -Origin.DirectionAside();
      Vertex.T = -Origin.DirectionUp();
      PSTriplane.AddVertex(&Vertex);
      PSTriplane.AddIndex(Stage, PSTriplane.GetNewVertexIndex() - 1);

      // Second side
      VIndex = PSTriplane.GetNewVertexIndex();

      Vertex.Position = Origin.Position() - HalfDirectionAside - HalfDirectionUp + Direction;
      Vertex.Normal = Origin.Direction();
      Vertex.U1 = 0.0f;
      Vertex.V1 = 1.0f;
      Vertex.S = -Origin.DirectionAside();
      Vertex.T = -Origin.DirectionUp();
      PSTriplane.AddVertex(&Vertex);
      PSTriplane.AddIndex(Stage, PSTriplane.GetNewVertexIndex() - 1);

      Vertex.Position = Origin.Position() + HalfDirectionAside - HalfDirectionUp + Direction;
      Vertex.Normal = Origin.Direction();
      Vertex.U1 = 1.0f;
      Vertex.V1 = 1.0f;
      Vertex.S = -Origin.DirectionAside();
      Vertex.T = -Origin.DirectionUp();
      PSTriplane.AddVertex(&Vertex);
      PSTriplane.AddIndex(Stage, PSTriplane.GetNewVertexIndex() - 1);

      Vertex.Position = Origin.Position() + HalfDirectionAside + HalfDirectionUp - Direction;
      Vertex.Normal = Origin.Direction();
      Vertex.U1 = 1.0f;
      Vertex.V1 = 0.0f;
      Vertex.S = -Origin.DirectionAside();
      Vertex.T = -Origin.DirectionUp();
      PSTriplane.AddVertex(&Vertex);
      PSTriplane.AddIndex(Stage, PSTriplane.GetNewVertexIndex() - 1);

      PSTriplane.AddIndex(Stage, VIndex + 0);
      PSTriplane.AddIndex(Stage, VIndex + 2);

      Vertex.Position = Origin.Position() - HalfDirectionAside + HalfDirectionUp - Direction;
      Vertex.Normal = Origin.Direction();
      Vertex.U1 = 0.0f;
      Vertex.V1 = 0.0f;
      Vertex.S = -Origin.DirectionAside();
      Vertex.T = -Origin.DirectionUp();
      PSTriplane.AddVertex(&Vertex);
      PSTriplane.AddIndex(Stage, PSTriplane.GetNewVertexIndex() - 1);
    }
  }

  HRESULT SaveSurfaceRGBAToTGA(ComRef<IDirect3DSurface8> rgba32Surface, const char *filename)
  {
    HRESULT hr;
    D3DSURFACE_DESC desc;
    hr = rgba32Surface->GetDesc(&desc);
    if (FAILED(hr)) return hr;

    D3DLOCKED_RECT lr;
    hr=rgba32Surface->LockRect(&lr, NULL, D3DLOCK_READONLY);
    if (FAILED(hr)) return hr;
    PPixel *pix = (PPixel*) lr.pBits;


    // Create picture
    Picture pic;
    pic.Create(desc.Width,desc.Height);

    int pixNum = 0;
    for (unsigned int y = 0; y < desc.Width; y++) {
      for (unsigned int x = 0; x < desc.Height; x++) {
        PPixel pixel = pix[pixNum];
        pic.SetPixel(x, y, pixel);
        pixNum++;
      }
    }

    // Unlock the texture
    rgba32Surface->UnlockRect();

    // Save TGA picture
    if (pic.SaveTGA(filename)!=0)
      return MK_E_CANTOPENFILE;
    return S_OK;

  }
  

  void CPolyplane::SaveTexture(RString fileName, ComRef<IDirect3DTexture8> &texture, bool normalMap) {
    HRESULT hr;

    // Get source texture description
    D3DSURFACE_DESC desc;
    hr = texture->GetLevelDesc(0, &desc);
    assert(SUCCEEDED(hr));    

    // Create temporary texture
    ComRef<IDirect3DSurface8> tmpMapSurface;
    _factory->GetLockableLevel(texture,tmpMapSurface,0);

    SaveSurfaceRGBAToTGA(tmpMapSurface,fileName);

  }

  CPolyplane::CPolyplane():_baseStage(0) {
  }

  CPolyplane::~CPolyplane() {
    Done();
  }

  HRESULT CPolyplane::Init(ITextureFactory *factory, int textureSize) 
  {
    // Save texture factory
    _factory=factory;
    // Set importancy to default value
    _importancy = -1.0f;

    _textureSize=textureSize;
    return S_OK;
  }

  HRESULT CPolyplane::Done() {
    return S_OK;
  }


  void CPolyplane::RegisterTextureToStage(CPrimitiveStream &PSStageInit, int offset, ComRef<IDirect3DTexture8> colorMap, ComRef<IDirect3DTexture8> normalMap)
  {
    PSStageInit.SetTextureContent(_baseStage+offset,colorMap,normalMap);
  }

  /*
  Vector3 GetAverageNormal(Vector3Par point, Vector3Par originalNormal, const RepresentativeArray &ra)
  {
    Vector3 avgNormal = Vector3(0, 0, 0);
    float sumWeight = 0.0f;
    for (int i = 0; i < ra.Size(); i++)
    {
      // Get the representative position and radius
      Vector3 rPosition = ra[i].GetAvgPosition();
      float rRadius = ra[i].GetAvgCenterDistance() * 2.0f; // Multiplied by size coefficient

      // Calculate weight according to radius
      float pointDistance = point.Distance(rPosition);

      // Get the weight of the representative
      float weight;
      if (rRadius > 0.0f)
      {
        weight = max(1.0f - pointDistance / rRadius, 0.0f);
      }
      else
      {
        weight = 0.0f;
      }

      // Add value to the normal
      Vector3 rNormal = (point - rPosition).Normalized();
//       rNormal = Vector3(0, rNormal.Y(), rNormal.Z()).Normalized();
      avgNormal = avgNormal + rNormal * weight;
      sumWeight += weight;
    }
    avgNormal.Normalize();

    // If final weight is not big enough, include the original normal as well
    if (sumWeight < 1.0f)
    {
      avgNormal = avgNormal * sumWeight + originalNormal * (1.0f - sumWeight);
    }
    return avgNormal;
  }
  */

  Vector3 GetAverageNormal(Vector3Par point, Vector3Par originalNormal, const RepresentativeArray &ra) {
    

      Vector3 avgPoint(0,0,0);
      float sumWeight = 0.0f;
      for (int i = 0; i < ra.Size(); i++) {

          Vector3 pt = ra[i].GetAvgPosition();
          int cnt = ra[i]._n;
          float dist = point.Distance2(pt);

          if (dist< 0.0000000001) {
              avgPoint = pt;
              sumWeight = 1;
              break;
          }
          avgPoint += pt * (cnt / dist);
          sumWeight+=cnt / dist;
      }
      avgPoint = avgPoint / sumWeight;
      Vector3 avgNormal = point - avgPoint;
      if (avgNormal.SquareSize() < 0.0000000001) return Vector3(0,0,0);
      else return avgNormal.Normalized();

  }

  void CPolyplane::ModifyNormals(CPrimitiveStream &PSBranch, CPrimitiveStream &PSPolyplane, const CPointList &pointList, float polyplaneVolume, const DrawParameters &dp) const
  {
    // Find the representatives
    //int representatives = max(toInt(dp._clusterRepresentativesPerMeter3 * polyplaneVolume), 1);
    pointList.FindRepresentatives(1.0f/dp._clusterRepresentativesPerMeter3, _ra);

    assert(PSBranch.GetVertexSize() == sizeof(SBranchVertex));
    for (int i = 0; i < (int)PSBranch.GetNewVertexIndex(); i++)
    {
      SBranchVertex *v = (SBranchVertex*)PSBranch.GetVertexData(i);
      v->Normal = GetAverageNormal(v->Position, v->Normal, _ra) * dp._clusterWeight + v->Normal * (1.0f - dp._clusterWeight);
//       v->Normal = GetAverageNormal(v->Position, v->Normal, _ra);
      v->Normal.Normalize();
//       v->Normal = v->Position - Vector3(0, 1.5, 0);
//       v->Normal = Vector3(0, v->Normal.Y(), v->Normal.Z()).Normalized();
    }

    assert(PSPolyplane.GetVertexSize() == sizeof(SPolyplaneVertex));
    for (int i = 0; i < (int)PSPolyplane.GetNewVertexIndex(); i++)
    {
      SPolyplaneVertex *v = (SPolyplaneVertex*)PSPolyplane.GetVertexData(i);
      v->Normal = GetAverageNormal(v->Position, v->Normal, _ra) * dp._clusterWeight + v->Normal * (1.0f - dp._clusterWeight);
//       v->Normal = GetAverageNormal(v->Position, v->Normal, _ra);
      v->Normal.Normalize();
//      v->Normal = v->Position - Vector3(0, 1.5, 0);
//      v->Normal = Vector3(0, v->Normal.Y(), v->Normal.Z()).Normalized();
    }
  }

  struct UVPair
  {
    float u, v;
    UVPair(float u, float v):u(u),v(v) {}
  };

    bool SPolyplaneVertex::GenerateST(const SPolyplaneVertex &bvec, const SPolyplaneVertex &cvec)
   {
    UVPair at(U1,V1);
    UVPair bt(bvec.U1,bvec.V1);
    UVPair ct(cvec.U1,cvec.V1);

    const Vector3 &apos=Position;
    Vector3 &outS=S;
    Vector3 &outT=T;
    const Vector3 &anormal=Normal;
    const Vector3 &bpos=bvec.Position;
    const Vector3 &cpos=cvec.Position;
    

    float k = bt.u - at.u;
    float l = bt.v - at.v;
    float m = ct.u - at.u;
    float n = ct.v - at.v;
    Vector3 v1 = bpos - apos;
    Vector3 v2 = cpos - apos;
    float kn = k * n;
    float lm = l * m;
    float d = lm - kn;

    // Test the vectors to be linearly dependent
    if (fabs(d) < 1e-8)
    {
      // First try s to be (1,0,0) vector
      Vector3 s(1, 0, 0);
      float t = s * anormal;
      s = s - anormal * t;
      if (fabs(s.SquareSize()) < FLT_MIN)
      {
        s.Normalize();
      }
      else
      {
        // (1,0,0) failed, so (0,1,0) must work
        s = Vector3(0, 1, 0);
        t = s * anormal;
        s = (s - anormal * t).Normalized();
      }

      // Write out some reasonable values
      outS = s;
      outT = anormal.CrossProduct(s);

      // The vectors in the texture are linearly dependent
      return false;
    }

    // U to S
    outS = (-n * v1 * d + l * v2 * d).Normalized();

    // V to T
    outT = (m * v1 * d - k * v2 * d).Normalized();

    // Normalize the outS and outT using the anormal
    // The reference vector is the normal - the outS and outT must be orthogonal to it
    {

      // Get the middleST vector and make it orthogonal to normal
      Vector3 middleST = (outS + outT) * 0.5f;
      float t = middleST * anormal;
      middleST = middleST - anormal * t;
      if (fabs(middleST.SquareSize()) < FLT_MIN)
      {
        // The middle ST is lineary dependent on normal
        return false;
      }
      middleST.Normalize();

      // Get vector orthogonal to the middleST
      Vector3 orthoMiddleST = anormal.CrossProduct(middleST);

      if (orthoMiddleST.DotProduct(outS) > 0.0f)
      {
        // Get outS and outT from the middle of the middleST and orthoModdleST
        outS = (orthoMiddleST + middleST).Normalized();
        outT = (-orthoMiddleST + middleST).Normalized();
      }
      else
      {
        // Get outS and outT from the middle of the middleST and orthoModdleST
        outS = (-orthoMiddleST + middleST).Normalized();
        outT = (orthoMiddleST + middleST).Normalized();
      }
    }


    return true;
  }

   bool SPolyplaneVertex::CalculateNormal(const SPolyplaneVertex &bvec, const SPolyplaneVertex &cvec)
   {
    Normal=(bvec.Position-Position).CrossProduct(cvec.Position-Position);
    float sz=Normal.Size();
    if (sz<0.000001) return false;
    Normal/=sz;
    return true;
   }
}
