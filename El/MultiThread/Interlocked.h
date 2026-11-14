#ifndef _INTERLOCKED__H_
#define _INTERLOCKED__H_

#pragma warning (push)
#pragma warning (disable : 4035)

namespace MultiThreadSafe
{

#if defined(_M_IX86) && defined(_MT)
static __forceinline  long  MTIncrement(long volatile *mem) 
  {
    __asm
    {
      mov eax,1
      mov edx,mem
      lock xadd [edx],eax
      inc eax;
    }
  }

static __forceinline long  MTDecrement(long volatile *mem) 
  {
    __asm
    {
      mov eax,-1
      mov edx,mem
      lock xadd [edx],eax
      dec eax
    }
  }

static __forceinline long  MTExchangeAdd(long volatile *mem, long value) 
  {
    __asm
    {
      mov eax,value
      mov edx,mem
      lock xadd [edx],eax
    }
  }

static __forceinline long  MTExchange(long volatile *mem, long value) 
  {
    __asm
    {
      mov eax,value
      mov edx,mem
      lock xchg [edx],eax
    }
  }

static __forceinline long  MTCompareExchange(long volatile *mem, long value, long comp) 
  {
    __asm
    {
      mov eax,comp
      mov edx,value
      mov edi,mem
      lock cmpxchg [edi],edx
    }
  }
#else

static inline long MTIncrement(long volatile *mem)
{
  return ++mem[0];
}
static inline long MTDecrement(long volatile *mem)
{
  return --mem[0];
}
static inline long MTExchangeAdd(long volatile *mem, long value)
{
  long res=mem[0];
  mem[0]+=value;
  return res;
}
static inline long MTExchange(long volatile *mem, long value)
{
  long res=mem[0];
  mem[0]=value;
  return res;
}
static inline long MTCompareExchange(long volatile *mem, long value, long test)
{
  long res=mem[0];
  if (res==value) mem[0]=value;
  return res;
}
#endif
};


#pragma warning (pop)


#endif

