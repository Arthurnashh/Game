#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TYPE_GEN_TEMPLATES_HPP
#define _TYPE_GEN_TEMPLATES_HPP

// generic templates for alll unknown types

template <class Type>
inline void Construct( Type &dst ) {ConstructAt(dst);}

template <class Type>
inline void ConstructArray( Type *dst, int count )
{
	for( int i=0; i<count; i++ ) ConstructAt(dst[i]);
}

template <class Type>
inline void CopyConstruct( Type &dst, const Type &src ) {ConstructAt(dst,src);}

template <class Type>
inline void CopyConstruct( Type *dst, const Type *src, int n )
{
	for( int i=0; i<n; i++ ) ConstructAt(dst[i],src[i]);
}

template <class Type>
inline void Destruct( Type &dst ) {(&dst)->~Type();}

template <class Type>
inline void DestructArray( Type *dst, int count )
{
	for( int i=0; i<count; i++ ) Destruct(dst[i]);
}

template <class Type>
inline void CopyData( Type *dst, const Type *src, int n )
{
	for( int i=0; i<n; i++ ) dst[i]=src[i];
}

template <class Type>
inline void MoveData( Type *dst, Type *src, int n )
{
	CopyConstruct(dst,src,n);
	DestructArray(src,n);
}


template <class Type>
inline void Insert( Type *dst, int n )
{
	// dst[n-1] is controlled before function call
	// TODO: consider using copy for first element
	for( int i=n-1; --i>=0; ) dst[i+1]=dst[i];
	Destruct(dst[0]);
	// dst[0] is not controlled after function call
}

template <class Type>
inline void Delete( Type *dst, int n )
{
	// dst[0] is not controlled before function call
	Construct(dst[0]);
	// TODO: consider using copy for first element
	for( int i=1; i<n; i++ ) dst[i-1]=dst[i];
	Destruct(dst[n-1]);
	// dst[n-1] is not controlled after function call
}

#endif
