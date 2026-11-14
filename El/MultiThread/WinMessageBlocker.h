#pragma once
#include "iblocker.h"

namespace MultiThread
{


  class WinMessageBlocker :  public IBlocker
  {  
    unsigned long _waitFlags;
  public:
    ///Constructs blocker that allows you wait for several message types
    /**
    @param waitFlags Flags - see MsgWaitForMultipleObject MSDN for flags
    */
    WinMessageBlocker(unsigned long waitFlags=0):_waitFlags(waitFlags) {}


    bool Acquire(unsigned long timeout=-1);
    void Block();
    void Unblock();
  };

};