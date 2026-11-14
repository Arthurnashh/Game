#ifdef _MSC_VER
#pragma once
#endif

#ifndef _EL_MEM_STORE_HPP
#define _EL_MEM_STORE_HPP

#include <El/elementpch.hpp>
#include <Es/Common/win.h>
#include <Es/Threads/multisync.hpp>

#if !defined _WIN32
// no memory store on Linux - no file mapping available
# define USE_MEM_STORE 0
#else
# define USE_MEM_STORE 1
#endif

#if USE_MEM_STORE

#define MEM_STORE_THREADSAFE 1

/// access to a physical/page file memory with no need to have virtual addresses reserved
class MemoryStore: private NoCopy
{
  #if MEM_STORE_THREADSAFE
  CriticalSection _lock;
  #endif
  
  InitVal<int> _threadId;
  
  /// all pages from freeFrom up are free
  int _freeFrom;
  
  /// list of free pages from 0 to _freeFrom
  /** typically the cache is full, and little pages are free */
  AutoArray<int> _free;
  /// some page may be allocated multiple times, because of buffers sharing memory
  AutoArray<unsigned char> _allocCount;
  
  typedef HANDLE MappingType;
  
  MappingType _mapping;
  int _pageSize;
  int _allocSize;
  int _pageCount;
  /// how many pages are currently committed
  int _committed;
  /// how many pages are currently "mapped" (diagnostics)
  AtomicInt _mapped;
  
  #if !NO_MAP
  #if MEM_STORE_THREADSAFE
  CriticalSection _lockMappingCache;
  #endif
  
  struct MappingCache
  {
    void *mapping;
    int refCount;
    
    MappingCache(void *mapping=NULL, int refCount=0):mapping(mapping),refCount(refCount)
    {
    }
    ClassIsSimpleZeroed(MappingCache);
  };
  
  AutoArray<MappingCache> _mappingCache;
  #endif
  
  protected:
  /// helper for freeing the allocations
  void MarkFree(int index)
  {
    // when returning the last page, increase free space instead
    // such situation is not very common, however handling it costs us very little
    if (index==_freeFrom-1)
    {
      _freeFrom=index;
    }
    else
    {
      // otherwise simply add into the list of free pages
      _free.Add(index);
    }
  }
  public:
  MemoryStore();
  
  void Init(size_t size);
  ~MemoryStore();
  
  /// allocate a page
  int Alloc();
  /// share the same allocation between multiple allocators
  void ShareAlloc(int index);
  /// release a reference to a page
  void Free(int index);
  
  
  void *MapView(int index, bool write=false);
  void UnmapView(int index, void *mem);
  
  int GetFreePageCount() const {return _pageCount-_committed;}
  size_t GetPageSize() const {return _pageSize;}
  size_t GetCommittedBytes() const {return _committed*_pageSize;}
  #if _ENABLE_CHEATS
  int GetMappedPages() const {return _mapped;}
  #endif
  size_t GetMaxBytes() const {return _pageSize*_pageCount;}
};

#endif

#endif
