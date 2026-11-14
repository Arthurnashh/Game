#ifdef _MSC_VER
#pragma once
#endif

#ifndef __OBJECTHIERARCHY_HPP__
#define __OBJECTHIERARCHY_HPP__

#include "visual.hpp"
#include "Shape/plane.hpp"
#include "cast.hpp"
#include <Es/Containers/sortedArray.hpp>
#include "Common/enf_inttypes.h"

//------------------------------------
// SimulationImportance
//------------------------------------
//! importance determines simulation precision
#ifndef DECL_ENUM_SIMULATION_IMPORTANCE
#define DECL_ENUM_SIMULATION_IMPORTANCE
DECL_ENUM(SimulationImportance)
#endif
DEFINE_ENUM_BEG(SimulationImportance)
SimulateVisibleNear,SimulateVisibleFar, //! it is visible
SimulateInvisibleNear,SimulateInvisibleFar, //! it is inivisible
SimulateDefault, //! no specific value
SimulationImportanceMask=0x1F,
SimulatePosValid=0x20, //! special value, used only for network transfer
SimulatePosCut=0x40 //! special value, used only for network transfer
DEFINE_ENUM_END(SimulationImportance)


//------------------------------------
// CollisionContexes
//------------------------------------
//! Physics layers for interaction
enum EPhysicsLayers
{
	EPL_WORLD = BITF(0),
	EPL_NOCOLLISION = BITF(1),

	EPL_DEFAULT = BITF(2),
	EPL_BUILDING = BITF(3),
	EPL_CHARACTER = BITF(4),
	EPL_VEHICLE = BITF(5),

	EPL_CARRIER = BITF(6),
	EPL_CARRIER_SPHERE = BITF(7),

	EPL_GRENADE = BITF(9),
	EPL_DEBRIS = BITF(10),
	EPL_SHELL = BITF(11),

	EPL_DOOR = BITF(12),
	EPL_RAGDOLL = BITF(13),

	EPL_WATERLAYER = BITF(14),
	EPL_TERRAIN = BITF(15),

	EPL_VEHICLE_NOTERRAIN = BITF(16),
	EPL_WORLDBOUNDS = BITF(17),

	EPL_FENCE = BITF(18),
	EPL_AI = BITF(19),
	EPL_SHIP_HULL = BITF(20),

	EPL_AMPHIBIOUS_VEHICLE = BITF(21),
};

class Object;
class IntersectionFilter
{
public:
  /// check if object collision should be tested
  virtual bool operator()(const Object *) const = 0; 
};

#define OBJ_INTERSECT_ENUM(type,prefix,XX) \
  XX(type, prefix, Fire) /*normal fire*/ \
  XX(type, prefix, View) /*view geometry*/\
  XX(type, prefix, Geom) /*collision geometry */\
  XX(type, prefix, IFire)/*indirect fire damage, uses same geometry as ObjIntersectFire, but different calculation*/ \
  XX(type, prefix, None) /*no test*/

DECLARE_ENUM(ObjIntersect, ObjIntersect, OBJ_INTERSECT_ENUM)

/// context for Landscape::CheckObjectCollision
struct CheckObjectCollisionContext;

class ObjectVisualState;

/// context for Landscape::ObjectCollision objectVsObjects
struct ObjectCollisionContext
{
  const Object * with;
  const ObjectVisualState &withPos; 
  bool person;
  bool onlyVehicles;

  ObjectCollisionContext(const ObjectVisualState &vs) : person(false), onlyVehicles(false), withPos(vs) {};
};

/// context for Landscape::ObjectCollisionFirstTouch
struct ObjectCollisionFirstTouchContext
{
  const Object *with;
  const ObjectVisualState & withPos;
  Vector3 deltaPos; // in with local pos !!! 
  bool onlyVehicles;
  const Object *except;

  ObjectCollisionFirstTouchContext(const ObjectVisualState &vs) : withPos(vs), except(NULL) {};
};

/// context for Landscape::GroundCollisionWithObject 
struct GroundCollisionWithObjectContext
{
  const Object *with;
  Vector3 centerPos;
  Buffer<Vector3> begEnd;
  float wRad;
  const Shape *withShape;
  Array<int> *solidWaterMap;
  const ObjectVisualState & withTrans; 
  float above; 
  int level; 

  GroundCollisionWithObjectContext(const ObjectVisualState &vs) : withTrans(vs) {};
};

#endif





