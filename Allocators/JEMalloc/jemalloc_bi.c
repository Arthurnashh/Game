#include <windows.h>
#ifndef __cplusplus
typedef BOOL bool;
#endif
#include "jemalloc.h"

#define DLL_EXPORT __declspec(dllexport)

DLL_EXPORT size_t __stdcall MemTotalReserved() {jemalloc_stats_t stats;jemalloc_stats(&stats);return stats.mapped;}
DLL_EXPORT size_t __stdcall MemTotalCommitted() {jemalloc_stats_t stats;jemalloc_stats(&stats);return stats.committed;}
DLL_EXPORT size_t __stdcall MemFlushCache(size_t size) {/* TODO: purge arenas */return 0;}
DLL_EXPORT void __stdcall MemFlushCacheAll() {/*No cleanup seems to be needed for JEMalloc*/}
DLL_EXPORT size_t __stdcall MemSize(void *mem) {return malloc_usable_size(mem);}
DLL_EXPORT void *__stdcall MemAlloc(size_t size) {return moz_malloc(size);}
DLL_EXPORT void __stdcall MemFree(void *mem) {moz_free(mem);}
// DLL_EXPORT __stdcall void *MemResize(void *mem, size_t size) {return moz_expand(mem,size);} // TODO: consider implementing expand?

BOOL WINAPI DllMain( HINSTANCE hInst, DWORD callReason, LPVOID c)
{
  if (callReason==DLL_PROCESS_ATTACH)
  {
    malloc_init_hard();
  }
  else if (callReason==DLL_THREAD_DETACH)
  {
    //mallocThreadShutdownNotification(NULL);
  }
  else if (callReason==DLL_PROCESS_DETACH)
  {
    //mallocProcessShutdownNotification();
  }
  return TRUE;
}
