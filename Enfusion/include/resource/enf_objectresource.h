//-----------------------------------------------------------------------------
// File: resource/enf_objectresource.h
//
// Desc: resource superclass
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_VOBJECTRESOURCE_H
#define ENF_VOBJECTRESOURCE_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "common/enf_mathlib.h"
#include "engine/enf_item.h"
#include "enf_resource.h"

namespace enf
{
/** @defgroup VObjects Visual object implementations
@{*/

class LoadResource;
class CInputStream;
class IResourceManager;
class ResourceManager;

const uint ObjectResourceID = 2000;

enum VObjectTypes
{
	ObjectResourceTypeID = ObjectResourceID + 1,
	VObjectTypeID,
	SpriteTypeID,
	MModelTypeID,
	AnimationTypeID,
	MeshObjectTypeID,
	ParticleEffectTypeID,
	AnimsetTypeID,
	TerrainTypeID,
	CutsceneTypeID,
	LipsyncDefTypeID
};

//---------------------------------------------------------------------------
class PUBLIC_API ObjectResource: public Item, public ResourceRef
{
protected:
	friend class IResourceManager;
	bool					m_bFlush;
	CStr					m_strBaseName;

	virtual void Init()
	{
	}

	ObjectResource():
		m_bFlush(false)
	{
	}

	/*!
	Method responsible for complete cleanup of VObject.
	Following Init/Load should work as with newborn object
	*/
	virtual void Unload();

	virtual ~ObjectResource();

public:
	ENF_DECLARE_ITEM(ObjectResource, Item);

	void Destroy()
	{
		Unload();
		delete this;
	}

	void Swap(ObjectResource& other)
	{
		enf_swap(m_bFlush, other.m_bFlush);
		enf_swap(m_strBaseName, other.m_strBaseName);
	}
	virtual LoadResource* CreateResourceLoader() { return NULL; }

	virtual void SetName(CStr name);

	CStr GetName() const
	{
		return Item::GetName();
	}
	ENF_INLINE CStr GetObjectName() const
	{
		return m_strBaseName;
	}

	/*!
	Release object
	\param flush False if you want to keep object in cache
	\return Number of references, or 0 if object has been removed.
	*/
	int Release(bool flush);

	/*!
	Release object, but keeps it in cache even when refcount==0
	\return Number of references, or 0 if object has been removed.
	*/
	int Release()
	{
		return Release(false);
	}

	/*!
	Method responsible for load of VObject.
	\param instream
	Input stream with object data
	\return
	True when success
	*/
	virtual bool Load(CInputStream* instream) {return true;}

	/*!
	Default method responsible for dynamic reloading.
	\param instream
	Optional input stream. When defined it reloads from the stream. when NULL,
	it loads from FileSystem by its name.
	\return
	True when successfully reloaded
	*/
	virtual bool Reload(CInputStream* instream = NULL);
};


//@}
}
#endif //ENF_VOBJECTRESOURCE_H
