#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MEMTABLE_HPP
#define _MEMTABLE_HPP

#include <Es/Containers/listBidir.hpp>
#include <Es/Containers/list.hpp>

#include "memGrow.hpp"

#include <Es/Threads/multisync.hpp>

#define FreeLink TLinkBidir<2> // 8B
#define BusyLink TLinkBidir<1> // 8B

#define INDIRECT 1

#include <Es/Memory/fastAlloc.hpp>
#include "El/FreeOnDemand/memFreeReq.hpp"

#include <Es/Memory/normalNew.hpp>

typedef size_t MemSize;
const int DefaultAlign=16;

void MemoryErrorMalloc( int size );

// very simple chunk based allocation code
// no reference counting, no chunk restoring

#pragma warning(disable:4200)

class InfoAlloc
{

  public:
  InfoAlloc(size_t n);
  ~InfoAlloc();

  void *Alloc(MemSharedState &shared, MemCommit &commit, size_t n );
  void Free( MemCommit &commit, void*pAlloc );
  void Destruct(MemCommit &commit){FreeChunks(commit);}
  size_t TotalAllocated(const MemCommit &commit) const;

  size_t ItemSize() const {return esize;}
  //static int ChunkSize() {return sizeof(Chunk);}

  protected:
  struct Chunk;
  struct Link //: public CLDLink
  {
    Link *next;
    Chunk *chunk; // which chunk is it in
  };
  struct Chunk
  {
    FastAlloc *allocator; // which allocator this chunk serves for
    Chunk *next;
    char mem[];
  };

  // change chunk reference count

  // no free items - add new chunk
  bool Grow(MemSharedState &shared, MemCommit &commit);

  // parent memory manager
  Chunk *NewChunk(MemSharedState &shared, MemCommit &commit);
  void DeleteChunk(MemCommit &commit, Chunk *chunk );

  // destruction - free all chunks
  void FreeChunks(MemCommit &commit);

  Chunk *chunks;
  unsigned int esize;
  Link *head;
};


struct BusyBlock: public BusyLink,public FreeLink
{
  char *_memory; // memory position

  bool IsFree() const
  {
    return FreeLink::next!=NULL;
  }

  private:
  void* operator new[]   (size_t n);
  void  operator delete[](void* ptr);
  public:
  void *operator new( size_t n, InfoAlloc &info, MemSharedState &shared, MemCommit &commit){return info.Alloc(shared,commit,n);}
  void *operator new( size_t n, InfoAlloc &info, MemSharedState &shared, MemCommit &commit, const char *file, int line ){return info.Alloc(shared,commit,n);}
  void operator delete( void* ptr, InfoAlloc &info, MemCommit &commit ){info.Free(commit,ptr);}
#ifdef __INTEL_COMPILER
  void operator delete( void* ptr, InfoAlloc &info, MemCommit &commit, const char *file, int line ){info.Free(commit,ptr);}
#endif
};

typedef BusyBlock FreeBlock;

typedef TListBidir<BusyBlock,BusyLink> BlockList;

#define NotEndOf(Type,item,list) (Type *)(item)!=(Type *)&(list)

class MemHeap;

class FreeList: public TListBidir<FreeBlock,FreeLink>
{
  public:
  MemSize MaxFreeLeft( const MemHeap *heap ) const;
  MemSize TotalFreeLeft( const MemHeap *heap ) const;
  int CountFreeLeft() const;
};

class SmallMemAlloc;

#if 1 //def _XBOX
// on Xbox we prefer a low overhead
static const int allocSizes[]=
{
  #define LINE(x) \
    x*8, x*9, x*10, x*11, x*12, x*13, x*14, x*15
  8, 12, 16, 20, 24, 28, // <0,32>
  LINE(4), // <32,64)
  LINE(8), // <64,128)
  LINE(16), // <128,256)
  LINE(32), // <256,512)
  LINE(64), // <512,1024)
};

//const int bigAllocGranularity = 256;
const int bigAllocGranularity = 1024;
const int smallAllocMax = 64*15;
const int smallAllocGranularity = 4;

#else
// experimental
// when this was used, memory allocation crashed
// because some blocks were allocated and not fully committed
// which caused access violation on block release

// on PC we want to avoid fragmentation
static const int allocSizes[]=
{
  // pow 2 makes sure overhead is less then 50 %
  16,32,64,128,256,512,1024,2048
};

const int smallAllocGranularity = 16;
/// max size handled
const int smallAllocMax = 2048;
/// size which should be handled by regular allocator
const int bigAllocGranularity = 4096;
#endif

const int nAllocSizes=sizeof(allocSizes)/sizeof(*allocSizes);


  
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
  MemCommit _memBlocks; // data area
  MemCommit _memRegions; // data area - used for regions only
  
  /// allocated directly via OS calls
  size_t _allocatedDirect;

  BlockList _blockList;

  FreeList _freeList;

  //! count of free items
  int _nItemsFree;
  //! total size of free items
  MemSize _freeSize;

  MemSize _align;
  MemSize _minAlloc;
  const char *_name;
  //{ FIX
  MemHeap **_lateDestruct;
  //}
  MemoryFreeOnDemandList _freeOnDemand; // memory allocated in the heap
  MemoryFreeOnDemandList _freeOnDemandLowLevel; // memory allocated in the heap
  
  //! here the alloc reside
  SmallMemAlloc *_smallAllocs[nAllocSizes];
  //! pointer only, several pointer to the same allocator possible
  SmallMemAlloc *_smallAllocSelect[smallAllocMax/smallAllocGranularity];
  //! statistics about each allocation size
  int _allocStats[smallAllocMax/smallAllocGranularity+1];
  InfoAlloc _infoAlloc;
  

  int SelectSmallAlloc(MemSize size);

  #if _DEBUG
    mutable InitVal<bool,false> _onDemandListClosed;
    /** after any list traversal the list is assumed to be closed and can never be changed again
    This guarantess thread safety for traversal inside of FreeOnDemandFrame without having to lock anything
    */
    
    void CloseOnDemandList() const {_onDemandListClosed=true;}
  #else
    void CloseOnDemandList() const {}
  #endif
  

  private:
  MemSize AlignSize( MemSize value )
  {
    return (value+_align-1)&~(_align-1);
  }
  MemSize AlignSize( MemSize value, int align )
  {
    return (value+align-1)&~(align-1);
  }

  void InitSmallAllocs();
  void DestructSmallAllocs();

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
  IMemoryFreeOnDemand *GetFirstFreeOnDemand() const {AssertMainThread();return _freeOnDemand.First();}
  IMemoryFreeOnDemand *GetNextFreeOnDemand(IMemoryFreeOnDemand *cur) const {return _freeOnDemand.Next(cur);}

  IMemoryFreeOnDemand *GetFirstFreeOnDemandLowLevel() const {return _freeOnDemandLowLevel.First();}
  IMemoryFreeOnDemand *GetNextFreeOnDemandLowLevel(IMemoryFreeOnDemand *cur) const {return _freeOnDemandLowLevel.Next(cur);}
  //@}

  IMemoryFreeOnDemand *GetFirstFreeOnDemandSystem() const {AssertMainThread();return GetFirstFreeOnDemandSystemMemory();}
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

  
  protected:
  //! find best match for needed size
  FreeBlock *FindBest( MemSize size );
  //! find best match for needed size and alignment
  FreeBlock *FindBest( MemSize size, MemSize align );

  void ReleaseBlock(BusyBlock *busy);
  BusyBlock *AllocateBlock(FreeBlock *best, MemSize size);


  public:
  //! allocate memory block
  void *Alloc( MemSize size, int aligned=-1 );
  //! free memory block
  void Free( void *pos );
  //! get size of given memory block
  MemSize GetSize(void *mem);

  //! Resize a memory block
  void *Resize(void *mem, MemSize size);
  
  FastAlloc *WhichAllocator( void *pos );

  void CleanUp();

  //@{ error reporting - will terminate the application
  void MemoryErrorPage(int size);
  void MemoryError(int size);
  //@}

  void *AllocRegion(MemSize size, MemSize align);
  void FreeRegion(void *region, MemSize size);
  size_t GetRegionRecommendedSize() const;

  int IsFromHeap( void *pos ) const;

  int BlockSize(BusyBlock *cur, BusyBlock *next) const
  {
    char *thisBeg=cur->_memory;
    char *nextBeg=next ? next->_memory : (char *)_memBlocks.End();
    return nextBeg-thisBeg;
  }

  int BlockSize( BusyBlock *cur ) const
  {
    BusyBlock *next=_blockList.Advance(cur);
    char *thisBeg=cur->_memory;
    char *nextBeg=NotEndOf(BusyLink,next,_blockList) ? next->_memory : (char *)_memBlocks.End();
    return nextBeg-thisBeg;
  }

  //void *Memory() const {return _memBlocks.Data();}
  //MemSize Size() const {return _memBlocks.Size();}

  MemSize MaxFreeLeft() const;
  MemSize TotalFreeLeft() const;
  int CountFreeLeft() const;
  /// how much is left for ordinary new
  MemSize NewFreeLeft() const {return _freeSize;}

  MemSize TotalAllocated() const;
  MemSize TotalCommitted() const;
  MemSize TotalReserved() const;
  MemSize CommittedByNew() const {return _memBlocks.GetCommitted();}
  MemSize CommittedByPages() const {return _memRegions.GetCommitted();}
  MemSize CommittedByDirect() const {return _allocatedDirect;}
  
  MemSize LongestFreeForNew() const {return _memBlocks.FindLongestRange()*_memBlocks.GetPageSize();}
  MemSize LongestFreeForPages() const {return _memRegions.FindLongestRange()*_memRegions.GetPageSize();}
  #if _ENABLE_REPORT
  bool Check() const;
  #else
  bool Check() const {return true;}
  #endif

  #if _ENABLE_REPORT
  void ReportTotals() const;
  void LogAllocStats() const;
  size_t InfoAllocAllocated() const;
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

  void Init( const char *name, MemSize size, MemSize align=DefaultAlign )
  {
    base::Init(name,size,align);
  }
  void SetVirtualSpaceMultiplier(int mult)
  {
    SCOPE_LOCK();
    base::SetVirtualSpaceMultiplier(mult);
  }
  void *Alloc( MemSize size, int aligned = -1){SCOPE_LOCK();return base::Alloc(size,aligned);}

  void *Resize(void *mem, MemSize size){SCOPE_LOCK();return base::Resize(mem,size);}

  void Free( void *mem ){SCOPE_LOCK();base::Free(mem);}

  void CleanUp(){SCOPE_LOCK();base::CleanUp();}

  MemSize GetSize(void *mem){ SCOPE_LOCK();return base::GetSize(mem);}

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

  MemSize MaxFreeLeft() const {SCOPE_LOCK();return base::MaxFreeLeft();}
  MemSize TotalFreeLeft() const{SCOPE_LOCK();return base::TotalFreeLeft();}
  int CountFreeLeft() const{SCOPE_LOCK();return base::CountFreeLeft();}
  MemSize NewFreeLeft() const {SCOPE_LOCK();return base::NewFreeLeft();}

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
  MemSize CommittedByDirect() const {/*atomic, no need to lock*/return base::CommittedByDirect();}
  
  MemSize LongestFreeForNew() const {SCOPE_LOCK(); return base::LongestFreeForNew();}
  MemSize LongestFreeForPages() const {SCOPE_LOCK(); return base::LongestFreeForPages();}

  void MemoryErrorPage(int size){SCOPE_LOCK();base::MemoryErrorPage(size);}
  void MemoryError(int size){SCOPE_LOCK();base::MemoryError(size);}

  #if _ENABLE_REPORT
  void ReportTotals() const {SCOPE_LOCK();base::ReportTotals();}
  void LogAllocStats() const {SCOPE_LOCK();base::LogAllocStats();}
  size_t InfoAllocAllocated() const {SCOPE_LOCK();return base::InfoAllocAllocated();}
  
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
