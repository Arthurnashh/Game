#include <malloc.h>
#include <float.h>
#include <stdlib.h>
#include ".\incidencetable.h"
#include <es/containers/array.hpp>

void LogF(const char *format,...);

IncidenceTable::IncidenceTable(int size, bool allocate):_size(size),onDemand(0) 
{
    if (allocate) 
    {
       GetF = &IncidenceTable::GetAllocated;
      _array2d=new float[size*size];
    }
    else
    {
       GetF = &IncidenceTable::GetNotAllocated;
      _array2d = 0;
    }
  
}

IncidenceTable::~IncidenceTable(void)
{
  delete [] _array2d;
}

void IncidenceTable::Set(int a, int b, float v)
{
  _array2d[a*_size+b]=v;
}

float IncidenceTable::GetNotAllocated(int a, int b) const
{
 return onDemand->Get(a,b);
}


float IncidenceTable::GetAllocated(int a, int b) const
{
  float x=_array2d[a*_size+b];
  if (x == -FLT_MAX) {
      x=_array2d[a*_size+b]=onDemand->Get(a,b);
  }
  return x;
}

IncidenceTableForGroups::IncidenceTableForGroups(int size, bool alloc):IncidenceTable(size, alloc)
{
  _activeRows=new bool[size];
  for (int i=0;i<size;i++) _activeRows[i]=true;
}
IncidenceTableForGroups::~IncidenceTableForGroups()
{
  delete _activeRows;
}

void IncidenceTable::SetOnDemand(IInciTableOnDemandCalc *onDemand)
{
    this->onDemand = onDemand;
    if (_array2d) for (int i = 0, cnt = _size*_size; i< cnt; i++) _array2d[i] = -FLT_MAX;
}

void IncidenceTableForGroups::RemoveRow(int i,bool remove)
{
  _activeRows[i]=!remove;
}

int IncidenceTableForGroups::GetGoodGroup(float groupSize, int &bestPoint, int *pointList)
{
  float bestDistance=FLT_MAX;
  int bestCount=0;
  bestPoint=-1;

  for (int i=0;i<_size;i++) if (_activeRows[i])
  {
    float curDiff=0;
    int curCount=0;
    for (int j=0;j<_size;j++) if (i!=j && _activeRows[j])
    {
      float df=Get(i,j);
      if (df<=groupSize)
      {
        curCount++;
        curDiff+=df;
      }
    }
    if (curCount>bestCount) 
    {
      bestCount=curCount;
      bestDistance=curDiff;
      bestPoint=i;
    }
    else if (curCount==bestCount && bestDistance>curDiff)
    {
      bestDistance=curDiff;
      bestPoint=i;
    }    
  }
  if (bestPoint==-1)
  {
    for (int i=0;i<_size;i++)
    {
      if (_activeRows[i])
      {
        bestPoint=i;
        pointList[0]=i;
        return 1;
      }
    }
    return 0;
  }
  else
  {    
//    LogF("Best %d ",bestPoint);
    int counter=1;
    *pointList++=bestPoint; //zapis ten ktery je best
    for (int i=0;i<_size;i++) if (i!=bestPoint && _activeRows[i]) //pro vsechny, ktere nejsou zarazeny a jsou aktivni
    {
      float df=Get(bestPoint,i);          //vyzvedni rozdil
      if (df<=groupSize )
      {       
        //LogF("...%d %g",i,Get(bestPoint,i));
        *pointList++=i;     
        counter++;
      }
    }
    return counter;
  }
}

int IncidenceTableForGroups::FindGoodGroupLevels(float groupSize, int &bestPoint, bool *disabledRows, int level)
{
  float bestDistance=FLT_MAX;
  int bestCount=0;
  bestPoint=-1;

  for (int i=0;i<_size;i++) if (_activeRows[i] && !disabledRows[i])
  {
    float curDiff=0;
    int curCount=0;
    for (int j=0;j<_size;j++) if (i!=j && _activeRows[j])
    {
      float df=Get(i,j);
      if (df<=groupSize)
      {
        curCount++;
        curDiff+=df;
      }
    }
    if (curCount>bestCount) 
    {
      bestCount=curCount;
      bestDistance=curDiff;
      bestPoint=i;
    }
    else if (curCount==bestCount && bestDistance>curDiff && curCount>0)
    {
      bestDistance=curDiff;
      bestPoint=i;
    }    
  }
//  LogF("Level %d, bestCount %d",level,bestCount);
  if (bestPoint==-1) 
  {
    bestPoint=-1;
    return level;
  }
  int *removedRows=(int *)alloca(sizeof(int)*(bestCount+1));
  int *p=removedRows;  
  for (int i=0;i<_size;i++) if (_activeRows[i])
  {
    float df=Get(bestPoint,i);          //vyzvedni rozdil
    if (df<=groupSize) 
    {
      RemoveRow(i);
      *p++=i;
    }
  }
  RemoveRow(bestPoint);
  *p++=bestPoint;
  int lev=GetGoodGroupRecursive(groupSize,level);
  for (int i=0;i<=bestCount;i++) RemoveRow(removedRows[i],false);
  return lev*bestCount;

}

int IncidenceTableForGroups::GetGoodGroupRecursive(float groupSize, int level)
{
  int *pointList=(int *)alloca(_size*sizeof(int));
  int bestPoint;
  return GetGoodGroup(groupSize,bestPoint,pointList);
}

int IncidenceTableForGroups::GetGoodGroup2(float groupSize, int &bestPoint, int *pointList)
{
  bool *disabledRows=(bool *)alloca(_size);
  int remain=0;
  for (int i=0;i<_size;i++) if (_activeRows[i]) remain++;
  int bestSz=0;
  int bestSel=-1;
  int lastCnt=0;
  int counter=0;
  while (lastCnt>=bestSz/2 || counter<5)
  {
    counter++;
    int sel;
    lastCnt=FindGoodGroupLevels(groupSize, sel, disabledRows,0);
    if (sel==-1) break;
    if (lastCnt>bestSz)
    {
      bestSz=lastCnt;
      bestSel=sel;
      LogF("Found best group %d - score: %d",bestSel,bestSz);
    }
    disabledRows[sel]=true;
  }
  if (bestSel==-1)
  {
    return GetGoodGroup(groupSize,bestPoint,pointList);
  }
  else
  {    
    bestPoint=bestSel;
 //   LogF("Best %d ",bestPoint);
    int counter=1;
    *pointList++=bestPoint; //zapis ten ktery je best
    for (int i=0;i<_size;i++) if (i!=bestPoint && _activeRows[i]) //pro vsechny, ktere nejsou zarazeny a jsou aktivni
    {
      float df=Get(bestPoint,i);          //vyzvedni rozdil
      if (df<=groupSize )
      {       
//        LogF("...%d %g",i,Get(bestPoint,i));
        *pointList++=i;     
        counter++;
      }
    }
    return counter;
  }

}



IncidenceTableBestGroups::IncidenceTableBestGroups(int size,bool alloc):IncidenceTable(size,alloc)
{
  _badDistance=new float[size];
  _groupIndex=new int[size];
  _forbidenPoints=new bool[size];
}
IncidenceTableBestGroups::~IncidenceTableBestGroups()
{
  delete [] _badDistance;
  delete [] _groupIndex;
  delete [] _forbidenPoints;
}


int IncidenceTableBestGroups::CreateGroups(float maxDistance)
{
  for (int i=0;i<_size;i++) {_groupIndex[i]=-1;_badDistance[i]=0;_forbidenPoints[i]=false;}


  int counter=0;
  while (CreateGroup(maxDistance,counter))
  {    
    counter++;
  }

  return counter;
}

bool IncidenceTableBestGroups::CreateGroup(float maxDistance,int index)
{
  int *groupPoints=(int *)alloca(_size*sizeof(int));

  int groupSize=0;
  int firstPoint=FindFirstPoint();
  if (firstPoint==-1) return false;
  groupPoints[groupSize++]=firstPoint;
  _badDistance[firstPoint]=0;
  _groupIndex[firstPoint]=index;
  for (int i=firstPoint+1;i<_size;i++) if (_groupIndex[i]==-1)
  {
    groupSize=AddPointToGroup(groupPoints,groupSize,i,index,maxDistance);
  }
  for (int i=0;i<_size;i++) if (_groupIndex[i]!=-1 && _groupIndex[i]!=index)
  {
    groupSize=AddPointToGroup(groupPoints,groupSize,i,index,maxDistance);
  }
  return true;
}

int IncidenceTableBestGroups::FindFirstPoint()
{
  for (int i=0;i<_size;i++) if (_groupIndex[i]==-1) return i;
  return -1;
}

int IncidenceTableBestGroups::AddPointToGroup(int *groupPoints,int groupSize, int point, int groupIndex, float condition)
{
  float worseDistance=0;
  for (int i=0;i<groupSize;i++)
  {
    float d=Get(groupPoints[i],point);
    if (d>condition) return groupSize;
    worseDistance+=d;  
  }
  if (_groupIndex[point]!=-1 && _badDistance[point]<worseDistance) return groupSize;
  for (int i=0;i<groupSize;i++)
  {
    float d=Get(groupPoints[i],point);    
    _badDistance[groupPoints[i]]+=d;
  }
  groupPoints[groupSize++]=point;
  _badDistance[point]=worseDistance;
  _groupIndex[point]=groupIndex;
  return groupSize;
}

int IncidenceTableBestGroups::GetGroup(int groupIndex, int &bestPoint, int *pointList)
{
  float minDist=FLT_MAX;
  bestPoint=-1;
  for (int i=0;i<_size;i++) if (_groupIndex[i]==groupIndex && !_forbidenPoints[i])
  {    
    float curDist=0;
    for (int j=0;j<_size;j++) if (_groupIndex[j]==groupIndex)
      curDist+=Get(i,j);
    if (curDist<minDist)
    {
      bestPoint=i;
      minDist=curDist;
    }
  }
  if (bestPoint==-1)
  {
    return 0;
  }
  else
  {
    int count=0;
    for (int i=0;i<_size;i++) if (_groupIndex[i]==groupIndex)
    {
      *pointList++=i;
      count++;
    }
    return count;
  }
}

int IncidenceTableBestGroups::GetTrueGroupCount() const
{
  AutoArray<int,MemAllocLocal<int, 256> > hlp;
  for (int i=0;i<_size;i++)
  {
    int idx=_groupIndex[i];
    if (idx>=hlp.Size()) 
    {
      for (int i=hlp.Size();i<=idx;i++) hlp.Add(0)      ;
    }
    hlp[idx]++;
  }

  int count=0;
  for (int i=0;i<hlp.Size();i++) if (hlp[i]) count++;
  return count;
}

