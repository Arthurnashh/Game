#ifdef _MSC_VER
#pragma once
#endif

#ifndef __PHYSHEAP_HPP
#define __PHYSHEAP_HPP

#if !_DISABLE_GUI || defined _XBOX

#include "d3dXBdefs.hpp"

#include "../heap.hpp"

//! aggregate several small allocation in one large
class PhysMemHeapXB: public RefCount, public Heap<int,int>
{
  typedef Heap<int,int> base;
  
  public:
  typedef base::HeapItem Item;
  
  explicit PhysMemHeapXB(size_t size);
  ~PhysMemHeapXB();
};
//! information about one small allocation
struct PhysMemHeapItemXB
{
  PhysMemHeapXB::Item *_item;
  PhysMemHeapXB *_heap;
  
  PhysMemHeapItemXB()
  :_item(NULL),_heap(NULL)
  {
  }
  PhysMemHeapItemXB(PhysMemHeapXB *heap, PhysMemHeapXB::Item *item)
  :_heap(heap),_item(item)
  {
  }
  
  bool NotNull() const {return _item!=NULL;}
  bool IsNull() const {return _item==NULL;}
  __forceinline void *GetAddress() const {return (void *)_item->Memory();}
};

//@{ malloc like access to PhysMemHeapItemXB collection
PhysMemHeapItemXB AllocateTextureMemory(size_t size);
void FreeTextureMemory(PhysMemHeapItemXB &mem);
int GetTextureMemoryAllocated();
//@}

//@{ malloc like access to PhysMemHeapItemXB collection
PhysMemHeapItemXB AllocatePhysicalMemory(size_t size);
void FreePhysicalMemory(PhysMemHeapItemXB &mem);
int GetPhysicalMemoryAllocated();
int GetPhysicalMemoryRequested();
//@}


#endif // !_DISABLE_GUI

#endif
