#pragma once
#include "IBlocker.h"
#include "ThreadBase.h"

namespace MultiThread
{


  ///Sleep blocker provides simple sleep operation
  /**
  When thread acquires this blocker, it must sleep for specified miliseconds.
  No other event can interrupt this sleeping.
  */
  class SleepBlocker : public IBlocker
  {
    unsigned long _time; ///< time in milliseconds
  public:
    ///Constructs SleepBlocker
    /**
    @param miliseconds - time in miliseconds to sleep
    */
    SleepBlocker(unsigned long miliseconds):_time(miliseconds) {}

    ///Acquires the blocker
    /**
    Thread will go to sleep
    @param timeout - maximum time for sleeping. If specified timeout is 
    lower then sleep-time, function does nothing and return false
    @return true - sleep done, false - timeout elapses.
    */
    bool Acquire(unsigned long timeout=-1)
    {
      if (timeout<time) return false;
      ThreadBase::Sleep(_time);
      return true;
    }

    /**
    @note Nothing
    */
    void Release() {}
  };

};