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

#include "btStaticPlaneShape.h"

#include "LinearMath/btTransformUtil.h"

#ifdef BT_PS3PPU_VEC
//#include "../btPs3ppuSpamColision.h"
#endif

btStaticPlaneShape::btStaticPlaneShape(const btVector3& planeNormal,btScalar planeConstant)
: btConcaveShape (), m_planeNormal(planeNormal.normalized()),
m_planeConstant(planeConstant),
#ifdef BT_PS3PPU_VEC
m_localScaling(btZeroVec())
#else
m_localScaling(btScalar(0.),btScalar(0.),btScalar(0.))
#endif
{
	m_shapeType = STATIC_PLANE_PROXYTYPE;
	//	btAssert( btFuzzyZero(m_planeNormal.length() - btScalar(1.)) );
}


btStaticPlaneShape::~btStaticPlaneShape()
{
}



void btStaticPlaneShape::getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const
{
	(void)t;

#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(90);
	aabbMin = btVector3(-BT_LARGE_FLOAT);
	aabbMax = btVector3(BT_LARGE_FLOAT);

	vec_float4 transNormal = (t.getBasis() * m_planeNormal).mVec;

	vec_float4 vecPlaneConstant = vec_float4(m_planeConstant);
	vec_float4 one = vec_float4(1.0f);
	aabbMax.mVec = vec_sel(aabbMax.mVec, vec_add(vecPlaneConstant, t.getOrigin().mVec), vec_cmpeq(transNormal, one));
	aabbMin.mVec = vec_sel(aabbMin.mVec, vec_add(vecPlaneConstant, t.getOrigin().mVec), vec_cmpeq(transNormal, vec_sub(btZeroVec(), one)));
#else

	/*
	btVector3 infvec (btScalar(BT_LARGE_FLOAT),btScalar(BT_LARGE_FLOAT),btScalar(BT_LARGE_FLOAT));

	btVector3 center = m_planeNormal*m_planeConstant;
	aabbMin = center + infvec*m_planeNormal;
	aabbMax = aabbMin;
	aabbMin.setMin(center - infvec*m_planeNormal);
	aabbMax.setMax(center - infvec*m_planeNormal); 
	*/
	aabbMin.setValue(btScalar(-BT_LARGE_FLOAT),btScalar(-BT_LARGE_FLOAT),btScalar(-BT_LARGE_FLOAT));
	aabbMax.setValue(btScalar(BT_LARGE_FLOAT),btScalar(BT_LARGE_FLOAT),btScalar(BT_LARGE_FLOAT));

	btVector3 transNormal = t.getBasis() * m_planeNormal;
	if(transNormal.getX() == 1.0)
	{
		aabbMax.setX(m_planeConstant + t.getOrigin().getX());
	}
	else if(transNormal.getX() == -1.0)
	{
		aabbMin.setX(m_planeConstant + t.getOrigin().getX());
	}
	if(transNormal.getY() == 1.0)
	{
		aabbMax.setY(m_planeConstant + t.getOrigin().getY());
	}
	else if(transNormal.getY() == -1.0)
	{
		aabbMin.setY(m_planeConstant + t.getOrigin().getY());
	}
	if(transNormal.getZ() == 1.0)
	{
		aabbMax.setZ(m_planeConstant + t.getOrigin().getZ());
	}
	else if(transNormal.getZ() == -1.0)
	{
		aabbMin.setZ(m_planeConstant + t.getOrigin().getZ());
	}
#endif
}


btScalar btStaticPlaneShape::getLineIntersection(const btCollisionObject* obj, const btTransform& colObjWorldTransform, const btVector3& from, const btVector3& to, btVector3& contactNormal, const btMaterial*& contactMaterial) const
{
	btVector3 lfrom = colObjWorldTransform.invXform(from);
	btVector3 lto = colObjWorldTransform.invXform(to);

	btScalar dist = m_planeConstant + getMargin();
	btScalar d0 = m_planeNormal.dot(lfrom) - dist;
	btScalar d1 = m_planeNormal.dot(lto) - dist;

	if(d0 >= 0 && d1 >= 0)
	{
		return 1.0;
	}
	contactMaterial = getMaterial();
	contactNormal = m_planeNormal;
	if(d0 < 0 && d1 < 0)
	{
		return 0;
	}
	btScalar frac = d0 / (d0 - d1);
	return frac;
}

void	btStaticPlaneShape::processAllTriangles(btTriangleCallback* callback,const btVector3& aabbMin,const btVector3& aabbMax) const
{

	btVector3 halfExtents = (aabbMax - aabbMin) * btScalar(0.5);
	btScalar radius = halfExtents.length();
	btVector3 center = (aabbMax + aabbMin) * btScalar(0.5);
	
	//this is where the triangles are generated, given AABB and plane equation (normal/constant)

	btVector3 tangentDir0,tangentDir1;

	//tangentDir0/tangentDir1 can be precalculated
	btPlaneSpace1(m_planeNormal,tangentDir0,tangentDir1);

	btVector3 supVertex0,supVertex1;

	btVector3 projectedCenter = center - (m_planeNormal.dot(center) - m_planeConstant)*m_planeNormal;
	
	btVector3 triangle[3];
	triangle[0] = projectedCenter + tangentDir0*radius + tangentDir1*radius;
	triangle[1] = projectedCenter + tangentDir0*radius - tangentDir1*radius;
	triangle[2] = projectedCenter - tangentDir0*radius - tangentDir1*radius;

	callback->processTriangle(triangle,0,NULL);

	triangle[0] = projectedCenter - tangentDir0*radius - tangentDir1*radius;
	triangle[1] = projectedCenter - tangentDir0*radius + tangentDir1*radius;
	triangle[2] = projectedCenter + tangentDir0*radius + tangentDir1*radius;

	callback->processTriangle(triangle,1,NULL);
}

void	btStaticPlaneShape::calculateLocalInertia(btScalar mass,btVector3& inertia) const
{
	(void)mass;

	//moving concave objects not supported
	
	inertia.setValue(btScalar(0.),btScalar(0.),btScalar(0.));
}

void	btStaticPlaneShape::setLocalScaling(const btVector3& scaling)
{
	m_localScaling = scaling;
}
const btVector3& btStaticPlaneShape::getLocalScaling() const
{
	return m_localScaling;
}
