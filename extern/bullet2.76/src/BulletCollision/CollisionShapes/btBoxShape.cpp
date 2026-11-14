/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2009 Erwin Coumans  http://bulletphysics.org

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/
#include "btBoxShape.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"

//{ 


void btBoxShape::getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const
{
	btTransformAabb(getHalfExtentsWithoutMargin(),getMarginNV(),t,aabbMin,aabbMax);
}


void	btBoxShape::calculateLocalInertia(btScalar mass,btVector3& inertia) const
{
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(51);
	btVector3 halfExtents = getHalfExtentsWithMargin();

	btVector3 l = btScalar(2.) * halfExtents;
	btVector3 l2 = l * l;

	vec_float4 tmp = vec_sld(l2.mVec, l2.mVec, 12);
	const btScalar scaledmass = mass * btScalar(0.08333333);
	inertia = scaledmass * (btVector3(vec_add(vec_sld(tmp, l2.mVec, 12), vec_sld(tmp, l2.mVec, 8))));
#else
	//btScalar margin = btScalar(0.);
	btVector3 halfExtents = getHalfExtentsWithMargin();

	btScalar lx=btScalar(2.)*(halfExtents.x());
	btScalar ly=btScalar(2.)*(halfExtents.y());
	btScalar lz=btScalar(2.)*(halfExtents.z());

	inertia.setValue(mass/(btScalar(12.0)) * (ly*ly + lz*lz),
					mass/(btScalar(12.0)) * (lx*lx + lz*lz),
					mass/(btScalar(12.0)) * (lx*lx + ly*ly));
#endif
}

const int CLIP_RIGHT		= 1 << 0;
const int CLIP_LEFT		= 1 << 1;
const int CLIP_TOP		= 1 << 2;
const int CLIP_BOTTOM	= 1 << 3;
const int CLIP_FRONT		= 1 << 4;
const int CLIP_BACK		= 1 << 5;

#ifdef BT_PS3PPU_VEC
const vec_int4 btBoxMask0 = {CLIP_RIGHT, CLIP_TOP, CLIP_BACK, 0};
const vec_int4 btBoxMask1 = {CLIP_LEFT, CLIP_BOTTOM, CLIP_FRONT, 0};
#endif

//-----------------------------------------------------------------------------
inline int Outcode(const btVector3& mins, const btVector3& maxs, const btVector3& p)
{
	int outcode = 0;
#ifdef BT_PS3PPU_VEC_extract
	//btVector3::btHitPrint(52);
	vec_bint4 cond1 = vec_cmpgt(p.mVec, maxs.mVec);
	vec_int4 res1 = vec_and(cond1, btBoxMask0);

	vec_bint4 cond2 = vec_cmplt(p.mVec, mins.mVec);
	vec_int4 res2 = vec_and(cond2, btBoxMask1);

	vec_int4 res = vec_sel((vec_int4)vec_sel(btIntZero, res2, cond2), res1, cond1);
	
	outcode = vec_extract(vec_or(vec_or(res, vec_sld(res, res, 4)), vec_sld(res, res, 8)), 0);
#else
	if( p.getX() > maxs.getX() ) {
		outcode |= CLIP_RIGHT;
	} else if( p.getX() < mins.getX() ) {
		outcode |= CLIP_LEFT;
	}
	if( p.getY() > maxs.getY() ) {
		outcode |= CLIP_TOP;
	} else if( p.getY() < mins.getY() ) {
		outcode |= CLIP_BOTTOM;
	}
	if( p.getZ() > maxs.getZ() ) {
		outcode |= CLIP_BACK;
	} else if( p.getZ() < mins.getZ() ) {
		outcode |= CLIP_FRONT;
	}
#endif
	return outcode;
}

//-----------------------------------------------------------------------------
bool IntersectionLineAABB(const btVector3& start, const btVector3& end, const btVector3& mins, const btVector3& maxs, btScalar& pfrac, btVector3& pNormal)
{
	pfrac = 0;

	int outcode1 = Outcode(mins, maxs, start );
	if( outcode1 == 0 )
	{
		//determine best side to get out of collision
#ifdef BT_PS3PPU_VEC
		static const btVector3 dirs[] = {
			btVector3(btIdentVec2),-btVector3(btIdentVec2),
			btVector3(btIdentVec0),-btVector3(btIdentVec0),
			btVector3(btIdentVec1),-btVector3(btIdentVec1)
		};
#else
		static const btVector3 dirs[] = {
			btVector3(0,0,1),btVector3(0,0,-1),
			btVector3(1,0,0),btVector3(-1,0,0),
			btVector3(0,1,0),btVector3(0,-1,0)
		};
#endif
		btVector3 dir = end - start;

		int best = 0;
		float bestDot = dir.dot(dirs[0]);
		for(int n = 1; n < 6; n++)
		{
			btScalar dot = dir.dot(dirs[n]);
			if(dot < bestDot)
			{
				bestDot = dot;
				best = n;
			}
		}
		pNormal = dirs[best];
		return true; // point inside bounding box
	}

	int outcode2 = Outcode(mins, maxs, end );
	
	if( (outcode1 & outcode2) != 0 )
		return false; // both points on same side of box

#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(53);
	// check intersections
	if( outcode1 & (CLIP_RIGHT | CLIP_LEFT | CLIP_TOP | CLIP_BOTTOM | CLIP_FRONT | CLIP_BACK)) 
	{
		const vec_int4 tmp = {outcode1 & CLIP_RIGHT, outcode1 & CLIP_TOP, outcode1 & CLIP_BACK, 0};
		const vec_bint4 mask = vec_cmpeq(tmp, btIntZero);

		vec_float4 vi = vec_sel(maxs.mVec, mins.mVec, mask);
		
		btVector3 v1 = vec_sub(end.mVec, start.mVec);		
		btVector3 v2 = vec_sub(vi, start.mVec);
    btVector3 vfrac = v2/v1;
		 
		vec_uint4 zero = (vec_uint4)btIntZero;

		if( outcode1 & (CLIP_RIGHT | CLIP_LEFT) )
		{ 
      vec_float4 vf = vec_sld(btZero, vec_vmrghw(vfrac.mVec, vfrac.mVec), 12);
      vec_float4 ixyzw = vec_madd(vf, vec_sub(end.mVec, start.mVec), start.mVec);
		
		  vec_bint4 condMax = vec_cmple(ixyzw, maxs.mVec);
		  vec_bint4 condMin = vec_cmpge(ixyzw, mins.mVec);
		  vec_uint4 cond = (vec_uint4)vec_and(condMax, condMin);

      if(vec_all_ne(vec_or(cond, vec_or(btSelVec0, btSelVec3)), zero))
			{
				pfrac = vfrac.getX();
				if(outcode1 & CLIP_RIGHT)
					pNormal.mVec = btIdentVec0;
				else
					pNormal.mVec = vec_sub(btZeroVec(), btIdentVec0);
				return true;
			}
		}

		if( outcode1 & (CLIP_TOP | CLIP_BOTTOM) )
		{ 
      vec_float4 vf = vec_vmrglw(vec_vmrghw(vfrac.mVec, vfrac.mVec), btZero);
      vec_float4 ixyzw = vec_madd(vf, vec_sub(end.mVec, start.mVec), start.mVec);
		
		  vec_bint4 condMax = vec_cmple(ixyzw, maxs.mVec);
		  vec_bint4 condMin = vec_cmpge(ixyzw, mins.mVec);
		  vec_uint4 cond = (vec_uint4)vec_and(condMax, condMin);
      if(vec_all_ne(vec_or(cond, vec_or(btSelVec1, btSelVec3)), zero))
			{
				pfrac = vfrac.getY();
				if(outcode1 & CLIP_TOP)
					pNormal.mVec = btIdentVec1;
				else
					pNormal.mVec = vec_sub(btZeroVec(), btIdentVec1);
				return true;
			}
		}

		if( outcode1 & (CLIP_FRONT | CLIP_BACK) )
		{
      vec_float4 vf = vec_vmrglw(vfrac.mVec, vfrac.mVec);
      vec_float4 ixyzw = vec_madd(vf, vec_sub(end.mVec, start.mVec), start.mVec);
		
		  vec_bint4 condMax = vec_cmple(ixyzw, maxs.mVec);
		  vec_bint4 condMin = vec_cmpge(ixyzw, mins.mVec);
		  vec_uint4 cond = (vec_uint4)vec_and(condMax, condMin);
      if(vec_all_ne(vec_or(cond, vec_or(btSelVec2, btSelVec3)), zero))
			{
				pfrac = vfrac.getZ();
				if(outcode1 & CLIP_BACK)
					pNormal.mVec = btIdentVec2;
				else
					pNormal.mVec = vec_sub(btZeroVec(), btIdentVec2);
				return true;
			} 
		}
	}
#else
	btScalar frac;

	// check intersections
	if( outcode1 & (CLIP_RIGHT | CLIP_LEFT) )
	{
		btScalar ix = (outcode1 & CLIP_RIGHT) ? maxs.getX() : mins.getX();

		btScalar x1 = end.getX() - start.getX();
		btScalar x2 = ix - start.getX();
		frac = x2 / x1;

		btScalar iy = start.getY() + (end.getY() - start.getY()) * frac;
		btScalar iz = start.getZ() + (end.getZ() - start.getZ()) * frac;
		if( iy <= maxs.getY() && iy >= mins.getY() && iz <= maxs.getZ() && iz >= mins.getZ())
		{
			pfrac = frac;
			pNormal.setValue((outcode1 & CLIP_RIGHT) ? btScalar(1.0f) : btScalar(-1.0f), btScalar(0), btScalar(0));
			return true;
		}
	}

	if( outcode1 & (CLIP_TOP | CLIP_BOTTOM) )
	{
		btScalar iy = (outcode1 & CLIP_TOP) ? maxs.getY() : mins.getY();

		btScalar y1 = end.getY() - start.getY();
		btScalar y2 = iy - start.getY();
		frac = y2 / y1;

		btScalar ix = start.getX() + (end.getX() - start.getX()) * frac;
		btScalar iz = start.getZ() + (end.getZ() - start.getZ()) * frac;
		if( ix <= maxs.getX() && ix >= mins.getX() &&  iz <= maxs.getZ() && iz >= mins.getZ())
		{
			pfrac = frac;
			pNormal.setValue(btScalar(0), (outcode1 & CLIP_TOP) ? btScalar(1.0f) : btScalar(-1.0f), btScalar(0));
			return true;
		}
	}

	if( outcode1 & (CLIP_FRONT | CLIP_BACK) )
	{
		btScalar iz = (outcode1 & CLIP_BACK) ? maxs.getZ() : mins.getZ();

		btScalar z1 = end.getZ() - start.getZ();
		btScalar z2 = iz - start.getZ();
		frac = z2 / z1;

		btScalar ix = start.getX() + (end.getX() - start.getX()) * frac;
		btScalar iy = start.getY() + (end.getY() - start.getY()) * frac;
		if( ix <= maxs.getX() && ix >= mins.getX() &&  iy <= maxs.getY() && iy >= mins.getY() )
		{
			pfrac = frac;
			pNormal.setValue(btScalar(0), btScalar(0), (outcode1 & CLIP_BACK) ? btScalar(1.0f) : btScalar(-1.0f));
			return true;
		}
	}
#endif
	// nothing found
	return false;
}

//-----------------------------------------------------------------------------
bool IntersectionLineAABB(const btVector3& start, const btVector3& end, const btVector3& mins, const btVector3& maxs)
{
	int outcode1 = Outcode(mins, maxs, start );
	if( outcode1 == 0 )
	{
		return true; // point inside bounding box
	}

	int outcode2 = Outcode(mins, maxs, end );
	
	if( (outcode1 & outcode2) != 0 )
		return false; // both points on same side of box
#ifdef BT_PS3PPU_VEC_ZASKODNIK
	//btVector3::btHitPrint(54);
	vec_int4 condMin, condMax;
	if( outcode1 & (CLIP_RIGHT | CLIP_LEFT | CLIP_TOP | CLIP_BOTTOM | CLIP_FRONT | CLIP_BACK))
	{
		const vec_int4 tmp = {outcode1 & CLIP_RIGHT, outcode1 & CLIP_TOP, outcode1 & CLIP_BACK, 0};
		const vec_bint4 mask = vec_cmpgt(tmp, btIntZero);
		btVector3 vi = vec_sel(mins.mVec, maxs.mVec, mask);
		
		btVector3 v1 = end - start;		
		btVector3 v2 = vi - start;
		btVector3 vfrac = v2 / v1;
		
		btVector3 ixyzw = start + (end - start) * vfrac;
		
		condMax = (vec_int4)vec_cmple(maxs.mVec, ixyzw.mVec);
		condMin = (vec_int4)vec_cmpge(mins.mVec, ixyzw.mVec);
	}
	
	if( outcode1 & (CLIP_RIGHT | CLIP_LEFT) )
	{
		return vec_all_ne(vec_vmrghw(vec_sld(condMax, condMax, 4), vec_sld(condMin, condMin, 4)), btIntZero);
	}

	if( outcode1 & (CLIP_TOP | CLIP_BOTTOM) )
	{
		return vec_all_ne(vec_vmrghw(vec_vmrghw(condMax, condMin), vec_vmrglw(condMax, condMin)), btIntZero);
	}

	if( outcode1 & (CLIP_FRONT | CLIP_BACK) )
	{
		return vec_all_ne(vec_vmrghw(condMax, condMin), btIntZero);
	}
#else
	// check intersections
	if( outcode1 & (CLIP_RIGHT | CLIP_LEFT) )
	{
		btScalar ix = (outcode1 & CLIP_RIGHT) ? maxs.getX() : mins.getX();

		btScalar x1 = end.getX() - start.getX();
		btScalar x2 = ix - start.getX();
		btScalar frac = x2 / x1;

		btScalar iy = start.getY() + (end.getY() - start.getY()) * frac;
		btScalar iz = start.getZ() + (end.getZ() - start.getZ()) * frac;
		if( iy <= maxs.getY() && iy >= mins.getY() && iz <= maxs.getZ() && iz >= mins.getZ())
		{
			return true;
		}
	}

	if( outcode1 & (CLIP_TOP | CLIP_BOTTOM) )
	{
		btScalar iy = (outcode1 & CLIP_TOP) ? maxs.getY() : mins.getY();

		btScalar y1 = end.getY() - start.getY();
		btScalar y2 = iy - start.getY();
		btScalar frac = y2 / y1;

		btScalar ix = start.getX() + (end.getX() - start.getX()) * frac;
		btScalar iz = start.getZ() + (end.getZ() - start.getZ()) * frac;
		if( ix <= maxs.getX() && ix >= mins.getX() &&  iz <= maxs.getZ() && iz >= mins.getZ())
		{
			return true;
		}
	}

	if( outcode1 & (CLIP_FRONT | CLIP_BACK) )
	{
		btScalar iz = (outcode1 & CLIP_BACK) ? maxs.getZ() : mins.getZ();

		btScalar z1 = end.getZ() - start.getZ();
		btScalar z2 = iz - start.getZ();
		btScalar frac = z2 / z1;

		btScalar ix = start.getX() + (end.getX() - start.getX()) * frac;
		btScalar iy = start.getY() + (end.getY() - start.getY()) * frac;
		if( ix <= maxs.getX() && ix >= mins.getX() &&  iy <= maxs.getY() && iy >= mins.getY() )
		{
			return true;
		}
	}
	// nothing found
	return false;
#endif
}

btScalar btBoxShape::getLineIntersection(const btCollisionObject* obj, const btTransform& colObjWorldTransform, const btVector3& from, const btVector3& to, btVector3& contactNormal, const btMaterial*& contactMaterial) const
{
	btVector3 lfrom = colObjWorldTransform.invXform(from);
	btVector3 lto = colObjWorldTransform.invXform(to);

#ifdef BT_PS3PPU_VEC
	btVector3 margin(getMarginNV());
#else
	btVector3 margin(getMarginNV(),getMarginNV(),getMarginNV());
#endif
	btVector3 halfExtents = margin + m_implicitShapeDimensions;
	
	btScalar frac;
	if(IntersectionLineAABB(lfrom, lto, -halfExtents, halfExtents, frac, contactNormal))
	{
		contactMaterial = getMaterial();
		return frac;
	}

	return btScalar(-1);
}

