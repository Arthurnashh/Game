#ifndef _primitiveTest_h_
#define _primitiveTest_h_

#include "primitive.h"


namespace TreeEngine
{
    class CPTest : public CPrimitiveWithTexture  {
  protected:
    float _arrowWidth;
    float _arrowHeight;
  public:
    //! Sets the texture properties
    void SetTexture(RString name, int index);
    //! Sets parameters for this primitive
    void SetParams(float arrowWidth, float arrowHeight);
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
