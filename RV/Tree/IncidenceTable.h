#pragma once

class IInciTableOnDemandCalc
{
public:
    virtual float Get(int a, int b) const = 0;
};

class IncidenceTable
{
protected:
  float *_array2d;
  int _size;
  IInciTableOnDemandCalc *onDemand;
  float GetAllocated(int a, int b) const;
  float GetNotAllocated(int a, int b) const;
public:
  IncidenceTable(int size, bool alloc = true);
  ~IncidenceTable();
  
  void Set(int a, int b, float v);
  float (IncidenceTable::*GetF )(int a, int b) const ;
  float Get(int a, int b) const  {return (this->*GetF)(a,b);}

  float operator()(int a, int b) const {return Get(a,b);}  
  void SetOnDemand(IInciTableOnDemandCalc *onDemand);
  int Size() const {return _size;}
};

class IncidenceTableForGroups: public IncidenceTable
{
protected:
  bool *_activeRows;
public:
  IncidenceTableForGroups(int size, bool alloc = true);
  ~IncidenceTableForGroups();

  void RemoveRow(int i, bool remove=true);
  int GetGoodGroup(float groupSize, int &bestPoint, int *pointList);
  int GetGoodGroup2(float groupSize, int &bestPoint, int *pointList);
  int FindGoodGroupLevels(float groupSize, int &bestPoint, bool *disabledRows, int maxlevel);
  int GetGoodGroupRecursive(float groupSize, int maxlevel);

};

class IncidenceTableBestGroups: public IncidenceTable
{
protected:
  float *_badDistance;
  int *_groupIndex;
  bool *_forbidenPoints;

  bool CreateGroup(float maxDistance,int index);
  int FindFirstPoint();
  int AddPointToGroup(int *groupPoints,int groupSize, int point, int groupIndex, float condition);
public:
  IncidenceTableBestGroups(int size, bool alloc = true);
  ~IncidenceTableBestGroups();

  int CreateGroups(float maxDistance);
  int GetGroup(int groupIndex, int &bestPoint, int *pointList);
  void SetPointForbiden(int point) {_forbidenPoints[point]=true;}
  int GetTrueGroupCount() const;


};

