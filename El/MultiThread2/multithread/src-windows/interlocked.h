#ifndef _MULTITHREAD_WIN32_INTERLOCKED_H_
#define _MULTITHREAD_WIN32_INTERLOCKED_H_

#pragma warning (push)
#pragma warning (disable : 4035)

namespace MultiThread
{

static __forceinline  long  MTIncrement(long volatile *mem) 
  {
#ifdef MUTLTITHREAD_USE_ASM_INTERLOCKED
    __asm
    {
      mov eax,1
      mov edx,mem
      lock xadd [edx],eax
      inc eax;
    }
#else
    return InterlockedIncrement(mem);
#endif
  }

static __forceinline long  MTDecrement(long volatile *mem) 
  {
#ifdef MUTLTITHREAD_USE_ASM_INTERLOCKED
    __asm
    {
      mov eax,-1
      mov edx,mem
      lock xadd [edx],eax
      dec eax
    }
#else
  return InterlockedDecrement(mem);
#endif
  }

static __forceinline long  MTExchangeAdd(long volatile *mem, long value) 
  {
#ifdef MUTLTITHREAD_USE_ASM_INTERLOCKED
    __asm
    {
      mov eax,value

      mov edx,mem
      lock xadd [edx],eax
    }
#else
    return InterlockedExchangeAdd(mem, value);
#endif
  }

static __forceinline long  MTExchange(long volatile *mem, long value) 
  {
#ifdef MUTLTITHREAD_USE_ASM_INTERLOCKED
    __asm
    {
      mov eax,value
      mov edx,mem
      lock xchg [edx],eax
    }
#else
    return InterlockedExchange(mem, value);
#endif
  }

static __forceinline long  MTCompareExchange(long volatile *mem, long value, long comp) 
  {
#ifdef MUTLTITHREAD_USE_ASM_INTERLOCKED
    __asm
    {
      mov eax,comp
      mov edx,value
      mov edi,mem
      lock cmpxchg [edi],edx
    }
  }
#else
    return InterlockedCompareExchange(mem, value, comp);
#endif
  }
}

#pragma warning (pop)

#endif /*INTERLOCKED_H_*/
