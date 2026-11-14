//-----------------------------------------------------------------------------
// File: enf_color.h
//
// Desc: Colorspace library
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------
#pragma once
#ifndef ENF_COLOR_H
#define ENF_COLOR_H

#include <float.h>
#include <math.h>
#include "enf_config.h"
#include "common/enf_inttypes.h"

namespace enf
{
/** @addtogroup Common
@{*/

struct Color;

//!Scale from 0...255 to 0...1 representation
const float FCOLOR_SCALE = (1.0f / 255.0f);

const float FCOLOR_R_WEIGHT = 0.3f;
const float FCOLOR_G_WEIGHT = 0.59f;
const float FCOLOR_B_WEIGHT = 0.11f;


//-----------------------------------------------------------------------------
//! Colorspace math in floats
//-----------------------------------------------------------------------------
struct FColor
{
public:
	//!color data
	union
	{
		float colors[4];				///< array of color component

		struct
		{
			float r, g, b, a;			///< names of color component
		};
	};


	//-----------------------------------------------------------------------------
	//!empty constructor. This *DON'T* initialize color!
	FColor()
	{
#ifdef _DEBUG
		//let the uninitialized variables to eat the shit
		*(int*)&r = 0x7fc00000;
		*(int*)&g = 0x7fc00000;
		*(int*)&b = 0x7fc00000;
		*(int*)&a = 0x7fc00000;
#endif
	}


	//-----------------------------------------------------------------------------
	/*!
	constructor
	\param ir	red 
	\param ig	green
	\param ib	blue
	\param ia	alpha 
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE FColor(float ir, float ig, float ib, float ia): r(ir), g(ig), b(ib), a(ia) {}


	//-----------------------------------------------------------------------------
	/*!
	constructor
	\param col	pointer to float array  
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE FColor(const float *col): r(col[0]), g(col[1]), b(col[2]), a(col[3]) {}


	//-----------------------------------------------------------------------------
	/*!
	constructor, to <0..255>
	\param argb	packed colors into 32bit uinteger
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE FColor(uint argb)
	{
		r = (float)((argb >> 16) & 255) * FCOLOR_SCALE;
		g = (float)((argb >> 8) & 255) * FCOLOR_SCALE;
		b = (float)(argb & 255) * FCOLOR_SCALE;
		a = (float)(argb >> 24) * FCOLOR_SCALE;
	}


	//-----------------------------------------------------------------------------
	/*!
	constructor, to <0..1>
	\param rgba
	Array of 8bit colors
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE FColor(const uchar *rgba)
	{
		r = (float) rgba[0] * FCOLOR_SCALE;
		g = (float) rgba[1] * FCOLOR_SCALE;
		b = (float) rgba[2] * FCOLOR_SCALE;
		a = (float) rgba[3] * FCOLOR_SCALE;
	}


	ENF_INLINE FColor(Color color);
	ENF_INLINE FColor& operator=(Color color);

	ENF_INLINE Color GetColorSwapRB() const;

	//-----------------------------------------------------------------------------
	/*!
	Pack color data to integer, must be in <0..255>
	\return packed data
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE uint PackToInt()
	{
		return (((int)a & 0xff) << 24) | (((int)r & 0xff) << 16) | (((int)g & 0xff) << 8) | ((int)b & 0xff);
	}


	//! Zeroes content
	ENF_INLINE FColor Zero()
	{
		r = g = b = a = 0.0f;
		return *this;
	}

	//!Returns true if values are zero
	ENF_INLINE bool IsZero() const
	{
		return (r == 0.0f && g == 0.0f && b == 0.0f && a == 0.0f);
	}

	bool IsEqual(const FColor& color, float epsilon = 0.0f) const
	{
		return (fabs(color.r - r) <= epsilon && fabs(color.g - g) <= epsilon && fabs(color.b - b) <= epsilon && fabs(color.a - a) <= epsilon);
	}

	//-----------------------------------------------------------------------------
	/*!
	clamp color data to interval <0..max>
	\param max	max value for clamping
	\return		clamped data
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE FColor Clamp(float max) const
	{

		//FUCKING error in 2003 compiler, the stack is incorrect in inline function when using SSE intrinsic
		#if 0
			FColor out;
			//#ifdef ENF_SSE
			__m128 rcol = _mm_loadu_ps(colors);
			__m128 rhi = _mm_set1_ps(max);
			__m128 rlo = _mm_setzero_ps();

			rcol = _mm_min_ps(rcol, rhi);
			rcol = _mm_max_ps(rcol, rlo);

			_mm_storeu_ps(out.colors, rcol);
			return out;
		#else
			return FColor(enf_limit(r, 0.0f, max), enf_limit(g, 0.0f, max), enf_limit(b, 0.0f, max), enf_limit(a, 0.0f, max));
		#endif

	}



	//-----------------------------------------------------------------------------
	/*!
	clamp color data to interval <min..max>
	\param min	min value for clamping
	\param max	max value for clamping
	\return		clamped data
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE FColor Clamp(float min, float max) const
	{
		#if 0
		//#ifdef ENF_SSE
			FColor out;
			__m128 rcol = _mm_loadu_ps(colors);
			__m128 rhi = _mm_set1_ps(max);
			__m128 rlo = _mm_set1_ps(min);

			rcol = _mm_min_ps(rcol, rhi);
			rcol = _mm_max_ps(rcol, rlo);

			_mm_storeu_ps(out.colors, rcol);
			return out;
		#else
			return FColor(enf_limit(r, min, max), enf_limit(g, min, max), enf_limit(b, min, max), enf_limit(a, min, max));
		#endif

	}


	//-----------------------------------------------------------------------------	
	/*!
	linear interpolation between two colors
	\param col		second color
	\param lerp		parametr
	\return			this + (col - this)*lerp
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE FColor Lerp(const FColor& col, float lerp) const
	{
		#if 0
		//#ifdef ENF_SSE
			FColor res;
			__m128 r2 = _mm_loadu_ps(col.colors);
			__m128 scale = _mm_set1_ps(lerp);
			__m128 r1 = _mm_loadu_ps(colors);
			r2 = _mm_sub_ps(r2, r1);
			r2 = _mm_mul_ps(r2, scale);
			r2 = _mm_add_ps(r2, r1);
			_mm_storeu_ps(res.colors, r2);
			return res;
		#else
			return FColor((col.r - r) * lerp + r, (col.g - g) * lerp + g, (col.b - b) * lerp + b, (col.a - a) * lerp + a);
		#endif

	}

	//! Returns luminance of color
	ENF_INLINE float GetLuminance() const
	{
		return r * FCOLOR_R_WEIGHT + g * FCOLOR_G_WEIGHT + b * FCOLOR_B_WEIGHT;
	}


	//! Multiply and addition
	ENF_INLINE FColor MA(const FColor& mult, const FColor& add) const
	{
		return FColor(r * mult.r + add.r, g * mult.g + add.g, b * mult.b + add.b, a * mult.a + add.a);
	}

	//-----------------------------------------------------------------------------
	/*!
	operator [], get color component
	\param index	index of component
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE float operator[](int index) const
	{
		enf_assert(index >= 0 && index <= 3);
		return colors[index];
	}

	//-----------------------------------------------------------------------------
	/*!
	operator *, scaled color
	\param scale	scale parameter
	\return			scale*this
	*/
	//-----------------------------------------------------------------------------
	FColor operator*(float scale) const
	{
		#if 0
			FColor out;
		//#ifdef ENF_SSE
			__m128 rcol = _mm_loadu_ps(colors);
			__m128 rscale = _mm_set1_ps(scale);

			_mm_storeu_ps(out.colors, _mm_mul_ps(rcol, rscale));
			return out;
		#else
			return FColor(r * scale, g * scale, b * scale, a * scale);
		#endif
	}


	//-----------------------------------------------------------------------------
	/*!
	operator +, add two colors
	\param p		color
	\return		this + p
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE FColor operator+(const FColor& p) const
	{

#if 0
		FColor out;
		//#ifdef ENF_SSE
		__m128 rcol1 = _mm_loadu_ps(colors);
		__m128 rcol2 = _mm_loadu_ps(p.colors);
		_mm_storeu_ps(out.colors, _mm_add_ps(rcol1, rcol2));
		return out;
#else
		return FColor(r + p.r, g + p.g, b + p.b, a + p.a);
#endif

	}

	ENF_INLINE FColor operator+=(const FColor& p)
	{
		r += p.r; g += p.g; b += p.b; a += p.a;
		return *this;
	}

	ENF_INLINE FColor operator-=(const FColor& p)
	{
		r -= p.r; g -= p.g; b -= p.b; a -= p.a;
		return *this;
	}
	
	//-----------------------------------------------------------------------------
	/*!
	operator -, subtract two colors
	\param p		color
	\return		this - p
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE FColor operator-(const FColor& p) const
	{
		#if 0
		//#ifdef ENF_SSE
			FColor out;
			__m128 rcol1 = _mm_loadu_ps(colors);
			__m128 rcol2 = _mm_loadu_ps(p.colors);
			_mm_storeu_ps(out.colors, _mm_sub_ps(rcol1, rcol2));
			return out;
		#else
			return FColor(r - p.r, g - p.g, b - p.b, a - p.a);
		#endif
	}


	//-----------------------------------------------------------------------------
	/*!
	operator *, multiply two colors
	\param p		color
	\return		this * p
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE FColor operator*(const FColor& p) const
	{
		#if 0
		//#ifdef ENF_SSE
			FColor out;
			__m128 rcol1 = _mm_loadu_ps(colors);
			__m128 rcol2 = _mm_loadu_ps(p.colors);
			_mm_storeu_ps(out.colors, _mm_mul_ps(rcol1, rcol2));
			return out;
		#else
			return FColor(r * p.r, g * p.g, b * p.b, a * p.a);
		#endif
	}
	
	ENF_INLINE FColor operator/(const FColor& p) const
	{
		return FColor(r / p.r, g / p.g, b / p.b, a / p.a);
	}



	//-----------------------------------------------------------------------------
	/*!
	operator =, set colors
	\param p		color
	\return		this
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE FColor& operator=(const FColor& p)
	{
		r = p.r; g = p.g; b = p.b; a = p.a;
		return *this;
	}


	//-----------------------------------------------------------------------------
	/*!
	operator =, set colors
	\param p		pointer to color
	\return		this
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE FColor& operator=(const float* p)
	{
		r = p[0]; g = p[1]; b = p[2]; a = p[3];
		return *this;
	}


	//-----------------------------------------------------------------------------
	/*!
	operator ==, compare colors
	\param p		color
	\return		this == p
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE bool operator==(const FColor& p) const
	{
		return (r == p.r && g == p.g && b == p.b && a == p.a);
	}


	//-----------------------------------------------------------------------------
	/*!
	operator !=, compare colors
	\param p		color
	\return		this != p
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE bool operator!=(const FColor& p) const
	{
		return (r != p.r || g != p.g || b != p.b || a != p.a);
	}
};



//-----------------------------------------------------------------------------
/*!
Help function for constructing FColor from just one value
\param f
value
\return
FColor with r = g = b = a = f;
*/
//-----------------------------------------------------------------------------
ENF_INLINEFUNC FColor FColor1(float f)
{
	return FColor(f, f, f, f);
}


//-----------------------------------------------------------------------------
//! Color-space math in bytes
//-----------------------------------------------------------------------------
struct Color
{
public:

	//!color data
	union
	{
		struct
		{
#if ENF_INTEGER_BIG_ENDIAN
			ubyte a, r, g, b;		///< names of components
#else
			ubyte b, g, r, a;		///< names of components
#endif
		};

		ubyte colors[4];			///< array of components
		uint	rgba;					///< packed data
	};


	//-----------------------------------------------------------------------------
	/*!
	constructor
	\param ir	red 
	\param ig	green
	\param ib	blue
	\param ia	alpha 
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE Color(ubyte ir, ubyte ig, ubyte ib, ubyte ia): r(ir), g(ig), b(ib), a(ia) {}


	//-----------------------------------------------------------------------------
	/*!
	constructor
	\param irgba	packed color data 
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE Color(uint irgba): rgba(irgba) {}

	ENF_INLINE bool IsZero() const
	{
		return rgba == 0;
	}


	//-----------------------------------------------------------------------------
	/*!
	constructor, from floats <0..1>
	\param ir	red 
	\param ig	green
	\param ib	blue
	\param ia	alpha 
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE Color(float ir, float ig, float ib, float ia)
	{
		r = (ubyte)(ir * 255.0f);
		g = (ubyte)(ig * 255.0f);
		b = (ubyte)(ib * 255.0f);
		a = (ubyte)(ia * 255.0f);
	}


	//-----------------------------------------------------------------------------
	/*!
	constructor from FColor
	\param src
	Source FColor we are creating from
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE Color(const FColor &src)
	{
		r = (ubyte)(src.r * 255.0f);
		g = (ubyte)(src.g * 255.0f);
		b = (ubyte)(src.b * 255.0f);
		a = (ubyte)(src.a * 255.0f);
	}


	//-----------------------------------------------------------------------------
	/*!
	empty constructor
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE Color()
	{
#ifdef _DEBUG
		rgba = 0xbaadf00d;
#endif
	}


	//-----------------------------------------------------------------------------
	/*!
	swap red and blue value, due to texture format A8R8G8B8 and A8B8G8R8
	\return swapped color
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE Color& SwapRB()
	{
		enf_swap<>(r, b);
		return *this;
	}

	//-----------------------------------------------------------------------------
	/*!
	get color in swapped format 
	\return swapped color
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE Color GetABGR()
	{
		return Color(b, g, r, a);
	}

	//-----------------------------------------------------------------------------
	/*!
	operator =, set color 
	\param p		color
	*/
	//-----------------------------------------------------------------------------
	void operator=(Color p)
	{
		rgba = p.rgba;
	}


	//-----------------------------------------------------------------------------
	/*!
	operator =, set color 
	\param p		color in <0..255>
	*/
	//-----------------------------------------------------------------------------
	void operator=(FColor p)
	{
		r = (ubyte)p.r;
		g = (ubyte)p.g;
		b = (ubyte)p.b;
		a = (ubyte)p.a;
	}


	//-----------------------------------------------------------------------------
	/*!
	operator -, subtract two colors, TODO: optimize MMX
	\param p		color in <0..255>
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE Color&  operator-(const Color &p)
	{
		r = (ubyte)enf_max((int)r - (int)p.r, 0);
		g = (ubyte)enf_max((int)g - (int)p.g, 0);
		b = (ubyte)enf_max((int)b - (int)p.b, 0);
		a = (ubyte)enf_max((int)a - (int)p.a, 0);
		return *this;
	}

	//-----------------------------------------------------------------------------
	/*!
	operator +, add two colors, TODO: optimize MMX
	\param p		color in <0..255>
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE Color& operator+(const Color &p)
	{
		r = (ubyte)enf_min((int)r + (int)p.r, 255);
		g = (ubyte)enf_min((int)g + (int)p.g, 255);
		b = (ubyte)enf_min((int)b + (int)p.b, 255);
		a = (ubyte)enf_min((int)a + (int)p.a, 255);
		return *this;
	}

	ENF_INLINE Color& operator*(const Color &p)
	{
		uint rgba1 = (rgba & 0x00ff00ff) * (p.rgba & 0x00ff00ff);
		uint rgba2 = ((rgba & 0xff00ff00) >> 8) * ((p.rgba & 0xff00ff00) >> 8);
		rgba = ((rgba1 >> 8) & 0x00ff00ff) | (rgba2 & 0xff00ff00);
		return *this;
	}
};

ENF_INLINE FColor::FColor(Color color):
	r((float)color.r * FCOLOR_SCALE),
	g((float)color.g * FCOLOR_SCALE),
	b((float)color.b * FCOLOR_SCALE),
	a((float)color.a * FCOLOR_SCALE)
{
}

ENF_INLINE FColor& FColor::operator=(Color color)
{
	r = (float)color.r * FCOLOR_SCALE;
	g = (float)color.g * FCOLOR_SCALE;
	b = (float)color.b * FCOLOR_SCALE;
	a = (float)color.a * FCOLOR_SCALE;
	return *this;
}
ENF_INLINE Color FColor::GetColorSwapRB() const
{
	return Color(
		(ubyte)(b * 255.0f),
		(ubyte)(g * 255.0f),
		(ubyte)(r * 255.0f),
		(ubyte)(a * 255.0f));
}

typedef const FColor& FColorPar;
typedef const Color& ColorPar;
//@}
}
#endif //ENF_COLOR