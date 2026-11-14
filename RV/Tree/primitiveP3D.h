#pragma once

#include "primitive.h"

namespace TreeEngine {

    class PrimitiveP3D: public CPrimitiveWithTexture
    {
    public:
        PrimitiveP3D(void);
        ~PrimitiveP3D(void);

    virtual int Draw( CPrimitiveStream &ps, int sharedVertexIndex,
                      int sharedVertexCount, bool isRoot,float detail,
                      SSlot *pSlot) = 0;
    //! Add important points of the primitive to the point list
    virtual void UpdatePointList(CPointList &pointList) = 0;

    };


}