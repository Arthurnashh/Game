#ifdef _MSC_VER
#pragma once
#endif

#ifndef _STATIC_ARRAY_HPP
#define _STATIC_ARRAY_HPP

#include <Es/Framework/debugLog.hpp>
#include <Es/Containers/array.hpp>

//! Template class suitable for advanced array management.
/*!
  StaticArray is similar to AutoArray, but uses different memory managment.  
  Memory can be shared between instancies.
*/

template <class Type>
class StaticArray: public AutoArray<Type,MemAllocSS>
{
  public:

  StaticArray(){}
    //! Constructor which will set up storage (memory), that will be used by the array. 
  explicit StaticArray( const MemAllocSS &alloc )
  :AutoArray<Type,MemAllocSS>(alloc)
  {
  }
};

#define StaticStorage MStorage

template <class Type>
class StaticArrayAuto: public AutoArray< Type, MemAllocSA >
{
  public:
  
#if _MSC_VER<=1200
	enum {MinimalGrow=32};
#else
	static const int MinimalGrow = 32;
#endif

  StaticArrayAuto(){} // no init - use dynamic allocation
  explicit StaticArrayAuto( void *mem, int size )
  {
    MemAllocSA::Init(mem,size);
  }
};

// different alignement requirements

// default - 4 B

/*#define AUTO_STORAGE_ALIGNED(Type,name,size,atype) \
  const int name##_minGrow = StaticArrayAuto< Type >::MinimalGrow; \
  const int name##_size=size>name##_minGrow ? size : name##_minGrow; \
  atype name[(name##_size*sizeof(Type)+sizeof(atype)-1)/sizeof(atype)];
  */ // it can be used like a class member so do all calculation in one line

#define AUTO_STORAGE_ALIGNED(Type,name,size,atype) \
  atype name[((size>StaticArrayAuto< Type >::MinimalGrow ? size : StaticArrayAuto< Type >::MinimalGrow)*sizeof(Type)+sizeof(atype)-1)/sizeof(atype)];




#define AUTO_STORAGE_1(Type,name,size) \
  AUTO_STORAGE_ALIGNED(Type,name,size,char)

#define AUTO_STORAGE_2(Type,name,size) \
  AUTO_STORAGE_ALIGNED(Type,name,size,short)

#define AUTO_STORAGE_4(Type,name,size) \
  AUTO_STORAGE_ALIGNED(Type,name,size,int)

#define AUTO_STORAGE_8(Type,name,size) \
  AUTO_STORAGE_ALIGNED(Type,name,size,double)

#ifdef _KNI
#define AUTO_STORAGE_16(Type,name,size) \
  AUTO_STORAGE_ALIGNED(Type,name,size,__m128)

#define AUTO_STORAGE AUTO_STORAGE_16
#else
#define AUTO_STORAGE AUTO_STORAGE_8
#endif

//! Static auto array creating
/*!
  Calling the macro inside a function  it will create an autoarray either on stack,
  until the size of the array reach the "size" parameter, or on heap elsewhere.
*/
#define AUTO_STATIC_ARRAY_ALIGNED(Type,name,size,atype) \
  const int name##_minGrow = StaticArrayAuto< Type >::MinimalGrow; \
  const int name##_size=size>name##_minGrow ? size : name##_minGrow; \
  atype name##_storage[(name##_size*sizeof(Type)+sizeof(atype)-1)/sizeof(atype)]; \
  StaticArrayAuto< Type > name(name##_storage,sizeof(name##_storage)); \
  name.Realloc(name##_size);

#define AUTO_STATIC_ARRAY_1(Type,name,size) AUTO_STATIC_ARRAY_ALIGNED(Type,name,size,char)

#define AUTO_STATIC_ARRAY_2(Type,name,size) AUTO_STATIC_ARRAY_ALIGNED(Type,name,size,short)

#define AUTO_STATIC_ARRAY_4(Type,name,size) AUTO_STATIC_ARRAY_ALIGNED(Type,name,size,int)

#define AUTO_STATIC_ARRAY_8(Type,name,size) AUTO_STATIC_ARRAY_ALIGNED(Type,name,size,double)

#if defined _KNI
# define AUTO_STATIC_ARRAY_16(Type,name,size) AUTO_STATIC_ARRAY_ALIGNED(Type,name,size,__m128)
#elif defined _M_PPC
# define AUTO_STATIC_ARRAY_16(Type,name,size) AUTO_STATIC_ARRAY_ALIGNED(Type,name,size,__vector4)
#else
  // even when platform does not offer any 16B alignment data type, we define the type
  // in such case no type which would require the alignment exists anyway
# define AUTO_STATIC_ARRAY_16(Type,name,size) AUTO_STATIC_ARRAY_ALIGNED(Type,name,size,double)
#endif

#define AUTO_STATIC_ARRAY(Type,name,size) AUTO_STATIC_ARRAY_8(Type,name,size)

template <class Type>
class FindArrayStaticAuto: public FindArray<Type,MemAllocSA>
{
public:
#if _MSC_VER<=1200
	enum {MinimalGrow=32};
#else
  static const int MinimalGrow = 32;
#endif
};


#define AUTO_FIND_ARRAY_ALIGNED(Type,name,size,atype) \
  const int name##_minGrow = FindArrayStaticAuto<Type>::MinimalGrow; \
  const int name##_size=size>name##_minGrow ? size : name##_minGrow; \
  atype name##_storage[(name##_size*sizeof(Type)+sizeof(atype)-1)/sizeof(atype)]; \
  FindArrayStaticAuto<Type> name; \
  name.SetStorage(MemAllocSA(name##_storage,sizeof(name##_storage))); \
  name.Realloc(name##_size);

#define AUTO_FIND_ARRAY_1(Type,name,size) \
  AUTO_FIND_ARRAY_ALIGNED(Type,name,size,char)

#define AUTO_FIND_ARRAY_2(Type,name,size) \
  AUTO_FIND_ARRAY_ALIGNED(Type,name,size,short)

#define AUTO_FIND_ARRAY_4(Type,name,size) \
  AUTO_FIND_ARRAY_ALIGNED(Type,name,size,int)

#define AUTO_FIND_ARRAY_8(Type,name,size) \
  AUTO_FIND_ARRAY_ALIGNED(Type,name,size,double)

#ifdef _KNI
#define AUTO_FIND_ARRAY_16(Type,name,size) \
  AUTO_FIND_ARRAY_ALIGNED(Type,name,size,__m128)

#define AUTO_FIND_ARRAY AUTO_FIND_ARRAY_16
#else
#define AUTO_FIND_ARRAY AUTO_FIND_ARRAY_8
#endif


#endif
