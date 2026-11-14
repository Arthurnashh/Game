//-----------------------------------------------------------------------------
// File: enf_thread.h
//
// Desc: System thread
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_THREAD_H
#define ENF_THREAD_H

#include "enf_config.h"
#include "enf_interlocked.h"
#ifdef __APPLE__
	#include <pthread.h>
	#include <mach/mach_types.h>
#endif

namespace enf
{
/** @addtogroup System
@{*/

enum SynchronizeType
{
	SYNC_NONE,
	SYNC_SEMAPHORE,
	SYNC_EVENT,
};

class PUBLIC_API CSynchronize
{
protected:
	friend class CThread;
#if defined(WIN32) || defined(ENF_XENON)
	void*		m_hHandle;
#elif defined(ENF_PS3)
	uint	m_hHandle;
#endif

public:
	CSynchronize();
	virtual ~CSynchronize();
	virtual SynchronizeType GetType() const=0;
};

//!System semaphore wrapper
class PUBLIC_API CSemaphore: public CSynchronize
{
private:
	friend class CThread;
#ifdef __MACOSX__
	semaphore_t m_hHandle;
#endif
public:
	/*!
	Creates semaphore
	\param initialCount Initial count of semaphore
	\param maximumCount Maximum count
	*/
	CSemaphore(uint initialCount, uint maximumCount);
	//!Destroys semaphore
	~CSemaphore();
	//!Increments semaphore
	uint Release(uint releaseCount = 1);
	SynchronizeType GetType() const
	{
		return SYNC_SEMAPHORE;
	}
};

//!System event wrapper
class PUBLIC_API CEvent: public CSynchronize
{
private:
	friend class CThread;
#ifdef __MACOSX__
	pthread_cond_t m_hCondHandle;
	pthread_mutex_t m_hMutexHandle;
	bool m_bReady;
#endif

public:
	/*!
	Creates event
	\param manualReset
	If this parameter is TRUE, the function creates a manual-reset event object which requires use of
	the CEvent::Reset set the state to non-signaled. If this parameter is FALSE, the function creates
	an auto-reset event object, and system automatically resets the state to non-signaled after a
	single waiting thread has been released.
	\param initialState
	If this parameter is TRUE, the initial state of the event object is signaled; otherwise, it is nonsignaled
	*/
	CEvent(bool manualReset, bool initialState);

	//!Destroys event
	~CEvent();

	//!Tells whether is event in signaled state
	bool IsSet() const;

	//!Sets event to signaled state
	void Set();

	//!Resets event to nonsignaled state
	void Reset();

	SynchronizeType GetType() const
	{
		return SYNC_EVENT;
	}
};

/*!
System thread wrapper
*/
class PUBLIC_API CThread
{
private:
	enum Status
	{
		STOPPED,
		RUNNING,
		SUSPEND
	};

	mutable IL_INT32	m_Status;
	size_t	m_iStackSize;

	friend uint _InternalStartThread(void* lpThreadParameter);

#if defined(WIN32) || defined(ENF_XENON)
	mutable void*		m_hThread;
	mutable int		m_iThreadID;
#elif defined(ENF_PS3)
	mutable sys_ppu_thread_t	m_hHandle;
#elif defined(__MACOSX__)
	mutable pthread_t m_hHandle;
#endif

	virtual void Run()=0;
	void Exit(int exitcode);

protected:

	#ifdef ENF_DEVELOPER
		SString<> m_sThreadName;
	#endif

	static const uint DEFAULT_STACK_SIZE = 65536;
	
	/*!
	Wait for this thread instance to exit.
	This does not actually stop the thread, call this from your Kill() implementation
	has been invoked.
	*/
	void WaitForExit() const;
	void Ended();

public:
	static const uint INFINITE_TIMEOUT = 0xffffffff;
	static const uint W_TIMEOUT = 0xffffffff;

	/*!
	Stops the thread until one or all events are signaled
	\param events Array of events to be waiting for
	\param numEvents Number of events
	\param waitAll When set to TRUE, it waits until all of events are signaled
	\param timeout in milliseconds or INFINITE_TIMEOUT
	\return Either index of event that has been signaled, or W_TIMEOUT
	*/
	static uint WaitForMultiple(CSynchronize** events, uint numEvents, bool waitAll, uint timeout);

	/*!
	Stops the thread until event is signaled
	\param event Event to be waiting for
	\param timeout in milliseconds or INFINITE_TIMEOUT
	\return Either index of event that has been signaled (0), or W_TIMEOUT
	*/
	static uint WaitForSingle(CSynchronize& event, uint timeout);

	static void Sleep(uint milliseconds);

	virtual ~CThread();
	CThread(uint stacksize = DEFAULT_STACK_SIZE);

	bool IsRunning() const
	{
		return m_Status == RUNNING;
	}
	bool Start(uint core, const char* name, int priority = 0);
	virtual void Kill()=0;
};

//@}

}

#endif //ENF_THREAD_H