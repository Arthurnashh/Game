#include "common_Win.h"
#include ".\winmessageblocker.h"

namespace MultiThread{


  bool WinMessageBlocker::Acquire(unsigned long timeout)
  {
    unsigned long wf=_waitFlags?_waitFlags:QS_ALLINPUT;
    return MsgWaitForMultipleObjects(0,0,FALSE,timeout,_waitFlags)!=WAIT_TIMEOUT;
  }
  void WinMessageBlocker::Block()
  {

  }
  void WinMessageBlocker::Unblock()
  {

  }

};