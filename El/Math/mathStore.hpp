#ifdef _MSC_VER
#pragma once
#endif

/** file
Compressed storage of various Math3D types
*/

#ifndef _EL_MATH_STORE_HPP
#define _EL_MATH_STORE_HPP

#include "math3d.hpp"

/// 16-b fixed point representation of float
template <int PrecBits>
class Float16bFixed
{
  short _value;

  public:
  Float16bFixed(){}
  explicit Float16bFixed(float val)
  {
    int v = toInt(val*(1<<PrecBits)); 
    Assert(v>=-0x7fff && v<0x7fff);
    _value = v;
  }
  operator float() const
  {
    return _value*(1.0f/(1<<PrecBits));
  }
};

/// 16-b exponent+mantissa representation of float
/**
The IEEE single precision floating point standard representation requires a 32 bit word, which may be represented as numbered from 0 to 31, left to right. The first bit is the sign bit, S, the next eight bits are the exponent bits, 'E', and the final 23 bits are the fraction 'F':

  S EEEEEEEE FFFFFFFFFFFFFFFFFFFFFFF
  0 1      8 9                    31

The value V represented by the word may be determined as follows:

If E=255 and F is nonzero, then V=NaN ("Not a number") 
If E=255 and F is zero and S is 1, then V=-Infinity 
If E=255 and F is zero and S is 0, then V=Infinity 
If 0<E<255 then V=(-1)**S * 2 ** (E-127) * (1.F) where "1.F" is intended to represent the binary number created by prefixing F with an implicit leading 1 and a binary point. 
If E=0 and F is nonzero, then V=(-1)**S * 2 ** (-126) * (0.F) These are "unnormalized" values. 
If E=0 and F is zero and S is 1, then V=-0 
If E=0 and F is zero and S is 0, then V=0 
*/

template <int MantBits>
class Float16b
{
  unsigned short _value;

  enum {ExpoBits=15-MantBits};

  public:
  Float16b()
  {
  }
  explicit Float16b(float val)
  {
    // we can forget:
    // most important bits of exponents
    // least important bits of mantissa
    unsigned raw = *(unsigned *)&val;
    // handle special case - zero
    // note: perhaps we should handle all denormals this way
    /*
    if (raw==0)
    {
      _value = 0;
      return;
    }
    */
    unsigned sign = raw&0x80000000; // 1b
    unsigned expo = raw&0x7f800000; // 8b
    //unsigned expoSign = raw&0x40000000; // 8b
    unsigned frac = raw&0x007fffff; // 23b
    #if _DEBUG
      unsigned signO = sign;
      unsigned expoO = expo;
      unsigned fracO = frac;
      (void)signO;(void)expoO;(void)fracO;
    #endif
    
    // calculate real exponent value
    int exp = (expo>>23)-0x80;
    // calculate minimal exponent we are able to display
    exp += (1<<(ExpoBits-1));
    //const int minExp = ((1<<(ExpoBits-1))-(1<<(ExpoBits-2)))<<(8-ExpoBits);
    if (exp<=0) // underflow
    {
      // check if exponent is so low that the result will be denormal or zero
      // if yes, flush to zero
      _value = 0;
    }
    else if (exp>=(1<<ExpoBits)) // overflow
    {
      _value = (sign>>16)|0x7fff;
      Fail("Overflow");
    }
    else
    {
      Assert(exp<(1<<ExpoBits));
      frac >>= 23-MantBits;
      expo = exp<<MantBits;

      sign >>= 16;
      Assert((sign&expo)==0);
      Assert((sign&frac)==0);
      Assert((frac&expo)==0);
      _value = sign|expo|frac;
    }
    
    #if _DEBUG
      float test = 0;
      if (_value)
      {
        unsigned signT = _value&0x8000;
        unsigned fracT = _value&((1<<MantBits)-1);
        unsigned expoT = _value&(((1<<ExpoBits)-1)<<MantBits);
        fracT <<= 23-MantBits;
        expoT -= (1<<(ExpoBits-1))<<MantBits;
        expoT <<= 16+ExpoBits-8;
        expoT += 0x40000000;
        expoT &= 0x7f800000;
        signT <<= 16;
        unsigned rawT = signT|expoT|fracT;
        test = *(float *)&rawT;
      }
      // check if value does not differ too much
      Assert (fabs(test-val)<fabs(0.01f*val) || fabs(test-val)<MinValue());
      //LogF("Check %10g ~ %10g",val,T);
    #endif

  }
  operator float() const
  {
    if (_value==0) return 0;
    unsigned sign = _value&0x8000;
    unsigned frac = _value&((1<<MantBits)-1);
    unsigned expo = _value&(((1<<ExpoBits)-1)<<MantBits);
    frac <<= 23-MantBits;
    expo -= (1<<(ExpoBits-1))<<MantBits;
    expo <<= 16+ExpoBits-8;
    expo += 0x40000000;
    sign <<= 16;
    unsigned raw = sign|expo|frac;
    return *(float *)&raw;
  }
  static Float16b<10> MaxValue()
  {
    Float16b<10> ret;
    ret._value = 0x7fff;
    return ret;
  }
  static Float16b<10> MinValue()
  {
    Float16b<10> ret;
    // zero mantisa, exponent 1
    ret._value = 1<<MantBits;
    return ret;
  }
};

/// Quaternion representation of 3x3 matrix
/**
float is always used for calculation, Number is used for storage only
*/

template <class Number>
class Quaternion
{
  typedef float Numeric;
  //float _a,_b,_c,_d;
  Number _x,_y,_z,_w;

public:
  Quaternion(){}
  Quaternion(Numeric x, Numeric y, Numeric z, Numeric w)
  :_x(x),_y(y),_z(z),_w(w)
  {
  }
  explicit Quaternion(Matrix3Par m)
  {
    FromMatrixRotationScale(m);
  }
  ///\param axis unit vector
  ///\param angle angle in radians
  explicit Quaternion(const Vector3& axis, Numeric angle)
  {
    FromAxisAngle(axis, angle);
  }
  /// convert from matrix, the matrix is known to be rotation only
  void FromMatrixRotation(Matrix3Par m);
  /// convert from matrix, the matrix is can contain rotation and scale
  void FromMatrixRotationScale(Matrix3Par m);
  
  /// generic quaternion to matrix conversion
  template <class Matrix>
  void ToMatrix(Matrix &m) const;
  /// if quaternion is unit, conversion can be faster
  void UnitToMatrix(Matrix3 &m) const;
  operator Matrix3() const
  {
    // we assume Named Return Value Optimization should be done here
    Matrix3 m; //(NoInit);
    ToMatrix(m);
    return m;
  }
  Numeric InvSize() const
  {
    Numeric x = X(), y = Y(), z = Z(), w = W();
    return InvSqrt(x*x+y*y+z*z+w*w);
  }
  void operator *= (Numeric c)
  {
    _x = Number(_x*c);
    _y = Number(_y*c);
    _z = Number(_z*c);
    _w = Number(_w*c);
  }
  Numeric DotProduct( const Quaternion &q2 ) const
  {
    return W()*q2.W() + X()*q2.X() + Y()*q2.Y() + Z()*q2.Z();
  }
  Quaternion operator * (Numeric c) const
  {
    return Quaternion(X()*c,Y()*c,Z()*c,W()*c);
  }
  Quaternion operator +( const Quaternion &b ) const
  {
    return Quaternion(X()+b.X(),Y()+b.Y(),Z()+b.Z(),W()+b.W());
  }
  Quaternion operator -( const Quaternion &b ) const
  {
    return Quaternion(X()-b.X(),Y()-b.Y(),Z()-b.Z(),W()-b.W());
  }
  Quaternion operator -() const
  {
    return Quaternion(-X(),-Y(),-Z(),-W());
  }
  Quaternion operator *( const Quaternion &b ) const
  {
    return Quaternion(
      W()*b.X() + X()*b.W() + Y()*b.Z() - Z()*b.Y(),
      W()*b.Y() + Y()*b.W() + Z()*b.X() - X()*b.Z(),
      W()*b.Z() + Z()*b.W() + X()*b.Y() - Y()*b.X(),       
      W()*b.W() - X()*b.X() - Y()*b.Y() - Z()*b.Z()
      );
  }

  bool operator ==( const Quaternion &b ) const {return _x==b._x && _y==b._y && _z==b._z && _w==b._w;}

  /// normalized linear interpolation - much faster than Slerp, not constant velocity
  Quaternion Nlerp(const Quaternion &b, Numeric bFactor) const
  {
    if (DotProduct(b)<0)
    { // negate b
      Quaternion r = (*this) - (b + *this) * bFactor;
      return r.Normalized();
    }
    else
    {
      Quaternion r = (*this) + (b - *this) * bFactor;
      return r.Normalized();
    }
  }
  /// spherical linear interpolation
  Quaternion Slerp(const Quaternion &q2, Numeric factor) const
  {
		Quaternion q;
		float fsin, finvsin, c0, c1;
    // Compute the cosine of the angle between the two vectors.
		float fcos = DotProduct(q2);
		float afcos=fabs(fcos);

		if ( (1-afcos)> 0.01f)
    {
			float angle = acos(afcos); // angle between input vectors
			fsin = sin(angle);
			finvsin = 1.0f/fsin;
			c0 = sin((1.0f-factor)*angle)*finvsin;
			c1 = sin(factor*angle)*finvsin;

			if (fcos < 0.0) // check for the shortest path
				c0 = -c0;
			return (*this)*c0+q2*c1;
		}
		else
		{
      // If the inputs are too close for comfort, linearly interpolate
      // and normalize the result.
			c0=1.0f-factor;
			c1=factor;

			if (fcos < 0.0) // check for the shortest path
				c0 = -c0;
			return ((*this)*c0+q2*c1).Normalized();
  }
  }
  void Normalize()
  {
    operator *= (InvSize());
  }
  Quaternion Normalized() const
  {
    return (*this)*InvSize();
  }
  
  ///\param axis unit vector
  ///\param angle angle in radians
  void FromAxisAngle(const Vector3& axis, Numeric angle)
  {
    Numeric angleHalf = angle/2;
    Numeric sina = sin(angleHalf);
    _x = axis[0] * sina;
    _y = axis[1] * sina;
    _z = axis[2] * sina;
    _w = cos(angleHalf);
  }

  /// creates quaternion from euler angles (in radians)
  void FromEuler(Numeric angleX, Numeric angleY, Numeric angleZ)
  {
    Numeric angleHalfX = angleX/2;
    Numeric angleHalfY = angleY/2;
    Numeric angleHalfZ = angleZ/2;

    *this = Quaternion(sin(angleHalfX), 0, 0, cos(angleHalfX)) *
      Quaternion(0, sin(angleHalfY), 0, cos(angleHalfY))*
      Quaternion(0, 0, sin(angleHalfZ), cos(angleHalfZ));

  }

  float X() const {return float(_x);}
  float Y() const {return float(_y);}
  float Z() const {return float(_z);}
  float W() const {return float(_w);}
};

template <class Number>
void Quaternion<Number>::FromMatrixRotationScale(Matrix3Par m)
{
  // extract scale first
  // note: Scale is quite slow
  float scale2 = m.Scale2();
  float invScale = InvSqrt(scale2);
  float scale = scale2*invScale;
  // TODO: optimize
  FromMatrixRotation(m*invScale);
  // quaternion scaling?
  operator *=(sqrt(scale));
}

template <class Number>
void Quaternion<Number>::FromMatrixRotation(Matrix3Par m)
{
  const static int nxt[3] = {1, 2, 0};
  
  float tr = m(0,0) + m(1,1) + m(2,2);
  
  // check the diagonal
  if (tr > 0.0f)
  {
    float s = (float)sqrt (tr + 1.0f);
    _w = Number(s*0.5f);
    s = 0.5f / s;
    _x = Number((m(1,2) - m(2,1)) * s);
    _y = Number((m(2,0) - m(0,2)) * s);
    _z = Number((m(0,1) - m(1,0)) * s);
  }
  else
  {		
    float  s, q[4];
  
    // diagonal is negative
    int i = 0;
    if (m(1,1) > m(0,0)) i = 1;
    if (m(2,2) > m(i,i)) i = 2;
    int j = nxt[i];
    int k = nxt[j];
    
    
    s = (float)sqrt ((m(i,i) - (m(j,j) + m(k,k))) + 1.0f);
    
    q[i] = s * 0.5f;
    
    if (s != 0.0f) s = 0.5f / s;
    
    
    q[3] = (m(j,k) - m(k,j)) * s;
    q[j] = (m(i,j) + m(j,i)) * s;
    q[k] = (m(i,k) + m(k,i)) * s;
    
    
    _x = Number(q[0]);
    _y = Number(q[1]);
    _z = Number(q[2]);
    _w = Number(q[3]);
  }
}

/// declare the template
template <class Number>
template <class Matrix>
void Quaternion<Number>::ToMatrix(Matrix &m) const
{
/*
Matrix = [ w2 + x2 - y2 - z2          2xy - 2wz           2xz + 2wy ]
         [ 2xy + 2wz          w2 - x2 + y2 - z2       2yz - 2wx     ]
         [ 2xz - 2wy              2yz + 2wx       w2 - x2 - y2 + z2 ]
*/
  float x = _x;
  float y = _y;
  float z = _z;
  float w = _w;
  
  m(0,0) = w*w + x*x - y*y - z*z;
  m(1,0) = 2*x*y - 2*w*z;
  m(2,0) = 2*x*z + 2*w*y;
  
  m(0,1) = 2*x*y + 2*w*z;
  m(1,1) = w*w - x*x + y*y - z*z;
  m(2,1) = 2*y*z - 2*w*x;
  
  m(0,2) = 2*x*z - 2*w*y;
  m(1,2) = 2*y*z + 2*w*x;
  m(2,2) = w*w - x*x - y*y + z*z;
}

#if defined _MATH3DK_HPP
/// declare SSE/VMX specific specialization
template <>
template <>
void Quaternion<Float16bFixed<14> >::ToMatrix(Matrix3K &m) const;

/// declare SSE/VMX specific specialization
template <>
template <>
void Quaternion<Float16bFixed<14> >::ToMatrix(Matrix4K &m) const;
#endif

template <class Number>
void Quaternion<Number>::UnitToMatrix(Matrix3 &m) const
{
  float x = _x;
  float y = _y;
  float z = _z;
  float w = _w;
  // calculate coefficients
  float x_2 = x + x, y_2 = y + y; 
  float z_2 = z + z;
  float xx_2 = x * x_2, xy_2 = x * y_2, xz_2 = x * z_2;
  float yy_2 = y * y_2, yz_2 = y * z_2, zz_2 = z * z_2;
  float wx_2 = w * x_2, wy_2 = w * y_2, wz_2 = w * z_2;
  
  m(0,0) = 1.0f - (yy_2 + zz_2);
  m(1,0) = xy_2 - wz_2;
  m(2,0) = xz_2 + wy_2;
  
  m(0,1) = xy_2 + wz_2;
  m(1,1) = 1.0f - (xx_2 + zz_2);
  m(2,1) = yz_2 - wx_2;
  
  
  m(0,2) = xz_2 - wy_2;
  m(1,2) = yz_2 + wx_2;
  m(2,2) = 1.0f - (xx_2 + yy_2);
}

/// compressed format for 4x3 matrix
class Matrix4Quat16b
{
  typedef Float16b<10> PosType;
  typedef Float16bFixed<14> OrientType;
  Quaternion<OrientType> _orientation;
  PosType _posX,_posY,_posZ;

  public:
  Matrix4Quat16b(){}
  
  Matrix4Quat16b(Matrix4Par src)
  :_orientation(src.Orientation()),
  _posX(src.Position().X()),
  _posY(src.Position().Y()),
  _posZ(src.Position().Z())
  {
  }
  // extract position
  Vector3 Position() const {return Vector3(_posX,_posY,_posZ);}
  // change position
  void SetPosition(Vector3Par pos)
  {
    _posX = PosType(pos.X());
    _posY = PosType(pos.Y());
    _posZ = PosType(pos.Z());
  }
  /// extract complete 4x3 matrix 
  void ToMatrix(Matrix4 &tgt) const
  {
    _orientation.ToMatrix(tgt);
    tgt.SetPosition(Vector3(_posX,_posY,_posZ));
  }
  /// extract orientation (3x3 matrix)
  void ToMatrix(Matrix3 &tgt) const
  {
    _orientation.ToMatrix(tgt);
  }
  //@{ extract orientation quaternion
  float GetOrientQX() const {return _orientation.X();}
  float GetOrientQY() const {return _orientation.Y();}
  float GetOrientQZ() const {return _orientation.Z();}
  float GetOrientQW() const {return _orientation.W();}
  Quaternion<float> GetOrientQ() const {
    return Quaternion<float>(_orientation.X(),_orientation.Y(),_orientation.Z(),_orientation.W());
  }
  //@}

  __forceinline operator Matrix4() const
  {
    // we assume Named Return Value Optimization should be done here
    Matrix4 matrix;
    ToMatrix(matrix);
    return matrix;
  }
  __forceinline operator Matrix3() const
  {
    // we assume Named Return Value Optimization should be done here
    Matrix3 matrix;
    ToMatrix(matrix);
    return matrix;
  }

  bool operator == (const Matrix4Quat16b &with) const
  {
    return _orientation==with._orientation && _posX==with._posX && _posY==with._posY && _posZ==with._posZ;
  }

  bool operator != (const Matrix4Quat16b &with) const {return !operator ==(with);}
};

TypeIsSimple(Matrix4Quat16b)

#endif
