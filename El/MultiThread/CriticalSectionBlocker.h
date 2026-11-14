#pragma once
#include "iblocker.h"
#include "CriticalSection.h"


namespace MultiThread
{

  ///Class provides simple wrapper for critical section.
  /**
  Using this blocker, you will able use it as both IBlocker an CriticalSection
  */
  class CriticalSectionBlocker : public IBlocker
  {
    CriticalSection _critSect;
  public:

    ///Constructs critical section blocker
    /**
    @param spinCount - specifies count of spins before Critical Section sleeps the thread, 
    if section is locked.
    */
    CriticalSectionBlocker(unsigned long spinCount=0):_critSect(spinCount) {}

    bool Acquire(unsigned long timeout=-1)
    {
      return _critSect.Lock(timeout);
    }

    void Release()
    {
      return _critSect.Unlock();
    }

    void Block() {}

    CriticalSection *operator->() {return &_critSect;}
    const CriticalSection *operator->() const {return &_critSect;}  

    operator CriticalSection &() {return _critSect;}
    operator const CriticalSection &() const {return _critSect;}  
  };

};