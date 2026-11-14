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



// CriticalSection.cpp: implementation of the CriticalSection class.
//
//////////////////////////////////////////////////////////////////////


#include "common_linux.h"
#include "../CriticalSection.h"
#include <Es/essencepch.hpp>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace MultiThread
{
 
  ///Count of threads waiting for mini critical sections in whole application
  long MiniCriticalSection::SWaitCount=0;
  ///Handle of global blocker
  static pthread_cond_t SMiniBlocker INIT_PRIORITY_HIGH = PTHREAD_COND_INITIALIZER;
  ///Blocker ticket counter
//  static long SMiniTicket=0;
  ///Mutex guards critical part of the WaitForCriticalSection function
  static pthread_mutex_t SMiniMutex INIT_PRIORITY_HIGH = PTHREAD_MUTEX_INITIALIZER;

  long ThreadInformation::CurrentThreadId()
  {
    return pthread_self();
  }
  
/*  static unsigned long GetTickCount()
  {
      struct timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      return ts.tv_sec*1000+ts.tv_nsec/1000;                      
  }
*/
  bool MiniCriticalSection::WaitForCriticalSection(unsigned long timeout)
  {
    //if mutex is not created yet, create it

    //serialize access to this section
    pthread_mutex_lock(&SMiniMutex);  
    //increment number of waiting threads
    SWaitCount++;

    //If blocker is not created yet, create it
    
    //calculate end of waiting
    struct timespec hlp;
    struct timespec *endtime=BlockerPlug::CalcTimeout(timeout,hlp);
    bool res=true;
        
    //try to lock critical section
    //interlocked operation is still needed
    while (MTCompareExchange(&_owner_thread,CurrentThreadId(),0)!=0)
    {
      int rc=endtime?pthread_cond_timedwait(&SMiniBlocker,&SMiniMutex,endtime):pthread_cond_wait(&SMiniBlocker,&SMiniMutex);
      if (rc!=0)  
      {
        res=false;
        break;
      }     
    }

    //Remove one waiting thread
    SWaitCount--;
    //release mutex
    pthread_mutex_unlock(&SMiniMutex);

    return res;
  }

  void MiniCriticalSection::ReleaseCriticalSection()
  {
    //Lock mini mutex to ensure, that no thread will be inside of the blocking loop during event release
    pthread_mutex_lock(&SMiniMutex);
    //Release all threads that are waiting on blocker, they will recheck status of mini critical section
    pthread_cond_broadcast(&SMiniBlocker);
    //Release all threads that are waiting on mutex, they will see, that they have an invalid ticket
    pthread_mutex_unlock(&SMiniMutex);
  }

#if MULTITHREAD_USE_EXTERN_CRITICAL_SECTION
  pthread_mutex_t CriticalSection::mutexInit = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#else
  bool CriticalSection::WaitForCriticalSection(unsigned long timeout)
  {
    long curth=CurrentThreadId();  //current thread id
    if (_spin)	//if spin defined
    {
      for (unsigned long p=0,cnt=_spin;p<cnt;p++)  //make spin cycle until section is not released
      {
        long owner=MTCompareExchange(&_owner_thread,curth,0); //mark critical section owned!
        if (owner==0)
        {
          _recursion_count=1;					//this is first recursion    
          return true;
        }
      }	
    }
    MTIncrement(&_wait_count);

    bool res=true;

    long owner=MTCompareExchange(&_owner_thread,curth,0); //mark critical section owned!
    while (owner!=0)
    {

      if (_blocker==0)      //event is not created, try to create it
      {
        PosixEvent *blocker=new PosixEvent(true,true);
        //we created signaled blocker to prevent deadlock on first request
        long lblk=(long)blocker;
        long *tblk=(long *)&_blocker;
        //try to save new blocker
        if (MTCompareExchange(tblk,lblk,0)!=0)
          delete blocker; //save failed, somebody already created the blocker - destroy it
      }   

      if (!ThreadInternal::WaitForSingleObject((PosixEvent *)_blocker,timeout))
        {
            res=false;
            break;
        }
      //Event is signaled, there is possible change, that section is ready for as
      owner=MTCompareExchange(&_owner_thread,curth,0); //mark critical section owned!
      //test success again
    }
    MTDecrement(&_wait_count);
    if (res) _recursion_count=1;
    return res;
  }

  void CriticalSection::ReleaseNextThread()
  {
    ((PosixEvent *)_blocker)->Unblock();
  }


  CriticalSection::~CriticalSection()
  {
    if (_blocker) delete (PosixEvent *)_blocker;
  }
#endif

};
