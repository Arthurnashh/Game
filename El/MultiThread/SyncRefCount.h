#ifndef _SYNCREFCOUNT_BREDYLIBS_MUTLITHREADS_
#define _SYNCREFCOUNT_BREDYLIBS_MUTLITHREADS_

#include "Synchronized.h"

namespace MultiThread
{


  class SyncRefCount
  {
  protected:
    mutable SyncInt _count;
  public:

    SyncRefCount():_count(0) {}

    SyncRefCount( const SyncRefCount &src ):_count(0) 
    {
    }

    //! Copying of object (_count attribute will not be copied).
    void operator =( const SyncRefCount &src )
    {
    } 

    virtual ~SyncRefCount() {}

    long AddRef() const
    {
      return ++_count;
    }

    long Release() const
    {
      int ret=--_count;
      if (ret<1) OnUnused();
      return ret;

    }

    virtual void OnUnused() const {Destroy();}

    void Destroy() const
    {
      delete const_cast<SyncRefCount *>(this);
    }

    long GetRefCount() const
    {
      return _count;
    }
    long RefCounter() const
    {
      return _count;
    }
    //@{ pretend the shape is still loaded during its unloading to avoid destructor recursion
    void TempAddRef() {_count++;}
    void TempRelease() {_count--;}
    //@}
  };


};
#endif