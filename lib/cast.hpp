#ifdef _MSC_VER
#pragma once
#endif

#ifndef __CAST_HPP__
#define __CAST_HPP__


// fast dynamic cast
//! Fast dynamic cast declaration
/*!
Place USE_CASTING to class that should be recnognized by dynamic casting.
Do not forget do use DEFINE_CASTING near class definition.
*\note Function dyn_cast is fast and robust when used correctly.
Each class that should be used as target for dyn_cast
must use USE_CASTING and DEFINE_CASTING macros.\n
Forgetting to do so may lead to incorrect program execution,
because class may be sharing _classId with its parent and IsClassId
will return true for both parent and derived class, making impossible'
to distinguish between them.
*/
#define USE_CASTING(baseType) \
  public: \
  static int _classId; \
  virtual size_t dyn_sizeof() const {return sizeof(*this);} \
  bool IsClassId( const int *t ) const \
  { \
    if( &_classId==t ) return true; \
    return baseType::IsClassId(t); \
  }

//! Fast dynamic cast root declaration
/*!
Place USE_CASTING_ROOT to class that should act as
root of dynamic casting hierarchy
*/

#define USE_CASTING_ROOT \
  public: \
  static int _classId; \
  virtual size_t dyn_sizeof() const {return sizeof(*this);} \
  virtual bool IsClassId( const int *t ) const { return ( &_classId==t ); }

//! Fast dynamic cast definition
#define DEFINE_CASTING(Type) \
  int Type::_classId=sizeof(Type);

#endif
