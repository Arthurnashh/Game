#ifndef _polyplaneBlockSimple2_h_
#define _polyplaneBlockSimple2_h_

#include "polyplane.h"


namespace TreeEngine
{
    //! Polyplane with no bending
  class CPolyplaneBlockSimple2 : public CPolyplane
  {
  private:
    //! Plane which lies in the plane Direction and Up (ZY). Z correspond to the up vector.
    PlaneResources _a;
    //! Plane which lies in the plane Direction and Aside (ZX)
    PlaneResources _b;
    //! Center of the bounding box in relative coordinates to _inverseRotation * origin.Orientation()
    Vector3 _centre;
    //! Dimension of the bounding box
    Vector3 _dimension;
  public:
    //! Constructor
    CPolyplaneBlockSimple2();
    //! Destructor
    ~CPolyplaneBlockSimple2();
    //! Virtual method
    virtual HRESULT Init(ITextureFactory *factory, int textureSize);
    //! Virtual method
    virtual HRESULT Done();
    //! Virtual method
    virtual void Render(
      CPrimitiveStream &PSBranch,
      CPrimitiveStream &PSPolyplane,
      CPrimitiveStream &PSStageInit,
      Matrix4Par Origin,
      const CPointList &pointList,
      float SizeCoef,
      const DrawParameters &dp);
    //! Virtual method
    virtual void Draw( CPrimitiveStream &PSTriplane,   Matrix4Par Origin,   float NewSizeCoef);
    //! Virtual method
    virtual void UpdatePointList(
      Matrix4Par origin,
      float newSizeCoef,
      CPointList &pointList);
    //! Virtual method
    virtual void Save(RString fileName, RString suffix);
    //! Virtual method
    virtual int GetNumberOfTextures() {return 2;};
    virtual ComRef<IDirect3DTexture8> GetTexture(int index);
    virtual ComRef<IDirect3DTexture8> GetNormalTexture(int index);      

  };

}
#endif
