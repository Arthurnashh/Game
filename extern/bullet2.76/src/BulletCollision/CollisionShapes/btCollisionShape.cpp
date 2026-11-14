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
#include "BulletCollision/CollisionShapes/btCollisionShape.h"
#include "LinearMath/btSerializer.h"

/*
  Make sure this dummy function never changes so that it
  can be used by probes that are checking whether the
  library is actually installed.
*/
extern "C" 
{
void btBulletCollisionProbe ();

void btBulletCollisionProbe () {}
}



void btCollisionShape::setMaterial(const btMaterial* mat)
{
	btAssert(mat != NULL);
	m_material = mat;
}

void	btCollisionShape::getBoundingSphere(btVector3& center,btScalar& radius) const
{
	btTransform tr;
	tr.setIdentity();
	btVector3 aabbMin,aabbMax;

	getAabb(tr,aabbMin,aabbMax);

	radius = (aabbMax-aabbMin).length()*btScalar(0.5);
	center = (aabbMin+aabbMax)*btScalar(0.5);
}


btScalar	btCollisionShape::getContactBreakingThreshold(btScalar defaultContactThreshold) const
{
	return getAngularMotionDisc() * defaultContactThreshold;
}

btScalar	btCollisionShape::getAngularMotionDisc() const
{
	///@todo cache this value, to improve performance
	btVector3	center;
	btScalar disc;
	getBoundingSphere(center,disc);
	disc += (center).length();
	return disc;
}

void btCollisionShape::calculateTemporalAabb(const btTransform& curTrans,const btVector3& linvel,const btVector3& angvel,btScalar timeStep, btVector3& temporalAabbMin,btVector3& temporalAabbMax) const
{
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(58);
	//start with static aabb
	getAabb(curTrans,temporalAabbMin,temporalAabbMax);

	// add linear motion
	vec_float4 linMotion = vec_madd(linvel.mVec, vec_float4(timeStep), btZeroVec());//btVector3 linMotion = linvel*timeStep;

	///@todo: simd would have a vector max/min operation, instead of per-element access
	vec_bint4 mask = vec_cmpgt(linMotion, btZeroVec());
	temporalAabbMax.mVec = vec_sel(temporalAabbMax.mVec, vec_add(temporalAabbMax.mVec, linMotion), mask);
	temporalAabbMin.mVec = vec_sel(vec_add(temporalAabbMin.mVec, linMotion), temporalAabbMin.mVec, mask);
	
	//add conservative angular motion
	btScalar angularMotion = angvel.length() * getAngularMotionDisc() * timeStep;
	vec_float4 angularMotion3d = vec_float4(angularMotion);

	temporalAabbMin = vec_sub(temporalAabbMin.mVec, angularMotion3d);
	temporalAabbMax = vec_add(temporalAabbMin.mVec, angularMotion3d);
#else
	//start with static aabb
	getAabb(curTrans,temporalAabbMin,temporalAabbMax);

	btScalar temporalAabbMaxx = temporalAabbMax.getX();
	btScalar temporalAabbMaxy = temporalAabbMax.getY();
	btScalar temporalAabbMaxz = temporalAabbMax.getZ();
	btScalar temporalAabbMinx = temporalAabbMin.getX();
	btScalar temporalAabbMiny = temporalAabbMin.getY();
	btScalar temporalAabbMinz = temporalAabbMin.getZ();

	// add linear motion
	btVector3 linMotion = linvel*timeStep;
	///@todo: simd would have a vector max/min operation, instead of per-element access
	if (linMotion.getX() > btScalar(0.))
		temporalAabbMaxx += linMotion.getX(); 
	else
		temporalAabbMinx += linMotion.getX();
	if (linMotion.getY() > btScalar(0.))
		temporalAabbMaxy += linMotion.getY(); 
	else
		temporalAabbMiny += linMotion.getY();
	if (linMotion.getZ() > btScalar(0.))
		temporalAabbMaxz += linMotion.getZ(); 
	else
		temporalAabbMinz += linMotion.getZ();

	//add conservative angular motion
	btScalar angularMotion = angvel.length() * getAngularMotionDisc() * timeStep;
	btVector3 angularMotion3d(angularMotion,angularMotion,angularMotion);

	temporalAabbMin = btVector3(temporalAabbMinx,temporalAabbMiny,temporalAabbMinz);
	temporalAabbMax = btVector3(temporalAabbMaxx,temporalAabbMaxy,temporalAabbMaxz);

	temporalAabbMin -= angularMotion3d;
	temporalAabbMax += angularMotion3d;
#endif
}

///fills the dataBuffer and returns the struct name (and 0 on failure)
const char*	btCollisionShape::serialize(void* dataBuffer, btSerializer* serializer) const
{
	btCollisionShapeData* shapeData = (btCollisionShapeData*) dataBuffer;
	shapeData->m_name = (char*) serializer->findNameForPointer(this);
	if (shapeData->m_name)
	{
		serializer->serializeName(shapeData->m_name);
	}
	shapeData->m_shapeType = m_shapeType;
	//shapeData->m_padding//??
	return "btCollisionShapeData";
}

void	btCollisionShape::serializeSingleShape(btSerializer* serializer) const
{
	int len = calculateSerializeBufferSize();
	btChunk* chunk = serializer->allocate(len,1);
	const char* structType = serialize(chunk->m_oldPtr, serializer);
	serializer->finalizeChunk(chunk,structType,BT_SHAPE_CODE,(void*)this);
}