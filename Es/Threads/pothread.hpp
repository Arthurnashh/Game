#ifdef _MSC_VER
#  pragma once
#endif

/**
    @file   pothread.hpp
    @brief  Portable threads.

    Implemented by CreateThread() (WIN32) OR pthread_create() (pthreads).
    <p>Copyright &copy; 2001-2002 by BIStudio (www.bistudio.com)
    @author PE
    @since  29.11.2001
    @date   8.7.2002
    @todo   More routines for "portable threads" will be needed?
*/

#ifndef _POTHREAD_H
#define _POTHREAD_H

#include "pocritical.hpp"
#include <assert.h>

//------------------------------------------------------------
//  Portability macros:

#ifdef _WIN32

   /// Thread identifier.
  class ThreadId
  {
    HANDLE _thread;
    unsigned long _threadId;
  public:
    ThreadId() {_thread=NULL;_threadId=0;}
    ThreadId(HANDLE thread,unsigned long id):_thread(thread),_threadId(id) {}
    ThreadId(const ThreadId &other)
    {
      assert(other._thread!=NULL);
      _threadId=other._threadId;
      DuplicateHandle(GetCurrentProcess(),other._thread,GetCurrentProcess(),&_thread,0,FALSE,DUPLICATE_SAME_ACCESS);
    }
    ThreadId &operator=(const ThreadId &other)
    {
      assert(other._thread!=NULL);
      _threadId=other._threadId;
      CloseHandle(_thread);
      DuplicateHandle(GetCurrentProcess(),other._thread,GetCurrentProcess(),&_thread,0,FALSE,DUPLICATE_SAME_ACCESS);
      return *this;
    }
    ~ThreadId()
    {
      if (_thread) CloseHandle(_thread);
    }

    bool operator==(const ThreadId &other) {return _threadId==other._threadId;}
    bool operator!=(const ThreadId &other) {return _threadId!=other._threadId;}

    operator HANDLE() const {return _thread;}
    unsigned long GetThreadId() {return _threadId;}
    operator unsigned long() {return _threadId;}
  };


   /// Return type of thread main routine.
#  define THREAD_PROC_RETURN DWORD
   /// Calling mode of thread main routine.
#  define THREAD_PROC_MODE   WINAPI

#else

   /// Thread identifier.
   typedef pthread_t ThreadId;

   /// Return type of thread main routine.
#  define THREAD_PROC_RETURN void*
   /// Calling mode of thread main routine.
#  define THREAD_PROC_MODE

   inline unsigned long GetCurrentThreadId() { return pthread_self(); }
#endif

//------------------------------------------------------------
//  Thread-manipulating functions:

/**
    Creates and starts a new thread.
    The new thread will be executed concurrently with the calling thread
    and will start immediately (in "runnable" mode).
    @param  id Pointer to new thread's id [out]. Can be <code>NULL</code>.
    @param  stackSize Initial stack size (or <code>0</code> for default/inherited size).
    @param  threadProc Address of the thread-routine.
    @param  arg Argument to be passed to the thread-routine.
    @return <code>true</code> if succeeded.
*/
extern bool poThreadCreate ( ThreadId *id, long stackSize, THREAD_PROC_RETURN (THREAD_PROC_MODE *threadProc)(void*), void *arg );

/**<
    The <code>threadProc</code> should be declared as follows:

    <code>
    <pre>
    THREAD_PROC_RETURN THREAD_PROC_MODE proc ( void *param )
    {
        ...
        return (THREAD_PROC_RETURN)anything;
    }
    </pre>
    </code>

*/

/**
    Joins the given thread with me.
    Suspends the execution of the calling thread (waits) until the thread identified
    by <code>id</code> terminates.
    @param  id Thread's identifier.
    @param  result Optional address of a buffer to receive thread return value.
    @return <code>true</code> if succeeded.
*/
extern bool poThreadJoin (const ThreadId &id, THREAD_PROC_RETURN *result );

/**
    Current thread's identifier.
    @param  id Thread's identifier [out].
*/
extern void poThreadId (ThreadId &id );

/**
    Sets priority of the given thread.
    @param  id Thread's identifier [in].
    @param  delta Relative thread priority (from <code>-2</code> /lowest/ to <code>2</code> /highest/) [in].
    @return <code>true</code> if succeeded.
*/
extern bool poSetPriority (const ThreadId &id, int delta );

/**
    Sets priority of the running (calling) thread.
    @param  delta Relative thread priority (from <code>-2</code> /lowest/ to <code>2</code> /highest/) [in].
    @return <code>true</code> if succeeded.
*/
extern bool poSetMyPriority ( int delta );

//------------------------------------------------------------
//  Private MT-safe support:

#ifdef _WIN32

class MemAllocSafe {

public:

#if ALLOC_DEBUGGER

    static void *Alloc ( size_t &size, const char *file, int line, const char *postfix )
    {
        return safeNew(size);
    }

#else

    static void *Alloc ( size_t &size )
    {
        return safeNew(size);
    }

#endif

    static void *Realloc ( void *mem, int oldSize, int size )
    {
      // we are lazy to implement safeRealloc, therefore we return NULL
      // TODO: safeRealloc
      return NULL;
    }

    static void Free ( void *mem, int size )
    {
        safeDelete(mem);
    }

    static void Unlink ( void *mem )
    {}

	static inline const int MinGrow() {return 32;}
};


#else

#  define MemAllocSafe MemAllocD

#endif

//------------------------------------------------------------
//  MT-safe RefCount:

class RefCountSafe {

private:

    /// Number of references to this object.
    mutable volatile long _count;

private:

#ifndef _WIN32
  /// Critical section to lock _count. It can NOT be used for general object locking (possible DeadLocks)
    PoCriticalSection lockCount;

    /// Enter the object's critical section.
    inline void enterCount () const
    { lockCount.enter(); }

    /// Leave the object's critical section.
    inline void leaveCount () const
    { lockCount.leave(); }
#endif

public:

    //! Default constructor.
    RefCountSafe ()
    { _count = 0; }

    //! Copy constructor (_count attribute will not be copied).
    /*!
        \param src Reference to source object.
    */
    RefCountSafe ( const RefCountSafe &src )
    { _count = 0; }

    //! Copying of object (_count attribute will not be copied).
    void operator = ( const RefCountSafe &src )
    { _count = 0; }

    //! Destructor
    virtual ~RefCountSafe ();               // use destructor of derived class


    //! Adding of reference to this object.
    /*!
        \return Number of references.
    */
    int AddRef () const
    {
#ifdef _WIN32
        int result = InterlockedIncrementAcquire(&_count);
#else
        enterCount();
        int result = ++_count;
        leaveCount();
#endif
        return result;
    }

    //! Releasing of reference to this object.
    /*!
        \return Number of references.
    */
    int Release () const
    {
#ifdef _WIN32
        int ret = InterlockedDecrementRelease(&_count); //flush every writes
#else
        enterCount();
        int ret=--_count;
        leaveCount();
#endif
        if( ret == 0 )
            delete (RefCountSafe*)this;
        return ret;
    }

    //! Determines number of references to this object.
    /*!
        \return Number of references.
    */
    int RefCounter () const
    { return _count; }

    //! get memory used by this object
    //! not including memory used to hold this object itself,
    //! which is covered by sizeof
    virtual double GetMemoryUsed () const;

    };

#if _USE_DEAD_LOCK_DETECTOR

/**
Dead Lock Detector 
  It tries to find potential deadlocks even when no one really happen.
Idea: Enter/Leave Detector every time the Enter/Leave critical section (or other lock)
  is processed. It memorizes order in which, inside given thread, the sections were entered.
  When there is a trial to enter it in the invert order (from different thread), the potential deadLock
  is reported.
Usage: set _USE_DEAD_LOCK_DETECTOR 1
  and toggle breakpoint in the Fail("Possible Dead Lock detected!"); line inside DeadLockDetector::TestDeadLocks
  Use ENTER_DEAD_LOCK_DETECTOR and LEAVE_DEAD_LOCK_DETECTOR in your critical sections.
*/
#include <Es/Containers/array.hpp>

struct SectionId
{
  const void *ptr;
  SectionId() {}
  SectionId(void *p) : ptr(p) {}
  SectionId(const void *p) : ptr(p) {}
  SectionId(const SectionId& sec) : ptr(sec.ptr) {}
  bool operator== (const SectionId &other) const
  {
    return ptr==other.ptr;
  }
};
TypeIsMovableZeroed(SectionId)

#include <Es/Memory/debugAlloc.hpp>
#include <Es/Memory/normalNew.hpp>

class DeadLockDetector
{
  CRITICAL_SECTION cs;

  struct DLDThreadSections
  {
    struct SectionIdPair
    {
      // section sec2 was entered after sec1
      SectionId sec1, sec2;
      // TODO: add some CallStack info (GDebugExceptionTrap.ExtractCallstack(_callstack,_calls,true,&GMapFile);)

      SectionIdPair() : sec1(), sec2() {}
      SectionIdPair(SectionId id1, SectionId id2) : sec1(id1), sec2(id2) {}

      bool operator == (const SectionIdPair &other) const
      {
        return ((other.sec1==sec1) && (other.sec2==sec2));
      }

      ClassIsMovableZeroed(SectionIdPair)
    };
    // DLDThreadSections is identified by its ThreadId
    DWORD _threadId;
    // edges for given thread determine the order of entering critical sections
    FindArray<SectionIdPair, DebugAlloc> _edges;
    // current sequence (stack) of entered critical sections
    AutoArray<SectionId, DebugAlloc> _sequence;

    DLDThreadSections() {}
    DLDThreadSections(DWORD threadId) : _threadId(threadId) {}

    bool operator == (const DLDThreadSections &other) const
    {
      return ( other._threadId==_threadId );
    }
    ClassIsMovable(DLDThreadSections)
  };
public:
  DeadLockDetector()
  {
    InitializeCriticalSection(&cs);
  }

  ~DeadLockDetector()
  {
    DeleteCriticalSection(&cs);
  }

  // The critical section is to be entered. Update structure and trigger potential deadLock detection.
  void Enter(SectionId dld_id);
  // The critical section is to be left. Update structure.
  void Leave(SectionId dld_id);
  // Test whether there are some new potential deadlocks and report them
  // Test new edges inside _sectionSeq[ix]._edges[from..end]
  void TestDeadLocks(int ix, int from);

  // sequences of critical section for each thread
  FindArray<DLDThreadSections, DebugAlloc> _sectionSeq;

  SectionId LastSection(int ix)
  {
    int size = _sectionSeq[ix]._sequence.Size();
    if (size) return _sectionSeq[ix]._sequence[size-1];
    return SectionId(); //should not happen
  }

  /// MT-safe new operator.
  static void* operator new ( size_t size );
  /// MT-safe new operator.
  static void* operator new ( size_t size, const char *file, int line );
  /// MT-safe delete operator.
  static void operator delete ( void *mem );
private:
//@{ DEAD LOCK DETECTOR of actual Dead Locks (not possible only)
  typedef enum { NotVisited, BeingVisited, DoneVisited } EVisited;
  struct VisitedItem
  {
    EVisited  _visited;
    SectionId _id;  //ix of critical section
    VisitedItem() : _visited(NotVisited) {}
    VisitedItem(EVisited visited, SectionId id) : _visited(visited), _id(id) {}
    ClassIsMovableZeroed(VisitedItem)
  };

  AutoArray<VisitedItem, DebugAlloc> visitedSections;
  AutoArray<int, DebugAlloc> possibleThreads;

  // Assume the last action was entering a critical section from thread _sectionSeq[ix]
  // IsDeadLock tries to determine, whether there is "dead lock cycle".
  bool IsDeadLock(int ix);
  // Helper function for IsDeadLock (uses Recursion)
  bool IsDeadLockCycle(int ix);
  // Set this section as BeingVisited or DoneVisited
  EVisited GetVisited(SectionId sec);
  // Get how is or is not this section being visited
  void SetVisited(SectionId sec, EVisited visited);
  // Generate RptF and special *.dld.bidmp file
  void ReportDeadLock(int ix);
public:
  // Initialize _sectionSeq and everything to test DeadLock and trigger IsDeadLock inside
  void TestDLD();
//@}
};

#include <Es/Memory/debugNew.hpp>

// Dead Lock Detector is managed through global 
extern DeadLockDetector GDeadLockDetector;

#define ENTER_DEAD_LOCK_DETECTOR GDeadLockDetector.Enter(this);
#define LEAVE_DEAD_LOCK_DETECTOR GDeadLockDetector.Leave(this);
#define ENTER_DEAD_LOCK_DETECTOR_ID(DLD_ID) GDeadLockDetector.Enter(DLD_ID);
#define LEAVE_DEAD_LOCK_DETECTOR_ID(DLD_ID) GDeadLockDetector.Leave(DLD_ID);

#else // _USE_DEAD_LOCK_DETECTOR
#define ENTER_DEAD_LOCK_DETECTOR
#define LEAVE_DEAD_LOCK_DETECTOR
#endif

#endif
