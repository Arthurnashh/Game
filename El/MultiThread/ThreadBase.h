#pragma once
#include "irunnable.h"
#include "waitFunctions.h"

namespace MultiThread
{
class ThreadBase_InfoStruct;
class ThreadBase_StartupClass;
struct ThreadWaitInfo;
class IBlocker;


  class ThreadBase :  public IRunnable
  {
    friend class ThreadBase_StartupClass;
  public:
    enum Priority
    {
      PriorityIdle=0,
      PriorityLow=1,
      PriorityNormal=2,
      PriorityAboveNormal=3,
      PriorityHigh=4,
      PriorityHighest=5
    };

  protected:
    enum {MaxInternalDataSize=16};
    ///16 bytes to hide implementation data
    unsigned char _threadData[MaxInternalDataSize];

    ///used by implementation to access hidden implementation data
    ThreadBase_InfoStruct &IS() {return *reinterpret_cast<ThreadBase_InfoStruct *>(_threadData);}
    ///used by implementation to access hidden implementation data
    const ThreadBase_InfoStruct &IS() const {return *reinterpret_cast<const ThreadBase_InfoStruct *>(_threadData);}

    ///Current thread priority
    Priority _priority;
    ///Initial stack for creating or recreating the thread
    size_t _initStack;  


    //interface
  protected:
    ///Called before thread is started.
    /**
    @return function returns true, if thread is ready to start. If function returns false,
    Run is not called, and thread calls Done.*/
    virtual bool Init() {return true;}  

    ///Called after thread exited
    /**
    @param initFailed parameter is set to true, if Done is called after Init returns with false.
    @note Thread objects are not destroyed automatically, after thread exits. If you want to
    destroy object, overwrite this function, and call delete this, as the latest action of Done.
    */
    virtual void Done(bool initFailed) {}

    // Forbidden operations
  private:

    ThreadBase(const ThreadBase &other);
    ThreadBase &operator=(const ThreadBase &other);


  protected: // Attach functions
    ThreadBase *AttachToCurrentThread();
    void DetachThread(ThreadBase *previous);

    // Construction
  public:
    ///Constructs thread with default settings
    ThreadBase(void);
    ///Constructs thread with specified priority and initial stack size
    ThreadBase(Priority priority, size_t initStack=0);
    ///Destructor
    ~ThreadBase(void);

    // operations
  public:
    ///Starts the thread.
    /**
    @note If thread still running, function returns false. 
    */
    bool Start();

    ///Starts the thread with additional settings
    /**
    @param suspended Thread is started, but still paused. Use Resume to start thread later
    @param securityDescriptor Description about security. Depends on implementation, this
    pointer is used to point on struct with additional security options
    */
    bool Start(bool suspended, void *securityDescriptor=0);

    ///Waits to thread.
    /**Join is useful, if one thread needs to wait for second thread.
    @param timeout timeout for waiting, default value means infinite waiting
    @return true, if thread exits, false, if timeout period elapses, or thread was not started.
    Function also returns true, if Join is called on itself.
    */
    bool Join(unsigned long timeout=(unsigned long)-1) const;

    ///Returns true, if thread is running
    /**
    @return returns true, if thread is running. False means, that thread already exited, or
    was never started.
    @note When this function returns, current state of thread can be already different.
    */
    bool IsRunning() const;

    ///Sleeps thread to specified miliseconds.
    /**
    You cannot sleep remote thread. To suspend thread, call Suspend
    */
    static void Sleep(unsigned long miliseconds);

    ///Returns instance of current thread
    /**
    @tip - Use dynamic_cast to cast pointer to derived thread class. You can use
    instance to store Thread Local data. (TLS)
    */
    static ThreadBase *GetCurrentThread();

    ///Returns pointer to currently runnable class
    /**
    @note - depend on thread type, this function can return pointer to current ThreadBase,
    or to instance of IRunnable class. Use this function to access thread local variables
    (member variables of class)
    */
    static IRunnable *GetCurrentRunnable();

    ///Returns pointer to runnable class of another thread
    virtual IRunnable *GetRunnable() const {return const_cast<ThreadBase *>(this);}

    ///Suspends the thread.
    /** Function returns count of suspends.
    @note - if implementation doesn't support counting, return value must be greater than 0.
    Zero means error.
    */  
    int Suspend();

    ///Resumes the thread
    /**
    @return count of suspends BEFORE resume. -1 is error. 0 means, that thread was not suspended.
    */
    int Resume();

    ///Returns true, if current thread belongs to specified class
    bool IsMineThread() const;

    ///Sets the priority of the thread
    void SetPriority(Priority priority);

    ///Gets the current priority of the thread
    /**
    @note, it returns last used priority set by SetPriority function - not current real thread priority
    */
    Priority GetPriority() {return _priority;}

    ///Returns exit code of the thread
    /**
    @note Thread must be in finish state (not running).
    */
    unsigned long GetExitCode() const;

    ///Wait on multiple threads
    static int JoinMulti(const Array<const ThreadBase *> &threads, bool waitAll=true, unsigned long timeout=-1);

    ///Wait on multiple threads
    static int JoinMulti(bool waitAll, unsigned long timeout, int count, const ThreadBase *thr1, const ThreadBase *thr2,...);

    ///Optains blocker that is changed to signaled when thread exits
    /**
      @note pointer is valid until instance of the Thread class is destroyed. If thread is destroyed
      before it is finished, you cannot use this function, because returned pointer will already not valid
      when thread finishes
    */
    IBlocker *GetBlocker() const;
    
    ///Joins this instance with the current thread.
    /**
     It is useful, when there is a system thread, or foreign thread (created by the 3rd part library) and you
     want to attach it to the existing ThreadBase type instance. When instance is attached, thread processes all
     registered hooks, and calls Run function. After that, hooks are cleaned and thread is detached. After that,
     function exits with thread exit code.

     To join instance with the thread, the instance must be newly created and also current thread must be 
     unassigned. If thread has already attached an instance or if the instance is attached to a different thread,
     function immediatelly exits with -1;

     @return exit code of the thread. You should pass this value to the system or to the 3rd part library). 
     Depends of library implementation, return value will be or will not be used as thread result.

     @note You cannot use GetBlocker() to optain blocker object. Instead, create ThreadBlocker object.

     @note Sometime, system or library can run outside part of the thread multiple times. A mew instance of the
     ThreadBase class should be used in every cycle.
    */
    unsigned long StartCurrentThread();
    //Extra overrides
  public:
    virtual bool WaitFunction(ThreadWaitInfo &waitInfo, unsigned long timeout)
    {
      return WaitFn::DefaultWait(waitInfo, timeout);
    }

  };
  ///RunningThread class is representing currently running thread
  /**
  Currently running thread is thread that creating instance of this class. You
  can create multiple instances for one thread, bud only latest instance is active.
  Instances MUST be destroyed in reversed order than they was created.

  Use this class for storing temporally TLS variables (Thread Local Space). You can
  access this instance anytime using ThreadBase::GetCurrentThread after static cast to
  your derived class. 

  You can use this class to control the thread flow. You can Suspend and Resume the thread,
  or you can define own set of wait functions. Changing of the thread priority is not 
  recommended. You can make it, but function GetPriority will not return new priority setting.

  Constructor of RunningThread is optimized. If current thread instance exists, it only
  copies handles for thread, no operation system assistance is needed. Only if there is
  no instance for thread, handles are recreated using OS functions. This can happen
  with unknown/foreing thread, which is not created by MultiThread library. So using
  RunningThread class is the best way to add the MultiThread library functionality on
  unknown/foreing thread.  

  RunningThread class is the template. You can specify another base class, which have to
  be derived class from ThreadBase. So, any derived ThreadBase class can be used as 
  RunningThread type class by specifying this class as template argument (default is ThreadBase)
  */
  template<class ThreadType=ThreadBase>
  class RunningThread: public ThreadType
  {
    ///Previous instance of thread
    ThreadBase *_previousInstance;
    ///No runn function is needed
    virtual unsigned long Run() {return 0;}
  public:
    
    RunningThread(): _previousInstance(AttachToCurrentThread()) {}  
    ~RunningThread() {DetachThread(_previousInstance);}  

    ///Returns previous instance of thread.
    ThreadBase *GetPreviousInstance() {return _previousInstance;}
  };

  
};
