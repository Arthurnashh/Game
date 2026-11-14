/*
Copyright (c) 2003-2006 Gino van den Bergen / Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/



#ifndef AABB_UTIL2
#define AABB_UTIL2

#include "btTransform.h"
#include "btVector3.h"
#include "btMinMax.h"

#ifdef BT_PS3PPU_VEC
	const vec_int4 btAabbMinus = {1, 2, 4, 0};
	const vec_int4 btAabbPlus = {8, 0x10, 0x20, 0};
#endif


SIMD_FORCE_INLINE void AabbExpand (btVector3& aabbMin,
								   btVector3& aabbMax,
								   const btVector3& expansionMin,
								   const btVector3& expansionMax)
{
	aabbMin = aabbMin + expansionMin;
	aabbMax = aabbMax + expansionMax;
}

/// conservative test for overlap between two aabbs
SIMD_FORCE_INLINE bool TestPointAgainstAabb2(const btVector3 &aabbMin1, const btVector3 &aabbMax1,
								const btVector3 &point)
{	
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(132);
	vec_int4 cond = (vec_int4)vec_or(vec_cmpgt(aabbMin1.mVec, point.mVec), vec_cmplt(aabbMax1.mVec, point.mVec));
	//ignore w
	cond = vec_sel(cond, btIntZero, btSelVec3);
	bool overlap = vec_all_eq(cond, btIntZero);
#else
	bool overlap = true;
	overlap = (aabbMin1.getX() > point.getX() || aabbMax1.getX() < point.getX()) ? false : overlap;
	overlap = (aabbMin1.getZ() > point.getZ() || aabbMax1.getZ() < point.getZ()) ? false : overlap;
	overlap = (aabbMin1.getY() > point.getY() || aabbMax1.getY() < point.getY()) ? false : overlap;
#endif
	return overlap;
}


/// conservative test for overlap between two aabbs
SIMD_FORCE_INLINE bool TestAabbAgainstAabb2(const btVector3 &aabbMin1, const btVector3 &aabbMax1,
								const btVector3 &aabbMin2, const btVector3 &aabbMax2)
{
#ifdef BT_PS3PPU_VEC	//t
	//btVector3::btHitPrint(133);
	vec_int4 cond = (vec_int4)vec_or(vec_cmpgt(aabbMin1.mVec, aabbMax2.mVec), vec_cmplt(aabbMax1.mVec, aabbMin2.mVec));
	//ignore w
	cond = vec_sel(cond, btIntZero, btSelVec3);
	bool overlap = vec_all_eq(cond, btIntZero);
#else
	bool overlap = true;
	overlap = (aabbMin1.getX() > aabbMax2.getX() || aabbMax1.getX() < aabbMin2.getX()) ? false : overlap;
	overlap = (aabbMin1.getZ() > aabbMax2.getZ() || aabbMax1.getZ() < aabbMin2.getZ()) ? false : overlap;
	overlap = (aabbMin1.getY() > aabbMax2.getY() || aabbMax1.getY() < aabbMin2.getY()) ? false : overlap;
#endif
	return overlap;
}

/// conservative test for overlap between triangle and aabb
SIMD_FORCE_INLINE bool TestTriangleAgainstAabb2(const btVector3 *vertices,
									const btVector3 &aabbMin, const btVector3 &aabbMax)
{
	const btVector3 &p1 = vertices[0];
	const btVector3 &p2 = vertices[1];
	const btVector3 &p3 = vertices[2];

#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(99);
	vec_int4 cond = (vec_int4)vec_or(vec_cmpgt(vec_min(vec_min(p1.mVec, p2.mVec), p3.mVec), aabbMax.mVec), vec_cmplt(vec_max(vec_max(p1.mVec, p2.mVec), p3.mVec), aabbMin.mVec));
	return vec_all_eq(vec_sel(cond, btIntZero, btSelVec3), btIntZero);
#else
	if (btMin(btMin(p1[0], p2[0]), p3[0]) > aabbMax[0]) return false;
	if (btMax(btMax(p1[0], p2[0]), p3[0]) < aabbMin[0]) return false;

	if (btMin(btMin(p1[2], p2[2]), p3[2]) > aabbMax[2]) return false;
	if (btMax(btMax(p1[2], p2[2]), p3[2]) < aabbMin[2]) return false;
  
	if (btMin(btMin(p1[1], p2[1]), p3[1]) > aabbMax[1]) return false;
	if (btMax(btMax(p1[1], p2[1]), p3[1]) < aabbMin[1]) return false;
	return true;
#endif
}

SIMD_FORCE_INLINE int	btOutcode(const btVector3& p,const btVector3& halfExtent) 
{
#ifdef BT_PS3PPU_VEC_extract
	//btVector3::btHitPrint(134);
	vec_int4 minRes = vec_and(btAabbMinus, vec_cmplt(p.mVec, vec_sub(btZeroVec(), halfExtent.mVec)));
	vec_int4 plusRes = vec_and(btAabbPlus, vec_cmpgt(p.mVec, halfExtent.mVec));
	vec_int4 res = vec_or(minRes, plusRes);
	return vec_extract(vec_or(vec_or(res, vec_sld(res, res, 4)), vec_sld(res, res, 8)), 0);
#else
	return (p.getX()  < -halfExtent.getX() ? 0x01 : 0x0) |    
		   (p.getX() >  halfExtent.getX() ? 0x08 : 0x0) |
		   (p.getY() < -halfExtent.getY() ? 0x02 : 0x0) |    
		   (p.getY() >  halfExtent.getY() ? 0x10 : 0x0) |
		   (p.getZ() < -halfExtent.getZ() ? 0x4 : 0x0) |    
		   (p.getZ() >  halfExtent.getZ() ? 0x20 : 0x0);
#endif
}



SIMD_FORCE_INLINE bool btRayAabb2(const btVector3& rayFrom,
								  const btVector3& rayInvDirection,
#ifdef BT_PS3PPU_VEC
								  const vec_bint4 raySign,
								  const btVector3* bounds,
								  //btScalar& tmin,
								  vec_float4 lambda_min,
								  vec_float4 lambda_max)
#else
								  const unsigned int raySign[3],
								  const btVector3* bounds,
								  //btScalar& tmin,
								  btScalar lambda_min,
								  btScalar lambda_max)
#endif
{
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(36); //checked
	btVector3 t0 = (bounds[0] - rayFrom) * rayInvDirection;
	btVector3 t1 = (bounds[1] - rayFrom) * rayInvDirection;
	
	//rozdeleni na min a max
	vec_float4 bmin = vec_sel(t0.mVec, t1.mVec, raySign);
	vec_float4 bmax = vec_sel(t1.mVec, t0.mVec, raySign);
	
	//je-li nake min vetsi nez nake max -> false
	//min abcx
	//max defx
	//-------------
	//min porovnat s efd a fde - vynechat posledni prvek
	vec_float4 xdef = vec_sld(bmax, bmax, 12);
	vec_bint4 cond1 = vec_and(vec_cmplt(bmin, vec_sld(xdef, bmax, 8)), vec_cmplt(bmin, vec_sld(xdef, bmax, 12)));
	vec_bint4 cond2 = vec_and(vec_cmplt(bmin, lambda_max), vec_cmpgt(bmax, lambda_min));
	bool vecret = vec_all_ne(vec_sel((vec_uint4)vec_and(cond1, cond2), btSelVec3, btSelVec3), (vec_uint4)btIntZero);

	return vecret;
#else
	btScalar tmin, tmax, tymin, tymax, tzmin, tzmax;
	tmin = (bounds[raySign[0]].getX() - rayFrom.getX()) * rayInvDirection.getX();
	tmax = (bounds[1-raySign[0]].getX() - rayFrom.getX()) * rayInvDirection.getX();
	tymin = (bounds[raySign[1]].getY() - rayFrom.getY()) * rayInvDirection.getY();
	tymax = (bounds[1-raySign[1]].getY() - rayFrom.getY()) * rayInvDirection.getY();

	if ( (tmin > tymax) || (tymin > tmax) )
		return false;

	if (tymin > tmin)
		tmin = tymin;

	if (tymax < tmax)
		tmax = tymax;

	tzmin = (bounds[raySign[2]].getZ() - rayFrom.getZ()) * rayInvDirection.getZ();
	tzmax = (bounds[1-raySign[2]].getZ() - rayFrom.getZ()) * rayInvDirection.getZ();

	if ( (tmin > tzmax) || (tzmin > tmax) )
		return false;
	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;
	return ( (tmin < lambda_max) && (tmax > lambda_min) );
#endif
}

SIMD_FORCE_INLINE bool btRayAabb(const btVector3& rayFrom, 
								 const btVector3& rayTo, 
								 const btVector3& aabbMin, 
								 const btVector3& aabbMax,
					  btScalar& param, btVector3& normal) 
{
	btVector3 aabbHalfExtent = (aabbMax-aabbMin)* btScalar(0.5);
	btVector3 aabbCenter = (aabbMax+aabbMin)* btScalar(0.5);
	btVector3	source = rayFrom - aabbCenter;
	btVector3	target = rayTo - aabbCenter;
	
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(180);
	//int	sourceOutcode = btOutcode(source,aabbHalfExtent);
	vec_float4 minusHalf = vec_sub(btZeroVec(), aabbHalfExtent.mVec);
	vec_int4 sMinRes = vec_sel((vec_int4)vec_cmplt(source.mVec, minusHalf), btIntZero, btSelVec3);	//1, 2, 4
	vec_bint4 sPlusRes = vec_cmpgt(source.mVec, aabbHalfExtent.mVec); //8, 16, 32
	//int targetOutcode = btOutcode(target,aabbHalfExtent);
	vec_int4 tMinRes = vec_sel((vec_int4)vec_cmplt(target.mVec, minusHalf), btIntZero, btSelVec3);
	vec_bint4 tPlusRes = vec_cmpgt(target.mVec, aabbHalfExtent.mVec); 
	
	//if ((sourceOutcode & targetOutcode) == 0x0)
	if(vec_all_eq(vec_and(sMinRes, tMinRes), btIntZero) &&	vec_all_eq(vec_sel((vec_int4)vec_and(sPlusRes, tPlusRes), btIntZero, btSelVec3), btIntZero))
	{
		btScalar lambda_enter = 0;
		btScalar lambda_exit  = param;
		btVector3 r = target - source;
		int i;
		btScalar	normSign = 1;	//podruhy -1
		btVector3	hitNormal = btZeroVec();	//normsign ve smeru kde je lambda_enter zbytek 0
		int bit=1;

		if(!(vec_all_eq(sMinRes, btIntZero), vec_all_eq(vec_sel(tMinRes, btIntZero, btSelVec3), btIntZero)))
		{
			btVector3 lambdaPlus = (-source - aabbHalfExtent) / r;
			btVector3 lambdaMinus = (-source + aabbHalfExtent) / r;
			
			//if (sourceOutcode & bit)..//vymaskuj, co se nepocita a co je mensi jak nula(taky se nepocita, ale je to treba?)
			vec_float4 lpcut = vec_max(btZeroVec(), vec_sel(btZeroVec(), lambdaPlus.mVec, (vec_bint4)sMinRes));	
			vec_float4 lmcut = vec_max(btZeroVec(), vec_sel(btZeroVec(), lambdaMinus.mVec, (vec_bint4)sMinRes));
			
			//if (sourceOutcode & bit)
			if(vec_any_ne(sMinRes, btIntZero))
			{
				//nastav w 0 at se neplete u hit normaly
				lpcut = vec_sel(lpcut, btZeroVec(), btSelVec3);	
				lmcut = vec_sel(lmcut, btZeroVec(), btSelVec3);	
				
				//maximum obou do lambda_enter(minimalne 0) ktera osa do hitNormalu(se znamenkem, kt1ery vektor)
				btVector3 maxlambda = vec_max(lpcut, lmcut);
				vec_bint4 xy_yz_z0 = vec_cmpgt(maxlambda.mVec, vec_sld(maxlambda.mVec, maxlambda.mVec, 4));
				vec_bint4 xyz_yz0_z0x = vec_and(xy_yz_z0, vec_cmpgt(maxlambda.mVec, vec_sld(maxlambda.mVec, maxlambda.mVec, 8)));
				vec_bint4 xyz0_yz0x_z0xy = vec_and(xyz_yz0_z0x, vec_cmpgt(maxlambda.mVec, vec_sld(maxlambda.mVec, maxlambda.mVec, 12)));	
				hitNormal.mVec = vec_sel(btZeroVec(), vec_float4(1.f), xyz0_yz0x_z0xy);
				vec_bint4 lambdaMask = vec_cmpge(lmcut, lpcut);
				hitNormal.mVec = vec_sel(hitNormal.mVec, vec_sub(btZeroVec(), hitNormal.mVec), lambdaMask);
				
				float *tmp = maxlambda.m_floats;
				
				lambda_enter = (tmp[0] < tmp[1]) ? ((tmp[1] < tmp[2]) ? tmp[2] : tmp[1]) : ((tmp[0] < tmp[2]) ? tmp[2] : tmp[0]);
			}
			//else if (targetOutcode & bit) 
			else if(vec_any_ne(tMinRes, btIntZero))
			{
				btVector3 minlambda = vec_min(lpcut, lmcut);
				float *tmp = minlambda.m_floats;
				
				lambda_exit = (tmp[0] > tmp[1]) ? ((tmp[1] > tmp[2]) ? tmp[2] : tmp[1]) : ((tmp[0] > tmp[2]) ? tmp[2] : tmp[0]);
			}
		}
#else
	int	sourceOutcode = btOutcode(source,aabbHalfExtent);
	int targetOutcode = btOutcode(target,aabbHalfExtent);
	if ((sourceOutcode & targetOutcode) == 0x0)
	{
		btScalar lambda_enter = btScalar(0.0);
		btScalar lambda_exit  = param;
		btVector3 r = target - source;
		int i;
		btScalar	normSign = 1;
		btVector3	hitNormal(0,0,0);
		int bit=1;

		for (int j=0;j<2;j++)
		{
			for (i = 0; i != 3; ++i)
			{
				if (sourceOutcode & bit)
				{
					btScalar lambda = (-source[i] - aabbHalfExtent[i]*normSign) / r[i];
					if (lambda_enter <= lambda)
					{
						lambda_enter = lambda;
						hitNormal.setValue(0,0,0);
						hitNormal[i] = normSign;
					}
				}
				else if (targetOutcode & bit) 
				{
					btScalar lambda = (-source[i] - aabbHalfExtent[i]*normSign) / r[i];
					btSetMin(lambda_exit, lambda);
				}
				bit<<=1;
			}
			normSign = btScalar(-1.);
		}
#endif
		if (lambda_enter <= lambda_exit)
		{
			param = lambda_enter;
			normal = hitNormal;
			return true;
		}
	}
	return false;
}



SIMD_FORCE_INLINE	void btTransformAabb(const btVector3& halfExtents, btScalar margin,const btTransform& t,btVector3& aabbMinOut,btVector3& aabbMaxOut)
{
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(135);
	vec_float4 halfExtentsWithMargin = vec_add(halfExtents.mVec, vec_float4(margin));
	btMatrix3x3 abs_b = t.getBasis().absolute();  
	btVector3 center = t.getOrigin();

	btVector3 extent = btVector3(vec_dot3mx(abs_b.get128ptr(), halfExtentsWithMargin));

	aabbMinOut = vec_sub(center.mVec, extent.mVec);
	aabbMaxOut = vec_add(center.mVec, extent.mVec);
#else
	btVector3 halfExtentsWithMargin = halfExtents+btVector3(margin,margin,margin);
	btMatrix3x3 abs_b = t.getBasis().absolute();  
	btVector3 center = t.getOrigin();
	btVector3 extent = btVector3(abs_b[0].dot(halfExtentsWithMargin),
		   abs_b[1].dot(halfExtentsWithMargin),
		  abs_b[2].dot(halfExtentsWithMargin));
	
	aabbMinOut = center - extent;
	aabbMaxOut = center + extent;
#endif
}


SIMD_FORCE_INLINE	void btTransformAabb(const btVector3& localAabbMin,const btVector3& localAabbMax, btScalar margin,const btTransform& trans,btVector3& aabbMinOut,btVector3& aabbMaxOut)
{
		btAssert(localAabbMin.getX() <= localAabbMax.getX());
		btAssert(localAabbMin.getY() <= localAabbMax.getY());
		btAssert(localAabbMin.getZ() <= localAabbMax.getZ());

		
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(136);
		vec_float4 half = vec_float4(0.5f);
		vec_float4 localHalfExtents = vec_madd(half, vec_sub(localAabbMax.mVec, localAabbMin.mVec), btZeroVec());
		localHalfExtents = vec_add(localHalfExtents, vec_float4(margin));
		btVector3 localCenter = vec_madd(half, vec_add(localAabbMax.mVec, localAabbMin.mVec), btZeroVec());
		btMatrix3x3 abs_b = trans.getBasis().absolute();  
		vec_float4 center = trans(localCenter).mVec;
		vec_float4 extent = vec_dot3mx(abs_b.get128ptr(), localHalfExtents);

		aabbMinOut = vec_sub(center, extent);
		aabbMaxOut = vec_add(center, extent);
#else
		btVector3 localHalfExtents = btScalar(0.5)*(localAabbMax-localAabbMin);
		localHalfExtents+=btVector3(margin,margin,margin);
		btVector3 localCenter = btScalar(0.5)*(localAabbMax+localAabbMin);
		btMatrix3x3 abs_b = trans.getBasis().absolute();  
		btVector3 center = trans(localCenter);

		btVector3 extent = btVector3(abs_b[0].dot(localHalfExtents),
			   abs_b[1].dot(localHalfExtents),
			  abs_b[2].dot(localHalfExtents));

		aabbMinOut = center-extent;
		aabbMaxOut = center+extent;
#endif
}

#define USE_BANCHLESS 1
#ifdef USE_BANCHLESS
	//This block replaces the block below and uses no branches, and replaces the 8 bit return with a 32 bit return for improved performance (~3x on XBox 360)
	SIMD_FORCE_INLINE unsigned testQuantizedAabbAgainstQuantizedAabb(const unsigned short int* aabbMin1,const unsigned short int* aabbMax1,const unsigned short int* aabbMin2,const unsigned short int* aabbMax2)
	{		
		return static_cast<unsigned int>(btSelect((unsigned)((aabbMin1[0] <= aabbMax2[0]) & (aabbMax1[0] >= aabbMin2[0])
			& (aabbMin1[2] <= aabbMax2[2]) & (aabbMax1[2] >= aabbMin2[2])
			& (aabbMin1[1] <= aabbMax2[1]) & (aabbMax1[1] >= aabbMin2[1])),
			1, 0));
	}
#else
	SIMD_FORCE_INLINE bool testQuantizedAabbAgainstQuantizedAabb(const unsigned short int* aabbMin1,const unsigned short int* aabbMax1,const unsigned short int* aabbMin2,const unsigned short int* aabbMax2)
	{
		bool overlap = true;
		overlap = (aabbMin1[0] > aabbMax2[0] || aabbMax1[0] < aabbMin2[0]) ? false : overlap;
		overlap = (aabbMin1[2] > aabbMax2[2] || aabbMax1[2] < aabbMin2[2]) ? false : overlap;
		overlap = (aabbMin1[1] > aabbMax2[1] || aabbMax1[1] < aabbMin2[1]) ? false : overlap;
		return overlap;
	}
#endif //USE_BANCHLESS

#endif


