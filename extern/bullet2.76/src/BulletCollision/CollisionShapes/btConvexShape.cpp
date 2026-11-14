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

#include "btConvexShape.h"
#include "btTriangleShape.h"
#include "btSphereShape.h"
#include "btCylinderShape.h"
#include "btCapsuleShape.h"
#include "btConvexHullShape.h"
#include "btConvexPointCloudShape.h"

///not supported on IBM SDK, until we fix the alignment of btVector3

btConvexShape::btConvexShape ()
{
}

btConvexShape::~btConvexShape()
{

}



static btVector3 convexHullSupport (const btVector3& localDirOrg, const btVector3* points, int numPoints, const btVector3& localScaling)
{	

	btVector3 vec = localDirOrg * localScaling;

#ifdef BT_PS3PPU_VEC 
	//btVector3::btHitPrint(68);
	vec_float4 localDir = vec.get128();

	__declspec(align(16)) static const int idxMax[4] = {-999,0,0,0};
	__declspec(align(16)) vec_float4 v_distMax = {-FLT_MAX,0,0,0};

	vec_uint4 v_idxMax = *(vec_uint4*)&idxMax;

	int numverts = numPoints & ~3;

	__declspec(align(16)) static const int _i0[4] = {0,1,2,3};
	__declspec(align(16)) static const int _iadd[4] = {4,4,4,4};
	vec_uint4 i0 = *(vec_uint4*)&_i0;
	vec_uint4 iadd = *(vec_uint4*)&_iadd;
	int v = 0;
	vec_float4* vpoints = (vec_float4*)points;
	assert(((uintptr_t)vpoints & 0xf) == 0);
	for(; v < numverts; v += 4)
	{
		vec_float4 p0 = vec_dot3(vec_lvlx((v + 0) * 16, vpoints), localDir);
		vec_float4 p1 = vec_dot3(vec_lvlx((v + 1) * 16, vpoints), localDir);
		vec_float4 p2 = vec_dot3(vec_lvlx((v + 2) * 16, vpoints), localDir);
		vec_float4 p3 = vec_dot3(vec_lvlx((v + 3) * 16, vpoints), localDir);
		vector bool retGt01 = vec_cmpgt(p0, p1); 
		vec_float4 pmax01 = vec_sel(p1, p0, retGt01);
		vec_uint4 imax01 = vec_sel(vec_splat(i0, 1), i0, retGt01);
		vector bool retGt23 = vec_cmpgt(p2, p3);
		vec_float4 pmax23 = vec_sel(p3, p2, retGt23);
		vec_uint4 imax23 = vec_sel(vec_splat(i0, 3), vec_splat(i0, 2), retGt23);
		vector bool retGt0123 = vec_cmpgt(pmax01, pmax23);
		vec_float4 pmax0123 = vec_sel(pmax23, pmax01, retGt0123);
		vec_uint4 imax0123 = vec_sel(imax23, imax01, retGt0123);
		vector bool retGtMax = vec_cmpgt(v_distMax, pmax0123);
		v_distMax = vec_sel(pmax0123, v_distMax, retGtMax);
		v_idxMax = vec_sel(imax0123, v_idxMax, retGtMax);
		i0 = vec_add(i0, iadd);
	}
	
	if(numPoints & 3)
	{
		vec_uint4 iadd = vec_splats(1u);

		for(;v<numPoints;v++)
		{
			vec_float4 p = vec_dot3(vec_lvlx(v * 16, vpoints), localDir);
			vector bool  retGtMax = vec_cmpgt(v_distMax,p);
			v_distMax = vec_sel(p, v_distMax, retGtMax);
			v_idxMax = vec_sel(i0, v_idxMax, retGtMax);
			i0 = vec_add(i0, iadd);
		}
	}
	int ptIndex = v_idxMax[0];
	btAssert(ptIndex < numPoints);
	const btVector3& supVec= points[ptIndex] * localScaling;
	return supVec;
#elif defined(BT_USE_VMX128)

	__vector4 localDir = __lvewx(&vec,0);

	__declspec(align(16)) static const int idxMax[4] = {-999,0,0,0};
	__declspec(align(16)) __vector4 v_distMax = {-FLT_MAX,0,0,0};

	__vector4 v_idxMax = *(__vector4*)&idxMax;

	int numverts = numPoints & ~3;

	__declspec(align(16)) static const int _i0[4] = {0,1,2,3};
	__declspec(align(16)) static const int _iadd[4] = {4,4,4,4};
	__vector4 i0 = *(__vector4*)&_i0;
	__vector4 iadd = *(__vector4*)&_iadd;
	int v = 0;
	for(; v < numverts; v += 4)
	{
		__vector4 p0 = __vmsum3fp(__lvx(points, (v + 0) * 16),localDir);
		__vector4 p1 = __vmsum3fp(__lvx(points, (v + 1) * 16),localDir);
		__vector4 p2 = __vmsum3fp(__lvx(points, (v + 2) * 16),localDir);
		__vector4 p3 = __vmsum3fp(__lvx(points, (v + 3) * 16),localDir);
		__vector4 retGt01 = __vcmpgtfp(p0,p1);
		__vector4 pmax01 = __vsel(p1,p0,retGt01);
		__vector4 imax01 = __vsel(__vspltw(i0,1),i0,retGt01);
		__vector4 retGt23 = __vcmpgtfp(p2,p3);
		__vector4 pmax23 = __vsel(p3,p2,retGt23);
		__vector4 imax23 = __vsel(__vspltw(i0,3),__vspltw(i0,2),retGt23);
		__vector4 retGt0123 = __vcmpgtfp(pmax01,pmax23);
		__vector4 pmax0123 = __vsel(pmax23,pmax01,retGt0123);
		__vector4 imax0123 = __vsel(imax23,imax01,retGt0123);
		__vector4 retGtMax = __vcmpgtfp(v_distMax,pmax0123);
		v_distMax = __vsel(pmax0123,v_distMax,retGtMax);
		v_idxMax = __vsel(imax0123,v_idxMax,retGtMax);
		i0 = __vadduws(i0, iadd);
	}
	
	if(numPoints & 3)
	{
		//i0 = __vspltw(i0, 0);
		__vector4 iadd = __vspltisw(1);

		for(;v<numPoints;v++)
		{
			__vector4 p = __vmsum3fp(__lvx(points,v * 16),localDir);
			__vector4  retGtMax = __vcmpgtfp(v_distMax,p);
			v_distMax = __vsel(p,v_distMax,retGtMax);
			v_idxMax = __vsel(i0,v_idxMax,retGtMax);
			i0 = __vadduws(i0, iadd);
		}
	}
	int ptIndex = v_idxMax.u[0];
	btAssert(ptIndex < numPoints);
	const btVector3& supVec= points[ptIndex] * localScaling;
	return supVec;
#else
	btScalar newDot,maxDot = btScalar(-BT_LARGE_FLOAT);
	int ptIndex = -1;

	for (int i=0;i<numPoints;i++)
	{

		newDot = vec.dot(points[i]);
		if (newDot > maxDot)
		{
			maxDot = newDot;
			ptIndex = i;
		}
	}
	btAssert(ptIndex >= 0);
	btVector3 supVec = points[ptIndex] * localScaling;
	return supVec;
#endif //__CELLOS_LV2__
}

btVector3 btConvexShape::localGetSupportVertexWithoutMarginNonVirtual (const btVector3& localDir) const
{
	switch (m_shapeType)
	{
    case SPHERE_SHAPE_PROXYTYPE:
	{
		return btZeroVec();
   }
	case BOX_SHAPE_PROXYTYPE:
	{
		btBoxShape* convexShape = (btBoxShape*)this;
		const btVector3& halfExtents = convexShape->getImplicitShapeDimensions();

#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(69);
		return btVector3(vec_sel(vec_sub(btZeroVec(), halfExtents.mVec), halfExtents.mVec, vec_cmpge(localDir.mVec, btZeroVec())));
#else
		return btVector3(btFsels(localDir.getX(), halfExtents.getX(), -halfExtents.getX()),
			btFsels(localDir.getY(), halfExtents.getY(), -halfExtents.getY()),
			btFsels(localDir.getZ(), halfExtents.getZ(), -halfExtents.getZ()));
#endif
	}
	case TRIANGLE_SHAPE_PROXYTYPE:
	{
		btTriangleShape* triangleShape = (btTriangleShape*)this;

#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(70);
		vec_float4 vlocalDir = localDir.get128();
		vec_float4 vert0 = (triangleShape->m_vertices1)->get128();//vec_lvlx(0 * sizeof(btVector3), (vec_float4*)&triangleShape->m_vertices1));
		vec_float4 vert1 = vec_lvlx(1 * sizeof(btVector3), (vec_float4*)&triangleShape->m_vertices1);
		vec_float4 vert2 = vec_lvlx(2 * sizeof(btVector3), (vec_float4*)&triangleShape->m_vertices1);

		vec_float4 dot0 = vec_dot3(vlocalDir, vert0);
		vec_float4 dot1 = vec_dot3(vlocalDir, vert1);
		vec_float4 dot2 = vec_dot3(vlocalDir, vert2);
		
		vector bool vcmp = vec_cmpgt(dot1, dot0);
		vert0 = vec_sel(vert0, vert1, vcmp);
		dot0 = vec_sel(dot0, dot1, vcmp);
		vcmp = vec_cmpgt(dot2, dot0);
		vert0 = vec_sel(vert0, vert2, vcmp);
		
		btVector3 ret;
		ret.set128(vert0);
		return ret;
#elif defined(BT_USE_VMX128)
		__vector4 vlocalDir = __lvewx(&localDir,0);
		__vector4 vert0 = __lvewx(&triangleShape->m_vertices1, 0 * sizeof(btVector3));
		__vector4 vert1 = __lvewx(&triangleShape->m_vertices1, 1 * sizeof(btVector3));
		__vector4 vert2 = __lvewx(&triangleShape->m_vertices1, 2 * sizeof(btVector3));

		__vector4 dot0 = __vmsum3fp(vlocalDir, vert0);
		__vector4 dot1 = __vmsum3fp(vlocalDir, vert1);
		__vector4 dot2 = __vmsum3fp(vlocalDir, vert2);
		__vector4 vcmp = __vcmpgtfp(dot1, dot0);
		vert0 = __vsel(vert0, vert1, vcmp);
		dot0 = __vsel(dot0, dot1, vcmp);
		vcmp = __vcmpgtfp(dot2, dot0);
		vert0 = __vsel(vert0, vert2, vcmp);
		btVector3 ret;
		__stvx(vert0, &ret, 0);
		return ret;
#else
		btVector3 dir(localDir.getX(),localDir.getY(),localDir.getZ());
		btVector3* vertices = &triangleShape->m_vertices1[0];
		btVector3 dots(dir.dot(vertices[0]), dir.dot(vertices[1]), dir.dot(vertices[2]));
		btVector3 sup = vertices[dots.maxAxis()];
		return btVector3(sup.getX(),sup.getY(),sup.getZ());
#endif
	}
	case CYLINDER_SHAPE_PROXYTYPE:
	{
		const btCylinderShape* cylShape = (btCylinderShape*)this;
		//mapping of halfextents/dimension onto radius/height depends on how cylinder local orientation is (upAxis)
		btVector3 halfExtents = cylShape->getImplicitShapeDimensions();
#ifdef BT_PS3PPU_VEC
		btVector3 v = localDir;
		int cylinderUpAxis = cylShape->getUpAxis();
		int XX(1),YY(cylinderUpAxis),ZZ(2);

		switch (cylinderUpAxis)
		{
		case 0:
		break;
		case 1:
		{
			XX = 0;
			ZZ = 2;	
		}
		break;
		case 2:
		{
			XX = 0;
			ZZ = 1;
			
		}
		break;
		default:
			btAssert(0);
		break;
		};

		btScalar radius = halfExtents.get(XX);
		btScalar halfHeight = halfExtents.get(cylinderUpAxis);

		btVector3 tmp = v * v;
		
		btScalar s = btSqrt(tmp.get(XX) + tmp.get(ZZ));
		if (s != btScalar(0.0))
		{
			btScalar d = radius / s;  
			tmp = v * d;
			tmp.set(YY, v.get(YY) < 0.0f ? -halfHeight : halfHeight);
			return tmp;
		} 
		else 
		{
			tmp.set(XX, radius);
			tmp.set(YY, v.get(YY) < 0.0f ? -halfHeight : halfHeight);
			tmp.set(ZZ, 0.0f);
			return tmp;
		}
#else
		btVector3 v = localDir;
		int cylinderUpAxis = cylShape->getUpAxis();
		int XX(1),YY(0),ZZ(2);

		switch (cylinderUpAxis)
		{
		case 0:
		{
			XX = 1;
			YY = 0;
			ZZ = 2;
		}
		break;
		case 1:
		{
			XX = 0;
			YY = 1;
			ZZ = 2;	
		}
		break;
		case 2:
		{
			XX = 0;
			YY = 2;
			ZZ = 1;
			
		}
		break;
		default:
			btAssert(0);
		break;
		};

		btScalar radius = halfExtents.get(XX);
		btScalar halfHeight = halfExtents.get(cylinderUpAxis);

		btVector3 tmp = v * v;
		btScalar d ;

		btScalar s = btSqrt(tmp.get(XX) + tmp.get(ZZ));
		if (s != btScalar(0.0))
		{
			d = radius / s;  
			tmp = v * d;
			tmp.set(YY, v.get(YY) < 0.0f ? -halfHeight : halfHeight);
			return tmp;
		} 
		else 
		{
			tmp.set(XX, radius);
			tmp.set(YY, v.get(YY) < 0.0f ? -halfHeight : halfHeight);
			tmp.set(ZZ, 0.0f);
			return tmp;
		}
#endif
	}
	case CAPSULE_SHAPE_PROXYTYPE:
	{
		const btCapsuleShape* capsuleShape = (const btCapsuleShape*)this;

#ifdef BT_PS3PPU_VEC 
		//btVector3::btHitPrint(71);
		vec_float4 vlocalDir = localDir.get128();
		vec_float4 vlength2 = vec_dot3(vlocalDir, vlocalDir);	
		__declspec(align(16)) static const float vepsilon[4] = {0.0001f, 0.0001f, 0.0001f, 0.0001f};
		__declspec(align(16)) static const float vup[4] = {1.0f, 0.0f, 0.0f, 0.0f};
		__declspec(align(16)) static const int maskX[4] = {0xffffffff, 0, 0, 0};
		__declspec(align(16)) static const int maskY[4] = {0, 0xffffffff, 0, 0};
		__declspec(align(16)) static const int maskZ[4] = {0, 0, 0xffffffff, 0};
		vec_float4 veps = *(vec_float4*)vepsilon;
	
		vector bool vcmp = vec_cmpgt(vlength2, veps);
		vlocalDir = vec_sel(*(vec_float4*)vup, vec_madd(vlocalDir, vec_rsqrte(vlength2), btZeroVec()), vcmp);

		vec_float4 vdims = capsuleShape->getImplicitShapeDimensions().get128();	
		vec_float4 vlocalScaling = capsuleShape->getLocalScalingNV().get128();

		__declspec(align(16)) float vmargin[4] = {capsuleShape->getMarginNV(),capsuleShape->getMarginNV(),capsuleShape->getMarginNV(),0};

		vec_float4 vhalfHeight,vradius;
		vradius = btZeroVec();
		switch(capsuleShape->getUpAxis())
		{
		case 0:
			vhalfHeight = vec_and(vdims, *(vec_float4*)maskX);
			vradius = vec_splat(vdims, 1);	
			break;
		case 1:
			vhalfHeight = vec_and(vdims, *(vec_float4*)maskY);
			vradius = vec_splat(vdims, 2);
			break;
		case 2:
			vhalfHeight = vec_and(vdims, *(vec_float4*)maskZ);
			vradius = vec_splat(vdims, 0);
			break;
		};
		vec_float4 vtemp = vec_madd(vec_madd(vlocalDir, vlocalScaling, btZeroVec()), vradius, btZeroVec());
		vtemp = vec_sub(vtemp, vec_madd(vlocalDir, *(vec_float4*)vmargin, btZeroVec()));
		
		vec_float4 vvtx1 = vec_add(vtemp, vhalfHeight);
		vec_float4 vvtx2 = vec_sub(vtemp, vhalfHeight);
		vec_float4 dot1 = vec_dot3(vlocalDir, vvtx1);
		vec_float4 dot2 = vec_dot3(vlocalDir, vvtx2);
		vcmp = vec_cmpgt(dot1, dot2);

		vtemp = vec_sel(vvtx2, vvtx1, vcmp);
		btVector3 ret;
		ret.set128(vtemp);
		return ret;
#elif defined(BT_USE_VMX128)
	
		__vector4 vlocalDir = __lvewx(&localDir,0);
		__vector4 vlength2 = __vmsum3fp(vlocalDir, vlocalDir);
		__declspec(align(16)) static const float vepsilon[4] = {0.0001f, 0.0001f, 0.0001f, 0.0001f};
		__declspec(align(16)) static const float vup[4] = {1.0f, 0.0f, 0.0f, 0.0f};
		__declspec(align(16)) static const int maskX[4] = {0xffffffff, 0, 0, 0};
		__declspec(align(16)) static const int maskY[4] = {0, 0xffffffff, 0, 0};
		__declspec(align(16)) static const int maskZ[4] = {0, 0, 0xffffffff, 0};
		__vector4 veps = *(__vector4*)vepsilon;
	
		__vector4 vcmp = __vcmpgtfp(vlength2, veps);
		vlocalDir = __vsel(*(__vector4*)vup, __vmulfp(vlocalDir, __vrsqrtefp(vlength2)), vcmp);

		__vector4 vdims = __lvewx(&capsuleShape->getImplicitShapeDimensions(),0);
		__vector4 vlocalScaling = __lvewx(&capsuleShape->getLocalScalingNV(),0);

		__declspec(align(16)) float vmargin[4] = {capsuleShape->getMarginNV(),capsuleShape->getMarginNV(),capsuleShape->getMarginNV(),0};

		__vector4 vhalfHeight,vradius;
		switch(capsuleShape->getUpAxis())
		{
		case 0:
			vhalfHeight = __vand(vdims, *(__vector4*)maskX);
			vradius = __vspltw(vdims, 1);
			break;
		case 1:
			vhalfHeight = __vand(vdims, *(__vector4*)maskY);
			vradius = __vspltw(vdims, 2);
			break;
		case 2:
			vhalfHeight = __vand(vdims, *(__vector4*)maskZ);
			vradius = __vspltw(vdims, 0);
			break;
		};
		__vector4 vtemp = __vmulfp(__vmulfp(vlocalDir, vlocalScaling), vradius);
		vtemp = __vsubfp(vtemp, __vmulfp(vlocalDir, *(__vector4*)vmargin));

		__vector4 vvtx1 = __vaddfp(vtemp, vhalfHeight);
		__vector4 vvtx2 = __vsubfp(vtemp, vhalfHeight);
		__vector4 dot1 = __vmsum3fp(vlocalDir, vvtx1);
		__vector4 dot2 = __vmsum3fp(vlocalDir, vvtx2);
		vcmp = __vcmpgtfp(dot1, dot2);

		vtemp = __vsel(vvtx2, vvtx1, vcmp);
		btVector3 ret;
		__stvx(vtemp, &ret, 0);
		return ret;
#endif
		//btVector3 vec0(localDir.getX(),localDir.getY(),localDir.getZ());
		btVector3 vec0 = localDir;

		btScalar halfHeight = capsuleShape->getHalfHeight();
		int capsuleUpAxis = capsuleShape->getUpAxis();

		btScalar radius = capsuleShape->getRadius();

		btVector3 vec = vec0;
		btScalar lenSqr = vec.length2();
		if (lenSqr < btScalar(0.0001))
		{
			vec.setValue(1,0,0);
		} else
		{
			btScalar rlen = btScalar(1.) / btSqrt(lenSqr );
			vec *= rlen;
		}
		btVector3 pos(capsuleUpAxis == 0 ? halfHeight : btScalar(0.f), capsuleUpAxis == 1 ? halfHeight : btScalar(0.f), capsuleUpAxis == 2 ? halfHeight : btScalar(0.f));
		btVector3 temp = vec*capsuleShape->getLocalScalingNV()*(radius) - vec * capsuleShape->getMarginNV();

		//vtx = pos +vec*(radius);
		btVector3 vtx1 = pos + temp;
		btVector3 vtx2 = -pos + temp;
		return (vec.dot(vtx1) > vec.dot(vtx2)) ? vtx1 : vtx2;
	}
	case CONVEX_POINT_CLOUD_SHAPE_PROXYTYPE:
	{
		btConvexPointCloudShape* convexPointCloudShape = (btConvexPointCloudShape*)this;
		btVector3* points = convexPointCloudShape->getUnscaledPoints ();
		int numPoints = convexPointCloudShape->getNumPoints ();
		return convexHullSupport (localDir, points, numPoints,convexPointCloudShape->getLocalScalingNV());
	}
	case CONVEX_HULL_SHAPE_PROXYTYPE:
	{
		btConvexHullShape* convexHullShape = (btConvexHullShape*)this;
		btVector3* points = convexHullShape->getUnscaledPoints();
		int numPoints = convexHullShape->getNumPoints ();
		return convexHullSupport (localDir, points, numPoints,convexHullShape->getLocalScalingNV());
	}
    default:
#ifndef __SPU__
		return this->localGetSupportingVertexWithoutMargin (localDir);
#else
		btAssert (0);
#endif
	}

	// should never reach here
	btAssert (0);
	return btVector3 (btZeroVec());
}

btVector3 btConvexShape::localGetSupportVertexNonVirtual (const btVector3& localDir) const
{
	btVector3 localDirNorm = localDir;
	if (localDirNorm .length2() < (SIMD_EPSILON*SIMD_EPSILON))
	{
		localDirNorm.setValue(btScalar(-1.),btScalar(-1.),btScalar(-1.));
	}
	localDirNorm.normalize ();

	return localGetSupportVertexWithoutMarginNonVirtual(localDirNorm)+ getMarginNonVirtual() * localDirNorm;
}

/* TODO: This should be bumped up to btCollisionShape () */
btScalar btConvexShape::getMarginNonVirtual () const
{
	switch (m_shapeType)
	{
    case SPHERE_SHAPE_PROXYTYPE:
	{
		btSphereShape* sphereShape = (btSphereShape*)this;
		return sphereShape->getRadius ();
	}
	case BOX_SHAPE_PROXYTYPE:
	{
		btBoxShape* convexShape = (btBoxShape*)this;
		return convexShape->getMarginNV ();
	}
	case TRIANGLE_SHAPE_PROXYTYPE:
	{
		btTriangleShape* triangleShape = (btTriangleShape*)this;
		return triangleShape->getMarginNV ();
	}
	case CYLINDER_SHAPE_PROXYTYPE:
	{
		btCylinderShape* cylShape = (btCylinderShape*)this;
		return cylShape->getMarginNV();
	}
	case CAPSULE_SHAPE_PROXYTYPE:
	{
		btCapsuleShape* capsuleShape = (btCapsuleShape*)this;
		return capsuleShape->getMarginNV();
	}
	case CONVEX_POINT_CLOUD_SHAPE_PROXYTYPE:
	/* fall through */
	case CONVEX_HULL_SHAPE_PROXYTYPE:
	{
		btPolyhedralConvexShape* convexHullShape = (btPolyhedralConvexShape*)this;
		return convexHullShape->getMarginNV();
	}
    default:
#ifndef __SPU__
		return this->getMargin ();
#else
		btAssert (0);
#endif
	}

	// should never reach here
	btAssert (0);
	return btScalar(0.0f);
}
#ifndef __SPU__
void btConvexShape::getAabbNonVirtual (const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
{
	switch (m_shapeType)
	{
    case SPHERE_SHAPE_PROXYTYPE:
	{
		btSphereShape* sphereShape = (btSphereShape*)this;
		btScalar radius = sphereShape->getImplicitShapeDimensions().getX();// * convexShape->getLocalScaling().getX();
		btScalar margin = radius + sphereShape->getMarginNonVirtual();
		const btVector3& center = t.getOrigin();
		btVector3 extent(margin);
		aabbMin = center - extent;
		aabbMax = center + extent;
    }
	break;
	case CYLINDER_SHAPE_PROXYTYPE:
	/* fall through */
	case BOX_SHAPE_PROXYTYPE:
	{
		btBoxShape* convexShape = (btBoxShape*)this;
		btScalar margin=convexShape->getMarginNonVirtual();
		btVector3 halfExtents = convexShape->getImplicitShapeDimensions();
		halfExtents += btVector3(margin,margin,margin);
		btMatrix3x3 abs_b = t.getBasis().absolute();  
		btVector3 center = t.getOrigin();
#ifdef BT_PS3PPU_VEC 
		//btVector3::btHitPrint(72);
		btVector3 extent = btVector3(vec_dot3mx(abs_b.get128ptr(), halfExtents.mVec));
#else
		btVector3 extent = btVector3(abs_b[0].dot(halfExtents),abs_b[1].dot(halfExtents),abs_b[2].dot(halfExtents));
#endif

		aabbMin = center - extent;
		aabbMax = center + extent;
		break;
	}
	case TRIANGLE_SHAPE_PROXYTYPE:
	{
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(178);
		////printf("optimalizovat BT_PS3PPU_VEC to check %s %i\n", __FILE__, __LINE__);
		btTriangleShape* triangleShape = (btTriangleShape*)this;
		btScalar margin = triangleShape->getMarginNonVirtual();
		for (int i=0;i<3;i++)
		{
			btVector3 vec = btZeroVec();
			vec.set(i, btScalar(1.));

			btVector3 tmp = t(localGetSupportVertexWithoutMarginNonVirtual(vec*t.getBasis()));
			aabbMax.set(i, tmp.get(i) + margin);
			
			vec.set(i, btScalar(-1.));
			tmp = t(localGetSupportVertexWithoutMarginNonVirtual(vec*t.getBasis()));
			aabbMin.set(i, tmp.get(i) - margin);
		}	
#else
		btTriangleShape* triangleShape = (btTriangleShape*)this;
		btScalar margin = triangleShape->getMarginNonVirtual();
		for (int i=0;i<3;i++)
		{
			btVector3 vec(btScalar(0.),btScalar(0.),btScalar(0.));
			vec[i] = btScalar(1.);

			btVector3 sv = localGetSupportVertexWithoutMarginNonVirtual(vec*t.getBasis());

			btVector3 tmp = t(sv);
			aabbMax[i] = tmp[i]+margin;
			vec[i] = btScalar(-1.);
			tmp = t(localGetSupportVertexWithoutMarginNonVirtual(vec*t.getBasis()));
			aabbMin[i] = tmp[i]-margin;
		}	
#endif
	}
	break;
	case CAPSULE_SHAPE_PROXYTYPE:
	{
		btCapsuleShape* capsuleShape = (btCapsuleShape*)this;
#ifdef BT_PS3PPU_VEC 
		//btVector3::btHitPrint(73);
		btVector3 halfExtents(capsuleShape->getRadius());
		int m_upAxis = capsuleShape->getUpAxis();
		halfExtents.set(m_upAxis, capsuleShape->getRadius() + capsuleShape->getHalfHeight());
		halfExtents += btVector3(capsuleShape->getMarginNonVirtual());
		btMatrix3x3 abs_b = t.getBasis().absolute();  
		btVector3 center = t.getOrigin();
		btVector3 extent = btVector3(vec_dot3mx(abs_b.get128ptr(), halfExtents.mVec));		  	
		aabbMin = center - extent;
		aabbMax = center + extent;
#else
		btVector3 halfExtents(capsuleShape->getRadius(),capsuleShape->getRadius(),capsuleShape->getRadius());
		int m_upAxis = capsuleShape->getUpAxis();
		halfExtents[m_upAxis] = capsuleShape->getRadius() + capsuleShape->getHalfHeight();
		halfExtents += btVector3(capsuleShape->getMarginNonVirtual(),capsuleShape->getMarginNonVirtual(),capsuleShape->getMarginNonVirtual());
		btMatrix3x3 abs_b = t.getBasis().absolute();  
		btVector3 center = t.getOrigin();
		btVector3 extent = btVector3(abs_b[0].dot(halfExtents),abs_b[1].dot(halfExtents),abs_b[2].dot(halfExtents));		  	
		aabbMin = center - extent;
		aabbMax = center + extent;
#endif
	}
	break;
	case CONVEX_POINT_CLOUD_SHAPE_PROXYTYPE:
	case CONVEX_HULL_SHAPE_PROXYTYPE:
	{
		btPolyhedralConvexAabbCachingShape* convexHullShape = (btPolyhedralConvexAabbCachingShape*)this;
		btScalar margin = convexHullShape->getMarginNonVirtual();
		convexHullShape->getNonvirtualAabb (t, aabbMin, aabbMax, margin);
	}
	break;
    default:
#ifndef __SPU__
		this->getAabb (t, aabbMin, aabbMax);
#else
		btAssert (0);
#endif
	break;
	}

	// should never reach here
	btAssert (0);
}

#endif //__SPU__
