#include <windows.h>
#include "tcmalloc.h"
#include "tcmalloc_guard.h"
#include "google\malloc_extension.h"
#include <new>

namespace nedmalloc {
   extern int VirtualReserved;
};
  
#define DLL_EXPORT __declspec(dllexport)

extern "C" {
DLL_EXPORT size_t __stdcall MemTotalReserved() {mallinfo info = tc_mallinfo(); return info.arena;}
DLL_EXPORT size_t __stdcall MemTotalCommitted() {mallinfo info = tc_mallinfo(); return info.arena - info.fordblks; /* == info.uordblks + info.fsmblks;*/}
DLL_EXPORT size_t __stdcall MemFlushCache(size_t size) {size_t before = tc_mallinfo().arena; MallocExtension::instance()->ReleaseToSystem(size);return before-tc_mallinfo().arena;}
DLL_EXPORT void __stdcall MemFlushCacheAll() {MallocExtension::instance()->ReleaseFreeMemory();}
DLL_EXPORT size_t __stdcall MemSize(void *mem) {return tc_malloc_size(mem);}
DLL_EXPORT void *__stdcall MemAlloc(size_t size) {return tc_malloc(size);}
DLL_EXPORT void __stdcall MemFree(void *mem) {tc_free(mem);}
};


extern "C" BOOL WINAPI DllMain( HINSTANCE hInst, DWORD callReason, LPVOID c)
{
  if (callReason==DLL_PROCESS_ATTACH)
  {
    new (DllMain) TCMallocGuard(); // this not used in TCMallocGuard -only used to call sideeffects functions, we cannot pass NULL
  }
  else if (callReason==DLL_THREAD_DETACH)
  {
  }
  else if (callReason==DLL_PROCESS_DETACH)
  {
    //mallocProcessShutdownNotification();
  }
  return TRUE;
}
