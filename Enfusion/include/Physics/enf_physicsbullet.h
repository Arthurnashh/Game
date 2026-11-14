//-----------------------------------------------------------------------------
// File: enf_physicsbullet.h
//
// Desc: Bullet implementation
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_PHYSICSBULLET_H
#define ENF_PHYSICSBULLET_H

#include "enf_config.h"
#include "common/enf_set.h"
#include "common/enf_types.h"
//#include "common/enf_hashmap.h"
//#include "baseworld/enf_entity.h"
#include "LinearMath/btMotionState.h"

//#include "physicsCommon.hpp"
#include "physics/enf_worldphysics.h"
//#include "Es\Containers\typeDefines.hpp"

#include <Es/Memory/normalNew.hpp>

//shutup dllexport warning on btMotionState
#pragma warning( disable : 4275 )

/**@defgroup Bullet Bullet Physics system
	@ingroup Physics
@{*/


class btCollisionObject;
class btRigidBody;
class btCollisionShape;
class btCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btConstraintSolver;
class btDynamicsWorld;
class btDiscreteDynamicsWorld;
class btMaterial;
struct btVehicleRaycaster;
class btRaycastVehicle;
class btInteractionBlock;
class btGeneric6DofConstraint;
class btGeneric6DofSpringConstraint;
class btHingeConstraint;
class btPoint2PointConstraint;
class btConeTwistConstraint;
class btTypedConstraint;
class btSliderConstraint;

class Object;
class Entity;
class Landscape;

namespace enf
{
struct BaseTrace;
class BulletObjectGeometry;
class VObject;
class SerializeOutputAdapter;
class SerializeInputAdapter;
class BulletManager;
class BulletWorld;
struct ContactFilter;
class BulletDebugDraw;
class Class;

extern PUBLIC_API BulletManager* g_pBulletManager;

const uint MATERIAL_SHIFT	= 24;
const float DEFAULT_GRAVITY = 9.8f;

//hodnoty k doladeni v realne pouzivane scene
const float MEQ_WORLDSCALE = 1.f;//2.f;//32.0f;
const float MEQ_WORLDISCALE = 1.f;//0.5f;//0.03125f;
const float BT_MARGIN = 0.0125f;

//---------------------------------------------------------------------------
struct BulletPhysicsMaterial: public PhysicsMaterial
{
	ENF_DECLARE_SMALLOBJECT(BulletPhysicsMaterial);

	float			m_fRestitution;
	float			m_fDynamicFriction;
	float			m_fLateralStiffness;
	float			m_fLongitudalStiffness;
	bool			m_bSingleSided;

	btMaterial*	m_pBtMaterial;
	BulletPhysicsMaterial(CStr name);
	BulletPhysicsMaterial();
	~BulletPhysicsMaterial();

	void Load(ParamEntryPar entry);
};

//---------------------------------------------------------------------------
struct BulletPhysicsMaterialCombiner: public BulletPhysicsMaterial
{
	ENF_DECLARE_SMALLOBJECT(BulletPhysicsMaterialCombiner);

	const BulletPhysicsMaterial*	m_pOther;
	BulletPhysicsMaterialCombiner(CStr name): BulletPhysicsMaterial(name), m_pOther(NULL) {}
	BulletPhysicsMaterialCombiner(): BulletPhysicsMaterial(), m_pOther(NULL) {}
	~BulletPhysicsMaterialCombiner();
};

//-----------------------------------------------------------------------
//!Wrapper over Bullet rigidBody
class PUBLIC_API BulletController: public PhysicsController
{
protected:
	friend class BulletWorld;
	btCollisionObject*	m_pBody;
	BulletObjectGeometry*	m_pGeometries;

	~BulletController();
	BulletController();
	BulletController(btCollisionObject* rb);

	ENF_INLINE Object* GetOwnerInline() const;

public:

	ENF_DECLARE_ITEM(BulletController, PhysicsController);
	void SetElementTransform(uint element, const Matrix4& mat);
	btCollisionObject* GetCollisionObject() const
	{
		return m_pBody;
	}
	//void AnimateElements(AnimatorInterface* anim);

	ObjectGeometry* GetObjectGeometry() const;
	virtual uint GetNumElements() const;
	virtual uint FindElement(CStr elementName) const;

	BulletWorld* GetWorld() const;
	Object* GetOwner() const;

	virtual void BeforeSimulate(BulletWorld* engine)
	{
	}

	virtual void AfterSimulate(BulletWorld* engine)
	{
	}
	virtual bool EnableCCD(bool enable);
  virtual float GetCCDRadius();

	void SetCCDThreshold(float maxMotion, float sweptRadius);

	bool SetSolid(bool enableSolid);
	void SetMass(float mass) { Assert2(0, "Unsupported"); }
	float GetMass() const { return 0.0f;}
	void SetInertiaTensor(const Matrix3& mat) {}
	void SetInertiaTensor(const Vector3& vec) {}
	void ComputeInertiaTensor() {}
	void SetVelocity(const Vector3& vel) {}
	Vector3 GetVelocity() const { return Vector3(0.0f, 0.f, 0.f);}
	void AddVelocity(const Vector3& vel) {}
	Vector3 GetVelocityAt(const Vector3& pos) const { return Vector3(0.f, 0.f, 0.0f);}
	void ApplyImpulseAt(const Vector3& pos, const Vector3& impulse) {}
	void ApplyImpulse(const Vector3& impulse, const bool activate = true) {}
	void ApplyForceAt(const Vector3& pos, const Vector3& force) {}
	void ApplyForce(const Vector3& force) {}
	void ApplyTorque(const Vector3& torque) {}
	void SetAngularVelocity(const Vector3& vel) {}
	Vector3 GetAngularVelocity() const { return Vector3(0.f, 0.f, 0.0f);}
	bool SetDynamic(bool dynamic) { Assert2(dynamic == false, "Unsupported"); return false; }

	virtual bool IsDynamic() const
	{
		return false;
	}
	virtual void SetMatrix(const Matrix4& mat);

	virtual bool IsActive() const;
	bool SetActive(ActiveState active);

	virtual bool AddGeoms(PhysicsGeomDef* geoms, uint numGeoms, uint interactionLayers = 0xffffffff);
	bool Intersects(const Vector3& start, const Vector3& stop, uint interactionLayers = 0xffffffff);
	bool TraceMove(BaseTrace* trace);
	void SetInteractionLayer(uint mask, int geom = -1);
	uint GetInteractionLayer(int geom = -1) const;
	void UpdateEntityFlags(Object* ent, uint oldFlags, uint newFlags);
	void UpdateEntityEvents(Object* ent, uint oldEvents, uint newEvents);
	virtual void ForceUpdateFrame(const Matrix4& frame);
	virtual void ForceUpdateOrientation(const Matrix3& orientation);
	virtual void Synchronize();
	virtual AAB GetBoundBox() const;
	virtual Matrix43 GetWorldTransform() const;
};

//---------------------------------------------------------------------------
class PUBLIC_API BulletDynamicController: public BulletController, protected btMotionState
{
protected:
	friend class BulletWorld;

	virtual void	getWorldTransform(btTransform& centerOfMassWorldTrans ) const;
	virtual void	setWorldTransform(const btTransform& centerOfMassWorldTrans);

	~BulletDynamicController();
	BulletDynamicController();
	BulletDynamicController(btRigidBody* rb);

public:
	ENF_DECLARE_ITEM(BulletDynamicController, BulletController);

	btRigidBody* GetRigidBody() const
	{
		return (btRigidBody*)m_pBody;
	}

	virtual void SetMatrix(const Matrix4& mat);
	virtual bool IsDynamic() const;
	void SetMass(float mass);
	float GetMass() const;
	void SetInertiaTensor(const Matrix3& mat);
	void SetInertiaTensor(const Vector3& vec);
	void ComputeInertiaTensor();
	void SetVelocity(const Vector3& vel);
	Vector3 GetVelocity() const;
	void AddVelocity(const Vector3& vel);
	Vector3 GetVelocityAt(const Vector3& pos) const;
	void ApplyImpulseAt(const Vector3& pos, const Vector3& impulse);
	void ApplyImpulse(const Vector3& impulse, const bool activate = true);
	void ApplyForceAt(const Vector3& pos, const Vector3& force);
	void ApplyForce(const Vector3& force);
	void ApplyTorque(const Vector3& torque);
	void SetAngularVelocity(const Vector3& vel);
	Vector3 GetAngularVelocity() const;
	bool SetDynamic(bool dynamic);
	virtual void ForceUpdateFrame(const Matrix4& frame);
	virtual void ForceUpdateOrientation(const Matrix3& orientation);
	void SetDamping(float linearDamping, float angularDamping);
	bool SetCollisionResponse(bool enableResponse);
	bool SetLinearFactor(const Vector3& linearFactor);
	bool SetSleepingTreshold(float linearTreshold, float angularTreshold);

	bool EnableGravity(bool enableGravity);
};

//---------------------------------------------------------------------------
class PUBLIC_API BulletDynamicControllerNC: public BulletDynamicController
{
protected:
	friend class BulletWorld;

	//!Center of mass offset
	Vector3			m_vCOMOffset;
	virtual void	getWorldTransform(btTransform& centerOfMassWorldTrans ) const;
	virtual void	setWorldTransform(const btTransform& centerOfMassWorldTrans);

	BulletDynamicControllerNC();
	BulletDynamicControllerNC(btRigidBody* rb, const Vector3& comOffset);

public:
	ENF_DECLARE_ITEM(BulletDynamicControllerNC, BulletDynamicController);

	virtual void SetMatrix(const Matrix4& mat);
	const Vector3& GetCenterOfMass() const
	{
		return m_vCOMOffset;
	}
	virtual void ForceUpdateFrame(const Matrix4& frame);
	virtual void ForceUpdateOrientation(const Matrix3& orientation);
	virtual Matrix43 GetWorldTransform() const;
};

//---------------------------------------------------------------------------
struct PUBLIC_API VehicleWheelParams
{
	float	m_fSuspensionStiffness;
	float	m_fSuspensionCompression;
	float	m_fSuspensionDamping;
	float	m_fMaxSuspensionTravel;
//	float	m_fFrictionSlip;
	float m_fRollInfluence;
	VehicleWheelParams();
};

enum WheelContactType
{
	WCT_NO_CONTACT,
	WCT_SOLID,
	WCT_WATER,
};

struct WheelContactInfo
{
	WheelContactType	m_Type;
	Vector3				m_vEndPos;
	float					m_fSuspensionLength;
	float					m_fFraction;
	Vector3				m_vContactNormal;
	const PhysicsMaterial*	m_pMaterial;
	Object*				m_pContactEntity;
};

//---------------------------------------------------------------------------
class PUBLIC_API BulletVehicleController: public BulletDynamicControllerNC
{
protected:
	friend class BulletWorld;
	
	btVehicleRaycaster*	m_pVehicleRaycaster;
	btRaycastVehicle*		m_pVehicle;

	~BulletVehicleController();
	BulletVehicleController();
	BulletVehicleController(btRigidBody* rb, const Vector3& comOffset);

public:
	ENF_DECLARE_ITEM(BulletVehicleController, BulletDynamicControllerNC);
	void AddWheel(const Vector3& lpos, const Vector3& suspensionDir, const Vector3& axleDir, float radius, const VehicleWheelParams& params, const PhysicsMaterial* mat);
	uint GetNumWheels() const;
	void ApplyEngineForce(uint wheel, float force);
	void SetBrake(uint wheel, float brake);
	void SetSteering(uint wheel, float steering);
	float GetSteering(uint wheel) const;
	float GetWheelAngle(uint wheel) const;
	float GetWheelAngularVelocity(uint wheel) const;
	float GetWheelSuspensionLength(uint wheel) const;
	Vector3 GetWheelEndPosition(uint wheel) const;
	Object* HasWheelContact(uint wheel) const;
	bool GetWheelContact(uint wheel, WheelContactInfo& info) const;
	void SetWaterLevel(float level, const PhysicsMaterial* mat, PhysicsController* obj);
	//!Returns current speed in km/h
	float GetSpeed() const;

	btRaycastVehicle* GetVehicle() const
	{
		return m_pVehicle;
	}
};

//-----------------------------------------------------------------------
class PUBLIC_API BulletPhysicsJoint: public PhysicsJoint
{	
public:
  BulletPhysicsJoint(btTypedConstraint* p): PhysicsJoint((void*)p) {}
	BulletPhysicsJoint(): PhysicsJoint() {}
	Object* GetEntityA();
	Object* GetEntityB();
	btTypedConstraint* constraint() const { return (btTypedConstraint*)ptr; }
};

//-----------------------------------------------------------------------
class PUBLIC_API PhysicsHingeJoint: public BulletPhysicsJoint
{
public:
	PhysicsHingeJoint(btHingeConstraint* p): BulletPhysicsJoint((btTypedConstraint*)p) {}
	void SetLimit(float low,float high,float _softness = 0.9f, float _biasFactor = 0.3f, float _relaxationFactor = 1.0f);
	void SetAxis(const Vector3& axisInA);
	void SetMotor(bool enable, float targetVelocity, float maxImpulse);
	void SetMotorTargetAngle(float angle, float dt, float maxImpulse);

	btHingeConstraint* constraint() const { return (btHingeConstraint*)ptr; }
};

//---------------------------------------------------------------------------
class PUBLIC_API PhysicsBallSocketJoint: public BulletPhysicsJoint
{
public:
	PhysicsBallSocketJoint(btPoint2PointConstraint* p): BulletPhysicsJoint((btTypedConstraint*)p) {}
	btPoint2PointConstraint* constraint() const { return (btPoint2PointConstraint*)ptr; }
};

//---------------------------------------------------------------------------
class PUBLIC_API PhysicsConeTwistJoint: public BulletPhysicsJoint
{
public:
	PhysicsConeTwistJoint(btConeTwistConstraint* p): BulletPhysicsJoint((btTypedConstraint*)p) {}


	void SetAngularOnly(bool angularOnly);
	void SetLimit(int limitIndex, float limitValue);
	// setLimit(), a few notes:
	// _softness:
	//		0->1, recommend ~0.8->1.
	//		describes % of limits where movement is free.
	//		beyond this softness %, the limit is gradually enforced until the "hard" (1.0) limit is reached.
	// _biasFactor:
	//		0->1?, recommend 0.3 +/-0.3 or so.
	//		strength with which constraint resists zeroth order (angular, not angular velocity) limit violation.
	// __relaxationFactor:
	//		0->1, recommend to stay near 1.
	//		the lower the value, the less the constraint will fight velocities which violate the angular limits.
	void SetLimit(float _swingSpan1,float _swingSpan2,float _twistSpan, float _softness = 1.f, float _biasFactor = 0.3f, float _relaxationFactor = 1.0f);
	btConeTwistConstraint* constraint() const { return (btConeTwistConstraint*)ptr; }
};

//---------------------------------------------------------------------------
class PUBLIC_API PhysicsSliderJoint: public BulletPhysicsJoint
{
public:
	PhysicsSliderJoint(btSliderConstraint* p): BulletPhysicsJoint((btTypedConstraint*)p) {}
	
	void SetLowerLinLimit(float lowerLimit);
	void SetUpperLinLimit(float upperLimit);
	void SetLowerAngLimit(float lowerLimit);
	void SetUpperAngLimit(float upperLimit);
	void SetSoftnessDirLin(float softnessDirLin);
	void SetRestitutionDirLin(float restitutionDirLin);
	void SetDampingDirLin(float dampingDirLin);
	void SetSoftnessDirAng(float softnessDirAng);
	void SetRestitutionDirAng(float restitutionDirAng);
	void SetDampingDirAng(float dampingDirAng);
	void SetSoftnessLimLin(float softnessLimLin);
	void SetRestitutionLimLin(float restitutionLimLin);
	void SetDampingLimLin(float dampingLimLin);
	void SetSoftnessLimAng(float softnessLimAng);
	void SetRestitutionLimAng(float restitutionLimAng);
	void SetDampingLimAng(float dampingLimAng);
	void SetSoftnessOrthoLin(float softnessOrthoLin);
	void SetRestitutionOrthoLin(float restitutionOrthoLin);
	void SetDampingOrthoLin(float dampingOrthoLin);
	void SetSoftnessOrthoAng(float softnessOrthoAng);
	void SetRestitutionOrthoAng(float restitutionOrthoAng);
	void SetDampingOrthoAng(float dampingOrthoAng);
	void SetPoweredLinMotor(bool onOff);
	void SetTargetLinMotorVelocity(float targetLinMotorVelocity);
	void SetMaxLinMotorForce(float maxLinMotorForce);
	void SetPoweredAngMotor(bool onOff);
	void SetTargetAngMotorVelocity(float targetAngMotorVelocity);
	void SetMaxAngMotorForce(float maxAngMotorForce);
	btSliderConstraint* constraint() const { return (btSliderConstraint*)ptr; }
};

//---------------------------------------------------------------------------
class PUBLIC_API Physics6DofJoint: public BulletPhysicsJoint
{
public:
	Physics6DofJoint(btGeneric6DofConstraint* p): BulletPhysicsJoint((btTypedConstraint*)p) {}
	Physics6DofJoint() {}
	void	SetLinearLowerLimit(const Vector3& linearLower);
	void	SetLinearUpperLimit(const Vector3& linearUpper);
	void	SetAngularLowerLimit(const Vector3& angularLower);
	void	SetAngularUpperLimit(const Vector3& angularUpper);
	//first 3 are linear, next 3 are angular
	void SetLimit(int axis, float lo, float hi);
	bool	IsLimited(int limitIndex) const;
	void SetFrames(const Matrix4& matA, const Matrix4& matB);
	btGeneric6DofConstraint* constraint() const { return (btGeneric6DofConstraint*)ptr; }
};

//---------------------------------------------------------------------------
class PUBLIC_API Physics6DofSpringJoint: public Physics6DofJoint
{
public:
	Physics6DofSpringJoint(btGeneric6DofSpringConstraint* p): Physics6DofJoint((btGeneric6DofConstraint*)p) {}
	Physics6DofSpringJoint() {}
	void EnableSpring(int index, bool onOff);
	void SetStiffness(int index, float stiffness);
	void SetDamping(int index, float damping);
	void SetEquilibriumPoint();
	void SetEquilibriumPoint(int index);
	void SetEquilibriumPoint(int index, float val);
	btGeneric6DofSpringConstraint* constraint() const { return (btGeneric6DofSpringConstraint*)ptr; }
};

class BulletBoneController;
//-----------------------------------------------------------------------
class PUBLIC_API BulletRagdollController: public PhysicsController
{
protected:
	friend class BulletWorld;
	friend class RagdollFactory;

	Object* m_pEntity;
	uint m_iInteractionLayer;
	enf::Array<BulletBoneController*>	m_Bones;
	enf::Array<BulletPhysicsJoint>	m_Joints;

public:
	ENF_DECLARE_ITEM(BulletRagdollController, PhysicsController);
	void AddBone(BulletBoneController* bone);
	Object* GetOwner() const
	{
    return m_pEntity;
	}
	uint GetNumBones() const
	{
    return m_Bones.GetCardinality();
	}
	BulletBoneController* FindBoneByNodeIndex(uint index);
	BulletBoneController* FindBoneByName(CStr name);
	BulletBoneController* GetBone(uint index)
	{
		return m_Bones[index];
	}
	bool TraceMove(BaseTrace* trace);
	bool Intersects(const Vector3& start, const Vector3& stop, uint interactionLayers = 0xffffffff);
  Matrix4 GetWorldTransform() const;// { m_pEntity->IGetMatrix();}
	bool IsActive() const;
	bool IsDynamic() const { return true; }

	BulletRagdollController(): m_pEntity(NULL), m_iInteractionLayer(0xffffffff) {}
	~BulletRagdollController();
};

//-----------------------------------------------------------------------
class PUBLIC_API BulletBoneController: public BulletDynamicController
{
protected:
	friend class BulletWorld;
	friend class BulletRagdollController;
	friend class RagdollFactory;

	BulletRagdollController* m_pRagdoll;
	Matrix4	m_Body2Bone;
	uint		m_iOriginalIndex;
	CStr		m_strName;

	virtual void	getWorldTransform(btTransform& centerOfMassWorldTrans ) const;
	virtual void	setWorldTransform(const btTransform& centerOfMassWorldTrans);

public:
	ENF_DECLARE_ITEM(BulletBoneController, BulletDynamicController);
	BulletBoneController(btRigidBody* body, const Matrix4& body2bone, uint originalIndex);
	void SetWorldTransform(const Matrix4& trans);
	CStr GetName() const
	{
		return m_strName;
	}
};

//---------------------------------------------------------------------------

class PUBLIC_API BulletWorld: public PhysicsWorld
{
	friend class BulletController;
	friend class RagdollFactory;

protected:
	BulletManager*		m_pManager;

	Landscape*			m_pWorld;
	AAB					m_WorldBounds;
	float					m_fTimeSlice;
	bool					m_bSceneSimulated;

	~BulletWorld() {};


//bullet stuff
	btCollisionDispatcher*		m_pCollisionDispatcher;
	btBroadphaseInterface*		m_pBroadPhase;
	btConstraintSolver*			m_pSolver;
	btDiscreteDynamicsWorld*	m_pDynamicsWorld;
	BulletDebugDraw*				m_pDebugDrawer;
	enf::Set<Object*>					m_BulletEntitiesToMove;
	enf::Set<Object*>					m_BulletEntitiesToUpdate;
	enf::Set<Object*>					m_BulletEntitiesAfterUpdate;
  
	//BulletObjectGeometry* GetObjectGeometry(Object* ent) const;
	BulletBoneController* CreateRagdollBone(Object* ent, const Matrix4& transform, const Matrix4& body2bone, uint originalIndex, float radius, float length, PhysicsMaterial* material, float mass, uint interactionLayer = 0xffffffff);
	
	void UpdateTime(float timeSlice);

public:
	ENF_DECLARE_ITEM(BulletWorld, PhysicsWorld);

	void DumpActiveBodies();
	void DumpActiveContacts();
	uint GetNumDynamicBodies() const;
	PhysicsController* GetDynamicBody(uint index) const;

	Landscape* GetWorld() const
	{
		return m_pWorld;
	}
	btDiscreteDynamicsWorld* GetBulletWorld() const
	{
		return m_pDynamicsWorld;
	}
	void MoveBulletEntity(Object* ent)
	{
    m_BulletEntitiesToMove.Insert(ent);
	}
	void Synchronize(btCollisionObject* body);

	bool						m_bWarming;

	void BulletSimulateEvent(float timeslice);
	void BulletAfterSimulateEvent(float timeslice);
  void RemoveDynamicBodyFromSimulation(Object* obj);


//Bullet <-> Enforce
	ENF_INLINE static Vector3 btVector2Vector(const btVector3& vec)
	{
		return Vector3(vec.getX(), vec.getY(), vec.getZ());
	}

	ENF_INLINE static Vector3 btVector2Point(const btVector3& pt)
	{
		return Vector3(pt.getX() * MEQ_WORLDSCALE, pt.getY() * MEQ_WORLDSCALE, pt.getZ() * MEQ_WORLDSCALE);
	}

	ENF_INLINE static Matrix4 btTransform2Matrix(const btTransform& trans)
	{
    btMatrix3x3 tbasis = trans.getBasis().transpose();
    return Matrix4(btVector2Vector(tbasis.getRow(0)),
					btVector2Vector(tbasis.getRow(1)),
					btVector2Vector(tbasis.getRow(2)),
					btVector2Point(trans.getOrigin()));
	}

	ENF_INLINE static Matrix4 btTransform2Matrix(const btTransform& trans, const Vector3& offset)
	{
		btMatrix3x3 tbasis = trans.getBasis().transpose();
    btVector3 toff = trans.getBasis().transpose() * btVector3(offset.X(), offset.Y(), offset.Z());
		Vector3 origin = btVector2Point(trans.getOrigin());

		return Matrix4(btVector2Vector(tbasis.getRow(0)),
					btVector2Vector(tbasis.getRow(1)),
					btVector2Vector(tbasis.getRow(2)),
					Vector3(origin.X() - toff.getX(), origin.Y() - toff.getY(), origin.Z() - toff.getZ()));
	}

	ENF_INLINE static btVector3 Vector2btVector(const Vector3& vec)
	{
    return btVector3(vec.X(), vec.Y(), vec.Z());
	}

	ENF_INLINE static btVector3 Point2btVector(const Vector3& pt)
	{
		return btVector3(pt.X() * MEQ_WORLDISCALE, pt.Y() * MEQ_WORLDISCALE, pt.Z() * MEQ_WORLDISCALE);
	}


	ENF_INLINE static btTransform Matrix2btTransform(const Matrix4& mat)
	{
    Matrix3 m3 = mat.Orientation();
    return btTransform(
           btMatrix3x3(
              m3.DirectionAside().X(), m3.DirectionUp().X(), m3.Direction().X(),
              m3.DirectionAside().Y(), m3.DirectionUp().Y(), m3.Direction().Y(),
              m3.DirectionAside().Z(), m3.DirectionUp().Z(), m3.Direction().Z()),
           Point2btVector(mat.Position())
					);
	}

	ENF_INLINE static btMatrix3x3 Matrix2btMatrix(const Matrix3& mat)
	{
    DoAssert2(0, "not checked");
		return btMatrix3x3(mat(0, 0), mat(1, 0), mat(2, 0),
              mat(0, 1), mat(1, 1), mat(2, 1),
              mat(0, 2), mat(1, 2), mat(2, 2));
	}

	ENF_INLINE static btTransform Matrix2btTransform(const Matrix4& mat, const Vector3& offset)
	{
    DoAssert2(0, "not checked");
    Vector3 origin = mat.FastTransform(offset);
    
		return btTransform(
						btMatrix3x3(mat(0, 0), mat(1, 0), mat(2, 0),
              mat(0, 1), mat(1, 1), mat(2, 1),
              mat(0, 2), mat(1, 2), mat(2, 2)),
						Point2btVector(origin)
					);
	}


/** \name Ragdoll methods
*/
//@{
	BulletRagdollController* CreateRagdoll(Entity* ent, CStr ragdollDefName, float mass, uint interactionLayer);
	
	bool dRagdollIntersect(Object* ent, BaseTrace* trace);
//@}

	void DestroyBody(Object *ent);

	//BulletDynamicController* CreateDynamicBody(Object* ent, const Vector3& trans, float mass, uint interactionLayer = 0xffffffff);
	//BulletController* CreateStaticBody(Object* ent, uint interactionLayer = 0xffffffff);
	BulletDynamicController* CreateDynamicBody(Object *ent, const Vector3& trans, PhysicsGeomDef* defs, uint numGeoms, float mass, uint interactionLayer = 0xffffffff);
	BulletController* CreateStaticBody(Object* ent, PhysicsGeomDef* defs, uint numGeoms, uint interactionLayer = 0xffffffff);
	BulletVehicleController* CreateVehicle(Object* ent, const Vector3& trans, float mass, uint interactionLayer = 0xffffffff);


/** \name Geometry methods
*/
//@{
	void DestroyGeom(PhysicsGeometry* geom);
	PhysicsGeometry* CreateSphereGeom(float radius);
	PhysicsGeometry* CreateBoxGeom(Vector3 size);
	PhysicsGeometry* CreatePlaneGeom(Vector3 normal, float distance);
	PhysicsGeometry* CreateCapsuleGeom(float radius, Vector3 extent);
	PhysicsGeometry* CreateCylinderGeom(float radius, Vector3 extent);
//@}

/** \name Joint, constraints and interaction methods
*/
//@{
	PhysicsHingeJoint CreateHingeJoint(PhysicsController* body1, PhysicsController* body2, const Vector3& point1, const Vector3& axis1, const Vector3& point2, const Vector3& axis2, bool block);
	PhysicsHingeJoint CreateHingeJoint(PhysicsController* body1, PhysicsController* body2, const Matrix4& frame1, const Matrix4& frame2, bool block);

	BulletPhysicsJoint CreateFixedJoint(PhysicsController* body1, PhysicsController* body2, const Vector3& point1, const Vector3& point2, bool block);
	BulletPhysicsJoint CreateFixedJoint(PhysicsController* pc1, PhysicsController* pc2, const Matrix4& frame1, const Matrix4& frame2, bool block);

	PhysicsBallSocketJoint CreateBallSocketJoint(PhysicsController* body1, PhysicsController* body2, const Vector3& point1, const Vector3& point2, bool block);
	PhysicsConeTwistJoint CreateConeTwistJoint(PhysicsController* body1, PhysicsController* body2, const Matrix4& mat1, const Matrix4& mat2, bool block);
	Physics6DofSpringJoint Create6DOFSpringJoint(PhysicsController* body1, PhysicsController* body2, const Matrix4& frame1, const Matrix4& frame2, bool block);
	Physics6DofJoint Create6DOFJoint(PhysicsController* body1, PhysicsController* body2, const Matrix4& frame1, const Matrix4& frame2, bool block);
	PhysicsSliderJoint CreateSliderJoint(PhysicsController* body1, PhysicsController* body2, const Matrix4& frame1, const Matrix4& frame2, bool block);

	void DestroyJoint(PhysicsJoint& joint);

	void SetInteractionLayer(uint mask1, uint mask2, bool enable);

	btInteractionBlock* CreateInteractionBlock(Object* ent1, Object* ent2);
	btInteractionBlock* CreateInteractionBlock(Object* ent1, uint elem, Object* ent2);
	void DestroyInteractionBlock(btInteractionBlock* ib);

//@}

/** \name Object related methods
*/
//@{
	void RenderEntity(PhysicsController* pc, uint mask = RM_ALL, const char* highlightGeom = NULL);
//@}

/** \name General methods
*/
//@{

	PhysicsManager* GetManager() const;

	void SetGravity(const Vector3& direction);

	bool InitWorldCollisions(bool loadsave, const AAB& bounds);
	bool InitTerrain(Scenery* terr);

	bool InitWorld(Landscape* world, bool loadsave, const AAB& bounds);

	void Serialize(SerializeOutputAdapter* );
	void Serialize(SerializeInputAdapter* );

	void Destroy();

	void Advance(float elapsedtime);
	Vector3 GetGravity() const;

	void WarmUp(float simTime = 5.0f);
	BulletWorld();
//@}
};

}
//@}
#endif //ENF_PHYSICSBULLET_H
