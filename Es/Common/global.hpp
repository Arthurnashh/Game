#ifdef _MSC_VER
#  pragma once
#endif

/**
    @file   common/global.hpp
    @brief  Global declarations for all Pepca's subprojects.

    Copyright &copy; 2001-2003 by BIStudio (www.bistudio.com)
    @author PE
    @since  29.11.2001
    @date   15.6.2003
*/

#ifndef _GLOBAL_H
#define _GLOBAL_H

//-----------------------------------------------------------
//  Integer types with precisely defined bit-length:

/// Signed 8-bit integer type.
typedef signed char int8;

/// Unsigned 8-bit integer type (byte).
typedef unsigned char unsigned8;

/// Signed 16-bit integer type.
typedef short int16;

/// Unsigned 16-bit integer type (word).
typedef unsigned short unsigned16;

/// Signed 32-bit integer type.
typedef int int32;

/// Unsigned 32-bit integer type (dword).
typedef unsigned unsigned32;

#ifdef _WIN32

/// Signed 64-bit integer type.
typedef __int64 int64;

/// Unsigned 64-bit integer type (qword).
typedef unsigned __int64 unsigned64;

#else

/// Signed 64-bit integer type.
typedef long long int64;

/// Unsigned 64-bit integer type (qword).
typedef unsigned long long unsigned64;

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned long long QWORD;
typedef unsigned int UINT;

#  ifndef FALSE
typedef int BOOL;
#    define FALSE 0
#    define TRUE  1
#  endif

#endif

const unsigned64 INFINITY_U64 = 0xffffffffffffffffULL;

const unsigned32 INFINITY_U32 = 0xffffffff;

//-----------------------------------------------------------
//  timing functions:

//#define HAS_CLOCK_GETTIME                   // not yet implemented in Linux ??
#define HAS_GETTIMEOFDAY

//-----------------------------------------------------------
//  Support macros:

#ifndef Assert

#  ifdef _WIN32
/**
    Assert macro. The given boolean expression should be true.
    Should be empty in Release build mode.
*/
#    define Assert(x)   ASSERT(x)
#  else
/**
    Assert macro. The given boolean expression should be true.
    Should be empty in Release build mode.
*/
#    define Assert(x)
#  endif

#endif

template<typename T>
inline void Zero(T& x) {memset((void*)&(x),0,sizeof(x));}

//-----------------------------------------------------------
//  Symbols for conditional compiling:

#undef  MAP_STAT
#undef  MAP_DEBUG

#endif
