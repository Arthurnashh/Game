//-----------------------------------------------------------------------------
// File: baseworld/enf_baseworldtrace.h
//
// Desc: Collision classes
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_BASEWORLDTRACE_H
#define ENF_BASEWORLDTRACE_H

#include "enf_config.h"
//#include "common/enf_types.h"
#include "common/enf_mathlib.h"
#include "resource/enf_surfaceparams.h"
//#include "script/enf_scriptvariable.h"


class Object;

namespace enf
{
/** @addtogroup World
@{*/
class Material;
//class Entity;
class Terrain;

enum TraceFlags
{
//! As precise as possible tracing against skeletal objects.
	TRACE_BONES				= BITF(1),

/*!
Used by point-to-point visibility tests. Rough
visibility test is performed first.
*/
	TRACE_VISTEST			= BITF(3),

//! Tracing against entities. Otherwise just world geometry is tested.
	TRACE_ENTS				= BITF(4),

	//! Tracing by just a line. Otherwise sweep-box is used.
	TRACE_LINE				= BITF(5),

//! Tracing against world geometry.
	TRACE_WORLD				= BITF(6),

//!Traces only against physics controllers
	TRACE_ONLY_PHYSICS	= BITF(7),

//! Trace against water surface. Otherwise tracing passes-thru water geometry.
	TRACE_WATER				= BITF(8),

//! Trace passes entities with TFL_TRANSLUCENT flag set.
	TRACE_PASSTRANSLUCENT = BITF(9),

//! Tracing against ragdolls. Otherwise are ragdolls passed.
	TRACE_RAGDOLLS			= BITF(10),
/*!
No tracing is performed. Used together with TRACE_VISTEST
to do just rough visibility test.
*/
	TRACE_NOTRACE			= BITF(11),

	TRACE_TRANSPARENT_OCCLUDERS = BITF(12),

//! Most common flag set.
	TRACE_DEFAULT			= (TRACE_ENTS|TRACE_WORLD)
};


//-----------------------------------------------------------------------
//Volume content flags. Used by convex brushes and scene-tree leafs.

enum ContentFlags
{
	//!Solid, opaque, colliding
	C_SOLID			= 0x01,
	//!Water is inside
	C_WATER			= 0x20,
	//!Used for detail and not structural geometry.
	C_DETAIL			= 0x8000000,
	//!Translucent. Used for glass etc.
	C_TRANSLUCENT	= 0x20000000,
	//!Special bit for tree leafs, which continue by another tree
	C_TREE			= 0x40000000,
	//!Special bit for tree leafs
	C_LEAF			= 0x80000000
};

struct BaseTrace
{
/** \name Input parameters for trace
*/
//@{
	const Object*	Exclude;			///<Exclude this Entity
	uint				Mask;				///<Mask of enum EntityFlags
	AAB				BBox;				///<Bounds of traced point
	uint				Flags;			///<See enf::TraceFlags
	uint				InteractionLayer; ///<Filter physics geometries
	Vector3			Start;			///<Start position from where we traced
	Vector3			Dir;				///<Direction of trace
	float				Len;				///<Length of trace
//@}

/** \name Results of collision
*/
//@{
	Vector3			EndPos;			///<End position where we collided
	Object*			ClipEntity;		///<Set to Entity we collided or NULL
	float				Fraction;		///<Set to fraction of traveled distance. It's 0...1
	uint				Contents;		///<Set to contents flags of collided geometry
	SurfaceParms	SurfParms;		///<Set to surface parameters of collided geometry
	uint				Bone;				///<Set to Ragdoll bone we collided, or INDEX_NOT_FOUND
	MPlane			Plane;			///<Set to plane of collided face
	Material*		MaterialPtr;	///<Set to Material we collided or NULL
	bool				StartSolid;		///<Set to true, if start position is stuck into something solid
	bool				AllSolid;		///<Set to true, if both start and end is stuck into something solid
//@}

	uint				TraceCount;		///<Internal counter for avoiding twice test

	/*!
	Default constructor, it clears everything
	*/
	BaseTrace():
		Exclude(NULL),
		ClipEntity(NULL),
		Mask(0),
		Contents(0),
		Fraction(0.0f),
		Flags(0),
		InteractionLayer(0xffffffff),
		Bone(0),
		Len(0.0f),
		MaterialPtr(NULL),
		StartSolid(false),
		AllSolid(false),
		TraceCount(0)
	{
		BBox.Zero();
		Plane.Zero();
		Start.Zero();
		EndPos.Zero();
		Dir.Zero();
	}

	/*!
	Initializes structure. Usable for BaseWorld::TraceMove call.
	\param exclude
	Exclude this Entity
	\param flags
	See enf::TraceFlags
	\param mask
	When set, it accepts only these entities, which has set ALL these flags.
	*/
	void Init(Object* exclude, uint flags, uint mask, uint interactionLayer = 0xffffffff)
	{
		Exclude = exclude;
		ClipEntity = NULL;
		Mask = mask;
		Contents = 0;
//		SurfParms = 0;
		Fraction = 1.0f;
		Flags = flags;
		InteractionLayer = interactionLayer;
		Bone = 0;
		Plane.Zero();
		BBox.Zero();
		MaterialPtr = NULL;
		StartSolid = false;
		AllSolid = false;
	}
	
	void Init(Object* exclude, const Vector3& start, const Vector3& end, uint flags, uint mask, uint interactionLayer = 0xffffffff)
	{
		Exclude = exclude;
		ClipEntity = NULL;
		Mask = mask;
		Contents = 0;
//		SurfParms = 0;
		Fraction = 1.0f;
		Flags = flags;
		InteractionLayer = interactionLayer;
		Bone = 0;
		Start = start;
		EndPos = end;
		Dir = end - start;
		Len = Dir.Normalize();
		Plane.Zero();
		BBox.Zero();
		MaterialPtr = NULL;
		StartSolid = false;
		AllSolid = false;
	}

	/*!
	Utility function, sets bound box of trace from entity and her children. Do not account for rotation!
	\param ent Entity and her children defining bounds
	*/
	void SetBBox(const Object* ent);

	ebool Intersects(const AAB& bbox) const;
	ebool Intersects(const AAB& bbox, const Vector3& end) const;
};

struct EntityTraceFilter
{
	virtual ~EntityTraceFilter() {}
	/*!
	Callback for filtering certain entities
	\param ent
	Entity to be tested
	\return
	When you return false, Entity will not be tested
	*/
	virtual ebool FilterEntity(Object* ent)
	{
		return true;
	}
};

struct MTrace: public BaseTrace, public EntityTraceFilter
{
};

class PUBLIC_API TraceContact//: public Instance
{
public:
	float				Fraction;
	uint				Content;
	SurfaceParms	SurfParms;
	uint				MaterialFlags;
	uint				Triangle;
	uint				SurfaceID;
	const char*		Material;
	const char*		OriginalMaterial;
	float*			pPlane;
	float*			pPoint;
	Vector4			Plane;
	Vector3			Point;

	TraceContact();
	~TraceContact();
};
//@}
}
#endif //ENF_BASEWORLDTRACE_H