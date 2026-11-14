#pragma once
#include "irunnable.h"

#undef Yield

namespace MultiThread
{


  class Fiber_StartupClass;

  class Fiber : public IRunnable
  {
    friend class Fiber_StartupClass;

  protected:
    void *_fiberHandle;
    Fiber *_backPtr;
    Fiber *_creator;
    size_t _maxStack;
    bool _exitState;
    unsigned long _exitCode;

  public:

    ///Contructs fiber object. 
    /**
    @param stackSize Size of memory reserved for stack. Reservation doesn't mean allocation.
    Each fiber reserves address space for its stack, but commits only first page.

    @note stackSize is supported in Windows XP and later. Previous version of Windows ignores
    this parameter and constructs default stack (mostly 1MB size).
    */
    Fiber(size_t stackSize=0);

    enum StartMode {StartNormal,StartSuspended};

    ///Starts the fiber (suspends current fiber)
    bool Start(StartMode mode=StartNormal);
    ///Resumes fiber and suspends current fiber
    bool Resume();

    ///Returns current fiber
    static Fiber *GetCurrentFiber();

    ///Stops fibber.
    /**
    @note Stopping itself works, but it is not recommended
    @note Stopping fiber removes it stack. If there are objects
    on stack, their constructors will not called. It can cause
    memory leaks.
    */
    bool Stop();

    ///Yields control of fiber, thread switches to caller
    bool Yield();

    bool IsActive()
    {
      return _fiberHandle!=0;
    }

    bool IsMySelf()
    {
      return GetCurrentFiber()==this;
    }

    ///Called before fiber exits.
    /**
    Note: Done is called in context of another fiber. GetCurrentFiber will return another 
    fiber then this
    */
    virtual void Done() {}

    unsigned long GetExitCode()
    {
      return _exitCode;
    }

    ~Fiber(void);
  };

  ///Instance of this class must be created before using the fibers
  class MasterFiber: public Fiber
  {
    Fiber *_previous;
  public:

    MasterFiber();
    ~MasterFiber();
    virtual unsigned long Run() {return 0;}
  };

};