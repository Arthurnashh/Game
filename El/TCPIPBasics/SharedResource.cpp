#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <stddef.h>
#include "SharedResource.h"

#define SHAREDRESOURCE_CLUSTERSZ 256
#define SYNCVALUE 0x3FFFFFFF

struct SharedResource_Cluster
{
  long refcnt;	///<Reference counter for this cluster
  union
  {
    SharedResource_Counter sync;	///<Synchronizing value (-1)
    SharedResource_Cluster *nextfree;
  };
  SharedResource_Counter counters[SHAREDRESOURCE_CLUSTERSZ];
};

struct SharedResource_Guard:public CRITICAL_SECTION
{
  SharedResource_Guard() 
  {InitializeCriticalSection(this);}
  ~SharedResource_Guard() 
  {DeleteCriticalSection(this);}
  void Lock() 
  {EnterCriticalSection(this);}
  void Unlock() 
  {LeaveCriticalSection(this);}
};

static SharedResource_Guard mtguard;

static SharedResource_Cluster *ClusterCurrent=NULL;
static SharedResource_Cluster *NextFree=NULL;

static SharedResource_Cluster *AllocFreeCluster()
{
  SharedResource_Cluster *out;
  if (NextFree)
  {
    out=NextFree;
    NextFree=NextFree->nextfree;	
  }
  else
  {
    out=new SharedResource_Cluster;
  }
  memset(out,0,sizeof(SharedResource_Cluster));
  out->sync.SetRefCount(SYNCVALUE);
  out->refcnt=0;  
  return out;
}

static void FreeCluster(SharedResource_Cluster *cls)
{
  cls->nextfree=NextFree;
  NextFree=cls;
}

SharedResource_Counter *SharedResource_Counter::AllocCounter()
{
  int i;
  if (ClusterCurrent==NULL) ClusterCurrent=AllocFreeCluster();
  if (ClusterCurrent->refcnt>=SHAREDRESOURCE_CLUSTERSZ)
  {
    ClusterCurrent=AllocFreeCluster();
  }
  for (i=0;i<SHAREDRESOURCE_CLUSTERSZ;i++)
    if (ClusterCurrent->counters[i].GetRefCount()==0)
    {
      ClusterCurrent->refcnt++;
      ClusterCurrent->counters[i].Reset();
      return ClusterCurrent->counters+i;
    }
  ClusterCurrent->refcnt=i;
  return AllocCounter();
}

static SharedResource_Cluster *FindClusterFromCounter(SharedResource_Counter *cntr)
{
  while (cntr->GetRefCount()!=SYNCVALUE) cntr--;
  return (SharedResource_Cluster *)((char *)cntr-offsetof(SharedResource_Cluster,sync));
}

void SharedResource_Counter::DeallocCounter(SharedResource_Counter *cntr)
{
  SharedResource_Cluster *clust=FindClusterFromCounter(cntr);
  clust->refcnt--;
  if (clust->refcnt==0) 
  {
    FreeCluster(clust);
    if (clust==ClusterCurrent) 
      ClusterCurrent=NULL;
  }
}

void SharedResource_Counter::AddRef()
{
  mtguard.Lock();
  if (value)
  {
    value++;
  }
  mtguard.Unlock();
}

bool SharedResource_Counter::Release() 
{	  
  mtguard.Lock();
  if (value) 
  {
    if (--value==0)
    {
      bool ret=closed!=1;
      SharedResource_Counter::DeallocCounter(this);
      mtguard.Unlock();
      return ret;
    }
  }	  		
  mtguard.Unlock();
  return false;
}

