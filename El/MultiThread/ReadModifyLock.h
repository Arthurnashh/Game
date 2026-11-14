#pragma once
#include "iblocker.h"
#include "EventBlocker.h"

namespace MultiThread
{
  class ReadModifyLock
  {

    friend class VirtModifyLock;
    friend class VirtReadLock;

    class InnerLockBlocker: public IBlocker
    {
      char _outerOffset;
    public:
      InnerLockBlocker(ReadModifyLock *lk)
      {
        _outerOffset=static_cast<char *>(this)-static_cast<char *>(lk);
      }

      ReadModifyLock &outer()
      {
        return *(static_cast<ReadModifyLock *>(static_cast<char *>(this)-_outerOffset));
      }

    };

    class VirtReadLock: public InnerLockBlocker
    {      
    public:
      VirtReadLock(ReadModifyLock *outer):InnerLockBlocker(outer) {}
      virtual bool Acquire(unsigned long timeout=-1)
      {
        return outer().AcquireRead(timeout);
      }
      virtual void Release()
      {
        outer().ReleaseRead();
      }
    };

    class VirtModifyLock: public InnerLockBlocker
    {      
    public:
      VirtModifyLock(ReadModifyLock *outer):InnerLockBlocker(outer) {}
      virtual bool Acquire(unsigned long timeout=-1)
      {
        return outer().AcquireModify(timeout);
      }
      virtual void Release()
      {
        outer().ReleaseModify();
      }
    };

  
    struct ThreadLockInfo
    {
      unsigned long _id;
      unsigned long _recursion;
      ThreadLockInfo(unsigned long id=0):_id(id),_recursion(0) {}
    };

  ///Guard for this lock
  CriticalSection _guard;

  int _readersIn;  

  EventBlocker _mdHalt;

  ///Virtual read lock
  VirtReadLock _rdLock;
  ///Virtual write lock
  VirtModifyLock _mdLock;

  bool AcquireRead(unsigned long timeout=-1)
  {
    if (_guard.Lock(timeout)==false) return false;
    _readersIn++;
    _guard.Unlock();
  }
  bool AcquireModify(unsigned long timeout=-1)
  {
    _guard.Lock();
    while (_readersIn)
    {
      _guard.Unlock();
      if (_mdHalt.Acquire(timeout)==false) return false;
      _guard.Lock()
    }
  }

  void ReleaseRead()
  {
    _guard.Lock();
    _readersIn--;
    if (_readers<=0) 
    {
      _readersIn=0;
      _mdHalt.Unlock();
    }
    _guard.Unlock();
  }

  void ReleaseModify()
  {
    _guard.Unlock();
  }

  public:
    ReadModifyLock(void):_readersIn(0),_rdLock(this),_mdLock(this),_mdHalt(EventBlocker::Unblocked_AutoReset) {}
    ~ReadModifyLock(void) {}
 
    IBlocker &ReadLock() {return _rdLock;}
    IBlocker &ModifyLock() {return _mdLock;}  

  };
}