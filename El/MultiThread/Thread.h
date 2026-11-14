#pragma once
#include "ThreadBase.h"

namespace MultiThread
{

  ///Class provides wrapper for any IRunnable class type
  class Thread : public ThreadBase
  {
    IRunnable *_runnable;
  public:

    ///Creates thread object and wraps runnable class
    Thread(IRunnable *r):_runnable(r)  {}
    ///Creates thread object and wraps runnable class
    Thread(IRunnable *r,Priority priority, size_t initStack=0):_runnable(r),ThreadBase(priority,initStack) {}


    ///Implements Run - starts runnable
    unsigned long Run()
    {
      return _runnable->Run();
    }

    ///Returns pointer to runnable class
    virtual IRunnable *GetRunnable() const {return _runnable;}

  };


  ///Implements dynamic allocated Thread that starts statically or auto allocated runnable
  /**
  When thread exists, Thread is destroyed, bud runnable leaved.
  */
  class ThreadDyn: public Thread
  {
  public:
    ThreadDyn(IRunnable *r): Thread(r) {}
    ThreadDyn(IRunnable *r,Priority priority, size_t initStack=0):Thread(r,priority,initStack) {}
    virtual void Done(bool initFailed)
    {
      delete this;
    }
  };

};