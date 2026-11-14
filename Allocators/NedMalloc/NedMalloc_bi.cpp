#include <windows.h>
#include "nedmalloc.h"

namespace nedmalloc {
   extern int VirtualReserved;
};
  
#define DLL_EXPORT __declspec(dllexport)

extern "C" {
DLL_EXPORT size_t __stdcall MemTotalReserved() {return nedmalloc::VirtualReserved;}
DLL_EXPORT size_t __stdcall MemTotalCommitted() {return nedmalloc::VirtualReserved;}
DLL_EXPORT size_t __stdcall MemFlushCache(size_t size) {size_t before = nedmalloc::VirtualReserved;nedalloc::nedmalloc_trim(0);return before-nedmalloc::VirtualReserved;}
DLL_EXPORT void __stdcall MemFlushCacheAll() {nedalloc::nedmalloc_trim(0);}
DLL_EXPORT size_t __stdcall MemSize(void *mem) {int isforeign;return nedalloc::nedblksize(&isforeign,mem);}
DLL_EXPORT void *__stdcall MemAlloc(size_t size) {return nedalloc::nedmalloc(size);}
DLL_EXPORT void __stdcall MemFree(void *mem) {nedalloc::nedfree(mem);}
// DLL_EXPORT __stdcall void *MemResize(void *mem, size_t size) {return moz_expand(mem,size);} // TODO: consider implementing expand?
};

extern "C" BOOL WINAPI DllMain( HINSTANCE hInst, DWORD callReason, LPVOID c)
{
  if (callReason==DLL_PROCESS_ATTACH)
  {
  }
  else if (callReason==DLL_THREAD_DETACH)
  {
    nedalloc::neddisablethreadcache(NULL);
  }
  else if (callReason==DLL_PROCESS_DETACH)
  {
    //mallocProcessShutdownNotification();
  }
  return TRUE;
}
