/**
    @file   pothread.cpp
    @brief  Portable threads.

    Implemented by CreateThread() (WIN32) OR pthread_create() (pthreads).
    <p>Copyright &copy; 2001-2003 by BIStudio (www.bistudio.com)
    @author PE
    @since  29.11.2001
    @date   18.6.2003
*/

#include <Es/essencepch.hpp>
#include "pothread.hpp"

//------------------------------------------------------------
//  Thread-manipulating functions:

#ifdef _WIN32

bool poThreadCreate ( ThreadId *id, long stackSize, THREAD_PROC_RETURN (THREAD_PROC_MODE *threadProc)(void*), void *arg )
{
    if ( !threadProc ) return false;
    if ( stackSize < 0L ) stackSize = 0L;
    DWORD thid;     
    HANDLE tid = CreateThread(NULL,stackSize,threadProc,arg,0,&thid);
    if (tid==NULL) return false;
    if ( id ) *id = ThreadId(tid,thid); 
    else  CloseHandle(tid);
    return true;
}

bool poThreadJoin (const ThreadId &id, THREAD_PROC_RETURN *result )
{
    if ( id == NULL ) return false;
    if ( WaitForSingleObject(id,INFINITE) != WAIT_OBJECT_0 ) return false;
    if ( !result ) return true;
    return( GetExitCodeThread(id,result) != FALSE );
}

void poThreadId ( ThreadId &id )
{
    
    id = ThreadId(GetCurrentThread(),GetCurrentThreadId());
}

const int PRIO[] = {
    THREAD_PRIORITY_IDLE,           ///< -3 and below
    THREAD_PRIORITY_LOWEST,         ///< -2
    THREAD_PRIORITY_BELOW_NORMAL,   ///< -1
    THREAD_PRIORITY_NORMAL,         ///<  0
    THREAD_PRIORITY_ABOVE_NORMAL,   ///<  1
    THREAD_PRIORITY_HIGHEST,        ///<  2
    THREAD_PRIORITY_TIME_CRITICAL   ///<  3 and more
    };

bool poSetPriority (const ThreadId &id, int delta )
{
    if ( delta < -3 ) delta = -3;
    else
    if ( delta > 3 ) delta = 3;
    return( SetThreadPriority(id,PRIO[delta+3]) != 0 );
}

bool poSetMyPriority ( int delta )
{
  ThreadId th;
  poThreadId(th);
  return poSetPriority(th,delta);
}

#else

bool poThreadCreate ( ThreadId *id, long stackSize, THREAD_PROC_RETURN (THREAD_PROC_MODE *threadProc)(void*), void *arg )
{
    if ( !threadProc ) return false;
        // stackSize is silently ignored!
    pthread_t tid;
    if ( pthread_create(&tid,NULL,threadProc,arg) != 0 ) return false;
    if ( id ) *id = tid;
    return true;
}

bool poThreadJoin (const ThreadId &id, THREAD_PROC_RETURN *result )
{
    return( pthread_join(id,result) == 0 );
}

void poThreadId ( ThreadId &id )
{
    id = (ThreadId)pthread_self();
}

bool poSetPriority (const ThreadId &id, int delta )
{
    // !!! not yet !!!
    return true;
}

bool poSetMyPriority ( int delta )
{
    return poSetPriority((ThreadId)pthread_self(),delta);
}

#endif

#if _USE_DEAD_LOCK_DETECTOR

// DeadLockDetector implementation

#include "El/Debugging/imexhnd.h"

void DeadLockDetector::ReportDeadLock(int ix)
{
  // Report DeadLock and generate *.bidmp
  Fail("Dead Lock detected!");
  RptF("((Critical Section Lists:");
  for (int i=0; i<_sectionSeq.Size(); i++)
  {
    if (_sectionSeq[i]._sequence.Size())
    {
      RptF(" Thread: %d %s", _sectionSeq[i]._threadId, ((i==ix) ? "DeadLock" : "") );
      for (int j=0; j<_sectionSeq[i]._sequence.Size(); j++)
      {
        RptF("   %08x", _sectionSeq[i]._sequence[j]);
      }
    }
  }
  RptF("))Critical Section Lists");

  // Generate special bidmp file
  GDebugExceptionTrap.GenerateBidmp(".dld");
}

void DeadLockDetector::Enter(SectionId dld_id)
{
  EnterCriticalSection(&cs);
  // Add given critical section id into current sequence of sections for given thread
  DWORD curThreadId = GetCurrentThreadId();
  //LogF("Entering DLD: Thread=%d, cs=%d", curThreadId, dld_id);
  int ix = _sectionSeq.Find(curThreadId);
  if (ix<0) ix = _sectionSeq.Add(DLDThreadSections(curThreadId));

  DLDThreadSections &sections = _sectionSeq[ix];
  // test whether critical section is not entered second time
  for(int from=0; from<sections._sequence.Size(); from++)
  {
    if (sections._sequence[from]==dld_id)
    { //add it to sequence array, but do not add any "conflict possible edges"
      sections._sequence.Add(dld_id); //leave should delete only one section (for each enter one item in the array)
      LeaveCriticalSection(&cs);
      return;
    }
  }
#if _USE_DEAD_LOCK_DETECTOR_POS
  // add all new edges, if there are some
  int dirtyFrom = -1;
  for(int from=0; from<sections._sequence.Size(); from++)
  {
    // when this is new edge, add it
    DLDThreadSections::SectionIdPair pair(sections._sequence[from], dld_id);
    int ixs = sections._edges.Find(pair);
    if (ixs<0)
    { //not found add it and set dirty to test DeadLock
      ixs = sections._edges.Add(pair);
      if (dirtyFrom<0) dirtyFrom = ixs;
    }
  }
#endif
  // add just entering SectionId into sequence
  sections._sequence.Add(dld_id);
#if _USE_DEAD_LOCK_DETECTOR_POS
  if (dirtyFrom>=0)
  { //test whether there can be potential DeadLock
    TestDeadLocks(ix, dirtyFrom);
  }
#endif
  if (IsDeadLock(ix))
  {
    ReportDeadLock(ix);
  }

  LeaveCriticalSection(&cs);
}

void DeadLockDetector::Leave(SectionId dld_id)
{
  EnterCriticalSection(&cs);
  // Remove given critical section id from current sequence of sections of given thread
  DWORD curThreadId = GetCurrentThreadId();
  //LogF("Leaving DLD: Thread=%d, cs=%d", curThreadId, dld_id);
  int ix = _sectionSeq.Find(curThreadId);
  if (ix<0) 
  { //strange, this should not happen, but return
    LeaveCriticalSection(&cs);
    return;
  }
  DLDThreadSections &sections = _sectionSeq[ix];
  // Delete dld_id section from sequence array
  for(int i=sections._sequence.Size()-1; i>=0; i--)
  {
    if (sections._sequence[i]==dld_id)
    {
      if (i!=sections._sequence.Size()-1)
        _asm nop; //this looks weird (enter(a),enter(b),leave(a),leave(b))
      sections._sequence.Delete(i);
      break; //removal is over (there can be more occurrences of this CS, as it could be entered more than once)
    }
  }
  LeaveCriticalSection(&cs);
}

void DeadLockDetector::TestDeadLocks(int ix, int from)
{
  FindArray<DLDThreadSections::SectionIdPair, DebugAlloc> &edges = _sectionSeq[ix]._edges;
  for (int i=from; i<edges.Size(); i++)
  {
    for (int j=0; j<_sectionSeq.Size(); j++)
    {
      if (j==ix) continue; //do not test against myself
      DLDThreadSections::SectionIdPair invPair(edges[i].sec2, edges[i].sec1); //inverse pair to detect possible DeadLock
      int dLockIx = _sectionSeq[j]._edges.Find(invPair);
      if (dLockIx>=0)
      {
        // possible DeadLock detected
        Fail("Possible Dead Lock detected!");
        RptF("Critical Section List:");
        for (int six=0; six<_sectionSeq[ix]._sequence.Size(); six++)
        {
          RptF("  %08x", _sectionSeq[ix]._sequence[six]);
        }
        RptF("Conflicting Thread: %d InvEdge: %08x,%08x dLockIx=%d", 
          _sectionSeq[j]._threadId, invPair.sec1, invPair.sec2, dLockIx);
        
        static bool doit = false;
        if (doit)
        { //trick: we delete the conflicting edge in the other thread edge sequence
          //   which would hopefully trigger the possible DeadLock from the other thread next time
          _sectionSeq[j]._edges.Delete(dLockIx);
          return;
        }
      }
    }
  }
}

// Assume the last action was entering a critical section from thread _sectionSeq[ix]
// IsDeadLock tries to determine, whether there is "dead lock cycle".
bool DeadLockDetector::IsDeadLock(int ix)
{
  visitedSections.Resize(0); //clear
  // optimization: enumerate only threads which have some CS in usage
  possibleThreads.Resize(0); //clear first
  for (int i=0; i<_sectionSeq.Size(); i++)
  {
    if (_sectionSeq[i]._sequence.Size()) possibleThreads.Add(i);
  }
  if (possibleThreads.Size()<2) return false; //no Dead Lock
  // try to find dead lock cycle
  if (IsDeadLockCycle(ix)) return true;
  return false;
};

void DeadLockDetector::SetVisited(SectionId sec, EVisited visited)
{
  for (int i=0; i<visitedSections.Size(); i++)
  {
    if (visitedSections[i]._id == sec) 
    {
      visitedSections[i]._visited = visited; 
      return; 
    }
  }
  visitedSections.Add(VisitedItem(visited, sec));
}

DeadLockDetector::EVisited DeadLockDetector::GetVisited(SectionId sec)
{
  for (int i=0; i<visitedSections.Size(); i++)
  {
    if (visitedSections[i]._id == sec) 
    {
      return visitedSections[i]._visited;
    }
  }
  return NotVisited;
}

bool DeadLockDetector::IsDeadLockCycle(int ix)
{
  SetVisited(LastSection(ix), BeingVisited);
  AutoArray<SectionId, DebugAlloc> &sequence = _sectionSeq[ix]._sequence;
  for (int i=0; i<sequence.Size()-1; i++) //all but one
  {
    SectionId &sec = sequence[i];
    EVisited howVisited = GetVisited(sec);
    if (howVisited==BeingVisited) //cycle found!
    {
      return true;
    }
    if (howVisited==DoneVisited) continue; //no cycle through this CS
    // test whether some thread is currently waiting on this critical section
    for (int tix=0; tix<possibleThreads.Size(); tix++)
    {
      if (tix==ix) continue;
      if (LastSection(tix)==sec) //this thread waits on it
      {
        if (IsDeadLockCycle(tix)) return true; //RECURSION
      }
    }
    SetVisited(sec, DoneVisited); //possibly set already
  }
  SetVisited(LastSection(ix), DoneVisited);
  return false;
}

// Initialize _sectionSeq and everything to test DeadLock and trigger IsDeadLock inside
void DeadLockDetector::TestDLD()
{
  struct TempScopeLock
  {
    CRITICAL_SECTION *_cs;
    TempScopeLock(CRITICAL_SECTION &cs) : _cs(&cs) 
    {
      EnterCriticalSection(_cs);
    }
    ~TempScopeLock()
    {
      LeaveCriticalSection(_cs);
    }
  };
  TempScopeLock lock(cs);
  // backup structure
  FindArray<DLDThreadSections, DebugAlloc> sectionSeqBackup = _sectionSeq;
  // initialize structure
  _sectionSeq.Resize(0);
  const int secPerThread[]={
    // {thread, sec1, ... secN}
    /*thread*/ 1, /*sec*/  1, 2, 3, 1, 4, 1, 18 , 0, //next thread
    /*thread*/ 2, /*sec*/ 18, 19, 0,
    /*thread*/ 3, /*sec*/ 19, 1, 0, 0 //double zero - end
    /*
      i.e. T3 waits on S1, which is locked by T1, which waits on S18, 
        which is locked by T2, which waits on S19, which is locked by T3 <-- Cycle!
    */
  };
  bool newThread = true;
  int curIx = 0;
  for (int i=0; secPerThread[i]+secPerThread[i+1]!=0; i++)
  {
    if (newThread)
    {
      curIx = _sectionSeq.Add(DLDThreadSections(secPerThread[i]));
      newThread = false;
    }
    else
    {
      if (!secPerThread[i]) newThread = true;
      else
      { //add new section into last thread
        _sectionSeq[curIx]._sequence.Add(SectionId((void *)secPerThread[i]));
      }
    }
  }
  // and test Deadlock finaly
  if (IsDeadLock(curIx))
  {
    ReportDeadLock(curIx);
  }

  // restore structure
  _sectionSeq = sectionSeqBackup;
}


#include "Es/Memory/normalNew.hpp"

void* DeadLockDetector::operator new ( size_t size )
{
  return safeNew(size);
}

void* DeadLockDetector::operator new ( size_t size, const char *file, int line )
{
  return safeNew(size);
}

void DeadLockDetector::operator delete ( void *mem )
{
  safeDelete(mem);
}

#include "Es/Memory/debugNew.hpp"

#endif // _USE_DEAD_LOCK_DETECTOR

//------------------------------------------------------------
//  MT-safe RefCount:

RefCountSafe::~RefCountSafe ()
    // use destructor of derived class
{
}

double RefCountSafe::GetMemoryUsed () const
{
    return 0.0;
}

