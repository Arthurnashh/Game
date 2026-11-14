#include "El/elementpch.hpp"
#include <Es/Containers/staticArray.hpp>
#include "memFreeReq.hpp"
#include "El/Common/perfProf.hpp"

/*!
\patch_internal 1.44 Date 2/13/2002 by Ondra
- New: Various caches deallocated when memory allocation is not successful.
*/

size_t MemoryFreeOnDemandHelper::Free(size_t amount)
{
	// free first candidate
	size_t freedTotal = 0;
	for(;;)
	{
		size_t freed = FreeOneItem();
		if (freed==0) break;
		freedTotal += freed;
		if (amount<=freedTotal) break;
	}
	return freedTotal;
}

size_t MemoryFreeOnDemandHelper::FreeAll()
{
	return Free(UINT_MAX);
}

void MemoryFreeOnDemandList::Register(IMemoryFreeOnDemand *object)
{
	// _freeOnDemand list is maintained sorted by priority
	// lowest priority first
	// find suitable position to insert this object
	for(IMemoryFreeOnDemand *walk=Start(); NotEnd(walk); walk = Advance(walk))
	{
		if (object->Priority()<walk->Priority())
		{
			InsertBefore(walk,object);
			return;
		}
	}

	// no existing object has higher priority
	// we have the highest and we need to be added at the list end
	Add(object);
}

size_t MemoryFreeOnDemandList::Free(size_t amount)
{
	// gather statistics from all main heap sources
	// the purpose of this function may look somewhat unclear - why should anyone want to avoid releasing from sys-mem sources?
	// releasing sys-mem sources releases memory for the main heap as well
	// actually it is used for low-level memory
	// references are: MemHeap::FreeOnDemand (unused), MemHeap::FreeOnDemandLowLevel (used in MemTableFunctions::NewPage...)
	AUTO_STATIC_ARRAY(OnDemandStats,stats,128);

	for(IMemoryFreeOnDemand *walk=Start(); NotEnd(walk); walk = Advance(walk))
	{
		OnDemandStats &mstat = stats.Append();
		mstat.mem = walk;
		mstat.system = false; // by default assume no system  memory
	}
	return BalanceList(amount,stats.Data(),stats.Size(),NULL,0);
}

/*!
@param system if provided, return how much memory marked as system in the list was released
@param lowLevel we need to make sure balancing is done relative to the lowLevel storage as well
@param amount how much memory do we want to release - may be negative, used in balancing
@return how much memory was released from the list

\patch 5156 Date 5/12/2007 by Ondra
- Fixed: Memory manager sometimes flushed caches too deeply, causing short intensive disk activity.
*/
mem_size_t MemoryFreeOnDemandList::BalanceList(
	mem_size_t amount, OnDemandStats *stats, int nStats,  IMemoryFreeOnDemand **lowLevel, int nLowLevel,
	mem_size_t *system
)
{
	if (system) *system = 0;
	// if there is nowhere to release from, return (avoids division by zero in 1/totalPriority)
  if (nStats==0) return 0;
	// 64b - we do not want to risk 32b sign overflow
	long long total = 0;
	float totalPriority = 0;
	// highest ratio check for all high level managers
	float highestCheckRatio = 0;
	for (int i=0; i<nStats; i++)
	{
		OnDemandStats &mstat = stats[i];
		mstat.controlled = mstat.mem->MemoryControlled();
		mstat.releasePossible = mstat.controlled>0;
		total += mstat.controlled;
		totalPriority += mstat.mem->Priority();

    float checkRatio  = mstat.mem->MemoryControlled()/mstat.mem->Priority();
    if (checkRatio>highestCheckRatio) highestCheckRatio = checkRatio;
	}
	// low-level balancing
  // never release any low level memory - it will be released only when needed
  // moreover, some low level memory should always have the biggest "balance" ratio of all
  // when not, it means some other item is over-allocated
  // note: currently there is only one low-level memory, and that is a file cache
  // highest ratio check for all low level managers
	float highestCheckRatioLL = 0;
	int highestCheckLLIndex = -1;
  for (int i=0; i<nLowLevel; i++)
  {
    IMemoryFreeOnDemand *ll = lowLevel[i];
    // we do not want to include stats from low-level in overall statistics
    //total += ll->MemoryControlled();
    //totalPriority += ll->Priority();

    // calculate allocation ratio reduction as a ratio
    mem_size_t llIncFree = ll->MemoryControlled()-( amount<0 ? amount : 0);
    float checkRatio = llIncFree/ll->Priority();
    if (checkRatio>highestCheckRatioLL) highestCheckRatioLL = checkRatio, highestCheckLLIndex = i;
  }
  
  // condition highestCheckLLIndex>=0 is to handle case where nLowLevel = 0
  if (highestCheckLLIndex>=0 && highestCheckRatioLL<highestCheckRatio)
  {
    // we want the selected low-level manager to achieve the highest ratio level
    IMemoryFreeOnDemand *ll = lowLevel[highestCheckLLIndex];
    
    mem_size_t llIncFree = ll->MemoryControlled()-( amount<0 ? amount : 0);
    float memLLWanted = highestCheckRatio/highestCheckRatioLL*llIncFree;
    
    // we never want to release a lot more than necessary
    // having lot and lots of free memory is useless
    // the reserve we want to have should be reasonable
    // reasonable means proportional to the total amount of free + low-level memory?
    
    // one possible case: LL memory may be under-allocated simply because it has no use of the memory,
    // or because of some limit of its own
    
    //memLLWanted = floatMin(memLLWanted,);
    
    // we need to free somewhat more than requested to avoid pushing low-level even more
    Assert(memLLWanted>=llIncFree)
    if (memLLWanted>llIncFree)
    {
      // we were passed request like: freeRequired-(heapFree+sysFree+llFree)>=0
      // we want to get into a state where freeRequired-memLLWanted>=0
      mem_size_t addAmount = toLargeInt(memLLWanted)-llIncFree;
      // we want to release only gradually, no more than certain amount at once
      const int maxAddAmount = 2*1024*1024;
      if (addAmount>maxAddAmount)
      {
        addAmount = maxAddAmount;
      }
      amount += addAmount;
    }
  }
  
  // if we need nothing to release, we can return
  // we were called because the caller was unsure if low level balancing will not cause releasing
  if (amount<=0) return 0;
  

  // prevent totalWanted zero or negative - this would break
  // however, when we want to release more than we have, we are in trouble anyway  
	long long totalWanted = total>amount ? total-amount : 1;
  float invTotalPriority = 1/totalPriority;
	// balance controlled/priority ratios

	if (total<=totalWanted) return 0;
	PROFILE_SCOPE_EX(gbFre,mem);
  if (PROFILE_SCOPE_NAME(gbFre).IsActive())
  {
    PROFILE_SCOPE_NAME(gbFre).AddMoreInfo(Format("Total %d, releasing %d",total,total-totalWanted));
  }
	mem_size_t freedTotal = 0;
	while (total>totalWanted)
	{
		// two possible ways:
		// 1) select the one with the biggest wanted/actual ratio
		// 2) select the one with the biggest wanted/actual difference
		// we use approach 1) ratio - it seems to keep balancing best
		int maxReductionI = -1;
		float maxReduction = -FLT_MAX;
		for (int i=0; i<nStats; i++)
		{
			const OnDemandStats &mstat = stats[i];
			if (!mstat.releasePossible) continue;
			IMemoryFreeOnDemand *mem = mstat.mem;
			// calculate how much memory this item should have after reduction
			float reduceTo = float(totalWanted)*mem->Priority()*invTotalPriority;
			// calculate reduction as a ratio
			float reduction = mstat.controlled/reduceTo;
			if (maxReduction<reduction)
			{
				maxReduction = reduction;
				maxReductionI = i;
			}
		}
		if (maxReductionI<0)
		{
			// nothing reduce??
			break;
		}
		PROFILE_SCOPE_EX(gbFr1,mem);
		OnDemandStats &rstat = stats[maxReductionI];
		size_t released = rstat.mem->Free(1);
		// if release failed, we will mark the item as unusable for future released
		if (released!=0)
		{
			rstat.controlled -= released;
			total -= released;
			freedTotal += released;
			if (rstat.system && system) *system += released;
		}
		else
		{
			rstat.releasePossible = false;
		}
	}
	return freedTotal;
}

size_t MemoryFreeOnDemandList::FreeAll()
{
	int freedTotal = 0;
	for(IMemoryFreeOnDemand *walk=Start(); NotEnd(walk); walk = Advance(walk))
	{
		int freed = walk->FreeAll();
		freedTotal += freed;
	}

	return freedTotal;
}
