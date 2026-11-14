//#include "precomp.h"
#include "system/enf_mutex.h"
#include "system/enf_thread.h"
#ifdef ENF_PS3
	#include <sys/ppu_thread.h>
#endif
#ifdef __MACOSX__
	#include <mach/mach_init.h>
	#include <mach/semaphore.h>
	#include <mach/task.h>
	#include <unistd.h>
#endif
#include "system/enf_memory.h"

namespace enf
{

#if defined(WIN32) || defined(ENF_XENON)

#define ValidHandle(h) ((h) != (void*)ERROR_INVALID_HANDLE && (h) != NULL)

#define MS_VC_EXCEPTION 0x406D1388

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void SetThreadName( DWORD dwThreadID, const char* threadName)
{
	::Sleep(10);
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = (LPSTR)threadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try
	{
		::RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
}
#endif

//-----------------------------------------------------------------------------
CSynchronize::CSynchronize()
#ifndef __MACOSX__
: m_hHandle(NULL)
#endif
{
	int a = 0;
}

//-----------------------------------------------------------------------------
CSynchronize::~CSynchronize()
{
#if defined(WIN32) || defined(ENF_XENON)
	if(m_hHandle)
	{
		::CloseHandle((HANDLE)m_hHandle);
		m_hHandle = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
CSemaphore::CSemaphore(uint initialCount, uint maximumCount)
{
	int a = 0;
#if defined(WIN32) || defined(ENF_XENON)
	m_hHandle = (void*)::CreateSemaphore(NULL, initialCount, maximumCount, NULL);
	enf_assert(m_hHandle != NULL);
#elif defined(ENF_PS3)
	sys_semaphore_attribute_t attr;
	sys_semaphore_attribute_initialize(attr);
//	attr.attr_protocol = SYS_SYNC_PRIORITY;
	int result = sys_semaphore_create((sys_semaphore_t*)&m_hHandle, &attr, initialCount, maximumCount);
	enf_assert(result == CELL_OK);
	
#elif defined(__MACOSX__)
	task_t task = mach_task_self();
//	sync_policy_t policy;
	kern_return_t ret = semaphore_create(task, &m_hHandle, 0, 0);
	
	enf_assert(ret == 0);
#endif
}
//-----------------------------------------------------------------------------
CSemaphore::~CSemaphore()
{
#if defined(ENF_PS3)
	if(m_hHandle)
	{
		sys_semaphore_destroy((sys_semaphore_t)m_hHandle);
		m_hHandle = NULL;
	}
	
#elif defined(__MACOSX__)
	if(m_hHandle)
	{
		semaphore_destroy(mach_task_self(), m_hHandle);
		m_hHandle = 0;
	}
	#endif
}

//-----------------------------------------------------------------------------
uint CSemaphore::Release(uint releaseCount)
{
#if defined(WIN32) || defined(ENF_XENON)
	LONG prev=0;
	BOOL res = ::ReleaseSemaphore(m_hHandle, releaseCount, &prev);
	if(res == 0)
	{
		DWORD err = ::GetLastError();
		err=err;
	}
#elif defined(ENF_PS3)

	sys_semaphore_value_t prev;
	int result = sys_semaphore_get_value((sys_semaphore_t)m_hHandle, &prev);
	enf_assert(result == CELL_OK);

	result = sys_semaphore_post((sys_semaphore_t)m_hHandle, (sys_semaphore_value_t)releaseCount);
	enf_assert(result == CELL_OK);
#elif defined(__APPLE__)
	int ret = semaphore_signal(m_hHandle);
	enf_assert(ret == 0);
	int prev = 0;
#endif
	return prev;
}

//-----------------------------------------------------------------------------
CEvent::CEvent(bool manualreset, bool initialState)
{
#if defined(WIN32) || defined(ENF_XENON)
	m_hHandle = (void*)::CreateEvent(NULL, manualreset, initialState, NULL);
	enf_assert(m_hHandle != NULL);
#elif defined(ENF_PS3)
	sys_event_flag_attribute_t attr;
	sys_event_flag_attribute_initialize(attr);
	int result = sys_event_flag_create((sys_event_flag_t*)&m_hHandle, &attr, initialState);
	enf_assert(result == CELL_OK);
#elif defined(__MACOSX__)
	pthread_mutexattr_t mattr;
	pthread_mutexattr_init(&mattr);
	pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
	int ret = pthread_mutex_init(&m_hMutexHandle, &mattr);
	enf_assert(ret == 0);
	ret = pthread_cond_init(&m_hCondHandle, NULL);
	enf_assert(ret == 0);
	m_bReady = true;
#endif
}

//-----------------------------------------------------------------------------
CEvent::~CEvent()
{
#if defined(ENF_PS3)
	if(m_hHandle)
	{
		int result = sys_event_flag_destroy((sys_event_flag_t)m_hHandle);
		enf_assert(result == CELL_OK);
		m_hHandle = NULL;
	}
#elif defined(__MACOSX__)
	pthread_mutex_destroy(&m_hMutexHandle);
	pthread_cond_destroy(&m_hCondHandle);
#endif
}

//-----------------------------------------------------------------------------
bool CEvent::IsSet() const
{
#if defined(WIN32) || defined(ENF_XENON)
	switch(::WaitForSingleObject(m_hHandle, 0))
	{
	case WAIT_TIMEOUT:
		return false;
	case WAIT_OBJECT_0:
		return true;
	}
#elif defined(ENF_PS3)

	uint64_t flags;
	int result = sys_event_flag_get((sys_event_flag_t)m_hHandle, &flags);
	enf_assert(result == CELL_OK);
	return (flags & 1) != 0;
#endif

	//WTF?
	return false;
}

//-----------------------------------------------------------------------------
void CEvent::Set()
{
#if defined(WIN32) || defined(ENF_XENON)
	::SetEvent(m_hHandle);
#elif defined(ENF_PS3)
	sys_event_flag_set((sys_event_flag_t)m_hHandle, 1);
#elif defined(__MACOSX__)
	pthread_mutex_lock(&m_hMutexHandle);
	m_bReady = true;
	pthread_cond_signal(&m_hCondHandle);
	pthread_mutex_unlock(&m_hMutexHandle);
#endif
}

//-----------------------------------------------------------------------------
void CEvent::Reset()
{
#if defined(WIN32) || defined(ENF_XENON)
	::ResetEvent(m_hHandle);
#elif defined(ENF_PS3)
	sys_event_flag_clear((sys_event_flag_t)m_hHandle, 1);
#endif
}

//-----------------------------------------------------------------------------
CThread::CThread(uint stackSize): m_Status(STOPPED),
	m_iStackSize(stackSize)
	#if defined(WIN32) || defined(ENF_XENON)
		, m_hThread(NULL), m_iThreadID(0)
	#elif defined(ENF_PS3)
		, m_hHandle(NULL)
	#elif defined(__MACOSX__)
		, m_hHandle(NULL)
	#endif
{
}

//-----------------------------------------------------------------------------
void CThread::Sleep(uint milliseconds)
{
	#if defined(WIN32) || defined(ENF_XENON)
		::Sleep(milliseconds);
	#elif defined(ENF_PS3)
		sys_timer_usleep(milliseconds * 1000);
	#elif defined(__MACOSX__)
		usleep(milliseconds * 1000);
	#endif
}

//-----------------------------------------------------------------------------
uint CThread::WaitForSingle(CSynchronize& event, uint timeout)
{
	#if defined(WIN32) || defined(ENF_XENON)
		enf_assert(event.m_hHandle != (void*)ERROR_INVALID_HANDLE);
		DWORD res = ::WaitForSingleObject(event.m_hHandle, timeout);
		if(res == WAIT_TIMEOUT)
			return W_TIMEOUT;

		return res;
	#elif defined(ENF_PS3)
		int res = 0;
		if(event.GetType() == SYNC_SEMAPHORE)
		{
			if(timeout == INFINITE_TIMEOUT)
				timeout = SYS_NO_TIMEOUT;
			else if(timeout == 0)
				timeout = 1;
			else
				timeout = timeout * 1000;

			res = sys_semaphore_wait(event.m_hHandle, timeout);
		}
		else if(event.GetType() == SYNC_EVENT)
		{
			uint64_t result;

			if(timeout == INFINITE_TIMEOUT)
				timeout = SYS_NO_TIMEOUT;
			else if(timeout == 0)
			{
				sys_event_flag_get(event.m_hHandle, &result);
				return ((result & 1) != 0) ? 0 : W_TIMEOUT;
			}
			else
				timeout = timeout * 1000;

			res = sys_event_flag_wait(event.m_hHandle, 1, SYS_EVENT_FLAG_WAIT_AND|SYS_EVENT_FLAG_WAIT_CLEAR , &result, timeout);
		}
		if(res == ETIMEDOUT)
			return W_TIMEOUT;
		enf_assert(res == CELL_OK);
		return 0;
	#elif defined(__MACOSX__)
		int res = 0;
		if(event.GetType() == SYNC_SEMAPHORE)
		{
			res = semaphore_wait(static_cast<CSemaphore&>(event).m_hHandle);
		}
		else if(event.GetType() == SYNC_EVENT)
		{
			CEvent& eevent = static_cast<CEvent&>(event);
			pthread_mutex_lock(&eevent.m_hMutexHandle);
			while(!eevent.m_bReady)
			{
				pthread_cond_wait(&eevent.m_hCondHandle, &eevent.m_hMutexHandle);
			}
			eevent.m_bReady = false;
			pthread_mutex_unlock(&eevent.m_hMutexHandle);
		}
		enf_assert(res == 0);
		return 0;

	#endif
}

//-----------------------------------------------------------------------------
uint _InternalStartThread(void* lpThreadParameter)
{
	CThread *thread = static_cast<CThread*>(lpThreadParameter);

	IL_EXCHANGE(thread->m_Status, CThread::RUNNING);

	thread->Run();
	thread->Ended();

	return 0;
}

//-----------------------------------------------------------------------------
#if defined(WIN32) || defined(ENF_XENON)
DWORD WINAPI _StartThread(LPVOID lpThreadParameter)
{
	return _InternalStartThread(lpThreadParameter);
}

#elif defined(ENF_PS3)

void _StartThread(uint64_t pThreadParameter)
{
	_InternalStartThread((void*)pThreadParameter);
}

#elif defined(__MACOSX__)
void* _StartThread(void* pThreadParameter)
{
	_InternalStartThread(pThreadParameter);
	return NULL;
}
#endif

//-----------------------------------------------------------------------------
bool CThread::Start(uint core, const char* name, int priority)
{
	#ifdef ENF_DEVELOPER
		m_sThreadName = name;
	#endif

	enf_assert(IL_READ(m_Status) < RUNNING);

	#if defined(WIN32) || defined(ENF_XENON)
		DWORD tid;

		m_hThread = ::CreateThread(NULL, m_iStackSize, &_StartThread, this, CREATE_SUSPENDED, &tid);
	//	::SetThreadPriority(m_hThread, THREAD_PRIORITY_ABOVE_NORMAL);
		::SetThreadPriority(m_hThread, THREAD_PRIORITY_NORMAL);

		m_iThreadID = tid;

		if(name)
			SetThreadName(tid, name);
		if(m_hThread != NULL)
		{
			#ifdef ENF_XENON
				static const core2core[] = {0, 2, 4, 1, 3, 5};

				enf_assert(core < 6);
				XSetThreadProcessor(m_hThread, core2core[core]);
			#endif

			::ResumeThread(m_hThread);
			return true;
		}

		return false;

	#elif defined(ENF_PS3)

		int result = sys_ppu_thread_create(&m_hHandle, &_StartThread, (uint64_t)this, priority, m_iStackSize, SYS_PPU_THREAD_CREATE_JOINABLE, name);
		enf_assert(result == CELL_OK);
	
		return true;
	
	#elif defined(__MACOSX__)
		pthread_attr_t attr;

		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, m_iStackSize);
		int ret = pthread_create(&m_hHandle, &attr, &_StartThread, (void*)this);
	return true;
	#endif
}


//-----------------------------------------------------------------------------
void CThread::Ended()
{
#if defined(WIN32) || defined(ENF_XENON)
	if(ValidHandle(m_hThread))
	{
		::CloseHandle(m_hThread);
		m_hThread = NULL;
	}
#elif defined(ENF_PS3)
	Exit(0);
#endif

	IL_EXCHANGE(m_Status, STOPPED);
}

//-----------------------------------------------------------------------------
void CThread::Exit(int exitcode)
{
	IL_EXCHANGE(m_Status, STOPPED);

	#if defined(WIN32) || defined(ENF_XENON)
		::ExitThread(exitcode);
	#elif defined(ENF_PS3)
		sys_ppu_thread_exit(exitcode);
	#elif defined(__MACOSX__)
		pthread_exit((void*)		exitcode);
#endif
}

//-----------------------------------------------------------------------------
CThread::~CThread()
{
	enf_assert(m_Status == STOPPED);
	#if defined(WIN32) || defined(ENF_XENON)
		enf_assert(m_hThread == NULL);
	#elif defined(__MACOSX__)
		enf_assert(m_hHandle == NULL);
	#elif defined(ENF_PS3)
		enf_assert(m_hHandle == NULL);
	#endif

	while(true)
	{
		if(m_Status == STOPPED)
			break;
	}
/*
	#if defined(WIN32) || defined(ENF_XENON)
		if(ValidHandle(m_hThread))
		{
			::CloseHandle(m_hThread);
			m_hThread = NULL;
		}
	#elif defined(__MACOSX__)
	if(m_hHandle)
		pthread_detach(m_hHandle);
	#elif defined(ENF_PS3)
		WaitForExit();
	#endif
*/
}

//-----------------------------------------------------------------------------
void CThread::WaitForExit() const
{
#if defined(WIN32) || defined(ENF_XENON)
	if( ValidHandle(m_hThread) )
	{
		// just wait
		::WaitForSingleObject( (HANDLE)m_hThread, INFINITE );
	}
#elif defined(ENF_PS3)
	uint64_t exitCode;
	if(m_hHandle)
	{
		int ret = sys_ppu_thread_join(m_hHandle, &exitCode);
		enf_assert(ret == CELL_OK);
		m_hHandle = 0;
	}
#elif defined(__MACOSX__)
	if(m_hHandle)
	{
		pthread_join(m_hHandle);
		m_hHandle = 0;
	}
#endif

}

//-----------------------------------------------------------------------------
#if defined(WIN32) || defined(ENF_XENON)
	DWORD g_MainThreadId = 0;
#elif defined(ENF_PS3)
	sys_ppu_thread_t g_MainThread = 0;
#elif defined(__MACOSX__)
	pthread_t g_MainThread = 0;
#endif

//-----------------------------------------------------------------------------
void InitMainThread()
{
	#if defined(WIN32) || defined(ENF_XENON)
		g_MainThreadId = ::GetCurrentThreadId();
	#elif defined(ENF_PS3)
		sys_ppu_thread_get_id(&g_MainThread);
	#elif defined(__MACOSX__)
		g_MainThread = pthread_self();
	#endif
}

//-----------------------------------------------------------------------------
bool CheckMainThread()
{
	#if defined(WIN32) || defined(ENF_XENON)
		return g_MainThreadId == ::GetCurrentThreadId();
	#elif defined(ENF_PS3)
		sys_ppu_thread_t currID;
		sys_ppu_thread_get_id(&currID);
	
		return currID == g_MainThread;
	#elif defined(__MACOSX__)
		return pthread_equal(g_MainThread, pthread_self());
	#else
		return true;
	#endif
}

//-----------------------------------------------------------------------------
}