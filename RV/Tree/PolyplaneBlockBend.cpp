#include "RVTreeCommon.h"
#include "polyplaneBlockBend.h"


namespace TreeEngine
{
    CPolyplaneBlockBend::CPolyplaneBlockBend() {
  }

  CPolyplaneBlockBend::~CPolyplaneBlockBend() {
    Done();
  }

  HRESULT CPolyplaneBlockBend::Init(ITextureFactory *factory, int textureSize) {
    HRESULT hr;

    // Call the parent Init method
    hr = CPolyplane::Init(factory, textureSize);
    if (FAILED(hr)) return hr;
/*
    // Creating of particular color maps
    if (FAILED(hr = CreatePolyplaneTexture(pD3DDevice,textureSize, _pA.Init()))) return hr;
    if (FAILED(hr = CreatePolyplaneTexture(pD3DDevice,textureSize, _pB.Init()))) return hr;
    if (FAILED(hr = CreatePolyplaneTexture(pD3DDevice,textureSize, _pC.Init()))) return hr;
    // Creating of particular normal maps
    if (FAILED(hr = CreatePolyplaneTexture(pD3DDevice,textureSize, _pANorm.Init()))) return hr;
    if (FAILED(hr = CreatePolyplaneTexture(pD3DDevice,textureSize, _pBNorm.Init()))) return hr;
    if (FAILED(hr = CreatePolyplaneTexture(pD3DDevice,textureSize, _pCNorm.Init()))) return hr;
*/
    return S_OK;
  }

  HRESULT CPolyplaneBlockBend::Done()
  {
    HRESULT hr;

    // Call the parent Done method
    hr = CPolyplane::Done();
    if (FAILED(hr)) return hr;

    // Free allocated surfaces
    _a.Free();
    _b.Free();
    _c.Free();

    return S_OK;
  }

  void CPolyplaneBlockBend::Render(CPrimitiveStream &psBranch,
                              CPrimitiveStream &psPolyplane,
                              CPrimitiveStream &PSStageInit,
                              Matrix4Par origin,
                              const CPointList &pointList,
                              float sizeCoef,
                              const DrawParameters &dp) {

    Vector3 focus;
    if (pointList.ComputeFocus(focus)) 
    {

      Vector3 newDirection = (focus - origin.Position()).Normalized();
      Matrix3 newDirAxis;
      newDirAxis.SetDirectionAndAside(newDirection,origin.DirectionAside());
      Matrix3 invOrig=origin.Orientation().InverseGeneral();
      Matrix3 dirTrns=invOrig*newDirAxis;
      _rotation=dirTrns;

/*      // Get transformation matrix and a new origin
      Vector3 newDirection = (focus - origin.Position()).Normalized();
      float dp = newDirection.DotProduct(origin.Direction());
      saturate(dp, -1.0f, 1.0f);
      float angle = acos(dp);
      Matrix3 rotation;
      rotation.SetRotationAxis(newDirection.CrossProduct(origin.Direction()), angle);
      _rotation = rotation.InverseRotation();*/
      Matrix4 newOrigin;
      newOrigin.SetOrientation(origin.Orientation() * _rotation);
      newOrigin.SetPosition(origin.Position());

      SBoundingBox bbox;
      if (pointList.ComputeBoundingBox(newOrigin, bbox)) {
        
        Vector3 focus;
        if (pointList.ComputeFocus(newOrigin, focus)) {

          // Get important coeficients
          _centre = bbox.GetFocus();
          _dimension = bbox.GetDimension();
          _centreOfMassZ = focus.Z();

          // Save the size coeficient
          _SizeCoef = sizeCoef;

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
            _a._p, _a._pNorm, pointList, newOrigin * Camera, _dimension.Y(), _dimension.Z(), dp, &_a._hull);

          // B plane
          Camera.SetDirectionAndUp(Vector3(0, -1, 0), Vector3(0, 0, 1));
          RenderPlane(psBranch, psPolyplane,
            _b._p, _b._pNorm, pointList, newOrigin * Camera, _dimension.X(), _dimension.Z(), dp, &_b._hull);

          // C plane
          Camera.SetDirectionAndUp(Vector3(0, 0, 1), Vector3(0, 1, 0));
          RenderPlane(psBranch, psPolyplane,
            _c._p, _c._pNorm, pointList, newOrigin * Camera, _dimension.X(), _dimension.Y(), dp, &_c._hull);

          RegisterTextureToStage(PSStageInit,0,_a._p,_a._pNorm);
          RegisterTextureToStage(PSStageInit,1,_b._p,_b._pNorm);
          RegisterTextureToStage(PSStageInit,2,_c._p,_c._pNorm);
        }
        else {
          LogF("Error while computing a focus of geometry.");
        }
      }
      else {
        LogF("Error while computing a bouding box of geometry.");
      }
    }
    else {
      LogF("Error while computing a focus of geometry.");
    }
  }

  void CPolyplaneBlockBend::Draw(
                                CPrimitiveStream &psTriplane,
                                Matrix4Par origin,
                                float newSizeCoef
                                ) {
    int stage=_baseStage;
    // Coeficient of the size of the drawing
    float drawSizeCoef = CalcScale(newSizeCoef);

    // Get new origin
    Matrix4 newOrigin;
    newOrigin.SetOrientation(origin.Orientation() * _rotation);
    newOrigin.SetPosition(origin.Position());

    // Retrieving the centre of the triplane
    Matrix4 Centre;
    Centre.SetOrientation(newOrigin.Orientation());
    Centre.SetPosition(newOrigin * (_centre * drawSizeCoef));

    Matrix4 Orient;
    float DX = _dimension.X() * drawSizeCoef;
    float DY = _dimension.Y() * drawSizeCoef;
    float DZ = _dimension.Z() * drawSizeCoef;

    // A
    Orient.SetDirectionAndUp(-newOrigin.DirectionAside(), newOrigin.Direction());
    Orient.SetPosition(Centre * Vector3(-_centre.X() * drawSizeCoef, 0, 0));
    DrawPlane(stage + 0, psTriplane, _a._p, _a._pNorm, Orient, DY, DZ, 0, &_a._hull);

    // B
    Orient.SetDirectionAndUp(-newOrigin.DirectionUp(), newOrigin.Direction());
    Orient.SetPosition(Centre * Vector3(0, -_centre.Y() * drawSizeCoef, 0));
    DrawPlane(stage + 1, psTriplane, _b._p, _b._pNorm, Orient, DX, DZ, 0, &_b._hull);

    // C
    Orient.SetDirectionAndUp(-newOrigin.Direction(), newOrigin.DirectionUp());
    Orient.SetPosition(Centre * Vector3(0, 0, (_centreOfMassZ - _centre.Z()) * drawSizeCoef));
    DrawPlane(stage + 2, psTriplane, _c._p, _c._pNorm, Orient, DX, DY, 0, &_c._hull);
  }

  void CPolyplaneBlockBend::UpdatePointList(Matrix4Par origin,
                                            float newSizeCoef,
                                            CPointList &pointList) {

    // Coeficient of the size of the drawing
    float drawSizeCoef = CalcScale(newSizeCoef);

    // Get new origin
    Matrix4 newOrigin;
    newOrigin.SetOrientation(origin.Orientation() * _rotation);
    newOrigin.SetPosition(origin.Position());

    // Retrieving the centre of the triplane
    Matrix4 Centre;
    Centre.SetOrientation(newOrigin.Orientation());
    Centre.SetPosition(newOrigin * (_centre * drawSizeCoef));

    Matrix4 Orient;
    float DX = _dimension.X() * drawSizeCoef;
    float DY = _dimension.Y() * drawSizeCoef;
    float DZ = _dimension.Z() * drawSizeCoef;

    // A
    Orient.SetDirectionAndUp(-newOrigin.DirectionAside(), newOrigin.Direction());
    Orient.SetPosition(Centre * Vector3(-_centre.X() * drawSizeCoef, 0, 0));
    pointList.AddPoint(Orient.Position() + Orient.DirectionAside() * DY * 0.5f + Orient.DirectionUp() * DZ * 0.5f);
    pointList.AddPoint(Orient.Position() + Orient.DirectionAside() * DY * 0.5f - Orient.DirectionUp() * DZ * 0.5f);
    pointList.AddPoint(Orient.Position() - Orient.DirectionAside() * DY * 0.5f - Orient.DirectionUp() * DZ * 0.5f);
    pointList.AddPoint(Orient.Position() - Orient.DirectionAside() * DY * 0.5f + Orient.DirectionUp() * DZ * 0.5f);

    // B
    Orient.SetDirectionAndUp(-newOrigin.DirectionUp(), newOrigin.Direction());
    Orient.SetPosition(Centre * Vector3(0, -_centre.Y() * drawSizeCoef, 0));
    pointList.AddPoint(Orient.Position() + Orient.DirectionAside() * DX * 0.5f + Orient.DirectionUp() * DZ * 0.5f);
    pointList.AddPoint(Orient.Position() + Orient.DirectionAside() * DX * 0.5f - Orient.DirectionUp() * DZ * 0.5f);
    pointList.AddPoint(Orient.Position() - Orient.DirectionAside() * DX * 0.5f - Orient.DirectionUp() * DZ * 0.5f);
    pointList.AddPoint(Orient.Position() - Orient.DirectionAside() * DX * 0.5f + Orient.DirectionUp() * DZ * 0.5f);

    // C
    Orient.SetDirectionAndUp(-newOrigin.Direction(), newOrigin.DirectionUp());
    Orient.SetPosition(Centre * Vector3(0, 0, (_centreOfMassZ - _centre.Z()) * drawSizeCoef));
    pointList.AddPoint(Orient.Position() + Orient.DirectionAside() * DX * 0.5f + Orient.DirectionUp() * DY * 0.5f);
    pointList.AddPoint(Orient.Position() + Orient.DirectionAside() * DX * 0.5f - Orient.DirectionUp() * DY * 0.5f);
    pointList.AddPoint(Orient.Position() - Orient.DirectionAside() * DX * 0.5f - Orient.DirectionUp() * DY * 0.5f);
    pointList.AddPoint(Orient.Position() - Orient.DirectionAside() * DX * 0.5f + Orient.DirectionUp() * DY * 0.5f);
  }

  void CPolyplaneBlockBend::Save(RString fileName, RString suffix) {
    SaveTexture(fileName + "_A" + suffix + ".tga", _a._p, false);
    SaveTexture(fileName + "_B" + suffix + ".tga", _b._p, false);
    SaveTexture(fileName + "_C" + suffix + ".tga", _c._p, false);
    SaveTexture(fileName + "_A" + suffix + "_NO.tga", _a._pNorm, true);
    SaveTexture(fileName + "_B" + suffix + "_NO.tga", _b._pNorm, true);
    SaveTexture(fileName + "_C" + suffix + "_NO.tga", _c._pNorm, true);
  }

  ComRef<IDirect3DTexture8> CPolyplaneBlockBend::GetTexture(int index)
  {
    switch (index)
    {
    case 0: return _a._p;
    case 1: return _b._p;
    case 2: return _c._p;
    default: return ComRef<IDirect3DTexture8>();
    }
  }
  ComRef<IDirect3DTexture8> CPolyplaneBlockBend::GetNormalTexture(int index)
  {
    switch (index)
    {
    case 0: return _a._pNorm;
    case 1: return _b._pNorm;
    case 2: return _c._pNorm;
    default: return ComRef<IDirect3DTexture8>();
    }
  }

}  