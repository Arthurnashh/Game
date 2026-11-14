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

#include "btCompoundShape.h"
#include "btCollisionShape.h"
#include "BulletCollision/BroadphaseCollision/btDbvt.h"
#include "LinearMath/btSerializer.h"

btCompoundShape::btCompoundShape(bool enableDynamicAabbTree)
: m_localAabbMin(btScalar(BT_LARGE_FLOAT),btScalar(BT_LARGE_FLOAT),btScalar(BT_LARGE_FLOAT)),
m_localAabbMax(btScalar(-BT_LARGE_FLOAT),btScalar(-BT_LARGE_FLOAT),btScalar(-BT_LARGE_FLOAT)),
m_dynamicAabbTree(0),
m_updateRevision(1),
m_collisionMargin(btScalar(0.)),
m_localScaling(btScalar(1.),btScalar(1.),btScalar(1.))
{
	m_shapeType = COMPOUND_SHAPE_PROXYTYPE;

	if (enableDynamicAabbTree)
	{
		void* mem = btAlignedAlloc(sizeof(btDbvt),16);
		m_dynamicAabbTree = new(mem) btDbvt();
		btAssert(mem==m_dynamicAabbTree);
	}
}


btCompoundShape::~btCompoundShape()
{
	if (m_dynamicAabbTree)
	{
		m_dynamicAabbTree->~btDbvt();
		btAlignedFree(m_dynamicAabbTree);
	}
}

int	btCompoundShape::addChildShape(const btTransform& localTransform,btCollisionShape* shape, unsigned int interactionLayer)
{
	m_updateRevision++;
	//m_childTransforms.push_back(localTransform);
	//m_childShapes.push_back(shape);
	btCompoundShapeChild child;
	child.m_node = 0;
	child.m_transform = localTransform;
	child.m_childShape = shape;
	child.m_childShapeType = shape->getShapeType();
	child.m_childMargin = shape->getMargin();
	child.m_interactionLayer = interactionLayer;
	
	//extend the local aabbMin/aabbMax
	btVector3 localAabbMin,localAabbMax;
	shape->getAabb(localTransform,localAabbMin,localAabbMax);
	for (int i=0;i<3;i++)
	{
		if (m_localAabbMin[i] > localAabbMin[i])
		{
			m_localAabbMin[i] = localAabbMin[i];
		}
		if (m_localAabbMax[i] < localAabbMax[i])
		{
			m_localAabbMax[i] = localAabbMax[i];
		}

	}
	if (m_dynamicAabbTree)
	{
		const btDbvtVolume	bounds=btDbvtVolume::FromMM(localAabbMin,localAabbMax);
		int index = m_children.size();
		child.m_node = m_dynamicAabbTree->insert(bounds,(void*)index);
	}

	m_children.push_back(child);
	return m_children.size() - 1;
}

void	btCompoundShape::updateChildTransform(int childIndex, const btTransform& newChildTransform)
{
	m_children[childIndex].m_transform = newChildTransform;

	if (m_dynamicAabbTree)
	{
		///update the dynamic aabb tree
		btVector3 localAabbMin,localAabbMax;
		m_children[childIndex].m_childShape->getAabb(newChildTransform,localAabbMin,localAabbMax);
		ATTRIBUTE_ALIGNED16(btDbvtVolume)	bounds=btDbvtVolume::FromMM(localAabbMin,localAabbMax);
		//int index = m_children.size()-1;
		m_dynamicAabbTree->update(m_children[childIndex].m_node,bounds);
	}

	recalculateLocalAabb();
}

void btCompoundShape::removeChildShapeByIndex(int childShapeIndex)
{
	m_updateRevision++;
	btAssert(childShapeIndex >=0 && childShapeIndex < m_children.size());
	if (m_dynamicAabbTree)
	{
		m_dynamicAabbTree->remove(m_children[childShapeIndex].m_node);
	}
	m_children.swap(childShapeIndex,m_children.size()-1);
    if (m_dynamicAabbTree) 
		m_children[childShapeIndex].m_node->dataAsInt = childShapeIndex;
	m_children.pop_back();

}



void btCompoundShape::removeChildShape(btCollisionShape* shape)
{
	m_updateRevision++;
	// Find the children containing the shape specified, and remove those children.
	//note: there might be multiple children using the same shape!
	for(int i = m_children.size()-1; i >= 0 ; i--)
	{
		if(m_children[i].m_childShape == shape)
		{
			removeChildShapeByIndex(i);
		}
	}



	recalculateLocalAabb();
}

void btCompoundShape::recalculateLocalAabb()
{
	// Recalculate the local aabb
	// Brute force, it iterates over all the shapes left.

	m_localAabbMin = btVector3(btScalar(BT_LARGE_FLOAT),btScalar(BT_LARGE_FLOAT),btScalar(BT_LARGE_FLOAT));
	m_localAabbMax = btVector3(btScalar(-BT_LARGE_FLOAT),btScalar(-BT_LARGE_FLOAT),btScalar(-BT_LARGE_FLOAT));

	//extend the local aabbMin/aabbMax
	for (int j = 0; j < m_children.size(); j++)
	{
		btVector3 localAabbMin,localAabbMax;
		m_children[j].m_childShape->getAabb(m_children[j].m_transform, localAabbMin, localAabbMax);
		for (int i=0;i<3;i++)
		{
			if (m_localAabbMin[i] > localAabbMin[i])
				m_localAabbMin[i] = localAabbMin[i];
			if (m_localAabbMax[i] < localAabbMax[i])
				m_localAabbMax[i] = localAabbMax[i];
		}
	}
}

///getAabb's default implementation is brute force, expected derived classes to implement a fast dedicated version
void btCompoundShape::getAabb(const btTransform& trans,btVector3& aabbMin,btVector3& aabbMax) const
{
	btVector3 localHalfExtents = btScalar(0.5)*(m_localAabbMax-m_localAabbMin);
	btVector3 localCenter = btScalar(0.5)*(m_localAabbMax+m_localAabbMin);
	
	//avoid an illegal AABB when there are no children
	if (!m_children.size())
	{
		localHalfExtents.setValue(0,0,0);
		localCenter.setValue(0,0,0);
	}
	localHalfExtents += btVector3(getMargin(),getMargin(),getMargin());
		

	btMatrix3x3 abs_b = trans.getBasis().absolute();  

	btVector3 center = trans(localCenter);

#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(61);
	btVector3 extent = btVector3(vec_dot3mx(abs_b.get128ptr(), localHalfExtents.mVec));
#else
	btVector3 extent = btVector3(abs_b[0].dot(localHalfExtents),
		abs_b[1].dot(localHalfExtents),
		abs_b[2].dot(localHalfExtents));
#endif
	aabbMin = center-extent;
	aabbMax = center+extent;
	
}

void	btCompoundShape::calculateLocalInertia(btScalar mass,btVector3& inertia) const
{
	//approximation: take the inertia from the aabb for now
	btTransform ident;
	ident.setIdentity();
	btVector3 aabbMin,aabbMax;
	getAabb(ident,aabbMin,aabbMax);

	btVector3 halfExtents = (aabbMax-aabbMin)*btScalar(0.5);

#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(62);
	vec_float4 l = vec_madd(halfExtents.mVec, vec_float4(2.f), btZeroVec());
	vec_float4 l2 = vec_madd(l, l, btZeroVec());
	
	vec_float4 tmp = vec_sld(l2, l2, 12);
	const btScalar scaledmass = mass * btScalar(0.08333333);
	inertia = scaledmass * (btVector3(vec_add(vec_sld(tmp, l2, 12), vec_sld(tmp, l2, 8))));
#else
	btScalar lx=btScalar(2.)*(halfExtents.getX());
	btScalar ly=btScalar(2.)*(halfExtents.getY());
	btScalar lz=btScalar(2.)*(halfExtents.getZ());

	inertia[0] = mass/(btScalar(12.0)) * (ly*ly + lz*lz);
	inertia[1] = mass/(btScalar(12.0)) * (lx*lx + lz*lz);
	inertia[2] = mass/(btScalar(12.0)) * (lx*lx + ly*ly);
#endif
}




void btCompoundShape::calculatePrincipalAxisTransform(btScalar* masses, btTransform& principal, btVector3& inertia) const
{
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(63);
	int n = m_children.size();

	btScalar totalMass = 0;
	btVector3 center = btZeroVec();
	int k;

	for (k = 0; k < n; k++)
	{
		btAssert(masses[k]>0);
		center += m_children[k].m_transform.getOrigin() * masses[k];
		totalMass += masses[k];
	}

	btAssert(totalMass>0);

	center /= totalMass;
	principal.setOrigin(center);

	btMatrix3x3 tensor(btZeroVec(), btZeroVec(), btZeroVec());

	for ( k = 0; k < n; k++)
	{
		btVector3 i;
		m_children[k].m_childShape->calculateLocalInertia(masses[k], i);

		const btTransform& t = m_children[k].m_transform;
		btVector3 o = t.getOrigin() - center;

		//compute inertia tensor in coordinate system of compound shape
		btMatrix3x3 j = t.getBasis();

		j[0] = vec_madd(j[0].mVec, i.mVec, btZeroVec());
		j[1] = vec_madd(j[1].mVec, i.mVec, btZeroVec());
		j[2] = vec_madd(j[2].mVec, i.mVec, btZeroVec());

		j = j.transpose();
		j = t.getBasis() * j;

		//add inertia tensor
		tensor[0] = vec_add(tensor[0].mVec, j[0].mVec);
		tensor[1] = vec_add(tensor[1].mVec, j[1].mVec);
		tensor[2] = vec_add(tensor[2].mVec, j[2].mVec);

		//compute inertia tensor of pointmass at o
		btVector3 o2 = btVector3(o.length2());

		j[0] = vec_sel(btZeroVec(), o2.mVec, btSelVec0);
		j[1] = vec_sld(j[0].mVec, j[0].mVec, 12);
		j[2] = vec_sld(j[0].mVec, j[0].mVec, 8);

		vec_float4 mo = vec_sub(btZeroVec(), o.mVec);
		vec_float4 tmp = vec_vmrghw(mo, mo);
		j[0] = vec_madd(o.mVec, vec_vmrghw(tmp, tmp), j[0].mVec); 
		j[1] = vec_madd(o.mVec, vec_vmrglw(tmp, tmp), j[1].mVec); 
		tmp = vec_vmrglw(mo, mo);
		j[2] = vec_madd(o.mVec, vec_vmrghw(tmp, tmp), j[2].mVec); 

		//add inertia tensor of pointmass
		vec_float4 massesk = vec_float4(masses[k]);
		tensor[0] = vec_madd(massesk, j[0].mVec, tensor[0].mVec);
		tensor[1] = vec_madd(massesk, j[1].mVec, tensor[1].mVec);
		tensor[2] = vec_madd(massesk, j[2].mVec, tensor[2].mVec);
	}

	tensor.diagonalize(principal.getBasis(), btScalar(0.00001), 20);
	inertia = tensor.getDiagonal();
#else
	int n = m_children.size();

	btScalar totalMass = 0;
	btVector3 center(0, 0, 0);
	int k;

	for (k = 0; k < n; k++)
	{
		btAssert(masses[k]>0);
		center += m_children[k].m_transform.getOrigin() * masses[k];
		totalMass += masses[k];
	}

	btAssert(totalMass>0);

	center /= totalMass;
	principal.setOrigin(center);

	btMatrix3x3 tensor(0, 0, 0, 0, 0, 0, 0, 0, 0);
	for ( k = 0; k < n; k++)
	{
		btVector3 i;
		m_children[k].m_childShape->calculateLocalInertia(masses[k], i);

		const btTransform& t = m_children[k].m_transform;
		btVector3 o = t.getOrigin() - center;

		//compute inertia tensor in coordinate system of compound shape
		btMatrix3x3 j = t.getBasis().transpose();

		j[0] *= i[0];
		j[1] *= i[1];
		j[2] *= i[2];
		j = t.getBasis() * j;

		//add inertia tensor
		tensor[0] += j[0];
		tensor[1] += j[1];
		tensor[2] += j[2];

		//compute inertia tensor of pointmass at o
		btScalar o2 = o.length2();

		j[0].setValue(o2, 0, 0);
		j[1].setValue(0, o2, 0);
		j[2].setValue(0, 0, o2);
		j[0] += o * -o.x(); 
		j[1] += o * -o.y(); 
		j[2] += o * -o.z();

		//add inertia tensor of pointmass
		tensor[0] += masses[k] * j[0];
		tensor[1] += masses[k] * j[1];
		tensor[2] += masses[k] * j[2];
	}

	tensor.diagonalize(principal.getBasis(), btScalar(0.00001), 20);
	inertia.setValue(tensor[0][0], tensor[1][1], tensor[2][2]);
#endif
}



void btCompoundShape::setLocalScaling(const btVector3& scaling)
{

	for(int i = 0; i < m_children.size(); i++)
	{
		btTransform childTrans = getChildTransform(i);
		btVector3 childScale = m_children[i].m_childShape->getLocalScaling();
//		childScale = childScale * (childTrans.getBasis() * scaling);
		childScale = childScale * scaling / m_localScaling;
		m_children[i].m_childShape->setLocalScaling(childScale);
		childTrans.setOrigin((childTrans.getOrigin())*scaling);
		updateChildTransform(i, childTrans);
		recalculateLocalAabb();
	}
	m_localScaling = scaling;
}


void btCompoundShape::createAabbTreeFromChildren()
{
    if ( !m_dynamicAabbTree )
    {
        void* mem = btAlignedAlloc(sizeof(btDbvt),16);
        m_dynamicAabbTree = new(mem) btDbvt();
        btAssert(mem==m_dynamicAabbTree);

        for ( int index = 0; index < m_children.size(); index++ )
        {
            btCompoundShapeChild &child = m_children[index];

            //extend the local aabbMin/aabbMax
            btVector3 localAabbMin,localAabbMax;
            child.m_childShape->getAabb(child.m_transform,localAabbMin,localAabbMax);

            const btDbvtVolume  bounds=btDbvtVolume::FromMM(localAabbMin,localAabbMax);
            child.m_node = m_dynamicAabbTree->insert(bounds,(void*)index);
        }
    }
}


///fills the dataBuffer and returns the struct name (and 0 on failure)
const char*	btCompoundShape::serialize(void* dataBuffer, btSerializer* serializer) const
{

	btCompoundShapeData* shapeData = (btCompoundShapeData*) dataBuffer;
	btCollisionShape::serialize(&shapeData->m_collisionShapeData, serializer);

	shapeData->m_collisionMargin = float(m_collisionMargin);
	shapeData->m_numChildShapes = m_children.size();
	shapeData->m_childShapePtr = 0;
	if (shapeData->m_numChildShapes)
	{
		btChunk* chunk = serializer->allocate(sizeof(btCompoundShapeChildData),shapeData->m_numChildShapes);
		btCompoundShapeChildData* memPtr = (btCompoundShapeChildData*)chunk->m_oldPtr;
		shapeData->m_childShapePtr = (btCompoundShapeChildData*)serializer->getUniquePointer(memPtr);

		for (int i=0;i<shapeData->m_numChildShapes;i++,memPtr++)
		{
			memPtr->m_childMargin = float(m_children[i].m_childMargin);
			memPtr->m_childShape = (btCollisionShapeData*)serializer->getUniquePointer(m_children[i].m_childShape);
			//don't serialize shapes that already have been serialized
			if (!serializer->findPointer(m_children[i].m_childShape))
			{
				btChunk* chunk = serializer->allocate(m_children[i].m_childShape->calculateSerializeBufferSize(),1);
				const char* structType = m_children[i].m_childShape->serialize(chunk->m_oldPtr,serializer);
				serializer->finalizeChunk(chunk,structType,BT_SHAPE_CODE,m_children[i].m_childShape);
			} 

			memPtr->m_childShapeType = m_children[i].m_childShapeType;
			m_children[i].m_transform.serializeFloat(memPtr->m_transform);
		}
		serializer->finalizeChunk(chunk,"btCompoundShapeChildData",BT_ARRAY_CODE,chunk->m_oldPtr);
	}
	return "btCompoundShapeData";
}

