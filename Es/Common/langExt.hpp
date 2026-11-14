#ifdef _MSC_VER
#  pragma once
#endif

/**
@file   C++ Languauge extenstions
*/

#ifndef _ES_LANG_EXT_HPP
#define _ES_LANG_EXT_HPP

/// cast using implicit conversions only
template <class To,class From>
inline To safe_cast( const From &from ) {return from;}

/// specialization to avoid C4800 warning
template <>
inline bool safe_cast<bool, const int>( const int &from ) {return from!=0;}

template <class Type, class From>
inline Type safe_cast(From &from) {return from;}

/// specialization to avoid C4800 warning
template <>
inline bool safe_cast<bool, int>( int &from ) {return from!=0;}

/// cast to const char * using implicit conversions only
#define cc_cast(from) safe_cast<const char *>(from)

/// remove const-ness from a pointer
template <class Type>
__forceinline Type *unconst_cast(const Type *ptr)
{
  return const_cast<Type *>(ptr);
}


#if !defined _MSC_VER || _MSC_VER>=1300
/// remove const-ness from a reference
template <class Type>
__forceinline Type &unconst_cast(const Type &ptr)
{
  return const_cast<Type &>(ptr);
}
#endif

/// helper type, can be used in sizeof to get a compile time constant
template <size_t N>
struct ArrayDimHelperSize {typedef char type[N];};

// derive the helper type from the array
template <typename T, size_t Size>
typename ArrayDimHelperSize<Size>::type &ArrayDimHelper(T(&)[Size]);

/// get length of an array
#define lenof(pArray) sizeof(ArrayDimHelper(pArray))

#define COMPILE_ASSERT_JOIN( X, Y ) COMPILE_ASSERT_JOIN2(X,Y)
#define COMPILE_ASSERT_JOIN2( X, Y ) X##Y

namespace compile_assert
{
  template <bool> struct STATIC_ASSERT_FAILURE;
  template <> struct STATIC_ASSERT_FAILURE<true> {};

  template<int x> struct compile_assert_test{};
}

#define COMPILE_ASSERT(x) \
  typedef ::compile_assert::compile_assert_test<sizeof(::compile_assert::STATIC_ASSERT_FAILURE< (bool)( x ) >)> \
  COMPILE_ASSERT_JOIN(_compile_assert_typedef, __LINE__)

#if _MSC_VER>=1600
#define STATIC_ASSERT(cond) static_assert(cond,#cond)
#else
#define STATIC_ASSERT(cond) COMPILE_ASSERT(cond)
#endif

#define COMPILETIME_COMPARE(a,b) STATIC_ASSERT(a==b);
#define COMPILETIME_COMPARE_ISLESSOREQUAL(a,b) STATIC_ASSERT(a<=b);

#define WARN_STRINGIZE2(x) #x
#define WARN_STRINGIZE(x) WARN_STRINGIZE2(x)
#define WARN_MESSAGE(x) (__FILE__ "(" WARN_STRINGIZE(__LINE__) "): " x)
// use like this:
//#pragma message WARN_MESSAGE("warning: Remove obsolete code")



// http://stackoverflow.com/questions/7658060/how-can-i-elide-a-call-if-an-edge-condition-is-known-at-compile-time/7658363#7658363
// http://encode.ru/threads/396-C-compile-time-constant-detection

struct chkconst {
  struct Small {char a;};
  struct Big: Small {char b;};
  struct Temp { Temp( int x ) {} };
  static Small chk2( void* ) { return Small(); }
  static Big chk2( Temp  ) { return Big(); }
};

#define is_const_0(X) (sizeof(chkconst::chk2(X))<sizeof(chkconst::Big))
#if ___GNUC__ // GCC overzealous with constant detection (non-conformant)
  #define is_const_pos(X) is_const_0( int(X)^(int(X)&INT_MAX) )
  #define is_const(X) (is_const_pos(X)|is_const_pos(-int(X))|is_const_pos(-(int(X)+1)))
#else
  #define is_const(X) is_const_0( int(X)-int(X) )
#endif

//@{ allow enabling features based on which user performs the build

#define ENABLED_FEATURE_D2(X,U) ENABLED_##X##_##U
#define ENABLED_FEATURE_D1(X,U) ENABLED_FEATURE_D2(X,U)
#define ENABLED_FEATURE(X) ENABLED_FEATURE_D1(X,BUILD_SYSTEM_USER)

#if USER_SPECIFIC_CODE_EXAMPLE_SECTION
// recent example, check directly for a specific user
#if USER_User
#endif

// to define a feature and enabled it for some user(s) use
#if USER_User || USER_Other
#define SOME_FEATURE 1
#endif
#if SOME_FEATURE
#endif

// Beware: username in this context is case sensitive, the same user may be spelled user or User on different windows computers.
If this applies to you, you may want to use
#if USER_User || USER_user
#endif

// Old example, using BUILD_SYSTEM_USER and ENABLED_FEATURE
#define ENABLED_FEATURE_EXAMPLE_User 1
#if ENABLED_FEATURE(FEATURE_EXAMPLE)
#endif
*/
//@}
#endif

/// allow initialization using a different type
template <class Type, class InitType=Type, InitType defaultValue=0>
class InitValT
{
public:
  typedef Type ValueType;
private:
  ValueType _value;
public:
  // This is the point of the exercise: provide a default constructor
  __forceinline InitValT():_value(Type(defaultValue)) {}

  // These make it close to interchangeable with T
  __forceinline InitValT(const InitValT &other) :_value(other._value){}
  __forceinline InitValT(const ValueType &initialValue):_value(initialValue){}
  __forceinline const InitValT & operator = (const InitValT &other) { _value = other._value;return *this;}
  __forceinline const InitValT & operator = (const ValueType &newValue) {_value = newValue;return *this;}
  
  __forceinline operator const ValueType &() const { return _value; }
  __forceinline operator ValueType &() { return _value; }

  // And these are useful sometimes
  __forceinline const ValueType &get() const {return _value;}
  __forceinline void set(const ValueType &newValue) {_value = newValue;}
};

/// variable with given initialization value
template <class Type, Type defaultValue=0>
class InitVal
{
public:
  typedef Type ValueType;
private:
  ValueType _value;
public:
  // This is the point of the exercise: provide a default constructor
  __forceinline InitVal():_value(defaultValue) {}

  // These make it close to interchangeable with T
  __forceinline InitVal(const InitVal &other) :_value(other._value){}
  __forceinline InitVal(const ValueType &initialValue):_value(initialValue){}
  __forceinline const InitVal & operator = (const InitVal &other) { _value = other._value;return *this;}
  __forceinline const InitVal & operator = (const ValueType &newValue) {_value = newValue;return *this;}
  
  __forceinline operator const ValueType &() const { return _value; }
  __forceinline operator ValueType &() { return _value; }

  // And these are useful sometimes
  __forceinline const ValueType &get() const {return _value;}
  __forceinline void set(const ValueType &newValue) {_value = newValue;}
};

/// check in the compile time (Debug configuration) scalars with different units (for example indices to different structures)
#if _DEBUG
template <typename Base>
class Derived
{
private:
  Base _val;

public:
  operator Base() const {return _val;}
  operator Base &() {return _val;}
  explicit Derived(Base val) {_val = val;}
  /// having default constructor as uninitialized is a little bit unsafe, but performant
  Derived(){}
};

/// convenience wrapper to Derived template
#define UNIQUE_TYPE(name, base) class name : public Derived<base> \
{ \
public: \
  explicit name(base val) : Derived<base>(val){} \
  name(){} \
}
#else
#define UNIQUE_TYPE(name, base) typedef base name
#endif

/// by inheriting from this class copy constructor and assignment is disabled
class NoCopy
{
public:
  NoCopy(){} // we do not want to disable default constructor

private:
  NoCopy(const NoCopy &src);
  NoCopy &operator =( const NoCopy &src);
};

#endif

