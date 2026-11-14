#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TYPE_DEFINES_HPP
#define _TYPE_DEFINES_HPP

#include <string.h>
#include <Es/Framework/debugLog.hpp>

#ifndef PREPROCESS_DOCUMENTATION

#ifdef _MSC_VER
# pragma warning(disable:4141) // inline used more than once - we do not care
#endif
// real implementation of type properties

template <class Type>
struct HasGenericConstructor
{
	static __forceinline void Construct( Type &dst ) {ConstructAt(dst);}
  static inline void ConstructArray( Type *dst, int count ) 
	{
	  for( int i=0; i<count; i++ ) ConstructAt(dst[i]);
	}
};

template <class Type>
struct HasEmptyConstructor
{
  static __forceinline void Construct( Type &dst ) {}
	static __forceinline void ConstructArray( Type *dst, int n ) {}
};

template <class Type>
struct HasZeroConstructor
{
	static __forceinline void Construct( Type &dst ) {memset(&dst,0,sizeof(Type));}
	static __forceinline void ConstructArray( Type *dst, int n ) {memset(dst,0,sizeof(Type)*n);}
};

template <class Type>
struct HasGenericDestructor
{
  static __forceinline void Destruct( Type &dst ) {dst.~Type();}
  static inline void DestructArray( Type *dst, int n )
  {
    for( int i=0; i<n; i++ ) dst[i].~Type();
  }
};

template <class Type>
struct HasEmptyDestructor
{
	static __forceinline void Destruct( Type &dst ) {}
	static __forceinline void DestructArray( Type *dst, int n ) {}
};

template <class Type>
struct HasGenericCopy
{
  static inline void CopyData( Type *dst, Type const *src, int n )
  {
    for( int i=0; i<n; i++ ) dst[i]=src[i];
  }
  static inline void CopyConstruct( Type *dst, Type const *src, int n )
  {
    for( int i=0; i<n; i++ ) ConstructAt(dst[i],src[i]);
  }
  static __forceinline void CopyConstruct( Type &dst, Type const &src ) {ConstructAt(dst,src);}
};

template <class Type>
struct HasBinaryCopy
{
  static __forceinline void CopyData( Type *dst, Type const *src, int n ) {memmove(dst,src,n*sizeof(Type));}
  static __forceinline void CopyConstruct( Type *dst, Type const *src, int n ) {memmove(dst,src,n*sizeof(Type));}
  static __forceinline void CopyConstruct( Type &dst, Type const &src ) {memmove(&dst,&src,sizeof(Type));}
};

//! traits - type can provide effective implementation of constructor / destructor
template <class Type>
struct ConstructBaseTraits
{
  // constructor implementation
  static __forceinline void Construct( Type &dst ) {Type::Construct(dst);}
  // constructor (array) implementation
  static __forceinline void ConstructArray( Type *dst, int count ){Type::ConstructArray(dst,count);}

  // destructor implementation
  static __forceinline void Destruct( Type &dst ) {Type::Destruct(dst);}
  // destructor (array) implementation
  static __forceinline void DestructArray( Type *dst, int n ) {Type::DestructArray(dst,n);}
};

//! traits - type can provide effective implementation of corresponding functions
template <class Type>
struct ConstructTraits: public ConstructBaseTraits<Type>
{
  // operator = implementation (array)
  static __forceinline void CopyData( Type *dst, Type const *src, int n )
  {
    Type::CopyData(dst,src,n);
  }
  // copy constructor implementation (array)
  static __forceinline void CopyConstruct( Type *dst, Type const *src, int n )
  {
    Type::CopyConstruct(dst,src,n);
  }
  // copy constructor implementation
  static __forceinline void CopyConstruct( Type &dst, Type const &src )
  {
    Type::CopyConstruct(dst,src);
  }

  // construct at dst, destruct at src 
  static __forceinline void MoveData( Type *dst, Type *src, int n )
  {
    Type::MoveData(dst,src,n);
  }
  // move data
  // dst[n-count]...dst[n-1] is controlled before function call
  // dst[0]...dst[count-1] is not controlled after function call
  static __forceinline void InsertData( Type *dst, int n, int count = 1 )
  {
    Type::InsertData(dst,n,count);
  }
  // move data
  // dst[0]..dst[count-1] is not controlled before function call
  // dst[n-count]..dst[n-1] is not controlled after function call
  static __forceinline void DeleteData( Type *dst, int n, int count )
  {
    Type::DeleteData(dst,n,count);
  }
};

// move variants

template <class Type>
struct HasGenericMove
{
  static __forceinline void MoveData( Type *dst, Type *src, int n )
  {
    ConstructTraits<Type>::CopyConstruct(dst,src,n);
    ConstructTraits<Type>::DestructArray(src,n);
  }
  static inline void InsertData( Type *dst, int n, int count=1 )
  {
    /* dst[n-count]...dst[n-1] is controlled before function call */
    for( int i=n-count; --i>=0; ) dst[i+count]=dst[i];
    ConstructTraits<Type>::DestructArray(dst,count);
    /* dst[0]..dst[count-1] is not controlled after function call */
  }
  static inline void DeleteData( Type *dst, int n, int count )
  {
    /* dst[0]..dst[count-1] is not controlled before function call */
    int i;
    for (i=0; i<count; i++) ConstructTraits<Type>::Construct(dst[i]);
    for (i=count; i<n; i++) dst[i-count]=dst[i];
    for (i=n-count; i<n; i++) ConstructTraits<Type>::Destruct(dst[i]);
    /* dst[n-count]..dst[n-1] is not controlled after function call */
  }
};


template <class Type>
struct HasBinaryMove
{
	static __forceinline void MoveData( Type *dst, Type *src, int n )
	{
		memmove(dst,src,n*sizeof(Type));
	}
	static __forceinline void InsertData( Type *dst, int n, int count=1 )
	{
    /* dst[n-count]...dst[n-1] is controlled before function call */
		ConstructTraits<Type>::DestructArray(dst+n-count, count);
		memmove(dst+count,dst,(n-count)*sizeof(Type));
    /* dst[0]..dst[count-1] is not controlled after function call */
	}
	static __forceinline void DeleteData( Type *dst, int n, int count )
	{
		/* dst[0]..dst[count-1] is not controlled before function call */
		memmove(dst,dst+count,(n-count)*sizeof(Type));
		/* dst[n-count]..dst[n-1] is not controlled after function call */
	}
};

#define TypeHasGenericConstructor(Type,decl) \
	decl __forceinline void Construct( Type &dst ) {ConstructAt(dst);} \
	decl inline void ConstructArray( Type *dst, int count ) \
	{for( int i=0; i<count; i++ ) ConstructAt(dst[i]);}

#if defined _M_PPC && _DEBUG
//@{ we need to check alignment on PPC debug, which is done in the constructor
# define TypeHasEmptyConstructor(Type,decl) TypeHasGenericConstructor(Type,decl)
# define TypeHasZeroConstructor(Type,decl) TypeHasGenericConstructor(Type,decl)
//@}
#else
# define TypeHasEmptyConstructor(Type,decl) \
	decl __forceinline void Construct( Type &dst ) {} \
	decl __forceinline void ConstructArray( Type *dst, int n ) {}

# define TypeHasZeroConstructor(Type,decl) \
	decl __forceinline void Construct( Type &dst ) {memset(&dst,0,sizeof(Type));} \
	decl __forceinline void ConstructArray( Type *dst, int n ) {memset(dst,0,sizeof(Type)*n);}
#endif

// destructor variants

#define TypeHasGenericDestructor(Type,decl) \
	decl __forceinline void Destruct( Type &dst ) {(dst).~Type();} \
	decl inline void DestructArray( Type *dst, int n ) \
	{for( int i=0; i<n; i++ ) (dst[i]).~Type();}

#define TypeHasEmptyDestructor(Type,decl) \
	decl __forceinline void Destruct( Type &dst ) {} \
	decl __forceinline void DestructArray( Type *dst, int n ) {}

// copy variants

#define TypeHasGenericCopy(Type,decl) \
	decl inline void CopyData( Type *dst, Type const *src, int n ) \
	{for( int i=0; i<n; i++ ) dst[i]=src[i];} \
	decl inline void CopyConstruct( Type *dst, Type const *src, int n ) \
	{for( int i=0; i<n; i++ ) ConstructAt(dst[i],src[i]);} \
  decl __forceinline void CopyConstruct( Type &dst, Type const &src ) {ConstructAt(dst,src);}

#define TypeHasBinaryCopy(Type,decl) \
	decl __forceinline void CopyData( Type *dst, Type const *src, int n ) \
	{memcpy(dst,src,n*sizeof(Type));} \
	decl __forceinline void CopyConstruct( Type *dst, Type const *src, int n ) \
	{memcpy(dst,src,n*sizeof(Type));} \
	decl __forceinline void CopyConstruct( Type &dst, Type const &src ) \
	{memcpy(&dst,&src,sizeof(Type));}

// move variants

#define TypeHasGenericMove(Type,decl) \
	decl __forceinline void MoveData( Type *dst, Type *src, int n ) \
	{ \
		CopyConstruct(dst,src,n); \
		DestructArray(src,n); \
	} \
  decl inline void InsertData( Type *dst, int n, int count ) \
	{ \
		/* dst[n-count]...dst[n-1] is controlled before function call */ \
		for( int i=n-count; --i>=0; ) dst[i+count]=dst[i]; \
		DestructArray(dst,count); \
		/* dst[0]..dst[count-1] is not controlled after function call */ \
	} \
	decl inline void DeleteData( Type *dst, int n, int count ) \
	{ \
		/* dst[0]..dst[count-1] is not controlled before function call */ \
		int i; \
		for (i=0; i<count; i++) Construct(dst[i]); \
		for (i=count; i<n; i++) dst[i-count]=dst[i]; \
		for (i=n-count; i<n; i++) Destruct(dst[i]); \
		/* dst[n-count]..dst[n-1] is not controlled after function call */ \
	}


#define TypeHasBinaryMove(Type,decl) \
  decl __forceinline void MoveData( Type *dst, Type *src, int n ) \
	{ \
		memmove(dst,src,n*sizeof(Type)); \
	} \
	decl __forceinline void InsertData( Type *dst, int n, int count ) \
	{ \
    /* dst[n-count]...dst[n-1] is controlled before function call */ \
		DestructArray(dst+n-count, count); \
		memmove(dst+count,dst,(n-count)*sizeof(Type)); \
    /* dst[0]..dst[count-1] is not controlled after function call */ \
	} \
	decl __forceinline void DeleteData( Type *dst, int n, int count ) \
	{ \
		/* dst[0]..dst[count-1] is not controlled before function call */ \
		memmove(dst,dst+count,(n-count)*sizeof(Type)); \
		/* dst[n-count]..dst[n-1] is not controlled after function call */ \
	}

template <class Type>
struct ConstructBaseTraitsIsGeneric: public HasGenericConstructor<Type>, public HasGenericDestructor<Type>
{
};
template <class Type>
struct ConstructBaseTraitsIsZeroed: public HasZeroConstructor<Type>, public HasGenericDestructor<Type>
{
};
template <class Type>
struct ConstructBaseTraitsIsSimple: public HasEmptyConstructor<Type>, public HasEmptyDestructor<Type>
{
};
template <class Type>
struct ConstructBaseTraitsIsSimpleZeroed: public HasZeroConstructor<Type>, public HasEmptyDestructor<Type>
{
};


template <class Type>
struct ConstructTraitsIsGeneric:
  public ConstructBaseTraitsIsGeneric<Type>, public HasGenericCopy<Type>, public HasGenericMove<Type>
{
};

template <class Type>
struct ConstructTraitsIsMovable:
  public ConstructBaseTraitsIsGeneric<Type>, public HasGenericCopy<Type>, public HasBinaryMove<Type>
{
};
template <class Type>
struct ConstructTraitsIsBinary:
  public ConstructBaseTraitsIsGeneric<Type>, public HasBinaryCopy<Type>, public HasBinaryMove<Type>
{
};

template <class Type>
struct ConstructTraitsIsMovableZeroed:
  public ConstructBaseTraitsIsZeroed<Type>, public HasGenericCopy<Type>, public HasBinaryMove<Type>
{
};

template <class Type>
struct ConstructTraitsIsBinaryZeroed:
  public ConstructBaseTraitsIsZeroed<Type>, public HasBinaryCopy<Type>, public HasBinaryMove<Type>
{
};

template <class Type>
struct ConstructTraitsIsSimpleZeroed:
  public ConstructBaseTraitsIsSimpleZeroed<Type>, public HasBinaryCopy<Type>, public HasBinaryMove<Type>
{
};

template <class Type>
struct ConstructTraitsIsSimple:
  public ConstructBaseTraitsIsSimple<Type>, public HasBinaryCopy<Type>, public HasBinaryMove<Type>
{
};

/// specialization of ConstructTraits for all pointers
/**
Using partial specialization we avoid having to specify for each plain pointer its properties.
*/
template <class Type> struct ConstructTraits<Type *>: public ConstructTraitsIsSimple<Type *> {};


template <class Type> void Construct( Type &dst ) {ErrF("Global function \"Construct\" called, which should be avoided!");};
template <class Type> void ConstructArray( Type *dst, int count ) {ErrF("Global function \"ConstructArray\" called, which should be avoided!");};
template <class Type> void CopyConstruct( Type &dst, const Type &src ) {ErrF("Global function \"CopyConstruct\" called, which should be avoided!");};
template <class Type> void CopyConstruct( Type *dst, const Type *src, int n ) {ErrF("Global function \"CopyConstruct\" called, which should be avoided!");};
template <class Type> void Destruct( Type &dst ) {ErrF("Global function \"Destruct\" called, which should be avoided!");};
template <class Type> void DestructArray( Type *dst, int count ) {ErrF("Global function \"DestructArray\" called, which should be avoided!");};
template <class Type> void CopyData( Type *dst, const Type *src, int n ) {ErrF("Global function \"CopyData\" called, which should be avoided!");};
template <class Type> void MoveData( Type *dst, Type *src, int n ) {ErrF("Global function \"MoveData\" called, which should be avoided!");};
template <class Type> void Insert( Type *dst, int n ) {ErrF("Global function \"Insert\" called, which should be avoided!");};
template <class Type> void Delete( Type *dst, int n ) {ErrF("Global function \"Delete\" called, which should be avoided!");};
template <class Type> void InsertData( Type *dst, int n, int count ) {ErrF("Global function \"InsertData\" called, which should be avoided!");};
template <class Type> void DeleteData( Type *dst, int n, int count ) {ErrF("Global function \"DeleteData\" called, which should be avoided!");};

template <class Type>
struct ConstructGlobalFunctions
{
	// constructor implementation
	static void Construct( Type &dst ) {::Construct(dst);}
	// constructor (array) implementation
	static void ConstructArray( Type *dst, int count ){::ConstructArray(dst,count);}

	// destructor implementation
	static void Destruct( Type &dst ) {::Destruct(dst);}
	// destructor (array) implementation
	static void DestructArray( Type *dst, int n ) {::DestructArray(dst,n);}

	// operator = implementation (array)
	static void CopyData( Type *dst, Type const *src, int n ){::CopyData(dst,src,n);}
	// copy constructor implementation (array)
	static void CopyConstruct( Type *dst, Type const *src, int n ){::CopyConstruct(dst,src,n);}
	// copy constructor implementation
	static void CopyConstruct( Type &dst, Type const &src ){::CopyConstruct(dst,src);}

	// contruct at dst, destruct at src 
	static void MoveData( Type *dst, Type *src, int n ){::MoveData(dst,src,n);}
	// move data
	// dst[n-1] is controlled before function call
	// dst[0] is not controlled after function call
	static void InsertData( Type *dst, int n, int count = 1 ){::InsertData(dst,n,count);}
	// move data
	// dst[0]..dst[count-1] is not controlled before function call
	// dst[n-count]..dst[n-1] is not controlled after function call
	static void DeleteData( Type *dst, int n, int count ){::DeleteData(dst,n,count);}
};

#else


// documentation only - do not expand any functions

#define TypeHasGenericConstructor(Type,decl) \
	void Type_Has_GenericConstructor(Type type);
#define TypeHasEmptyConstructor(Type,decl) \
	void Type_Has_EmptyConstructor(Type type);
#define TypeHasZeroConstructor(Type,decl) \
	void Type_Has_ZeroConstructor(Type type);
#define TypeHasGenericDestructor(Type,decl) \
	void Type_Has_GenericDestructor(Type type);
#define TypeHasEmptyDestructor(Type,decl)\
	void Type_Has_EmptyDestructor(Type type);
#define TypeHasGenericCopy(Type,decl)\
	void Type_Has_GenericCopy(Type type);
#define TypeHasBinaryCopy(Type,decl)\
	void Type_Has_BinaryCopy(Type type);
#define TypeHasGenericMove(Type,decl)\
	void Type_Has_GenericMove(Type type);
#define TypeHasBinaryMove(Type,decl)\
	void Type_Has_BinaryMove(Type type);

#endif

#define TypeIsGenericDecl(Type,decl) \
	TypeHasGenericConstructor(Type,decl) \
	TypeHasGenericDestructor(Type,decl) \
	TypeHasGenericCopy(Type,decl) \
	TypeHasGenericMove(Type,decl)

#define TypeIsMovableDecl(Type,decl) \
	TypeHasGenericConstructor(Type,decl) \
	TypeHasGenericDestructor(Type,decl) \
	TypeHasGenericCopy(Type,decl) \
	TypeHasBinaryMove(Type,decl)

#define TypeIsMovableZeroedDecl(Type,decl) \
	TypeHasZeroConstructor(Type,decl) \
	TypeHasGenericDestructor(Type,decl) \
	TypeHasGenericCopy(Type,decl) \
	TypeHasBinaryMove(Type,decl)

#define TypeIsBinaryDecl(Type,decl) \
	TypeHasGenericConstructor(Type,decl) \
	TypeHasGenericDestructor(Type,decl) \
	TypeHasBinaryCopy(Type,decl) \
	TypeHasBinaryMove(Type,decl)

#define TypeIsBinaryZeroedDecl(Type,decl) \
	TypeHasZeroConstructor(Type,decl) \
	TypeHasGenericDestructor(Type,decl) \
	TypeHasBinaryCopy(Type,decl) \
	TypeHasBinaryMove(Type,decl)


#define TypeIsSimpleDecl(Type,decl) \
	TypeHasEmptyConstructor(Type,decl) \
	TypeHasEmptyDestructor(Type,decl) \
	TypeHasBinaryCopy(Type,decl) \
	TypeHasBinaryMove(Type,decl)

#define TypeIsSimpleZeroedDecl(Type,decl) \
	TypeHasZeroConstructor(Type,decl) \
	TypeHasEmptyDestructor(Type,decl) \
	TypeHasBinaryCopy(Type,decl) \
	TypeHasBinaryMove(Type,decl)

////
#define TemplateIsGenericDecl(Type,decl) \
	TemplateHasGenericConstructor(Type,decl) \
	TemplateHasGenericDestructor(Type,decl) \
	TemplateHasGenericCopy(Type,decl) \
	TemplateHasGenericMove(Type,decl)

#define TemplateIsMovableDecl(Type,decl) \
	TemplateHasGenericConstructor(Type,decl) \
	TemplateHasGenericDestructor(Type,decl) \
	TemplateHasGenericCopy(Type,decl) \
	TemplateHasBinaryMove(Type,decl)

#define TemplateIsMovableZeroedDecl(Type,decl) \
	TemplateHasZeroConstructor(Type,decl) \
	TemplateHasGenericDestructor(Type,decl) \
	TemplateHasGenericCopy(Type,decl) \
	TemplateHasBinaryMove(Type,decl)

#define TemplateIsBinaryDecl(Type,decl) \
	TemplateHasGenericConstructor(Type,decl) \
	TemplateHasGenericDestructor(Type,decl) \
	TemplateHasBinaryCopy(Type,decl) \
	TemplateHasBinaryMove(Type,decl)

#define TemplateIsBinaryZeroedDecl(Type,decl) \
	TemplateHasZeroConstructor(Type,decl) \
	TemplateHasGenericDestructor(Type,decl) \
	TemplateHasBinaryCopy(Type,decl) \
	TemplateHasBinaryMove(Type,decl)


#define TemplateIsSimpleDecl(Type,decl) \
	TemplateHasEmptyConstructor(Type,decl) \
	TemplateHasEmptyDestructor(Type,decl) \
	TemplateHasBinaryCopy(Type,decl) \
	TemplateHasBinaryMove(Type,decl)

#define TemplateIsSimpleZeroedDecl(Type,decl) \
	TemplateHasZeroConstructor(Type,decl) \
	TemplateHasEmptyDestructor(Type,decl) \
	TemplateHasBinaryCopy(Type,decl) \
	TemplateHasBinaryMove(Type,decl)

#ifdef NO_TYPE_DEFINES
	#include "typeGenTemplates.hpp"

	#define TypeIsGeneric(Type)

	#define TypeIsMovable(Type)
	#define TypeIsBinary(Type)
	#define TypeIsSimple(Type)

	#define TypeIsMovableZeroed(Type)
	#define TypeIsBinaryZeroed(Type)
	#define TypeIsSimpleZeroed(Type)

	// use this form in class declaration (usefull esp. for templates)

	#define ClassIsGeneric(Type)

	#define ClassIsMovable(Type)
	#define ClassIsBinary(Type)
	#define ClassIsSimple(Type)

	#define ClassIsMovableZeroed(Type)
	#define ClassIsBinaryZeroed(Type)
	#define ClassIsSimpleZeroed(Type)

#else

#ifndef PREPROCESS_DOCUMENTATION

	// use this form outside of class declaration

	#define TypeIsGeneric(Type) \
	template <> struct ConstructBaseTraits< Type >: public ConstructBaseTraitsIsGeneric< Type > {}; \
	template <> struct ConstructTraits< Type >: public ConstructTraitsIsGeneric< Type > {};

	#define TypeIsMovable(Type) \
	template <> struct ConstructBaseTraits< Type >: public ConstructBaseTraitsIsGeneric< Type > {}; \
	template <> struct ConstructTraits< Type >: public ConstructTraitsIsMovable< Type > {};

	#define TypeIsBinary(Type) \
	template <> struct ConstructBaseTraits< Type >: public ConstructBaseTraitsIsGeneric< Type > {}; \
	template <> struct ConstructTraits< Type >: public ConstructTraitsIsBinary< Type > {};

	#define TypeIsMovableZeroed(Type) \
	template <> struct ConstructBaseTraits< Type >: public ConstructBaseTraitsIsZeroed< Type > {}; \
	template <> struct ConstructTraits< Type >: public ConstructTraitsIsMovableZeroed< Type > {};

	#define TypeIsBinaryZeroed(Type) \
	template <> struct ConstructBaseTraits< Type >: public ConstructBaseTraitsIsZeroed< Type > {}; \
	template <> struct ConstructTraits< Type >: public ConstructTraitsIsBinaryZeroed< Type > {};

  
  #define TypeIsSimpleZeroed(Type) \
	template <> struct ConstructBaseTraits< Type >: public ConstructBaseTraitsIsSimpleZeroed< Type > {}; \
	template <> struct ConstructTraits< Type >: public ConstructTraitsIsSimpleZeroed< Type > {};

	#define TypeIsSimple(Type) \
	template <> struct ConstructBaseTraits< Type >: public ConstructBaseTraitsIsSimple< Type > {}; \
	template <> struct ConstructTraits< Type >: public ConstructTraitsIsSimple< Type > {};

// use this form in class declaration (usefull esp. for templates)

	#define ClassIsGeneric(Type) TypeIsGenericDecl(Type,static)

	#define ClassIsMovable(Type) TypeIsMovableDecl(Type,static)
	#define ClassIsBinary(Type) TypeIsBinaryDecl(Type,static)
	#define ClassIsSimple(Type) TypeIsSimpleDecl(Type,static)

	#define ClassIsMovableZeroed(Type) TypeIsMovableZeroedDecl(Type,static)
	#define ClassIsBinaryZeroed(Type) TypeIsBinaryZeroedDecl(Type,static)
	#define ClassIsSimpleZeroed(Type) TypeIsSimpleZeroedDecl(Type,static)

#else

	// documentation only
	#define TypeIsGeneric(Type) void Type_Is_Generic(Type type);
	#define TypeIsMovable(Type) void Type_Is_Movable(Type type);
	#define TypeIsBinary(Type) void Type_Is_Binary(Type type);
	#define TypeIsSimple(Type) void Type_Is_Simple(Type type);

	#define TypeIsMovableZeroed(Type) void Type_Is_MovableZeroed(Type type);
	#define TypeIsBinaryZeroed(Type) void Type_Is_BinaryZeroed(Type type);
	#define TypeIsSimpleZeroed(Type) void Type_Is_SimpleZeroed(Type type);

	#define ClassIsGeneric(Type) void Class_Is_Generic(Type type);

	#define ClassIsMovable(Type) void Class_Is_Movable(Type type);
	#define ClassIsBinary(Type) void Class_Is_Binary(Type type);
	#define ClassIsSimple(Type) void Class_Is_Simple(Type type);

	#define ClassIsMovableZeroed(Type) void Class_Is_MovableZeroed(Type type);
	#define ClassIsBinaryZeroed(Type) void Class_Is_BinaryZeroed(Type type);
	#define ClassIsSimpleZeroed(Type) void Class_Is_SimpleZeroed(Type type);

#endif

#endif

#ifndef __forceinline

#ifdef __MWERKS__
	#define __forceinline
#elif !defined _MSC_VER || _MSC_VER<1200
	#define __forceinline inline
#endif

#endif

#endif
