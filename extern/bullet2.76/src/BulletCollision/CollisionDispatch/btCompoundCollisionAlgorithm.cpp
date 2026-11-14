/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "BulletCollision/CollisionDispatch/btCompoundCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletCollision/BroadphaseCollision/btDbvt.h"

#include "BulletCollision/BroadphaseCollision/btOverlappingPairCache.h"
#include "LinearMath/btIDebugDraw.h"
#include "LinearMath/btAabbUtil2.h"
#include "btManifoldResult.h"

btCompoundCollisionAlgorithm::btCompoundCollisionAlgorithm( const btCollisionAlgorithmConstructionInfo& ci,btCollisionObject* body0,btCollisionObject* body1,bool isSwapped)
:btActivatingCollisionAlgorithm(ci,body0,body1),
m_isSwapped(isSwapped),
m_sharedManifold(ci.m_manifold)
{
	m_ownsManifold = false;

	btCollisionObject* colObj = m_isSwapped? body1 : body0;
	btAssert (colObj->getCollisionShape()->isCompound());
	
	btCompoundShape* compoundShape = static_cast<btCompoundShape*>(colObj->getCollisionShape());
	m_compoundShapeRevision = compoundShape->getUpdateRevision();
	
	preallocateChildAlgorithms(body0,body1);
}

void	btCompoundCollisionAlgorithm::preallocateChildAlgorithms(btCollisionObject* body0,btCollisionObject* body1)
{
	btCollisionObject* colObj = m_isSwapped? body1 : body0;
	btCollisionObject* otherObj = m_isSwapped? body0 : body1;
	btAssert (colObj->getCollisionShape()->isCompound());
	
	btCompoundShape* compoundShape = static_cast<btCompoundShape*>(colObj->getCollisionShape());

	int numChildren = compoundShape->getNumChildShapes();
	int i;
	
	m_childCollisionAlgorithms.resize(numChildren);
	for (i=0;i<numChildren;i++)
	{
		if (compoundShape->getDynamicAabbTree())
		{
			m_childCollisionAlgorithms[i] = 0;
		} else
		{
			btCollisionShape* tmpShape = colObj->getCollisionShape();
			btCollisionShape* childShape = compoundShape->getChildShape(i);
			colObj->internalSetTemporaryCollisionShape( childShape );
			m_childCollisionAlgorithms[i] = m_dispatcher->findAlgorithm(colObj,otherObj,m_sharedManifold);
			colObj->internalSetTemporaryCollisionShape( tmpShape );
		}
	}
}

void	btCompoundCollisionAlgorithm::removeChildAlgorithms()
{
	int numChildren = m_childCollisionAlgorithms.size();
	int i;
	for (i=0;i<numChildren;i++)
	{
		if (m_childCollisionAlgorithms[i])
		{
			m_childCollisionAlgorithms[i]->~btCollisionAlgorithm();
			m_dispatcher->freeCollisionAlgorithm(m_childCollisionAlgorithms[i]);
		}
	}
}

btCompoundCollisionAlgorithm::~btCompoundCollisionAlgorithm()
{
	removeChildAlgorithms();
}



extern const btOverlappingPairCache* g_overlappingPairCache;

struct	btCompoundLeafCallback : btDbvt::ICollide
{

public:

	btCollisionObject* m_compoundColObj;
	btCollisionObject* m_otherObj;
	btDispatcher* m_dispatcher;
	const btDispatcherInfo& m_dispatchInfo;
	btManifoldResult*	m_resultOut;
	btCollisionAlgorithm**	m_childCollisionAlgorithms;
	btPersistentManifold*	m_sharedManifold;
	btVector3 m_cachedAabbMins;
	btVector3 m_cachedAabbMaxs;
	bool m_cachedAabb;


	btCompoundLeafCallback (btCollisionObject* compoundObj,btCollisionObject* otherObj,btDispatcher* dispatcher,const btDispatcherInfo& dispatchInfo,btManifoldResult*	resultOut,btCollisionAlgorithm**	childCollisionAlgorithms,btPersistentManifold*	sharedManifold)
		:m_compoundColObj(compoundObj),m_otherObj(otherObj),m_dispatcher(dispatcher),m_dispatchInfo(dispatchInfo),m_resultOut(resultOut),
		m_childCollisionAlgorithms(childCollisionAlgorithms),
		m_sharedManifold(sharedManifold),
		m_cachedAabb(false)
	{

	}


	inline void	ProcessChildShape(btCollisionShape* childShape,int index,btCompoundShape* compoundShape)
	{
		btAssert(index>=0);
		btAssert(index<compoundShape->getNumChildShapes());

		btCollisionObject* compoundColObj = m_compoundColObj;
		const btTransform& childTrans = compoundShape->getChildTransform(index);
		btTransform	newChildWorldTrans = compoundColObj->getWorldTransform() * childTrans ;

		//perform an AABB check first
		btVector3 aabbMin0,aabbMax0;//,aabbMin1,aabbMax1;
		childShape->getAabb(newChildWorldTrans,aabbMin0,aabbMax0);
		
		if(!m_cachedAabb)
		{
			m_cachedAabb = true;
			m_otherObj->getCollisionShape()->getAabb(m_otherObj->getWorldTransform(),m_cachedAabbMins,m_cachedAabbMaxs);
		}

		if (TestAabbAgainstAabb2(aabbMin0,aabbMax0,m_cachedAabbMins,m_cachedAabbMaxs))
		{
			//backup
			btTransform	orgTrans = compoundColObj->getWorldTransform();
			btTransform	orgInterpolationTrans = compoundColObj->getInterpolationWorldTransform();
			unsigned int orgInteractionLayer = compoundColObj->getInteractionLayer();

			compoundColObj->setInteractionLayer(compoundShape->getChildInteractionLayer(index));
			compoundColObj->setWorldTransform( newChildWorldTrans);
			compoundColObj->setInterpolationWorldTransform(newChildWorldTrans);

			//the contactpoint is still projected back using the original inverted worldtrans
			btCollisionShape* tmpShape = compoundColObj->getCollisionShape();
			compoundColObj->internalSetTemporaryCollisionShape( childShape );

			if (!m_childCollisionAlgorithms[index])
				m_childCollisionAlgorithms[index] = m_dispatcher->findAlgorithm(compoundColObj,m_otherObj,m_sharedManifold);

			///detect swapping case
			if (m_resultOut->getBody0Internal() == compoundColObj)
			{
				m_resultOut->setShapeIdentifiersA(index);
			} else
			{
				m_resultOut->setShapeIdentifiersB(index);
			}

			m_childCollisionAlgorithms[index]->processCollision(compoundColObj,m_otherObj,m_dispatchInfo,m_resultOut);
			if (m_dispatchInfo.m_debugDraw && (m_dispatchInfo.m_debugDraw->getDebugMode() & btIDebugDraw::DBG_DrawAabb))
			{
				btVector3 worldAabbMin,worldAabbMax;
#ifdef BT_PS3PPU_VEC
				m_dispatchInfo.m_debugDraw->drawAabb(aabbMin0,aabbMax0,btOne);
				m_dispatchInfo.m_debugDraw->drawAabb(m_cachedAabbMins,m_cachedAabbMaxs,btOne);
#else
				m_dispatchInfo.m_debugDraw->drawAabb(aabbMin0,aabbMax0,btVector3(1,1,1));
				m_dispatchInfo.m_debugDraw->drawAabb(m_cachedAabbMins,m_cachedAabbMaxs,btVector3(1,1,1));
#endif
			}
			
			//revert back transform
			compoundColObj->internalSetTemporaryCollisionShape( tmpShape);
			compoundColObj->setWorldTransform(  orgTrans );
			compoundColObj->setInterpolationWorldTransform(orgInterpolationTrans);
			compoundColObj->setInteractionLayer(orgInteractionLayer);
		}
	}
	void		Process(const btDbvtNode* leaf)
	{
		int index = leaf->dataAsInt;

		btCompoundShape* compoundShape = static_cast<btCompoundShape*>(m_compoundColObj->getCollisionShape());
		btCollisionShape* childShape = compoundShape->getChildShape(index);
		
		if (m_dispatchInfo.m_debugDraw && (m_dispatchInfo.m_debugDraw->getDebugMode() & btIDebugDraw::DBG_DrawAabb))
		{
			btVector3 worldAabbMin,worldAabbMax;
			btTransform	orgTrans = m_compoundColObj->getWorldTransform();
			btTransformAabb(leaf->volume.Mins(),leaf->volume.Maxs(),0.,orgTrans,worldAabbMin,worldAabbMax);
#ifdef BT_PS3PPU_VEC
			m_dispatchInfo.m_debugDraw->drawAabb(worldAabbMin,worldAabbMax,btIdentVec0);
#else
			m_dispatchInfo.m_debugDraw->drawAabb(worldAabbMin,worldAabbMax,btVector3(1,0,0));
#endif
		}

		//check interaction layers of children
		if(!g_overlappingPairCache->hasInteractionEnabled(compoundShape->getChildInteractionLayer(index), m_otherObj->getInteractionLayer()))
			return;

		//check interaction blocks
		if(m_compoundColObj->hasInteractionBlocks() && m_otherObj->hasInteractionBlocks())
		{
			btInteractionBlock ib(m_compoundColObj, compoundShape, m_otherObj, m_otherObj->getCollisionShape());

			if(g_overlappingPairCache->checkInteractionBlock(ib))
				return;
		}


		ProcessChildShape(childShape,index, compoundShape);

	}
};






void btCompoundCollisionAlgorithm::processCollision (btCollisionObject* body0,btCollisionObject* body1,const btDispatcherInfo& dispatchInfo,btManifoldResult* resultOut)
{
	bool isSwapped = m_isSwapped;
	btCollisionObject* colObj = isSwapped? body1 : body0;
	btCollisionObject* otherObj = isSwapped? body0 : body1;

	

	btAssert (colObj->getCollisionShape()->isCompound());

	btCompoundShape* compoundShape = static_cast<btCompoundShape*>(colObj->getCollisionShape());

	///btCompoundShape might have changed:
	////make sure the internal child collision algorithm caches are still valid
	if (compoundShape->getUpdateRevision() != m_compoundShapeRevision)
	{
		///clear and update all
		removeChildAlgorithms();
		
		preallocateChildAlgorithms(body0,body1);
	}


	btDbvt* tree = compoundShape->getDynamicAabbTree();

	///we need to refresh all contact manifolds
	///note that we should actually recursively traverse all children, btCompoundShape can nested more then 1 level deep
	///so we should add a 'refreshManifolds' in the btCollisionAlgorithm
	{
		int i;
		static btManifoldArray manifoldArray;
		manifoldArray.resize(0);
		for (i=0;i<m_childCollisionAlgorithms.size();i++)
		{
			if (m_childCollisionAlgorithms[i])
			{
				m_childCollisionAlgorithms[i]->getAllContactManifolds(manifoldArray);
				for (int m=0;m<manifoldArray.size();m++)
				{
					if (manifoldArray[m]->getNumContacts())
					{
						resultOut->setPersistentManifold(manifoldArray[m]);
						resultOut->refreshContactPoints();
						resultOut->setPersistentManifold(0);//??necessary?
					}
				}
				manifoldArray.resize(0);
			}
		}
	}

	if (tree)
	{
		//use a dynamic aabb tree to cull potential child-overlaps
		btCompoundLeafCallback  callback(colObj,otherObj,m_dispatcher,dispatchInfo,resultOut,&m_childCollisionAlgorithms[0],m_sharedManifold);

		btVector3 localAabbMin,localAabbMax;
		btTransform otherInCompoundSpace;
		otherInCompoundSpace = colObj->getWorldTransform().inverse() * otherObj->getWorldTransform();
		otherObj->getCollisionShape()->getAabb(otherInCompoundSpace,localAabbMin,localAabbMax);

		const ATTRIBUTE_ALIGNED16(btDbvtVolume)	bounds=btDbvtVolume::FromMM(localAabbMin,localAabbMax);
		//process all children, that overlap with  the given AABB bounds
#if 1
		tree->collideTV(tree->m_root,bounds,callback);
#else

		int numChildren = m_childCollisionAlgorithms.size();
		int i;

		for (i=0;i<numChildren;i++)
		{
			//check interaction layers of children
			if(!g_overlappingPairCache->hasInteractionEnabled(compoundShape->getChildInteractionLayer(i), otherObj->getInteractionLayer()))
				continue;

			//check interaction blocks
			if(colObj->hasInteractionBlocks() && otherObj->hasInteractionBlocks())
			{
				btInteractionBlock ib(colObj, compoundShape, otherObj, otherObj->getCollisionShape());

				if(g_overlappingPairCache->checkInteractionBlock(ib))
					return;
			}

			btCollisionShape* childShape = compoundShape->getChildShape(i);
			callback.ProcessChildShape(childShape, i, compoundShape);
		}
#endif

	} else
	{
		//iterate over all children, perform an AABB check inside ProcessChildShape
		int numChildren = m_childCollisionAlgorithms.size();
		int i;
		btVector3 aabbMin0,aabbMax0,aabbMin1,aabbMax1;
		otherObj->getCollisionShape()->getAabb(otherObj->getWorldTransform(),aabbMin1,aabbMax1);
		bool changed = false;

		for (i=0;i<numChildren;i++)
		{
			//check interaction layers of children
			if(!g_overlappingPairCache->hasInteractionEnabled(compoundShape->getChildInteractionLayer(i), otherObj->getInteractionLayer()))
				continue;

			//check interaction blocks
			if(colObj->hasInteractionBlocks() && otherObj->hasInteractionBlocks())
			{
				btInteractionBlock ib(colObj, compoundShape, otherObj, otherObj->getCollisionShape());

				if(g_overlappingPairCache->checkInteractionBlock(ib))
					return;
			}

			btCollisionShape* childShape = compoundShape->getChildShape(i);


			const btTransform& childTrans = compoundShape->getChildTransform(i);
			btTransform	newChildWorldTrans = colObj->getWorldTransform() * childTrans ;

			//perform an AABB check first
			childShape->getAabb(newChildWorldTrans,aabbMin0,aabbMax0);

			if (TestAabbAgainstAabb2(aabbMin0,aabbMax0,aabbMin1,aabbMax1))
			{
				//backup
				btTransform	orgTrans = colObj->getWorldTransform();
				btTransform	orgInterpolationTrans = colObj->getInterpolationWorldTransform();
				unsigned int orgInteractionLayer = colObj->getInteractionLayer();

				colObj->setInteractionLayer(compoundShape->getChildInteractionLayer(i));
				colObj->setWorldTransform( newChildWorldTrans);
				colObj->setInterpolationWorldTransform(newChildWorldTrans);
				colObj->internalSetTemporaryCollisionShape( childShape );

				if (!m_childCollisionAlgorithms[i])
					m_childCollisionAlgorithms[i] = m_dispatcher->findAlgorithm(colObj,otherObj,m_sharedManifold);

				///detect swapping case
				if (isSwapped)
				{
					resultOut->setShapeIdentifiersB(i);
				} else
				{
					resultOut->setShapeIdentifiersA(i);
				}

				m_childCollisionAlgorithms[i]->processCollision(colObj,otherObj,dispatchInfo,resultOut);
				if (dispatchInfo.m_debugDraw && (dispatchInfo.m_debugDraw->getDebugMode() & btIDebugDraw::DBG_DrawAabb))
				{
					btVector3 worldAabbMin,worldAabbMax;
#ifdef BT_PS3PPU_VEC
					dispatchInfo.m_debugDraw->drawAabb(aabbMin0,aabbMax0,btOne);
					dispatchInfo.m_debugDraw->drawAabb(aabbMin1,aabbMax1,btOne);
#else
					dispatchInfo.m_debugDraw->drawAabb(aabbMin0,aabbMax0,btVector3(1,1,1));
					dispatchInfo.m_debugDraw->drawAabb(aabbMin1,aabbMax1,btVector3(1,1,1));
#endif
				}
				//revert back transform
				colObj->internalSetTemporaryCollisionShape(compoundShape);
				colObj->setWorldTransform(  orgTrans );
				colObj->setInterpolationWorldTransform(orgInterpolationTrans);
				colObj->setInteractionLayer(orgInteractionLayer);
			}
			else
			{
				if(m_childCollisionAlgorithms[i])
				{
					m_childCollisionAlgorithms[i]->~btCollisionAlgorithm();
					m_dispatcher->freeCollisionAlgorithm(m_childCollisionAlgorithms[i]);
					m_childCollisionAlgorithms[i] = 0;
				}
			}
		}
	}

}

btScalar	btCompoundCollisionAlgorithm::calculateTimeOfImpact(btCollisionObject* body0,btCollisionObject* body1,const btDispatcherInfo& dispatchInfo,btManifoldResult* resultOut)
{

	btCollisionObject* colObj = m_isSwapped? body1 : body0;
	btCollisionObject* otherObj = m_isSwapped? body0 : body1;

	btAssert (colObj->getCollisionShape()->isCompound());
	
	btCompoundShape* compoundShape = static_cast<btCompoundShape*>(colObj->getCollisionShape());

	//We will use the OptimizedBVH, AABB tree to cull potential child-overlaps
	//If both proxies are Compound, we will deal with that directly, by performing sequential/parallel tree traversals
	//given Proxy0 and Proxy1, if both have a tree, Tree0 and Tree1, this means:
	//determine overlapping nodes of Proxy1 using Proxy0 AABB against Tree1
	//then use each overlapping node AABB against Tree0
	//and vise versa.

	btScalar hitFraction = btScalar(1.);

	int numChildren = m_childCollisionAlgorithms.size();
	int i;
    btTransform	orgTrans;
    btScalar frac;
	for (i=0;i<numChildren;i++)
	{
		//temporarily exchange parent btCollisionShape with childShape, and recurse
		btCollisionShape* childShape = compoundShape->getChildShape(i);

		//backup
        orgTrans = colObj->getWorldTransform();
	
		const btTransform& childTrans = compoundShape->getChildTransform(i);
		//btTransform	newChildWorldTrans = orgTrans*childTrans ;
		colObj->setWorldTransform( orgTrans*childTrans );

		btCollisionShape* tmpShape = colObj->getCollisionShape();
		colObj->internalSetTemporaryCollisionShape( childShape );
        frac = m_childCollisionAlgorithms[i]->calculateTimeOfImpact(colObj,otherObj,dispatchInfo,resultOut);
		if (frac<hitFraction)
		{
			hitFraction = frac;
		}
		//revert back
		colObj->internalSetTemporaryCollisionShape( tmpShape);
		colObj->setWorldTransform( orgTrans);
	}
	return hitFraction;

}



