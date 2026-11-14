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

#include <Es/Common/win.h>
#include <Es/Files/commandLine.hpp>
#include <El/QStream/qbStream.hpp>
#include <El/Debugging/debugTrap.hpp>
#include <El/Common/perfLog.hpp>
#include <El/Common/perfProf.hpp>
#include <Es/Containers/staticArray.hpp>
#include <es/Framework/appFrame.hpp>

#include "memGrow.hpp"

//#include "engineDll.hpp"

#include <El/Debugging/imexhnd.h>

static MemoryFreeOnDemandList GFreeOnDemandSystem; // memory allocated by GlobalAlloc, malloc, etc..

void RegisterFreeOnDemandSystemMemory(IMemoryFreeOnDemand *object)
{
  GFreeOnDemandSystem.Register(object);
}

IMemoryFreeOnDemand *GetFirstFreeOnDemandSystemMemory()
{
  return GFreeOnDemandSystem.First();
}
IMemoryFreeOnDemand *GetNextFreeOnDemandSystemMemory(IMemoryFreeOnDemand *cur)
{
  return GFreeOnDemandSystem.Next(cur);
}

TypeIsSimple(IMemoryFreeOnDemand *)

//! balance both system and heap memory at the same time
mem_size_t FreeOnDemandAnyMemory(
  mem_size_t size, bool physical, mem_size_t &systemReleased,
  IMemoryFreeOnDemand **extras, int nExtras
)
{
  // note: allocation here is extremely dangerous
  // we need to avoid it at all costs
  AUTO_STATIC_ARRAY(OnDemandStats,stats,128);
  AUTO_STATIC_ARRAY(IMemoryFreeOnDemand *,lowLevel,16);

  const Memory::LockedFreeOnDemandEnumerator &demand = Memory::GetFreeOnDemandEnumerator();
  
  for(int i=0; i<nExtras; i++)
  {
    if (!extras[i]) continue;
    if (!physical && !extras[i]->UsesVirtualMemory()) continue;
    OnDemandStats &mstat = stats.Append();
    mstat.mem = extras[i];
    mstat.system = false;
  }
  // BalanceList needs to get information about low level memory so that it can keep a space for it
  for(IMemoryFreeOnDemand *walk=demand.GetFirstFreeOnDemandLowLevel(); walk; walk = demand.GetNextFreeOnDemandLowLevel(walk))
  {
    if (!physical && !walk->UsesVirtualMemory()) continue;
    lowLevel.Add(walk);
  }
  for(IMemoryFreeOnDemand *walk=demand.GetFirstFreeOnDemand(); walk; walk = demand.GetNextFreeOnDemand(walk))
  {
    if (!physical && !walk->UsesVirtualMemory()) continue;
    OnDemandStats &mstat = stats.Append();
    mstat.mem = walk;
    mstat.system = false;
  }
  for(IMemoryFreeOnDemand *walk=GetFirstFreeOnDemandSystemMemory(); walk; walk = GetNextFreeOnDemandSystemMemory(walk))
  {
    if (!physical && !walk->UsesVirtualMemory()) continue;
    OnDemandStats &mstat = stats.Append();
    mstat.mem = walk;
    mstat.system = true;
  }
  
  return MemoryFreeOnDemandList::BalanceList(size,stats.Data(),stats.Size(),lowLevel.Data(),lowLevel.Size(),&systemReleased);
}

void PrintVMMap(int extended, const char *title=NULL);
void ReportMemoryStatus();
IMemoryFreeOnDemand *ProgressGetFreeOnDemandInterface();

size_t FreeOnDemandSystemMemoryLowLevel(size_t size)
{
  PROFILE_SCOPE_EX(gbSML,mem);
  size_t released = 0;
  size_t pageSize = GetPageRecommendedSize();
  while (size>0)
  {
    // try decommitting some unused heap memory
    size_t toRelease = size;
    if (toRelease<pageSize) toRelease = pageSize;
    size_t sizeFree = Memory::FreeSystemMemory(toRelease);
    if (sizeFree==0)
    {
      // no system memory released - we need to release some low-level releasable memory
      // note: as we are releasing directly from the memory store, it means we are decomitting it directly
      // even if we would be wrong, the caller would fail again, call us again, and we would fix it by decomitting
      sizeFree = FreeOnDemandLowLevelMemory(toRelease);
      if (sizeFree==0)
      {
        // nothing more to release
        break;
      }
    }
    if (sizeFree<size)
    {
      size -= sizeFree;
      released += sizeFree;
    }
    else
    {
      size = 0;
      released += sizeFree;
    }
  }
  #if _ENABLE_REPORT
    if (released<=0)
    {
      ReportMemoryStatus();
      // report how much memory is allocated in various containers
      //ReportAnyMemory("No low-level sysmem to release");
    }
  #endif
  return released;
}

void FreeOnDemandGarbageCollectSystemMemoryLowLevel(size_t freeSysRequired)
{
}

size_t FreeOnDemandSystemMemory(
  size_t size, IMemoryFreeOnDemand **extras, int nExtras
)
{
  PROFILE_SCOPE_EX(gbSM,mem);
  mem_size_t released = 0;
  size_t pageSize = GetPageRecommendedSize();
  while (size>0)
  {
    // try decommitting some unused heap memory
    size_t toRelease = size;
    if (toRelease<pageSize) toRelease = pageSize;
    size_t sizeFree = Memory::FreeSystemMemory(toRelease);
    released += sizeFree;
    if (sizeFree<size)
    {
      size -= sizeFree;
    }
    else
    {
      size = 0;
      break;
    }
    
    if (!sizeFree)
    {
      // check how much system memory was released during this operation
      mem_size_t toRelease = size;
      if (toRelease<pageSize) toRelease = pageSize;
      mem_size_t systemReleased;
      mem_size_t totalReleased = FreeOnDemandAnyMemory(toRelease,false,systemReleased,extras,nExtras);
      released += systemReleased;
      if (systemReleased<size)
      {
        size -= systemReleased;
      }
      else
      {
        size = 0;
        break;
      }
      if (!totalReleased)
      {
        // nothing more to release
        break;
      }
    }
  }
  #if _ENABLE_REPORT
    if ((int)size>0 && released<=0)
    {
      // report how much memory is allocated in various containers
      Memory::ReportAnyMemory("No sysmem to release");
    }
  #endif
  return MemSizeTToSizeT(released);
}

mem_size_t ThrottleMemoryUsagePhysical;
mem_size_t ThrottleMemoryUsageVirtual;


template <typename TypeA, typename TypeMin, typename TypeMax>
inline void saturate( TypeA &a, TypeMin min, TypeMax max )
{
  if( a<min ) a=min;
  if( a>max ) a=max;
}

void ChangeMemoryUsageLimit(int memLimit)
{
  const int oneMB = mem_size_t(1024*1024);
  ThrottleMemoryUsagePhysical += memLimit*16*oneMB;
  ThrottleMemoryUsageVirtual += memLimit*16*oneMB;
  saturate(ThrottleMemoryUsagePhysical,512*oneMB,2047*oneMB);
  saturate(ThrottleMemoryUsageVirtual,512*oneMB,2047*oneMB);
  DIAG_MESSAGE(2000,Format("Heap limit %lld MB, Virtual %lld MB",ThrottleMemoryUsagePhysical/oneMB,ThrottleMemoryUsageVirtual/oneMB));
}

mem_size_t GetMemoryUsageLimit(mem_size_t *virtualLimit)
{
  if (virtualLimit) *virtualLimit = ThrottleMemoryUsageVirtual;
  return ThrottleMemoryUsagePhysical;
}

// command line context
struct MemArgumentContext
{
  int maxSize;
};

static int ExThreads = ~0;

bool CheckExThreads(int mask, bool autodetected)
{
  if (ExThreads==~0) return autodetected;
  return (ExThreads&mask)!=0;
}

/// check if an option with arguments matches
static bool IsOptionWArgs(const char *beg, const char *opt)
{
  // when this is used, the last character of opt should be '=', like in -x=800
  Assert(opt[0]!=0 && opt[strlen(opt)-1]=='=');
  return !strnicmp(beg,opt,strlen(opt));
}

static bool IsOption(const char *beg, const char *end, const char *opt)
{
  return end-beg==(int)strlen(opt) && !strnicmp(beg,opt,end-beg);
}

bool MemoryValidator;

static bool MemSingleArgument(const CommandLine::SingleArgument &arg, MemArgumentContext &ctx)
{
  // check if arg is recognized
  const char *beg = arg.beg;
  const char *end = arg.end;

  if( end==beg ) return false;
  if( *beg=='-' || *beg=='/' )
  {
    beg++;
    // option
    static const char maxmem[]="maxmem=";
    static const char memoryvalidator[]="mv";
    static const char exThreads[]="exthreads=";

    if( !strnicmp(beg,maxmem,strlen(maxmem)) )
    {
      ctx.maxSize = atoi(beg+strlen(maxmem));
    }
    if (IsOption(beg,end,memoryvalidator))
    {
      MemoryValidator = true;
    }
    else if (IsOptionWArgs(beg,exThreads))
      ExThreads=atoi(beg + strlen(exThreads));
  }
  return false;
}

/*!
\patch 5123 Date 1/29/2007 by Ondra
- Improved: Increased internal cache size for computers with more than 512 MB of RAM.
*/

int DetectHeapSizeMB()
{
  #if defined _WIN32
  // 768 seems to be the most stable setting available
  int sizeMBPhys = 768;
  int sizeMBVirt = 768;
    MEMORYSTATUSEX memstat;
    memstat.dwLength = sizeof(memstat);
    GlobalMemoryStatusEx(&memstat);
    if (memstat.ullTotalPhys>1024*1024*1024)
    {
      // autodetection changes only physical memory limit, not the virtual one
      int totalMB = int(memstat.ullTotalPhys>>20);
      int sizeMBPage = int(memstat.ullAvailPageFile>>20);
      sizeMBPhys = (totalMB-512)*2/3;
      saturate(sizeMBPhys,768,8*1024);
      // never commit more than allowed by a page file
      if (sizeMBPhys>sizeMBPage)
      {
        RptF("Warning: memory usage limited by a page file. Current limit %d MB, wanted %d MB.",sizeMBPage,sizeMBPhys);
        RptF("Increasing your page file size might improve game performance.");
        sizeMBPhys = sizeMBPage;
      }
    }
    if (memstat.ullAvailVirtual>3ULL*1024*1024*1024)
    {
      // allow 1.5 GB of virtual space on x64 with LARGEADDRESSAWARE
      sizeMBVirt = 1024+512;
    }

	// autoselection can be overridden from the command line
  CommandLine cmd(GetCommandLine());
  cmd.SkipArgument(); // skip exe name
  MemArgumentContext ctx;
  ctx.maxSize = 0;
  ForEach<CommandLineForEachTraits>(cmd,MemSingleArgument,ctx);
  if (ctx.maxSize)
  {
    sizeMBPhys = ctx.maxSize;
    sizeMBVirt = ctx.maxSize/4*3;
  }
  // TODO: allow more than 32b including the file cache
  saturate(sizeMBPhys,256,2047);
  saturate(sizeMBVirt,256,2047);
  ThrottleMemoryUsagePhysical = mem_size_t(sizeMBPhys)*1024*1024;
  // do not use more than 1 GB of virtual memory unless running on x64 with LARGEADDRESSAWARE
  size_t maxVirtual = 1024;
  if (memstat.ullTotalVirtual>3U*1024*1024*1024) maxVirtual = 2*1024;
  ThrottleMemoryUsageVirtual = intMin(sizeMBVirt,maxVirtual)*mem_size_t(1024*1024);
  return sizeMBVirt;
#else
  return 512;
#endif
}

mem_size_t MemoryStoreUsed();


void FreeOnDemandGarbageCollectMain(size_t freeRequired, size_t freeSysRequired, bool isMain)
{
  #if MEM_PERF_LOG
    PROFILE_SCOPE_EX(gbCol,*);
  #endif
  {
    // first of all make sure we stay within desired limits
    mem_size_t overchargePhysical = Memory::TotalAllocated()+MemoryStoreUsed()-ThrottleMemoryUsagePhysical;
    mem_size_t overchargeVirtual = Memory::TotalAllocated()-ThrottleMemoryUsageVirtual;

    if (overchargePhysical>0 || overchargeVirtual>0)
    {
      const Memory::LockedFreeOnDemandEnumerator &demand = Memory::GetFreeOnDemandEnumerator();
      IMemoryFreeOnDemand *extras[32];
      int nExtras = 0;
      for (
        IMemoryFreeOnDemand *extra=demand.GetFirstFreeOnDemandLowLevel();
        extra;
        extra = demand.GetNextFreeOnDemandLowLevel(extra)
      )
      {
        if (nExtras>=32)
        {
          Fail("Too many low-level allocators");
          break;
        }
        extras[nExtras++] = extra;
      }
      
      extras[nExtras++] = ProgressGetFreeOnDemandInterface();
      if (overchargeVirtual>0)
      {
        mem_size_t released = FreeOnDemandMemory(overchargeVirtual,false,extras,nExtras);
        if (released==0)
        {
          LogF("Warning: No overcharged memory can be released");
        }
      }
      else if (overchargePhysical>0)
      {
        mem_size_t released = FreeOnDemandMemory(overchargePhysical,true,extras,nExtras);
        if (released==0)
        {
          LogF("Warning: No overcharged memory can be released");
        }
      }
    }
    mem_size_t overcommit = Memory::TotalCommitted()+MemoryStoreUsed()-ThrottleMemoryUsagePhysical;
    if (overcommit>0)
    {
      // TODO: on PC is has no sense to free memory which is not committed by our manager at all
      size_t released = Memory::FreeSystemMemory(overcommit);
      if (released==0)
      {
        //LogF("Warning: No overcommitted memory can be released");
      }
    }
  }
  // we want the balancing to be called at least once to make sure low-level memory is not exhausted
  // however, we want to do this only once per frame, that is in the "isMain=true" call
  bool balanceAtLeastOnce = isMain;
  for(;;)
  {
    // no regular balancing on PC, toFree has no sense here
    int toFree = 0;
    if (toFree>0)
    {
      // we need to free some main heap memory
      IMemoryFreeOnDemand *extras[1]= {ProgressGetFreeOnDemandInterface()};
      balanceAtLeastOnce = false;
      mem_size_t released = FreeOnDemandMemory(toFree,true,extras,1);
      if (released==0)
      {
        LogF("Warning: No more main heap memory can be released");
        break;
      }
    }
    else
    {
       if (Memory::CheckVirtualFree(16*1024*1024))
          break;

        #if _DEBUG || _PROFILE
          ReportMemoryStatus();
        #endif
        // if VirtualAlloc has failed, we need to free something
        // pretend we have some less memory than needed to guarantee this
        size_t toFree = 4*1024*1024;
      
      // assume low-level memory can be seen as free for main heap operations,
      // but not for system heap
      
      IMemoryFreeOnDemand *extras[32];
      int nExtras = 0;
      const Memory::LockedFreeOnDemandEnumerator &demand = Memory::GetFreeOnDemandEnumerator();
      for (
        IMemoryFreeOnDemand *extra=demand.GetFirstFreeOnDemandLowLevel();
        extra;
        extra = demand.GetNextFreeOnDemandLowLevel(extra)
      )
      {
        if (nExtras>=32)
        {
          Fail("Too many low-level allocators");
          break;
        }
        extras[nExtras++] = extra;
      }
      
      extras[nExtras++] = ProgressGetFreeOnDemandInterface();
      balanceAtLeastOnce = false;
      mem_size_t released = FreeOnDemandMemory(toFree,false,extras,nExtras);
      if (released==0)
      {
        LogF("Warning: No more system memory can be released");
        ReportMemoryStatus();
        break;
      }
    }
  }
  #if 0
  if (watchGC) ReportGC("After GC");
  #endif
  #if 0 //_ENABLE_REPORT
  lastNewFreeLeft = BMemory->NewFreeLeft();
  #endif
}


size_t FreeOnDemandSystemMemoryAll()
{
  return GFreeOnDemandSystem.FreeAll();
}


/**
@param physical do we need to release a memory which is physical only (no virtual addresses used?)
*/

mem_size_t FreeOnDemandMemory(mem_size_t size, bool physical, IMemoryFreeOnDemand **extras, int nExtras)
{
  PROFILE_SCOPE_EX(gbMem,mem);
  // balanced releasing of all kinds of memory
  mem_size_t systemReleased;
  mem_size_t released = FreeOnDemandAnyMemory(size,physical,systemReleased,extras,nExtras);

  #if _ENABLE_REPORT
    if ((int)size>0 && released<=0)
    {
      // report how much memory is allocated in various containers
      Memory::ReportAnyMemory("No mem to release");
    }
  #endif
  return released;
}




#else

// Embed the appFrameExt
#if _MSC_VER && !defined INIT_SEG_COMPILER
// we want Memory Heap to deallocate last
#pragma warning(disable:4074)
#pragma init_seg(compiler)
#define INIT_SEG_COMPILER
#endif

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

// MT safe heap - equal to GMemFunctions or GSafeMemFunctions depending on configuration
#define MAIN_HEAP_MT_SAFE 1
#if MAIN_HEAP_MT_SAFE
MemFunctions *GMTMemFunctions = GMemFunctions; 
#else
MemFunctions *GMTMemFunctions = GSafeMemFunctions; 
#endif

int FrameId=0;

#endif //_win32

