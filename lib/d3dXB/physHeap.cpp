#include "../wpch.hpp"

#if !_DISABLE_GUI || defined _XBOX
#include "physHeap.hpp"
#include <El/Common/perfProf.hpp>

DEFINE_FAST_ALLOCATOR(PhysMemHeapXB::HeapItem);

PhysMemHeapXB::PhysMemHeapXB(size_t size)
{
  void *start = D3D_AllocContiguousMemory(size,D3DTILE_ALIGNMENT);
  if (start)
  {
    Init((int)start,size,256);
  }
  else
  {
    Init((int)start,0,256);
  }
}
PhysMemHeapXB::~PhysMemHeapXB()
{
  void *mem = (void *)Memory();
  if (mem)
  {
    D3D_FreeContiguousMemory(mem);
  }
}

/// manager to provide a malloc like access to PhysMemHeapXB
class PhysHeapsManager
{
  size_t _heapSize;
  
  /// heaps that have at least some space free
  RefArray<PhysMemHeapXB> _ready;
  // heaps that are completely full
  RefArray<PhysMemHeapXB> _full;
  
  public:
  explicit PhysHeapsManager(size_t heapSize):_heapSize(heapSize){}
  
  PhysMemHeapItemXB Allocate(size_t size);
  void Free(PhysMemHeapItemXB &mem);
  /// total memory held in all allocated buffers
  size_t GetTotalAllocated() const;
  /// total memory held in all blocks requested
  size_t GetTotalRequested() const;
  
};

// custom physical memory manager
PhysMemHeapItemXB PhysHeapsManager::Allocate(size_t size)
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
      PhysMemHeapXB *heap = _ready[i];
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
      PhysMemHeapXB *heap = _ready[bestI];
      PhysMemHeapXB::HeapItem *item = heap->Alloc(size);
      // item is certainly allocated - we selected best heap to fit
      DoAssert(item);
      if (heap->TotalFreeLeft()==0)
      {
        _full.Add(heap);
        _ready.DeleteAt(bestI);
      }
      return PhysMemHeapItemXB(heap,item);
    }
  #else
    // find first fit
    for (int i=0; i<_ready.Size(); i++)
    {
      PhysMemHeapXB *heap = _ready[i];
      PhysMemHeapXB::HeapItem *item = heap->Alloc(size);
      if (item)
      {
        if (heap->TotalFreeLeft()==0)
        {
          _full.Add(heap);
          _ready.DeleteAt(i);
        }
        return PhysMemHeapItemXB(heap,item);
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
  Ref<PhysMemHeapXB> heap = new PhysMemHeapXB(heapSize);
  if (heap->Size()==0)
  {
    // heap not allocated: return NULL
    return PhysMemHeapItemXB();
  }
  PhysMemHeapXB::HeapItem *item = heap->Alloc(size);
  DoAssert(item);
  if (heap->TotalFreeLeft()==0)
  {
    _full.Add(heap);
  }
  else
  {
    _ready.Add(heap);
  }
  return PhysMemHeapItemXB(heap,item);
}

void PhysHeapsManager::Free(PhysMemHeapItemXB &mem)
{
  // check which heap is this item from
  if (!mem._item) return;
  Ref<PhysMemHeapXB> heap = mem._heap;
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

PhysMemHeapItemXB AllocatePhysicalMemory(size_t size)
{
  PhysMemHeapItemXB ret = PhysicalMemory.Allocate(size);
//  DIAG_MESSAGE(
//    5000,Format(
//      "Phys mem %d, %d, overhead %d",
//      PhysicalMemory.GetTotalAllocated(),PhysicalMemory.GetTotalRequested(),
//      PhysicalMemory.GetTotalAllocated()-PhysicalMemory.GetTotalRequested()
//    )
//  );
  return ret;
}

void FreePhysicalMemory(PhysMemHeapItemXB &mem)
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

PhysMemHeapItemXB AllocateTextureMemory(size_t size)
{
  return TextureMemory.Allocate(size);
}

void FreeTextureMemory(PhysMemHeapItemXB &mem)
{
  TextureMemory.Free(mem);
}

int GetTextureMemoryAllocated()
{
  return TextureMemory.GetTotalAllocated();
}


#endif
