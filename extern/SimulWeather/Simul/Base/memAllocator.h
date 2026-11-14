#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SIMUL_WEATHER_BASE_MEMALLOC_H
#define _SIMUL_WEATHER_BASE_MEMALLOC_H

#include <malloc.h>

namespace simul
{
  namespace base
  {
    // base memory allocator
    class BaseMemoryAllocator
    {
    public:
      static BaseMemoryAllocator* GetMemoryAllocatorStd()
      {
        static BaseMemoryAllocator memAlloc;
        return &memAlloc;
      }

//#ifdef SIMUL_DLL
#if 0
      // in DLL version we must use standard allocator
      static BaseMemoryAllocator* GetMemoryAllocator() {return GetMemoryAllocatorStd();}
#else
      // Host applications MUST define this function
      // This gives user the possibility do define custom memory allocator
      // If you want to use standard memory allocator, simply return GetMemoryAllocatorStd();
      static BaseMemoryAllocator* GetMemoryAllocator();
#endif

      virtual void *New(size_t size) {return malloc(size);}
      virtual void *New(size_t size, const char *file, int line) {file; line; return malloc(size);}
      virtual void Delete(void *mem){free(mem);}
      virtual void Delete(void *mem, const char *file, int line) {file; line; free(mem);}     
    }; //class BaseMemoryAllocator
  }//namespace base
}//namespace simul


#endif //_SIMUL_WEATHER_BASE_MEMALLOC_H
