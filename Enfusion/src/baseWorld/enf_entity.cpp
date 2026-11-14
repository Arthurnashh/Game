//-----------------------------------------------------------------------------
// File: enf_entity.cpp
//
// Desc: Entity class
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "common/enf_types.h"
#include "baseworld/enf_entity.h"
//#include "baseworld/enf_baseworld.h"
#include "system/enf_profiler.h"
#include "script/enf_scriptapi.h"
//#include "script/enf_scriptthread.h"
#include "Physics//enf_worldphysics.h"
#include "baseWorld/enf_shapes.h"
#include "resource/enf_vobject.h"
#include "common/enf_serialize.h"
//#include "BaseWorld/enf_entitydata.h"
#include "common/enf_diag.h"

namespace enf
{
#ifdef ENF_DONE
//-----------------------------------------------------------------------
bool EntityID::operator==(const EntityID& id) const
{
	return (ptr->GetID() == id.ptr->GetID());
}

//-----------------------------------------------------------------------
bool EntityID::operator>(const EntityID& id) const
{
	return (ptr->GetID() > id.ptr->GetID());
}

//-----------------------------------------------------------------------
bool EntityID::operator<(const EntityID& id) const
{
	return (ptr->GetID() < id.ptr->GetID());
}

#endif


#ifdef ENF_DEVELOPER
void ReportEntityOutOfBounds(Entity* ent, const Vector3& org)
{
	SString<> temp;
	const char* objectName = ent->GetInstance() ? ent->GetInstance()->GetVObject()->GetName().cstr() : "";
	const char* parentName = ent->GetParent() ? ent->GetParent()->GetTypeName() : "";

	temp.format("Entity out of world bounds. Type '%s', Name '%s', Parent '%s', Object '%s' Coords %f %f %f",
		ent->GetTypeName(),
		ent->GetName(),
		parentName,
		objectName,
		org.x, org.y, org.z);

	enf_rlsassert2(false, temp.cstr());
}
#endif

//-----------------------------------------------------------------------
void Entity::ISetMatrix(const Matrix43& mat)
{
	enf_assert(!BadMatrix43(mat));

#ifdef ENF_DEVELOPER
	if(mat._41 < (float)BaseWorld::MIN_SIZE || mat._41 >= (float)BaseWorld::MAX_SIZE ||
		mat._42 < (float)BaseWorld::MIN_SIZE || mat._42 >= (float)BaseWorld::MAX_SIZE ||
		mat._43 < (float)BaseWorld::MIN_SIZE || mat._43 >= (float)BaseWorld::MAX_SIZE)
	{
		ReportEntityOutOfBounds(this, mat.GetTranslation());
	}
	m_Matrix = mat;
#else
	float x = enf_limit(mat._41, (float)BaseWorld::MIN_SIZE, (float)BaseWorld::MAX_SIZE);
	float y = enf_limit(mat._42, (float)BaseWorld::MIN_SIZE, (float)BaseWorld::MAX_SIZE);
	float z = enf_limit(mat._43, (float)BaseWorld::MIN_SIZE, (float)BaseWorld::MAX_SIZE);
	m_Matrix.SetOrientation(mat.GetOrientation());
	m_Matrix.SetTranslation(Vector3(x, y, z));
#endif

	m_iFlags = (m_iFlags & ~TFL_UNNORMALIZED) | (mat.GetOrientation().IsNormalized() ? 0 : TFL_UNNORMALIZED);

}

//-----------------------------------------------------------------------
Entity* Entity::FromEntityChain(const CLink* l)
{
	return STRUCT_FROM_CLINK(l, Entity, m_liChain);
}

//-----------------------------------------------------------------------
const char* Entity::GetTypeName() const
{
	return GetClass() ? GetClass()->GetName() : NULL;
}

//-----------------------------------------------------------------------
const char* Entity::GetName() const
{
	if(GetClass())
	{
		const IVariable* name = GetIVariable("_name");
		if(name)
			return name->String;
	}
	return NULL;
}

//-----------------------------------------------------------------------
void Entity::SetID(uint id)
{
	if(m_pWorld)
	{
		if(m_iID)
			m_pWorld->m_EntitiesByID.Remove(EntityID(this));

		m_iID = id;
		if(m_iID)
		{
			m_pWorld->m_EntitiesByID.Insert(EntityID(this));
		}
		return;
	}

	m_iID = id;
}

//-----------------------------------------------------------------------
Entity::~Entity()
{
	//IL_EXCHANGE_ADD(MemStats::m_iLevelData, -GetClass()->GetSizeof());
	//enf_assert(!BaseWorld::Current()->m_NamedEntities.Contains(this));
	//Entity* ent = this;
	//ent=ent;
}

//-----------------------------------------------------------------------
void* Entity::operator new(size_t size, char const * file, int line)
{
	enf_assert(g_pScript);
	return g_pScript->AllocMem(size, NULL, __FILE__, __LINE__);
}

void* Entity::operator new[](size_t size, char const * file, int line)
{
	enf_assert(g_pScript);
	return g_pScript->AllocMem(size, NULL, __FILE__, __LINE__);
}

void* Entity::operator new(size_t size)
{
	enf_assert(g_pScript);
	return g_pScript->AllocMem(size, NULL, __FILE__, __LINE__);
}

void* Entity::operator new[](size_t size)
{
	enf_assert(g_pScript);
	return g_pScript->AllocMem(size, NULL, __FILE__, __LINE__);
}

void Entity::operator delete(void* ptr, char const * file, int line)
{
	enf_assert(g_pScript);
	g_pScript->FreeMem(ptr);
}

void Entity::operator delete[](void* ptr, char const * file, int line)
{
	enf_assert(g_pScript);
	g_pScript->FreeMem(ptr);
}

void Entity::operator delete(void* ptr)
{
	enf_assert(g_pScript);
	g_pScript->FreeMem(ptr);
}

void Entity::operator delete[](void* ptr)
{
	enf_assert(g_pScript);
	g_pScript->FreeMem(ptr);
}

//-----------------------------------------------------------------------
void Entity::Serialize(SerializeInputAdapter* adapter)
{
	enf_assert(adapter);
	Instance::Serialize(adapter);

	m_iFlags = adapter->ReadUInt32();
	if(adapter->ReadBool())
	{
		m_pHierarchy = ENF_NEW EntityHierarchy();
		m_pHierarchy->m_pParent = (Entity*)adapter->ReadReference(ENF_MAKEFOURCC('I', 'N', 'S', 'T' ));
		m_pHierarchy->m_iPivot = adapter->ReadUInt32();
		m_pHierarchy->m_pSibling = (Entity*)adapter->ReadReference(ENF_MAKEFOURCC('I', 'N', 'S', 'T' ));
		m_pHierarchy->m_pChildren = (Entity*)adapter->ReadReference(ENF_MAKEFOURCC('I', 'N', 'S', 'T' ));
	}
	m_BBox = adapter->ReadAAB();

	if(BadVec(m_BBox.GetMins()) || BadVec(m_BBox.GetMaxs()))
	{
		m_BBox.Zero();
	}

	m_Matrix = adapter->ReadMatrix43();
	m_iEventMask = adapter->ReadUInt32();

	DirtyFlags(TFL_UPDATE_MDL|TFL_UPDATE_LINK);
}

//-----------------------------------------------------------------------
void Entity::Serialize(SerializeOutputAdapter* adapter)
{
	enf_assert(adapter);
	Instance::Serialize(adapter);

	adapter->WriteUInt32(m_iFlags);
	adapter->WriteBool(m_pHierarchy != NULL);
	if(m_pHierarchy)
	{
		adapter->WriteReference(ENF_MAKEFOURCC('I', 'N', 'S', 'T' ), m_pHierarchy->m_pParent);
		adapter->WriteUInt32(m_pHierarchy->m_iPivot);
		adapter->WriteReference(ENF_MAKEFOURCC('I', 'N', 'S', 'T' ), m_pHierarchy->m_pSibling);
		adapter->WriteReference(ENF_MAKEFOURCC('I', 'N', 'S', 'T' ), m_pHierarchy->m_pChildren);
	}
	adapter->WriteAAB(m_BBox);
	adapter->WriteMatrix43(m_Matrix);
	adapter->WriteUInt32(m_iEventMask);
}

//-----------------------------------------------------------------------
void VisualizeEntity(const Entity* ent, bool active)
{
#ifdef ENF_DEVELOPER
	enf_assert(ent);

	if(ent->GetInstance() == NULL || !ent->GetInstance()->IsVisibleInCamera(0))
		return;

	int mint = 0;

	switch(ent->GetInstance()->GetVObject()->GetTypeID())
	{
	case MeshObjectTypeID:
		mint = 1;
		break;

	case ParticleEffectTypeID:
		mint = 2;
		break;

	case SpriteTypeID:
		mint = 3;
		break;
	}

	if(ENF_DIAG_VALUE(DM_SHOW_AABB) == mint)
	{
		Shape::CreateBox(ent->GetBBox().GetMins(), ent->GetBBox().GetMaxs(),
			active ? 0x1f4f0000 : 0x1f004f00,
			SS_NOZWRITE|SS_DOUBLESIDE|SS_TRANSP|SS_ONCE);
	}
#endif
}

//-----------------------------------------------------------------------
VObject* Entity::GetVObject() const
{
	if(m_pObjectInstance == NULL)
		return NULL;

	return m_pObjectInstance->GetVObject();
}

//-----------------------------------------------------------------------
VObjectInstance* Entity::SetObject(VObject* obj, const char* options)
{
	if(GetWorld())
	{
		GetWorld()->GetMutex().Lock();
		enf_assert2( m_pObjectInstance == NULL || !GetWorld()->IsRendering(), "Trying to call Entity::SetObject during Entity's world rendering");
	}

	SAFE_DESTROY(m_pObjectInstance);
	if(obj)
	{
		enf_assert2(!obj->IsLoading(), "Trying to create instance of not-loaded object");

		m_pObjectInstance = obj->AllocInstance(this, options);
		enf_assert(m_pObjectInstance);

		m_pObjectInstance->UpdateAnim(this, 0.0f, 0);
		m_pObjectInstance->Update(this);
	}
	if(IsFlagSet(TFL_STATIC))
		GetWorld()->Unlink(this, false);

	DirtyFlags(TFL_UPDATE_LINK|TFL_UPDATE_BOUNDS);
	UpdateBBox();
	if(GetWorld())
		GetWorld()->GetMutex().Unlock();

	return m_pObjectInstance;
}

//-----------------------------------------------------------------------
void Entity::SetPhysicsController(PhysicsController* pc)
{
	enf_assert2(pc == NULL || GetWorld() != NULL, "Entity with physics must be insert into world!");
	if(m_pPhysics)
		delete m_pPhysics;
	m_pPhysics = pc;
}

//-----------------------------------------------------------------------
ebool Entity::IsShadowCaster() const
{
	return m_pObjectInstance && m_pObjectInstance->IsShadowCaster();
}

//-----------------------------------------------------------------------
AAB Entity::ComputeBoundBox() const
{
	AAB newbox;
	if(m_pObjectInstance == NULL)
	{
		//it might be non visible entity with physics geometry.
		const PhysicsController* pc = GetPhysics();
		if(pc)
		{
			newbox = pc->GetBoundBox();
		}
		else
		{
			//at least, include origin
			newbox.Init(m_Matrix[3]);
		}
	}
	else
	{
		AAB newbox2 = m_pObjectInstance->ComputeWorldBoundBox(m_Matrix);
		newbox.InitFromBox(Vector3().Zero(), newbox2, 0.1f);
		//check degenerated bbox
		enf_assert(newbox.IsValid());
	}
	return newbox;
}

//-----------------------------------------------------------------------
AAB Entity::GetUserBoundBox() const
{
	return ComputeBoundBox();
}

//-----------------------------------------------------------------------
bool Entity::UpdateBBox()
{
	AAB newbox = IsFlagSet(TFL_USER_BOUNDBOX) ? GetUserBoundBox() : ComputeBoundBox();

	if(m_pObjectInstance != NULL)
	{
		if(ENF_DIAG_VALUE(DM_SHOW_AABB))
			VisualizeEntity(this, true);
	}
	
	ebool inside = ((m_iFlags & TFL_UPDATE_BOUNDS) != 0) ? false : m_BBox.Inside(newbox);

	if(IsFlagSet(TFL_STATIC))
	{
		m_BBox = newbox;
		return !inside;
	}

	if(!inside)
	{
		m_BBox = newbox.GetEnlargeQuantized(32.0f);
	}
	return !inside;
}

//-----------------------------------------------------------------------
void Entity::DirtyFlags(uint fl)
{
	Entity *ent = this;

	ent->m_iFlags |= fl | TFL_UPDATE;

	//mark all parents that we need to be updated!
	for(ent = ent->GetParent(); ent && !(ent->m_iFlags & TFL_UPDATE); ent = ent->GetParent())
		ent->m_iFlags |= TFL_UPDATE;
}

//-----------------------------------------------------------------------
ebool Entity::RemoveChild(Entity* child, bool keeptransform)
{
	enf_assert(child);
	enf_assert(!IsFlagSet(TFL_REMOVE));

	if(m_pHierarchy == NULL)
		return false;

	Entity* ent;
	for(Entity** prev = &m_pHierarchy->m_pChildren; *prev; prev = &(ent->m_pHierarchy->m_pSibling))
	{
		ent = *prev;
		if(child != ent)
			continue;

		Matrix43 mat = child->m_pHierarchy->m_Matrix;

		*prev = ent->m_pHierarchy->m_pSibling;

		child->m_pHierarchy->m_pParent = NULL;
		if(child->m_pHierarchy->m_pChildren == NULL)
			SAFE_DELETE(child->m_pHierarchy);

		if(!keeptransform)
		{
			//must be here, so we mark dirty all children
			child->SetMatrix(mat);
		}

		if(m_pHierarchy->m_pChildren == NULL && m_pHierarchy->m_pParent == NULL)
			SAFE_DELETE(m_pHierarchy);

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------
ebool Entity::AddChild(Entity *child, uint pivot)
{
	enf_assert(child);
	enf_assert(child->GetWorld() != NULL);
	enf_assert(!IsFlagSet(TFL_REMOVE));

	//avoid cyclic link!
	if(child == this)
		return false;

	Entity* ent;
	Entity** prev;
	
	if(m_pHierarchy == NULL)
	{
		m_pHierarchy = ENF_NEW EntityHierarchy();
		prev = &m_pHierarchy->m_pChildren;
	}
	else
	{
		for(prev = &m_pHierarchy->m_pChildren; *prev; prev = &(ent->m_pHierarchy->m_pSibling))
		{
			ent = *prev;

			//avoid duplicities
			if(ent == child)
				return false;
		}
	}

	//unparent when we are already child
	if(child->GetParent())
		child->GetParent()->RemoveChild(child);

	if(child->m_pHierarchy == NULL)
		child->m_pHierarchy = ENF_NEW EntityHierarchy();

	*prev = child;
	child->m_pHierarchy->m_iPivot = pivot;
	child->m_pHierarchy->m_pParent = this;

	//get current matrix and set it again, it will respect new parent
	Matrix43 mat = child->IGetMatrix();
	child->SetMatrix(mat);
	return true;
}

//-----------------------------------------------------------------------
uint Entity::SetFlags(uint flags, ebool recursively)
{
	//force PVS update when areas changed
	if(m_pWorld && flags & TFL_AREACLOSED && !(m_iFlags & TFL_AREACLOSED))
		m_pWorld->PVSChanged();

	const uint mask = TFL_VISIBLE|TFL_SOLID|TFL_TRIGGER|TFL_USER_BOUNDBOX;
	uint newflags = m_iFlags | flags;

	//relinkuj, pokud se zviditelni/zesolidni/zmeni charakter
	if((newflags & mask) != (m_iFlags & mask))
		DirtyFlags(TFL_UPDATE_LINK);

	if(m_pWorld && (newflags & TFL_ACTIVE) != 0 && (m_iFlags & TFL_ACTIVE) == 0)
	{
		m_pWorld->m_ActiveEntities.Insert(this);
	}

	if((newflags & TFL_STATIC) != (m_iFlags & TFL_STATIC))
	{
		GetWorld()->Unlink(this, true);
		DirtyFlags(TFL_UPDATE_LINK);
	}

	if(GetInstance() && (newflags & TFL_VISIBLE) != (m_iFlags & TFL_VISIBLE))
	{
		GetInstance()->SetVisible((newflags & TFL_VISIBLE) != 0);
	}

	m_iFlags |= flags;

	if(recursively)
	{
		for(Entity* ent = GetChildren(); ent; ent = ent->GetSibling())
		{
			ent->SetFlags(flags, true);
		}
	}
	return m_iFlags;
}

//-----------------------------------------------------------------------
uint Entity::ClearFlags(uint flags, ebool recursively)
{
	//force PVS update when areas changed
	if(m_pWorld && (flags & TFL_AREACLOSED) && (m_iFlags & TFL_AREACLOSED))
		m_pWorld->PVSChanged();

	const uint mask = TFL_VISIBLE|TFL_SOLID|TFL_TRIGGER|TFL_USER_BOUNDBOX;

	uint newflags = m_iFlags & ~flags;

	//relinkuj, pokud se zviditelni/zesolidni/zmeni charakter
	if((newflags & mask) != (m_iFlags & mask))
		DirtyFlags(TFL_UPDATE_LINK);

	if((newflags & TFL_ACTIVE) == 0 && (m_iFlags & TFL_ACTIVE) != 0)
	{
		m_pWorld->m_ActiveEntities.Remove(this);
	}

	if((newflags & TFL_STATIC) != (m_iFlags & TFL_STATIC))
	{
		GetWorld()->Unlink(this, true);
		DirtyFlags(TFL_UPDATE_LINK);
	}

	uint res = m_iFlags & flags;

	m_iFlags &= ~flags;

	if(recursively)
	{
		for(Entity* ent = GetChildren(); ent; ent = ent->GetSibling())
		{
			ent->ClearFlags(flags, true);
		}
	}

	return res;
}

//-----------------------------------------------------------------------
void Entity::RemoveFromHierarchy()
{
	if(m_pHierarchy)
	{
		Entity* sibling;

		for(Entity* child = m_pHierarchy->m_pChildren; child; child = sibling)
		{
			sibling = child->m_pHierarchy->m_pSibling;
			if(child->m_pHierarchy->m_pChildren == NULL)
			{
				SAFE_DELETE(child->m_pHierarchy);
			}
			else
			{
				child->m_pHierarchy->m_pParent = NULL;
				child->m_pHierarchy->m_pSibling = NULL;
			}
			child->DirtyFlags();
		}
		m_pHierarchy->m_pChildren = NULL;

		if(m_pHierarchy->m_pParent)
		{
			m_pHierarchy->m_pParent->RemoveChild(this);
		}
		SAFE_DELETE(m_pHierarchy);
	}
}

//-----------------------------------------------------------------------
void Entity::KillThreads()
{
	//kill associated threads
	while(m_psThread)
		m_psThread->Kill();
}

//-----------------------------------------------------------------------
void Entity::PrepareForDelete()
{
	enf_assert((m_iFlags & TFL_REMOVE) == 0);

	RemoveFromHierarchy();
	KillThreads();

	if(m_pWorld)
	{
		m_pWorld->Unlink(this, true);
		if(m_iID)
		{
			m_pWorld->m_EntitiesByID.Remove(EntityID(this));
		}
		m_pWorld->RemovedEntities.Insert(this);
		if(m_pWorld->m_peWorldEntity == this)
			m_pWorld->m_peWorldEntity = NULL;
	}

	m_iFlags = TFL_REMOVE;	//soft way to remove entity on the fly
	ClearEventMask(EV_ALL);	//disable all exceptions

	SAFE_DESTROY(m_pObjectInstance);
	SAFE_DELETE(m_pPhysics);

	if(m_pWorld == NULL)
	{
		g_pScript->FreeInstance(this);
	}
}

//-----------------------------------------------------------------------
uint Entity::r_Update(const Matrix43* pvp)
{
	uint updated = 0;


	if(pvp)
	{
		ISetMatrix((*pvp) * m_pHierarchy->m_Matrix);
	}

	updated |= TFL_UPDATE;

	//TODO: Hack! Is it correct?
	if(m_pWorld && pvp)
	{
		PhysicsController* pc = GetPhysics();
		if(pc)
		{
			pc->ForceUpdateFrame(IGetMatrix());
			pc->SetActive(enf::AS_INACTIVE);
			if(pc->IsDynamic())
			{
				pc->SetAngularVelocity(Vector3(0.0f));
				pc->SetVelocity(Vector3(0.0f));
			}
		}
	}

	VObjectInstance* inst = m_pObjectInstance;

	if(m_iFlags & TFL_UPDATE_MDL)
	{
		updated |= TFL_UPDATE_MDL;

		if(inst)
			inst->Update(this);
	}

	ebool changed = UpdateBBox();

	//invalidate light interactions
	if(changed && inst)
	{
		m_pObjectInstance->RemoveAllInteractions(true);
	}

	//relink is performed always when new bound box is not completely inside of previous one
	if(m_pWorld && (changed || m_iFlags & TFL_UPDATE_LINK))
	{
		m_pWorld->Unlink(this, false);
		
		//must be either visible, or solid, or trigger
		if(m_iFlags & (TFL_VISIBLE | TFL_SOLID | TFL_TRIGGER))
		{
			if(m_pWorld->GetWorldEntity() != this && (GetInstance() == NULL || GetInstance()->GetTypeID() != TerrainInstanceTypeID))
			{
				m_pWorld->Link(this, m_BBox);
			}
		}
	}

	m_iFlags &= ~(TFL_UPDATE_MDL|TFL_UPDATE_LINK|TFL_UPDATE|TFL_UPDATE_BOUNDS);

	for(Entity* child = GetChildren(); child; child = child->GetSibling())
	{
		Matrix43 tmp;
	
		if(inst == NULL || !inst->GetVObject()->MultiplyPivot(this, child->m_pHierarchy->m_iPivot, tmp))
			updated |= child->r_Update(&m_Matrix);
		else
			updated |= child->r_Update(&tmp);
	}
	return updated;
}


//-----------------------------------------------------------------------
uint Entity::Update()
{
	if(m_iFlags & TFL_REMOVE)
		return 0;

	uint flags = 0;

	if(m_iFlags & TFL_UPDATE)
	{
		ENF_BEGIN_PROFILE(PROFILE_ENTITY_UPDATE);
		
		//we have to go from the top
		Entity* root = this;
		while(root->GetParent())
			root = root->GetParent();

		flags = root->r_Update(NULL);

		ENF_END_PROFILE();
	}
	else
	{
		if(ENF_DIAG_VALUE(DM_SHOW_AABB))
			VisualizeEntity(this, false);
	}

	return flags;
}

//-----------------------------------------------------------------------
ebool Entity::IsHierarchy(const Entity *ent) const
{
	enf_assert(ent);
	if(m_pHierarchy == NULL || ent->m_pHierarchy == NULL)
		return false;

	//entities from the same hierarchy has the same root
	while(ent->GetParent())
		ent = ent->GetParent();

	const Entity* tent;
	for(tent = this; tent->GetParent(); tent = tent->GetParent());

	return (tent == ent);
}

//-----------------------------------------------------------------------
ebool Entity::SetMatrix(const Matrix43& mat)
{
	enf_assert(!IsFlagSet(TFL_REMOVE));

	const Entity* parent = GetParent();
	if(parent)
	{
		if(m_pHierarchy->m_Matrix == mat)
			return false;

		m_pHierarchy->m_Matrix = mat;

		//TODO: must not be correct, when there are not-updated parents
		VObjectInstance* inst = parent->m_pObjectInstance;
		Matrix43 tmp;
		if(inst == NULL || !inst->GetVObject()->MultiplyPivot(parent, m_pHierarchy->m_iPivot, tmp))
			ISetMatrix(parent->IGetMatrix() * mat);
		else
			ISetMatrix(tmp * mat);
	}
	else
	{
		if(m_Matrix == mat)
			return false;

		ISetMatrix(mat);
	}

	PhysicsController* pc = GetPhysics();
	if(pc != NULL)
	{
		pc->ForceUpdateFrame(IGetMatrix());
	}
	DirtyFlags();
	return true;
}

//-----------------------------------------------------------------------
void Entity::ISetOrigin(const Vector3& org)
{
	enf_assert(!BadVec(org));

	if(GetParent())
	{
		m_pHierarchy->m_Matrix.SetTranslation(org);
	}
	else
	{
		m_Matrix.SetTranslation(org);
	}
}

//-----------------------------------------------------------------------
ebool Entity::SetOrigin(const Vector3& org)
{
	enf_assert(!BadVec(org));

	const Entity* parent = GetParent();
	if(parent)
	{
		if(m_pHierarchy->m_Matrix.GetTranslation() == org)
			return false;

		m_pHierarchy->m_Matrix.SetTranslation(org);

		//TODO: must not be correct, when there are not-updated parents
		VObjectInstance* inst = parent->m_pObjectInstance;
		Matrix43 tmp;
		if(inst == NULL || !inst->GetVObject()->MultiplyPivot(parent, m_pHierarchy->m_iPivot, tmp))
			ISetMatrix(parent->IGetMatrix() * m_pHierarchy->m_Matrix);
		else
			ISetMatrix(tmp * m_pHierarchy->m_Matrix);
	}
	else
	{
		if(m_Matrix.GetTranslation() == org)
			return false;

#ifdef ENF_DEVELOPER
		if(org.x < (float)BaseWorld::MIN_SIZE || org.x >= (float)BaseWorld::MAX_SIZE ||
			org.y < (float)BaseWorld::MIN_SIZE || org.y >= (float)BaseWorld::MAX_SIZE ||
			org.z < (float)BaseWorld::MIN_SIZE || org.z >= (float)BaseWorld::MAX_SIZE)
		{
			ReportEntityOutOfBounds(this, org);
		}

		m_Matrix.SetTranslation(org);
#else
		float x = enf_limit(org.x, (float)BaseWorld::MIN_SIZE, (float)BaseWorld::MAX_SIZE);
		float y = enf_limit(org.y, (float)BaseWorld::MIN_SIZE, (float)BaseWorld::MAX_SIZE);
		float z = enf_limit(org.z, (float)BaseWorld::MIN_SIZE, (float)BaseWorld::MAX_SIZE);
		m_Matrix.SetTranslation(Vector3(x, y, z));
#endif
	}

	PhysicsController* pc = GetPhysics();
	if(pc != NULL)
	{
		pc->ForceUpdateFrame(IGetMatrix());
	}

	DirtyFlags();
	return true;

}

//-----------------------------------------------------------------------
ebool Entity::SetAngles(const Vector3& angles)
{
	Matrix33 mat;
	mat.RollPitchYaw(angles);

	const Entity* parent = GetParent();
	if(parent)
	{
		if(m_pHierarchy->m_Matrix.GetOrientation() == mat)
			return false;
		m_pHierarchy->m_Matrix.SetOrientation(mat);
		
		//TODO: must not be correct, when there are not-updated parents
		VObjectInstance* inst = parent->m_pObjectInstance;
		Matrix43 tmp;
		if(inst == NULL || !inst->GetVObject()->MultiplyPivot(parent, m_pHierarchy->m_iPivot, tmp))
			ISetMatrix(parent->IGetMatrix() * m_pHierarchy->m_Matrix);
		else
			ISetMatrix(tmp * m_pHierarchy->m_Matrix);
	}
	else
	{
		if(m_Matrix.GetOrientation() == mat)
			return false;

		m_Matrix.SetOrientation(mat);
		ClearFlags(TFL_UNNORMALIZED);
	}

	PhysicsController* pc = GetPhysics();
	if(pc != NULL)
	{
//		pc->ForceUpdateOrientation(mat);
		pc->ForceUpdateFrame(IGetMatrix());
	}

	DirtyFlags();
	return true;
}

//-----------------------------------------------------------------------
Vector3 Entity::GetAngles() const
{
	return m_Matrix.ToAngles();
}

//-----------------------------------------------------------------------
Vector3 Entity::GetLocalAngles() const
{
	if(m_pHierarchy && m_pHierarchy->m_pParent)
		return m_pHierarchy->m_Matrix.ToAngles();
	else
		return m_Matrix.ToAngles();
}

//-----------------------------------------------------------------------
float Entity::GetRollAngle() const
{
	return m_Matrix.ToRoll();
}

//-----------------------------------------------------------------------
float Entity::GetPitchAngle() const
{
	return m_Matrix.ToPitch();
}

//-----------------------------------------------------------------------
float Entity::GetYawAngle() const
{
	return m_Matrix.ToYaw();
}

//-----------------------------------------------------------------------
bool Entity::GetTick(float& period, float& actualTime)
{
	if(m_pWorld && m_iEventMask & EV_TICK)
	{
		return m_pWorld->GetEntityTick(this, period, actualTime);
	}
	return false;
}

//-----------------------------------------------------------------------
void Entity::RemoveTick()
{
	if(m_pWorld && m_iEventMask & EV_TICK)
	{
		m_pWorld->RemoveEntityTick(this);
		m_iEventMask &= ~EV_TICK;
	}
}

//-----------------------------------------------------------------------
void Entity::SetTick(float period)
{
	if(m_pWorld)
	{
		m_iEventMask |= EV_TICK;
		m_pWorld->SetEntityTick(this, period, -1);
	}
}

//-----------------------------------------------------------------------
void Entity::SetTickEx(float period, float actualTime)
{
	if(m_pWorld)
	{
		m_iEventMask |= EV_TICK;
		m_pWorld->SetEntityTick(this, period, actualTime);
	}
}

//-----------------------------------------------------------------------
uint Entity::SetEventMask(uint mask)
{
	if(m_pWorld)
	{
		if((mask & EV_NOTVISIBLE) && !(m_iEventMask & EV_NOTVISIBLE))
		{
			//register these, we must notify them after render
			m_pWorld->m_EntitiesToNotify.Insert(this);
		}

		if((mask & EV_TICK) && !(m_iEventMask & EV_TICK))
		{
			m_pWorld->SetEntityTick(this, TICK_PERIOD);
		}
	}

	uint oldEventMask = m_iEventMask;
	m_iEventMask = oldEventMask | mask;

	PhysicsController* pc = GetPhysics();
	if(pc != NULL)
	{
		pc->UpdateEntityEvents(this, oldEventMask, oldEventMask | mask);
	}
	return m_iEventMask;
}

//-----------------------------------------------------------------------
uint Entity::ClearEventMask(uint mask)
{
	if(m_pWorld)
	{
		if((mask & EV_NOTVISIBLE) && (m_iEventMask & EV_NOTVISIBLE))
		{
			//unregister these
			m_pWorld->m_EntitiesToNotify.Remove(this);
		}

		if((mask & EV_TICK) && (m_iEventMask & EV_TICK))
		{
			m_pWorld->RemoveEntityTick(this);
		}
	}

	uint oldEvents = m_iEventMask;
	uint ret = oldEvents & mask;
	uint newEvents = oldEvents & ~mask;
	m_iEventMask = newEvents;

	if(m_pPhysics)
		m_pPhysics->UpdateEntityEvents(this, oldEvents, newEvents);

	return ret;
}

//-----------------------------------------------------------------------
void Entity::FilterNextTrace()
{
	if(m_pWorld)
		m_iTraceCount = m_pWorld->GetTraceCount() + 1;
}

//-----------------------------------------------------------------------
uint Entity::GetNumFramesSinceRendered() const
{
	if(m_pWorld)
	{
		uint currentFrame = m_pWorld->GetFrameCount();
		uint renderFrame = m_iRenderFrame;
		if(currentFrame >= renderFrame)
			return currentFrame - renderFrame;
		else
			return (0xffffffff - renderFrame) + currentFrame;
	}
	else
		return 0;
}

//-----------------------------------------------------------------------
int Entity::_ThrowEvent(Entity* actor, uint eventmask, void* extra)
{
	enf_assert(eventmask);

	if((m_iEventMask & eventmask) == 0)
		return 0;

	if(actor != NULL && (m_iFlags | actor->GetFlags()) & TFL_REMOVE)
		return 0;

	int ret = 0;
	switch(eventmask)
	{
	case EV_TOUCH:
		EOnTouch(actor, (int)extra);
		break;

	case EV_INIT:
		EOnInit(actor, (int)extra);
		break;

	case EV_EXTRA:
		EOnExtra(actor, (int)extra);
		break;

	case EV_NOTVISIBLE:
		EOnNotVisible(actor, (int)extra);
		break;

	case EV_VISIBLE:
		EOnVisible(actor, (int)extra);
		break;

	case EV_FRAME:
		EOnFrame(actor, (int)extra);
		break;

	case EV_POSTFRAME:
		EOnPostFrame(actor, (int)extra);
		break;

	case EV_WORLDPROCESS:
		EOnWorldProcess(actor, (int)extra);
		break;

	case EV_FLASHLIGHT:
		EOnFlashlight(actor, (int)extra);
		break;

	case EV_ANIMEND:
		EOnAnimEnd(actor, (int)extra);
		break;

	case EV_ANIMBLEND:
		EOnAnimBlend(actor, (int)extra);
		break;

	case EV_SOUNDEND:
		EOnSoundEnd(actor, (int)extra);
		break;

	case EV_TICK:
		EOnTick(actor, (int)extra);
		break;

	case EV_SIMULATE:
		EOnSimulate(actor, *(float*)&extra);
		break;

	case EV_POSTSIMULATE:
		EOnPostSimulate(actor, *(float*)&extra);
		break;

	case EV_JOINTBREAK:
		EOnJointBreak(actor, (int)extra);
		break;

	case EV_PHYSICSMOVE:
		EOnPhysicsMove(actor, (int)extra);
		break;

	case EV_CONTACT:
		EOnContact(actor, (const Contact*)extra);
		break;

	default:
		EOnEvent(actor, eventmask, (int)extra);
		break;
	}

	if(m_pWorld && m_pWorld->m_pEntityEventHandler != NULL)
		return m_pWorld->m_pEntityEventHandler->HandleEvent(EntityEvent(this, actor, eventmask, extra)) | ret;

	return 0;
}

//-----------------------------------------------------------------------
void Entity::InitScriptVariables(const EntitySource* src)
{
	if(!src || !src->GetNumKeys())
		return;

	//set all class variables, if some matches!
	const Class* cl = GetClass();

	//skip classes without script variables, there is not hope for match anyway
	if(!cl->GetNumVariables())
		return;

	for(uint key = 0; key < src->GetNumKeys(); key++)
	{
		const EntityDef::Key& kdef = src->GetKeyDef(key);
		uint vindex = cl->GetVariableIndex(kdef.Name.cstr());

		if(vindex != INDEX_NOT_FOUND)
		{
			VType vtype = cl->GetVariable(vindex)->GetVType();
			if(kdef.Type == vtype)
			{
				switch(vtype)
				{
				case VARTYPE_INT:
					GetIVariable(vindex)->Int = src->GetInt(kdef.Name);
					break;
				case VARTYPE_FLOAT:
					GetIVariable(vindex)->Float = src->GetFloat(kdef.Name);
					break;
				case VARTYPE_VECTOR:
					*(Vector3*)GetIVariable(vindex)->Pointer = src->GetVector(kdef.Name);
					break;
				case VARTYPE_STRING:
					g_pScript->CopyString(src->GetString(kdef.Name).cstr(), cl->GetVariable(vindex), &GetIVariable(vindex)->String);
					break;
				}
			}
			else
			{
				switch(vtype)
				{
				case VARTYPE_INT:
					enf_assert2(kdef.Type == VARTYPE_FLOAT, "Illegal conversion");
					GetIVariable(vindex)->Int = src->GetFloat(kdef.Name);
					break;
				case VARTYPE_FLOAT:
					enf_assert2(kdef.Type == VARTYPE_INT, "Illegal conversion");
					GetIVariable(vindex)->Float = src->GetInt(kdef.Name);
					break;
				default:
					enf_assert(!"Illegal conversion");
					break;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------
Texture* Entity::GetRTTexture(uint index) const
{
	if(m_pObjectInstance)
	{
		return m_pObjectInstance->GetRTTexture();
	}
	return NULL;
}

//-----------------------------------------------------------------------
void Entity::SetRTTexture(RTTextureWidget* rtwidget)
{
	if(m_pObjectInstance)
	{
		m_pObjectInstance->SetRTTexture(rtwidget);
	}
}

//-----------------------------------------------------------------------
Entity::Entity(const EntitySource* src):
	Instance(NULL),
	m_iFlags(TFL_UPDATE),
	m_iEventMask(0),
	m_iTraceCount(0),
	m_iRenderFrame(0),
	m_pObjectInstance(NULL),
	m_BBox(Vector3(0.0f)),
	m_Matrix(true),
	m_pWorld(NULL),
	m_pLink(NULL),
	m_iID(0),
	m_pHierarchy(NULL),
	m_pPhysics(NULL)
{
	if(src)
	{
		//init variables stored in template as first
		InitScriptVariables( src->GetTemplate() );

		//init variables stored in source (overwrite values from template possible)
		InitScriptVariables(src);
	}
}

//-----------------------------------------------------------------------
Entity::Entity():
	m_iFlags(TFL_UPDATE),
	m_iEventMask(0),
	m_iTraceCount(0),
	m_iRenderFrame(0),
	m_pObjectInstance(NULL),
	m_BBox(Vector3(0.0f)),
	m_Matrix(true),
	m_pWorld(NULL),
	m_pLink(NULL),
	m_iID(0),
	m_pHierarchy(NULL),
	m_pPhysics(NULL)
{
	//for entities created from C++ code. Just initialize
	Instance::Init(g_pScript->m_pEntityClass);
	//IL_EXCHANGE_ADD(MemStats::m_iLevelData, GetClass()->GetSizeof());
}


#ifdef ENF_WB
//-------------------------------------------------------------------------------
void Entity::WB_SetOrientation(const Vector3& pos, const Vector3& angles, const enf::EntitySource* src)
{
	Matrix43 mat;
	mat.RollPitchYaw(angles[ROLL], angles[PITCH], angles[YAW]);
	mat.SetTranslation(pos);
	SetMatrix(mat);

	Update();
}

//-------------------------------------------------------------------------------
void Entity::WB_MakeSolid(const enf::EntitySource* src, bool solid)
{
	if(solid)
	{
		SetFlags(TFL_SOLID);

//		if(GetInstance())
//			GetInstance()->SetFlags(VIF_TRACE_BONES | VIF_TRACE_MODEL);	//do not set this, because phys geometry is ignored then
	}
	else
	{
		ClearFlags(TFL_SOLID);

//		if(GetInstance())
//			GetInstance()->ClearFlags(VIF_TRACE_BONES | VIF_TRACE_MODEL);
	}
}

//-------------------------------------------------------------------------------
void Entity::WB_MakeVisible(const enf::EntitySource* src, bool visible)
{
	if(visible)
		SetFlags(TFL_VISIBLE);
	else
		ClearFlags(TFL_VISIBLE);
}

//-------------------------------------------------------------------------------
void Entity::WB_GetBoundBox(AAB& bbox, const enf::EntitySource* src)
{
	VObjectInstance* instance = GetInstance();

	if(instance)
		bbox = instance->ComputeLocalBoundBox(IGetMatrix(), enf::VIF_DYNAMICBBOX);
	else
		bbox.Zero();
}

//-------------------------------------------------------------------------------
void Entity::WB_GetCBoundBox(AAB& bbox, const enf::EntitySource* src)
{
	bbox = GetBBox();
}

//-------------------------------------------------------------------------------
bool Entity::WB_IsVisibleVolume(const enf::EntitySource* src)
{
	VObjectInstance* instance = GetInstance();

	if(!instance)
		return false;

	if(instance->HasType(ParticleEffectInstanceTypeID))
		return false;
	if(instance->GetVObject()->HasType(SpriteTypeID))
		return false;

	return IsFlagSet(TFL_VISIBLE);
}

//-------------------------------------------------------------------------------
void Entity::WB_SetExtraVisualiser(EntityVisualizerType type, const enf::EntitySource* src)
{
}

//-------------------------------------------------------------------------------
GameEditorAPI*	Entity::WB_GetEditorAPI(bool CheckForNull)
{
	if(!GetWorld())
		return NULL;

	return GetWorld()->GetEditorAPI();
}
#endif

}




