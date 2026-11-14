#pragma once

namespace MultiThread
{
  
  ///Installs and controls special thread hooks
  /**
   * Thread hook is class that defines two functions. One called when any thread
   * starts and one when any thread exits.
   *
   * Your application can define any count of hooks, that will be processed for every
   * thread. This works similar as THREAD_ATTACH in DLLMain. When you install a hook 
   * class, any thread will run its function on startup. The same is done during thread exit.
   *
   * Thread hooks are very useful if you want to prepare other part of application
   * for new thread. You can prepare specific TLS data for this thread. When
   * thread is exiting, you will need unprepared TLS data, and release allocated memory.
   *
   * @par Notes:
   *
   * Thread hooks can very degrade application performance. For each thread, all hooks
   * are processed. So be very careful with installing hooks
   *
   * When hook is processed a global application lock is held. This means you
   * cannot wait on finishing other thread. This causes the deadlock, because exiting
   * thread cannot finish, because it cannot process hooks. And if there is
   * hook, which is waiting for another thread, it will wait forever.
   *
   * Normally, hooks are processed for each thread started by ThreadBase::Start. But
   * this is not done, when thread is started by other way. In this case, you
   * can initialize hooks by calling ThreadHook::ProcessStartHooks and uninitialize hooks
   * by ThreadHook::ProcessExitHooks. But don't forget: both functions must be called 
   * once per thread.
   *
   */

  class ThreadBase;

  class ThreadHook
  {
    ThreadHook *_nextHook;          

    static ThreadHook *hookList;

  public:
    ThreadHook(void);
    ~ThreadHook(void);

    ///Function is called before thread started
    /**
    Use this function to install additional services for the thread. When install is complette, function
    must call ProcessNextHook function. This function exites after thread finish his work. In remain time, hook
    can remove thread services.

    Thread can allocate additional stack space for TLS variables. This space is controled only by the hook and
    hook gets chance to deallocate this space after thread finishes its work

    @param instance pointer to thread instance. Pointer can be used for prepating the services. Pointer MUST be
    passed as parameter to the function ProcessNextHook

    @note Instance is not initialized. Hooks are processed before Init is called.

    @return function MUST return result of ProcessNextHook function calling. Only when hook fails in initialization, 
    it can return error state as exit code.
    */
    virtual unsigned long ProcessThreadHook(ThreadBase *instance)=0;


    ///Processed next hook in order
    /**
      To successfully start the thread, every hook MUST call this function. 
      @param instance the same value as parameter passed to the hook
      @return exit code of the thread. This code SHOULD be returned by the hook.
    */
    unsigned long ProcessNextHook(ThreadBase *instance) {if (_nextHook) return _nextHook->ProcessThreadHook(instance);return -1;}

   
    ///Adds new hook into hook-list
    /**
     * New hooks are added at top of list and they are processed first. 
     @param hk pointer to hook. Pointer must be valid until it is not removed
     @retval true success
     @retval false hook is already installed
     */
    static bool AddHook(ThreadHook *hk);
    /// Removes hook from hook-list
    /**
     * You can remove any hook. You don't need remove hook in reverse order.
     @param hk pointer to hook to remove. In this point pointer still must be valid. Beware
     when you calling this in destructor
     @retval true success
     @retval false hook was not found
     */
    static bool RemoveHook(ThreadHook *hk);

    /// Starts the thread instance by processing all hooks in queue
    /**
     * Function is used by thread loader. It processes all hooks and routes code into Run function in the thread
     * instance. This is normally done by ThreadBase::Start function. If there is thread not created by the
     * ThreadBase instance, to start the thread instance in the context of current thread use this function
     * 
     * @note Don't call this function of thread created by ThreadBase class. Function fails with exit code -1
     */
    static unsigned long StartThreadInstance(ThreadBase *instance);

  };

}