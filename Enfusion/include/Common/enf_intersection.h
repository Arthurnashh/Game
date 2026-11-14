//-----------------------------------------------------------------------------
// File: enf_intersection.h
//
// Desc: Geometric primitive intersections
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_INTERSECTION_H
#define ENF_INTERSECTION_H

#include "enf_config.h"
#include "common/enf_mathlib.h"
#include "common/enf_mathlib2d.h"

namespace enf
{
/** @addtogroup Math3D
@{*/

const uint CLIP_RIGHTB	= 0;
const uint CLIP_RIGHT	= BITF(CLIP_RIGHTB);	// cohen-sutherland clipping outcodes
const uint CLIP_LEFTB	= 1;
const uint CLIP_LEFT		= BITF(CLIP_LEFTB);
const uint CLIP_TOPB		= 2;
const uint CLIP_TOP		= BITF(CLIP_TOPB);
const uint CLIP_BOTTOMB	= 3;
const uint CLIP_BOTTOM	= BITF(CLIP_BOTTOMB);
const uint CLIP_FRONTB	= 4;
const uint CLIP_FRONT	= BITF(CLIP_FRONTB);
const uint CLIP_BACKB	= 5;
const uint CLIP_BACK		= BITF(CLIP_BACKB);

//-----------------------------------------------------------------------
/*!
Return Cohen-Sutherland code for point p and AAB box  defined by mins and
maxs
\param mins 
Minimum of AAB
\param maxs
Maximum of AAB
\param p
Tested point
*/
//-----------------------------------------------------------------------
ENF_INLINEFUNC uint Outcode(const Vector3& mins, const Vector3& maxs, const Vector3& p)
{
  uint outcode = 0;

  if( p.x > maxs.x ) {
    outcode |= CLIP_RIGHT;
  } else if( p.x < mins.x ) {
    outcode |= CLIP_LEFT;
  }
  if( p.y > maxs.y ) {
    outcode |= CLIP_TOP;
  } else if( p.y < mins.y ) {
    outcode |= CLIP_BOTTOM;
  }
  if( p.z > maxs.z ) {
    outcode |= CLIP_BACK;
  } else if( p.z < mins.z ) {
    outcode |= CLIP_FRONT;
  }
  //#endif
  return outcode;
}

ENF_INLINEFUNC uint Outcode(const Vector2& mins, const Vector2& maxs, const Vector2& p)
{
  uint outcode = 0;

  if( p.x > maxs.x ) {
    outcode |= CLIP_RIGHT;
  } else if( p.x < mins.x ) {
    outcode |= CLIP_LEFT;
  }
  if( p.y > maxs.y ) {
    outcode |= CLIP_TOP;
  } else if( p.y < mins.y ) {
    outcode |= CLIP_BOTTOM;
  }

  return outcode;
}


//-----------------------------------------------------------------------
/*!
Return Cohen-Sutherland code for direction -> from where the vector points
\param dir
Tested direction
*/
//-----------------------------------------------------------------------
ENF_INLINEFUNC uint OutcodeDir(const Vector3& dir)
{
	uint outcode = 0;

	if (dir.x != 0.0)
	{
		if (dir.x > 0.0)
			outcode = CLIP_LEFT;
		else
			outcode = CLIP_RIGHT;
	}

	if (dir.y != 0.0)
	{
		if (dir.y > 0.0)
			outcode |= CLIP_BOTTOM;
		else
			outcode |= CLIP_TOP;
	}

	if (dir.z != 0.0)
	{
		if (dir.z > 0.0)
			outcode |= CLIP_FRONT;
		else
			outcode |= CLIP_BACK;
	}

	return outcode;
}

ENF_INLINEFUNC uint OutcodeDir(const Vector2& dir)
{
	uint outcode = 0;

	if (dir.x != 0.0)
	{
		if (dir.x > 0.0)
			outcode = CLIP_LEFT;
		else
			outcode = CLIP_RIGHT;
	}

	if (dir.y != 0.0)
	{
		if (dir.y > 0.0)
			outcode |= CLIP_BOTTOM;
		else
			outcode |= CLIP_TOP;
	}

	return outcode;
}
//-----------------------------------------------------------------------
class Segment
{
public:
	//! Constructor
	ENF_INLINE Segment() {}
	//! Constructor
	ENF_INLINE Segment(const Vector3& _p0, const Vector3& _p1) : p0(_p0), p1(_p1){}
	//! Copy constructor
	ENF_INLINE Segment(const Segment& seg) : p0(seg.p0), p1(seg.p1) {}
	//! Destructor
	ENF_INLINE ~Segment() {}

	ENF_INLINE const Vector3&	GetOrigin() const
	{
		return p0;
	}
	ENF_INLINE Vector3 ComputeDirection() const
	{
		return p1 - p0;
	}

	ENF_INLINE void SetOriginDirection(const Vector3& origin, const Vector3& direction)
	{
		p0 = origin;
		p1 = origin + direction;
	}

	/**
	 *	Computes a point on the segment
	 *	\param		pt	[out] point on segment
	 *	\param		t	[in] point's parameter [t=0 => pt = p0, t=1 => pt = p1]
	 */
	ENF_INLINE void ComputePoint(Vector3& pt, float t) const
	{
		pt = p0 + (p1 - p0) * t;
	}

	float	SquareDistance(const Vector3& point, float* t = NULL) const
	{
		Vector3 ap = point - p0;
		Vector3 ab = p1 - p0;
		float ab2 = ab.DotProduct( ab );
		float ap_ab = ap.DotProduct( ab );
		float tt = ap_ab / ab2;

		// clamp to segment
		if( tt < 0 ) tt = 0;
		else if( tt > 1.f ) tt = 1.f;

		if( t ) *t = tt;
		Vector3 cp = (p0 + (ab * tt));
		return cp.Distance2( point );
	}
	ENF_INLINE float Distance(const Vector3& point, float* t = NULL) const
	{
		return sqrtf(SquareDistance(point, t));
	}

	Vector3	p0;		//!< Start of segment
	Vector3	p1;		//!< End of segment
};


/*!
Tests whether is sphere intersecting axis aligned box.
\param origin
Origin of sphere
\param radiusSq
Squared radius of sphere
\param mins
Minimums of bound box
\param maxs
Maximums of bound box
\return
True when intersects
*/
PUBLIC_API bool IntersectionSphereAABB(const Vector3& origin, double radiusSq, const Vector3& mins, const Vector3& maxs);

/*!
Tests whether is axis aligned box intersecting moving axis aligned box.
\param A
First AAB
\param B
Second AAB
\param v
Relative velocity
\param u0
First time of intersection
\param u1
Last time of intersection
\return
True when intersects
*/
PUBLIC_API bool IntersectionAABBSweep(const AAB& A, const AAB& B, const Vector3& v, float& u0, float& u1);

/*!
Tests whether is axis aligned box intersecting moving axis aligned box.
\param A
First AAB
\param B
Second AAB
\param v
Relative velocity
\return
True when intersects
*/
PUBLIC_API bool IntersectionAABBSweep(const AAB& A, const AAB& B, const Vector3& v);

/*!
Tests whether is sphere intersecting cone
\param origin Origin of sphere
\param radius Radius of sphere
\param conepos Position of top of cone
\param axis Orientation of cone in direction from top to bottom
\param angle Angle of cone in radians
\return True when intersects
*/
PUBLIC_API bool IntersectionSphereCone(const Vector3& origin, float radius, const Vector3& conepos, const Vector3& axis, float angle);

PUBLIC_API bool IntersectionAABBOBB(const AAB& _aabb, const Matrix33& _mat, const AAB& _obb);
PUBLIC_API bool IntersectionLineAABB(const Vector3& start, const Vector3& end, const Vector3& mins, const Vector3& maxs, float *pfrac);
PUBLIC_API bool _IntersectionLineAABB(const Vector3& start, const Vector3& end, const Vector3& mins, const Vector3& maxs, uint outcode1);
PUBLIC_API bool _IntersectionLineAABB(const Vector2& start, const Vector2& end, const Vector2& mins, const Vector2& maxs, uint outcode1);
ENF_INLINE bool IntersectionLineAABB(const Vector3& start, const Vector3& end, const Vector3& mins, const Vector3& maxs)
{
	uint outcode1 = Outcode(mins, maxs, start );
	if( outcode1 == 0 )
		return true; // point inside bounding box

	uint outcode2 = Outcode(mins, maxs, end );
	
	//we need to find point of collision :-(
	if( outcode2 == 0 )
		return true; // point inside bounding box

	if( (outcode1 & outcode2) != 0 )
		return false; // both points on same side of box
	return _IntersectionLineAABB(start, end, mins, maxs, outcode1);
}

ENF_INLINE bool IntersectionLineAABB(const Vector2& start, const Vector2& end, const Vector2& mins, const Vector2& maxs)
{
	uint outcode1 = Outcode(mins, maxs, start );
	if( outcode1 == 0 )
		return true; // point inside bounding box

	uint outcode2 = Outcode(mins, maxs, end );

	//we need to find point of collision :-(
	if( outcode2 == 0 )
		return true; // point inside bounding box

	if( (outcode1 & outcode2) != 0 )
		return false; // both points on same side of box
	return _IntersectionLineAABB(start, end, mins, maxs, outcode1);
}
PUBLIC_API bool IntersectionSphereRay(const Vector3& raybase, const Vector3& raycos, const Vector3& center, float radius, float *rin);
PUBLIC_API bool IntersectionUnitSphereRay(const Vector3& raybase, const Vector3& raycos, float *rin);
PUBLIC_API bool IntersectionLineTriangle(const Vector3& start, const Vector3& dir, const Vector3& v0, const Vector3& v1, const Vector3& v2, float *pu, float *pv);
PUBLIC_API bool IntersectionLineTriangleEdges(const Vector3& start, const Vector3& dir, const Vector3& p0, const Vector3& edge1, const Vector3& edge2, float *pu, float *pv);
PUBLIC_API bool IntersectionSphereTriangle(const Vector3& mCenter, float mRadius, const Vector3& vert0, const Vector3& vert1, const Vector3& vert2);
PUBLIC_API bool IntersectionAABBTriangle(const AAB& box, const Vector3& V0, const Vector3& V1, const Vector3& V2);
PUBLIC_API bool IntersectionOBBTriangle(const AAB& obb, const Matrix43& orientation, const Vector3& V0, const Vector3& V1, const Vector3& V2);
//!returns 1 when in front of plane, 2 when behind plane, 3 when intersected. intersection parametr is valid only if return == 3!
PUBLIC_API uint IntersectionLinePlane(const Vector3& start, const Vector3& end, const MPlane& plane, Vector3& intersection);

PUBLIC_API float SegmentTriangleSqrDist(const Segment& segment, const Vector3& p0, const Vector3& p1, const Vector3& p2);
PUBLIC_API float SegmentSegmentSqrDist( const Segment& segment1, const Segment& segment2 );

ENF_INLINE bool IntersectionLSSTriangle(const Segment& seg, float sqrRadius, const Vector3& vert0, const Vector3& vert1, const Vector3& vert2)
{
	float s2 = SegmentTriangleSqrDist(seg, vert0, vert1, vert2);
	return (s2 < sqrRadius);
}

bool PUBLIC_API IntersectionOBBOBB(const AAB& a, const Matrix43& frameA, const AAB& b, const Matrix43& frameB);
bool IntersectionOBBOBB(const Vector3& a, const Matrix43& Ax, const Vector3& b, const Matrix43& Bx);

//@}
}
#endif //ENF_INTERSECTION_H