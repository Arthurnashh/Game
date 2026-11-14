#include <Es/essencepch.hpp>
#include <Es/Common/win.h>

#ifdef _MSVC
# pragma once
#endif

#ifndef _SLEEPY_API_HPP
#define _SLEEPY_API_HPP

#ifndef SLEEPY
# if _ENABLE_PERFLOG && _WIN32
#  define SLEEPY 1
# else
#  define SLEEPY 0
# endif
#endif

#if SLEEPY
class SleepyAPI: private NoCopy
{
  //protected:
  /// server end of the communication pipe
  HANDLE _callstackPipe;
  /// signal worker thread to terminate
  HANDLE _terminate;
  /// profiling worker thread
  HANDLE _workerThread;
  /// handle to the thread being profiled
  HANDLE _mainThread;
  /// event - frame
  HANDLE _frame;
  /// TODO: synchronize with event - use message instead of event?
  DWORD _frameDuration;
  
  DWORD _lastFrameTime;
  
  bool _disconnected;
  
  void WritePipe(void *data, size_t dataSize);
  
  
  void SubmitCallstack();
  void SubmitFrame(DWORD duration);
  
  
  void WorkerThread();
  
  static DWORD WINAPI ThreadCallback(LPVOID parameter)
  {
    ((SleepyAPI *)parameter)->WorkerThread();
    return 0;
  }

  public:
  SleepyAPI();
  ~SleepyAPI();
  void Frame();
  
};

extern SleepyAPI Sleepy;

#define SleepyAPINameBase "BIS_{B8D1E291-55CA-49a6-A1DD-EFB9C32582DB}"
#define SleepyAPIName(XXX) SleepyAPINameBase #XXX

#endif

#endif
