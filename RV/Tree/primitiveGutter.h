#ifndef _primitiveGutter_h_
#define _primitiveGutter_h_

#include "primitive.h"


namespace TreeEngine
{
    class CPGutter : public CPrimitiveWithTexture
  {
  protected:
    //! Width of the gutter
    float _width;
    //! Height of the gutter
    float _height;
    //! Depth of the gutter
    float _angle;
    //! Number of segments the gutter consists from <1, ...>
    int _segmentsCount;
    //! V offset of the texture corresponds to the current depth of the tree
    float _textureVOffset;
    //! U tiling 
    float _textureUTiling;
    //! V tiling coefficient
    float _textureVTiling;
  public:
    //! Sets the texture properties
    void SetTexture(RString name, int index);
    //! Sets parameters for this primitive
    void SetGutterParams(
      float width,
      float height,
      float angle,
      int segmentsCount,
      float textureVOffset,
      float textureUTiling,
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
  };

}
#endif
