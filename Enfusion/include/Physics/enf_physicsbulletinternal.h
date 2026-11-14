//-----------------------------------------------------------------------------
// File: enf_physicsbulletinternal.h
//
// Desc: Bullet implementation
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_PHYSICSBULLETINTERNAL_H
#define ENF_PHYSICSBULLETINTERNAL_H

#include "enf_config.h"
//#include "common/enf_types.h"
//#include "common/enf_mathlib.h"
//#include "Physics/enf_physicsbullet.h"
//#include "enf_mutex.h"

//#include "El/Math/mathDefs.hpp"
#include "enf_physicsbullet.h"
#include "Es/Threads/pocritical.hpp"
#include "Common/enf_set.h"
#include "Common/enf_hashmap.h"
#include "Es/Containers/typeDefines.hpp"

#include <Es/Memory/normalNew.hpp>

class btCollisionShape;

namespace enf
{
class Shape;
class RagdollDef;

/** @addtogroup Bullet
@{*/
//---------------------------------------------------------------------------

struct BulletRBElement: public RBElement
{
	btCollisionShape*	m_pShape;

	BulletRBElement():
		m_pShape(NULL)
	{
	}
};

//---------------------------------------------------------------------------
const uint BulletObjectGeometryTypeID = ObjectGeometryTypeID + 1;

class BulletObjectGeometry: public ObjectGeometry
{
private:
	friend class BulletWorld;
	friend class BulletManager;

	enf::Array<BulletRBElement>		m_Elements;

	BulletObjectGeometry() {}
	~BulletObjectGeometry();
public:
	//ENF_DECLARE_ITEM(BulletObjectGeometry, ObjectGeometry);
	ENF_DECLARE_SMALLOBJECT(BulletObjectGeometry);
  BulletObjectGeometry(CStr name, const Vector3& com): ObjectGeometry(name, com)	{}

	virtual void SetupPhysicsGeomDefs(PhysicsGeomDef* geoms, uint numGeoms);

	virtual uint GetNumElements() const;
	
  enf::Array<BulletRBElement>& GetElements()
	{
		return m_Elements;
	}
	virtual const RBElement& GetElement(uint index) const
	{
		return m_Elements[index];
	}

};

//---------------------------------------------------------------------------
class PUBLIC_API BulletManager: public PhysicsManager
{
friend class BulletWorld;
friend class TerrainCollisionGeometry; //ADDED !!!!
friend class BulletObjectGeometry;
	//CMutex	m_Mutex;
  PoCriticalSection m_Mutex;

	enf::Array<RagdollDef*>	m_Ragdolls;
	
	HashMap<const char*, BulletObjectGeometry*, StringHashFunc> m_ObjectGeometriesHash;
  Set<BulletObjectGeometry*> m_ObjectGeometries;
  
	HashMap<const char*, RagdollDef*, StringHashFunc> m_RagdollsHash;
  
	enf::HashMap<const char*, BulletPhysicsMaterial*, StringHashFunc>	m_MaterialsHash;
  enf::Array<BulletPhysicsMaterial*>	m_Materials;
	BulletPhysicsMaterial*			m_BaseMaterials[MAX_BASE_MATERIALS];
	void UnloadRagdolls();
	void UnloadMaterials();
	btCollisionConfiguration*  m_pConfig;

	~BulletManager();

	Set< Pair<btCollisionShape*, Shape*> > m_ShapeCache;
  BulletPhysicsMaterial* CloneMaterialData(BulletPhysicsMaterial* cloned, const BulletPhysicsMaterial* mat);

friend void* BulletAllocFunc(size_t size, int line, const char* file);
friend void BulletFreeFunc(void *memblock);

	static size_t m_iAllocatedBlocks;
	static uint m_iAllocated;

public:
	Shape* GetCached(btCollisionShape* shape);
	void InsertCached(btCollisionShape* shape, Shape* sh);
  
	btCollisionConfiguration* GetCollisionConfiguration()
	{
		return m_pConfig;
	}
	void SetCurrent()
	{
		g_pBulletManager = this;
	}

	ENF_DECLARE_ITEM(BulletManager, PhysicsManager);

	BulletManager();

	PhysicsWorld* CreatePhysicsWorld(CStr type);
	const BulletPhysicsMaterial*const* GetBaseMaterials() const
	{
		return &m_BaseMaterials[0];
	}
	const BulletPhysicsMaterial* GetBaseMaterial(uint index) const
	{
		Assert(index < MAX_BASE_MATERIALS);
		return m_BaseMaterials[index];
	}

/** \name Materials
*/
//@{
	bool LoadMaterials(const char* name);
  bool LoadRagdollMaterial();

	void InsertMaterial(BulletPhysicsMaterial* mat)
	{
    m_MaterialsHash.Insert(mat->m_Name.cstr(), mat);
    m_Materials.Insert(mat);
	}

	BulletPhysicsMaterial* CloneMaterial(CStr dest, uint matindex, const BulletPhysicsMaterial* mat);

	BulletPhysicsMaterial* FindMaterial(const char* name) const;
//@}

/** \name Ragdolls
*/
//@{
  bool LoadCharRagdoll(); //docasny pomocny!
	bool LoadRagdolls(const char *defname);

	const Array<RagdollDef*>& GetRagdolls() const
	{
		return m_Ragdolls;
	}
//@}

/** \name Geometries
*/
//@{

	/*!
	Gets geometry (XOC file) and increments reference count
	\param name
	Name of geometry
	\param obj
	Object to which the geometry belongs
	\return
	Pointer to Geometry or NULL whe not found
	*/
	//ObjectGeometry* GetGeometry(CStr name, LODShape* obj);

	//ObjectGeometry* LoadGeometry(CStr name, IFFInput* iff, const char** stringPool, const Vector3& centerOfMass);

//@}

	uint GetAllocatedBlocks() const;
	size_t GetAllocation() const;
};


//@}
}

#endif //ENF_PHYSICSBULLETINTERNAL_H