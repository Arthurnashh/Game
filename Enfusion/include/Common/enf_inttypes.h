//-----------------------------------------------------------------------------
// File: enf_inttypes.h
//
// Desc: Basic int types
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#pragma once

#ifndef ENF_INTTYPES_H
#define ENF_INTTYPES_H

#include "enf_config.h"

#ifdef ENF_XENON
	#include <ppcintrinsics.h>
#endif

#ifdef __GCC__
	#include <limits.h>
	#include <inttypes.h>
#endif

namespace enf
{
/** @defgroup Common Common types, containers and libs
@{*/
typedef unsigned short	ushort;
typedef unsigned int		uint;
typedef unsigned long	ulong;
typedef unsigned char	uchar;
typedef unsigned char	ubyte;

#ifdef ENF_PS3
	typedef uint WPARAM;
#endif

#ifdef __GCC__
	typedef unsigned long DWORD;
	typedef uint64_t	uint64;
	typedef int64_t	int64;
	#define LONG64(n) n ## LL
	#define ULONG64(n) n ## LLU
#else
	typedef unsigned __int64	uint64;
	typedef __int64	int64;
	#define LONG64(n) n ## i64
	#define ULONG64(n) n ## ui64
#endif
typedef uint64 TimeStamp;


#ifdef ENF_XENON
	typedef int				ebool;
#else
	typedef bool			ebool;
#endif

#if defined(WIN32)
	typedef volatile long IL_INT32;
	typedef volatile __int64 IL_INT64;
#elif defined(ENF_XENON)
	typedef volatile long IL_INT32;
	typedef volatile __int64 IL_INT64;
#elif defined(ENF_PS3)
	typedef volatile int IL_INT32;
	typedef volatile int64 IL_INT64;
#elif defined(__APPLE__)
	typedef volatile int IL_INT32;
	typedef volatile int64_t IL_INT64;
#endif

typedef uint ERESULT;
typedef uint FileOffset;
typedef uint FileHandle;
const FileHandle BAD_FILE_HANDLE = 0xffffffff;
const uint INDEX_NOT_FOUND = 0xffffffffUL;
const ubyte BYTE_INDEX_NOT_FOUND = 0xffUL;

#define BITF(a) (1UL << (enf::uint)(a))

const ebool efalse	= 0;
const ebool etrue		= 1;

/*!
absolute value of an integer (the same speed as abs(i), but on XBOX could be faster)
timing:	abs(i)			 3.2106 (uses cdq instruction, almost the same as absint)
absint			 3.2496
i>=0?i:-i		 10.575
i*signint		 3.2753 
\param i	value
*/
ENF_INLINEFUNC int enf_absint (int i)
{
//#ifdef ENF_POWERPC
	int mask = i >> 31;
	return (i + mask) ^ mask;
//#else
//	return abs(i);
//#endif
}

/*!
returns log2 of a number x
\param x
*/
ENF_INLINE int enf_log2 (long x)
{
#ifdef ENF_INTEL
	__asm
	{
		bsr	eax, x
		mov	x, eax
	}
	return x;
#elif defined(ENF_XENON)
	return 31 - _CountLeadingZeros( x );
#elif defined(__GCC__)
	return 31 - __builtin_clz( x );
#else
	int l2;
	for (l2 = 0; x > 1; x >>=1)
	{
		++ l2;
	}
	return (l2);
#endif
}

//-----------------------------------------------------------------------------
/*!
get the highest set bit, count from 0: 1->0, 2->1...32->5, zero must be test
elsewhere, reason: to use the result directly in shift and power functions 
\param n	number
*/
//-----------------------------------------------------------------------------
ENF_INLINEFUNC uint GetHighestBit(uint n) 
{ 
	enf_assert(n);

#ifdef ENF_INTEL
	_asm 
	{ 
		//the simplest way
		xor eax, eax
		bsr eax, n
	}
#elif defined(ENF_XENON)
	return 31 - _CountLeadingZeros( n );
#elif defined(__GCC__)
	return 31 - __builtin_clz( n );
#else
	uint bit = 0;
	if (n & 0xFFFF0000)
	{
		n >>= 16;
		bit = 16;
	}

	if (n & 0x0000FF00)
	{
		n >>= 8;
		bit += 8;
	}

	if (n & 0x000000F0)
	{
		n >>= 4;
		bit += 4;
	}

	if (n & 0x0000000C)
	{
		n >>= 2;
		bit += 2;
	}

	if (n & 0x00000002)
	{
		return bit + 1;
	}

	return bit;
#endif
}

//-----------------------------------------------------------------------------
/*!
get the lowest set bit, count from 0: 1->0, 2->1...32->5, zero must be test
elsewhere, reason: to use the result directly in shift and power functions 

gcc has __builtin_ctz, snc not
xbox and pc should have _CountTrailingZeroes() intrinsic

\param n	number
*/
//-----------------------------------------------------------------------------
ENF_INLINEFUNC uint GetLowestBit(uint n) 
{ 
	enf_assert(n);

#ifdef ENF_INTEL
	_asm 
	{ 
		xor eax, eax
		bsf eax, n
	}
#else
	uint bit = 0;
	if (!(n & 0x0000FFFF))
	{
		n >>= 16;
		bit = 16;
	}

	if (!(n & 0x000000FF))
	{
		n >>= 8;
		bit += 8;
	}

	if (!(n & 0x0000000F))
	{
		n >>= 4;
		bit += 4;
	}

	if (!(n & 0x00000003))
	{
		n >>= 2;
		bit += 2;
	}

	if (!(n & 0x00000001))
	{
		return bit + 1;
	}

	return bit;
#endif
}


//-----------------------------------------------------------------------------
/*!
get the lowest set bit of 64 bit number, count from 0: 1->0, 2->1...32->5, 
zero must be test elsewhere, reason: to use the result directly in shift and 
power functions 

gcc has __builtin_ctzll, snc not
xbox and pc should have _CountTrailingZeroes64() intrinsic

\param n	number
*/
//-----------------------------------------------------------------------------
ENF_INLINEFUNC uint GetLowestBit64(uint64 n) 
{ 
	enf_assert(n);

	uint bit = 0;

	if (!(n & 0x00000000FFFFFFFF))
	{
		n >>= 32;
		bit = 32;
	}

	if (!(n & 0x0000FFFF))
	{
		n >>= 16;
		bit += 16;
	}

	if (!(n & 0x000000FF))
	{
		n >>= 8;
		bit += 8;
	}

	if (!(n & 0x0000000F))
	{
		n >>= 4;
		bit += 4;
	}

	if (!(n & 0x00000003))
	{
		n >>= 2;
		bit += 2;
	}

	if (!(n & 0x00000001))
	{
		return bit + 1;
	}

	return bit;
}




//-----------------------------------------------------------------------------
/*!
return true if number is power of two
*/
//-----------------------------------------------------------------------------
ENF_INLINEFUNC bool IsPowerOfTwo(uint n)
{ 
	return (n & (n - 1)) == 0;
}

}

#endif //ENF_INTTYPES_H


