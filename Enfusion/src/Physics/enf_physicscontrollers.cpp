//-----------------------------------------------------------------------------
// File: enf_physicsbullet.cpp
//
// Desc: Bullet implementation
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------
//#include "precomp.h"
//#include "baseworld/enf_entity.h"
#include "baseworld/enf_baseworldtrace.h"
#include "physics/enf_physicsbulletinternal.h"
//#include "enf_shapes.h"
#include "lib\vehicle.hpp"
//#include "lib\landscape.hpp"

#include <Es/Memory/normalNew.hpp>

//bullet
#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h"

#include "BulletCollision/CollisionShapes/btCapsuleShape.h"
#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"
#include "BulletCollision/CollisionShapes/btConvexHullShape.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletCollision/CollisionShapes/btCylinderShape.h"

#include "BulletCollision/CollisionShapes/btStridingMeshInterface.h"
#include "BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h"

#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "BulletDynamics/Vehicle/btRaycastVehicle.h"

#include "lib/Network/network.hpp"

extern int gNumAlignedAllocs;
extern int gNumAlignedFree;
extern int gTotalBytesAlignedAllocs;

namespace enf
{

//-----------------------------------------------------------------------
void BulletObjectGeometry::SetupPhysicsGeomDefs(PhysicsGeomDef* geoms, uint numGeoms)
{
	for(uint e = 0; e < m_Elements.GetCardinality(); e++)
	{
		const BulletRBElement& elem = m_Elements[e];
		geoms[e].m_Name = elem.m_Name.cstr();
		btCollisionShape* shape = elem.m_pShape;
		geoms[e].m_pGeom = (PhysicsGeometry*)shape;
		geoms[e].m_pMaterial = elem.m_pMaterial;
		geoms[e].m_Frame = elem.m_Frame;
		geoms[e].m_iParentNode = elem.m_iParentNode;
		geoms[e].m_iInteractionLayer = 0xffffffff;
		geoms[e].m_iElementIndex = INDEX_NOT_FOUND;
	}
}	

//-----------------------------------------------------------------------
/*
void BulletController::AnimateElements(AnimatorInterface* anim)
{
	Assert(anim);
	enf::ObjectGeometry* og = GetObjectGeometry();
	bool animated = false;
	if(og)
	{
		enf::uint numGeoms = og->GetNumElements();
		for(enf::uint n = 0; n < numGeoms; n++)
		{
			const enf::RBElement& geom = og->GetElement(n);
			uint parentNode = geom.m_iParentNode;
			if(parentNode != 0 && parentNode != INDEX_NOT_FOUND)
			{
				Matrix43 rmat = anim->Animate(parentNode, geom.m_Frame);

				//enf::Shape::CreateBox(enf::Vector3(-32,-32,-32), enf::Vector3(32,32,32), 0xffff0000, enf::SS_ONCE)->SetMatrix(IGetMatrix() * rmat);
				//enf::Shape::CreateLine(IGetMatrix().GetTranslation(), (IGetMatrix() * rmat).GetTranslation(), 0xffffffff, enf::SS_ONCE);
				SetElementTransform(n, rmat);
				animated = true;
			}
		}
	}
	
	if(animated && !IsActive())
	{
		BulletWorld* wrld = GetWorld();
		wrld->GetBulletWorld()->updateSingleAabb(GetCollisionObject());
	}
}
*/
//-----------------------------------------------------------------------
ObjectGeometry* BulletController::GetObjectGeometry() const
{
	return m_pGeometries;
}

//-----------------------------------------------------------------------
uint BulletController::GetInteractionLayer(int geom) const
{
	return GetCollisionObject()->getInteractionLayer();
}

//-----------------------------------------------------------------------
void BulletController::SetElementTransform(uint element, const Matrix43& mat)
{
  RlsAssert(!BadMatrix43(mat));
  /*
  Assert(mat.Position().X() >= (float)BaseWorld::MIN_SIZE && mat.Position().X() <= (float)BaseWorld::MAX_SIZE);
	Assert(mat.Position().Y() >= (float)BaseWorld::MIN_SIZE && mat.Position().Y() <= (float)BaseWorld::MAX_SIZE);
	Assert(mat.Position().Z() >= (float)BaseWorld::MIN_SIZE && mat.Position().Z() <= (float)BaseWorld::MAX_SIZE);
  */

	btCollisionObject* obj = GetCollisionObject();
	btCollisionShape* shape = obj->getCollisionShape();

	if(shape->getShapeType() != COMPOUND_SHAPE_PROXYTYPE)
	{
		return;
	}

	btCompoundShape* cshape = (btCompoundShape*)shape;
	cshape->updateChildTransform(element, BulletWorld::Matrix2btTransform(mat, GetCenterOfMass()));
}

//-----------------------------------------------------------------------
void BulletController::ForceUpdateFrame(const Matrix43& frame)
{
	RlsAssert(!BadMatrix43(frame));
	btTransform trans = BulletWorld::Matrix2btTransform(frame);
	const btTransform& curr = GetCollisionObject()->getWorldTransform();
	if(trans.getOrigin() != curr.getOrigin() ||
		trans.getBasis().getRow(0) != curr.getBasis().getRow(0) ||
		trans.getBasis().getRow(1) != curr.getBasis().getRow(1) ||
		trans.getBasis().getRow(2) != curr.getBasis().getRow(2)
		)
	{
		GetCollisionObject()->setWorldTransform(trans);
		GetCollisionObject()->setInterpolationWorldTransform(trans);
		BulletWorld* wrld = GetWorld();
		wrld->GetBulletWorld()->updateSingleAabb(GetCollisionObject());
	}
}

//-----------------------------------------------------------------------
void BulletController::ForceUpdateOrientation(const Matrix33& orientation)
{
	btMatrix3x3 mat = BulletWorld::Matrix2btMatrix(orientation);
	const btTransform& curr = GetCollisionObject()->getWorldTransform();
	if(mat.getRow(0) != curr.getBasis().getRow(0) ||
		mat.getRow(1) != curr.getBasis().getRow(1) ||
		mat.getRow(2) != curr.getBasis().getRow(2)
		)
	{
		GetCollisionObject()->setWorldTransform(btTransform(mat, curr.getOrigin()));
		GetCollisionObject()->setInterpolationWorldTransform(btTransform(mat, curr.getOrigin()));
		BulletWorld* wrld = GetWorld();
		wrld->GetBulletWorld()->updateSingleAabb(GetCollisionObject());
	}
}

//-----------------------------------------------------------------------
Matrix43 BulletController::GetWorldTransform() const
{
	return BulletWorld::btTransform2Matrix(GetCollisionObject()->getWorldTransform());
}

//-----------------------------------------------------------------------
AAB BulletController::GetBoundBox() const
{
		//calculate new AABB
	const btTransform& trans = m_pBody->getWorldTransform();

	btVector3 minAabb;
	btVector3 maxAabb;
	m_pBody->getCollisionShape()->getAabb(trans, minAabb, maxAabb);

	return AAB(BulletWorld::btVector2Point(minAabb), BulletWorld::btVector2Point(maxAabb));
}

//-----------------------------------------------------------------------
bool BulletDynamicController::EnableGravity(bool enableGravity)
{
	int oldCf = GetCollisionObject()->getCollisionFlags();
	int newCf = enableGravity ? (oldCf & ~btCollisionObject::CF_NO_GRAVITY) : (oldCf | btCollisionObject::CF_NO_GRAVITY);
	GetCollisionObject()->setCollisionFlags(newCf);
	return (oldCf & btCollisionObject::CF_NO_GRAVITY) != 0;
}

//-----------------------------------------------------------------------
void BulletDynamicController::ForceUpdateFrame(const Matrix43& frame)
{
	btTransform trans = BulletWorld::Matrix2btTransform(frame);
/*	const btTransform& curr = GetCollisionObject()->getWorldTransform();
	if(trans.getOrigin() != curr.getOrigin() ||
		trans.getBasis().getRow(0) != trans.getBasis().getRow(0) ||
		trans.getBasis().getRow(1) != trans.getBasis().getRow(1) ||
		trans.getBasis().getRow(2) != trans.getBasis().getRow(2)
		)
	{*/
		GetRigidBody()->setCenterOfMassTransform(trans);
		if(!IsActive())
		{
			BulletWorld* wrld = GetWorld();
			wrld->GetBulletWorld()->updateSingleAabb(GetCollisionObject());
		}
//	}
}

//-----------------------------------------------------------------------
void BulletDynamicController::ForceUpdateOrientation(const Matrix33& orientation)
{
	btMatrix3x3 mat = BulletWorld::Matrix2btMatrix(orientation);
	const btTransform& curr = GetCollisionObject()->getWorldTransform();
	if(mat.getRow(0) != curr.getBasis().getRow(0) ||
		mat.getRow(1) != curr.getBasis().getRow(1) ||
		mat.getRow(2) != curr.getBasis().getRow(2)
		)
	{
		GetRigidBody()->setCenterOfMassOrientation(mat);
		if(!IsActive())
		{
			BulletWorld* wrld = GetWorld();
			wrld->GetBulletWorld()->updateSingleAabb(GetCollisionObject());
		}
	}
}

//-----------------------------------------------------------------------
void BulletDynamicControllerNC::ForceUpdateFrame(const Matrix43& frame)
{
	btTransform trans = BulletWorld::Matrix2btTransform(frame, m_vCOMOffset);
/*	const btTransform& curr = GetCollisionObject()->getWorldTransform();
	if(trans.getOrigin() != curr.getOrigin() ||
		trans.getBasis().getRow(0) != trans.getBasis().getRow(0) ||
		trans.getBasis().getRow(1) != trans.getBasis().getRow(1) ||
		trans.getBasis().getRow(2) != trans.getBasis().getRow(2)
		)
	{*/
		GetRigidBody()->setCenterOfMassTransform(trans);
		if(!IsActive())
		{
			BulletWorld* wrld = GetWorld();
			wrld->GetBulletWorld()->updateSingleAabb(GetCollisionObject());
		}
//	}
}

//-----------------------------------------------------------------------
void BulletDynamicControllerNC::ForceUpdateOrientation(const Matrix33& orientation)
{
	btMatrix3x3 mat = BulletWorld::Matrix2btMatrix(orientation);
	const btTransform& curr = GetCollisionObject()->getWorldTransform();
	if(mat.getRow(0) != curr.getBasis().getRow(0) ||
		mat.getRow(1) != curr.getBasis().getRow(1) ||
		mat.getRow(2) != curr.getBasis().getRow(2)
		)
	{
		GetRigidBody()->setCenterOfMassTransform(btTransform(mat, curr.getOrigin()));
		if(!IsActive())
		{
			BulletWorld* wrld = GetWorld();
			wrld->GetBulletWorld()->updateSingleAabb(GetCollisionObject());
		}
	}
}

//-----------------------------------------------------------------------
void BulletController::Synchronize()
{
	GetWorld()->Synchronize(GetCollisionObject());
}

//-----------------------------------------------------------------------
void BulletDynamicController::SetDamping(float linearDamping, float angularDamping)
{
	RlsAssert(!BadFloat(linearDamping) && !BadFloat(angularDamping));
	GetRigidBody()->setDamping(linearDamping, angularDamping);
}

//-----------------------------------------------------------------------
void BulletController::UpdateEntityFlags(Object* ent, uint oldFlags, uint newFlags)
{
}

//-----------------------------------------------------------------------
void BulletController::UpdateEntityEvents(Object* ent, uint oldFlags, uint newFlags)
{
  BulletWorld* mw = (BulletWorld*)ent->GetWorldPhysics();		
	if(mw)
	{
		if((oldFlags & EV_SIMULATE) == 0 && (newFlags & EV_SIMULATE) != 0)
			mw->m_BulletEntitiesToUpdate.Insert(ent);
		else if((oldFlags & EV_SIMULATE) == 0 && (newFlags & EV_SIMULATE) != 0)
			mw->m_BulletEntitiesToUpdate.Remove(ent);

		if((oldFlags & EV_POSTSIMULATE) == 0 && (newFlags & EV_POSTSIMULATE) != 0)
			mw->m_BulletEntitiesAfterUpdate.Insert(ent);
		else if((oldFlags & EV_POSTSIMULATE) == 0 && (newFlags & EV_POSTSIMULATE) != 0)
			mw->m_BulletEntitiesAfterUpdate.Remove(ent);

	}
}

//-----------------------------------------------------------------------
BulletController::~BulletController()
{
	btCollisionObject* obj = GetCollisionObject();
	if(obj)
	{
		Object* ent = (Object*)obj->getUserPointer();
    BulletWorld* mw = (BulletWorld*)ent->GetWorldPhysics();

		Assert(!mw->IsLocked());
		Assert(ent->GetPhysics() == this || ent->GetPhysics()->HasType(BulletRagdollControllerTypeID));

		if(mw)
		{
      mw->m_BulletEntitiesToUpdate.Remove(ent);
			mw->m_BulletEntitiesAfterUpdate.Remove(ent);

			if(btRigidBody::upcast(obj))
				mw->GetBulletWorld()->removeRigidBody(btRigidBody::upcast(obj));
			else
				mw->GetBulletWorld()->removeCollisionObject(obj);
		}

		btCollisionShape* shape = obj->getCollisionShape();
		if(shape)
		{
			if(shape->getShapeType() == COMPOUND_SHAPE_PROXYTYPE)
			{
				btCompoundShape* cshape = (btCompoundShape*)shape;

				for(int n = 0; n < cshape->getNumChildShapes(); n++)
				{
					if(cshape->getChildShape(n)->getUserPointer() == NULL)
						delete cshape->getChildShape(n);
				}
				delete cshape;
			}
			else
			{
				if(shape->getUserPointer() == NULL)
					delete shape;
			}
		}
    
    delete obj;
		obj = 0;
	}
	SAFE_RELEASE(m_pGeometries);
}

//-----------------------------------------------------------------------
BulletDynamicController::~BulletDynamicController()
{
}

//-----------------------------------------------------------------------
BulletController::BulletController(): m_pBody(NULL), m_pGeometries(NULL)
{
}

//-----------------------------------------------------------------------
BulletController::BulletController(btCollisionObject* body): m_pBody(body), m_pGeometries(NULL)
{
}

//-----------------------------------------------------------------------
uint BulletController::GetNumElements() const
{
	btCollisionShape* shape = m_pBody->getCollisionShape();
	if(shape->getShapeType() == COMPOUND_SHAPE_PROXYTYPE)
	{
		const btCompoundShape* cs = (const btCompoundShape*)shape;

		return cs->getNumChildShapes();
	}
	else
	{
		return 1;
	}
}

//-----------------------------------------------------------------------
uint BulletController::FindElement(CStr elementName) const
{
	btCollisionShape* shape = m_pBody->getCollisionShape();
	if(shape->getShapeType() == COMPOUND_SHAPE_PROXYTYPE)
	{
		const btCompoundShape* cs = (const btCompoundShape*)shape;

		for(int n = 0; n < cs->getNumChildShapes(); n++)
		{
			if(cs->getChildShape(n)->getUserPointer() == elementName.cstr())
				return n;
		}
	}
	else
	{
		if(shape->getUserPointer() == elementName.cstr())
			return 0;
	}

	return INDEX_NOT_FOUND;
}

//-----------------------------------------------------------------------
void BulletController::SetInteractionLayer(uint mask, int geom)
{
	btCollisionObject* body = m_pBody;
	btCollisionShape* shape = body->getCollisionShape();
	bool doUpdate = false;

	if(geom == -1)
	{
		if(mask != body->getInteractionLayer())
		{
			body->setInteractionLayer(mask);
			doUpdate = true;
		}
	}
	else
	{
		if(shape->getShapeType() == COMPOUND_SHAPE_PROXYTYPE)
		{
			btCompoundShape* cshape = (btCompoundShape*)shape;
			Assert(geom < cshape->getNumChildShapes());
			if(cshape->getChildInteractionLayer(geom) != mask)
			{
				cshape->setChildInteractionLayer(geom, mask);
				uint newMask = m_pBody->getInteractionLayer() | mask;
				if(newMask != body->getInteractionLayer())
				{
					body->setInteractionLayer(newMask);
					doUpdate = true;
				}
			}
		}
		else
		{
			if(mask != body->getInteractionLayer())
			{
				body->setInteractionLayer(mask);
				doUpdate = true;
			}
		}
	}

	if(!doUpdate)
		return;

	GetWorld()->Synchronize(body);
}

//-----------------------------------------------------------------------
BulletDynamicController::BulletDynamicController()
{
}

//-----------------------------------------------------------------------
BulletDynamicController::BulletDynamicController(btRigidBody* body): BulletController(body)
{
}

//-----------------------------------------------------------------------
BulletDynamicControllerNC::BulletDynamicControllerNC(): m_vCOMOffset(0.0f)
{
}

//-----------------------------------------------------------------------
BulletDynamicControllerNC::BulletDynamicControllerNC(btRigidBody* body, const Vector3& comOffset): BulletDynamicController(body), m_vCOMOffset(comOffset)
{
}

//-----------------------------------------------------------------------
BulletWorld* BulletController::GetWorld() const
{
	Object* ent = (Object*)GetCollisionObject()->getUserPointer();
  return (BulletWorld*)ent->GetWorldPhysics();//GetWorld()->GetPhysics();
}

//-----------------------------------------------------------------------
Object* BulletController::GetOwner() const
{
	return (Object*)GetCollisionObject()->getUserPointer();
}

//-----------------------------------------------------------------------
Object* BulletController::GetOwnerInline() const
{
	Object* ent = (Object*)GetCollisionObject()->getUserPointer();
	Assert(ent);
	return ent;
}

//-----------------------------------------------------------------------
bool BulletController::TraceMove(BaseTrace* trace)
{
  if(trace->Len == 0)
		return false;

	Object* owner = GetOwnerInline();
	BulletWorld* mw = (BulletWorld*)owner->GetWorldPhysics();

	if(!mw->GetBulletWorld()->getBroadphase()->getOverlappingPairCache()->hasInteractionEnabled(trace->InteractionLayer, m_pBody->getInteractionLayer()))
		return false;

	btVector3 btStart = BulletWorld::Point2btVector(trace->Start);
	btVector3 btStop = BulletWorld::Point2btVector(trace->EndPos);
	btTransform trStart;
	trStart.getBasis().setIdentity();
	trStart.setOrigin(btStart);
	btTransform trStop;
	trStop.getBasis().setIdentity();
	trStop.setOrigin(btStop);
	btCollisionShape* shape = m_pBody->getCollisionShape();

	btCollisionWorld::ClosestRayResultCallback rrc(btStart, btStop);

	mw->GetBulletWorld()->getBroadphase()->getOverlappingPairCache()->setCurrent();

	mw->GetBulletWorld()->rayTestSingle(trStart, trStop,
										m_pBody,
										shape,
										m_pBody->getWorldTransform(),
										rrc,
										trace->InteractionLayer);

	if(rrc.hasHit())
	{
		Vector3 endpos = trace->Start.Lerp(trace->EndPos, rrc.m_closestHitFraction);
    
		trace->Plane.x = rrc.m_hitNormalWorld.getX();
		trace->Plane.y = rrc.m_hitNormalWorld.getY();
		trace->Plane.z = rrc.m_hitNormalWorld.getZ();
		trace->Plane.w = trace->Plane.Dot(endpos);
#pragma message("jz todo: add content type")
		trace->Contents = C_SOLID;
		uint surfParms = 0;
		if(rrc.m_material)
		{
			const BulletPhysicsMaterial* pmat = (const BulletPhysicsMaterial*)rrc.m_material->getUserPointer();
			if(pmat)
			{
				surfParms = pmat->m_iMaterialIndex;
			}
		}
		else if(rrc.m_collisionShape)
		{
			const btMaterial* mat = rrc.m_collisionShape->getMaterial();
			if(mat)
			{
				const BulletPhysicsMaterial* pmat = (const BulletPhysicsMaterial*)mat->getUserPointer();
				if(pmat)
				{
					surfParms = pmat->m_iMaterialIndex;
				}
			}
		}

		trace->SurfParms = surfParms;
		trace->ClipEntity = owner;
		trace->MaterialPtr = NULL;
		trace->Bone = -1;
		trace->Fraction = rrc.m_closestHitFraction;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------
bool BulletController::Intersects(const Vector3& start, const Vector3& stop, uint interactionLayers)
{
  BulletWorld* mw = (BulletWorld*)GetOwnerInline()->GetWorldPhysics();//GetWorld()->GetPhysics();

	if(!mw->GetBulletWorld()->getBroadphase()->getOverlappingPairCache()->hasInteractionEnabled(interactionLayers, m_pBody->getInteractionLayer()))
		return false;

	btVector3 btStart = BulletWorld::Point2btVector(start);
	btVector3 btStop = BulletWorld::Point2btVector(stop);
	btCollisionShape* shape = m_pBody->getCollisionShape();

	mw->GetBulletWorld()->getBroadphase()->getOverlappingPairCache()->setCurrent();
#if 1
	return mw->GetBulletWorld()->rayCheckSingle(btStart, btStop,
										m_pBody,
										shape,
										m_pBody->getWorldTransform(),
										interactionLayers);
#else
	btCollisionWorld::ClosestRayResultCallback rrc(btStart, btStop);

	mw->GetBulletWorld()->getBroadphase()->getOverlappingPairCache()->setCurrent();
	btTransform trStart;
	trStart.getBasis().setIdentity();
	trStart.setOrigin(btStart);
	btTransform trStop;
	trStop.getBasis().setIdentity();
	trStop.setOrigin(btStop);

	mw->GetBulletWorld()->rayTestSingle(trStart, trStop,
										m_pBody,
										shape,
										m_pBody->getWorldTransform(),
										rrc,
										interactionLayers);

	return rrc.hasHit();
#endif
}


//-----------------------------------------------------------------------
bool BulletController::IsActive() const
{
	return GetCollisionObject()->isActive();
}

//-----------------------------------------------------------------------
bool BulletController::SetActive(ActiveState act)
{
	bool ret = GetCollisionObject()->isActive();
	if(act == AS_ALWAYS_ACTIVE)
		GetCollisionObject()->setActivationState(DISABLE_DEACTIVATION);
	else if(act == AS_INACTIVE)
		GetCollisionObject()->setActivationState(ISLAND_SLEEPING);
	else
		GetCollisionObject()->activate(true);
	return ret;
}

//-----------------------------------------------------------------------
bool BulletController::AddGeoms(PhysicsGeomDef* geoms, uint numGeoms, uint interactionLayers)
{
	Assert(geoms);
	if(numGeoms == 0)
		return false;

	const Vector3& offset = GetCenterOfMass();

	btCollisionObject* rb = GetCollisionObject();
	Assert(rb);

	bool isOk = true;
	uint layersMask = 0;
	if(rb->getCollisionShape())
	{
		layersMask = rb->getInteractionLayer();
	}

	bool aabbTree = false;
  /*
  uint numGeomsForAabb = IsDynamic() ? 9 : 6;
	if(numGeoms > numGeomsForAabb)
	{
		aabbTree = true;
		//if we have animated geoms, do not use aabb tree
		for(uint n = 0; n < numGeoms; n++)
		{
			if(geoms[n].m_iParentNode != 0)
			{
				aabbTree = false;
				break;
			}
		}
	}
  */
  
	BulletPhysicsMaterial* tmpMaterial = (BulletPhysicsMaterial*)GetWorld()->GetManager()->FindMaterial("material/default");
	
	//if we already had some shape set and it's not compound
	if(rb->getCollisionShape() && rb->getCollisionShape()->getShapeType() != COMPOUND_SHAPE_PROXYTYPE)
	{
		btCompoundShape* cshape = new btCompoundShape(aabbTree);
		
		BulletPhysicsMaterial* material = (BulletPhysicsMaterial*)geoms[0].m_pMaterial;
		if(!material)
			material = tmpMaterial;
		
		cshape->setMaterial(material->m_pBtMaterial);
		cshape->addChildShape(BulletWorld::Matrix2btTransform(M4IdentityP), rb->getCollisionShape(), rb->getInteractionLayer());
		rb->setCollisionShape(cshape);
	}

	if(numGeoms > 1 || !geoms[0].m_Frame.IsIdentity() || rb->getCollisionShape())
	{
		btCompoundShape* cshape = (btCompoundShape*)rb->getCollisionShape();
		if(cshape == NULL)
			cshape = new btCompoundShape(aabbTree);

		for(uint n = 0; n < numGeoms; n++)
		{
			btCollisionShape* shape = (btCollisionShape*)geoms[n].m_pGeom;
		
			BulletPhysicsMaterial* material = (BulletPhysicsMaterial*)geoms[n].m_pMaterial;
			if(!material)
				material = tmpMaterial;
			
			if(shape == NULL || material == NULL)
			{
				isOk = false;
				continue;
			}
			Matrix43 mat = geoms[n].m_Frame;
      mat.SetTranslation(mat.Position() - offset);
			btTransform transform = BulletWorld::Matrix2btTransform(mat);

			Assert(material->m_pBtMaterial);
			shape->setMaterial(material->m_pBtMaterial);
			layersMask |= geoms[n].m_iInteractionLayer;
			
			//need not be, but for safety..
			if(cshape->getMaterial() == NULL)
				cshape->setMaterial(material->m_pBtMaterial);
			geoms[n].m_iElementIndex = cshape->addChildShape(transform, shape, geoms[n].m_iInteractionLayer & interactionLayers);
		}

		rb->setCollisionShape(cshape);
	}
	else
	{
		btTransform transform = BulletWorld::Matrix2btTransform(geoms[0].m_Frame);
		btCollisionShape* shape = (btCollisionShape*)geoms[0].m_pGeom;
		BulletPhysicsMaterial* material = tmpMaterial;//(BulletPhysicsMaterial*)geoms[0].m_pMaterial;
		if(shape == NULL || material == NULL)
		{
			isOk = false;
		}
		else
		{
			layersMask |= geoms[0].m_iInteractionLayer;
			shape->setMaterial(material->m_pBtMaterial);
			rb->setCollisionShape(shape);
			geoms[0].m_iElementIndex = 0;
		}
	}
	rb->setInteractionLayer(layersMask & interactionLayers);
	
	Assert(isOk);
	return isOk;
}

//-----------------------------------------------------------------------
Vector3 BulletDynamicController::GetVelocity() const
{
	const btVector3& vel = GetRigidBody()->getLinearVelocity();
	return BulletWorld::btVector2Point(vel);
}

//-----------------------------------------------------------------------
void BulletDynamicController::SetVelocity(const Vector3& vel)
{
	Assert(!BadVec(vel));
	btRigidBody* rb = GetRigidBody();
	rb->setLinearVelocity(BulletWorld::Point2btVector(vel));
	if(!vel.IsZero())
		rb->activate();
}

//-----------------------------------------------------------------------
void BulletDynamicController::SetAngularVelocity(const Vector3& vel)
{
	Assert(!BadVec(vel));
	btRigidBody* rb = GetRigidBody();
	rb->setAngularVelocity(BulletWorld::Point2btVector(vel));
	if(!vel.IsZero())
		rb->activate();
}

//-----------------------------------------------------------------------
void BulletDynamicController::AddVelocity(const Vector3& vel)
{
	Assert(!BadVec(vel));
	if(!vel.IsZero())
	{
		btRigidBody* rb = GetRigidBody();
		rb->setLinearVelocity(BulletWorld::Point2btVector(vel) + rb->getLinearVelocity());
		rb->activate();
	}
}

//-----------------------------------------------------------------------
Vector3 BulletDynamicController::GetVelocityAt(const Vector3& pos) const
{
	btRigidBody* rb = GetRigidBody();
	btVector3 lpos = rb->getWorldTransform().invXform(BulletWorld::Point2btVector(pos));
	return BulletWorld::btVector2Point(rb->getVelocityInLocalPoint(lpos));
}

//-----------------------------------------------------------------------
Vector3 BulletDynamicController::GetAngularVelocity() const
{
	return BulletWorld::btVector2Point(GetRigidBody()->getAngularVelocity());
}

//-----------------------------------------------------------------------
void BulletDynamicController::ApplyImpulseAt(const Vector3& pos, const Vector3& impulse)
{
	Assert(!BadVec(impulse));
	if(!impulse.IsZero())
	{
		Assert(!BadVec(pos));
		btRigidBody* rb = GetRigidBody();
		btVector3 lpos = rb->getWorldTransform().invXform(BulletWorld::Point2btVector(pos));
		rb->applyImpulse(BulletWorld::Point2btVector(impulse), lpos);
		rb->activate();
	}
}

//-----------------------------------------------------------------------
void BulletDynamicController::ApplyImpulse(const Vector3& impulse, const bool activate)
{
	Assert(!BadVec(impulse));
	if(!impulse.IsZero())
	{
		btRigidBody* rb = GetRigidBody();
		rb->applyCentralImpulse(BulletWorld::Point2btVector(impulse));
		if(activate)
			rb->activate();
	}
}

//-----------------------------------------------------------------------
void BulletDynamicController::ApplyForce(const Vector3& force)
{
	Assert(!BadVec(force));
	if(!force.IsZero())
	{
		btRigidBody* rb = GetRigidBody();
		rb->applyCentralForce(BulletWorld::Vector2btVector(force));
		rb->activate();
	}
}

//-----------------------------------------------------------------------
void BulletDynamicController::ApplyForceAt(const Vector3& pos, const Vector3& force)
{
	Assert(!BadVec(force));
	if(!force.IsZero())
	{
		Assert(!BadVec(pos));
		btRigidBody* rb = GetRigidBody();
		btVector3 lpos = rb->getWorldTransform().invXform(BulletWorld::Point2btVector(pos));
		rb->applyForce(BulletWorld::Vector2btVector(force), lpos);
		rb->activate();
	}
}

//-----------------------------------------------------------------------
void BulletDynamicController::ApplyTorque(const Vector3& torque)
{
	Assert(!BadVec(torque));
	if(!torque.IsZero())
	{
		btRigidBody* rb = GetRigidBody();
		rb->applyTorque(BulletWorld::Vector2btVector(torque));
		rb->activate();
	}
}

//-----------------------------------------------------------------------
void BulletDynamicController::SetMass(float mass)
{
	RlsAssert(!BadFloat(mass));
	btVector3 inertia(0.0f, 0.0f, 0.0f);
	btRigidBody* rb = GetRigidBody();
	rb->getCollisionShape()->calculateLocalInertia(mass, inertia);
	rb->setMassProps(mass, inertia);
}

//-----------------------------------------------------------------------
float BulletDynamicController::GetMass() const
{
	float imass = GetRigidBody()->getInvMass();
	return (imass == 0.0f) ? 0.0f : (1.0f / imass);
}

//-----------------------------------------------------------------------
void BulletDynamicController::SetInertiaTensor(const Matrix33& mat)
{
	//GetRigidBody()->setInertiaTensor(meq::InertiaTensor(mat._11, mat._22, mat._33, mat._21, mat._31, mat._32));
}

//-----------------------------------------------------------------------
void BulletDynamicController::SetInertiaTensor(const Vector3& inertia)
{
	btVector3 diagInvInertia(inertia.X() != 0.0f ? btScalar(1.0) / inertia.X(): btScalar(0.0),
				   inertia.Y() != btScalar(0.0) ? btScalar(1.0) / inertia.Y(): btScalar(0.0),
				   inertia.Z() != btScalar(0.0) ? btScalar(1.0) / inertia.Z(): btScalar(0.0));

	GetRigidBody()->setInvInertiaDiagLocal(diagInvInertia);
}

//-----------------------------------------------------------------------
void BulletDynamicController::ComputeInertiaTensor()
{
	btRigidBody* rb = GetRigidBody();
	btVector3 inertia(0.0f, 0.0f, 0.0f);
	btScalar mass = btScalar(1.0f) / rb->getInvMass();
	rb->getCollisionShape()->calculateLocalInertia(mass, inertia);
	rb->setMassProps(mass, inertia);
}

//-----------------------------------------------------------------------
bool BulletDynamicController::SetDynamic(bool dynamic)
{
	bool ret = GetRigidBody()->isStaticObject();

	if(!dynamic)
		GetRigidBody()->setCollisionFlags(GetRigidBody()->getCollisionFlags() | (btCollisionObject::CF_STATIC_OBJECT|btCollisionObject::CF_NO_CONTACT_RESPONSE));
	else
		GetRigidBody()->setCollisionFlags(GetRigidBody()->getCollisionFlags() & ~(btCollisionObject::CF_STATIC_OBJECT|btCollisionObject::CF_NO_CONTACT_RESPONSE));

	return ret;
}

//-----------------------------------------------------------------------
bool BulletDynamicController::IsDynamic() const
{
	const btRigidBody* rb = GetRigidBody();

	return !rb->isStaticOrKinematicObject();
}

//-----------------------------------------------------------------------
bool BulletDynamicController::SetLinearFactor(const Vector3& linearFactor)
{
	btRigidBody* rb = GetRigidBody();
	rb->setLinearFactor(BulletWorld::Vector2btVector(linearFactor));
	return true;
}

//-----------------------------------------------------------------------
bool BulletDynamicController::SetSleepingTreshold(float linearTreshold, float angularTreshold)
{
	btRigidBody* rb = GetRigidBody();
	rb->setSleepingThresholds(linearTreshold, angularTreshold);
	return true;
}

//-----------------------------------------------------------------------
bool BulletController::SetSolid(bool enableSolid)
{
	btCollisionObject* obj = GetCollisionObject();

	int flags = obj->getCollisionFlags();
	if(enableSolid)
		flags &= ~btCollisionObject::CF_NO_CONTACT;
	else
		flags |= btCollisionObject::CF_NO_CONTACT;
	obj->setCollisionFlags(flags);
	return true;
}

//-----------------------------------------------------------------------
bool BulletDynamicController::SetCollisionResponse(bool enableResponse)
{
	btRigidBody* rb = GetRigidBody();

	int flags = rb->getCollisionFlags();
	if(enableResponse)
		flags &= ~btCollisionObject::CF_NO_COLLISION_RESPONSE;
	else
		flags |= btCollisionObject::CF_NO_COLLISION_RESPONSE;
	rb->setCollisionFlags(flags);
	return true;
}

//-----------------------------------------------------------------------
void BulletController::SetMatrix(const Matrix43& mat)
{
	RlsAssert(!BadMatrix43(mat));
	/*
  Assert(mat[3].x >= (float)BaseWorld::MIN_SIZE && mat[3].x <= (float)BaseWorld::MAX_SIZE);
	Assert(mat[3].y >= (float)BaseWorld::MIN_SIZE && mat[3].y <= (float)BaseWorld::MAX_SIZE);
	Assert(mat[3].z >= (float)BaseWorld::MIN_SIZE && mat[3].z <= (float)BaseWorld::MAX_SIZE);
  */
	GetCollisionObject()->setWorldTransform(BulletWorld::Matrix2btTransform(mat));
}

//-----------------------------------------------------------------------
void BulletDynamicController::SetMatrix(const Matrix43& mat)
{
	RlsAssert(!BadMatrix43(mat));
	/*
  Assert(mat[3].x >= (float)BaseWorld::MIN_SIZE && mat[3].x <= (float)BaseWorld::MAX_SIZE);
	Assert(mat[3].y >= (float)BaseWorld::MIN_SIZE && mat[3].y <= (float)BaseWorld::MAX_SIZE);
	Assert(mat[3].z >= (float)BaseWorld::MIN_SIZE && mat[3].z <= (float)BaseWorld::MAX_SIZE);
  */
	GetCollisionObject()->setWorldTransform(BulletWorld::Matrix2btTransform(mat));
}

//-----------------------------------------------------------------------
void BulletDynamicControllerNC::SetMatrix(const Matrix43& mat)
{
	RlsAssert(!BadMatrix43(mat));
  /*
  Assert(mat[3].x >= (float)BaseWorld::MIN_SIZE && mat[3].x <= (float)BaseWorld::MAX_SIZE);
	Assert(mat[3].y >= (float)BaseWorld::MIN_SIZE && mat[3].y <= (float)BaseWorld::MAX_SIZE);
	Assert(mat[3].z >= (float)BaseWorld::MIN_SIZE && mat[3].z <= (float)BaseWorld::MAX_SIZE);
  */
	GetCollisionObject()->setWorldTransform(BulletWorld::Matrix2btTransform(mat, m_vCOMOffset));
}

//-----------------------------------------------------------------------
void BulletDynamicController::getWorldTransform(btTransform& transform) const
{
	Object* ent = GetOwnerInline();
	const Matrix43& mat = ent->IGetMatrix();
	RlsAssert(!BadMatrix43(mat));
	transform = BulletWorld::Matrix2btTransform(mat);
}

//-----------------------------------------------------------------------
void BulletDynamicController::setWorldTransform(const btTransform& transform)
{
	Object* ent = GetOwnerInline();
	if(ent)
	{
		ent->SetTransform(BulletWorld::btTransform2Matrix(transform));
		BulletWorld* mw = (BulletWorld*)ent->GetWorldPhysics();
		mw->MoveBulletEntity(ent);
	}
}

//-----------------------------------------------------------------------
void BulletDynamicControllerNC::getWorldTransform(btTransform& transform) const
{
	Object* ent = GetOwnerInline();
	const Matrix43& mat = ent->IGetMatrix();
	RlsAssert(!BadMatrix43(mat));
	transform = BulletWorld::Matrix2btTransform(mat, m_vCOMOffset);
}

//-----------------------------------------------------------------------
void BulletDynamicControllerNC::setWorldTransform(const btTransform& transform)
{
  Object* ent = GetOwnerInline();
	if(ent)
	{
		ent->SetTransform(BulletWorld::btTransform2Matrix(transform));
		BulletWorld* mw = (BulletWorld*)ent->GetWorldPhysics();
		mw->MoveBulletEntity(ent);
	}
}

//-----------------------------------------------------------------------
Matrix43 BulletDynamicControllerNC::GetWorldTransform() const
{
	return BulletWorld::btTransform2Matrix(GetCollisionObject()->getWorldTransform(), m_vCOMOffset);
}

//-----------------------------------------------------------------------
BulletVehicleController::BulletVehicleController():
	m_pVehicleRaycaster(NULL),
	m_pVehicle(NULL)
{
}

//-----------------------------------------------------------------------
BulletVehicleController::~BulletVehicleController()
{
	GetWorld()->GetBulletWorld()->removeAction(m_pVehicle);
	SAFE_DELETE(m_pVehicle);
	SAFE_DELETE(m_pVehicleRaycaster);
}

//-----------------------------------------------------------------------
BulletVehicleController::BulletVehicleController(btRigidBody* rb, const Vector3& comOffset): BulletDynamicControllerNC(rb, comOffset)
{
}

//-----------------------------------------------------------------------
float BulletVehicleController::GetSpeed() const
{
	return m_pVehicle->getCurrentSpeedKmHour();
}


//-----------------------------------------------------------------------
btRaycastVehicle::btVehicleTuning Convert(const VehicleWheelParams& params)
{
	btRaycastVehicle::btVehicleTuning tuning;
	
	tuning.m_suspensionStiffness = params.m_fSuspensionStiffness;
	tuning.m_suspensionCompression = params.m_fSuspensionCompression;
	tuning.m_suspensionDamping = params.m_fSuspensionDamping;
	tuning.m_maxSuspensionTravel = params.m_fMaxSuspensionTravel * MEQ_WORLDISCALE;
	return tuning;
}

//-----------------------------------------------------------------------
VehicleWheelParams::VehicleWheelParams():
	m_fSuspensionStiffness(20.f),
	m_fSuspensionCompression(4.4f),
	m_fSuspensionDamping(2.3f),
	m_fMaxSuspensionTravel(32.f),
	m_fRollInfluence(0.1f)
{
}

//-----------------------------------------------------------------------
uint BulletVehicleController::GetNumWheels() const
{
	return m_pVehicle->getNumWheels();
}

//-----------------------------------------------------------------------
void BulletVehicleController::AddWheel(const Vector3& lpos, const Vector3& suspensionDir, const Vector3& axleDir, float radius, const VehicleWheelParams& params, const PhysicsMaterial* mat)
{
	Vector3 suspDir = suspensionDir;
	float suspensionLength = suspDir.Normalize();
	btRaycastVehicle::btVehicleTuning tuning = Convert(params);

	const BulletPhysicsMaterial* mbmat = static_cast<const BulletPhysicsMaterial*>(mat);
	tuning.m_btMaterial = mbmat->m_pBtMaterial;

	btWheelInfo& info = m_pVehicle->addWheel(
		BulletWorld::Point2btVector(lpos),
		BulletWorld::Vector2btVector(suspDir),
		BulletWorld::Vector2btVector(axleDir),
		btScalar(suspensionLength) * MEQ_WORLDISCALE,
		btScalar(radius) * MEQ_WORLDISCALE + BT_MARGIN,
		tuning,
		false);

	info.m_rollInfluence = params.m_fRollInfluence;
}

//-----------------------------------------------------------------------
void BulletVehicleController::ApplyEngineForce(uint wheel, float force)
{
	m_pVehicle->applyEngineForce(btScalar(force), wheel);
}

//-----------------------------------------------------------------------
void BulletVehicleController::SetBrake(uint wheel, float brake)
{
	RlsAssert(!BadFloat(brake));
	m_pVehicle->setBrake(btScalar(brake), wheel);
}

//-----------------------------------------------------------------------
void BulletVehicleController::SetSteering(uint wheel, float steering)
{
	RlsAssert(!BadFloat(steering));
	m_pVehicle->setSteeringValue(btScalar(steering), wheel);
}

//-----------------------------------------------------------------------
float BulletVehicleController::GetSteering(uint wheel) const
{
	return m_pVehicle->getSteeringValue(wheel);
}

//-----------------------------------------------------------------------
Object* BulletVehicleController::HasWheelContact(uint wheel) const
{
	if(m_pVehicle->getWheelInfo(wheel).m_raycastInfo.m_isInContact != WC_NOCONTACT)
	{
		btCollisionObject* obj = (btCollisionObject*)m_pVehicle->getWheelInfo(wheel).m_raycastInfo.m_groundObject;
		if(obj && obj->getUserPointer())
		{
			return (Object*)obj->getUserPointer();
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------
bool BulletVehicleController::GetWheelContact(uint wheel, WheelContactInfo& contactInfo) const
{
	const btWheelInfo& wheelInfo = m_pVehicle->getWheelInfo(wheel);
	const btWheelInfo::RaycastInfo& raycastInfo = wheelInfo.m_raycastInfo;

	contactInfo.m_vEndPos = BulletWorld::btVector2Point(raycastInfo.m_contactPointWS);
	contactInfo.m_fSuspensionLength = MEQ_WORLDSCALE * raycastInfo.m_suspensionLength;
	contactInfo.m_Type = WCT_NO_CONTACT;

	if(wheelInfo.m_raycastInfo.m_isInContact != WC_NOCONTACT)
	{
		btCollisionObject* obj = raycastInfo.m_groundObject;
		if(obj && obj->getUserPointer())
		{
			contactInfo.m_fFraction = raycastInfo.m_fraction;
			contactInfo.m_vContactNormal = BulletWorld::btVector2Vector(raycastInfo.m_contactNormalWS);
			contactInfo.m_pContactEntity = (Object*)obj->getUserPointer();
			contactInfo.m_pMaterial = (const PhysicsMaterial*)raycastInfo.m_material->getUserPointer();
			switch(raycastInfo.m_isInContact)
			{
			case WC_SOLID_GROUND:
				contactInfo.m_Type = WCT_SOLID;
				break;
			case WC_WATER:
				contactInfo.m_Type = WCT_WATER;
				break;
			}				
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------
Vector3 BulletVehicleController::GetWheelEndPosition(uint wheel) const
{
	const btWheelInfo& wheelInfo = m_pVehicle->getWheelInfo(wheel);

	btScalar raylen = wheelInfo.getSuspensionRestLength() + wheelInfo.m_wheelsRadius;
	btVector3 rayvector = wheelInfo.m_raycastInfo.m_wheelDirectionWS * (raylen);
	btTransform chassisTrans = m_pVehicle->getChassisWorldTransform();
	btVector3 hardPointWS = chassisTrans( wheelInfo.m_chassisConnectionPointCS );
	btVector3 target = hardPointWS + rayvector;

	return BulletWorld::btVector2Point(target);
}

//-----------------------------------------------------------------------
void BulletVehicleController::SetWaterLevel(float level, const PhysicsMaterial* mat, PhysicsController* obj)
{
	if(mat)
		m_pVehicle->setWaterLevel(level * MEQ_WORLDISCALE, ((const BulletPhysicsMaterial*)mat)->m_pBtMaterial, ((BulletController*)obj)->GetCollisionObject());
	else
		m_pVehicle->setWaterLevel(-FLT_MAX, NULL, NULL);
}

//-----------------------------------------------------------------------
float BulletVehicleController::GetWheelAngle(uint wheel) const
{
	return m_pVehicle->getWheelInfo(wheel).m_rotation;
}
//-----------------------------------------------------------------------
float BulletVehicleController::GetWheelAngularVelocity(uint wheel) const
{
	return m_pVehicle->getWheelInfo(wheel).m_rotation;
}

//-----------------------------------------------------------------------
float BulletVehicleController::GetWheelSuspensionLength(uint wheel) const
{
	return m_pVehicle->getWheelInfo(wheel).m_raycastInfo.m_suspensionLength;
}

//-----------------------------------------------------------------------
void BulletController::SetCCDThreshold(float maxMotion, float sweptRadius)
{
	RlsAssert(!BadFloat(maxMotion) && !BadFloat(sweptRadius));
	btCollisionObject* obj = GetCollisionObject();
	obj->setCcdMotionThreshold(maxMotion * MEQ_WORLDISCALE);
	obj->setCcdSweptSphereRadius(sweptRadius * MEQ_WORLDISCALE);
}

//-----------------------------------------------------------------------
float BulletController::GetCCDRadius()
{
  btCollisionObject* obj = GetCollisionObject();
  return obj->getCcdSweptSphereRadius();
}

//-----------------------------------------------------------------------
bool BulletController::EnableCCD(bool enable)
{
	btCollisionObject* obj = GetCollisionObject();
	if(enable && obj->getCollisionShape()->isConvex())
	{
		if(obj->getCollisionShape()->getShapeType() == SPHERE_SHAPE_PROXYTYPE)
		{
			btSphereShape* sphere = (btSphereShape*)obj->getCollisionShape();
			float radius = sphere->getRadius() * MEQ_WORLDSCALE;
			SetCCDThreshold(0.000001f, radius * 0.45f);
		}
		else
		{
			btVector3 mins, maxs;
			
			obj->getCollisionShape()->getAabb(btTransform::getIdentity(), mins, maxs);
			btVector3 size = maxs - mins;
			btScalar minDiameter = btMin(btMin(size.getX(), size.getY()), size.getZ()) * MEQ_WORLDSCALE;
			btScalar minRadius = minDiameter * 0.5f;
			SetCCDThreshold(0.000001f, minRadius * 0.45f);
		}
		return true;
	}
	SetCCDThreshold(0,0);
	return false;
}

//-----------------------------------------------------------------------
//class EnforceVehicleRaycaster : public btVehicleRaycaster
//{
//	BaseWorld*	m_pWorld;
//	_WNode*		m_pTopNode;
//
//public:
//	EnforceVehicleRaycaster(BaseWorld* world)
//		:m_pWorld(world), m_pTopNode(NULL)
//	{
//	}
//	virtual void prepare(const btVector3& mins, const btVector3& maxs)
//	{
//		m_pTopNode = m_pWorld->FindBBoxTopNode(NULL, AAB(BulletWorld::btVector2Point(mins), BulletWorld::btVector2Point(maxs)));
//		//Shape::CreateBox(BulletWorld::btVector2Point(mins), BulletWorld::btVector2Point(maxs), 0x3fff0000, SS_ONCE|SS_TRANSP);
//	}
//
//	virtual btCollisionObject* castRay(btCollisionObject* exclude, const btVector3& from,const btVector3& to, btVehicleRaycasterResult& result)
//	{
//		Object* ent = (Object*)exclude->getUserPointer();
//
//		enf::MTrace trace;
//		enf::Vector3 start = BulletWorld::btVector2Point(from);
//		enf::Vector3 end = BulletWorld::btVector2Point(to);
//		trace.Init( ent, TRACE_LINE | TRACE_WORLD | TRACE_ENTS | TRACE_ONLY_PHYSICS, TFL_SOLID, exclude->getInteractionLayer());
//		if(m_pWorld->TraceMove( m_pTopNode, start, end, &trace))
//		{
//			result.m_distFraction = trace.Fraction;
//			result.m_hitNormalInWorld = BulletWorld::Vector2btVector(trace.Plane.v);
//			result.m_hitPointInWorld = BulletWorld::Point2btVector(trace.EndPos);
//			result.m_material = g_pBulletManager->GetBaseMaterial(trace.SurfParms.GetMaterialIndex())->m_pBtMaterial;
//			//Shape::CreateLine(start, trace.EndPos, 0xffffffff, SS_ONCE);
//			if(trace.ClipEntity && trace.ClipEntity->GetPhysics())
//			{
//				return ((BulletController*)trace.ClipEntity->GetPhysics())->GetCollisionObject();
//			}
//		}
//		return NULL;
//	}
//};
//
//class WaterSupportRaycaster: public btDefaultVehicleRaycaster
//{
//public:
//	static btScalar m_time;
//	static btScalar m_amplitudeScale;
//	static btScalar m_coordScale;
//
//	btScalar GetWaterLevel(btScalar x, btScalar y) const
//	{
//		btScalar s = sin(x * m_coordScale + m_time);
//		btScalar c = cos(y * m_coordScale + m_time);
//		return (s + c) * m_amplitudeScale;
//	}
//	WaterSupportRaycaster(btDynamicsWorld* dw, const btCollisionObject* exclude): btDefaultVehicleRaycaster(dw, exclude)
//	{
//	}
//};
//
//btScalar WaterSupportRaycaster::m_time = 0.0f;
//btScalar WaterSupportRaycaster::m_amplitudeScale = 0.1f;
//btScalar WaterSupportRaycaster::m_coordScale = 1.0f;

//-----------------------------------------------------------------------
void BulletWorld::UpdateTime(float timeSlice)
{
  ///no watter support so far
	/*
  WaterSupportRaycaster::m_time += timeSlice;
	while(WaterSupportRaycaster::m_time > ENF_PI2)
		WaterSupportRaycaster::m_time -= ENF_PI2;
  */
}

//-----------------------------------------------------------------------
BulletVehicleController* BulletWorld::CreateVehicle(Object *ent, const Vector3& trans, float mass, uint interactionLayer)
{
  RlsAssert2(0, "not implemented");
  return NULL;
  /*
	Assert(ent != NULL);
	Assert(m_pDynamicsWorld != NULL);

	//early exit
	BulletObjectGeometry* objgeom = GetObjectGeometry(ent);
	if(objgeom == NULL)
		return NULL;

	btRigidBody::btRigidBodyConstructionInfo constructionInfo(0.0f, NULL, NULL);
	constructionInfo.m_linearDamping = 0.2f;
	constructionInfo.m_angularDamping = 0.2f;
	constructionInfo.m_linearSleepingThreshold = 1.0f;
	constructionInfo.m_angularSleepingThreshold = 1.0f;
	btRigidBody* rb = new btRigidBody(constructionInfo);
	//by default, there is static flag set. It's wrongfully identified as static in AddGeoms
	rb->setCollisionFlags(0);

	Vector3 comOffset = trans;// + objgeom->GetCenterOfMass();

	CMutexLock lock(ent->GetWorld()->GetMutex());
	BulletVehicleController* controller = ENF_NEW BulletVehicleController(rb, comOffset);
	controller->m_pGeometries = objgeom;
	ent->SetPhysicsController(controller);
	rb->setUserPointer((void*)ent);

	//this initializes body transform too
	rb->setMotionState(controller);

	uint numGeoms = objgeom->GetNumElements();
	PhysicsGeomDef* defs = ALLOCAINIT(PhysicsGeomDef, numGeoms);
	objgeom->SetupPhysicsGeomDefs(defs, numGeoms);
	controller->AddGeoms(defs, numGeoms, interactionLayer);

	if(rb->getCollisionShape())
	{
		btVector3 inertia;
		rb->getCollisionShape()->calculateLocalInertia(mass, inertia);
		rb->setMassProps(mass, inertia);
	}
	
	controller->m_pVehicleRaycaster = new WaterSupportRaycaster(m_pDynamicsWorld, rb);
//	controller->m_pVehicleRaycaster = new EnforceVehicleRaycaster(m_pWorld);
	btRaycastVehicle* vehicle = controller->m_pVehicle = new btRaycastVehicle(rb,controller->m_pVehicleRaycaster);

	m_pDynamicsWorld->addAction(vehicle);

	int forwardIndex = 0;
	int rightIndex = 1;
	int upIndex = 2;

	vehicle->setCoordinateSystem(rightIndex,upIndex,forwardIndex);

	m_pDynamicsWorld->addRigidBody(rb);
	controller->UpdateEntityFlags(ent, 0, ent->GetFlags());
	controller->UpdateEntityEvents(ent, 0, ent->GetEventMask());
	return controller;
  */
}

//-----------------------------------------------------------------------
}