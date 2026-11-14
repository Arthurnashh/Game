//-----------------------------------------------------------------------------
// File: enf_mutex.h
//
// Desc: Mutual exclusive access object
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_MUTEX_H
#define ENF_MUTEX_H

#include "enf_config.h"

#ifdef ENF_XENON
	#include <xtl.h>
#elif defined(ENF_PS3)
	#include <sys/synchronization.h>
#elif defined(__APPLE__)
	#include <pthread.h>
	#include <errno.h>
#endif

//!Placeholder for win32's CRITICAL_SECTION
#if defined(WIN32)
	struct SCriticalSectionStandIn
	{
		void* a;
		long b;
		long c;
		void* d;
		void* e;
		unsigned long f;
	};
	struct _RTL_CRITICAL_SECTION;

	extern "C"
	{
		__declspec(dllimport) void __stdcall InitializeCriticalSection(_RTL_CRITICAL_SECTION*);
		__declspec(dllimport) void __stdcall DeleteCriticalSection(_RTL_CRITICAL_SECTION*);
		__declspec(dllimport) void __stdcall EnterCriticalSection(_RTL_CRITICAL_SECTION*);
		__declspec(dllimport) int __stdcall TryEnterCriticalSection(_RTL_CRITICAL_SECTION*);
		__declspec(dllimport) void __stdcall LeaveCriticalSection(_RTL_CRITICAL_SECTION*);
	}
	#define P_CRITICAL_SECTION _RTL_CRITICAL_SECTION*
#elif defined(ENF_XENON)
	#define SCriticalSectionStandIn CRITICAL_SECTION
	#define P_CRITICAL_SECTION PRTL_CRITICAL_SECTION
#elif defined(ENF_PS3)
	#define SCriticalSectionStandIn sys_mutex_t
#elif defined(__APPLE__)
	#define SCriticalSectionStandIn pthread_mutex_t
#endif

namespace enf
{
/** @addtogroup System
@{*/

/*!
Object for preventing to access some place of code
from two threads at the same time.
*/
class PUBLIC_API CMutex
{
	mutable SCriticalSectionStandIn m_CS;

	// prevent copying and assignment
	CMutex(const CMutex&);
	const CMutex& operator=(const CMutex&);

public:

	//!Constructs mutex object
	ENF_INLINE CMutex()
	{
#ifdef ENF_PS3
		sys_mutex_attribute_t attr;
		sys_mutex_attribute_initialize(attr);
		attr.attr_recursive = SYS_SYNC_RECURSIVE;
		int res = sys_mutex_create(&m_CS, &attr);
		enf_assert(res == CELL_OK);
#elif defined(__APPLE__)
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		int res = pthread_mutex_init(&m_CS, &attr);
#elif defined(ENF_XENON) || defined(WIN32)
		InitializeCriticalSection((P_CRITICAL_SECTION)&m_CS);
#endif
	}

	//!Deletes mutex object
	ENF_INLINE ~CMutex()
	{
#if defined(ENF_XENON) || defined(WIN32)
		DeleteCriticalSection((P_CRITICAL_SECTION)&m_CS);
#elif defined(ENF_PS3)
		sys_mutex_destroy(m_CS);
#elif defined(__APPLE__)
		//pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
		pthread_mutex_destroy(&m_CS);
#endif
	}

	/*!
	Locks the mutex object. Any other thread,
	attempting to lock this mutex will be stopped until
	Unlock is called
	*/
	ENF_INLINE void Lock() const
	{
#ifdef ENF_PS3
		int ret = sys_mutex_lock(m_CS, 0);
		enf_assert(ret == CELL_OK);
#elif defined(ENF_XENON) || defined(WIN32)
		EnterCriticalSection((P_CRITICAL_SECTION)&m_CS);
#elif defined(__APPLE__)
		pthread_mutex_lock(&m_CS);
#endif
	}

	ENF_INLINE bool TryLock() const
	{
#ifdef ENF_PS3
		int res = sys_mutex_trylock(m_CS);
		if(res == EBUSY)
			return false;
		enf_assert(res == CELL_OK);
		return true;

#elif defined(__APPLE__)
		int res = pthread_mutex_trylock(&m_CS);
		if(res == EBUSY)
			return false;
		enf_assert(res == 0);
		return true;
#elif defined(ENF_XENON) || defined(WIN32)
		return TryEnterCriticalSection((P_CRITICAL_SECTION)&m_CS) != 0;
#endif
	}
	//!Unlocks the mutex object.
	ENF_INLINE void Unlock() const
	{
#ifdef ENF_PS3
		int res = sys_mutex_unlock(m_CS);
		enf_assert(res == CELL_OK);
#elif defined(ENF_XENON) || defined(WIN32)
		LeaveCriticalSection((P_CRITICAL_SECTION)&m_CS);
#elif defined(__APPLE__)
		pthread_mutex_unlock(&m_CS);
#endif
	}
};

static const bool ConstTrue = true;
template<class Type> class CLock
{
	const Type& m_ToLock;
	const bool& m_bCond;

	// prevent copying and assignment
	CLock(const CLock&);
	const CLock& operator=(const CLock&);

public:

	ENF_INLINE CLock(const Type& toLock, const bool& cond = ConstTrue) :
		m_ToLock(toLock),
		m_bCond(cond)
	{
		if(m_bCond)
			m_ToLock.Lock();
	}

	ENF_INLINE ~CLock()
	{
		if(m_bCond)
			m_ToLock.Unlock();
	}
};
/*!
Convenient way for locking CMutex, where CMutex::Lock()
is called from constructor and CMutex::Unlock() is called
from destructor, so you don't need to care about unlocking,
when leaving critical section of code
*/
typedef CLock<CMutex> CMutexLock;

template<class Type> class CLockPtr
{
	const Type* m_pToLock;

	// prevent copying and assignment
	CLockPtr(const CLockPtr&);
	const CLockPtr& operator=(const CLockPtr&);

public:

	ENF_INLINE CLockPtr(const Type* toLock) :
		m_pToLock(toLock)
		{
			if(m_pToLock)
				m_pToLock->Lock();
		}

		ENF_INLINE ~CLockPtr()
		{
			if(m_pToLock)
				m_pToLock->Unlock();
		}
};
/*!
Convenient way for locking CMutex, where CMutex::Lock()
is called from constructor and CMutex::Unlock() is called
from destructor, so you don't need to care about unlocking,
when leaving critical section of code
*/
typedef CLockPtr<CMutex> CMutexLockPtr;
//@}
}
#endif //ENF_MUTEX_H
