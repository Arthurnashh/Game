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


#include "btCollisionObject.h"
#include "LinearMath/btSerializer.h"
#include "BulletCollision/CollisionShapes/btCollisionShape.h"

btInteractionBlock::btInteractionBlock(btCollisionObject* object1, btCollisionObject* object2)
{
	if(object1 < object2)
	{
		m_feature1.m_object = object1;
		m_feature2.m_object = object2;
	}
	else
	{
		m_feature1.m_object = object2;
		m_feature2.m_object = object1;
	}
}

btInteractionBlock::btInteractionBlock(btCollisionObject* object1, const btCollisionShape* shape1, btCollisionObject* object2, const btCollisionShape* shape2)
{
	if(object1 < object2)
	{
		m_feature1.m_object = object1;
		m_feature1.m_shape = shape1;
		m_feature2.m_object = object2;
		m_feature2.m_shape = shape2;
	}
	else
	{
		m_feature1.m_object = object1;
		m_feature1.m_shape = shape2;
		m_feature2.m_object = object1;
		m_feature2.m_shape = shape1;
	}
}

unsigned int btInteractionBlock::getHashCode() const
{
	return (unsigned int)(
		((intptr_t)m_feature1.m_object / sizeof(btCollisionObject)) +
		((intptr_t)m_feature1.m_shape / sizeof(btCollisionShape)) +
		((intptr_t)m_feature2.m_object / sizeof(btCollisionObject)) +
		((intptr_t)m_feature2.m_shape / sizeof(btCollisionShape)));
}

const btMaterial* btMaterial::findCombiner(const btMaterial* other) const
{
	if(hasCombiners())
	{
		int index = m_combiners.findBinarySearch(Pair(other, NULL));
		if(index < m_combiners.size())
				return m_combiners[index].data;
	}
	return NULL;
}

void btMaterial::addCombiner(const btMaterial* other, const btMaterial* combination)
{
	btAssert(findCombiner(other) == NULL);
	m_combiners.insertSorted(Pair(other, combination));
}


btCollisionObject::btCollisionObject()
	:
	m_contactProcessingThreshold(BT_LARGE_FLOAT),
		m_broadphaseHandle(0),
		m_collisionShape(0),
		m_rootCollisionShape(0),
		m_interactionLayer(-1),
		m_interactionBlockCount(0),
		m_collisionFlags(btCollisionObject::CF_STATIC_OBJECT),
		m_islandTag1(-1),
		//m_companionId(-2),
		m_activationState1(1),
		m_deactivationTime(btScalar(0.)),
    m_lifeTime(BT_LARGE_FLOAT),
		//m_material(0),
		m_internalType(CO_COLLISION_OBJECT),
		m_userObjectPointer(0),
		m_hitFraction(btScalar(1.)),
		m_ccdSweptSphereRadius(btScalar(0.)),
		m_ccdMotionThreshold(btScalar(0.)),
		m_checkCollideWith(false)
{
	m_worldTransform.setIdentity();
}

btCollisionObject::~btCollisionObject()
{
	m_userObjectPointer = 0;
}

void btCollisionObject::setActivationState(int newState) 
{ 
	if (m_activationState1 != newState && (m_activationState1 != DISABLE_DEACTIVATION) && (m_activationState1 != DISABLE_SIMULATION))
	{
		//if(newState == ACTIVE_TAG)
		//	newState=newState;
		m_activationState1 = newState;
	}
}

void btCollisionObject::forceActivationState(int newState)
{
		//if(newState == ACTIVE_TAG)
		//	newState=newState;
	m_activationState1 = newState;
}

void btCollisionObject::activate(bool forceActivation)
{
	if (forceActivation || !(m_collisionFlags & (CF_STATIC_OBJECT|CF_KINEMATIC_OBJECT)))
	{
		if ((m_activationState1 != ACTIVE_TAG) && (m_activationState1 != DISABLE_DEACTIVATION) && (m_activationState1 != DISABLE_SIMULATION))
			m_activationState1 = ACTIVE_TAG;

		m_deactivationTime = btScalar(0.);
    m_lifeTime = BT_LARGE_FLOAT;
	}
}

const btMaterial* btCollisionObject::getMaterial() const
{
	return getCollisionShape()->getMaterial();
}

void btCollisionObject::setCollisionShape(btCollisionShape* collisionShape)
{
	btAssert(collisionShape == NULL || collisionShape->getMaterial());
	m_collisionShape = collisionShape;
	m_rootCollisionShape = collisionShape;
}

/*
bool	btCollisionObject::hasAnisotropicFriction() const
{
	return m_collisionShape->m_material->m_hasAnisotropicFriction;
}
const btVector3& btCollisionObject::getAnisotropicFriction() const
{
	return m_collisionShape->m_material->m_anisotropicFriction;
}
*/
const char* btCollisionObject::serialize(void* dataBuffer, btSerializer* serializer) const
{

	btCollisionObjectData* dataOut = (btCollisionObjectData*)dataBuffer;

	m_worldTransform.serialize(dataOut->m_worldTransform);
	m_interpolationWorldTransform.serialize(dataOut->m_interpolationWorldTransform);
	//m_interpolationLinearVelocity.serialize(dataOut->m_interpolationLinearVelocity);
	//m_interpolationAngularVelocity.serialize(dataOut->m_interpolationAngularVelocity);
	btVector3 friction;
	friction.serialize(dataOut->m_anisotropicFriction);
	dataOut->m_hasAnisotropicFriction = false;//hasAnisotropicFriction();
	dataOut->m_contactProcessingThreshold = m_contactProcessingThreshold;
	dataOut->m_broadphaseHandle = 0;
	dataOut->m_collisionShape = serializer->getUniquePointer(m_collisionShape);
	dataOut->m_rootCollisionShape = 0;//@todo
	dataOut->m_collisionFlags = m_collisionFlags;
	dataOut->m_islandTag1 = m_islandTag1;
//	dataOut->m_companionId = m_companionId;
	dataOut->m_activationState1 = m_activationState1;
	dataOut->m_activationState1 = m_activationState1;
	dataOut->m_deactivationTime = m_deactivationTime;
  dataOut->m_lifeTime = m_lifeTime;
	dataOut->m_friction = 0;//getFriction();
	dataOut->m_restitution = 0;//getRestitution();
	dataOut->m_internalType = m_internalType;
	dataOut->m_name = (char*) serializer->findNameForPointer(this);
	if (dataOut->m_name)
	{
		serializer->serializeName(dataOut->m_name);
	}
	dataOut->m_hitFraction = m_hitFraction;
	dataOut->m_ccdSweptSphereRadius = m_ccdSweptSphereRadius;
	dataOut->m_ccdMotionThreshold = m_ccdMotionThreshold;
	dataOut->m_ccdMotionThreshold = m_ccdMotionThreshold;
	dataOut->m_checkCollideWith = m_checkCollideWith;

	return btCollisionObjectDataName;
}


void btCollisionObject::serializeSingleObject(class btSerializer* serializer) const
{
	int len = calculateSerializeBufferSize();
	btChunk* chunk = serializer->allocate(len,1);
	const char* structType = serialize(chunk->m_oldPtr, serializer);
	serializer->finalizeChunk(chunk,structType,BT_COLLISIONOBJECT_CODE,(void*)this);
}

btQuaternion btCollisionObject::getOrientation() const
{
		btQuaternion orn;
		m_worldTransform.getBasis().getRotation(orn);
		return orn;
}
