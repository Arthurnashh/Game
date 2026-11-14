#include <Es/essencepch.hpp>
#include <Es/platform.hpp>
#include <Es/Framework/debugLog.hpp>
#include "debugAlloc.hpp"

#include <Es/Common/win.h>

#if ALLOC_DEBUGGER
void *DebugAlloc::Alloc( size_t &size, const char *file, int line, const char *postfix )
{
#ifdef _WIN32
  return GlobalAlloc(GMEM_FIXED, size);
#else
  return malloc(size);
#endif
}
#else
void *DebugAlloc::Alloc( size_t &size )
{
#ifdef _WIN32
  return GlobalAlloc(GMEM_FIXED, size);
#else
  return malloc(size);
#endif
}
#endif
void DebugAlloc::Free( void *mem, size_t size )
{
#ifdef _WIN32
  GlobalFree(mem);
#else
  free(mem);
#endif
}

void *DebugAlloc::Realloc(void *mem, size_t oldSize, size_t size)
{
#ifdef _WIN32
  return GlobalReAlloc(mem, size, 0);
#else
  // there is no portable way to resize the memory block
  // realloc is allowed to move it, which we cannot
  return NULL;
#endif
}

void DebugAlloc::Unlink( void *mem )
{
}

