#ifndef _primitiveStem_h_
#define _primitiveStem_h_

#include "primitive.h"


namespace TreeEngine
{
    class CPStem : public CPrimitiveConeType {
  protected:
    //! Width of the stem
    float _width;
    //! Height of the stem
    float _height;
    //! V offset of the texture corresponds to the current depth of the tree
    float _textureVOffset;
    //! Spin geometry relative to the origin
    float _geometrySpin;
    //! U tiling 
    float _textureUTiling;
    //! V tiling coefficient
    float _textureVTiling;
  public:
    //! Sets the texture properties
    void SetTexture(RString name, int index);
    //! Sets parameters for this primitive
    void SetStemParams(
      float width,
      float height,
      float textureVOffset,
      float geometrySpin,
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
    //! Virtual method
    virtual int DrawBody(ObjectData *o, int &bodyId);
    static void RegisterCommands(GameState &gState);
  };

 

}
#endif
