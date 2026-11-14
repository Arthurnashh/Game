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

// CriticalSection.h: interface for the CriticalSection class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CRITICALSECTION_H__34F08AB2_F59E_4117_AE3F_00D38DC2C82E__INCLUDED_)
#define AFX_CRITICALSECTION_H__34F08AB2_F59E_4117_AE3F_00D38DC2C82E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MULTITHREAD_USE_EXTERN_CRITICAL_SECTION 1
#if MULTITHREAD_USE_EXTERN_CRITICAL_SECTION && !defined _WIN32
#include <pthread.h>
#endif
#include "Interlocked.h"

namespace MultiThread
{
  ///Allowes retrieve informations about current thread
  class ThreadInformation
  {
  public:
    ///Retrieves unique identifier number of current thread
    /**
     * ID can be used when you want to identify ownership of an object. ID is
     * not changed during thread lifetime. But the same ID can be used by another
     * thread when the current thread exits.
     */
    static long CurrentThreadId();
  };


  ///MiniCriticalSection provides the simplest mutable exclusive locks
  /**
  Use this class to lock very small block of critical code, that should be processed exclusive. 
  If there is a collision, second thread is stopped on global blocker, that is shared by
  all instances of MiniCriticalSection. Every thread that is leaving any instance of critical section
  notifies this blocker, which releases all waiting threads. Each relesed thread checks state of his critical 
  section and repeats waiting in case that lock is still unsuccessful. MiniCriticalSection excepts, that there
  will be very little count of waiting threads, and most of time, blocker will be empty. Too many waiting
  threads may cause reducing performance, because every thread that is leaving critical section causes restarting
  of all waiting threads at one time.

  Exclusive code should be small as possible.
  */
  class MiniCriticalSection: protected ThreadInformation
  {
  protected:
    long _owner_thread; ///<It contains ID of owning thread

    static long SWaitCount; ///<Total count of threads that waits on any mini-critical section in whole application

    bool WaitForCriticalSection(unsigned long timeout);
    void ReleaseCriticalSection();
  public:
    ///Constructs critical section with no owner
    MiniCriticalSection():_owner_thread(0) {}

    ///Tries to lock the critical section
    /**
    You can specify timeout. During waiting, thread is repeatedly trying to acquire the critical section
    @param timeout timeout in milliseconds - default parameter means infinite waiting
    @return true, if critical section has been acquired or false when timeout elapses
    @note You cannot acquire critical section multiple times. Use MiniCriticalSectionR to handle recursive locks or see LockR and UnlockR function
    */
    bool Lock(unsigned long timeout=0xFFFFFFFF)
    {
      if (MTCompareExchange(&_owner_thread,CurrentThreadId(),0)!=0)
      {
        if (timeout) return WaitForCriticalSection(timeout);
        return false;
      }
      return true;
    }


    ///Unlocks the critical section
    /**
    @note This class doesn't support recursive locks
    */
    void Unlock()
    {
      MTCompareExchange(&_owner_thread,0,CurrentThreadId());
      if (SWaitCount!=0) ReleaseCriticalSection();
    }

    ///Gets ownership state of the critical section
    /**
    @return true - critical section is owned by current thread or false - critical section is not owned
    by current thread, or critical section is not owned.
    */
    bool IamOwner()
    {
      return _owner_thread==CurrentThreadId();
    }

    ///Tries to lock the critical section
    /**
    * Works similar as Lock and Unlock. Function enables recursive locks without recursive counter
    * @param laststate Variable that will hold the last state of the lock. Variable can be unintialized.
    *  Function stores previous state of the lock. Stored state must be used by UnlockR function.
    * @retval true critical section is owned
    * @retval false timeout elapses
    */
    bool LockR(bool &laststate, unsigned long timeout=-1)
    {
      if (IamOwner()) {laststate=true;return true;}
      laststate=false;
      return Lock(timeout);
    }

    ///Unlock the critical section
    /**
    * @param laststate Variable initialized by LockR function. It holds critical section state before LockR.
    *  if this value is true, critical section remains locked. Otherwise it unlocked.
    */
    void UnlockR(bool laststate)
    {
      if (!laststate) Unlock();

    }
  };


  

  ///MiniCriticalSectionR provides the simplest mutable exclusive locks with recursive counter
  class MiniCriticalSectionR: public MiniCriticalSection
  {
  protected:
    unsigned long _recursion_count;
  public:
    MiniCriticalSectionR():_recursion_count(0) {}

    ///Tries to lock the critical section
    /**
    You can specify timeout. During waiting, thread is repeatedly trying to acquire the critical section
    @param timeout timeout in milliseconds - default parameter means infinite waiting
    @return true, if critical section has been acquired or false when timeout elapses
    @note You can call Lock multiple times for one thread. Each Lock must have Unlock
    */
    bool Lock(unsigned long timeout=0xFFFFFFFF)
    {
      if (IamOwner()) _recursion_count++;
      else
      {
        if (!MiniCriticalSection::Lock(timeout)) return false;
        _recursion_count=1;
      }
      return true;
    }

    ///Unlocks the critical section
    /**
    @note Each Unlock decrements lock counter. When counter reaches zero, critical section is unlocked
    */
    void Unlock()
    {
      if (--_recursion_count==0) MiniCriticalSection::Unlock();
    }
  };

#if MULTITHREAD_USE_EXTERN_CRITICAL_SECTION
#ifdef _WIN32
  class CriticalSection: public MiniCriticalSectionR
  {
  protected:
    CRITICAL_SECTION section;
  public:
    ///Creates critical section, and initializes it
    CriticalSection() { InitializeCriticalSection(&section); }
    ~CriticalSection() { DeleteCriticalSection(&section); }

    ///Locks the critical section
    /**
    Function locks the critical section. If critical section is already locked,
    thread will wait until critical section is unlocked.
    */
    bool Lock()
    {
      EnterCriticalSection(&section);
      return true;
    }

    ///Unlocks the critical section
    /**Unlocks the critical section and releases any waiting thread.
    @note There is difference comparing with Mutex. If thread locks the same
    critical section immediately after unlock, none waiting thread can be release, and
    locking thread can obtain ownership back without waiting.
    */

    void Unlock()
    {
      LeaveCriticalSection(&section);
    }
  };
#else
  class CriticalSection: public MiniCriticalSectionR
  {
    static pthread_mutex_t mutexInit; // = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
  protected:
    /// POSIX mutex structure.
    mutable pthread_mutex_t mutex;
  public:
    ///Creates critical section, and initializes it
    CriticalSection() { mutex = mutexInit; }
    ~CriticalSection() { pthread_mutex_destroy(&mutex); }

    ///Locks the critical section
    /**
    Function locks the critical section. If critical section is already locked,
    thread will wait until critical section is unlocked.
    */
    bool Lock()
    {
      return (pthread_mutex_lock(&mutex) != 0);
    }

    ///Unlocks the critical section
    /**Unlocks the critical section and releases any waiting thread.
    @note There is difference comparing with Mutex. If thread locks the same
    critical section immediately after unlock, none waiting thread can be release, and
    locking thread can obtain ownership back without waiting.
    */

    void Unlock()
    {
      pthread_mutex_unlock(&mutex);
    }
  };
#endif
#else // !MULTITHREAD_USE_EXTERN_CRITICAL_SECTION
  ///Standard critical section
  /**
   * Provides full mutable exclusive access to the object
   * Implementation is very fast. Running thread don't need to fall down into to system core, 
   * when critical section is unlocked. Critical section is best to guard a small pieces of
   * code, which may be serialized. 
   *
   */
  class CriticalSection: public MiniCriticalSectionR
  {
  protected:
    long volatile _wait_count;
    void volatile *_blocker;
    unsigned long _spin;  

    bool WaitForCriticalSection(unsigned long timeout);
    void ReleaseNextThread();
    void ReleaseBlocker();
  public:

    ///Creates critical section, and initializes it
    /**@param spin count of cycles during waiting for section in multiprocess system
    */
    CriticalSection(unsigned long spin=0):
        _wait_count(0),_blocker(0),_spin(spin) {}
        ~CriticalSection();

        ///Locks the critical section
        /**
        Function locks the critical section. If critical section is already locked,
        thread will wait until critical section is unlocked. Waiting can be shorted by specify
        some timeout. 
        @param timeout Timeout in milliseconds. Default value means INFINITE waiting. You can 
        use zero to try enter critical section. Function is optimized for this special case
        and will not try to obtain waiting object.
        */
        bool Lock(unsigned long timeout=0xFFFFFFFF)
        {
          long curth=CurrentThreadId();  //current thread id


          long owner=MTCompareExchange(&_owner_thread,curth,0); //mark critical section owned!
          if (owner!=0)                     //ownership failed
          {
            if (owner==curth) {_recursion_count++;return true;}
            if (timeout==0) return false;
            else return WaitForCriticalSection(timeout);
          }
          _recursion_count=1;					//this is first recursion    
          return true;
        }

        ///Unlocks the critical section
        /**Unlocks the critical section and releases any waiting thread.
        @note There is difference comparing with Mutex. If thread locks the same
        critical section immediately after unlock, none waiting thread can be release, and
        locking thread can obtain ownership back without waiting.
        */

        void Unlock()
        {
          if (_owner_thread==CurrentThreadId())	
          {
            if (--_recursion_count) return;		//decrease recursion count and if there some recursion, return now
            _owner_thread=0;      //mark critical section not owned        
            if (MTExchangeAdd(&_wait_count,0) && _blocker) ReleaseNextThread();//Try to increase lock. We should get zero for no waiting thread.        

          }
        }

        ///Changes spin count on this critical section.
        void SetSpinCount(unsigned long val) {_spin=val;}
  };
#endif


  ///Solves various problems with locking and unlocking critical section recursive
  /**
   * AutoLockR locks critical section and remembers previous state.
   * If can be used with MiniCriticalSection, because that class doesn't support
   * recursive locks. 
   *
   * Seconds advantage is that you can anytime unlock section (and lock it back)
   * regardless count of recursive locks held.
   * 
   */
  template<class T=MiniCriticalSection>
  class AutoLockR
  {
    T &_obj;
    bool laststate;
  public:
    ///lock the critical section
    /**
     * @param obj object to lock
     * @timeout timeout. To test, whether lock failed due timeout, use operator!
     */
    AutoLockR(T &obj, unsigned long timeout=-1):_obj(obj) {_obj.LockR(laststate,timeout);}
    ///destructor
    ~AutoLockR() {if (!(*this)) return; _obj.UnlockR(laststate);}
    ///
    /**
     * @retval true lock failed, object is not owned
     * @retval false lock successes, object is owned 
     */
    bool operator!() const {return !_obj.IamOwner();}

    AutoLockR(const AutoLockR<T> &other):_obj(other._obj),laststate(laststate)
    {
      const_cast<AutoLockR<T> &>(other).laststate=true;
    }
  };


};
#endif // !defined(AFX_CRITICALSECTION_H__34F08AB2_F59E_4117_AE3F_00D38DC2C82E__INCLUDED_)
