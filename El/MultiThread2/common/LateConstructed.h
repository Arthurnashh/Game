#ifndef BREDYLIBS_LIBS_COMMON_LATE_CONSTRUCTED_H_
#define BREDYLIBS_LIBS_COMMON_LATE_CONSTRUCTED_H_

#pragma once

#include <exception>
#include <cstddef>
#include "Assert.h"

namespace BredyLibs
{

using std::size_t;




///Implements private memory, that can be used for allocation of one object
/**
 * @param sz size of memory
 * 
 * This template class is base class of HiddenClass and LateConstructed class
 */
template<size_t sz>
class PrivateMemory
{
    ///You cannot copy this object
  PrivateMemory(const PrivateMemory& other);
    ///You cannot copy this object
  PrivateMemory& operator=(const PrivateMemory& other);
protected:
    ///reserved space
  char _space[sz];
    ///when true, memory is allocated
  bool _allocated;
public:
    ///defines exceptions "not enought space"
    /**
     * It raised when allocation request is bigger then reserved space
     */
  class PrivateMemoryException: public std::exception
  {
  public:
  	 virtual const char* what() const throw() {return "PrivateMemory - not enough space";}
  };

    ///constructs empty space
  PrivateMemory():_allocated(false) {}
    ///allocates memory inside of reserved space
    /**
     * @param size requested size. It is used to check, whether reserved memory is enough
     * @return pointer to allocated space
     */
  void *Alloc(size_t size) 
  {
    Assert(sz>=size && _allocated==false);
    if (sz>=size && _allocated==false) {_allocated=true;return _space;}
#if defined(_CPPUNWIND) || !defined(_MSC_VER)
    throw PrivateMemoryException();
#else
    else return 0;
#endif
  }
    ///deallocates memory inside of reserved space
    /**
     * marks space free
     */
  
  void Free() throw ()
  {
    Assert(_allocated==true);
    _allocated=false;
  }

    ///returns true when memory space is allocated
  bool IsInited() const throw()
  {
    return _allocated;
  }
    ///returns true, when given pointer is known for this instance
  bool IsMyPtr(void *ptr) const throw()
  {
    return _space==ptr;
  }
};

    ///HiddenClass template is useful to hide implementation of a class
    /**
     * Implementation of "pimpl idiom" (search google). Template needs name of class (that should be hidden)
     * and size reserved for instance. Template doesn't require complette declaration of the class. 
     * Forward declaration is also acceptable (and for the "pimpl idiom" is required).
     * 
     * Class check whether reserved space is big enought to hold instance of the class. When space is too small
     * assertation / exception is throw.
     * 
     * 
     * 
     * Example:
     * 
     *   class Hidden;
     * 
     *   HiddenClass<Hidden,100> _hiddenInstance; //declare instance of unknown class
     * 
     * @note Declared variable is not inicialized yet. Before usage, use @b new(_hiddenInstance) @b Hidden. 
     * At this point, you will need full declaration of the class
     * 
     * @param T Name of class, that is hidden (use forward declaration for name)
     * @param sz resered space to instance data
     */
          
template<class T, size_t sz>
class HiddenClass: public PrivateMemory<sz>
{
	typedef PrivateMemory<sz> B;
	
#ifdef _DEBUG
    /**
     * use this pointer to inspect data 
     */
  const T *_debugPtr;
#endif
public:
#ifdef _DEBUG
  HiddenClass():_debugPtr(reinterpret_cast<T *>(B::_space)) {}
#endif
    ///dereference
  T *operator->() {return reinterpret_cast<T *>(B::_space);}
    ///const dereference
  const T *operator->() const {return reinterpret_cast<const T *>(B::_space);}  
    ///dereference
  operator T *() {return reinterpret_cast<T *>(B::_space);}
    ///const dereference
  operator const T *() const {return reinterpret_cast<const T *>(B::_space);} 
    ///derefence
  T &operator *() {return *reinterpret_cast<T *>(B::_space);}
    ///const dereference
  const T &operator *() const {return *reinterpret_cast<const T *>(B::_space);}

  ///Destruction
  /**
   * Destructor is not included in header file. You can find it in @b LateConstructed.cpp.
   * This file must be included (use #include "LateConstructed.cpp") into the implementation file
   *  (or where this class is visible)
   */
  ~HiddenClass();
};

    ///Implements class that its instance should be constructed later
    /**
     * @param T class name that should be constructed later. Class must be fully declared 
     */
     
template<class T>
class LateConstructed: public HiddenClass<T,sizeof(T)>
{
  typedef HiddenClass<T,sizeof(T)> B;

public:
    ///Construction
  LateConstructed() {}
    ///Copy constructor
    /**
     * makes copy of the instance when source instance is inicialized
     */
  LateConstructed(const LateConstructed& other) {if (other._allocated) new(*this) T(*other);}
    ///make copy
    /**
     * makes copy of the instance when source instance is inicialized. Previous instance is destroyed
     */
  LateConstructed& operator=(const LateConstructed& other)
  {
    if (B::_allocated) 
    {
        Free();
    }
    if (other._allocated) new (*this) T(*other);
  }
  
    ///destroyes the instance and prepares it to new allocation
  void Free()
  {
    (*this)->~T();
    B::Free();
  }
};

}
template<size_t size>
static inline void *operator new(size_t allocSz,BredyLibs::PrivateMemory<size> &space)
{
  return space.Alloc(allocSz);
}

template<size_t size>
static inline void operator delete(void *ptr,BredyLibs::PrivateMemory<size> &space)
{
  space.Free();
}

template<class T>
static inline void *operator new(size_t allocSz,BredyLibs::LateConstructed<T> &space)
{
  return space.Alloc(allocSz);
}

template<class T>
static inline void operator delete(void *ptr,BredyLibs::LateConstructed<T> &space)
{
  space.Free();
}

#endif
