#pragma once
//NOTE: Internal header - don't use in your project.

/* Types defined in this header is platform depend */

#ifdef _WIN32

namespace MultiThread
{
  struct ThreadWaitInfo;

  ///Various wait functions
  /**
  Each thread can define implementation of wait function. Depend on thread type,
  wait function can 

  - suspend thread normally.<br>
  - during waiting, watch and pop up system messages.<br>
  - detect APC states.<br>
  etc.

  Each thread can define different type of wait function, or implement own alternative. 
  All function defined in thread library will respect this (except "fast lockers", because
  they excepts, that thread will not wait for long time).

  To define wait function for thread, override WaitForSingleObject and WaitForMultiObject
  function in ThreadBase class

  If you want to extend this class, derive it, and define new sub-classes and implementation
  of desired type of wait functions.
  */
  namespace WaitFn
  {

    const unsigned long InfiniteWait=0xFFFFFFFF;
    ///Function defines normal waiting, thread is suspended with no exceptions.
    /**
    This is default behavior.
    */
    bool DefaultWait(ThreadWaitInfo &waitInfo, unsigned long timeout=InfiniteWait);
    ///Function defines UI waiting, waiting thread can process all important messages
    bool UIWait(ThreadWaitInfo &waitInfo, unsigned long timeout=InfiniteWait);
    ///Function defines AlterAble waiting, thread can process user APC routines during waiting
    /**
    Each waiting enters the thread into "alterable" state. Thread can process any APC routine.
    Instead of WinAPI, processing the APC routine will not interrupt the waiting.
    */
    bool AlterAbleWait(ThreadWaitInfo &waitInfo,unsigned long timeout=InfiniteWait);  
  };

};
#endif