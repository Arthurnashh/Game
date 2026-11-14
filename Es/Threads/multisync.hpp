#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MULTISYNC_HPP
#define _MULTISYNC_HPP

#ifdef _WIN32

#include <Es/Common/win.h>
#include <Es/Threads/pothread.hpp>

enum _noInitHandle {NoInitHandle};

/// Encapsulation of any HANDLE based synchronization object (usable as argument of WaitForXXX)
class SignaledObject : private NoCopy
{
protected:
  mutable HANDLE _handle;

public:
  /// create uninitialized object
  SignaledObject(enum _noInitHandle) : _handle(NULL) {}
  /// create initialized object
  SignaledObject(HANDLE handle) : _handle(handle)
  {
    if (!_handle)
    {
      ErrorMessage("Win32 Error: Cannot create MT object.");
    }
  }
  ~SignaledObject() {Done();}

  bool Init(HANDLE handle)
  {
    DoAssert(!_handle);
    _handle = handle;
    if (!_handle)
    {
      ErrorMessage("Win32 Error: Cannot create MT object.");
      return false;
    }
    return true;
  }
  void Done()
  {
    if (_handle)
    {
      CloseHandle(_handle);
      _handle = NULL;
    }
  }
  
  /// get the handle
  /** sometimes we use SignaledObject to store handle like threads, which we may need to access */
  HANDLE GetHandle() const {return _handle;}

  HANDLE DetachHandle() const
  {
    HANDLE handle = _handle;
    _handle = NULL;
    return handle;
  }

  /// check the initialization state
  operator bool() const {return _handle != NULL;}

  // wait/lock are synonymous
  // do not wait for uninitialized objects
  bool Wait() const {return _handle ? WaitForSingleObject(_handle, INFINITE) == WAIT_OBJECT_0 : false;}
  bool TryWait(DWORD time = 0) const {return _handle ? WaitForSingleObject(_handle, time) == WAIT_OBJECT_0 : false;}
  bool Lock() const {return _handle ? WaitForSingleObject(_handle, INFINITE) == WAIT_OBJECT_0 : false;}
  bool TryLock() const {return _handle ? WaitForSingleObject(_handle, 0) == WAIT_OBJECT_0 : false;}

  static int WaitForMultiple(SignaledObject *events[], int n, int timeout = -1);
};

/// Encapsulation of Event
class Event : public SignaledObject
{
public:
  Event() : SignaledObject(CreateEvent(NULL, FALSE, FALSE, NULL)) {}
  explicit Event(bool manualReset) : SignaledObject(CreateEvent(NULL, manualReset, FALSE, NULL)) {}
  Event(enum _noInitHandle) : SignaledObject(NoInitHandle) {}

  bool Init(bool manualReset = false)
  {
    return SignaledObject::Init(CreateEvent(NULL, manualReset, FALSE, NULL));
  }
  bool InitFromHandle(HANDLE handle)
  {
    return SignaledObject::Init(handle);
  }

  void Set() {if (_handle) SetEvent(_handle);}
  void Reset() {if (_handle) ResetEvent(_handle);}
  void Pulse() {if (_handle) PulseEvent(_handle);}
  
  bool IsInitialized() const {return _handle!=NULL;}
};

/// Encapsulation of Mutex
class Mutex: public SignaledObject
{
public:
  Mutex() : SignaledObject(CreateMutex(NULL, FALSE, NULL)) {}
  Mutex(enum _noInitHandle) : SignaledObject(NoInitHandle) {}

  bool Init()
  {
    return SignaledObject::Init(CreateMutex(NULL, FALSE, NULL));
  }

  void Unlock() const {if (_handle) ReleaseMutex(_handle);}
  bool IsLocked() const
  {
    bool ret = TryLock();
    if (ret) Unlock();
    return ret;
  }
};

/// Encapsulation of Semaphore
class Semaphore: public SignaledObject
{
  // example: serialize disk access between audio and texture
  // semaphore semantics would be: signaled when disk is busy
  // when texture load receives a request for loading, is Unlocks semaphore
  // (semaphore becomes signaled)
  // after processing request (load or delete), semaphore is Locked
  // and eventually becomes non-signaled
  // audio can check semaphore to see if texture loader uses disk
  // if semaphore is signaled, disk is busy
  
public:
  Semaphore(int init = 0, int max = INT_MAX)
  :SignaledObject(CreateSemaphore(NULL,init,max,NULL))
  {
  }
  Semaphore(enum _noInitHandle) : SignaledObject(NoInitHandle) {}

  bool Init(int init = 0, int max = INT_MAX)
  {
    return SignaledObject::Init(CreateSemaphore(NULL, init, max, NULL));
  }

  bool Unlock(int count = 1) const {return _handle ? ReleaseSemaphore(_handle, count, NULL) != FALSE : true;}
  bool IsLocked() const
  {
    bool ret = TryLock();
    if (ret) Unlock();
    return ret;
  }
};

/// multi threaded synchronization functionality
class CriticalSection
{
private:
  bool _initialized;
  mutable CRITICAL_SECTION _handle;
  
public:
  /// create uninitialized object
  CriticalSection(enum _noInitHandle) {_initialized = false;}
  /// create initialized object
  CriticalSection() {InitializeCriticalSection(&_handle); _initialized = true;}
  /// create initialized object
  CriticalSection(DWORD spinCount) {InitializeCriticalSectionAndSpinCount(&_handle, spinCount); _initialized = true;}
  ~CriticalSection() {Done();}

  void Init()
  {
    DoAssert(!_initialized);
    InitializeCriticalSection(&_handle);
    _initialized = true;
  }
  void Done()
  {
    if (_initialized)
    {
      DeleteCriticalSection(&_handle);
      _initialized = false;
    }
  }

  bool Lock() const
  {
    if (_initialized) 
    { 
      ENTER_DEAD_LOCK_DETECTOR  
      EnterCriticalSection(&_handle);
    }
    return true;
  }
  void Unlock() const
  {
    if (_initialized) 
    {
      LEAVE_DEAD_LOCK_DETECTOR
      LeaveCriticalSection(&_handle);
    }
  }
};

#include <Es/Types/scopeLock.hpp>

class ScopeLockMutex: public ScopeLock<Mutex>
{
  public:
  ScopeLockMutex( Mutex &lock )
  :ScopeLock<Mutex>(lock)
  {}
  bool TryLock() const {return _lock.TryLock();}
};

typedef ScopeLock<CriticalSection> ScopeLockSection;

#else

//----------------- POSIX implementation ---------------------

#include "Es/essencepch.hpp"
#include "Es/Common/global.hpp"
#include "Es/Threads/pocritical.hpp"

class SignaledObject
{
  private:
  // no copy
  SignaledObject ( const SignaledObject &src );
  void operator = ( const SignaledObject &src );

  public:
  /// create uninitialized object
  SignaledObject(enum _noInitHandle) {}
  SignaledObject ()
  {}
  ~SignaledObject()
  {}

  HANDLE DetachHandle() const { return 0; }

  // wait/lock are synonymous
  bool Wait () const
  { return TRUE; }
  
  bool TryWait ( DWORD time=0 ) const
  { return TRUE; }
  
  bool Lock () const
  { return TRUE; }
  
  bool TryLock () const
  { return TRUE; }

  HANDLE GetHandle() const{ return 0; }
  /// check the initialization state
  operator bool() const { return false; }
  void Done() {}

  bool Init(HANDLE handle)
  {
    return TRUE;
  }

  static int WaitForMultiple ( SignaledObject *events[], int n, int timeout=-1 );
};

class Event: public SignaledObject
{
  public:
  Event ()
  {}
  explicit Event ( bool manualReset )
  {}
  void Set ()
  {}
  void Reset ()
  {}
  bool InitFromHandle(HANDLE handle)
  {
    return SignaledObject::Init(handle);
  }
  bool IsInitialized() const {return false;}
};

class Mutex: public SignaledObject
{
        protected:
  mutable pthread_mutex_t mutex;
  
  public:
  Mutex ()
  {
      mutex = mutexInit;
  }
  void Unlock () const
  {
      pthread_mutex_unlock(&mutex);
  }
  bool IsLocked () const
  {
      bool ret = (pthread_mutex_trylock(&mutex) == 0);
      if ( ret ) Unlock();
      return ret;
  }
};

class Semaphore: public SignaledObject
{
protected:
  mutable sem_t sem;
  // example: serialize disk access between audio and texture
  // semaphore semantics would be: signaled when disk is busy
  // when texture load receives a request for loading, is Unlocks semaphore
  // (semaphore becomes signaled)
  // after processing request (load or delete), semaphore is Locked
  // and eventually becomes non-signaled
  // audio can check semaphore to see if texture loader uses disk
  // if semaphore is signaled, disk is busy

public:
  Semaphore ( int init=0, int max=INT_MAX )
  {
    sem_init(&sem,0,(unsigned)init);
  }
  bool Unlock ( int count=1 ) const
  {
    while ( count-- > 0 )
      sem_post(&sem);
    return TRUE;
  }
  bool IsLocked () const
  {
    int val = 0;
    if ( sem_getvalue(&sem,&val) != 0 ) val = 0;
    return( val != 0 );
  }
};

// multi threaded synchronization functionality
class CriticalSection
{
  private:
  mutable pthread_mutex_t mutex;
  
  public:
  CriticalSection ()
  {
    mutex = mutexInit;
  }
  ~CriticalSection ()
  {
    pthread_mutex_destroy(&mutex);
  }
  bool Lock () const
  {
    pthread_mutex_lock(&mutex);
    return true;
  }
  void Unlock () const
  {
    pthread_mutex_unlock(&mutex);
  }
};

#include <Es/Types/scopeLock.hpp>

class ScopeLockMutex: public ScopeLock<Mutex>
{
  public:
  ScopeLockMutex ( Mutex &lock ) : ScopeLock<Mutex>(lock)
  {}
  bool TryLock() const
  {
      return _lock.TryLock();
  }
};

typedef ScopeLock<CriticalSection> ScopeLockSection;

#endif

#if _MSC_VER
#include <intrin.h>

#if _DEBUG || _RELEASE_NOOPT// without debug intrinsics are allowed globally
  #pragma intrinsic ( _InterlockedAnd ) 
  #pragma intrinsic ( _InterlockedOr ) 
  #pragma intrinsic ( _InterlockedXor ) 
#endif
#else //LINUX ... empty implementation
inline void _ReadWriteBarrier () {__sync_synchronize();}
inline void _ReadBarrier () {__sync_synchronize();}
inline void _WriteBarrier () {__sync_synchronize();}
inline void MemoryBarrier() {__sync_synchronize();}
#endif

/// int with atomic (interlocked) operations
class AtomicInt
{
  volatile long _value;

#ifndef _WIN32
  /// critical section to control access to _value
  PoCriticalSection _atomicIntCriticalSection;

  void enterAtomicInt() { _atomicIntCriticalSection.enter(); }
  void leaveAtomicInt() { _atomicIntCriticalSection.leave(); }
#endif

  public:  
  operator int() const volatile {return _value;}
  //operator AtomicInt &() {return _value;}
  explicit AtomicInt(int val) {_value = val;}
  /// having default constructor as uninitialized is a little bit unsafe, but performant
  AtomicInt(){}
  
  void operator = (const AtomicInt &src) volatile {_value = src._value;}
  
  //@{ note: postfix implementation not native, but we can derive original value from the resulting one
  int operator ++(int postfix) volatile {return InterlockedIncrement(&_value)-1;}
  int operator --(int postfix) volatile {return InterlockedDecrement(&_value)+1;}
  //@}
  int operator ++() volatile {return InterlockedIncrement(&_value);}
  int operator --() volatile {return InterlockedDecrement(&_value);}
  /** note: native implementation returns original value, but we can derive the new one */
  int operator += (int x) volatile {return InterlockedExchangeAdd(&_value,x)+x;}
  int operator -= (int x) volatile {return InterlockedExchangeAdd(&_value,-x)-x;}
#ifdef _WIN32
  // caution: X360 and PC returns a different value from _InterlockedOr/_InterlockedAnd - do not rely upon it
  void operator |= (int x) volatile {_InterlockedOr(&_value,x);}
  void operator &= (int x) volatile {_InterlockedAnd(&_value,x);}
  int CompareSwap(int compare, int exchange) {return _InterlockedCompareExchange(&_value,exchange,compare);}
  int Swap(int exchange) {return _InterlockedExchange(&_value,exchange);}
#else
  int CompareSwap(int compare, int exchange) 
  {
    enterAtomicInt(); 
    long retVal = _value;
    if (_value==compare) _value = exchange;
    leaveAtomicInt();
    return retVal;
  }
  int Swap(int exchange) 
  {
    enterAtomicInt(); 
    long retVal = _value;
    _value = exchange;
    leaveAtomicInt();
    return retVal;
  }
  void operator |= (int x) { enterAtomicInt(); _value |= x;  leaveAtomicInt(); }
  void operator &= (int x) { enterAtomicInt(); _value &= x;  leaveAtomicInt(); }
#endif
};

#if _M_IX86_FP>0 // when SSE enabled, use it

  // TODO: check YieldProcessor vs _mm_pause efficiency
  #define YieldProcessorNice() YieldProcessor();YieldProcessor();YieldProcessor();YieldProcessor()

#elif defined _WIN32

  // YieldProcessor() is portable
  #define YieldProcessorNice() YieldProcessor()

#else

  #define YieldProcessorNice() sched_yield()

#endif


/// publish any data modified so far (Release semantics, done before advertising)
/**
See Lockless Programming Considerations for Xbox 360 and Microsoft Windows
*/
static __forceinline void MemoryPublish()
{
  #if !defined _DEBUG && !defined _RELEASE_NOOPT
    // cannot use in debug: linker errors because of missing function
    // not a real problem: with debug no optimizations done, no compiler barriers needed
    _WriteBarrier();
  #endif
    // make sure write complete before any reads are started (on x86 reads can be reordered ahead of writes)
    #if _M_IX86_FP>0
      // MemoryBarrier not using SSE on Win32, only xchng, explicit SSE seems more efficient
      _mm_sfence();
    #else
      MemoryBarrier();
    #endif
}

/// subsribe to data (Acquire semantics, done after checking)
static __forceinline void MemorySubscribe()
{
#if !defined _DEBUG && !defined _RELEASE_NOOPT
  // cannot use in debug: linker errors because of missing function
  // not a real problem: with debug no optimizations done, no compiler barriers needed
    _ReadBarrier();
  #endif
    // no CPU memory barrier required on x86 - reads not reordered by CPU
  
}


#if 0

/**
adapted from http://www.glennslayden.com/code/win32/reader-writer-lock
Writer preference readers / writer lock
*/

class WriterLock
{
protected:
  mutable CriticalSection _writeCS;
  mutable Event _readersCleared;

public:
  WriterLock():_readersCleared(true/*manual reset*/)
  {
    _readersCleared.Set();
  }
  ~WriterLock()
  {
    _readersCleared.Wait();
  }
  void Lock() const
  {
    _writeCS.Lock(); // block any coming readers
    _readersCleared.Wait(); // wait until all readers left
  }
  void Unlock() const
  {
    _writeCS.Unlock();
  }
};


class ReadersWriterLock: public WriterLock
{
  mutable CriticalSection _readerCountCS;
  mutable long _readersCount;
public:
  ReadersWriterLock()
  {
    _readersCount = 0;
  }

  WriterLock &GetWriterLock() {return *this;}

  void Lock() const
  {
    _writeCS.Lock(); // give writer opportunity to lock (write preference)
    _readerCountCS.Lock();
    if (++_readersCount == 1)
      _readersCleared.Reset();

    _readerCountCS.Unlock();
    _writeCS.Unlock();
  }

  void Unlock() const
  {
    _readerCountCS.Lock();
    if (--_readersCount == 0)
      _readersCleared.Set();
    _readerCountCS.Unlock();
  }

};

#else

// dummy implementation, real implementation causes deadlocks

class WriterLock
{
public:
  WriterLock()
  {
  }
  ~WriterLock()
  {
  }
  void Lock() const
  {
  }
  void Unlock() const
  {
  }
};

class ReadersWriterLock: public WriterLock
{
};


#endif

#endif
