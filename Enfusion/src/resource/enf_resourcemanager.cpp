//-----------------------------------------------------------------------------
// File: enf_resourcemanager.cpp
//
// Desc: Resource manager
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "enf_main.h"
#include "engine/enf_engine.h"
#include "common/enf_streams.h"
#include "common/enf_quicksort.h"
#include "common/enf_diag.h"
#include "Common/enf_color.h"
#include "system/enf_timer.h"
#include "system/enf_profiler.h"
#include "system/enf_vfilesystem.h"
#include "resource/enf_iresourcemanager.h"
#include "resource/enf_objectresource.h"

//#include "render/enf_rendmain.h"
//#include "render/enf_texture.h"
//#include "render/enf_textureresloader.h"
#include "animations/enf_animation.h"
//#include "vobject/enf_sprite.h"
//#include "vobject/meshobject/enf_meshobject.h"
#include "resource/enf_particleeffect.h"
//#include "vobject/enf_animset.h"
//#include "enf_lipsync.h"

//#include "enf_imageloader.h"
#include "tinyxml/tinyxml.h"

//#include "enf_scriptapivars.h"


#ifdef __APPLE__
	#define ENF_FORCE_SYNC
#endif

//#define ENF_DONE

namespace enf
{
#ifdef ENF_DEVELOPER
	//#define STRESSTEST
#endif

#ifdef STRESSTEST
	#define MIN_FREEMEM_TRESHOLD			0
	#define MIN_RELEASE_MEM					1

	#define MIN_LOAD_FREEMEM_TRESHOLD	0
	#define MIN_LOAD_RELEASE_MEM			1
#else
	/*!
	memory thresholds while playing
	*/
	#ifdef ENF_PS3
		#define MIN_FREEMEM_TRESHOLD			10
		#define MIN_RELEASE_MEM					16
	#else
		#define MIN_FREEMEM_TRESHOLD			18
		#define MIN_RELEASE_MEM					24
	#endif

	/*!
	memory thresholds while loading
	*/
	#define MIN_LOAD_FREEMEM_TRESHOLD	8
	#define MIN_LOAD_RELEASE_MEM			14
#endif


/*
currently loaded resources in manager
*/
#if defined(ENF_XENON)
	#define MIN_LOADED_TRESHOLD			20
#elif defined(ENF_PS3)
	#define MIN_LOADED_TRESHOLD			8
#else
	#define MIN_LOADED_TRESHOLD			128
#endif


ResourceManager* g_pResources = NULL;
IL_INT32 ResourceRef::m_iCurrentFrame = 0;
//static const char *strTexExt[] = {".dds", ".tga", NULL};


//-----------------------------------------------------------------------
LoadResource::LoadResource():
	m_pResource(NULL),
	ENF_LIST_NEXT(NULL),
	ENF_LIST_PREV(NULL),
	m_iPriority(0),
	m_iLastFrame(0),
	m_iLoadedSize(0)
{
	m_iOrdinalNumber = 0;
}

//-----------------------------------------------------------------------
LoadResource::LoadResource(ResourceRef* ref):
	m_pResource(ref),
	ENF_LIST_NEXT(NULL),
	ENF_LIST_PREV(NULL),
	m_iPriority(0),
	m_iLastFrame(0),
	m_iLoadedSize(0)
{
	enf_assert(ref);
	enf_assert(ref->m_pLoadHandle == NULL);
	ref->m_pLoadHandle = this;

	m_iOrdinalNumber = ref->GetFileHandle() == BAD_FILE_HANDLE ? 0 : GetFileSystem()->GetOrdinalNumberFromHandle(ref->GetFileHandle());
}

//-----------------------------------------------------------------------
int64 LoadResource::GetSortValue(uint lastFrame) const
{
	int64 sortValue = 1000 - enf_limit(m_iPriority, -1000, 1000);
	sortValue += (32767 - enf_min(int(32767), int(lastFrame - m_iLastFrame))) << 12;
	sortValue = (sortValue << 35) | m_iOrdinalNumber;
	return sortValue;
}

//-----------------------------------------------------------------------
class PUBLIC_API ResourceLoaderThread: protected CThread
{
public:
	ResourceLoaderThread(IResourceManager* manager);
	~ResourceLoaderThread();

	bool Start(uint core)
	{
		return CThread::Start(core, "ResourceLoader");
	}

	void Kill()
	{
		IL_EXCHANGE(m_QuitFlag, 1);
		IL_EXCHANGE(m_pManager->m_iDisableLoad, 0);
		m_pManager->m_QueueEvent.Release();
		WaitForExit();
	}

protected:
	IResourceManager* m_pManager;
	void Run();
	IL_INT32		m_QuitFlag;
};

//-----------------------------------------------------------------------
ResourceLoaderThread::~ResourceLoaderThread()
{
}

//-----------------------------------------------------------------------
ResourceLoaderThread::ResourceLoaderThread(IResourceManager* manager):
	CThread(512 * 1024),
	m_pManager(manager),
	m_QuitFlag(0)
{
}

//-----------------------------------------------------------------------
void IResourceManager::Flush (unsigned int level)
{
	FlushCache();
}

//-----------------------------------------------------------------------
LoadResource* IResourceManager::DequeueResource()
{
	CMutexLock lock(m_LoadMutex);
	if(m_LoadResources.IsEmpty())
		return NULL;

	LoadResource* first = m_LoadResources.GetFirstNode();
	m_LoadResources.Remove(first);
	return first;

}

//-----------------------------------------------------------------------
void IResourceManager::EnqueueLoadedResource(LoadResource* res)
{
	CMutexLock lock(m_LoadedMutex);
	res->m_iLoadedSize = res->GetDataSize();
	IL_EXCHANGE_ADD(m_iLoaded, res->m_iLoadedSize);
	m_LoadedResources.Insert(res);
}

//-----------------------------------------------------------------------
void ResourceLoaderThread::Run()
{
	while(true)
	{
		WaitForSingle(m_pManager->m_QueueEvent, INFINITE_TIMEOUT);

		//how to make it better?
		while(IL_READ(m_pManager->m_iDisableLoad) > 0)
			CThread::Sleep(1);

		if(IL_READ(m_QuitFlag))
			return;

		//give it chance to solve it
		uint maxIterations = 500;

		#if defined(ENF_XENON) || defined(ENF_PS3)
			uint freeMem = GetEngine()->IsLoading() ? ENF_MEGABYTE(MIN_LOAD_FREEMEM_TRESHOLD) : ENF_MEGABYTE(MIN_FREEMEM_TRESHOLD);

			while (MemoryManager::GetFreeMemory() < freeMem || m_pManager->m_iLoaded > ENF_MEGABYTE(MIN_LOADED_TRESHOLD))
			{
				if (--maxIterations == 0)
					break;

				CThread::Sleep(10);			

				freeMem = GetEngine()->IsLoading() ? ENF_MEGABYTE(MIN_LOAD_FREEMEM_TRESHOLD) : ENF_MEGABYTE(MIN_FREEMEM_TRESHOLD);
			}
		#else
			while (m_pManager->m_iLoaded > ENF_MEGABYTE(MIN_LOADED_TRESHOLD))
			{
				if (--maxIterations == 0)
					break;

				CThread::Sleep(10);
			}
		#endif

		LoadResource* res = m_pManager->DequeueResource();

		//WTF??
		if(res == NULL)
			continue;
/*
static bool oom = false;

if(oom)
{
	oom = false;
	MemoryManager::OutOfMemory(__FILE__, __LINE__, MT_PHYSICAL, 1000000);
}
*/
		//Timer::PushTime();
	
		if(!res->DoLoad())
			LogF(LL_ERROR, "Failed to load '%s'", res->GetName().cstr());

		/*float ms = Timer::PopTime();
		CStr name = "";
		if(res->GetResource() && res->GetResource()->GetFileHandle() != BAD_FILE_HANDLE)
			name = GetFileSystem()->GetNameFromHandle(res->GetResource()->GetFileHandle());

		LogF(LL_NORMAL, "Ordinal# %08x time %.1f res %s\n", res->GetOrdinalNumber(), ms, name.cstr());
*/
		m_pManager->EnqueueLoadedResource(res);

		if(IL_READ(m_QuitFlag))
			return;
	}
}


//-----------------------------------------------------------------------
void ResourceRef::FinishLoading(LoadResource* ldres)
{
	enf_assert(m_pLoadHandle == ldres);
	m_pLoadHandle = NULL;
}

//-----------------------------------------------------------------------
uint ResourceRef::GetAge() const
{
	uint lf = IL_READ(m_iLastFrame);

	if(lf <= (uint)m_iCurrentFrame)
		return m_iCurrentFrame - lf;
	else
		return (0xffffffff - lf) + m_iCurrentFrame;
}
//-----------------------------------------------------------------------
void ResourceRef::SetFileHandle(FileHandle handle)
{
	FileHandle prev = m_FileHandle;
	if(prev != handle)
	{
		if(prev != BAD_FILE_HANDLE)
			GetFileSystem()->FreeHandle(prev);

		m_FileHandle = handle;
	}
}

//-----------------------------------------------------------------------
ResourceRef::~ResourceRef()
{
	//release handle
	SetFileHandle(BAD_FILE_HANDLE);
}

//-----------------------------------------------------------------------
void ResourceRef::_Touch(uint level) const
{
	m_iLastFrame = m_iCurrentFrame;
	m_iXLevel = level;
	if(m_pLoadHandle && m_pManager)
		((IResourceManager*)m_pManager)->TouchResource((LoadResource*)m_pLoadHandle, level, 0);
}

//-----------------------------------------------------------------------
void IResourceManager::TouchResource(LoadResource* res, int level, int priority)
{
	//just change frame, timestamp etc
	IL_EXCHANGE(res->m_iLastFrame, m_iLastFrame);

	//TODO: put them to per-frame set, so we will evaluate needs at once
}

//-----------------------------------------------------------------------
void IResourceManager::CancelLoading(ResourceRef* res)
{
	CMutexLock lock(m_LoadMutex);
	LoadResource* ld = (LoadResource*)res->m_pLoadHandle;
	if(ld)
	{
		if(m_LoadedResources.Contains(ld))
		{
			m_LoadedResources.Remove(ld);
			IL_EXCHANGE_ADD(m_iLoaded, -ld->m_iLoadedSize);
		}
		else
			m_LoadResources.Remove(ld);

		if(ld->HasType(LoadVObjectResourceTypeID))
			IL_DECREMENT(m_iNumLoadingObjects);
		else if(ld->HasType(LoadTextureResourceTypeID))
			IL_DECREMENT(m_iNumLoadingTextures);

		delete ld;
	}
}

//-----------------------------------------------------------------------
void IResourceManager::AddLoadResource(LoadResource* res)
{
	{
		CMutexLock lock(m_LoadMutex);
		
		if(res->HasType(LoadVObjectResourceTypeID))
			IL_INCREMENT(m_iNumLoadingObjects);
		else if(res->HasType(LoadTextureResourceTypeID))
			IL_INCREMENT(m_iNumLoadingTextures);


		if(m_LoadResources.IsEmpty())
		{
			m_LoadResources.Insert(res);
		}
		else
		{
			uint lastFrame = IL_READ(m_iLastFrame);
			int64 value = res->GetSortValue(lastFrame);

			if(m_LoadResources.GetFirstNode()->GetSortValue(lastFrame) >= value)
			{
				m_LoadResources.InsertAsFirst(res);
			}
			else if(m_LoadResources.GetLastNode()->GetSortValue(lastFrame) <= value)
			{
				m_LoadResources.Insert(res);
			}
			else
			{
				ListIterator<LoadResource> lditer(m_LoadResources);
				bool insert = false;
				while(lditer.HasMoreElements())
				{
					LoadResource* ldres = lditer.GetNext();
					int64 sortValue = ldres->GetSortValue(lastFrame);

					if(sortValue > value)
					{
						m_LoadResources.InsertBefore(res, ldres);
						break;
					}
				}
			}
		}
	}
	m_QueueEvent.Release();
}

//-----------------------------------------------------------------------
bool IResourceManager::WaitForTextures(bool wait4tex)
{
	bool prev = m_bWait4Textures;
	m_bWait4Textures = wait4tex;
	return prev;
}

//-----------------------------------------------------------------------
uint IResourceManager::GetNumLoadingObjects()
{
	return IL_READ(m_iNumLoadingObjects);
}

//-----------------------------------------------------------------------
uint IResourceManager::GetNumLoadingResources()
{
	uint num = 0;

	if(m_bWait4Textures)
		num += IL_READ(m_iNumLoadingTextures);
	num += IL_READ(m_iNumLoadingObjects);

//	return m_LoadResources.GetCardinality();
	return num;
}

//-----------------------------------------------------------------------
uint IResourceManager::GetNumRecentLoadingTextures()
{
	uint numTexs = 0;
	if(m_bWait4Textures)
	{
		{
			CMutexLock lock(m_LoadMutex);
			ListIterator<LoadResource> lditer(m_LoadResources);

			while(lditer.HasMoreElements())
			{
				LoadResource* ldres = lditer.GetNext();
				if(ldres->HasType(LoadTextureResourceTypeID))
				{
		//			if(lres->m_iLastFrame != 0)
						numTexs++;
				}
			}
		}
		{
			CMutexLock lock(m_LoadedMutex);
			ListIterator<LoadResource> lditer(m_LoadedResources);

			while(lditer.HasMoreElements())
			{
				LoadResource* ldres = lditer.GetNext();
				if(ldres->HasType(LoadTextureResourceTypeID))
				{
		//			if(lres->m_iLastFrame != 0)
						numTexs++;
				}
			}
		}
	}
	return numTexs;
}

//-----------------------------------------------------------------------
bool IResourceManager::OnChangeSettings(TiXmlElement* xmlnode)
{
	CMutexLock lock(m_GeneralMutex);
	TiXmlElement* elem = xmlnode->FindOrCreateChildElement("video");

	CStr texdetail = elem->FindOrCreateAttribute("texturedetail", "0");

	int idetail = enf_limit(texdetail.ToInt(), 0, 2);

#ifdef ENF_DONE
	if(SetTextureDetail(idetail) != idetail)
	{
		CMutexLock lock2(m_TexturesMutex);
		//reload all textures
		for(uint t = 0; t < m_Textures.GetCardinality(); t++)
		{
			Texture* tex = m_Textures[t];
			if(tex->IsFlagSet(TEXTR_NORESIZE|TEXTR_NOMIPMAP))
				continue;
			tex->SetFlags(TEXTR_RELOAD);
			m_bReloadTextures = true;
		}
	}

	CStr modelDetail = elem->FindOrCreateAttribute("modeldetail", "0");
	if(!modelDetail.IsEmpty())
	{
		int mdetail = enf_limit(modelDetail.ToInt(), 0, 2);
		MeshObject::SetMinLoadedLOD(mdetail);
	}
#endif
	return true;
}

//-----------------------------------------------------------------------
void IResourceManager::OnInsert(TiXmlElement* xmlnode)
{
	OnChangeSettings(xmlnode);
}

//-----------------------------------------------------------------------
void IResourceManager::ReloadTextures()
{
	CMutexLock lock(m_TexturesMutex);
#ifdef ENF_DONE
	for(uint t = 0; t < m_Textures.GetCardinality(); t++)
	{
		Texture* tex = m_Textures[t];
		if(tex->IsLoading())
		{
			continue;
		}
		if(!tex->IsFlagSet(TEXTR_RELOAD))
			continue;

		tex->ClearFlags(TEXTR_RELOAD);
		//find texture file name
		char tempName[MAX_FILENAME_LENGTH];
		if(!GetPhysicalTextureName(tex->GetName(), tempName))
		{
			//not created from file? Ignore it
			continue;
		}

		if(tex->GetFlags() & TEXTR_MANAGED)
		{
			LoadTextureResource* ld = ENF_NEW LoadTextureResource(tex, tex->GetFlags(), tex->GetUsage(), 0);

			ld->SetName(tempName);

			ComPtr<Texture> tmptex = CreatePlaceholderTexture(tex->GetName().cstr(), tex->GetFlags(),tex->GetUsage());
			tmptex->Swap(tex);

			AddLoadResource(ld);
		}
		else
		{
			ReloadTexture(tex);
		}
	}
#endif
}


//-----------------------------------------------------------------------
void IResourceManager::SetReloadFlag()
{
#ifdef ENF_DONE
	//set flags to textures which has to be reloaded
	for(uint t = 0; t < m_Textures.GetCardinality(); t++)
	{
		m_Textures[t]->SetFlags(TEXTR_RELOAD);
	}
#endif
}

#ifdef STRESSTEST
const char* allxob[] = {
#include "allxob.h"
};
const char* alldds[] = {
#include "alldds.h"
};
#endif
//-----------------------------------------------------------------------
void IResourceManager::Update(uint64 starttime, uint maxMilliseconds)
{
	AssertMainThread();
//	Timer::PushTime();

#ifdef STRESSTEST
	//////
	//stress-test
	static int xobindex = 0;
	static int ddsindex = 0;
	static bool dostress = false;


	if(dostress)
	{
		static bool doeatmem = false;
		if(!doeatmem)
		{
			doeatmem = true;
			MemoryManager::SetAllocationLimit(300);
			MemoryManager::Alloc(ENF_MEGABYTE(200), __FILE__, __LINE__);
		}
		for(int n = 0; n < 3; n++)
		{
			ComPtr<ObjectResource> xobptr = GetResourceObject(allxob[xobindex++], VLF_PRELOAD);
			if(xobindex == sizeof(allxob)/sizeof(allxob[0]))
				xobindex = 0;
			ComPtr<Texture> ddsptr = LoadAnyTexture(alldds[ddsindex++], TEXTR_MANAGED, MAP_DIFFUSE, 0);
			if(ddsindex == sizeof(alldds)/sizeof(alldds[0]))
				ddsindex = 0;
		}
	}
#endif

	{
		if(ENF_DIAG_VALUE(DM_STATS_STRESSMEM))
		{
			ENF_DIAG_SET(DM_STATS_STRESSMEM, 0);
			FreeManagedMemory(ENF_MEGABYTE(16));
		}

		if(ENF_DIAG_VALUE(DM_STATS_FLUSHMEM))
		{
			ENF_DIAG_SET(DM_STATS_FLUSHMEM, 0);
			Flush(0);
		}

		size_t freeMem = MemoryManager::GetFreeMemory();

#ifdef ENF_DONE
		uint freeMemTreshold = GetEngine()->IsLoading() ? ENF_MEGABYTE(MIN_LOAD_FREEMEM_TRESHOLD) : ENF_MEGABYTE(MIN_FREEMEM_TRESHOLD);
		uint relMem = GetEngine()->IsLoading() ? ENF_MEGABYTE(MIN_LOAD_RELEASE_MEM) : ENF_MEGABYTE(MIN_RELEASE_MEM);

		if(freeMem < freeMemTreshold)
		{
			FreeManagedMemory(relMem - freeMem);
		}
#endif
	}
	CMutexLock lock(m_LoadedMutex);

#if defined(ENF_DEVELOPER) && defined(WIN32)
	if(_GetAsyncKeyState(SysKey::LSHIFT) & 0x8000 && _GetAsyncKeyState(SysKey::Fn + 9) & 0x8000)
	{
		SetReloadFlag();
		m_bReloadTextures = true;
	}
#endif

	if(m_bReloadTextures)
	{
		m_bReloadTextures = false;
		ReloadTextures();
	}

	ResourceRef::m_iCurrentFrame = IL_INCREMENT(m_iLastFrame);

	m_iTimeStamp = starttime;
	if(!m_LoadedResources.IsEmpty())
	{
		ENF_PROFILE(PROFILE_RESOURCEMANAGER_UPDATE);
		ListIterator<LoadResource> lditer(m_LoadedResources);
		LoadResource* ldres = lditer.GetNext();

		while(ldres)
		{
			size_t freeMem = MemoryManager::GetFreeMemory();

			uint freeMemTreshold = GetEngine()->IsLoading() ? ENF_MEGABYTE(MIN_LOAD_FREEMEM_TRESHOLD) : ENF_MEGABYTE(MIN_FREEMEM_TRESHOLD);
			uint relMem = GetEngine()->IsLoading() ? ENF_MEGABYTE(MIN_LOAD_RELEASE_MEM) : ENF_MEGABYTE(MIN_RELEASE_MEM);

			if(freeMem < freeMemTreshold)
			{
				FreeManagedMemory(relMem - freeMem);
				//next round, please
				break;
			}

			ldres->Apply();
			//make it actual
			ldres->m_pResource->Touch(0);

			LoadResource* next = lditer.HasMoreElements() ? lditer.GetNext() : NULL;

			if(ldres->HasType(LoadVObjectResourceTypeID))
				IL_DECREMENT(m_iNumLoadingObjects);
			else if(ldres->HasType(LoadTextureResourceTypeID))
				IL_DECREMENT(m_iNumLoadingTextures);

			m_LoadedResources.Remove(ldres);
			IL_EXCHANGE_ADD(m_iLoaded, -ldres->m_iLoadedSize);
			delete ldres;


			ldres = next;
			TimeStamp diff = Timer::Sub(Timer::GetTicks(), starttime);

			if(diff > Timer::Time2Tick(maxMilliseconds, Timer::RESOLUTION_MILLISECONDS))
				break;
		}
	}

//	int tookMs = Timer::PopTime(Timer::RESOLUTION_MILLISECONDS);
//	if(tookMs > maxMilliseconds)
//		LogF(LL_NORMAL, "ResourceManager took %d instead of %d", tookMs, (int)maxMilliseconds);
}

//-----------------------------------------------------------------------
ResourceManager* CreateResourceManager(Renderer* device)
{
	return ENF_NEW IResourceManager(device);
}

//-----------------------------------------------------------------------
ResourceManager::ResourceManager(Renderer* device)
{
}

//-----------------------------------------------------------------------------
ResourceManager::ResourceManager()
{
}

//-----------------------------------------------------------------------------
ResourceManager::~ResourceManager()
{
}

//-----------------------------------------------------------------------
IResourceManager::IResourceManager(Renderer* device):
	m_pDevice(device),
	m_iTimeStamp(0),
	m_iLastFrame(0),
	m_pLoaderThread(NULL),
	m_QueueEvent(0, 65536),
	m_iLoaded(0),
	m_iDisableLoad(0),
	m_pImageLoader(NULL),
	m_bReloadTextures(false),
	m_bWait4Textures(true),
	m_iNumLoadingObjects(0),
	m_iNumLoadingTextures(0),
	m_iDetail(0)
{
	Init();
	MemoryManager::RegisterFlushableObject(this);

#ifdef ENF_DONE
	m_pDiffusePlaceholder = m_pDevice->CreateTextureWithFormat(NULL, 1, 1, TEXFMT_A8R8G8B8, 0, MAP_DIFFUSE);
	m_pDiffusePlaceholder->FillColor(0x007f7f7f, 0, 0, 1, 1);
	m_pNormalPlaceholder = m_pDevice->CreateTextureWithFormat(NULL, 1, 1, TEXFMT_A8R8G8B8, 0, MAP_NORMAL);
	m_pNormalPlaceholder->FillColor(0xff7f7fff, 0, 0, 1, 1);
	m_pNormalDXTPlaceholder = m_pDevice->CreateTextureWithFormat(NULL, 1, 1, TEXFMT_A8R8G8B8, 0, MAP_NORMAL_DXT);
	m_pNormalDXTPlaceholder->FillColor(0x7f007f00, 0, 0, 1, 1);
	m_pBlackPlaceholder = m_pDevice->CreateTextureWithFormat(NULL, 1, 1, TEXFMT_A8R8G8B8, 0, MAP_UNKNOWN);
	m_pBlackPlaceholder->FillColor(0x00000000, 0, 0, 1, 1);
	
	#ifdef ENF_PS3
		m_pFloatPlaceholder = m_pDevice->CreateTextureWithFormat(NULL, 1, 1, TEXFMT_R32F, TEXTR_VERTEXTEXTURE, MAP_UNKNOWN);
		m_pFloatPlaceholder->FillColor(0x00000000, 0, 0, 1, 1);
	#endif
#endif
}

//-----------------------------------------------------------------------------
IResourceManager::IResourceManager():
	m_pDevice(NULL),
	m_iTimeStamp(0),
	m_iLastFrame(0),
	m_pLoaderThread(NULL),
	m_QueueEvent(0, 4096),
	m_iLoaded(0),
	m_iDisableLoad(0),
	m_pImageLoader(NULL),
	m_bReloadTextures(false),
	m_bWait4Textures(true),
	m_iNumLoadingObjects(0),
	m_iNumLoadingTextures(0)
{
	Init();
	MemoryManager::RegisterFlushableObject(this);
}


//-----------------------------------------------------------------------------
bool IResourceManager::AddTexture(Texture* tex)
{
	CMutexLock lock(m_TexturesMutex);
#ifdef ENF_DONE
  if(tex->m_pManager != NULL)
	{
		//already managed by this manager?
		if(tex->m_pManager == this)
			return false;

		//remove from other manager
		tex->m_pManager->RemoveTexture(tex);
	}

	m_Textures.Insert(tex);
	tex->m_pManager = this;

	//don't hash unnamed textures
	if(tex->m_strName != NULL)
		m_TextureHashList.Insert(tex->m_strName.cstr(), tex);
#endif
	return true;
}

//-----------------------------------------------------------------------
bool IResourceManager::RemoveTexture(Texture* tex)
{
	CMutexLock lock(m_TexturesMutex);
#ifdef ENF_DONE
  //not managed, or not managed by this manager
	if(tex->m_pManager == NULL || tex->m_pManager != this)
		return false;

	m_Textures.Remove(tex);

	enf_assert(!tex->IsLoading());

	if(tex->m_strName != NULL)
		m_TextureHashList.Remove(tex->m_strName.cstr());
#endif
	return true;
}


//-----------------------------------------------------------------------
size_t IResourceManager::FlushCache()
{
	size_t flushedMem = 0;
	uint flushed = 0;

	{
		CMutexLock objLock(m_DisposableObjectsMutex);

		//do it twice, as some vobjects references another ones (AnimSet->Animation)
		for(uint c = 0; c < 2; c++)
		{
			for(uint n = 0; n < m_DisposableObjects.GetCardinality();)
			{
				ObjectResource* obj = m_DisposableObjects[n];

				if(obj->RefCount() == 0)
				{
					if(!obj->GetName().IsEmpty())
						m_DisposableObjectHashList.Remove(obj->GetName().cstr());

					m_DisposableObjects.RemoveElement(n);
					flushedMem += obj->GetSize();
					obj->Destroy();
					flushed++;
					continue;
				}
				n++;
			}
		}
	}

	if(m_TexturesMutex.TryLock())
	{
#ifdef ENF_DONE
		for(uint n = 0; n < m_Textures.GetCardinality();)
		{
			Texture* tex = m_Textures[n];
			size_t texMemSize = tex->GetMemorySize();

			if(tex->Flush())
			{
				flushedMem += texMemSize;
				flushed++;
			}
			else
				n++;
		}
#endif
		m_TexturesMutex.Unlock();
	}
	return flushedMem;
}

//-----------------------------------------------------------------------
void FreeObjects(Set<ObjectResource*>& objects)
{
	for(uint n = 0; n < objects.GetCardinality(); n++)
	{
		if(objects[n]->HasType(AnimsetTypeID))
		{
			enf::LogF(LL_ERROR, "%s   %d", objects[n]->GetName().cstr(), objects[n]->RefCount());
			objects[n]->Destroy();
			objects[n] = NULL;
		}
	}

	for(uint n = 0; n < objects.GetCardinality(); n++)
	{
		if(objects[n])
		{
			enf::LogF(LL_ERROR, "%s   %d", objects[n]->GetName().cstr(), objects[n]->RefCount());
			objects[n]->Destroy();
		}
	}
	objects.Clear();
}

//-----------------------------------------------------------------------
void ShowTitle(bool& title)
{
	if(!title)
	{
		title = true;
		enf::LogF(LL_ERROR, "==== Resource leaks ====");
	}
}

//-----------------------------------------------------------------------
void IResourceManager::OnEvent(const Event& event)
{
	if(event.HasType(TerminationEventTypeID))
	{
		CMutexLock lock(m_GeneralMutex);
		if(m_pLoaderThread)
		{
			m_pLoaderThread->Kill();
			delete m_pLoaderThread;
			m_pLoaderThread = NULL;
		}
	}
}

//-----------------------------------------------------------------------
IResourceManager::~IResourceManager()
{
	MemoryManager::UnregisterFlushableObject(this);
	FlushCache();

	CMutexLock lock(m_GeneralMutex);
	if(m_pLoaderThread)
	{
		m_pLoaderThread->Kill();
		delete m_pLoaderThread;
		m_pLoaderThread = NULL;
	}
	
	if(!m_LoadResources.IsEmpty())
	{
		ListIterator<LoadResource> lditer(m_LoadResources);

		LoadResource* ldres = lditer.GetNext();

		while(ldres)
		{
			LoadResource* next = lditer.HasMoreElements() ? lditer.GetNext() : NULL;

			m_LoadResources.Remove(ldres);
			delete ldres;
			ldres = next;
		}
	}

	if(!m_LoadedResources.IsEmpty())
	{
		ListIterator<LoadResource> lditer(m_LoadedResources);

		LoadResource* ldres = lditer.GetNext();

		while(ldres)
		{
			LoadResource* next = lditer.HasMoreElements() ? lditer.GetNext() : NULL;

			m_LoadedResources.Remove(ldres);
			IL_EXCHANGE_ADD(m_iLoaded, -ldres->m_iLoadedSize);
			delete ldres;
			ldres = next;
		}
	}

	bool title = false;

	{
		CMutexLock lock(m_ObjectsMutex);
		if(!m_Objects.IsEmpty())
		{
			ShowTitle(title);
			FreeObjects(m_Objects);
		}
	}
	{
		CMutexLock lock(m_DisposableObjectsMutex);
		if(!m_DisposableObjects.IsEmpty())
		{
			ShowTitle(title);
			FreeObjects(m_DisposableObjects);
		}
	}

	{
		CMutexLock lock(m_TexturesMutex);

		if(!m_Textures.IsEmpty())
		{
			ShowTitle(title);
#ifdef ENF_DONE
			while(!m_Textures.IsEmpty())
			{
				Texture *tex;

				tex = m_Textures[0];
				if(tex->RefCount())
				{
					enf::LogF(LL_ERROR, "%s   %d", tex->GetName().cstr(), tex->RefCount());
					while(tex->Release());
				}
				tex->Flush();
			}
#endif
		}
	}

	for(uint n = 0; n < m_Loaders.GetCardinality(); n++)
	{
		m_Loaders[n].Data->Release();
	}
	m_Loaders.Clear();

#ifdef ENF_DONE
	if(m_pImageLoader)
	{
		delete m_pImageLoader;
		m_pImageLoader = NULL;
	}
#endif
	if(g_pResources == this)
		g_pResources = NULL;
}

//-----------------------------------------------------------------------------
Texture* IResourceManager::LoadAnyTexture(CStr strName, uint dwFlags, TextureUsage tusage, int loadPriority)
{
	enf_assert(!strName.IsNull());
	if(strName.IsEmpty())
   	return NULL;

const char *dptr = strName.strrchr('.');

	char name[MAX_FILENAME_LENGTH];
	int nlen;

	if(dptr)
		nlen = (int)(dptr - strName.cstr());
	else
		nlen = strName.Length();

	CopyMem(name, strName.cstr(), nlen);
	name[nlen] = '\0';

	CMutexLock lock(m_TexturesMutex);
	return LoadAnyTextureInternal(name, nlen, dwFlags, tusage, loadPriority);
}

//-----------------------------------------------------------------------------
bool ResourceManager::GetPhysicalTextureName(CStr texname, char* resname)
{
	enf_assert(!texname.IsEmpty());
	enf_assert(resname != NULL);
#ifdef ENF_DONE
	strcpy(resname, texname.cstr());
	int nlen = strlen(resname);

	const char** strTexExt = GetImageLoader()->GetExtensionsStringArray();
	for(int n = 0; strTexExt[n]; n++)
	{			
		strcpy(resname + nlen, strTexExt[n]);
		
		bool lres = (GetFileSystem()->FileExists(resname) != 0);

		if(lres)
			return true;
	}
#endif
	resname[0] = 0;
	return false;
}

//-----------------------------------------------------------------------------
Texture* IResourceManager::CreatePlaceholderTexture(const char* name, uint dwFlags, TextureUsage tusage)
{
	Texture* placeHolder = NULL;
#ifdef ENF_DONE
	if((dwFlags & TEXTR_CUBE) == 0)
	{
		#ifdef ENF_PS3
			if (dwFlags & TEXTR_VERTEXTEXTURE)
			{
				placeHolder = m_pFloatPlaceholder;
			}
			else
		#endif

		if(tusage == MAP_DIFFUSE)
		{
			placeHolder = m_pDiffusePlaceholder;
		}
		else if(tusage == MAP_NORMAL)
		{
			placeHolder = m_pNormalPlaceholder;
		}
		else if(tusage == MAP_NORMAL_DXT)
		{
			placeHolder = m_pNormalDXTPlaceholder;
		}
		else
		{
			placeHolder = m_pBlackPlaceholder;
		}
	}
	else
	{
		FColor color;
	
		switch(tusage)
		{
		case MAP_DIFFUSE:
			color = FColor(0.5f, 0.5f, 0.5f, 0.0f);
			break;
		case MAP_NORMAL:
			color = FColor(1.0f, 0.5f, 0.5f, 1.0f);
			break;
		case MAP_NORMAL_DXT:
			color = FColor(0.0f, 0.5f, 0.0f, 0.5f);
			break;
		default:
			color = FColor(0.0f, 0.0f, 0.0f, 0.0f);
			break;
		}
		placeHolder = GetMaterialSystem()->GetIdentityColorMap(color, dwFlags);
	}

	placeHolder = m_pDevice->CreateTexture(name, placeHolder, dwFlags & ~TEXTR_MANAGED);
#endif
	return placeHolder;
}

//-----------------------------------------------------------------------------
Texture* IResourceManager::LoadAnyTextureInternal(char* name, int nlen, uint dwFlags, TextureUsage tusage, int loadPriority)
{
	enf_assert(name);
#ifdef ENF_DONE
ComPtr<Texture> tex;
#ifdef ENF_FORCE_SYNC
	dwFlags &= ~TEXTR_MANAGED;
#endif

//cannot create directly from other threads
//#ifndef ENF_XENON
	if((dwFlags & TEXTR_MANAGED) == 0 && !CheckMainThread())
		dwFlags |= TEXTR_MANAGED;
//#endif

	const char** strTexExt = GetImageLoader()->GetExtensionsStringArray();
	if(dwFlags & TEXTR_MANAGED)
	{
		VFile* file = NULL;

		for(int n = 0; strTexExt[n]; n++)
		{			
			strcpy(name + nlen, strTexExt[n]);
			
			file = GetFileSystem()->Open(name, FILEMODE_READ|FILEFLAGS_NO_PREREAD);

			if(file)
				break;
		}

		if(file == NULL)
			return NULL;

		name[nlen] = '\0';
		enf_assert(tex.IsNull());

		tex = CreatePlaceholderTexture(name, dwFlags, tusage);
		tex->SetPriority(loadPriority);
		tex->SetFileHandle(file->GetHandle());
		file->Close();

		LoadTextureResource* ld = ENF_NEW LoadTextureResource(tex, dwFlags, tusage, loadPriority);

		AddLoadResource(ld);
		AddManagedResource(tex);
	}
	else
	{
		for(int n = 0; strTexExt[n]; n++)
		{			
			strcpy(name + nlen, strTexExt[n]);
			
			VFile* file = GetFileSystem()->Open(name, FILEMODE_READ);

			if(file)
			{
				name[nlen] = '\0';

				CFileInputStream inStream(file);
				tex = m_pDevice->CreateTextureFromImage(name, inStream, dwFlags);
				break;
			}
		}
	}
	tex.AddRef();
	return tex;
#endif
  return NULL;
}

//-----------------------------------------------------------------------------
Texture *LoadTexture(Renderer *dev, CStr strName, uint dwFlags, TextureUsage usage)
{
	if(strName == NULL)
   	return NULL;
#ifdef ENF_DONE
	char tname[MAX_FILENAME_LENGTH];

	CStr ext = ResourceManager::FixName(strName, tname);

	VFile* vf = GetFileSystem()->Open(tname, FILEMODE_READ);

	if(vf)
	{
		CFileInputStream inStream(vf);
		return dev->CreateTextureFromImage(tname, inStream, dwFlags);
	}
#endif
	return NULL;
}

//-----------------------------------------------------------------------------
Texture* IResourceManager::GetTexture(CStr strName)
{
	char tname[MAX_FILENAME_LENGTH];
	ResourceManager::FixName(strName, tname);

	CMutexLock lock(m_TexturesMutex);
	return GetTextureInternal(tname);
}

//-----------------------------------------------------------------------------
Texture* IResourceManager::GetTextureInternal(const char* strName)
{
	Texture* tex = m_TextureHashList.Find(strName);
#ifdef ENF_DONE
  if(tex)
	{
		//was disposed
		if(tex->AddRef() == 1)
		{
			MemStats::m_iDisposedTextures -= tex->GetSize();
		}
	}
#endif
	return tex;
}

//-----------------------------------------------------------------------------
ebool IResourceManager::ReloadTexture(Texture* tex)
{
	CMutexLock lock(m_TexturesMutex);
#ifdef ENF_DONE
	if(tex->m_pManager != this)
		return false;

	if(tex->GetName().IsNull() || tex->GetName().IsEmpty())
		return false;

	SString<> tname = tex->GetName();
	ComPtr<Texture> tc = LoadAnyTextureInternal(tname.First(), tname.Length(), tex->GetFlags() & ~TEXTR_MANAGED, MAP_UNKNOWN, 0);

	if(tc.IsNull())
		return false;

	enf_assert(!tex->IsLoading());
	tex->Swap(tc);
#endif
	return true;
}

//-----------------------------------------------------------------------------
Texture	*IResourceManager::CreateTextureFromFile(CStr strName, uint dwFlags, TextureUsage usage, int loadPriority)
{
	enf_assert(!strName.IsNull());

	SString<> name;
	FixName(strName, name.First());
	name.StripExt();

	CMutexLock lock(m_TexturesMutex);
	Texture* tc = GetTextureInternal(name.cstr());

	if(tc == NULL)
	{
		tc = LoadAnyTextureInternal(name.First(), name.Length(), dwFlags, usage, loadPriority);

		if(tc)
		{
			AddTexture(tc);
		}
	}

	return tc;
}
//-----------------------------------------------------------------------
bool IResourceManager::AddVObjectInternal(ObjectResource* obj)
{
	enf_assert(obj != NULL);
	enf_assert(obj->m_pManager == NULL);

	if(!m_Objects.Insert(obj))
		return false;

	if(!obj->GetName().IsEmpty())
	{
		m_ObjectHashList.Insert(obj->GetName().cstr(), obj);
	}

	obj->m_pManager = this;
	return true;
}

//-----------------------------------------------------------------------
bool IResourceManager::AddVObject(ObjectResource* obj)
{
	CMutexLock lock(m_ObjectsMutex);
	return AddVObjectInternal(obj);
}

//-----------------------------------------------------------------------
void IResourceManager::RemoveVObject(ObjectResource* obj)
{
	enf_assert(obj != NULL);
	enf_assert(obj->m_pManager == this);
	enf_assert(!obj->IsLoading());

	bool removed = false;
	CMutexLock lock(m_DisposableObjectsMutex);
	CMutexLock lock2(m_ObjectsMutex);

	removed = m_Objects.Remove(obj);
	if(removed)
	{
		if(!obj->GetName().IsEmpty())
			m_ObjectHashList.Remove(obj->GetName().cstr());
	}
	else
	{
		if(m_DisposableObjects.Remove(obj))
		{
			if(!obj->GetName().IsEmpty())
			{
				m_DisposableObjectHashList.Remove(obj->GetName().cstr());
			}
		}
	}
	obj->m_pManager = NULL;
}

//-----------------------------------------------------------------------
void IResourceManager::DisposeVObject(ObjectResource* obj)
{
	CMutexLock lock(m_DisposableObjectsMutex);
	CMutexLock lock2(m_ObjectsMutex);
	enf_assert(obj != NULL);
	enf_assert(obj->m_pManager == this);
	enf_assert(!obj->IsLoading());

	if(!obj->GetName().IsEmpty())
	{
		m_ObjectHashList.Remove(obj->GetName().cstr());
		m_DisposableObjectHashList.Insert(obj->GetName().cstr(), obj);
	}
	m_Objects.Remove(obj);
	m_DisposableObjects.Insert(obj);
}

//-----------------------------------------------------------------------
CStr ResourceManager::FixName(CStr in, char* out)
{
	//fix bad names

	const char* p = in.cstr();

	const char* ext = NULL;
	char chr;
	bool backslashes = false;
	do
	{
		chr = *p++;
#ifdef ENF_DEVELOPER
		if(chr == '\\')
		{
			if(!backslashes)
			{
				enf::LogF(LL_ERROR, "Bad resource name '%s' contains backslashes!", in.cstr());
				//enf_assert(!"Bad resource name. Must not contain backslashes!");
				backslashes = true;
			}
			chr = '/';
		}
		else 
#endif
		if(chr == '.')
			ext = out + 1;
		else if(chr >= 'A' && chr <= 'Z')
			chr |= 0x20;
			//chr = tolower(chr);

		*out++ = chr;

	} while(chr);

	return ext;
}

//-----------------------------------------------------------------------
void IResourceManager::RegisterResourceLoader(CStr ext, ResourceLoader* loader)
{
	CMutexLock lock(m_GeneralMutex);
	enf_assert(m_Loaders.GetIndexOf(TLoaderItem(ext, NULL)) == INDEX_NOT_FOUND);
	m_Loaders.Insert(TLoaderItem(ext, loader));
}

//-----------------------------------------------------------------------
void IResourceManager::UnregisterResourceLoader(ResourceLoader* loader)
{
	CMutexLock lock(m_GeneralMutex);
	for(uint n = 0; n < m_Loaders.GetCardinality(); n++)
	{
		if(m_Loaders[n].Data == loader)
		{
			m_Loaders[n].Data->Release();
			m_Loaders.RemoveElement(n);
			return;
		}
	}
}

//-----------------------------------------------------------------------
ObjectResource* IResourceManager::GetResourceObject(CStr name, uint flags)
{
	enf_assert(name != NULL);


ObjectResource *obj = NULL;

#ifdef ENF_FORCE_SYNC
	flags &= ~VLF_PRELOAD;
#endif

	char tname[MAX_FILENAME_LENGTH];
	CStr ext = FixName(name, tname);

	if(!ext.IsEmpty())
	{
		m_DisposableObjectsMutex.Lock();
		m_ObjectsMutex.Lock();
		//Try to find object in disposable cache
		if((obj = m_DisposableObjectHashList.Find(tname)) != NULL)
		{
			m_DisposableObjectHashList.Remove(tname);
			m_DisposableObjects.Remove(obj);
			m_ObjectHashList.Insert(obj->GetName().cstr(), obj);
			m_Objects.Insert(obj);
		}
		else
		{
			//Try to find object in cache
			obj = m_ObjectHashList.Find(tname);
		}

		if(obj)
		{
			obj->m_bFlush = false;
			obj->AddRef();

			obj->Touch(0);
			m_ObjectsMutex.Unlock();
			m_DisposableObjectsMutex.Unlock();
			return obj;
		}
		m_DisposableObjectsMutex.Unlock();

		VFile* vf = GetFileSystem()->Open(tname, FILEMODE_READ | (((flags & VLF_PRELOAD) != 0) ? FILEFLAGS_NO_PREREAD : 0) );
		if(vf != NULL)
		{
			uint ld = m_Loaders.GetIndexOf(TLoaderItem(ext, NULL));
			if(ld != INDEX_NOT_FOUND)
			{
				obj = m_Loaders[ld].Data->CreateResource();
			}

			if(obj == NULL)
			{
				vf->Close();
				if(!(flags & VLF_NO_LOG_ERRORS))
					enf::LogF(LL_ERROR, "Unknown object type %s", tname);
				m_ObjectsMutex.Unlock();
				return NULL;
			}

			obj->SetName(tname);
			obj->Touch(0);

			bool res = false;
			LoadResource* loader = NULL;
			if(flags & VLF_PRELOAD)
			{
				//TODO: must be set prior to CreateResourceLoader. Make it better
				obj->SetFileHandle(vf->GetHandle());
				obj->SetPriority(RESPRI_IMMEDIATE);
				loader = obj->CreateResourceLoader();
				if(loader)
				{
					AddLoadResource(loader);
					vf->Close();
					res = true;
				}
				else
				{
					obj->SetFileHandle(BAD_FILE_HANDLE);
				}
			}
			
			if(loader == NULL)
			{
				CFileInputStream in(vf);

				//enf::LogF(LL_WARNING, "Not preloaded %s", tname);
				if(obj->Load(&in))
				{
					res = true;
				}
				else
				{
					obj->Destroy();
					obj = NULL;
				}
			}

			if(res == true)
			{
				AddVObjectInternal(obj);
				//AddManagedResource
			}
		}
		m_ObjectsMutex.Unlock();
	}

	if(obj == NULL && !(flags & VLF_NO_LOG_ERRORS))
	{
		enf::LogF(LL_ERROR, "Failed to open object %s", tname);
	}

   return obj;
}

//-----------------------------------------------------------------------
struct LRUItem
{
	ResourceRef*	ref;
	int				pri;
	int				type;
	size_t			size;
};

//-----------------------------------------------------------------------
class CompareResources
{
public:
	int operator()(LRUItem& ref0, LRUItem& ref1) const
	{
		if(ref0.pri < ref1.pri)
			return -1;
		if(ref0.pri > ref1.pri)
			return 1;
		uint age0 = ref0.ref->GetAge();
		uint age1 = ref1.ref->GetAge();
		if(age0 > age1)
			return -1;
		if(age0 < age1)
			return 1;
		if(ref0.size > ref1.size)
			return -1;
		if(ref0.size < ref1.size)
			return 1;
		return 0;
	}
};

//-----------------------------------------------------------------------
void FillObjects(LRUItem* refs, const Set<ObjectResource*>& objects, uint& numRefs, uint arraySize, int type)
{
	for(uint n = 0; n < objects.GetCardinality() && numRefs < arraySize; n++)
	{
		ObjectResource* res = objects[n];
		if(res->IsLoading())
			continue;
		if(res->RefCount() > 0)
			continue;
		if(res->GetAge() <= 1)
			continue;
		size_t size = res->GetSize();
		//would be nice, but we don't want to fragment memory
		//if(size <= WORTH2FREE_SIZE)
		//	continue;
		//cannot use priority, it's RESPRI_IMMEDIATE to be loaded before textures, but we need to free it in reverse order
		refs[numRefs].pri = /*m_Objects[n]->GetPriority() +*/ RESPRI_LOWEST;
		refs[numRefs].size = size;
		refs[numRefs].type = type;
		refs[numRefs++].ref = res;
	}
}

//-----------------------------------------------------------------------
size_t IResourceManager::FreeManagedMemory(size_t size)
{
	size_t freeMemory = 0;

	bool managedLocked = false;
	bool objectsLocked = false;

	uint arraySize = 0;

	CMutexLock lock(m_DisposableObjectsMutex);

	if(m_ManagedMutex.TryLock())
	{
		managedLocked = true;
		arraySize += m_ManagedResources.GetCardinality();
	}
	if(m_ObjectsMutex.TryLock())
	{
		objectsLocked = true;
		arraySize += m_Objects.GetCardinality();
	}

	arraySize += m_DisposableObjects.GetCardinality();

	if(arraySize > 0)
	{
		static const size_t WORTH2FREE_SIZE = 8 * 1024;

		arraySize += 256; //reserve

		LRUItem* refs = ALLOCA(LRUItem, arraySize);
		uint numRefs = 0;

		FillObjects(refs, m_DisposableObjects, numRefs, arraySize, 3);

		if(objectsLocked)
		{
			FillObjects(refs, m_Objects, numRefs, arraySize, 1);
		}

		if(managedLocked)
		{
			for(uint n = 0; n < m_ManagedResources.GetCardinality() && numRefs < arraySize; n++)
			{
				ResourceRef* ref = m_ManagedResources[n];
				if(ref->IsLoading())
					continue;
				if(ref->GetAge() <= 1)
					continue;
				size_t size = ref->GetSize();
				//would be nice, but we don't want to fragment memory
				//if(size <= WORTH2FREE_SIZE)
				//	continue;

				refs[numRefs].pri = ref->GetPriority();
				refs[numRefs].size = size;
				refs[numRefs].type = 2;
				refs[numRefs++].ref = ref;
			}
		}
		QuickSortCmp<LRUItem, CompareResources>(numRefs, refs);

		for(uint n = 0; n < numRefs && freeMemory < size; n++)
		{
			if(refs[n].type == 1)
			{
				ObjectResource* obj = static_cast<ObjectResource*>(refs[n].ref);
				//someone has referenced it meanwhile?
				if(obj->RefCount() != 0)
					continue;

				uint index = m_Objects.GetIndexOf(obj);
				if(index == INDEX_NOT_FOUND)
					continue;

				if(!obj->GetName().IsEmpty())
					m_ObjectHashList.Remove(obj->GetName().cstr());

				m_Objects.RemoveElement(index);
				freeMemory += obj->GetSize();
				obj->Destroy();
			}
			else if(refs[n].type == 3)
			{
				ObjectResource* obj = static_cast<ObjectResource*>(refs[n].ref);
				//someone has referenced it meanwhile?
				if(obj->RefCount() != 0)
					continue;

				uint index = m_DisposableObjects.GetIndexOf(obj);
				if(index == INDEX_NOT_FOUND)
					continue;

				if(!obj->GetName().IsEmpty())
					m_DisposableObjectHashList.Remove(obj->GetName().cstr());

				m_DisposableObjects.RemoveElement(index);
				freeMemory += obj->GetSize();
				obj->Destroy();
			}
			else
			{
				freeMemory += refs[n].ref->Flush(0);
			}
		}
		if(objectsLocked)
			m_ObjectsMutex.Unlock();
		if(managedLocked)
			m_ManagedMutex.Unlock();
	}

	LogF(LL_DEBUG, "Flushed %d bytes", freeMemory);

	#if defined(ENF_PS3) && defined(ENF_DEVELOPER)
		static OutOfMemCounter OutOfMemory(OutOfMemCounter::MT_RESOURCE, 0, 30000.0f);
		OutOfMemory.Next(freeMemory);
	#endif

	return freeMemory;
}

//-----------------------------------------------------------------------------
void IResourceManager::GetInfo(ResourceInfo& info)
{
	{
		CMutexLock lock(m_ObjectsMutex);
		info.m_iNumObjects = m_Objects.GetCardinality();
	}
	{
		CMutexLock lock(m_DisposableObjectsMutex);
		info.m_iNumDisposableObjects = m_DisposableObjects.GetCardinality();
	}
	{
		CMutexLock lock(m_TexturesMutex);
		info.m_iNumTextures = m_Textures.GetCardinality();
	}
	info.m_iLoadedBytes = m_iLoaded;

	{
		CMutexLock lock(m_LoadMutex);
		info.m_iLoadingResources = m_LoadResources.GetCardinality();
	}

	{
		CMutexLock lock(m_LoadedMutex);
		info.m_iLoadedResource = m_LoadedResources.GetCardinality();
	}
}

//-----------------------------------------------------------------------------
class ANMResourceLoader: public ResourceLoader
{
public:
	//ENF_DECLARE_SMALLOBJECT(ANMResourceLoader);
	ANMResourceLoader() {}
	ObjectResource* CreateResource()
	{
		return ENF_NEW Animation();
	}
};

#ifdef ENF_DONE
class ASETResourceLoader: public ResourceLoader
{
public:
	ENF_DECLARE_SMALLOBJECT(ASETResourceLoader);
	ASETResourceLoader() {}
	ObjectResource* CreateResource()
	{
		return ENF_NEW Animset();
	}
};

class SPRResourceLoader: public ResourceLoader
{
public:
	ENF_DECLARE_SMALLOBJECT(SPRResourceLoader);
	SPRResourceLoader() {}
	ObjectResource* CreateResource()
	{
		return ENF_NEW Sprite();
	}
};

class XOBResourceLoader: public ResourceLoader
{
public:
	ENF_DECLARE_SMALLOBJECT(XOBResourceLoader);
	XOBResourceLoader() {}
	ObjectResource* CreateResource()
	{
		return ENF_NEW MeshObject();
	}
};

class PTCResourceLoader: public ResourceLoader
{
public:
	ENF_DECLARE_SMALLOBJECT(PTCResourceLoader);
	PTCResourceLoader() {}
	ObjectResource* CreateResource()
	{
		return ENF_NEW ParticleEffect();
	}
};

class ANNOResourceLoader: public ResourceLoader
{
public:
	ENF_DECLARE_SMALLOBJECT(ANNOResourceLoader);
	ANNOResourceLoader() {}
	ObjectResource* CreateResource()
	{
		return ENF_NEW LipsyncDef();
	}
};

DECLARE_ITEM_FACTORY(VSpriteInstance);
DEFINE_ITEM_FACTORY(VSpriteInstance);
DECLARE_ITEM_FACTORY(ParticleEffectInstance);
DEFINE_ITEM_FACTORY(ParticleEffectInstance);
DECLARE_ITEM_FACTORY(VObjectInstance);
DEFINE_ITEM_FACTORY(VObjectInstance);
DECLARE_ITEM_FACTORY(MeshObjectInstance);
DEFINE_ITEM_FACTORY(MeshObjectInstance);
#endif
//-----------------------------------------------------------------------------
void IResourceManager::Init()
{
	CMutexLock lock(m_GeneralMutex);
	RegisterResourceLoader("anm", ENF_NEW ANMResourceLoader());
#ifdef ENF_DONE
	RegisterResourceLoader("xob", ENF_NEW XOBResourceLoader());
	RegisterResourceLoader("ptc", ENF_NEW PTCResourceLoader());
	RegisterResourceLoader("aset", ENF_NEW ASETResourceLoader());
	RegisterResourceLoader("spr", ENF_NEW SPRResourceLoader());
	RegisterResourceLoader("anno", ENF_NEW ANNOResourceLoader());

	Register(ITEM_FACTORY(VSpriteInstance));
	Register(ITEM_FACTORY(ParticleEffectInstance));
	Register(ITEM_FACTORY(VObjectInstance));
	Register(ITEM_FACTORY(MeshObjectInstance));

	m_pImageLoader = CreateImageLoader();
#endif
	m_pLoaderThread = ENF_NEW ResourceLoaderThread(this);
	m_pLoaderThread->Start(4);
}

//-----------------------------------------------------------------------
ObjectResource::~ObjectResource()
{
	enf_assert(!IsLoading());
}

//-----------------------------------------------------------------------
bool ObjectResource::Reload(CInputStream* instream)
{
	//enf_assert2(m_pResourceManager != NULL, "Cannot reload unmanaged object");

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

	return true;
}

//-----------------------------------------------------------------------
void ObjectResource::Unload()
{
	//not used ATM
	//if(GetManager())
	//	GetManager()->RemoveManagedResource(this);
}

//-----------------------------------------------------------------------
int ObjectResource::Release(bool flush)
{
	//take unnamed, or unmanaged objects as force released, as they can't be referenced anyway
	flush = m_bFlush || flush || GetName().IsEmpty() || m_pManager == NULL;

	if(flush != m_bFlush)
		m_bFlush = flush;

	if(Unref() == 0)
	{
		if(flush)
		{
			if(m_pManager)
			{
				m_pManager->RemoveVObject(this);
			}
			Destroy();
		}
		else
		{
			if(m_pManager)
			{
				m_pManager->DisposeVObject(this);
			}
		}
		return 0;
	}

	return RefCount();
}

//-----------------------------------------------------------------------
void ObjectResource::SetName(CStr name)
{
	Item::SetName(name);

	const char* bname = CStr(m_strName.cstr()).strrchr('/');
	if(bname == NULL)
		m_strBaseName = m_strName.cstr();
	else
		m_strBaseName = bname + 1;
}


}