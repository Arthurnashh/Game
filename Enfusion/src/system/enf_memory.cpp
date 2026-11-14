//-----------------------------------------------------------------------------
// File: enf_memory.cpp
//
// Desc: Memory allocator
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "common/enf_types.h"
#include "common/enf_strings.h"
#include "system/enf_memory.h"
#include "system/enf_thread.h"

#include <stdio.h>

#ifdef ENF_BUILD
	#include "resource/enf_iresourcemanager.h"
#endif

#ifdef WIN32
	//#define WIN32_LEAN_AND_MEAN
  #ifndef NOMINMAX
	  #define NOMINMAX
  #endif
	#include <windows.h>
	//#include <Ntifs.h>

#elif defined(ENF_XENON)
	#include <Xtl.h>
#elif defined(ENF_PS3)
	#include "common/enf_memoryPs3.h"
#endif

#include "enf_main.h"

//#define ENF_MEMORYVALIDATE
#define ENF_ALLOC_PATTERN 0x7fcdf00d //0xbaadf00d
#define ENF_FREE_PATTERN 0xfeeefeee

//don't allow this option on XBOX release builds 
#if !defined(ENF_CONSOLE) || defined(_DEBUG)
	#define ENF_HEAPVALIDATE
#endif

#ifdef ENF_LEAK_DETECTOR
	#include "common/enf_memoryreport.h"
	#define DEBUG

	#ifdef WIN32
//	#ifdef ENF_ALLOC_STACKTRACE
		#include "Dbghelp.h"
	#endif
#endif

namespace enf
{
#define ENF_BEGIN_GUARD 0xfeedf1d0
#define ENF_END_GUARD 0xdeadf1d0

ebool g_MemoryValidate = false;

void EnableMemoryValidation() { g_MemoryValidate = true; }
void DisableMemoryValidation() { g_MemoryValidate = false; }

IL_INT32 MemStats::m_iAnims = 0;
IL_INT32 MemStats::m_iTotalIndices = 0;
IL_INT32 MemStats::m_iTotalVertices = 0;
IL_INT32 MemStats::m_iObjectIndices = 0;
IL_INT32 MemStats::m_iObjectVertices = 0;
IL_INT32 MemStats::m_iRenderTargets = 0;
IL_INT32 MemStats::m_iTextures = 0;
IL_INT32 MemStats::m_iDisposedTextures = 0;
IL_INT32 MemStats::m_iLevelData = 0;
IL_INT32 MemStats::m_iParticles = 0;
IL_INT32 MemStats::m_iObjects = 0;
IL_INT32 MemStats::m_iSoundSystem = 0;

FlushableObject* MemoryManager::m_pFlushableObjects = NULL;
IL_INT32	MemoryManager::m_iTotalAllocatedBytes = 0;
IL_INT32	MemoryManager::m_iTotalAllocations = 0;
IL_INT32	MemoryManager::m_iTotalPhysicalAllocatedBytes = 0;
IL_INT32	MemoryManager::m_iTotalPhysicalAllocations = 0;
bool		MemoryManager::m_bInitialized = false;
int		MemoryManager::m_iFlushRecursion = 0;
uint		MemoryManager::m_iAllocationLimit = 64;
IL_INT32	MemoryManager::m_iAllocationID = 1;
uint		MemoryManager::m_iBreakOnAllocationID =  0;//0x389bd;//0x3;

SCriticalSectionStandIn			MemoryManager::m_Mutex;
MemoryManager::MemoryChunk*	MemoryManager::m_pChunks = NULL;

#ifdef ENF_PS3
	sys_memory_info_t MemoryManager::m_StartMemory;

	/*!
	memory accessible for RSX in main memory - needed
	~1MB	- shaders
	~2MB	- terrain shared static height map for 2049x2049
	~3MB	- terrain shared index buffer block cache
	---------------
	7MB (must be aligned to megabytes)
	*/
	const uint MemoryManager::PS3MainMemorySize = ENF_MEGABYTE(8);
	
	PS3MainMemoryPool*  MemoryManager::m_pMainMemPool = NULL;
	PS3LocalMemoryPool* MemoryManager::m_pLocalMemPool = NULL;	
#endif


#ifdef ENF_PS3

//-----------------------------------------------------------------------
OutOfMemCounter::OutOfMemCounter(OutOfMemCounter::OutMemoryType type, uint wantMem, float time): m_iMemType(type), m_fMaxTime(time), m_iWantMem(wantMem)
{
	m_fThisTime = Timer::GetTime();
}

//-----------------------------------------------------------------------
void OutOfMemCounter::Next(int freeMem)
{
	if (freeMem > 0)
	{
		m_fThisTime	= -1.0f;
		return;
	}

	if (m_fThisTime == -1.0f)
	{
		m_fThisTime = Timer::GetTime();	
		return;
	}

	if (Timer::GetTime() - m_fThisTime > m_fMaxTime)
	{
		SString<1024> str;

		SString<> wanted("");
		if (m_iWantMem)
			wanted.format ("(wanted %dKB) ", m_iWantMem >> 10);

		str.format("Ouch, out of %s memory %s, more than %2.0f sec ;-( \n\n", m_iMemType == OutOfMemCounter::MT_VIDEO ? "RSX local" : m_iMemType == OutOfMemCounter::MT_PHYSICAL ? "RSX main" :  m_iMemType == OutOfMemCounter::MT_RESOURCE ? "resource" : "system", wanted.cstr(), m_fMaxTime*0.001f);

		SString<> tmp;
		if (m_iMemType == OutOfMemCounter::MT_GENERAL || m_iMemType == MT_RESOURCE)
		{
			str += tmp.format("Free sysMem %8d Kbytes\n", MemoryManager::GetFreeMemory() >> 10);
		}

		str += tmp.format("* Animations     %8d Kbytes\n", MemStats::m_iAnims / 1024);
		str += tmp.format("* TotalIndices   %8d Kbytes\n", MemStats::m_iTotalIndices / 1024);
		str += tmp.format("* TotalVertices  %8d Kbytes\n", MemStats::m_iTotalVertices / 1024);
		str += tmp.format("* ObjectIndices  %8d Kbytes\n", MemStats::m_iObjectIndices / 1024);
		str += tmp.format("* RenderTargets  %8d Kbytes\n", MemStats::m_iRenderTargets / 1024);
		str += tmp.format("* ObjectVertices %8d Kbytes\n", MemStats::m_iObjectVertices / 1024);
		str += tmp.format("* Textures       %8d Kbytes\n", MemStats::m_iTextures / 1024);
		str += tmp.format("* LevelData      %8d Kbytes\n", MemStats::m_iLevelData / 1024);
		str += tmp.format("* Particles      %8d Kbytes\n", MemStats::m_iParticles / 1024);
		str += tmp.format("* Objects        %8d Kbytes\n",   MemStats::m_iObjects / 1024);

		GetEngine()->ShowMessageBoxUI("", str.cstr(), 0, NULL, 0, enf::MBB_OK);

		m_fThisTime	= -1.0f;
	}
}

#endif


//-----------------------------------------------------------------------
struct MemoryManager::GuardBand
{	
	uint Pattern[2];
	
	inline void Init(bool start)
	{
		Pattern[0] = Pattern[1] = start ? ENF_BEGIN_GUARD : ENF_END_GUARD;
	}
	inline void Check(uint pattern) const
	{
		enf_rlsassert2(Pattern[0] == pattern && Pattern[1] == pattern, "Memory is overwritten before/past allocated block");
	}

	inline void DeInit(bool start)
	{
		Check(start ? ENF_BEGIN_GUARD : ENF_END_GUARD);
		Pattern[0] = Pattern[1] = 0;
	}
};

//-----------------------------------------------------------------------
struct MemoryManager::MemoryChunk
{
	const char*		File;
	int				Line;
	MemoryChunk*	Next;
	MemoryChunk**	Prev;
	size_t			Size;
	uint				ID;
#ifdef ENF_ALLOC_STACKTRACE
	intptr_t			Stack[MAX_STACK_TRACE];
#endif
	GuardBand		Guard;
};

//-----------------------------------------------------------------------
MemoryManager::MemoryManager()
{
	m_iTotalAllocatedBytes = 0;
	m_iTotalAllocations = 0;
	m_iTotalPhysicalAllocatedBytes = 0;
	m_iTotalPhysicalAllocations = 0;
}

#ifdef ENF_PS3
	#define MUTEX_LOCK() {Init(); sys_mutex_lock(m_Mutex, 0);}
	#define MUTEX_UNLOCK() sys_mutex_unlock(m_Mutex)
#elif defined(WIN32) || defined(ENF_XENON)
	#define MUTEX_LOCK() {Init(); EnterCriticalSection((LPCRITICAL_SECTION)&m_Mutex);}
	#define MUTEX_UNLOCK() LeaveCriticalSection((LPCRITICAL_SECTION)&m_Mutex)
#elif defined(__APPLE__)
	#define MUTEX_LOCK() {Init(); pthread_mutex_lock(&m_Mutex);}
	#define MUTEX_UNLOCK() pthread_mutex_unlock(&m_Mutex)
#endif

//-----------------------------------------------------------------------
ENF_INLINE void ValidateMemory()
{

#ifdef ENF_HEAPVALIDATE
	#if defined(ENF_XENON) && defined(_DEBUG)
		MEMORYSTATUS mstat;
		::GlobalMemoryStatus(&mstat);
		if(mstat.dwAvailPhys < (1024 * 1024 * 1))
		{
			_OutputDebugString("Low memory\n");
			//enf_assert(!"Low memory");
		}
	#endif

	if(g_MemoryValidate)
		MemoryManager::Validate();
#endif
}



//-----------------------------------------------------------------------
void MemoryManager::Validate()
{
#if defined(WIN32)
	//::HeapValidate(::GetProcessHeap(), 0, NULL);
#endif
}

//-----------------------------------------------------------------------
size_t MemoryManager::GetFreeMemory()
{
#ifdef ENF_XENON
	MEMORYSTATUS stat;
	stat.dwLength = sizeof(stat);
	::GlobalMemoryStatus( &stat );
	
//	return (stat.dwTotalPhys - stat.dwAvailPhys);
	return stat.dwAvailPhys;
#elif defined(WIN32)
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof (statex);

	GlobalMemoryStatusEx (&statex);	//ehm

	__int64 rest = statex.ullAvailPhys - (64 * 1024 * 1024);
	rest = enf_limit(rest, __int64(0), __int64(512 * 1024 * 1024));

	__int64 allocated = m_iTotalAllocatedBytes + MemStats::m_iTextures + MemStats::m_iObjectVertices + MemStats::m_iObjectIndices;
	__int64 maxavail = 1280 * 1024 * 1024;
	__int64 avail = maxavail - allocated;

	avail = enf_limit(avail, __int64(0), __int64(512 * 1024 * 1024));

	if(avail < rest)
		return (size_t) avail;
	else
		return (size_t) rest;
#elif defined(__MACOSX__)
	return ENF_MEGABYTE(32);
#elif defined(ENF_PS3)
	//memory allocated by system memory manager (malloc,free)
	malloc_managed_size mallocSizeFast;
	int ret1 = malloc_stats_fast(&mallocSizeFast);

	//free memory from kernel (sys_memory...), kernel returns just memory of 64KB and 1MB blocks
	sys_memory_info_t memInfo;
	int ret2 = sys_memory_get_user_memory_size(&memInfo);

	enf_assert(ret1|ret2 == 0);
	return ret1|ret2 == 0 ? memInfo.available_user_memory + mallocSizeFast.current_system_size - mallocSizeFast.current_inuse_size : 0;
#endif
}

//-----------------------------------------------------------------------
bool MemoryManager::OutOfMemory(const char* fileName, int line, uint memType, size_t requested)
{
	RefCounter<int> counter(m_iFlushRecursion);
	
	if(m_iFlushRecursion == ENF_CLEANUP_FLUSHLEVEL)
	{
		#ifdef ENF_LEAK_DETECTOR
			Dump("Out of memory!\n", MD_NODATA|MD_NOID);
		#endif
		return false;
	}
	SString<> temp;

	#ifdef ENF_PS3
		//don't bother me with video memory, it's normal on PS3
		if (memType != MT_VIDEO)
			LogF(LL_DEBUG, "%s(%d) Out of memory when requested %d Type: %s %s %s\n", fileName, line, requested,
				((memType & MT_GENERAL) ? "System" : ""),
				((memType & MT_VIDEO) ? "RSX Local" : ""),
				((memType & MT_PHYSICAL) ? "RSX Main" : ""));
	#else
		LogF(LL_DEBUG, "%s(%d) Out of memory when requested %d Type: %s %s %s\n", fileName, line, requested,
			((memType & MT_GENERAL) ? "General" : ""),
			((memType & MT_VIDEO) ? "Video" : ""),
			((memType & MT_PHYSICAL) ? "Physical" : "") );
	#endif

	if (memType & MT_GENERAL)
	{
		LogF(LL_DEBUG, "  Free sysMem    %8d Kbytes\n", GetFreeMemory() >> 10);
	}

	#ifdef ENF_PS3
		if (memType != MT_VIDEO)
	#endif
	{
		LogF(LL_DEBUG, "  Animations     %8d Kbytes\n", MemStats::m_iAnims / 1024);
		LogF(LL_DEBUG, "  TotalIndices   %8d Kbytes\n", MemStats::m_iTotalIndices / 1024);
		LogF(LL_DEBUG, "  TotalVertices  %8d Kbytes\n", MemStats::m_iTotalVertices / 1024);
		LogF(LL_DEBUG, "  ObjectIndices  %8d Kbytes\n", MemStats::m_iObjectIndices / 1024);
		LogF(LL_DEBUG, "  RenderTargets  %8d Kbytes\n", MemStats::m_iRenderTargets / 1024);
		LogF(LL_DEBUG, "  ObjectVertices %8d Kbytes\n", MemStats::m_iObjectVertices / 1024);
		LogF(LL_DEBUG, "  Textures       %8d Kbytes\n", MemStats::m_iTextures / 1024);
		LogF(LL_DEBUG, "  LevelData      %8d Kbytes\n", MemStats::m_iLevelData / 1024);
		LogF(LL_DEBUG, "  Particles      %8d Kbytes\n", MemStats::m_iParticles / 1024);
		LogF(LL_DEBUG, "  Objects        %8d Kbytes\n", MemStats::m_iObjects / 1024);
	}


#ifdef ENF_BUILD
	if(IsResourceManager())
	{
		size_t flushed = GetResources()->FlushCache();
		if(flushed < requested)
			flushed += GetResources()->FreeManagedMemory(requested);

		if(flushed == 0)
		{
			//we are fucked. ResouceManager is locked. Sleep and hope...
			CThread::Sleep(5);
		}
	}
#endif
	Flush(m_iFlushRecursion - 1);

#ifdef WIN32
	//if(m_iFlushRecursion == 1)
		//::HeapCompact(::GetProcessHeap(), 0);
#endif
	return true;
}

//-----------------------------------------------------------------------
bool MemoryManager::InnerOutOfMemory(const char* fileName, int line, uint memoryType, size_t requested)
{
	MUTEX_UNLOCK();
	bool ret = OutOfMemory(fileName, line, memoryType, requested);
	MUTEX_LOCK();

	return ret;
}

//-----------------------------------------------------------------------
void MemoryManager::Init()
{
	if(!m_bInitialized)
	{
#ifdef ENF_PS3
		sys_mutex_attribute_t attr;
		sys_mutex_attribute_initialize(attr);
		int res = sys_mutex_create(&m_Mutex, &attr);
		enf_assert(res == CELL_OK);
#elif defined(__APPLE__)
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		int res = pthread_mutex_init(&m_Mutex, &attr);
		
#elif defined(WIN32) || defined(ENF_XENON)
		InitializeCriticalSection((LPCRITICAL_SECTION)&m_Mutex);
#endif
		m_bInitialized = true;
	}
}

//-----------------------------------------------------------------------
void MemoryManager::Destroy()
{
	Flush(ENF_CLEANUP_FLUSHLEVEL);
}

//-----------------------------------------------------------------------
void MemoryManager::RegisterFlushableObject(FlushableObject* obj)
{
	MUTEX_LOCK();
	obj->SetNextFlushableObject(m_pFlushableObjects);
	obj->SetPreviousFlushableObject(NULL);
	if(m_pFlushableObjects)
		m_pFlushableObjects->SetPreviousFlushableObject(obj);
	m_pFlushableObjects = obj;
	MUTEX_UNLOCK();
}

//-----------------------------------------------------------------------
void MemoryManager::UnregisterFlushableObject(FlushableObject* obj)
{
	MUTEX_LOCK();
	if (obj->GetNextFlushableObject())
	{
		obj->GetNextFlushableObject()->SetPreviousFlushableObject(obj->GetPreviousFlushableObject());
	}
	if (obj->GetPreviousFlushableObject())
	{
		obj->GetPreviousFlushableObject()->SetNextFlushableObject(obj->GetNextFlushableObject());
	}
	else
	{
		m_pFlushableObjects = obj->GetNextFlushableObject();
	}
	MUTEX_UNLOCK();
}

//-----------------------------------------------------------------------
void MemoryManager::Flush(unsigned int level)
{
	if(g_pSmallBlockAllocator)
		g_pSmallBlockAllocator->Flush();

	//not safe to go further
	if(!CheckMainThread())
		return;

	for(FlushableObject* obj = m_pFlushableObjects; obj; obj = obj->GetNextFlushableObject())
	{
		obj->Flush(level);
	}
}

//-----------------------------------------------------------------------
void MemoryManager::PhysicalFree(void* ptr)
{
	enf_assert(ptr != NULL);
	
	#ifdef ENF_XENON
		IL_EXCHANGE_ADD(m_iTotalPhysicalAllocatedBytes, -XPhysicalSize(ptr));
		IL_DECREMENT(m_iTotalPhysicalAllocations);
		XPhysicalFree(ptr);
	#elif defined(ENF_PS3)
			//MUTEX_LOCK();
			int freeSize = (int) m_pMainMemPool->Free(ptr);

			IL_EXCHANGE_ADD(m_iTotalPhysicalAllocatedBytes, -freeSize);
			IL_DECREMENT(m_iTotalPhysicalAllocations);

			//MUTEX_UNLOCK();
	#endif
}

#ifdef ENF_PS3


//-----------------------------------------------------------------------
size_t MemoryManager::GetPhysicalSize()
{
	return m_pMainMemPool->GetFullSize();
}
//-----------------------------------------------------------------------
#ifdef ENF_LEAK_DETECTOR
	void* MemoryManager::LocalAlloc(size_t size, uint alignment, const char* fileName, uint line)
#else
	void* MemoryManager::LocalAlloc(size_t size, uint alignment)
#endif
{
	if (!size)
		return NULL;

	while(true)
	{
		#ifdef ENF_LEAK_DETECTOR
			void* ptr = m_pLocalMemPool->Alloc(size, alignment, NULL, fileName, line);
		#else
			void* ptr = m_pLocalMemPool->Alloc(size, alignment);
		#endif

		if(ptr != NULL)
		{
			return ptr;
		}

		MUTEX_LOCK();
			InnerOutOfMemory(__FILE__, __LINE__, MT_VIDEO, size);
		MUTEX_UNLOCK();
	}
}
//-----------------------------------------------------------------------
void MemoryManager::SetLocalMem()
{
	// get config
	CellGcmConfig config;
	cellGcmGetConfiguration(&config);

	config.localSize = enf_min(config.localSize, ENF_MEGABYTE(249)); 

	// buffer memory allocation
	if (!m_pLocalMemPool)
	{
		m_pLocalMemPool = ENF_NEW PS3LocalMemoryPool(config.localAddress, config.localSize);
		//m_pLocalMemPool = (PS3LocalMemoryPool*) ENF_NEW MemoryPool(config.localAddress, config.localSize, 16, PS3BinSizes);
		//TestMemoryPool(m_pLocalMemPool);
	}
	
	LogF(LL_NORMAL, "RSX local memory size %uMB <0x%x..0x%x>\n", config.localSize >> 20, (uint) config.localAddress, (uint) config.localAddress + config.localSize);
}
//-----------------------------------------------------------------------
void MemoryManager::LocalFree(void* ptr)
{
	enf_assert(ptr != NULL);
	
	//MUTEX_LOCK();
	m_pLocalMemPool->Free(ptr);
	//MUTEX_UNLOCK();
}
//-----------------------------------------------------------------------
uint MemoryManager::LocalOffsetFromPtr(void* ptr)
{
	return m_pLocalMemPool->AdressToOffset(ptr);
}

//-----------------------------------------------------------------------
void MemoryManager::CheckMemoryAddress(void* from, void* to)
{
	if (m_pMainMemPool)
		m_pMainMemPool->SetCheckAddresses((uchar*) from, (uchar*) to);
}

//-----------------------------------------------------------------------
uint MemoryManager::PhysicalOffsetFromPtr(void* ptr)
{
	return m_pMainMemPool->AdressToOffset(ptr);
}

//-----------------------------------------------------------------------
void* MemoryManager::PtrFromLocalOffset(uint offs)
{
	return m_pLocalMemPool->OffsetToAddress(offs);
}

//-----------------------------------------------------------------------
void* MemoryManager::PtrFromPhysicalOffset(uint offs)
{
	return m_pMainMemPool->OffsetToAddress(offs);
}

//-----------------------------------------------------------------------
void* MemoryManager::PhysicalInit(const uint reserveCmdBufSize)
{
	sys_memory_info_t memInfo;
	sys_memory_get_user_memory_size(&memInfo);

	#ifdef ENF_DEVELOPER
		LogF(LL_NORMAL, "Total memory %dMB, available after start %dMB",
			memInfo.total_user_memory >> 20, 
			m_StartMemory.available_user_memory >> 20);
	#endif

	//size must be div 1M, size + cmdBufSize < 256M, this size of main memory is available for RSX
	uint size = PS3MainMemorySize;
	enf_assert(size < memInfo.available_user_memory);
	
	m_StartMemory.available_user_memory -= size;

	sys_addr_t alladdr = NULL;
	int ret = sys_memory_allocate(size, SYS_MEMORY_PAGE_SIZE_1M, &alladdr);

	if (!m_pMainMemPool)
	{
		m_pMainMemPool = ENF_NEW PS3MainMemoryPool((void*) alladdr, size, reserveCmdBufSize);
	}

	return (void*)alladdr;
}

//-----------------------------------------------------------------------
void MemoryManager::PhysicalDestroy()
{
	sys_memory_free((sys_addr_t) m_pMainMemPool->GetCmdBufferAddress());

	SAFE_DELETE(m_pMainMemPool);
	SAFE_DELETE(m_pLocalMemPool);
}

//-----------------------------------------------------------------------
void TestPS3MemoryPoolsIntegrity()
{
	if (MemoryManager::m_pMainMemPool)
	{
		LogF(LL_NORMAL, "-----------------------------");
		LogF(LL_NORMAL, "Main memory pool:");
		MemoryManager::m_pMainMemPool->TestIntegrity(true);
	}

	if (MemoryManager::m_pLocalMemPool)
	{
		LogF(LL_NORMAL, "-----------------------------");
		LogF(LL_NORMAL, "Local memory pool:");
		MemoryManager::m_pLocalMemPool->TestIntegrity(true);
	}
}


#endif
 

//---------------------------------------------------------------------------
void MemoryManager::FreeAligned(void *ptr)
{
	ubyte* aligned = (ubyte*)ptr;
	Free((void*)(aligned - (aligned[-1] << 2)));
}


//-----------------------------------------------------------------------
#ifdef ENF_LEAK_DETECTOR
//-----------------------------------------------------------------------
void* MemoryManager::PhysicalAlloc(size_t size, uint alignment, uint protect, const char *src, int line)
{
	enf_assert(size != 0);
#ifdef ENF_XENON
	if(alignment == 0x10000)
	{
		protect = (protect & ~MEM_16MB_PAGES) | MEM_LARGE_PAGES;
	}
	if(protect == 0)
		protect = PAGE_READWRITE;

	while(true)
	{
		LPVOID ptr = ::XPhysicalAlloc(size, MAXULONG_PTR, alignment, protect);
		if(ptr != NULL)
		{
			IL_EXCHANGE_ADD(m_iTotalPhysicalAllocatedBytes, size);
			IL_INCREMENT(m_iTotalPhysicalAllocations);
			return ptr;
		}
		OutOfMemory(__FILE__, __LINE__, MT_PHYSICAL, size);
	}
#elif defined(ENF_PS3)

	while(true)
	{
		uint realSize = size;
		void* ptr = m_pMainMemPool->Alloc(size, alignment, &realSize, src, line);

		if(ptr != NULL)
		{
			IL_EXCHANGE_ADD(m_iTotalPhysicalAllocatedBytes, realSize);
			IL_INCREMENT(m_iTotalPhysicalAllocations);

			return ptr;
		}
		MUTEX_LOCK();
			InnerOutOfMemory(__FILE__, __LINE__, MT_PHYSICAL, size);
		MUTEX_UNLOCK();
	}
#else
	return Alloc16(size);
#endif
}

//-----------------------------------------------------------------------
void AllocationSet::Dump(unsigned int flags)
{
#ifdef ENF_ALLOC_STACKTRACE

	unsigned long err = InitDbgSymbols();
#endif

	for(uint a = 0; a < GetCardinality(); a++)
	{
		const AllocLocation& loc = (*this)[a];

		SString<256> temp;

		if(flags & MD_NOID)
		{
			if(loc.File)
				LogF(LL_DEBUG, temp.format("%s(%d) %8dx %8d total bytes\n", loc.File, loc.Line, loc.Count, loc.Size));
			else
				LogF(LL_DEBUG, temp.format("UNNAMED %8dx %8d total bytes\n", loc.Count, loc.Size));
		}
		else
		{
			if(loc.File)
				LogF(LL_DEBUG, temp.format("%s(%d) ID %x %8dx %8d total bytes\n", loc.File, loc.Line, loc.ID, loc.Count, loc.Size));
			else
				LogF(LL_DEBUG, temp.format("UNNAMED ID %x %8dx %8d total bytes\n", loc.ID, loc.Count, loc.Size));
		}

#ifdef ENF_ALLOC_STACKTRACE
		if(err == 0 && (flags & MD_NOSTACK) == 0)
		{
			if(loc.Stack[0] != NULL)
				LogF(LL_DEBUG, "Stack trace:\n");

			for(uint s = 0; s < MAX_STACK_TRACE; s++)
			{
				if(loc.Stack[s] == NULL)
					break;

				char temp[1024];

				if(_GetLineInfoFromAddress(loc.Stack[s], temp, true))
					LogF(LL_DEBUG, temp);
			}
		}
#endif
		if((flags & MD_NODATA) == 0)
		{
			char databuff[256] = "Data: <";
			uint p = 7;
			for(uint b = 0; b < enf_min(16U, loc.Size); b++)
			{
				char c = ((const char*)loc.Data)[b];
				if(c < ' ') c = ' ';
				databuff[p++] = c;
			}
			databuff[p++] = '>';
			databuff[p++] = ' ';

			for(uint b = 0; b < enf_min(16U, loc.Size); b++)
			{
				sprintf(databuff + p, "%02x ", (int)((const char*)loc.Data)[b]);
				p += 3;
			}
			databuff[p++] = '\n';
			databuff[p++] = '\n';
			databuff[p++] = 0;
			LogF(LL_DEBUG, databuff);
		}
	}
}

//---------------------------------------------------------------------------
void MemoryManager::Dump(const char* title, unsigned int flags)
{
	MUTEX_LOCK();

	if(title)
		LogF(LL_DEBUG, title);

	//enf_assert(0);
	int total = 0, named = 0;

	AllocationSet chunks;

	for(MemoryChunk *m = m_pChunks; m; m = m->Next)
	{
		m->Guard.Check(ENF_BEGIN_GUARD);
//		((GuardBand*)((char*)m + m->Size))->Check(ENF_END_GUARD);

		total += m->Size;

/*		
		if(m->File && m->Line == 587)
		{
			if(strstr(m->File, "enf_strings.h"))
			{
				LogF(LL_DEBUG, "leaked %s", ((char*)&m[1] + 8));
				m=m;
			}
		}
*/
/*		if(m->File && m->Line == 1053)
		{
			if(strstr(m->File, "enf_scriptcontext.cpp"))
			{
				LogF(LL_DEBUG, "leaked %s", (*(enf::Instance*)((char*)&m[1] + 4)).tempName);
				m=m;
			}
		}
*/
//		if(m->File && m->Size != 0)
		{
			named += m->Size;
			//enf_assert(IsBadStringPtr(m->File, 64) == false);
			
#ifdef ENF_ALLOC_STACKTRACE
			chunks.Insert(m->ID, m->File, m->Line, m->Size, m + 1, m->Stack);
#else
			chunks.Insert(m->ID, m->File, m->Line, m->Size, m + 1);
#endif
		}
	}

	chunks.Dump(flags);

	SString<256> temp;
	LogF(LL_DEBUG, temp.format("Total %dkb  Named %dkb\n", total / 1024, named / 1024));

	#if defined(ENF_PS3)
		if (m_pLocalMemPool)
		{
			LogF(LL_DEBUG, "----------------- Dumping local memory ------------------------\n");
			m_pLocalMemPool->Dump();
		}

		if (m_pMainMemPool)
		{
			LogF(LL_DEBUG, "----------------- Dumping main memory -------------------------\n");
			m_pMainMemPool->Dump();
		}
	#endif

	MUTEX_UNLOCK();
}

//---------------------------------------------------------------------------
void Validate(MemoryManager::MemoryChunk* chunk)
{
	while(chunk)
	{
		chunk->Guard.Check(ENF_BEGIN_GUARD);
		((MemoryManager::GuardBand*)((char*)(chunk + 1) + chunk->Size))->Check(ENF_END_GUARD);

		chunk = chunk->Next;
	}
}


static AllocationSnapshot SystemSnapshot;
static AllocationSnapshot SmallAllocSnapshot;

//---------------------------------------------------------------------------
void AllocationSnapshot::Dump(const char* title, const AllocationSnapshot& snap)
{
	if (title != NULL)
	{
		LogF(LL_DEBUG, "-----------------------------------------");
		LogF(LL_DEBUG, "%s", title);
		LogF(LL_DEBUG, "");
	}

	int change = 0;

	for (uint k = 0; k < snap.GetCardinality(); k++)
	{
		const char* snapFile = snap[k].File;
		int snapLine = snap[k].Line;
		uint snapSize = snap[k].Size;

		int p = 0;
		int r = count - 1;
		int i;

		bool found = false;
		for(;r >= p;)
		{
			i = (p + r) / 2;

			int cmp = (CompareString(data1[i].File, snapFile) * 1000000 + (data1[i].Line - snapLine));

			if(cmp > 0)
			{
				r = i - 1;
			}
			else
			{
				if(cmp == 0)
				{
					if (data1[i].Size != snapSize)
					{
						LogF(LL_DEBUG, "dif: %s (%d), %d -> %d, diff %d", snapFile, snapLine, data1[i].Size, snapSize, snapSize - data1[i].Size);
						change += snapSize - data1[i].Size;
					}

					found = true;
					break;
				}
				p = i + 1;
			}
		}

		if (!found)
		{
			LogF(LL_DEBUG, "new: %s (%d), %d", snapFile, snapLine, snapSize);
			change += snapSize;
		}
	}

	LogF(LL_DEBUG, "Changes to previous snapshot %d", change);
}

//---------------------------------------------------------------------------
void MemoryManager::CreateSystemSnapshot(AllocationSnapshot& snap)
{
	snap.Clear();

	for(MemoryChunk *m = m_pChunks; m; m = m->Next)
	{
		m->Guard.Check(ENF_BEGIN_GUARD);
		snap.Insert(m->File, m->Line, m->Size);
	}
}

//---------------------------------------------------------------------------
void MemoryManager::CreateSnapshot()
{
	while (reinterpret_cast<IResourceManager*>(GetEngine()->GetResourceManager())->FlushCache())
		;

	MUTEX_LOCK();

	CreateSystemSnapshot (SystemSnapshot);
	g_pSmallBlockAllocator->CreateSnapshot(SmallAllocSnapshot);

	MUTEX_UNLOCK();
}


//---------------------------------------------------------------------------
void MemoryManager::CompareSnapshot()
{
	if (SystemSnapshot.GetCardinality() < 1)
		return;

	while (reinterpret_cast<IResourceManager*>(GetEngine()->GetResourceManager())->FlushCache())
		;

	MUTEX_LOCK();

	AllocationSnapshot snap;

	CreateSystemSnapshot(snap);
	SystemSnapshot.Dump("System memory changes...", snap);

	g_pSmallBlockAllocator->CreateSnapshot(snap);	
	SmallAllocSnapshot.Dump("SmallBlockAllocator changes...", snap);

	MUTEX_UNLOCK();

}



//---------------------------------------------------------------------------
#ifdef WIN32
bool PUBLIC_API _GetLineInfoFromAddress(intptr_t ptr, char* buff, bool lf)
{
	DWORD displ;
	char symbuff[sizeof(IMAGEHLP_SYMBOL)+1024];
	IMAGEHLP_SYMBOL* symbol = (IMAGEHLP_SYMBOL *)symbuff;

	IMAGEHLP_LINE line;
	memset(&line, 0, sizeof(IMAGEHLP_LINE));
	line.SizeOfStruct = sizeof(IMAGEHLP_LINE);

	memset(symbol, 0, sizeof(IMAGEHLP_SYMBOL));
	symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
	symbol->MaxNameLength = 1024;

	BOOL ret = SymGetSymFromAddr(::GetCurrentProcess(), ptr, &displ, symbol);
	if (!ret)
	{
		sprintf(buff, "SymGetSymFromAddr:%i, addr:0x%08x\n", GetLastError(), ptr);
	}
	else
	{
		if (SymGetLineFromAddr(::GetCurrentProcess(), ptr, &displ, &line))
		{
			sprintf(buff, "%s(%i)\n", line.FileName, line.LineNumber);
		}
		else
		{
			sprintf(buff, "??? [%s] addr:0x%08x\n", symbol->Name, ptr);
		}
	}
	return true;
}

void FillStackTrace(intptr_t* ptr, int maxSize, int skip)
{
	unsigned long err = InitDbgSymbols();
	if(err != 0)
		return;

/*	USHORT levels = CaptureStackBackTrace(skip, maxSize, (PVOID*)ptr, NULL);
	for (uint n = levels; n < maxSize; n++)
		ptr[n] = 0;

	return;
*/	DWORD _ebp, _eip, _esp;
	__asm {
		push eax
		mov _ebp, ebp
		mov _esp, esp
		call dummy
dummy:
		pop eax
		mov _eip, eax
		pop eax
	}

	STACKFRAME stackFrame;
	memset(&stackFrame, 0, sizeof(stackFrame));
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrPC.Offset = _eip;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Offset = _ebp;
	stackFrame.AddrStack.Mode = AddrModeFlat;
	stackFrame.AddrStack.Offset = _esp;

	HANDLE cprocess = ::GetCurrentProcess();
	HANDLE cthread = ::GetCurrentThread();
	
	//skip N levels + this
	for(int s = 0; s < (skip + 1); s++)
	{
		if(!StackWalk(IMAGE_FILE_MACHINE_I386, cprocess, cthread, &stackFrame, NULL, NULL, NULL, NULL, NULL))
			break;
	}

	int stackptr = 0;
	for (; stackptr < maxSize; stackptr++)
	{
		BOOL ret = StackWalk(IMAGE_FILE_MACHINE_I386, cprocess, cthread, &stackFrame, NULL, NULL, NULL, NULL, NULL);
		if (!ret)
		{
			break;
		}
		ptr[stackptr] = stackFrame.AddrPC.Offset;
	}

	for (; stackptr < maxSize; stackptr++)
		ptr[stackptr] = 0;
}
#endif

//---------------------------------------------------------------------------
uint MemoryManager::GetAllocationID()
{
	uint allocationId = IL_INCREMENT(m_iAllocationID) - 1;
	enf_assert(m_iBreakOnAllocationID != allocationId);
	return allocationId;
}

//---------------------------------------------------------------------------
void *MemoryManager::Alloc(size_t cb, const char* name, int line)
{
	//enf_assert(cb != 0);
	enf_rlsassert2(cb < ENF_MEGABYTE(m_iAllocationLimit), "Requested >AllocationLimit of memory");
	MUTEX_LOCK();

	ValidateMemory();

	#if defined(ENF_PS3) && defined(ENF_DEVELOPER)
		OutOfMemCounter outOfMem(OutOfMemCounter::MT_GENERAL, cb);
	#endif

	MemoryChunk* ptr;
	do 
	{
//#ifdef ENF_CONSOLE
//		if(cb > (34 * 1024 * 1024))
//			enf_assert(!"Pretty huge allocation!");
//#endif
		//Validate(m_pChunks);
#if defined(ENF_XENON) || defined(WIN32)
    ptr = GMemFunctions->New(cb, name, line);
#elif defined(ENF_PS3)
		ptr = (MemoryChunk*)malloc(cb + sizeof(MemoryChunk) + sizeof(GuardBand));
#elif defined(__APPLE__)
		ptr = (MemoryChunk*)malloc(cb + sizeof(MemoryChunk) + sizeof(GuardBand));
#endif

		if(ptr != NULL)
			break;

		#if defined(ENF_PS3) && defined(ENF_DEVELOPER)
			outOfMem.Next();
		#endif
	} while(InnerOutOfMemory(name, line, MT_GENERAL, cb));

	enf_rlsassert(ptr != NULL);

	

	ptr->ID = GetAllocationID();
	ptr->File = name;
	ptr->Line = line;
	ptr->Prev = &m_pChunks;
	ptr->Next = m_pChunks;
	if(m_pChunks)
		m_pChunks->Prev = &ptr->Next;
	m_pChunks = ptr;
	ptr->Size = cb;
#ifdef ENF_ALLOC_STACKTRACE
	FillStackTrace(ptr->Stack, MAX_STACK_TRACE, 1);
#endif
	ptr->Guard.Init(true);
	ptr++;
	((GuardBand*)((char*)ptr + cb))->Init(false);

	FillMemory32(ptr, cb / 4, ENF_ALLOC_PATTERN);


	IL_EXCHANGE_ADD(m_iTotalAllocatedBytes, cb);
	IL_INCREMENT(m_iTotalAllocations);
	MUTEX_UNLOCK();
	//enf_assert(0);

   return ptr;
}

//---------------------------------------------------------------------------
void *MemoryManager::ReAlloc(void* ptr, size_t size, const char* name, int srcLine)
{
	//enf_assert(size != 0);
	if(ptr == NULL)
	{
		return Alloc(size, name, srcLine);
	}

	enf_rlsassert2(size < ENF_MEGABYTE(m_iAllocationLimit), "Requested >AllocationLimit of memory");
	MUTEX_LOCK();

	ValidateMemory();

	MemoryChunk *chunk = (MemoryChunk*)ptr - 1;
	chunk->Guard.DeInit(true);
	((GuardBand*)((char*)ptr + chunk->Size))->DeInit(false);

	if(chunk->Next)
		chunk->Next->Prev = chunk->Prev;
	*chunk->Prev = chunk->Next;

	IL_EXCHANGE_ADD(m_iTotalAllocatedBytes, -chunk->Size);
	IL_DECREMENT(m_iTotalAllocations);

	const char* file = chunk->File;
	int line = chunk->Line;
	uint id = chunk->ID;

	do 
	{
#if defined(ENF_XENON) || defined(WIN32)
		chunk = GMemFunctions->Realloc(chunk, size + sizeof(MemoryChunk) + sizeof(GuardBand), name, srcLine);
#elif defined(ENF_PS3)
		chunk = (MemoryChunk*)realloc(chunk, size + sizeof(MemoryChunk) + sizeof(GuardBand));
#elif defined(__APPLE__)
		chunk = (MemoryChunk*)realloc(chunk, size + sizeof(MemoryChunk) + sizeof(GuardBand));
#else
#endif

		if(ptr != NULL)
			break;
	} while(InnerOutOfMemory(__FILE__, __LINE__, MT_GENERAL, size));
	enf_rlsassert(chunk != NULL);

	chunk->ID = id;
	chunk->File = file;
	chunk->Line = line;
	chunk->Prev = &m_pChunks;
	chunk->Next = m_pChunks;
	if(m_pChunks)
		m_pChunks->Prev = &chunk->Next;
	m_pChunks = chunk;
	chunk->Size = size;
	chunk->Guard.Init(true);
	chunk++;
	((GuardBand*)((char*)chunk + size))->Init(false);

	IL_EXCHANGE_ADD(m_iTotalAllocatedBytes, size);
	IL_INCREMENT(m_iTotalAllocations);
	MUTEX_UNLOCK();

   return chunk;
}

//---------------------------------------------------------------------------
void MemoryManager::Free(void *p)
{
	enf_assert(p);
	MUTEX_LOCK();
	ValidateMemory();

	MemoryChunk *ptr = (MemoryChunk*)p - 1;


static bool showStack = false;

#if defined(WIN32)
	if(showStack)
	{
		LogF(LL_NORMAL, "free %x", (int)ptr);
		intptr_t stack[16];
		FillStackTrace(stack, 8, 1);
		for(uint s = 0; s < 8; s++)
		{
			if(stack[s] == NULL)
				break;

			char temp[1024];

			if(_GetLineInfoFromAddress(stack[s], temp, true))
				LogF(LL_NORMAL, temp);
		}
	}
#endif

	ptr->Guard.DeInit(true);
	((GuardBand*)((char*)p + ptr->Size))->DeInit(false);
	if(ptr->Next)
		ptr->Next->Prev = ptr->Prev;

	*ptr->Prev = ptr->Next;
	IL_EXCHANGE_ADD(m_iTotalAllocatedBytes, -ptr->Size);
	IL_DECREMENT(m_iTotalAllocations);

	FillMemory32(ptr, ptr->Size / 4, ENF_FREE_PATTERN);

#if defined(ENF_XENON) || defined(WIN32)
	GMemFunctions->Delete(p);
#elif defined(ENF_PS3)
	free(ptr);
#elif defined(__APPLE__)
	free(ptr);
#else
#endif
	MUTEX_UNLOCK();
}

//---------------------------------------------------------------------------
void MemoryManager::CheckMemory(void* p)
{
	enf_assert(p);
	MUTEX_LOCK();

	const MemoryChunk *ptr = (MemoryChunk*)p - 1;
	ptr->Guard.Check(ENF_BEGIN_GUARD);
	const GuardBand* band = ((GuardBand*)((char*)p + ptr->Size));
	band->Check(ENF_END_GUARD);
}

//---------------------------------------------------------------------------
size_t MemoryManager::GetSize(void* ptr)
{
	if(ptr == NULL)
		return 0;
	
	MUTEX_LOCK();
	size_t size = (((MemoryChunk*)ptr) - 1)->Size;
	MUTEX_UNLOCK();
	return size;
}

//---------------------------------------------------------------------------
void* MemoryManager::AllocAligned(size_t cb, uint alignment, const char *src, int line)
{
	enf_assert(alignment <= 1024 && (alignment & 3) == 0 && (1 << GetHighestBit(alignment)) == alignment);

	//enf_assert(cb != 0);
	ubyte* ptr = (ubyte*)Alloc(cb + (alignment - 1) + 1, src, line);
	ubyte* aligned = (ubyte*)((intptr_t)(ptr + (alignment - 1) + 1) & ~intptr_t(alignment - 1));
	aligned[-1] = (ubyte)((aligned - ptr) >> 2);

	return aligned;
}


#else

//---------------------------------------------------------------------------
void *MemoryManager::Alloc(size_t cb)
{
	//enf_assert(cb != 0);
	enf_rlsassert2(cb < ENF_MEGABYTE(m_iAllocationLimit), "Requested >AllocationLimit of memory");

	MUTEX_LOCK();

	ValidateMemory();

	void* ptr = NULL;
	size_t size = 0;

	#if defined(ENF_PS3) && defined(ENF_DEVELOPER)
		OutOfMemCounter outOfMem(OutOfMemCounter::MT_GENERAL, cb);
	#endif
	
	do 
	{
#if defined(_DEBUG) || defined(ENF_MEMORYVALIDATE)
		//in debug mode append some guard band
		ptr = GMemFunctions->New(cb + 16);
		*(int*)ptr = ENF_BEGIN_GUARD;
		*(int*)((char*)ptr + 4) = ENF_BEGIN_GUARD;

		ptr = (void*)((char*)ptr + 8);

		FillMemory32(ptr, cb / 4, ENF_ALLOC_PATTERN);

		*(int*)((char*)ptr + cb) = ENF_END_GUARD;
		*(int*)((char*)ptr + cb + 4) = ENF_END_GUARD;

		size = cb + 16;

#else
#if defined(ENF_XENON) || defined(WIN32)
    ptr = GMemFunctions->New(cb);
#elif defined(ENF_PS3)
		ptr = malloc(cb);
#elif defined(__APPLE__)
		ptr = malloc(cb);
#else
#endif
		//if(ptr) FillMemory32(ptr, cb / 4, 0x00000000);
		size = cb;
#endif

		if(ptr != NULL)
			break;

		#if defined(ENF_PS3) && defined(ENF_DEVELOPER)
			outOfMem.Next();
		#endif
	} while(InnerOutOfMemory(__FILE__, __LINE__, MT_GENERAL, cb));


	enf_rlsassert(ptr != NULL);
	IL_EXCHANGE_ADD(m_iTotalAllocatedBytes, size);
	IL_INCREMENT(m_iTotalAllocations);
	MUTEX_UNLOCK();
   return ptr;
}

//---------------------------------------------------------------------------
void *MemoryManager::ReAlloc(void* ptr, size_t size)
{
//	enf_assert(size != 0);
	if(ptr == NULL)
	{
		return Alloc(size);
	}

	enf_rlsassert2(size < ENF_MEGABYTE(m_iAllocationLimit), "Requested >AllocationLimit of memory");
	MUTEX_LOCK();

	ValidateMemory();

#if defined(_DEBUG) || defined(ENF_MEMORYVALIDATE)
	char* cp = ((char*)ptr) - 8;
	enf_rlsassert(*(int*)(cp + 0) == ENF_BEGIN_GUARD);
	enf_rlsassert(*(int*)(cp + 4) == ENF_BEGIN_GUARD);

	size_t cb = GMemFunctions->GetSize(cp);

	IL_EXCHANGE_ADD(m_iTotalAllocatedBytes, -cb);
	cb -= 16;

	enf_rlsassert(*(int*)(cp + 8 + cb) == ENF_END_GUARD);
	enf_rlsassert(*(int*)(cp + 12 + cb) == ENF_END_GUARD);

	cp = (char*)GMemFunctions->Realloc(cp, size + 16);

	//if we allocated bigger block, fill rest of memory with pattern
	if(size > cb)
	{
		FillMemory32(cp + 8 + cb, (size - cb) / 4, ENF_ALLOC_PATTERN);
	}

	*(int*)(cp + 8 + size) = ENF_END_GUARD;
	*(int*)(cp + 12 + size) = ENF_END_GUARD;

	ptr = cp + 8;

	IL_EXCHANGE_ADD(m_iTotalAllocatedBytes, size + 16);

#else
#if defined(ENF_XENON) || defined(WIN32)
	
	IL_EXCHANGE_ADD(m_iTotalAllocatedBytes, -GMemFunctions->GetSize(ptr));
#endif

	do 
	{
#if defined(ENF_XENON) || defined(WIN32)
		ptr = GMemFunctions->Realloc(ptr, size);
#elif defined(ENF_PS3)
		ptr = realloc(ptr, size);
#elif defined(__APPLE__)
		ptr = realloc(ptr, size);
#else
#endif
		//if(ptr) FillMemory32(ptr, size / 4, 0x00000000);
		if(ptr != NULL)
			break;
	} while(InnerOutOfMemory(__FILE__, __LINE__, MT_GENERAL, size));
	enf_rlsassert(ptr != NULL);
	IL_EXCHANGE_ADD(m_iTotalAllocatedBytes, size);
#endif
	MUTEX_UNLOCK();

   return ptr;
}

//---------------------------------------------------------------------------
void MemoryManager::Free(void *p)
{
	MUTEX_LOCK();

	ValidateMemory();

#if defined(_DEBUG) || defined(ENF_MEMORYVALIDATE)
	char* cp = ((char*)p) - 8;
	enf_rlsassert2(*(int*)(cp + 0) == ENF_BEGIN_GUARD, "Memory overwritten before block");
	enf_rlsassert2(*(int*)(cp + 4) == ENF_BEGIN_GUARD, "Memory overwritten before block");

	size_t cb = GMemFunctions->GetSize(cp);
	IL_EXCHANGE_ADD(m_iTotalAllocatedBytes, -cb);

	cb -= 16;

	FillMemory32(cp + 8, cb / 4, ENF_FREE_PATTERN);

	enf_rlsassert2(*(int*)(cp + 8 + cb) == ENF_END_GUARD, "Memory overwritten past block");
	enf_rlsassert2(*(int*)(cp + 12 + cb) == ENF_END_GUARD, "Memory overwritten past block");

	GMemFunctions->Delete(cp);

#else

#if defined(ENF_XENON) || defined(WIN32)
	IL_EXCHANGE_ADD(m_iTotalAllocatedBytes, -GMemFunctions->GetSize(p));
	GMemFunctions->Delete(p);
#elif defined(ENF_PS3)
	free(p);
#elif defined(__APPLE__)
	free(p);
#else
#endif

#endif

	IL_DECREMENT(m_iTotalAllocations);
	MUTEX_UNLOCK();
}

//---------------------------------------------------------------------------
size_t MemoryManager::GetSize(void* ptr)
{
	if(ptr == NULL)
		return 0;


	MUTEX_LOCK();
#if defined(_DEBUG) || defined(ENF_MEMORYVALIDATE)
	size_t size = GMemFunctions->GetSize(((char*)ptr) - 8) - 16;
#elif defined(ENF_XENON) || defined(WIN32)
	size_t size = GMemFunctions->GetSize(ptr);
#else
	size_t size = 0;
#endif
	MUTEX_UNLOCK();

	return size;
}

//---------------------------------------------------------------------------
void MemoryManager::CheckMemory(void* p)
{
	enf_assert(p);
}

//---------------------------------------------------------------------------
void* MemoryManager::AllocAligned(size_t cb, uint alignment)
{
	enf_assert(alignment <= 1024 && (alignment & 3) == 0 && (1 << GetHighestBit(alignment)) == alignment);

	//enf_assert(cb != 0);
	ubyte* ptr = (ubyte*)Alloc(cb + (alignment - 1) + 1);
	ubyte* aligned = (ubyte*)((intptr_t)(ptr + (alignment - 1) + 1) & ~intptr_t(alignment - 1));
	aligned[-1] = (ubyte)((aligned - ptr) >> 2);

	return aligned;
}

//-----------------------------------------------------------------------
void* MemoryManager::PhysicalAlloc(size_t size, uint alignment, uint protect)
{
	enf_assert(size != 0);
#ifdef ENF_XENON
	if(alignment == 0x10000)
	{
		protect = (protect & ~MEM_16MB_PAGES) | MEM_LARGE_PAGES;
	}
	if(protect == 0)
		protect = PAGE_READWRITE;
	while(true)
	{
		LPVOID ptr = ::XPhysicalAlloc(size, MAXULONG_PTR, alignment, protect);
		if(ptr != NULL)
		{
			IL_EXCHANGE_ADD(m_iTotalPhysicalAllocatedBytes, size);
			IL_INCREMENT(m_iTotalPhysicalAllocations);
			return ptr;
		}
		OutOfMemory(__FILE__, __LINE__, MT_PHYSICAL, size);
	}
#elif defined(ENF_PS3)
	while(true)
	{
		uint realSize = size;

		void* ptr = m_pMainMemPool->Alloc(size, alignment, &realSize);

		if(ptr != NULL)
		{
			IL_EXCHANGE_ADD(m_iTotalPhysicalAllocatedBytes, realSize);
			IL_INCREMENT(m_iTotalPhysicalAllocations);
			
			return ptr;
		}

		MUTEX_LOCK();
			InnerOutOfMemory(__FILE__, __LINE__, MT_PHYSICAL, size);
		MUTEX_UNLOCK();
	}
#else
	return Alloc16(size);
#endif
}
#endif
}

#ifdef __GCC__
	#pragma GCC visibility push(default)
#endif

//must not be in namespace!!!
#if !defined(ENF_STATIC) || defined(ENF_XENON) || defined(ENF_PS3) || defined(__APPLE__)
//---------------------------------------------------------------------------
void* operator new[](NEW_SIZE_T cb) THROW_NEW
{
	if(cb == 0)
		cb=cb;
	return enf::MemoryManager::Alloc(cb, __FILE__, __LINE__);
}

//---------------------------------------------------------------------------
void* operator new(NEW_SIZE_T cb) THROW_NEW
{
	if(cb == 0)
		cb=cb;
	return enf::MemoryManager::Alloc(cb, __FILE__, __LINE__);
}

//---------------------------------------------------------------------------
void *operator new(NEW_SIZE_T count, size_t align) THROW_NEW
{
	return enf::MemoryManager::Alloc(count, __FILE__, __LINE__);
}

//---------------------------------------------------------------------------
void operator delete[](void *p) THROW_DELETE
{
	if(p == NULL)
		return;

	enf::MemoryManager::Free(p);
}

//---------------------------------------------------------------------------
void operator delete(void *p) THROW_DELETE
{
	if(p == NULL)
		return;

	enf::MemoryManager::Free(p);
}
#endif

#ifdef ENF_LEAK_DETECTOR
//---------------------------------------------------------------------------
void* operator new[](NEW_SIZE_T cb, const char* file, int line) THROW_NEW
{
	if(cb == 0)
		cb=cb;
	return enf::MemoryManager::Alloc(cb, file, line);
}

//---------------------------------------------------------------------------
void* operator new(NEW_SIZE_T cb, const char* file, int line) THROW_NEW
{
	if(cb == 0)
		cb=cb;
	return enf::MemoryManager::Alloc(cb, file, line);
}

//---------------------------------------------------------------------------
void operator delete[](void *p, const char* file, int line) THROW_DELETE
{
	if(p == NULL)
		return;

	enf::MemoryManager::Free(p);
}

//---------------------------------------------------------------------------
void operator delete(void *p, const char* file, int line) THROW_DELETE
{
	if(p == NULL)
		return;

	enf::MemoryManager::Free(p);
}

#endif

#ifdef __GCC__
	#pragma GCC visibility pop
#endif
