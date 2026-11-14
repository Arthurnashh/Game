//-----------------------------------------------------------------------------
// File: enf_threadmanager.h
//
// Desc: System thread manager
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_THREADMANAGER_H
#define ENF_THREADMANAGER_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "system/enf_mutex.h"
#include "system/enf_thread.h"

namespace enf
{
/** @addtogroup System
@{*/

class CThreadManager;

//-----------------------------------------------------------------------------
class PUBLIC_API CThreadJob
{
public:
	ENF_DECLARE_SMALLOBJECT(CThreadJob);
	CThreadJob() {}
	virtual int Run()=0;
	virtual ~CThreadJob()
	{}
};

//-----------------------------------------------------------------------------
enum JobState
{
	JS_EMPTY,
	JS_QUEUED,
	JS_RUNNING,
	JS_FINISHED
};

//-----------------------------------------------------------------------------
template<class T> class CCyclicQueue
{
protected:
	T*				m_pQueue;
	IL_INT32	m_iBottom;
	IL_INT32	m_iTop;
	uint			m_iSize;
	uint			m_iShift;

public:

	~CCyclicQueue()
	{
		SAFE_DELETE_ARRAY(m_pQueue);
	}

	CCyclicQueue(uint size): m_pQueue(NULL), m_iBottom(0), m_iTop(0), m_iSize(size)
	{
		uint n = 0;
		for(; n < 32; n++)
		{
			if((1 << n) == size)
				break;
		}
		enf_assert2(n != 32, "Size must be power of two!");
		m_iShift = n;
		m_pQueue = ENF_NEW T[size];
	}

	ENF_INLINE bool IsValid(uint spincount) const
	{
		int diff = 0;
		uint spin = m_iTop >> m_iShift;
		if(spincount > spin)
			diff = (65536 - spincount) + spin;
		else
			diff = spin - spincount;

		return (diff <= 1);
	}
	ENF_INLINE bool IsEmpty() const
	{
		uint mask = m_iSize - 1;
		return ((m_iBottom & mask) == (m_iTop & mask));
	}

	ENF_INLINE uint NumItems() const
	{
		uint mask = m_iSize - 1;
		uint bottom = m_iBottom & mask;
		uint top = m_iTop & mask;
		if(bottom <= top)
			return top - bottom;
		else
			return (m_iSize - bottom) + top;
	}

	ENF_INLINE T* Enqueue()
	{
		if(NumItems() < (m_iSize - 2))
		{
			uint top = IL_INCREMENT(m_iTop) - 1;
			T* item = &m_pQueue[top & (m_iSize - 1)];
			return item;
		}
		return NULL;
	}
	ENF_INLINE T* Enqueue(uint& handle)
	{
		if(NumItems() < (m_iSize - 2))
		{
			uint top = IL_INCREMENT(m_iTop) - 1;
			handle = top;
			T* item = &m_pQueue[top & (m_iSize - 1)];
			return item;
		}
		return NULL;
	}
	
	ENF_INLINE T* Dequeue()
	{
		if(!IsEmpty())
		{
			return &m_pQueue[m_iBottom & (m_iSize - 1)];
		}
		return NULL;
	}
};

//-----------------------------------------------------------------------------
struct QueueItem
{
	JobState		m_State;
	CThreadJob*	m_pJob;
	int			m_iResult;
	uint			m_iPad0;
	ENF_INLINE QueueItem();
};

typedef CCyclicQueue<QueueItem> TJobQueue;

//-----------------------------------------------------------------------------
class PUBLIC_API CThreadManager: protected CThread, protected TJobQueue
{
public:
	CThreadManager(uint queuesize, uint stacksize);
	~CThreadManager();
	JobState GetState(TJobHandle handle);
	int GetResult(TJobHandle handle);


	void WaitForIdle();
	int WaitFor(TJobHandle handle);
	TJobHandle Submit(CThreadJob* job);

	bool Start(uint core, const char* name)
	{
		return CThread::Start(core, name);
	}
	void Kill();

protected:
	void Run();

	ENF_INLINE QueueItem* RemoveLastAndDequeue();
	ENF_INLINE void RemoveLast();

	ENF_INLINE QueueItem* Dequeue();
	ENF_INLINE uint Enqueue(CThreadJob* job);
	CSemaphore	m_QueueSemaphore;
	IL_INT32		m_QuitFlag;
	CEvent			m_EmptyEvent;
	CMutex			m_QueueMutex;
};

//@}
}

#endif //ENF_THREADMANAGER_H