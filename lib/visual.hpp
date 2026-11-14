#ifdef _MSC_VER
#pragma once
#endif

#ifndef _VISUAL_HPP
#define _VISUAL_HPP

#include <El/Math/math3d.hpp>
#include "types.hpp"

#ifdef _DEBUG
  #define ASSERT_ORIENTATION(fnc, name, onlyScaled) if (onlyScaled) AssertOrientation(fnc, name);
#else
  #define ASSERT_ORIENTATION(fnc, name, onlyScaled) ;
#endif

// Frame gives new coordinate system
// base class for camera, objects ..
class Frame: public Matrix4 
{
#ifdef _DEBUG
public:
  virtual bool IsClassId(const int *t) const {return false;} // casting info is needed only in AssertOrientation
  void AssertOrientation(const char * func, const char * name) const; // verify that matrix is only scaled (orthogonal with the same scale in every dir)
#endif

public:
  Frame() :Matrix4(MIdentity) {}
  Frame(Matrix4Par trans) :Matrix4(trans) {};

  const Matrix4 &Transform() const {return *this;}
  void SetTransform(Matrix4Par trans) {this->Matrix4::operator=(trans);}

  Matrix4 CalcInvTransform(const char* name = NULL) const
  {
    ASSERT_ORIENTATION("Frame::SetTransform", name, true);
    return InverseScaled();
  }
  void SetOrient(Vector3Par dir, Vector3Par up) {SetDirectionAndUp(dir, up);}
  void SetOrientScaleOnly(float scale) {SetOrientationScaleOnly(scale);}  

  // model to world transformations
  const Matrix4 &ModelToWorld() const {return *this;}
  const Matrix3 &DirModelToWorld() const {return Orientation();}
  
#ifdef _KNI
  Vector3 PositionModelToWorld(Vector3Par v) const {return FastTransform(v);}
  Vector3 DirectionModelToWorld(Vector3Par v) const {return Rotate(v);}
#else
  Vector3 PositionModelToWorld(Vector3Par v) const {return Vector3(VFastTransform, *this, v);}
  Vector3 DirectionModelToWorld(Vector3Par v) const {return Vector3(VRotate, *this, v);}
#endif

  // faster but less convenient versions
  void PositionModelToWorld(Vector3 &res, Vector3Par v) const {res.SetFastTransform(*this, v);}
  void DirectionModelToWorld(Vector3 &res, Vector3Par v) const {res.SetRotate(*this, v);}
  
  bool Equal(const Frame &with) const;
  bool operator==(const Frame &with) const {return Equal(with);}
  bool operator!=(const Frame &with) const {return !Equal(with);}
};

/** frame with possibility of virtual optimization - used as base for ObjectVisualState
  * Transformation represented by FrameBase can be scaled or general matrix. 
  * It is considered that transformation is only scaled (orientation is orthogonal with the same scale in every direction).
  * Since once the transformation is marked like a general/skewed matrix, it is considered that transformation is general rest of FrameBase lifetime.
  */ 
class FrameBase: public Frame
{
  friend class Object;

  typedef Frame base;
protected:
  float _scale; //< remember scale to avoid its calculation
  float _maxScale; //< if it is negative matrix is scaled, otherwise matrix is general and it is maximal scale of vector transformed by this object
  
public:
  FrameBase();
  explicit FrameBase(Matrix4Par trans, bool scaled=true)
  :Frame(trans)
  {
    if (scaled)
    {
      _scale = base::Direction().Size(); // same scale in every dir
      _maxScale = -1.0f;
    }
    else
      base::ScaleAndMaxScale(_scale, _maxScale);
    ASSERT_ORIENTATION("FrameBase::FrameBase", NULL, scaled);  // this is a newly created FrameBase - can't have a name
  }

  // optimized scale calculation
  float Scale() const {return _scale;}
  float MaxScale() const {return (_maxScale < 0.0f) ? _scale : _maxScale;} //< if transformation is scaled max scale is equivalent to scale
  void SetScale(float scale)
  {
    if (_maxScale < 0.0f)
    {
      // setting a scale does not change orthogonality
      _scale = scale;
      Frame::SetScale(scale);
    }
    else
    {
      // remove the old scale from the _maxScale, introduce the new scale there
      _maxScale = _maxScale / _scale * scale;
      _scale = scale;
      Frame::SetScale(scale);
    }
  }

  // virtually optimized Frame inverse transform
  virtual void SetPosition(Vector3Par pos);

  virtual void SetTransform(const Matrix4 &transform);
  virtual void SetTransformSkewed(const Matrix4 &transform); // Since now transformation will be handled like a general/skewed
  virtual void SetTransformPossiblySkewed(const Matrix4 &transform ); // check if transformation is skewed

  virtual void SetOrient(const Matrix3 &dir, const char* name = NULL);
  virtual void SetOrientSkewed(const Matrix3 &dir); // Since now transformation will be handled like a general/skewed
  virtual void SetOrient(Vector3Par dir, Vector3Par up);
  virtual void SetOrientScaleOnly(float scale);

  virtual Matrix4 GetInvTransform() const;

  void SetSkewed() {_maxScale = Frame::MaxScale();}  // Since now transformation will be handled like a general/skewed
  /// check if matrix is orthogonal (no skew, possible scale)
  bool GetSkewed() const {return _maxScale >= 0.0f;}
  /// check if matrix is orthonormal (no skew, no scale)
  bool GetOrthonormal() const {return _maxScale < 0.0f || fabs(_scale - 1.0f) < 1e-3f;}
};

/// visual state age relative to Current (Render) state
typedef float VisualStateAge;

#endif
