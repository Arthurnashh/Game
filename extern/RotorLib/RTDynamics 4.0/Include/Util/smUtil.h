/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once


#include <sstream>
/**
*	This method is slow !
*	Because writing a call-by-reference version produces
*	a lot of ambiguity problems, only this version exists..
*
*/
// Disable conversion from 'size_t' to 'int', possible loss of data warning
#pragma warning( push )
#pragma warning(disable : 4267)
template <class TYPE> std::string smToString( const TYPE t )
{
    std::ostringstream os;
    os << t ;
    return os.str();
}
template <class TYPE> std::string smToString( const TYPE t, int precision )
{
    std::ostringstream os;
	os.precision(precision);
    os << t ;
    return os.str();
}
#pragma warning( pop )

/**	smToStringLeftAligned is especially suitable for displaying text
*	on screen.
*/
// Good information on output stream formatting can be found at
// http://www.codeguru.com/cpp/tic/tic0192.shtml
// Bruce Eckel's Thinking in C++, 2nd Ed
template <class TYPE> std::string smToStringLeftAligned( const TYPE t, size_t prec=3, char fillChar=' ', size_t minWidth=10 )
{
	std::ostringstream os;

	// Shall look like  "132.0384    "
	os.setf( std::ios::dec ) ;
	os.setf( std::ios::fixed ) ;
	os.setf( std::ios::left ) ;

	// Width works like minimum width.
	os.width( minWidth ) ;
	os.fill( fillChar ) ;
	os.precision( prec ) ;
	os << t ;
	return os.str();
}

template <class TYPE> std::string smToStringSci( const TYPE t, int width, int precision )
{
    std::ostringstream os;
    os.width( width );
    os.precision( precision ) ;
    os << t ;
    return os.str();
}

template <class TYPE> std::string smToStringAsHex( const TYPE t )
{
	std::ostringstream os;
	os << std::ios::hex<< t ;
	return os.str();
}

//
//	This macro avoids a Win32 warning.
//
#define SM_SAFE_INT_BOOL_CAST( exp )	( (exp) != 0 )
//
//	Use SM_INLINE for method declarations
//
#if _MSC_VER > 1000
    #define SM_INLINE	__forceinline
#else
    #define SM_INLINE	inline
#endif

//
//	OS isolation for sleep functions.
//	Can not use smReal here due to header lock effect.
//
void smSleep( double seconds ) ;

bool smValidateAndWarn(bool statement, const std::string& what, const std::string& where );
bool smValidateAndExit(bool statement, const std::string& what, const std::string& where );

