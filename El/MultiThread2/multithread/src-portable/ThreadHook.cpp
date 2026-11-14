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

#if defined(_WIN32)
#include "../src-windows/common_Win.h"
#endif
#include "../MTCommon.h"
#include "../ThreadHook.h"
#include "../CriticalSection.h"

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

#ifdef _MSVC
#pragma init_seg (lib)
#endif 

  ThreadHook *ThreadHook::hookList=0;
  static CriticalSection &GetLock() 
  { 
    static CriticalSection Slock;
    return Slock; 
  }

  bool ThreadHook::AddHook(ThreadHook *hk)
  {
    if (hk->_nextHook!=0) return false;
    GetLock().Lock();

    hk->_nextHook=hookList;
    hookList=hk;

    GetLock().Unlock();
    return true;
  }
  
  bool ThreadHook::RemoveHook(ThreadHook *hk)
  {
    GetLock().Lock();

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
        GetLock().Unlock();
        return false;
      }
    }

    hk->_nextHook=0;
    GetLock().Unlock();
    return true;
  }
  

  
  unsigned long ThreadHook::StartThreadInstance(ThreadBase *instance)
  {
    GetLock().Lock();
    Assert(hookList!=0);
    unsigned long res=hookList->ProcessThreadHook(instance);
    if (GetLock().IamOwner()) GetLock().Unlock();
    return res;
  }

  void ThreadHook_ThreadStartedNotify()
  {
    GetLock().Unlock();
  }


}
