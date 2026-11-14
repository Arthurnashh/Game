#ifndef _IRUNNABLE_INTERFACE_BISTUDIO_
#define _IRUNNABLE_INTERFACE_BISTUDIO_
#pragma once

namespace MultiThread
{


  ///Common interface for all runnable classes. 
  /** 
  Interface is abstract.
  It is declares Run method
  */
  class IRunnable
  {
  public:
    ///Virtual destructor - You can destroy any runnable object
    virtual ~IRunnable(void)=0 {}

    ///Run method 
    /**
    Callers: call this to start runnable class
    Implementors: implement any runnable...
    */
    virtual unsigned long Run()=0;
  };


};
#endif