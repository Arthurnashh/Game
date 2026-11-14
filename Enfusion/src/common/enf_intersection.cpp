//-----------------------------------------------------------------------------
// File: enf_intersection.cpp
//
// Desc: Various geometry intersection tests
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "common/enf_intersection.h"
#include "baseWorld/enf_shapes.h"

namespace enf
{

//-----------------------------------------------------------------------------
PUBLIC_API uint IntersectionLinePlane(const Vector3& start, const Vector3& end, const MPlane& plane, Vector3& intersection)
{
	float d1, d2;

	uint res = plane.IntersectLine(start, end, d1, d2);
	if(res != 3)
		return res;

	intersection = start.Lerp(end, d1 / (d1 - d2));
	return 3;
}


//-----------------------------------------------------------------------------
/*
bool IntersectionConePlane(const Vector3& origin, const Vector3& direction, float length, const Vector4& plane)
{
	float planedist = (origin.x * plane.x + origin.y * plane.y + origin.z * plane.z) - plane.w;


}
*/

//-----------------------------------------------------------------------------
bool IntersectionAABBSweep(const AAB& A, const AAB& B, const Vector3& v, float& u0, float& u1)
{
	//check if they were overlapping 
	// on the previous frame
	if( A.Intersects(B) )
	{
		u0 = u1 = 0;
		return true;
	}

	//first times of overlap along each axis
	u0 = 0.0f;

	//last times of overlap along each axis
	u1 = 1.0f;

	const Vector3& Amin = A.GetMins();
	const Vector3& Amax = A.GetMaxs();
	const Vector3& Bmin = B.GetMins();
	const Vector3& Bmax = B.GetMaxs();

	//find the possible first and last times
	//of overlap along each axis
	for(uint i = 0 ; i < 3; i++)
	{
		if( Amax[i] < Bmin[i] && v[i] < 0)
		{
			u0 = enf_max(u0, (Amax[i] - Bmin[i]) / v[i]);
		}
		else if( Bmax[i] < Amin[i] && v[i] > 0 )
		{
			u0 = enf_max(u0, (Amin[i] - Bmax[i]) / v[i]);
		}

		if( Bmax[i] > Amin[i] && v[i] < 0 )
		{ 
			u1 = enf_min(u1, (Amin[i] - Bmax[i]) / v[i]); 
		}
		else if( Amax[i] > Bmin[i] && v[i] > 0 )
		{
			u1 = enf_min(u1, (Amax[i] - Bmin[i]) / v[i]);
		}
	}

	//they could have only collided if
	//the first time of overlap occurred
	//before the last time of overlap
	return u0 <= u1;
}

//-----------------------------------------------------------------------------
bool IntersectionAABBSweep(const AAB& A, const AAB& B, const Vector3& v)
{
	//check if they were overlapping 
	// on the previous frame
	if( A.Intersects(B) )
	{
		return true;
	}

	//first times of overlap along each axis
	float u0 = 0.0f;

	//last times of overlap along each axis
	float u1 = 1.0f;

	const Vector3& Amin = A.GetMins();
	const Vector3& Amax = A.GetMaxs();
	const Vector3& Bmin = B.GetMins();
	const Vector3& Bmax = B.GetMaxs();

	//find the possible first and last times
	//of overlap along each axis
	for(uint i = 0 ; i < 3; i++)
	{
		if( Amax[i] < Bmin[i] && v[i] < 0)
		{
			u0 = enf_max(u0, (Amax[i] - Bmin[i]) / v[i]);
		}
		else if( Bmax[i] < Amin[i] && v[i] > 0 )
		{
			u0 = enf_max(u0, (Amin[i] - Bmax[i]) / v[i]);
		}

		if( Bmax[i] > Amin[i] && v[i] < 0 )
		{ 
			u1 = enf_min(u1, (Amin[i] - Bmax[i]) / v[i]); 
		}
		else if( Amax[i] > Bmin[i] && v[i] > 0 )
		{
			u1 = enf_min(u1, (Amax[i] - Bmin[i]) / v[i]);
		}
	}

	//they could have only collided if
	//the first time of overlap occurred
	//before the last time of overlap
	return u0 <= u1;
}

//-----------------------------------------------------------------------------
bool IntersectionAABBOBB(const AAB& _aabb, const Matrix33& _mat, const AAB& _obb)
{
	float t, s;
	int r;
	float Bf[3][3];
	float reps = (float) 1e-6;

	Vector3 a = _aabb.GetCorner();
	Vector3 b = _obb.GetCorner();
	const Vector3 T = _obb.GetCenter() - _aabb.GetCenter();

//	float B[3][3];

//	const Matrix3x3 &m = _obb.matrix;
//	B[0][0] = m._11;	B[0][1] = m._21;	B[0][2] = m._31;
//	B[1][0] = m._12;	B[1][1] = m._22;	B[1][2] = m._32;
//	B[2][0] = m._13;	B[2][1] = m._23;	B[2][2] = m._33;

	typedef float _mat3x3[3][3];
	_mat3x3 B;
	memcpy(B, &_mat, sizeof(_mat3x3));
  
	// Bf = fabs(B)
	Bf[0][0] = fabsf(B[0][0]);  Bf[0][0] += reps;
	Bf[0][1] = fabsf(B[0][1]);  Bf[0][1] += reps;
	Bf[0][2] = fabsf(B[0][2]);  Bf[0][2] += reps;
	Bf[1][0] = fabsf(B[1][0]);  Bf[1][0] += reps;
	Bf[1][1] = fabsf(B[1][1]);  Bf[1][1] += reps;
	Bf[1][2] = fabsf(B[1][2]);  Bf[1][2] += reps;
	Bf[2][0] = fabsf(B[2][0]);  Bf[2][0] += reps;
	Bf[2][1] = fabsf(B[2][1]);  Bf[2][1] += reps;
	Bf[2][2] = fabsf(B[2][2]);  Bf[2][2] += reps;

	// if any of these tests are one-sided, then the polyhedra are disjoint
	r = 1;

	// A1 x A2 = A0
	t = fabsf(T[0]);
	r &= (t <= (a[0] + b[0] * Bf[0][0] + b[1] * Bf[0][1] + b[2] * Bf[0][2]));
	if (!r) return false;
  
	// B1 x B2 = B0
	s = T[0]*B[0][0] + T[1]*B[1][0] + T[2]*B[2][0];
	t = fabsf(s);
	r &= ( t <= (b[0] + a[0] * Bf[0][0] + a[1] * Bf[1][0] + a[2] * Bf[2][0]));
	if (!r) return false;
    
	// A2 x A0 = A1
	t = fabsf(T[1]);
	r &= ( t <=  (a[1] + b[0] * Bf[1][0] + b[1] * Bf[1][1] + b[2] * Bf[1][2]));
	if (!r) return false;

	// A0 x A1 = A2
	t = fabsf(T[2]);
	r &= ( t <= (a[2] + b[0] * Bf[2][0] + b[1] * Bf[2][1] + b[2] * Bf[2][2]));
	if (!r) return false;

	// B2 x B0 = B1
	s = T[0]*B[0][1] + T[1]*B[1][1] + T[2]*B[2][1];
	t = fabsf(s);
	r &= ( t <= (b[1] + a[0] * Bf[0][1] + a[1] * Bf[1][1] + a[2] * Bf[2][1]));
	if (!r) return false;

	// B0 x B1 = B2
	s = T[0]*B[0][2] + T[1]*B[1][2] + T[2]*B[2][2];
	t = fabsf(s);
	r &= ( t <= (b[2] + a[0] * Bf[0][2] + a[1] * Bf[1][2] + a[2] * Bf[2][2]));
	if (!r) return false;

	// A0 x B0
	s = T[2] * B[1][0] - T[1] * B[2][0];
	t = fabsf(s);
  
	r &= ( t <= (a[1] * Bf[2][0] + a[2] * Bf[1][0] + b[1] * Bf[0][2] + b[2] * Bf[0][1]));
	if (!r) return false;
  
	// A0 x B1
	s = T[2] * B[1][1] - T[1] * B[2][1];
	t = fabsf(s);
	r &= ( t <= (a[1] * Bf[2][1] + a[2] * Bf[1][1] + b[0] * Bf[0][2] + b[2] * Bf[0][0]));
	if (!r) return false;

	// A0 x B2
	s = T[2] * B[1][2] - T[1] * B[2][2];
	t = fabsf(s);
	r &= ( t <= (a[1] * Bf[2][2] + a[2] * Bf[1][2] + b[0] * Bf[0][1] + b[1] * Bf[0][0]));
	if (!r) return false;

	// A1 x B0
	s = T[0] * B[2][0] - T[2] * B[0][0];
	t = fabsf(s);
	r &= ( t <= (a[0] * Bf[2][0] + a[2] * Bf[0][0] + b[1] * Bf[1][2] + b[2] * Bf[1][1]));
	if (!r) return false;

	// A1 x B1
	s = T[0] * B[2][1] - T[2] * B[0][1];
	t = fabsf(s);
	r &= ( t <= (a[0] * Bf[2][1] + a[2] * Bf[0][1] +  b[0] * Bf[1][2] + b[2] * Bf[1][0]));
	if (!r) return false;

	// A1 x B2
	s = T[0] * B[2][2] - T[2] * B[0][2];
	t = fabsf(s);
	r &= (t <= (a[0] * Bf[2][2] + a[2] * Bf[0][2] +  b[0] * Bf[1][1] + b[1] * Bf[1][0]));
	if (!r) return false;

	// A2 x B0
	s = T[1] * B[0][0] - T[0] * B[1][0];
	t = fabsf(s);
	r &= (t <= (a[0] * Bf[1][0] + a[1] * Bf[0][0] +  b[1] * Bf[2][2] + b[2] * Bf[2][1]));
	if (!r) return false;

	// A2 x B1
	s = T[1] * B[0][1] - T[0] * B[1][1];
	t = fabsf(s);
	r &= ( t <= (a[0] * Bf[1][1] + a[1] * Bf[0][1] + b[0] * Bf[2][2] + b[2] * Bf[2][0]));
	if (!r) return false;

	// A2 x B2
	s = T[1] * B[0][2] - T[0] * B[1][2];
	t = fabsf(s);
	r &= ( t <= (a[0] * Bf[1][2] + a[1] * Bf[0][2] +  b[0] * Bf[2][1] + b[1] * Bf[2][0]));
	if (!r) return false;

	return true;
}

//-----------------------------------------------------------------------------
bool IntersectionLineAABB(const Vector3& start, const Vector3& end, const Vector3& mins, const Vector3& maxs, float* pfrac)
{
	*pfrac = 0;

	uint outcode1 = Outcode(mins, maxs, start );
	if( outcode1 == 0 )
		return true; // point inside bounding box

	uint outcode2 = Outcode(mins, maxs, end );
	
	//we need to find point of collision :-(
	//if( outcode2 == 0)
	//	return true; // point inside bounding box

	if( (outcode1 & outcode2) != 0 )
		return false; // both points on same side of box

	float frac;

	// check intersections
	if( outcode1 & (CLIP_RIGHT | CLIP_LEFT) )
	{
		float ix = (outcode1 & CLIP_RIGHT) ? maxs.x : mins.x;

		float x1 = end.x - start.x;
		float x2 = ix - start.x;
		frac = x2 / x1;

		float iy = start.y + (end.y - start.y) * frac;
		float iz = start.z + (end.z - start.z) * frac;
		if( iy <= maxs.y && iy >= mins.y && iz <= maxs.z && iz >= mins.z)
		{
			*pfrac = frac;
			return true;
		}
	}

	if( outcode1 & (CLIP_TOP | CLIP_BOTTOM) )
	{
		float iy = (outcode1 & CLIP_TOP) ? maxs.y : mins.y;

		float y1 = end.y - start.y;
		float y2 = iy - start.y;
		frac = y2 / y1;

		float ix = start.x + (end.x - start.x) * frac;
		float iz = start.z + (end.z - start.z) * frac;
		if( ix <= maxs.x && ix >= mins.x &&  iz <= maxs.z && iz >= mins.z)
		{
			*pfrac = frac;
			return true;
		}
	}

	if( outcode1 & (CLIP_FRONT | CLIP_BACK) )
	{
		float iz = (outcode1 & CLIP_BACK) ? maxs.z : mins.z;

		float z1 = end.z - start.z;
		float z2 = iz - start.z;
		frac = z2 / z1;

		float ix = start.x + (end.x - start.x) * frac;
		float iy = start.y + (end.y - start.y) * frac;
		if( ix <= maxs.x && ix >= mins.x &&  iy <= maxs.y && iy >= mins.y )
		{
			*pfrac = frac;
			return true;
		}
	}

	// nothing found
	return false;
}

//-----------------------------------------------------------------------------
bool _IntersectionLineAABB(const Vector3& start, const Vector3& end, const Vector3& mins, const Vector3& maxs, uint outcode1)
{
	float ix, iy, iz;
	float frac;

	// check intersections
	if( outcode1 & (CLIP_RIGHT | CLIP_LEFT) )
	{
		ix = (outcode1 & CLIP_RIGHT) ? maxs.x : mins.x;

		float x1 = end.x - start.x;
		float x2 = ix - start.x;
		frac = x2 / x1;

		iy = start.y + (end.y - start.y) * frac;
		iz = start.z + (end.z - start.z) * frac;
		if( iy <= maxs.y && iy >= mins.y && iz <= maxs.z && iz >= mins.z)
		{
			return true;
		}
	}

	if( outcode1 & (CLIP_TOP | CLIP_BOTTOM) )
	{
		iy = (outcode1 & CLIP_TOP) ? maxs.y : mins.y;

		float y1 = end.y - start.y;
		float y2 = iy - start.y;
		frac = y2 / y1;

		ix = start.x + (end.x - start.x) * frac;
		iz = start.z + (end.z - start.z) * frac;
		if( ix <= maxs.x && ix >= mins.x &&  iz <= maxs.z && iz >= mins.z)
		{
			return true;
		}
	}

	if( outcode1 & (CLIP_FRONT | CLIP_BACK) )
	{
		iz = (outcode1 & CLIP_BACK) ? maxs.z : mins.z;

		float z1 = end.z - start.z;
		float z2 = iz - start.z;
		frac = z2 / z1;

		ix = start.x + (end.x - start.x) * frac;
		iy = start.y + (end.y - start.y) * frac;
		if( ix <= maxs.x && ix >= mins.x &&  iy <= maxs.y && iy >= mins.y )
		{
			return true;
		}
	}

	// nothing found
	return false;
}

//-----------------------------------------------------------------------------
bool _IntersectionLineAABB(const Vector2& start, const Vector2& end, const Vector2& mins, const Vector2& maxs, uint outcode1)
{
	float ix, iy;
	float frac;

	// check intersections
	if( outcode1 & (CLIP_RIGHT | CLIP_LEFT) )
	{
		ix = (outcode1 & CLIP_RIGHT) ? maxs.x : mins.x;

		float x1 = end.x - start.x;
		float x2 = ix - start.x;
		frac = x2 / x1;

		iy = start.y + (end.y - start.y) * frac;
		if( iy <= maxs.y && iy >= mins.y)
		{
			return true;
		}
	}

	if( outcode1 & (CLIP_TOP | CLIP_BOTTOM) )
	{
		iy = (outcode1 & CLIP_TOP) ? maxs.y : mins.y;

		float y1 = end.y - start.y;
		float y2 = iy - start.y;
		frac = y2 / y1;

		ix = start.x + (end.x - start.x) * frac;
		if( ix <= maxs.x && ix >= mins.x)
		{
			return true;
		}
	}

	// nothing found
	return false;
}

//----------------------------------------------------------------------
bool IntersectionLineTriangle(const Vector3& start, const Vector3& dir, const Vector3& v0, const Vector3& v1, const Vector3& v2, float *pu, float *pv)
{
	Vector3 edge1 = v1 - v0;
	Vector3 edge2 = v2 - v0;

	// begin calculating determinant - also used to calculate U parameter
	Vector3 pvec = dir.CrossProduct(edge2);

	// if determinant is near zero, ray lies in plane of triangle
	double det = edge1.DotProduct(pvec);

//	if (det > -DIST_EPSILON && det < DIST_EPSILON)
	if (det > -0.001 && det < 0.001)
		return false;

	double inv_det = 1.0 / det;

	// calculate distance from vert0 to ray origin
	Vector3 tvec = start - v0;

	// calculate U parameter and test bounds
	double u = tvec.DotProduct(pvec) * inv_det;
	if (u < 0.0 || u > 1.0)
		return false;

	// prepare to test V parameter
	Vector3 qvec = tvec.CrossProduct(edge1);

	// calculate V parameter and test bounds
	double v = dir.DotProduct(qvec) * inv_det;
	if (v < 0.0 || u + v > 1.0)
		return false;

	*pu = (float)u;
	*pv = (float)v;
	return true;
}

//----------------------------------------------------------------------
bool IntersectionLineTriangleEdges(const Vector3& start, const Vector3& dir, const Vector3& v0, const Vector3& edge1, const Vector3& edge2, float *pu, float *pv)
{
	// begin calculating determinant - also used to calculate U parameter
	Vector3 pvec = dir.CrossProduct(edge2);

	// if determinant is near zero, ray lies in plane of triangle
	double det = edge1.DotProduct(pvec);

//	if (det > -DIST_EPSILON && det < DIST_EPSILON)
	if (det > -0.001 && det < 0.001)
		return false;

	double inv_det = 1.0 / det;

	// calculate distance from vert0 to ray origin
	Vector3 tvec = start - v0;

	// calculate U parameter and test bounds
	double u = tvec.DotProduct(pvec) * inv_det;
	if (u < 0.0 || u > 1.0)
		return false;

	// prepare to test V parameter
	Vector3 qvec = tvec.CrossProduct(edge1);

	// calculate V parameter and test bounds
	double v = dir.DotProduct(qvec) * inv_det;
	if (v < 0.0 || u + v > 1.0)
		return false;

	*pu = (float)u;
	*pv = (float)v;
	return true;
}

//-----------------------------------------------------------------------
bool IntersectionSphereRay(const Vector3& raybase, const Vector3& raycos, const Vector3& center, float radius, float *rin)
{
	Vector3 d = raybase - center;
	double bsq = raycos.DotProduct(d);
	double u = d.DotProduct(d) - sqr(radius);
	double disc = sqr(bsq) - u;
  
	bool hit = (disc >= 0.0);

	if(rin)
		*rin = (-bsq - enf_sqrte((float)disc));

	return hit;
}

//-----------------------------------------------------------------------
bool IntersectionUnitSphereRay(const Vector3& raybase, const Vector3& raycos, float *rin)
{
	double bsq = raycos.DotProduct(raybase);
	double u = raybase.DotProduct(raybase) - 1.0f;
	double disc = sqr(bsq) - u;
  
	bool hit = (disc >= 0.0);

	if(rin)
		*rin = (-bsq - enf_sqrte((float)disc));

	return hit;
}

//-----------------------------------------------------------------------
bool IntersectionSphereCone(const Vector3& origin, float radius, const Vector3& conepos, const Vector3& axis, float angle)
{
	float invsin = 1.0f / (float)sin(angle);
	float cossqr = (float)(cos(angle) * cos(angle));

	Vector3 kCmV = origin - conepos;
	Vector3 kD = (axis * (radius * invsin)) + kCmV;

	float fDSqrLen = kD.SquareSize();

	float fE = kD.DotProduct(axis);

	if ( fE > 0.0f && fE * fE >= fDSqrLen * cossqr )
	{
		float fSinSqr = (float)(sin(angle) * sin(angle));

		fDSqrLen = kCmV.SquareSize();
		fE = -kCmV.DotProduct(axis);
		if ( fE > 0.0f && fE * fE >= fDSqrLen * fSinSqr )
		{
			float fRSqr = radius * radius;
			return fDSqrLen <= fRSqr;
		}
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------
bool IntersectionSphereAABB(const Vector3& origin, double radiusSq, const Vector3& mins, const Vector3& maxs)
{
double d = 0; 
	//find the square of the distance
	//from the sphere to the box

	for( int i = 0; i < 3; i++ ) 
	{ 
		if( origin[i] < mins[i] )
		{
			double s = origin[i] - mins[i];
			d += s * s; 
		}
		else if( origin[i] > maxs[i] )
		{
			double s = origin[i] - maxs[i];
			d += s * s; 
		}
	}
	return d <= radiusSq;
}

//-----------------------------------------------------------------------
bool IntersectionSphereTriangle(const Vector3& mCenter, float mRadius, const Vector3& vert0, const Vector3& vert1, const Vector3& vert2)
{
	float mRadius2 = mRadius * mRadius;

	// Early exit if one of the vertices is inside the sphere
	Vector3 kDiff = vert2 - mCenter;
	float fC = kDiff.SquareSize();
	if(fC <= mRadius2)
		return true;

	kDiff = vert1 - mCenter;
	fC = kDiff.SquareSize();
	if(fC <= mRadius2)
		return true;

	kDiff = vert0 - mCenter;
	fC = kDiff.SquareSize();
	if(fC <= mRadius2)
		return true;

	// Else do the full distance test
	Vector3 TriEdge0	= vert1 - vert0;
	Vector3 TriEdge1	= vert2 - vert0;

//Vector3 kDiff	= vert0 - mCenter;
	float fA00	= TriEdge0.SquareSize();
	float fA01	= TriEdge0.DotProduct(TriEdge1);
	float fA11	= TriEdge1.SquareSize();
	float fB0	= kDiff.DotProduct(TriEdge0);
	float fB1	= kDiff.DotProduct(TriEdge1);
//float fC	= kDiff.SquareSize();
	float fDet	= fabsf(fA00*fA11 - fA01*fA01);
	float u		= fA01*fB1-fA11*fB0;
	float v		= fA01*fB0-fA00*fB1;
	float SqrDist;

	if(u + v <= fDet)
	{
		if(u < 0.0f)
		{
			if(v < 0.0f)  // region 4
			{
				if(fB0 < 0.0f)
				{
//					v = 0.0f;
					if(-fB0>=fA00)			{ /*u = 1.0f;*/		SqrDist = fA00+2.0f*fB0+fC;	}
					else					{ u = -fB0/fA00;	SqrDist = fB0*u+fC;			}
				}
				else
				{
//					u = 0.0f;
					if(fB1>=0.0f)			{ /*v = 0.0f;*/		SqrDist = fC;				}
					else if(-fB1>=fA11)		{ /*v = 1.0f;*/		SqrDist = fA11+2.0f*fB1+fC;	}
					else					{ v = -fB1/fA11;	SqrDist = fB1*v+fC;			}
				}
			}
			else  // region 3
			{
//				u = 0.0f;
				if(fB1>=0.0f)				{ /*v = 0.0f;*/		SqrDist = fC;				}
				else if(-fB1>=fA11)			{ /*v = 1.0f;*/		SqrDist = fA11+2.0f*fB1+fC;	}
				else						{ v = -fB1/fA11;	SqrDist = fB1*v+fC;			}
			}
		}
		else if(v < 0.0f)  // region 5
		{
//			v = 0.0f;
			if(fB0>=0.0f)					{ /*u = 0.0f;*/		SqrDist = fC;				}
			else if(-fB0>=fA00)				{ /*u = 1.0f;*/		SqrDist = fA00+2.0f*fB0+fC;	}
			else							{ u = -fB0/fA00;	SqrDist = fB0*u+fC;			}
		}
		else  // region 0
		{
			// minimum at interior point
			if(fDet==0.0f)
			{
//				u = 0.0f;
//				v = 0.0f;
				SqrDist = 3.4e+38f;// FLT_MAX;
			}
			else
			{
				float fInvDet = 1.0f/fDet;
				u *= fInvDet;
				v *= fInvDet;
				SqrDist = u*(fA00*u+fA01*v+2.0f*fB0) + v*(fA01*u+fA11*v+2.0f*fB1)+fC;
			}
		}
	}
	else
	{
		float fTmp0, fTmp1, fNumer, fDenom;

		if(u < 0.0f)  // region 2
		{
			fTmp0 = fA01 + fB0;
			fTmp1 = fA11 + fB1;
			if(fTmp1 > fTmp0)
			{
				fNumer = fTmp1 - fTmp0;
				fDenom = fA00-2.0f*fA01+fA11;
				if(fNumer >= fDenom)
				{
//					u = 1.0f;
//					v = 0.0f;
					SqrDist = fA00+2.0f*fB0+fC;
				}
				else
				{
					u = fNumer/fDenom;
					v = 1.0f - u;
					SqrDist = u*(fA00*u+fA01*v+2.0f*fB0) + v*(fA01*u+fA11*v+2.0f*fB1)+fC;
				}
			}
			else
			{
//				u = 0.0f;
				if(fTmp1 <= 0.0f)		{ /*v = 1.0f;*/		SqrDist = fA11+2.0f*fB1+fC;	}
				else if(fB1 >= 0.0f)	{ /*v = 0.0f;*/		SqrDist = fC;				}
				else					{ v = -fB1/fA11;	SqrDist = fB1*v+fC;			}
			}
		}
		else if(v < 0.0f)  // region 6
		{
			fTmp0 = fA01 + fB1;
			fTmp1 = fA00 + fB0;
			if(fTmp1 > fTmp0)
			{
				fNumer = fTmp1 - fTmp0;
				fDenom = fA00-2.0f*fA01+fA11;
				if(fNumer >= fDenom)
				{
//					v = 1.0f;
//					u = 0.0f;
					SqrDist = fA11+2.0f*fB1+fC;
				}
				else
				{
					v = fNumer/fDenom;
					u = 1.0f - v;
					SqrDist = u*(fA00*u+fA01*v+2.0f*fB0) + v*(fA01*u+fA11*v+2.0f*fB1)+fC;
				}
			}
			else
			{
//				v = 0.0f;
				if(fTmp1 <= 0.0f)		{ /*u = 1.0f;*/		SqrDist = fA00+2.0f*fB0+fC;	}
				else if(fB0 >= 0.0f)	{ /*u = 0.0f;*/		SqrDist = fC;				}
				else					{ u = -fB0/fA00;	SqrDist = fB0*u+fC;			}
			}
		}
		else  // region 1
		{
			fNumer = fA11 + fB1 - fA01 - fB0;
			if(fNumer <= 0.0f)
			{
//				u = 0.0f;
//				v = 1.0f;
				SqrDist = fA11+2.0f*fB1+fC;
			}
			else
			{
				fDenom = fA00-2.0f*fA01+fA11;
				if(fNumer >= fDenom)
				{
//					u = 1.0f;
//					v = 0.0f;
					SqrDist = fA00+2.0f*fB0+fC;
				}
				else
				{
					u = fNumer/fDenom;
					v = 1.0f - u;
					SqrDist = u*(fA00*u+fA01*v+2.0f*fB0) + v*(fA01*u+fA11*v+2.0f*fB1)+fC;
				}
			}
		}
	}

	return fabsf(SqrDist) < mRadius2;
}



//-----------------------------------------------------------------------
/*!
from ozcollide library, intersection between axis aligned box and triangle,
the box must be centered
*/
//-----------------------------------------------------------------------
bool IntersectionAABBTriangleCentered(const AAB& box, const Vector3& v0, const Vector3& v1, const Vector3& v2)
{
	Assert(box.GetCenter().Size() < 0.001f );

	#define FINDMINMAX(x0,x1,x2,min,max) \
		min = max = x0;   \
		if(x1<min) min=x1;\
		if(x1>max) max=x1;\
		if(x2<min) min=x2;\
		if(x2>max) max=x2;

	/*======================== X-tests ========================*/
	#define AXISTEST_X01(a, b, fa, fb)			   \
		p0 = a*v0.y - b*v0.z;			       	   \
		p2 = a*v2.y - b*v2.z;			       	   \
		if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
		rad = fa * box.GetMaxs().y + fb * box.GetMaxs().z;   \
		if(min>rad || max<-rad) return efalse;

	#define AXISTEST_X2(a, b, fa, fb)			   \
		p0 = a*v0.y - b*v0.z;			           \
		p1 = a*v1.y - b*v1.z;			       	   \
		if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
		rad = fa * box.GetMaxs().y + fb * box.GetMaxs().z;   \
		if(min>rad || max<-rad) return efalse;

	/*======================== Y-tests ========================*/
	#define AXISTEST_Y02(a, b, fa, fb)			   \
		p0 = -a*v0.x + b*v0.z;		      	   \
		p2 = -a*v2.x + b*v2.z;	       	       	   \
		if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
		rad = fa * box.GetMaxs().x + fb * box.GetMaxs().z;   \
		if(min>rad || max<-rad) return efalse;

	#define AXISTEST_Y1(a, b, fa, fb)			   \
		p0 = -a*v0.x + b*v0.z;		      	   \
		p1 = -a*v1.x + b*v1.z;	     	       	   \
		if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
		rad = fa * box.GetMaxs().x + fb * box.GetMaxs().z;   \
		if(min>rad || max<-rad) return efalse;

	/*======================== Z-tests ========================*/
	#define AXISTEST_Z12(a, b, fa, fb)			   \
		p1 = a*v1.x - b*v1.y;			           \
		p2 = a*v2.x - b*v2.y;			       	   \
		if(p2<p1) {min=p2; max=p1;} else {min=p1; max=p2;} \
		rad = fa * box.GetMaxs().x + fb * box.GetMaxs().y;   \
		if(min>rad || max<-rad) return efalse;

	#define AXISTEST_Z0(a, b, fa, fb)			   \
		p0 = a*v0.x - b*v0.y;				   \
		p1 = a*v1.x - b*v1.y;			           \
		if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
		rad = fa * box.GetMaxs().x + fb * box.GetMaxs().y;   \
		if(min>rad || max<-rad) return efalse;


	/*    use separating axis theorem to test overlap between triangle and box */
	/*    need to test for overlap in these directions: */
	/*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
	/*       we do not even need to test these) */
	/*    2) normal of the triangle */
	/*    3) crossproduct(edge from tri, {x,y,z}-directin) */
	/*       this gives 3x3=9 more tests */
	float min, max, d,p0,p1,p2,rad;

	/* This is the fastest branch on Sun */
	/* move everything so that the boxcenter is in (0,0,0) */
	//SUB(v0,triverts[0],boxcenter);
	//SUB(v1,triverts[1],boxcenter);
	//SUB(v2,triverts[2],boxcenter);

	/* compute triangle edges */
	Vector3 e0 = v1 - v0;
	Vector3 e1 = v2 - v1; 
	Vector3 e2 = v0 - v2;

	/* Bullet 3:  */
	/*  test the 9 tests first (this was faster) */
	float fex = fabsf(e0.x);
	float fey = fabsf(e0.y);
	float fez = fabsf(e0.z);
	AXISTEST_X01(e0.z, e0.y, fez, fey);
	AXISTEST_Y02(e0.z, e0.x, fez, fex);
	AXISTEST_Z12(e0.y, e0.x, fey, fex);

	fex = fabsf(e1.x);
	fey = fabsf(e1.y);
	fez = fabsf(e1.z);
	AXISTEST_X01(e1.z, e1.y, fez, fey);
	AXISTEST_Y02(e1.z, e1.x, fez, fex);
	AXISTEST_Z0(e1.y, e1.x, fey, fex);

	fex = fabsf(e2.x);
	fey = fabsf(e2.y);
	fez = fabsf(e2.z);
	AXISTEST_X2(e2.z, e2.y, fez, fey);
	AXISTEST_Y1(e2.z, e2.x, fez, fex);
	AXISTEST_Z12(e2.y, e2.x, fey, fex);

	/* Bullet 1: */
	/*  first test overlap in the {x,y,z}-directions */
	/*  find min, max of the triangle each direction, and test for overlap in */
	/*  that direction -- this is equivalent to testing a minimal AABB around */
	/*  the triangle against the AABB */

	/* test in X-direction */
	FINDMINMAX(v0.x, v1.x, v2.x, min, max);
	if(min > box.GetMaxs().x || max < box.GetMins().x) 
		return efalse;

	/* test in Y-direction */
	FINDMINMAX(v0.y, v1.y, v2.y, min, max);
	if(min > box.GetMaxs().y || max < box.GetMins().y) 
		return efalse;

	/* test in Z-direction */
	FINDMINMAX(v0.z, v1.z, v2.z, min, max);
	if(min > box.GetMaxs().z || max < box.GetMins().z) 
		return efalse;

	/* Bullet 2: */
	/*  test if the box intersects the plane of the triangle */
	/*  compute plane equation of triangle: normal*x+d=0 */
	Vector3 normal = e0.CrossProduct(e1);
	d = -normal.DotProduct(v0);  /* plane eq: normal.x+d=0 */
	MPlane plane(normal, d);

	return box.CrossingPlane(normal, d);
}



//-----------------------------------------------------------------------
bool IntersectionAABBTriangle(const AAB& box, const Vector3& V0, const Vector3& V1, const Vector3& V2)
{
	Vector3 center = box.GetCenter();

	Vector3 P0 = V0 - center;
	Vector3 P1 = V1 - center;
	Vector3 P2 = V2 - center;

	Vector3 ext = box.GetMaxs() - center;

	return IntersectionAABBTriangleCentered(AAB(-ext, ext), P0, P1, P2);
}



//-----------------------------------------------------------------------
bool IntersectionOBBTriangle(const AAB& obb, const Matrix43& orientation, const Vector3& V0, const Vector3& V1, const Vector3& V2)
{
	Vector3 P0 = orientation.InvTransformCoord(V0);
	Vector3 P1 = orientation.InvTransformCoord(V1);
	Vector3 P2 = orientation.InvTransformCoord(V2);

	return IntersectionAABBTriangle(obb, P0, P1, P2);
}


// Following code from Magic-Software (http://www.magic-software.com/)
static const float gs_fTolerance = 1e-05f;

//-----------------------------------------------------------------------
static float PointTriangleSqrDist(const Vector3& point, const Vector3& p0, const Vector3& p1, const Vector3& p2)
{
	// Hook
	Vector3 TriEdge0 = p1 - p0;
	Vector3 TriEdge1 = p2 - p0;

	Vector3 kDiff = p0 - point;
	float fA00 = TriEdge0.SquareSize();
	float fA01 = TriEdge0.DotProduct(TriEdge1);
	float fA11 = TriEdge1.SquareSize();
	float fB0 = kDiff.DotProduct(TriEdge0);
	float fB1 = kDiff.DotProduct(TriEdge1);
	float fC	= kDiff.SquareSize();
	float fDet = fabsf(fA00*fA11 - fA01*fA01);
	float fS	= fA01*fB1-fA11*fB0;
	float fT	= fA01*fB0-fA00*fB1;
	float fSqrDist;

	if(fS + fT <= fDet)
	{
		if(fS < 0.0f)
		{
			if(fT < 0.0f)  // region 4
			{
				if(fB0 < 0.0f)
				{
					if(-fB0 >= fA00)
						fSqrDist = fA00+2.0f*fB0+fC;
					else
						fSqrDist = fB0*(-fB0/fA00)+fC;
				}
				else
				{
					if(fB1 >= 0.0f)
						fSqrDist = fC;
					else if(-fB1 >= fA11)
						fSqrDist = fA11+2.0f*fB1+fC;
					else
						fSqrDist = fB1*(-fB1/fA11)+fC;
				}
			}
			else  // region 3
			{
				if(fB1 >= 0.0f)
					fSqrDist = fC;
				else if(-fB1 >= fA11)
					fSqrDist = fA11+2.0f*fB1+fC;
				else
					fSqrDist = fB1*(-fB1/fA11)+fC;
			}
		}
		else if(fT < 0.0f)  // region 5
		{
			if(fB0 >= 0.0f)
				fSqrDist = fC;
			else if(-fB0 >= fA00)
				fSqrDist = fA00+2.0f*fB0+fC;
			else
				fSqrDist = fB0*(-fB0/fA00)+fC;
		}
		else  // region 0
		{
			// minimum at interior point
			if(fDet==0.0f)
			{
				fSqrDist = FLT_MAX;
			}
			else
			{
				float fInvDet = 1.0f/fDet;
				fS *= fInvDet;
				fT *= fInvDet;
				fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) + fT*(fA01*fS+fA11*fT+2.0f*fB1)+fC;
			}
		}
	}
	else
	{
		float fTmp0, fTmp1, fNumer, fDenom;

		if(fS < 0.0f)  // region 2
		{
			fTmp0 = fA01 + fB0;
			fTmp1 = fA11 + fB1;
			if(fTmp1 > fTmp0)
			{
				fNumer = fTmp1 - fTmp0;
				fDenom = fA00-2.0f*fA01+fA11;
				if(fNumer >= fDenom)
				{
					fSqrDist = fA00+2.0f*fB0+fC;
				}
				else
				{
					fS = fNumer/fDenom;
					fT = 1.0f - fS;
					fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) + fT*(fA01*fS+fA11*fT+2.0f*fB1)+fC;
				}
			}
			else
			{
				if(fTmp1 <= 0.0f)
					fSqrDist = fA11+2.0f*fB1+fC;
				else if(fB1 >= 0.0f)
					fSqrDist = fC;
				else
					fSqrDist = fB1*(-fB1/fA11)+fC;
			}
		}
		else if(fT < 0.0f)  // region 6
		{
			fTmp0 = fA01 + fB1;
			fTmp1 = fA00 + fB0;
			if(fTmp1 > fTmp0)
			{
				fNumer = fTmp1 - fTmp0;
				fDenom = fA00-2.0f*fA01+fA11;
				if(fNumer >= fDenom)
				{
					fSqrDist = fA11+2.0f*fB1+fC;
				}
				else
				{
					fT = fNumer/fDenom;
					fS = 1.0f - fT;
					fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) + fT*(fA01*fS+fA11*fT+2.0f*fB1)+fC;
				}
			}
			else
			{
				if(fTmp1 <= 0.0f)
					fSqrDist = fA00+2.0f*fB0+fC;
				else if(fB0 >= 0.0f)
					fSqrDist = fC;
				else
					fSqrDist = fB0*(-fB0/fA00)+fC;
			}
		}
		else  // region 1
		{
			fNumer = fA11 + fB1 - fA01 - fB0;
			if(fNumer <= 0.0f)
			{
				fSqrDist = fA11+2.0f*fB1+fC;
			}
			else
			{
				fDenom = fA00-2.0f*fA01+fA11;
				if(fNumer >= fDenom)
				{
					fSqrDist = fA00+2.0f*fB0+fC;
				}
				else
				{
					fS = fNumer/fDenom;
					fT = 1.0f - fS;
					fSqrDist = fS*(fA00*fS+fA01*fT+2.0f*fB0) + fT*(fA01*fS+fA11*fT+2.0f*fB1)+fC;
				}
			}
		}
	}
	return fabsf(fSqrDist);
}


//-----------------------------------------------------------------------
class Ray
{
public:
	//! Constructor
	ENF_INLINE Ray() {}
	//! Constructor
	ENF_INLINE Ray(const Vector3& _orig, const Vector3& _dir) : orig(_orig), dir(_dir) {}
	//! Copy constructor
	ENF_INLINE Ray(const Ray& ray) : orig(ray.orig), dir(ray.dir) {}
	//! Destructor
	ENF_INLINE ~Ray() {}

	float	SquareDistance(const Vector3& point, float* t = NULL) const;
	ENF_INLINE float Distance(const Vector3& point, float* t = NULL) const
	{
		return sqrtf(SquareDistance(point, t));
	}

	Vector3	orig;		//!< Ray origin
	Vector3	dir;		//!< Normalized direction
};

//-----------------------------------------------------------------------
float SegmentSegmentSqrDist(const Segment& rkSeg0, const Segment& rkSeg1)
{
	// Hook
	Vector3 rkSeg0Direction	= rkSeg0.ComputeDirection();
	Vector3 rkSeg1Direction	= rkSeg1.ComputeDirection();

	Vector3 kDiff = rkSeg0.p0 - rkSeg1.p0;
	float fA00 = rkSeg0Direction.SquareSize();
	float fA01 = -rkSeg0Direction.DotProduct(rkSeg1Direction);
	float fA11 = rkSeg1Direction.SquareSize();
	float fB0 = kDiff.DotProduct(rkSeg0Direction);
	float fC	= kDiff.SquareSize();
	float fDet = fabsf(fA00*fA11-fA01*fA01);

	float fB1, fS, fT, fSqrDist, fTmp;

	if(fDet>=gs_fTolerance)
	{
		// line segments are not parallel
		fB1 = -kDiff.DotProduct(rkSeg1Direction);
		fS = fA01 * fB1 - fA11 * fB0;
		fT = fA01 * fB0 - fA00 * fB1;

		if(fS >= 0.0f)
		{
			if(fS <= fDet)
			{
				if(fT >= 0.0f)
				{
					if(fT <= fDet)  // region 0 (interior)
					{
						// minimum at two interior points of 3D lines
						float fInvDet = 1.0f / fDet;
						fS *= fInvDet;
						fT *= fInvDet;
						fSqrDist = fS * (fA00 * fS + fA01 * fT + 2.0f * fB0) + fT * (fA01 * fS + fA11 * fT + 2.0f * fB1) + fC;
					}
					else  // region 3 (side)
					{
						fTmp = fA01 + fB0;
						if(fTmp >= 0.0f)
							fSqrDist = fA11 + 2.0f * fB1 + fC;
						else if(-fTmp >= fA00)
							fSqrDist = fA00 + fA11 + fC + 2.0f * (fB1 + fTmp);
						else
							fSqrDist = fTmp * (-fTmp / fA00) + fA11 + 2.0f * fB1 + fC;
					}
				}
				else  // region 7 (side)
				{
					if(fB0 >= 0.0f)
						fSqrDist = fC;
					else if(-fB0 >= fA00)
						fSqrDist = fA00 + 2.0f * fB0 + fC;
					else
						fSqrDist = fB0 * (-fB0 / fA00) + fC;
				}
			}
			else
			{
				if ( fT >= 0.0 )
				{
					if ( fT <= fDet )  // region 1 (side)
					{
						fTmp = fA01 + fB1;
						if(fTmp >= 0.0f)
							fSqrDist = fA00 + 2.0f * fB0 + fC;
						else if(-fTmp>=fA11)
							fSqrDist = fA00 + fA11 + fC + 2.0f * (fB0 + fTmp);
						else
							fSqrDist = fTmp * (-fTmp / fA11) + fA00 + 2.0f * fB0 + fC;
					}
					else  // region 2 (corner)
					{
						fTmp = fA01 + fB0;
						if ( -fTmp <= fA00 )
						{
							if(fTmp >= 0.0f)
								fSqrDist = fA11 + 2.0f * fB1 + fC;
							else
								fSqrDist = fTmp * (-fTmp / fA00) + fA11 + 2.0f * fB1 + fC;
						}
						else
						{
							fTmp = fA01 + fB1;
							if(fTmp >= 0.0f)
								fSqrDist = fA00 + 2.0f * fB0 + fC;
							else if(-fTmp >= fA11)
								fSqrDist = fA00 + fA11 + fC + 2.0f * (fB0 + fTmp);
							else
								fSqrDist = fTmp * (-fTmp / fA11) + fA00 + 2.0f * fB0 + fC;
						}
					}
				}
				else  // region 8 (corner)
				{
					if ( -fB0 < fA00 )
					{
						if(fB0>=0.0f)
							fSqrDist = fC;
						else
							fSqrDist = fB0 * (-fB0 / fA00) + fC;
					}
					else
					{
						fTmp = fA01 + fB1;
						if(fTmp>=0.0f)
							fSqrDist = fA00 + 2.0f * fB0 + fC;
						else if(-fTmp>=fA11)
							fSqrDist = fA00 + fA11 + fC + 2.0f * (fB0 + fTmp);
						else
							fSqrDist = fTmp * (-fTmp / fA11) + fA00 + 2.0f * fB0 + fC;
					}
				}
			}
		}
		else 
		{
			if ( fT >= 0.0f )
			{
				if ( fT <= fDet )  // region 5 (side)
				{
					if(fB1>=0.0f)
						fSqrDist = fC;
					else if(-fB1>=fA11)
						fSqrDist = fA11+2.0f*fB1+fC;
					else
						fSqrDist = fB1*(-fB1/fA11)+fC;
				}
				else  // region 4 (corner)
				{
					fTmp = fA01+fB0;
					if ( fTmp < 0.0f )
					{
						if(-fTmp>=fA00)
							fSqrDist = fA00+fA11+fC+2.0f*(fB1+fTmp);
						else
							fSqrDist = fTmp*(-fTmp/fA00)+fA11+2.0f*fB1+fC;
					}
					else
					{
						if(fB1>=0.0f)
							fSqrDist = fC;
						else if(-fB1>=fA11)
							fSqrDist = fA11+2.0f*fB1+fC;
						else
							fSqrDist = fB1*(-fB1/fA11)+fC;
					}
				}
			}
			else   // region 6 (corner)
			{
				if ( fB0 < 0.0f )
				{
					if(-fB0>=fA00)
						fSqrDist = fA00+2.0f*fB0+fC;
					else
						fSqrDist = fB0*(-fB0/fA00)+fC;
				}
				else
				{
					if(fB1>=0.0f)
						fSqrDist = fC;
					else if(-fB1>=fA11)
						fSqrDist = fA11+2.0f*fB1+fC;
					else
						fSqrDist = fB1*(-fB1/fA11)+fC;
				}
			}
		}
	}
	else
	{
		// line segments are parallel
		if ( fA01 > 0.0f )
		{
			// direction vectors form an obtuse angle
			if ( fB0 >= 0.0f )
			{
				fSqrDist = fC;
			}
			else if ( -fB0 <= fA00 )
			{
				fSqrDist = fB0*(-fB0/fA00)+fC;
			}
			else
			{
				fB1 = -kDiff.DotProduct(rkSeg1Direction);
				fTmp = fA00+fB0;
				if ( -fTmp >= fA01 )
				{
					fSqrDist = fA00+fA11+fC+2.0f*(fA01+fB0+fB1);
				}
				else
				{
					fT = -fTmp/fA01;
					fSqrDist = fA00+2.0f*fB0+fC+fT*(fA11*fT+2.0f*(fA01+fB1));
				}
			}
		}
		else
		{
			// direction vectors form an acute angle
			if ( -fB0 >= fA00 )
			{
				fSqrDist = fA00+2.0f*fB0+fC;
			}
			else if ( fB0 <= 0.0f )
			{
				fSqrDist = fB0*(-fB0/fA00)+fC;
			}
			else
			{
				fB1 = -kDiff.DotProduct(rkSeg1Direction);
				if ( fB0 >= -fA01 )
				{
					fSqrDist = fA11+2.0f*fB1+fC;
				}
				else
				{
					fT = -fB0/fA01;
					fSqrDist = fC+fT*(2.0f*fB1+fA11*fT);
				}
			}
		}
	}
	return fabsf(fSqrDist);
}

ENF_INLINE float SegmentRaySqrDist(const Segment& rkSeg0, const Ray& rkSeg1)
{
	return SegmentSegmentSqrDist(rkSeg0, Segment(rkSeg1.orig, rkSeg1.orig + rkSeg1.dir));
}

//-----------------------------------------------------------------------
float SegmentTriangleSqrDist(const Segment& segment, const Vector3& p0, const Vector3& p1, const Vector3& p2)
{
	// Hook
	const Vector3 TriEdge0 = p1 - p0;
	const Vector3 TriEdge1 = p2 - p0;

	const Vector3& rkSegOrigin	= segment.GetOrigin();
	Vector3 rkSegDirection = segment.ComputeDirection();

	Vector3 kDiff = p0 - rkSegOrigin;
	float fA00 = rkSegDirection.SquareSize();
	float fA01 = -rkSegDirection.DotProduct(TriEdge0);
	float fA02 = -rkSegDirection.DotProduct(TriEdge1);
	float fA11 = TriEdge0.SquareSize();
	float fA12 = TriEdge0.DotProduct(TriEdge1);
	float fA22 = TriEdge1.DotProduct(TriEdge1);
	float fB0  = -kDiff.DotProduct(rkSegDirection);
	float fB1  = kDiff.DotProduct(TriEdge0);
	float fB2  = kDiff.DotProduct(TriEdge1);
	float fCof00 = fA11 * fA22 - fA12 * fA12;
	float fCof01 = fA02 * fA12 - fA01 * fA22;
	float fCof02 = fA01 * fA12 - fA02 * fA11;
	float fDet = fA00 * fCof00 + fA01 * fCof01 + fA02 * fCof02;

	Vector3 kPt;
	float fSqrDist, fSqrDist0;

	if(fabsf(fDet)>=gs_fTolerance)
	{
		float fCof11 = fA00 * fA22 - fA02 * fA02;
		float fCof12 = fA02 * fA01 - fA00 * fA12;
		float fCof22 = fA00 * fA11 - fA01 * fA01;
		float fInvDet = 1.0f / fDet;
		float fRhs0 = -fB0 * fInvDet;
		float fRhs1 = -fB1 * fInvDet;
		float fRhs2 = -fB2 * fInvDet;

		float fR = fCof00 * fRhs0 + fCof01 * fRhs1 + fCof02 * fRhs2;
		float fS = fCof01 * fRhs0 + fCof11 * fRhs1 + fCof12 * fRhs2;
		float fT = fCof02 * fRhs0 + fCof12 * fRhs1 + fCof22 * fRhs2;

		if ( fR < 0.0f )
		{
			if ( fS+fT <= 1.0f )
			{
				if ( fS < 0.0f )
				{
					if ( fT < 0.0f )  // region 4m
					{
						// min on face s=0 or t=0 or r=0
						fSqrDist = SegmentRaySqrDist(segment, Ray(p0, TriEdge1));
						fSqrDist0 = SegmentRaySqrDist(segment, Ray(p0, TriEdge0));
						fSqrDist = enf_min(fSqrDist, fSqrDist0);
						fSqrDist0 = PointTriangleSqrDist(rkSegOrigin, p0, p1, p2);
						fSqrDist = enf_min(fSqrDist, fSqrDist0);
					}
					else  // region 3m
					{
						// min on face s=0 or r=0
						fSqrDist = SegmentRaySqrDist(segment, Ray(p0, TriEdge1));
						fSqrDist0 = PointTriangleSqrDist(rkSegOrigin, p0, p1, p2);
						fSqrDist = enf_min(fSqrDist, fSqrDist0);
					}
				}
				else if ( fT < 0.0f )  // region 5m
				{
					// min on face t=0 or r=0
					fSqrDist = SegmentRaySqrDist(segment, Ray(p0, TriEdge0));
					fSqrDist0 = PointTriangleSqrDist(rkSegOrigin, p0, p1, p2);
					fSqrDist = enf_min(fSqrDist, fSqrDist0);
				}
				else  // region 0m
				{
					// min on face r=0
					fSqrDist = PointTriangleSqrDist(rkSegOrigin, p0, p1, p2);
				}
			}
			else
			{
				if ( fS < 0.0f )  // region 2m
				{
					// min on face s=0 or s+t=1 or r=0
					fSqrDist = SegmentRaySqrDist(segment, Ray(p0, TriEdge1));
					fSqrDist0 = SegmentRaySqrDist(segment, Ray(p1, TriEdge1 - TriEdge0));
					fSqrDist = enf_min(fSqrDist, fSqrDist0);
					fSqrDist0 = PointTriangleSqrDist(rkSegOrigin, p0, p1, p2);
					fSqrDist = enf_min(fSqrDist, fSqrDist0);
				}
				else if ( fT < 0.0f )  // region 6m
				{
					// min on face t=0 or s+t=1 or r=0
					fSqrDist = SegmentRaySqrDist(segment, Ray(p0, TriEdge0));
					fSqrDist0 = SegmentRaySqrDist(segment, Ray(p1, TriEdge1 - TriEdge0));
					fSqrDist = enf_min(fSqrDist, fSqrDist0);
					fSqrDist0 = PointTriangleSqrDist(rkSegOrigin, p0, p1, p2);
					fSqrDist = enf_min(fSqrDist, fSqrDist0);
				}
				else  // region 1m
				{
					// min on face s+t=1 or r=0
					fSqrDist = SegmentRaySqrDist(segment, Ray(p1, TriEdge1 - TriEdge0));
					fSqrDist0 = PointTriangleSqrDist(rkSegOrigin, p0, p1, p2);
					fSqrDist = enf_min(fSqrDist, fSqrDist0);
				}
			}
		}
		else if ( fR <= 1.0f )
		{
			if ( fS+fT <= 1.0f )
			{
				if ( fS < 0.0f )
				{
					if ( fT < 0.0f )  // region 4
					{
						// min on face s=0 or t=0
						fSqrDist = SegmentRaySqrDist(segment, Ray(p0, TriEdge1));
						fSqrDist0 = SegmentRaySqrDist(segment, Ray(p0, TriEdge0));
						fSqrDist = enf_min(fSqrDist, fSqrDist0);
					}
					else  // region 3
					{
						// min on face s=0
						fSqrDist = SegmentRaySqrDist(segment, Ray(p0, TriEdge1));
					}
				}
				else if ( fT < 0.0f )  // region 5
				{
					// min on face t=0
					fSqrDist = SegmentRaySqrDist(segment, Ray(p0, TriEdge0));
				}
				else  // region 0
				{
					// global minimum is interior, done
					fSqrDist = fR*(fA00*fR+fA01*fS+fA02*fT+2.0f*fB0)
						+fS*(fA01*fR+fA11*fS+fA12*fT+2.0f*fB1)
						+fT*(fA02*fR+fA12*fS+fA22*fT+2.0f*fB2)
						+kDiff.SquareSize();
				}
			}
			else
			{
				if ( fS < 0.0f )  // region 2
				{
					// min on face s=0 or s+t=1
					fSqrDist = SegmentRaySqrDist(segment, Ray(p0, TriEdge1));
					fSqrDist0 = SegmentRaySqrDist(segment, Ray(p1, TriEdge1 - TriEdge0));
					fSqrDist = enf_min(fSqrDist, fSqrDist0);
				}
				else if ( fT < 0.0f )  // region 6
				{
					// min on face t=0 or s+t=1
					fSqrDist = SegmentRaySqrDist(segment, Ray(p0, TriEdge0));
					fSqrDist0 = SegmentRaySqrDist(segment, Ray(p1, TriEdge1 - TriEdge0));
					fSqrDist = enf_min(fSqrDist, fSqrDist0);
				}
				else  // region 1
				{
					// min on face s+t=1
					fSqrDist = SegmentRaySqrDist(segment, Ray(p1, TriEdge1 - TriEdge0));
				}
			}
		}
		else  // fR > 1
		{
			if ( fS+fT <= 1.0f )
			{
				if ( fS < 0.0f )
				{
					if ( fT < 0.0f )  // region 4p
					{
						// min on face s=0 or t=0 or r=1
						fSqrDist = SegmentRaySqrDist(segment, Ray(p0, TriEdge1));
						fSqrDist0 = SegmentRaySqrDist(segment, Ray(p0, TriEdge0));
						fSqrDist = enf_min(fSqrDist, fSqrDist0);
						kPt = rkSegOrigin + rkSegDirection;
						fSqrDist0 = PointTriangleSqrDist(kPt, p0, p1, p2);
						fSqrDist = enf_min(fSqrDist, fSqrDist0);
					}
					else  // region 3p
					{
						// min on face s=0 or r=1
						fSqrDist = SegmentRaySqrDist(segment, Ray(p0, TriEdge1));
						kPt = rkSegOrigin + rkSegDirection;
						fSqrDist0 = PointTriangleSqrDist(kPt, p0, p1, p2);
						fSqrDist = enf_min(fSqrDist, fSqrDist0);
					}
				}
				else if ( fT < 0.0f )  // region 5p
				{
					// min on face t=0 or r=1
					fSqrDist = SegmentRaySqrDist(segment, Ray(p0, TriEdge0));
					kPt = rkSegOrigin + rkSegDirection;
					fSqrDist0 = PointTriangleSqrDist(kPt, p0, p1, p2);
					fSqrDist = enf_min(fSqrDist, fSqrDist0);
				}
				else  // region 0p
				{
					// min face on r=1
					kPt = rkSegOrigin + rkSegDirection;
					fSqrDist = PointTriangleSqrDist(kPt, p0, p1, p2);
				}
			}
			else
			{
				if ( fS < 0.0f )  // region 2p
				{
					// min on face s=0 or s+t=1 or r=1
					fSqrDist = SegmentRaySqrDist(segment, Ray(p0, TriEdge1));
					fSqrDist0 = SegmentRaySqrDist(segment, Ray(p1, TriEdge1 - TriEdge0));
					fSqrDist = enf_min(fSqrDist, fSqrDist0);
					kPt = rkSegOrigin+rkSegDirection;
					fSqrDist0 = PointTriangleSqrDist(kPt, p0, p1, p2);
					fSqrDist = enf_min(fSqrDist, fSqrDist0);
				}
				else if ( fT < 0.0f )  // region 6p
				{
					// min on face t=0 or s+t=1 or r=1
					fSqrDist = SegmentRaySqrDist(segment, Ray(p0, TriEdge0));
					fSqrDist0 = SegmentRaySqrDist(segment, Ray(p1, TriEdge1 - TriEdge0));
					fSqrDist = enf_min(fSqrDist, fSqrDist0);
					kPt = rkSegOrigin + rkSegDirection;
					fSqrDist0 = PointTriangleSqrDist(kPt, p0, p1, p2);
					fSqrDist = enf_min(fSqrDist, fSqrDist0);
				}
				else  // region 1p
				{
					// min on face s+t=1 or r=1
					fSqrDist = SegmentRaySqrDist(segment, Ray(p1, TriEdge1 - TriEdge0));
					kPt = rkSegOrigin + rkSegDirection;
					fSqrDist0 = PointTriangleSqrDist(kPt, p0, p1, p2);
					fSqrDist = enf_min(fSqrDist, fSqrDist0);
				}
			}
		}
	}
	else
	{
		// segment and triangle are parallel
		fSqrDist = SegmentRaySqrDist(segment, Ray(p0, TriEdge0));
		fSqrDist0 = SegmentRaySqrDist(segment, Ray(p0, TriEdge1));
		fSqrDist = enf_min(fSqrDist, fSqrDist0);

		fSqrDist0 = SegmentRaySqrDist(segment, Ray(p1, TriEdge1 - TriEdge0));
		fSqrDist = enf_min(fSqrDist, fSqrDist0);

		fSqrDist0 = PointTriangleSqrDist(rkSegOrigin, p0, p1, p2);
		fSqrDist = enf_min(fSqrDist, fSqrDist0);

		kPt = rkSegOrigin+rkSegDirection;
		fSqrDist0 = PointTriangleSqrDist(kPt, p0, p1, p2);
		fSqrDist = enf_min(fSqrDist, fSqrDist0);
	}
	return fabsf(fSqrDist);
}


//-----------------------------------------------------------------------
//check if two oriented centered bounding boxes overlap
//Original code by Miguel Gomez @Gamasutra
#if 0
bool IntersectionOBBOBB(const Vector3& a, const Matrix43& Ax, const Vector3& b, const Matrix43& Bx) 
{ 
	//translation, in parent frame
	Vector3 v = Bx.GetTranslation() - Ax.GetTranslation(); 
	//translation, in A's frame
	Vector3 T = Ax.InvTransformVector(v); 

	//B's basis with respect to A's local frame
	//calculate rotation matrix
	Matrix33 R = Ax.GetOrientation() * Bx.GetOrientation();

	/*ALGORITHM: Use the separating axis test for all 15 potential 
	separating axes. If a separating axis could not be found, the two 
	boxes overlap. */ 

	float ra, rb, t;

	//A's basis vectors
	for(int i = 0; i < 3; i++ )
	{
		ra = a[i]; 
		rb = b[0] * fabs(R[i][0]) + b[1] * fabs(R[i][1]) + b[2] * fabs(R[i][2]); 
		t = fabs( T[i] ); 

		if( t > ra + rb ) 
			return efalse; 
	} 

	//B's basis vectors
	for(int k = 0; k < 3; k++ )
	{ 
		ra = a[0] * fabs(R[0][k]) + a[1] * fabs(R[1][k]) + a[2] * fabs(R[2][k]); 
		rb = b[k]; 
		t = fabs(T[0] * R[0][k] + T[1] * R[1][k] + T[2] * R[2][k]); 

		if( t > ra + rb )
			return efalse; 
	} 

	//9 cross products

	//L = A0 x B0
	ra = a[1] * fabs(R[2][0]) + a[2] * fabs(R[1][0]); 
	rb = b[1] * fabs(R[0][2]) + b[2] * fabs(R[0][1]); 
	t = fabs( T[2] * R[1][0] - T[1] * R[2][0] ); 

	if( t > ra + rb )
		return efalse; 

	//L = A0 x B1
	ra = a[1] * fabs(R[2][1]) + a[2] * fabs(R[1][1]); 
	rb = b[0] * fabs(R[0][2]) + b[2] * fabs(R[0][0]); 
	t = fabs( T[2] * R[1][1] - T[1] * R[2][1] ); 

	if( t > ra + rb )
		return efalse; 

	//L = A0 x B2
	ra = a[1] * fabs(R[2][2]) + a[2] * fabs(R[1][2]); 
	rb = b[0] * fabs(R[0][1]) + b[1] * fabs(R[0][0]); 
	t = fabs( T[2] * R[1][2] - T[1] * R[2][2] ); 

	if( t > ra + rb )
		return efalse; 

	//L = A1 x B0
	ra = a[0] * fabs(R[2][0]) + a[2] * fabs(R[0][0]); 
	rb = b[1] * fabs(R[1][2]) + b[2] * fabs(R[1][1]); 
	t = fabs( T[0] * R[2][0] - T[2] * R[0][0] ); 

	if( t > ra + rb )
		return efalse; 

	//L = A1 x B1
	ra = a[0] * fabs(R[2][1]) + a[2] * fabs(R[0][1]); 
	rb = b[0] * fabs(R[1][2]) + b[2] * fabs(R[1][0]); 
	t = fabs( T[0] * R[2][1] - T[2] * R[0][1] ); 

	if( t > ra + rb )
		return efalse; 

	//L = A1 x B2
	ra = a[0] * fabs(R[2][2]) + a[2] * fabs(R[0][2]); 
	rb = b[0] * fabs(R[1][1]) + b[1] * fabs(R[1][0]); 
	t = fabs( T[0] * R[2][2] - T[2] * R[0][2] ); 

	if( t > ra + rb )
		return efalse; 

//L = A2 x B0
	ra = a[0] * fabs(R[1][0]) + a[1] * fabs(R[0][0]); 
	rb = b[1] * fabs(R[2][2]) + b[2] * fabs(R[2][1]); 
	t = fabs( T[1] * R[0][0] - T[0] * R[1][0] ); 

	if( t > ra + rb )
		return efalse; 

	//L = A2 x B1
	ra = a[0] * fabs(R[1][1]) + a[1] * fabs(R[0][1]); 
	rb = b[0] *fabs(R[2][2]) + b[2] * fabs(R[2][0]); 
	t = fabs( T[1] * R[0][1] - T[0] * R[1][1] ); 

	if( t > ra + rb )
		return efalse; 

	//L = A2 x B2
	ra = a[0] * fabs(R[1][2]) + a[1] * fabs(R[0][2]); 
	rb = b[0] * fabs(R[2][1]) + b[1] * fabs(R[2][0]); 
	t = fabs( T[1] * R[0][2] - T[0] * R[1][2] ); 

	if( t > ra + rb )
		return efalse; 

	/*no separating axis found,
	the two boxes overlap */
	return etrue; 
}

//-----------------------------------------------------------------------
bool IntersectionOBBOBB(const AAB& a, const Matrix43& frameA, const AAB& b, const Matrix43& frameB) 
{
	Vector3 centerA = a.GetCenter();
	Vector3 centerB = b.GetCenter();
	Matrix43 frameA2(frameA.GetOrientation(), frameA.TransformCoord(centerA));
	Matrix43 frameB2(frameB.GetOrientation(), frameB.TransformCoord(centerB));

	Vector3 extentsA = a.GetMaxs() - centerA;
	Vector3 extentsB = b.GetMaxs() - centerB;

	bool res = IntersectionOBBOBB(extentsA, frameA2, extentsB, frameB2);

	if(!res)
	{
		//Check points inside, because IntersectionOBBOBB is missing one case
		//TODO: find another code for OBBOBB intersection :-P
		for(uint n = 0; n < 8; n++)
		{
			Vector3 pointA = Vector3((n & 4) == 0 ? a.GetMins().z : a.GetMaxs().z,
										(n & 2) == 0 ? a.GetMins().y : a.GetMaxs().y,
										(n & 1) == 0 ? a.GetMins().x : a.GetMaxs().x);

			Vector3 pointInB = frameB.InvTransformCoord(frameA.TransformCoord(pointA));
			
			if(b.Intersects(pointInB))
			{
				return true;
			}

			Vector3 pointB = Vector3((n & 4) == 0 ? b.GetMins().z : b.GetMaxs().z,
										(n & 2) == 0 ? b.GetMins().y : b.GetMaxs().y,
										(n & 1) == 0 ? b.GetMins().x : b.GetMaxs().x);

			Vector3 pointInA = frameA.InvTransformCoord(frameB.TransformCoord(pointB));
			if(a.Intersects(pointInA))
			{
				return true;
			}
		}
	}


	//Shape* shapeA = Shape::CreateBox(-extentsA, extentsA, res ? 0xff4f0000 : 0xff3f3f00, SS_ONCE|SS_NOOUTLINE);
	//shapeA->SetMatrix(frameA2);

	//shapeA = Shape::CreateBox(a.GetMins(), a.GetMaxs(), 0xffffffff, SS_ONCE|SS_WIREFRAME);
	//shapeA->SetMatrix(frameA);

	//Shape* shapeB = Shape::CreateBox(-extentsB, extentsB, 0xff003f3f, SS_ONCE|SS_NOOUTLINE);
	//shapeB->SetMatrix(frameB2);

	//shapeB = Shape::CreateBox(b.GetMins(), b.GetMaxs(), 0xffffffff, SS_ONCE|SS_WIREFRAME);
	//shapeB->SetMatrix(frameB);

	return res;

}
#endif

}