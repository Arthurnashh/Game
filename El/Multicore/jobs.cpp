/// Job base class, Job Manager class implementation

#include <El/elementpch.hpp>
#include "Es/Framework/appFrame.hpp"
#include "jobs.hpp"
#include <Es/Containers/staticArray.hpp>
# include <El/Common/perfProf.hpp>

// switch on to measure overhead cost of micro-jobs 
#define MICRO_TASKS_PERF 0

Job::Job()
{
  _jobState = JSNone;
  _jobCommand = JCNone;
  _threadNumber = -1;
  _jobFinishUntil = 0;
}

Job::~Job()
{
  // job should never be destroyed while processed or waiting to processing
  DoAssert(_jobState<JSWaiting || _jobState>JSSuspended);
}
void JobsQueue::Insert(Job *job)
{
  Assert(job);
  ScopeLockSection lock(_queueLock);
  _queue.HeapInsert(job);
  _submitSemaphore.Unlock();
}

bool JobsQueue::RemoveFirst(Job *&result)
{
  ScopeLockSection lock(_queueLock);
  return _queue.HeapRemoveFirst(result);
}

bool JobsQueue::Cancel(Job *job)
{
  Assert(job);
  ScopeLockSection lock(_queueLock);
  int index = _queue.Find(job);
  if (index < 0) return false;
  // try to recover the semaphore
  if (!_submitSemaphore.TryLock())
  {
    LogF("Cancel job did not lock the submit semaphore, lock count broken");
  }
  // we can remove it now
  _queue[index]->_jobState = Job::JSCanceled;
  _queue.HeapDeleteAt(index);
  return true;
}

void JobsQueue::Clear()
{
  ScopeLockSection lock(_queueLock);
  // recover the semaphore
  for (int i=0; i<_queue.Size(); i++)
  {
    Verify(_submitSemaphore.TryLock());
    _queue[i]->_jobState = Job::JSCanceled;
  }
  _queue.Clear();
}

bool JobManagerThread::Init(int threadNumber, JobsQueue *queue)
{
#if DUMMY_MULTICORE
  return false; //no implementation
#else
  DoAssert(!_threadHandle); // avoid multiple initialization

  _threadNumber = threadNumber;
  _queue = queue;

  int cpuNumber = threadNumber + 1; // begin from CPU #1
  // do not limit the main application and micro-jobs threads
#if USE_MT_LIB_FOR_JOBS
  MultiThread::ThreadBase *manThread = CreateMTThreadOnCPU(64*1024, JobManagerThread::ThreadProcedureCallback, this,
    cpuNumber, MultiThread::ThreadBase::PriorityLow, Format("Job#%d", threadNumber+1));
  if (manThread)
  {
    _threadHandle.Init(manThread);
    return true;
  }
  else
  {
    RptF("Cannot create job manager thread");
    return false;
  }
#else
  HANDLE handle = CreateThreadOnCPU(64 * 1024, JobManagerThread::ThreadProcedureCallback, this,
    cpuNumber, THREAD_PRIORITY_LOWEST, Format("Job#%d", threadNumber+1));
  if (handle)
  {
    _threadHandle.Init(handle);
    return true;
  }
  else
  {
    RptF("Cannot create job manager thread");
    return false;
  }
#endif
#endif
}

void JobManagerThread::Done()
{
  if (_threadHandle)
  {
    // tell the thread to finish
    _terminateEvent.Set();
    // tell the current job to finish immediately
    Cancel();
    // wait until it is done
    _threadHandle.Wait();
    // finish
    _threadHandle.Done();
  }
}

void JobManagerThread::Cancel()
{
  {
    // make sure nobody will set _currentJob to NULL
    ScopeLockSection lock(_currentJobLock);
    if (!_currentJob) return;

    // cancel and wait until done
    _currentJob->CancelJob();
  }
  _cancelEvent.Wait();
}

bool JobManagerThread::Cancel(Job *job)
{
  {
    // make sure nobody will set _currentJob to NULL
    ScopeLockSection lock(_currentJobLock);
    if (_currentJob != job) return false;

    // cancel and wait until done
    _currentJob->CancelJob();
  }
  _cancelEvent.Wait();
  return true;
}

DWORD JobManagerThread::ThreadProcedureCallback(void *context)
{
  // redirect to the member function
  JobManagerThread *instance = reinterpret_cast<JobManagerThread *>(context);
  return instance->ThreadProcedure();
}

DWORD JobManagerThread::ThreadProcedure()
{
  #ifndef _M_PPC
  // there is no precision control on PPC, precision is controlled by instructions instead
//  Assert((_controlfp(0, 0)&_MCW_PC)==_PC_24);
  #endif
  while (true)
  {
    // wait until some job is ready or termination
#if USE_MT_LIB_FOR_JOBS
    BlockerArItem handles[] = {&_terminateEvent, _queue->GetSubmitSemaphore()};
    int result = WaitForMultiple(handles, 2, false, INFINITE);
    if (result != 2) break; // done 2==SubmitSemaphore
#else
    SignaledObject *handles[] = {&_terminateEvent, _queue->GetSubmitSemaphore()};
    int result = SignaledObject::WaitForMultiple(handles, lenof(handles));
    if (result == 0) break; // done
#endif

    // get the job
    Job *newJob;
    if (!_queue->RemoveFirst(newJob) || newJob == NULL)
    {
      LogF("Announced job not found in the queue, lock count recovered after Cancel job.");
      continue;
    }
    // set the properties
    newJob->_jobState = Job::JSRunning;
    newJob->_threadNumber = _threadNumber;

    {
      ScopeLockSection lock(_currentJobLock);
      _currentJob = newJob;
    }

    // work
    bool done = _currentJob->Process();

    Job *currentJob = _currentJob;
    {
      ScopeLockSection lock(_currentJobLock);
      _currentJob = NULL;
      // currentJob cannot be canceled from now

      // ensure Process finished with some reason
      Assert(done || currentJob->_jobCommand != Job::JCNone);
      // fix the state - do not suspend finished task
      if (done && currentJob->_jobCommand == Job::JCSuspend) currentJob->_jobCommand = Job::JCNone;

      // we are done with the job, set the properties
      currentJob->_threadNumber = -1;
      switch (currentJob->_jobCommand)
      {
      case Job::JCNone:
        currentJob->_jobState = Job::JSDone;
        _queue->StateChanged();
        break;
      case Job::JCSuspend:
        currentJob->_jobState = Job::JSSuspended;
        currentJob->_jobCommand = Job::JCNone;
        break;
      case Job::JCCancel:
        currentJob->_jobState = Job::JSCanceled;
        currentJob->_jobCommand = Job::JCNone;
        // let the canceling thread known it is done
        _cancelEvent.Set();
        _queue->StateChanged();
        break;
      }
    }
#define DEBUG_JOBS 0
#if DEBUG_JOBS
    static int jobCount = 0;
    if (jobCount++ % 100 == 0) RptF("JobsCount: %d", jobCount);
#endif
  }
  return 0;
}

void JobManager::Wait(Job *job)
{
  // wait only when running or waiting to run, cannot wait for it
  while (job->GetJobState()<Job::JSCanceled)
  {
    _queue.WaitForStateChanged();
  }
}

/**
@param threadNumber 1 .. CPUCount()-1
*/
bool MicroJobsThread::Init(int threadNumber, MicroJobsList *jobs)
{
#if DUMMY_MICROJOBS
  return false;  //no implementation
#else
  DoAssert(!_threadHandle); // avoid multiple initialization

  _jobs = jobs;

  int cpuNumber = threadNumber+1; // begin from CPU #1

  // do not limit the main application and micro-jobs threads
  HANDLE handle = CreateThreadOnCPU(512 * 1024, MicroJobsThread::ThreadProcedureCallback,
    this, cpuNumber, THREAD_PRIORITY_NORMAL, Format("MJob#%d", threadNumber+1));
  if (handle)
  {
    _threadHandle.Init(handle);
    return true;
  }
  else
  {
    RptF("Cannot create micro jobs thread");
    return false;
  }
#endif
}

void MicroJobsThread::Done()
{
  if (_threadHandle)
  {
    // tell the thread to finish
    _terminateEvent.Set();
    // wait until it is done
    _threadHandle.Wait();
    // finish
    _threadHandle.Done();
  }
}

DWORD MicroJobsThread::ThreadProcedureCallback(void *context)
{
  // redirect to the member function
  MicroJobsThread *instance = reinterpret_cast<MicroJobsThread *>(context);
  return instance->ThreadProcedure();
}

DWORD MicroJobsThread::ThreadProcedure()
{
#if DUMMY_MICROJOBS
  return 0;
#else
  #ifndef _M_PPC
  // there is no precision control on PPC, precision is controlled by instructions instead
//  Assert((_controlfp(0, 0)&_MCW_PC)==_PC_24);
  #endif
  
  while (true)
  {
    // wait until some task is ready or until termination
    {
      //PROFILE_SCOPE_DETAIL_EX(mJobE,*);
#if USE_MT_LIB_FOR_JOBS
    BlockerArItem handles[] = {&_terminateEvent, &_jobs->_readyEvent[_thrIndex-1]};
        int result = WaitForMultiple(handles, 2, false, INFINITE);
        if (result != 2) break; // done 2==readyEvent
#else
    SignaledObject *handles[] = {&_terminateEvent, &_jobs->_readyEvent[_thrIndex-1]};
        int result = SignaledObject::WaitForMultiple(handles, lenof(handles));
        if (result == 0) break; // done
#endif
    }

    // check false start: if the corresponding bit in the wake up mask was already reset, ignore the request
    // it means we are in process of resetting the event
    // report we are pending before checking wake up - while pending, we need to be covered by the flag
    InterlockedIncrementAcquire(&_jobs->_threadsPending);
    
    PROFILE_SCOPE_EX(mJob,*);
    
    MemorySubscribe();
    if (!_jobs->_dataReady)
    {
      InterlockedDecrementRelease(&_jobs->_threadsPending); // we are actually not pending
        PROFILE_SCOPE_DETAIL_EX(mJobW,*);
      continue;
    }
    

    // some jobs are ready, process all of them
    MicroJobsList::Schedule *schedule = &_jobs->_schedule[_jobs->_actSchedule[_thrIndex]];
    
    while (true)
    {
      // get the task
      int index = InterlockedIncrementAcquire(&schedule->_started);
      if (index >= schedule->_end)
      {
        // no more jobs in our schedule, try to switch to another schedule to steal
        for (int so = 1; so<_jobs->_threadCount; so++)
        {
          int s = _thrIndex+so;
          if (s>=_jobs->_threadCount) s -= _jobs->_threadCount;
          // try stealing
          MicroJobsList::Schedule *stealFrom = &_jobs->_schedule[s];
          index = InterlockedIncrementAcquire(&stealFrom->_started);
          if (index >= stealFrom->_end) continue;
          // next iteration attempt the same stealing
          _jobs->_actSchedule[_thrIndex] = s;
          goto DoTask;
        }
        // if there is nobody to steal from, terminate
        break;
      }
      DoTask:
      DoAssert(_jobs->_threadsPending>0);
      // execute the task
      IMicroJob *task = _jobs->_jobsData[index];
      (*task)(_context,_thrIndex);
      DoAssert(_jobs->_threadsPending>0);
    }
    // signal thread finish
    DoAssert(_jobs->_threadsPending>0);
    InterlockedDecrementRelease(&_jobs->_threadsPending);
  }
  return 0;
#endif
}

JobManager::JobManager()
{
  // we need to be able to represent CPUs using a 32b bitmap
  _microJobsCountLimit=31;
}
JobManager::~JobManager()
{
  Done();
}

void JobManager::Init()
{
  // decide what number of threads we will manage
  int cpuCount = GetCPUCount();
  if (cpuCount <2) return; // no available CPU
  // to stay future proof, ignore more CPUs than we can handle
  if (cpuCount>MaxMicroJobThreads) cpuCount = MaxMicroJobThreads;
  // we could consider leaving one CPU for service threads when available on multi-core systems 
  // leaving one core unused seems better on Vista systems (probably because of drivers using threads)
  int threadsCount = intMax(1, cpuCount - 2);
  //int threadsCount = cpuCount - 1;
  
  _microJobsThreads.Realloc(threadsCount);
  for (int i=0; i<threadsCount; i++)
  {
    // index as i+1, because 0 is used for the main thread
    Ref<MicroJobsThread> thread = new MicroJobsThread(i+1);
    if (thread->Init(i, &_microJobsList)) _microJobsThreads.Add(thread);
  }

  _threads.Realloc(threadsCount);
  for (int i=0; i<threadsCount; i++)
  {
    Ref<JobManagerThread> thread = new JobManagerThread();
    if (thread->Init(i, &_queue)) _threads.Add(thread);
  }
}

void JobManager::Done()
{
  // TODO: optimization - finish all threads at once instead one by one
  _queue.Clear();
  _threads.Clear();

  _microJobsThreads.Clear();
}

void JobManager::CancelAllJobs()
{
  _queue.Clear();
  // let threads running, only cancel the running jobs
  for (int i=0; i<_threads.Size(); i++) _threads[i]->Cancel();
}

void JobManager::CreateJob(Job *job, DWORD timeMs)
{
  Assert(job->_jobState == Job::JSNone || job->_jobState == Job::JSCanceled || job->_jobState == Job::JSDone);
  Assert(job->_jobCommand == Job::JCNone);
  Assert(job->_threadNumber == -1);

  job->_jobState = Job::JSWaiting;
  job->_jobFinishUntil = GlobalTickCount() + timeMs;
  _queue.Insert(job);
}

void JobManager::ResumeJob(Job *job)
{
  Assert(job->_jobState == Job::JSSuspended);
  Assert(job->_jobCommand == Job::JCNone);
  Assert(job->_threadNumber == -1);

  job->_jobState = Job::JSWaiting;
  // keep the _jobFinishUntil unchanged
  _queue.Insert(job);
}

void JobManager::CancelJob(Job *job)
{
  if (job->_jobState != Job::JSWaiting && job->_jobState != Job::JSRunning)
  {
    if (job->_jobState == Job::JSSuspended) job->_jobState = Job::JSCanceled;
    return; // out of our control already
  }
  if (job->_jobState == Job::JSWaiting)
  {
    // could be in the queue
    if (_queue.Cancel(job))
    {
      Assert(job->_jobState == Job::JSCanceled);
      return; // found
    }
    // launched meanwhile
  }
  for (int i=0; i<_threads.Size(); i++)
  {
    if (_threads[i]->Cancel(job))
    {
      Assert(job->_jobState == Job::JSCanceled);
      return; // found
    }
  }
  Assert(job->_jobState == Job::JSNone || job->_jobState == Job::JSCanceled || job->_jobState == Job::JSDone);
}

void JobManager::SuspendJob(Job *job)
{
  // always called from a job
  DoAssert(job->_threadNumber>=0);
  JobManagerThread *thread = _threads[job->_threadNumber];
  thread->Suspend(job);
}

void JobManagerThread::Suspend(Job *job)
{
  ScopeLockSection lock(_currentJobLock);
  DoAssert(job==_currentJob);
  if (job->_jobCommand < Job::JCSuspend) job->_jobCommand = Job::JCSuspend;
}



#if !DUMMY_MICROJOBS

#define NICE_PRIORITIES 0

/// Yield CPU, automatically lower priority as needed to handle when CPU is not available
static inline void YieldProcessorNiceAutoPriority(int &spinCount, int countToIdle)
{
  #if NICE_PRIORITIES // no reason to do this on Xbox, no other thread is shedulled to CPU #0
  if (--spinCount<=0)
  {
    // when spinning too long with no result, lower priority
    // this should help when no CPU is ready
    // priority of the background thread is Below normal, we want to wait for it
    // once it finishes its work, it will wait using WaitForMultipleObjects
    if (spinCount==0)
    {
      SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_LOWEST);
    }
    #if 0 // it seems going to Idle does more harm then good, too many processes can prevent us scheduling then
    else if (spinCount==countToIdle)
    {
      // done to workaround nVidia priority inversion driver bug (IDLE thread holding RtlHeap critical section)
      //LogF("Background thread possible deadlock detected (%d ms), going IDLE to recover",GlobalTickCount()-time);
      SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_IDLE);
    }
    #endif
  }
  #endif
  YieldProcessorNice();
}

static inline void YieldProcessorNiceAutoPriorityCleanup(int spinCount)
{
  #if NICE_PRIORITIES
  if (spinCount<=0)
  {
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_NORMAL);
  }
  #endif
}
#endif

void JobManager::ProcessMicroJobs(IMTTContext *context, int contextSize, IMicroJob *const*jobsData, int jobsCount)
{
  // if there is zero jobs, no thread would start any task and no _doneEvent would be set
  // to prevent this, we are calling serial implementation
  // calling it when there is only one task is most likely smart as well,
  // as it removes the synchronization overhead in such case
  // perhaps the limit should be even higher (depending on the task duration)
  if (_microJobsThreads.Size()<=0 || jobsCount<=1)
  {
    // special case - no MT safety or synchronization needed
    ProcessMicroJobsSerial(context,contextSize,jobsData,jobsCount);
    return;
  }
#if !DUMMY_MICROJOBS
  // we will never spawn more than allowed by a limit
  int spawnThreads = intMin(_microJobsThreads.Size(),_microJobsCountLimit-1);
  int threadCount = spawnThreads+1;
  // prepare the thread contexts
  AutoArray< char, MemAllocLocal<char, 1024> > contexts; // avoid allocation
  if (context)
  {
#if MICRO_TASKS_PERF
    PROFILE_SCOPE_GRF_EX(mtCtx, *, 0xffc000c0);
#endif

    contexts.Resize(spawnThreads * contextSize);
    char *ptr = contexts.Data();
    for (int i=0; i<spawnThreads; i++)
    {
      MicroJobsThread *thread = _microJobsThreads[i];
      thread->SetContext(context->CreateContext(ptr));
      ptr += contextSize;
    }
  }
  else
  {
    // make sure the variable is initialized
    for (int i=0; i<spawnThreads; i++)
    {
      MicroJobsThread *thread = _microJobsThreads[i];
      thread->SetContext(NULL);
    }
  }
  
  // initialize the task list
  {
#if MICRO_TASKS_PERF
    PROFILE_SCOPE_GRF_EX(mtIni, *, 0xffc000c0);
#endif
    {
      PROFILE_SCOPE_DETAIL_EX(mtIni, job);
      _microJobsList._jobsData = jobsData;
      _microJobsList._jobsCount = jobsCount;
      _microJobsList._threadCount = threadCount;
      // for debugging purposes we may want to avoid any microjobs on the main thread
      #define NO_MAIN_THREAD_JOBS 0
      #if !NO_MAIN_THREAD_JOBS
      // we need to setup schedule limits first
      int current = 0;
      // compute quotient and remainder
      // first remainder thread get quotient+1, all other get quotient
      // this makes sure thread 0 is always the most saturated one (reduces waiting on it)
      // at the same time the distribution is as even as possible
      int perThread = jobsCount/threadCount;
      int perThreadRem = jobsCount-threadCount*perThread;
      for (int i=0; i<threadCount; i++)
      {
        _microJobsList._schedule[i]._beg = current;
        current += i<perThreadRem ? perThread+1 : perThread;
        _microJobsList._schedule[i]._end = current;
        // start with no stealing (each thread processing its own schedule)
        _microJobsList._actSchedule[i] = i;
      }
      DoAssert(current==jobsCount);
      #else
      // debugging - setup everything into thread #1
      for (int i=0; i<threadCount; i++)
      {
        _microJobsList._schedule[i]._beg = 0;
        _microJobsList._schedule[i]._end = i==1 ? jobsCount : 0;
        // start with no stealing (each thread processing its own schedule)
        _microJobsList._actSchedule[i] = i;
      }
      #endif
      // now indicate each schedule is ready to run
      for (int i=0; i<threadCount; i++)
      {
        // no task started yet - point before the beginning
        _microJobsList._schedule[i]._started = _microJobsList._schedule[i]._beg-1;
        // reset all thread priorities back to normal, so that they take precedence over jobs
      }
      
    }
    {
      // wake up worker threads
      MemoryPublish(); // publish _microJobsList by setting _dataReady
      PROFILE_SCOPE_DETAIL_EX(mtWkU, job);
      _microJobsList._dataReady = true;
    }
    {
      PROFILE_SCOPE_DETAIL_EX(mtRdE, job);
      for (int i=0; i<threadCount; i++)
        _microJobsList._readyEvent[i].Set();
    }
  }

  // the main thread will work simultaneously with the working threads
  MicroJobsList::Schedule *schedule = &_microJobsList._schedule[_microJobsList._actSchedule[0]];
  while (true)
  {
    // get the task
    int index = InterlockedIncrementAcquire(&schedule->_started);
    if (index >= schedule->_end)
    {
      #if !NO_MAIN_THREAD_JOBS
      // no more jobs in our schedule, try to switch to another schedule to steal
      for (int s = 1; s<threadCount; s++)
      {
        // try stealing
        MicroJobsList::Schedule *stealFrom = &_microJobsList._schedule[s];
        index = InterlockedIncrementAcquire(&stealFrom->_started);
        if (index >= stealFrom->_end) continue;
        // next iteration attempt the same stealing
        _microJobsList._actSchedule[0] = s;
        goto DoTask;
      }
      #endif
      // if there is nobody to steal from, terminate
      break;
    }
    DoTask:

    // execute the task
    IMicroJob *task = _microJobsList._jobsData[index];
    // calling from the main thread - ID is always zero
    (*task)(context,0);

  }

  {
    PROFILE_SCOPE_GRF_EX(mtWai, *, 0xffc000c0);

    // we need to wait until all thread processing some tasks are finished with them
    // spin-wait is efficient on the main thread, because we are not scheduling any other work on it
    // spinCount = 1000 corresponds approx. to 0.1 ms on 2.4 GHz Intel Core CPU
    int spinCount = 1000;
    int countToIdle = GetCPUCount()==1 ? -20000 : -200000 ; // cca 1 ms : cca 10 ms
    
    // no more tasks - indicate the data are no longer valid
    _microJobsList._dataReady = false;
    // while waiting for microjobs, we might want to decrease our priority to give them better chance of finishing soon
    // TODO: if this is to be robust, the microjobs need to reset the main thread priority
    while (_microJobsList._threadsPending!=0) YieldProcessorNiceAutoPriority(spinCount,countToIdle);
    YieldProcessorNiceAutoPriorityCleanup(spinCount);
  }
  
  // clean-up
#if MICRO_TASKS_PERF
  PROFILE_SCOPE_GRF_EX(mtDon, *, 0xffc000c0);
#endif
  MemoryPublish();
  _microJobsList._jobsData = NULL;
  _microJobsList._jobsCount = 0;
  for (int i=0; i<_microJobsThreads.Size()+1; i++)
  {
    // values selected so that writing sensible values into _end / _started in any order does not start processing
    _microJobsList._schedule[i]._beg = 0;
    _microJobsList._schedule[i]._end = 0;
    _microJobsList._schedule[i]._started = INT_MAX / 2;
  }
  MemoryPublish(); // make sure the _jobsCount is visible before we submit new jobs
  
  // collect contexts from threads
  if (context)
  {
    for (int i=0; i<_microJobsThreads.Size(); i++)
    {
      MicroJobsThread *thread = _microJobsThreads[i];
      context->AggregateContext(thread->GetContext());
      thread->SetContext(NULL);
    }
  }
#endif
}

void JobManager::ProcessMicroJobsSerial(IMTTContext *context, int contextSize, IMicroJob *const*jobsData, int jobsCount)
{
  // if there is a context, there needs to be exactly one
  Assert(!context || contextSize==1);

  // the main thread is the only one working here
  for (int index=0; index<jobsCount; index++)
  {
    // execute the task
    IMicroJob *task = jobsData[index];
    (*task)(context,0);
  }
}

JobManager GJobManager;
