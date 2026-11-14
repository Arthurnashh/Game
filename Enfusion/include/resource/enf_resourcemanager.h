//-----------------------------------------------------------------------------
// File: enf_resourcemanager.h
//
// Desc: Resource manager
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_RESOURCEMANAGER_H
#define ENF_RESOURCEMANAGER_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "engine/enf_module.h"
#include "enf_resource.h"
#include "render/enf_renderenums.h"

namespace enf
{
/** @defgroup Resources Resource manager
@{*/

class Renderer;
class Texture;
class ObjectResource;
class AnimScript;
class ResourceManager;
class LoadResource;
class ImageLoader;

extern PUBLIC_API ResourceManager* g_pResources;

//-----------------------------------------------------------------------------
class PUBLIC_API ResourceLoader: public RefCountObject
{
public:
	ENF_DECLARE_SMALLOBJECT(ResourceLoader);
	ResourceLoader() {}
	virtual ~ResourceLoader() {}
	virtual ObjectResource* CreateResource()=0;
};

const uint LoadResourceID = 9000;

//-----------------------------------------------------------------------------
enum LoadResourceTypes
{
	LoadResourceTypeID = LoadResourceID + 1,
	LoadTextureResourceTypeID,
	LoadMeshLODResourceTypeID,
	LoadSampleResourceTypeID,
	LoadAnimsetResourceTypeID,

	LoadVObjectResourceTypeID,
	LoadMeshObjectResourceTypeID,
	LoadParticleEffectResourceTypeID,
	LoadSpriteResourceTypeID,
	LoadAnimationResourceTypeID,
	LoadLipsyncDefResourceTypeID,
};

//-----------------------------------------------------------------------------
class PUBLIC_API LoadResource: public BaseItem
{
protected:
	friend class IResourceManager;
	friend class List<LoadResource>;
	friend class ListIterator<LoadResource>;

	ResourceRef*	m_pResource;
	LoadResource* ENF_LIST_NEXT;
	LoadResource* ENF_LIST_PREV;

	int				m_iPriority;
	uint			m_iOrdinalNumber;
	size_t		m_iLoadedSize;
	IL_INT32	m_iLastFrame;

public:
	ENF_DECLARE_ITEM(LoadResource, BaseItem);
	ENF_DECLARE_SMALLOBJECT(LoadResource);
	LoadResource(ResourceRef* ref);
	LoadResource();
	virtual ~LoadResource() {}
	ENF_INLINE int64 GetSortValue(uint lastFrame) const;
	ENF_INLINE uint GetOrdinalNumber() const
	{
		return m_iOrdinalNumber;
	}
	ResourceRef* GetResource() const
	{
		return m_pResource;
	}
	virtual CStr GetName() const=0;
	virtual bool DoLoad()=0;
	virtual void Apply()=0;
	virtual size_t GetDataSize() const=0;
};

//-----------------------------------------------------------------------------
class PUBLIC_API LoadVObjectResource: public LoadResource
{
public:
	ENF_DECLARE_ITEM(LoadVObjectResource, LoadResource);
	LoadVObjectResource(ResourceRef* ref): LoadResource(ref) {}
	LoadVObjectResource(): LoadResource() {}
};

//-----------------------------------------------------------------------------
enum VObjectLoadFlags
{
	VLF_PRELOAD = BITF(0),
	VLF_NO_LOG_ERRORS = BITF(1)
};

//-----------------------------------------------------------------------------
struct ResourceInfo
{
	uint	m_iLoadedBytes;
	uint	m_iLoadingResources;
	uint	m_iLoadedResource;
	uint	m_iNumObjects;
	uint	m_iNumDisposableObjects;
	uint	m_iNumTextures;
};

//-----------------------------------------------------------------------------
class PUBLIC_API ResourceManager: public Module
{
protected:
friend class Texture;
//!Frees resources and deletes ourself
	virtual ~ResourceManager();

public:
	ENF_DECLARE_ITEM(ResourceManager, Module);

	void SetCurrent()
	{
		g_pResources = this;
	}

	virtual ImageLoader* GetImageLoader()=0;
	virtual void RegisterResourceLoader(CStr ext, ResourceLoader* loader)=0;
	virtual void UnregisterResourceLoader(ResourceLoader* loader)=0;

	/*static*/ bool GetPhysicalTextureName(CStr texname, char* resname);
	virtual uint SetTextureDetail(uint detail)=0;
	virtual uint GetTextureDetail() const=0;
	virtual bool WaitForTextures(bool wait4tex)=0;
	virtual void OnEvent(const Event& event)=0;

	/*!
	It makes clean name, without backslashes, all lowercase and
	returns pointer to extension.
	\param in Input name
	\param out Output name. There must be enough space!
	\return Pointer to extension (i.e. "xob" in case of obj/test.xob)
	*/
	static CStr FixName(CStr in, char* out);

	/*!
	Allow to add already existing texture into manager
	*/
	virtual bool AddTexture(Texture* tex)=0;

	/*!
	Remove texture from manager
	*/
	virtual bool RemoveTexture(Texture* tex)=0;

	/*!
	Tries to load texture with all known extensions (.dds, .tga)
	\param name Name of texture file
	\param flags Flags. See enf::TextureCreationFlags
	\param usage Usage, see enf::TextureUsage
	\return Texture, or NULL when not found
	*/
	virtual Texture* LoadAnyTexture(CStr name, uint flags, TextureUsage usage, int loadPriority = 0)=0;

	/*!
	Gets texture from cache, or loads it from disk
	This method increments refcount.
	\param name Name of texture
	\param flags Creation flags See: enf::TextureCreationFlags
	\param usage Usage hint. See enf::TextureUsage
	\return Pointer to texture, or NULL when doesn't exists
	*/
	virtual Texture* CreateTextureFromFile(CStr name, uint flags, TextureUsage usage, int loadPriority = 0)=0;

	/*!
	Reloads texture if possible
	\param tex Pointer to managed texture. 
	\return True when success, false otherwise
	*/
	virtual ebool ReloadTexture(Texture* tex)=0;

	/*!
	Gets texture from cache, otherwise returns NULL.
	This method increments refcount.
	\param name Name of texture
	\return Pointer to texture, or NULL when doesn't exists
	*/
	virtual Texture* GetTexture(CStr name)=0;

	//!Creates cloned placeholder texture
	virtual Texture* CreatePlaceholderTexture(const char* name, uint dwFlags, TextureUsage tusage)=0;

	/*!
	Gets array of objects in cache
	*/
	virtual const ObjectResource*const* GetObjects() const=0;
	virtual uint GetNumObjects() const=0;

	/*!
	Gets object from cache, or loads it from disk
	This method increments refcount.
	\param name Name of object
	\param flags Optional flags
	\return Pointer to loaded object, or NULL when doesn't exists.
	*/
	virtual ObjectResource *GetResourceObject(CStr name, uint flags = 0)=0;

	/*!
	Adds already existing object to cache.
	\param obj Pointer to object
	\return True when object is inserted into cache and refcount was incremented.
	False when it already was there.
	*/
	virtual bool AddVObject(ObjectResource* obj)=0;

	/*!
	Remove existing object from cache.
	\param obj Pointer to object
	*/
	virtual void RemoveVObject(ObjectResource* obj)=0;
	virtual void DisposeVObject(ObjectResource* obj)=0;

//!Flushes cached resources with refcount == 0
	virtual size_t FlushCache()=0;

	virtual void AddManagedResource(ResourceRef* res)=0;
	virtual void RemoveManagedResource(ResourceRef* res)=0;
	virtual size_t FreeManagedMemory(size_t size)=0;
	virtual void AddLoadResource(LoadResource* res)=0;
	virtual void CancelLoading(ResourceRef* res)=0;

	virtual uint GetNumLoadingResources()=0;
	virtual uint GetNumRecentLoadingTextures()=0;
	//TODO: temporary!
	virtual uint GetNumLoadingObjects()=0;
	virtual void Update(uint64 timeStamp, uint maxMilliseconds)=0;
	virtual void Lock() const=0;
	virtual void Unlock() const=0;
	virtual void DisableLoad()=0;
	virtual void EnableLoad()=0;

	virtual void GetInfo(ResourceInfo& info)=0;

	/*!
	Creates new resourcemanager.
	\param device Pointer to existing Renderer device which will manage textures and
	other device dependent resources.
	*/
	ResourceManager(Renderer* device);

	//!Empty constructor for enf::Module convenience
	ResourceManager();
};


PUBLIC_API ResourceManager* CreateResourceManager(Renderer* device);

//-----------------------------------------------------------------------------
ENF_INLINEFUNC bool IsResourceManager()
{
	return g_pResources != NULL;
}

//-----------------------------------------------------------------------------
ENF_INLINEFUNC ResourceManager* GetResources()
{
	enf_assert(g_pResources);
	return g_pResources;
}
//@}
}

#endif //ENF_RESOURCEMANAGER_H
