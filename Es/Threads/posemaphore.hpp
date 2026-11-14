#ifdef _MSC_VER
#  pragma once
#endif

/**
    @file   posemaphore.hpp
    @brief  Portable semaphore objects.

    Copyright (C) 2001-2002 by BIStudio (www.bistudio.com)
    @author PE
    @since  29.11.2001
    @date   11.6.2002
*/

#ifndef _POSEMAPHORE_H
#define _POSEMAPHORE_H

//------------------------------------------------------------
//  Portable simple semaphore:

/**
    Portable simple semaphore.
*/
#include "pothread.hpp"
#include <Es/Containers/array.hpp>

class PoSemaphore : public RefCountSafe {

protected:
  /// Critical section to lock object. RefCountSafe lock cannot be used due to possible DeadLock. 
  PoCriticalSection lock;

  /// Enter the object's critical section.
  inline void enter() const
  { lock.enter(); }

  /// Leave the object's critical section.
  inline void leave() const
  { lock.leave(); }

#ifdef _WIN32

    /// HANDLE returned by ::CreateSemaphore() call
    HANDLE handle;

#else

    /// POSIX semaphore structure
    sem_t sem;

#endif

public:

    /// error status (<code>true</code> should be very rare).
    bool error;

    /**
        Initializes the semaphore.
        <p>#error is set if maximum allowed counter value is exceeded.
        Generally - failure reasons are not portable.
        @param  init Initial counter value (must be <code>&gt;=0</code>).
        @param  maxCount Maximum counter value (=maximum number of shared objects/titbits/signals)
    */
    PoSemaphore ( long init =0L, long maxCount =12 );

    /**
        Wait operation.
        Suspends (blocks) the current thread until the semaphore has positive counter value.
        <p>#error variable is not affected.
    */
    virtual void wait ();

    /**
        Conditional wait operation.
        Test if the semaphore's counter is positive. Returns <code>true</code> if
        it has succeeded, <code>false</code> if the semaphore is blocked.
        Returns immediately in both cases.
        <p>#error variable is not affected.
        @return <code>true</code> if (non-blocking) wait operation was executed successfully
    */
    virtual bool tryWait ();

    /**
        Signal operation.
        Increments the semaphore's counter and can cause re-scheduling of some thread[s]
        waiting for that..
        @param  count Increment count (number of potentially waiting threads to start again).
                      If maximum allowed counter value is exceeded,
                      #error is set.
    */
    virtual void signal ( long count =1L );

    /**
        Gets the actual semaphore counter value.
        @return Actual counter value.
    */
    virtual long getValue ();

    /**
        Destroys the semaphore instance.
        There shouldn't be any threads waiting for it - due to the
        @link RefCount @endlink ancestor.
    */
    virtual ~PoSemaphore ();

    };

//------------------------------------------------------------
//  Support for AutoArray<Type>:

TypeIsSimpleZeroed(void*);

//------------------------------------------------------------
//  Portable semaphore with "titbit" capability:

/**
    @class  PoSemaphoreTitbit
    Portable semaphore with "titbit" capability.
    <p>A "tibit" value (TitbitType) is stored together with each
    signal() event - consumers (wait()) get titbits in FIFO order.
    @param  TitbitType Data type stored with each signal() operation.
*/
template < class TitbitType >
class PoSemaphoreTitbit : public PoSemaphore {

protected:

    AutoArray<TitbitType,MemAllocSafe> tb;  ///< Titbit array.

    long size;                              ///< Maximum counter value (= titbit array size).

    long first;                             ///< Next titbit to be consumed.

    long last;                              ///< Points to place where the next incoming titbit will be stored.

    TitbitType defaultTitbit;               ///< Default titbit value.

    /**
        Puts the given titbit into the queue.
        Doesn't use synchronization at all!
        @param  titbit A new titbit.
    */
    void putTitbit ( TitbitType titbit )
    {
        long newLast = last + 1;
        if ( newLast >= size ) newLast = 0L;
        if ( newLast != first ) {
            tb[last] = titbit;
            last = newLast;
            }
    }

    /**
        Gets next titbit from the queue.
        Uses synchronization if #cs is initialized.
        @param  The oldest titbit stored in the queue. Undefined if the queue is empty.
    */
    void getTitbit ( TitbitType &titbit )
    {
        enter();
        if ( first != last ) {
            titbit = tb[first++];
            if ( first >= size ) first = 0L;
            }
        leave();
    }

public:

    /**
        Initializes the semaphore.
        <p>#error is set if maximum allowed counter
        value is exceeded. Generally - failure reasons are not portable.
        @param  init Initial counter value (must be <code>&gt;=0</code>).
        @param  maxCount Maximum counter value (=maximum number of shared objects/titbits/signals)
    */
    PoSemaphoreTitbit ( long init =0L, long maxCount =12L ) : PoSemaphore(init,maxCount)
    {
        LockRegister(lock,"PoSemaphoreTitbit");
        size = first = last = 0;
        if ( maxCount < 1 ) maxCount = 1;
        if ( init < 0L ) init = 0L;
        else
            if ( init > maxCount ) init = maxCount;
        if ( error ) return;
        tb.Resize(size=maxCount+12);        // room for 10 nested interrupted waits
        long i;
        for ( i = 0; i++ < init; )
            putTitbit(defaultTitbit);
    }

    /**
        Wait operation.
        Suspends (blocks) the current thread until the semaphore has positive counter value.
        <p>#error variable is not affected,
        associated titbit is discarded.
    */
    virtual void wait ()
    {
        PoSemaphore::wait();
        TitbitType dummy;
        getTitbit(dummy);
    }

    /**
        Wait operation.
        Suspends (blocks) the current thread until the semaphore has positive counter value.
        <p>#error variable is not affected.
        @param titbit Titbit to be consumed.
    */
    virtual void wait ( TitbitType &titbit )
    {
        PoSemaphore::wait();
        getTitbit(titbit);
    }

    /**
        Conditional wait operation.
        Test if the semaphore's counter is positive.
        Returns <code>true</code> if it has succeeded, <code>false</code> if the semaphore is blocked.
        Returns immediately in both cases.
        <p>#error variable is not affected,
        associated titbit is discarded in case of success.
        @return <code>true</code> if (non-blocking) wait operation was executed successfully.
    */
    virtual bool tryWait ()
    {
        bool success = PoSemaphore::tryWait();
        if ( success ) {
            TitbitType dummy;
            getTitbit(dummy);
            }
        return success;
    }

    /**
        Conditional wait operation.
        Test if the semaphore's counter is positive.
        Returns <code>true</code> if it has succeeded, <code>false</code> if the semaphore is blocked.
        Returns immediately in both cases.
        <p>#error variable is not affected.
        @param titbit Titbit to be consumed (won't be filled if wait operation fails)
        @return <code>true</code> if (non-blocking) wait operation was executed successfully.
    */
    virtual bool tryWait ( TitbitType &titbit )
    {
        bool success = PoSemaphore::tryWait();
        if ( success ) getTitbit(titbit);
        return success;
    }

    /**
        Signal operation.
        Increments the semaphore's counter and can cause re-scheduling of some thread[s]
        waiting for that..
        <p>Uses default titbit (setDefaultTitbit()).
        @param  count Increment count (number of potentially waiting threads to start
                      again). If maximum allowed counter value is exceeded,
                      #error is set.
    */
    virtual void signal ( long count =1L )
    {
        if ( count < 1L ) count = 1L;
        enter();
        PoSemaphore::signal(count);
        if ( !error )
            while ( count-- ) putTitbit(defaultTitbit);
        leave();
    }

    /**
        Signal operation.
        Increments the semaphore's counter and can cause re-scheduling of some thread[s]
        waiting for that..
        <p>If maximum allowed counter value is exceeded, #error is set.
        @param titbit Titbit associated with this signal event (it will be passed to
                      future consument).
    */
    virtual void signal ( TitbitType titbit )
    {
        enter();
        PoSemaphore::signal();
        putTitbit(titbit);
        leave();
    }

    /**
        Signal operation.
        Increments the semaphore's counter and can cause re-scheduling of some thread[s]
        waiting for that..
        @param  count Increment count (number of potentially waiting threads to
                      start again). If maximum allowed counter value is exceeded,
                      #error is set.
        @param  titbitArray Caller-supplied array with titbit values. If <code>NULL</code>, it is
                            equivalent to <code>signal(long)</code>.
    */
    virtual void signal ( long count, TitbitType *titbitArray )
    {
        if ( count < 1L ) count = 1L;
        enter();
        PoSemaphore::signal(count);
        if ( !error )
            while ( count-- ) putTitbit(*titbitArray++);
        leave();
    }

    /**
        Sets the default titbit for subsequent <code>signal(long)</code> calls.
        <p>#error variable is not affected.
        @param  dTitbit New default titbit value.
    */
    virtual void setDefaultTitbit ( TitbitType dTitbit )
    {
        defaultTitbit = dTitbit;
    }

    /**
        Destroys the semaphore instance (there shouldn't be any threads waiting for it -
        due to the @link RefCount @endlink ancestor).
    */
    virtual ~PoSemaphoreTitbit ()
    {
    }

    };

#endif
