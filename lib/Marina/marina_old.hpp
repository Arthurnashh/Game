#ifdef _MSC_VER
#pragma once
#endif

#ifndef __MARINA_OLD_HPP__
#define __MARINA_OLD_HPP__

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include <El/elementpch.hpp>
#include <El/Math/math3d.hpp>

#include "marinaArray_old.hpp"

typedef Coord marina_real;

#define MARINA_OK       1
#define MARINA_FAILURE  0

#define ASSERT(a) Assert(a);

typedef int BOOL;
#define FALSE 0
#define TRUE 1

#include "matrixA_old.hpp"

//#define NUMERICAL_ERROR 0.00001

//__forceinline bool IsNumericalZero(float fValue)
//{
//	return ((fValue <= NUMERICAL_ERROR) && (fValue >= -NUMERICAL_ERROR));
//}

// Function returns unsigned minimum. Negative values mean infinity.
template <class Type> Type umin(Type n, Type m)
{
	if (n < 0) 
		return m;
	
	if (m < 0)
		return n;

	return n < m ? n : m;
};

// Debugging tools
extern unsigned long g_iDebugID;
extern unsigned long g_iStopID;

inline void DebugPoint(void)
{
	if (g_iStopID == g_iDebugID)
	{
		//ASSERT(FALSE);
//		int i = 3;
	}
	
	//LogEx("DebugPoint %d", g_iDebugID);

	g_iDebugID++;
	
}

typedef Matrix<marina_real> M_MATRIX;
typedef Vector<marina_real> M_VECTOR;

#endif
