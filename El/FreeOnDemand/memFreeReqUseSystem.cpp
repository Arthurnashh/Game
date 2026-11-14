#include "El/elementpch.hpp"
#include <Es/Containers/staticArray.hpp>
#include "memFreeReq.hpp"
#include <Es/Common/win.h>
#include "Es/Framework/netlog.hpp"

#if (_ENABLE_REPORT || !defined(_XBOX)) && defined _WIN32
#if defined(NET_LOG) && defined(EXTERN_NET_LOG)
#  ifdef NET_LOG_PERIOD
NetLogger netLogger(NET_LOG_PERIOD);
#  else
NetLogger netLogger;
#  endif
#endif
#endif

#if !defined _WIN32
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <malloc.h>
#endif

#pragma warning(disable:4073)
#pragma init_seg(lib)

// free on demand global list
static MemoryFreeOnDemandList GMemoryFreeOnDemandList INIT_PRIORITY_HIGH;

void RegisterFreeOnDemandMemory(IMemoryFreeOnDemand *object)
{
	GMemoryFreeOnDemandList.Register(object);
}

IMemoryFreeOnDemand *GetFirstFreeOnDemandMemory()
{
	return GMemoryFreeOnDemandList.First();
}
IMemoryFreeOnDemand *GetNextFreeOnDemandMemory(IMemoryFreeOnDemand *cur)
{
	return GMemoryFreeOnDemandList.Next(cur);
}

TypeIsSimple(IMemoryFreeOnDemand *)

size_t FreeOnDemandMemory(size_t size, IMemoryFreeOnDemand **extras, int nExtras)
{
	// return GMemoryFreeOnDemandList.Free(size);
  // note: allocation here is extremely dangerous
  // we need to avoid it at all costs
  AUTO_STATIC_ARRAY(OnDemandStats,stats,128);
  AUTO_STATIC_ARRAY(IMemoryFreeOnDemand *,lowLevel,16);

  for(int i=0; i<nExtras; i++)
  {
    if (!extras[i]) continue;
    OnDemandStats &mstat = stats.Append();
    mstat.mem = extras[i];
    mstat.system = false;
  }
  // BalanceList needs to get information about low level memory so that it can keep a space for it
  for(IMemoryFreeOnDemand *walk=GetFirstFreeOnDemandLowLevelMemory(); walk; walk = GetNextFreeOnDemandLowLevelMemory(walk))
  {
    lowLevel.Add(walk);
  }
  for(IMemoryFreeOnDemand *walk=GetFirstFreeOnDemandMemory(); walk; walk = GetNextFreeOnDemandMemory(walk))
  {
    OnDemandStats &mstat = stats.Append();
    mstat.mem = walk;
    mstat.system = false;
  }
  //FreeOnDemandSystemMemory is equivalent to the FreeOnDemandMemory, so this is removed
  //for(IMemoryFreeOnDemand *walk=GetFirstFreeOnDemandSystemMemory(); walk; walk = GetNextFreeOnDemandSystemMemory(walk))

  mem_size_t systemReleased;
  return MemoryFreeOnDemandList::BalanceList(size,stats.Data(),stats.Size(),lowLevel.Data(),lowLevel.Size(),&systemReleased);
}

/// Free on Demand System memory
// It uses the same list as the FreeOnDemandMemory

void RegisterFreeOnDemandSystemMemory(IMemoryFreeOnDemand *object)
{
  GMemoryFreeOnDemandList.Register(object);
}

IMemoryFreeOnDemand *GetFirstFreeOnDemandSystemMemory()
{
  return GMemoryFreeOnDemandList.First();
}

IMemoryFreeOnDemand *GetNextFreeOnDemandSystemMemory(IMemoryFreeOnDemand *cur)
{
  return GMemoryFreeOnDemandList.Next(cur);
}

/// Free on Demand Low Level Memory
static MemoryFreeOnDemandList GLowLevelMemoryFreeOnDemandList INIT_PRIORITY_HIGH;

void RegisterFreeOnDemandLowLevelMemory(IMemoryFreeOnDemand *object)
{
  GLowLevelMemoryFreeOnDemandList.Register(object);
}

IMemoryFreeOnDemand *GetFirstFreeOnDemandLowLevelMemory()
{
  return GLowLevelMemoryFreeOnDemandList.First();
}
IMemoryFreeOnDemand *GetNextFreeOnDemandLowLevelMemory(IMemoryFreeOnDemand *cur)
{
  return GLowLevelMemoryFreeOnDemandList.Next(cur);
}

size_t FreeOnDemandLowLevelMemory(size_t size)
{
  // balanced releasing of all kinds of memory
  // note: allocation here is extremely dangerous
  AUTO_STATIC_ARRAY(OnDemandStats,stats,128);

  for(IMemoryFreeOnDemand *walk=GetFirstFreeOnDemandLowLevelMemory(); walk; walk = GetNextFreeOnDemandLowLevelMemory(walk))
  {
    OnDemandStats &mstat = stats.Append();
    mstat.mem = walk;
    mstat.system = false;
  }
  mem_size_t systemReleased;
  size_t released = MemoryFreeOnDemandList::BalanceList(size,stats.Data(),stats.Size(),NULL,0,&systemReleased);
#if 0 // _ENABLE_REPORT
  if (released>0)
  {
    LogF("Low level memory release forced - %d B",released);
  }
#endif
  return released;
}

/*
 * Already declared as extern and defined in memTable.cpp
// Free On Demand Frame
static int FrameId=0;
*/

void FreeOnDemandFrame()
{
  typedef IMemoryFreeOnDemand *GetFirstMemoryFreeOnDemandF();
  typedef IMemoryFreeOnDemand *GetNextMemoryFreeOnDemandF(IMemoryFreeOnDemand *cur);
  const static struct 
  {
    GetFirstMemoryFreeOnDemandF *first;
    GetNextMemoryFreeOnDemandF *next;
    const char *name;
  } onDemandFcs[]=
  {
    {GetFirstFreeOnDemandMemory,GetNextFreeOnDemandMemory,"Heap"},
    {GetFirstFreeOnDemandLowLevelMemory,GetNextFreeOnDemandLowLevelMemory,"LL"},
  };
  for (int i=0; i<sizeof(onDemandFcs)/sizeof(*onDemandFcs); i++)
  {
    for (
      IMemoryFreeOnDemand *mem = onDemandFcs[i].first();
      mem;
      mem = onDemandFcs[i].next(mem)
    )
    {
      mem->MemoryControlledFrame();
    }
  }
  FrameId++;
}

// Getting Memory info

static int ThrottleMemoryUsage;
int DetectHeapSizeMB(int maxmem=1024) //1GB as default
{
  // assume OS and disk cache will need around 1/4 of total memory
  int dwTotalPhys;
#if defined _WIN32
  // note: memory allocation is not available yet, command line arguments are not parsed yet
  MEMORYSTATUS memstat;
  memstat.dwLength = sizeof(memstat);
  GlobalMemoryStatus(&memstat);
  dwTotalPhys = memstat.dwTotalPhys;
#else
  struct sysinfo sys_info;
  if(sysinfo(&sys_info) != 0) 
  {
    //perror("sysinfo");
    dwTotalPhys = 512*1024*1024; //512 MBytes?
  }
  else
  {
    //sys_info.totalram, sys_info.freeram
    long long llTotalPhys = (long long)sys_info.totalram * sys_info.mem_unit;
    dwTotalPhys = (llTotalPhys > INT_MAX) ? INT_MAX : (int)llTotalPhys;
  }
#endif

  // always try to leave at least 32 MB free for OS
  const int minKeepForOs = 32*1024*1024;  
  int keepForOS = dwTotalPhys/4;
  if (keepForOS<minKeepForOs) keepForOS = minKeepForOs;

  ThrottleMemoryUsage = dwTotalPhys-keepForOS;
  if (ThrottleMemoryUsage > maxmem*1024*1024) ThrottleMemoryUsage = maxmem*1024*1024;

  return dwTotalPhys/1024/1024;
}

mem_size_t GetMemoryUsageLimit(mem_size_t *virtualLimit)
{
  (void *)virtualLimit; //force no warning
  return ThrottleMemoryUsage;
}

size_t FreeOnDemandSystemMemoryLowLevel(size_t size)
{
  return FreeOnDemandLowLevelMemory(size);
}

void FreeOnDemandGarbageCollectSystemMemoryLowLevel(size_t freeSysRequired)
{
  // This function has nonempty body only for XBox on WIN32
}

// Main Free on Demand function

#define PROFILE_MALLINFO 0
#if PROFILE_MALLINFO
#ifndef _WIN32
#include <time.h>
__int64 ProfileTime()
{
  timespec time1;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time1);
  return (__int64)(time1.tv_nsec) + (__int64)(1000000000)*(__int64)(time1.tv_sec);
}
#endif
#endif

void FreeOnDemandGarbageCollectMain(size_t freeRequired, size_t freeSysRequired, bool isMain)
{
  {
    // first of all make sure we stay within desired limits
#ifdef _WIN32
    int overcharge = (int)TotalAllocatedWin()-ThrottleMemoryUsage;
#else
    static int iAmLazyCount = 0;
    size_t totalAllocated = 0;

    if (!iAmLazyCount)
    {
#if PROFILE_MALLINFO
      static int count = 0;
      static int timeSUM = 0;
      __int64 startTime = ProfileTime();
#endif

    struct mallinfo mall;
    mall = mallinfo();
      totalAllocated = mall.uordblks + mall.hblkhd; //mem + mmap (both affects available Address Space limits)

#if PROFILE_MALLINFO
      __int64 endTime = ProfileTime();
      timeSUM += (endTime-startTime);
      if (++count >= 100)
      {
        count = 0;
        RptF("100x mallinfo lasts %.3f ms", timeSUM/1000000.0f);
        timeSUM = 0;
      }
#endif
    }
    iAmLazyCount--; // making it negative when there was mallinfo call

    int overcharge = totalAllocated - ThrottleMemoryUsage;
#endif
    if (overcharge>0)
    {
      IMemoryFreeOnDemand *extras[32];
      int nExtras = 0;
      for (
        IMemoryFreeOnDemand *extra=GetFirstFreeOnDemandLowLevelMemory();
        extra;
        extra = GetNextFreeOnDemandLowLevelMemory(extra)
      )
      {
        if (nExtras>=32)
        {
          Fail("Too many low-level allocators");
          break;
        }
        extras[nExtras++] = extra;
      }

      //extras[nExtras++] = ProgressGetFreeOnDemandInterface();
      size_t released = FreeOnDemandMemory(overcharge,extras,nExtras);
      if (released==0)
      {
        LogF("Warning: No overcharged memory can be released");
      }
    }
#ifndef _WIN32
    else if (iAmLazyCount<0) //there was mallinfo call
    { // memory was tested but there is space available
      __int64 availMem = -overcharge;
      int percentleft = (availMem*100)/ThrottleMemoryUsage;
      if (percentleft<=5) iAmLazyCount = 0; //test often
      else iAmLazyCount = std::min(percentleft-5, 15);
  }
#endif
 }
/*
  // we want the balancing to be called at least once to make sure low-level memory is not exhausted
  // however, we want to do this only once per frame, that is in the "isMain=true" call
  bool balanceAtLeastOnce = isMain;
  for(;;)
  {
    // check how much memory is free now
#ifdef _WIN32
    MEMORYSTATUS memstat;
    memstat.dwLength = sizeof(memstat);
    GlobalMemoryStatus(&memstat);
    // on PC it has no sense to track physical memory at all
    // however, even virtual memory is a scarce resource (nowadays often even more than the physical one)
    size_t sysFree = memstat.dwAvailVirtual;
#else
    size_t virtMax = 2*1024*1024*1024; //2GBytes
    struct rlimit rlp;
    if (!getrlimit (RLIMIT_DATA, &rlp))  //?? or RLIMIT_AS (address space) ??
      virtMax = rlp.rlim_cur;
    struct mallinfo mall;
    mall = mallinfo();
    size_t memUsed = mall.uordblks + mall.hblkhd; //mem + mmap (both affects available Address Space limits)
    size_t sysFree = virtMax - memUsed;
#endif

    // if we have enough memory, we can break
    if (sysFree>=freeSysRequired)
    {
      break;
    }
    balanceAtLeastOnce = false;
  }
*/
}

/// Global Functions with an Empty implementation
size_t FreeOnDemandSystemMemory(size_t size, IMemoryFreeOnDemand **extras, int nExtras)
{
  return 0;
}

RString GetMemStat(int statId, int &statVal)
{
  statVal = 0;
  return RString();
}

void MemoryInit()
{
  DetectHeapSizeMB();
}

void MemoryInit(int maxmem)
{
  DetectHeapSizeMB(maxmem);
}

void MemoryDone(){}
void MemoryFootprint(){}
#ifndef MFC_NEW
void MemoryCleanUp(){}
#endif

size_t FreeOnDemandReleaseSlot(const char *id, size_t toRelease, size_t *verifyRelease)
{
  //???
  return 0;
}
void ChangeMemoryUsageLimit(int memLimit)
{
  //used only as the reaction to GInput.GetCheatXToDo(CXTMemLimit)
} 

void ReportMemoryStatus(int level)
{
}

void ReportMemoryStatus()
{
  ReportMemoryStatus(0);
}

#ifdef _WIN32
size_t TotalAllocatedWin()
{
  MEMORYSTATUS memstat;  memstat.dwLength = sizeof(memstat);
  GlobalMemoryStatus(&memstat);
  return (memstat.dwTotalVirtual - memstat.dwAvailVirtual);
}
#endif

void MemoryMainDone(){}
void MemoryMainInit(){}
