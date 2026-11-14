#ifndef _CHECKMEM_HPP
#define _CHECKMEM_HPP

#include "Es/platform.hpp"
#include <malloc.h>

//! interface to memory allocations
class MemFunctions
{
	public:
	virtual void *New(size_t size) {return malloc(size);}
	virtual void *New(size_t size, const char *file, int line) {return malloc(size);}
	virtual void Delete(void *mem){free(mem);}
	virtual void Delete(void *mem, const char *file, int line) {free(mem);}
	//@{ POSIX style realloc - memory is allowed to move
	virtual void *Realloc(void *mem, size_t size) {return realloc(mem,size);}
	virtual void *Realloc(void *mem, size_t size, const char *file, int line) {return realloc(mem,size);}
	//@}
	/// grow (or shrink) given memory region
	virtual void *Resize(void *mem, size_t size) {return 0;}

  virtual void *NewPage(size_t size, size_t align) {return malloc(size);}
  virtual void DeletePage(void *page, size_t size) {free(page);}
  virtual size_t GetPageRecommendedSize() {return 4*1024;}

	//! base of memory allocation space, NULL is unknown/undefined
	virtual void *HeapBase() {return 0;}

  virtual size_t GetSize(void *ptr) {return 0;}
  
#ifdef _WIN32
  //! approximate total amount of allocated memory
  virtual size_t HeapUsed()
  {
    size_t TotalAllocatedWin();
    return TotalAllocatedWin();
  }
#else
  //! approximate total amount of allocated memory
  virtual size_t HeapUsed() {return linuxMemoryUsage();}
#endif

	//! approximate total amount of allocated memory
	//! only memory allocated by new is considered if possible
	virtual size_t HeapUsedByNew() {return HeapUsed();}
	
	//! approximate total amount of commited memory
	virtual size_t HeapCommited() {return 0;}

	//! approximate total count of free memory blocks (used to determine fragmentation)
	virtual int FreeBlocks() {return 1;}

	//! approximate total count of allocated memory blocks (used to determine fragmentation)
	virtual int MemoryAllocatedBlocks() {return 1;}

	//! list all allocated blocks to debug log
	virtual void Report() {}

	//! check heap integrity
	virtual bool CheckIntegrity() {return true;}
	//! check if out of memory is signalized
	//! if true, application should limit memory allocation as much as possible
	virtual bool IsOutOfMemory() {return false;}

	//! clean-up any cached memory
	virtual void CleanUp() {}
	
	//@{ access to a memory critical section, can be used to implement CS hierarchy
	virtual void Lock() {}
	virtual void Unlock() {}
	//@}
	
	//@{ let memory allocated handle per-thread initialization/cleanup if necessary
	virtual void OnThreadEnter() {}
	virtual void OnThreadExit() {}
	//@}
};

extern MemFunctions *GMemFunctions;
extern MemFunctions *GSafeMemFunctions;
extern MemFunctions *GMTMemFunctions; // MT safe heap - equal to GMemFunctions or GSafeMemFunctions depending on configuration

#if defined MFC_NEW
	// MFC application needs to use MFC new

	#define safeNew(size) malloc(size)
	#define safeDelete(mem) free(mem)

  #define mtNew(size) malloc(size)
  #define mtDelete(mem) free(mem)

	inline size_t MemoryUsed() {return 1024*1024;}
	inline size_t MemoryUsedByNew() {return 1024*1024;}
	inline size_t MemoryCommited() {return 0;}

	inline void SafeMemoryCleanUp() {}
	inline void MemoryCleanUp() {}
	inline int MemoryFreeBlocks() {return 1;}
	inline bool IsOutOfMemory() {return false;}

  typedef void *MemPageHandle;

  //! allocate page of memory, use virtual memory pages directly if possible
  #define NewPage(size,align) malloc(size)
  //! free page allocated by NewPage
  #define DeletePage(mem,size) free(mem)
  //! get recommended page size
  #define GetPageRecommendedSize() (4*1024) // 4 K is default on x86 and most other platforms as well

	// if desired, use MS VC++ memory debuging
	//#define _CRTDBG_MAP_ALLOC 1
	//#include <crtdbg.h>
	#define PAGE_ALIGNMENT_POSSIBLE 0

#else


	inline void * CCALL operator new (size_t size, intptr_t addr, char const *file, int line)
	{ // placement new
		return (void *)addr;
	}
	#if defined _CPPUNWIND && !defined _INC_CRTDBG
		inline void CCALL operator delete (void *mem, intptr_t addr, char const *file, int line)
		{ // placement delete
		}
	#endif

	// note: global array versions ([]) are ignored in VC5.0

	inline RESTRICT_RETURN void * CCALL operator new ( size_t size ) {return GMemFunctions->New(size);}
	inline RESTRICT_RETURN void * CCALL operator new[] ( size_t size ) {return GMemFunctions->New(size);}

	inline RESTRICT_RETURN void * CCALL operator new ( size_t size, const char *file, int line )
	{
		return GMemFunctions->New(size,file,line);
	}
	inline RESTRICT_RETURN void * CCALL operator new[] ( size_t size, const char *file, int line )
	{
		return GMemFunctions->New(size,file,line);
	}
	inline void CCALL operator delete ( void *ptr )
	{
		GMemFunctions->Delete(ptr);
	}
	inline void CCALL operator delete[] ( void *ptr )
	{
		GMemFunctions->Delete(ptr);
	}

	#ifdef _CPPUNWIND
		inline void CCALL operator delete ( void *ptr, const char *file, int line )
		{
			GMemFunctions->Delete(ptr,file,line);
		}
		inline void CCALL operator delete[] ( void *ptr, const char *file, int line )
		{
			GMemFunctions->Delete(ptr,file,line);
		}
	#endif

	#if !_RELEASE && defined _WIN32

		#ifdef _CPPRTTI
			#define ALLOC_DEBUGGER 1
		#else
			// when there is no RTTI, alloc debugging is not possible
			#define ALLOC_DEBUGGER 0
		#endif

		#ifndef CCALL
			#define CCALL
		#endif

		// memory errors tracking for custom new handlers


		#define debugNew new(__FILE__,__LINE__)
		#define debugNewAddr(addr) new(addr,__FILE__,__LINE__)

		#define stdNew new()
		#define stdNewAddr new(addr)

		#define newAddr(addr) new(addr)

		#include <Es/Memory/debugNew.hpp>
	#endif

  inline RESTRICT_RETURN void *NewPage(size_t size, size_t align) {return GMemFunctions->NewPage(size,align);}
  inline void DeletePage(void *page,size_t size) {GMemFunctions->DeletePage(page,size);}

  inline size_t GetPageRecommendedSize() {return GMemFunctions->GetPageRecommendedSize();}

	// required interface for network safe heap
	#if ALLOC_DEBUGGER
	  #define safeNew(size) (GSafeMemFunctions->New((size),__FILE__,__LINE__))
	#else
	  inline RESTRICT_RETURN void *safeNew ( size_t size ){return GSafeMemFunctions->New(size);}
	#endif
	inline void safeDelete ( void *mem ){GSafeMemFunctions->Delete(mem);}
  
  // required interface for MT safe heap
  #if ALLOC_DEBUGGER
    #define mtNew(size) (GMTMemFunctions->New((size),__FILE__,__LINE__))
  #else
    inline RESTRICT_RETURN void *mtNew ( size_t size ){return GMTMemFunctions->New(size);}
  #endif
  inline void mtDelete ( void *mem ){GMTMemFunctions->Delete(mem);}

	inline bool IsOutOfMemory() {return GMemFunctions->IsOutOfMemory();}
	inline size_t MemoryUsed() {return GMemFunctions->HeapUsed();}
	inline size_t MemoryUsedByNew() {return GMemFunctions->HeapUsedByNew();}
	inline size_t MemoryCommited() {return GMemFunctions->HeapCommited();}

	inline void SafeMemoryCleanUp() {GSafeMemFunctions->CleanUp();}
	//inline void MemoryCleanUp() {GMemFunctions->CleanUp();}
	inline int MemoryFreeBlocks() {return GMemFunctions->FreeBlocks();}

#ifdef MALLOC_WIN_TEST
  #define PAGE_ALIGNMENT_POSSIBLE 0
#else
  #define PAGE_ALIGNMENT_POSSIBLE 1
#endif

#endif
  extern size_t TotalAllocatedWin();

#endif
