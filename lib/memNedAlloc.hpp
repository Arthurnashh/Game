#ifndef _MEM_NED_ALLOC_HPP
#define _MEM_NED_ALLOC_HPP

#include "memGrow.hpp"

#include <Es/Threads/multisync.hpp>

#define INDIRECT 1


#include "El/FreeOnDemand/memFreeReq.hpp"

#include <Es/Memory/normalNew.hpp>

typedef size_t MemSize;
const int DefaultAlign=16;

void MemoryErrorMalloc( int size );
const char *MemoryAllocatorName();
// very simple chunk based allocation code
// no reference counting, no chunk restoring

#pragma warning(disable:4200)


class MemHeap;


  
//!/ Memory allocation heap 
/*!
\patch_internal 1.11 Date 8/1/2001 by Ondra
- Fixed: Error in memory heap clean-up during MP shutdown.
This error caused that in debug build
memory leaks were not reported after MP game.
*/

class MemHeap: public RefCount, public IFreeOnDemandEnumerator
{
  private:
  mutable bool _destruct;

  private:
  int _vspaceMultiplier;
  MemSharedState _shared;
  MemCommit _memRegions; // data area - used for regions only
  
  /// allocated directly via OS calls
  size_t _allocatedDirect;

  const char *_name;
  //{ FIX
  MemHeap **_lateDestruct;
  //}
  MemoryFreeOnDemandList _freeOnDemand; // memory allocated in the heap
  MemoryFreeOnDemandList _freeOnDemandLowLevel; // memory allocated in the heap
  
  #if _DEBUG
    mutable InitVal<bool,false> _onDemandListClosed;
    /** after any list traversal the list is assumed to be closed and can never be changed again
    This guarantess thread safety for traversal inside of FreeOnDemandFrame without having to lock anything
    */
    
    void CloseOnDemandList() const {_onDemandListClosed=true;}
  #else
    void CloseOnDemandList() const {}
  #endif
  
  /// remember address of a large free 
  void *_freeVMRegion;

  
  private:
  void DoConstruct(MemHeap **lateDestructPointer);
  void DoConstruct
  (
    const char *name, MemSize size, MemSize align,
    MemHeap **lateDestructPointer
  );
  void DoDestruct();

  public:

  MemHeap(
    const char *name, MemSize size, MemSize align=DefaultAlign,
    MemHeap **lateDestructPointer = NULL
  );
  MemHeap(
    MemHeap **lateDestructPointer = NULL
  );
  void Lock() const {}
  void Unlock() const {}

  bool CheckVirtualFree(size_t size);
  
  void SetVirtualSpaceMultiplier(int mult)
  {
    _vspaceMultiplier = mult;
  }
  void Init
  (
    const char *name, MemSize size,
    MemSize align=DefaultAlign, MemHeap **lateDestructPointer = NULL
  )
  {
    DoDestruct();
    DoConstruct(name,size,align,lateDestructPointer);
  }
  void Clear() {DoDestruct();}
  ~MemHeap();

  void BeginUse(size_t minUsage);
  void EndUse();
  
  size_t FreeOnDemand(size_t size) {return _freeOnDemand.Free(size);}

  size_t FreeOnDemandAll() {return _freeOnDemand.FreeAll();}

  void RegisterFreeOnDemand(IMemoryFreeOnDemand *object)
  {
    AssertMainThread();
    _freeOnDemand.Register(object);
  }

  //@( caution - while enumerating, heap may need to be locked - use GetFreeOnDemandEnumerator whenever possible
  IMemoryFreeOnDemand *GetFirstFreeOnDemand() const {return _freeOnDemand.First();}
  IMemoryFreeOnDemand *GetNextFreeOnDemand(IMemoryFreeOnDemand *cur) const {return _freeOnDemand.Next(cur);}

  IMemoryFreeOnDemand *GetFirstFreeOnDemandLowLevel() const {return _freeOnDemandLowLevel.First();}
  IMemoryFreeOnDemand *GetNextFreeOnDemandLowLevel(IMemoryFreeOnDemand *cur) const {return _freeOnDemandLowLevel.Next(cur);}
  //@}

  IMemoryFreeOnDemand *GetFirstFreeOnDemandSystem() const {return GetFirstFreeOnDemandSystemMemory();}
  IMemoryFreeOnDemand *GetNextFreeOnDemandSystem(IMemoryFreeOnDemand *cur) const {return GetNextFreeOnDemandSystemMemory(cur);}
  
  /// safe way to access on demand enumeration
  const IFreeOnDemandEnumerator &GetFreeOnDemandEnumerator() const {return *this;}
  
  size_t FreeOnDemandLowLevel(size_t size) {return _freeOnDemandLowLevel.Free(size);}

  size_t FreeOnDemandLowLevelAll() {return _freeOnDemandLowLevel.FreeAll();}

  void RegisterFreeOnDemandLowLevel(IMemoryFreeOnDemand *object)
  {
    Assert(!_onDemandListClosed);
    _freeOnDemandLowLevel.Register(object);
  }

  
  void CleanUp();

  //@{ error reporting - will terminate the application
  void MemoryErrorPage(int size);
  void MemoryError(int size);
  //@}

  void *AllocRegion(MemSize size, MemSize align);
  void FreeRegion(void *region, MemSize size);
  size_t GetRegionRecommendedSize() const;

  int IsFromHeap( void *pos ) const;

  MemSize TotalAllocated() const;
  MemSize TotalCommitted() const;
  MemSize TotalReserved() const;
  MemSize CommittedByNew() const;
  MemSize ReservedByNew() const;
  MemSize CommittedByPages() const {return _memRegions.GetCommitted();}
  MemSize ReservedByPages() const {return _memRegions.GetReserved();}
  MemSize CommittedByDirect() const {return _allocatedDirect;}
  
  MemSize LongestFreeForNew() const {return 0;}
  MemSize LongestFreeForPages() const {return _memRegions.FindLongestRange()*_memRegions.GetPageSize();}
  #if _ENABLE_REPORT
  bool Check() const;
  #else
  bool Check() const {return true;}
  #endif

  #if _ENABLE_REPORT
  void ReportTotals() const;
  void ReportAnyMemory(const char *reason);
  size_t FreeOnDemandReleaseSlot(const char *id, size_t toRelease, size_t *verifyRelease);
  #endif
  
  void FreeOnDemandFrame();

  // MemHeap must use customized new,
  // because global new probable uses MemHeap
  /*!
  \patch_internal 1.22 Date 9/11/2001 by Ondra
  - Fixed: Out-of memory diagnostics improved.
  */
  void *operator new( size_t size )
  {
    void *ret=malloc(size);
    if( !ret ) MemoryErrorMalloc(size);
    return ret;
  }
  void operator delete( void *mem ) {free(mem);}

  bool CheckIntegrity() const;

  //! some unused system memory is needed outside the heap
  size_t FreeSystemMemory(size_t size);
  size_t TotalLowLevelMemory() const;
  size_t FreeOnDemandLowLevelMemory(size_t size);

  private:
  MemHeap( const MemHeap &src );
  void operator = ( const MemHeap &src );
};


#define SCOPE_LOCK() ScopeLock<const MemHeapLocked> lock(*this)

class MemHeapLocked: private MemHeap
{
  typedef MemHeap base;

  mutable CriticalSection _lock; // synchronize multithread access

  
  public:
  MemHeapLocked(
    const char *name, MemSize size, MemSize align=DefaultAlign,
    MemHeapLocked **lateDestructPointer = NULL
  );
  MemHeapLocked();

  void Lock() const;
  void Unlock() const {_lock.Unlock();}

  // (1) to create use Ref<MemHeapLocked> heap = new MemHeapLocked()
  // (2) to initialize use Init
  //     pass heap name and heap maximumum size
  //     for main application heap limit should be something at least 512 MB
  //     for small auxiliary heaps 8..32 MB may be reasonable
  // (3) when the heap is no longer needed, use heap.Free()
  void BeginUse(size_t minUsage)
  {
    SCOPE_LOCK();
    base::BeginUse(minUsage);
  }
  void EndUse()
  {
    SCOPE_LOCK();
    base::EndUse();
  }

  bool CheckVirtualFree(size_t size) {return base::CheckVirtualFree(size);}

  void Init( const char *name, MemSize size, MemSize align=DefaultAlign )
  {
    base::Init(name,size,align);
  }
  void SetVirtualSpaceMultiplier(int mult)
  {
    SCOPE_LOCK();
    base::SetVirtualSpaceMultiplier(mult);
  }

  void CleanUp(){SCOPE_LOCK();base::CleanUp();}

  int IsFromHeap( void *pos ) const {SCOPE_LOCK();return base::IsFromHeap(pos);}

  void *operator new( size_t size )
  {
    void *ret=malloc(size);
    if( !ret ) MemoryErrorMalloc(size);
    return ret;
  }
  void *operator new( size_t size, const char *file, int line )
  {
    void *ret=malloc(size);
    if( !ret ) MemoryErrorMalloc(size);
    return ret;
  }
  void operator delete( void *mem ) {free(mem);}
#ifdef __INTEL_COMPILER
  void operator delete( void *mem, const char *file, int line ) {free(mem);}
#endif

  int AddRef() const;
  int Release() const;

  //void *Memory() const {SCOPE_LOCK();return base::Memory();}
  //MemSize Size() const {SCOPE_LOCK();return base::Size();}

  void *AllocRegion(MemSize size, MemSize align)
  {
    SCOPE_LOCK();return base::AllocRegion(size,align);
  }
  void FreeRegion(void *region, MemSize size)
  {
    SCOPE_LOCK();base::FreeRegion(region,size);
  }

  size_t GetRegionRecommendedSize() const
  {
    return base::GetRegionRecommendedSize();
  }

  MemSize TotalAllocated() const{SCOPE_LOCK();return base::TotalAllocated();}
  MemSize TotalCommitted() const{SCOPE_LOCK();return base::TotalCommitted();}
  MemSize TotalReserved() const{SCOPE_LOCK();return base::TotalReserved();}

  size_t FreeOnDemand(size_t size){SCOPE_LOCK();return base::FreeOnDemand(size);}
  size_t FreeOnDemandAll(){SCOPE_LOCK();return base::FreeOnDemandAll();}
  void RegisterFreeOnDemand(IMemoryFreeOnDemand *object)
  {
    SCOPE_LOCK();base::RegisterFreeOnDemand(object);
  }

  size_t FreeOnDemandLowLevel(size_t size){SCOPE_LOCK();return base::FreeOnDemandLowLevel(size);}
  size_t FreeOnDemandLowLevelAll(){SCOPE_LOCK();return base::FreeOnDemandLowLevelAll();}
  void RegisterFreeOnDemandLowLevel(IMemoryFreeOnDemand *object)
  {
    SCOPE_LOCK();base::RegisterFreeOnDemandLowLevel(object);
  }
  
  int RefCounter() const {SCOPE_LOCK();return base::RefCounter();}

  #if _ENABLE_REPORT
  bool Check() const {SCOPE_LOCK();return base::Check();}
  #else
  bool Check() const {return true;}
  #endif

  size_t FreeSystemMemory(size_t size) {SCOPE_LOCK(); return base::FreeSystemMemory(size);}
  size_t TotalLowLevelMemory() const {SCOPE_LOCK(); return base::TotalLowLevelMemory();}

  MemSize CommittedByNew() const {SCOPE_LOCK(); return base::CommittedByNew();}
  MemSize CommittedByPages() const {SCOPE_LOCK(); return base::CommittedByPages();}
  MemSize ReservedByNew() const {SCOPE_LOCK(); return base::ReservedByNew();}
  MemSize ReservedByPages() const {SCOPE_LOCK(); return base::ReservedByPages();}
  MemSize CommittedByDirect() const {/*atomic, no need to lock*/return base::CommittedByDirect();}
  
  MemSize LongestFreeForNew() const {SCOPE_LOCK(); return base::LongestFreeForNew();}
  MemSize LongestFreeForPages() const {SCOPE_LOCK(); return base::LongestFreeForPages();}

  void MemoryErrorPage(int size){SCOPE_LOCK();base::MemoryErrorPage(size);}
  void MemoryError(int size){SCOPE_LOCK();base::MemoryError(size);}

  #if _ENABLE_REPORT
  void ReportTotals() const {SCOPE_LOCK();base::ReportTotals();}
  
  void ReportAnyMemory(const char *reason){SCOPE_LOCK();return base::ReportAnyMemory(reason);}
  size_t FreeOnDemandReleaseSlot(const char *id, size_t toRelease, size_t *verifyRelease)
  {
    SCOPE_LOCK();return base::FreeOnDemandReleaseSlot(id,toRelease,verifyRelease);
  }
  #endif

  const IFreeOnDemandEnumerator &GetFreeOnDemandEnumerator() const {return base::GetFreeOnDemandEnumerator();}
  void FreeOnDemandFrame(){base::FreeOnDemandFrame();}
  size_t FreeOnDemandLowLevelMemory(size_t size){SCOPE_LOCK();return base::FreeOnDemandLowLevelMemory(size);}

  IMemoryFreeOnDemand *GetFirstFreeOnDemand() const {SCOPE_LOCK();return base::GetFirstFreeOnDemand();}
  IMemoryFreeOnDemand *GetNextFreeOnDemand(IMemoryFreeOnDemand *cur) const {SCOPE_LOCK();return base::GetNextFreeOnDemand(cur);}

  IMemoryFreeOnDemand *GetFirstFreeOnDemandLowLevel() const {SCOPE_LOCK();return base::GetFirstFreeOnDemandLowLevel();}
  IMemoryFreeOnDemand *GetNextFreeOnDemandLowLevel(IMemoryFreeOnDemand *cur) const {SCOPE_LOCK();return base::GetNextFreeOnDemandLowLevel(cur);}
};

#include <Es/Memory/debugNew.hpp>

#endif
