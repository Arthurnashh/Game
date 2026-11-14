// implementation of OperMap 

#include "../wpch.hpp"

#include "supressId.hpp"
#include "ai.hpp"
#include "aiDefs.hpp"
#include "operMap.hpp"
//#include "../landscape.hpp"
#include <El/DebugWindow/debugWin.hpp>

#include <El/Common/perfProf.hpp>
#include <El/Common/perfLog.hpp>
#include <Es/Containers/quadtreeEx.hpp>
#include "../landscape.hpp"
#include "../roads.hpp"
#include "../diagModes.hpp"
#include "../global.hpp"

#pragma optimize ("t",on)

void SuppressTargetList::CollectTargets(AIBrain *unit)
{
  EntityAI *veh = unit->GetVehicle();

  // slight avoid even my own line of fire - chance is someone else might be firing along it as well
  const float myCost = 1e-7f;
  // as fire sectors create huge areas, we need to assign almost zero cost to the friendly fire sectors
  const float friendlyCost = 1e-6f;
  // enemy fire is very high cost - not absolute, but still extremely high
  const float enemyCost = 1000.0f;

  Clear();
  Add(SuppressTargetInfo(veh->GetObjectId(), myCost));
  const TargetList *targetList = unit->AccessTargetList();
  if (targetList)
  {
    for (int i=0; i<targetList->AnyCount(); i++)
    {
      const TargetNormal *target = targetList->GetAny(i);
      if (!target) continue;
      const TargetType *targetId = target->idExact;
      if (!targetId || targetId == veh) continue; // my vehicle already added
      // ignore targets which we do not know about
      if (!target->IsKnownBy(unit)) continue;
      // ignore targets with no known weapons
      if (!targetId->IsCombatUnit()) continue;

      // check suppression factor - this considers only suppression known to our group
      float suppressed = target->FadingSuppressed();
      // check if given source is friendly or enemy
      float baseCost = unit->IsEnemy(target->GetSide()) ? enemyCost : friendlyCost;
      // note: friendly targets should normally not be suppressed by us, but it should do no harm
      Add(SuppressTargetInfo(targetId->GetObjectId(), baseCost * (1 - suppressed)));
    }
  }
}

float SourceId::GetCost(const SuppressTargetList &list) const
{
  int index = list.FindKey(_source);
  // if we do not know the shooter, we will not count its fire
  if (index < 0) return 0;
  const SuppressTargetInfo &tgt = list[index];

  // consider age of the fire
  float ageFactor = InterpolativC(Glob.time-_time,5.0f,60.0f,1.0f,0.01f);
  float hit = _ammoHit * ageFactor;
  return tgt._costCoef * hit;
}


/// implementation of SuppressMap interface
class SuppressFieldFull: public SuppressField, public SupressIdList
{
protected:
  /// data of all layers
  unsigned int _data[OperItemRange][OperItemRange];
  
  void AndData(int mask);
  void OrData(int mask);
  
public:
  void ClearLayer(int id);
  
  SuppressFieldFull(LandIndex x, LandIndex z);

  /// notification about a destroyed target
  virtual void RemoveSource(EntityAI *source);
  
  virtual float GetCost(int x, int z, const SuppressTargetList &list) const;
  
  //@{ access to individual bits in the layer
  bool GetItem(int id, int x, int z) const {return (_data[z][x]&(1<<id))!=0;}
  void SetItem(int id, int x, int z, bool state) {_data[z][x] |= 1<<id;}
  //@}
};

// DEFINE_FAST_ALLOCATOR(QuadTreeExRoot<SuppressFieldFull *>::QuadTreeType)

SuppressFieldFull::SuppressFieldFull(LandIndex x, LandIndex z)
: SuppressField(x, z)
{
  memset(_data,0,sizeof(_data));
}

inline void SuppressFieldFull::AndData(int mask)
{
  for (int zz=0; zz<OperItemRange; zz++)
  for (int xx=0; xx<OperItemRange; xx++)
  {
    _data[zz][xx] &= mask;
  }
}
inline void SuppressFieldFull::OrData(int mask)
{
  for (int zz=0; zz<OperItemRange; zz++)
  for (int xx=0; xx<OperItemRange; xx++)
  {
    _data[zz][xx] |= mask;
  }
}

void SuppressFieldFull::ClearLayer(int id)
{
  AndData(~(1<<id));
}


static inline float ComputePriority(EntityAI *source, float ammoHit, float ammoPrior)
{
  return ammoHit*ammoPrior;
}

SupressIdList::SupressIdList()
{
  for (int i=0; i<MaxSources; i++)
  {
    _sources[i]._state = SupSrcNone;
    _sources[i]._priority = 0;
  }
}

int SupressIdList::AddId(EntityAI *source, Vector3Par srcPos, Vector3Par srcDir, float ammoHit, float ammoPrior)
{
  float newPrior = ComputePriority(source,ammoHit,ammoPrior);
  
  // check if this ID is already present
  ObjectId sourceId = source->GetObjectId();
  // source ID needs to be valid, otherwise it cannot be used for identification
  DoAssert(!sourceId.IsNull());
  if (sourceId.IsNull())
  {
    return -1;
  }
  for (int i=0; i<MaxSources; i++)
  {
    SourceId &s = _sources[i];
    // different source or different ammo means no match
    if (s._source!=sourceId || s._ammoHit!=ammoHit) continue;
    const float maxAngleCos = 0.96592582629f; // cos 15 deg
    if (s._srcDir*srcDir<maxAngleCos) continue;
    if (s._srcPos.Distance2(srcPos)>Square(3*OperItemGrid)) continue;
    Assert(s._state!=SupSrcNone);
    // we want to update part of the the description
    s._time = Glob.time;
    s._priority = newPrior;
    // do not update position / direction - when it is updated, there is no upper bound about what old data may be contained
    // because the gradual incremental change may change the original values significantly
    //s._srcPos = srcPos;
    //s._srcDir = srcDir;
    return i;
  }
  
  // when not, we may need to create it
  // find a slot with lowest priority
  // we are interested only in slots with a priority lower than we have
  float minPrior = newPrior;
  int minPriorI = -1;
  for (int i=0; i<MaxSources; i++)
  {
    const SourceId &s = _sources[i];
    float ageFactor = InterpolativC(Glob.time-s._time,5.0f,60.0f,1.0f,0.01f);
    float prior = s.Priority()*ageFactor;
    if (minPrior>prior)
    {
      minPrior = prior;
      minPriorI = i;
      if (minPrior<=0) break;
    }
  }
  if (minPriorI<0)
  {
    return -1;
  }
  // we are evicting some slot because it has a lower priority
  ClearLayer(minPriorI);
  
  SourceId &s = _sources[minPriorI];
  s._state = SupSrcActive;
  s._source = sourceId;
  s._srcPos = srcPos;
  s._srcDir = srcDir;
  s._ammoHit = ammoHit;
  s._priority = newPrior;
  s._time = Glob.time;
  
  return minPriorI;
}

float SupressIdList::GetTotalCost(const SuppressTargetList &list) const
{
  float totalCost = 0;
  for (int i=0; i<MaxSources; i++)
  {
    if (_sources[i]._source.IsNull()) continue;
    if (_sources[i]._state!=SupSrcActive) continue;
    totalCost += _sources[i].GetCost(list);
  }
  return totalCost;
}

float SuppressFieldFull::GetCost(int x, int z, const SuppressTargetList &list) const
{
  int data = _data[z][x];
  // handle very common special case - no records in given field
  if (data==0) return 0;
  Assert(x>=0 && x<OperItemRange);
  Assert(z>=0 && z<OperItemRange);
  // based on profiling, we might choose an alternate solution:
  // cost of individual layers could be precomputed and we could compute only the "dot product" here
  float totalCost = 0;
  // if there are multiple layers by the same entity, we want it to be computed only once
  // we could do it in some other way, but a simple way to achieve this is to use max instead of sum
  // this way we will handle some other situation not that well, like two enemy zone crossing,
  // but even in such situation the result should be reasonable
  for (int i=0; i<MaxSources; i++)
  {
    if (data&(1<<i))
    {
      const SourceId &s = _sources[i];
      // if source no longer exists, ignore its fire
      if (s._source.IsNull()) continue;
      if (s._state!=SupSrcActive) continue;
      
      float cost = s.GetCost(list);
      saturateMax(totalCost, cost);
    }
    else if ((data&(~0U<<i))==0)
    {
      // all successive fields empty - nothing more to track
      break;
    }
    
  }
  return totalCost;
}

void SuppressFieldFull::RemoveSource(EntityAI *source)
{
  int mask = 0;
  ObjectId sourceId = source->GetObjectId();
  for (int i=0; i<MaxSources; i++)
  {
    if (_sources[i]._source==sourceId)
    {
      mask |= 1<<i;
      // clear the source ID
      _sources[i]._source = ObjectId();
      _sources[i]._ammoHit = 0;
      _sources[i]._priority = 0;
    }
  }
  if (mask) AndData(~mask);
}

TypeIsSimple(SuppressFieldFull *)

class SuppressCache: public ISuppressCache, public MemoryFreeOnDemandHelper
{
private:
	TListBidirRef<SuppressFieldFull, SimpleCounter<SuppressFieldFull> > _fields;
  QuadTreeExRoot<SuppressFieldFull *> _index;

public:
  SuppressCache(Landscape *land);
	~SuppressCache() {Destroy();}

  virtual void Trace(EntityAI *source, float ammoHit, float distanceLeft, Vector3Par tBeg, Vector3Par tEnd);
  virtual float GetSuppressionLevel(Vector3Par pos, float radius, AIBrain *unit);
  
  virtual SuppressFieldFull *GetField(LandIndex x, LandIndex z, int mask);
  virtual void RemoveField(SuppressField * fld);
  virtual void RemoveField(LandIndex x, LandIndex z);
  virtual void Update();
  virtual int NFields() const;

	//@{ implementation of MemoryFreeOnDemandHelper interface
	virtual size_t FreeOneItem();
	virtual float Priority() const;
  virtual size_t MemoryControlled() const;
  virtual RString GetDebugName() const;
  //@}

private:
	void Destroy();
};

SuppressCache::SuppressCache(Landscape *land)
: _index(land->GetLandRange(), land->GetLandRange(), NULL)
{
	RegisterFreeOnDemandMemory(this);
}

SuppressFieldFull* SuppressCache::GetField(LandIndex x, LandIndex z, int mask)
{
	if (!InRange(z, x))
		return NULL;
	SuppressFieldFull *fld = _index(x,z);
	
	// MASK_NO_UPDATE means we want to only inspect the state of the field, never create or update it
	if (mask&MASK_NO_UPDATE) return fld;
	
	if (fld)
	{
		Assert( fld->_x == x);
		Assert( fld->_z == z);
		Ref<SuppressFieldFull> temp = fld;
		_fields.Delete(fld); // remove from current location
		_fields.Insert(fld); // insert as first
		fld->_lastUsed = Glob.time;
		return fld;
	}

	if (mask&MASK_NO_CREATE) return NULL;
	
	Ref<SuppressFieldFull> entry = new SuppressFieldFull(x, z);

	// create a new entry
	_fields.Insert(entry);
  _index.Set(x, z, entry);
	entry->_lastUsed = Glob.time;
	return entry;
}

float SuppressCache::GetSuppressionLevel(Vector3Par pos, float radius, AIBrain *unit)
{
  SuppressTargetList suppressTargets;
  suppressTargets.CollectTargets(unit);

  OperMapIndex x(toIntFloor(pos.X()*InvOperItemGrid));
  OperMapIndex z(toIntFloor(pos.X()*InvOperItemGrid));
  int rad = toIntCeil(radius*InvOperItemGrid);
  float cost = 0;
  for (OperMapIndex zz(z-rad); zz<=z+rad; zz++) for (OperMapIndex xx(x-rad); xx<=x+rad; xx++)
  {
    int dist2 = zz*zz+xx*xx;
    if (dist2>=rad*rad) continue;
    float dist = sqrt(zz*zz+xx*xx);
    float weight = Interpolativ(dist,0,rad,1,0);
    SPLIT_INDICES(xx, zz, x0, z0, x1, z1);
    Ref<SuppressFieldFull> field = GetField(x0,z0,MASK_NO_CREATE);
    if (field)
    {
      // compute weighted average
      // total weight for all visited fields should be 1?
      cost += field->GetCost(x1,z1,suppressTargets)*weight;
    }
  }
  return cost;
}

void SuppressCache::RemoveField(SuppressField * fld)
{
  _index.Set(fld->_x, fld->_z, NULL);
	_fields.Delete(static_cast<SuppressFieldFull *>(fld));
}
void SuppressCache::RemoveField(LandIndex x, LandIndex z)
{
	if (!InRange(z, x)) return;
	SuppressFieldFull *fld = _index(x,z);
	if (!fld) return;
	RemoveField(fld);
}
void SuppressCache::Update()
{

}

static inline float FloatSign(float x) {return x>=0 ? 1.0f : -1.0f;}

void SuppressCache::Trace(EntityAI *source, float ammoHit, float distanceLeft, Vector3Par tBeg, Vector3Par tEnd)
{
  // traverse all fields on end..beg
  PROFILE_SCOPE_DETAIL_EX(scTrc,coll);

  // adapted from Landscape::IntersectWithGround
  Vector3 dir = tEnd-tBeg;
  // if there is no horizontal movement, no need to rasterize
  if (dir.SquareSizeXZ()<=0)
  {
    return;
  }
  float size2 = dir.SquareSize();
  float invSize = InvSqrt(size2);
  float size = invSize * size2;
  Vector3 dNorm = dir*invSize;
  
  Vector3 pos = tBeg;

  float deltaX = dNorm.X();
  float deltaZ = dNorm.Z();

  LandIndex xInt(toIntFloor(tBeg.X()*InvLandGrid));
  LandIndex zInt(toIntFloor(tBeg.Z()*InvLandGrid));

  float invDeltaX = fabs(deltaX)<1e-10f ? FloatSign(deltaX)*1e10f : 1/deltaX;
  float invDeltaZ = fabs(deltaZ)<1e-10f ? FloatSign(deltaZ)*1e10f : 1/deltaZ;
  int ddx = deltaX>=0 ? 1 : -1;
  int ddz = deltaZ>=0 ? 1 : -1;
  float dnx = deltaX>=0 ? LandGrid : 0;
  float dnz = deltaZ>=0 ? LandGrid : 0;

  // maintain beg, end on current square
  float tDone = 0;
  float tRest = size;
  while (tRest>0)
  {
    Vector3 beg = pos;

    //advance to next relevant neighbour
    LandIndex xio(xInt), zio(zInt);
    float tx = (xInt*LandGrid+dnx-pos.X()) * invDeltaX;
    float tz = (zInt*LandGrid+dnz-pos.Z()) * invDeltaZ;
    //Assert( tx>=-0.01 );
    //Assert( tz>=-0.01 );
    float tDoneBeg = tDone;
    if (tx<=tz)
    {
      saturateMin(tx,tRest);
      xInt += ddx;
      tRest -= tx;
      tDone += tx;
      pos += dNorm*tx;
    }
    else
    {
      saturateMin(tz,tRest);
      zInt += ddz;
      tRest -= tz;
      tDone += tz;
      pos += dNorm*tz;
    }

    // check beg..pos segment in the xio, zio grid
    {
      // trace suppression for roads
      if (InRange(xio, zio))
      {
        const RoadList &roadList = GLandscape->GetRoadNet()->GetRoadList(xio, zio);
        for (int i=0; i<roadList.Size(); i++)
        {
          RoadLink *link = roadList[i];
          link->TraceSuppression(source,source->FutureVisualState().Position(),dNorm,ammoHit,distanceLeft,beg,pos);
        }
      
      }

      // for each grid acquire ID and rasterize the line
      SuppressFieldFull *field = GetField(xio,zio,0);
      if (field)
      {
        int id = field->AddId(source,source->FutureVisualState().Position(),dNorm,ammoHit,distanceLeft);
        if (id>=0)
        {
          #if 0
          // for each ID we want only the last bullet to be tracked
          field->ClearLayer(id);
          #endif
          // now we want to rasterize the line
          // we need the line to be uncrossable by A*, which means we need to rasterize each field we touch
          // we do it the same way we did for the large fields, i.e. always determining the next node to visit
          
          Vector3 ppos = beg;
          int xxInt = toIntFloor(beg.X()*InvOperItemGrid);
          int zzInt = toIntFloor(beg.Z()*InvOperItemGrid);

          float dnxx = deltaX>=0 ? OperItemGrid : 0;
          float dnzz = deltaZ>=0 ? OperItemGrid : 0;
          
          float fRest = tDone-tDoneBeg;
          while (fRest>0)
          {
            //advance to next relevant neighbour
            int xxio = xxInt-xio*OperItemRange, zzio = zzInt-zio*OperItemRange;
            float actX = xxInt*OperItemGrid+dnxx;
            float actZ = zzInt*OperItemGrid+dnzz;
            float txx = (actX-ppos.X()) * invDeltaX;
            float tzz = (actZ-ppos.Z()) * invDeltaZ;
            if (txx<=tzz)
            {
              saturateMin(txx,fRest);
              xxInt += ddx;
              fRest -= txx;
              ppos += dNorm*txx;
            }
            else
            {
              saturateMin(tzz,fRest);
              zzInt += ddz;
              fRest -= tzz;
              ppos += dNorm*tzz;
            }
            
            // TODO: sometimes it should be possible to skip SurfaceY
            float surfY = GLandscape->SurfaceY(actX,actZ);
            // check if we are close to the surface to be relevant
            // if we fly too high or too low, we can be ignored
            if (ppos.Y()>=surfY-2 && ppos.Y()<=surfY+2)
            {
              if (xxio>=0 && xxio<OperItemRange && zzio>=0 && zzio<OperItemRange)
              {
                field->SetItem(id,xxio,zzio,true);
              }
            }
          }
        }
      
      }
    
    }
    
    // reduce the distanceLeft as you proceed
    distanceLeft -= tDone-tDoneBeg;
  }
  
  
  
}

int SuppressCache::NFields() const
{
  return _fields.GetCounter().GetCount();
}

void SuppressCache::Destroy()
{

}

size_t SuppressCache::FreeOneItem()
{
	SuppressFieldFull *fld =_fields.Last();
	if (!fld) return 0;
	RemoveField(fld);
	return sizeof(SuppressFieldFull);
}

float SuppressCache::Priority() const
{
	return 0.2f;
}

RString SuppressCache::GetDebugName() const
{
  return "Suppression";
}

size_t SuppressCache::MemoryControlled() const
{
  return _fields.GetCounter().GetCount() * sizeof(SuppressFieldFull);
}

ISuppressCache *CreateSuppressCache(Landscape *land){return new SuppressCache(land);}
