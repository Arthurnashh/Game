#include <El/elementpch.hpp>
#include "sleepyAPI.hpp"

#if SLEEPY

#include <El/HiResTime/hiResTime.hpp>
#include <El/Debugging/imexhnd.h>
#include <mmsystem.h>
#pragma comment(lib,"winmm")

const int SleepyVersion = 2;

enum SleepyMsg
{
  SleepyFrame, // 0
};

struct SleepyInit
{
  DWORD version;
  DWORD pid;
};

struct SleepyFrameMsg
{
  DWORD id;
  DWORD duration;
  SleepyFrameMsg():id(SleepyFrame){}
};


void SleepyAPI::WorkerThread()
{
  SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST); // THREAD_PRIORITY_TIME_CRITICAL
  if (_callstackPipe!=INVALID_HANDLE_VALUE)
  {
    // TODO: connect only when Sleepy has signalled it wants to connect (use some event for this)
    RetryConnect:
    {
      // allow connecting, no wait
      DWORD mode = PIPE_NOWAIT;
      SetNamedPipeHandleState(_callstackPipe,&mode,NULL,NULL);
      ConnectNamedPipe(_callstackPipe,NULL);
      mode = PIPE_WAIT;
      SetNamedPipeHandleState(_callstackPipe,&mode,NULL,NULL);
    }
    

    // post the thread ID as a first message on the pipe
    Retry:
    SleepyInit init;
    init.version = SleepyVersion;
    init.pid = GetCurrentProcessId();
    DWORD written = 0;
    BOOL ok = WriteFile(_callstackPipe,&init,sizeof(init),&written,NULL);
    if (!ok || written!=sizeof(init))
    {
      if (WaitForSingleObject(_terminate,1000)==WAIT_OBJECT_0)
      {
        return;
      }
      HRESULT err = GetLastError();
      // ERROR_PIPE_LISTENING happens before any client has connected
      // ERROR_NO_DATA happens once client has disconnected
      // ERROR_PIPE_NOT_CONNECTED happens after server called DisconnectNamedPipe
      if (err==ERROR_NO_DATA || err==ERROR_PIPE_NOT_CONNECTED)
      {
        goto RetryConnect;
      }
      if (err==ERROR_PIPE_LISTENING)
      {
        goto Retry;
      }
      LogF("Failed named pipe write, error %x",GetLastError());
      goto Retry;
    }
    
    // connected: reset disconnected flag
    _disconnected = false;
    
    SectionTimeHandle section = StartSectionTime();
    
//     HANDLE periodic = CreateEvent(NULL,FALSE,FALSE,"");
//     timeSetEvent(1,1,(LPTIMECALLBACK)periodic,NULL,TIME_CALLBACK_EVENT_SET|TIME_PERIODIC);
    
    int samples = 0;
    float timeSpent = 0;
    HANDLE waitObjects[2] = {_terminate,_frame};
    // sample as frequently as possible
    for(;;)
    {
      // if disconnected, try connecting again
      if (_disconnected) goto Retry;
      
      //DWORD res = WaitForSingleObject(_terminate,1);
      DWORD res = WaitForMultipleObjects(lenof(waitObjects),waitObjects,FALSE,1);
      if (res==WAIT_OBJECT_0)
      {
        return;
      }
      else if (res==WAIT_OBJECT_0+1)
      {
        SubmitFrame(_frameDuration);
      }
      else
      {
        SectionTimeHandle measure = StartSectionTime();
        SubmitCallstack();
        timeSpent += GetSectionTime(measure);
        
        samples++;
        float time = GetSectionTime(section);
        if (time>1.0f)
        {
          LogF("Profiler: average sample interval %.1f us, measuring time %.1f us",time*1e6f/samples,timeSpent*1e6f/samples);
          section = StartSectionTime();
          samples = 0;
          timeSpent = 0;
          
        }
      }
    }
  }
  

}

SleepyAPI::SleepyAPI()
{
  _terminate = ::CreateEvent(NULL,FALSE,FALSE,NULL);
  _frame = ::CreateEvent(NULL,FALSE,FALSE,NULL);
  // is always constructed from the main thread
  HANDLE thisThread = GetCurrentThread();
  HANDLE thisProcess = GetCurrentProcess();
  ::DuplicateHandle(thisProcess, thisThread, thisProcess, &_mainThread, 0, FALSE, DUPLICATE_SAME_ACCESS) ;
  _callstackPipe = CreateNamedPipe(
    "\\\\.\\pipe\\" SleepyAPIName(Callstacks),PIPE_ACCESS_OUTBOUND|FILE_FLAG_FIRST_PIPE_INSTANCE,
    //PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT,
    PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT,
    1, 4*1024, 4*1024, 100, NULL
  );
  
  _disconnected = false;
  
  _workerThread = CreateThread(NULL,16*1024,ThreadCallback,this,0,NULL);
}

SleepyAPI::~SleepyAPI()
{
  ::SetEvent(_terminate);
  ::WaitForSingleObject(_workerThread,INFINITE);
  ::CloseHandle(_workerThread);
  ::CloseHandle(_mainThread);
  
  // if the worker thread is still spawned, it is probably waiting inside of ConnectNamedPipe()
  // in such case we need to kill it
  ::CloseHandle(_callstackPipe);
  ::CloseHandle(_frame);
  ::CloseHandle(_terminate);
}

void SleepyAPI::Frame()
{
  DWORD now = timeGetTime();
  if (now>=_lastFrameTime)
  {
    _frameDuration = now-_lastFrameTime;
  }
  else
  {
    _frameDuration = 0;
  }
  _lastFrameTime = now;
  ::SetEvent(_frame);
  
}

void SleepyAPI::WritePipe(void *data, size_t dataSize)
{
  DWORD written;
  BOOL ok = WriteFile(_callstackPipe,data,dataSize,&written,NULL);
  if (!ok || written!=dataSize)
  {
    HRESULT hr = GetLastError();
    switch (hr)
    {
      case ERROR_NO_DATA:
        DisconnectNamedPipe(_callstackPipe);
      case ERROR_PIPE_LISTENING:
      default:
        _disconnected = true;
    }
  }
}

void SleepyAPI::SubmitCallstack()
{
  // capture the callstack of the main thread
//   SectionTimeHandle start = StartSectionTime();

  CallInfo callstack[256];
  int callstackSize = lenof(callstack);
  GDebugExceptionTrap.ExtractThreadCallstack(_mainThread,(DWORD *)GDebugExceptionTrap.GetStackBottom(), callstack, callstackSize);
  
//   float time = GetSectionTime(start);
//   LogF("duration %.1f us",time*1e6f);
  if (callstackSize>0)
  {
    // write only the called addresses to the pipe - it will ignore the rest anyway
    
    DWORD write[lenof(callstack)+1];
    write[0] = callstackSize;
    for (int i=0; i<callstackSize; i++) write[i+1] = (DWORD)callstack[i].retAddr;
    
    WritePipe(write,sizeof(*write)*(callstackSize+1));
  }
  
  // transmit it via the pipe
}


void SleepyAPI::SubmitFrame(DWORD duration)
{
  // zero or negative number means a special message
  SleepyFrameMsg msg;
  msg.duration = duration;
  WritePipe(&msg,sizeof(msg));
}

  
SleepyAPI Sleepy;

#endif

