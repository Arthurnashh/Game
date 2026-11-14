#pragma once
#include "iblocker.h"
#include "MTCommon.h"

namespace MultiThread
{

  class BlockerInternal_InfoStruct;
  class ThreadBase;

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

  public:
    BlockerSimpleBase(void);
    ~BlockerSimpleBase(void);

    BlockerSimpleBase(const BlockerSimpleBase &other);
    BlockerSimpleBase& operator=(const BlockerSimpleBase &other);

    ///Allowes to wait to multiple blockers at time
    /**
    Function allows to wait on multiple blockers derived from this class down.
    @param blockerList Array of blockers
    @param waitAll true - wait for all objects, false - wait for any object
    @param timeout - timeout in milliseconds
    @return 0 - timeout elapses, -1 - error, other value is one based index to array. If waitAll
    is true, returns 1.  
    */
    static int WaitForMultiple(const Array<const BlockerSimpleBase *> &blockerList, bool waitAll, unsigned long timeout);
  };

  ///Event blocker.
  /**
  Event blocker is blocker that allows you anytime switch this blocker into blocker or unblocked
  */
  class EventBlocker: public BlockerSimpleBase
  {
  public:
    enum Mode
    {
      Blocked_AutoReset, ///< Event is blocked + Auto Reset event
      Unblocked_AutoReset,///< Event is unblocked + Auto Reset event
      Blocked_ManualReset,///< Event is blocked + Manual Reset event
      Unblocked_ManualReset ///< Event is unblocked + Manual Reset event
    };  
    EventBlocker(Mode mode=Blocked_AutoReset, void *securityDescriptor=0);  
    EventBlocker(const EventBlocker &other);
    EventBlocker &operator=(const EventBlocker &other);

    ///Acquire the event
    bool Acquire(unsigned long timeout=-1);
    ///Nothing
    void Release();
    ///Unblock the event
    void Unblock();
    ///Block the event
    void Block();
    ///Pulse the event
    void Pulse();
  };

  class MutexBlocker: public BlockerSimpleBase
  {
  public:
    MutexBlocker(bool initOwner=false, void *securityDescriptor=0);
    MutexBlocker(const MutexBlocker &other);
    MutexBlocker &operator=(const MutexBlocker &other);

    ///Acquire and lock Mutex
    bool Acquire(unsigned long timeout=-1);
    ///Release the mutex
    void Release();
  };

  class SemaphoreBlocker: public BlockerSimpleBase
  {
  public:
    SemaphoreBlocker(int maximumCount,int initialCount=-1, void *securityDescriptor=0);
    SemaphoreBlocker(const SemaphoreBlocker &other);
    SemaphoreBlocker &operator=(const SemaphoreBlocker &other);

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
    ProcessBlocker(unsigned long pid, void *securityDescriptor=0);
    ProcessBlocker(const ProcessBlocker &other);
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

    ///Waits until process is not in idle state and ready to process incomming events
    /**
     * @param timeout timeout to wait - default means infinite waiting 
     * @retval true success - process is ready
     * @retval false failed - timeout ellapses or error state. If timeout is set to infinite,
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
    */
    ThreadBlocker(unsigned long tid, void *securityDescriptor=0);

    ///You can create blocker from ThreadBase object
    /**
    This has one big advantage. After blocker is created, it is still valid even if the thread
    has been terminated and the corresponding ThreadBase object has been destroyed. 
    When thread exits, this blocker unblocks any waiting thread.

    Alternatively you can call ThreadBase::GetBlocker function to obtain Thread blocker.
    In this case, blocker life depend on life of thread object.

    */
    ThreadBlocker(const ThreadBase &thr);
    ThreadBlocker(const ThreadBlocker &other);
    ThreadBlocker &operator=(const ThreadBlocker &other);

    ///Waits until thread exits
    bool Acquire(unsigned long timeout=-1);
    ///nothing
    void Release();
  };

};