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

#include "btTriangleMeshShape.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btQuaternion.h"
#include "btStridingMeshInterface.h"
#include "LinearMath/btAabbUtil2.h"
#include "BulletCollision/CollisionShapes/btCollisionMargin.h"

btTriangleMeshShape::btTriangleMeshShape(btStridingMeshInterface* meshInterface)
: btConcaveShape (), m_meshInterface(meshInterface)
{
	m_shapeType = TRIANGLE_MESH_SHAPE_PROXYTYPE;
	if(meshInterface)
	{
		if(meshInterface->hasPremadeAabb())
		{
			meshInterface->getPremadeAabb(&m_localAabbMin, &m_localAabbMax);
		}
		else
		{
			recalcLocalAabb();
		}
	}
}


btTriangleMeshShape::~btTriangleMeshShape()
{
		
}




void btTriangleMeshShape::getAabb(const btTransform& trans,btVector3& aabbMin,btVector3& aabbMax) const
{
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(91);
	btVector3 half = btVector3(0.5f);
	btVector3 localHalfExtents = half*(m_localAabbMax-m_localAabbMin);
	localHalfExtents += btVector3(getMargin());
	btVector3 localCenter = half*(m_localAabbMax+m_localAabbMin);
	
	btMatrix3x3 abs_b = trans.getBasis().absolute();  

	btVector3 center = trans(localCenter);

	btVector3 extent = btVector3(vec_dot3mx(abs_b.get128ptr(), localHalfExtents.mVec));

	aabbMin = center - extent;
	aabbMax = center + extent;
#else
	btVector3 localHalfExtents = btScalar(0.5)*(m_localAabbMax-m_localAabbMin);
	localHalfExtents += btVector3(getMargin(),getMargin(),getMargin());
	btVector3 localCenter = btScalar(0.5)*(m_localAabbMax+m_localAabbMin);
	
	btMatrix3x3 abs_b = trans.getBasis().absolute();  

	btVector3 center = trans(localCenter);

	btVector3 extent = btVector3(abs_b[0].dot(localHalfExtents),
		   abs_b[1].dot(localHalfExtents),
		  abs_b[2].dot(localHalfExtents));
	aabbMin = center - extent;
	aabbMax = center + extent;
#endif
}

void	btTriangleMeshShape::recalcLocalAabb()
{
#ifdef BT_PS3PPU_VEC 
	//btVector3::btHitPrint(92);
	m_localAabbMax = localGetSupportingVertex(btIdentVec0);	
	m_localAabbMin = localGetSupportingVertex(-btVector3(btIdentVec0));

	m_localAabbMax = vec_sel(m_localAabbMax.mVec, localGetSupportingVertex(btIdentVec1).mVec, btSelVec1);	
	m_localAabbMin = vec_sel(m_localAabbMin.mVec, localGetSupportingVertex(-btVector3(btIdentVec1)).mVec, btSelVec1);

	m_localAabbMax = vec_sel(m_localAabbMax.mVec, localGetSupportingVertex(btIdentVec2).mVec, btSelVec2);	
	m_localAabbMin = vec_sel(m_localAabbMin.mVec, localGetSupportingVertex(-btVector3(btIdentVec2)).mVec, btSelVec2);

	m_localAabbMax = m_localAabbMax + m_collisionMargin;
	m_localAabbMin = m_localAabbMin - m_collisionMargin;
#else
	for (int i=0;i<3;i++)
	{
		btVector3 vec(btScalar(0.),btScalar(0.),btScalar(0.));
		vec[i] = btScalar(1.);
		btVector3 tmp = localGetSupportingVertex(vec);
		m_localAabbMax[i] = tmp[i]+m_collisionMargin;
		vec[i] = btScalar(-1.);
		tmp = localGetSupportingVertex(vec);
		m_localAabbMin[i] = tmp[i]-m_collisionMargin;
	}
#endif
}



class SupportVertexCallback : public btTriangleCallback
{

	btVector3 m_supportVertexLocal;
public:

	btTransform	m_worldTrans;
	btScalar m_maxDot;
	btVector3 m_supportVecLocal;

	SupportVertexCallback(const btVector3& supportVecWorld,const btTransform& trans)
		: m_supportVertexLocal(btScalar(0.),btScalar(0.),btScalar(0.)), m_worldTrans(trans) ,m_maxDot(btScalar(-BT_LARGE_FLOAT))
		
	{
		m_supportVecLocal = supportVecWorld * m_worldTrans.getBasis();
	}

	virtual void processTriangle( btVector3* triangle,int triangleIndex, const btMaterial* mat)
	{
		(void)triangleIndex;
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(100)
		//mult
		vec_float4 vec0 = vec_madd(triangle[0].mVec, m_supportVecLocal.mVec, btZeroVec());
		vec_float4 vec1 = vec_madd(triangle[1].mVec, m_supportVecLocal.mVec, btZeroVec());
		vec_float4 vec2 = vec_madd(triangle[2].mVec, m_supportVecLocal.mVec, btZeroVec());

		//transpose
		vec_float4 tmp = vec_vmrghw(vec0, vec2);	//ax cx ay cy
		vec_float4 mt0 = vec_vmrghw(tmp, vec1); //ax bx cx by
		vec_float4 mt1 = vec_vmrglw(tmp, vec_sld(vec1, vec1, 12));	//ay by cy bz
		vec_float4 mt2 = vec_vmrghw(vec_vmrglw(vec0, vec2), vec_sld(vec1, vec1,	8)); //az bz cz x

		//sum
		btVector3 dot = vec_add(mt2, vec_add(mt0, mt1));
		tmp = vec_max(btZeroVec(), vec_sub(dot.mVec, vec_float4(m_maxDot)));		//zajima me jen co je vetsi jak maxdot
		if(vec_any_lt(tmp, btZeroVec()))
		{
			int axe = dot.maxAxis();
			m_maxDot = dot.get(axe);
			m_supportVertexLocal = triangle[axe];
		}
#else
		for (int i=0;i<3;i++)
		{
			btScalar dot = m_supportVecLocal.dot(triangle[i]);
			if (dot > m_maxDot)
			{
				m_maxDot = dot;
				m_supportVertexLocal = triangle[i];
			}
		}
#endif
	}

	btVector3 GetSupportVertexWorldSpace()
	{
		return m_worldTrans(m_supportVertexLocal);
	}

	btVector3	GetSupportVertexLocal()
	{
		return m_supportVertexLocal;
	}

};

	
void btTriangleMeshShape::setLocalScaling(const btVector3& scaling)
{
	m_meshInterface->setScaling(scaling);
	recalcLocalAabb();
}

const btVector3& btTriangleMeshShape::getLocalScaling() const
{
	return m_meshInterface->getScaling();
}






//#define DEBUG_TRIANGLE_MESH



void	btTriangleMeshShape::processAllTriangles(btTriangleCallback* callback,const btVector3& aabbMin,const btVector3& aabbMax) const
{
		struct FilteredCallback : public btInternalTriangleIndexCallback
	{
		btTriangleCallback* m_callback;
		btVector3 m_aabbMin;
		btVector3 m_aabbMax;

		FilteredCallback(btTriangleCallback* callback,const btVector3& aabbMin,const btVector3& aabbMax)
			:m_callback(callback),
			m_aabbMin(aabbMin),
			m_aabbMax(aabbMax)
		{
		}

		virtual void internalProcessTriangleIndex(btVector3* triangle,int triangleIndex)
		{
			if (TestTriangleAgainstAabb2(&triangle[0],m_aabbMin,m_aabbMax))
			{
				//check aabb in triangle-space, before doing this
				m_callback->processTriangle(triangle,triangleIndex,NULL);
			}
			
		}

	};

	FilteredCallback filterCallback(callback,aabbMin,aabbMax);

	m_meshInterface->InternalProcessAllTriangles(&filterCallback,aabbMin,aabbMax);
}





void	btTriangleMeshShape::calculateLocalInertia(btScalar mass,btVector3& inertia) const
{
	
	btVector3 halfExtents = (m_localAabbMax - m_localAabbMin) * 0.5f;
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(93);
	
	btVector3 l = btScalar(2.) * halfExtents;
	btVector3 l2 = l * l;

	vec_float4 tmp = vec_sld(l2.mVec, l2.mVec, 12);
	const btScalar scaledmass = mass * btScalar(0.08333333);
	inertia = scaledmass * (btVector3(vec_add(vec_sld(tmp, l2.mVec, 12), vec_sld(tmp, l2.mVec, 8))));
#else
	btScalar lx=btScalar(2.)*(halfExtents.x());
	btScalar ly=btScalar(2.)*(halfExtents.y());
	btScalar lz=btScalar(2.)*(halfExtents.z());

	inertia.setValue(mass/(btScalar(12.0)) * (ly*ly + lz*lz),
					mass/(btScalar(12.0)) * (lx*lx + lz*lz),
					mass/(btScalar(12.0)) * (lx*lx + ly*ly));
#endif
/*
	(void)mass;
	//moving concave objects not supported
	btAssert(0);
	inertia.setValue(btScalar(0.),btScalar(0.),btScalar(0.));
	*/
}


btVector3 btTriangleMeshShape::localGetSupportingVertex(const btVector3& vec) const
{
	btVector3 supportVertex;

	btTransform ident;
	ident.setIdentity();

	SupportVertexCallback supportCallback(vec,ident);

	btVector3 aabbMax(btScalar(BT_LARGE_FLOAT),btScalar(BT_LARGE_FLOAT),btScalar(BT_LARGE_FLOAT));
	
	processAllTriangles(&supportCallback,-aabbMax,aabbMax);
		
	supportVertex = supportCallback.GetSupportVertexLocal();

	return supportVertex;
}


