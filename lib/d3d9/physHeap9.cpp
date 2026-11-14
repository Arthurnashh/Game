#include "../wpch.hpp"

#if !_DISABLE_GUI && defined _XBOX
#include "physHeap9.hpp"
#include <El/Common/perfProf.hpp>

DEFINE_FAST_ALLOCATOR(PhysMemHeap9::HeapItem);

PhysMemHeap9::PhysMemHeap9(size_t size)
{
  //void *start = XPhysicalAlloc(size,MAXULONG_PTR,0,PAGE_READWRITE|PAGE_WRITECOMBINE|MEM_LARGE_PAGES);
  void *start = XPhysicalAlloc(size,MAXULONG_PTR,0,PAGE_READWRITE|PAGE_WRITECOMBINE);
  if (start)
  {
    Init((int)start,size,256);
  }
  else
  {
    Init((int)start,0,256);
  }
}
PhysMemHeap9::~PhysMemHeap9()
{
  void *mem = (void *)Memory();
  if (mem)
  {
    XPhysicalFree(mem);
  }
}

/// manager to provide a malloc like access to PhysMemHeap9
class PhysHeapsManager
{
  size_t _heapSize;
  
  /// heaps that have at least some space free
  RefArray<PhysMemHeap9> _ready;
  // heaps that are completely full
  RefArray<PhysMemHeap9> _full;
  
  public:
  explicit PhysHeapsManager(size_t heapSize):_heapSize(heapSize){}
  
  PhysMemHeapItem9 Allocate(size_t size);
  void Free(PhysMemHeapItem9 &mem);
  /// total memory held in all allocated buffers
  size_t GetTotalAllocated() const;
  /// total memory held in all blocks requested
  size_t GetTotalRequested() const;
  
};

// custom physical memory manager
PhysMemHeapItem9 PhysHeapsManager::Allocate(size_t size)
{
  PROFILE_SCOPE(phMem);
  DoAssert(size!=0);
  // find some physical heap that has some memory free
  #if 1
    // find best fit
    // best fit may seem slower, but it is often faster in long term
    // moreover, it results in less overhead
    int bestI = -1;
    int bestDiff = INT_MAX;
    for (int i=0; i<_ready.Size(); i++)
    {
      PhysMemHeap9 *heap = _ready[i];
      int diff = heap->TryAllocMatch(size);
      if (diff<0) continue;
      if (bestDiff>diff)
      {
        bestDiff = diff;
        bestI = i;
      }
    }
    if (bestI>=0)
    {
      PhysMemHeap9 *heap = _ready[bestI];
      PhysMemHeap9::HeapItem *item = heap->Alloc(size);
      // item is certainly allocated - we selected best heap to fit
      DoAssert(item);
      if (heap->TotalFreeLeft()==0)
      {
        _full.Add(heap);
        _ready.DeleteAt(bestI);
      }
      return PhysMemHeapItem9(heap,item);
    }
  #else
    // find first fit
    for (int i=0; i<_ready.Size(); i++)
    {
      PhysMemHeap9 *heap = _ready[i];
      PhysMemHeap9::HeapItem *item = heap->Alloc(size);
      if (item)
      {
        if (heap->TotalFreeLeft()==0)
        {
          _full.Add(heap);
          _ready.DeleteAt(i);
        }
        return PhysMemHeapItem9(heap,item);
      }
    }
  #endif
  const size_t noAlign = 16*1024;
  size_t heapSize = _heapSize;
  // the heap must be large enough to contain the item currently allocated
  if (size>heapSize)
  {
    // do not perform any additional alignment for very big block
    // we do not want a small items blocking this allocation
    // as this would result in fragmentation
    if (size<noAlign)
    {
      const size_t sizeAlign = 4*1024;
      heapSize = (size+sizeAlign-1)&~(sizeAlign-1);
    }
    else
    {
      // alignment 256 is always needed
      const size_t sizeMinAlign = 256;
      heapSize = (size+sizeMinAlign-1)&~(sizeMinAlign-1);
    }
  }
  Ref<PhysMemHeap9> heap = new PhysMemHeap9(heapSize);
  if (heap->Size()==0)
  {
    // heap not allocated: return NULL
    return PhysMemHeapItem9();
  }
  PhysMemHeap9::HeapItem *item = heap->Alloc(size);
  DoAssert(item);
  if (heap->TotalFreeLeft()==0)
  {
    _full.Add(heap);
  }
  else
  {
    _ready.Add(heap);
  }
  return PhysMemHeapItem9(heap,item);
}

void PhysHeapsManager::Free(PhysMemHeapItem9 &mem)
{
  // check which heap is this item from
  if (!mem._item) return;
  Ref<PhysMemHeap9> heap = mem._heap;
  if (heap->TotalFreeLeft()==0)
  {
    _ready.Add(mem._heap);
    Verify( _full.DeleteKey(mem._heap) );
  }
  mem._heap->Free(mem._item);
  mem._heap = NULL;
  mem._item = NULL;
  if (heap->TotalBusy()==0)
  {
    _ready.DeleteKey(heap);
    Assert(heap->RefCounter()==1);
    heap.Free();
  }
}

/**
The difference between GetTotalAllocated and GetTotalRequested
is the allocation overhead - memory held in big buffers but not really used
*/
size_t PhysHeapsManager::GetTotalRequested() const
{
  size_t total = 0;
  for (int i=0; i<_ready.Size(); i++)
  {
    total += _ready[i]->TotalBusy();
  }
  for (int i=0; i<_full.Size(); i++)
  {
    Assert(_full[i]->TotalBusy()==_full[i]->Size());
    total += _full[i]->TotalBusy();
  }
  return total;
  
}

/// used for real memory consumption measurements
static inline int AlignedAllocationSize(int size)
{
  int align = 4*1024;
  return (size+align-1)&~(align-1);
}
size_t PhysHeapsManager::GetTotalAllocated() const
{
  size_t total = 0;
  for (int i=0; i<_ready.Size(); i++)
  {
    total += AlignedAllocationSize(_ready[i]->Size());
  }
  for (int i=0; i<_full.Size(); i++)
  {
    total += AlignedAllocationSize(_full[i]->Size());
  }
  return total;
}

/// generic physical memory allocator to avoid fragmentation for pushbuffers / vertex buffer
//static PhysHeapsManager PhysicalMemory(64*1024);
static PhysHeapsManager PhysicalMemory(8*1024);

#include <Es/framework/appFrame.hpp>

PhysMemHeapItem9 AllocatePhysicalMemory(size_t size)
{
  PhysMemHeapItem9 ret = PhysicalMemory.Allocate(size);
//  DIAG_MESSAGE(
//    5000,Format(
//      "Phys mem %d, %d, overhead %d",
//      PhysicalMemory.GetTotalAllocated(),PhysicalMemory.GetTotalRequested(),
//      PhysicalMemory.GetTotalAllocated()-PhysicalMemory.GetTotalRequested()
//    )
//  );
  return ret;
}

void FreePhysicalMemory(PhysMemHeapItem9 &mem)
{
  PhysicalMemory.Free(mem);
}

int GetPhysicalMemoryAllocated()
{
  return PhysicalMemory.GetTotalAllocated();
}

int GetPhysicalMemoryRequested()
{
  return PhysicalMemory.GetTotalRequested();
}

/// texture memory - separate space, as texture allocation is quite static
static PhysHeapsManager TextureMemory(32*1024);

PhysMemHeapItem9 AllocateTextureMemory(size_t size)
{
  return TextureMemory.Allocate(size);
}

void FreeTextureMemory(PhysMemHeapItem9 &mem)
{
  TextureMemory.Free(mem);
}

int GetTextureMemoryAllocated()
{
  return TextureMemory.GetTotalAllocated();
}


#endif
