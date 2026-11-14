// implementation of Path steering

#include "wpch.hpp"

#include "pathSteer.hpp"
#include "landscape.hpp"
#include "AI/ai.hpp"
#include "AI/operMap.hpp"
#include "global.hpp"
#include "Network/network.hpp"
#include "paramArchiveExt.hpp"

// # pragma optimize( "", off )

DEFINE_ENUM(ASONodeType, NT, ASO_NODE_TYPE_ENUM)

inline float Dist2( const OperInfoResult &info, Vector3Par pos )
{
	return info._pos.Distance2(pos);
}

int Path::FindNearest( Vector3Par pos ) const
{
	// find nearest point on path
	int minI=-1;
	float minDist2=1e10;
	for( int i=0; i<Size(); i++ )
	{
		const OperInfoResult &info=Get(i);
		float dist2=Dist2(info,pos);
		if( minDist2>dist2 )
		{
			minDist2=dist2;
			minI=i;
		}
	}
	return minI;
}

float Path::Distance( int index, Vector3Par pos ) const
{
	const OperInfoResult &curr=Get(index);
	const OperInfoResult &next=Get(index+1);
	Vector3 b = curr._pos;
	Vector3 e = next._pos - curr._pos;
	Vector3 p = pos - curr._pos;
	float eSize2 = e.SquareSize();
	if (eSize2>0)
	{
	  float t=(e*p)/e.SquareSize();
	  saturate(t,0,1);
	  Vector3 nearest=b+t*e;
	  float dist2Next=nearest.Distance2(pos);
	  return dist2Next;
	}
	else
	{
	  // b and e is the same point
	  return b.Distance2(pos);
	}
}

int Path::FindNext(Vector3Par pos, bool minCost) const
{
	// find index so that distance of line index-1,index to pos 
	// is minimal
	int index=0;
	float minDist=1e10;
  const float epsilon = 0.10;
	for( int i=0; i<Size()-1; i++ )
	{
    if (minCost)
    {
      const OperInfoResult &next = Get(i + 1);
      if (_minCost > next._cost) continue;
    }
		float dist=Distance(i,pos);
		if (minDist >= dist - epsilon) minDist = dist, index=i;
	}
	return index+1;
}

int Path::FindNextCost(float cost) const
{
  // at least two elements assumed
  Assert(Size()>=1);
	// find index so that distance of line index-1,index to pos 
	// is minimal
  // find first such as Get(i)._cost>cost
  // returning zero has no sense - we always want to return a segment
	for( int i=1; i<Size()-1; i++ )
	{
    const OperInfoResult &info = Get(i);
    if (info._cost>=cost)
    {
      return i;
    }
	}
  // we can find more than the last
	return Size()-1;
}

inline Vector3Val OperPos( const OperInfoResult &info )
{
	return info._pos;
}


Path::Path()
{
  _minCost = 0;
  _openCover = false;
  _completed = true;
}

//#include "scene.hpp"

float Path::CostAtPos(Vector3Par pos, bool updateCost) const
{
	if( Size()<2 )
  {
    if (Size()<1)
      return 0;
    return Get(0)._cost;
  }
	int index=FindNext(pos, updateCost);
	if( index<1 )
	{
		Fail("Bad next.");
		return Get(0)._cost;
	}
	const OperInfoResult &curr=Get(index);
	const OperInfoResult &prev=Get(index-1);
	float tPrev = NearestPointT(curr._pos,prev._pos,pos);
	saturate(tPrev,0,1);
  float cost = curr._cost+(prev._cost-curr._cost)*tPrev;
  // following assumption is often correct, however when updateCost is used, it may fail
  //Assert(PosAtCost(cost).Distance2(NearestPos(pos))<Square(0.5f));
  if (updateCost) saturateMax(cost, _minCost);
  return cost;
}

Vector3 Path::PosAtDist(Vector3Par pos, float dist)
{
  // first find the original position on the path
  if (Size() < 2) return Get(0)._pos;
  int index = FindNext(pos);
  if (index < 1)
  {
    Fail("Bad next.");
    return Get(0)._pos;
  }
  
  // convert to searching from pos0
  Vector3Val pos0 = Get(index - 1)._pos;
  Vector3Val pos1 = Get(index)._pos;
  dist += pos0.Distance(NearestPoint(pos0, pos1, pos));

  // follow the path
  for (int i=index; i<Size(); i++)
  {
    Vector3Val pos0 = Get(i - 1)._pos;
    Vector3Val pos1 = Get(i)._pos;
    float segmentDist = pos1.Distance(pos0);
    if (segmentDist >= dist)
    {
      // found, interpolate
      return pos0 + (dist / segmentDist) * (pos1 - pos0);
    }
    dist -= segmentDist;
  }
  // not found, return the end of the path
  return Get(Size() - 1)._pos;
}

Vector3 Path::NearestPos( Vector3Par pos ) const
{
	if( Size()<2 ) return Last()._pos;
	int index=FindNext(pos);
	if( index<=0 )
	{
		Fail("Bad next.");
		return Last()._pos;
	}
	const OperInfoResult &curr=Get(index);
	const OperInfoResult &prev=Get(index-1);

	return NearestPoint(curr._pos,prev._pos,pos);
}

static inline Vector3 NearestPointXZ( Vector3Par beg, Vector3Par end, Vector3Par pos )
{
  // point on line beg .. end nearest to pos
  Vector3 eb = end - beg;
  Vector3 pb = pos - beg;
  eb[1] = 0;
  pb[1] = 0;
  float denom2 = eb.SquareSize();
  if (denom2<=0) return beg;
  float t = (eb*pb) / denom2;
  saturate(t,0,1);
  return beg+eb*t;
}

Vector3 Path::NearestPosXZ( Vector3Par pos ) const
{
	if( Size()<2 ) return Last()._pos;
	int index=FindNext(pos);
	if( index<=0 )
	{
		Fail("Bad next.");
		return Last()._pos;
	}
	const OperInfoResult &curr=Get(index);
	const OperInfoResult &prev=Get(index-1);

	return NearestPointXZ(curr._pos,prev._pos,pos);
}

Vector3 Path::PosAtCost( float cost ) const
{
	int next = FindNextCost(cost);
	if( next>=Size() )
	{
		// return end of path
		return OperPos(Last());
	}
	// interpolate
	if (next<=0)
	{
	  const OperInfoResult &nInfo=Get(next);
	  return OperPos(nInfo);
	}
	const OperInfoResult &pInfo=Get(next-1);
	const OperInfoResult &nInfo=Get(next);
	float denom=nInfo._cost-pInfo._cost;
	if( denom<=0 ) return OperPos(nInfo);
	float factor=(cost-pInfo._cost)/denom;
	Assert( factor>=-0.001 );
	Assert( factor<=+1.001 );

	Vector3Val beg = pInfo._pos;
	Vector3Val end = nInfo._pos;

	return beg * (1-factor) + end * factor;
}

float Path::DistanceAtCost( float cost ) const
{
  // first find the original position on the path
  if (Size() < 2) return 0;
  int index = FindNextCost(cost);
  if (index < 1)
  {
    return 0;
  }
  float dist = 0;
  // distance from the previous point
  Vector3 pos = PosAtCost(cost);
  Vector3Val pos0 = Get(index - 1)._pos;
  dist += pos0.Distance(pos);

  // follow the path, add distance to all segments
  for (int i=1; i<index; i++)
  {
    Vector3Val pos0 = Get(i - 1)._pos;
    Vector3Val pos1 = Get(i)._pos;
    dist += pos1.Distance(pos0);
  }
  return dist;
}

#include "scene.hpp"

bool Path::InHouseAtCost( float cost, Vector3Par pos ) const
{
	int next;
	int size=Size();
	if (size<=0) return false;
	for( next=1; next<size; next++ )
	{
		if( Get(next)._cost>=cost ) break;
	}
	if( next>=size )
	{
		// return end of path
		const OperInfoResult &last=Get(size-1);
		return last._house!=NULL;
	}
	// interpolate
	const OperInfoResult &pInfo=Get(next-1);
	const OperInfoResult &nInfo=Get(next);

	return pInfo._house!=NULL && nInfo._house!=NULL;
}

static bool PathActionEdge(const PathAction *from, const PathAction *to)
{
  if (!from) return false;
  if (to)
  {
    // edge, check if right type
    switch (from->GetType())
    {
    case PATLadderTop:
      return to->GetType() == PATLadderBottom;
    case PATLadderBottom:
      return to->GetType() == PATLadderTop;
    case PATUserActionBegin:
      return to->GetType() == PATUserActionEnd;
    case PATUserActionEnd:
      return to->GetType() == PATUserActionBegin;
    }
  }
  else
  {
    // no edge, check if edge is needed
    switch (from->GetType())
    {
    case PATLadderTop:
    case PATLadderBottom:
    case PATUserActionBegin:
    case PATUserActionEnd:
      return false;
    }
  }
  return true;
}

Vector3 Path::PosAtCost(
  float cost, float minDist, Vector3Par point, float originalCost, Ref<const PathAction> *action, OLink(Object) *building, Clearance *clearance
) const
{
  Assert(Size()>=1);
  // Note: original cost is not necessarily the same as NearestPos(point)
  // original cost is guaranteed to always go forward

  // the starting point for the search is at cost originalCost
  Vector3 startPos = PosAtCost(originalCost);
  // subtract that from the distance required
  minDist -= startPos.Distance(point);

  int start = FindNextCost(originalCost);
  // we start on start-1 .. start
  //Assert(start>0); //if (start<=0){}
  // startPos should now lie on Get(start-1)._pos ... Get(start)._pos line
  Assert( start<=0 || startPos.Distance2(NearestPoint(Get(start-1)._pos,Get(start)._pos,startPos))<Square(0.3f));
  
	int size=Size();
  // while searching, consider minimal distance as well
  // part of the first segment until startPos is already passed
  Vector3 lastPos = startPos;
  int next;
	for( next=start; next<size; next++ )
	{
    const OperInfoResult &info = Get(next);
    float distToNext = info._pos.Distance(lastPos);
		if (info._cost>=cost && distToNext>=minDist) break;
    minDist -= distToNext;
    lastPos = info._pos;
    if (clearance && *clearance>info._clearance)
    {
      *clearance = info._clearance;
    }
	}
	if( next>=size )
	{
		// return end of path
    const OperInfoResult &info = Get(size-1);
    if (action/* && originalCost < info._cost*/)
    {
      // edge detection
      if (PathActionEdge(info._action, NULL))
        *action = info._action;
    }
    if (building) *building = info._house;
		return OperPos(info);
	}
	if (next<=0)
	{
  	const OperInfoResult &nInfo=Get(next);
	  return OperPos(nInfo);
	}
	const OperInfoResult &nInfo=Get(next);
	const OperInfoResult &pInfo=Get(next-1);
  if (action/* && originalCost < pInfo._cost*/)
  {
    if (PathActionEdge(pInfo._action, nInfo._action))
      *action = pInfo._action;
  }
  if (building) *building = pInfo._house;

  // interpolate
  float denom = nInfo._cost-pInfo._cost;
	if( denom<=0 ) return OperPos(nInfo);
  
	float factorCost = (cost-pInfo._cost)/denom;
  // we still need to travel minDist on the next-1..next segment
  float dist2ToNext = nInfo._pos.Distance2(pInfo._pos);
  float factorDist = dist2ToNext>0 ? minDist*InvSqrt(dist2ToNext) : 0;
  float factor = floatMax(factorCost,factorDist);
	Assert( factor>=-0.001 );
	Assert( factor<=+1.001 );


	Vector3Val beg = pInfo._pos;
	Vector3Val end = nInfo._pos;

	// ret is point on line corresponding to cost
	Vector3 ret = beg * (1-factor) + end * factor;
	
  if (pInfo._type != NTGrid) return ret; // height update only when moving in landscape

	// the point must be somewhere above or below the path

	// containing point
	// i.e. everything should happen in plane containing beg,end and end+VUp
	// adjust direction is aside to beg..end

	
	Vector3Val eb = end - beg;

	if (eb.SquareSize()<1e-6)
	{
		// singular case - between two identical points
		LogF("Singular path");
		return ret;
	}
	// calculate normal of vertical plane containing eb
	Vector3 planeNormal = VUp.CrossProduct(eb);
	// calculate most vertical perpendicular to eb
	Vector3 perpFromEB = planeNormal.CrossProduct(eb);

	// search for point ret..ret+aside nearest to point
	Vector3 retPoint = NearestPointInfinite(ret,ret+perpFromEB,point);

	#if 0
	if (CHECK_DIAG(DECostMap)
	{

	// white ball - point
	{
		Ref<Object> obj=new ObjectColored(GScene->Preloaded(SphereModel),-1);
		obj->SetPosition(point);
		obj->SetScale(0.1);
		obj->SetConstantColor(PackedColor(Color(1,1,1)));
		GScene->ShowObject(obj);
	}

	// red ball - end
	{
		Ref<Object> obj=new ObjectColored(GScene->Preloaded(SphereModel),-1);
		obj->SetPosition(end);
		obj->SetScale(0.1);
		obj->SetConstantColor(PackedColor(Color(1,0,0)));
		GScene->ShowObject(obj);
	}

	// green ball - beg
	{
		Ref<Object> obj=new ObjectColored(GScene->Preloaded(SphereModel),-1);
		obj->SetPosition(beg);
		obj->SetScale(0.1);
		obj->SetConstantColor(PackedColor(Color(0,1,0)));
		GScene->ShowObject(obj);
	}

	// blue ball - ret
	{
		Ref<Object> obj=new ObjectColored(GScene->Preloaded(SphereModel),-1);
		obj->SetPosition(ret);
		obj->SetScale(0.1);
		obj->SetConstantColor(PackedColor(Color(0,0,1)));
		GScene->ShowObject(obj);
	}

	// yellow ball - ret+normal
	{
		Ref<Object> obj=new ObjectColored(GScene->Preloaded(SphereModel),-1);
		obj->SetPosition(ret+planeNormal);
		obj->SetScale(0.1);
		obj->SetConstantColor(PackedColor(Color(1,1,0)));
		GScene->ShowObject(obj);
	}

	// cyan ball - ret+perpFromEB
	{
		Ref<Object> obj=new ObjectColored(GScene->Preloaded(SphereModel),-1);
		obj->SetPosition(ret+perpFromEB);
		obj->SetScale(0.1);
		obj->SetConstantColor(PackedColor(Color(0,1,1)));
		GScene->ShowObject(obj);
	}

	// black ball - retPoint
	{
		Ref<Object> obj=new ObjectColored(GScene->Preloaded(SphereModel),-1);
		obj->SetPosition(retPoint);
		obj->SetScale(0.1);
		obj->SetConstantColor(PackedColor(Color(0,0,0)));
		GScene->ShowObject(obj);
	}

	}

	#endif

	return retPoint;
}

/**
@return 0 means the vehicle should stop. Never return 0 if stopping is not expected.
*/
float Path::SpeedAtCost( float cost, bool isSoldier ) const
{
	int next;
	int size=Size();
	for( next=1; next<size; next++ )
	{
		if( Get(next)._cost>=cost ) break;
	}
	if( next>=size )
	{
		// end of path - no speed
		next=size-1;
		if( next<=0 ) return 0; // no path - no speed
	}
	// interpolate
	const OperInfoResult &pInfo=Get(next-1);
	const OperInfoResult &nInfo=Get(next);
  if (/*pInfo._road && pInfo._road->IsLockedByWaiting(isSoldier) ||*/ nInfo._road && nInfo._road->IsLockedByWaiting(isSoldier))
  {
    // we have to stop - vehicle waiting
    return 0;
  }
	float nom=OperPos(nInfo).Distance(OperPos(pInfo));
	float denom=nInfo._cost-pInfo._cost;
	if( nInfo._cost>GET_UNACCESSIBLE )
	{
		// TODO: BUG repair
		LogF("GET_UNACCESSIBLE cost in path N");
		return 1;
	}
	if( pInfo._cost>GET_UNACCESSIBLE )
	{
		// TODO: BUG repair
		LogF("GET_UNACCESSIBLE cost in path P");
		return 1;
	}
	if( denom<=0 )
	{
		if( nom>1 )
		{
			// TODO: BUG repair
			LogF("Singular cost in path");
			LogF("Path size %d, %.1f/%.1f",size,nom,denom);
		}
		return nom*100;
	}
	return floatMax(0.001f,nom/denom);
}

Vector3 Path::Begin() const
{
	return OperPos(Get(0));
}

Vector3 Path::End() const
{
	return OperPos(Get(Size()-1));
}

float Path::EndCost() const
{
	return Get(Size()-1)._cost;
}


LSError OperInfoResult::Serialize(ParamArchive &ar)
{
	CHECK(::Serialize(ar, "pos", _pos, 1))
	CHECK(ar.Serialize("cost", _cost, 1))
  CHECK(ar.SerializeEnum("type", _type, 1, NTGrid))
  CHECK(ar.SerializeEnum("clearance", _clearance, 1, ClearanceNormal))
  // TODO: serialize patch action as well
  // how to serialize _road?
  if (ar.IsSaving())
  {
    OLinkLPtr(Object) objRoadL = _road ? _road->GetObject() : OLinkLPtr(Object)();
    OLink(Object) objRoad(objRoadL);
    CHECK(ar.SerializeRef("road", objRoad, 1))
  }
  else
  {
    OLink(Object) objRoad;
    CHECK(ar.SerializeRef("road", objRoad, 1))
    // now find the corresponding road object
    if (objRoad)
    {
      LandIndex x(toIntFloor(objRoad->FutureVisualState().Position().X() * InvLandGrid));
      LandIndex z(toIntFloor(objRoad->FutureVisualState().Position().Z() * InvLandGrid));
      for (LandIndex zz(z-1); zz<=z+1; zz++) for (LandIndex xx(x-1); xx<=x+1; xx++)
      {
        if (!InRange(xx, zz)) continue;
        const RoadList &roadList = GLandscape->GetRoadNet()->GetRoadList(xx, zz);
        for (int i=0; i<roadList.Size(); i++)
        {
          const RoadLink *item = roadList[i];
          if (item->GetObject()==objRoad)
          {
            _road = item;
            goto Break;
          }
        }
      }
      _road = NULL;
      Break:;
    }
    else
    {
      _road = NULL;
    }
  }
	return LSOK;
}

LSError Path::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("Path", *((base *)this), 1))
	if (ar.IsSaving() || ar.GetArVersion() >= 21)
  {
    CHECK(ar.Serialize("costReplan", _costReplan, 1, 0))
    CHECK(ar.Serialize("costForceReplan", _costForceReplan, 1, 0))
  }
  else
  {
    int maxIndex;
    CHECK(ar.Serialize("maxIndex", maxIndex, 1))
    if (maxIndex <= Size())
      _costForceReplan = _costReplan = Get(maxIndex - 1)._cost;
    else
      _costForceReplan = _costReplan = 0;
  }
	CHECK(::Serialize(ar, "searchTime", _searchTime, 1));
  CHECK(::Serialize(ar,"coverPos",_cover._oper._pos,1,VZero));
  CHECK(ar.Serialize("coverOperHeading",_cover._oper._heading,1,FLT_MAX));
  CHECK(ar.Serialize("coverOperX",_cover._oper._x,1,0));
  CHECK(ar.Serialize("coverOperZ",_cover._oper._z,1,0));
  CHECK(ar.Serialize("coverVars",_cover._payload,1));
  CHECK(ar.Serialize("openCover",_openCover,1,false));
  CHECK(ar.Serialize("completed",_completed,1,true));

	return LSOK;
}

void Path::Clear()
{
  base::Clear();
  _cover = CoverInfo();
  _openCover = false;
  _completed = true;
}

void Path::SetCover(const CoverInfo &cover, bool openCover)
{
  _cover=cover;
  _openCover=openCover;
  // this means the path is fresh - we mark is as unused
  _completed = false;
}

#define PATH_POINT_MSG(MessageName, XX) \
  XX(MessageName, float, pos, NDTVector, float, NCTNone, DEFVALUE(Vector3, VZero), DOC_MSG("Path node position"), TRANSF) \
  XX(MessageName, Vector3, cost, NDTFloat, Vector3, NCTNone, DEFVALUE(float, 0), DOC_MSG("Path node cost"), TRANSF)

DECLARE_NET_INDICES(PathPoint, PATH_POINT_MSG)
DEFINE_NET_INDICES(PathPoint, PATH_POINT_MSG)

NetworkMessageFormat &OperInfoResult::CreateFormat
(
	NetworkMessageClass cls,
	NetworkMessageFormat &format
)
{
  PATH_POINT_MSG(PathPoint, MSG_FORMAT)
	return format;
}

TMError OperInfoResult::TransferMsg(NetworkMessageContext &ctx)
{
	PREPARE_TRANSFER(PathPoint)

/* TODO: fix error
	TRANSF(pos)
	TRANSF(cost)
*/
	return TMOK;
}

/* Not used currently

#define PATH_MSG(MessageName, XX) \
  XX(MessageName, int, operIndex, NDTInteger, int, NCTSmallUnsigned, DEFVALUE(int, 1), DOC_MSG("Currently executed path node index"), TRANSF, ET_NONE, 0) \
  XX(MessageName, int, maxIndex, NDTInteger, int, NCTSmallUnsigned, DEFVALUE(int, 1), DOC_MSG("Last valid path node index"), TRANSF, ET_NONE, 0) \
  XX(MessageName, Time, searchTime, NDTTime, Time, NCTNone, DEFVALUE(Time, Time(0)), DOC_MSG("Time, when path was planned"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE) \
  XX(MessageName, bool, onRoad, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("Path on road"), TRANSF, ET_NONE, 0) \
  XX(MessageName, AutoArray<OperInfoResult>, path, NDTObjectArray, NCTNone, DEFVALUE_MSG(NMTPathPoint), DOC_MSG("List of nodes"), TRANSF_ARRAY, ET_NONE, 0)

class IndicesPath
{
public:
  PATH_MSG(MSG_IDEF_ERR)

	IndicesPath();
	void Scan(NetworkMessageFormatBase *format);
};

IndicesPath::IndicesPath()
{
  PATH_MSG(MSG_IINIT_ERR)
}

void IndicesPath::Scan(NetworkMessageFormatBase *format)
{
  PATH_MSG(MSG_ISCAN_ERR)
}

IndicesPath *GetIndicesPath() {return new IndicesPath();}
void DeleteIndicesPath(IndicesPath *path) {delete path;}

void ScanIndicesPath(IndicesPath *path, NetworkMessageFormatBase *format)
{
	path->Scan(format);
}

void Path::CreateFormat
(
	NetworkMessageFormat &format
)
{
  PATH_MSG(MSG_FORMAT_ERR)
}

TMError Path::TransferMsg(NetworkMessageContext &ctx, IndicesPath *indices)
{
	ITRANSF(operIndex)
	ITRANSF(maxIndex)
	ITRANSF(searchTime)
	ITRANSF(onRoad)
	TRANSF_ARRAY_EX(path, *((base *)this))
	return TMOK;
}

float Path::CalculateError(NetworkMessageContext &ctx, IndicesPath *indices)
{
	float error = 0;

	ICALCERR_NEQ(Time, searchTime, ERR_COEF_MODE)
	// ?? _operIndex

	return error;
}
*/

#define DIAG 0

void Path::Optimize( EntityAI *vehicle )
{
	Fail("Not used, optimization included in OperMap::ResultPath");
	return;
	
}

void Path::AvoidCollision( EntityAI *vehicle )
{
	// change path to avoid collisions
	// TODO: implement
}

