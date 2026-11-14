#include "RVTreeCommon.h"
#include "PolyplaneBunch.h"


namespace TreeEngine
{
    CPolyplaneBunch::CPolyplaneBunch() {
  }

  CPolyplaneBunch::~CPolyplaneBunch() {
    Done();
  }

/*  HRESULT CPolyplaneBunch::Init(PDirect3DDevice &pD3DDevice) {
    HRESULT hr;

    // Call the parent Init method
    hr = CPolyplane::Init(pD3DDevice);
    if (FAILED(hr)) return hr;

    // Creating of particular color maps
    if (FAILED(hr = CreatePolyplaneTexture(pD3DDevice,textureSize, _pA.Init()))) return hr;
    if (FAILED(hr = CreatePolyplaneTexture(pD3DDevice,textureSize, _pB.Init()))) return hr;
    if (FAILED(hr = CreatePolyplaneTexture(pD3DDevice,textureSize, _pC.Init()))) return hr;  
    // Creating of particular normal maps
    if (FAILED(hr = CreatePolyplaneTexture(pD3DDevice,textureSize, _pANorm.Init()))) return hr;
    if (FAILED(hr = CreatePolyplaneTexture(pD3DDevice,textureSize, _pBNorm.Init()))) return hr;
    if (FAILED(hr = CreatePolyplaneTexture(pD3DDevice,textureSize, _pCNorm.Init()))) return hr;

    return S_OK;
  }*/

  HRESULT CPolyplaneBunch::Done() {
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

  void CPolyplaneBunch::Render(
    CPrimitiveStream &PSBranch,
    CPrimitiveStream &PSPolyplane,
    CPrimitiveStream &PSStageInit,
    Matrix4Par Origin,
    const CPointList &pointList,
    float SizeCoef,
    const DrawParameters &dp) 
  {

    // Retrieve values from point list
    SBoundingBox bbox;
    pointList.ComputeBoundingBox(Origin, bbox);
    _Centre = bbox.GetFocus();
    _Dimension = bbox.GetDimension();
    Vector3 focus;
    pointList.ComputeFocus(Origin, focus);
    _CentreOfMass = Vector3(0, 0, focus.Z());

    // Save the size coeficient
    _SizeCoef = SizeCoef;

    // Render planes
    Matrix4 Camera;
    Camera.SetPosition(_Centre);

    // Modify geometry normals
    ModifyNormals(PSBranch, PSPolyplane, pointList, bbox.GetVolume(), dp);

    // Preparing of the graphics
    PSBranch.Prepare();
    PSPolyplane.Prepare();

    // A plane - parallel with X axis
    Camera.SetDirectionAndUp(Vector3(0, 1, 0), Vector3(0, 0, 1));
    RenderPlane(PSBranch, PSPolyplane,
      _a._p, _a._pNorm, pointList, Origin * Camera, _Dimension.X(), _Dimension.Z(), dp, &_a._hull);

    // B plane - rotated by 60 degree from X axis
    Camera.SetDirectionAndUp(Vector3(cos(H_PI * 0.33f), sin(H_PI * 0.33f), 0), Vector3(0, 0, 1));
    RenderPlane(PSBranch, PSPolyplane,
      _b._p, _b._pNorm, pointList, Origin * Camera, _Dimension.X(), _Dimension.Z(), dp, &_b._hull);

    // C plane - rotated by 120 degree from X axis
    Camera.SetDirectionAndUp(Vector3(cos(H_PI * 0.66f), sin(H_PI * 0.66f), 0), Vector3(0, 0, 1));
    RenderPlane(PSBranch, PSPolyplane,
      _c._p, _c._pNorm, pointList, Origin * Camera, _Dimension.X(), _Dimension.Z(), dp, &_c._hull);
    
    // Switch on lights
    //_pD3DDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    RegisterTextureToStage(PSStageInit,0,_a._p,_a._pNorm);
    RegisterTextureToStage(PSStageInit,1,_b._p,_b._pNorm);
    RegisterTextureToStage(PSStageInit,2,_c._p,_c._pNorm);
  }

  void CPolyplaneBunch::Draw(
                          CPrimitiveStream &PSTriplane,
                          Matrix4Par Origin,
                          float NewSizeCoef) {
    int Stage=_baseStage;

    // Coeficient of the size of the drawing
    float DrawSizeCoef = CalcScale(NewSizeCoef);

    // Retrieving the centre of the triplane
    Matrix4 Centre;
    Centre.SetOrientation(Origin.Orientation());
    Centre.SetPosition(Origin * (_Centre * DrawSizeCoef));

    Matrix4 Orient;
    float DX = _Dimension.X() * DrawSizeCoef;

    float DZ = _Dimension.Z() * DrawSizeCoef;

    // A plane - parallel with X axis
    Orient.SetDirectionAndUp(Origin.DirectionUp(), Origin.Direction());
    Orient.SetPosition(Centre * Vector3(0, (_CentreOfMass.Y() - _Centre.Y()) * DrawSizeCoef, 0));
    DrawPlane(Stage + 0, PSTriplane, _a._p, _a._pNorm, Orient, DX, DZ, 0.3f, &_a._hull);

    // B plane - rotated by 60 degree from X axis
    Matrix4 RotY;
    RotY.SetRotationY(H_PI * 0.33f);
    Orient.SetDirectionAndUp(Origin.DirectionUp(), Origin.Direction());
    Orient.SetPosition(Centre * Vector3(0, (_CentreOfMass.Y() - _Centre.Y()) * DrawSizeCoef, 0));
    DrawPlane(Stage + 1, PSTriplane, _b._p, _b._pNorm, Orient * RotY, DX, DZ, 0.3f, &_b._hull);

    // B plane - rotated by 60 degree from X axis
    RotY.SetRotationY(H_PI * 0.66f);
    Orient.SetDirectionAndUp(Origin.DirectionUp(), Origin.Direction());
    Orient.SetPosition(Centre * Vector3(0, (_CentreOfMass.Y() - _Centre.Y()) * DrawSizeCoef, 0));
    DrawPlane(Stage + 2, PSTriplane, _c._p, _c._pNorm, Orient * RotY, DX, DZ, 0.3f, &_c._hull);

  }

  void CPolyplaneBunch::UpdatePointList(Matrix4Par Origin,
                                        float NewSizeCoef,
                                        CPointList &pointList) {
  }

  void CPolyplaneBunch::Save(RString fileName, RString suffix) {
    SaveTexture(fileName + "_A" + suffix + ".tga", _a._p, false);
    SaveTexture(fileName + "_B" + suffix + ".tga", _b._p, false);
    SaveTexture(fileName + "_C" + suffix + ".tga", _c._p, false);
    SaveTexture(fileName + "_A" + suffix + "_NO.tga", _a._pNorm, true);
    SaveTexture(fileName + "_B" + suffix + "_NO.tga", _b._pNorm, true);
    SaveTexture(fileName + "_C" + suffix + "_NO.tga", _c._pNorm, true);

  /*
    D3DXSaveTextureToFile(fileName + "_A.bmp", D3DXIFF_BMP, _pA, NULL);
    D3DXSaveTextureToFile(fileName + "_B.bmp", D3DXIFF_BMP, _pB, NULL);
    D3DXSaveTextureToFile(fileName + "_C.bmp", D3DXIFF_BMP, _pC, NULL);
    D3DXSaveTextureToFile(fileName + "_An.bmp", D3DXIFF_BMP, _pANorm, NULL);
    D3DXSaveTextureToFile(fileName + "_Bn.bmp", D3DXIFF_BMP, _pBNorm, NULL);
    D3DXSaveTextureToFile(fileName + "_Cn.bmp", D3DXIFF_BMP, _pCNorm, NULL);
  */
  }

  ComRef<IDirect3DTexture8> CPolyplaneBunch::GetTexture(int index)
  {
    switch (index)
    {
    case 0: return _a._p;
    case 1: return _b._p;
    case 2: return _c._p;
    default: return ComRef<IDirect3DTexture8>();
    }
  }
  ComRef<IDirect3DTexture8> CPolyplaneBunch::GetNormalTexture(int index)
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