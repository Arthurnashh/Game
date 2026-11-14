#ifndef _THREAD_COMMON_HEADER_H_
#define _THREAD_COMMON_HEADER_H_



#pragma once

///defines common "internal" functions.
/**
Don't include this header into your project. It is used by Window implementation only
*/

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501
#define WINVER 0x501
#include <tchar.h>
#include <es/common/win.h>
#include <stdlib.h>
#include <process.h>
#include "MTCommon.h"
#include "Interlocked.h"
#include "Synchronized.h"
#include "CriticalSection.h"

#include "IBlocker.h"


namespace MultiThread
{


  class ThreadInternal
  {
  public:
    ///Provides waiting for single object.
    /**
    Implementation takes current thread instance and call one of override wait functions
    */

    static bool WaitForSingleObject(HANDLE object, DWORD timeout);

    ///Provides waiting for multiple objects
    /**
    Implementation takes current thread instance and call one of override wait functions
    */

    static int WaitForMultipleObjects(HANDLE *objects, int count, bool waitAll, DWORD timeout);
  };

  class BlockerInternal_InfoStruct
  {
  public:
    HANDLE hBlock;
  };


  class ThreadBase_InfoStruct: public IBlocker
  {
  public:
    unsigned long threadId;	///<thread ID
    HANDLE hThread;			///<thread handle
    MiniCriticalSection start; ///<mini critical section solves some MT collisions

    bool Acquire(unsigned long timeout=-1)
    {
      if (hThread==0) return true;
      return ThreadInternal::WaitForSingleObject(hThread,timeout);
    }

    virtual void Release() {}
  };

};
#endif