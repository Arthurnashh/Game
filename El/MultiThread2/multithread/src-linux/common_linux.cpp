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
#include "common_linux.h"
#include "Blocker.h"
#include "../ThreadBase.h"
#include "waitFunctions.h"
#include "PosixThread.h"

namespace MultiThread
{
    
  ThreadBase_InfoStruct *ThreadBase_InfoStruct::GetCurrentThread()
  {
    ThreadBase *thread=ThreadBase::GetCurrentThread();
    if (thread==0) return 0;
    ThreadBase_InfoStruct *res=&thread->IS();
    return res;
  }
    
  namespace ThreadInternal
  {

    bool WaitForSingleObject(BlockerSlot *object, unsigned long timeout)
    {
        ThreadBase *thread=ThreadBase::GetCurrentThread();
        AssertDesc(thread!=0,"WaitForSingleObject - Cannot handle operation - this thread is not controlled by MultiThread library");

        if (thread==0) 
        {
            #if 0
              // cause access violation (debugging opportunity)
              volatile int a = *(int *)NULL;
              (void)a;
            #endif
            return false;
        }
        ThreadWaitInfo nfo(&object);
        if (thread->WaitFunction(nfo,timeout)==false) return false;
        return true;
    }
    int WaitForMultipleObjects(BlockerSlot **object, int count, bool waitAll, unsigned long timeout)
    {
        ThreadBase *thread=ThreadBase::GetCurrentThread();
        AssertDesc(thread!=0,"WaitForMultipleObjects - Cannot handle operation - this thread is not controlled by MultiThread library");
        if (thread==0) return false;
        ThreadWaitInfo nfo(object,count,waitAll);
        if (thread->WaitFunction(nfo,timeout)==false) return 0;
        return nfo.index;        
    }    
    
  }
  void ThreadBase_InfoStruct::AttachThread(pthread_t threadId)
  {
      _threadId=threadId;
      _threadInfo=new PosixThread();
  }

  bool ThreadBase_InfoStruct::Acquire(unsigned long timeout)
  {
      if (_threadInfo==0) return true;
      Ref<PosixThread> hold=_threadInfo;
      return hold->WaitForThreadEnd(timeout);
  }

  unsigned int ThreadBase_InfoStruct::GetExitCode() const
  {
      if (_threadInfo==0) return 0;
      else return _threadInfo->GetExitCode();
  }
   
}
