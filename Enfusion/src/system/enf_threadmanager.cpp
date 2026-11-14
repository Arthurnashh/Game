//#include "precomp.h"
#include "system/enf_mutex.h"
#include "system/enf_threadmanager.h"
#include "system/enf_profiler.h"

namespace enf
{
//-----------------------------------------------------------------------------
QueueItem::QueueItem(): m_State(JS_EMPTY), m_pJob(NULL), m_iResult(0)
{
}

//-----------------------------------------------------------------------------
uint CThreadManager::Enqueue(CThreadJob* job)
{
	ENF_PROFILE(PROFILE_THREAD_ENQUEUE);

	QueueItem* item = NULL;

	while(true)
	{
		uint handle;
		item = TJobQueue::Enqueue(handle);

		if(item)
		{
			enf_assert(item->m_State != JS_QUEUED || item->m_State != JS_RUNNING);
			enf_assert(item->m_pJob == NULL);
			item->m_State = JS_QUEUED;
			item->m_pJob = job;
			uint prev = m_QueueSemaphore.Release(1);
//	SString<> temp;
//	_OutputDebugString(temp.format("TM %x Enqueue %d %x items %d\n", (int)this, (int)prev, (int)job, NumItems()));

			return handle;
		}

		WaitForSingle(m_EmptyEvent, INFINITE_TIMEOUT);
	}
}

//-----------------------------------------------------------------------------
QueueItem* CThreadManager::Dequeue()
{
	QueueItem* item = TJobQueue::Dequeue();
	enf_assert(item == NULL || item->m_State == JS_QUEUED);
	if(item != NULL)
	{
//		SString<> temp;
//		_OutputDebugString(temp.format("TM %x Dequeue %x\n", (int)this, (int)item));
	}
	return item;
}

//-----------------------------------------------------------------------------
QueueItem* CThreadManager::RemoveLastAndDequeue()
{
	CMutexLock lock(m_QueueMutex);

	uint bottom = IL_INCREMENT(m_iBottom) - 1;
	bottom &= m_iSize - 1;
	enf_assert(m_pQueue[bottom].m_State == JS_RUNNING);
	m_pQueue[bottom].m_State = JS_FINISHED;
	m_EmptyEvent.Set();

	return TJobQueue::Dequeue();
}

//-----------------------------------------------------------------------------
void CThreadManager::RemoveLast()
{
	CMutexLock lock(m_QueueMutex);

	uint bottom = IL_INCREMENT(m_iBottom) - 1;
	bottom &= m_iSize - 1;
	enf_assert(m_pQueue[bottom].m_State == JS_RUNNING);
	m_pQueue[bottom].m_State = JS_FINISHED;
	m_EmptyEvent.Set();
}

//-----------------------------------------------------------------------------
TJobHandle CThreadManager::Submit(CThreadJob* job)
{
	uint top = Enqueue(job);
	return TJobHandle(top);
}

//-----------------------------------------------------------------------------
CThreadManager::~CThreadManager()
{
	Kill();
}

//-----------------------------------------------------------------------------
void CThreadManager::Kill()
{
	//FIXME: this is bad, there is a dangerous unlocked hole!
	WaitForIdle();

	//prevent from adding another shit
	CMutexLock lock(m_QueueMutex);

	//set quit
	IL_EXCHANGE(m_QuitFlag, 1);
	m_QueueSemaphore.Release();

	WaitForExit();
}

//-----------------------------------------------------------------------------
void CThreadManager::Run()
{
	while(true)
	{
		WaitForSingle(m_QueueSemaphore, INFINITE_TIMEOUT);
		if(IL_READ(m_QuitFlag))
			return;

		QueueItem* item = Dequeue();
		item->m_State = JS_RUNNING;
		item->m_iResult = item->m_pJob->Run();
		SAFE_DELETE(item->m_pJob);
		RemoveLast();
	}
}
//-----------------------------------------------------------------------------
CThreadManager::CThreadManager(uint queuesize, uint stacksize):
	CThread(stacksize),
	TJobQueue(queuesize),
	m_QueueSemaphore(0, queuesize + 1),
	m_QuitFlag(0),
	m_EmptyEvent(false, true)
{
}

//-----------------------------------------------------------------------------
int CThreadManager::WaitFor(TJobHandle handle)
{
	if(handle == BAD_JOBHANDLE)
		return 0;

	uint index  = handle & (m_iSize - 1);
	uint spincount = handle >> m_iShift;

	if(index >= m_iSize)
		return -1;

	ENF_PROFILE(PROFILE_THREAD_WAITFOR);

	while(true)
	{
		//isn't the handle too old?
		if(!IsValid(spincount))
			return -1;

		if(m_pQueue[index].m_State == JS_FINISHED)
		{
			return m_pQueue[index].m_iResult;
		}
		WaitForSingle(m_EmptyEvent, INFINITE_TIMEOUT);
	}
}

//-----------------------------------------------------------------------------
void CThreadManager::WaitForIdle()
{
	ENF_PROFILE(PROFILE_THREAD_WAITFORIDLE);
	while(true)
	{
		if(IsEmpty())
			return;
		WaitForSingle(m_EmptyEvent, INFINITE_TIMEOUT);
	}
}
}
/*
class MyJob: public enf::CThreadJob
	{
	public:
		int num;
		static int jobcount;

		MyJob(int n): num(n)
		{
			jobcount++;
		}
		~MyJob()
		{
			jobcount--;
		}
		int Run()
		{
			int res = 0;
			while(num--)
			{
				res += sqrtf(rand());
			}
			return res;
		}
	};

int MyJob::jobcount = 0;

#include "enf_winengine.h"
#include "enf_timer.h"

void TestThreadManager()
{
	enf::CThreadManager tm(32, 4096);
	enf::CThreadManager tm2(32, 4096);

	tm.Start(1);
	tm2.Start(2);

	enf::Timer::PushTime();
#define LCOUNT 100000

	for(int c = 0; c < 100; c++)
	{
#if 0
		enf::JobHandle h1 = tm.Submit(ENF_NEW MyJob(LCOUNT));
		enf::JobHandle h2 = tm.Submit(ENF_NEW MyJob(LCOUNT));

//		enf::LogF(enf::LL_NORMAL, "JC: %d ,%x %x %x, %lf", MyJob::jobcount, h1, h2, h3, enf::Timer::GetTime());
//		tm.WaitFor(h2);
//		tm.WaitForIdle();
#endif

#if 1
		enf::TJobHandle h1 = tm.Submit(ENF_NEW MyJob(LCOUNT));
		enf::TJobHandle h2 = tm2.Submit(ENF_NEW MyJob(LCOUNT));

//		enf::LogF(enf::LL_NORMAL, "JC: %d ,%x %x %x, %lf", MyJob::jobcount, h1, h2, h3, enf::Timer::GetTime());
//		tm.WaitFor(h2);
//		tm.WaitForIdle();
#endif

#if 0
		MyJob mj0(LCOUNT);
		mj0.Run();
		MyJob mj1(LCOUNT);
		mj1.Run();
#endif

#if 0
		enf::TJobHandle h1 = tm.Submit(ENF_ NEW MyJob(LCOUNT));
		MyJob mj0(LCOUNT);
		mj0.Run();
#endif
//		enf::LogF(enf::LL_NORMAL, "JC: %d ,%x %x %x, %lf", MyJob::jobcount, h1, h2, h3, enf::Timer::GetTime());
//		tm.WaitFor(h2);
//		tm.WaitForIdle();

	}
	tm.WaitForIdle();
	tm2.WaitForIdle();

	int64 ticks = enf::Timer::PopTick();
	ticks=ticks;
}
*/