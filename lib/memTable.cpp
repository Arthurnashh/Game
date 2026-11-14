// global redefinition of new, delete operators
#include "wpch.hpp"

#if defined _WIN32 && !defined MALLOC_WIN_TEST

#pragma optimize("t",on)
#if !_RELEASE
  // it seems in VS 2005 optimize("t",on) applies /Oy as well (Omit frame pointers)
  // in Testing we need stack frames for memory call-stack and crash-dumps
  #pragma optimize("y",off)
#endif

#include "memHeap.hpp"

#ifdef MFC_NEW // depending on memHeap.hpp content enable or disable whole implementation
#include <Es/Common/win.h>
#include <Es/Files/commandLine.hpp>
#include <El/QStream/qbStream.hpp>
#include <El/Debugging/debugTrap.hpp>
#include <El/Common/perfLog.hpp>
#include <El/Common/perfProf.hpp>
#include <Es/Containers/staticArray.hpp>

#include "memGrow.hpp"

#ifdef _XBOX
  #include <io.h>
  #include <fcntl.h>
  #if _ENABLE_REPORT
  #include <XbDm.h>
  #endif
#endif
//#include "engineDll.hpp"

#include <Es/Memory/normalNew.hpp>

// note: MEM_CHECK makes sure leaks are detected, but if leak source tracking is needed
// you need to use ALLOC_DEBUGGER

#if _PROFILE
  #define MEM_CHECK 0
  #define DO_MEM_STATS 0
  #define DO_MEM_FILL 0
#elif _DEBUG
  #define MEM_CHECK 1
  #define DO_MEM_STATS 0
  #define DO_MEM_FILL 0
#elif _RELEASE
  #define MEM_CHECK 0
  #define DO_MEM_STATS 0
  #if _ENABLE_REPORT
    #define DO_MEM_FILL 1
  #else
    #define DO_MEM_FILL 0
  #endif
#else
  // testing
  #define DO_MEM_FILL 0
  #define MEM_CHECK 1
  #define DO_MEM_STATS 1
#endif


// when using byte, we have little to choose from
#define MEM_GUARD_VAL 0x6a
#define MEM_FREE_VAL 0x7f
#define MEM_NEW_VAL 0x71

// such value is big positive as int, and at the same time it is float signalling NaN
#define MEM_NEW_VAL_32 0x7f8f7f8f
#define MEM_FREE_VAL_32 0x7f817f81


/// alternative to memset, filling memory with a 32b value aligned to 32b
static __forceinline void MemSet32(void *dst, int val32, int size)
{
  // assume memory is 32b aligned
  // if not, fill less then required
  int *d = (int *)dst;
  size>>=2;
  while (--size>=0) *d++ = val32;
}


#if _MSC_VER && !defined INIT_SEG_COMPILER
  // we want Memory Heap to deallocate last
  #pragma warning(disable:4074)
  #pragma init_seg(compiler)
  #define INIT_SEG_COMPILER
#endif

/// should the primary memory heap should be multi-thread safe?
#define MAIN_HEAP_MT_SAFE 1

//{ useAppFrameExt.cpp is embedded here:
//  - LogF is used in MemHeap construction, so it must be defined in compiler init_seg

#include "appFrameExt.hpp"
static OFPFrameFunctions GOFPFrameFunctions INIT_PRIORITY_URGENT;
AppFrameFunctions *CurrentAppFrameFunctions = &GOFPFrameFunctions;

//} End of embedded useAppFrameExt.cpp

#ifndef MemAllocDataStack
  /// define a store for MemAllocDataStack
  DataStack GDataStack(1*1024*1024);
#endif


#ifndef MFC_NEW

#pragma error This allocator is no longer supported, only dummy "MFC" implementation should not be used

#else

MemFunctions OMemFunctions INIT_PRIORITY_URGENT;
MemFunctions OSafeMemFunctions INIT_PRIORITY_URGENT;
MemFunctions *GMemFunctions = &OMemFunctions;
MemFunctions *GSafeMemFunctions = &OSafeMemFunctions;

// no garbage collect or any on demand releasing with default new

void RegisterFreeOnDemandMemory(IMemoryFreeOnDemand *object)
{
}

size_t MemoryFreeOnDemand(size_t size)
{
  return 0;
}

void FreeOnDemandGarbageCollectMain(size_t freeRequired, size_t freeSysRequired, bool isMain)
{
}

mem_size_t GetMemoryUsageLimit(mem_size_t *virtualLimit)
{
  if (virtualLimit) *virtualLimit = INT_MAX;
  return INT_MAX; // no limit - 2GB allowed
}

int FrameId=0;

void FreeOnDemandFrame()
{
  FrameId++;
}

void RegisterFreeOnDemandSystemMemory(IMemoryFreeOnDemand *object)
{
}

void RegisterFreeOnDemandLowLevelMemory(IMemoryFreeOnDemand *obj)
{
}

void FreeOnDemandGarbageCollectSystemMemoryLowLevel(size_t freeSysRequired)
{
}

size_t FreeOnDemandSystemMemoryLowLevel(size_t size)
{
  return 0;
}

size_t FreeOnDemandSystemMemory(
  size_t size, IMemoryFreeOnDemand **extras, int nExtras
)
{
  return 0;
}

void MemoryInit(){}
void MemoryDone(){}
void MemoryFootprint(){}
void PrintVMMap(int extended, const char *title=NULL){}

#endif

#ifdef _XBOX


LPVOID WINAPI XMemAlloc(SIZE_T dwSize, DWORD dwAllocAttributes)
{
  Retry:
  LPVOID mem = XMemAllocDefault(dwSize,dwAllocAttributes);
  if (!mem)
  {
    // we should try releasing some low-level memory
    // note: FreeOnDemandSystemMemoryLowLevel is thread safe
    size_t released = FreeOnDemandSystemMemoryLowLevel(dwSize);
    if (released>0)
    {
      //LogF("XMemAlloc: low-level released %d, requested %d",released,dwSize);
      goto Retry;
    }
    ErrF("XMemAlloc %d failed",dwSize);
    return mem;
  }
  
  #if _ENABLE_CHEATS
    XALLOC_ATTRIBUTES attr = *(XALLOC_ATTRIBUTES *)&dwAllocAttributes;
    SIZE_T memSize = XMemSizeDefault(mem,dwAllocAttributes);
    // lock access to memory tracking stats
    ScopeLock<CriticalSection> lock(XMemoryLock);
    
    int *totAlloc = NULL;
    #if _ENABLE_CHEATS
    int *idAlloc = NULL;
    const char *name = "";
    #endif
    if (attr.dwMemoryType==XALLOC_MEMTYPE_PHYSICAL)
    {
      totAlloc = &XPhysMemoryAllocated;
      #if _ENABLE_CHEATS
      idAlloc = XPhysMemoryAllocatedById;
      name = "Phys";
      #endif
    }
    else
    {
      totAlloc = &XVirtMemoryAllocated;
      #if _ENABLE_CHEATS
      idAlloc = XVirtMemoryAllocatedById;
      name = "Virt";
      #endif
    }
    *totAlloc += memSize;
    #if _ENABLE_CHEATS
      int i = attr.dwAllocatorId-firstTrackedXAId;
      //LogF("Alloc - Memory %s:%d  - %d",name,i,memSize);
      if (i>=0 && i<nTrackedXAIds)
      {
        idAlloc[i]+= memSize;
      }
    #endif
  #endif
  
  return mem;
}

VOID WINAPI XMemFree(PVOID pAddress, DWORD dwAllocAttributes)
{
  // X360 calls XMemFree with NULL during init phase
  if (pAddress==NULL) return;
  #if _ENABLE_CHEATS
  
  XALLOC_ATTRIBUTES attr = *(XALLOC_ATTRIBUTES *)&dwAllocAttributes;

  SIZE_T memSize = XMemSizeDefault(pAddress,dwAllocAttributes);
  
  int *totAlloc = NULL;
  #if _ENABLE_CHEATS
  int *idAlloc = NULL;
  const char *name = "";
  #endif
  ScopeLock<CriticalSection> lock(XMemoryLock);
  if (attr.dwMemoryType==XALLOC_MEMTYPE_PHYSICAL)
  {
    totAlloc = &XPhysMemoryAllocated;
    #if _ENABLE_CHEATS
    idAlloc = XPhysMemoryAllocatedById;
    name = "Phys";
    #endif
  }
  else
  {
    totAlloc = &XVirtMemoryAllocated;
    #if _ENABLE_CHEATS
    idAlloc = XVirtMemoryAllocatedById;
    name = "Virt";
    #endif
  }

  *totAlloc -= memSize;
  if (*totAlloc<0)
  {    
    #if _ENABLE_CHEATS
      LogF("Memory %s underflow %d",name,memSize);
    #endif
    *totAlloc = 0;
  }
  #if _ENABLE_CHEATS
    int i = attr.dwAllocatorId-firstTrackedXAId;
    //LogF("Free  - Memory %s:%d  - %d",name,i,memSize);
    if (i>=0 && i<=nTrackedXAIds)
    {
      idAlloc[i] -= memSize;
      if (idAlloc[i]<0)
      {
        LogF("Memory %s:%d underflow %d",name,i,memSize);
        idAlloc[i] = 0;
      }
    }
  #endif
  
  #endif
  return XMemFreeDefault(pAddress,dwAllocAttributes);
}

SIZE_T WINAPI XMemSize(PVOID pAddress, DWORD dwAllocAttributes)
{
  return XMemSizeDefault(pAddress,dwAllocAttributes);
}
#endif

#if  _ENABLE_REPORT && !defined MFC_NEW

#if defined _XBOX

#include <El/Enum/enumNames.hpp>

extern const EnumName XALLOCEnumNames[];

template <>
const EnumName *GetEnumNames(XALLOC_ALLOCATOR_IDS dummy)
{
  return XALLOCEnumNames;
}

#endif

RString GetMemStat(mem_size_t statId, mem_size_t &statVal)
{
  int id = 0; // compiler will eliminate the ++ done on this into constants
  #ifdef _XBOX
  //int nXALLOCEnumNames = sizeof(XALLOCEnumNames)/sizeof(*XALLOCEnumNames)-1;
  if (statId<(id+=nTrackedXAIds))
  {
    // no lock here:
    // we are only reading, no need for a proper synchronization - we do not mind if the value is a little bit old
    int i = (statId-(id-nTrackedXAIds)); // id now points at the end of the region, need -nTrackedXAIds
    DoAssert(i>=0 && i<nTrackedXAIds);
    statVal = XPhysMemoryAllocatedById[i];
    return RString("P ")+XALLOCEnumNames[i].GetName();
  }
  #endif
  #if 0
  if (statId==id++)
  {
    // no lock here:
    statVal = XPhysMemoryAllocated;
    return "** XPhys";
  }
  if (statId==id++)
  {
    statVal = 0;
    return " ";
  }
  #endif
  #ifdef _XBOX
  if (statId<(id+=nTrackedXAIds))
  {
    // no lock here:
    int i = statId-(id-nTrackedXAIds); // id now points at the end of the region, need -nTrackedXAIds
    DoAssert(i>=0 && i<nTrackedXAIds);
    statVal = XVirtMemoryAllocatedById[i];
    return RString("V ")+XALLOCEnumNames[i].GetName();
  }
  #endif
  #if 0
  if (statId==id++)
  {
    // no lock here:
    statVal = XVirtMemoryAllocated;
    return "** XVirt";
  }
  #endif
  if (statId==id++)
  {
    statVal = 0;
    return " ";
  }
  if (statId==id++)
  {
    statVal = MemoryUsed();
    return "MemAlloc";
  }
  if (statId==id++)
  {
    statVal = MemoryUsedByNew();
    return "MemOvhead";
  }
  if (statId==id++)
  {
    statVal = MemoryFreeBlocks();
    return "MemFragm";
  }
  if (statId==id++)
  {
    statVal = MemoryCommited();
    return "MemCommit";
  }
  if (statId==id++)
  {
    statVal = BMemory->CommittedByPages();
    return "MemCommit (Page)";
  }
  if (statId==id++)
  {
    statVal = BMemory->CommittedByNew();
    return "MemCommit (New)";
  }
  if (statId==id++)
  {
    statVal = BMemory->CommittedByDirect();
    return "MemCommit (Direct)";
  }
  if (statId==id++)
  {
    statVal = BMemory->LongestFreeForPages();
    return "MemLongFree (Page)";
  }
  if (statId==id++)
  {
    statVal = BMemory->LongestFreeForNew();
    return "MemLongFree (New)";
  }
  if (statId==id++)
  {
    statVal = MemoryStoreUsed();
    return "Memory Store";
  }
  if (statId==id++)
  {
    statVal = GMemStore.GetMappedPages()*GMemStore.GetPageSize();
    return "Mapped Store";
  }
  if (statId==id++)
  {
    statVal = safeHeap->TotalReserved();
    return "MP MemResvd";
  }
  if (statId==id++)
  {
    statVal = safeHeap->TotalCommitted();
    return "MP MemCommit";
  }
  if (statId==id++)
  {
    statVal = safeHeap->TotalAllocated();
    return "MP MemAlloc";
  }
  if (statId==id++)
  {
    statVal = 0;
    return " ";
  }
  if (statId==id++)
  {
    MEMORYSTATUS mem;
    mem.dwLength = sizeof(mem);
    GlobalMemoryStatus(&mem);
    statVal = mem.dwAvailVirtual;
    return "Virtual Free";
  }
  if (statId==id++)
  {
    MEMORYSTATUS mem;
    mem.dwLength = sizeof(mem);
    GlobalMemoryStatus(&mem);
    statVal = mem.dwAvailPhys;
    return "Physical Free";
  }

  
  statVal = 0;
  return RString();
}
#endif

#if _ENABLE_REPORT && !defined MFC_NEW
void ReportMemoryTotals()
{
  FastCAlloc::ReportTotals();
  LogF("Fastalloc requested %d",FastCAlloc::TotalRequested());
  LogF("Fastalloc allocated %d",FastCAlloc::TotalAllocated());
  if (BMemory)
  {
    BMemory->ReportTotals();
    BMemory->ReportAnyMemory("");
    // report all basic memory stats
    for( int c=0; c<100; c++ )
    {
      int value;
      RString name = GetMemStat(c,value);
      if (name.GetLength()==0) break;
      if (value==0) continue;
      LogF("%s - %d",(const char *)name,value);
    }
  }
#ifdef _XBOX
  DM_MEMORY_STATISTICS stats;
  stats.cbSize = sizeof(stats);
  DmQueryMemoryStatistics(&stats);
  #define LOG_DMS(x) LogF(#x " %d",stats.x##Pages*4096);
  LOG_DMS(Total);
  LOG_DMS(Available);
  LOG_DMS(Stack);
  LOG_DMS(VirtualPageTable);
  LOG_DMS(SystemPageTable);
  LOG_DMS(Pool);
  LOG_DMS(VirtualMapped);
  LOG_DMS(Image);
  LOG_DMS(FileCache);
  LOG_DMS(Contiguous);
  LOG_DMS(Debugger);
  #undef LOG_DMS
#endif
}
#else
void ReportMemoryTotals()
{
}
#endif

// MT safe heap - equal to GMemFunctions or GSafeMemFunctions depending on configuration
#if MAIN_HEAP_MT_SAFE
  MemFunctions *GMTMemFunctions = GMemFunctions; 
#else
  MemFunctions *GMTMemFunctions = GSafeMemFunctions; 
#endif

size_t TotalAllocatedWin()
{
#ifdef MFC_NEW
  return 0;
#else
  if( !BMemory ) return 0;
  return BMemory->TotalAllocated();
#endif
}

#endif // enable / disable whole implementation

#endif //_win32

