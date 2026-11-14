//-----------------------------------------------------------------------------
// File: enf_iresourcemanager.h
//
// Desc: Resource manager implementation
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_IRESOURCEMANAGER_H
#define ENF_IRESOURCEMANAGER_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "common/enf_set.h"
#include "common/enf_hashmap.h"
#include "system/enf_thread.h"
#include "engine/enf_module.h"
#include "enf_resourcemanager.h"

namespace enf
{
/** @defgroup Resources Resource manager
@{*/

class Renderer;
class Texture;
class ObjectResource;
class AnimScript;
class ResourceManager;
class ResourceLoaderThread;
class ImageLoader;

//-----------------------------------------------------------------------------
class PUBLIC_API IResourceManager: public ResourceManager, public FlushableObject
{
private:
	friend class ResourceLoaderThread;

	Set<ObjectResource*>	m_Objects;
	HashMap<const char*, ObjectResource*, StringHashFunc> m_ObjectHashList;
	Set<ObjectResource*>	m_DisposableObjects;
	HashMap<const char*, ObjectResource*, StringHashFunc> m_DisposableObjectHashList;

	Renderer*			m_pDevice;
	ImageLoader*	m_pImageLoader;
	Set<Texture*>	m_Textures;
	HashMap<const char*, Texture*, StringHashFunc> m_TextureHashList;
//!Frees resources and deletes ourself
	~IResourceManager();

	CMutex							m_GeneralMutex;
	CMutex							m_ManagedMutex;
	CMutex							m_ObjectsMutex;
	CMutex							m_DisposableObjectsMutex;
	CMutex							m_TexturesMutex;
	CMutex							m_LoadMutex;
	CSemaphore					m_QueueEvent;
	CMutex							m_LoadedMutex;
	IL_INT32						m_iDisableLoad;
	List<LoadResource>	m_LoadResources;
	List<LoadResource>	m_LoadedResources;
	typedef Pair<CStr, ResourceLoader*> TLoaderItem;
	Set<TLoaderItem>		m_Loaders;
	IL_INT32						m_iLastFrame;
	
	IL_INT32						m_iNumLoadingObjects;
	IL_INT32						m_iNumLoadingTextures;

	uint64							m_iTimeStamp;
	ResourceLoaderThread*	m_pLoaderThread;
	IL_INT32						m_iLoaded;
	uint								m_iDetail;
	bool								m_bWait4Textures;

	bool								m_bReloadTextures;
#ifdef ENF_DONE
	ComPtr<Texture>			m_pDiffusePlaceholder;
	ComPtr<Texture>			m_pNormalPlaceholder;
	ComPtr<Texture>			m_pNormalDXTPlaceholder;
	ComPtr<Texture>			m_pBlackPlaceholder;

	#ifdef ENF_PS3
		ComPtr<Texture>		m_pFloatPlaceholder;
	#endif
#endif
	Set<ResourceRef*>		m_ManagedResources;

	void Init();

	//!The same as GetTexture, but without name validation
	Texture* GetTextureInternal(const char* name);

	/*!
	@copydoc enf::ResourceManager::LoadAnyTexture
	\param nlen		length of name	
	*/
	Texture* LoadAnyTextureInternal(char* name, int nlen, uint flags, TextureUsage usage, int loadPriority);

	void AddLoadResource(LoadResource* res);
	void CancelLoading(ResourceRef* res);
	LoadResource* DequeueResource();
	void EnqueueLoadedResource(LoadResource* res);
	void SetReloadFlag();
	void ReloadTextures();
	Texture* CreatePlaceholderTexture(const char* name, uint dwFlags, TextureUsage tusage);
	virtual void Flush (unsigned int level);
	bool AddVObjectInternal(ObjectResource* obj);

public:
	ENF_DECLARE_MMOBJECT(IResourceManager);

	bool OnChangeSettings(TiXmlElement* xmlnode);
	void OnInsert(TiXmlElement* xmlnode);
	void OnEvent(const Event& event);

	void TouchResource(LoadResource* res, int level, int priority);
	ImageLoader* GetImageLoader()
	{
		return m_pImageLoader;
	}

	uint SetTextureDetail(uint detail)
	{
		uint prevDetail = m_iDetail;
		m_iDetail = detail;
		return prevDetail;
	}
	uint GetTextureDetail() const
	{
		return m_iDetail;
	}
	bool WaitForTextures(bool wait4tex);

	uint GetNumLoadingResources();
	uint GetNumRecentLoadingTextures();
	uint GetNumLoadingObjects();
	void GetInfo(ResourceInfo& info);

	void	Update(uint64 timeStamp, uint maxMilliseconds);
	void Lock() const
	{
		m_LoadedMutex.Lock();
	}
	void Unlock() const
	{
		m_LoadedMutex.Unlock();
	}

	void DisableLoad()
	{
		IL_INCREMENT(m_iDisableLoad);
	}
	void EnableLoad()
	{
		enf_assert(m_iDisableLoad > 0);
		IL_DECREMENT(m_iDisableLoad);
	}

	void RegisterResourceLoader(CStr ext, ResourceLoader* loader);
	void UnregisterResourceLoader(ResourceLoader* loader);

	bool		AddTexture(Texture* tex);
	bool		RemoveTexture(Texture* tex);
	Texture* CreateTextureFromFile(CStr name, uint flags, TextureUsage usage, int loadPriority);
	ebool ReloadTexture(Texture* tex);
	Texture* GetTexture(CStr name);
	
	/*!
	loads texture 
	\param name		name of texture
	\param flags	texture flags (like TEXTR_NOMIPMAP or TEXTR_CUBE...)
	\param usage	internal usage of texture (like MAP_DIFFUSE...)
	*/
	Texture* LoadAnyTexture(CStr name, uint flags, TextureUsage usage, int loadPriority);

	const ObjectResource*const* GetObjects() const
	{
		return m_Objects.GetArray();
	}

	uint GetNumObjects() const
	{
		return m_Objects.GetCardinality();
	}

	ObjectResource* GetResourceObject(CStr name, uint flags = 0);
	bool AddVObject(ObjectResource* obj);
	void RemoveVObject(ObjectResource* obj);
	void DisposeVObject(ObjectResource* obj);
	size_t FlushCache();
	void AddManagedResource(ResourceRef* res)
	{
		CMutexLock lock(m_ManagedMutex);
		m_ManagedResources.Insert(res);
	}
	void RemoveManagedResource(ResourceRef* res)
	{
		CMutexLock lock(m_ManagedMutex);
		m_ManagedResources.Remove(res);
	}
	size_t FreeManagedMemory(size_t size);

	IResourceManager(Renderer* device);
	IResourceManager();
};

//@}
}

#endif //ENF_IRESOURCEMANAGER_H