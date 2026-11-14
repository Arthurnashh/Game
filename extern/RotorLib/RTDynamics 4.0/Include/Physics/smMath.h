/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

#include <math.h>
#include <cmath>
#include <stdlib.h>
#include <float.h>

#include <string>

// Careful about sm includes. Avoid header lock.
#include "Physics/smPrecision.h"

class smAngleD ;
class smAngle ;
class smVec3r;
class smVec3d;

// Return the absolute value of f.
template<class T>
inline T smAbs( T f )
{ return ( f < 0.0f ? -f : f ); }


// Return the larger of the two values. Use this function instead of max (only defined on Windows)
// to maintain multiplatform.
template<class T>
inline T smMax( T a, T b )
{ return ( a > b ? a : b ); }


// Return the lesser of the two values.Use this function instead of max (only defined on Windows)
// to maintain multiplatform.
template<class T>
inline T smMin( T a, T b )
{ return ( a < b ? a : b ); }


// Return (int)(a + 0.5f)
template<class T>
inline int smRound( T a )
{ return int( a + .5f ); }


// Return the square of t.
template<class T>
inline T smSquare( T t )
{ return ( t * t ) ; }

template<class T>
inline T smFreqToPeriodInSeconds( T t )
{ return 1.0f / t ; }

template<class T>
inline T smPeriodInSecondsToFreq( T t )
{ return 1.0f / t ; }

//
//	Converts Ascii string to smReal and uses std::string as input format
//
inline smReal
smAToR( const std::string& str )
{ return static_cast<smReal>( atof( str.c_str () ) ); }

/**
*
*	smMath includes static methods that implement
*	commonly used mathematical operations on scalars.
*
*/
class smMath
{
public:

    /** PI to 10 decimal places. */
    static const smReal sPI ;

    /** 2PI to 10 decimal places. */
    static const smReal sTwoPI ;

    /** PI / 2.0. */
    static const smReal sHalfPI ;

    /** The smallest acceptable 32 bit floating point value. */
    static const smReal sMinFloat ;

    /** The largest acceptable 32 bit floating point value. */
    static const smReal sMaxFloat ;

    /** Values smaller than this are considered to be zero. */
    static const smReal sZeroTol ;

    /** Values smaller than this are considered to be zero. */
    static const smDouble sZeroTolD ;

    /** The smallest acceptable 64 bit floating point value. */
    static const smReal sMinDouble ;

    /** The largest acceptable 64 bit floating point value. */
    static const smReal sMaxDouble ;

    /**  */
    static const smReal sGravity ;

    static smReal cirToX(smReal magnitude, smReal h /* radians */);
    static smReal cirToY(smReal magnitude, smReal h /* radians */);

    /** Returns the magnitude of a vector represented by three floats. */
    static smReal magnitude(smReal x, smReal y, smReal z);

	template<class VectorType>
	static VectorType cross( VectorType left, VectorType right ) 
	{
		VectorType result ;
		result[0] = left[1] * right[2] - left[2] * right[1] ;
		result[1] = left[2] * right[0] - left[0] * right[2] ;
		result[2] = left[0] * right[1] - left[1] * right[0] ;
		return result ;
	}

    /** Checks if a value is between the given min and max values. */
    static bool isBetween	( smReal min, smReal max, smReal val)
	{
		if ( (val > min) && (val<max))
			return true;

		return false;
	}

    static bool isInRect	( smReal centerX, smReal centerY, smReal width, smReal hight, smReal x, smReal y );

    /**
    *	Return the position of the given value "current" between min and max.
    *	If trim is true, when the current value is above max, it is truncated, this
    *	truncation process will take time if the current value is too big.
    **/
    static smReal linearMap( smReal min, smReal max, smReal current, bool trim = true )
    {
		if ( min == max )
			return min ;

        if ( trim )
            while ( current > max )
                current -= max ;

            return static_cast<smReal>( current - min ) / ( max - min ) ;
    }

	/** General purpose simple n-dimensional linear interpolation
	*	function.
	*/
	template<class T>
    static T linearInterp( const T& y1, const T& y2, smReal t, bool trim = true )
    {
		if ( y1 == y2 )
			return y1 ;

        if ( trim )
            if ( t > 1.0f )
                t = 1.0f ;

		return (y2-y1)*t + y1 ;
    }


    /** Generate a random number between 0.0 and 1.0.   */
    static smReal makeNormalizedPosRand( )
    {
        return static_cast<smReal>( rand() ) / static_cast<smReal>( RAND_MAX ) ;
    }

    /** Generate a random number between -1.0 and +1.0. */
    static smReal makeNormalizedRand( )
    {
        return static_cast<smReal>( rand() - RAND_MAX / 2 ) / static_cast<smReal>( RAND_MAX ) ;
    }

    /** Return a converted from degrees to radians. */
	template<class T>
    static T degToRad( const T& a )
    {
        return a * static_cast<T>(0.01745329252222);	// same as : a * ( smPI / 180.0 )
    }

    /** Return a converted from radians to degrees. */
	template<class T>
    static T radToDeg( const T& a )
    {
        return a * static_cast<T>(57.2957795056);		// same as : a * ( 180.0 / smPI )
    }

	/**
	*	Makes sure that the given angle is in [-180,180] range. Returns corrected
	*	angle ;
	*/
	static smReal fitAngleIn180( smReal angle ) ;

    /** This function round x up to the nearest integer. This method exists for multi-platform reasons. */
    static int ceiling( const smReal& a )
    {
        return static_cast<int>( ceil( a ) ) ;
    }

    /** Return the cosine of a which is specified in radians. */
    static smReal cos( const smAngle& a ) ;

    /** Return the sine of a which is specified in radians.   */
    static smReal sin( const smAngle& a ) ;

    /** Return the arc sine of a which is specified in degrees.   */    
	template<class T>
    static T asin( const T& a ) 
	{
		return radToDeg( std::asin( smMin( smMax( a, T(-1.0) ), T(1.0) ) ) );
	}

    /** Return the arc cosine of a which is specified in degrees. */
	template<class T>
    static T acos( const T& a ) 
	{
		return radToDeg( std::acos( smMin( smMax( a, T(-1.0) ), T(1.0) ) ) );
	}

    /** Return the tangent of a which is specified in degrees.    */
	template<class T>
    static T tan( const T& a ) 
	{
		return std::tan( degToRad( a ) );
	}    

    /** Return the arc tangent of value a.  */
	template<class T>
    static T atan( const T& y ) 
	{
		return radToDeg(std::atan(y));
	}

    /** Return the arc tangent of y / x.  y and x are specified in degrees. */
	template<class T>
    static T atan2( const T& y,const T& x ) 
	{
		return radToDeg(std::atan2(y,x));
	}    

    /** Square root. Deprecated. See sqrt() */
	template<class T>
    static T	sqRoot( T f )
    {
        return static_cast<smReal>( sqrt( f ) ) ;
    }

    static smReal exp ( smReal f )
    {
        return static_cast<smReal>( exp( f ) ) ;
    }

    static smReal power( smReal f , smReal g)
    {
        return static_cast<smReal>( pow( f, g ) ) ;
    }

    /** Square root. */
	static double sqrt( const double& f ){ return std::sqrt( f ) ; }
    static float sqrt( const float& f ){ return std::sqrt( f ) ; }
    static smVec3r sqrt( const smVec3r& f );
    static smVec3d sqrt( const smVec3d& f );

	/**
    *	Returns sign of a number
    */
    static smReal	sign( smReal f ) ;

    /**
    *	Returns inverse sign of a number
    */
    static smReal	invSign( smReal f ) ;

    /**
    * Returns true of f1 and f2 are equal +-tol.  By default, tol =
    * smZeroTol.
    *
    */
    static bool almostEqual( const smReal& f1, const smReal& f2, smReal tol= smMath::sZeroTol )
    {
        return ( ( f1 >= ( f2 - tol ) ) && ( f1 <= ( f2 + tol ) ) );
    }

    static bool almostEqual( const smDouble& f1, const smDouble& f2, smDouble tol= smMath::sZeroTolD )
    {
        return ( ( f1 >= ( f2 - tol ) ) && ( f1 <= ( f2 + tol ) ) );
    }
	
	static bool almostEqual( const smVec3r& lhs , const smVec3r& rhs, smReal tolerance );
	static bool almostEqual( const smVec3d& lhs , const smVec3d& rhs, smDouble tolerance );

	static void getSinCos( smAngleD a, smReal& s, smReal& c );

    static smReal log2( smReal a );

    /** Returns the result of a^b in matlab context. */
    static smReal expN( smReal a, smReal b )
    {
#ifdef SM_DOUBLE_PRECISION
        return static_cast<smReal>( exp( b * log(a) ) ) ;
#else
        return static_cast<smReal>( expf( b * logf(a) ) ) ;
#endif
    }

    static int isFinite( const double& val )
    {
#if defined(__linux)
        return finite( val ) ;
#elif defined (WIN32)
        return _finite( val ) ;
#endif
    }

    /** Clamp the value to given min and max    */
    static smReal clamp( smReal num , smReal min , smReal max );
    /** If given value is below the min value, min value is returned. */
    static smReal clampLow( smReal num , smReal min );
    /** If given value is above the max value, max value is returned. */
    static smReal clampUp( smReal num , smReal max );

    /** Clamp the value to given min and max    */
    static smDouble clamp( smDouble num , smDouble min , smDouble max );
    /** If given value is below the min value, min value is returned. */
    static smDouble clampLow( smDouble num , smDouble min );
    /** If given value is above the max value, max value is returned. */
    static smDouble clampUp( smDouble num , smDouble max );
};

