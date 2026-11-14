//-----------------------------------------------------------------------------
// File: baseworld/enf_worldphysics.h
//
// Desc: 
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_WORLDPHYSICS_H
#define ENF_WORLDPHYSICS_H

#include "enf_config.h"
//#include "common/enf_mathlib.h"
#include "engine/enf_module.h"
#include "Common/enf_strings.h"
#include "El\Math\math3d.hpp"
#include "Common/enf_mathlib.h"
#include "Common/enf_staticbuffer.h"

#include "Es\Strings\bString.hpp"

#include "El\ParamFile\paramFileDecl.hpp"

class Object;
class LODShape;
class EntityType;
//class Landscape;
class Scenery;
class Landscape;

namespace enf
{
class PhysicsWorld;
class PhysicsManager;
class PhysicsController;
class PhysicsJoint;
struct PhysicsGeomDef;
class IFFInput;
class Terrain;

class SerializeOutputAdapter;
class SerializeInputAdapter;
struct MTrace;
struct BaseTrace;

/** @defgroup Physics Physics management and implementations
@{*/

class PhysicsGeometry;

const uint MAX_BASE_MATERIALS = 256;

struct PhysicsMaterial: public RefCount, private NoCopy
{
	ENF_DECLARE_SMALLOBJECT(PhysicsMaterial);
  BString<256>  m_Name;
	uint		      m_iMaterialIndex; //engine material index (surfparm)
  float         m_roughness;

	PhysicsMaterial(CStr name);
  PhysicsMaterial();
  virtual ~PhysicsMaterial() {}
};

//---------------------------------------------------------------------------
const uint ObjectGeometryTypeID = 2000;

enum GeometryType
{
	GT_NONE,
	GT_CONVEX,
	GT_BOX,
	GT_SPHERE,
	GT_CAPSULE,
	GT_MESH,
	GT_CYLINDER
};

struct RBElement
{
	GeometryType		m_Type;
	CStr					m_Name;
	Matrix43				m_Frame;
	int					m_iParentNode;
	PhysicsMaterial*	m_pMaterial;

	RBElement():
		m_Type(GT_NONE),
		m_Frame(true),
		m_iParentNode(0),
		m_pMaterial(NULL)
	{
	}
};


//---------------------------------------------------------------------------
/*!
Container for physics geometry loaded (usually) from VObject
*/
class ObjectGeometry: public Item
{
private: 
  DString m_strName;

protected:

	ObjectGeometry():m_vCenterOfMass (0.0f) {}
	~ObjectGeometry() {}
	Vector3	m_vCenterOfMass;
  
public:

	ENF_DECLARE_ITEM(ObjectGeometry, Item);
	ENF_DECLARE_SMALLOBJECT(ObjectGeometry);

  //---------------------------------------------------------------------------
  CStr GetName() const
  {
	  return m_strName.c_str();
  }

  //---------------------------------------------------------------------------
  void SetName(CStr name)
  {
	  m_strName = name.cstr();
  }

	ObjectGeometry(CStr name, const Vector3& com):m_vCenterOfMass (com)
	{
		SetName(name);
	}
	virtual void SetupPhysicsGeomDefs(PhysicsGeomDef* geoms, uint numGeoms)=0;

	virtual uint GetNumElements() const=0;
	virtual const RBElement& GetElement(uint index) const=0;
	const Vector3& GetCenterOfMass() const
	{
		return m_vCenterOfMass;
	}
};

//---------------------------------------------------------------------------
struct PhysicsGeomDef
{
	//!For reference. It's stored as user data to RB
	const char*			m_Name;
	//!Physics geometry. Native
	PhysicsGeometry*	m_pGeom;
	//!Local transformation
	Matrix43				m_Frame;
	//!Parent node
	int					m_iParentNode;
	//!Physics material
	PhysicsMaterial*	m_pMaterial;
	//!Interaction layer mask
	uint					m_iInteractionLayer;
	//!Filled upon AddGeoms
	uint					m_iElementIndex;

	PhysicsGeomDef(): m_Name(NULL), m_pGeom(NULL), m_Frame(true), m_iParentNode(0), m_pMaterial(NULL), m_iInteractionLayer(0xffffffff) {}
	PhysicsGeomDef(const char* name, PhysicsGeometry* geom, const Matrix43& frame, int parentNode, PhysicsMaterial* material, uint interactionLayer = 0xffffffff):
		m_Name(name), m_pGeom(geom), m_Frame(frame), m_iParentNode(parentNode), m_pMaterial(material), m_iInteractionLayer(interactionLayer),
		m_iElementIndex(INDEX_NOT_FOUND) {}
  void Init(const char* name, PhysicsGeometry* geom, const Matrix43& frame, int parentNode, PhysicsMaterial* material, uint interactionLayer = 0xffffffff)
  {
    m_Name = name; m_pGeom = geom; m_Frame = frame; m_iParentNode = parentNode; m_pMaterial = material; m_iInteractionLayer = interactionLayer;
		m_iElementIndex = INDEX_NOT_FOUND;
  }
};


//---------------------------------------------------------------------------
/*!
Implementation of physical simulation of BaseWorld.
*/
enum PhysicsWorldTypes
{
	PhysicsWorldTypeID = 3100,
	BulletWorldTypeID,
};

class PUBLIC_API PhysicsWorld: public BaseItem
{
protected:
	int	m_iLock;

public:
	ENF_DECLARE_ITEM(PhysicsWorld, BaseItem);

	virtual void Destroy()=0;
	virtual PhysicsManager* GetManager() const=0;

	void Lock()
	{
		m_iLock++;
	}
	void Unlock()
	{
		Assert(m_iLock > 0);
		m_iLock--;
	}
	bool IsLocked() const
	{
		return m_iLock > 0;
	}

	virtual void DumpActiveBodies() {}

/** \name Rigid body methods (New)
*/
//@{
	//virtual PhysicsController* CreateStaticBody(Object *ent, uint interactionLayer = 0xffffffff)=0;
	//virtual PhysicsController* CreateDynamicBody(Object *ent, const Vector3& comOffset, float mass = 0, uint interactionLayer = 0xffffffff)=0;
	virtual PhysicsController* CreateStaticBody(Object *ent, PhysicsGeomDef* defs, uint numGeoms, uint interactionLayer = 0xffffffff)=0;
	virtual PhysicsController* CreateDynamicBody(Object *ent, const Vector3& comOffset, PhysicsGeomDef* defs, uint numGeoms, float mass = 0, uint interactionLayer = 0xffffffff)=0;

	//virtual void RemoveBody(Object *ent)=0;//PhysicsController* controler)=0;
	//virtual void AddStaticBody(Object *ent)=0;//PhysicsController* controler)=0;

	virtual uint GetNumDynamicBodies() const=0;
	virtual PhysicsController* GetDynamicBody(uint index) const=0;

/*!
Destroys physics controller.
\param ent The entity which controller has to be destroyed
*/
	virtual void DestroyBody(Object *ent)=0;
	virtual void DestroyJoint(PhysicsJoint& joint)=0;
//@}


/** \name Geometry methods
*/
//@{
	virtual void DestroyGeom(PhysicsGeometry* geom)=0;
	virtual PhysicsGeometry* CreateSphereGeom(float radius)=0;
	virtual PhysicsGeometry* CreateBoxGeom(Vector3 size)=0;
	virtual PhysicsGeometry* CreatePlaneGeom(Vector3 normal, float distance)=0;
	virtual PhysicsGeometry* CreateCapsuleGeom(float radius, Vector3 extent)=0;
	virtual PhysicsGeometry* CreateCylinderGeom(float radius, Vector3 extent)=0;
//@}

/*!
Advances physics simulation
\param elapsedtime Time slice of last frame
*/
	virtual void Advance(float elapsedtime)=0;

/*!
Serializes physics world
\param adapter Adapter to which we serialize
*/
	virtual void Serialize(SerializeOutputAdapter* adapter)=0;

/*!
Serializes physics world
\param adapter Adapter from which we serialize
*/
	virtual void Serialize(SerializeInputAdapter* adapter)=0;

	enum RenderMask
	{
		RM_CONVEX	= BITF(0),
		RM_BOX		= BITF(1),
		RM_CAPSULE	= BITF(2),
		RM_SPHERE	= BITF(3),
		RM_MESH		= BITF(4),
		RM_RAGDOLL	= BITF(5),
		RM_WHEEL		= BITF(6),
		RM_PIVOT		= BITF(7),
		RM_CENTEROFMASS = BITF(8),
		RM_ALL		= RM_CONVEX|RM_BOX|RM_CAPSULE|RM_SPHERE|RM_MESH|RM_RAGDOLL|RM_PIVOT|RM_CENTEROFMASS|RM_WHEEL
	};

/*!
For debug purpose. Is visualizes geometry of rigid body
\param pc PhysicsController to be visualized
\param mask What primitives are visualized
\param highlightGeom Geometry of this name will be highlighted
*/
	virtual void RenderEntity(PhysicsController* pc, uint mask = RM_ALL, const char* highlightGeom = NULL)=0;

/*!
Attaches world to physic world
\param world
The world to be attached
\param loadsave
When called from load game (TODO: what purpose?)
\param bounds
Default bounds of the world
*/
	virtual bool InitWorld(Landscape* world, bool loadsave, const AAB& bounds)=0;

	virtual bool InitTerrain(Scenery* terr)=0;//Terrain* terr)=0;

/*!
Called upon begin of play. It gives time to physics to put
rigid bodies to rest.
*/
	virtual void WarmUp(float simTime = 5.0f)=0;

//!Sets gravity acceleration in m/s^2
	virtual void SetGravity(const Vector3& direction)=0;
	virtual Vector3 GetGravity() const=0;
	virtual void SetInteractionLayer(uint mask1, uint mask2, bool enable)=0;

	PhysicsWorld(): m_iLock(0)	{}
	virtual ~PhysicsWorld() {}
	ENF_DECLARE_MMOBJECT(PhysicsWorld);
};

const uint ControllerID					= 3000;

enum PhysicsControllerTypes
{
	PhysicsControllerTypeID = ControllerID + 1,
	BulletControllerTypeID,
	BulletDynamicControllerTypeID,
	BulletDynamicControllerNCTypeID,
	BulletVehicleControllerTypeID,
	BulletRagdollControllerTypeID,
	BulletBoneControllerTypeID,
};

enum ActiveState
{
	AS_INACTIVE = 0,
	AS_ACTIVE = 1,
	AS_ALWAYS_ACTIVE = 2
};

//-----------------------------------------------------------------------
class AnimatorInterface
{
public:
	virtual ~AnimatorInterface() {}
	virtual Matrix43 Animate(uint nodeIndex, const Matrix43& transform) const=0;
};

//-----------------------------------------------------------------------
/*!
Wrapper over physics simulation of entity. Created by PhysicsWorld
associated with BaseWorld instance
*/
class PUBLIC_API PhysicsController: public BaseItem
{
protected:
	friend class Object;

	PhysicsController()
	{
	}

	virtual ~PhysicsController();

public:
	ENF_DECLARE_SMALLOBJECT(PhysicsController);
	ENF_DECLARE_ITEM(PhysicsController, BaseItem);

	virtual void Serialize(SerializeOutputAdapter* adapter);
	virtual void Serialize(SerializeInputAdapter* adapter);

	//!Returns pointer to Object we are attached to
	virtual Object* GetOwner() const=0;

/** \name Physics simulation related
*/
//@{
	//!Returns linear velocity
	virtual Vector3 GetVelocity() const
	{
		return Vector3().Zero();
	}

	//!Sets linear velocity
	virtual void SetVelocity(const Vector3& vel) {}

	//!Adds linear velocity
	virtual void AddVelocity(const Vector3& vel) {}
	virtual Vector3 GetVelocityAt(const Vector3& pos) const { return Vector3().Zero();}
	virtual void ApplyImpulseAt(const Vector3& pos, const Vector3& impulse) {}
	virtual void ApplyImpulse(const Vector3& impulse, const bool activate = true) {}
	virtual void ApplyForceAt(const Vector3& pos, const Vector3& force) {}
	virtual void ApplyTorque(const Vector3& torque) {}
	virtual void ApplyForce(const Vector3& force) {}
	virtual void SetAngularVelocity(const Vector3& vel) {}
	virtual Vector3 GetAngularVelocity() const { return Vector3().Zero(); }
	
	virtual void SetMass(float mass) {}
	virtual float GetMass() const { return 0; }

	virtual void SetInertiaTensor(const Matrix33& mat) {}
	virtual void SetInertiaTensor(const Vector3& vec) {}
	virtual void ComputeInertiaTensor() {}

	virtual bool IsActive() const { return true; }
	virtual bool IsDynamic() const { return false; }
	virtual bool EnableGravity(bool enableGravity) { return true; }
	virtual bool SetActive(ActiveState active) { return true; }
	virtual bool SetDynamic(bool dynamic) { return false; }
	virtual bool SetCollisionResponse(bool enableResponse) { return true; }
	//!Solid objects create contact upon collision. Non-solid don't and they just do callback
	virtual bool SetSolid(bool enableSolid) { return true; }
	virtual bool SetLinearFactor(const Vector3& linearFactor) { return false; }
	virtual bool SetSleepingTreshold(float linearTreshold, float angularTreshold) { return false; }
	virtual void SetDamping(float linearDamping, float angularDamping) {}

//@}

	virtual bool AddGeoms(PhysicsGeomDef* geoms, uint numGeoms, uint interactionLayers = 0xffffffff) { return false; };
	virtual uint GetNumElements() const { return 0; }
	virtual void SetElementTransform(uint element, const Matrix43& mat) {}
	virtual uint FindElement(CStr elementName) const { return INDEX_NOT_FOUND; }

	virtual AAB GetBoundBox() const { return AAB(Vector3(0,0,0));}
	virtual void ForceUpdateFrame(const Matrix43& frame){}
	virtual void ForceUpdateOrientation(const Matrix33& orientation){}
	virtual void SetMatrix(const Matrix43& mat){}
	virtual Matrix43 GetWorldTransform() const=0;

/*!
Synchronizes entity flags and event masks with rigid body representation.
Called by Object when any flags are changed.
*/
	virtual void UpdateEntityFlags(Object* ent, uint oldFlags, uint newFlags) {}
	virtual void UpdateEntityEvents(Object* ent, uint oldEvents, uint newEvents) {}
	virtual void Synchronize() {}
	virtual void SetInteractionLayer(uint mask, int geom = -1) {}
	virtual uint GetInteractionLayer(int geom = -1) const { return 0xffffffff;}
	virtual bool EnableCCD(bool enable) { return false; }
  virtual float GetCCDRadius(){ return 0; }
	virtual bool Intersects(const Vector3& start, const Vector3& end, uint interactionLayers = 0xffffffff) { return true; }
	virtual bool TraceMove(BaseTrace* trace) { return true; }
	virtual void AnimateElements(AnimatorInterface* mi) {}
	virtual ObjectGeometry* GetObjectGeometry() const { return NULL; }
	virtual const Vector3& GetCenterOfMass() const;
};

//-----------------------------------------------------------------------
class PUBLIC_API PhysicsManager: public Module
{
public:
	ENF_DECLARE_ITEM(PhysicsManager, Item);
	/*!
	create physics
	\param type
	*/
	virtual PhysicsWorld* CreatePhysicsWorld(CStr type)=0;
	//virtual ObjectGeometry* LoadGeometry(CStr name, IFFInput* iff, const char** stringPool, const Vector3& centerOfMass)=0;

	//!old implementation. Remove!
	//virtual ObjectGeometry* GetGeometry(CStr name, LODShape* obj)=0;
	virtual PhysicsMaterial* FindMaterial(const char* name) const=0;

	virtual uint GetAllocatedBlocks() const { return 0; }
	virtual size_t GetAllocation() const { return 0; }
	virtual bool LoadRagdolls(const char *defname)=0;
	virtual bool LoadMaterials(const char* name)=0;
	
	PhysicsManager();
	~PhysicsManager();
};

//-----------------------------------------------------------------------
class PUBLIC_API PhysicsJoint
{
protected:
	void*	ptr;
public:
	PhysicsJoint(): ptr(NULL) {}
	PhysicsJoint(void* p): ptr(p) {}
	~PhysicsJoint() { ptr = NULL; }
	bool IsNull() const { return ptr == NULL; }
};

//@}

}
#endif //ENF_WORLDPHYSICS_H
