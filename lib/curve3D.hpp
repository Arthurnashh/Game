
/// @file Classes for 3D curves



#ifdef _MSC_VER
#pragma once
#endif

#ifndef _CURVE3D_HPP
#define _CURVE3D_HPP

/// Lineary interpolated 1D curve
class LinCurve1D : public RefCount
{
  AutoArray<float> _x;
  AutoArray<float> _y;
public:
  /// Returns interpolated value on position x.
  float GetInterpolated(float x) const;

  /// Adds value to the curve. AddValue must be called with increasing x  
  void AddValue(float x, float val);
 
  /// Compact data
  void Compact() {_x.Compact();_y.Compact();};

  ClassIsGeneric(LinCurve1D);
};

/// Interpolated 3D curve. Interpolation is separated in a and y coord and is provided by Curve1D
template <class Curve1D> 
class Curve3D : public RefCount
{
protected:
  Curve1D _x;
  AutoArray<Curve1D> _yz;

public:
  /// Returns interpolated value...
  virtual float GetInterpolated(float x, float y) const;
};

template <class Curve1D>
float Curve3D<Curve1D>::GetInterpolated(float x, float y) const
{
  float posX = _x.GetInterpolated(x);

  int row0 = toIntFloor(posX);
  int row1 = row0 + 1;
  float coefrow1 = posX - row0;
  float coefrow0 = 1 - coefrow1;

  float ret = 0; 
  if (row0 >= 0 && row0 < _yz.Size())  
    ret = _yz[row0].GetInterpolated(y) * coefrow0;

  if (row1 >= 0 && row1 < _yz.Size())  
    ret += _yz[row1].GetInterpolated(y) * coefrow1;

  //LogF("row0 %d, row1 %d, coefrow0 %lf, coefrow1 %lf", row0, row1, coefrow0, coefrow1);
  return ret;
};

/// 3D curve lineary interpolated and initialized from config.
class ConfigCurve3D : public Curve3D<LinCurve1D> 
{
  typedef Curve3D<LinCurve1D> base;
protected:
  bool _loaded;
public:
  ConfigCurve3D() : _loaded(false) {};

  /// Loads curve from Param File
  bool Load(ParamEntryPar par);

  /// Returns interpolated value if curve was loaded, otherwise it return y.
  virtual float GetInterpolated(float x, float y) const;
};

#endif //_CURVE3D_HPP
