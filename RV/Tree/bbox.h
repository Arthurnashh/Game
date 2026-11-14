#ifndef _bbox_h_
#define _bbox_h_

#include <El/Math/math3d.hpp>

namespace TreeEngine
{


  //! Bounding box structure.
  struct SBBox {
    //! Direction limits.
    float _MinDir, _MaxDir;
    //! Up limits.
    float _MinUp, _MaxUp;
    //! Aside limits.
    float _MinAside, _MaxAside;
    //! Retrieves focus of the bounding box.
    inline Vector3 GetFocus() {
      Vector3 Result;
      Result[0] = (_MaxAside + _MinAside) * 0.5f;
      Result[1] = (_MaxUp + _MinUp) * 0.5f;
      Result[2] = (_MaxDir + _MinDir) * 0.5f;
      return Result;
    }
    //! Retrieves dimensions of the bounding box.
    inline Vector3 GetDimension() {
      Vector3 Result;
      Result[0] = _MaxAside - _MinAside;
      Result[1] = _MaxUp - _MinUp;
      Result[2] = _MaxDir - _MinDir;
      return Result;
    }
  };

  //! Bounding box counter class.
  /*!
  Class suitable for bounding box computation. Add all vertices that should be within
  the bounding box using AddVertex(...) method. Finally you can retrieve the dimension of
  the bounding box using GetBBox() method. Don't forget to call Init(...) method before
  using this class.
  */
  class CBBoxCounter {
  private:
    //! Inverse origin of the bounding box.
    Matrix4 _InvOrigin;
    //! Bounding box.
    SBBox _BBox;
    //! Number of vertices the bounding box is built from.
    int _VertexCount;
    //! Sums for each axis for average values computation.
    Vector3 _Sum;
  public:
    //! Initialization of the counter.
    inline void Init(Matrix4Par Origin) {
      _InvOrigin.SetInvertGeneral(Origin);
      _BBox._MinDir = FLT_MAX;
      _BBox._MaxDir = -FLT_MAX;
      _BBox._MinUp = FLT_MAX;
      _BBox._MaxUp = -FLT_MAX;
      _BBox._MinAside = FLT_MAX;
      _BBox._MaxAside = -FLT_MAX;
      _VertexCount = 0;
      _Sum = Vector3(0, 0, 0);
    }
    //! Adding of one vertex - extenting of the bounding box.
    inline void AddVertex(Vector3Par Vertex) {
      Vector3 InvVertex = _InvOrigin * Vertex;
      _BBox._MinDir = __min(_BBox._MinDir, InvVertex.Z());
      _BBox._MaxDir = __max(_BBox._MaxDir, InvVertex.Z());
      _BBox._MinUp = __min(_BBox._MinUp, InvVertex.Y());
      _BBox._MaxUp = __max(_BBox._MaxUp, InvVertex.Y());
      _BBox._MinAside = __min(_BBox._MinAside, InvVertex.X());
      _BBox._MaxAside = __max(_BBox._MaxAside, InvVertex.X());
      _VertexCount++;
      _Sum += InvVertex;
    }
    //! Bounding box dimension retrieving.
    inline SBBox GetBBox() {
      return _BBox;
    }
    //! Average value in Direction direction.
    inline float GetAvgDir() {
      return _Sum.Z() / ((float) _VertexCount);
    }
  };

};
#endif