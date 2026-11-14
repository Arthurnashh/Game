#include "common_Win.h"
#include ".\blockersimplebase.h"
#include "ThreadBase.h"
#include <malloc.h>

namespace MultiThread
{


  BlockerSimpleBase::BlockerSimpleBase(void)
  {
    IS().hBlock=0;
  }

  BlockerSimpleBase::~BlockerSimpleBase(void)
  {
    if (IS().hBlock) CloseHandle(IS().hBlock);
  }

  BlockerSimpleBase::BlockerSimpleBase(const BlockerSimpleBase &other)
  {
    if (other.IS().hBlock)
      DuplicateHandle(GetCurrentProcess(),other.IS().hBlock,GetCurrentProcess(),&(IS().hBlock),0,FALSE,DUPLICATE_SAME_ACCESS);
    else
      IS().hBlock=0;
  }
  BlockerSimpleBase& BlockerSimpleBase::operator=(const BlockerSimpleBase &other)
  {
    if (IS().hBlock) CloseHandle(IS().hBlock);
    if (other.IS().hBlock) 
      DuplicateHandle(GetCurrentProcess(),other.IS().hBlock,GetCurrentProcess(),&(IS().hBlock),0,FALSE,DUPLICATE_SAME_ACCESS);
    else 
      IS().hBlock=0;  
    return *this;
  }

  EventBlocker::EventBlocker(Mode mode, void *securityDescriptor)
  {
    IS().hBlock=CreateEvent(
      (LPSECURITY_ATTRIBUTES)securityDescriptor,
      mode==Blocked_ManualReset || mode==Unblocked_ManualReset,
      mode==Unblocked_AutoReset || mode==Unblocked_ManualReset,
      0);
  }

  EventBlocker::EventBlocker(const EventBlocker &other):BlockerSimpleBase(other) {}
  EventBlocker &EventBlocker::operator=(const EventBlocker &other)
  {
    BlockerSimpleBase::operator =(other);
    return *this;
  }

  bool EventBlocker::Acquire(unsigned long timeout)
  {
    return ThreadInternal::WaitForSingleObject(IS().hBlock,timeout);
  }

  void EventBlocker::Block()
  {
    ResetEvent(IS().hBlock);
  }

  void EventBlocker::Unblock()
  {
    SetEvent(IS().hBlock);
  }

  void EventBlocker::Pulse()
  {
    PulseEvent(IS().hBlock);
  }

  void EventBlocker::Release()
  {
  }

  MutexBlocker::MutexBlocker(bool initOwner, void *securityDescriptor)
  {
    IS().hBlock=CreateMutex((LPSECURITY_ATTRIBUTES)securityDescriptor,initOwner,0);
  }

  MutexBlocker::MutexBlocker(const MutexBlocker &other):BlockerSimpleBase(other) {}

  MutexBlocker &MutexBlocker::operator=(const MutexBlocker &other)
  {
    BlockerSimpleBase::operator =(other);
    return *this;
  }

  bool MutexBlocker::Acquire(unsigned long timeout)
  {
    return ThreadInternal::WaitForSingleObject(IS().hBlock,timeout);
  }
  void MutexBlocker::Release()
  {
    ReleaseMutex(IS().hBlock);
  }


  SemaphoreBlocker::SemaphoreBlocker(int maximumCount,int initialCount, void *securityDescriptor)
  {
    if (initialCount==-1) initialCount=maximumCount;
    IS().hBlock=CreateSemaphore((LPSECURITY_ATTRIBUTES)securityDescriptor,initialCount,maximumCount,0);
  }

  SemaphoreBlocker::SemaphoreBlocker(const SemaphoreBlocker &other):BlockerSimpleBase(other) {}

  SemaphoreBlocker &SemaphoreBlocker::operator=(const SemaphoreBlocker &other)
  {
    BlockerSimpleBase::operator =(other);
    return *this;
  }

  bool SemaphoreBlocker::Acquire(unsigned long timeout)
  {
    return ThreadInternal::WaitForSingleObject(IS().hBlock,timeout);
  }
  void SemaphoreBlocker::Release()
  {
    LONG cnt;
    ReleaseSemaphore(IS().hBlock,1,&cnt);
  }

  ProcessBlocker::ProcessBlocker(unsigned long pid, void *securityDescriptor)
  {
    if (pid==0) IS().hBlock=0;
    else IS().hBlock=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_INFORMATION,FALSE,pid);
  }

  ProcessBlocker::ProcessBlocker(const ProcessBlocker &other):BlockerSimpleBase(other) {}
  ProcessBlocker &ProcessBlocker::operator=(const ProcessBlocker &other)
  {
    BlockerSimpleBase::operator =(other);
    return *this;
  }

  bool ProcessBlocker::Acquire(unsigned long timeout)
  {
    return ThreadInternal::WaitForSingleObject(IS().hBlock,timeout);
  }

  void ProcessBlocker::Release(void)
  {

  }

  bool ProcessBlocker::WaitForProcessReady(unsigned long timeout)
  {
    DWORD res=WaitForInputIdle(IS().hBlock,timeout);
    if (res==WAIT_FAILED || res==WAIT_TIMEOUT) return false;
    return true;
  }

  unsigned long ProcessBlocker::GetExitCode() const
  {
    unsigned long exitcode=0;
    GetExitCodeProcess(IS().hBlock,&exitcode);
    int err=GetLastError();
    return exitcode;
  }

  ThreadBlocker::ThreadBlocker(unsigned long tid, void *securityDescriptor)
  {
    //  IS().hBlock=OpenThread(SYNCHRONIZE,FALSE,tid);
  }

  ThreadBlocker::ThreadBlocker(const ThreadBase &thr)
  {
    ThreadBase_InfoStruct *thrBlocker=static_cast<ThreadBase_InfoStruct *>(thr.GetBlocker());
    if (thrBlocker!=0)
      DuplicateHandle(GetCurrentProcess(),thrBlocker->hThread,GetCurrentProcess(),&(IS().hBlock),SYNCHRONIZE,FALSE,0);
  }

  ThreadBlocker::ThreadBlocker(const ThreadBlocker &other): BlockerSimpleBase(other) {}
  ThreadBlocker &ThreadBlocker::operator=(const ThreadBlocker &other)
  {
    BlockerSimpleBase::operator =(other);
    return *this;
  }

  bool ThreadBlocker::Acquire(unsigned long timeout)
  {
    return ThreadInternal::WaitForSingleObject(IS().hBlock,timeout);
  }

  void ThreadBlocker::Release(void)
  {

  }

  int BlockerSimpleBase::WaitForMultiple(const Array<const BlockerSimpleBase *> &blockerList, bool waitAll, unsigned long timeout)
  {
    HANDLE *hlist=(HANDLE *)alloca(blockerList.Size()*sizeof(HANDLE));
    for (unsigned int i=0;i<(unsigned)blockerList.Size();i++)  
      hlist[i]=blockerList[i]->IS().hBlock;
    return ThreadInternal::WaitForMultipleObjects(hlist,blockerList.Size(),waitAll,timeout);
  }


};