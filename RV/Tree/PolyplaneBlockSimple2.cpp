#include "RVTreeCommon.h"
#include "polyplaneBlockSimple2.h"


namespace TreeEngine
{

  CPolyplaneBlockSimple2::CPolyplaneBlockSimple2() {
  }

  CPolyplaneBlockSimple2::~CPolyplaneBlockSimple2() {
    Done();
  }

  HRESULT CPolyplaneBlockSimple2::Init(ITextureFactory *factory, int textureSize) 
  {
    HRESULT hr;

    // Call the parent Init method
    hr = CPolyplane::Init(factory, textureSize);
/*    if (FAILED(hr)) return hr;

    // Creating of particular color maps
    if (FAILED(hr = CreatePolyplaneTexture(pD3DDevice,textureSize, _pA.Init()))) return hr;
    if (FAILED(hr = CreatePolyplaneTexture(pD3DDevice,textureSize, _pB.Init()))) return hr;
    // Creating of particular normal maps
    if (FAILED(hr = CreatePolyplaneTexture(pD3DDevice,textureSize, _pANorm.Init()))) return hr;
    if (FAILED(hr = CreatePolyplaneTexture(pD3DDevice,textureSize, _pBNorm.Init()))) return hr;
*/
    return S_OK;
  }

  HRESULT CPolyplaneBlockSimple2::Done() {
    HRESULT hr;

    // Call the parent Done method
    hr = CPolyplane::Done();
    if (FAILED(hr)) return hr;

    // Free allocated surfaces
    _a.Free();
    _b.Free();

    return S_OK;
  }

  void CPolyplaneBlockSimple2::Render(CPrimitiveStream &psBranch,      
          CPrimitiveStream &psPolyplane,  CPrimitiveStream &psStageInit, Matrix4Par origin,  const CPointList &pointList,
          float sizeCoef,   const DrawParameters &dp)
  {
    SBoundingBox bbox;
    if (pointList.ComputeBoundingBox(origin, bbox))
    {

      // Get important coeficients
      _centre = bbox.GetFocus();
      _dimension = bbox.GetDimension();

      // Save the size coeficient
      _SizeCoef = sizeCoef;
      DoAssert(_SizeCoef > 0.0f);

      // Render planes
      Matrix4 Camera;
      Camera.SetPosition(_centre);

      // Modify geometry normals
      ModifyNormals(psBranch, psPolyplane, pointList, bbox.GetVolume(), dp);

      // Preparing of the graphics
      psBranch.Prepare();
      psPolyplane.Prepare();

      // A plane
      Camera.SetDirectionAndUp(Vector3(-1, 0, 0), Vector3(0, 0, 1));
      RenderPlane(psBranch, psPolyplane,
        _a._p, _a._pNorm, pointList, origin * Camera, _dimension.Y(), _dimension.Z(), dp, &_a._hull);

      // B plane
      Camera.SetDirectionAndUp(Vector3(0, -1, 0), Vector3(0, 0, 1));
      RenderPlane(psBranch, psPolyplane,
        _b._p, _b._pNorm, pointList, origin * Camera, _dimension.X(), _dimension.Z(), dp, &_b._hull);

      RegisterTextureToStage(psStageInit,0,_a._p,_a._pNorm);
      RegisterTextureToStage(psStageInit,1,_b._p,_b._pNorm);
    }
    else
    {
      LogF("Error while computing a bounding box of geometry.");
    }
  }

  void CPolyplaneBlockSimple2::Draw(
                                CPrimitiveStream &psTriplane,
                                Matrix4Par origin,
                                float newSizeCoef) {

    int stage=_baseStage;
    // Coeficient of the size of the drawing
    float drawSizeCoef = CalcScale(newSizeCoef);

    // Retrieving the centre of the triplane
    Matrix4 Centre;
    Centre.SetOrientation(origin.Orientation());
    Centre.SetPosition(origin * (_centre * drawSizeCoef));

    Matrix4 Orient;
    float DX = _dimension.X() * drawSizeCoef;
    float DY = _dimension.Y() * drawSizeCoef;
    float DZ = _dimension.Z() * drawSizeCoef;

    // A
    Orient.SetDirectionAndUp(-origin.DirectionAside(), origin.Direction());
    Orient.SetPosition(Centre * Vector3(-_centre.X() * drawSizeCoef, 0, 0));
    DrawPlane(stage + 0, psTriplane, _a._p, _a._pNorm, Orient, DY, DZ, 0, &_a._hull);

    // B
    Orient.SetDirectionAndUp(-origin.DirectionUp(), origin.Direction());
    Orient.SetPosition(Centre * Vector3(0, -_centre.Y() * drawSizeCoef, 0));
    DrawPlane(stage + 1, psTriplane, _b._p, _b._pNorm, Orient, DX, DZ, 0, &_b._hull);
  }

  void CPolyplaneBlockSimple2::UpdatePointList(Matrix4Par origin,
                                            float newSizeCoef,
                                            CPointList &pointList) {

    // Coeficient of the size of the drawing
    float drawSizeCoef = CalcScale(newSizeCoef);

    // Retrieving the centre of the triplane
    Matrix4 Centre;
    Centre.SetOrientation(origin.Orientation());
    Centre.SetPosition(origin * (_centre * drawSizeCoef));

    Matrix4 Orient;
    float DX = _dimension.X() * drawSizeCoef;
    float DY = _dimension.Y() * drawSizeCoef;
    float DZ = _dimension.Z() * drawSizeCoef;

    // A
    Orient.SetDirectionAndUp(-origin.DirectionAside(), origin.Direction());
    Orient.SetPosition(Centre * Vector3(-_centre.X() * drawSizeCoef, 0, 0));
    pointList.AddPoint(Orient.Position() + Orient.DirectionAside() * DY * 0.5f + Orient.DirectionUp() * DZ * 0.5f);
    pointList.AddPoint(Orient.Position() + Orient.DirectionAside() * DY * 0.5f - Orient.DirectionUp() * DZ * 0.5f);
    pointList.AddPoint(Orient.Position() - Orient.DirectionAside() * DY * 0.5f - Orient.DirectionUp() * DZ * 0.5f);
    pointList.AddPoint(Orient.Position() - Orient.DirectionAside() * DY * 0.5f + Orient.DirectionUp() * DZ * 0.5f);

    // B
    Orient.SetDirectionAndUp(-origin.DirectionUp(), origin.Direction());
    Orient.SetPosition(Centre * Vector3(0, -_centre.Y() * drawSizeCoef, 0));
    pointList.AddPoint(Orient.Position() + Orient.DirectionAside() * DX * 0.5f + Orient.DirectionUp() * DZ * 0.5f);
    pointList.AddPoint(Orient.Position() + Orient.DirectionAside() * DX * 0.5f - Orient.DirectionUp() * DZ * 0.5f);
    pointList.AddPoint(Orient.Position() - Orient.DirectionAside() * DX * 0.5f - Orient.DirectionUp() * DZ * 0.5f);
    pointList.AddPoint(Orient.Position() - Orient.DirectionAside() * DX * 0.5f + Orient.DirectionUp() * DZ * 0.5f);
  }

  void CPolyplaneBlockSimple2::Save(RString fileName, RString suffix) {
    SaveTexture(fileName + "_A" + suffix + ".tga", _a._p, false);
    SaveTexture(fileName + "_B" + suffix + ".tga", _b._p, false);
    SaveTexture(fileName + "_A" + suffix + "_NO.tga", _a._pNorm, true);
    SaveTexture(fileName + "_B" + suffix + "_NO.tga", _b._pNorm, true);
  }

  ComRef<IDirect3DTexture8> CPolyplaneBlockSimple2::GetTexture(int index)
  {
    switch (index)
    {
    case 0: return _a._p;
    case 1: return _b._p;
    default: return ComRef<IDirect3DTexture8>();
    }
  }
  ComRef<IDirect3DTexture8> CPolyplaneBlockSimple2::GetNormalTexture(int index)
  {
    switch (index)
    {
    case 0: return _a._pNorm;
    case 1: return _b._pNorm;
    default: return ComRef<IDirect3DTexture8>();
    }
  }

}