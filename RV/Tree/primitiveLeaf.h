#ifndef _primitiveLeaf_h_
#define _primitiveLeaf_h_

#include "primitive.h"


namespace TreeEngine
{
    class CPLeaf : public CPrimitiveWithTexture {
  protected:
    float _width;
    float _height;
    UVTransform _uvTransform[2];
  public:
    //! Sets the texture properties
    void SetTexture(RString name, int index);
    //! Sets parameters for this primitive
    void SetLeafParams(float width, float height);
    //! Virtual method
    virtual int Draw(
      CPrimitiveStream &ps,
      int sharedVertexIndex,
      int sharedVertexCount,
      bool isRoot,
      float detail,
      SSlot *pSlot);
    //! Virtual method
    virtual void UpdatePointList(CPointList &pointList);
    static void RegisterCommands(GameState &gState);
    void SetLeafUVTransform(const UVTransform &trns) {_uvTransform[0]=_uvTransform[1]=trns;}
    void SetLeafUVTransformFront(const UVTransform &trns) {_uvTransform[0]=trns;}
    void SetLeafUVTransformBack(const UVTransform &trns) {_uvTransform[1]=trns;}
  };


    class CPSlice: public CPLeaf
    {
    public:
      virtual int Draw(
        CPrimitiveStream &ps,
        int sharedVertexIndex,
        int sharedVertexCount,
        float detail,
        SSlot *pSlot);
    };
    
}
#endif
