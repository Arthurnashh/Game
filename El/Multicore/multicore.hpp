/// Helper functions and classes for environment with multiple CPUs

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MULTICORE_HPP
#define _MULTICORE_HPP

#include <Es/Common/win.h>
#include <Es/platform.hpp>
#include <Es/Threads/multisync.hpp>
#include <Es/Strings/rString.hpp>

#ifndef _WIN32
  #define DUMMY_MULTICORE 0
  #define DUMMY_MICROJOBS 1
  /* Use MultiThread library to replace:
  *   Event     for MultiThread::EventBlocker
  *   Semaphore for MultiThread::SemaphoreBlocker
  * Note: this can be turned ON also in Win32 to test it during gameplay
  */
  #define USE_MT_LIB_FOR_JOBS 1
#else
  #define DUMMY_MULTICORE 0
  #define DUMMY_MICROJOBS 0
  // use Win32 Synchronization Classes
  #define USE_MT_LIB_FOR_JOBS 0
#endif

#if USE_MT_LIB_FOR_JOBS
#include "El/FileServer/multithread.hpp"

#ifndef _WIN32
  #define LPVOID void *
  typedef DWORD ( *LPTHREAD_START_ROUTINE )( LPVOID lpThreadParameter );
#endif

class ThreadObject : public MultiThread::ThreadBase
{
  LPTHREAD_START_ROUTINE _start;
  LPVOID _param;
public:
  ThreadObject(int stackSize, LPTHREAD_START_ROUTINE start, LPVOID param, MultiThread::ThreadBase::Priority priority)
    : MultiThread::ThreadBase(priority, stackSize), _start(start), _param(param)
  {}
  unsigned long Run()
  {
    _start(_param);
    return 0;
  }
};
#endif


/// force the number of CPUs GetCPUCount() will return
void ForceCPUCount(int count);


/// return the current number of CPUs
int GetCPUCount();

#if !DUMMY_MULTICORE
#if USE_MT_LIB_FOR_JOBS
  MultiThread::ThreadBase *CreateMTThreadOnCPU(
    DWORD stackSize, LPTHREAD_START_ROUTINE startAddress, LPVOID parameter, int cpu, MultiThread::ThreadBase::Priority priority, const char *name
    );
#endif
/// create the thread on given CPU
HANDLE CreateThreadOnCPU(
    DWORD stackSize, LPTHREAD_START_ROUTINE startAddress, LPVOID parameter, int cpu, int priority, const char *name,
    DWORD *threadId=NULL
);
#endif

/// register thread to be able receive its name and cpu later
void RegisterThread(DWORD threadId, int cpu, const char *name);
/// find the name of the registered thread
RString GetThreadName(DWORD threadId);
/// find the cpu of the registered thread
int GetThreadCPU(DWORD threadId);

#if !_SUPER_RELEASE && defined _WIN32
#define USE_THREAD_NAME 1
#endif


#if USE_THREAD_NAME
  void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName);
#else
  inline void SetThreadName(DWORD dwThreadID, const char * szThreadName) {}
#endif

#endif

