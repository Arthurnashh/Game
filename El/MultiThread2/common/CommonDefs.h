#ifndef BREDYLIBS_LIBS_COMMON_COMMONDEFS_H_
#define BREDYLIBS_LIBS_COMMON_COMMONDEFS_H_

#include <cstdlib>
#include <ctype.h>
#include <wchar.h>

#ifdef _WIN32
#include <tchar.h>
#else
#include <wctype.h>
#ifndef _T
#ifdef _UNICODE
#define _T(x) L##x
#else
#define _T(x) x
#endif
#endif

#ifndef _TCHAR
#ifdef _UNICODE
#define _TCHAR wchar_t
#else
#define _TCHAR char
#endif
#endif

#endif

#ifndef __max
#define __max(a,b) ((a)>(b)?(a):(b))
#endif 

#ifndef __min
#define __min(a,b) ((a)<(b)?(a):(b))
#endif 

#ifndef lenof
#define lenof(x) (sizeof(x)/sizeof(x[0]))
#endif


#define DefineClassProperty(PropName) template<class Type> struct PropName { }
#define DefineClassPropertyDefault(PropName,Default) template<class Type> struct PropName {typedef Default Value;}
#define SetClassProperty(PropName,Type,Val) template<> struct PropName< Type > {typedef Val Value;}  
#define SetTemplateClassProperty(PropName,Type,Val) struct PropName< Type > {typedef Val Value;}  

namespace BredyLibs
{

    using std::size_t;

///qualifier manipulator
/**
 * Use following class to manipulate with const in template 
 * 
 * @b ConstObject<Type>::Remove removes const from the Type
 * @b ConstObject<Type>::Add adds const to the Type 
 * @b ConstObject<Type>::Invert inverts const on the Type
 */
 
template<class T>
struct ConstObject
{
    typedef T Remove;
    typedef const T Invert;
    typedef const T Add; 
};

template<class T>
struct ConstObject<const T>
{
    typedef T Remove;
    typedef T Invert;
    typedef const T Add;
};

template<class T>
struct Pointer
{
    typedef T Remove;
    typedef T *Invert;
    typedef T *Add;
};

template<class T>
struct Pointer<T *>
{
    typedef T Remove;
    typedef T Invert;
    typedef T *Add;
};

class NullClass
{
    
};


typedef _TCHAR tchar_t;
typedef const tchar_t *tstring_t;

template<class T>
class NoReleaseRule
{
public:
    void operator()(T *ptr) const {}
};

template<class T>
class ReleaseDeleteRule
{
public:
    void operator()(T *ptr) const  {delete ptr;}
};

template<class T>
class ReleaseDeleteArrayRule
{
public:
    void operator()(T *ptr) const  {delete [] ptr;}
};

template<class T>
class ReleaseByFunctionRule
{
public:
    void operator()(T *ptr) const  {ptr->Release();}
};

template<class T>
class ReleaseByFunctionFreeRule
{
public:
  void operator()(T *ptr) const  {ptr->Free();}
};


class NullMTLock
{
public:  
  bool Lock() const {return true;} //it is const, because locking is not change of object state
  bool Unlock() const {return true;} //it is const, because locking is not change of object state
};

template<class T>
class LockContext
{
  const T &_lock;
  bool _locked;
public:
  LockContext(const T &other):_lock(other) {_locked=const_cast<T &>(_lock).Lock();}
  ~LockContext() {if (_locked) const_cast<T &>(_lock).Unlock();}
  bool operator!() const {return !_locked;}
  operator bool () const {return _locked;}
};

///Sets value in variable and returns previous value
/**
 * Function is optimalized to reduce count of copy constructor calling 
 */
template<class Type>
Type ExchangeValue(Type &variable, const Type &value)
{
    struct Hlpr
    {
        Type &var;
        const Type &val;
    
        Hlpr(Type &var,const Type &val):var(var),val(val) {}
        ~Hlpr() {var=val;}
    };
    
    Hlpr scope(variable,value);
    return Type(variable);
}

}

#endif /*_COMMONDEFS_H_*/

