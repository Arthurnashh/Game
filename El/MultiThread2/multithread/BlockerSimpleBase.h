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

#pragma once
#include "IBlocker.h"
#include "MTCommon.h"

namespace MultiThread
{

  class BlockerInternal_InfoStruct;
  class ThreadBase;

  ///Base class for all blockers
  /**
   * This class is abstract. To use it, create instance of any derived class
   */
  class BlockerSimpleBase : public IBlocker
  {
  protected:
    enum {privateAreaSize=8};
    char privateArea[privateAreaSize];

    BlockerInternal_InfoStruct &IS()
    {
      return *reinterpret_cast<BlockerInternal_InfoStruct *>(privateArea);
    }

    const BlockerInternal_InfoStruct &IS() const
    {
      return *reinterpret_cast<const BlockerInternal_InfoStruct *>(privateArea);
    }

    BlockerSimpleBase(const BlockerSimpleBase &other);
    BlockerSimpleBase& operator=(const BlockerSimpleBase &other);
  public:    
    BlockerSimpleBase(void);
    ~BlockerSimpleBase(void);


    ///Allowes to wait to multiple blockers at time
    /**
    Function allows to wait on multiple blockers derived from this class down.
    @param blockerList Array of blockers
    @param waitAll true - wait for all objects, false - wait for any object
    @param timeout - timeout in milliseconds
    @return 0 - timeout elapses, -1 - error, other value is one based index to array. If waitAll
    is true, returns 1.  
    */
    static int WaitForMultiple(const BredyLibs::Array<const BlockerSimpleBase *> &blockerList, bool waitAll, unsigned long timeout);
  };

  ///Event blocker.
  /**
  Event blocker is blocker that allows you anytime switch this blocker into blocker or unblocked
  */
  class EventBlocker: public BlockerSimpleBase
  {
    EventBlocker(const EventBlocker &other);
    EventBlocker &operator=(const EventBlocker &other);
  public:
    enum Mode
    {
      Blocked_AutoReset, ///< Event is blocked + Auto Reset event
      Unblocked_AutoReset,///< Event is unblocked + Auto Reset event
      Blocked_ManualReset,///< Event is blocked + Manual Reset event
      Unblocked_ManualReset ///< Event is unblocked + Manual Reset event
    };  
    EventBlocker(Mode mode=Blocked_AutoReset, void *securityDescriptor=0);  

    ///Acquire the event
    bool Acquire(unsigned long timeout=(size_t)-1);
    ///Nothing
    void Release();
    ///Unblock the event
    void Unblock();
    ///Block the event
    void Block();
    ///Pulse the event
    void Pulse();
  };

  ///Provides simple Mutable Exclusive lock
  class MutexBlocker: public BlockerSimpleBase
  {
    MutexBlocker(const MutexBlocker &other);
    MutexBlocker &operator=(const MutexBlocker &other);
  public:

    ///Construction
    /**
     * @param initOwner when true, thread that creating this object gets ownership by default,
     *   so mutex is in blocking state when created. To unblock it, call Release()
     * @param securityDescriptor Platform depend descriptor, use NULL to construct the object 
     *   with default descriptor
     */
    MutexBlocker(bool initOwner=false, void *securityDescriptor=0);

    ///Acquire and lock Mutex
    bool Acquire(unsigned long timeout=-1);
    ///Release the mutex
    void Release();
  };

  ///Provides semaphore blocker
  class SemaphoreBlocker: public BlockerSimpleBase
  {
    SemaphoreBlocker(const SemaphoreBlocker &other);
    SemaphoreBlocker &operator=(const SemaphoreBlocker &other);
  public:
    ///Construction
    /**
     * @param maximumCount Maximum count of threads,  that can own the semaphore
     * @param initialCount Count of threads that "virtually" owns this semaphore after
     *   construction. This parameter can be ignored in some platforms. It is better
     *   to leave default value
     * @param securityDescriptor  Platform depend descriptor, use NULL to construct the object 
     *   with default descriptor
     */
    SemaphoreBlocker(int maximumCount,int initialCount=-1, void *securityDescriptor=0);

    ///Acquire and lock Semaphore
    bool Acquire(unsigned long timeout=-1);
    ///Release the Semaphore
    void Release();
  };

  ///Blocks until specified process exits
  /**
  This blocker can block thread, until specified process exits.
  Process is identified by its PID
  */
  class ProcessBlocker: public BlockerSimpleBase
  {
  public:
    ///Construction
    /**
     * @param pid Process ID. Blocker is unblocked after given process exits
     * @param securityDescriptor  Platform depend descriptor, use NULL to construct the object 
     *   with default descriptor
     */
    ProcessBlocker(unsigned long pid, void *secuityDescriptor=0);
    ///Copy constructor
    /**
     * Creates a new blocker with the same behavior as original (refers the same PID) 
     */
    ProcessBlocker(const ProcessBlocker &other);
    ///Assignment
    /**
     * This is the way, how to switch this object into blocked state. You can
     * assign to this object another PID, and object blocks, until given process exits
     */
    ProcessBlocker &operator=(const ProcessBlocker &other);

    ///Waits until process exits
    bool Acquire(unsigned long timeout=-1);
    ///Nothing
    void Release();
    ///Returns process's exit code
    /**
     * @return Exit code of finished process. This code is valid only when process already finished. Call Acquire before.
     */
    unsigned long GetExitCode() const;

    ///Waits until process is not in idle state and ready to process incoming events
    /**
     * @param timeout timeout to wait - default means infinite waiting 
     * @retval true success - process is ready
     * @retval false failed - timeout elapses or error state. If timeout is set to infinite,
     * this mean error - process is finished or another error condition
     */
    bool WaitForProcessReady(unsigned long timeout=-1);
  };

  ///Blocks until specified thread exists
  class ThreadBlocker: public BlockerSimpleBase
  {
  public:
    ///You can create blocker from TID (thread ID)
    /**
    @param tid  Thread Identifier
    @param securityDescriptor optional security descriptor platform depend.
    @note Constructor is not supported in Linux implementation. Object need some extra
     informations, that are not accessible through thread ID. 
    */
    ThreadBlocker(unsigned long tid, void *securityDescriptor=0);

    ///You can create blocker from ThreadBase object
    /**
    @param thr Thread object. When blocker is created, reference to the thread object
     is no more held. Thread object can be destroyed anytime later. Created instance is blocked
     while thread is running and it is unblocked after thread's exit.
    */
    ThreadBlocker(const ThreadBase &thr);
    ///Copy constructor
    /**
    * Creates a new blocker with the same behavior as original (refers the same thread) 
    */
    ThreadBlocker(const ThreadBlocker &other);
    ///Assignment
    /**
    * This is the way, how to switch this object into blocked state. You can
    * assign to this object another thread, and object blocks, until given thread exits
    */
    ThreadBlocker &operator=(const ThreadBlocker &other);

    ///Waits until thread exits
    bool Acquire(unsigned long timeout=-1);
    ///nothing
    void Release();
    ///Returns thread's exit code
    /**
    * @return Exit code of finished thread. This code is valid only when thread already finished. 
    * Call Acquire before.
    * @note Function can be called event if thread object has been destroyed.
    */
    unsigned long GetExitCode() const;

    ~ThreadBlocker();
  };

}

