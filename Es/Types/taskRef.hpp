#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TYPE_REF_HPP
#define _TYPE_REF_HPP

// This file introduces the family of classes handling ownership of objects processing by different thread

#include "pointers.hpp"
#include "lLinks.hpp"

#if _ENABLE_REPORT

/// adding the counting of ownership locks
class TaskEnabledRefCount : public RefCount
{
private:
  mutable int _taskRefCount;

public:
  /// Default constructor.
  TaskEnabledRefCount() {_taskRefCount = 0;}
  //! Copy constructor (_count attribute will not be copied).
  TaskEnabledRefCount(const TaskEnabledRefCount &src) : RefCount(src) {_taskRefCount = 0;}
  //! Copying of object
  void operator =(const TaskEnabledRefCount &src) {}

  /// Adding of reference to this object.
  int TaskAddRef() const {return ++_taskRefCount;}
  /// Releasing of reference to this object.
  int TaskRelease() const {return --_taskRefCount;}
  /// Check reference count
  int GetTaskRefCounter() const {return _taskRefCount;}
};

/// adding the counting of ownership locks
class TaskEnabledRemoveLLinks : public RemoveLLinks
{
private:
  mutable int _taskRefCount;

public:
  /// Default constructor.
  TaskEnabledRemoveLLinks() {_taskRefCount = 0;}
  //! Copy constructor (_count attribute will not be copied).
  TaskEnabledRemoveLLinks(const TaskEnabledRemoveLLinks &src) : RemoveLLinks(src) {_taskRefCount = 0;}
  //! Copying of object
  void operator =(const TaskEnabledRemoveLLinks &src)
  {
    RemoveLLinks::operator =(src);
  }

  /// Adding of reference to this object.
  int TaskAddRef() const {return ++_taskRefCount;}
  /// Releasing of reference to this object.
  int TaskRelease() const {return --_taskRefCount;}
  /// Check reference count
  int GetTaskRefCounter() const {return _taskRefCount;}
};

#else

typedef RefCount TaskEnabledRefCount;
typedef RemoveLLinks TaskEnabledRemoveLLinks;

#endif

/// Ownership lock implementation
template <class Type>
class TaskOwnerRef : public Ref<const Type>
{
  typedef Ref<const Type> base;

public:
  /// Default constructor.
  TaskOwnerRef() {}
  /// Create the reference from the existing pointer
  TaskOwnerRef(const Type *ptr) : base(ptr)
  {
#if _ENABLE_REPORT
    if (base::_ref) base::_ref->TaskAddRef();
#endif
  }
  /// Copy constructor.
  TaskOwnerRef(const TaskOwnerRef &ref) : base(ref)
  {
#if _ENABLE_REPORT
    if (base::_ref) base::_ref->TaskAddRef();
#endif
  }

  ~TaskOwnerRef()
  {
#if _ENABLE_REPORT
    if (base::_ref) base::_ref->TaskRelease();
#endif
  }

  //! Copying of pointer.
  const TaskOwnerRef<Type> &operator = (const Type *source)
  {
#if _ENABLE_REPORT
    if (source != base::_ref)
    {
      if (source) source->TaskAddRef();
      if (base::_ref) base::_ref->TaskRelease();
    }
#endif
    base::operator =(source);
    return *this;
  }
  //! Copying of pointer.
  const TaskOwnerRef<Type> &operator = (const TaskOwnerRef &ref)
  {
#if _ENABLE_REPORT
    Type *source = ref._ref;
    if (source != base::_ref)
    {
      if (source) source->TaskAddRef();
      if (base::_ref ) base::_ref->TaskRelease();
    }
#endif
    base::operator =(ref);
    return *this;
  }

};

/// Encapsulation of pointer checking if the owner is holding the reference
template <class Type>
class TaskPtr
{
private:
  const Type *_ptr;

  void Validate()
  {
#if _ENABLE_REPORT
    if (_ptr)
    {
      DoAssert(_ptr->GetTaskRefCounter() > 0);
    }
#endif
  }

public:
  /// Default constructor
  TaskPtr() : _ptr(NULL)
  {
  }
  /// Initialization through temporary pointer
  TaskPtr(const TaskOwnerRef<Type> &source) : _ptr(source.GetRef())
  {
    Validate();
  }
  /// Copy constructor
  TaskPtr(const TaskPtr &source) : _ptr(source._ptr)
  {
    Validate();
  }

  ~TaskPtr()
  {
    Validate();
  }

  /// Copying of pointer
  void operator = (const TaskOwnerRef<Type> &source)
  {
    Validate();
    _ptr = source.GetRef();
    Validate();
  }
  /// Copying of pointer
  void operator = (const TaskPtr &source)
  {
    Validate();
    _ptr = source._ptr;
    Validate();
  }

  /// Retrieving the pointer
  __forceinline const Type *operator -> () const {return _ptr;}
  /// Casting
  __forceinline operator const Type *() const {return _ptr;}
};

#endif
