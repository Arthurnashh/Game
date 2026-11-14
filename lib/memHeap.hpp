#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MEM_HEAP_HPP
#define _MEM_HEAP_HPP

#include "El/FreeOnDemand/memFreeReq.hpp"

/// interface to allow safe enumeration of memory intended for discarding
struct IFreeOnDemandEnumerator
{
  virtual IMemoryFreeOnDemand *GetFirstFreeOnDemand() const = 0;
  virtual IMemoryFreeOnDemand *GetNextFreeOnDemand(IMemoryFreeOnDemand *cur) const = 0;
  
  virtual IMemoryFreeOnDemand *GetFirstFreeOnDemandLowLevel() const = 0;
  virtual IMemoryFreeOnDemand *GetNextFreeOnDemandLowLevel(IMemoryFreeOnDemand *cur) const = 0;

  
  virtual void Lock() const = 0;
  virtual void Unlock() const = 0;
};


#include "memNedAlloc.hpp"

namespace Memory
{
  void ReportAnyMemory(const char *reason);
  
  mem_size_t TotalAllocated();
  mem_size_t TotalCommitted();
  mem_size_t TotalLowLevelMemory();
  mem_size_t FreeSystemMemory(mem_size_t size);
  bool CheckVirtualFree(mem_size_t size);


  struct LockedFreeOnDemandEnumerator
  {
    const IFreeOnDemandEnumerator &_enumerator;
    
    LockedFreeOnDemandEnumerator(const IFreeOnDemandEnumerator &enumerator):_enumerator(enumerator)
    {
      _enumerator.Lock();
    }
    LockedFreeOnDemandEnumerator(const LockedFreeOnDemandEnumerator &lockedEnumerator) : _enumerator(lockedEnumerator._enumerator) 
    {
      _enumerator.Lock();
    }
    ~LockedFreeOnDemandEnumerator()
    {
      _enumerator.Unlock();
    }

    /* we know the whole object is */
    virtual IMemoryFreeOnDemand *GetFirstFreeOnDemand() const {return _enumerator.GetFirstFreeOnDemand();}
    virtual IMemoryFreeOnDemand *GetNextFreeOnDemand(IMemoryFreeOnDemand *cur) const {return _enumerator.GetNextFreeOnDemand(cur);}
  
    virtual IMemoryFreeOnDemand *GetFirstFreeOnDemandLowLevel() const {return _enumerator.GetFirstFreeOnDemandLowLevel();}
    virtual IMemoryFreeOnDemand *GetNextFreeOnDemandLowLevel(IMemoryFreeOnDemand *cur) const {return _enumerator.GetNextFreeOnDemandLowLevel(cur);}
  };

  LockedFreeOnDemandEnumerator GetFreeOnDemandEnumerator();
}


extern mem_size_t ThrottleMemoryUsagePhysical;
extern mem_size_t ThrottleMemoryUsageVirtual;


#endif

