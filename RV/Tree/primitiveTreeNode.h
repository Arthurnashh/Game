#ifndef _primitiveTreeNode_h_
#define _primitiveTreeNode_h_

#include "primitive.h"


namespace TreeEngine
{
    struct SPTreeNodeChild {
    //! Final width
    float _width;
    //! Height
    float _height;
    //! Twist
    float _twist;
    //! Spin
    float _spin;
    //! Diversion
    float _diversion;
    //! U tiling coeficient
    float _textureUTiling;
    //! V tiling coeficient
    float _textureVTiling;
    //! Number of segments
    int _segmentCount;
    //!
    bool _forceSegmentCount;

  };

  #define TREENODE_MAXCHILDCOUNT 16

  class CPTreeNode : public CPrimitiveConeType {
  protected:
    //! Width of the base of the node
    float _baseWidth;
    //! Offset of the texture
    float _textureVOffset;
    //! Spin geometry relative to the origin
    float _geometrySpin;
    //! Number of child nodes
    int _childNum;
    //! Children (branches)
    SPTreeNodeChild _child[TREENODE_MAXCHILDCOUNT];
  public:
    //! Sets the texture properties
    void SetTexture(RString name, int index);
    //! Set tree node common parameters
    void SetTreeNodeParams(
      float baseWidth,
      float textureVOffset,
      float geometrySpin,
      int childNum);
    //! Set parameters of specified child
    void SetTreeNodeChildParams(
      int childIndex,
      float width,
      float height,
      float twist,
      float spin,
      float diversion,
      float textureUTiling,
      float textureVTiling,
      int segmentCount,
      bool forceSegmentCount);

    //! Returns child of specified index
    SPrimitiveChild GetChild(int childIndex);
    //! Returns side child of the child of specified index
    SPrimitiveChild GetSideChild(
      int childIndex,
      float directionPos,
      float sideShift,
      float slopeAngle,
      float twistAngle,
      float spinAngle);
    //! Free axis version of the GetSideChild function
    SPrimitiveChild GetSideChildFA(
      int childIndex,
      float directionPos,
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

  #endif
}