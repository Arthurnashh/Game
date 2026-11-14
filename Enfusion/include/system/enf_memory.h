//-----------------------------------------------------------------------------
// File: enf_memory.h
//
// Desc: Memory allocator
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------
#pragma once
#ifndef ENF_MEMORY_H
#define ENF_MEMORY_H

#include "enf_config.h"
#include "common/enf_inttypes.h"
#include "Es/Memory/memAlloc.hpp"
#include "Es/Memory/normalNew.hpp"

#ifdef ENF_PS3
	#include <sys/memory.h>
	#include <string.h>
	#include <alloca.h>
	#include <new>
#elif defined(ENF_XENON) || defined(WIN32)
	#include <malloc.h>
	#include <new.h>
	#include <memory.h>
	#include <string.h>
#elif defined(__APPLE__)
	#include <string.h>
	#include <alloca.h>
	#include <new>
#endif

#include "enf_mutex.h"

#if defined(__APPLE__) && defined(__GCC__)
	#define THROW_NEW throw (std::bad_alloc)
	#define THROW_DELETE throw()
	#define NEW_SIZE_T std::size_t
#else
	#ifdef WIN32
		#define THROW_NEW throw(...) //_THROW1(std::bad_alloc)
	#else
		#define THROW_NEW
	#endif
	#define THROW_DELETE
	#define NEW_SIZE_T size_t
#endif


/*!
renaming from Enforce ALIGN16 because it collided with SimulWeather/Math/Align.h
*/
#ifdef __GCC__
	#define ENF_ALIGN16(a) a __attribute__ ((aligned (16)))
#else
	#define ENF_ALIGN16(a) __declspec( align(16) ) a
#endif


namespace enf
{

/** @addtogroup Common
@{*/
class FlushableObject;


void PUBLIC_API EnableMemoryValidation();
void PUBLIC_API DisableMemoryValidation();

enum MemoryType
{
	MT_GENERAL	= BITF(0),
	MT_VIDEO		= BITF(1),
	MT_PHYSICAL	= BITF(2)
};

enum MemoryDumpFlags
{
	MD_NODATA	= BITF(0),
	MD_NOID		= BITF(1),
	MD_NOSTACK	= BITF(2),
};

#define SAFE_DESTROY(p)  { if(p) { (p)->Destroy();     (p)=NULL; } }
#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
template<class T> void ENF_INLINEFUNC DeleteArrayPtr(T*& ptr)
{
	delete[] ptr;
	ptr = NULL;
}
template<class T> void ENF_INLINEFUNC DeletePtr(T*& ptr)
{
	delete ptr;
	ptr = NULL;
}
template<class T> void ENF_INLINEFUNC SafeDeleteArrayPtr(T*& ptr)
{
	if(ptr != NULL)
	{
		delete[] ptr;
		ptr = NULL;
	}
}
template<class T> void ENF_INLINEFUNC SafeDeletePtr(T*& ptr)
{
	if(ptr != NULL)
	{
		delete ptr;
		ptr = NULL;
	}
}

//#ifdef __GCC__
//	#define PLACEMENT_NEW(a) new ((int)(a))
//#else
	#define PLACEMENT_NEW(a) new ((void*)(a))
//#endif
ENF_INLINE size_t ENF_MEGABYTE(int n)
{
	return n * 1048576;
}
ENF_INLINE size_t ENF_KILOBYTE(int n)
{
	return n * 1024;
}
template<class T> ENF_INLINEFUNC T* NewObjects(T* ptr, int count)
{
	for(int n = 0; n < count; n++)
		PLACEMENT_NEW(&ptr[n]) T;
	return ptr;
}

template<class T> ENF_INLINEFUNC T* DeleteObjects(T* ptr, int count)
{
	for(int n = 0; n < count; n++)
		ptr[n].~T();
	return ptr;
}

ENF_INLINEFUNC void* Align16(void* ptr)
{
	intptr_t iptr = (intptr_t)ptr;
	const intptr_t mask = (intptr_t)~ULONG64(0x0f);
	iptr = (iptr + 15) & mask;
	return (void*)iptr;
}

#ifdef __GCC__
	#define ENF_ALIGN16(a) a __attribute__ ((aligned (16)))
	#define ALLOCA(type, size) (type*)alloca(sizeof(type) * (size))
	#define ALLOCAINIT(type, size) enf::NewObjects((type*)alloca(sizeof(type) * (size)), (size))
	#define ALLOCA16(type, size) (type*)Align16(alloca(sizeof(type) * (size) + 16))
#else
	#define ENF_ALIGN16(a) __declspec( align(16) ) a
	#define ALLOCA(type, size) (type*)_alloca(sizeof(type) * (size))
	#define ALLOCAINIT(type, size) enf::NewObjects((type*)_alloca(sizeof(type) * (size)), (size))
	#define ALLOCA16(type, size) (type*)Align16(_alloca(sizeof(type) * (size) + 16))
#endif


struct MemStats
{
	static IL_INT32	m_iAnims;
	static IL_INT32	m_iTextures;
	static IL_INT32	m_iDisposedTextures;
	static IL_INT32	m_iRenderTargets;
	static IL_INT32	m_iTotalVertices;
	static IL_INT32	m_iTotalIndices;
	static IL_INT32	m_iObjectVertices;
	static IL_INT32	m_iObjectIndices;
	static IL_INT32	m_iParticles;
	static IL_INT32	m_iObjects;
	static IL_INT32	m_iSoundSystem;
	static IL_INT32	m_iLevelData;
};


inline void FillMemory32(void *mptr, int num, int data)
{
	for(int n = 0; n < num; n++)
		((int*)mptr)[n] = data;
}

//-----------------------------------------------------------------------------
inline void CopyMem(void *dest, const void *src, int len)
{
	memcpy(dest, src, len);
}

//-----------------------------------------------------------------------------
inline void CopyMem32(void *dest, const void *src, int len)
{
	memcpy(dest, src, len * 4);
}

//-----------------------------------------------------------------------------
inline void FillMem(void *dest, int len, int data)
{
	memset(dest, data, len);
}

//-----------------------------------------------------------------------------
inline void ZeroMem(void *dest, int len)
{
	memset(dest, 0, len);
}
//-----------------------------------------------------------------------------
inline void ZeroMem32(void *dest, int len)
{
	memset(dest, 0, len * 4);
}

//-----------------------------------------------------------------------------
inline void CopyMemOverlapped(void *dest, const void* src, int len)
{
	memmove(dest, src, len);
}

#ifdef ENF_PS3
	class PS3MainMemoryPool;
	class PS3LocalMemoryPool;
#endif


#ifdef ENF_LEAK_DETECTOR
	class AllocationSnapshot;
#endif


#ifdef ENF_PS3

//-----------------------------------------------------------------------
class OutOfMemCounter
{
public:
	enum OutMemoryType
	{
		MT_GENERAL,
		MT_VIDEO,
		MT_PHYSICAL,
		MT_RESOURCE
	};

	OutOfMemCounter(OutMemoryType type, uint wantMem, float time = 8000.0f);
	void Next(int freeMem = -1);

private:
	float m_fThisTime;
	float m_fMaxTime;
	int m_iWantMem;
	OutMemoryType m_iMemType;
};

#endif

//-----------------------------------------------------------------------
/*!
General memory manager. ATM it's wrapper over heap allocator
with added leak detector. All Enforce objects has to allocate
through this. Either by calling Alloc/Free methods or by
macro ENF_DECLARE_MMOBJECT in their class body.
*/
//-----------------------------------------------------------------------
class PUBLIC_API MemoryManager
{
public:
	struct MemoryChunk;
	struct GuardBand;

private:
	static IL_INT32	m_iTotalAllocatedBytes;
	static IL_INT32	m_iTotalAllocations;
	static IL_INT32	m_iTotalPhysicalAllocatedBytes;
	static IL_INT32	m_iTotalPhysicalAllocations;
	static IL_INT32	m_iAllocationID;
	static uint m_iBreakOnAllocationID;
	static uint m_iAllocationLimit;
	static SCriticalSectionStandIn m_Mutex;
	static bool m_bInitialized;
	static int m_iFlushRecursion;

	static MemoryChunk* m_pChunks;
	static FlushableObject* m_pFlushableObjects;
#ifdef ENF_PS3
public:
	static PS3MainMemoryPool* m_pMainMemPool;
	static PS3LocalMemoryPool* m_pLocalMemPool;

	static const uint PS3MainMemorySize;
	static sys_memory_info_t m_StartMemory;
#endif

	static bool InnerOutOfMemory(const char* fileName, int line, uint memoryType, size_t requested);

public:
	static bool OutOfMemory(const char* fileName, int line, uint memoryType, size_t requested);

	static size_t GetFreeMemory();
	
	//!changes "suspicious allocation" limit. Returns previous limit
	static uint SetAllocationLimit(uint megs)
	{
		uint prev = m_iAllocationLimit;
		m_iAllocationLimit = megs;
		return prev;
	}

	static uint GetAllocationID();

	/*!
	Returns number of allocated blocks
	\return
	Number of blocks
	*/
	static ENF_INLINE size_t GetTotalAllocations()
	{
		return m_iTotalAllocations;
	}

	/*!
	Returns allocated memory in bytes
	*/
	static ENF_INLINE size_t GetTotalAllocated()
	{
		return m_iTotalAllocatedBytes;
	}

	/*!
	Returns number of allocated physical blocks
	\return
	Number of blocks
	*/
	static ENF_INLINE size_t GetTotalPhysicalAllocations()
	{
		return m_iTotalPhysicalAllocations;
	}

	/*!
	Returns allocated physical memory in bytes
	*/
	static ENF_INLINE size_t GetTotalPhysicalAllocated()
	{
		return m_iTotalPhysicalAllocatedBytes;
	}

	/*!
	Returns estimated overhead caused by many tiny
	allocations
	*/
	static ENF_INLINE size_t GetAllocOverhead()
	{
//		return m_iTotalAllocations * 48; //probably correct value
		return m_iTotalAllocations * 32; //probably correct value
	}

	/*!
	Gets size of allocation.
	\param ptr
	Pointer to allocation. It must be pointer returned by Alloc() or ReAlloc() method!
	\return
	Size of allocation
	*/
	static size_t GetSize(void* ptr);

	void CheckMemory(void* ptr);

#ifdef ENF_LEAK_DETECTOR
	/*!
	Reallocates memory. If it's possible, allocated block is just enlarged.
	Otherwise new block is allocated and old one is freed. In both cases
	data from old allocation are copied to the new block
	\param ptr
	Pointer to allocation. It must be pointer returned by Alloc() or ReAlloc() method!
	\param size
	Needed size
	\param src
	Name of source file
	\param line
	Line in source code
	\return
	Pointer to new allocation
	*/
	static void* ReAlloc(void* ptr, size_t size, const char *src, int line);


	/*!
	Allocates memory
	\param size
	Needed size
	\param src
	Name of source file
	\param line
	Line in source code
	\return
	Pointer to new allocation
	*/
	static void* Alloc(size_t size, const char *src, int line);

	/*!
	Allocates memory
	\param size
	Needed size
	\return
	Pointer to new allocation
	*/
	static void* Alloc(size_t size)
	{
		return Alloc(size, __FILE__, __LINE__);
	}

	/*!
	Allocates memory, aligned
	\param size	Needed size
	\param align Alignment
	\param src Name of source file
	\param line Line in source code
	\return Pointer to new allocation
	*/
	static void* AllocAligned(size_t size, uint align, const char *src, int line);

	/*!
	Allocates memory, aligned to multiply of 16
	\param size
	Needed size
	\param src
	Name of source file
	\param line
	Line in source code
	\return
	Pointer to new allocation
	*/
	ENF_INLINE static void* Alloc16(size_t size, const char *src, int line) { return AllocAligned(size, 16, src, line); }

	/*!
	Allocates memory, aligned to multiply of 16
	\param size
	Needed size
	\return
	Pointer to new allocation
	*/
	ENF_INLINE static void* Alloc16(size_t size)
	{
		return Alloc16(size, __FILE__, __LINE__);
	}

	static void* PhysicalAlloc(size_t size, uint alignment, uint protect, const char *src, int line);

	static void* PhysicalAlloc(size_t size, uint alignment, uint protect = 0)
	{
		return PhysicalAlloc(size, alignment, protect, __FILE__, __LINE__);
	}

	static void CreateSystemSnapshot(AllocationSnapshot& snap);

#else
	static void* ReAlloc(void* ptr, size_t size);

	static void* Alloc(size_t cb);
	static void* AllocAligned(size_t cb, uint alignment);
	ENF_INLINE static void* Alloc16(size_t cb) { return AllocAligned(cb, 16); }

	static void* Alloc(size_t cb, const char *src, int line)
	{
		return Alloc(cb);
	}
	static void* Alloc16(size_t cb, const char *src, int line)
	{
		return Alloc16(cb);
	}
	static void* PhysicalAlloc(size_t size, uint alignment, uint protect = 0);
	static void* PhysicalAlloc(size_t size, uint alignment, uint protect, const char *src, int line)
	{
		return PhysicalAlloc(size, alignment, protect);
	}
#endif

#ifdef ENF_PS3
	static void* PhysicalInit(const uint cmdBufSize);
	static size_t GetPhysicalSize();
	static void PhysicalDestroy();
	
	static void SetLocalMem();
	#ifdef ENF_LEAK_DETECTOR
		static void* LocalAlloc(size_t size, uint alignment, const char* fileName, uint line);
	#else 
		static void* LocalAlloc(size_t size, uint alignment);
	#endif
	static void LocalFree(void* ptr);
	
	static void CheckMemoryAddress(void* from, void* to);

	static uint PhysicalOffsetFromPtr(void* ptr);
	static uint LocalOffsetFromPtr(void* ptr);

	static void* PtrFromLocalOffset(uint offs);
	static void* PtrFromPhysicalOffset(uint offs);
#endif

	static void PhysicalFree(void* ptr);
	/*!
	Frees memory allocated by Alloc() / ReAlloc()
	\param ptr
	Pointer to allocation to be freed
	*/
	static void Free(void *ptr);
	
	/*!
	Frees memory allocated by AllocAligned()
	\param ptr
	Pointer to allocation to be freed
	*/
	static void FreeAligned(void *ptr);

	/*!
	Frees memory allocated by Alloc16()
	\param ptr
	Pointer to allocation to be freed
	*/
	ENF_INLINE static void Free16(void *ptr) { FreeAligned(ptr); }

	/*!
	Dumps current memory usage to VS Output window
	\param title
	String put before report
	*/
	static void Dump(const char* title, unsigned int flags);

#ifdef ENF_LEAK_DETECTOR
	/*!
	Create snapshot of memory
	*/
	static void CreateSnapshot();

	/*!
	compare memory snapshot
	*/
	static void CompareSnapshot();
#endif

	/*!
	Must be called prior to any usage!
	*/
	static void Init();
	static void Destroy();
	static void Validate();

	static void RegisterFlushableObject(FlushableObject* obj);
	static void UnregisterFlushableObject(FlushableObject* obj);
	static void Flush(unsigned int level);
	MemoryManager();
	~MemoryManager();
};

struct MemoryStats
{
	//!Total allocated memory
	size_t	Allocated;
	//!Overhead and cached allocations
	size_t	Overhead;
	//!Number of allocated blocks
	size_t	NumAllocations;
	//!Number of internal heap allocations
	size_t	NumBlocks;
};
/*!
SmallBlockAllocator for managing small memory allocs.
Only blocks of fixed size may be allocated by this.
*/
class PUBLIC_API SmallBlockAllocator
{
public:
	virtual ~SmallBlockAllocator() {}
	virtual void *Alloc(size_t size, const char* file, int line)=0;
	ENF_INLINE void *Alloc(size_t size)
	{
		return Alloc(size, NULL, 0);
	}

	//!Checks whether the pointer is allocated by this allocator
	virtual bool CheckPointer(void *pointer)=0;

	//!Release memory, return true if successfully
	virtual bool Free(void *pointer)=0;
	
	//!Releases all memory
	virtual void FreeAll()=0;
	
	//!Returns statistics
	virtual void GetStats(MemoryStats& stats) const=0;

	//!Dumps statistics into debug output window
	virtual void Dump(const char* str, unsigned int flags) const=0;

#ifdef ENF_LEAK_DETECTOR
	virtual uint CreateSnapshot(AllocationSnapshot &snap) const=0;
#endif

	//!Flushes unused memory
	virtual void Flush()=0;
};

extern PUBLIC_API SmallBlockAllocator* g_pSmallBlockAllocator;


#ifdef ENF_PS3
	void TestPS3MemoryPoolsIntegrity();
#endif


#ifndef ENF_DOXYGEN
	#define ENF_DECLARE_SMALLOBJECT(type) \
		static void* operator new(NEW_SIZE_T size, char const * file, int line){ return enf::g_pSmallBlockAllocator->Alloc(size, file, line);}\
		static void* operator new(NEW_SIZE_T size){ return enf::g_pSmallBlockAllocator->Alloc(size);} \
		static void operator delete(void * ptr, char const * file, int line){enf::g_pSmallBlockAllocator->Free(ptr);}\
		static void operator delete(void* ptr) { enf::g_pSmallBlockAllocator->Free(ptr);}\
		static void* operator new(NEW_SIZE_T size, void *p){ return p;}\
		static void* operator new[](NEW_SIZE_T size, void *p){ return p;}\
		static void operator delete(void* ptr, void* p) { }\
		static void operator delete[](void* ptr, void* p) { }

	#ifdef ENF_LEAK_DETECTOR
		#define ENF_DECLARE_MMOBJECT(type) \
			static void* operator new(NEW_SIZE_T size, char const * file, int line){ return enf::MemoryManager::Alloc(size, file, line);}\
			static void* operator new[](NEW_SIZE_T size, char const * file, int line){ return enf::MemoryManager::Alloc(size, file, line);}\
			static void* operator new(NEW_SIZE_T size){ return enf::MemoryManager::Alloc(size);} \
			static void* operator new[](NEW_SIZE_T size){ return enf::MemoryManager::Alloc(size);} \
			static void operator delete(void* ptr, char const * file, int line){enf::MemoryManager::Free(ptr);}\
			static void operator delete[](void* ptr, char const * file, int line){enf::MemoryManager::Free(ptr);}\
			static void operator delete(void* ptr) { enf::MemoryManager::Free(ptr);}\
			static void operator delete[](void* ptr) { enf::MemoryManager::Free(ptr);}\
			static void* operator new(NEW_SIZE_T size, void *p){ return p;}\
			static void* operator new[](NEW_SIZE_T size, void *p){ return p;}\
			static void operator delete(void* ptr, void* p) { }\
			static void operator delete[](void* ptr, void* p) { }
		#define ENF_DECLARE_MMOBJECTA16(type) \
			static void* operator new(NEW_SIZE_T size, char const * file, int line){ return enf::MemoryManager::Alloc16(size, file, line);}\
			static void* operator new[](NEW_SIZE_T size, char const * file, int line){ enf_assert((size & 0x0f) == 0); return enf::MemoryManager::Alloc16(size, file, line);}\
			static void* operator new(NEW_SIZE_T size){ return enf::MemoryManager::Alloc16(size);} \
			static void* operator new[](NEW_SIZE_T size){ enf_assert((size & 0x0f) == 0); return enf::MemoryManager::Alloc16(size);} \
			static void operator delete(void* ptr, char const * file, int line){enf::MemoryManager::Free16(ptr);}\
			static void operator delete[](void* ptr, char const * file, int line){enf::MemoryManager::Free16(ptr);}\
			static void operator delete(void* ptr) { enf::MemoryManager::Free16(ptr);}\
			static void operator delete[](void* ptr) { enf::MemoryManager::Free16(ptr);}\
			static void* operator new(NEW_SIZE_T size, void *p){ return p;}\
			static void* operator new[](NEW_SIZE_T size, void *p){ return p;}\
			static void operator delete(void* ptr, void* p) { }\
			static void operator delete[](void* ptr, void* p) { }

	#else
		#define ENF_DECLARE_MMOBJECT(type) \
			static void* operator new(NEW_SIZE_T size){ return enf::MemoryManager::Alloc(size);} \
			static void* operator new[](NEW_SIZE_T size){ return enf::MemoryManager::Alloc(size);} \
			static void operator delete(void* ptr) { enf::MemoryManager::Free(ptr);}\
			static void operator delete[](void* ptr) { enf::MemoryManager::Free(ptr);} \
			static void* operator new(NEW_SIZE_T size, void *p){ return p;}\
			static void* operator new[](NEW_SIZE_T size, void *p){ return p;}\
			static void operator delete(void* ptr, void* p) { }\
			static void operator delete[](void* ptr, void* p) { }
		#define ENF_DECLARE_MMOBJECTA16(type) \
			static void* operator new(NEW_SIZE_T size){ return enf::MemoryManager::Alloc16(size);} \
			static void* operator new[](NEW_SIZE_T size){ enf_assert((size & 0x0f) == 0); return enf::MemoryManager::Alloc16(size);} \
			static void operator delete(void* ptr) { enf::MemoryManager::Free16(ptr);}\
			static void operator delete[](void* ptr) { enf::MemoryManager::Free16(ptr);}\
			static void* operator new(NEW_SIZE_T size, void *p){ return p;}\
			static void* operator new[](NEW_SIZE_T size, void *p){ return p;}\
			static void operator delete(void* ptr, void* p) { }\
			static void operator delete[](void* ptr, void* p) { }
	#endif
#else
	#define ENF_DECLARE_SMALLOBJECT(type)
	#define ENF_DECLARE_MMOBJECT(type)
	#define ENF_DECLARE_MMOBJECTA16(type)
#endif

#ifdef ENF_LEAK_DETECTOR
	#define REALLOC(ptr, size) MemoryManager::ReAlloc(ptr, size, __FILE__, __LINE__)

	#define ALLOCALL(size, align) MemoryManager::AllocAligned(size, align, __FILE__, __LINE__)
	#define FREEALL(ptr) MemoryManager::FreeAligned(ptr)

	#define ALLOC16(size) enf::MemoryManager::Alloc16(size, __FILE__, __LINE__)
	#define FREE16(ptr) enf::MemoryManager::Free16(ptr)

	#define ENF_NEW new(__FILE__, __LINE__)

	#define PHYS_ALLOC(size, align, protect)	enf::MemoryManager::PhysicalAlloc(size, align, protect, __FILE__, __LINE__)
	#define PHYS_FREE(ptr)	enf::MemoryManager::PhysicalFree(ptr)

	#ifdef ENF_PS3	
		#define LOCAL_ALLOC(size,align) enf::MemoryManager::LocalAlloc(size, align, __FILE__, __LINE__)
		#define LOCAL_ALLOC2(size,align,fileName,line) enf::MemoryManager::LocalAlloc(size, align, fileName, line)
		#define PHYS_ALLOC2(size, align, protect, fileName, line)	enf::MemoryManager::PhysicalAlloc(size, align, protect, fileName, line)

		#define LOCAL_FREE(ptr)	enf::MemoryManager::LocalFree(ptr)
	#endif
#else

	#define REALLOC(ptr, size) MemoryManager::ReAlloc(ptr, size)

	#define ALLOCALL(size, align) MemoryManager::AllocAligned(size, align)
	#define FREEALL(ptr) MemoryManager::FreeAligned(ptr)

	#define ALLOC16(size) enf::MemoryManager::Alloc16(size)
	#define FREE16(ptr) enf::MemoryManager::Free16(ptr)

	#define PHYS_ALLOC(size, align, protect)	enf::MemoryManager::PhysicalAlloc(size, align, protect)
	#define PHYS_FREE(ptr)	enf::MemoryManager::PhysicalFree(ptr)

	#ifdef ENF_PS3	
		#define LOCAL_ALLOC(size, align)	 enf::MemoryManager::LocalAlloc(size, align)
		#define LOCAL_ALLOC2(size, align, fileName, line)	enf::MemoryManager::LocalAlloc(size, align)
		#define PHYS_ALLOC2(size, align, protect, fileName, line) enf::MemoryManager::PhysicalAlloc(size, align, protect)

		#define LOCAL_FREE(ptr)	enf::MemoryManager::LocalFree(ptr)
	#endif
#endif

//@}

/*!
The base class for all objects that can register in the memory manager
as flushable objects. A flushable object can release unused memory
upon request. A typical example is a pool or growable array that does
not shrink automatically. It is also used for releasing memory allocated
by function-local static objects during a flush operation.

A flushable object must register in the memory manager to
flush automatically. See MemoryManager::RegisterFlushableObject().
*/
class PUBLIC_API FlushableObject
{
public:
	ENF_DECLARE_MMOBJECT(FlushableObject);

	FlushableObject() : m_pNextFlushableObject(NULL), m_pPreviousFlushableObject(NULL)
	{
	}

	virtual ~FlushableObject()
	{
	}

	/*!
	Flush this object and release extra memory used.
	*/
	virtual void Flush (unsigned int level) = 0;

	ENF_INLINE FlushableObject* GetNextFlushableObject()
	{
		return m_pNextFlushableObject;
	}

	ENF_INLINE void SetNextFlushableObject(FlushableObject* next)
	{
		m_pNextFlushableObject = next;
	}

	ENF_INLINE FlushableObject* GetPreviousFlushableObject()
	{
		return m_pPreviousFlushableObject;
	}

	ENF_INLINE void SetPreviousFlushableObject(FlushableObject* previous)
	{
		m_pPreviousFlushableObject = previous;
	}

protected:
	friend class MemoryManager;

	FlushableObject* m_pNextFlushableObject;
	FlushableObject* m_pPreviousFlushableObject;
};

/*!
This class is a flushable object wrapper and agent for any class not 
inheriting the flushable object base. It will automatically register
and unregister itself in the MemoryManager. It is used internally 
to keep track of allocations by function-local static variables that 
may allocate memory (arrays for instance).

Class T must have a void Flush(uint) method.
*/
template <class T> class FlushableObjectAgent: public FlushableObject
{
public:
	ENF_DECLARE_MMOBJECT(FlushableObjectAgent);

	FlushableObjectAgent(T* original) : FlushableObject(), m_pObject(original)
	{
		MemoryManager::RegisterFlushableObject(this);
	}

	~FlushableObjectAgent()
	{
		MemoryManager::UnregisterFlushableObject(this);
	}

	virtual void Flush(unsigned int level)
	{
		m_pObject->Flush(level);
	}

private:
	T* m_pObject;
};

class PUBLIC_API DefaultAllocator
{
public:
	DefaultAllocator() {}
	void Free(void* ptr)
	{
		MemoryManager::Free(ptr);
	}

	void* Alloc(size_t size, const char* file, int line)
	{
		return MemoryManager::Alloc(size, file, line);
	}

	void* ReAlloc(void* ptr, size_t size)
	{
		return REALLOC(ptr, size);
	}
};

class CustomDataAllocator
{
private:
	CustomDataAllocator() {}
	void*		m_pData;
	size_t	m_iMaxSize;

public:
	CustomDataAllocator(void* ptr, size_t size): m_pData(ptr), m_iMaxSize(size) {}
	CustomDataAllocator(const CustomDataAllocator& init): m_pData(init.m_pData), m_iMaxSize(init.m_iMaxSize) {}
	void Free(void* ptr)
	{
	}
	void* GetBuffer() { return m_pData; }
	size_t GetSize() const { return m_iMaxSize; }
	void* Alloc(size_t size, const char* file, int line)
	{
		enf_assert(size < m_iMaxSize);
		return m_pData;
	}

	void* ReAlloc(void* ptr, size_t size)
	{
		enf_assert(ptr == NULL || ptr == m_pData);
		enf_assert(size < m_iMaxSize);
		return m_pData;
	}
};

} //namespace enf

#ifdef __GCC__
	#pragma GCC visibility push(default)
#endif

extern "C++"
{

#ifdef ENF_LEAK_DETECTOR
	void *operator new[](NEW_SIZE_T cb, const char* file, int line) THROW_NEW;
	void *operator new(NEW_SIZE_T cb, const char* file, int line) THROW_NEW;

	void operator delete[](void *p, const char* file, int line) THROW_DELETE;
	void operator delete(void *p, const char* file, int line) THROW_DELETE;

	/*#if _MSC_VER >= 1500
		_Ret_bytecap_(_Size) void *operator new[](NEW_SIZE_T _Size);
		_Ret_bytecap_(_Size) void *operator new(NEW_SIZE_T _Size);
	#else*/
		void *operator new[](NEW_SIZE_T cb) THROW_NEW;
		void *operator new(NEW_SIZE_T cb)  THROW_NEW;
		#ifdef ENF_PS3
			void *operator new(NEW_SIZE_T count, size_t align);
		#endif
	//#endif
	void operator delete[](void *p) THROW_DELETE;
	void operator delete(void *p) THROW_DELETE;	
#else

	#define ENF_NEW new

/*	#if _MSC_VER >= 1500
		_Ret_bytecap_(_Size) void *operator new[](NEW_SIZE_T _Size);
		_Ret_bytecap_(_Size) void *operator new(NEW_SIZE_T _Size);
	#else*/
		void *operator new[](NEW_SIZE_T cb) THROW_NEW;
		void *operator new(NEW_SIZE_T cb) THROW_NEW;
		#ifdef ENF_PS3
			void *operator new(NEW_SIZE_T count, size_t align);
		#endif
//	#endif
	void operator delete[](void *p) THROW_DELETE;
	void operator delete(void *p) THROW_DELETE;
#endif
}
#ifdef __GCC__
	#pragma GCC visibility pop
#endif

#endif //ENF_MEMORY_H
