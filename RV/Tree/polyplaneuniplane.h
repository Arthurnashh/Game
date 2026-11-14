#ifndef _POLYPLANEUNIPLANE_H_
#define _POLYPLANEUNIPLANE_H_

#include "Polyplane.h"


namespace TreeEngine
{
    //! Triplane with a bunch shape.
  class CPolyplaneUniplane : public CPolyplane
  {
  private:
    
    PlaneResources _a;
    //! Center of the bounding box in relative coordinates to _inverseRotation * origin.Orientation()
    Vector3 _centre;
    //! Dimension of the bounding box
    Vector3 _dimension;
    //! Z value of the center of mass in same coordinates like _centre
    float _centreOfMassZ;
    //! Rotation matrix
    Matrix3 _rotation;
  public:

      enum Axe {
          axe_x, axe_y, axe_z
      };

    //! Constructor.
    CPolyplaneUniplane(Axe axe);

    //! Destructor.
    ~CPolyplaneUniplane();

    //! Initialization of triplane.
//    virtual HRESULT Init(ITextureFactory *factory, int textureSize);

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

    virtual int GetNumberOfTextures() {return 1;};

  protected:

      Axe selAxe;
  };

}
#endif