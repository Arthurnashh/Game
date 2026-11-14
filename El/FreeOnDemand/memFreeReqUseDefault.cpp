#include "El/elementpch.hpp"
#include <Es/Containers/staticArray.hpp>
#include "memFreeReq.hpp"

#pragma warning(disable:4073)
#pragma init_seg(lib)
// free on demand global list

static MemoryFreeOnDemandList GMemoryFreeOnDemandList INIT_PRIORITY_HIGH;

void RegisterFreeOnDemandMemory(IMemoryFreeOnDemand *object)
{
	GMemoryFreeOnDemandList.Register(object);
}

IMemoryFreeOnDemand *GetFirstFreeOnDemandMemory()
{
	return GMemoryFreeOnDemandList.First();
}
IMemoryFreeOnDemand *GetNextMemoryFreeOnDemand(IMemoryFreeOnDemand *cur)
{
	return GMemoryFreeOnDemandList.Next(cur);
}

size_t FreeOnDemandMemory(size_t size)
{
	return GMemoryFreeOnDemandList.Free(size);
}

int FrameId = 0;

void FreeOnDemandFrame()
{
  FrameId++;
  for (IMemoryFreeOnDemand *mem = GMemoryFreeOnDemandList.First(); mem; mem = GMemoryFreeOnDemandList.Next(mem))
  {
    mem->MemoryControlledFrame();
  }
}

void FreeOnDemandGarbageCollectMain(size_t freeRequired, size_t freeSysRequired)
{
}

size_t FreeOnDemandSystemMemoryLowLevel(size_t size)
{
  return 0;
}
