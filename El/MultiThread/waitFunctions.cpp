#include "common_Win.h"
#include "waitFunctions_win.h"
#include "waitFunctions.h"

namespace MultiThread
{
namespace WaitFn
{
  bool DefaultWait(ThreadWaitInfo &waitInfo, unsigned long timeout)
  {
    if (waitInfo.count==1)
    {    
      waitInfo.index=1;
       return ::WaitForSingleObject(*waitInfo.hList,timeout)!=WAIT_TIMEOUT;
    }
    else
    {
      DWORD res=WaitForMultipleObjects(waitInfo.count,waitInfo.hList,waitInfo.waitAll,timeout);
      if (res>=WAIT_OBJECT_0 && res<WAIT_OBJECT_0+waitInfo.count)
      {
        if (waitInfo.waitAll) {waitInfo.index=1;return true;}
        else {waitInfo.index=res-WAIT_OBJECT_0+1;return true;}
      }
      if (res>=WAIT_ABANDONED_0 && res<WAIT_ABANDONED_0+waitInfo.count)
      {
        if (waitInfo.waitAll) {waitInfo.index=1;return true;}
        else {waitInfo.index=res-WAIT_ABANDONED_0+1;return true;}
      }  
      if (res!=WAIT_TIMEOUT) {waitInfo.index=0;return true;}
      return false;
    }
  }

  bool UIWait(ThreadWaitInfo &waitInfo, unsigned long timeout)
  {
    DWORD curTime=GetTickCount();
    DWORD finishWait=curTime+(timeout & 0x7FFFFFFF);
    while (curTime<=finishWait)
    {
      DWORD remain=timeout==InfiniteWait?INFINITE:(finishWait-curTime);
      if (waitInfo.waitAll && remain>100) remain=100; //minimum wait, we need check message queue
      DWORD res=MsgWaitForMultipleObjects(waitInfo.count,waitInfo.hList,waitInfo.waitAll,remain,QS_SENDMESSAGE);
      if (res>=WAIT_OBJECT_0 && res<WAIT_OBJECT_0+waitInfo.count)
      {
        waitInfo.index=res-WAIT_OBJECT_0;
        return true;
      }
      MSG msg;
      while (PeekMessage(&msg,0,0,0,PM_REMOVE | PM_NOYIELD | PM_QS_SENDMESSAGE))
      {
        DispatchMessage(&msg);
      }
      curTime=GetTickCount();
    }
    return false;
  }
};

};