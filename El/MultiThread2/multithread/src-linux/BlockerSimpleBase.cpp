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
#include "../BlockerSimpleBase.h"
#include "../ThreadBase.h"
#include "PosixEvent.h"
#include "PosixSemaphore.h"
#include "PosixMutex.h"
#include "Blocker.h"
#include <malloc.h>


namespace MultiThread
{
    using namespace BredyLibs;

  class BlockerInternal_InfoStruct
  {
    public:
        BlockerSlot *_block;
        Ref<PosixThread> _tmp;
  };
    

  BlockerSimpleBase::BlockerSimpleBase(void)
  {
    new(privateArea) BlockerInternal_InfoStruct;
    IS()._block=0;
  }

  BlockerSimpleBase::~BlockerSimpleBase(void)
  {
    if (IS()._block) delete IS()._block;
  }

  BlockerSimpleBase::BlockerSimpleBase(const BlockerSimpleBase &other)
  {
    IS()._block=other.IS()._block;
    const_cast<BlockerSimpleBase &>(other).IS()._block=0;
  }
  BlockerSimpleBase& BlockerSimpleBase::operator=(const BlockerSimpleBase &other)
  {
    if (IS()._block) delete IS()._block;
    IS()._block=other.IS()._block;
    const_cast<BlockerSimpleBase &>(other).IS()._block=0;
    return *this;
  }
  
  
  EventBlocker::EventBlocker(Mode mode, void *securityDescriptor)
  {
    IS()._block=new PosixEvent(mode==Unblocked_AutoReset || mode==Unblocked_ManualReset,
    mode==Blocked_AutoReset || mode==Unblocked_AutoReset);      
  }

  EventBlocker::EventBlocker(const EventBlocker &other):BlockerSimpleBase(other) {}
  EventBlocker &EventBlocker::operator=(const EventBlocker &other)
  {
    BlockerSimpleBase::operator =(other);
    return *this;
  }

  bool EventBlocker::Acquire(unsigned long timeout)
  {
    return ThreadInternal::WaitForSingleObject(IS()._block,timeout);
  }

  void EventBlocker::Block()
  {
    static_cast<PosixEvent *>(IS()._block)->Block();
  }

  void EventBlocker::Unblock()
  {
    static_cast<PosixEvent *>(IS()._block)->Unblock();
  }

  void EventBlocker::Pulse()
  {
    static_cast<PosixEvent *>(IS()._block)->Pulse();
  }

  void EventBlocker::Release()
  {
  }
 
   MutexBlocker::MutexBlocker(bool initOwner, void *securityDescriptor)
  {
    IS()._block=new PosixMutex(initOwner?PosixThread::GetActiveThreadPlug():0);
  }

  MutexBlocker::MutexBlocker(const MutexBlocker &other):BlockerSimpleBase(other) {}

  MutexBlocker &MutexBlocker::operator=(const MutexBlocker &other)
  {
    BlockerSimpleBase::operator =(other);
    return *this;
  }

  bool MutexBlocker::Acquire(unsigned long timeout)
  {
    return ThreadInternal::WaitForSingleObject(IS()._block,timeout);
  }
  
  void MutexBlocker::Release()
  {
    IS()._block->Release(PosixThread::GetActiveThreadPlug());
  }

  SemaphoreBlocker::SemaphoreBlocker(int maximumCount,int initialCount, void *securityDescriptor)
  {
#ifdef BREDY_OLD_IMPLEMENTATION
    IS()._block=new PosixSemaphore(maximumCount);
#else
    IS()._block=new PosixSemaphore(maximumCount, initialCount);
#endif
  }

  SemaphoreBlocker::SemaphoreBlocker(const SemaphoreBlocker &other):BlockerSimpleBase(other) {}

  SemaphoreBlocker &SemaphoreBlocker::operator=(const SemaphoreBlocker &other)
  {
    BlockerSimpleBase::operator =(other);
    return *this;
  }

  bool SemaphoreBlocker::Acquire(unsigned long timeout)
  {
    return ThreadInternal::WaitForSingleObject(IS()._block,timeout);
  }
  void SemaphoreBlocker::Release()
  {
    IS()._block->Release(PosixThread::GetActiveThreadPlug());    
  }
 
  ThreadBlocker::ThreadBlocker(unsigned long tid, void *securityDescriptor)
  {
    AssertDesc(false,"Operation is not supported");
  }

  ThreadBlocker::ThreadBlocker(const ThreadBase &thr)
  {
    ThreadBase_InfoStruct *nfo=static_cast<ThreadBase_InfoStruct *>(thr.GetBlocker());
    IS()._tmp=nfo->_threadInfo;
    IS()._block=nfo->_threadInfo->GetEvent();
  }

  ThreadBlocker::ThreadBlocker(const ThreadBlocker &other): BlockerSimpleBase(other) {IS()._tmp=other.IS()._tmp;}
  ThreadBlocker &ThreadBlocker::operator=(const ThreadBlocker &other)
  {
    BlockerSimpleBase::operator =(other);
    IS()._tmp=other.IS()._tmp;
    return *this;
  }

  bool ThreadBlocker::Acquire(unsigned long timeout)
  {
    return ThreadInternal::WaitForSingleObject(IS()._block,timeout);
  }

  void ThreadBlocker::Release(void)
  {
  }

  ThreadBlocker::~ThreadBlocker(void)
  {
    IS()._tmp=0;
    IS()._block=0;
    
  }


  int BlockerSimpleBase::WaitForMultiple(const Array<const BlockerSimpleBase *> &blockerList, bool waitAll, unsigned long timeout)
  {
    BlockerSlot **hlist=(BlockerSlot **)alloca(blockerList.Size()*sizeof(BlockerSlot *));
    for (unsigned int i=0;i<(unsigned)blockerList.Size();i++)  
      hlist[i]=blockerList[i]->IS()._block;
    return ThreadInternal::WaitForMultipleObjects(hlist,(int)blockerList.Size(),waitAll,timeout);
  }
  
}
