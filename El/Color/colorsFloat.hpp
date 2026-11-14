#ifdef _MSC_VER
#pragma once
#endif

#ifndef _COLORSFLOAT_HPP
#define _COLORSFLOAT_HPP

#include <Es/Common/fltopts.hpp>

#define R_EYE 0.299f
#define G_EYE 0.587f
#define B_EYE 0.114f

// passing by reference preferred
#define ColorVal const ColorP &

enum _noInitColor {NoInitColor};

class ColorP
{
	// color used for shading calculations
	private:
	// valid color is in range <0,1)
	float _r,_g,_b,_a;
	
	public:
	__forceinline ColorP(){} // default is uninitialized
	__forceinline ColorP( _noInitColor ){} // default is uninitialized
	/*
	__forceinline ColorP( enum _black )
	{
		_r=_g=_b=0;
		_a=1;
	}
	__forceinline ColorP( enum _white )
	{
		_r=_g=_b=1;
		_a=1;
	}
	*/
	__forceinline ColorP( float r, float g, float b )
	{
		_r=r;
		_g=g;
		_b=b;
		_a=1;
	}
	__forceinline ColorP( float r, float g, float b, float a )
	{
		_r=r;
		_g=g;
		_b=b;
		_a=a;
	}
	explicit ColorP( long rgb )
	{
		_a=((rgb>>24)&0xff)*(1/255.0f);
		_r=((rgb>>16)&0xff)*(1/255.0f);
		_g=((rgb>>8)&0xff)*(1/255.0f);
		_b=((rgb>>0)&0xff)*(1/255.0f);
	}
	__forceinline void SetR(float r) {_r=r;}
	__forceinline void SetG(float g) {_g=g;}
	__forceinline void SetB(float b) {_b=b;}
	__forceinline void SetA(float a) {_a=a;}
	__forceinline float R() const {return _r;}
	__forceinline float G() const {return _g;}
	__forceinline float B() const {return _b;}
	__forceinline float A() const {return _a;}
	
	const float *GetArray() const {return &_r;}
	
	__forceinline int R8() const {return toInt(_r*255);}
	__forceinline int G8() const {return toInt(_g*255);}
	__forceinline int B8() const {return toInt(_b*255);}
	__forceinline int A8() const {return toInt(_a*255);}
	
  bool operator == (ColorVal op) const
  {
    return op.R()==R() && op.G()==G() && op.B()==B() && op.A()==A();
  }
  bool operator != (ColorVal op) const
  {
    return op.R()!=R() || op.G()!=G() || op.B()!=B() || op.A()!=A();
  }
	ColorP operator *( ColorVal op ) const
	{
		return ColorP(_r*op._r,_g*op._g,_b*op._b,_a*op._a);
	}
	ColorP operator /( ColorVal op ) const
	{
		return ColorP(_r/op._r,_g/op._g,_b/op._b,_a/op._a);
	}
	ColorP operator *( float c ) const
	{
		return ColorP(_r*c,_g*c,_b*c,_a*c);
	}
	__forceinline ColorP &operator +=( ColorVal op )
	{
		_r+=op._r;
		_g+=op._g;
		_b+=op._b;
		_a+=op._a;
		return *this;
	}

	ColorP operator +( ColorVal op ) const
	{
		return ColorP(_r+op._r,_g+op._g,_b+op._b,_a+op._a);
	}
	ColorP operator -( ColorVal op ) const
	{
		return ColorP(_r-op._r,_g-op._g,_b-op._b,_a-op._a);
	}
	float Brightness() const
	{
		return _r*R_EYE+_g*G_EYE+_b*B_EYE;
	}
  float SquareSize() const
  {
    return _r * _r + _g * _g + _b * _b + _a * _a;
  }
  
  // Do the special kind of normalization of the color - increment alpha in case the color is over 1
  // (increment alpha instead of having color over 1)
  void NormalizeColor()
  {
    float maxColor = floatMax(_r, _g, _b);
    if (_a > 0.0f)
    {
      float maxPossibleAlphaCoef = 1.0f / _a;
      float coef = floatMin(maxColor, maxPossibleAlphaCoef);
      if (coef > 0.0f)
      {
        _r /= coef;
        _g /= coef;
        _b /= coef;
        _a *= coef;
      }
    }
  }

	void SaturateZero()
	{
		saturateMax(_r,0.0f);
		saturateMax(_g,0.0f);
		saturateMax(_b,0.0f);
		saturateMax(_a,0.0f);
	}
	void SaturateMax(ColorVal sat)
	{
		saturateMax(_r,sat._r);
		saturateMax(_g,sat._g);
		saturateMax(_b,sat._b);
		saturateMax(_a,sat._a);
	}
	void Saturate()
	{
		saturateAbove(_r,1.0f);
		saturateAbove(_g,1.0f);
		saturateAbove(_b,1.0f);
		// alpha never overflows
		saturate(_a,0.0f,1.0f);
	}
	void SaturateMinMax()
	{
		saturate(_r,0.0f,1.0f);
		saturate(_g,0.0f,1.0f);
		saturate(_b,0.0f,1.0f);
		saturate(_a,0.0f,1.0f);
	}
	void SaturateRGBMinMax(float vMin, float vMax)
	{
		saturate(_r,vMin,vMax);
		saturate(_g,vMin,vMax);
		saturate(_b,vMin,vMax);
	}
};

extern const ColorP HBlackP;
extern const ColorP HWhiteP;

inline int PackColorComponent(float c)
{
  int ci = toInt(c*255);
  saturate(ci,0,255);
  return ci;
}

class PackedColor
{ // packed color - for efficient storing, no calculations
  typedef unsigned long IntType;

	IntType _value;

	public:
	__forceinline PackedColor(){}
	__forceinline PackedColor( int r, int g, int b, int a ) // no clipping
	{
		_value=(a<<24)|(r<<16)|(g<<8)|b;
	}
	__forceinline explicit PackedColor( IntType value ) {_value=value;}
	explicit PackedColor( ColorVal color ) // full clipping
	{	// convert with saturation
		int r=PackColorComponent(color.R());
		int g=PackColorComponent(color.G());
		int b=PackColorComponent(color.B());
		int a=PackColorComponent(color.A());
		_value=(a<<24)|(r<<16)|(g<<8)|b;
	}
	/// decrease RGB intensity 2x, keep alpha
	PackedColor HalfRGB() const
	{
	  return PackedColor(((_value&0xfefefe)>>1)|(_value&0xff000000));
	}
	void SetRGB(IntType rgb){_value=(_value&~0xffffff)|(rgb&0xffffff);}
	void SetA8( int val ) {_value=(_value&0xffffff)|(val<<24);}
	__forceinline int A8() const {return (_value>>24)&0xff;}
	__forceinline int R8() const {return (_value>>16)&0xff;}
	__forceinline int G8() const {return (_value>>8)&0xff;}
	__forceinline int B8() const {return (_value>>0)&0xff;}
	operator ColorP () const
	{
		return ColorP(R8()*(1.0f/255),G8()*(1.0f/255),B8()*(1.0f/255),A8()*(1.0f/255));
	}
	__forceinline operator IntType () const {return _value;}
	friend PackedColor PackedColorRGB( ColorVal rgb, int a );
	friend PackedColor PackedColorRGB( PackedColor rgb, int a );
};

inline PackedColor PackedColorRGB( ColorVal color, int a=255 )
{
	int r=PackColorComponent(color.R());
	int g=PackColorComponent(color.G());
	int b=PackColorComponent(color.B());
	PackedColor ret;
	ret._value=(a<<24)|(r<<16)|(g<<8)|b;
	return ret;
}


inline PackedColor PackedColorRGB( PackedColor color, int a=255 )
{
	PackedColor ret;
	ret._value=(color._value&0xffffff)|(a<<24);
	return ret;
}

#define PackedWhite   PackedColor(0xffffffff)
#define PackedBlack   PackedColor(0xff000000)
#define PackedGreen   PackedColor(0xff00ff00)
#define PackedRed     PackedColor(0xffff0000)
#define PackedBlue    PackedColor(0xff0000ff)
#define PackedOrange  PackedColor(0xffe9ab17)

// binary copy will do
TypeIsSimple(ColorP);
TypeIsSimple(PackedColor);

#endif

