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

#include <cstdio>
#include <malloc.h>
#include <memory>
#include "common_linux.h"
#include "../ThreadBase.h"
#include "../Tls.h"
#include "../IBlocker.h"
#include "waitFunctions.h"
#include "../ThreadHook.h"
#include "PosixThread.h"

#include "Es/Framework/debugLog.hpp"

namespace MultiThread
{
    using namespace BredyLibs;

    static TLS(ThreadBase) SCurrentThread INIT_PRIORITY_HIGH;
    
    ThreadBase::ThreadBase(void):_priority(PriorityNormal),_initStack(0)
    {
      new(_threadData) ThreadBase_InfoStruct;
    }

    ThreadBase::ThreadBase(Priority priority, size_t initStack):_priority(priority),_initStack(initStack)
    {
      new(_threadData) ThreadBase_InfoStruct;
    }

    ThreadBase::~ThreadBase(void)
    {
      if (IsRunning() && GetCurrentThread()!=this)
      {  
        AssertDesc(false,"Critical:Destroying running thread!");        
        RptF("Critical:Destroying running thread!");
        #if 0
          // cause access violation (debugging opportunity)
          volatile int a = *(int *)NULL;
          (void)a;
        #endif
    }
      IS().~ThreadBase_InfoStruct();
    }
    
    bool ThreadBase::Start()
    {
      return Start(false);
    }

    void ThreadHook_ThreadStartedNotify();

    class FakeThreadBase: public ThreadBase
    {
    public:
        void WaitToStartFinish()
        {
            IS()._startLock.Lock();
            IS()._startLock.Unlock();
        }
    };

    class ThreadBase_StartupClass: public ThreadHook
    {
    public:
      unsigned long ProcessThreadHook(ThreadBase *instance)
      {
        if (SCurrentThread!=0) return (unsigned long)-1;
        ThreadHook_ThreadStartedNotify();
       
        SCurrentThread=instance;
      FakeThreadBase *thread=static_cast<FakeThreadBase *>(instance);
      thread->WaitToStartFinish();  
    
        bool init=instance->Init();
        unsigned long res=(unsigned long)-1;
        if (init) res=instance->Run();
        instance->Done(!init);
        return res;
      }
      
   
      ThreadBase_StartupClass()
      {
        ThreadHook::AddHook(this);    
      }
      ~ThreadBase_StartupClass()
      {
        ThreadHook::RemoveHook(this);
      }
    };

    static ThreadBase_StartupClass ___startup_hook INIT_PRIORITY_HIGH;
    
    static void *StartThread(void *data)
    {
      FakeThreadBase *thread=reinterpret_cast<FakeThreadBase *>(data);
      DebugPrint2("Thread id %08X, object %08X, has been started.\n",pthread_self(),data);          
      unsigned int res=ThreadHook::StartThreadInstance(thread);
      ThreadBase_InfoStruct::GetCurrentThread()->_threadInfo->FinishNotify(res);
      DebugPrint3("Thread id %08X, object %08X, has exited with the code: %d\n",pthread_self(),data,res);          
      return 0;
    }

    bool ThreadBase::Start(bool suspended, void *securityDescriptor)
    {
      bool res=false;
    
      IS()._startLock.Lock();
    
      if (!IsRunning())
      {
        pthread_t id;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        if (_initStack) pthread_attr_setstacksize(&attr,_initStack);
        pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
        //TODO: use securityDescriptor to specify thread attributes
        int rc=pthread_create(&id,&attr,&StartThread,this);
        pthread_attr_destroy(&attr);
        if (rc==0)
        {
            IS().AttachThread(id);
            res=true;            
        }
      }
      
      //TODO: Implement suspended start
      if (!res || !suspended) IS()._startLock.Unlock();
      return res;;
    }
    
    void ThreadBase::SetPriority(Priority priority)
    {
        //TODO: Set priority implementation
    }
    
    bool ThreadBase::Join(unsigned long timeout) const
    {
        if (IS()._threadInfo==0) return true;
        return IS()._threadInfo->WaitForThreadEnd(timeout);
    }
    
    bool ThreadBase::IsRunning() const
    {
      return !Join(0);
    }
        
    
    void ThreadBase::Sleep(unsigned long miliseconds)
    {
      static PosixEvent sleeper(false,false);
      ThreadInternal::WaitForSingleObject(&sleeper,miliseconds);
    }
    
    int ThreadBase::Suspend()
    {
        //TODO: implement suspend
        return -1;
    }
    
    //Use only for resuming thread started suspended
    //You must call this function from the thread that created suspended thread
    int ThreadBase::Resume()
    {
        if (IS()._startLock.IamOwner())
        {
            IS()._startLock.Unlock();
            return 0;
        }
        else
        {
            return -1;         
        }
    }
    
    ThreadBase *ThreadBase::GetCurrentThread()
    {
        return SCurrentThread;
    }

    ThreadBase *ThreadBase::AttachToCurrentThread()
    {
      if (IS()._threadId!=0) 
      {
        #if 0
          // cause access violation (debugging opportunity)
          volatile int a = *(int *)NULL;
          (void)a;
        #endif
        return this;
      }
      ThreadBase *thisThrd=GetCurrentThread();
      if (thisThrd)
      {
        IS()._threadId=thisThrd->IS()._threadId;
        IS()._threadInfo=thisThrd->IS()._threadInfo;
        _priority=thisThrd->_priority;
        _initStack=thisThrd->_initStack;
      }
      else
      {
        IS().AttachThread(pthread_self());
        _priority=PriorityNormal;
        _initStack=0;
      }
      SCurrentThread=this;
      return thisThrd;
    }
    
    void ThreadBase::DetachThread(ThreadBase *previous)
    {
      if (previous==this) return;
      IS()._threadId=0;
      IS()._threadInfo=0;  
      SCurrentThread=previous;
    }
    
    unsigned long ThreadBase::GetExitCode() const
    {
      return IS().GetExitCode();
    }
    
    int ThreadBase::JoinMulti(const Array<ThreadBase *> &threads, bool waitAll,unsigned long timeout)
    {              
        BlockerSlot **plugList=(BlockerSlot **)alloca(sizeof(BlockerSlot *)*threads.Size());
        for (unsigned int i=0;i<threads.Size();i++) 
            {
//                DebugPrint2("%08X, %08X\n",threads[i]->IS()._threadInfo.GetRef(),threads[i]->IS()._threadInfo->GetEvent());
                plugList[i]=threads[i]->IS()._threadInfo->GetEvent();
            }
        return ThreadInternal::WaitForMultipleObjects(plugList, threads.Size(), waitAll, timeout);            
    }
    
    IRunnable *ThreadBase::GetCurrentRunnable()
    {
      ThreadBase *curr=GetCurrentThread();
      if (curr==0) return 0;
      return curr->GetRunnable();
    }
    
    IBlocker *ThreadBase::GetBlocker() const
    {
      return const_cast<ThreadBase_InfoStruct *>(&(IS()));
    }
    
    bool ThreadBase::IsMineThread() const
    {
      return GetCurrentThread()==this;
    }

    unsigned long ThreadBase::StartCurrentThread()
    {
      //lock Start function
      IS()._startLock.Lock();
      //thread object or thread are already in use?
      if (IS()._threadId!=0) 
      {
        //unlock Start function
        IS()._startLock.Unlock();
        //this is not allowed
        return (unsigned long)-1;
      }
      
      //Attach to the thread, function should return 0
      Verify(AttachToCurrentThread()==0);
      
      SCurrentThread=0;
      //unlock Start function - it will now return false - thread already running
      IS()._startLock.Unlock();
    
      DebugPrint2("Thread id %0X8 has been successfully attached by the object %0X8\n",pthread_self(),this);  
      //start thread instance, call hooks and Run
      unsigned long res=ThreadHook::StartThreadInstance(this);
    
      //lock Start function 
      IS()._startLock.Lock();
      //detach the thread
      DetachThread(0);
      //unlock
      IS()._startLock.Unlock();
      DebugPrint3("Thread id %0X8 has been successfully detached by the object %0X8, exit code %ld\n",pthread_self(),this,res);  
    
      return res;
    }

}
    
