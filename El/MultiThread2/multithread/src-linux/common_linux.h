/*
Internal header
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

#ifndef _MULTITHREAD_COMMON_LINUX_H_
#define _MULTITHREAD_COMMON_LINUX_H_

#include <stdio.h>       /* standard I/O routines                 */
#include <pthread.h>     /* pthread functions and data structures */
#include <queue>

#ifndef _LINUX
#define _LINUX
#endif

#include "../MTCommon.h"
#include "../Interlocked.h"
#include "../Synchronized.h"
#include "../CriticalSection.h"
#include "../IBlocker.h"
#include "../../common/Ref.h"

namespace MultiThread
{
  using BredyLibs::Ref;

  class PosixThread;
  class BlockerSlot;
  
  namespace ThreadInternal
  {
    bool WaitForSingleObject(BlockerSlot *object, unsigned long timeout=(unsigned long)-1);
    int WaitForMultipleObjects(BlockerSlot **object, int count, bool waitAll, unsigned long timeout=(unsigned long)-1);    
  }

  class ThreadBase_InfoStruct: public IBlocker
  {
  public:
    pthread_t _threadId;
    MiniCriticalSection _startLock;
    Ref<PosixThread> _threadInfo;
    
    virtual bool Acquire(unsigned long timeout);
   
    virtual void Release() {}    
    
    static ThreadBase_InfoStruct *GetCurrentThread();
    
    ThreadBase_InfoStruct():_threadId(0),_threadInfo(0) {}
    
    void AttachThread(pthread_t threadId);
    
    unsigned int GetExitCode() const;
  };


    
}

#include "PosixThread.h"




#endif /*COMMON_LINUX_H_*/
