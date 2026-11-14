// Landscape - generalized landscape drawing
// (C) 1996, SUMA
#include "wpch.hpp"
#include "landscape.hpp"
#include "scene.hpp"
#include "world.hpp"
#include "vehicle.hpp"
#include "shots.hpp"
#include "camera.hpp"
#include "vehicleAI.hpp"
#include "AI/ai.hpp"
#include <El/Common/randomGen.hpp>
#include "global.hpp"
#include <El/Common/perfLog.hpp>
#include "Network/network.hpp"
#include "thing.hpp"
#include "diagModes.hpp"
#include "txtPreload.hpp"
#include "soldierOld.hpp"

#define PROFILE_COLLISIONS 1 // exact performance evaluation

#include <El/Common/perfProf.hpp>

#include "baseWorld/enf_shapes.h"

#if _PROFILE
# pragma optimize( "", off )
#endif

#if _ENABLE_CHEATS
#define STARS 1
#endif

void ObjRadiusRectangle(int &xMin, int &xMax, int &zMin, int &zMax, Vector3Par oPos, Vector3Par nPos, float radius)
{
  const float maxObjRadius=25;
  radius+=maxObjRadius;
  float xMinF=floatMin(nPos.X(),oPos.X())-radius;
  float xMaxF=floatMax(nPos.X(),oPos.X())+radius;
  float zMinF=floatMin(nPos.Z(),oPos.Z())-radius;
  float zMaxF=floatMax(nPos.Z(),oPos.Z())+radius;

#if _VBS3_LARGE_OBJECTS
  float min[2]; float max[2];
  float dMin[2]; float dMax[2];

  min[0] = xMinF; min[1] = zMinF;
  max[0] = xMaxF; max[1] = zMaxF;

  // find min, max grid in world coordinates for dynamic objects
  GLandscape->SearchDynamicRTree(min, max, dMin, dMax);

  xMinF = floatMin(xMinF, dMin[0]); zMinF = floatMin(zMinF, dMin[1]);
  xMaxF = floatMax(xMaxF, dMax[0]); zMaxF = floatMax(zMaxF, dMax[1]);
#endif

  xMin=toIntFloor(xMinF*InvObjGrid);
  xMax=toIntFloor(xMaxF*InvObjGrid);
  zMin=toIntFloor(zMinF*InvObjGrid);
  zMax=toIntFloor(zMaxF*InvObjGrid);

#if _VBS3_LARGE_OBJECTS
  int sMin[2]; int sMax[2];

  // find min, max landgrid coordinates for static objects
  GLandscape->SearchStaticRTree(min, max, sMin, sMax);

  xMin = intMin(xMin, sMin[0]); zMin = intMin(zMin, sMin[1]);
  xMax = intMax(xMax, sMax[0]); zMax = intMax(zMax, sMax[1]);
#endif

  saturate(xMin, 0, ObjRange - 1);
  saturate(xMax, 0, ObjRange - 1);
  saturate(zMin, 0, ObjRange - 1);
  saturate(zMax, 0, ObjRange - 1);
}

void ObjRadiusRectangle(int &xMin, int &xMax, int &zMin, int &zMax, Vector3Par oPos, float radius)
{
  float xMinF = oPos.X()-radius;
  float xMaxF = oPos.X()+radius;
  float zMinF = oPos.Z()-radius;
  float zMaxF = oPos.Z()+radius;

#if _VBS3_LARGE_OBJECTS
  float min[2]; float max[2];
  float dMin[2]; float dMax[2];

  min[0] = xMinF; min[1] = zMinF;
  max[0] = xMaxF; max[1] = zMaxF;

  GLandscape->SearchDynamicRTree(min, max, dMin, dMax);

  xMinF = floatMin(xMinF, dMin[0]); zMinF = floatMin(zMinF, dMin[1]);
  xMaxF = floatMax(xMaxF, dMax[0]); zMaxF = floatMax(zMaxF, dMax[1]);
#endif

  xMin=toIntFloor(xMinF*InvObjGrid);
  xMax=toIntFloor(xMaxF*InvObjGrid);
  zMin=toIntFloor(zMinF*InvObjGrid);
  zMax=toIntFloor(zMaxF*InvObjGrid);

#if _VBS3_LARGE_OBJECTS
  int sMin[2]; int sMax[2];

  GLandscape->SearchStaticRTree(min, max, sMin, sMax);

  xMin = intMin(xMin, sMin[0]); zMin = intMin(zMin, sMin[1]);
  xMax = intMax(xMax, sMax[0]); zMax = intMax(zMax, sMax[1]);
#endif

  saturate(xMin, 0, ObjRange - 1);
  saturate(xMax, 0, ObjRange - 1);
  saturate(zMin, 0, ObjRange - 1);
  saturate(zMax, 0, ObjRange - 1);
}

/**
@param radius radius of a largest supported object
*/
void ObjRectangle(int &xMin, int &xMax, int &zMin, int &zMax, Vector3Par minC, Vector3Par maxC, float radius)
{
  float xMinF = minC.X()-radius;
  float xMaxF = maxC.X()+radius;
  float zMinF = minC.Z()-radius;
  float zMaxF = maxC.Z()+radius;
  xMin=toIntFloor(xMinF*InvObjGrid);
  xMax=toIntFloor(xMaxF*InvObjGrid);
  zMin=toIntFloor(zMinF*InvObjGrid);
  zMax=toIntFloor(zMaxF*InvObjGrid);
  saturate(xMin, 0, ObjRange - 1);
  saturate(xMax, 0, ObjRange - 1);
  saturate(zMin, 0, ObjRange - 1);
  saturate(zMax, 0, ObjRange - 1);
}

void ObjMapRadiusRectangle(int &xMin, int &xMax, int &zMin, int &zMax)
{
#if _VBS3_LARGE_OBJECTS
  float min[2]; float max[2];

  min[0] = xMin*ObjGrid; min[1] = zMin*ObjGrid;
  max[0] = xMax*ObjGrid; max[1] = zMax*ObjGrid;

  int sMin[2]; int sMax[2];

  GLandscape->SearchStaticRTree(min, max, sMin, sMax);

  xMin = intMin(xMin, sMin[0]); zMin = intMin(zMin, sMin[1]);
  xMax = intMax(xMax, sMax[0]+1); zMax = intMax(zMax, sMax[1]+1);

  saturate(xMin, 0, ObjRange - 1);
  saturate(xMax, 0, ObjRange - 1);
  saturate(zMin, 0, ObjRange - 1);
  saturate(zMax, 0, ObjRange - 1);
#endif
}

#include <Es/Memory/normalNew.hpp>

/// object used for diagnostics displays
class TempDiag: public Entity
{
  typedef Entity base;

  float _ttl;
  Color _color;

public:
  TempDiag(LODShape *shape, const CreateObjectId &id, ColorVal color, float ttl)
  :base(shape,VehicleTypes.New("#tempdiag"),id),_ttl(ttl),_color(color)
  {
    SetType(TypeTempVehicle);
  }

  void Simulate(float deltaT, SimulationImportance prec);

  void SetConstantColor( ColorVal color ) {_color=color;}
  ColorVal GetConstantColor() const {return _color;}
  // no instancing possible - we need per-object color information
  virtual bool CanBeInstanced(int level, float distance2, const DrawParameters &dp) const{return false;}

  void GetMaterial(TLMaterial &mat) const {CreateMaterial(mat,GetConstantColor());}

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

DEFINE_FAST_ALLOCATOR(TempDiag)

void TempDiag::Simulate(float deltaT, SimulationImportance prec)
{
  _ttl -= deltaT;
  if (_ttl<0)
    // vanish once expired
    SetDelete();
}


/**
separating axis test - sphere against AABB
*/

bool IsIntersectionBoxWithSphere(Vector3Val boxMin, Vector3Val boxMax, Vector3Par center, float radius)
{
  Vector3Val boxCenter = (boxMin+boxMax)*0.5f;
  Vector3Val boxExtent = (boxMax-boxMin)*0.5f;
  
  // sphere center relative to the box center
  Vector3Val sphereCenter = center-boxCenter;
  
	float dist2 = 0;
	
	// Add distance squared from sphere centerpoint to box for each axis
	for ( int i = 0; i < 3; i++ )
	{
		float dist = fabs(sphereCenter[i]) - boxExtent[i];
		if (dist>0)
			dist2 += Square(dist);
	}
	return dist2 <= Square(radius);
}

#define DIAG 0
#define DIAG_LAND 0
#define DIAG_VISIBLE_THROUGH 0

void Landscape::ObjectCollision(CollisionBuffer &retVal, const Object *with, const ObjectVisualState &withPos, bool onlyVehicles, bool bPerson) const
{ 
  const Shape *geometry = with->IsWreck() ? with->GetShape()->WreckLevel() : with->GetShape()->GeometryLevel();
  if (!geometry) return; // no geometry - no test
#if _VBS3
  if(with->_inVisibilityFlags & VBS_IFGeo) return; //geo collision is disabled
#endif
  PROFILE_SCOPE_EX(cLOb1,coll);
  if (PROFILE_SCOPE_NAME(cLOb1).IsActive())
    PROFILE_SCOPE_NAME(cLOb1).AddMoreInfo(with->GetDebugName());
  Vector3Val nPos=withPos.Position();
  Vector3Val oPos=with->FutureVisualState().Position();
  float radius=with->GetRadius();
  int xMin,xMax,zMin,zMax;
  ObjRadiusRectangle(xMin,xMax,zMin,zMax,oPos,nPos,radius);

  ObjectCollisionContext context(withPos);
  context.with = with;
  context.person = bPerson;
  context.onlyVehicles = onlyVehicles;
 
  // when we know which list are we contained in, we can perform some special case optimizations
  int xl,zl;
  _terrain->SelectObjectList(xl,zl,with->FutureVisualState().Position().X(),with->FutureVisualState().Position().Z());
  ObjectListFull *withList = _objects(xl,zl).GetList();
  #if _ENABLE_CHEATS // _DEBUG
    if (withList)
    {
      // assert the object is really present in the list
      bool found = false;
      for (int i=0; i<withList->Size(); i++)
      {
        if (withList->Get(i)==with)
        {
          found = true;
          break;
        }
      }
      DoAssert(found);
    }
  #endif
  
  VisualStateAge withAge = with->GetVisualStateAge(withPos);
  FrameBase withPos2;            
  withPos2.SetTransform(withPos);            
  for (int  x = xMin; x <= xMax; x++) for (int z = zMin; z <= zMax; z++)
  {
    const ObjectListFull *listFull = _objects(x, z).GetList();
    if (!listFull)
      continue;
    int n = listFull->Size();
    int nNonStatic = listFull->GetNonStaticCount();
    // a common special case: the vehicle tested is the only one in the list
    bool allStatic = nNonStatic==0 || listFull==withList && nNonStatic==1;
    // for static objects we may check bounding box/sphere
    Vector3Val withSphere = withPos.Position();
    float withRadius = with->GetCollisionRadius(withPos);
    // create a local copy so that we can modify it
    bool onlyVehiclesInThisRect = onlyVehicles;
    const Vector3 *minMax = listFull->GetMinMax();
    if (!IsIntersectionBoxWithSphere(minMax[0], minMax[1], withSphere, withRadius))
      // when we know we are out of reach for all static objects, we may safely test against vehicles only
      onlyVehiclesInThisRect = true;
    
    // when we are testing against vehicles only and there are no vehicles to test, we are done
    // this also covers the situation when static objects were rejected by the bounding test
    bool done = onlyVehiclesInThisRect && allStatic;
    for( int i=n; --i>=0 && !done; )
    {
      const Object *obj = listFull->Get(i);
      if( onlyVehiclesInThisRect )
      {
        if (obj->Static())
          continue;
        if (nNonStatic--<=0)
          done = true;      
      }
#if _ENABLE_ATTACHED_OBJECTS
      //don't collide with an attached entity!
      //if (obj->GetAttachedTo() == with || with->GetAttachedTo() == obj)
      if (obj->AreAttached(with))
        continue;
#endif
#if _VBS2
      //geo collision is disabled
      if(obj->_inVisibilityFlags & VBS_IFGeo) continue; 
#endif
      PROFILE_SCOPE_EX(cLO12,coll);
      if (PROFILE_SCOPE_NAME(cLO12).IsActive())
        PROFILE_SCOPE_NAME(cLO12).AddMoreInfo(obj->GetDebugName());
      Landscape::ObjectCollisionHelper(retVal, obj, obj->GetVisualStateByAge(withAge), context); 
    }
  }
}

void Landscape::ObjectCollisionHelper(CollisionBuffer &retVal, const Object *obj, const ObjectVisualState &objPos, ObjectCollisionContext& context)
{ 
  if( context.onlyVehicles )
  {
    if (obj->Static())
      return;
    // suspended items are also static
    const EntityAI* entity = dyn_cast<const EntityAI, const Object>(obj);
    if (entity && entity->GetStopped())
      return;  
    // do not collide with AIs only with players
    const Person *man = dyn_cast<const Person, const Object>(obj);
    if (man && !man->IsNetworkPlayer())
      return;    
  }
  ObjectType objType = obj->GetType();
  if ((objType&(Network|Temporary|TypeTempVehicle))!=0) // (see label CRATERCOLISION)
  {
    if (objType != Network)
      return;
    const LODShape *objShape = obj->GetShape();
    if (!objShape)
      return; // no collisions with roads
    const Shape *geometry = obj->IsWreck() ? objShape->WreckLevel() : objShape->GeometryLevel();
    if (!geometry)
      return;
  }

#if _ARMA3_TERRAIN_DECALS
  if (objType==LandDecal)
    return;
#endif

  // do not collide with "soft" objects (dead man bodies, trees, insects, etc.)
  if (!obj->HasGeometry())
    return;
  const Object *with = context.with;
  if (obj == with)
    return;
#if _ENABLE_ATTACHED_OBJECTS
  // don't collide with an attached entity!
  if (with && obj->AreAttached(with)/*(obj->GetAttachedTo() == with || with->GetAttachedTo() == obj)*/)
    return;
#endif
  obj->Intersect(retVal, with, objPos, context.withPos, 0, context.person); 
}

void Landscape::ObjectCollisionFirstTouch(float &doneFrac, Vector3& normal, ObjectCollisionFirstTouchContext& context) const
{ // check with all object
  // AI soldier test collisions only with non-static objects

  const Object *with = context.with;
  const Shape *geometry = with->IsWreck() ? with->GetShape()->WreckLevel() : with->GetShape()->GeometryLevel();
  if (!geometry) return; // no geometry - no test
#if _VBS3
  if(with->_inVisibilityFlags & VBS_IFGeo) return; //geo collision is disabled
#endif
  PROFILE_SCOPE_EX(cLObF,coll);
  if (PROFILE_SCOPE_NAME(cLObF).IsActive())
    PROFILE_SCOPE_NAME(cLObF).AddMoreInfo(with->GetDebugName());
  Vector3Val nPos=context.withPos.Position();
  Vector3Val oPos= nPos + context.deltaPos;
  float radius=with->GetRadius();
  int xMin,xMax,zMin,zMax;
  ObjRadiusRectangle(xMin,xMax,zMin,zMax,oPos,nPos,radius);
  doneFrac = 1;
  int ret = 0;  

  VisualStateAge age = with->GetFutureVisualStateAge();
  for (int x = xMin; x <= xMax; x++) for (int z = zMin; z <= zMax; z++)
  {
    const ObjectList &list=_objects(x,z);
    int n=list.Size();
    for( int i=0; i<n; i++ )
    {
      const Object *obj=list[i];
      if(obj == context.except)
        continue;
#if _VBS3
      if(obj->_inVisibilityFlags & VBS_IFGeo) return; //geo collision is disabled
#endif
    
      float doneFracNew = 1.0f;
      Vector3 normalNew; 
      PROFILE_SCOPE_EX(cLOF2,coll);
      if (PROFILE_SCOPE_NAME(cLOF2).IsActive())
        PROFILE_SCOPE_NAME(cLOF2).AddMoreInfo(obj->GetDebugName());
      int returnTemp = ObjectCollisionFirstTouchHelper(doneFracNew, normalNew, obj, obj->GetVisualStateByAge(age), context); 

      if (returnTemp > 0 && doneFrac > doneFracNew)
      {
        doneFrac = doneFracNew;
        normal = normalNew;
        ret = returnTemp;
      }
    }
  }
  /* switch (ret)
  {
  case 1:
  GlobalShowMessage(10, "This plane.");
  break;
  case 2:
  GlobalShowMessage(10, "With plane.");
  break;
  case 3:
  GlobalShowMessage(10, "Average plane.");
  break;
  default:;
  }*/
}

int Landscape::ObjectCollisionFirstTouchHelper(float &doneFrac, Vector3& normal, const Object *obj, const ObjectVisualState& objPos, ObjectCollisionFirstTouchContext& context) 
{ // check with all object
  // AI soldier test collisions only with non-static objects  
  if( obj==context.with ) return 0;
  if( obj->GetShape() == NULL) return 0;
  ObjectType objType = obj->GetType();
  if( objType==Network)
  {
    const Shape *geometry = obj->IsWreck() ? obj->GetShape()->WreckLevel() : obj->GetShape()->GeometryLevel();
    if (!geometry) return 0; // no collisions with roads
  }
  if( objType==Temporary ) return 0; // no collisions with temporary
  if( objType==TypeTempVehicle ) return 0;
#if _ARMA3_TERRAIN_DECALS
  if (objType==LandDecal) return 0;
#endif 

  if( context.onlyVehicles )
  {
    if (obj->Static())
      return 0;
  }
  // do not collide with "soft" dead objects (man bodies, trees)
  if( !obj->HasGeometry() ) 
     return 0;
 
  if (!obj->IsPassable())
  {
    return obj->FirstTouch(doneFrac, normal, objPos, context.with, context.withPos, context.deltaPos);
  }
  return 0;
}

/***
line against min-max box implemented as line against AABB - adapted from http://www.3dkingdoms.com/weekly/bbox.cpp
for explanation see http://www.harveycartel.org/metanet/tutorials/tutorialA.html#section2
*/
bool IsIntersectionBoxWithLine(Vector3Val boxMin, Vector3Val boxMax, Vector3Val lineBeg, Vector3Val lineEnd)
{
  Vector3Val boxCenter = (boxMin+boxMax)*0.5f;
  Vector3Val boxExtent = (boxMax-boxMin)*0.5f;
  // Get line midpoint and extent
  
  // midpoint is relative to the box, so that we get AABB extending around 0,0,0
  Vector3Val mid = (lineBeg + lineEnd) * 0.5f - boxCenter;
  Vector3Val lineHalf = (lineEnd - boxCenter) - mid;
  
  Vector3Val lineExtent = Vector3( fabs(lineHalf.X()), fabs(lineHalf.Y()), fabs(lineHalf.Z()) );

  bool checkStatic = true;
  // Use Separating Axis Test
  // Separation vector from box center to line center is mid, since the line is in box space
  if ( fabs( mid.X() ) > boxExtent.X() + lineExtent.X() ) checkStatic = false;
  else if ( fabs( mid.Y() ) > boxExtent.Y() + lineExtent.Y() ) checkStatic = false;
  else if ( fabs( mid.Z() ) > boxExtent.Z() + lineExtent.Z() ) checkStatic = false;
  // Crossproducts of line and each axis
  else if ( fabs( mid.Y() * lineHalf.Z() - mid.Z() * lineHalf.Y())  >  (boxExtent.Y() * lineExtent.Z() + boxExtent.Z() * lineExtent.Y()) ) checkStatic = false;
  else if ( fabs( mid.X() * lineHalf.Z() - mid.Z() * lineHalf.X())  >  (boxExtent.X() * lineExtent.Z() + boxExtent.Z() * lineExtent.X()) ) checkStatic = false;
  else if ( fabs( mid.X() * lineHalf.Y() - mid.Y() * lineHalf.X())  >  (boxExtent.X() * lineExtent.Y() + boxExtent.Y() * lineExtent.X()) ) checkStatic = false;
  return checkStatic;
}

DEFINE_ENUM(ObjIntersect, ObjIntersect, OBJ_INTERSECT_ENUM)

struct CheckObjectCollisionContext
{
  CheckObjectCollisionContext() 
    : with(NULL), ignore(NULL),
    flags(0),
    filter(NULL)
     {};

  const Object *with;
  const Object *ignore;
  Vector3 beg, end;
  float radius;
  ObjIntersect typePri;
  /** secondary type is used for objects which cannot be tested using primary.
  This was introduced because OcclusionView is false for soldiers, and as a result
  view line intersection always failed for them.
  */
  ObjIntersect typeSec;
  Vector3 boundingCenter;
  float boundingSphere;
  VisualStateAge age;
  unsigned int flags;
  const IntersectionFilter *filter;
};


void Landscape::CheckObjectCollision(int x, int z, CollisionBuffer &retVal, CheckObjectCollisionContext &context) const
{
  if (!land_InRange(x,z))
    return;
  const ObjectList &list=_objects(x,z);

  int n = list.Size();
	if (n == 0)
		return;
		
	// quick reject based on list bounding box / sphere

  bool checkStatic = true;
  if(context.flags & CF_ONLY_DYNAMIC)
    checkStatic = false;
  else
  {
  #if 0 // as landscape grids tend to be flat, AABB test is much more likely to be efficient
    // check point on the line nearest to the bSphere center
    Vector3Val nearest = NearestPointInfinite(context.beg,context.end,list->GetBSphereCenter());
    float dist2FromSphere = nearest.Distance2Inline(list->GetBSphereCenter());
    if (dist2FromSphere>Square(list->GetBSphereRadius()))
      checkStatic = false;
    else
  #endif
    {
      // when bSphere test failed, try AABB
      const Vector3 *minMax = list->GetMinMax();
      if(context.radius > 0)
      {
        Vector3 newMins = minMax[0] - Vector3(context.radius);
        Vector3 newMaxs = minMax[1] + Vector3(context.radius);
        checkStatic = IsIntersectionBoxWithLine(newMins, newMaxs, context.beg, context.end);
      }
      else
        checkStatic = IsIntersectionBoxWithLine(minMax[0], minMax[1], context.beg, context.end);
      // No separating axis, the line intersects
    }
  }

  // note: when checkStatic is true, most likely even vehicle intersection is not possible, but we cannot be sure
  // if there are no moving (non-static) objects, we know no intersection is possible
  int from = checkStatic ? 0 : (list->Size() - list->GetNonStaticCount());
  int to = (context.flags & CF_ONLY_STATIC) ? (list->Size() - list->GetNonStaticCount()) : list->Size();

  for(int i = from; i < to; i++)
  {
    const Object *obj = list[i];
    if (i < (n - 1))
		  PrefetchT0Off(list[i + 1], 0);
    CheckObjectCollisionHelper(retVal, obj, obj->GetVisualStateByAge(context.age), context);
    
    if(retVal.Size() && (context.flags & (CF_FIRST_CONTACT|CF_ALL_OBJECTS)) == CF_FIRST_CONTACT)
      break;
  }
}

static inline bool TestAvailable(ObjIntersect type, const Object *obj)
{
  switch (type)
  {
  case ObjIntersectGeom:
    return obj->HasGeometry();
  case ObjIntersectView:
    return obj->OcclusionView();
    //case ObjIntersectIFire: case ObjIntersectFire:
  case ObjIntersectNone:
    return false;
  default:
    return obj->OcclusionFire();
  }
}


void Landscape::CheckObjectCollisionHelper(CollisionBuffer &retVal, const Object *__restrict obj, const ObjectVisualState &objPos, CheckObjectCollisionContext &context)
{ 
  if (obj==context.with) return;
  if (obj==context.ignore) return;
  if (context.filter && !(*context.filter)(obj)) return;

  ObjectType objType = obj->GetType();
  if (objType==Network)
  {
    const Shape *geometry = obj->IsWreck() ? obj->GetShape()->WreckLevel() : obj->GetShape()->GeometryLevel();
    if (!geometry) return; // no collisions with roads
  }
  if (objType==Temporary ) return; // no collisions with temporary
  if (objType==TypeTempVehicle ) return;
#if _ARMA3_TERRAIN_DECALS
  if (objType==LandDecal) return;
#endif 

#if _ENABLE_ATTACHED_OBJECTS
  //don't collide with an attached entity!
  if (context.with && obj->AreAttached(context.with)/*obj->GetAttachedTo() == context.with || context.with->GetAttachedTo() == obj)*/) 
    return;
#endif

  // if primary is not possible, we want to use secondary
  ObjIntersect type = context.typePri;
  
  if (!TestAvailable(type,obj))
  {
    type = context.typeSec;
    if (!TestAvailable(type, obj))
      return;
  }
#if _VBS3 //check if that object is invisible for the collision type
  if(obj->_inVisibilityFlags > 0)
  {
    bool invisible = false;
    switch(type)
    {
    case ObjIntersectFire:
    case ObjIntersectIFire:
      invisible = (obj->_inVisibilityFlags & VBS_IFFire) > 0;
      break;
    case ObjIntersectView:
      invisible = (obj->_inVisibilityFlags & VBS_IFView) > 0;
      break;
    case ObjIntersectGeom:
      invisible = (obj->_inVisibilityFlags & VBS_IFGeo) > 0;
      break;
    }
    if(invisible) return;
  }
#endif

  // check if obj is in min-max of tested trajectory
  float dist2=objPos.Position().Distance2Inline(context.boundingCenter);
  float collRadius = obj->GetCollisionRadius(objPos);
  float maxDist2=Square(context.boundingSphere+collRadius);
  if( dist2>maxDist2 ) return;

  int n = retVal.Size();
  obj->IntersectLine(objPos,retVal,context.beg,context.end,context.radius,type, 0, context.flags);

  if (type!=context.typePri)
  {
    for(; n < retVal.Size(); n++)
    {
      // part of the answer has no meaning if different collision type query is used
      CollisionInfo& info = retVal[n];
      info.surface = NULL;
      info.texture = NULL;
      info.component = -1;
      info.geomLevel = -1;
    }
  }
}

/// debugging helper for DDA Landscape::ObjectCollision
struct GridVisited
{
  int _x,_z;
  
  GridVisited(){}
  GridVisited(int x, int z) :_x(x),_z(z) {}
  bool operator==(const GridVisited &w) const {return _x==w._x && _z==w._z;}
};

TypeIsSimple(GridVisited);

void Landscape::ObjectCollisionLine(VisualStateAge age, CollisionBuffer &retVal, const IntersectionFilter &f, Vector3Par beg, Vector3Par end, float radius, ObjIntersect typePri, ObjIntersect typeSec, unsigned int collisionFlags) const
{ // check with all object
  PROFILE_SCOPE_EX(ObjCollLine,coll);
  if (PROFILE_SCOPE_NAME(ObjCollLine).IsActive())
  {
    PROFILE_SCOPE_NAME(ObjCollLine).AddMoreInfo(Format("%.1f",beg.Distance(end)));
  }

  int xMin,xMax,zMin,zMax;
  ObjRadiusRectangle(xMin,xMax,zMin,zMax,beg,end,radius);

  //Log("boundingSphere %f",boundingSphere);
  CheckObjectCollisionContext context;
  
  context.beg = beg;
  context.end = end;
  context.ignore = NULL; // TODO: replace all occurences of ignore/with with filter
  context.with = NULL;
  context.radius = radius;
  context.typePri = typePri;
  context.typeSec = typeSec;
  // create beg-end bounding box
  // this may help with very short queries - many objects can be quickly eliminated
  context.boundingCenter = (end+beg)*0.5f;
  context.boundingSphere = end.Distance(beg)*0.5f + radius;
  context.age = age;
  context.flags = collisionFlags;
  context.filter = &f;

  #define DDA_OPTIMIZED 1
  #define DEBUG_DDA 0
#if _ENABLE_PERFLOG
    int nTestsNew = 0;
#endif
#if DEBUG_DDA
    // build list by complete walk-through
    FindArrayKey<GridVisited> visitedOld;
    FindArrayKey<GridVisited> visitedNew;
    int nTestsOld = 0;
#endif
#if DDA_OPTIMIZED && !DEBUG_DDA
  if (context.boundingSphere<50)
#endif
  {
    // line is short - perform direct sphere test 
    for (int x = xMin; x <= xMax; x++) for (int z = zMin; z <= zMax; z++)
    {
#if DEBUG_DDA
        int oldRet = retVal.Size();
        nTestsOld++;
#endif
      CheckObjectCollision(x,z,retVal,context);
      if(retVal.Size() && (context.flags & (CF_FIRST_CONTACT|CF_ALL_OBJECTS)) == CF_FIRST_CONTACT)
      {
        x = xMax;
        break;
      }
#if DEBUG_DDA
        if (oldRet!=retVal.Size())
          visitedOld.AddUnique(GridVisited(x,z));
#endif
    }
  }
#if DDA_OPTIMIZED
#if !DEBUG_DDA
  else
#endif
  {

    // if line is long, it is better to consider only some squares on the line
    // DDA-like test (see Landscape::Visible)
    // assume max. 50 m objects present
    // TODO: make border more accurate, accordingly to ObjRadiusRectangle properties
    float borderSize = 1.0f+50.0f*GetInvLandGrid();
    int border=toIntFloor(borderSize);

    int ox=toIntFloor(end.X()*GetInvLandGrid());
    int oz=toIntFloor(end.Z()*GetInvLandGrid());
    int sx=toIntFloor(beg.X()*GetInvLandGrid());
    int sz=toIntFloor(beg.Z()*GetInvLandGrid());
    int aox=abs(ox-sx),aoz=abs(oz-sz);

    if( aox>aoz )
    {
      // mainly horizontal - primary coordinate is x, derived is z
      float dd=float(oz-sz)/aox;
      int pd=ox>sx ? +1 : -1;
      int p=sx-pd*border;
      float d=sz-dd*border;
      int plen=aox+2*border+1;
      for( int i=plen; --i>=0; )
      {
        int dis=toIntFloor(d-borderSize);
        int die=toIntFloor(d+borderSize);
        for( int di=dis; di<=die; di++ )
        {
#if _ENABLE_PERFLOG
          nTestsNew++;
#endif
#if DEBUG_DDA
          int oldRet = retVal.Size();
#endif
          CheckObjectCollision(p,di,retVal,context);
          if(retVal.Size() && (context.flags & (CF_FIRST_CONTACT|CF_ALL_OBJECTS)) == CF_FIRST_CONTACT)
          {
            i = 0;
            break;
          }

#if DEBUG_DDA
            if (oldRet!=retVal.Size())
              visitedNew.AddUnique(GridVisited(p,di));
#endif
        }
        d+=dd;
        p+=pd;
      }
    }
    else if( aoz!=0 )
    {
      // mainly vertical - primary coordinate is z, derived is x
      float dd=float(ox-sx)/aoz;
      int pd=oz>sz ? +1 : -1;
      int p=sz-pd*border;
      Assert( abs(p)<2000 );
      float d=sx-dd*border;
      int plen=aoz+2*border+1;
      for( int i=plen; --i>=0; )
      {
        int dis=toIntFloor(d-borderSize);
        int die=toIntFloor(d+borderSize);
        for( int di=dis; di<=die; di++ )
        {
#if _ENABLE_PERFLOG
          nTestsNew++;
#endif
#if DEBUG_DDA
          int oldRet = retVal.Size();
#endif
          CheckObjectCollision(di,p,retVal,context);        
          if(retVal.Size() && (context.flags & (CF_FIRST_CONTACT|CF_ALL_OBJECTS)) == CF_FIRST_CONTACT)
          {
            i = 0;
            break;
          }
#if DEBUG_DDA
          if (oldRet!=retVal.Size())
            visitedNew.AddUnique(GridVisited(di,p));
#endif
        }
        d+=dd;
        p+=pd;
      }
    }
    else
    {
      for (int ssx = -border; ssx <= border; ssx++) for (int ssz = -border; ssz <= border; ssz++)
      {
#if _ENABLE_PERFLOG
        nTestsNew++;
#endif
#if DEBUG_DDA
        int oldRet = retVal.Size();
#endif
        int xx = ox + ssx;
        int zz = oz + ssz;
        CheckObjectCollision(xx, zz, retVal, context);
        if(retVal.Size() && (context.flags & (CF_FIRST_CONTACT|CF_ALL_OBJECTS)) == CF_FIRST_CONTACT)
        {
          ssx = border;
          break;
        }
#if DEBUG_DDA
          if (oldRet!=retVal.Size())
            visitedNew.AddUnique(GridVisited(xx,zz));
#endif
      }
    }
  }
#if DEBUG_DDA
    // compare visitedOld / visitedNew
    for (int i=0; i<visitedOld.Size(); i++)
    {
      if (visitedNew.FindKey(visitedOld[i])<0)
        LogF("%d,%d not visited by DDA",visitedOld[i]._x,visitedOld[i]._z);
    }
    for (int i=0; i<visitedNew.Size(); i++)
    {
      if (visitedOld.FindKey(visitedNew[i])<0)
        LogF("%d,%d not visited by BBox",visitedNew[i]._x,visitedNew[i]._z);
    }
    ADD_COUNTER(clObO,nTestsOld);
    ADD_COUNTER(clObN,nTestsNew);
#elif _ENABLE_PERFLOG
    ADD_COUNTER(clObO,(xMax+1-xMin)*(zMax+1-zMin));
    ADD_COUNTER(clObN,nTestsNew);
#endif
#endif
}

/// compute a differential from normal

void ComputeDDFromNormal(float &maxDX, float &maxDZ, Vector3Par normal)
{
  float nY = normal.Y();
  if (fabs(nY) > 1e-2f)
  {
    maxDX = -normal.X() / nY;
    maxDZ = -normal.Z() / nY;
  }
  else
  {
    maxDX = 0.0f;
    maxDZ = 0.0f;
  }
}

/*!
\param dX [out] world space ground plane differential
\param dZ [out] world space ground plane differential
\param center [in] obj model space center of the tested object
*/
float Landscape::UnderRoadSurface(const Object *obj, AnimationContext &animContext, Vector3Par modelPos, Vector3Par center,
  float bumpy, const ObjDebuggingContext &ctx, float *dX, float *dZ, Texture **texture) const
{
  ObjDebuggingContext thisCtx = ctx.Transform(obj);
  
  // if we are on road, return road surface parameters
  LODShape *lShape = obj->GetShape();
  const Shape *shape = lShape->RoadwayLevel();
  Assert(shape);
#if _VBS3 //DirectionUp is not correct for new roads
  const Vector3& VUpModel = obj->FutureVisualState().GetInvTransform().Direction().CrossProduct(obj->FutureVisualState().GetInvTransform().DirectionAside());
#else
  const Vector3& VUpModel = obj->FutureVisualState().GetInvTransform().DirectionUp();
#endif  
  const float noCollisionY = -100.0f;

  struct UnderRoadSurfaceCheckOneFace
    {
    static float MaxSteep() {return -0.25f;} // cos 75 deg
    static float MinUnder() {return -0.5f;}
    static float MaxUnderWater() {return 3.0f;}
    static float MaxUnderSolid() {return 1.0f;}
    static float MaxUnder(const Shape *shape)
      {
      float maxUnder = 0;
      for (int s=0; s<shape->NSections(); s++)
      {
        const ShapeSection &ss = shape->GetSection(s);
        saturateMax(maxUnder,MaxUnder(ss));
      }
      return maxUnder;
    }
    static float MaxUnder(const PolyProperties &pp)
    {
      return pp.GetTexture() && pp.GetTexture()->IsWater() ? MaxUnderWater() : MaxUnderSolid();
    }
    
    void operator () (
      const Vector3 &VUpModel, const Plane &plane, Vector3Par modelPos,
      Matrix4Par toWorld, const PolyProperties &pp, Vector3Par center, const Poly &face, AnimationContext &animContext, const Shape * shape,
      float &maxY, float &maxDX, float &maxDZ, Texture * &maxTexture
    ) const
    {
        // check if pos can be under face
        // check using face plane equation
#if _VBS3
        if (VUpModel * plane.Normal() > 0.0f)
#else
      if (VUpModel * plane.Normal() > MaxSteep())
#endif
          // ignore roadways that are too steep or lying 
        return;

      float under = MinUnder(); //should be initialized (face.Inside do not set it in some cases)
      #if _VBS3
        // TODO: VBS: is normal reversal needed for new roads? - if it is, y needs to be replaced by under - caution: different meaning
        float dX,dZ;
        bool isInside = (plane.Normal().Y() > 0) ? 
          face.InsideFromBottom(animContext.GetPosArray(shape), plane, modelPos, &y)
          : face.Inside(VUpModel,animContext.GetPosArray(shape), plane, modelPos, &under, &dX, &dZ);
      #else
        bool isInside = face.Inside(VUpModel,animContext.GetPosArray(shape), plane, modelPos, &under);
      #endif
      #if _DEBUG
        // check the face planarity
        float maxD = 0;
        for (int i=0; i<face.N(); i++)
        {
          Vector3Val vPos = animContext.GetPosArray(shape)[face.GetVertex(i)];
          float d = plane.Distance(vPos);
          maxD = floatMax(maxD,d);
        }
        if (maxD>0.02f)
        {
          __asm nop;
        }
      #endif


        // max 1 m step allowed
        // we test only some area above
      if (under<=MinUnder()) return;
        // we test only some area below
        // we assume max. 3m for water and 1m for solid surfaces
      float maxUnder = MaxUnder(pp);
      if (under>maxUnder) return;
        // if the model center is way below, we need to ignore this roadway
        float comUnder = plane.Distance(center);
      if (comUnder>=maxUnder) return;
        // use max test for this
        if (isInside)
        {
        float worldY = (toWorld*modelPos).Y()+under;
        if (maxY<worldY)
          {
          maxY = worldY;
          Vector3 normal = toWorld.Rotate(plane.Normal());
          // deduce dX,dZ from the world space normal
          ComputeDDFromNormal(maxDX,maxDZ,normal);
            maxTexture = pp.GetTexture();
          }
        }
      }
  };


  if (!(shape->GetOrHints() & ClipLandOn))
  {
    float maxY=-1e10f;
    float maxDX=0, maxDZ=0;
    Texture *maxTexture=NULL;


    bool animated = obj->IsAnimated(lShape->FindRoadwayLevel())>=AnimatedGeometry;
    if (!animated)
    {
      const RoadwayFaces &rFaces = lShape->BuildRoadwayFaces();
      OffTree<RoadwayFace>::QueryResult res;
      // if we are not vertical, we have to query a larger area
      float minUnder = UnderRoadSurfaceCheckOneFace().MinUnder();
      float maxUnder = UnderRoadSurfaceCheckOneFace().MaxUnder(shape);
      // check extents in the XZ range
      float xMin = floatMin(minUnder*VUpModel.X(),maxUnder*VUpModel.X());
      float xMax = floatMax(minUnder*VUpModel.X(),maxUnder*VUpModel.X());
      float zMin = floatMin(minUnder*VUpModel.Z(),maxUnder*VUpModel.Z());
      float zMax = floatMax(minUnder*VUpModel.Z(),maxUnder*VUpModel.Z());
      
      rFaces.Query(res,modelPos.X()+xMin,modelPos.Z()+zMin,modelPos.X()+xMax+0.0001f,modelPos.Z()+zMax+0.0001f);
      
      #if _DEBUG
        float dMaxY = maxY;
        float dMaxDX = maxDX;
        float dMaxDZ = maxDZ;
      #endif
      for (int r=0; r<res.regions.Size(); r++)
      {
        const RoadwayFace &rf = res.regions[r];

        const Poly &face=shape->Face(rf.o);
        const Plane &plane = animContext.GetPlane(shape, rf.i);
        const ShapeSection &ss = shape->GetSection(rf.section);
        const PolyProperties &pp = ss;
        
        UnderRoadSurfaceCheckOneFace()(VUpModel, plane, modelPos, obj->GetFrameBase(), pp, center, face, animContext, shape, maxY, maxDX, maxDZ, maxTexture);
    }
      #if _DEBUG
        int i=0;
        Assert(dMaxY<=maxY+0.001f);
        for (int s=0; s<shape->NSections(); s++)
        {
          const ShapeSection &ss = shape->GetSection(s);
          const PolyProperties &pp = animContext.GetSection(shape, s);
          for( Offset f=ss.beg; f<ss.end; shape->NextFace(f),i++ )
          {
            const Poly &face = shape->Face(f);
            const Plane &plane = animContext.GetPlane(shape, i);

            UnderRoadSurfaceCheckOneFace()(VUpModel, plane, modelPos, obj->GetFrameBase(), pp, center, face, animContext, shape, dMaxY, dMaxDX, dMaxDZ, maxTexture);
            Assert(dMaxY<=maxY+0.001f);
            __asm nop;
    }
        }
        Assert(fabs(maxY-dMaxY)<1e-2f);
      #endif
    }
    else
    {
      int i=0;
      for (int s=0; s<shape->NSections(); s++)
      {
      const ShapeSection &ss = shape->GetSection(s);
      const PolyProperties &pp = animContext.GetSection(shape, s);
      for( Offset f=ss.beg; f<ss.end; shape->NextFace(f),i++ )
      {
        const Poly &face = shape->Face(f);
        const Plane &plane = animContext.GetPlane(shape, i);

          UnderRoadSurfaceCheckOneFace()(VUpModel, plane, modelPos, obj->GetFrameBase(), pp, center, face, animContext, shape, maxY, maxDX, maxDZ, maxTexture);
      }
    }
    }
#if 1
    // scan proxies as well
    const EntityType *type = obj->GetEntityType();
    if (type)
    {
      int roadLevel = lShape->FindRoadwayLevel();
      int nProxies = type->GetProxyCountForLevel(roadLevel);
      for (int i=0; i<nProxies; i++)
      {
        // try a "roadway collision" with a proxy
        const ProxyObjectTyped &proxy = type->GetProxyForLevel(roadLevel,i);
        if (proxy.GetShape()->FindRoadwayLevel()>=0)
        {
          // convert modelPos into the proxy coordinate system
          Vector3 proxyPos = proxy.invTransform.FastTransform(modelPos);
          Vector3 proxyCenter = proxy.invTransform.FastTransform(center);
          // check if we have some chance of hit
          Vector3Val proxyMin = proxy.GetShape()->RoadwayLevel()->Min();
          Vector3Val proxyMax = proxy.GetShape()->RoadwayLevel()->Max();
          if (
            proxyPos.X()<proxyMin.X() || proxyPos.X()>proxyMax.X()
            || proxyPos.Z()<proxyMin.Z() || proxyPos.Z()>proxyMax.Z()
          )
          {
            // out of min-max box - we can skip the proxy
            continue;
          }

          // we may need to get an animated position
          Matrix4 trans = obj->GetProxyTransform(roadLevel,i,proxy.GetTransform(),obj->FutureVisualState().Transform());
          // if the object is collapsed, no need to test it
          if (trans.Orientation().Scale2()<1e-3f) continue;
          // TODO: currently we handle only ignoring, but we could handle other animations as well
          // this would however require animating before min-max test above
          
          // check if the corresponding proxy face is hidden
          int section = type->GetProxySectionForLevel(roadLevel,i);
          if (section>=0)
          {
            ShapeUsed lock = lShape->Level(roadLevel);
            int spec = animContext.GetSection(lock, section).Special();
            if (spec & IsHidden) continue;
          }

          // currently we assume top direction is roughly identical
          Assert(proxy.GetTransform().DirectionUp().Y()>=0.9f);
          // perform a test
          // assume proxy is not animated - if it should be, it would be a lot more complicated
          AnimationContext noAnimation(proxy.obj->FutureVisualState());
          float proxyDX,proxyDZ;
          Texture *proxyTex = NULL;
          Assert(VUpModel.Y()>0.95f); // roadway proxies supported only for vertical models
          float underProxy = UnderRoadSurface(proxy.obj,noAnimation,proxyPos,proxyCenter,bumpy,thisCtx,&proxyDX,&proxyDZ,&proxyTex);
          
          // we are searching for maximum point above
          float worldY = underProxy+modelPos.Y()+obj->GetFrameBase().Position().Y();
          if (underProxy>noCollisionY && worldY>maxY)
          {
            // we have assumed the vertical is almost the same in both systems
            // therefore we do not have to recompute the depth
            maxY = worldY;
            maxTexture = proxyTex;
            if (dX || dZ)
            {
              // convert to world space
              Vector3 normalProxy = obj->FutureVisualState().DirectionModelToWorld(proxy.GetTransform().Rotate(Vector3(-proxyDX,1,-proxyDZ)));
              ComputeDDFromNormal(maxDX,maxDZ,normalProxy);
            }
          }
        }
      }
    }
#endif
    
    if( maxY>=-1e3 )
    {
      if (dX || dZ)
      {
        if( dX ) *dX=maxDX;
        if( dZ ) *dZ=maxDZ;
      }
      if( texture )
        *texture=maxTexture;
      return maxY-obj->FutureVisualState().PositionModelToWorld(modelPos).Y();
    }
  }
  else
  {
    // only one polygon should contain the point when we look from the top
    float minUnder = 1e10;
    float maxDX=0, maxDZ=0;
    Texture *maxTexture=NULL;
    int i=0;
    for (int s=0; s<shape->NSections(); s++)
    {
      const ShapeSection &ss = shape->GetSection(s);
      const PolyProperties &pp = animContext.GetSection(shape, s);
      for( Offset f=ss.beg; f<ss.end; shape->NextFace(f),i++ )
      {
        const Poly &face=shape->Face(f);
        const Plane &plane = animContext.GetPlane(shape, i);
        // check if pos can be under face
        // check using face plane equation
        float y,dX,dZ;
        // get landscape parameters

#if _VBS3 //fix for new roads
        if (VUpModel * plane.Normal() > 0.0f)
#else
        if (VUpModel * plane.Normal() > UnderRoadSurfaceCheckOneFace().MaxSteep())
#endif
          continue; // roadway is probably on animated object and object is lying
#if _VBS3
        bool isInside = (plane.Normal().Y() > 0) ? 
            face.InsideFromBottom(animContext.GetPosArray(), plane, modelPos, &y, &dX, &dZ)
          : face.InsideFromTop(animContext.GetPosArray(), plane, modelPos, &y, &dX, &dZ);
        if(isInside)
#else
        if (face.InsideFromTop(animContext.GetPosArray(shape), plane, modelPos, &y, &dX, &dZ))
#endif        
        {
#if _VBS3_CRATERS_DEFORM_TERRAIN // This branch is nicely done than the #else part, but in the end it is not required by the craters - they don't use the ClipLandOn at all
          // Set the dX and dZ in world space (we cannot use the one created in InsideFrom*, because they are in model space)
          {
            // Get face normal in world coordinates
            Vector3 wNormal = obj->DirectionModelToWorld(plane.Normal());

            // Calculate dX and dZ from the world normal
            if (fabs(wNormal.Y()) < 1e-2)
            {
              dX = dZ = 0.0f;
            }
            else
            {
              // Similar calculation like label NORMALDXDZ
              float invNY = 1.0f / wNormal.Y();
              dX = -wNormal.X()*invNY;
              dZ = -wNormal.Z()*invNY;
            }
          }

          // Get the tested point in world coordinates
          Vector3 wPos = obj->PositionModelToWorld(modelPos);

          // Calculate the real surface Y (ground surface + model thickness)
          float landDX, landDZ;
          float realSurfaceY = SurfaceY(wPos[0], wPos[2], &landDX, &landDZ) + y;

          // Calculate how much is the specified point under real surface
          // use max test for this
          // max 1 m step allowed
          float under = realSurfaceY - wPos.Y();

          // Draw the debugging shape
#if STARS
          if (CHECK_DIAG(DECollision))
          {
            Vector3 spos = wPos;
            spos[1] = realSurfaceY;
            _world->GetScene()->DrawCollisionStar(spos,0.025,PackedColor(Color(0.5,0.5,0)));
          }
#endif

          // Avoid checking landscape right above roadway
          if (under < -1.0f) continue;

          // Remember the closest point to the surface (so that some part of the model wouldn't be in the air?)
          if (minUnder > under)
          {
            minUnder = under;
            maxDX = dX + landDX;
            maxDZ = dZ + landDZ;
            maxTexture=pp.GetTexture();
          }
#else
          Vector3 wPos=obj->FutureVisualState().PositionModelToWorld(modelPos);
          float y=SurfaceY(wPos[0],wPos[2],&dX,&dZ);
          // use max test for this
          // max 1 m step allowed
          float under=y-wPos.Y();
#if 0 // STARS
          if (CHECK_DIAG(DECollision))
          {
            Vector3 spos = wPos;
            spos[1] = y;
            _world->GetScene()->DrawCollisionStar(spos,0.025,PackedColor(Color(0.5,0.5,0)));
          }
#endif
          //Log("under %f",under);
          if( under<-1.0f ) continue; // avoid checking landscape right above roadway
          // the face is not sure to be a triangle
          if( minUnder>under )
          {
            minUnder=under,maxDX=dX,maxDZ=dZ;
            maxTexture=pp.GetTexture();
          }
#endif
        }
      }
    }
    if( minUnder < 1e3 )
    {
      //Vector3 retModelPos(modelPos[0],maxY,modelPos[0]);
      //modelPos[1]=maxY;
      //Point3 ret=obj->PositionModelToWorld(modelPos);
      if (dX) *dX = maxDX;
      if (dZ) *dZ = maxDZ;
      if (texture) *texture = maxTexture;
      return minUnder;
    }
  }
  // otherwise return no collision
  return noCollisionY;
}

/// check intersection of a convex component against a bounding box
/**
All computations done in the with space
*/

static bool CalculateIntersectionBox(
  Vector3 *result, const Object *thisObj, Matrix4Val thisToWith,
  const AnimationContext &acThis, const Shape *sThis, const ConvexComponent &cThis,
  const Vector3 *box
)
{
  // calculate sum of center of volume*volume
  // all calculation done in this space
  // clip all faces of cThis with cWith

  // detect singularity - when zero matrix applied, all normals are singular
  if (cThis.NPlanes() < 1 || cThis.GetPlane(acThis.GetPlanesArray(sThis), 0).Normal().SquareSize() == 0) return false;

//   const Vector3 *box = cThis.MinMax();
//   Matrix4Val thisToWith = MIdentity;
  // calculate direction
  LogicalPoly poly;
  LogicalPoly resClip;

  bool someClip = false;
  Vector3 &clipMin = result[0];
  Vector3 &clipMax = result[1];
  for (int i = 0; i < cThis.NPlanes(); i++)
  {
    const Poly &face = cThis.GetFace(sThis, i);
    // construct analytical poly
    poly.Clear();
    // face of this in with space
    for (int f=0; f<face.N(); f++)
    {
      Vector3Val pos = acThis.GetPos(sThis, face.GetVertex(f));
      // verify vertex is in plane
      poly.Add(thisToWith.FastTransform(pos));
    }
    // clip it with all planes of the box
    // 0    1    2    3    4    5
    // xMin xMax yMin yMax zMin zMax
    static const Vector3 n[6]={Vector3(1,0,0),Vector3(-1,0,0),Vector3(0,1,0),Vector3(0,-1,0),Vector3(0,0,1),Vector3(0,0,-1)};
    // N*x+d == 0
    
    static float exSign = -1;
    const float d[6]={-box[1].X(),+box[0].X(),-box[1].Y(),+box[0].Y(),-box[1].Z(),+box[0].Z()};
    Assert(fabs(box[1]*n[0]+d[0])<1e-6f);
    Assert(fabs(box[0]*n[1]+d[1])<1e-6f);
    Assert(fabs(box[1]*n[2]+d[2])<1e-6f);
    Assert(fabs(box[0]*n[3]+d[3])<1e-6f);
    Assert(fabs(box[1]*n[4]+d[4])<1e-6f);
    Assert(fabs(box[0]*n[5]+d[5])<1e-6f);
    for (int i=0; i<6; i++)
    {
      resClip.Clear();
      if (poly.Clip(resClip, n[i]*exSign,d[i]*exSign))
      {
        poly = resClip;
        if (poly.N() < 3) break;
      }
    }
    // check if something is rest after clipping
    if (poly.N() >= 3)
    {
      // from the result compute min-max box
      Vector3 polyMin = poly.Get(0);
      Vector3 polyMax = poly.Get(0);
      for (int i=1; i<poly.N(); i++)
      {
        SaturateMin(polyMin,poly.Get(i));
        SaturateMax(polyMax,poly.Get(i));
      }
      if (!someClip)
        clipMin = polyMin, clipMax = polyMax, someClip = true;
      else
        SaturateMin(clipMin,polyMin),SaturateMax(clipMax,polyMax);
    }
  }

  return someClip && clipMin.X()<clipMax.X() && clipMin.Y()<clipMax.Y() && clipMin.Z()<clipMax.Z();
}


/**
Functions search for axis aligned box in given coordinate space. Position pos needs to be free.

Y and Z components of the box is used as input only - reducing is done in X only.

@param in/out max. box to check, largest free box found
@param ignore object to ignore - e.g. the soldier we are doing the test for
@param pos coordinate space - orientation defines axes of the box, position defines zero.
@param staticOnly ignore objects which are dynamic (used when trying to get long-term information)
@param allowedMinZ what offset from the pos is allowed in z coordinate
*/
void Landscape::CheckBoxFreeOfObjects(Vector3 *minmax, const Object *ignore, const FrameBase &pos, bool staticOnly, float allowedMinZ) const
{
  // scan all objects nearby
  // based on their components reduce the min-max box as needed
  const float maxObjRadius=25;
  
  // compute bounding sphere around a minmax box
  float minMaxRadius = floatMax(minmax[0].Size(),minmax[1].Size());
  int xMin,xMax,zMin,zMax;
  ObjRadiusRectangle(
    xMin,xMax,zMin,zMax,pos.Position(),pos.Position(),minMaxRadius
  );
  
  float maxDist2 = Square(minMaxRadius+maxObjRadius);
  
  Matrix4 toPos = pos.GetInvTransform();
  
  for (int z=zMin; z<=zMax; z++) for (int x=xMin; x<=xMax; x++)
  {
    const ObjectList &list=_objects(x,z);
    int n=list.Size();
    for( int i=0; i<n; i++ )
    {
      Object *obj = list[i];
      // assume distance check will discard most objects
      float dist2 = obj->FutureVisualState().Position().Distance2Inline(pos.Position());
      if (dist2>maxDist2) continue;
      // check object type and collision possibility
      if (obj->GetType()!=Primary && obj->GetType()!=Network
#if _ARMA3_TERRAIN_DECALS
        && obj->GetType() != LandDecal
#endif
        )
      {
        if (staticOnly || obj->GetType()!=TypeVehicle) continue;
        // optimization: following test has no sense for primary/road objects, as exclusion is done for moving entities
        if (obj==ignore) continue;
      }
      const LODShape *objShape = obj->GetShape();
      if (!objShape) continue;
      const Shape *geom = obj->IsWreck() ? objShape->WreckLevel() : objShape->GeometryLevel();
      if (!geom) continue;
      if (!obj->HasGeometry()) continue;
      if (obj->IsPassable()) continue;
      // check with actual object size - somewhat slower than above, however will reject a lot of small objects
      if (dist2>Square(obj->GetRadius()+minMaxRadius)) continue;

      // scan over components, clip minmax box as needed
      const ConvexComponents &ccs = objShape->GetGeomComponents();

      AnimationContextStorageGeom storage;
      AnimationContext animContext(obj->FutureVisualState(),storage);
      geom->InitAnimationContext(animContext, &ccs, true);

      // minmax needs to be clipped in pos space
      obj->AnimateGeometry(animContext);
      animContext.RecalculateNormalsAsNeeded(geom, ccs);
      
      Matrix4 objToPos = toPos*obj->FutureVisualState().Transform();
      for (int c=0; c<ccs.Size(); c++)
      {
        // const ConvexComponent &cc = *ccs[c];

        // if there is any intersection, we need to reduce the box
        // note: sometimes we have more options how to reduce.
        // We reduce so that the resulting box has largest possible smaller of x/z range
        // check intersection of two arbitrary aligned boxes
        // simple approximation: compute bound box of cc in pos space
        // intended usage of this function is using cover. For this the pos is aligned along the tangent of the cover.
        // Therefore for the cover itself we get very reliable results.
        // we assume the object Y axis is untouched
        Vector3 ccMin(+FLT_MAX,+FLT_MAX,+FLT_MAX);
        Vector3 ccMax(-FLT_MAX,-FLT_MAX,-FLT_MAX);
        for (int zc=0; zc<2; zc++) for (int xc=0; xc<2; xc++) for (int yc=0; yc<2; yc++)
        {
          Vector3 modelPos(animContext.GetCCMinMax(c)[xc].X(), animContext.GetCCMinMax(c)[yc].Y(), animContext.GetCCMinMax(c)[zc].Z());
          Vector3 objPos = objToPos.FastTransform(modelPos);
          SaturateMin(ccMin, objPos);
          SaturateMax(ccMax, objPos);
        }
        // we have both ccMin and ccMax in pos space
        // now compute the intersection with minmax
        Vector3 isectMinEst = VectorMax(minmax[0], ccMin);
        Vector3 isectMaxEst = VectorMin(minmax[1] ,ccMax);
        // check if intersection is nonempty and reduce minmax as needed
        if (isectMinEst.X()<isectMaxEst.X() && isectMinEst.Y()<isectMaxEst.Y() && isectMinEst.Z()<isectMaxEst.Z())
        {
          // TODO: more precise test possible: compute minmax box of polyedra/minmax intersection
          Vector3 isectBox[2];
          if (CalculateIntersectionBox(isectBox,obj,objToPos,animContext,geom,*ccs[c],minmax))
          {
            Vector3Val isectMin = isectBox[0];
            Vector3Val isectMax = isectBox[1];
            // intersection verified
            Assert (isectMin.X()<=isectMax.X() && isectMin.Y()<=isectMax.Y() && isectMin.Z()<=isectMax.Z());
            /*
            invariant:
            minmax[0] - XY negative or zero, Z negative
            minmax[1] - XY positive or zero, Z small negative or zero
            */
            // special (but common) case: Geometry may be out of fire-geometry a little bit, this should not prevent us using the cover
          if (isectMin.Z() > allowedMinZ)
          {
            saturateMin(minmax[1][2], isectMin.Z());
          }
          else if (isectMax.X()<0)
          { // intersection is left of pos, move left edge to the right, i.e. increase min
              if (fabs(isectMax.Z())>fabs(floatMax(minmax[0][0], isectMax.X())))
                // if there is more maneuvering in Z than it would be in half of X, rather shorten the Z
              saturateMax(minmax[0][2], isectMax.Z());
            else
              saturateMax(minmax[0][0], isectMax.X());
          }
          else if (isectMin.X()>0)
          { // intersection is right of pos, move right edge to the left, i.e. decrease max
              if (fabs(isectMax.Z())>fabs(floatMin(minmax[1][0], isectMin.X())))
                // if there is more maneuvering in Z than it would be in half of X, rather shorten the Z
              saturateMax(minmax[0][2], isectMax.Z());
            else
              saturateMin(minmax[1][0], isectMin.X());
          }
          else
            // intersection around the point - no solution other than shortening z possible
              saturateMax(minmax[0][2], isectMax.Z());
        }
      }
      }
      obj->DeanimateGeometry();
      // once we know the area is empty, there is no reason to continue
      if (minmax[0][0]>=minmax[1][0] || minmax[0][2]>=minmax[1][2])
        return;
    }
  }
}

static int SelectLevelForLandContant(const Landscape *l, const LODShape *withShape, Matrix4Par withPos, bool enableLandcontact)
{
  int level = withShape->FindLandContactLevel();
  // soldier uses always land-contact
  if (!enableLandcontact || level<0)
    return withShape->FindGeometryLevel();
  else
  {
    // TODO: relevant collision may be with the roadway
    // in such case we are not interested about surface normal
    float dx,dz;
    Vector3Val wPos = withPos.Position();
    float posY=l->SurfaceY( wPos.X(),wPos.Z(), &dx, &dz );
    (void)posY;

    Vector3 normal(-dx,1,-dz);
    if (posY<l->GetSeaLevelMax() && posY<l->GetSeaLevel(wPos)) normal = VUp;

    float surfaceAngle=normal.CosAngle(withPos.DirectionUp());
    if( surfaceAngle<0.86  )
      level=withShape->FindGeometryLevel();
  }
  return level;  
}


/**
@return number of points in the level tested
*/

int Landscape::GroundCollision(GroundCollisionBuffer &retVal, Object *with, const ObjectVisualState &withPos, float above, float bumpy, bool enableLandcontact, bool soldier) const
{
  PROFILE_SCOPE_EX(cLGnd,coll);
  // changed: all vehicles can now use all roadways
  // only soldier can walk on houses and other objects
  //bool testObjects=soldier;

  // check which points are under the ground level
  // for normal orientation use LandContact LOD level
  const LODShape *shape = with->GetShape();
  int level;
  if (with->IsWreck())
    level = shape->FindWreckLevel();
  else if (!soldier)
    level = SelectLevelForLandContant(this, shape, withPos, enableLandcontact);
  else
    level = shape->FindLandContactLevel();
  if ( level<0) return 0;
  ShapeUsed withShape = shape->Level(level);
  if (withShape.IsNull())
  {
    Fail("!withShape");
    return 0;
  }

  // temporary workaround for discardable wrecks - do not test when the vertex data are discarded
  if (withShape->IsLoadLocked()) 
  {
    Vector3Val wPos=withPos.Position();
    int xMin,xMax,zMin,zMax;
    float wRad=with->GetCollisionRadius(withPos);
    ObjRadiusRectangle(xMin,xMax,zMin,zMax,wPos,wPos,wRad);
    int x,z;
    // check against landscape
    AnimationContextStorageGeom storage;
    AnimationContext withContext(withPos,storage);
    withShape->InitAnimationContext(withContext, with->GetShape()->GetConvexComponents(level), true);
    // TODO: check special case: one point ground-collision level
    with->Animate(withContext, level, false, with, -FLT_MAX);
    Matrix4Val withTrans=withPos.Transform();
    for( int j=0; j<withShape->NPos(); j++ )
    {
      Point3 pos(VFastTransform, withTrans, withContext.GetPos(withShape, j));
      float dX,dZ;

      Texture *texture=NULL;
      float surfaceY;
      float bump = 0;
      if( bumpy>0 )
        surfaceY=BumpySurfaceY(pos[0],pos[2],dX,dZ,texture,bumpy,bump);     
      else
        surfaceY=SurfaceY(pos[0],pos[2],&dX,&dZ,&texture);
      //surfaceY += bump;
      float underLevel = surfaceY+above-pos[1];
      if( surfaceY<GetSeaLevelMax())
      {
        // consider water waves
        float seaWithWaves = GetSeaLevel(pos);
        // water on level bumpySea
        float underWater=above+seaWithWaves-pos[1];
        // due to waves even when we are above water we may be below ground
        if( underWater>0 )
        {
          UndergroundInfo &info = retVal.Append();
          info.texture = texture;
          info.obj = NULL;
          info.under = underWater;
          info.pos = pos;
          info.pos[1] = seaWithWaves; // applied directly on surface
          info.dX = 0;
          info.dZ = 0;
          info.vertex = j;
          info.level = level;
          info.type = GroundWater;
        }
      }
      // Now we know we are in contact with the ground. Take bump into account.    
      underLevel += bump;
      if( underLevel<=0 ) continue;
#if 0 //STARS
        if (CHECK_DIAG(DECollision))
          _world->GetScene()->DrawCollisionStar(pos,0.05,PackedColor(Color(1,1,0)));
#endif
      UndergroundInfo &info=retVal.Append();
      info.texture=texture;
      info.obj = NULL;
      info.under=underLevel;
      info.pos=pos;
      info.dX=dX;
      info.dZ=dZ;
      info.vertex=j;
      info.level=level;
      info.type=GroundSolid;
    }
    // check against all near roads
    int nLands=retVal.Size();
    for( z=zMin; z<=zMax; z++ ) for( x=xMin; x<=xMax; x++ )
    {
      //Point3 pos(xC,0,zC);
      if (!IsSomeRoadway(x,z)) continue;
      const ObjectListUsed &list=UseObjects(x,z);
      // optimization - many slots can be skipped
      Assert (!list.IsNull() && list.GetRoadwayCount()>0);

      int n=list.Size();
      for( int i=0; i<n; i++ )
      {
        Object *obj=list[i];
        // check only roads in range
        const LODShape *objShape=obj->GetShape();
        if( !objShape ) continue;
        int roadwayLevel=objShape->FindRoadwayLevel();

        if( roadwayLevel<0 ) continue;
        if( obj==with ) continue;

        // ignore objects for which collision detection is not done
        if (!obj->HasGeometry()) continue;
        float oRad=obj->GetRadius();
        if( obj->FutureVisualState().Position().Distance2Inline(wPos)>Square(oRad+wRad) ) continue;

        PROFILE_SCOPE_EX(cLGnR,coll);
        const Shape *roadway = objShape->RoadwayLevel();
        AnimationContextStorageRoadway storage;
        AnimationContext animContext(obj->GetVisualStateByAge(with->GetVisualStateAge(withPos)),storage);
        roadway->InitAnimationContext(animContext, NULL, true); // no convex components for roadway level

        obj->Animate(animContext, roadwayLevel, false, obj, -FLT_MAX);
        animContext.RecalculateNormalsAsNeeded(roadway);

        Matrix4Val fromWithToObj=obj->FutureVisualState().GetInvTransform()*withTrans;
        float fromWithToObjScale = fromWithToObj.Scale();

        // if the object center of mass is way below the roadway, we need to ignore the collision as well

        Vector3 withCenter = with->GetShape()->GeometryCenter();
        Vector3 center(VFastTransform, fromWithToObj, withCenter);
        center[1] -= above*fromWithToObjScale;

        ObjDebuggingContext ctx;
        for (int j = 0; j < withShape->NPos(); j++)
        {
          Vector3Val wPos = withContext.GetPos(withShape, j);
          Vector3 pos(VFastTransform,fromWithToObj,wPos);
          pos[1] -= above*fromWithToObjScale;
          float dX,dZ; // plane differential in obj model space
          Texture *texture=NULL;
          // if the point itself is way below the surface, ignore the position
          float underLevel=UnderRoadSurface(obj, animContext, pos, soldier ? pos : center, bumpy, ctx, &dX, &dZ, &texture);
          underLevel /= fromWithToObjScale;
          if( underLevel<=-1.5f ) continue;
#if 0 //STARS
            if (CHECK_DIAG(DECollision))
            {
              Vector3 spos = obj->PositionModelToWorld(pos);
              _world->GetScene()->DrawCollisionStar(spos,0.05,PackedColor(Color(1,0,0)));
            }
#endif
          // search if there is already some collision with the same vertex
          if (!soldier && underLevel<-0.15) continue;
          UndergroundInfo info;
          info.texture=texture;
          info.obj = obj;
          info.under=underLevel;
          // FIX + . 25.3.2003 Returned must be landcontact
          //info.pos=obj->PositionModelToWorld(pos);
          info.pos=withPos.PositionModelToWorld(wPos);
          //FIX -
                  
          // dX, dZ is always world space, already normalized
          info.dX = dX;
          info.dZ = dZ;

          info.vertex=j;
          info.level=level;
          bool noAdd = false;
          if (!info.texture || info.texture!=GetSeaTexture() && !info.texture->IsWater())
          {
            float preferRoad = bumpy * 0.5;
            info.type=GroundSolid;
            // prefer road to deal with landscape bumps
            // check with landscape collisions
            for( int p=0; p<nLands; p++ )
            {
              const UndergroundInfo &pInfo=retVal[p];
              if( pInfo.vertex!=j ) continue;
              // use the higher one, prefer road
              // to achieve this delete land info that is already there
              if( pInfo.under<=underLevel+preferRoad )
              {
                retVal.Delete(p);
                nLands--;
              }
              else
                noAdd = true;
              break;
            }
          }
          else
            // do not add water which is below landscape
            info.type = GroundWater;
          // check with other road collisions
          for( int p=nLands; p<retVal.Size(); p++ )
          {
            const UndergroundInfo &pInfo=retVal[p];
            if( pInfo.vertex!=j ) continue;
            // both are roads - use the higher one
            if( pInfo.under<=underLevel )
              retVal.Delete(p);
            else
              noAdd = true;
            break;
          }
          if (!noAdd)
            retVal.Add(info);
        }
        obj->Deanimate(roadwayLevel, false);
      }
    }
    with->Deanimate(level, false);
#if 0 //STARS
    if (CHECK_DIAG(DECollision))
    {
      int i=0;
      for (; i<nLands; i++)
      {
        UndergroundInfo &info=retVal[i];
        _world->GetScene()->DrawCollisionStar(info.pos,0.05,PackedColor(Color(1,1,0)));
      }
      for (; i<retVal.Size(); i++)
      {
        UndergroundInfo &info=retVal[i];
        _world->GetScene()->DrawCollisionStar(info.pos,0.05,PackedColor(Color(1,0,0)));
      }
    }
#endif
  }
  return withShape->NVertex();
}

//WIP:EXTEVARS:FIXME the whole function
void Landscape::ExplosionDamageEffects(EntityAI *owner, Object *directHit, HitInfoPar hitInfo,
  int componentIndex, bool enemyDamage, float energyFactor, float explosionFactor)
{
  Vector3Par pos = hitInfo._pos;
  if (explosionFactor>0 && hitInfo._explosive>0)
  {
    RString effectsName = "ExplosionEffects";
    if (hitInfo._ammoType) effectsName = hitInfo._ammoType->_explosionEffects;
    if (effectsName.GetLength() > 0)
    {
      const float MinExplosion = 0.25f;
      const float MaxExplosion = 1.0f;
      float size = hitInfo._hit * 0.01f * explosionFactor * hitInfo._explosive;
      saturate(size,MinExplosion,MaxExplosion);
      Explosion *explosion=new Explosion(NULL,effectsName,owner,size);
      explosion->SetPosition(pos);
      GWorld->AddAnimal(explosion);
    }
  }
  
  // enemyDamage - some enemy was damaged, reveal actual unit side
  if (owner && hitInfo._ammoType)
  {
    // if fire is visible, disclose position
    // otherwise assume explosion position
    float maxDist = (hitInfo._hit * 0.5f + hitInfo._indirectHit * hitInfo._indirectHitRange * 0.5f) * energyFactor;
    float maxRange = floatMax(300,TACTICAL_VISIBILITY);
    saturate(maxDist,3,maxRange);
    // if fire is visible, disclose owners position
    Disclose(DCExplosion,owner,pos,maxDist,enemyDamage,hitInfo._ammoType->visibleFire>1);
    Stress(owner,pos,hitInfo._ammoType->hit);
  }
  
  float distance2ToCamera = GScene->GetCamera()->Position().Distance2(pos);
  if (distance2ToCamera>Square(Glob.config.horizontZ))
    return;

  Texture *tex = NULL;
  float dX, dZ;
  // if terrain is below water, use water style effects
  float surfY = RoadSurfaceY(pos, &dX, &dZ, &tex);
  bool water = surfY < WaterSurfaceY(pos) - 0.05f;
#if 0// WIP:EXTEVARS:FIXME proper surface normal
  Vector3 surfNormal(-dX, 1, -dZ);
  surfNormal.Normalize();
#endif

  if (hitInfo._ammoType)
  { // handle hit sound
    SurfaceInfo::SoundHitType hitSound = SurfaceInfo::SHDefault;
    // select surface information from LOD
    if (componentIndex >= 0)
    {
      if (directHit)
      {
        const LODShape *lShape = directHit->GetShape();
        // we have better info what was hit, try to find the surface info
        const ConvexComponents &ccGeom = lShape->GetFireComponents();
        if ( componentIndex < ccGeom.Size() )
        {
          const ConvexComponent &cc = *ccGeom[componentIndex];
          const SurfaceInfo *info = cc.GetSurfaceInfo();
          if (info)
            hitSound = info->_soundHit;
        }
      }
    }
    else
    {
      // from texture
      if (tex)
      {
        const SurfaceInfo &info = tex->GetSurfaceInfo();
        hitSound = info._soundHit;
      }
    }
    // define random sample per group
    float randProb = GRandGen.RandomValue();
    const SoundPars *pars = hitSound>=0 && hitSound<SurfaceInfo::NSoundHitTypes ? &hitInfo._ammoType->_hit[hitSound].SelectSound(randProb) : NULL;
    // play hit sound
    if (pars)
    {
      if (pars->name.GetLength() > 0 && GSoundScene->CanBeAudible(pos, pars->distance))
      {
        AbstractWave *sound = GSoundScene->OpenAndPlayOnce(pars->name, NULL, false, pos, VZero, pars->vol, pars->freq, pars->distance);
        
        if (sound)
        {
          float occlusion = 1, obstruction = 1;
          GWorld->CheckSoundObstruction(NULL, false, obstruction, occlusion);

          if (hitInfo._ammoType && (hitInfo._ammoType->_whistleOnFire != 1) && (hitInfo._ammoType->_whistleDist > 0) 
            && GSoundsys->GetListenerPosition().Distance(pos) < hitInfo._ammoType->_whistleDist)
            sound->SetEWPars(true, pos, hitInfo._ammoType->_whistleDist);

          sound->EnableAutoUpdate(true);
          sound->EnableFilter(true, GenericLowPFilter);
          sound->SetObstruction(obstruction, occlusion);
          GSoundScene->SimulateSpeedOfSound(sound);
          GSoundScene->AddSound(sound);
        }
      }
    }
  }

  // camera shake effect
  GWorld->GetCameraShakeManager().AddSource(CameraShakeParams(CameraShakeSource::CSExplosion, pos, hitInfo._ammoType));

  static const float craterTimeCoef = 5.0f;
  static const float craterSizeCoef = 3.5f;
  // small/big explosion
  bool smallExplosion = hitInfo._explosive < 0.5f;

  // prepare evars from hitInfo
  EffectVariables evars(hitInfo);

  if (smallExplosion)
  {
    float landAlpha=floatMin(hitInfo._hit*energyFactor*(1.0f/100),1);
    float scale=landAlpha*0.2f*(GRandGen.RandomValue()*0.3f+0.8f);
    float alpha=10.0f*landAlpha*(GRandGen.RandomValue()*0.3f+0.7f);
    saturateMin(alpha,1.0f);
    saturateMin(scale,0.05f);

    float timeToLive=60.0f*scale*craterTimeCoef;
    saturate(timeToLive,10.0f,30.0f);

    if (directHit)
    { // direct hit on object (smallExplosion && directHit)

      const LODShape *lShape = directHit->GetShape();

      // TODO: do not draw crater on men
      
      LODShape *shape = NULL;
//      LODShape *shape=GLandscape->NewShape("data3d\\colision.p3d",false,false);

      // crater on object/vehicle
      // note: directHit may be a part of some hierarchy

      //Entity *vehicleHit=dyn_cast<Entity>(directHit);
      Matrix4Val toModel = directHit->WorldInvTransform(directHit->RenderVisualState());
      Vector3 rPos = toModel.FastTransform(pos);
      // TODO: find nearest face of finest LOD and use its normal
      Vector3 rDir = hitInfo._inSpeed;
      // rDir is relative to directHit object
      //LogF("Dir %.1f,%.1f,%.1f",rDir[0],rDir[1],rDir[2]);
      rDir.Normalize();

      Crater *crater = NULL;
      if (/*vehicleHit*/true
        )
      {
        RString effectsType;
        float effectsTimeToLive;
        float size = 0.5f * scale;
        bool NoBlood();
        if (!NoBlood() && Glob.config.bloodLevel>0 && dyn_cast<Person>(directHit) != NULL)
        {
          effectsType = "ImpactEffectsBlood";
          effectsTimeToLive = 0.1;
        }
        else
        {
          effectsType = "ImpactEffectsSmall";
          if (hitInfo._ammoType)
          {
            RString impactType = "vehicle";
            if (componentIndex >= 0)
            {
              // we have better info what was hit, try to find the surface info
              const ConvexComponents &ccGeom = lShape->GetFireComponents();
              if ( componentIndex < ccGeom.Size() )
              {
                const ConvexComponent &cc = *ccGeom[componentIndex];
                const SurfaceInfo *info = cc.GetSurfaceInfo();
                if (info && info->_impact.GetLength() > 0) impactType = info->_impact;
              }
            }
            effectsType = hitInfo._ammoType->GetHitEffects(impactType);
          }
          effectsTimeToLive = floatMin(10.0f * size, 0.1f * timeToLive);
#if _VBS3
          // always make sure that the particles are visible
          saturate(effectsTimeToLive, 0.2f, 0.3f);
#else
          saturate(effectsTimeToLive, 0.02f, 0.2f);
#endif
        }
        
        {
          // Create crater (only in case the skeleton is discrete && some component was hit)
          const ConvexComponents &ccGeom = lShape->GetFireComponents();
          SkinnedPosition skinning;                
          if (lShape->IsSkeletonSourceDiscrete() && componentIndex >= 0 && componentIndex < ccGeom.Size() && size > 0)
          {
            // Initialize un-skinned position with the model position (not necessarily unskinned)
            Vector3 unskinnedPosition = rPos;

            // If skinned component was hit, calculate the unskinned position
            {
              // If bones are specified per vertex then read the bone index and weight for the componentIndex (first vertex), else don't transform
              const Shape *s = lShape->FireGeometryLevel();
              if (s->IsVertexBoneRefPresent())
              {
                // Get the vertex index
                const ConvexComponents &ccGeom = lShape->GetFireComponents();
                const ConvexComponent &cc = *ccGeom[componentIndex];

                //BlendSkinning skinning;
                cc.GetSkinning(skinning,s,lShape);

                auto deanimated = directHit->DeanimatePoint(directHit->RenderVisualState(),lShape->FindFireGeometryLevel(),skinning,rPos);
                if (!deanimated.second) goto NoCrater;
                unskinnedPosition = deanimated.first;
              }
            }
            // do not create crater on proxy
            if (directHit->GetDestructType()!=DestructNo && !directHit->GetObjectId().IsNull())
            directHit->AddCrater(unskinnedPosition, size * craterSizeCoef);
NoCrater:   ;
          }
          directHit->DirectHit(hitInfo, skinning);

          Assert(!directHit->GetObjectId().IsNull()); // ID must not be null, as we need to be able to serialize
          // WIP:EXTEVARS:FIXME: fill evars properly
          CraterOnVehicle *cov = new CraterOnVehicle(shape, VehicleTypes.New("#crateronvehicle"), effectsType,
            evars, effectsTimeToLive, timeToLive, size, directHit, rPos, rDir);
          crater = cov;
        }
      }
      else
      {
        RString effectsType;
        float effectsTimeToLive;
        float size = 0.5 * scale;
        if (water)
        {
          effectsType = "ImpactEffectsWater";
          effectsTimeToLive = 0.3;
        }
        else
        {
          effectsType = "ImpactEffectsSmall";
          if (hitInfo._ammoType)
          {
            RString impactType = "object";
            if (componentIndex >= 0)
            {
              // we have better info what was hit, try to find the surface info
              const ConvexComponents &ccGeom = lShape->GetFireComponents();
              const ConvexComponent &cc = *ccGeom[componentIndex];
              if ( componentIndex < ccGeom.Size() )
              {
                const SurfaceInfo *info = cc.GetSurfaceInfo();
                if (info && info->_impact.GetLength() > 0) impactType = info->_impact;
              }
            }
            effectsType = hitInfo._ammoType->GetHitEffects(impactType);
          }
          effectsTimeToLive = floatMin(10.0f * size, 0.1f * timeToLive);
          saturate(effectsTimeToLive, 0.02f, 0.2f);
        }
        Matrix4 transform;
        Matrix4 toWorld = directHit->WorldTransform(directHit->RenderVisualState());
        Vector3Val wDir=toWorld.Rotate(rDir);
        Vector3 pDir=fabs(wDir.Y())<0.9 ? VUp : VForward;
        transform.SetDirectionAndUp(wDir,pDir);
#if !_VBS3_CRATERS_DEFORM_TERRAIN
        transform.SetScale(scale*0.5);
#endif
        Vector3 wPos=toWorld.FastTransform(rPos)-wDir*0.05;
        transform.SetPosition(wPos);
        // WIP:EXTEVARS:FIXME: fill evars properly
        crater = new Crater(transform, shape, VehicleTypes.New("#crater"), effectsType, evars, effectsTimeToLive, timeToLive, size);
      }
      if (crater)
      {
#if !_VBS3_CRATERS_DEFORM_TERRAIN
          crater->SetAlpha(alpha);
#endif
        crater->SetID(NewObjectID());
        GLOB_WORLD->AddAnimal(crater);
      }
    }
    else
    { // hitting ground/water (smallExplosion && !directHit)

      //LODShape *shape=GLandscape->NewShape("data3d\\colision.p3d",false,false);
      LODShape *shape=GLOB_SCENE->Preloaded(CraterShell);
      // crater on ground
      float azimut=GRandGen.RandomValue()*H_PI*2;

      Matrix4 transform(MIdentity);
      transform.SetOrientation(Matrix3(MRotationY,azimut));
#if !_VBS3_CRATERS_DEFORM_TERRAIN
      transform.SetScale(scale*0.5);
#endif
      transform.SetPosition(pos);

      // create vehicle
      RString effectsType;
      float effectsTimeToLive;
      float size = 0.5 * scale;
      if (water)
      {
        effectsType = "ImpactEffectsWater";
        effectsTimeToLive = 0.3;
      }
      else
      {
        effectsType = "ImpactEffectsSmall";
        if (tex && hitInfo._ammoType)
        {
          RString impactType = tex->GetSurfaceInfo()._impact;
          effectsType = hitInfo._ammoType->GetHitEffects(impactType);
        }
        effectsTimeToLive = floatMin(10.0f * size, 0.1f * timeToLive);
        saturate(effectsTimeToLive, 0.02f, 0.2f);
      }
      // WIP:EXTEVARS:FIXME: fill evars properly
      Crater *crater = new Crater(transform, shape, VehicleTypes.New("#crater"), effectsType, evars, effectsTimeToLive, timeToLive, size);
      crater->SetID(NewObjectID());
#if !_VBS3_CRATERS_DEFORM_TERRAIN
      crater->SetAlpha(alpha);
#endif
      GLOB_WORLD->AddAnimal(crater);
    }
  }
  else
  { // big explosion (!smallExplosion)
    float maxY=floatMax(hitInfo._indirectHitRange*2.0, 0.5);
    float y=pos[1]-surfY;
    if( y<maxY )
    {
      // place explosion crater on the ground
      float hit=floatMax(hitInfo._indirectHit * hitInfo._indirectHitRange, 5);
      float landAlpha=(1-y/maxY)*hit*(1.0/200);
      //if( landAlpha>0.05 )
      if( landAlpha>0.001 )
      {
        // crater visible - draw it
        saturateMin(landAlpha, 2);
        // use smoke simulation for drawing
        float scale = landAlpha*(GRandGen.RandomValue()*0.3+0.8);
        float azimut = GRandGen.RandomValue()*H_PI*2;
        Matrix4 transform(MIdentity);
        transform.SetOrientation(Matrix3(MRotationY,azimut));
#if !_VBS3_CRATERS_DEFORM_TERRAIN
        float alpha = 0.5*landAlpha*(GRandGen.RandomValue()*0.3+0.7);
        transform.SetScale(scale*0.5);
#endif
        LODShape *shape=GLOB_SCENE->Preloaded(CraterShell);
        // small/big explosion

        Vector3 offset=transform.Rotate(shape->BoundingCenter());
        transform.SetPosition(Vector3(pos[0],surfY,pos[2])+offset);

        float timeToLive=scale*60;
        timeToLive*=craterTimeCoef;
#if _VBS3
        if(_VBS3_CONSTANT_CRATERS_LIFETIME > 0)    
          timeToLive = _VBS3_CONSTANT_CRATERS_LIFETIME;
        else
#endif
        saturateMin(timeToLive, 120);
        // create vehicle
//LogF("Crater: ammo %s, hit %.0f, y %.3f, landAlpha %.3f, scale %.3f", (const char *)type->GetName(), hit, y, landAlpha, scale);
        
        // TODO: read the explosion config name from the ammo type
        RString effectsType;
        float effectsTimeToLive;
        float size = 0.5f * scale;
        if (water)
        {
          effectsType = "ImpactEffectsWater";
          effectsTimeToLive = 0.3f;
        }
        else
        {
          effectsType = "ImpactEffectsBig";
          if (hitInfo._ammoType)
            effectsType = hitInfo._ammoType->_craterEffects;
          effectsTimeToLive = floatMin(2.0f * size, timeToLive);
          saturate(effectsTimeToLive, 0.2f, 1.0f);
        }
        // WIP:EXTEVARS:FIXME: fill evars properly
        Crater *crater = new Crater(transform, shape, VehicleTypes.New("#crater"), effectsType, evars, effectsTimeToLive, timeToLive, size);
        crater->SetID(NewObjectID());

#if !_VBS3_CRATERS_DEFORM_TERRAIN
        crater->SetAlpha(alpha);
#endif
        GLOB_WORLD->AddAnimal(crater);

        // check if explosion is near the camera
        if (distance2ToCamera<Square(500))
        {
          int count = toIntFloor(landAlpha*10);
          while (--count>=0)
          {
            // some ground effects flying out
            Vector3 vel(
              (GRandGen.RandomValue()*15-7.5)*landAlpha,
              (GRandGen.RandomValue()*10)*landAlpha,
              (GRandGen.RandomValue()*15-7.5)*landAlpha
            );
            Vector3 posOffset(
              (GRandGen.RandomValue()*2-1)*scale,
              (GRandGen.RandomValue()*1)*scale,
              (GRandGen.RandomValue()*2-1)*scale
            );
            Matrix4 fxTrans = transform;
            // random orientation
            fxTrans.SetOrientation(
              Matrix3(MRotationX,GRandGen.RandomValue()*(2*H_PI))*
              Matrix3(MRotationY,GRandGen.RandomValue()*(2*H_PI))
            );
            fxTrans.SetPosition(transform.Position()+posOffset);
            ThingEffectKind kind = dyn_cast<Transport>(directHit) ? TEArmor: TEGround;
            Entity *fx = CreateThingEffect(kind,fxTrans,vel);
            if (fx)
            {
              // perform additional effects on fx (set angular momentum)
            }
          }
        }
      }
    }
  }
}


HitInfo::HitInfo(const AmmoType *ammoType, Vector3Par pos,  Vector3Par surfNormal, Vector3Par inSpeed, Vector3Par outSpeed)
: _pos(pos), _surfNormal(surfNormal), _inSpeed(inSpeed), _outSpeed(outSpeed)
{
  _ammoType = ammoType;
#if _VBS2
  _external = 0;
#endif
  _hit = ammoType ? ammoType->hit : 0.0f;
  _indirectHit = ammoType ? ammoType->indirectHit : 0.0f;
  _indirectHitRange = ammoType ? ammoType->indirectHitRange : 0.0f;
  _explosive = ammoType ? ammoType->explosive : 0.0f;
}

/*!
@param energyFactor if the bullet energy was lessened by 
@param resultSpeed speed at which the bullet departs the explosion site
Used to calculate difference in the momentum.
*/
void Landscape::ExplosionDamage(EntityAI *owner, Vector3Par originDir, Object *directHit, Vector3Par directHitModelPos, HitInfoPar hitInfo,
  Object *componentOwner, int componentIndex, float energyFactor, float explosionFactor, float hitDepth)
{
  if (!GetNetworkManager().IsServerOwner())
  {
    // we need to request damage processing on the server
    // shot cannot be passed, as it is not a real network object
    GetNetworkManager().ExplosionDamage(owner,originDir,directHit,directHitModelPos,hitInfo,componentOwner,componentIndex,energyFactor,explosionFactor, hitDepth);
    return;
  }
  //ADD_COUNTER(explo,1);
  bool enemyDamage=false; // detect if we damaged some enemy unit

  AIBrain *ownerUnit = owner ? owner->CommanderUnit() : NULL;

  // TODO: try to merge results for the direct object
  const float impulseKinScale = 1.0f;
  const float impulseExplScale = 100.0f;
  // simulate direct hit
  Vector3Par pos = hitInfo._pos;
  RString ammo = hitInfo._ammoType ? RString(hitInfo._ammoType->ParClass().GetName()) : RString();
  Object::DoDamageResult directHitResult;
  if (directHit)
  {
    float hitVal = (hitInfo._hit-hitInfo._indirectHit);

    // shot effects should be synchronized with the apparent object position -> use RenderVisualState
    Matrix4Val invTransform = directHit->WorldInvTransform(directHit->RenderVisualState());
    Vector3Val center = invTransform.FastTransform(pos);

    Object::ShotInfo shotInfo(hitInfo._ammoType, owner,center, hitInfo._ammoType!=nullptr, true, originDir);
    directHit->DirectDamage(directHitResult,shotInfo,hitVal,energyFactor);
    // if directHit is vehicle, add force impulse
    Entity *veh = dyn_cast<Entity>(directHit);
    if (veh && hitInfo._ammoType)
    {
      if (ownerUnit && ownerUnit->IsEnemy(veh->GetTargetSide()))
        enemyDamage = true;
      float factor=veh->GetShape()->GeometrySphere()*(1.0/6);
      Vector3 relPos=pos-veh->COMPosition(veh->RenderVisualState());
      Vector3 forcePos=relPos+Vector3(
        GRandGen.RandomValue()*2*factor-factor,
        GRandGen.RandomValue()*2*factor-factor,
        GRandGen.RandomValue()*2*factor-factor
        );
      // calculate force
      // direct hit - transfer inertia plus direct explosion
      Vector3 force = (hitInfo._inSpeed-hitInfo._outSpeed)*(hitInfo._ammoType->GetMass()*impulseKinScale);
      // note: some ammo is not explosive
      // this applied especially for bullets
      if (hitInfo._explosive>0)
        force += relPos.Normalized()*(hitVal*factor*-impulseExplScale)*hitInfo._explosive;
      veh->AddImpulseNetAware(force,forcePos.CrossProduct(force));
    }
  }
  // damage all objects in maxRadius
  // damage drops with the square of distance
  const float maxRadius=hitInfo._indirectHitRange*4;
  const float hit=hitInfo._indirectHit*energyFactor;

  //ExplosionDamage is used for melee too, but without indirect damage, so we can quickly skip this part
  if(maxRadius > 0 && hit > 0)
  {
    int xMin,xMax,zMin,zMax;
  #if 0
    LogF("%s: Indirect damage max range %.2f, damage %.1f",(const char *)hitInfo._ammoType->GetName(),maxRadius,hit);
  #endif
    ObjRadiusRectangle(xMin,xMax,zMin,zMax,pos,pos,maxRadius);
    for (int x = xMin; x <= xMax; x++) for (int z = zMin; z <= zMax; z++)
    {
      const ObjectList &list=_objects(x,z);
      int n=list.Size();
      for( int i=0; i<n; i++ )
      {
        Object *obj=list[i];

        if( !obj->GetShape() ) continue;
        if( obj->GetType()==Temporary ) continue;
        if( obj->GetType()==TypeTempVehicle ) continue;
        if (!obj->CanBeHit()) continue;
        float dist2=obj->RenderVisualState().Position().Distance2(pos);
        float maxDist2=Square(maxRadius+obj->GetRadius());
        /*
        LogF("%s test range %.2f, range %.2f",(const char *)obj->GetDebugName(),sqrt(maxDist2),sqrt(dist2));
        */
        if( dist2>maxDist2 ) continue;
        // obj is probably effected by an explosion
  #if 0
        LogF("%s: Explosion damage %.2f range %.1f dist %.1f",(const char *)obj->GetDebugName(),hit,hitInfo._indirectHitRange,sqrt(dist2));
  #endif


        // contain a list of all the events for this one object
        AutoArray<HitEventElement> hitList;

        // First step here.
        // 1. have an explosion.
        // 2. have the object within the radius.
        // 3. Go through each component see if it has reference to a name selection
        // 4. is within explosion radius, then execute script for that named selection
        LODShape *shp = obj->GetShape();
        int level = shp->FindFireGeometryLevel();

        EntityAI *entity = dyn_cast<EntityAI>(obj);       
        if(maxRadius>0 && level >= 0 && (entity && entity->IsEventHandler(EEHitPart)))
        {
          // search for and find the name selection for this component
          const Shape *fire = shp->FireGeometryLevel();
          const ConvexComponents &fireComponent = shp->GetFireComponents();

          AnimationContextStorageGeom storage;
          AnimationContext animContext(obj->FutureVisualState(), storage);
          fire->InitAnimationContext(animContext, shp->GetConvexComponents(level), true);

          obj->Animate(animContext, level, false, obj, -FLT_MAX);


          for( int z = 0; z < fire->NNamedSel(); ++z )
          {
            // Need to calculate the center
            Vector3 minMax[2];
            Vector3 center;
            Vector3 direction;
            minMax[0] = minMax[1] = VZero;

            const NamedSelection &nameSel = fire->NamedSel(z);
            const Selection &faceIndecies = nameSel.Faces();

            if(nameSel.Size()>0)
            {
              minMax[0] = minMax[1] = fire->Pos(nameSel[0]);
              for( int k = 1; k < nameSel.Size(); k++ )
              {
                SaturateMin(minMax[0],fire->Pos(nameSel[k]));
                SaturateMax(minMax[1],fire->Pos(nameSel[k]));
              }
            }
            // Calculate the center
            center = minMax[0] + ((minMax[1] - minMax[0])/2.0f);

            // calculate the direction from center to pos
            direction = (center-pos);
            direction.Normalize();
            // calculate the world center pos
            obj->FutureVisualState().PositionModelToWorld(center,center);

            float dist2c = center.Distance2(pos);
            float maxDist2c=Square(maxRadius/2.0f);
            if(dist2c<=maxDist2c)
            {
              for( int y = 0; y < fireComponent.Size(); ++y)
              {
                const Ref<ConvexComponent> component = fireComponent[y];

                AutoArray<RString> nameSelections;
                RString surfaceType; 
                float   radius = component->GetRadius();

                const SurfaceInfo* surf = component->GetSurfaceInfo();
                if(surf)
                  surfaceType = surf->_surfaceType;

                if(component->NPlanes() == 0 ) continue;
                if(radius == 0.0f)             continue;            

                int cFaceIndicie = component->GetPlaneIndex(0);
                for( int x = 0; x < faceIndecies.Size(); ++x )
                {
                  if( faceIndecies[x] == cFaceIndicie)
                  {
                    nameSelections.Add(nameSel.Name());
                    break;
                  }
                }

                // only register for named selection hits
                if(nameSelections.Size()==0) continue;

                // Add it to the master list, to be executed by some
                // script in one go.
                int index = hitList.Add();
                hitList[index].Init
                (
                  hitInfo._ammoType,
                  owner,
                  center,
                  Vector3(0.0f,0.0f,0.0f),
                  nameSelections,
                  direction, 
                  radius, 
                  surfaceType,
                  false,
                  0,
                  nullptr // TODO: add skinning information
                );

                // We only need to find one component that has not been animated
                // can safely ignore the rest and continue onto the next name selection
                break;
              }            
            }
          }

          if(hitList.Size()>0)
            entity->OnHitPartEvent(EEHitPart,hitList);    

          obj->Deanimate(level,true);
        }

        // check direct visibility line between obj and pos
        float unshielded = 1;
        if (obj!=directHit)
        {
          // no shield checking on short distance


  #if 0 //_ENABLE_CHEATS
          // visualization of the shielding intersection test
          LODShape *shape = GScene->Preloaded(SphereModel);
          {
            Ref<TempDiag> diag = new TempDiag(shape,CreateObjectId(),Color(0,1,0,0.5),30.0f);
            diag->SetPosition(pos);
            diag->SetScale(0.1f);
            GWorld->AddAnimal(diag);
          }
          {
            Ref<TempDiag> diag = new TempDiag(shape,CreateObjectId(),Color(0,1,1,0.5),30.0f);
            diag->SetPosition(obj->Position());
            diag->SetScale(0.1f);
            GWorld->AddAnimal(diag);
          }
  #endif

          // objCenter
          float objectSize=obj->VisibleSizeRequired(obj->FutureVisualState())*0.8f;
          // even the object which was hit (directHit) can shield other objects,
          // unless it is the object itself which is exploding (like exploding vehicle causing damage around it)
          // object explosion is recognized by shot being NULL
          // when there is no shot, pass directHit as skip1 argument
          Object *directShield = hitInfo._ammoType==NULL ? directHit : NULL;
          unshielded = Visible(obj->GetRenderVisualStateAge(),pos,obj->COMPosition(obj->RenderVisualState()),objectSize,directShield,obj,ObjIntersectIFire);
  #if 0
          LogF("%s unshielded %.2f, directHit %s, owner %s, shot %s",
            (const char *)obj->GetDebugName(),unshielded,
            directHit ? (const char *)directHit->GetDebugName() : "<null>",
            owner ? (const char *)owner->GetDebugName() : "<null>",
            shot ? (const char *)shot->GetDebugName() : "<null>");
  #endif
        }
        /*
        if (unshielded<1)
          LogF("%s unshielded %.2f",(const char *)obj->GetDebugName(),unshielded);
        */

        float unshieldedHit = hit*unshielded; 
        if (unshieldedHit>0)
        {
          Object::DoDamageResult result;
          float indirectHitRange = hitInfo._indirectHitRange;

          float indirectHitFactor = obj->GetIndirectHitFactor();

          Matrix4Val invTransform = obj->WorldInvTransform(obj->RenderVisualState());
          Vector3Val center = invTransform.FastTransform(pos);
          Object::ShotInfo shotInfo(hitInfo._ammoType, owner,center, hitInfo._ammoType!=nullptr, true, originDir);
          obj->IndirectDamage(result,shotInfo,unshieldedHit*indirectHitFactor,indirectHitRange);
          if (obj == directHit)
            directHitResult += result;
          else
          {
            // indirect hit has no particular location, we are unable to provide a position
            obj->ApplyDoDamage(result,owner,ammo, SkinnedPosition());
          }

          Entity *veh = dyn_cast<Entity>(obj);
          if (veh && shotInfo.ammo)
          {
            if (ownerUnit && ownerUnit->IsEnemy(veh->GetTargetSide()))
              enemyDamage=true;
            float factor=veh->GetShape()->GeometrySphere()*(1.0f/6);
            Vector3 relPos=pos-veh->COMPosition(veh->RenderVisualState());
            // random explosion momentum
            Vector3 forcePos=relPos+Vector3(
              GRandGen.RandomValue()*2*factor-factor,
              floatMin(0,-GRandGen.RandomValue()*2*factor-factor), // cause force to lead up
              GRandGen.RandomValue()*2*factor-factor
            );
            // calculate force
            float relDist = relPos.SquareSize();
            float minRelDist = Square(hitInfo._indirectHitRange);
            float hitFactor = relDist>minRelDist ? minRelDist/relDist : 1;
            float hitVal = unshieldedHit*hitFactor;
            // indirect hit - only explosion
            Vector3 relDir = relPos.Normalized();
            // the speed we can give is not very high
            // certainly not above the speed of the sound, but probably much less
            // assume speed higher than 15 m/s is not possible
            // moreover, if the object is already moving away, we cannot accelerate it any more
          
            const float maxSpeedAway = 15.0f;
            // we also consider also the impulses which are already given to the vehicle
            // this is done to avoid simultaneous explosions causing excessive acceleration
          
            // ignore shot speed - we assume speed of the explosion is always zero
            // explosion medium is the air, which is static
            Vector3 relSpeed = veh->SpeedWithImpulse(); // -shot->Speed();
            float relSpeedAway = floatMax(0,-relSpeed*relDir);
            float maxVelDelta = maxSpeedAway-relSpeedAway;
            float forceSize = hitVal*factor*impulseExplScale;
            float velDelta = forceSize*veh->GetInvMass();
            // never add more then maxVelDelta
            if (velDelta>maxVelDelta)
            {
              // make sure forceSize*veh->GetInvMass() == maxVelDelta
              forceSize = maxVelDelta*veh->GetMass();
              // force can never be negative
              if (forceSize<0)
                forceSize = 0;
            }
            Vector3 force = relDir*-forceSize;
          
            veh->AddImpulseNetAware(force,forcePos.CrossProduct(force));
          }
        }
      }
    } 
  }

  if (directHit)
  {
    SkinnedPosition skinning;
    if (componentOwner==directHit && componentIndex>=0)
    {
      skinning.obj = directHit;
      LODShape *lShape = directHit->GetShape();
      if (lShape->GetSkeleton())
      {
        const ConvexComponents &fireComponent = lShape->GetFireComponents();
        // caution: componentIndex is transferred over network, if client/server data are not matching, we may get it wrong
        if (componentIndex>=0 && componentIndex<fireComponent.Size())
        {
          const ConvexComponent *component = fireComponent.Get(componentIndex);

          component->GetSkinning(skinning,lShape->FireGeometryLevel(),lShape);

          auto deanimated = directHit->DeanimatePoint(directHit->RenderVisualState(),lShape->FindFireGeometryLevel(),skinning,directHitModelPos);
          if (!deanimated.second)
          {
            skinning.bones.Clear();
          }
          else
          {
            skinning.originalPosition = deanimated.first;
          }
        }
      }
    }
    
    directHit->ApplyDoDamage(directHitResult,owner,ammo,skinning);
  }

  ExplosionDamageEffects(owner,componentOwner,hitInfo,componentIndex,enemyDamage,energyFactor,explosionFactor);

  // if hit object is not a network object, we cannot reference it
  if ((directHit) && ((directHit->GetType() == Temporary || directHit->GetNetworkId().IsNull())))
    directHit = NULL;
  GetNetworkManager().ExplosionDamageEffects(owner,componentOwner,hitInfo,componentIndex,enemyDamage,energyFactor,explosionFactor);
}

void Landscape::ApplyProjectileDecal(Object *directHit, int componentIndex, HitInfoPar hitInfo, float hitDepth,
                                     Vector3Par hitPosLcl, Vector3Par hitDirLcl, Vector3Par hitDirUpLcl)
{
  if (directHit && componentIndex >= 0)
  {
    SkeletonIndex domBone = SkeletonIndexNone;
    SkinnedPosition skinning;
    skinning.obj = directHit;
    const LODShape *lShape = directHit->GetShape();
    if (lShape->GetSkeleton())
    {
      const ConvexComponents &fireComponent = lShape->GetFireComponents();
      // caution: componentIndex is transferred over network, if client/server data are not matching, we may get it wrong
      if (componentIndex >= 0 && componentIndex < fireComponent.Size())
      {
        const ConvexComponent *component = fireComponent.Get(componentIndex);
        component->GetSkinning(skinning, lShape->FireGeometryLevel(), lShape);

        float domW = 0;
        for (int i=0; i<skinning.bones.Size(); i++)
        {
          float w = skinning.bones[i].weight;
          if (w>domW)
          {
            domW = w;
            domBone = skinning.bones[i].bone;
          }
        }
      }
    }

    LODShape *projShape = hitInfo._ammoType->_decalShape;
    ProjectileDecalOnVehicle *decal = new ProjectileDecalOnVehicle(directHit, projShape, hitInfo._ammoType, hitDepth, 
                                                                    domBone, hitPosLcl, hitDirLcl, hitDirUpLcl);
    if (decal)
    {
      decal->SetID(NewObjectID());
      Man *man = dyn_cast<Man, Object>(directHit);
      if (man) man->AddProjectileDecal(decal);
    }
  }
}

void Landscape::ApplyProjectileDecal(HitInfoPar hitInfo, float hitDepth)
{
  const bool hasDecal = !hitInfo._ammoType->_decalClass.IsEmpty();
  if (hasDecal)
  {
    Ref<Entity> projectileDecal = GWorld->NewNonAIVehicleWithID(hitInfo._ammoType->_decalClass);
    if (projectileDecal.NotNull())
    {
      Matrix4 transform;
      const Matrix3 dir(MDirection, hitInfo._inSpeed, VUp);
      const Vector3 decalPos(hitInfo._pos + hitDepth * dir.Direction());
      transform.SetPosition(decalPos);
      transform.SetOrientation(dir);
      projectileDecal->SetTransform(transform);

      bool allOK = true;
      InventoryItem *item = dyn_cast<InventoryItem>(projectileDecal.GetRef());
      if (item)
      {
        const InventoryItemType *type = item->Type();
        if (type)
        {
          //game variables
          const int _max = type->GetStackedMax();
          allOK = (_max > 0);
          if (allOK)
          {
            GameVarSpace *vars = item->GetVars();
            vars->VarSet("quantity", 1.f);
          }
        }
      }

      if (allOK)
      {
        // we need other entities nearby to be notified of our presence
        EntityAI* ai = dyn_cast<EntityAI>(projectileDecal.GetRef());
        if( ai )
        {
          GWorld->OnEntityMovedFar(ai);
          ai->OnMoved();
          ai->SetTargetCategory("loot");
        }

        projectileDecal->DisableSimulation(true);
        projectileDecal->SetInvisible(false);
        GWorld->AddSlowVehicle(projectileDecal);

        if (GWorld->GetMode() == GModeNetware)
        {
          GetNetworkManager().CreateVehicle(projectileDecal, VLTSlowVehicle, "", -1);
        }
      }
    }
  }
}

#include <Es/Containers/staticArray.hpp>

void Landscape::Stress(EntityAI *owner, Vector3Par pos, float hit)
{
  float maxDist = InterpolativC(hit,7,100,2.0f,10.0f);
  int xMin,xMax,zMin,zMax;
  ObjRadiusRectangle(xMin,xMax,zMin,zMax,pos,pos,maxDist);
  for( int x=xMin; x<=xMax; x++ ) for( int z=zMin; z<=zMax; z++ )
  {
    const ObjectList &list=_objects(x,z);
    int n=list.Size();
    for( int i=0; i<n; i++ )
    {
      Object *obj=list[i];
      if( obj->GetType()!=TypeVehicle ) continue;
      EntityAIFull *veh=dyn_cast<EntityAIFull>(obj);
      if( !veh ) continue;
      if( veh->FutureVisualState().Position().Distance2(pos)>Square(maxDist) ) continue;
      if (veh==owner) continue;
      veh->OnStress(pos,hit);
    }
  }
}

void Landscape::Disclose(DangerCause cause, EntityAI *owner, Vector3Par pos, float maxDist, bool discloseSide, bool disclosePosition)
{
  // tell all near enemy vehicles they are disclosed
  if (!owner)
  {
    // owner may not exist (for example mines from mission editor)
    LogF("No owner");
    return;
  }
  AIBrain *ownerUnit=owner->CommanderUnit();
  if( !ownerUnit ) return;

#if 0
  LogF("Disclose %s, maxDist %g, disclosePosition %d",(const char *)owner->GetDebugName(),maxDist,disclosePosition);
#endif

  // 
  int xMin,xMax,zMin,zMax;
  ObjRadiusRectangle(xMin,xMax,zMin,zMax,pos,pos,maxDist);
  for( int x=xMin; x<=xMax; x++ ) for( int z=zMin; z<=zMax; z++ )
  {
    const ObjectList &list=_objects(x,z);
    int n=list.Size();
    for( int i=0; i<n; i++ )
    {
      Object *obj=list[i];
      if( obj->GetType()!=TypeVehicle ) continue;
      EntityAIFull *veh=dyn_cast<EntityAIFull>(obj);
      if( !veh ) continue;
      float dist2 = veh->FutureVisualState().Position().Distance2(pos);
      if( dist2>Square(maxDist) ) continue;
      AIBrain *vehBrain=veh->CommanderUnit();
      if( vehBrain && vehBrain->LSIsAlive())
      {
        if (ownerUnit->GetSide() != vehBrain->GetSide())
        {
          //if( myCenter->IsEnemy(veh->GetTargetSide()) )
          // we do not want friendly units to be "disclosing"
          if (vehBrain->IsEnemy(owner->GetTargetSide()))
          {
            vehBrain->Disclose(cause, pos, false);
            if (veh->PilotUnit() && veh->PilotUnit() != vehBrain)
              veh->PilotUnit()->Disclose(cause, pos, owner, false);
            if (veh->GunnerUnit() && veh->GunnerUnit() != vehBrain)
              veh->GunnerUnit()->Disclose(cause, pos, owner, false);
          }
          // add owner to group sensor list
          float delay = 3.0f*veh->GetInvAbility(AKSpotTime);
          // if vehicle already busy, reaction is slower
          if (vehBrain->GetTargetAssigned()) delay *= 2;
#if 0
          LogF("  Disclosed by %s (dist %g)",(const char *)veh->GetDebugName(),veh->Position().Distance(pos));
#endif

          float accuracy = 0.1f;
          float sideAcc = discloseSide ? 1.5f : 0.1f;
          if (disclosePosition)
            vehBrain->AddTarget(owner,accuracy,sideAcc,delay,NULL,vehBrain);
          else
            // assume owner is at explosion position 
            vehBrain->AddTarget(owner,accuracy,sideAcc,delay,&pos,vehBrain);
          
        }
      }
    }
  }
}

void Landscape::PredictCollision(VehicleCollisionBuffer &ret, const Entity *vehicle, float maxTime, float gap, float maxDistance) const
{
  PROFILE_SCOPE_EX(cLPre,coll);
  //float posDistance=1e10; // distance for pos point
  // predict collision with all near objects and vehicles
  // use bounding sphere test only
  float vRadius=vehicle->CollisionSize()*0.8;
  //Vector3Val beg=vehicle->Position();
  float maxDist2=vehicle->FutureVisualState().Speed().SquareSize()*Square(maxTime);
  saturateMax(maxDist2,Square(maxDistance));
  //Vector3 end=vehicle->Position()+vehicle->Speed()*maxTime;

  float radius = maxDist2*InvSqrt(maxDist2);
  int xMin,xMax,zMin,zMax;
  Vector3Val vehPos = vehicle->FutureVisualState().Position();
  ObjRadiusRectangle(xMin,xMax,zMin,zMax,vehPos,radius);
  for (int x=xMin; x<=xMax; x++) for (int z=zMin; z<=zMax; z++)
  {
    const ObjectListFull *list = _objects(x,z).GetList();
    if (!list || list->GetNonStaticCount()<=0) continue; // no vehicles in this list
    for (int i=0; i<list->Size(); i++)
    {
      const Object *obj = list->Get(i);
      if (obj->GetType()!=TypeVehicle) continue;
      if (obj->FutureVisualState().Position().Distance2Inline(vehPos)>maxDist2) continue;
      const EntityAI *ai = dyn_cast<const EntityAI>(obj);
      if (!ai) continue;

      if( !ai->GetShape() ) continue;
      if( !ai->GetShape()->GeometryLevel() ) continue;
      if( !ai->HasGeometry() ) continue;
      // special test for vehicle collisions
      // run prediction loop
      if( ai==vehicle ) continue;

#if _ENABLE_ATTACHED_OBJECTS
      //don't collide with an attached entity!
      // if (ai->GetAttachedTo() == vehicle || vehicle->GetAttachedTo() == ai) 
      if (ai->AreAttached(vehicle))
        continue;
#endif

      // find analytical solution
      float minTime=NearestPoint(vehicle->FutureVisualState().Position(),vehicle->FutureVisualState().Speed(),ai->FutureVisualState().Position(),ai->FutureVisualState().Speed());
      saturate(minTime,0,maxTime);
      Vector3 pt1 = vehicle->FutureVisualState().Position()+vehicle->FutureVisualState().Speed()*minTime;
      Vector3 pt2 = ai->FutureVisualState().Position()+ai->FutureVisualState().Speed()*minTime;
      float minDist2 = pt1.Distance2Inline(pt2);

      if( minDist2<Square(maxDistance) )
      {
        float cRadius=ai->CollisionSize();
        Vector3Val norm = (ai->FutureVisualState().Position()-vehicle->FutureVisualState().Position());
        Vector3 relDir=norm.Normalized();
        float relSpeed = relDir*(vehicle->FutureVisualState().Speed()-ai->FutureVisualState().Speed());
        // if relative speed is small, there is no need to have big gap?
        float iGap=Interpolativ(relSpeed,0,10,gap*0.2,gap);
        float sumR=vRadius+cRadius+iGap;
        // search for nearest collision
        float minDist=sqrt(minDist2);

        if( minDist<sumR )
        {
          /*
          if( (Entity *)vehicle==GWorld->CameraOn() )
          {
            LogF
            (
              "minDist %s,%s: %.2f, max %.2f, time %.3f, rad %.1f,%.1f",
              (const char *)vehicle->GetDebugName(),
              (const char *)ai->GetDebugName(),
              sqrt(minDist2),maxDistance,
              minTime,vRadius,cRadius
            );
          }
          */

          VehicleCollision &info=ret.Append();
          // calculate collision point
          info.pos = pt2;
          info.who = ai;
          info.time = minTime;
          info.distance = minDist-vRadius-cRadius;
          info.xSeparation = (pt2-pt1)*vehicle->FutureVisualState().DirectionAside();
        }
      }
    }
  }
}

struct VisCheckContext
{
  const Object *skip1,*target;
  Vector3 sensorPos;
  Vector3 objectPos;
  Vector3 lineDist;
  Vector3 lineDir;
  float lineSize;
  float objectSize;
  float objectSeem;
  VisualStateAge age;
};

float Landscape::CheckVisibility(int x, int z, const VisCheckContext &context, ObjIntersect isect) const
{
// PROFILE_SCOPE_GRF_EX(visCh, *, 0xffffffff);

  // note: return value is never used
  if( !land_InRange(x,z) ) return 1;
  const ObjectListFull *list=_objects(x,z).GetList();
  if (!list) return 1;

  int no=list->Size();
  ADD_COUNTER(viTst,no);
  float visibleThroughResult = 1;
  for( int oi=0; oi<no; oi++ )
  {
    Object *obstacle=list->Get(oi);
    // note: accessing obstacle contect may be very expensive
    // because of memory latency
    LODShape *obstacleShape=obstacle->GetShape();
    if( !obstacleShape ) continue;

    //cheapest reject tests should go first
    if (isect==ObjIntersectFire || isect==ObjIntersectIFire)
    {
      if( !obstacle->OcclusionFire() )
        continue;
    }
    else if (isect==ObjIntersectGeom)
    {
      if (!obstacle->HasGeometry())
        continue;
    }
    else
    {
      if( !obstacle->OcclusionView() )
        continue;
    }
    if( obstacle==context.skip1 || obstacle==context.target )
      continue;
    if( obstacle->GetType()==TypeTempVehicle )
      continue;
    if( obstacle->GetDestructType()==DestructTree && obstacle->IsDestroyed() )
      continue;

    // some obstacles may be ignored - this is used for laser target designation
    if (context.target && context.target->IgnoreObstacle(obstacle,isect))
      continue;

    //Line-sphere test is done *again* in Object::IntersectLine
/*
    // first of all check distance - this is likely to sort out most objects
    Vector3Val obstaclePos = obstacle->VisiblePosition(obstacle->FutureVisualState());
    // calculate distance of obj from object..sensor line
    float t = context.lineDir * (obstaclePos-context.sensorPos);
    saturate(t,0.01f,context.lineSize);
    Vector3 nearest = t*context.lineDir+context.sensorPos;
    float dist2 = obstaclePos.Distance2Inline(nearest);
    float obstacleSize = obstacle->GetRadius(); // VisibleSize();
    // check object distance from line
    if( dist2>Square(obstacleSize) ) continue;
*/    
    // object is near - may intersect
    CollisionBuffer result;
    /*
    class IntersectionCallback
    {
    public:
      virtual bool operator()(CollisionInfo& info)=0;
    };
    
    class VisibilityCallback: public IntersectionCallback
    {
      ObjIntersect _isect;

    public:
      virtual bool operator()(CollisionInfo& info)
      {
      }

      VisibilityCallback(ObjIntersect isect): _isect(isect)
      {
      }
    };

    VisibilityCallback check(isect);
    //TODO: put there functor instead of result array, as we can stop testing much earlier, based on penetrability/visibility tests
    */
    obstacle->IntersectLine(context.age,result,context.objectPos,context.sensorPos,0,isect);
    if( result.Size()>0 )
    {
      float visibleThrough = 1;
      if (isect==ObjIntersectIFire)
      {
        // indirect fire dammage
        // check if we can fire through - check object armor / distance through
        // sum all object travel distance along the line
        float distanceThrough = 0;
        for (int i=0; i<result.Size(); i++)
           distanceThrough += result[i].dirOut.Size();
        float obstacleLogArmor = obstacle->GetLogArmor();

        // armor protection function is:
        // c^distanceThrough, where c is given by object armor
        //float unshielded = pow(1/obstacleArmor,distanceThrough);
        //float unshielded = exp(-obstacleLogArmor*distanceThrough)*3;
        // smaller multiplier means better shielding
        // multiplier 1.0 is enough to keep soldier behind and 10 cm wall with armor 300 alive and almost healthy
        float unshielded = exp(-obstacleLogArmor*distanceThrough)*1.0f;
        // calculate armor density
        /*
        LogF(
          "%s to %s - %s: distanceThrough %.3f, armor %.3f, unshielded %.3f",
          context.target ? cc_cast(context.target->GetDebugName()) : "<null>",
          context.skip1 ? cc_cast(context.skip1->GetDebugName()) : "<null>",
          cc_cast(obstacle->GetDebugName()),
          distanceThrough,obstacle->GetArmor(),unshielded
        );
        */

        visibleThrough = floatMin(unshielded,1);

      }
      else if (isect==ObjIntersectFire)
      {
        // skip glass
        const Texture *glass = GPreloadedTextures.New(TextureBlack);
        float slowdown = 0;
        for (int i=0; i<result.Size(); i++)
        {
          const CollisionInfo &info = result[i];
          if (info.texture == glass) continue;
          float penetrability = info.BulletPenetrability();
          if (penetrability>0)
          {
            float distInside = info.DistInside();
            float thickness = info.Thickness();
            if (thickness>0) distInside = floatMin(distInside,thickness*2);

            slowdown += penetrability*distInside; // *caliber - but caliber not known here
            if (slowdown<200) continue; // not slowing down much, can fire through
          }
          visibleThrough = 0;
          break;
        }

        // direct fire damage
#if DIAG_VISIBLE_THROUGH>=30
          float distanceThrough = 0;
          for (int i=0; i<result.Size(); i++)
             distanceThrough += result[i].dirOut.Size();
          LogF("  fire obstacle, through %.2f", distanceThrough);
#endif
      }
      else 
      {
        // normal visibility test
        float viewDensity = obstacle->ViewDensity();
        // make exponential fog of obstacle alpha density
        // check if we can see through
        if (viewDensity<=-9)
        {
          // some obstacles are considered opaque
          visibleThrough = 0;
#if DIAG_VISIBLE_THROUGH>=30
          if (context.skip1 && context.target)
          {
            float distanceThrough = 0;
            for (int i=0; i<result.Size(); i++)
               distanceThrough += result[i].dirOut.Size();
            LogF("  %s to %s: %s, solid, obstacleAlpha %.3f, through %.2f",
              (const char *)context.skip1->GetDebugName(),(const char *)context.target->GetDebugName(),
              (const char *)obstacleShape->Name(),obstacleAlpha,distanceThrough);
          }
#endif
        }
        else
        {
          // consider object alpha
          // sum all object travel distance along the line
          float distanceThrough = 0;
          for (int i=0; i<result.Size(); i++)
            distanceThrough += result[i].dirOut.Size();
          //float viewDensity = obstacle->ViewDensity();
          float vtLog = viewDensity*distanceThrough;
          if (vtLog<=-4.6f) // -4.6 gives 0.01 visibility
          {
            visibleThrough=0;
#if DIAG_VISIBLE_THROUGH>=20
            if (context.skip1 && context.target)
              LogF("  %s to %s: %s, opaque, distance %.1f, viewDensity %.2f",
                (const char *)context.skip1->GetDebugName(),(const char *)context.target->GetDebugName(),
                (const char *)obstacleShape->Name(),distanceThrough,viewDensity);
#endif
          }
          else
          {
            visibleThrough = exp(vtLog);
            saturateMin(visibleThrough,1);
#if DIAG_VISIBLE_THROUGH>=10
            if (context.skip1 && context.target)
              LogF("  %s to %s: %s, distance %.1f, visibleThrough %.2f, dens %.2f",
                (const char *)context.skip1->GetDebugName(),(const char *)context.target->GetDebugName(),
                (const char *)obstacleShape->Name(),distanceThrough,visibleThrough,viewDensity);
#endif
          }
        }
      }
      if (visibleThrough<=0) return visibleThrough;
      visibleThroughResult *= visibleThrough;
    }
  }
  return visibleThroughResult;
}

const float MaxAbove = 10.0f;

/**
@return distance how much the line is under the surface
@param underPos t where the line is max. under
*/
float Landscape::CheckUnderLandDist(Vector3Par beg, Vector3Par dir, float tMin, float tMax, int x, int z, float &underT, TerrainCache *cache) const
{
  // the extreme value must be one of the three points: rect entry, rect exit, rect diagonal

  // beg+tMin*dir and beg+tMax*dir must be in square (x,z)

  Vector3 begPoint = beg+tMin*dir; // entry into the grid
  Vector3 endPoint = beg+tMax*dir; // leave the grid
  float minLineY = floatMin( begPoint.Y(), endPoint.Y() );
  
  float above = _terrain->CheckSeparationGuaranteed(x,z,minLineY);
  if (above>MaxAbove) return -MaxAbove;


  float y00,y01,y10,y11;
  _terrain->GetRectY(x,z,y00,y01,y10,y11,cache);

  float maxSurfY = floatMax( y00, y01, y10, y11);
  // we want to know not only intersections, but also height left above the ground
  if (minLineY+MaxAbove>maxSurfY )
    return -MaxAbove; // no intersection guaranteed

  // each face is divided to two triangles
  // determine which triangle contains beg/end point
  float xInBeg = (begPoint.X()-x*GetTerrainGrid())*GetInvTerrainGrid(); // relative 0..1 in square
  float zInBeg = (begPoint.Z()-z*GetTerrainGrid())*GetInvTerrainGrid();
  float xInEnd = (endPoint.X()-x*GetTerrainGrid())*GetInvTerrainGrid(); // relative 0..1 in square
  float zInEnd = (endPoint.Z()-z*GetTerrainGrid())*GetInvTerrainGrid();

  float yBeg =  RectBilint(y00,y01,y10,y11,xInBeg,zInBeg);
  float yEnd =  RectBilint(y00,y01,y10,y11,xInEnd,zInEnd);

  // calculation is done in vertical, as this is a lot simpler and most often accurate enough
  float begUnder = yBeg-begPoint.Y();
  float endUnder = yEnd-endPoint.Y();
  
  // how much under the surface the max is
  float under;
  // t for point where under was reached
  if (begUnder>endUnder)
  {
    under = begUnder;
    underT = tMin;
  }
  else
  {
    under = endUnder;
    underT = tMax;
  }

//  Vector3 begOnSurface(begPoint.X(),yBeg,begPoint.Z());
//  Vector3 endOnSurface(endPoint.X(),yEnd,endPoint.Z());

  // check if we are crossing the diagonal
  if (RectIsInAOrB(xInBeg,zInBeg)!=RectIsInAOrB(xInEnd,zInEnd))
  {
    // check intersection with diagonal
    float squareX = x*GetTerrainGrid(), squareZ = z*GetTerrainGrid();
    float diagNom = GetTerrainGrid()-beg.X()-beg.Z()+squareX+squareZ;
    float diagDenom = dir.X()+dir.Z();
    // note: diagDenom is constant across many iterations
    // note: diagDenom may be <0
    // we need it>=0 to be able to avoid division
    //float diagT = diagNom/diagDenom;
    // following condition should be true, we know we are crossing it
    // avoid division
    //if (diagDenom<0) diagNom = -diagNom, diagDenom=-diagDenom;
    //if (diagNom>=tMin*diagDenom && diagNom<=tMax*diagDenom)
    // note: we need both diagT and 1/diagT
    float diagT = diagNom/diagDenom;
    // calculate position on diagonal
    Vector3 diagPoint = beg+diagT*dir;

    float xInDiag = (diagPoint.X()-x*GetTerrainGrid())*GetInvTerrainGrid(); // relative 0..1 in square
    float zInDiag = (diagPoint.Z()-z*GetTerrainGrid())*GetInvTerrainGrid();

    float yDiag =  RectBilintDiag(y00,y01,y10,y11,xInDiag,zInDiag);
    
    float diagUnder = yDiag-diagPoint.Y();
    if (under<diagUnder)
    {
      under = diagUnder;
      underT = diagT;
    }
  }
  return under;
}

static float TravelUnder(Vector3Par begPoint, Vector3Par endPoint, float yBeg, float yEnd)
{
  float begYDif = yBeg-begPoint.Y();
  float endYDif = yEnd-endPoint.Y();
  if (begYDif>0 && endYDif>0)
  { // both points under, their distance is the travel distance
    return begPoint.Distance(endPoint);
  }
  else if (begYDif*endYDif<=0 && (begYDif>0 || endYDif>0))
  {
    // one below, second above
    float above = -floatMin(begYDif,endYDif);
    float below = +floatMax(begYDif,endYDif);
    float d = begPoint.Distance(endPoint);
    return d*below/(above+below);
  }
  else
  { // both above
    return 0; 
  }
}

float Landscape::ComputeUnder(
  float y00, float y01, float y10, float y11, int x, int z, float tMin, float tMax, Vector3Par beg, Vector3Par dir, float *travelUnder
) const
{
  DoAssert(tMin>=-0.02f);
  DoAssert(tMax>=-0.02f);
  Vector3 begPoint = beg+tMin*dir;
  Vector3 endPoint = beg+tMax*dir;

  float maxSurfY = floatMax( y00, y01, y10, y11);
  float minLineY = floatMin( begPoint.Y(), endPoint.Y() );

  // we want to know not only intersections, but also height left above the ground
  if (minLineY>maxSurfY )
  {
    if (travelUnder) *travelUnder = 0;
    return 0; // no intersection guaranteed
  }

  // each face is divided to two triangles
  // determine which triangle contains beg/end point
  float xInBeg = (begPoint.X()-x*GetTerrainGrid())*GetInvTerrainGrid(); // relative 0..1 in square
  float zInBeg = (begPoint.Z()-z*GetTerrainGrid())*GetInvTerrainGrid();
  float xInEnd = (endPoint.X()-x*GetTerrainGrid())*GetInvTerrainGrid(); // relative 0..1 in square
  float zInEnd = (endPoint.Z()-z*GetTerrainGrid())*GetInvTerrainGrid();
  
  float yBeg =  RectBilint(y00,y01,y10,y11,xInBeg,zInBeg);
  float yEnd =  RectBilint(y00,y01,y10,y11,xInEnd,zInEnd);

  // calculation is done in vertical, as this is a lot simpler and most often accurate enough
  float begUnder = tMin>0 ? (yBeg-begPoint.Y())/tMin : 0;
  float endUnder = tMax>0 ? (yEnd-endPoint.Y())/tMax : 0;

  // how much under the surface the max is
  float under;
  // t for point where under was reached
  if (begUnder>endUnder)
  {
    under = begUnder;
    //underT = tMin;
  }
  else
  {
    under = endUnder;
    //underT = tMax;
  }

  // check if we are crossing the diagonal
  if (RectIsInAOrB(xInBeg,zInBeg)!=RectIsInAOrB(xInEnd,zInEnd))
  {
    // check intersection with diagonal
    float squareX = x*GetTerrainGrid(), squareZ = z*GetTerrainGrid();
    float diagNom = GetTerrainGrid()-beg.X()-beg.Z()+squareX+squareZ;
    float diagDenom = dir.X()+dir.Z();
    // note: diagDenom is constant across many iterations
    float diagT = diagNom/diagDenom;
    // calculate position on diagonal
    Vector3 diagPoint = beg+diagT*dir;

    float xInDiag = (diagPoint.X()-squareX)*GetInvTerrainGrid(); // relative 0..1 in square
    float zInDiag = (diagPoint.Z()-squareZ)*GetInvTerrainGrid();

    float yDiag =  RectBilintDiag(y00,y01,y10,y11,xInDiag,zInDiag);
    
    float diagUnder = diagT>0 ? (yDiag-diagPoint.Y())/diagT : 0;
    if (under<diagUnder)
    {
      under = diagUnder;
    }
    if (travelUnder)
    {
      *travelUnder = TravelUnder(begPoint,diagPoint,yBeg,yDiag) + TravelUnder(diagPoint,endPoint,yDiag,yEnd);
    }
  }
  else
  {
    if (travelUnder)
    {
      *travelUnder = TravelUnder(begPoint,endPoint,yBeg,yEnd);
    }
  }
  return under;
}


struct Landscape::CheckUnderLandContext
{
  bool considerGrass;
  float grassVisibility; // accumulate grass visibility factor
  #if _PROFILE
  InitValT<float,int> grassDistance; // accumulate grass visibility factor
  #endif
  Vector3 beg;
  Vector3 dirT;
  Vector3 dirG;
};

/**
@return angle how much are we under the surface
*/
float Landscape::CheckUnderLandAngle(float tMin, float tMax, int x, int z, CheckUnderLandContext &ctx) const
{

  // beg+tMin*dir and beg+tMax*dir must be in square (x,z)
  
  // we assume grass layer is completely opaque - we return continuous angle of visibility anyway

  float y00,y01,y10,y11;
  _terrain->GetRectY(x,z,y00,y01,y10,y11);

  float underTerrain = ComputeUnder(y00, y01, y10, y11, x, z, tMin, tMax, ctx.beg, ctx.dirT, NULL);
  if (ctx.considerGrass)
  {
    float g00 = y00, g01 = y01, g10 = y10, g11 = y11;
    float gg00,gg01,gg10,gg11;
    GetGrassRectY(x,z,1,1,gg00,gg01,gg10,gg11);
    static float grassHeightFactor = 2.0f;
    g00 += gg00*grassHeightFactor, g01 += gg01*grassHeightFactor, g10 += gg10*grassHeightFactor, g11 += gg11*grassHeightFactor;

    float g;
    ComputeUnder(g00, g01, g10, g11, x, z, tMin, tMax, ctx.beg, ctx.dirG, &g);
    if (g>0)
    {
      static float logDensity = -0.25f; // more negative means more opaque
      //pow(density,g) = exp (g*ln density)
      float grassFog = exp(logDensity*g);
      ctx.grassVisibility *= grassFog;
      #if _PROFILE
      ctx.grassDistance += g;
      #endif
    }
  }
  //float underGrass = 
  return underTerrain;
}

/**
@return angle how much is the line under the surface
@param from line start
@param dir line direction (needs to be normalized)
@param pos point where the depth of the line under surface is maximum
@param bendUp bend the casted line along Y axis. Amount of "bending" per one meter of squared XZ distance from "from" point
  (Used to reduce shadow length for landscape shadows)
@param bendUpOutside same as bendUp, but outside of valid terrain range.
  (Used to reduce shadow length in TerSynth area even more)
@param cache used to optimize TerSynth data acquisition
*/
float Landscape::IntersectUnder(Vector3Par from, Vector3Par dir, float maxDist, float bendUp, float bendUpOutside, TerrainCache *cache) const
{
  // dir 
  DoAssert(fabs(dir.SquareSize()-1.0f)<1e-3);
  PROFILE_SCOPE_EX(cLUnd,coll);
  
  float deltaX = dir.X();
  float deltaZ = dir.Z();

  float dirSizeXZ = dir.SizeXZ();
  int xInt = toIntFloor(from.X()/GetTerrainGrid());
  int zInt = toIntFloor(from.Z()/GetTerrainGrid());
  
  int xInt0 = xInt, zInt0 = zInt;
  (void)xInt0,(void)zInt0;

  float invDeltaX = fabs(deltaX)<1e-10 ? 1e10 : 1/deltaX;
  float invDeltaZ = fabs(deltaZ)<1e-10 ? 1e10 : 1/deltaZ;
  int ddx = deltaX>=0 ? 1 : -1;
  int ddz = deltaZ>=0 ? 1 : -1;
  float dnx = deltaX>=0 ? GetTerrainGrid() : 0;
  float dnz = deltaZ>=0 ? GetTerrainGrid() : 0;

  // maintain beg, end on current square
  float tRest = maxDist;
  int maxIter = toInt(Glob.config.horizontZ*GetInvTerrainGrid()*15);
  float tMin = 0, tMax = 0;
  // 
  float maxNUnder = -MaxAbove;
  float maxNUnderT = 0;
  while (tRest>0)
  {
    if (--maxIter<0)
    {
      LogF("Too much iterations");
      break;
    }
    //Vector3 beg = pos;
    tMin = tMax;

    //advance to next relevant neighbor
    int xio = xInt, zio = zInt;
    Vector3 tPoint = from+dir*tMin;
    float tx = (xInt*GetTerrainGrid()+dnx-tPoint.X()) * invDeltaX;
    float tz = (zInt*GetTerrainGrid()+dnz-tPoint.Z()) * invDeltaZ;
    float dt;
    if (tx<=tz)
    {
      xInt += ddx;
      dt = tx;
    }
    else
    {
      zInt += ddz;
      dt = tz;
    }
    saturateMin(dt,tRest);
    tRest -= dt;
    tMax += dt;

    // computing intersections outside of normal landscape (using TerSynth) is a lot slower
    // therefore we want to bend more there to make sure we end sooner
    float bendUpFactor = land_TerrainInRange(xio,zio) ? bendUp : bendUpOutside;
    
    // if we are very high and going even higher, it makes no sense to continue
    // this is important optimization for shadow casting calculation
    float bendBeg = Square(tMin)*dirSizeXZ*bendUpFactor;
    float bendEnd = Square(tMax)*dirSizeXZ*bendUpFactor;
  
    
    Vector3 bentFrom = from+Vector3(0,bendBeg,0);
    Assert(dt>0 || tMax<=tMin); // when dt is 0, bendBeg must be equal to bendEnd (tMax equal to tMin)
    Vector3 bentDir = dir + Vector3(0,dt>0 ? (bendEnd-bendBeg)/dt : 0,0);
    if (dir.Y()>=0)  
    {
      Vector3 begPoint = bentFrom+tMin*dir; // entry into the grid
      if (begPoint.Y()>_terrain->_maxHeight)
        // above the highest point - no more intersections possible
        break;
    }
    
    // check end-beg segment
    float maxUnderT;
    float maxUnder = CheckUnderLandDist(bentFrom,bentDir,tMin,tMax,xio,zio,maxUnderT,cache);
    // max. distance under ground
    if (maxNUnder<maxUnder)
    {
      maxNUnder = maxUnder;
      maxNUnderT = maxUnderT;
    }
  }

  if (PROFILE_SCOPE_NAME(cLUnd).IsActive())
  {
    PROFILE_SCOPE_NAME(cLUnd).AddMoreInfo(Format("%d,%d->%d,%d",xInt0,zInt0,xInt,zInt));
  }
  return maxNUnder;
}

#define VISIBILITY_PERF       0
#define VISIBILITY_MULTICORE  0

/// helper structure, used for particle visibility intersection debugging
struct ISectInfo
{
  float minVis;
  float factor;
  float size;
  int isect;
  ClassIsSimple(ISectInfo)
};

#if VISIBILITY_PERF || VISIBILITY_MULTICORE

/// Aggregation of Landscape::CheckVisibility results
class CheckVisibilityContext : public IMTTContext
{
protected:
  float _result;
  bool &_done;

public:
  CheckVisibilityContext(float result, bool &done) : _result(result), _done(done) {}

  // access to the stored values
  float GetResult() const {return _result;}
  void UpdateResult(float result)
  {
    _result *= result;
    if (_result <= 0.025) _done = true;
  }
  bool IsDone() const {return _done;}

  // implementation of IMTTContext
  virtual IMTTContext *CreateContext(char *mem) const
  {
    ::new(mem) CheckVisibilityContext(1, _done); // placement new to avoid allocations
    return reinterpret_cast<IMTTContext *>(mem);
  } 
  virtual void AggregateContext(const IMTTContext *ctx)
  {
    const CheckVisibilityContext *context = static_cast<const CheckVisibilityContext *>(ctx);
    UpdateResult(context->GetResult());
  }

};

/// Functor calling Landscape::CheckVisibility, used for parallelization
class CheckVisibilityFunc : public IMicroJob
{
protected:
  const Landscape *_landscape;
  int _x;
  int _z;
  const VisCheckContext *_context;
  ObjIntersect _isect;

public:
  void Init(const Landscape *landscape, int x, int z, const VisCheckContext *context, ObjIntersect isect)
  {
    _landscape = landscape; _x = x; _z = z;
    _context = context; _isect = isect;
  }

  virtual void operator () (IMTTContext *ctx)
  {
    CheckVisibilityContext *context = static_cast<CheckVisibilityContext *>(ctx);
    if (!context->IsDone())
    {
      float visible = _landscape->CheckVisibility(_x, _z, *_context, _isect);
      context->UpdateResult(visible);
    }
  }
};

TypeIsMovable(CheckVisibilityFunc)

#endif

#if 0 // _ENABLE_CHEATS
# pragma optimize("",off)
#endif

float Landscape::Visible(VisualStateAge age, Vector3Par from, Vector3Par to, float toRadius, const Object *skip1, const Object *target, ObjIntersect isect, bool cloudlets) const
{
#if DIAG
  LogF("***From %s to %s, type %d",
    skip1 ? (const char *)skip1->GetDebugName() : "<null>",
    target ? (const char *)target->GetDebugName() : "<null>",
    isect);
#endif
  PROFILE_SCOPE_EX(cLVis,coll);
  // maximum sampling is _landGrid
  float objectSize=toRadius;
  Vector3Val objectPos=to;
  Vector3Val sensorPos=from;

  Vector3 objDir=objectPos-sensorPos;
  float dist2=objDir.SquareSize();
  // if no distance is there, nothing can obstruct it
  if (dist2<=0) return 1;
  float invDist=InvSqrt(dist2);
  float dist=dist2*invDist;
  objDir*=invDist;

  Vector3 pos = sensorPos;
  Vector3 tgtT = objectPos+Vector3(0,-objectSize,0);
  Vector3 tgtG = objectPos;
  Vector3 dNormT = tgtT-pos;
  Vector3 dNormG = tgtG-pos;
  float normFactor = dNormT.InvSize();
  dNormT *= normFactor;
  dNormG *= normFactor; // keep the same step in the x,z direction
  float deltaX = dNormT.X();
  float deltaZ = dNormT.Z();

  float minVis = 1;

  {
    int xInt = toIntFloor(pos.X()/GetTerrainGrid());
    int zInt = toIntFloor(pos.Z()/GetTerrainGrid());

    float invDeltaX = fabs(deltaX)<1e-10 ? 1e10 : 1/deltaX;
    float invDeltaZ = fabs(deltaZ)<1e-10 ? 1e10 : 1/deltaZ;
    int ddx = deltaX>=0 ? 1 : -1;
    int ddz = deltaZ>=0 ? 1 : -1;
    float dnx = deltaX>=0 ? GetTerrainGrid() : 0;
    float dnz = deltaZ>=0 ? GetTerrainGrid() : 0;

    // maintain beg, end on current square
    float tRest = dist;
    int maxIter = toInt(Glob.config.horizontZ*GetInvTerrainGrid()*15);
    float tMin = 0, tMax = 0;
    // global result accumulator
    float maxNUnder = 0;
    CheckUnderLandContext ctx;
    ctx.considerGrass = isect==ObjIntersectView && GWorld->GetGrassEnabled();
    ctx.grassVisibility = 1.0f;
    ctx.beg = sensorPos;
    ctx.dirT = dNormT;
    ctx.dirG = dNormG;
    while (tRest>0)
    {
      if (--maxIter<0)
      {
        LogF("From %s to %s, type %d",
          skip1 ? (const char *)skip1->GetDebugName() : "<null>",
          target ? (const char *)target->GetDebugName() : "<null>",
          isect);
        LogF("Max iters failed (dist %.1f), from %.1f,%.1f,%.1f, to %.1f,%.1f,%.1f",
          dist,sensorPos[0],sensorPos[1],sensorPos[2],objectPos[0],objectPos[1],objectPos[2]);
        LogF("hz %.0f, hz*5 %.0f",Glob.config.horizontZ,Glob.config.horizontZ*5);
        break;
      }
      //Vector3 beg = pos;
      tMin = tMax;
      DoAssert(tMax>=-0.02f);

      //advance to next relevant neighbourgh
      int xio = xInt, zio = zInt;
      Vector3 tPoint = pos+dNormT*tMin;
      float tx = (xInt*GetTerrainGrid()+dnx-tPoint.X()) * invDeltaX;
      float tz = (zInt*GetTerrainGrid()+dnz-tPoint.Z()) * invDeltaZ;
      if (tx<=tz)
      {
        saturateMin(tx,tRest);
        xInt += ddx;
        tMax += tx;
        tRest -= tx;
        DoAssert(tMax>=-0.02f);
      }
      else
      {
        saturateMin(tz,tRest);
        zInt += ddz;
        tRest -= tz;
        tMax += tz;
        DoAssert(tMax>=-0.02f);
      }

      // check end-beg segment
      float maxUnder = CheckUnderLandAngle(tMin,tMax,xio,zio,ctx);
      // max. angle under ground
      if (maxUnder>maxNUnder) maxNUnder = maxUnder;
      // check if something of the object is visible still
      // object visibility would be 1 - maxNUnder/(objectSize*2*invDist);
      // if this is <=0, object is fully occluded
      // 
      //if (1 - maxNUnder/(objectSize*2*invDist)<=0)
      if ((objectSize*2*invDist)<=maxNUnder)
      {
#if DIAG
        LogF("  fully covered by ground");
#endif
        // no need for any more calculations - already fully covered
        return 0;
      }
    }
    // maxNUnder is max. angle under ground
    // object angular size is objectSize*2*invDist
    minVis = 1 - maxNUnder/(objectSize*2*invDist);
    // when we see too little, the grass pattern will most likely hide it completely
    if (ctx.grassVisibility<0.05f) ctx.grassVisibility = 0;
    minVis *= ctx.grassVisibility;

    // if landscape fully covers, no need to test objects
    if (minVis<1e-3)
      return minVis;
  }

  saturate(minVis,0,1);

  // consider objects
  // check all objects on sensor..object line

  VisCheckContext context;
  context.lineDist=objectPos-sensorPos;
  context.lineDir=objDir;
  context.lineSize=dist;
  context.objectSeem=objectSize*invDist;
  context.objectPos=objectPos;
  context.sensorPos=sensorPos;
  context.skip1=skip1;
  context.target=target;
  context.objectSize=objectSize;
  context.age = age;
  
  // perform DDA los traversal

  //const int border=4; // how much squares are reserved
  int border=toIntFloor(1.5+objectSize*GetInvLandGrid());

  int ox=toIntFloor(objectPos.X()*GetInvLandGrid());
  int oz=toIntFloor(objectPos.Z()*GetInvLandGrid());
  int sx=toIntFloor(sensorPos.X()*GetInvLandGrid());
  int sz=toIntFloor(sensorPos.Z()*GetInvLandGrid());
  int aox=abs(ox-sx),aoz=abs(oz-sz);
  const float sBorder=1.5;

#if VISIBILITY_PERF
  __int64 spentSC;
  __int64 spentMC;
#endif

#if VISIBILITY_PERF || VISIBILITY_MULTICORE
  bool done = false; // set when visibility reach 0 to avoid further processing
  CheckVisibilityContext threadContext(1, done);
#endif

  float objVis = 1;
  {
    if( aox>aoz )
    {
      // mainly horizontal - primary coordinate is x, derived is z
      float dd=float(oz-sz)/aox;
      int pd=ox>sx ? +1 : -1;
      int pBeg=sx-pd*border;
      float dBeg=sz-dd*border;
      int plen=aox+2*border+1;

#if VISIBILITY_PERF
      // call all CheckVisibility to create caches
      {
        PROFILE_SCOPE_GRF_EX(visPC, *, 0xffc0c0c0);
        for (int i=0; i<plen; i++)
        {
          float d = dBeg + i * dd;
          int p = pBeg + i * pd;

          int dis=toIntFloor(d-sBorder);
          int die=toIntFloor(d+sBorder);
          for (int di=dis; di<=die; di++)
          {
            CheckVisibility(p, di, context, isect);
          }
        }
      }
#endif

#if VISIBILITY_PERF || !VISIBILITY_MULTICORE
      // Single-core solution
      {
#if VISIBILITY_PERF
        PROFILE_SCOPE_GRF_EX(visSC, *, 0xff000000);
        __int64 start = ProfileTime();
#endif
        for (int i=0; i<plen; i++)
        {
          float d = dBeg + i * dd;
          int p = pBeg + i * pd;

          int dis=toIntFloor(d-sBorder);
          int die=toIntFloor(d+sBorder);
          for (int di=dis; di<=die; di++)
          {
            if (objVis > 0.025)
            {
              float visible = CheckVisibility(p, di, context, isect);
              // note: multiplication is commutative and associative
              objVis *= visible;
            }
          }
        }
#if VISIBILITY_PERF
        spentSC = ProfileTime() - start;
#endif
      }
#endif // single-core

#if VISIBILITY_PERF || VISIBILITY_MULTICORE
      // Multi-core solution
      {
#if VISIBILITY_PERF
        PROFILE_SCOPE_GRF_EX(visMC, *, 0xffc000c0);
        __int64 start = ProfileTime();
#endif
        AutoArray< CheckVisibilityFunc, MemAllocLocal<CheckVisibilityFunc, 128> > funcs;
        {
#if VISIBILITY_PERF
          PROFILE_SCOPE_GRF_EX(visMI, *, 0xffc000c0);
#endif
          for (int i=0; i<plen; i++)
          {
            float d = dBeg + i * dd;
            int p = pBeg + i * pd;

            int dis=toIntFloor(d-sBorder);
            int die=toIntFloor(d+sBorder);
            for (int di=dis; di<=die; di++)
            {
              int index = funcs.Add();
              funcs[index].Init(this, p, di, &context, isect);
            }
          }
        }
        {
#if VISIBILITY_PERF
          PROFILE_SCOPE_GRF_EX(visMP, *, 0xffc000c0);
#endif
          GJobManager.ProcessMicroJobs(&threadContext, sizeof(threadContext), funcs.Data(), funcs.Size(), sizeof(funcs[0]));        
        }
#if VISIBILITY_PERF
        spentMC = ProfileTime() - start;
#else
        objVis = threadContext.GetResult();
#endif
      }
#endif // multi-core
    }
    else if (aoz != 0)
    {
      // mainly vertical - primary coordinate is z, derived is x
      float dd=float(ox-sx)/aoz;
      int pd=oz>sz ? +1 : -1;
      int pBeg=sz-pd*border;
      Assert( abs(pBeg)<2000 );
      float dBeg=sx-dd*border;
      int plen=aoz+2*border+1;

#if VISIBILITY_PERF
      // call all CheckVisibility to create caches
      {
        PROFILE_SCOPE_GRF_EX(visPC, *, 0xffc0c0c0);
        for (int i=0; i<plen; i++)
        {
          float d = dBeg + i * dd;
          int p = pBeg + i * pd;

          int dis=toIntFloor(d-sBorder);
          int die=toIntFloor(d+sBorder);
          for (int di=dis; di<=die; di++)
          {
            CheckVisibility(di,p,context,isect);
          }
        }
      }
#endif

#if VISIBILITY_PERF || !VISIBILITY_MULTICORE
      // Single-core solution
      {
#if VISIBILITY_PERF
        PROFILE_SCOPE_GRF_EX(visSC, *, 0xff000000);
        __int64 start = ProfileTime();
#endif
        for (int i=0; i<plen; i++)
        {
          float d = dBeg + i * dd;
          int p = pBeg + i * pd;

          int dis=toIntFloor(d-sBorder);
          int die=toIntFloor(d+sBorder);
          for (int di=dis; di<=die; di++)
          {
            if (objVis > 0.025)
            {
              float visible = CheckVisibility(di,p,context,isect);
              // note: multiplication is commutative and associative
              objVis *= visible;
            }
          }
        }
#if VISIBILITY_PERF
        spentSC = ProfileTime() - start;
#endif
      }
#endif // single-core

#if VISIBILITY_PERF || VISIBILITY_MULTICORE
      // Multi-core solution
      {
#if VISIBILITY_PERF
        PROFILE_SCOPE_GRF_EX(visMC, *, 0xffc000c0);
        __int64 start = ProfileTime();
#endif
        AutoArray< CheckVisibilityFunc, MemAllocLocal<CheckVisibilityFunc, 128> > funcs;
        {
#if VISIBILITY_PERF
          PROFILE_SCOPE_GRF_EX(visMI, *, 0xffc000c0);
#endif
          for (int i=0; i<plen; i++)
          {
            float d = dBeg + i * dd;
            int p = pBeg + i * pd;

            int dis=toIntFloor(d-sBorder);
            int die=toIntFloor(d+sBorder);
            for (int di=dis; di<=die; di++)
            {
              int index = funcs.Add();
              funcs[index].Init(this, di, p, &context, isect);
            }
          }
        }
        {
#if VISIBILITY_PERF
          PROFILE_SCOPE_GRF_EX(visMP, *, 0xffc000c0);
#endif
          GJobManager.ProcessMicroJobs(&threadContext, sizeof(threadContext), funcs.Data(), funcs.Size(), sizeof(funcs[0]));        
        }
#if VISIBILITY_PERF
        spentMC = ProfileTime() - start;
#else
        objVis = threadContext.GetResult();
#endif
      }
#endif // multi-core
    }
    else
    {
#if VISIBILITY_PERF
      // call all CheckVisibility to create caches
      {
        PROFILE_SCOPE_GRF_EX(visPC, *, 0xffc0c0c0);
        for (int ssx=-border; ssx<=border; ssx++)
          for (int ssz=-border; ssz<=border; ssz++)
          {
            CheckVisibility(sx+ssx,sz+ssz,context,isect);
          }
      }
#endif

#if VISIBILITY_PERF || !VISIBILITY_MULTICORE
      // Single-core solution
      {
#if VISIBILITY_PERF
        PROFILE_SCOPE_GRF_EX(visSC, *, 0xff000000);
        __int64 start = ProfileTime();
#endif
        for (int ssx=-border; ssx<=border; ssx++)
          for (int ssz=-border; ssz<=border; ssz++)
          {
            if (objVis > 0.025)
            {
              float visible = CheckVisibility(sx+ssx,sz+ssz,context,isect);
              // note: multiplication is commutative and associative
              objVis *= visible;
            }
          }
#if VISIBILITY_PERF
        spentSC = ProfileTime() - start;
#endif 
      }
#endif // single-core

#if VISIBILITY_PERF || VISIBILITY_MULTICORE
      // Multi-core solution
      {
#if VISIBILITY_PERF
        PROFILE_SCOPE_GRF_EX(visMC, *, 0xffc000c0);
        __int64 start = ProfileTime();
#endif
        AutoArray< CheckVisibilityFunc, MemAllocLocal<CheckVisibilityFunc, 128> > funcs;
        {
#if VISIBILITY_PERF
          PROFILE_SCOPE_GRF_EX(visMI, *, 0xffc000c0);
#endif
          for (int ssx=-border; ssx<=border; ssx++)
            for (int ssz=-border; ssz<=border; ssz++)
            {
              int index = funcs.Add();
              funcs[index].Init(this, sx+ssx, sz+ssz, &context, isect);
            }
        }
        {
#if VISIBILITY_PERF
          PROFILE_SCOPE_GRF_EX(visMP, *, 0xffc000c0);
#endif
          GJobManager.ProcessMicroJobs(&threadContext, sizeof(threadContext), funcs.Data(), funcs.Size(), sizeof(funcs[0]));        
        }

#if VISIBILITY_PERF
        spentMC = ProfileTime() - start;
#else
        objVis = threadContext.GetResult();
#endif
      }
#endif // multi-core
    }
  }

#if VISIBILITY_PERF
  if (objVis > 0.025 && fabs(threadContext.GetResult() - objVis) > 0.001)
  {
    RptF("Different results");
  }

  int timeSC = ((10 * spentSC) >> PROF_ACQ_SCALE_LOG) / ProfileScale();
  int timeMC = ((10 * spentMC) >> PROF_ACQ_SCALE_LOG) / ProfileScale();
  int len = intMax(aox, aoz);

  static int totalSC = 0;
  static int totalMC = 0;
  if (len >= 10)
  {
    totalSC += timeSC;
    totalMC += timeMC;
    LogF("Spent in visibility len = %d, (MC / SC) %d us / %d us = %.2f, total %d us / %d us = %.2f",
      len, timeMC, timeSC, (float)timeMC / timeSC, totalMC, totalSC, (float)totalMC / totalSC);
  }
#endif

  if (objVis <= 0.025) return 0;
  
  if (isect==ObjIntersectView && cloudlets)
  {
    #if 0 // _ENABLE_CHEATS
      // record all previous intersections, can be interesting for debugging
      //AutoArray<int, MemAllocLocal<int,1024> > _isectIndex;
      AutoArray<ISectInfo, MemAllocLocal<ISectInfo,1024> > isect;
    #endif
    // check visibility against particles / cloudlets
    // TODO: use OffTree + spatial query
    // sum x instead of summing exp(x) ... this allows us to avoid frequent exp
    for (int i=0; i<GWorld->NCloudlets(); i++)
    {
      const Entity *cloudlet = GWorld->GetCloudlet(i);
      // check cloudlet size and alpha 
      
      float size = cloudlet->VisibleSize(cloudlet->FutureVisualState());
      if (size==0) continue;
      // TODO: alpha should consider texture as well
      // negative alpha is possible - means additive blending
      float alpha = fabs(cloudlet->GetConstantColor().A());
      // check line-sphere intersection
      if (alpha==0) continue;
      
      Vector3Val pos = cloudlet->FutureVisualState().Position();
      Vector3Val nearest = NearestPoint(sensorPos,objectPos,pos);
      float nearestDist2 = nearest.Distance2(pos);
      // if there is no intersection, skip the particle
      if (nearestDist2>Square(size)) continue;
      // assume the particles as more or less spherical
      // compute the chord length
      
      // first approximation: linear attenuation based on chord length / diameter factor
      // radius:r, half of the chord length: x, chord distance from the center: d
      // r*r = d*d + x*x
      //float distanceThrough = 2*sqrt(Square(size)-nearestDist2);
      //float factor = distanceThrough / (2*size);
      
      // TODO: exponential attenuation based on chord length
      
      // it seems linear attenuation based on distance from center may be more accurate
      float factor = 1-sqrt(nearestDist2)/size;
      
      // simulate alpha blending, assume some typical particle alpha
      const float avgAlpha = 0.5f;
      minVis *= 1-(factor*alpha*avgAlpha);
      
#if 0 // _ENABLE_CHEATS
        ISectInfo &is = isect.Append();
        is.isect = i;
        is.size = size;
        is.factor = factor;
        is.minVis = minVis;
#endif
      
      if (minVis<1e-4f)
        return 0;
    }
  }
  return minVis * objVis; // partially visible
}

#if 0 //_ENABLE_CHEATS
# pragma optimize("",on)
#endif

void Landscape::IsInside(StaticArrayAuto< OLink(Object) > &objects, Object *ignore, Vector3Par pos, ObjIntersect isect)
{
  objects.Clear();
  int xMin,xMax,zMin,zMax;
  ObjRadiusRectangle(xMin,xMax,zMin,zMax,pos,25);
  int x,z;
  for( x=xMin; x<=xMax; x++ ) for( z=zMin; z<=zMax; z++ )
  {
    const ObjectList &list=_objects(x,z);
    int n=list.Size();
    for( int i=0; i<n; i++ )
    {
      Object *obj=list[i];
      if( obj==ignore ) continue;
      if (obj->IsInside(pos,isect))
        objects.Add(obj);
    }
  }
}

static RString ShapeName(const Object *obj)
{
  if (!obj) return RString();
  if (!obj->GetShape()) return RString();
  return obj->GetShape()->GetName();
}

static RString ShapeDebugName(const Object *obj)
{
  if (!obj) return RString("<NULL>");
  return ShapeName(obj)+RString(" ")+obj->GetDebugName();
}

/**
@param headOnly returned value describes head area visibility, not the whole vehicle
*/
float Landscape::Visible(bool &headOnly, Vector3Par sensorPos, const Object *sensor, const Object *object, float reserve, ObjIntersect isect, bool cloudlets) const
{
  headOnly = false;
  // maximum sampling is _landGrid
  LODShape *objectShape=object->GetShape();
  //LODShape *sensorShape=sensor->GetShape();
  if( !objectShape ) return 0;
  float objectSize=object->VisibleSizeRequired(object->FutureVisualState())*reserve*0.8;
  // assume some minimal size
  // calculations are based on area covered
  // this would fail if area would be zero
  saturateMax(objectSize,0.01);

  Vector3Val objectPos=object->AimingPosition(object->FutureVisualState());
  
  float dist2 = objectPos.Distance2(sensorPos);
  if (dist2<Square(Glob.config.horizontZ*4))
  {
    float visObj = Visible(object->GetFutureVisualStateAge(),sensorPos,objectPos,objectSize,sensor,object,isect,cloudlets);
    // when object not fully visible, still partially visible, check if we can see the head (if there is any)
    //if (visObj<0.7f && visObj>0.0f)
    // TODO: some better means how to limit head detection that a plain distance
    // TODO: better detection of headless objects
    //if (visObj<0.7f && dist2<Square(200) && dyn_cast<Man>(object))
    if (visObj<0.7f && dist2<Square(200))
    {
      Vector3Val objectHead=object->AimingPositionHeadShot(object->FutureVisualState());
      if (objectHead.Distance2(objectPos)>Square(0.25f))
      {
        float headSize = floatMin(objectSize,0.15f);
        float visHead = Visible(object->GetFutureVisualStateAge(),sensorPos,objectHead,headSize,sensor,object,isect,cloudlets);
        if (visHead>visObj)
        {
          headOnly = true;
          return visHead;
        }
      }
    }
    return visObj;
  }
  return 0;
}

float Landscape::Visible(const Object *sensor, const Object *object, float reserve, ObjIntersect isect, bool cloudlets) const
{
  bool headOnly;
  return Visible(headOnly,sensor->EyePosition(sensor->FutureVisualState()),sensor,object,reserve,isect,cloudlets);
}

float Landscape::VisibleStrategic(Vector3Par from, Vector3Par to) const
{
  // landscape/forest (GeographyInfo) check
  PROFILE_SCOPE_EX(cLVsS,coll);

  //const int maxVisSam=16;
  float dist=to.Distance(from);
  int samples=toIntFloor(dist*GetInvTerrainGrid()*2);
  if (samples<=0) return 1;
  float invSamples = 1.0/samples;
  Vector3 step=(to-from)*invSamples;
  Point3 pt;
  int i;
  int tLog = GetTerrainRangeLog()-GetLandRangeLog();
  for( i=samples,pt=from; --i>=0; pt+=step )
  {
    // use rough SurfaceY estimation only
    int ix=toIntFloor(pt.X()*GetInvTerrainGrid());
    int iz=toIntFloor(pt.Z()*GetInvTerrainGrid());
    if( !land_TerrainInRange(ix,iz) ) continue;
    if( !land_TerrainInRange(ix+1,iz+1) ) continue;
    float y00=GetData(ix,iz);
    float y10=GetData(ix,iz+1);
    float y01=GetData(ix+1,iz);
    float y11=GetData(ix+1,iz+1);
    float y=y00;
    saturateMin(y,y01);
    saturateMin(y,y10);
    saturateMin(y,y11);
    // for this purpose assume sea level is always around 0
    saturateMax(y,0); // GetSeaLevel approximation
    // consider GeographyInfo as part of surface
    y-=3; // be a little bit pesimistic
    int ixt = ix>>tLog;
    int izt = iz>>tLog;
    GeographyInfo g=GetGeography(ixt,izt);
    if( g.u.full ) y+=15;
    static const float objectsHeight[]={0,0,2,5};
    y+=objectsHeight[g.u.howManyObjects];
    if( y>pt[1] ) return 0;
  }
  return 1;
}

#if 0 // not used anymore
/**
@param tRet 0 means beg, 1 means end
*/
bool Landscape::CheckIntersectionOrig(Vector3Par beg, Vector3Par end, int x, int z, float &tRet) const
{
  // beg and end must be in square (x,z)
  // calculate surface level on given coordinates

  float y00,y01,y10,y11;
  GetRectY(x,z,y00,y01,y10,y11);

  float maxSurfY = floatMax( floatMax(y00,y01), floatMax(y10,y11) );
  float minLineY = floatMin( beg.Y(), end.Y() );
  if (minLineY>maxSurfY ) return false;
  // each face is divided to two triangles
  // determine which triangle contains point
  Vector3 normal1;
  Vector3 normal2;
  normal1.Init();
  normal2.Init();
  //
  // triangle 00,01,10
  normal1[0]=(y10-y00)*-GetInvTerrainGrid();
  normal1[1]=1;
  normal1[2]=(y01-y00)*-GetInvTerrainGrid();
  // triangle 01,10,11
  normal2[0]=(y11-y01)*-GetInvTerrainGrid();
  normal2[1]=1;
  normal2[2]=(y11-y10)*-GetInvTerrainGrid();
  // 
  Vector3 point((x+1)*GetTerrainGrid(),y10,z*GetTerrainGrid());
  // size of normal1 and normal2 is nearly 1
  Plane plane1(normal1,point);
  Plane plane2(normal2,point);

  // precision of in A / in B check
  const float inEps=GetInvTerrainGrid()*0.01;

  // calculate intersection with plane under point pos
  Vector3 direction = end-beg;
  float dist1 = plane1.Distance(beg);
  if (dist1<0)
  { // beg already under surface
    // check if beg in this triangle
    float xIn=beg.X()*GetInvTerrainGrid()-x; // relative 0..1 in square
    float zIn=beg.Z()*GetInvTerrainGrid()-z;
    if( xIn<=1-zIn+inEps )
    {
      tRet=0;
      return true;
    }
  }
  float dist2 = plane2.Distance(beg);
  if (dist2<0)
  { // point already under surface
    float xIn=beg.X()*GetInvTerrainGrid()-x; // relative 0..1 in square
    float zIn=beg.Z()*GetInvTerrainGrid()-z;
    if( xIn>=1-zIn-inEps )
    {
      tRet=0;
      return true;
    }
  }

  float denom1=-plane1.Normal()*direction;
  if( fabs(denom1)>1e-10 )
  //if( denom1>1e-10 )
  {
    float t = dist1/denom1;
    if( t>=0 && t<=1 )
    {
      Vector3 nPos=beg+t*direction;

      float xIn=nPos.X()*GetInvTerrainGrid()-x; // relative 0..1 in square
      float zIn=nPos.Z()*GetInvTerrainGrid()-z;
      if( xIn<=1-zIn+inEps )
      {
        tRet=t;
        return true;
      }
    }
  }

  float denom2=-plane2.Normal()*direction;
  if( denom2>1e-10 )
  {
    float t = dist2/denom2;
    if( t>=0 && t<=1 )
    //if( dist2<=denom2 )
    {
      //float t = dist2/denom2;
      Vector3 nPos=beg+t*direction;

      float xIn=nPos.X()*GetInvTerrainGrid()-x; // relative 0..1 in square
      float zIn=nPos.Z()*GetInvTerrainGrid()-z;
      if( xIn>=1-zIn-inEps  )
      {
        tRet=t;
        return true;
      }
    }
  }
  return false;
}
#endif

inline float FloatSign(float x)
{
  if (x>=0) return 1;
  return -1;
}

#if 0 // not used anymore
/**
@param minDist unit is size of dir - beg point is dir*minDist. Note: argument is currently not used.
@param maxDist unit is size of dir - end point is dir*maxDist
*/
float Landscape::IntersectWithGroundOrig(Vector3 *ret, Vector3Par from, Vector3Par dir, float minDist, float maxDist) const
{
  // return time from minDist to maxDist (when intersection is found)
  PROFILE_SCOPE_DETAIL_EX(cLInG,coll);

  // when normalizing direction, we need to change maxDist
  // max
  float size2 = dir.SquareSize();
  if (size2<=0)
  {
    if (ret)
      *ret = from;
    return maxDist;
  }
  float invSize = InvSqrt(size2);
  float size = invSize * size2;
  Vector3 dNorm = dir*invSize;
  // normalize dist so that it is real distance
  if (maxDist>=FLT_MAX)
    maxDist = Glob.config.horizontZ;
  else
    maxDist *= size;
  minDist *= size;
  
  float maxDist0 = maxDist*1.1f;
  //Vector3 direction=dir.Normalized();
  // find first intersection with ground
  // optimized landscape traversal
  // initialize
  Vector3 pos = from;

  float deltaX = dNorm.X();
  float deltaZ = dNorm.Z();

  int xInt = toIntFloor(pos.X()*GetInvTerrainGrid());
  int zInt = toIntFloor(pos.Z()*GetInvTerrainGrid());

  float invDeltaX = fabs(deltaX)<1e-10 ? FloatSign(deltaX)*1e10 : 1/deltaX;
  float invDeltaZ = fabs(deltaZ)<1e-10 ? FloatSign(deltaZ)*1e10 : 1/deltaZ;
  int ddx = deltaX>=0 ? 1 : -1;
  int ddz = deltaZ>=0 ? 1 : -1;
  float dnx = deltaX>=0 ? GetTerrainGrid() : 0;
  float dnz = deltaZ>=0 ? GetTerrainGrid() : 0;

  // maintain beg, end on current square
  float tRest = maxDist;
  float tDone = 0;
  // max. iterations helps to keep number of iterations with a finite limits
  int maxIter = toInt(10000*GetInvTerrainGrid()*15);
  while (tRest>0)
  {
    if (--maxIter<0)
    {
      LogF("IntersectWithGround: Max iterations failed (dist %.1f)",maxDist);
      break;
    }
    Vector3 beg = pos;

    //advance to next relevant neighbor
    int xio = xInt, zio = zInt;
    float tx = (xInt*GetTerrainGrid()+dnx-pos.X()) * invDeltaX;
    float tz = (zInt*GetTerrainGrid()+dnz-pos.Z()) * invDeltaZ;
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

    // check end-beg segment
    float t;
    bool col = CheckIntersectionOrig(beg,pos,xio,zio,t);
    if (col)
    {
      tDone = tDoneBeg + t*beg.Distance(pos);
      if (ret)
        *ret = tDone*dNorm+from;
#if _DEBUG
        // note: only positive tDone handled by old version
        if (tDone>0)
        {
          Vector3 retPoint = beg*(1-t)+pos*t;
          Vector3 testPoint = tDone*dNorm+from;
          float testDist = from.Distance(retPoint);
          DoAssert(testPoint.Distance2(retPoint)<=Square(5e-2));
          DoAssert(fabs(testDist-tDone)<5e-2);
        }
#endif
      //return from.Distance(retPoint)*invSize;
      return tDone*invSize;
    }
  }
  if (ret)
    *ret = from+dNorm*maxDist0;
  return maxDist0;
}
#endif

/*!
inline function which takes the maximum height from four heights
*/
inline void MaxHeight(float h00, float h01, float h10, float h11, float &maxh)
{
  maxh = floatMax(floatMax(h00, h01), floatMax(h10, h11));
}



void DrawDebugQuad(int color, int cellX, int cellZ, float h00, float h10, float h01, float h11, float terrGrid)
{
	Vector3 p00 (cellX*terrGrid, h00, cellZ*terrGrid);
	Vector3 p10 ((cellX + 1)*terrGrid, h10, cellZ*terrGrid);
	Vector3 p01 (cellX*terrGrid, h01, (cellZ + 1)*terrGrid);
	Vector3 p11 ((cellX + 1)*terrGrid, h11, (cellZ + 1)*terrGrid);

	enf::Shape::CreateLine(p00, p10, color, enf::SS_ONCE);
	enf::Shape::CreateLine(p10, p11, color, enf::SS_ONCE);
	enf::Shape::CreateLine(p11, p01, color, enf::SS_ONCE);
	enf::Shape::CreateLine(p01, p00, color, enf::SS_ONCE);

	enf::Shape::CreateLine(p01, p10, color, enf::SS_ONCE);
}


/**
@param tRet 0 means beg, 1 means end
*/
bool Landscape::CheckIntersection(Vector3Par beg, Vector3Par end, float cellx, float cellz, float &tRet,
  const float h00, const float h10, const float h01, const float h11) const
{
  Vector3 normal1, normal2;
  normal1.Init(); normal2.Init();

  // triangle 00,01,10
  normal1[0]=(h10 - h00)*-GetInvTerrainGrid();
  normal1[1]=1;
  normal1[2]=(h01 - h00)*-GetInvTerrainGrid();

  // triangle 01,10,11
  normal2[0]=(h11-h01)*-GetInvTerrainGrid();
  normal2[1]=1;
  normal2[2]=(h11-h10)*-GetInvTerrainGrid();

  // point on both planes
  Vector3 point((cellx + 1)*GetTerrainGrid(), h10, cellz*GetTerrainGrid());

  // size of normal1 and normal2 is nearly 1
  Plane plane1(normal1,point);
  Plane plane2(normal2,point);

  // precision of in A / in B check
  const float inEps=GetInvTerrainGrid()*0.01;

  // calculate intersection with plane under point pos
  Vector3 direction = end-beg;

  float dist1 = plane1.Distance(beg);
  if (dist1<0)
  {
    // relative position of beg in cell
    float xIn=(beg.X() - cellx*GetTerrainGrid())*GetInvTerrainGrid(); // relative 0..1 in square
    float zIn=(beg.Z() - cellz*GetTerrainGrid())*GetInvTerrainGrid();

    // check if beg in this triangle
    if (xIn <= 1 - zIn + inEps)
    {
      tRet = 0;
      return true;
    }
  }

  float dist2 = plane2.Distance(beg);
  if (dist2<0)
  { 
    // relative position of beg in cell
    float xIn=(beg.X() - cellx*GetTerrainGrid())*GetInvTerrainGrid(); // relative 0..1 in square
    float zIn=(beg.Z() - cellz*GetTerrainGrid())*GetInvTerrainGrid();

    if (xIn>=1-zIn-inEps)
    {
      tRet=0;
      return true;
    }
  }

  float denom1=-plane1.Normal()*direction;
  if ( fabs(denom1) > 1e-10 )
  {
    float t = dist1/denom1;
    if (t >= 0 && t <= 1)
    {
      Vector3 nPos = beg+t*direction;

      float xIn=(nPos.X() - cellx*GetTerrainGrid())*GetInvTerrainGrid(); // relative 0..1 in square
      float zIn=(nPos.Z() - cellz*GetTerrainGrid())*GetInvTerrainGrid();

      if( xIn<=1-zIn+inEps )
      {
        tRet=t;
        return true;
      }
    }
  }

  float denom2=-plane2.Normal()*direction;
  if( denom2>1e-10 )
  {
    float t = dist2/denom2;
    if( t>=0 && t<=1 )
    {
      Vector3 nPos = beg+t*direction;

      float xIn=(nPos.X() - cellx*GetTerrainGrid())*GetInvTerrainGrid(); // relative 0..1 in square
      float zIn=(nPos.Z() - cellz*GetTerrainGrid())*GetInvTerrainGrid();
      if( xIn>=1-zIn-inEps  )
      {
        tRet=t;
        return true;
      }
    }
  }
  return false;
}

float Landscape::IntersectWithGround(Vector3 *ret, 	Vector3Par from, Vector3Par dir,	float minDist, float maxDist) const
{
  // return time from 0 to maxDist (when intersection is found)
  PROFILE_SCOPE_DETAIL_EX(TerrainIntersection,coll);

  // when normalizing direction, we need to change maxDist
  // max
  float size2 = dir.SquareSize();
  if (size2<=0)
  {
    if (ret)
      *ret = from;
    return maxDist;
  }

  float size = 1.0f, invSize = 1.0f;
  Vector3 dNorm;

  // do nothing if size of vector is 1
  if (fabs(size2 - 1.0f) > 0.0001)
  {
    invSize	= InvSqrt(size2);
    size		= invSize * size2;
    dNorm		= dir*invSize;
  }
  else
    dNorm = dir;

  // normalize dist so that it is real distance
  if (maxDist>=FLT_MAX)
    maxDist = Glob.config.horizontZ;
  else
    maxDist *= size;

  float maxDist0 = maxDist*1.1f;

  Vector3 pos		= from;
  Vector3 endpos = from + dNorm*maxDist0;

  float deltaX = dNorm.X();
  float deltaZ = dNorm.Z();

  float fxInt = pos.X()/GetTerrainGrid();
  float fzInt = pos.Z()/GetTerrainGrid();

  int xInt = toIntFloor(fxInt);
  int zInt = toIntFloor(fzInt);

	fxInt = xInt;
	fzInt = zInt;


  float invDeltaX = fabs(deltaX)<1e-10 ? FloatSign(deltaX)*1e10 : 1/deltaX;
  float invDeltaZ = fabs(deltaZ)<1e-10 ? FloatSign(deltaZ)*1e10 : 1/deltaZ;

  int ddx, ddz;
  float dnx, dnz, fddx, fddz;

  if (deltaX >= 0)
  {
    ddx	= 1;
    fddx	= 1.0f;
    dnx	= GetTerrainGrid();
  } 
  else 
  {
    ddx	= -1;
    fddx	= -1.0f;
    dnx	= 0.0f;
  }

  if (deltaZ >= 0)
  {
    ddz	= 1;
    fddz	= 1.0f;
    dnz	= GetTerrainGrid();
  }
  else 
  {
    ddz	= -1;
    fddz	= -1.0f;
    dnz	= 0.0f;
  }

  //load heights in first cell
  float h00, h01, h11, h10, maxh;
  _terrain->GetRectY(xInt, zInt, h00, h01, h10, h11);
  MaxHeight(h00, h10, h01, h11, maxh);

  int maxIter = toInt(Glob.config.horizontZ*GetInvTerrainGrid()*15);

  // maintain beg, end on current square
  float tRest = maxDist;
  // float tDone = 0;

  while (tRest > 0.0)
  {
	  if (--maxIter<0)
	  {
		  LogF("Too much iterations");
		  break;
	  }

    //save
    float tDoneBeg = maxDist - tRest;
    Vector3 beg = pos;

    float cellXCurrent = fxInt, cellZCurrent = fzInt;

    //find where to continue
    float tx = (fxInt*GetTerrainGrid()+dnx-pos.X()) * invDeltaX;
	  float tz = (fzInt*GetTerrainGrid()+dnz-pos.Z()) * invDeltaZ;

	  saturateMax(tx, 0.0f);
		saturateMax(tz, 0.0f);

		if (tx<=tz)
    {
      xInt	+= ddx;
      fxInt += fddx;
      tRest -= tx;
      pos	+= dNorm*tx;
    }
    else
    {
      zInt	+= ddz;
      fzInt += fddz;
      tRest -= tz;
      pos	+= dNorm*tz;
    }

    //AAB test of current cell and a part of line
    float minf = floatMin(pos.Y(), beg.Y());
    if (minf < maxh)
    {
      //we didn't saturate each loop turn, so test here if it's necessary to saturate and use the end pos
      if (tRest < 0.0)
        pos = endpos;

      // check end-beg segment
      float t;

      bool col = CheckIntersection(beg, pos, cellXCurrent, cellZCurrent, t, h00, h10, h01, h11);
      if (col)
      {
        tDoneBeg += t*beg.Distance(pos);
				
        if (ret)
          *ret = tDoneBeg*dNorm + from;
        return tDoneBeg*invSize;
      }
    }

    if (tx <= tz)
    {
      if (ddx > 0)
      {
        h00 = h10;
        h01 = h11;

        h10 = ClippedDataXZ(xInt + 1, zInt);
        h11 = ClippedDataXZ(xInt + 1, zInt + 1);
      }
      else
      {
        h10 = h00;
        h11 = h01;

        h00 = ClippedDataXZ(xInt, zInt);
        h01 = ClippedDataXZ(xInt, zInt + 1);
      }
    }
    else
    {
      if (ddz > 0)
      {
        h00 = h01;
        h10 = h11;

        h01 = ClippedDataXZ(xInt, zInt + 1);
        h11 = ClippedDataXZ(xInt + 1, zInt + 1);
      }
      else
      {
        h01 = h00;
        h11 = h10;

        h00 = ClippedDataXZ(xInt, zInt);
        h10 = ClippedDataXZ(xInt + 1, zInt);
      }
    }
    MaxHeight(h00, h10, h01, h11, maxh);
  }
  if (ret)
    *ret = endpos;
  return maxDist0;
}

float Landscape::IntersectWithGroundOrSea(Vector3 *ret, bool &sea, Vector3Par from, Vector3Par dir, float minDist, float maxDist) const
{
  Assert(fabs(dir.SquareSize()-1.0f)<0.02f);
  
  if (maxDist>=FLT_MAX) maxDist = Glob.config.horizontZ;
  // first check intersection with sea level
  sea = false;
  float seaLevel = _seaLevelWave;
  // X: from+t*dir
  // X1 == seaLevel
  // X1: from1+t*dir1
  // seaLevel = from1+t*dir1
  // (seaLevel-from1)/dir1 = t
  float tSea = maxDist*2;
  float denom = dir[1];
  float nom = seaLevel-from[1];
  if (nom>0)
  {
    // already under sea
    if (ret) *ret = from+minDist*dir;
    sea = true;
    return minDist;
  }
  if (denom<-1e-6)
  {
    // check only when dir goes down
    tSea = nom/denom;

    if (tSea<minDist)
    {
      if (ret)
        *ret = from+minDist*dir;
      sea = true;
      return minDist;
    }
  }
  saturateMin(maxDist,tSea); // check only earlier intersection

  // calculate when 
  float t = IntersectWithGround(ret,from,dir,minDist,maxDist);
  if (t>tSea)
  {
    if (ret)
      *ret = from+dir*tSea;
    sea = true;
    return tSea;
  }
  return t;
}

float Landscape::IntersectWithGroundOrSea(Vector3 *ret, Vector3Par from, Vector3Par dir, float minDist, float maxDist) const
{
  bool sea;
  return IntersectWithGroundOrSea(ret, sea, from, dir, minDist, maxDist);
}

Vector3 Landscape::IntersectWithGround(Vector3Par from, Vector3Par dir, float minDist, float maxDist) const
{
  Vector3 ret;
  IntersectWithGround(&ret, from, dir, minDist, maxDist);
  return ret;
}

Vector3 Landscape::IntersectWithGroundOrSea(Vector3Par from, Vector3Par dir, float minDist, float maxDist) const
{
  Vector3 ret;
  IntersectWithGroundOrSea(&ret, from, dir, minDist, maxDist);
  return ret;
}

bool Landscape::CheckIntegrity() const
{
  #if 0 // _PROFILE && !_DEBUG
  // when integrity is broken, we expect the program to crash most often
  struct ObjectListCheckIntegrity
  {
    mutable bool error;
    ObjectListCheckIntegrity():error(false){}
    void operator () (const ObjectList &list, int x, int y, int w, int h) const
    {
      int n=list.Size();
      for (int i=0; i<n; i++)
      {
        Object *obj = list->Get(i);
        if (!obj) continue;
        if (obj->RefCounter()<=0 || obj->RefCounter()>1000000)
        {
          error = true;
          return;
        }
        LODShape *shape = obj->GetShape();
        if (!shape) continue;
        if (shape->BoundingSphere()<0 || shape->NLevels()<0 || shape->NLevels()>MAX_LOD_LEVELS)
        {
          error = true;
          return;
        }
      }
    }
  } check;
  struct DetectAll
  {
    bool operator () (int x, int y, int w, int h) const
    {
      return true;
    }
  };
  //_objects.ForEachItem(check);
  _objects.ForSomeItem(DetectAll(),check);
  if (check.error)
  {
    return false;
  }
  #endif
  return true;
}
