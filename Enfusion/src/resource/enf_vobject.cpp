//-----------------------------------------------------------------------------
// File: enf_vobject.cpp
//
// Desc: Visual object superclass
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "enf_main.h"
#include "baseworld/enf_entity.h"
//#include "baseworld/enf_baseworld.h"
#include "baseworld/enf_baseworldlight.h"
//#include "baseworld/enf_decals.h"
#include "resource/enf_vobject.h"
#include "materials/enf_materialsystem.h"
#include "materials/enf_materialclass.h"
#include "pipeline/enf_pipeline.h"

#include "common/enf_serialize.h"
#include "resource/enf_resourcemanager.h"
#include "common/enf_streams.h"

#define ENF_CROSSFADE_TIME				300

namespace enf
{
uint VObject::m_iMinLoadedLOD = 0;



//-----------------------------------------------------------------------
uint VObjectSkinDef::AddMaterial(CStr name, uint creationflags)
{
	for(uint n = 0; n < Defs.GetCardinality(); n++)
	{
		if(Defs[n].CreationFlags == creationflags && name == Defs[n].Name)
			return n;
	}

	return Defs.Insert(MaterialDef(name.Static(), creationflags));
}

//-----------------------------------------------------------------------
ebool VObjectSkinDef::RemapMaterial(CStr matname, CStr newname)
{
	bool replaced = false;
	for(uint index = 0; index < GetNumMaterials(); index++)
	{
		if(matname.cmpi(GetMaterialName(index)) == 0)
		{
			Defs[index].Name = newname.Static();
			replaced = true;
		}
	}

	return replaced;
}

//-----------------------------------------------------------------------
VObjectSkin::~VObjectSkin()
{
	ReleaseMaterials();
	if(m_pObject)
	{
		m_pObject->RemoveSkin(this);
	}
}

//-----------------------------------------------------------------------
bool VObjectSkin::CreateMaterials(CStr ownerName)
{
	bool ok = true;
	enf_assert2(m_Materials.IsEmpty(), "Materials already created");

	m_Materials.SetSize(GetNumMaterials());
	ZeroMem(m_Materials.GetArray(), m_Materials.GetCardinality() * sizeof(Material*));
	for(uint m = 0; m < GetNumMaterials(); m++)
	{
		m_Materials[m] = GetMaterialSystem()->GetMaterial(GetMaterialName(m), GetMaterialFlags(m));

		if(m_Materials[m] == NULL)
		{
			ok = false;
			enf::LogF(LL_ERROR, "Object %s cannot create material %s", ownerName.cstr(), GetMaterialName(m).cstr());
		}
	}
	if(ok == false)
	{
		ReleaseMaterials();
	}
	return ok;
}

//-----------------------------------------------------------------------
uint VObjectSkin::Load()
{
	if(m_iLoadCount == 0)
	{
		bool ok = true;

		enf_assert(m_Materials.GetCardinality() == GetNumMaterials());

		for(uint m = 0; m < GetNumMaterials(); m++)
		{
			m_Materials[m]->Load();
		}
	}
	return ++m_iLoadCount;
}

//-----------------------------------------------------------------------
void VObjectSkin::ReleaseMaterials()
{
	for(uint n = 0; n < m_Materials.GetCardinality(); n++)
		SAFE_RELEASE(m_Materials[n]);
	m_Materials.Clear();
}

//-----------------------------------------------------------------------
uint VObjectSkin::Unload()
{
	enf_assert(m_iLoadCount > 0);

	if(--m_iLoadCount > 0)
		return m_iLoadCount;

	for(uint n = 0; n < m_Materials.GetCardinality(); n++)
	{
		Material* mat = m_Materials[n];

		if(mat != NULL)
		{
			mat->Unload();
		}
	}
	return 0;
}

//-----------------------------------------------------------------------
bool VObjectSkinDef::operator==(const VObjectSkinDef& def) const
{
	if(def.GetNumMaterials() != GetNumMaterials())
		return false;

	for(uint n = 0; n < GetNumMaterials(); n++)
	{
		if(Defs[n].Name != def.Defs[n].Name)
			return false;
		if(Defs[n].CreationFlags != def.Defs[n].CreationFlags)
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------
VObjectSkinDef& VObjectSkinDef::operator=(const VObjectSkinDef& def)
{
	Defs = def.Defs;
	return *this;
}

//-----------------------------------------------------------------------
void VObject::Init()
{
	ObjectResource::Init();
	m_iMaxDistance = 0x7fffffff;
	m_BBox.Zero();
	m_fEstimateArea = 0;
	m_fRadius = 0;
	m_fCRadius = 0;
}

//-----------------------------------------------------------------------------
uint VObject::CalculateLOD(float aspect) const
{
	//No extra LODs
	uint maxLOD = GetMaxLoadedLOD();
	if(maxLOD == 0)
	{
		return 0;
	}
	
	//No more than one LOD
	uint l = GetMinLoadedLOD();
	if(l == maxLOD)
	{
		return l;
	}

	for(; l <= maxLOD; l++)
	{
		if(aspect >= m_fLODFactors[l])
		{
			return l;
		}
	}
	
	return BYTE_INDEX_NOT_FOUND;
}

//-----------------------------------------------------------------------------
ENF_INLINE uint TimeStampDiff(uint timeNow, uint timeBefore, uint wrapMask)
{
	timeNow &= wrapMask;
	return (timeNow < timeBefore) ? ((65535 - timeBefore) + timeNow) : (timeNow - timeBefore);
}

//-----------------------------------------------------------------------------
void VObjectInstance::SetLOD(uint lod, TVisibleEntity& mc, uint timeStamp)
{
	//for failed lod number, or unsupported objects get out.
	ubyte maxLOD = (ubyte)GetVObject()->GetMaxLoadedLOD();
	if(lod != BYTE_INDEX_NOT_FOUND && lod > maxLOD)
	{
		mc.SourceLOD = maxLOD;
		mc.TargetLOD = maxLOD;
		return;
	}

	if(m_iForceLOD != NO_FORCE_LOD)
	{
		lod = m_iForceLOD;
	}
#if 1

/*	m_iCurrentLOD = 0;
	m_iTargetLOD = 1;

	mc.SourceLOD = 0;
	mc.TargetLOD = 1;

	mc.LODFade = 0.2;

	return;
*/	

	ubyte currentLOD = m_iCurrentLOD;
	ubyte targetLOD = m_iTargetLOD;
	if(lod != targetLOD)
	{
		ushort dist = TimeStampDiff(timeStamp, m_iLastLODChange, 0xffff);
		if(dist > ENF_CROSSFADE_TIME)
		{
			currentLOD = targetLOD;
			targetLOD = lod;
			m_iLastLODChange = (ushort)timeStamp;
			m_iCurrentLOD = currentLOD;
			m_iTargetLOD = targetLOD;
			mc.LODFade = 0;
		}
		else
		{
			mc.LODFade = enf_limit((float)dist * (1.0f / ENF_CROSSFADE_TIME), 0.0f, 1.0f);
		}
	}
	else if(currentLOD != targetLOD)
	{
		ushort dist = TimeStampDiff(timeStamp, m_iLastLODChange, 0xffff);
		if(dist > ENF_CROSSFADE_TIME)
		{
			currentLOD = targetLOD;
			m_iLastLODChange = (ushort)timeStamp;
			m_iCurrentLOD = currentLOD;
			mc.LODFade = 0;
		}
		else
		{
			mc.LODFade = enf_limit((float)dist * (1.0f / ENF_CROSSFADE_TIME), 0.0f, 1.0f);
		}
	}
	//we want to limit it here. Limited number of LOD variation can lead to better instance-batching
	ubyte minLOD = (ubyte)GetVObject()->GetMinLoadedLOD();
	mc.SourceLOD = currentLOD == BYTE_INDEX_NOT_FOUND ? BYTE_INDEX_NOT_FOUND : enf_limit(currentLOD, minLOD, maxLOD);
	mc.TargetLOD = targetLOD == BYTE_INDEX_NOT_FOUND ? BYTE_INDEX_NOT_FOUND : enf_limit(targetLOD, minLOD, maxLOD);

#else
	mc.SourceLOD = m_iCurrentLOD = lod;
	mc.TargetLOD = m_iTargetLOD = lod;
#endif
}

//-----------------------------------------------------------------------------
void VObjectInstance::SetLOD(TVisibleEntity& mc, uint timeStamp)
{
	//for failed lod number, or unsupported objects get out.
	uint maxLOD = GetVObject()->GetMaxLoadedLOD();
	if(maxLOD == 0)
	{
		mc.SourceLOD = 0;
		mc.TargetLOD = 0;
		return;
	}

	uint currentLOD = m_iCurrentLOD;
	uint targetLOD = m_iTargetLOD;
	
	if(currentLOD != targetLOD)
	{
		ushort dist = TimeStampDiff(timeStamp, m_iLastLODChange, 0xffff);
		if(dist > ENF_CROSSFADE_TIME)
		{
			currentLOD = targetLOD;
			mc.LODFade = 0;
		}
		else
		{
			mc.LODFade = enf_limit((float)dist * (1.0f / ENF_CROSSFADE_TIME), 0.0f, 1.0f);
		}
	}

	//we want to limit it here. Limited number of LOD variation can lead to better instance-batching
	uint minLOD = GetVObject()->GetMinLoadedLOD();
	mc.SourceLOD = enf_limit(currentLOD, minLOD, maxLOD);
	mc.TargetLOD = enf_limit(targetLOD, minLOD, maxLOD);
}

#ifdef ENF_DONE
//-----------------------------------------------------------------------
bool VObject::RenderGroup(const TVisibleEntity* vent, uint from, const VisibleGroup& group, Pipeline* pipeline)
{
	for(uint n = from; n < group.LastEntity; n++)
		Render(vent[n], pipeline);
	return true;
}
#endif
//-----------------------------------------------------------------------
void VObject::SetMaxDistance(int maxdist)
{
	m_iMaxDistance = maxdist;
}

//-----------------------------------------------------------------------
void VObject::AreaFromBoundingBox()
{
  Vector3 size = m_BBox.GetSize();
  float avgDiameter = (size.x + size.y + size.z) * (0.5f / 3);
  m_fEstimateArea = sqr(avgDiameter);
}

//-----------------------------------------------------------------------
uint VObject::FindSkin(const VObjectSkinDef& skin) const
{
	for(uint n = 0; n < m_Skins.GetCardinality(); n++)
	{
		if(m_Skins[n] != NULL)
		{
			if(skin == *m_Skins[n])
				return n;
		}
	}
	return INDEX_NOT_FOUND;
}

//-----------------------------------------------------------------------
uint VObject::AddSkin(VObjectSkin* skin)
{
	enf_assert(skin);
	enf_assert2(skin->m_Materials.GetCardinality() == skin->GetNumMaterials(), "CreateMaterials must be called prior to AddSkin");
	skin->AddRef();
	skin->m_pObject = this;
	for(uint n = 0; n < m_Skins.GetCardinality(); n++)
	{
		if(m_Skins[n] == NULL)
		{
			m_Skins[n] = skin;
			skin->m_iSkinIndex = n;
			return n;
		}
	}
	uint index = skin->m_iSkinIndex = m_Skins.Insert(skin);
	return index;
}

//-----------------------------------------------------------------------
const VObjectSkinDef* VObject::GetDefaultSkin() const
{
	return m_Skins.IsEmpty() ? NULL : m_Skins[0];
}

//-----------------------------------------------------------------------
uint VObjectInstance::SetCameraMask(uint mask)
{
	uint prev = m_iCameraMask;
	if(prev != mask)
	{
		m_iCameraMask = mask;
	}
	return prev;
}

//-----------------------------------------------------------------------
uint VObjectInstance::SetSkin(const VObjectSkinDef& skindef)
{
	uint skinindex = m_pObject->FindSkin(skindef);
	if(m_iSkin == skinindex)
		return skinindex;

	uint foundSkin = skinindex;
	bool loaded = IsFlagSet(VIF_MATERIAL_LOADED);
	
	if(skinindex == INDEX_NOT_FOUND)
	{
		ComPtr<VObjectSkin> newSkin = ENF_NEW VObjectSkin(skindef);
		if(!newSkin->CreateMaterials(m_pObject->GetName()))
			return INDEX_NOT_FOUND;

		skinindex = m_pObject->AddSkin(newSkin);
	}
	else
	{
		m_pObject->m_Skins[skinindex]->AddRef();
	}

	if(loaded && skinindex != 0)
	{
		//LogF(LL_WARNING, "Changing already loaded skin. Object '%s'", m_pObject->GetName().cstr());
	}

	if(loaded)
		m_pObject->m_Skins[skinindex]->Load();

	if(m_iSkin != NO_SKIN_INDEX)
	{
		if(loaded)
			m_pObject->m_Skins[m_iSkin]->Unload();
		
		m_pObject->m_Skins[m_iSkin]->Release();
	}

	m_iSkin = skinindex;
	UpdateFlags();
	return skinindex;
}

//-----------------------------------------------------------------------
uint VObjectInstance::SetSkin(uint skinindex)
{
	if(m_iSkin == skinindex)
		return skinindex;

	uint foundSkin = skinindex;
	bool loaded = IsFlagSet(VIF_MATERIAL_LOADED);

	m_pObject->m_Skins[skinindex]->AddRef();

	if(loaded && skinindex != 0)
	{
		//LogF(LL_WARNING, "Changing already loaded skin. Object '%s'", m_pObject->GetName().cstr());
	}

	if(loaded)
		m_pObject->m_Skins[skinindex]->Load();

	if(m_iSkin != NO_SKIN_INDEX)
	{
		if(loaded)
			m_pObject->m_Skins[m_iSkin]->Unload();

		m_pObject->m_Skins[m_iSkin]->Release();
	}

	m_iSkin = skinindex;
	UpdateFlags();
	return skinindex;
}

//-----------------------------------------------------------------------
void VObject::RemoveSkin(VObjectSkin* skin)
{
	enf_assert(skin->m_iSkinIndex < m_Skins.GetCardinality());
	enf_assert(m_Skins[skin->m_iSkinIndex] == skin);

	m_Skins[skin->m_iSkinIndex] = NULL;
	skin->m_pObject = NULL;
	skin->m_iSkinIndex = INDEX_NOT_FOUND;
}

//-----------------------------------------------------------------------
void VObjectInstance::UpdateFlags()
{
	ClearFlags(VIF_SHADOWCASTER|VIF_NEED_ENVMAP|VIF_ACCEPTS_LIGHT|VIF_NEEDS_AMBIENT_LIGHT|VIF_SHADOWRECEIVER|VIF_LOD_CROSSFADE|VIF_INSTANCING);

	for(uint n = 0; n < GetVObject()->GetNumMaterials(); n++)
	{
		const Material* mat = GetMaterial(n);

		if(mat->IsShadowCaster())
			SetFlags(VIF_SHADOWCASTER);

		if(mat->NeedsEnvMap())
			SetFlags(VIF_NEED_ENVMAP);

		if(mat->CheckPassMask(BITF(PT_DIFFUSE_SPECULAR)))
			SetFlags(VIF_ACCEPTS_LIGHT);

		if(mat->CheckPassMask(BITF(PT_AMBIENT)))
			SetFlags(VIF_NEEDS_AMBIENT_LIGHT);

		if(mat->IsShadowReceiver())
			SetFlags(VIF_SHADOWRECEIVER);

		if(mat->IsFlagSet(MF_CANCROSSFADE))
			SetFlags(VIF_LOD_CROSSFADE);

		if(mat->IsFlagSet(MF_INSTANCING))
			SetFlags(VIF_INSTANCING);
	}
}

//-----------------------------------------------------------------------
void VObjectInstance::AddInteraction(LightInteraction* li)
{
	enf_assert(li != NULL);
	li->m_pNextOnEntity = m_pInteractions;
	li->m_pPrevOnEntity = &m_pInteractions;
	if(m_pInteractions)
		m_pInteractions->m_pPrevOnEntity = &li->m_pNextOnEntity;
	m_pInteractions = li;
}

//-----------------------------------------------------------------------
void VObjectInstance::RemoveAllInteractions(bool dirty)
{
	LightInteraction* li = m_pInteractions;

	if(li)
	{
		while(li)
		{
			LightInteraction* next = li->m_pNextOnEntity;
			delete li;
			li = next;
		}

		m_pInteractions = NULL;
	}
	SetEnvMap(NULL);
	SetFlags(dirty ? VIF_LIGHTINTERACTIONS_DIRTY : 0);
}

//-----------------------------------------------------------------------
AAB VObjectInstance::ComputeWorldBoundBox(const Matrix43& mat, uint flags) const
{
	AAB box;

	// expand for rotation
	box.InitFromOBB(mat, m_pObject->GetBoundingBox());

	return box;
}

//-----------------------------------------------------------------------
AAB VObjectInstance::ComputeLocalBoundBox(const Matrix43& mat, uint flags) const
{
	//default dumb implementation
	Matrix43 lmat = mat.GetInverse();
	AAB wbbox = ComputeWorldBoundBox(mat, flags);
	AAB lbbox;
	lbbox.InitFromOBB(lmat, wbbox);
	return lbbox;
}

//-----------------------------------------------------------------------
void VObjectInstance::Serialize(SerializeInputAdapter* adapter)
{
	BaseItem::Serialize(adapter);
	m_iForceLOD = adapter->ReadUInt8();
	m_iCameraIndex = adapter->ReadUInt8();
	m_iCameraMask = adapter->ReadUInt32();
	m_iFlags = adapter->ReadUInt32();
}

//-----------------------------------------------------------------------
void VObjectInstance::Serialize(SerializeOutputAdapter* adapter)
{
	BaseItem::Serialize(adapter);
	adapter->WriteUInt8(m_iForceLOD);
	adapter->WriteUInt8(m_iCameraIndex);
	adapter->WriteUInt32(m_iCameraMask);
	adapter->WriteUInt32(m_iFlags);
}

//-----------------------------------------------------------------------
VObjectInstance::VObjectInstance(VObject* obj, Entity* owner):
	m_pObject(obj),
	m_pInteractions(NULL),
	m_iFlags(0),
	m_iDLights(0),
	m_iDLightFrame(0),
	m_iLODTime(0),
	m_iLastLODChange(0),
	m_iCurrentLOD(0),
	m_iTargetLOD(0),
	m_pEnvMap(NULL),
	m_iSkin(NO_SKIN_INDEX),
	m_iCameraIndex(0),
	m_iCameraMask(0xffffffff),
	m_iForceLOD(0xff)
{
	enf_assert(obj);
	enf_assert(owner);

	MakeInteractionsDirty();
	obj->AddRef();
#ifndef ENF_CONSOLE
	obj->m_Instances.Insert(this);
#endif
}

//-----------------------------------------------------------------------
VObjectInstance::~VObjectInstance()
{
	RemoveAllInteractions();
	if(m_iSkin != NO_SKIN_INDEX)
	{
		if(IsFlagSet(VIF_MATERIAL_LOADED))
			m_pObject->m_Skins[m_iSkin]->Unload();
		m_pObject->m_Skins[m_iSkin]->Release();
	}

#ifndef ENF_CONSOLE
	m_pObject->m_Instances.Remove(this);
#endif
	m_pObject->Release();
	//TODO: release camera
	//GetEntity()->GetWorld()->GetCamera(m_iCameraIndex);
}

//-----------------------------------------------------------------------
void VObjectInstance::SetRenderView(Entity* ent, uint camindex, uint width, uint height)
{
	m_iCameraIndex = camindex;
#ifdef ENF_DONE
	ent->GetWorld()->SetCameraRenderTarget(camindex, width, height);
#endif
}

//-----------------------------------------------------------------------
bool VObject::Reload(CInputStream* instream)
{
	//enf_assert2(m_pResourceManager != NULL, "Cannot reload unmanaged object");

#ifdef ENF_CONSOLE
	enf_assert(!"Not supported");
#else
	TVObjectInstanceListIterator iter(m_Instances);

	while(iter.HasMoreElements())
	{
		iter.GetNext()->Unload();
	}

	Unload();
	Init();

	if(instream)
	{
		enf_assert(instream->IsOpen());

		if(!Load(instream))
			return false;
	}
	else
	{
		CFileInputStream in(GetName().cstr());

		if(!in.IsOpen() || !Load(&in))
			return false;
	}

	TVObjectInstanceListIterator iter2(m_Instances);

	while(iter2.HasMoreElements())
	{
		iter2.GetNext()->Init();
	}
#endif
	return true;
}


}