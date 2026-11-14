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


#include "btConvexInternalShape.h"

btConvexInternalShape::btConvexInternalShape()
: 
m_localScaling(1.f),
m_collisionMargin(CONVEX_DISTANCE_MARGIN)
{
}


void	btConvexInternalShape::setLocalScaling(const btVector3& scaling)
{
	m_localScaling = scaling.absolute();
}



void	btConvexInternalShape::getAabbSlow(const btTransform& trans,btVector3&minAabb,btVector3&maxAabb) const
{
#ifndef __SPU__
#ifdef BT_PS3PPU_VEC8	//upravovat slow?
	//btVector3::btHitPrint(66);
	//use localGetSupportingVertexWithoutMargin?
	btScalar margin = getMargin();
	
	btVector3 vec = btIdentVec0;
	vec_float4 tmpMax = (trans(localGetSupportingVertex(vec*trans.getBasis()))).mVec;
	vec = btZeroVec() - vec;
	vec_float4 tmpMin = (trans(localGetSupportingVertex(vec*trans.getBasis()))).mVec;

	vec = btIdentVec1;
	tmpMax = vec_sel(tmpMax, (trans(localGetSupportingVertex(vec*trans.getBasis()))).mVec, btSelVec1);
	vec = btZeroVec() - vec;
	tmpMin = vec_sel(tmpMin, (trans(localGetSupportingVertex(vec*trans.getBasis()))).mVec, btSelVec1);

	vec = btIdentVec2;
	maxAabb.mVec = vec_sel(tmpMax, (trans(localGetSupportingVertex(vec*trans.getBasis()))).mVec, btSelVec2);
	vec = btZeroVec() - vec;
	minAabb.mVec = vec_sel(tmpMin, (trans(localGetSupportingVertex(vec*trans.getBasis()))).mVec, btSelVec2);
#else
	//use localGetSupportingVertexWithoutMargin?
	btScalar margin = getMargin();
	for (int i=0;i<3;i++)
	{
		btVector3 vec(btScalar(0.),btScalar(0.),btScalar(0.));
		vec[i] = btScalar(1.);

		btVector3 sv = localGetSupportingVertex(vec*trans.getBasis());

		btVector3 tmp = trans(sv);
		maxAabb[i] = tmp[i]+margin;
		vec[i] = btScalar(-1.);
		tmp = trans(localGetSupportingVertex(vec*trans.getBasis()));
		minAabb[i] = tmp[i]-margin;
	}
#endif
#endif
}



btVector3	btConvexInternalShape::localGetSupportingVertex(const btVector3& vec)const
{
#ifndef __SPU__
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(67);
	vec_float4 supVertex = localGetSupportingVertexWithoutMargin(vec).get128();

	if(getMarginNV() != btScalar(0.))
	{
		vec_float4 vecnorm = vec.get128();
		vec_float4 len2 = vec_dot3(vecnorm, vecnorm);
		if (len2 < vec_float4(SIMD_EPSILON*SIMD_EPSILON))
		{
			vecnorm = vec_float4(-1.f);//.setValue(btScalar(-1.),btScalar(-1.),btScalar(-1.));
		} 
		vecnorm = vec_madd(vecnorm, vec_rsqrte(len2), btZeroVec());	//vecnorm.normalize();
		supVertex = vec_madd(vec_float4(getMarginNV()), vecnorm, supVertex);	//supVertex+= getMarginNV() * vecnorm;
	}

	btVector3 ret(supVertex);
	return ret;
#else
	btVector3 supVertex = localGetSupportingVertexWithoutMargin(vec);

	if ( getMarginNV()!=btScalar(0.) )
	{
		btVector3 vecnorm = vec;
		if (vecnorm .length2() < (SIMD_EPSILON*SIMD_EPSILON))
		{
			vecnorm.setValue(btScalar(-1.),btScalar(-1.),btScalar(-1.));
		} 
		vecnorm.normalize();
		supVertex+= getMarginNV() * vecnorm;
	}
	return supVertex;
#endif
#else
	btAssert(0);
	return btZeroVec();
#endif //__SPU__

 }


btConvexInternalAabbCachingShape::btConvexInternalAabbCachingShape()
	:	btConvexInternalShape(),
m_localAabbMin(1,1,1),
m_localAabbMax(-1,-1,-1),
m_isLocalAabbValid(false)
{
}


void btConvexInternalAabbCachingShape::getAabb(const btTransform& trans,btVector3& aabbMin,btVector3& aabbMax) const
{
	getNonvirtualAabb(trans,aabbMin,aabbMax,getMargin());
}

void	btConvexInternalAabbCachingShape::setLocalScaling(const btVector3& scaling)
{
	btConvexInternalShape::setLocalScaling(scaling);
	recalcLocalAabb();
}


void	btConvexInternalAabbCachingShape::recalcLocalAabb()
{
	m_isLocalAabbValid = true;
	
	#if 1
	static const btVector3 _directions[] =
	{
		btVector3( 1.,  0.,  0.),
		btVector3( 0.,  1.,  0.),
		btVector3( 0.,  0.,  1.),
		btVector3( -1., 0.,  0.),
		btVector3( 0., -1.,  0.),
		btVector3( 0.,  0., -1.)
	};
	
	btVector3 _supporting[] =
	{
		btVector3( 0., 0., 0.),
		btVector3( 0., 0., 0.),
		btVector3( 0., 0., 0.),
		btVector3( 0., 0., 0.),
		btVector3( 0., 0., 0.),
		btVector3( 0., 0., 0.)
	};
	
	batchedUnitVectorGetSupportingVertexWithoutMargin(_directions, _supporting, 6);
	
	for ( int i = 0; i < 3; ++i )
	{
		m_localAabbMax[i] = _supporting[i][i] + m_collisionMargin;
		m_localAabbMin[i] = _supporting[i + 3][i] - m_collisionMargin;
	}
	
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
