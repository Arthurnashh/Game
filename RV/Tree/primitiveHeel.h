#ifndef _primitiveHeel_h_
#define _primitiveHeel_h_

#include "primitive.h"

namespace TreeEngine
{
  class CPHeel : public CPrimitiveConeType
  {
  protected:
    //! Width of the stem
    float _width;
    //! U tiling 
    float _textureUTiling;
    //! V offset of the texture of the trunk part
    float _textureVOffsetTrunk;
    //! V offset of the texture of the root part
    float _textureVOffsetRoot;
  public:
    //! Sets parameters for this primitive
    void SetHeelParams(float width, float textureUTiling, float textureVOffsetTrunk, float textureVOffsetRoot);
    //! Returns the trunk child parameters
    SPrimitiveChild GetTrunkChild();
    //! Returns the root child parameters
    SPrimitiveChild GetRootChild();
    //! Virtual method
    virtual int Draw(
      CPrimitiveStream &ps,
      int sharedVertexIndex,
      int sharedVertexCount,
      bool isRoot,
      float detail,
      SSlot *pSlot);
    //! Virtual method
    virtual void UpdatePointList(CPointList &pointList) {}
    //! Registering of script commands
    static void RegisterCommands(GameState &gState);
  };
}

#endif
