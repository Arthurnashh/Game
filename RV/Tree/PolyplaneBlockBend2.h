#ifndef _polyplaneBlockBend2_h_
#define _polyplaneBlockBend2_h_

#include "polyplane.h"


namespace TreeEngine
{
    //! Polyplane with a block shape bended to focus of the approximated area
  class CPolyplaneBlockBend2 : public CPolyplane
  {
  private:
    //! Plane which lies in the plane Direction and Up (ZY). Z correspond to the up vector.
    PlaneResources _a;
    //! Plane which lies in the plane Direction and Aside (ZX)
    PlaneResources _b;
    //! Rotation matrix
    Matrix3 _rotation;
    //! Center of the bounding box in relative coordinates to _inverseRotation * origin.Orientation()
    Vector3 _centre;
    //! Dimension of the bounding box
    Vector3 _dimension;
    //! Z value of the center of mass in same coordinates like _centre
    float _centreOfMassZ;
  public:
    //! Constructor
    CPolyplaneBlockBend2();
    //! Destructor
    ~CPolyplaneBlockBend2();
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
    virtual ComRef<IDirect3DTexture8> GetTexture(int index);
    virtual ComRef<IDirect3DTexture8> GetNormalTexture(int index);      
    virtual int GetNumberOfTextures() {return 2;};


  };

}
#endif
