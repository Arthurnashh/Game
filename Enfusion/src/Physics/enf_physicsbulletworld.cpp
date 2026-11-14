//-----------------------------------------------------------------------------
// File: enf_physicsbullet.cpp
//
// Desc: Bullet implementation
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------
//#include "precomp.h"
//#include "enf_profiler.h"
//#include "common/enf_diag.h"
//#include "common/enf_serialize.h"
//#include "BSPWorld/enf_bspworld.h"
//#include "baseworld/enf_entity.h"
#include "baseworld/enf_baseworldtrace.h"
//#include "terrain/enf_terrain.h"
#include "physics/enf_physicsbulletinternal.h"
#include "baseworld/enf_shapes.h"
#include "physics/enf_btHeightFieldTerrainShape.h"
//#include "enf_scriptapi.h"
//#include "enf_scriptapivars.h"

#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h"
#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h"
#include "BulletCollision/BroadphaseCollision/btDbvtBroadphase.h"
#include "LinearMath/btIDebugDraw.h"
#include "LinearMath/btQuickprof.h"
#include "BulletDynamics/ConstraintSolver/btPoint2PointConstraint.h"
#include "BulletDynamics/ConstraintSolver/btHingeConstraint.h"
#include "BulletDynamics/ConstraintSolver/btConeTwistConstraint.h"
#include "BulletDynamics/ConstraintSolver/btSliderConstraint.h"
#include "BulletDynamics/ConstraintSolver/btGeneric6DofConstraint.h"
#include "BulletDynamics/ConstraintSolver/btGeneric6DofSpringConstraint.h"

#include "BulletCollision/CollisionShapes/btCapsuleShape.h"
#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"
#include "BulletCollision/CollisionShapes/btStaticPlaneShape.h"
#include "BulletCollision/CollisionShapes/btCylinderShape.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h"
#include "BulletCollision/CollisionShapes/btConvexHullShape.h"
#include "BulletCollision/CollisionShapes/btShapeHull.h"

#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "BulletDynamics/Vehicle/btRaycastVehicle.h"

#include "lib/landscape.hpp"
#include "lib/vehicle.hpp"
#include "lib/scenery.hpp"

#include <Es/Memory/normalNew.hpp>


namespace enf
{
class BulletDebugDraw: public btIDebugDraw
{
protected:
	uint	m_iDebugMask;

public:
	BulletDebugDraw() {}
	ENF_INLINE uint btColor2Color(const btVector3& color) const
	{
		int r = (int)(color.getX() * 255.0f);
		int g = (int)(color.getY() * 255.0f);
		int b = (int)(color.getZ() * 255.0f);

		uint col = 0xff000000 | (r << 16) | (g << 8) | b;
		return col;
	}

	void	drawLine(const btVector3& from,const btVector3& to,const btVector3& color)
	{
    RlsAssert2(0, "not implemented");
		//Shape::CreateLine(BulletWorld::btVector2Point(from), BulletWorld::btVector2Point(to), btColor2Color(color), SS_ONCE);
	}
	virtual void drawBox(const btVector3& bbMin, const btVector3& bbMax, const btVector3& color)
	{
    RlsAssert2(0, "not implemented");
		//Shape::CreateBox(BulletWorld::btVector2Point(bbMin), BulletWorld::btVector2Point(bbMax), btColor2Color(color), SS_ONCE|SS_WIREFRAME);
	}
	virtual void drawBox(const btVector3& bbMin, const btVector3& bbMax, const btTransform& trans, const btVector3& color)
	{
		RlsAssert2(0, "not implemented");
    //Shape* sh = Shape::CreateBox(BulletWorld::btVector2Point(bbMin), BulletWorld::btVector2Point(bbMax), btColor2Color(color), SS_ONCE|SS_WIREFRAME);
		//sh->SetMatrix(BulletWorld::btTransform2Matrix(trans));
	}
	virtual void drawAabb(const btVector3& from,const btVector3& to,const btVector3& color)
	{
		drawBox(from, to, color);
	}

	virtual void	drawTriangle(const btVector3& v0,const btVector3& v1,const btVector3& v2,const btVector3& color, btScalar /*alpha*/)
	{
    RlsAssert2(0, "not implemented");
    /*
		Vector3 verts[3];
		verts[0] = BulletWorld::btVector2Point(v0);
		verts[1] = BulletWorld::btVector2Point(v1);
		verts[2] = BulletWorld::btVector2Point(v2);
		Shape::CreateMesh(verts, 1, btColor2Color(color), SS_ONCE|SS_WIREFRAME);
    */
	}
	virtual void drawTransform(const btTransform& transform, btScalar orthoLen)
	{
    RlsAssert2(0, "not implemented");
    /*
		Vector3 start = BulletWorld::btVector2Point(transform.getOrigin());
		Vector3 axis0 = BulletWorld::btVector2Vector(transform.getBasis().getRow(0));
		Vector3 axis1 = BulletWorld::btVector2Vector(transform.getBasis().getRow(1));
		Vector3 axis2 = BulletWorld::btVector2Vector(transform.getBasis().getRow(2));
		float scale = orthoLen * MEQ_WORLDSCALE;

		Shape::CreateLine(start, start.MA(axis0, scale), 0xffff0000, SS_ONCE);
		Shape::CreateLine(start, start.MA(axis1, scale), 0xff00ff00, SS_ONCE);
		Shape::CreateLine(start, start.MA(axis2, scale), 0xff0000ff, SS_ONCE);
    */
	}

	virtual void	drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color)
	{
    RlsAssert2(0, "not implemented");
		//Shape::CreateLine(BulletWorld::btVector2Point(PointOnB), BulletWorld::btVector2Point(normalOnB * btScalar(2.0f) + PointOnB), btColor2Color(color), SS_ONCE);
	}
	virtual void	reportErrorWarning(const char* warningString)
	{
	}
	virtual void	draw3dText(const btVector3& location,const char* textString)
	{
	}
	virtual void	setDebugMode(int debugMode)
	{
		m_iDebugMask = debugMode;
	}
	virtual int		getDebugMode() const
	{
		return m_iDebugMask;
	}
};


//-----------------------------------------------------------------------
Object* BulletPhysicsJoint::GetEntityA()
{
	return (Object*)constraint()->getRigidBodyA().getUserPointer();
}

//-----------------------------------------------------------------------
Object* BulletPhysicsJoint::GetEntityB()
{
	return (Object*)constraint()->getRigidBodyB().getUserPointer();
}

//-----------------------------------------------------------------------
void BulletWorld::Serialize(SerializeInputAdapter* adapter)
{
}

//-----------------------------------------------------------------------
void BulletWorld::Serialize(SerializeOutputAdapter* adapter)
{
}

//-----------------------------------------------------------------------
PhysicsManager* BulletWorld::GetManager() const
{
	return m_pManager;
}

/*
//-----------------------------------------------------------------------
BulletObjectGeometry* BulletWorld::GetObjectGeometry(Object* ent) const
{
  //early exit
  VObject* obj = ent->GetResourceObject();
  if(obj == NULL)
		return NULL;

	BulletObjectGeometry* geom = BulletObjectGeometry::Cast(m_pManager->GetGeometry(CStr(obj->GetName()), obj));
  
	return geom;
}
*/
//-----------------------------------------------------------------------
PhysicsGeometry* BulletWorld::CreatePlaneGeom(Vector3 normal, float distance)
{
	Assert(!BadVec(normal));
	Assert(!BadFloat(distance));

	btStaticPlaneShape* shape = new btStaticPlaneShape(Vector2btVector(normal), btScalar(distance * MEQ_WORLDISCALE));
	return (PhysicsGeometry*)shape;
}

//-----------------------------------------------------------------------
PhysicsGeometry* BulletWorld::CreateBoxGeom(Vector3 size)
{
	Assert(!BadVec(size));
	btBoxShape* shape = new btBoxShape(Point2btVector(size * 0.5f));
	return (PhysicsGeometry*)shape;
}

//-----------------------------------------------------------------------
PhysicsGeometry* BulletWorld::CreateSphereGeom(float radius)
{
	Assert(!BadFloat(radius));
	btSphereShape* shape = new btSphereShape(radius * MEQ_WORLDISCALE);
	return (PhysicsGeometry*)shape;
}

//-----------------------------------------------------------------------
PhysicsGeometry* BulletWorld::CreateCylinderGeom(float radius, Vector3 extent)
{
	Assert(!BadVec(extent));
	Assert(!BadFloat(radius));

	btCylinderShape* shape = NULL;
	
  if(extent.Get(2) != 0.0f)
		shape = new btCylinderShapeZ(btVector3(radius * MEQ_WORLDISCALE, radius * MEQ_WORLDISCALE, extent.Get(2) * MEQ_WORLDISCALE));
	else if(extent.Get(0) != 0.0f)
		shape = new btCylinderShapeX(btVector3(extent.Get(0) * MEQ_WORLDISCALE, radius * MEQ_WORLDISCALE, radius * MEQ_WORLDISCALE));
	else if(extent.Get(1) != 0.0f)
		shape = new btCylinderShape(btVector3(radius * MEQ_WORLDISCALE, extent.Get(1) * MEQ_WORLDISCALE, radius * MEQ_WORLDISCALE));
	return (PhysicsGeometry*)shape;
}

//-----------------------------------------------------------------------
PhysicsGeometry* BulletWorld::CreateCapsuleGeom(float radius, Vector3 extent)
{	
	Assert(!BadVec(extent));
	Assert(!BadFloat(radius));

	btCapsuleShape* shape = NULL;
	
	if(extent.Get(2) != 0.0f)
		shape = new btCapsuleShapeZ(radius * MEQ_WORLDISCALE, extent.Get(2) * 2.0f * MEQ_WORLDISCALE);
	else if(extent.Get(0) != 0.0f)
		shape = new btCapsuleShapeX(radius * MEQ_WORLDISCALE, extent.Get(0) * 2.0f * MEQ_WORLDISCALE);
	else if(extent.Get(1) != 0.0f)
		shape = new btCapsuleShape(radius * MEQ_WORLDISCALE, extent.Get(1) * 2.0f * MEQ_WORLDISCALE);
	return (PhysicsGeometry*)shape;
}

//-----------------------------------------------------------------------
void BulletWorld::DestroyGeom(PhysicsGeometry* _geom)
{
	btCollisionShape* shape = (btCollisionShape*)_geom;
	delete shape;
}

//-----------------------------------------------------------------------
void BulletWorld::DestroyBody(Object *ent)
{
  Assert(ent != NULL && ent->GetPhysics() != NULL);
#pragma message("jz todo: give back lock")
	CMutexLock lock(ent->GetWorld()->GetMutex());
  //RemoveDynamicBodyFromSimulation(ent);
	ent->SetPhysicsController(NULL);
}

//-----------------------------------------------------------------------
void BulletWorld::Advance(float elapsedtime)
{
  RlsAssert(!BadFloat(elapsedtime));

	//world is not initialized, get out!
	if(m_pWorld == NULL)
		return;

#pragma message("jz todo: pushback physic ragdolls")
/*
	if(ENF_DIAG_VALUE(DM_PHYSICS_RAGDOLLS))
	{
		ENF_DIAG_SET(DM_PHYSICS_RAGDOLLS, 0);
		//FIXME: HACK! We don't know the filename
		GetManager()->LoadRagdolls("physics/ragdolls.h");
	}
*/
	//ENF_BEGIN_PROFILE(PROFILE_PHYSICS_ADVANCE);


	if(elapsedtime > 0.5f)
		elapsedtime = 0.5f;

	//empty world
	if(m_pDynamicsWorld->getCollisionObjectArray().size() == 0)
	{
		//ENF_END_PROFILE();
		return;
	}

	ScopeLock<PhysicsWorld> lock(*this);

  /*
	if(ENF_DIAG_BOOL(DM_FORCE_SLEEP))
	{
		btAlignedObjectArray<btCollisionObject*>& objects = m_pDynamicsWorld->getNonStaticCollisionObjects();

		for(int n = 0; n < objects.size(); n++)
		{
			objects[n]->setActivationState(ISLAND_SLEEPING);
		}
	}
  */

	if(0)//ENF_DIAG_BOOL(DM_SHOW_BULLET))
	{
		m_pDebugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe|
				btIDebugDraw::DBG_DrawAabb|
	//			btIDebugDraw::DBG_DrawFeaturesText|
				btIDebugDraw::DBG_DrawContactPoints|
	//			btIDebugDraw::DBG_NoDeactivation|
				btIDebugDraw::DBG_NoHelpText|
	//			btIDebugDraw::DBG_DrawText|
	//			btIDebugDraw::DBG_ProfileTimings|
	//			btIDebugDraw::DBG_EnableSatComparison|
	//			btIDebugDraw::DBG_DisableBulletLCP|
	//			btIDebugDraw::DBG_EnableCCD|
				btIDebugDraw::DBG_DrawConstraints|
				btIDebugDraw::DBG_DrawConstraintLimits|
				btIDebugDraw::DBG_FastWireframe
				);
	}
	else
	{
		m_pDebugDrawer->setDebugMode(0);
	}

	/*
  int doProfileBullet = ENF_DIAG_VALUE(DM_PROFILE_BULLET);

	if(doProfileBullet)
	{
		Timer::PushTime();
	}
  */

	m_BulletEntitiesToMove.Clear();

	if(elapsedtime > 0.0f)
  {
    m_pDynamicsWorld->stepSimulation(elapsedtime, 9 , 1.0f / 60.0f);
  }

  /*
	for(uint n = 0; n < m_BulletEntitiesAfterUpdate.GetCardinality(); n++)
	{
		Object* ent = m_BulletEntitiesAfterUpdate[n];
		ent->ThrowEvent(ent, EV_POSTSIMULATE, (void*)(*(int*)&elapsedtime));
	}
  */

	//ENF_BEGIN_PROFILE(PROFILE_PHYSICS_MOVE);
	/*
  for(uint n = 0; n < m_BulletEntitiesToMove.GetCardinality(); n++)
	{
		Object* o = m_BulletEntitiesToMove[n];
		if(o && o->GetPhysics())
			o->Move(o->GetPhysics()->GetWorldTransform());
			//o->FutureVisualStateScope()->SetTransform(o->GetPhysics()->GetWorldTransform());//ThrowEvent(m_pWorld->GetWorldEntity(), EV_PHYSICSMOVE, NULL);
	}
	*/
  //ENF_END_PROFILE();

  /*
	if(doProfileBullet)
	{
		static const double profileLimits[] = {0, 1, 5, 10, 20, 50};

		double ms = Timer::PopTime(Timer::RESOLUTION_MILLISECONDS);

		if(ms > profileLimits[doProfileBullet])
		{
			freopen( "bullet.txt", "w", stdout );
#ifndef BT_NO_PROFILE
			CProfileManager::dumpAll();
#endif
			freopen( "CON", "w", stdout );
		}
	}
  */

  /*
	if(ENF_DIAG_BOOL(DM_DUMP_ACTIVE_BODIES))
	{
		ENF_DIAG_SET(DM_DUMP_ACTIVE_BODIES, false);

		LogF("Active entities");
		DumpActiveBodies();
	}
  */

	m_pDynamicsWorld->debugDrawWorld();
	//EngineStats::m_iActiveBodies += m_pDynamicsWorld->getNumActiveObjects();
	//EngineStats::m_iPassiveBodies += m_pDynamicsWorld->getNumCollisionObjects() - m_pDynamicsWorld->getNumActiveObjects();

	//ENF_END_PROFILE();
}

//-----------------------------------------------------------------------
void BulletWorld::SetGravity(const Vector3& direction)
{
	Assert(!BadVec(direction));
	m_pDynamicsWorld->setGravity(btVector3(direction.Get(0), direction.Get(1), direction.Get(2)));
}

//-----------------------------------------------------------------------
Vector3 BulletWorld::GetGravity() const
{
	return btVector2Vector(m_pDynamicsWorld->getGravity());
}

//-----------------------------------------------------------------------
///not used
void BulletWorld::WarmUp(float simTime)
{
  RlsAssert(!BadFloat(simTime));
	m_bWarming = true;

	//empty world
	if(m_pDynamicsWorld->getCollisionObjectArray().size() != 0)
	{
    ScopeLock<PhysicsWorld> lock(*this);

		uint debugMode = 0;
		if(m_pDebugDrawer)
		{
			debugMode = m_pDebugDrawer->getDebugMode();
			m_pDebugDrawer->setDebugMode(0);
		}

		m_BulletEntitiesToMove.Clear();
		/*int numSimSteps = */m_pDynamicsWorld->stepSimulation(simTime, 1000000);

		if(m_pDebugDrawer && debugMode)
		{
			m_pDebugDrawer->setDebugMode(debugMode);
		}
	}
	m_bWarming = false;
}

//-----------------------------------------------------------------------
void RenderBulletShape(btCollisionShape* shape, const Matrix43& matrix, uint mask)
{
  Shape* sh = NULL;

	int shapeType = shape->getShapeType();

	//BOX_SHAPE_PROXYTYPE,
	//TRIANGLE_SHAPE_PROXYTYPE,
	//CONVEX_TRIANGLEMESH_SHAPE_PROXYTYPE,
	//CONVEX_HULL_SHAPE_PROXYTYPE,
	//CONVEX_POINT_CLOUD_SHAPE_PROXYTYPE,

	////implicit convex shapes
	//SPHERE_SHAPE_PROXYTYPE,
	//CAPSULE_SHAPE_PROXYTYPE,
	//CONVEX_SHAPE_PROXYTYPE,
	//CYLINDER_SHAPE_PROXYTYPE,

	//TRIANGLE_MESH_SHAPE_PROXYTYPE,
	//SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE,
	//TERRAIN_SHAPE_PROXYTYPE,
	//GIMPACT_SHAPE_PROXYTYPE,



#if 1
	const int commonFlags = SS_NOZWRITE|SS_TRANSP;
	const int commonAlpha = 0x4f000000;
#else
	const int commonFlags = 0;
	const int commonAlpha = 0xff000000;
#endif

	if(shapeType == TRIANGLE_MESH_SHAPE_PROXYTYPE)
	{
		//if((mask & PhysicsWorld::RM_MESH) == 0)
			//return;
		
		sh = g_pBulletManager->GetCached(shape);

		if(sh == NULL)
		{
			const btBvhTriangleMeshShape* trimesh = (const btBvhTriangleMeshShape*)shape;
			const btStridingMeshInterface* meshInterface = trimesh->getMeshInterface();

			const unsigned char* vertexBase;
			const unsigned char* indexBase;
			int numVerts;
			PHY_ScalarType type;
			int vertsStride;
			int indexStride;
			int numFaces;
			PHY_ScalarType indicesType;

			meshInterface->getLockedReadOnlyVertexIndexBase(&vertexBase, numVerts, type, vertsStride, &indexBase, indexStride, numFaces, indicesType, 0);
			//sh = Shape::CreateIndexedMesh((const Vector3*)vertexBase, numVerts, vertsStride, (const ushort*)indexBase, numFaces, indexStride/3, 32.0f, commonAlpha|0x001f3f7f, commonFlags|SS_NOINSERT|SS_BACKFACE);
      sh = ENF_SHAPE_CREATEINDEXMESH((const Vector3*)vertexBase, numVerts, vertsStride, (const ushort*)indexBase, numFaces, indexStride/3, 32.0f, commonAlpha|0x001f3f7f, commonFlags|SS_NOINSERT|SS_BACKFACE, 0);
			g_pBulletManager->InsertCached(shape, sh);
		}

		//sh = Shape::Create(sh, SS_ONCE);
    sh = ENF_SHAPE_CREATE(sh, SS_ONCE);
	}
	else if(shapeType == CONVEX_HULL_SHAPE_PROXYTYPE)
	{
		if((mask & PhysicsWorld::RM_CONVEX) == 0)
			return;

		//sh = g_pBulletManager->GetCached(shape);

		if(sh == NULL)
		{
			const btConvexHullShape* geom = (const btConvexHullShape*)shape;
			btShapeHull shapehull(geom);
			shapehull.buildHull(0);

			//sh = Shape::CreateIndexedMesh((const Vector3*)shapehull.getVertexPointer(), shapehull.numVertices(), sizeof(btVector3), (const ushort*)shapehull.getIndexPointer(), shapehull.numTriangles(), sizeof(uint), MEQ_WORLDSCALE, commonAlpha|0x005f1f3f, commonFlags|SS_NOINSERT|SS_BACKFACE);
      sh = ENF_SHAPE_CREATEINDEXMESH((const Vector3*)shapehull.getVertexPointer(), shapehull.numVertices(), sizeof(btVector3), (const ushort*)shapehull.getIndexPointer(), shapehull.numTriangles(), sizeof(uint), MEQ_WORLDSCALE, commonAlpha|0x005f1f3f, commonFlags|SS_BACKFACE|SS_ONCE, 0);
			//g_pBulletManager->InsertCached(shape, sh);
		}

		//sh = Shape::Create(sh, SS_ONCE);
    //sh = ENF_SHAPE_CREATE(sh, SS_ONCE);
	}	
	else if(shapeType == BOX_SHAPE_PROXYTYPE)
	{
		if((mask & PhysicsWorld::RM_BOX) == 0)
			return;

		const btBoxShape* geom = (const btBoxShape*)shape;

		Vector3 corner = BulletWorld::btVector2Point(geom->getHalfExtentsWithMargin());

		//sh = Shape::CreateBox(-corner, corner, 0x4f5f1f3f, commonFlags|SS_ONCE);
    sh = ENF_SHAPE_CREATEBOX(-corner, corner, 0x4f5f1f3f, commonFlags|SS_ONCE, 0);
	}
	else if(shapeType == CAPSULE_SHAPE_PROXYTYPE)
	{
		if((mask & PhysicsWorld::RM_CAPSULE) == 0)
			return;

		//sh = g_pBulletManager->GetCached(shape);

		if(sh == NULL)
		{
			const btCapsuleShape* geom = (const btCapsuleShape*)shape;
			
			float radius = geom->getRadius() * MEQ_WORLDSCALE;
			float length = geom->getHalfHeight() * 2 * MEQ_WORLDSCALE;

			sh = ENF_SHAPE_CREATECAPSULE(Vector3(0,0,0), radius, length + radius * 2, commonAlpha|0x001f5f3f, commonFlags|SS_ONCE, 0);
			//g_pBulletManager->InsertCached(shape, sh);
		}

		//sh = ENF_SHAPE_CREATE(sh, SS_ONCE);
	}
	else if(shapeType == CYLINDER_SHAPE_PROXYTYPE)
	{
		if((mask & PhysicsWorld::RM_CAPSULE) == 0)
			return;

		sh = g_pBulletManager->GetCached(shape);

		if(sh == NULL)
		{
			const btCylinderShape* geom = (const btCylinderShape*)shape;
			float radius = geom->getRadius() * MEQ_WORLDSCALE;
			float length = geom->getHalfExtentsWithMargin()[geom->getUpAxis()] * MEQ_WORLDSCALE * 2;

			sh = ENF_SHAPE_CREATECYLINDER(Vector3(0,0,0), radius, length, commonAlpha|0x001f5f3f, commonFlags|SS_NOINSERT, 0);
			g_pBulletManager->InsertCached(shape, sh);
		}

		sh = ENF_SHAPE_CREATE(sh, SS_ONCE);
	}
	else if(shapeType == SPHERE_SHAPE_PROXYTYPE)
	{
		if((mask & PhysicsWorld::RM_SPHERE) == 0)
			return;

		sh = g_pBulletManager->GetCached(shape);

		if(sh == NULL)
		{
			const btSphereShape* geom = (const btSphereShape*)shape;
			
			sh = ENF_SHAPE_CREATESPHERE(Vector3(0,0,0), geom->getRadius() * MEQ_WORLDSCALE, commonAlpha|0x001f5f3f, commonFlags|SS_NOINSERT, 0);
      g_pBulletManager->InsertCached(shape, sh);
		}

		sh = ENF_SHAPE_CREATE(sh, SS_ONCE);
	}

	if(sh)
	{
		sh->SetMatrix(matrix);
	}
}

//-----------------------------------------------------------------------
void BulletWorld::RenderEntity(PhysicsController* pc, uint mask, const char* highlightGeom)
{
  BulletController* bc = BulletController::SafeCast(pc);
	if(bc == NULL)
	{
		BulletRagdollController* ragdoll = BulletRagdollController::SafeCast(pc);
		if(ragdoll)
		{
			//shift it just for testing purposes - give back as the ragdoll will work properly
			for(uint n = 0; n < ragdoll->m_Bones.GetCardinality(); n++)
			{
				btTransform tr = btTransform(ragdoll->m_Bones[n]->GetRigidBody()->getCenterOfMassTransform());
				tr.setOrigin(tr.getOrigin() + btVector3(1.f, 0, 0));
				ragdoll->m_Bones[n]->GetRigidBody()->setCenterOfMassTransform(tr);
			}
			for(uint n = 0; n < ragdoll->m_Bones.GetCardinality(); n++)
			{
				RenderEntity(ragdoll->m_Bones[n], mask & ~RM_CENTEROFMASS, highlightGeom);
			}
			for(uint n = 0; n < ragdoll->m_Bones.GetCardinality(); n++)
			{
				btTransform tr = btTransform(ragdoll->m_Bones[n]->GetRigidBody()->getCenterOfMassTransform());
				tr.setOrigin(tr.getOrigin() + btVector3(-1.f, 0, 0));
				ragdoll->m_Bones[n]->GetRigidBody()->setCenterOfMassTransform(tr);
			}
		}
		return;
	}

	btCollisionObject* obj = bc->GetCollisionObject();
  if(!obj)
    return;

	btCollisionShape* shape = obj->getCollisionShape();

	if((mask & RM_CENTEROFMASS) != 0 && pc->HasType(BulletDynamicControllerTypeID))
	{
		btVector3 mins, maxs;
		shape->getAabb(btTransform::getIdentity(), mins, maxs);
		float size = enf_min(16.0f, (maxs - mins).length() * MEQ_WORLDSCALE * 0.05f);
		Shape* sh = Shape::CreateDiamond(Vector3(-size, -size, -size), Vector3(size, size, size), 0xffffffff, SS_NOZWRITE|SS_TRANSP|SS_ONCE);
		sh->SetMatrix(btTransform2Matrix(obj->getWorldTransform()));
	}
  
	Matrix43 objMat = btTransform2Matrix(obj->getWorldTransform(), bc->GetCenterOfMass());

	btTransform worldTransform = obj->getWorldTransform();
	if(shape->getShapeType() == COMPOUND_SHAPE_PROXYTYPE)
	{
		btCompoundShape* cshape = (btCompoundShape*)shape;
		for(int n = 0; n < cshape->getNumChildShapes(); n++)
		{
			btTransform childWorldTrans = worldTransform * cshape->getChildTransform(n);

			RenderBulletShape(cshape->getChildShape(n), btTransform2Matrix(childWorldTrans), mask);
		}
	}
	else
	{
		RenderBulletShape(shape, objMat, mask);
	}

	if((mask & RM_WHEEL) == 0)
		return;

#pragma message("jz todo: give back vehicle visualisation")
/*
	BulletVehicleController* vc = BulletVehicleController::SafeCast(bc);

	if(vc)
	{
		btRaycastVehicle* vehicle = vc->GetVehicle();
		for(int w = 0; w < vehicle->getNumWheels(); w++)
		{
			btWheelInfo wi = vehicle->getWheelInfo(w);
			vehicle->updateWheelTransformsWS(wi);
			Matrix43 matrix = btTransform2Matrix(wi.m_worldTransform);

			Shape::CreateLine(matrix.GetTranslation(), matrix.GetTranslation() + BulletWorld::btVector2Vector(vehicle->getForwardVector()) * 64, 0xffffff00, SS_ONCE);

			Matrix33 rmat;
			rmat.Roll(90);
			matrix.SetOrientation(matrix.GetOrientation()*rmat);

			Shape* sh = Shape::CreateCylinder(Vector3(0,0,0), wi.m_wheelsRadius * MEQ_WORLDSCALE, 8, 0x3f1f5f3f, SS_NOZWRITE|SS_TRANSP|SS_ONCE);
			sh->SetMatrix(matrix);

			continue;
		}
	}
*/
}

//-----------------------------------------------------------------------
bool BulletWorld::InitTerrain(Scenery* terr)//Terrain* terr)
{
  Assert(terr);

  BulletPhysicsMaterial* mat = m_pManager->m_BaseMaterials[0];
	if(mat == NULL)
		return false;

	if(m_pDynamicsWorld == NULL)
		return false;

	Matrix43 terrMatrix = MIdentity;

	btScalar minHeight = terr->_minHeight * MEQ_WORLDISCALE;
  btScalar maxHeight = terr->_maxHeight * MEQ_WORLDISCALE;
	
  HeightfieldTerrainShape* tg = new HeightfieldTerrainShape(terr, this, 
		MEQ_WORLDISCALE, minHeight, maxHeight);

	tg->setLocalScaling(btVector3(terr->GetUnitScale() * MEQ_WORLDISCALE, 1.0f, terr->GetUnitScale() * MEQ_WORLDISCALE));

	tg->setMargin(BT_MARGIN);
  
  btCollisionObject* rb = new btCollisionObject();

  BulletController* controller = new BulletController(rb);
  rb->setUserPointer((void*)terr);
	terr->SetPhysicsController(controller);

  terrMatrix.SetTranslation(Vector3(0.f, 0.f, 0.f));
	btTransform transform = Matrix2btTransform(terrMatrix);
	rb->setWorldTransform(transform);

	tg->setMaterial(mat->m_pBtMaterial);
	rb->setCollisionShape(tg);
	rb->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
	rb->forceActivationState(ISLAND_SLEEPING);
  rb->setInteractionLayer(EPL_TERRAIN);
	m_pDynamicsWorld->addCollisionObject(rb, short(btBroadphaseProxy::StaticFilter), short(btBroadphaseProxy::AllFilter ^ btBroadphaseProxy::StaticFilter));
  controller->UpdateEntityFlags(terr, 0, terr->GetFlags()); 
	controller->UpdateEntityEvents(terr, 0, terr->GetEventMask());

  return true;
}

//-----------------------------------------------------------------------
bool BulletWorld::InitWorld(Landscape* world, bool loadsave, const AAB& bounds)
{
  Assert(world != NULL);

	m_pWorld = world;
	return true;
}

//-----------------------------------------------------------------------
bool BulletWorld::InitWorldCollisions(bool loadsave, const AAB& bounds)
{
	BulletPhysicsMaterial* mat = m_pManager->m_BaseMaterials[0];
	if(mat == NULL)
		return false;

	m_WorldBounds = bounds;
	return true;
}

//-----------------------------------------------------------------------
void BulletWorld::BulletSimulateEvent(float timeslice)
{
  /*
  Object* worldEnt = m_pWorld->GetWorldEntity();
	if(worldEnt)
		worldEnt->ThrowEvent(worldEnt, EV_SIMULATE, (void*)(*(int*)&timeslice));
  */

	for(uint n = 0; n < m_BulletEntitiesToUpdate.GetCardinality(); n++)
	{
		Object* ent = m_BulletEntitiesToUpdate[n];
    //ent->ThrowEvent(ent, EV_SIMULATE, (void*)(*(int*)&timeslice));
      ent->SimulatePhysics(ent, timeslice);
	}

	UpdateTime(timeslice);
	if(!m_bWarming)
	{
		btDispatcher* dispatcher = m_pDynamicsWorld->getDispatcher();

		uint numManifolds = dispatcher->getNumManifolds();
		for (uint i = 0; i < numManifolds; i++)
		{
			btPersistentManifold* contactManifold = dispatcher->getManifoldByIndexInternal(i);
			if(contactManifold->getNumContacts() == 0)
				continue;

			const btCollisionObject* obA = static_cast<const btCollisionObject*>(contactManifold->getBody0());
			const btCollisionObject* obB = static_cast<const btCollisionObject*>(contactManifold->getBody1());
		
			if(!obA->isActive() && !obB->isActive())
				continue;

			Object* ent0 = (Object*)obA->getUserPointer();
			Object* ent1 = (Object*)obB->getUserPointer();
			if(ent0 == NULL || ent1 == NULL)
				continue;

			uint eventMask0 = ent0->GetEventMask() & EV_CONTACT;
			uint eventMask1 = ent1->GetEventMask() & EV_CONTACT;

			if(eventMask0 == 0 && eventMask1 == 0)
				continue;

			const btRigidBody* rbA = btRigidBody::upcast(obA);
			const btRigidBody* rbB = btRigidBody::upcast(obB);
		
			if(rbA)
			{
				contactManifold->m_linVel1 = rbA->getLinearVelocity();
				contactManifold->m_angVel1 = rbA->getAngularVelocity();
			}
			if(rbB)
			{
				contactManifold->m_linVel2 = rbB->getLinearVelocity();
				contactManifold->m_angVel2 = rbB->getAngularVelocity();
			}
		}
	}
}

//-----------------------------------------------------------------------
void BulletWorld::RemoveDynamicBodyFromSimulation(Object* obj)
{
  btCollisionObject* collisionObject = ((enf::BulletController*)obj->GetPhysics())->GetCollisionObject();
  if(!collisionObject)
    return;

  DestroyBody(obj);
}

//-----------------------------------------------------------------------
void BulletWorld::BulletAfterSimulateEvent(float timeslice)
{
  if(!m_bWarming)
	{
    ///remove sleeping dynamic items
    btCollisionObjectArray& objects = m_pDynamicsWorld->getNonStaticCollisionObjects();
    for (int i = 0; i < objects.size(); ++i)
		{
      btCollisionObject* collisionObject = objects[i];
      if(!collisionObject->isActive())
      {
        Object* obj = (Object*)collisionObject->getUserPointer();
        

        if(collisionObject->getLifeTime() < 0.f)
        {
          btTransform btrans = collisionObject->getWorldTransform();
          btVector3 start = btrans.getOrigin();
          btVector3 end = btVector3(start.get(0), -BT_LARGE_FLOAT, start.get(2));
          btCollisionWorld::ClosestRayResultCallback rayCallback(start, end);

          GetBulletWorld()->rayTest(start, end, rayCallback);
          
          RemoveDynamicBodyFromSimulation(obj);

          if(rayCallback.hasHit()) 
          {
            //zarovna stred na zem, nepolozi to na nej
            obj->FutureVisualState().SetPosition(enf::BulletWorld::btVector2Point(rayCallback.m_hitPointWorld));
          }
        }
        else
        {
          RemoveDynamicBodyFromSimulation(obj);
        }
      }
    }
    
		btDispatcher* dispatcher = m_pDynamicsWorld->getDispatcher();

		uint numManifolds = dispatcher->getNumManifolds();
		for (uint i = 0; i < numManifolds; i++)
		{
			const btPersistentManifold* contactManifold = dispatcher->getManifoldByIndexInternal(i);
			if(contactManifold->getNumContacts() == 0)
				continue;

			const btCollisionObject* obA = static_cast<const btCollisionObject*>(contactManifold->getBody0());
			const btCollisionObject* obB = static_cast<const btCollisionObject*>(contactManifold->getBody1());
		
			if(!obA->isActive() && !obB->isActive())
				continue;

			Object* ent0 = (Object*)obA->getUserPointer();
			Object* ent1 = (Object*)obB->getUserPointer();
			if(ent0 == NULL || ent1 == NULL)
				continue;

			uint eventMask0 = ent0->GetEventMask() & EV_CONTACT;
			uint eventMask1 = ent1->GetEventMask() & EV_CONTACT;

			if(eventMask0 == 0 && eventMask1 == 0)
				continue;

			const btRigidBody* rbA = btRigidBody::upcast(obA);
			const btRigidBody* rbB = btRigidBody::upcast(obB);
		
			uint processedContacts[8];
			uint numProcessed = 0;

			uint numContacts = contactManifold->getNumContacts();
			for (uint j = 0; j < numContacts; j++)
			{
				const btManifoldPoint& pt = contactManifold->getContactPoint(j);
				if (pt.getDistance() > contactManifold->getContactProcessingThreshold())
					continue;

				uint n;
				for(n = 0; n < numProcessed; n++)
				{
					const btManifoldPoint& pt2 = contactManifold->getContactPoint(processedContacts[n]);
					if(pt.m_normalWorldOnB.dot(pt2.m_normalWorldOnB) > 0.985f && pt.getAppliedImpulse() > pt2.getAppliedImpulse())
					{
						numProcessed--;
						break;
					}
				}
				processedContacts[n] = j;
				numProcessed++;
			}

      //Contact& cont = g_pScript->m_Contact;
      CollisionInfo cont;
			for (uint j = 0; j < numProcessed; j++)
			{
				const btManifoldPoint& pt = contactManifold->getContactPoint(processedContacts[j]);

				btVector3 relVel;
				if(rbA && rbB)
					relVel = (contactManifold->m_linVel2 + contactManifold->m_angVel2.cross(pt.m_localPointB)) - (contactManifold->m_linVel1 + contactManifold->m_angVel1.cross(pt.m_localPointA));
				else if(rbA == NULL)
					relVel = (contactManifold->m_linVel2 + contactManifold->m_angVel2.cross(pt.m_localPointB));
				else
					relVel = -(contactManifold->m_linVel1 + contactManifold->m_angVel1.cross(pt.m_localPointA));

				//ignore tiny contacts
				if(relVel.length2() < 0.0001f)
					continue;

        //cont.m_vRelativeVelocityBefore = btVector2Vector(relVel);
				//cont.m_fRelativeNormalVelocityBefore = relVel.dot(pt.m_normalWorldOnB);
        const btVector3& ptA = pt.getPositionWorldOnA();
				//const btVector3& ptB = pt.getPositionWorldOnB();

        //cont.m_pMaterial1 = (PhysicsMaterial*)pt.m_material0->getUserPointer();
				//cont.m_iMaterial1 = cont.m_pMaterial1 != NULL ? SurfaceParms(cont.m_pMaterial1->m_iMaterialIndex).GetMaterialIndex() : 0;
				//cont.m_pMaterial2 = (PhysicsMaterial*)pt.m_material1->getUserPointer();
				//cont.m_iMaterial2 = cont.m_pMaterial2 != NULL ? SurfaceParms(cont.m_pMaterial2->m_iMaterialIndex).GetMaterialIndex() : 0;
        
				//cont.m_fImpulse = pt.m_appliedImpulse;
				//cont.m_vPosition = btVector2Point(ptA);
        cont.pos = btVector2Point(ptA);
				//cont.m_vNormal = btVector2Vector(pt.m_normalWorldOnB);
        if(rbA && rbB)
					relVel = rbB->getVelocityInLocalPoint(pt.m_localPointB) - rbA->getVelocityInLocalPoint(pt.m_localPointA);
				else if(rbA == NULL)
					relVel = rbB->getVelocityInLocalPoint(pt.m_localPointB);
				else
					relVel = -rbA->getVelocityInLocalPoint(pt.m_localPointA);

				//cont.m_vRelativeVelocityAfter = btVector2Vector(relVel);
        cont.dirOutNotNorm = btVector2Vector(relVel);
				//cont.m_fRelativeNormalVelocityAfter = relVel.dot(pt.m_normalWorldOnB);
        cont.dirOut = cont.dirOutNotNorm.Normalized();//relVel.dot(pt.m_normalWorldOnB);
				
				if(eventMask0)
        {
					//ent0->ThrowEvent(ent1, EV_CONTACT, &cont);
          ent0->EOnContact(ent1, &cont);
        }
				
				if(eventMask1)
				{
					//cont.m_vNormal = -cont.m_vNormal;
					//enf_swap(cont.m_pMaterial1, cont.m_pMaterial2);
					//ent1->ThrowEvent(ent0, EV_CONTACT, &cont);
          ent1->EOnContact(ent0, &cont);
				}
			}
    }
	}
}

//-----------------------------------------------------------------------
void InternalPreTickCallback(btDynamicsWorld *world, btScalar timeStep)
{
	BulletWorld* mworld = (BulletWorld*)world->getWorldUserInfo();
	mworld->BulletSimulateEvent(timeStep);
}

void InternalTickCallback(btDynamicsWorld *world, btScalar timeStep)
{
	BulletWorld* mworld = (BulletWorld*)world->getWorldUserInfo();
	mworld->BulletAfterSimulateEvent(timeStep);
}

//-----------------------------------------------------------------------
btInteractionBlock* BulletWorld::CreateInteractionBlock(Object* ent1, Object* ent2)
{
  Assert(ent1 && ent2);
	BulletController* bc1 = BulletController::SafeCast(ent1->GetPhysics());
	if(bc1 == NULL)
		return NULL;
	BulletController* bc2 = BulletController::SafeCast(ent2->GetPhysics());
	if(bc2 == NULL)
		return NULL;

	btInteractionBlock* ib = new btInteractionBlock(bc1->GetCollisionObject(), bc2->GetCollisionObject());
	m_pDynamicsWorld->getPairCache()->insertInteractionBlock(ib);
	return ib;
}

//-----------------------------------------------------------------------
btInteractionBlock* BulletWorld::CreateInteractionBlock(Object* ent1, uint elem, Object* ent2)
{
  Assert(ent1 && ent2);
	BulletController* bc1 = BulletController::SafeCast(ent1->GetPhysics());
	if(bc1 == NULL)
		return NULL;
	BulletController* bc2 = BulletController::SafeCast(ent2->GetPhysics());
	if(bc2 == NULL)
		return NULL;

	btCollisionShape* sh1 = bc1->GetCollisionObject()->getCollisionShape();
	if(sh1->getShapeType() == COMPOUND_SHAPE_PROXYTYPE && (int)elem < ((btCompoundShape*)sh1)->getNumChildShapes())
		sh1 = ((btCompoundShape*)sh1)->getChildShape(elem);

	btInteractionBlock* ib = new btInteractionBlock(bc1->GetCollisionObject(), sh1, bc2->GetCollisionObject(), bc2->GetCollisionObject()->getCollisionShape());
	m_pDynamicsWorld->getPairCache()->insertInteractionBlock(ib);
	return ib;
}

//-----------------------------------------------------------------------
void BulletWorld::DestroyInteractionBlock(btInteractionBlock* ib)
{
	Assert(ib);
	m_pDynamicsWorld->getPairCache()->removeInteractionBlock(ib);
	delete ib;

}

//-----------------------------------------------------------------------
BulletWorld::BulletWorld(): PhysicsWorld(),
	m_pWorld(NULL),
	m_pCollisionDispatcher(NULL),
	m_pBroadPhase(NULL),
	m_pSolver(NULL),
	m_pDynamicsWorld(NULL),
	m_pDebugDrawer(NULL),
	m_bSceneSimulated(false),
	m_bWarming(false),
	m_fTimeSlice(0)
{
	m_pManager = g_pBulletManager;

	//Bullet initialize

	m_pCollisionDispatcher = new btCollisionDispatcher(m_pManager->GetCollisionConfiguration());
	m_pBroadPhase = new btDbvtBroadphase();
	m_pSolver = new btSequentialImpulseConstraintSolver();

	btDiscreteDynamicsWorld* dworld = new btDiscreteDynamicsWorld(m_pCollisionDispatcher,m_pBroadPhase,m_pSolver,m_pManager->GetCollisionConfiguration());

	m_pDynamicsWorld = dworld;
	dworld->setGravity(btVector3(0.f, -9.8f, 0.f));
  m_pDebugDrawer = new BulletDebugDraw();
	dworld->setDebugDrawer(m_pDebugDrawer);

	dworld->setInternalTickCallback(&InternalPreTickCallback, this, true);
	dworld->setInternalTickCallback(&InternalTickCallback, this, false);
		
	//s timhle si asi delate prdel, ne?
	dworld->setForceUpdateAllAabbs(false);
	dworld->setSynchronizeAllMotionStates(false);
}

//-----------------------------------------------------------------------
void BulletWorld::Destroy()
{
	Assert(!IsLocked());
	SAFE_DELETE(m_pDynamicsWorld);
	SAFE_DELETE(m_pSolver);
	SAFE_DELETE(m_pBroadPhase);
	SAFE_DELETE(m_pCollisionDispatcher);
	SAFE_DELETE(m_pDebugDrawer);
#ifndef BT_NO_PROFILE
	CProfileManager::CleanupMemory();
#endif
	delete this;
}

//-----------------------------------------------------------------------
void BulletWorld::SetInteractionLayer(uint mask1, uint mask2, bool enable)
{
	Assert(m_pDynamicsWorld != NULL);
	btOverlappingPairCache* pcache = m_pDynamicsWorld->getPairCache();

	if(enable)
		pcache->enableInteractionLayer(mask1, mask2);
	else
		pcache->disableInteractionLayer(mask1, mask2);
}

//-----------------------------------------------------------------------
void BulletWorld::Synchronize(btCollisionObject* body)
{
	btDynamicsWorld* dworld = m_pDynamicsWorld;
	btBroadphaseInterface* bp = (btBroadphaseInterface*)dworld->getBroadphase();

	bp->getOverlappingPairCache()->cleanProxyFromPairs(body->getBroadphaseHandle(), dworld->getDispatcher());
	bp->destroyProxy(body->getBroadphaseHandle(), dworld->getDispatcher());
	body->setBroadphaseHandle(0);

	//calculate new AABB
	const btTransform& trans = body->getWorldTransform();

	btVector3 minAabb;
	btVector3 maxAabb;
	body->getCollisionShape()->getAabb(trans, minAabb, maxAabb);

	bool isDynamic = !(body->isStaticObject() || body->isKinematicObject());
	short collisionFilterGroup = isDynamic ? short(btBroadphaseProxy::DefaultFilter) : short(btBroadphaseProxy::StaticFilter);
	short collisionFilterMask = isDynamic ? short(btBroadphaseProxy::AllFilter) : short(btBroadphaseProxy::AllFilter ^ btBroadphaseProxy::StaticFilter);

	int type = body->getCollisionShape()->getShapeType();
	body->setBroadphaseHandle( bp->createProxy(
		minAabb,
		maxAabb,
		type,
		body,
		collisionFilterGroup,
		collisionFilterMask,
		body->getInteractionLayer(),
		dworld->getDispatcher(),0
		));
}
/*
//-----------------------------------------------------------------------
BulletController* BulletWorld::CreateStaticBody(Object *ent, uint interactionLayer)
{
	Assert(ent != NULL);
	Assert(ent->GetPhysics() == NULL);
	Assert(m_pDynamicsWorld != NULL);

	//early exit
	BulletObjectGeometry* objgeom = GetObjectGeometry(ent);
	if(objgeom == NULL)
		return NULL;

	uint numGeoms = objgeom->GetNumElements();
	PhysicsGeomDef* defs = ALLOCAINIT(PhysicsGeomDef, numGeoms);
	objgeom->SetupPhysicsGeomDefs(defs, numGeoms);
	BulletController* controller = CreateStaticBody(ent, defs, numGeoms, interactionLayer);
	if(controller)
	{
		controller->m_pGeometries = objgeom;
	}
	return controller;
}
*/
//-----------------------------------------------------------------------
BulletController* BulletWorld::CreateStaticBody(Object *ent, PhysicsGeomDef* defs, uint numGeoms, uint interactionLayer)
{
  Assert(ent != NULL);
	Assert(ent->GetPhysics() == NULL);
	Assert(m_pDynamicsWorld != NULL);

	btCollisionObject* rb = new btCollisionObject();
	rb->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT|btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);
	rb->setActivationState(ISLAND_SLEEPING);

#pragma message("jz todo: give lock back")
  CMutexLock lock(ent->GetWorld()->GetMutex());
	BulletController* controller = new BulletController(rb);

	rb->setUserPointer((void*)ent);
	ent->SetPhysicsController(controller);

	controller->SetMatrix(ent->IGetMatrix());
	controller->AddGeoms(defs, numGeoms, interactionLayer);

	m_pDynamicsWorld->addCollisionObject(rb, short(btBroadphaseProxy::StaticFilter), short(btBroadphaseProxy::AllFilter ^ btBroadphaseProxy::StaticFilter));

	controller->UpdateEntityFlags(ent, 0, ent->GetFlags());
	controller->UpdateEntityEvents(ent, 0, ent->GetEventMask());

	return controller;
}
/*
//-----------------------------------------------------------------------
void BulletWorld::RemoveBody(Object *ent)//BulletController* controler)
{
	if(ent)
	{
		BulletController* controler = (BulletController*)ent->GetPhysics();
		if(controler && controler->GetCollisionObject())
			m_pDynamicsWorld->removeCollisionObject(controler->GetCollisionObject());
	}
}

//-----------------------------------------------------------------------
void BulletWorld::AddStaticBody(Object *ent)//BulletController* controler)
{
	if(ent)
	{
		BulletController* controler = (BulletController*)ent->GetPhysics();
		if(controler && controler->GetCollisionObject())
			m_pDynamicsWorld->addCollisionObject(controler->GetCollisionObject(), short(btBroadphaseProxy::StaticFilter), short(btBroadphaseProxy::AllFilter ^ btBroadphaseProxy::StaticFilter));
	}
}
*/
//-----------------------------------------------------------------------
BulletDynamicController* BulletWorld::CreateDynamicBody(Object *ent, const Vector3& trans, PhysicsGeomDef* defs, uint numGeoms, float mass, uint interactionLayer)
{
  Assert(ent != NULL);
	Assert(ent->GetPhysics() == NULL);
	Assert(m_pDynamicsWorld != NULL);
	Assert(!BadFloat(mass));
	if(ent == NULL || ent->GetPhysics() != NULL || m_pDynamicsWorld == NULL)
		return 0;

	btRigidBody::btRigidBodyConstructionInfo constructionInfo(0.0f, NULL, NULL);
	constructionInfo.m_linearDamping = 0.1f;
	constructionInfo.m_angularDamping = 0.5f;
	btRigidBody* rb = new btRigidBody(constructionInfo);
	
	if(mass == 0.0f)
		rb->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
	else
		rb->setCollisionFlags(0);

	CMutexLock lock(ent->GetWorld()->GetMutex());
	BulletDynamicController* controller = NULL;

	if(!trans.IsZero())
		controller = new BulletDynamicControllerNC(rb, trans);
	else
		controller = new BulletDynamicController(rb);
	//controller->SetActive(enf::AS_INACTIVE);

	rb->setUserPointer((void*)ent);
	ent->SetPhysicsController(controller);
	//this initializes body transform too
	rb->setMotionState(controller);

	controller->AddGeoms(defs, numGeoms, interactionLayer);

	if(rb->getCollisionShape())
	{
		if(mass != 0.0f)
		{
			btVector3 inertia;
			rb->getCollisionShape()->calculateLocalInertia(mass, inertia);
			rb->setMassProps(mass, inertia);
		}
	}
	rb->updateInertiaTensor();

	m_pDynamicsWorld->addRigidBody(rb);
	controller->UpdateEntityFlags(ent, 0, ent->GetFlags());
	controller->UpdateEntityEvents(ent, 0, ent->GetEventMask());

	return controller;
}

//-----------------------------------------------------------------------
void PhysicsHingeJoint::SetLimit(float low, float high, float _softness, float _biasFactor, float _relaxationFactor)
{
	Assert(!BadFloat(low) && !BadFloat(high) && !BadFloat(_softness) && !BadFloat(_biasFactor) && !BadFloat(_relaxationFactor));
	constraint()->setLimit(low, high, _softness, _biasFactor, _relaxationFactor);
}

//-----------------------------------------------------------------------
void PhysicsHingeJoint::SetMotor(bool enable, float targetVelocity, float maxImpulse)
{
	constraint()->enableAngularMotor(enable, targetVelocity, maxImpulse);
}

//-----------------------------------------------------------------------
void PhysicsHingeJoint::SetMotorTargetAngle(float angle, float dt, float maxImpulse)
{
	constraint()->enableMotor(true);
	constraint()->setMaxMotorImpulse(maxImpulse);
	constraint()->setMotorTarget(angle, dt);
}

//-----------------------------------------------------------------------
void PhysicsHingeJoint::SetAxis(const Vector3& axisInA)
{
	Assert(!BadVec(axisInA));
	btVector3 axis = BulletWorld::Vector2btVector(axisInA);
	constraint()->setAxis(axis);
}

//-----------------------------------------------------------------------
PhysicsHingeJoint BulletWorld::CreateHingeJoint(PhysicsController* pc1, PhysicsController* pc2, const Vector3& point1, const Vector3& axis1, const Vector3& point2, const Vector3& axis2, bool block)
{
	BulletDynamicController* bc1 = BulletDynamicController::SafeCast(pc1);
	BulletDynamicController* bc2 = BulletDynamicController::SafeCast(pc2);
	Assert(bc1);
	Assert(!BadVec(point1));
	Assert(!BadVec(axis1));
	Assert(!BadVec(point2));
	Assert(!BadVec(axis2));

	btRigidBody* body1 = bc1->GetRigidBody();

	btVector3 pivotInA = BulletWorld::Point2btVector(point1);
	btVector3 axisInA = BulletWorld::Vector2btVector(axis1);

	btHingeConstraint* hinge;
	if(bc2 != NULL)
	{
		btRigidBody* body2 = bc2->GetRigidBody();
		btVector3 pivotInB = BulletWorld::Point2btVector(point2);
		btVector3 axisInB = BulletWorld::Vector2btVector(axis2);
		hinge = new btHingeConstraint(*body1, *body2, pivotInA, pivotInB, axisInA, axisInB, true);
	}
	else
	{
		hinge = new btHingeConstraint(*body1, pivotInA, axisInA);
	}
	
	m_pDynamicsWorld->addConstraint(hinge, block);
	hinge->setDbgDrawSize(btScalar(0.1f));
	return PhysicsHingeJoint(hinge);
}

//-----------------------------------------------------------------------
PhysicsHingeJoint BulletWorld::CreateHingeJoint(PhysicsController* pc1, PhysicsController* pc2, const Matrix43& frame1, const Matrix43& frame2, bool block)
{
	BulletDynamicController* bc1 = BulletDynamicController::SafeCast(pc1);
	BulletDynamicController* bc2 = BulletDynamicController::SafeCast(pc2);
	Assert(bc1);
	Assert(!BadMatrix43(frame1));
	Assert(!BadMatrix43(frame2));

	btRigidBody* body1 = bc1->GetRigidBody();

	btTransform tr1 = Matrix2btTransform(frame1);

	btHingeConstraint* hinge;
	if(bc2 != NULL)
	{
		btRigidBody* body2 = bc2->GetRigidBody();
		btTransform tr2 = Matrix2btTransform(frame2);
		hinge = new btHingeConstraint(*body1, *body2, tr1, tr2, true);
	}
	else 
	{
		hinge = new btHingeConstraint(*body1, tr1, true);
	}
	//hinge->setLimit(-ENF_PI, ENF_PI);

	//float	targetVelocity = 1.f;
	//float	maxMotorImpulse = 1.0f;
	//hinge->enableAngularMotor(true,targetVelocity,maxMotorImpulse);

	m_pDynamicsWorld->addConstraint(hinge, block);
	hinge->setDbgDrawSize(btScalar(0.1f));
	return PhysicsHingeJoint(hinge);
}

//-----------------------------------------------------------------------
PhysicsBallSocketJoint BulletWorld::CreateBallSocketJoint(PhysicsController* pc1, PhysicsController* pc2, const Vector3& point1, const Vector3& point2, bool block)
{
	BulletDynamicController* bc1 = BulletDynamicController::SafeCast(pc1);
	BulletDynamicController* bc2 = BulletDynamicController::SafeCast(pc2);
	Assert(bc1);
	Assert(!BadVec(point1));
	Assert(!BadVec(point2));

	btRigidBody* body1 = bc1->GetRigidBody();

	btVector3 pivotInA = BulletWorld::Point2btVector(point1);

	btPoint2PointConstraint* ballSocket;
	if(bc2 != NULL)
	{
		btRigidBody* body2 = bc2->GetRigidBody();
		btVector3 pivotInB = BulletWorld::Point2btVector(point2);
		ballSocket = new btPoint2PointConstraint(*body1, *body2, pivotInA, pivotInB);
	}
	else
	{
		ballSocket = new btPoint2PointConstraint(*body1, pivotInA);
	}

	m_pDynamicsWorld->addConstraint(ballSocket, block);
	ballSocket->setDbgDrawSize(btScalar(0.1f));
	return PhysicsBallSocketJoint(ballSocket);
}

//-----------------------------------------------------------------------
BulletPhysicsJoint BulletWorld::CreateFixedJoint(PhysicsController* pc1, PhysicsController* pc2, const Vector3& point1, const Vector3& point2, bool block)
{
	BulletDynamicController* bc1 = BulletDynamicController::SafeCast(pc1);
	BulletDynamicController* bc2 = BulletDynamicController::SafeCast(pc2);
	Assert(bc1);
	Assert(!BadVec(point1));
	Assert(!BadVec(point2));

	btRigidBody* body1 = bc1->GetRigidBody();

	btVector3 pivotInA = BulletWorld::Point2btVector(point1);

	btGeneric6DofConstraint* fixedpoint;
	if(bc2 != NULL)
	{
		btRigidBody* body2 = bc2->GetRigidBody();
		btVector3 pivotInB = BulletWorld::Point2btVector(point2);
		fixedpoint = new btGeneric6DofConstraint(*body1, *body2, btTransform(pivotInA), btTransform(body1->getCenterOfMassTransform().getBasis().timesTranspose(body2->getCenterOfMassTransform().getBasis()), pivotInB), true);
	}
	else
	{
		fixedpoint = new btGeneric6DofConstraint(*body1, btTransform(pivotInA), true);
	}
	fixedpoint->setLinearUpperLimit(btZeroVec());
	fixedpoint->setLinearLowerLimit(btZeroVec());
	fixedpoint->setAngularLowerLimit(btZeroVec());
	fixedpoint->setAngularUpperLimit(btZeroVec());

	m_pDynamicsWorld->addConstraint(fixedpoint, block);
	fixedpoint->setDbgDrawSize(btScalar(0.1f));
	return BulletPhysicsJoint(fixedpoint);
}

//-----------------------------------------------------------------------
BulletPhysicsJoint BulletWorld::CreateFixedJoint(PhysicsController* pc1, PhysicsController* pc2, const Matrix43& frame1, const Matrix43& frame2, bool block)
{
	BulletDynamicController* bc1 = BulletDynamicController::SafeCast(pc1);
	BulletDynamicController* bc2 = BulletDynamicController::SafeCast(pc2);
	Assert(bc1);
	Assert(!BadMatrix43(frame1));
	Assert(!BadMatrix43(frame2));

	btRigidBody* body1 = bc1->GetRigidBody();

	btTransform tr1 = Matrix2btTransform(frame1);

	btGeneric6DofConstraint* fixedpoint;
	if(bc2 != NULL)
	{
		btRigidBody* body2 = bc2->GetRigidBody();
		btTransform tr2 = Matrix2btTransform(frame2);
		fixedpoint = new btGeneric6DofConstraint(*body1, *body2, tr1, tr2, false);
	}
	else
	{
		fixedpoint = new btGeneric6DofConstraint(*body1, tr1, true);
	}
	fixedpoint->setLinearUpperLimit(btZeroVec());
	fixedpoint->setLinearLowerLimit(btZeroVec());
	fixedpoint->setAngularLowerLimit(btZeroVec());
	fixedpoint->setAngularUpperLimit(btZeroVec());

	m_pDynamicsWorld->addConstraint(fixedpoint, block);
	fixedpoint->setDbgDrawSize(btScalar(0.1f));
	return BulletPhysicsJoint(fixedpoint);
}

//-----------------------------------------------------------------------
void BulletWorld::DestroyJoint(PhysicsJoint& joint)
{
	Assert(!joint.IsNull());

	btTypedConstraint* btJoint = ((BulletPhysicsJoint&)joint).constraint();
	m_pDynamicsWorld->removeConstraint(btJoint);
	delete btJoint;
	joint = PhysicsJoint(NULL);
}

//-----------------------------------------------------------------------
PhysicsConeTwistJoint BulletWorld::CreateConeTwistJoint(PhysicsController* pc1, PhysicsController* pc2, const Matrix43& frame1, const Matrix43& frame2, bool block)
{
	BulletDynamicController* bc1 = BulletDynamicController::SafeCast(pc1);
	BulletDynamicController* bc2 = BulletDynamicController::SafeCast(pc2);
	Assert(bc1);
	Assert(!BadMatrix43(frame1));
	Assert(!BadMatrix43(frame2));

	btRigidBody* body1 = bc1->GetRigidBody();

	btTransform tr1 = Matrix2btTransform(frame1);
	btConeTwistConstraint* coneTwist;
	if(bc2 != NULL)
	{
		btRigidBody* body2 = bc2->GetRigidBody();
		btTransform tr2 = Matrix2btTransform(frame2);
		coneTwist = new btConeTwistConstraint(*body1, *body2, tr1, tr2);
	}
	else
	{
		coneTwist = new btConeTwistConstraint(*body1, tr1);
	}
	
	m_pDynamicsWorld->addConstraint(coneTwist, block);
	coneTwist->setDbgDrawSize(btScalar(0.1f));
	return PhysicsConeTwistJoint(coneTwist);
}

//-----------------------------------------------------------------------
void PhysicsConeTwistJoint::SetAngularOnly(bool angularOnly)
{
	constraint()->setAngularOnly(angularOnly);
}

void PhysicsConeTwistJoint::SetLimit(int limitIndex, float limitValue)
{
	constraint()->setLimit(limitIndex, limitValue);
}

void PhysicsConeTwistJoint::SetLimit(float _swingSpan1,float _swingSpan2,float _twistSpan, float _softness, float _biasFactor, float _relaxationFactor)
{
	constraint()->setLimit(_swingSpan1, _swingSpan2, _twistSpan, _softness, _biasFactor, _relaxationFactor);
}

void Physics6DofJoint::SetLinearLowerLimit(const Vector3& linearLower)
{
	constraint()->setLinearLowerLimit(BulletWorld::Point2btVector(linearLower));
}

void Physics6DofJoint::SetLinearUpperLimit(const Vector3& linearUpper)
{
	constraint()->setLinearUpperLimit(BulletWorld::Point2btVector(linearUpper));
}
void Physics6DofJoint::SetAngularLowerLimit(const Vector3& angularLower)
{
	constraint()->setAngularLowerLimit(BulletWorld::Vector2btVector(angularLower));
}
void Physics6DofJoint::SetAngularUpperLimit(const Vector3& angularUpper)
{
	constraint()->setAngularUpperLimit(BulletWorld::Vector2btVector(angularUpper));
}
//first 3 are linear, next 3 are angular
void Physics6DofJoint::SetLimit(int axis, float lo, float hi)
{
	constraint()->setLimit(axis, lo, hi);
}
bool Physics6DofJoint::IsLimited(int limitIndex) const
{
	return constraint()->isLimited(limitIndex);
}

void Physics6DofJoint::SetFrames(const Matrix43& frameA, const Matrix43& frameB)
{
	Assert(!BadMatrix43(frameA));
	Assert(!BadMatrix43(frameB));
//	btTransform tr1 = constraint()->getRigidBodyA()->getCenterOfMassTransform().inverseTimes(Matrix2btTransform(frameA));
	constraint()->setFrames(BulletWorld::Matrix2btTransform(frameA), BulletWorld::Matrix2btTransform(frameB));
}


void Physics6DofSpringJoint::EnableSpring(int index, bool onOff)
{
	constraint()->enableSpring(index, onOff);
}
void Physics6DofSpringJoint::SetStiffness(int index, float stiffness)
{
	constraint()->enableSpring(index, stiffness != 0);
	constraint()->setStiffness(index, stiffness);
}
void Physics6DofSpringJoint::SetDamping(int index, float damping)
{
	constraint()->setDamping(index, damping);
}
void Physics6DofSpringJoint::SetEquilibriumPoint()
{
	constraint()->setEquilibriumPoint();
}
void Physics6DofSpringJoint::SetEquilibriumPoint(int index)
{
	constraint()->setEquilibriumPoint(index);
}
void Physics6DofSpringJoint::SetEquilibriumPoint(int index, float val)
{
	constraint()->setEquilibriumPoint(index, val);
}

void PhysicsSliderJoint::SetLowerLinLimit(float lowerLimit)
{
	constraint()->setLowerLinLimit(lowerLimit);
}

void PhysicsSliderJoint::SetUpperLinLimit(float upperLimit)
{
	constraint()->setUpperLinLimit(upperLimit);
}

void PhysicsSliderJoint::SetLowerAngLimit(float lowerLimit)
{
	constraint()->setLowerAngLimit(lowerLimit);
}

void PhysicsSliderJoint::SetUpperAngLimit(float upperLimit)
{
	constraint()->setUpperAngLimit(upperLimit);
}

void PhysicsSliderJoint::SetSoftnessDirLin(float softnessDirLin)
{
	constraint()->setSoftnessDirLin(softnessDirLin);
}

void PhysicsSliderJoint::SetRestitutionDirLin(float restitutionDirLin)
{
	constraint()->setRestitutionDirLin(restitutionDirLin);
}

void PhysicsSliderJoint::SetDampingDirLin(float dampingDirLin)
{
	constraint()->setDampingDirLin(dampingDirLin);
}

void PhysicsSliderJoint::SetSoftnessDirAng(float softnessDirAng)
{
	constraint()->setSoftnessDirAng(softnessDirAng);
}

void PhysicsSliderJoint::SetRestitutionDirAng(float restitutionDirAng)
{
	constraint()->setRestitutionDirAng(restitutionDirAng);
}

void PhysicsSliderJoint::SetDampingDirAng(float dampingDirAng)
{
	constraint()->setDampingDirAng(dampingDirAng);
}

void PhysicsSliderJoint::SetSoftnessLimLin(float softnessLimLin)
{
	constraint()->setSoftnessLimLin(softnessLimLin);
}

void PhysicsSliderJoint::SetRestitutionLimLin(float restitutionLimLin)
{
	constraint()->setRestitutionLimLin(restitutionLimLin);
}

void PhysicsSliderJoint::SetDampingLimLin(float dampingLimLin)
{
	constraint()->setDampingLimLin(dampingLimLin);
}

void PhysicsSliderJoint::SetSoftnessLimAng(float softnessLimAng)
{
	constraint()->setSoftnessLimAng(softnessLimAng);
}

void PhysicsSliderJoint::SetRestitutionLimAng(float restitutionLimAng)
{
	constraint()->setRestitutionLimAng(restitutionLimAng);
}

void PhysicsSliderJoint::SetDampingLimAng(float dampingLimAng)
{
	constraint()->setDampingLimAng(dampingLimAng);
}

void PhysicsSliderJoint::SetSoftnessOrthoLin(float softnessOrthoLin)
{
	constraint()->setSoftnessOrthoLin(softnessOrthoLin);
}

void PhysicsSliderJoint::SetRestitutionOrthoLin(float restitutionOrthoLin)
{
	constraint()->setRestitutionOrthoLin(restitutionOrthoLin);
}

void PhysicsSliderJoint::SetDampingOrthoLin(float dampingOrthoLin)
{
	constraint()->setDampingOrthoLin(dampingOrthoLin);
}

void PhysicsSliderJoint::SetSoftnessOrthoAng(float softnessOrthoAng)
{
	constraint()->setSoftnessOrthoAng(softnessOrthoAng);
}

void PhysicsSliderJoint::SetRestitutionOrthoAng(float restitutionOrthoAng)
{
	constraint()->setRestitutionOrthoAng(restitutionOrthoAng);
}

void PhysicsSliderJoint::SetDampingOrthoAng(float dampingOrthoAng)
{
	constraint()->setDampingOrthoAng(dampingOrthoAng);
}

void PhysicsSliderJoint::SetPoweredLinMotor(bool onOff)
{
	constraint()->setPoweredLinMotor(onOff);
}

void PhysicsSliderJoint::SetTargetLinMotorVelocity(float targetLinMotorVelocity)
{
	constraint()->setTargetLinMotorVelocity(targetLinMotorVelocity);
}

void PhysicsSliderJoint::SetMaxLinMotorForce(float maxLinMotorForce)
{
	constraint()->setMaxLinMotorForce(maxLinMotorForce);
}

void PhysicsSliderJoint::SetPoweredAngMotor(bool onOff)
{
	constraint()->setPoweredAngMotor(onOff);
}

void PhysicsSliderJoint::SetTargetAngMotorVelocity(float targetAngMotorVelocity)
{
	constraint()->setTargetAngMotorVelocity(targetAngMotorVelocity);
}

void PhysicsSliderJoint::SetMaxAngMotorForce(float maxAngMotorForce)
{
	constraint()->setMaxAngMotorForce(maxAngMotorForce);
}

//-----------------------------------------------------------------------
Physics6DofSpringJoint BulletWorld::Create6DOFSpringJoint(PhysicsController* pc1, PhysicsController* pc2, const Matrix43& frame1, const Matrix43& frame2, bool block)
{
	BulletDynamicController* bc1 = BulletDynamicController::SafeCast(pc1);
	BulletDynamicController* bc2 = BulletDynamicController::SafeCast(pc2);
	Assert(bc1);
	Assert(!BadMatrix43(frame1));
	Assert(!BadMatrix43(frame2));

	btRigidBody* body1 = bc1->GetRigidBody();

	btGeneric6DofSpringConstraint* sixDof;
	btTransform tr1 = Matrix2btTransform(frame1);
	if(bc2 != NULL)
	{
		btRigidBody* body2 = bc2->GetRigidBody();
		btTransform tr2 = Matrix2btTransform(frame2);
		sixDof = new btGeneric6DofSpringConstraint(*body1, *body2, tr1, tr2, true);
	}
	else
	{
		sixDof = new btGeneric6DofSpringConstraint(*body1, tr1, true);
	}
	
	m_pDynamicsWorld->addConstraint(sixDof, block);
	sixDof->setEquilibriumPoint();
	sixDof->setDbgDrawSize(btScalar(0.1f));
	return Physics6DofSpringJoint(sixDof);
}

//-----------------------------------------------------------------------
Physics6DofJoint BulletWorld::Create6DOFJoint(PhysicsController* pc1, PhysicsController* pc2, const Matrix43& frame1, const Matrix43& frame2, bool block)
{
	BulletDynamicController* bc1 = BulletDynamicController::SafeCast(pc1);
	BulletDynamicController* bc2 = BulletDynamicController::SafeCast(pc2);
	Assert(bc1);
	Assert(!BadMatrix43(frame1));
	Assert(!BadMatrix43(frame2));

	btRigidBody* body1 = bc1->GetRigidBody();

	btGeneric6DofConstraint* sixDof;
	btTransform tr1 = Matrix2btTransform(frame1);
	if(bc2 != NULL)
	{
		btRigidBody* body2 = bc2->GetRigidBody();
		btTransform tr2 = Matrix2btTransform(frame2);
		sixDof = new btGeneric6DofConstraint(*body1, *body2, tr1, tr2, true);
	}
	else
	{
		sixDof = new btGeneric6DofConstraint(*body1, tr1, true);
	}
	
	m_pDynamicsWorld->addConstraint(sixDof, block);
	sixDof->setDbgDrawSize(btScalar(0.1f));
	return Physics6DofJoint(sixDof);
}

//-----------------------------------------------------------------------
PhysicsSliderJoint BulletWorld::CreateSliderJoint(PhysicsController* pc1, PhysicsController* pc2, const Matrix43& frame1, const Matrix43& frame2, bool block)
{
	BulletDynamicController* bc1 = BulletDynamicController::SafeCast(pc1);
	BulletDynamicController* bc2 = BulletDynamicController::SafeCast(pc2);
	Assert(bc1);
	Assert(!BadMatrix43(frame1));
	Assert(!BadMatrix43(frame2));

	btRigidBody* body1 = bc1->GetRigidBody();
	btSliderConstraint* sliderJoint;
	btTransform tr1 = Matrix2btTransform(frame1);
	
	if(bc2 != NULL)
	{
		btRigidBody* body2 = bc2->GetRigidBody();
		btTransform tr2 = Matrix2btTransform(frame2);
		sliderJoint = new btSliderConstraint(*body1, *body2, tr1, tr2, true);
	}
	else
	{
		sliderJoint = new btSliderConstraint(*body1, tr1, true);
	}

	m_pDynamicsWorld->addConstraint(sliderJoint, block);
	sliderJoint->setDbgDrawSize(btScalar(0.1f));
	return PhysicsSliderJoint(sliderJoint);
}

//-----------------------------------------------------------------------
void DumpObject(const Object* ent, const btCollisionObject* obj)
{
  Assert2(0, "not implemented");
  /*
	const btRigidBody* rb = btRigidBody::upcast(obj);

	LogF(LL_NORMAL, "%s %s '%s' name '%s' object '%s' at <%f %f %f>",
		obj->isActive() ? "Active" : "Inactive",
		rb ? "RigidBody" : "CollisionObject",
		ent->GetClass()->GetName(),
		ent->GetName(),
		ent->GetResourceObject() ? ent->GetResourceObject()->GetObjectName().cstr() : "",
		ent->IGetTranslation().x, ent->IGetTranslation().y, ent->IGetTranslation().z);

	if(rb)
	{
		LogF(LL_NORMAL, " Linear <%f %f %f> Angular <%f %f %f>",
			rb->getLinearVelocity().getX(), rb->getLinearVelocity().getY(), rb->getLinearVelocity().getZ(),
			rb->getAngularVelocity().getX(), rb->getAngularVelocity().getY(), rb->getAngularVelocity().getZ());
		if(rb->getLinearVelocity().length() > 50)
			LogF(LL_NORMAL, " WARNING: Linear velocity too high!");
		if(rb->getAngularVelocity().length() > 50)
			LogF(LL_NORMAL, " WARNING: Angular velocity too high!");
	}
  */
}

//-----------------------------------------------------------------------
uint BulletWorld::GetNumDynamicBodies() const
{
	btAlignedObjectArray<btCollisionObject*>& objects = m_pDynamicsWorld->getNonStaticCollisionObjects();
	return objects.size();
}

//-----------------------------------------------------------------------
PhysicsController* BulletWorld::GetDynamicBody(uint index) const
{
  btAlignedObjectArray<btCollisionObject*>& objects = m_pDynamicsWorld->getNonStaticCollisionObjects();
	return ((Object*)objects[index])->GetPhysics();
}

//-----------------------------------------------------------------------
void BulletWorld::DumpActiveBodies()
{
	btAlignedObjectArray<btCollisionObject*>& objects = m_pDynamicsWorld->getNonStaticCollisionObjects();
	uint total = 0;
	for(int n = 0; n < objects.size(); n++)
	{
		if(objects[n]->isActive())
		{
			const Object* ent = (Object*)objects[n]->getUserPointer();
			if(ent)
			{
				DumpObject(ent, objects[n]);
				total++;
			}
		}
	}
	//LogF(LL_NORMAL, "Total %d objects", total);
  LogF("Total %d objects", total);
}

//-----------------------------------------------------------------------
void BulletWorld::DumpActiveContacts()
{
	btDispatcher* dispatcher = m_pDynamicsWorld->getDispatcher();

	uint total = 0;
	uint numManifolds = dispatcher->getNumManifolds();
	for (uint i = 0; i < numManifolds; i++)
	{
		const btPersistentManifold* contactManifold = dispatcher->getManifoldByIndexInternal(i);
		if(contactManifold->getNumContacts() == 0)
			continue;

		const btCollisionObject* obA = static_cast<const btCollisionObject*>(contactManifold->getBody0());
		const btCollisionObject* obB = static_cast<const btCollisionObject*>(contactManifold->getBody1());
	
		if(!obA->isActive() && !obB->isActive())
			continue;

		Object* ent0 = (Object*)obA->getUserPointer();
		Object* ent1 = (Object*)obB->getUserPointer();
		if(ent0 == NULL || ent1 == NULL)
			continue;

		float maxAppliedImpulse = -FLT_MAX;
		uint maxContact = -1;

		for (int j = 0; j < contactManifold->getNumContacts(); j++)
		{
			const btManifoldPoint& pt = contactManifold->getContactPoint(j);
			if (pt.getDistance() > contactManifold->getContactProcessingThreshold())
				continue;

			if(pt.getAppliedImpulse() > maxAppliedImpulse)
			{
				maxAppliedImpulse = pt.getAppliedImpulse();
				maxContact = j;
			}
		}

		if(maxContact != -1)
		{
			const btManifoldPoint& pt = contactManifold->getContactPoint(maxContact);

			//LogF(LL_NORMAL, "AppliedImpulse %f", pt.getAppliedImpulse());
			//LogF(LL_NORMAL, "Body A:");				
      LogF("AppliedImpulse %f\n", pt.getAppliedImpulse());
      LogF("Body A:");				
			DumpObject(ent0, obA);

			//LogF(LL_NORMAL, "Body B:");
      LogF("Body B:");
			DumpObject(ent1, obB);
			total++;
		}
	}
	//LogF(LL_NORMAL, "Total %d contacts", total);
  LogF("Total %d contacts", total);
}

}