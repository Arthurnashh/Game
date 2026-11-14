#ifndef _primitiveFunnel_h_
#define _primitiveFunnel_h_

#include "primitive.h"


namespace TreeEngine
{
    class CPFunnel : public CPrimitiveWithTexture {
  protected:
    float _width;
    float _angle;
    int _sideCount;
    bool _twotextures;
  public:
      CPFunnel():_twotextures(false) {}
    //! Sets the texture properties
    void SetTexture(RString name, int index);
    //! Sets parameters for this primitive
    void SetParams(float width, float angle, int sideCount);
    //! enables two textures
    void EnableTwoTextures(bool twotexts) {_twotextures = twotexts; }
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
