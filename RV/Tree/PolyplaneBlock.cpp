#include "RVTreeCommon.h"
#include "component.h"
#include "PolyplaneBlock.h"

namespace TreeEngine
{

  CPolyplaneBlock::CPolyplaneBlock() {
  }

  CPolyplaneBlock::~CPolyplaneBlock() {
    Done();
  }

  HRESULT CPolyplaneBlock::Init(ITextureFactory *factory, int textureSize) {
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

  HRESULT CPolyplaneBlock::Done() {
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

  void CPolyplaneBlock::Render(CPrimitiveStream &PSBranch,
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

    // Save the size coefficient
    _SizeCoef = SizeCoef;

    // Render planes
    Matrix4 Camera;
    Camera.SetPosition(_Centre);

    // Modify geometry normals
    ModifyNormals(PSBranch, PSPolyplane, pointList, bbox.GetVolume(), dp);

    // Preparing of the graphics
    PSBranch.Prepare();
    PSPolyplane.Prepare();

    // A plane
    Camera.SetDirectionAndUp(Vector3(-1, 0, 0), Vector3(0, 0, 1));
    RenderPlane(PSBranch, PSPolyplane,
      _a._p, _a._pNorm, pointList, Origin * Camera, _Dimension.Y(), _Dimension.Z(), dp, &_a._hull);

    // B plane
    Camera.SetDirectionAndUp(Vector3(0, -1, 0), Vector3(0, 0, 1));
    RenderPlane(PSBranch, PSPolyplane,
      _b._p, _b._pNorm, pointList, Origin * Camera, _Dimension.X(), _Dimension.Z(), dp, &_b._hull);

    // C plane
    Camera.SetDirectionAndUp(Vector3(0, 0, -1), Vector3(0, 1, 0));
    RenderPlane(PSBranch, PSPolyplane,
      _c._p, _c._pNorm, pointList, Origin * Camera, _Dimension.X(), _Dimension.Y(), dp, &_c._hull);

    RegisterTextureToStage(PSStageInit,0,_a._p,_a._pNorm);
    RegisterTextureToStage(PSStageInit,1,_b._p,_b._pNorm);
    RegisterTextureToStage(PSStageInit,2,_c._p,_c._pNorm);
  }

  void CPolyplaneBlock::Draw(
                            CPrimitiveStream &PSTriplane,
                            Matrix4Par Origin,
                            float NewSizeCoef
                            ) 
  {
    int Stage=_baseStage;

    // Coeficient of the size of the drawing
    //float DrawSizeCoef = 1.0f;//NewSizeCoef / _SizeCoef;
    float DrawSizeCoef = CalcScale(NewSizeCoef);

    // Retrieving the centre of the triplane
    Matrix4 Centre;
    Centre.SetOrientation(Origin.Orientation());
    Centre.SetPosition(Origin * (_Centre * DrawSizeCoef));

    Matrix4 Orient;
    float DX = _Dimension.X() * DrawSizeCoef;
    float DY = _Dimension.Y() * DrawSizeCoef;
    float DZ = _Dimension.Z() * DrawSizeCoef;

    // A
    Orient.SetDirectionAndUp(-Origin.DirectionAside(), Origin.Direction());
    Orient.SetPosition(Centre * Vector3((_CentreOfMass.X() - _Centre.X()) * DrawSizeCoef, 0, 0));
    DrawPlane(Stage + 0, PSTriplane, _a._p, _a._pNorm, Orient, DY, DZ, 0, &_a._hull);

    // B
    Orient.SetDirectionAndUp(-Origin.DirectionUp(), Origin.Direction());
    Orient.SetPosition(Centre * Vector3(0, (_CentreOfMass.Y() - _Centre.Y()) * DrawSizeCoef, 0));
    DrawPlane(Stage + 1, PSTriplane, _b._p, _b._pNorm, Orient, DX, DZ, 0, &_b._hull);

    // C
    Orient.SetDirectionAndUp(-Origin.Direction(), Origin.DirectionUp());
    Orient.SetPosition(Centre * Vector3(0, 0, (_CentreOfMass.Z() - _Centre.Z()) * DrawSizeCoef));
    DrawPlane(Stage + 2, PSTriplane, _c._p, _c._pNorm, Orient, DX, DY, 0, &_c._hull);
  }

  void CPolyplaneBlock::UpdatePointList(Matrix4Par Origin,
                                        float NewSizeCoef,
                                        CPointList &pointList) {

    // Coeficient of the size of the drawing
    float DrawSizeCoef = CalcScale(NewSizeCoef);

    // Retrieving the centre of the triplane
    Matrix4 Centre;
    Centre.SetOrientation(Origin.Orientation());
    Centre.SetPosition(Origin * (_Centre * DrawSizeCoef));

    Matrix4 Orient;
    float DX = _Dimension.X() * DrawSizeCoef;
    float DY = _Dimension.Y() * DrawSizeCoef;
    float DZ = _Dimension.Z() * DrawSizeCoef;

    // A
    Orient.SetDirectionAndUp(-Origin.DirectionAside(), Origin.Direction());
    Orient.SetPosition(Centre * Vector3((_CentreOfMass.X() - _Centre.X()) * DrawSizeCoef, 0, 0));
    pointList.AddPoint(Orient.Position() + Orient.DirectionAside() * DY * 0.5f + Orient.DirectionUp() * DZ * 0.5f);
    pointList.AddPoint(Orient.Position() + Orient.DirectionAside() * DY * 0.5f - Orient.DirectionUp() * DZ * 0.5f);
    pointList.AddPoint(Orient.Position() - Orient.DirectionAside() * DY * 0.5f - Orient.DirectionUp() * DZ * 0.5f);
    pointList.AddPoint(Orient.Position() - Orient.DirectionAside() * DY * 0.5f + Orient.DirectionUp() * DZ * 0.5f);

    // B
    Orient.SetDirectionAndUp(-Origin.DirectionUp(), Origin.Direction());
    Orient.SetPosition(Centre * Vector3(0, (_CentreOfMass.Y() - _Centre.Y()) * DrawSizeCoef, 0));
    pointList.AddPoint(Orient.Position() + Orient.DirectionAside() * DX * 0.5f + Orient.DirectionUp() * DZ * 0.5f);
    pointList.AddPoint(Orient.Position() + Orient.DirectionAside() * DX * 0.5f - Orient.DirectionUp() * DZ * 0.5f);
    pointList.AddPoint(Orient.Position() - Orient.DirectionAside() * DX * 0.5f - Orient.DirectionUp() * DZ * 0.5f);
    pointList.AddPoint(Orient.Position() - Orient.DirectionAside() * DX * 0.5f + Orient.DirectionUp() * DZ * 0.5f);

    // C
    Orient.SetDirectionAndUp(-Origin.Direction(), Origin.DirectionUp());
    Orient.SetPosition(Centre * Vector3(0, 0, (_CentreOfMass.Z() - _Centre.Z()) * DrawSizeCoef));
    pointList.AddPoint(Orient.Position() + Orient.DirectionAside() * DX * 0.5f + Orient.DirectionUp() * DY * 0.5f);
    pointList.AddPoint(Orient.Position() + Orient.DirectionAside() * DX * 0.5f - Orient.DirectionUp() * DY * 0.5f);
    pointList.AddPoint(Orient.Position() - Orient.DirectionAside() * DX * 0.5f - Orient.DirectionUp() * DY * 0.5f);
    pointList.AddPoint(Orient.Position() - Orient.DirectionAside() * DX * 0.5f + Orient.DirectionUp() * DY * 0.5f);
  }

  void CPolyplaneBlock::Save(RString fileName, RString suffix) {
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

  ComRef<IDirect3DTexture8> CPolyplaneBlock::GetTexture(int index)
  {
    switch (index)
    {
    case 0: return _a._p;
    case 1: return _b._p;
    case 2: return _c._p;
    default: return ComRef<IDirect3DTexture8>();
    }
  }
  ComRef<IDirect3DTexture8> CPolyplaneBlock::GetNormalTexture(int index)
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