#ifndef _primitiveBunch_h_
#define _primitiveBunch_h_

#include "primitive.h"

namespace TreeEngine
{
  class CPBunch : public CPrimitive
  {
  public:
    //! Returns the child
    SPrimitiveChild GetChild(
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
    virtual void UpdatePointList(CPointList &pointList);
    //! Registering of script commands
    static void RegisterCommands(GameState &gState);
  };
}

#endif
