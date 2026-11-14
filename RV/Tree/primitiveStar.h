#ifndef _primitiveStar_h_
#define _primitiveStar_h_

#include "primitive.h"


namespace TreeEngine
{
    class CPStar: public CPrimitiveWithTexture 
  {
  protected:
    //! Width of the gutter
    float _width;
    //! Height of the gutter
    float _height;
    //! Number of segments the star consists from <1, ...>
    int _segmentsCount;
    //! V offset of the texture corresponds to the current depth of the tree
    float _textureVOffset;
    //! V tiling coefficient
    float _textureVTiling;
  public:
    //! Sets the texture properties
    void SetTexture(RString name, int index);
    //! Sets parameters for this primitive
    void SetParams(
      float width,
      float height,
      int segmentsCount,
      float textureVOffset,
      float textureVTiling);
    //! Returns the front child parameters
    SPrimitiveChild GetFrontChild();
    //! Returns the side child according to a specified parameters
    SPrimitiveChild GetSideChild(
      float directionShift,
      float sideShift,
      float slopeAngle,
      float twistAngle,
      float spinAngle);
    SPrimitiveChild GetSideChildFA(
      float directionShift,
      float forwardShift,
      const AxisRotation *ar,
      int arCount);
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
    static void CPStar::RegisterCommands(GameState &gState);
  };


}
#endif
