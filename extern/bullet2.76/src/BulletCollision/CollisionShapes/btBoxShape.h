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

#ifndef OBB_BOX_MINKOWSKI_H
#define OBB_BOX_MINKOWSKI_H

#include "btPolyhedralConvexShape.h"
#include "btCollisionMargin.h"
#include "BulletCollision/BroadphaseCollision/btBroadphaseProxy.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btMinMax.h"

///The btBoxShape is a box primitive around the origin, its sides axis aligned with length specified by half extents, in local shape coordinates. When used as part of a btCollisionObject or btRigidBody it will be an oriented box in world space.
class btBoxShape: public btPolyhedralConvexShape
{

	//btVector3	m_boxHalfExtents1; //use m_implicitShapeDimensions instead


public:

	btVector3 getHalfExtentsWithMargin() const
	{
		btVector3 halfExtents = getHalfExtentsWithoutMargin();
		btVector3 margin(getMarginNV());
		halfExtents += margin;
		return halfExtents;
	}
	
	const btVector3& getHalfExtentsWithoutMargin() const
	{
		return m_implicitShapeDimensions;//scaling is included, margin is not
	}
	

	virtual btVector3	localGetSupportingVertex(const btVector3& vec) const
	{
#ifdef BT_PS3PPU_VEC 
		//btVector3::btHitPrint(24);
		vec_float4 halfExtents = getHalfExtentsWithoutMargin().mVec;
		vec_float4 margin = vec_float4(getMargin());
		halfExtents = vec_add(halfExtents, margin);
		
		//a >= btScalar(0.f) ? b : c;
		vec_bint4 ge = vec_cmpge(vec.mVec, btZeroVec());
		return btVector3(vec_sel(vec_sub(btZeroVec(), halfExtents), halfExtents, ge));
#else
		btVector3 halfExtents = getHalfExtentsWithoutMargin();
		btVector3 margin(getMargin(),getMargin(),getMargin());
		halfExtents += margin;
		
		return btVector3(btFsels(vec.x(), halfExtents.x(), -halfExtents.x()),
			btFsels(vec.y(), halfExtents.y(), -halfExtents.y()),
			btFsels(vec.z(), halfExtents.z(), -halfExtents.z()));
#endif
	}

	SIMD_FORCE_INLINE  btVector3	localGetSupportingVertexWithoutMargin(const btVector3& vec)const
	{
		const btVector3& halfExtents = getHalfExtentsWithoutMargin();
		
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(25);
		//a >= btScalar(0.f) ? b : c;
		vec_bint4 ge = vec_cmpge(vec.mVec, btZeroVec());
		return btVector3(vec_sel(vec_sub(btZeroVec(), halfExtents.mVec), halfExtents.mVec, ge));
#else
		return btVector3(btFsels(vec.x(), halfExtents.x(), -halfExtents.x()),
			btFsels(vec.y(), halfExtents.y(), -halfExtents.y()),
			btFsels(vec.z(), halfExtents.z(), -halfExtents.z()));
#endif
	}

	virtual void	batchedUnitVectorGetSupportingVertexWithoutMargin(const btVector3* vectors,btVector3* supportVerticesOut,int numVectors) const
	{
		const btVector3& halfExtents = getHalfExtentsWithoutMargin();
	
		for (int i=0;i<numVectors;i++)
		{
			const btVector3& vec = vectors[i];
#ifdef BT_PS3PPU_VEC
			//btVector3::btHitPrint(26);
			//a >= btScalar(0.f) ? b : c;
			vec_bint4 ge = vec_cmpge(vec.mVec, btZeroVec());
			supportVerticesOut[i].mVec = vec_sel(vec_sub(btZeroVec(), halfExtents.mVec), halfExtents.mVec, ge);
#else
			supportVerticesOut[i].setValue(btFsels(vec.getX(), halfExtents.getX(), -halfExtents.getX()),
				btFsels(vec.getY(), halfExtents.getY(), -halfExtents.getY()),
				btFsels(vec.getZ(), halfExtents.getZ(), -halfExtents.getZ())); 
#endif
		}

	}

	virtual btScalar getLineIntersection(const btCollisionObject* obj, const btTransform& colObjWorldTransform, const btVector3& from, const btVector3& to, btVector3& contactNormal, const btMaterial*& contactMaterial) const;

	btBoxShape( const btVector3& boxHalfExtents) 
		: btPolyhedralConvexShape()
	{
		m_shapeType = BOX_SHAPE_PROXYTYPE;
		btVector3 margin(getMargin());
		m_implicitShapeDimensions = (boxHalfExtents * m_localScaling) - margin;
	};

	virtual void setMargin(btScalar collisionMargin)
	{
		//correct the m_implicitShapeDimensions for the margin
		btVector3 oldMargin(getMargin());
		btVector3 implicitShapeDimensionsWithMargin = m_implicitShapeDimensions+oldMargin;
		
		btConvexInternalShape::setMargin(collisionMargin);
		btVector3 newMargin(getMargin(),getMargin(),getMargin());
		m_implicitShapeDimensions = implicitShapeDimensionsWithMargin - newMargin;

	}
	virtual void	setLocalScaling(const btVector3& scaling)
	{
		btVector3 oldMargin(getMargin());
		btVector3 implicitShapeDimensionsWithMargin = m_implicitShapeDimensions+oldMargin;
		btVector3 unScaledImplicitShapeDimensionsWithMargin = implicitShapeDimensionsWithMargin / m_localScaling;

		btConvexInternalShape::setLocalScaling(scaling);

		m_implicitShapeDimensions = (unScaledImplicitShapeDimensionsWithMargin * m_localScaling) - oldMargin;

	}

	virtual void getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const;

	

	virtual void	calculateLocalInertia(btScalar mass,btVector3& inertia) const;

	virtual void getPlane(btVector3& planeNormal,btVector3& planeSupport,int i ) const
	{
		//this plane might not be aligned...
		btVector4 plane ;
		getPlaneEquation(plane,i);
#ifdef BT_PS3PPU_VEC 
		//btVector3::btHitPrint(27);
		planeNormal = btVector3(plane.mVec);
#else
		planeNormal = btVector3(plane.getX(),plane.getY(),plane.getZ());
#endif
		planeSupport = localGetSupportingVertex(-planeNormal);
	}

	
	virtual int getNumPlanes() const
	{
		return 6;
	}	
	
	virtual int	getNumVertices() const 
	{
		return 8;
	}

	virtual int getNumEdges() const
	{
		return 12;
	}


	virtual void getVertex(int i,btVector3& vtx) const
	{
		btVector3 halfExtents = getHalfExtentsWithoutMargin();

#ifdef BT_PS3PPU_VEC_INT
		//btVector3::btHitPrint(28);
		vec_bint4 mask = {(i&1), (i&2)>>1, (i&4)>>2, 0};
		printf("mask for i %i: %i, %i, %i\n", i, mask[0], mask[1], mask[2]);
		vtx.mVec = vec_sel(halfExtents.mVec, vec_sub(btZeroVec(), halfExtents.mVec), mask);
#else
		vtx = btVector3(
				halfExtents.getX() * (1-(i&1)) - halfExtents.getX() * (i&1),
				halfExtents.getY() * (1-((i&2)>>1)) - halfExtents.getY() * ((i&2)>>1),
				halfExtents.getZ() * (1-((i&4)>>2)) - halfExtents.getZ() * ((i&4)>>2));
#endif
	}
	

	virtual void	getPlaneEquation(btVector4& plane,int i) const
	{
		btVector3 halfExtents = getHalfExtentsWithoutMargin();

#ifdef BT_PS3PPU_VEC_SLOW
		//btVector3::btHitPrint(29);
		switch (i)
		{
		case 0:
			plane.mVec = vec_sld(btIdentVec1, vec_sub(btZeroVec(), halfExtents.mVec), 4);
			break;
		case 1:
			plane.mVec = vec_sub(btZeroVec(), vec_sld(btIdentVec1, halfExtents.mVec, 4));
			break;
		case 2:
			plane.mVec = vec_sld(btIdentVec2, vec_sub(btZeroVec(), vec_sld(halfExtents.mVec, halfExtents.mVec, 4)), 4);
			break;
		case 3:
			plane.mVec = vec_sub(btZeroVec(), vec_sld(btIdentVec2, vec_sld(halfExtents.mVec, halfExtents.mVec, 4), 4));
			break;
		case 4:
			{
				vec_float4 tmp = vec_sel(btIdentVec1, vec_sub(btZeroVec(), halfExtents.mVec), btSelVec2);
				plane.mVec = vec_sld(tmp, tmp, 4);
			}
			break;
		case 5:
			{
				vec_float4 tmp = vec_sub(btZeroVec(), vec_sel(btIdentVec1, halfExtents.mVec, btSelVec2));
				plane.mVec = vec_sld(tmp, tmp, 4);
			}
			break;
		default:
			btAssert(0);
		}
#else
		switch (i)
		{
		case 0:
			plane.setValue(btScalar(1.),btScalar(0.),btScalar(0.),-halfExtents.getX());
			break;
		case 1:
			plane.setValue(btScalar(-1.),btScalar(0.),btScalar(0.),-halfExtents.getX());
			break;
		case 2:
			plane.setValue(btScalar(0.),btScalar(1.),btScalar(0.),-halfExtents.getY());
			break;
		case 3:
			plane.setValue(btScalar(0.),btScalar(-1.),btScalar(0.),-halfExtents.getY());
			break;
		case 4:
			plane.setValue(btScalar(0.),btScalar(0.),btScalar(1.),-halfExtents.getZ());
			break;
		case 5:
			plane.setValue(btScalar(0.),btScalar(0.),btScalar(-1.),-halfExtents.getZ());
			break;
		default:
			btAssert(0);
		}
#endif
	}

	
	virtual void getEdge(int i,btVector3& pa,btVector3& pb) const
	//virtual void getEdge(int i,Edge& edge) const
	{
		int edgeVert0 = 0;
		int edgeVert1 = 0;

		switch (i)
		{
		case 0:
				edgeVert0 = 0;
				edgeVert1 = 1;
			break;
		case 1:
				edgeVert0 = 0;
				edgeVert1 = 2;
			break;
		case 2:
			edgeVert0 = 1;
			edgeVert1 = 3;

			break;
		case 3:
			edgeVert0 = 2;
			edgeVert1 = 3;
			break;
		case 4:
			edgeVert0 = 0;
			edgeVert1 = 4;
			break;
		case 5:
			edgeVert0 = 1;
			edgeVert1 = 5;

			break;
		case 6:
			edgeVert0 = 2;
			edgeVert1 = 6;
			break;
		case 7:
			edgeVert0 = 3;
			edgeVert1 = 7;
			break;
		case 8:
			edgeVert0 = 4;
			edgeVert1 = 5;
			break;
		case 9:
			edgeVert0 = 4;
			edgeVert1 = 6;
			break;
		case 10:
			edgeVert0 = 5;
			edgeVert1 = 7;
			break;
		case 11:
			edgeVert0 = 6;
			edgeVert1 = 7;
			break;
		default:
			btAssert(0);

		}

		getVertex(edgeVert0,pa );
		getVertex(edgeVert1,pb );
	}




	
	virtual	bool isInside(const btVector3& pt,btScalar tolerance) const
	{
		btVector3 halfExtents = getHalfExtentsWithoutMargin();

		//btScalar minDist = 2*tolerance;
		
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(30);	//checked
		//dont let w mess it up
		vec_float4 tol = vec_add(halfExtents.mVec, vec_float4(tolerance));
		vec_float4 mtol = vec_sub(btZeroVec(), tol);

		vec_int4 tmp = (vec_int4)vec_and(vec_cmple(pt.mVec, tol), vec_cmpge(pt.mVec, mtol));
		bool result = vec_all_ne(vec_sel(tmp, (vec_int4)btSelVec3, btSelVec3), btIntZero);
#else
		bool result =	(pt.x() <= (halfExtents.x()+tolerance)) &&
						(pt.x() >= (-halfExtents.x()-tolerance)) &&
						(pt.y() <= (halfExtents.y()+tolerance)) &&
						(pt.y() >= (-halfExtents.y()-tolerance)) &&
						(pt.z() <= (halfExtents.z()+tolerance)) &&
						(pt.z() >= (-halfExtents.z()-tolerance));
#endif
		
		return result;
	}


	//debugging
	virtual const char*	getName()const
	{
		return "Box";
	}

	virtual int		getNumPreferredPenetrationDirections() const
	{
		return 6;
	}
	
	virtual void	getPreferredPenetrationDirection(int index, btVector3& penetrationVector) const
	{
		switch (index)
		{
		case 0:
			penetrationVector.setValue(btScalar(1.),btScalar(0.),btScalar(0.));
			break;
		case 1:
			penetrationVector.setValue(btScalar(-1.),btScalar(0.),btScalar(0.));
			break;
		case 2:
			penetrationVector.setValue(btScalar(0.),btScalar(1.),btScalar(0.));
			break;
		case 3:
			penetrationVector.setValue(btScalar(0.),btScalar(-1.),btScalar(0.));
			break;
		case 4:
			penetrationVector.setValue(btScalar(0.),btScalar(0.),btScalar(1.));
			break;
		case 5:
			penetrationVector.setValue(btScalar(0.),btScalar(0.),btScalar(-1.));
			break;
		default:
			btAssert(0);
		}
	}

};


#endif //OBB_BOX_MINKOWSKI_H


