#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TYPE_TEMPLATES_HPP
#define _TYPE_TEMPLATES_HPP

#include "typeGenTemplates.hpp"

template <class Type>
class Movable
{
	friend void MoveData( Type *dst, Type *src, int n )
	{
		memcpy(dst,src,n*sizeof(Type));
	}
	friend void Insert( Type *dst, int n )
	{
		// dst[n-1] is controlled before function call
		Destruct(dst[n-1]);
		memmove(dst+1,dst,(n-1)*sizeof(Type));
		// dst[0] is not controlled after function call
	}

	friend void Delete( Type *dst, int n )
	{
		// dst[0] is not controlled before function call
#ifdef _WIN32
    memcpy(dst,dst+1,(n-1)*sizeof(Type));
#else
    memmove(dst,dst+1,(n-1)*sizeof(Type));
#endif
		// dst[n-1] is not controlled after function call
	}
};

template <class Type>
class Binary: public Movable<Type>
{
	// optimize copy for binary data types
	friend void CopyData( Type *dst, const Type *src, int n )
	{
		memcpy(dst,src,n*sizeof(Type));
	}
};

template <class Type>
class Simple: public Binary<Type>
{
	// no init required for simple types
	friend void Construct( Type &dst ) {}
	friend void ConstructArray( Type *dst, int n ) {}
	friend void Destruct( Type &dst ) {}
	friend void DestructArray( Type *dst, int n ) {}
};

// make common access for template/define approach

#ifdef NO_TYPE_DEFINES

#define TypeIsMovable(type) template Movable<type>;
#define TypeIsMovableZeroed(type) template Movable<type>;
#define TypeIsBinary(type) template Binary<type>;
#define TypeIsSimple(type) template Simple<type>;

#else

#define TypeIsGeneric(type) // template Movable<type>
#define TypeIsMovable(type) //template Movable<type>
#define TypeIsBinary(type) //template Movable<type>
#define TypeIsSimple(type) template Simple<type>
#define TypeIsMovableZeroed(type) //template Movable<type>
#define TypeIsBinaryZeroed(type) //template Movable<type>
#define TypeIsSimpleZeroed(type) //template Movable<type>

#define ClassIsGeneric(type)
#define ClassIsMovable(type)
#define ClassIsBinary(type)
#define ClassIsSimple(type)
#define ClassIsMovableZeroed(type)
#define ClassIsBinaryZeroed(type)
#define ClassIsSimpleZeroed(type)


#endif

#endif

