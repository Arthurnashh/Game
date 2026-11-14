/*
Copyright (C)2006 bo Ondrej Novak
http://bredy.jinak.cz
novak.on@centrum.cz


This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#pragma once



#include "Interlocked.h"


namespace MultiThread
{

  ///Defines general reference counter
  /**
  * Counter can solve several problems with reference counting. You can use this
  * counter also with statically allocated instances that needn't be removed by delete
  *
  * Counter also solves MultiThread problem. Slow interlockuing is not longer
  * necesery, when program promisses to counter, that object will not shared
  * between the threads. In this case, omits slow interlocked operations. 
  * In the single processor unit, interlocking is not necesery even if object
  * is used in multiple threads.
  *
  * @param MTAwareByDefault when it true, any instance created by this class has
  *  set MTAware flag, so it can be used between the thread without any special action. Otherwise
  *  this flag must be set later.
  */
  class SmartRefCount
  {
  protected:
    ///Ref counter
    /**
    * Note that final number is combination of ref counter and the flags. 
    */
    mutable unsigned long _refCounter;

    ///Flag marks statically allocated object
    /**
    * If flag is set, instance of that class is statically allocated.
    * It means, that when ref counter reaches zero, it should not be deleted
    * This flags enables to use statically allocated object in ref-counted system   
    */
    static const unsigned long FlagStaticAlloc=0x80000000;

    ///Flag marks MT aware object
    /**
    * If flag is set, instance is shared between the threads.
    */
    static const unsigned long FlagMTAware=0x40000000;

    ///Mask for ref count value
    static const unsigned long RefCountMask=~(FlagMTAware|FlagStaticAlloc);

  public:

    ///Constructor
    /**
    * Initializes counter to zero 
    */
    SmartRefCount(bool MTAwareByDefault):_refCounter(MTAwareByDefault?FlagMTAware:0)  {}

    ///Copy constructor
    /**
    * @note copy constructor will not copy ref counter, because new object is not yet referenced
    */
    SmartRefCount(const SmartRefCount &other):_refCounter(other._refCounter & FlagMTAware)  {}

    ///Assign operator
    /**
    * No change in ref counter. because assign will not change reference count 
    */
    SmartRefCount &operator=(const SmartRefCount &other) {return *this;}

    unsigned long GetRefCount() const
    {
      return _refCounter & RefCountMask;
    }

    virtual ~SmartRefCount(void)
    {
      Assert(GetRefCount()==0);
    }

  protected:
    /// assert/debugging opportunity - verify object state is correct when used
    /**
    Debugging only - Never called in release build.
    */
    virtual void OnUsed() const {}

    /// Adding of reference to this object.
    /**
    * @return Number of references.
    */
  public:
    long AddRef() const
    {
      long res;
      if ((_refCounter & FlagMTAware)==0) res=++_refCounter;
      else res=MultiThread::MTIncrement((long *)&_refCounter);
      Assert(GetRefCount()!=RefCountMask); //Counter overflow!
      return res & RefCountMask;
    }

    //* Releasing of reference to this object.
    /**
    * @return Number of references.
    */
    long Release() const
    {
      long res;
      if ((_refCounter & FlagMTAware)==0)
      {
        res=--_refCounter;
      }
      else
      {
        res=MultiThread::MTDecrement((long *)&_refCounter);
      }
      res&= RefCountMask;
      Assert(res!=RefCountMask); //Counter underflow!
      if (res!=0) return res;
      const_cast<SmartRefCount *>(this)->OnUnused();
      return res;
    }

  protected:
    virtual void OnUnused() 
    {
      if ((_refCounter & FlagStaticAlloc)==0)  delete const_cast<SmartRefCount *>(this);
    }

#ifndef _USE_BREDY_LIBS
  public:
    /// check reference count
    int RefCounter() const {return GetRefCount();}

    /// get memory used by this object
    /**
    Does not include memory used to hold this object itself,
    which is already covered by sizeof
    */
    virtual double GetMemoryUsed() const {return 0;}

  protected:
    //@{ pretend the shape is still loaded during its unloading to avoid destructor recursion
    void TempAddRef() {_refCounter++;}
    void TempRelease() {_refCounter--;}
    //@}
#endif

    ///Marks object statically allocated
    /** Statically allocated object will not be deleted when ref count reaches zero.
    That refcounter only simulates counting, tracking current value, and can return
    count of references, that currently referencing the object. In Debug version,
    assert raises, when object is destroed with counter > 0
    @param staticAlloc true - object is statically allocated, false - object is not statically alocated
    */
    void SetStatic(bool staticAlloc=true)
    {
      if (staticAlloc) _refCounter |= FlagStaticAlloc;
      else _refCounter &= ~FlagStaticAlloc;
    }

    ///Returns state of static alloc flag
    bool IsStatic() const
    {
      return (_refCounter & FlagStaticAlloc)!=0;
    }

    ///Marks object sharedable between different threads
    /**
    * When object is marked MTAware, interlocked operations are being used on the counter
    * This enables to share object between the threads. Interlocked operations are slower
    * then simply increment and decrement, so you can switch MTAware off, when object
    * is not shared. BE CAREFULL, don't forget turn on MTAware before reference is passed into
    * another thread. It is better to keep MTAware on, when you cannot ensure, whether object
    * can be shared or not. MTAware turned on causes only worse performance. MTAware turned 
    * off may trigger some race conditions and crash your application when counter
    * is changed and shared between the threads
    *
    * @param mtAware true - object can be shared between the thread. false - object cannot 
    *  be shared between the threads.
    */
    void SetMTAware(bool mtAware=true)
    {
      if (mtAware) _refCounter |= FlagMTAware;
      else _refCounter &= ~FlagMTAware;
    }

    ///Returns state of MTAware
    bool IsMTAware() const
    {
      return (_refCounter & FlagMTAware)!=0;
    }

  };
}

