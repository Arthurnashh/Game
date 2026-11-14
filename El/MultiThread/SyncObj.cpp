#include "common_Win.h"
#include "Synchronized.h"
#include "CriticalSection.h"
#include "HashTable.h"



namespace MultiThread
{


#pragma warning( disable : 4074 ) 

#pragma init_seg(compiler)
  
  static MultiThread::CriticalSection GlobalLocker;



  class LockedPointer
  {
    const void *_lptr;
    unsigned long _owner;
    HANDLE _blocker;
    unsigned short _recursions;
    unsigned short _waiters;

  public:

    LockedPointer(const void *ptr):_lptr(ptr),_blocker(0),_owner(0),_recursions(0),_waiters(0) {}
    ~LockedPointer()
    {
      if (_blocker) CloseHandle(_blocker);
    }

    void SetOwner() {_owner=GetCurrentThreadId();}
    void Enter() {_recursions++;}
    bool Leave() 
    {
      _recursions--;
      return _recursions==0;
    }

    void Anounce() {_waiters++;}
    void Unanounce() {_waiters--;}
    HANDLE GetBlocker() {return _blocker;}
    void SetBlocker(HANDLE h) {_blocker=h;}
    bool ImOwner() {return _owner==GetCurrentThreadId();}  
    bool IsUnowned() {return _recursions==0;}
    bool IsAnounced() {return _waiters>0;}

    int operator%(int sz) const
    {
      return ((unsigned long)_lptr)%sz;
    }
    bool operator==(const LockedPointer &ptr) const
    {return _lptr==ptr._lptr;}
    bool operator!=(const LockedPointer &ptr) const
    {return _lptr!=ptr._lptr;}
  };

  static HashTable::Expandable<LockedPointer> SLockedDB;


  static HANDLE TryLockObjects(const void **arr, int count, unsigned long timeout, LockedPointer *& anounced)
  {
    int i;
    GlobalLocker.Lock();

    if (anounced) 
    {
      anounced->Unanounce();
      anounced=0;
    }

    for (i=0;i<count;i++)
    {
      LockedPointer *fnd=const_cast<LockedPointer *>(SLockedDB.Find(LockedPointer(arr[i])));
      if (fnd!=0 && !fnd->IsUnowned())
      {
        if (!fnd->ImOwner())
        {
          fnd->Anounce();
          if (fnd->GetBlocker()==0) 
            fnd->SetBlocker(CreateEvent(0,0,0,0));
          GlobalLocker.Unlock();
          anounced=fnd;
          return fnd->GetBlocker();
        }
      }
    }

    for (i=0;i<count;i++)
    {
      LockedPointer *fnd=const_cast<LockedPointer *>(SLockedDB.Find(LockedPointer(arr[i])));
      if (fnd==0)
      {
        LockedPointer nwptr(arr[i]);
        SLockedDB.Add(nwptr);
        fnd=const_cast<LockedPointer *>(SLockedDB.Find(LockedPointer(arr[i])));
      }
      fnd->SetOwner();
      fnd->Enter();
    }
    GlobalLocker.Unlock();
    return 0;
  }

  bool SynchronizeBase::LockObjects(const void **arr, int count, unsigned long timeout)
  {
    LockedPointer *anounced=0;
    HANDLE h=TryLockObjects(arr,count,timeout,anounced);
    while (h!=0)
    {
      DWORD v=GetTickCount();
      bool res=ThreadInternal::WaitForSingleObject(h,timeout);
      DWORD w=GetTickCount();
      DWORD t=w-v;
      if (t>timeout) timeout=0;
      else timeout-=t;
      if (res==false && timeout==0) return false;
      h=TryLockObjects(arr,count,timeout,anounced);
    }
    return true;
  }

  void SynchronizeBase::UnlockObjects(const void **arr, int count)
  {
    int i;
    GlobalLocker.Lock();
    for (i=0;i<count;i++)
    {
      LockedPointer *fnd=const_cast<LockedPointer *>(SLockedDB.Find(LockedPointer(arr[i])));
      if (fnd)
      {
        if (fnd->Leave()==true)
        {
          if (fnd->GetBlocker())
          {
            SetEvent(fnd->GetBlocker());
          }
          if (!fnd->IsAnounced())
          {
            SLockedDB.Remove(*fnd);
          }
        }
      }
    }
    GlobalLocker.Unlock();
  }

};