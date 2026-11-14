/// Job base class, Job Manager class interfaces

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MULTICORE_JOBS_HPP
#define _MULTICORE_JOBS_HPP

#include "multicore.hpp"

#if USE_MT_LIB_FOR_JOBS
  class SubmitSemaphore : public MultiThread::SemaphoreBlocker
  {
  public:
    SubmitSemaphore(int maximumCount,int initialCount=-1, void *securityDescriptor=0)
      : MultiThread::SemaphoreBlocker(maximumCount, initialCount, securityDescriptor)
    {}
    bool TryLock()
    {
      return Acquire(0);
    }
  };
  class JobEvent : public MultiThread::EventBlocker
  {
  public:
    JobEvent(MultiThread::EventBlocker::Mode mode=MultiThread::EventBlocker::Blocked_AutoReset, void *securityDescriptor=0)
      : MultiThread::EventBlocker(mode, securityDescriptor)
    {}
    JobEvent(bool manualReset)
      : MultiThread::EventBlocker(manualReset ? MultiThread::EventBlocker::Blocked_ManualReset : MultiThread::EventBlocker::Blocked_AutoReset)
    {}

    void Set() { Unblock(); }
    void Wait() { Acquire(INFINITE); }
    void Reset() { Block(); }
  };
  class ThreadSignaledObject
  {
    MultiThread::ThreadBase *_thread;
  public:
    ThreadSignaledObject(enum _noInitHandle) : _thread(NULL)
    {}
    bool Init(MultiThread::ThreadBase *threadO)
    {
      DoAssert(!_thread);
      _thread = threadO;
      return true;
    }
    void Done() { delete _thread; _thread=NULL; }
    /// check the initialization state
    operator bool() const { return _thread!=NULL; }

    // wait/lock are synonymous
    // do not wait for uninitialized objects
    bool Wait() const { return _thread->Join(); }
    bool Lock() const { return _thread->Join(); }
    bool TryLock() const { return _thread->Join(0); }
  };
  #define JOB_SUBMIT_SEMAPHORE SubmitSemaphore
  #define JOB_EVENT JobEvent
  #define SIGNALED_OBJECT ThreadSignaledObject
#else
  #define JOB_SUBMIT_SEMAPHORE Semaphore
  #define JOB_EVENT Event
  #define SIGNALED_OBJECT SignaledObject
#endif

/// we often use a 32b bitmask for CPU designation - this gives a natural limit
const int MaxMicroJobThreads = 32;

/// Base class for jobs managed by the JobManager
class Job
{
friend class JobManagerThread;
friend class JobManager;
friend class JobsQueue;

public:
  enum JobState
  {
    JSNone, // job was not executed yet
    JSWaiting, // job is waiting in the jobs queue
    JSRunning, // job is processing
    JSSuspended, // job is suspended (need to be planned again to resume)
    JSCanceled, // job is canceled
    JSDone // job finished its work
  };
  enum JobCommand
  {
    JCNone, // no command
    JCSuspend, // suspend the job
    JCCancel // cancel the job
  };

protected:
  /// State of the job - set by the job manager working thread, read by the main thread
  JobState _jobState;
  /// We want to interrupt the Job from some reason
  JobCommand _jobCommand;

  /// Unique identifier of the Job Manager worker thread job is executing on
  int _threadNumber;

  /// time (GlobalTickCount() related) we want the task to be finished, used as a job priority
  DWORD _jobFinishUntil;

public:
  Job();
  ~Job();

  /// ask about the state
  JobState GetJobState() const {return _jobState;}
  /// ask about the state
  bool IsJobRunning() const {return _jobState == JSRunning;}

  /// ask about the command
  bool WantToBeSuspended() const {return _jobCommand == JCSuspend;}
  /// ask about the command
  bool WantToBeCanceled() const {return _jobCommand == JCCancel;}

  /// set the command
  void CancelJob() {if (_jobCommand < JCCancel) _jobCommand = JCCancel;}

  /// where the job is running
  int GetThreadNumber() const {return _threadNumber;}

  /// compare jobs (for priority queue)
  bool IsMoreUrgent(const Job &job) const {return _jobFinishUntil < job._jobFinishUntil;}

  /// perform the job work, job is done whenever returned and _jobCommand == JCNone
  virtual bool Process() = 0;

  private:
  /// set the command
  void SuspendJob() {if (_jobCommand < JCSuspend) _jobCommand = JCSuspend;}
};

TypeIsSimple(Job *);

template <>
struct HeapTraits<Job *>
{
  typedef Job *Type;
  // default traits: Type is pointer to actual value
  static bool IsLess(const Type a, const Type b)
  {
    return a->IsMoreUrgent(*b);
  }
};

/// priority queue
class JobsQueue
{
protected:
  /// queue itself 
  HeapArray<Job *, MemAllocDSafe> _queue;
  /// access lock
  CriticalSection _queueLock;
  /// signalize presence of jobs
  JOB_SUBMIT_SEMAPHORE _submitSemaphore;

  /// whenever a thread changes a job state, notify via event
  /** useful for waiting, to avoid using busy loop */
  JOB_EVENT _stateChanged;

public:

#if USE_MT_LIB_FOR_JOBS
  JobsQueue() : _submitSemaphore(INT_MAX, 0) {}
#endif
  /// insert a new job into the queue
  void Insert(Job *job);
  /// get the most priority job and remove it from the queue
  bool RemoveFirst(Job *&result);
  /// access to the synchronization object
  JOB_SUBMIT_SEMAPHORE *GetSubmitSemaphore() {return &_submitSemaphore;}
  
  /// remove the job from the queue
  bool Cancel(Job *job);
  /// remove all jobs from the queue
  void Clear();

  void StateChanged() {_stateChanged.Set();}

  void WaitForStateChanged() {_stateChanged.Wait();}
};

/// info about the worker thread jobs are executing on
class JobManagerThread : public RefCount
{
protected:
  /// Unique identifier of the Job Manager worker thread
  int _threadNumber;
  /// Thread object
  SIGNALED_OBJECT _threadHandle;
  /// tell the thread to terminate
  JOB_EVENT _terminateEvent;
  /// set by the job when canceled
  JOB_EVENT _cancelEvent;

  /// access to the queue of jobs
  JobsQueue *_queue;
  /// currently executing job
  Job *_currentJob;
  /// to ensure _currentJob will not be set to NULL when canceling
  CriticalSection _currentJobLock;

public:
  JobManagerThread()
    : _threadNumber(-1), _threadHandle(NoInitHandle), _queue(NULL), _currentJob(NULL)
  {
  }
  ~JobManagerThread() {Done();}

  /// create the working thread, return if succeeded
  bool Init(int threadNumber, JobsQueue *queue);
  /// stop the working thread
  void Done();

  /// cancel the currently running job, wait until finished
  void Cancel();
  /// cancel the job, wait until finished
  bool Cancel(Job *job);

  /// suspend the job, wait until finished
  /** may be called with _currentJob only */
  void Suspend(Job *job);

  /// redirect the procedure to the member function
  static DWORD WINAPI ThreadProcedureCallback(void *context);
  DWORD ThreadProcedure();
};

/// Context for micro-jobs thread - used for aggregation of values for all jobs processed by a single thread
class IMTTContext
{
public:
  /// create a new context for the thread
  virtual IMTTContext *CreateContext(char *mem) const = 0;
  /// aggregate given context with the main context
  virtual void AggregateContext(const IMTTContext *ctx) = 0;
};

/// Functor representing the micro-job (similar to OpenMP piece of work)
class IMicroJob
{
public:
  /**
  @param thrIndex index of the thread, in range 0 .. GJobManager.GetMicroJobCount()-1
  */
  virtual void operator () (IMTTContext *context, int thrIndex) = 0;
  virtual ~IMicroJob(){}
};


/// List of micro-jobs
struct MicroJobsList
{
  /// id of the jobs batch
  /** used to prevent race conditions causing slowly working thread reading incomplete data from the next batch */
  volatile bool _dataReady;
  /// external array of jobs - data
  IMicroJob * const *_jobsData;
  /// external array of jobs - count of jobs
  int _jobsCount;
  /// how many threads are working on the data
  int _threadCount;
  /// the last started task
  struct Schedule
  {
    /// area we are supposed to process
    volatile LONG _beg,_end;
    /// last task which was already started
    volatile LONG _started;
  };
  /** each thread has its own pointer. Once thread is out of his own jobs, it may steal tasks to other jobs */
  Schedule _schedule[MaxMicroJobThreads];
  /// index of a schedule which we are getting (or stealing) tasks from
  int _actSchedule[MaxMicroJobThreads];
  /// how many jobs are pending
  volatile LONG _threadsPending;
  /// signaled when some task is available
  /** event not needed for the main thread */
  JOB_EVENT _readyEvent[MaxMicroJobThreads-1];
  MicroJobsList(): _dataReady(false), _threadsPending(0) {}
};

/// Thread for executing of micro-jobs
class MicroJobsThread : public RefCount
{
protected:
  /// index of the thread, to be passed to micro-jobs
  int _thrIndex;
  /// Thread object
  SignaledObject _threadHandle;
  /// list of jobs
  MicroJobsList *_jobs;
  /// context for this thread
  IMTTContext *_context;
  /// tell the thread to terminate
  JOB_EVENT _terminateEvent;

#if WAKE_UP_THREAD_EVENT
  /// signaled when some task is available
  Event _readyEvent;
#endif

public:
  MicroJobsThread(int thrIndex) 
    : _threadHandle(NoInitHandle), _jobs(NULL), _thrIndex(thrIndex)
  {
  }
  ~MicroJobsThread() {Done();}
  /// create the working thread, return if succeeded
  bool Init(int threadNumber, MicroJobsList *jobs);
  /// stop the working thread
  void Done();
  /// access to the context
  const IMTTContext *GetContext() const {return _context;}
  /// set the context
  void SetContext(IMTTContext *context) {_context = context;}
#if WAKE_UP_THREAD_EVENT
  void WakeUp() {_readyEvent.Set();}
#endif
  /// redirect the procedure to the member function
  static DWORD WINAPI ThreadProcedureCallback(void *context);
  DWORD ThreadProcedure();
  
  HANDLE GetThreadHandle() const {return _threadHandle.GetHandle();}
};

/// safe pointer conversions - plain pointer (identity)
template <class Type>
static inline IMicroJob * const *GetMicroJobPtr(Type * const *ptrs)
{
  // constraint only, no code expected - make sure conversion between types exists
  IMicroJob *test = (Type *)NULL;
  (void)test;
  // if it does, we may convert the result
  return reinterpret_cast<IMicroJob * const *>(ptrs);
}
/// safe pointer conversions - SRef
static inline IMicroJob * const *GetMicroJobPtr(SRef<IMicroJob> const * ptrs)
{
  // we know SRef memory layout is the same as a plain pointer one
  return reinterpret_cast<IMicroJob * const *>(ptrs);
}

/// safe pointer conversions - Ref
static inline IMicroJob * const *GetMicroJobPtr(Ref<IMicroJob> const *ptrs)
{
  // we know Ref memory layout is the same as a plain pointer one
  return reinterpret_cast<IMicroJob * const *>(ptrs);
}

/// manager of the concurrent jobs for MultiCore / SMP systems
/**
Two basic concurrent task types: Jobs and MicroJobs
Job is long term. 
MicroJob is used to implement a parallel loop.
*/

class JobManager
{
protected:
  /// worker threads
  RefArray<JobManagerThread> _threads;
  /// queue of jobs
  JobsQueue _queue;

  /// worker threads for micro-jobs
  RefArray<MicroJobsThread> _microJobsThreads;
  /// info about the list of micro-jobs
  MicroJobsList _microJobsList;
  /// limiting MJs spawned may be useful both runtime and for debugging
  int _microJobsCountLimit;

public:
  JobManager();
  ~JobManager();

  /// prepare the worker threads
  void Init();
  /// finish the work
  void Done();
  /// cancel and remove all jobs (running and queued)
  void CancelAllJobs();

  /// check if usable
  bool IsUsable() const {return _threads.Size() > 0;}

  /// schedule the job, should be done until timeMs from now
  void CreateJob(Job *job, DWORD timeMs);
  /// reschedule the suspended job
  void ResumeJob(Job *job);
  /// cancel the given job
  void CancelJob(Job *job);
  /// suspend the job - can be called only from the job
  void SuspendJob(Job *job);

  /// wait until given job completes
  void Wait(Job *job);

  void SetMicroJobCountLimit(int microJobsCountLimit){_microJobsCountLimit=microJobsCountLimit;}
  int GetMicroJobCountLimit() const {return _microJobsCountLimit;}
  
  /// check how many micro-job threads will be use in a ProcessMicroJobs loop
  /** can be used to pre-allocate external context arrays */ 
  int GetMicroJobCount() const {return intMin(_microJobsThreads.Size()+1,_microJobsCountLimit);}
  /// Process all micro-jobs
  void ProcessMicroJobs(IMTTContext *context, int contextSize, IMicroJob * const *jobsData, int jobsCount);
  /// Process all micro-jobs without using any worker thread
  /** Useful for testing or for running on single CPU to make sure overhead is as low as possible */
  void ProcessMicroJobsSerial(IMTTContext *context, int contextSize, IMicroJob * const *jobsData, int jobsCount);
  
  /// provide safe overloads for arrays of various pointer types
  /**
  allows to use the same implementation and avoid casting on the callers side, keeping usage cleaner
  */
  template <class Container>
  void ProcessMicroJobs(IMTTContext *context, int contextSize, const Container &jobsData)
  {
    ProcessMicroJobs(context,contextSize,GetMicroJobPtr(jobsData.Data()),jobsData.Size());
  }
  template <class Container>
  void ProcessMicroJobsSerial(IMTTContext *context, int contextSize, const Container &jobsData)
  {
    ProcessMicroJobsSerial(context,contextSize,GetMicroJobPtr(jobsData.Data()),jobsData.Size());
  }

  /// process array of jobs in a linear array
  /** convenience wrapped - creates a pointer array first */
  template <class Container>
  void ProcessMicroJobsArray(IMTTContext *context, int contextSize, Container &jobsData)
  {
    AutoArray<IMicroJob *, MemAllocLocal<IMicroJob *,128> > taskPtrs;
    for (int i=0; i<jobsData.Size(); i++)
    {
      taskPtrs.Add(&jobsData[i]);
    }
    ProcessMicroJobs(context,contextSize,GetMicroJobPtr(taskPtrs.Data()),taskPtrs.Size());
  }
};

extern JobManager GJobManager;

#endif
