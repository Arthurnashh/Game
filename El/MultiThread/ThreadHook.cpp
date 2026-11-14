#include "common_win.h"
#include ".\threadhook.h"

namespace MultiThread
{


  ThreadHook::ThreadHook(void)
  {
    _nextHook=0;
  }

  ThreadHook::~ThreadHook(void)
  {
    Assert(_nextHook==0);
    if (_nextHook) RemoveHook(this);
  }


#pragma init_seg (lib)

  ThreadHook *ThreadHook::hookList=0;
  static CriticalSection Slock;


  bool ThreadHook::AddHook(ThreadHook *hk)
  {
    if (hk->_nextHook!=0) return false;
    Slock.Lock();

    hk->_nextHook=hookList;
    hookList=hk;

    Slock.Unlock();
    return true;
  }
  
  bool ThreadHook::RemoveHook(ThreadHook *hk)
  {
    Slock.Lock();

    if (hookList==hk) hookList=hk->_nextHook;
    else
    {
      ThreadHook *cc=hookList;
      while (cc && cc->_nextHook!=hk) cc=cc->_nextHook;
      if (cc) 
      {
        cc->_nextHook=hk->_nextHook;
      }
      else
      {
        Slock.Unlock();
        return false;
      }
    }

    hk->_nextHook=0;
    Slock.Unlock();
    return true;
  }
  

  
  unsigned long ThreadHook::StartThreadInstance(ThreadBase *instance)
  {
    Slock.Lock();
    Assert(hookList!=0);
    unsigned long res=hookList->ProcessThreadHook(instance);
    if (Slock.IamOwner()) Slock.Unlock();
    return res;
  }

  void ThreadHook_ThreadStartedNotify()
  {
    Slock.Unlock();
  }


}