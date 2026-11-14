#ifdef _MSC_VER
#pragma once
#endif

#ifndef __PHYSHEAP_HPP
#define __PHYSHEAP_HPP

#if !_DISABLE_GUI && defined _XBOX

#include "d3d9defs.hpp"

#include "../heap.hpp"

//! aggregate several small allocation in one large
class PhysMemHeap9: public RefCount, public Heap<int,int>
{
  typedef Heap<int,int> base;
  
  public:
  typedef base::HeapItem Item;
  
  explicit PhysMemHeap9(size_t size);
  ~PhysMemHeap9();
};
//! information about one small allocation
struct PhysMemHeapItem9
{
  PhysMemHeap9::Item *_item;
  PhysMemHeap9 *_heap;
  
  PhysMemHeapItem9()
  :_item(NULL),_heap(NULL)
  {
  }
  PhysMemHeapItem9(PhysMemHeap9 *heap, PhysMemHeap9::Item *item)
  :_heap(heap),_item(item)
  {
  }
  
  bool NotNull() const {return _item!=NULL;}
  bool IsNull() const {return _item==NULL;}
  __forceinline void *GetAddress() const {return (void *)_item->Memory();}
};

//@{ malloc like access to PhysMemHeapItem9 collection
PhysMemHeapItem9 AllocateTextureMemory(size_t size);
void FreeTextureMemory(PhysMemHeapItem9 &mem);
int GetTextureMemoryAllocated();
//@}

//@{ malloc like access to PhysMemHeapItem9 collection
PhysMemHeapItem9 AllocatePhysicalMemory(size_t size);
void FreePhysicalMemory(PhysMemHeapItem9 &mem);
int GetPhysicalMemoryAllocated();
int GetPhysicalMemoryRequested();
//@}


#endif // !_DISABLE_GUI

#endif
