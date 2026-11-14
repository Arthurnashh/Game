#ifndef _primitiveTree_h_
#define _primitiveTree_h_

#include "primitive.h"


namespace TreeEngine
{
  #define DM_DRAWCHILDA 1
  #define DM_DRAWCHILDB 2

  class CPTree : public CPrimitiveConeType {
  protected:
    //ComRef<IDirect3DTexture8> _pTexture;
    float _width;
    float _height;
    float _balance;
    float _stiffness;
    float _twist;
    float _spinA;
    float _spinB;
    //! Spread of branches (number from interval <0, 1>)
    float _spread;
    //! Number of segments for both branches
    int _segmentCount;
    //! Specifies which branch to draw (0-none, 1-A, 2-B, 3-A&B)
    int _drawMask;
    //! V offset of the texture corresponds to the current depth of the tree
    float _textureVOffset;
    //! Spin geometry relative to the origin
    float _geometrySpin;;
    //! U tiling coeficient
    float _textureUTiling;
    //! V tiling coeficient
    float _textureVTiling;

    bool _forceSegmentCount;
  public:
    //! Sets the texture properties
    void SetTexture(RString name, int index);
    //! Sets parameters for this primitive
    void SetTreeParams(
      float width,
      float height,
      float balance,
      float stiffness,
      float twist,
      float spinA,
      float spinB,
      float spread,
      int segmentCount,
      int drawMask,
      float textureVOffset,
      float geometrySpin,
      float textureUTiling,
      float textureVTiling,
      bool forceSegmentCount);
    SPrimitiveChild GetChildA();
    SPrimitiveChild GetSideChildA(
      float directionPos,
      float sideShift,
      float slopeAngle,
      float twistAngle,
      float spinAngle);
    SPrimitiveChild GetChildB();
    SPrimitiveChild GetSideChildB(
      float directionPos,
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
