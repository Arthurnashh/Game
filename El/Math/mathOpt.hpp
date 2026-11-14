#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MATHOPT_HPP
#define _MATHOPT_HPP

#define INV_EPS ( 1e-4 )
#define INVSQRT_EPS ( 1e-3 )

#include "mathDefs.hpp"
#include <math.h>
#include <Es/Common/fltopts.hpp>

/// convert degree to radian
static inline Coord HDegree( Coord angle )
{
	return angle*coord(H_PI/180);
}

/// sign of a float as a float
static inline float fSign( float x )
{
	if( x>0 ) return +1.0;
	if( x<0 ) return -1.0;
	return 0;
}

/// sign of a float as an integer
static inline int sign( float x )
{
	if( x>0 ) return +1;
	if( x<0 ) return -1;
	return 0;
}

/// binary representation for FLT_SNAN
extern const int SNanIEEE32b;

/// signaling NaN used for initialization
#define FLT_SNAN (*(float *)&SNanIEEE32b)


#if !USING_SSE

static __forceinline float Inv( float c ) {return 1/c;}

float InvSqrt( float c );

static inline float Square( float x ) {return x*x;}

#else

// consider: NewtonRaphson Reciprocal 
//   [2 * rcpps(x) - (x * rcpps(x) * rcpps(x))]
__forceinline float Inv( float c ) {return 1/c;}
/*
inline float Inv( float f )
{
	__m128 ff=_mm_set_ss(f);
	__m128 ra0 = _mm_rcp_ss(ff);
	__m128 res=_mm_sub_ss(_mm_add_ss(ra0, ra0), _mm_mul_ss(_mm_mul_ps(ra0, ff), ra0));
	float ret;
	_mm_store_ss(&ret,res);
	return ret;
}
*/


float InvSqrt( float f );
static __forceinline float Square( float x ) {return x*x;}


#endif

inline float InvSqrtFast(float x)
{
	float xhalf = 0.5f*x;
	union
	{
		float x;
		int i;
	} u;
	u.x = x;
	u.i = 0x5f3759df - (u.i >> 1);
	x = u.x * (1.5f - xhalf * u.x * u.x);
	return x;
}

#endif
