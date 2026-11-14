#include <Es/essencepch.hpp>
#include "checkMem.hpp"
#include <Es/Common/win.h>

#pragma warning(disable:4074)
#pragma init_seg(compiler)

#if defined _WIN32 && !MFC_NEW
class MemFunctionsWin32: public MemFunctions
{
  int _pageSize;
  int _allocationGranularity;
  #if _DEBUG
  int _allocated;
  
  void CountAlloc(int n){_allocated+=n;}
  #else
  void CountAlloc(int n){}
  #endif
  
  public:
  MemFunctionsWin32()
  {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    // we do not need to know page size exactly,
    // but if we know it, we can better commit pages
    // we do not care if actual page size is power of two
    // we ony want to be sure that our _pageSize is power of two
    _pageSize=info.dwPageSize;
    _allocationGranularity=info.dwAllocationGranularity;
  }
  /**
  Note: this call has a significant virtual space overhead.
  For each _pageSize (4 KB) allocated there is _allocationGranularity (64 KB) of virtual space is used.
  */
  virtual void *NewPage(size_t size, size_t align) {CountAlloc(size/_pageSize);return VirtualAlloc(NULL,_pageSize,MEM_COMMIT,PAGE_READWRITE);}
  virtual void DeletePage(void *page, size_t size) {CountAlloc(-int(size/_pageSize));VirtualFree(page,0,MEM_RELEASE);}
  virtual size_t GetPageRecommendedSize() {return _pageSize;}
};

static MemFunctionsWin32 MemFunctionsMalloc INIT_PRIORITY_URGENT;

MemFunctions *GMemFunctions = &MemFunctionsMalloc;
MemFunctions *GSafeMemFunctions = &MemFunctionsMalloc;

#else
static MemFunctions MemFunctionsMFCNew INIT_PRIORITY_URGENT;

MemFunctions *GMemFunctions = &MemFunctionsMFCNew;
MemFunctions *GSafeMemFunctions = &MemFunctionsMFCNew;

#endif

#ifdef _WIN32

size_t TotalAllocatedWin()
{
#ifdef MFC_NEW
  return 0;
#else
  //#include <crtdbg.h>
  //_CrtMemState state;
  //_CrtMemCheckpoint(&state);
  //return state.lTotalCount;
  // check: is this needed or used by anyone?
  return 0;
#endif
}

#endif
