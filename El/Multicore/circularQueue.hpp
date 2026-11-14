/// Helper functions and classes for environment with multiple CPUs

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _CIRCULAR_QUEUE_HPP
#define _CIRCULAR_QUEUE_HPP

#include <El/elementpch.hpp>
#include <El/Multicore/multicore.hpp>

/// circular buffer, different threads: consumer, publisher
/**
based on SerialArrayCircular, less generic, simpler, mt safe
_readFrom is always behind _writeTo
area from _writeTo to _readFrom+bufferSize is empty

Modulo bufferSize is used for each access
*/


template <class Type, int bufferSize, int nConsumers=1>
class CircularArray
{
  Type _data[bufferSize];
  /// read (consumer) cursor
  /** modified by the consumer only, read by both threads */
  int _readFrom[nConsumers];
  /// write (producer) cursor
  /** modified by the producer only, read by both threads */
  int _writeTo;
  
  public:
  
  CircularArray()
  {
    for (int i=0; i<nConsumers; i++)
    {
      _readFrom[i] = 0;
    }
    _writeTo = 0;
    // bufferSize must be a power of 2, otherwise mod 2 integer overflow would make it inconsistent
    Assert(roundTo2PowerNCeil(bufferSize)==bufferSize);
  }
  ~CircularArray()
  {
    // verify consumer has consumed all data
    for (int i=0; i<nConsumers; i++)
    {
      Assert(_readFrom[i]==_writeTo);
    }
  }
  
  /** 
  Can be used when multiple threads need to produce data simultaneously
  Consumer cannot be active at the same time.
  */
  bool PutMultipleProducers(const Type &src)
  {
    int writeBefore = InterlockedIncrementAcquire((volatile long *)&_writeTo);
    for (int i=0; i<nConsumers; i++)
    {
      // check how much space there is between writeTo and the read pointer
      if (writeBefore-_readFrom[i]>bufferSize) return false;
    }
    _data[(writeBefore-1)&(bufferSize-1)] = src;
    return true;
  }
  
  void Put(const Type &val)
  {
    Type *dst = PutBegin(0);
    *dst = val;
    PutEnd(1);
  }

  RESTRICT_RETURN Type * PutBegin(int itemOffset)
  {
    for (int i=0; i<nConsumers; i++)
    {
      Assert(EmptySpace(i)>itemOffset);
    }
    return &_data[(_writeTo+itemOffset)&(bufferSize-1)];
  }
  
  /** called only by the producer */
  int GetWriteCursor() const {return _writeTo;}
  
  void PutEnd(int nItems)
  {
    for (int i=0; i<nConsumers; i++)
    {
      Assert(EmptySpace(i)>=nItems);
    }
    MemoryPublish(); // before reporting the data, make sure the results are visible
    // and advance - no need to wrap here - wrap is done on FinishWrite only
    _writeTo += nItems;
  }
  
  __forceinline Type &Get(int consumer)
  {
    Assert(consumer<nConsumers);
    // simply return a pointer to a data we already have - no copy needed
    return _data[_readFrom[consumer]&(bufferSize-1)];
  }

  __forceinline void Advance(int consumer)
  {
    Assert(consumer<nConsumers);
    MemoryPublish(); // before marking the memory as free, make sure destruction or other changes are visible
    _readFrom[consumer]++;
  }

  /// called by the producer thread only  
  /** this means the consumer designated by consumerToAdvance should be on the same thread */
  __forceinline void AdvanceToWrite(int consumerToAdvance)
  {
    Assert(consumerToAdvance<nConsumers);
    _readFrom[consumerToAdvance] = _writeTo;
  }
  
  /// called by the consumer thread only
  /** consumerToAdvance needs to be handled by the same thread */
  __forceinline void AdvanceTo(int consumerToAdvance, int advanceTo)
  {
    Assert(consumerToAdvance<nConsumers);
    // we must move only forward
    Assert(_readFrom[consumerToAdvance] <= advanceTo);
    _readFrom[consumerToAdvance] = advanceTo;
  }
  
  /// called from consumer thread only
  int CheckPointer(int consumer) const {return _readFrom[consumer];}
  
  __forceinline const Type &Check(int pos) const
  {
    // simply return a pointer to a data we already have - no copy needed
    return _data[pos&(bufferSize-1)];
  }
  
  __forceinline Type &Check(int pos)
  {
    // simply return a pointer to a data we already have - no copy needed
    return _data[pos&(bufferSize-1)];
  }
  
  /// check empty space - used by the producer only
  __forceinline int EmptySpace(int consumer) const
  {
    // read the read pointer as volatile - it is written to by another thread
    int readFrom = const_cast<volatile int &>(_readFrom[consumer]);
    // check how much space there is between writeTo and the read pointer
    return bufferSize-(_writeTo-readFrom);
  }

  /// check how many pending items - used by the consumer only
  __forceinline int CountPending(int consumer) const
  {
    // read the _writeTo pointer as volatile - it is written to by another thread
    return const_cast<volatile int &>(_writeTo)-_readFrom[consumer];
  }

  int TotalSize() const {return bufferSize;}
  /// should be called by the one owning the consumerBase consumer
  __forceinline int HowMuchBehind(int consumerBehind, int consumerBase) const
  {
    // read the read pointer as volatile - it is written to by another thread
    int readFrom = *const_cast<volatile int *>(&_readFrom[consumerBehind]);
    // check how much space there is between writeTo and the read pointer
    return _readFrom[consumerBase]-readFrom;
  }
  
  /** must be called only from the main thread, and only when BGThread is known to be flushed and waiting */
  void Compact()
  {
    for (int c=0; c<nConsumers; c++)
    {
      if (_readFrom[c]!=_writeTo)
      {
        Assert(false);
        return;
      }
    }
    for (int c=0; c<nConsumers; c++)
    {
      _readFrom[c] = 0;
    }
    _writeTo = 0;
  }
};

#endif

