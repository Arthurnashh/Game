#ifndef _POLYPLANEBLOCK_H_
#define _POLYPLANEBLOCK_H_

#include "Polyplane.h"


namespace TreeEngine
{
    //! Triplane with a block shape.
  class CPolyplaneBlock : public CPolyplane
  {
  private:
    //! Plane which lies in the plane Direction and Up (ZY). Z correspond to the up vector.
    PlaneResources _a;
    //! Plane which lies in the plane Direction and Aside (ZX).
    PlaneResources _b;
    //! Plane which lies in the plane Up and Aside (YX).
    PlaneResources _c;
    //! Position of the geometric center of the polyplane (bounding box) relative to the Origin.
    /*!
      Using this, Origin and _Dimension parameter we can obtain a block of polyplane
      (its position, orientation and size)
    */
    Vector3 _Centre;
    //! Dimension of the polyplane
    Vector3 _Dimension;
    //! Center of mass (f.i. point which belongs to all planes) relative to the Origin.
    Vector3 _CentreOfMass;
  public:
    //! Constructor.
    CPolyplaneBlock();
    //! Destructor.
    ~CPolyplaneBlock();
    //! Initialization of triplane.
    virtual HRESULT Init(ITextureFactory *factory, int textureSize);
    //! Deinitialization of triplane.
    virtual HRESULT Done();
    //! Preparing of the triplane.
    virtual void Render(
      CPrimitiveStream &PSBranch,
      CPrimitiveStream &PSPolyplane,
      CPrimitiveStream &PSStageInit,
      Matrix4Par Origin,
      const CPointList &pointList,
      float SizeCoef,
      const DrawParameters &dp);
    //! Drawing of the triplane.
    virtual void Draw( CPrimitiveStream &PSTriplane,   Matrix4Par Origin,   float NewSizeCoef);
    //! Virtual method
    virtual void UpdatePointList(
      Matrix4Par Origin,
      float NewSizeCoef,
      CPointList &pointList);
    //! Virtual method
    virtual void Save(RString fileName, RString suffix);
    virtual ComRef<IDirect3DTexture8> GetTexture(int index);
    virtual ComRef<IDirect3DTexture8> GetNormalTexture(int index);      
  };

}
#endif
