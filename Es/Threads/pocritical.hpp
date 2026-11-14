#ifdef _MSC_VER
#  pragma once
#endif

/**
    @file  pocritical.hpp
    @brief Portable critical section.

    Copyright &copy; 2001-2003 by BIStudio (www.bistudio.com)
    @author PE
    @since  29.11.2001
    @date   13.2.2003
*/

#ifndef _POCRITICAL_H
#define _POCRITICAL_H

//------------------------------------------------------------
//  Lock-tracing (for debugging purposes only):

#if !_SUPER_RELEASE
//#  define LOCK_TRACING
#endif

#ifdef LOCK_TRACING

#  define LockDecl(lock,descr)      lock(__FILE__,__LINE__,descr)
#  define LockInit(lock,descr,val)  lock(__FILE__,__LINE__,descr,val)
#  define LockRegister(lock,descr)  lock.registerMe(__FILE__,__LINE__,descr)

#else

#  define LockDecl(lock,descr)      lock
#  define LockInit(lock,descr,val)  lock(val)
#  define LockRegister(lock,descr)

#endif

//------------------------------------------------------------
//  Portable critical section:

#ifndef _WIN32
extern pthread_mutex_t mutexInit;
#endif

#include <Es/Types/pointers.hpp>


#if defined _WIN32
#  include <Es/Common/win.h>
#else
#  include <pthread.h>
#  include <semaphore.h>
#endif

#include <Es/Memory/normalNew.hpp>

#if _SUPER_RELEASE
// never use DeadLockDetector in SUPER_RELEASE!
//((TO_BE_USED_IN_FINAL
#define _USE_DEAD_LOCK_DETECTOR 0
#define _USE_DEAD_LOCK_DETECTOR_POS 0
//))TO_BE_USED_IN_FINAL
/*
//((TO_BE_REMOVED_IN_FINAL
#undef _USE_DEAD_LOCK_DETECTOR
#undef _USE_DEAD_LOCK_DETECTOR_POS
#define _USE_DEAD_LOCK_DETECTOR 1
#define _USE_DEAD_LOCK_DETECTOR_POS 0
//))TO_BE_REMOVED_IN_FINAL
*/
#else
#define _USE_DEAD_LOCK_DETECTOR 0
#define _USE_DEAD_LOCK_DETECTOR_POS 0
#endif

/**
    Portable critical section.
*/
class PoCriticalSection
{

protected:

#ifdef _WIN32

    /// Win32 structure to represent state of a critical section.
    mutable CRITICAL_SECTION cs;

#else

    /// POSIX mutex structure.
    mutable pthread_mutex_t mutex;

#endif

#ifdef LOCK_TRACING

    /// Lock id (index into LockInstance array) or -1 if not registered.
    mutable int id;

#endif

    /// Validity of this lock (if <code>false</code>, all operations are ignored).
    bool valid;

public:

    /// error status (<code>true</code> should be very rare).
    mutable bool error;

    /**
        Initializes the critical section.
        <p>The critical section can be entered recursively by the same thread..
        @param val Is this lock valid? If <code>false</code>, all operations are ignored.
    */
    PoCriticalSection ( bool val );

    /**
        Initializes the critical section.
        <p>The critical section can be entered recursively by the same thread..
    */
    PoCriticalSection ();

#ifdef LOCK_TRACING

    /**
        Initialization for lock-tracing.
    */
    PoCriticalSection ( const char *srcFile, int lineNo, const char *descr =NULL );

    void registerMe ( const char *srcFile, int lineNo, const char *descr =NULL );

#endif

    /**
        Enters (locks) the critical section. If the section is already locked, suspends (blocks)
        the current thread until it is unlocked again.
        <p>One process is able to call enter() more than one times.
        But anyway - it has to call leave() exactly equal times afterwards..
    */
    void enter () const;

    /// The synonym for enter.
    inline void Lock () const
    {
        enter();
    }

    /**
        Test if the critical section is locked - if not, enters it immediately and returns <code>true</code>.
        Otherwise returns <code>false</code> (does not block in any case).
        @return <code>true</code> if (non-blocking) enter operation was executed successfully.
    */
    bool tryEnter () const;

    /**
        Leaves (unlocks) previously entered critical section.
        <p>Number of enter() (plus successfull tryEnter()) operations
        must match with number of subsequent leave() calls!
    */
    void leave () const;

    void enterNoDeadLockDetector() const;
    void leaveNoDeadLockDetector() const;

    /// The synonym for leave.
    inline void Unlock () const
    {
        leave();
    }

    /**
        Destroys the critical section (there shouldn't be any threads waiting for it -
        due to the @link RefCount @endlink ancestor).
    */
    ~PoCriticalSection ();

    /**
        MT-safe new operator.
    */
    static void* operator new ( size_t size );

    static void* operator new ( size_t size, const char *file, int line );

    /**
        MT-safe delete operator.
    */
    static void operator delete ( void *mem );

#ifdef __INTEL_COMPILER

    /**
        Intel compiler needs this for exception unwind..
    */
    static void operator delete ( void *mem, const char *file, int line );

#endif

    };

#include <Es/Memory/debugNew.hpp>

#endif
