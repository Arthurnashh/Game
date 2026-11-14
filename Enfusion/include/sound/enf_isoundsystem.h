//-----------------------------------------------------------------------------
// File: enf_isoundsystem.h
//
// Desc: XSoundSystem common implementation
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_ISOUNDSYSTEM_H
#define ENF_ISOUNDSYSTEM_H

#include "enf_config.h"
#include "sound/enf_xsoundsystem.h"
#include "common/enf_hashmap.h"

namespace enf
{
/** @addtogroup XSoundSystem
@{*/
class XSound;
struct XSoundRef;

enum SoundType
{
	ST_NONE,
	ST_AMBIENT,
	ST_SPATIAL
};

class PUBLIC_API XSoundEnv: public Item
{
public:
	ENF_DECLARE_ITEM(XSoundEnv, Item);
	XSoundEnv(CStr name): Item()
	{
		SetName(name);
	}
};

const uint MAX_SOUNDS_BITS = 9;
const uint MAX_SOUNDS_MASK = (1 << MAX_SOUNDS_BITS) - 1;
const uint MAX_SPINCOUNT_BITS = 32 - MAX_SOUNDS_BITS;
const uint MAX_SPINCOUNT_MASK = (1 << MAX_SPINCOUNT_BITS) - 1;
const uint MAX_SOUNDS = 1 << MAX_SOUNDS_BITS;

struct XSoundRef
{
	XSoundRef*	ENF_LIST_PREV;
	XSoundRef*	ENF_LIST_NEXT;

	uint			m_iSpinCount:MAX_SPINCOUNT_BITS;
	uint			m_iSceneIndex:MAX_SOUNDS_BITS;
	XSound*		m_pSound;

	XSoundRef(): ENF_LIST_PREV(NULL), ENF_LIST_NEXT(NULL), m_iSpinCount(1), m_iSceneIndex(-1), m_pSound(NULL)
	{
	}
	void InvalidateHandle();
};

struct CommonFormat;

enum StreamStatus
{
	STS_END,
	STS_DATA,
	STS_NO_DATA
};

class XSoundStreaming
{
public:
	ENF_DECLARE_MMOBJECT(XSoundStreaming);
	XSoundStreaming()	{}
	virtual ~XSoundStreaming() {}
	virtual StreamStatus FillData(void* buffer, uint numSamples, bool loop, uint& readSamples)=0;
	virtual StreamStatus SubmitBuffer(XSoundSystem* sys, XSoundRef* soundRef, bool loop, uint userData)=0;
	virtual bool SetupFormat(CommonFormat& fmt) const=0;
	virtual bool IsStarted() const=0;
	virtual uint BufferSizeInMSecs() const { return 2000; }
};


class PUBLIC_API XSound
{
protected:
	uint							m_iIndex;
	uint							m_iFlags;
	uint							m_iShaderFlags;
	XSoundSource*				m_pSource;
	XSoundStreaming*			m_pStreaming;
	ComPtr<XSoundSample>		m_pSample;
	List<XSoundRef>*			m_pList;

public:
	ENF_DECLARE_SMALLOBJECT(XSound);

	void InsertToList(XSoundRef* ref, List<XSoundRef>* list)
	{
		enf_assert(m_pList == NULL);
		list->Insert(ref);
		m_pList = list;
	}
	void RemoveFromList(XSoundRef* ref)
	{
		m_pList->Remove(ref);
		m_pList = NULL;
	}

	virtual SoundType GetType() const
	{
		return ST_NONE;
	}
	void SetFlags(uint flags)
	{
		m_iFlags |= flags;
	}

	void ClearFlags(uint flags)
	{
		m_iFlags &= ~flags;
	}

	bool IsFlagsSet(uint flags) const
	{
		return ((m_iFlags & flags) != 0);
	}
	
	bool IsShaderFlagsSet(uint flags) const
	{
		return ((m_iShaderFlags & flags) != 0);
	}

	XSoundSample* GetSample() const
	{
		return m_pSample;
	}

	XSoundSource* GetSource() const
	{
		return m_pSource;
	}
	XSoundStreaming* GetStreaming() const
	{
		return m_pStreaming;
	}

	ENF_INLINE bool IsAmbient() const
	{
		return GetType() == ST_AMBIENT;
	}

	XSound(uint index, uint flags, uint shaderFlags): m_iIndex(index), m_iFlags(flags), m_iShaderFlags(shaderFlags), m_pSource(NULL), m_pStreaming(NULL), m_pList(NULL)
	{
	}
	virtual ~XSound()
	{
		SAFE_DELETE(m_pStreaming);
	}
};

//-----------------------------------------------------------------------------
class PUBLIC_API ISoundSystem: public XSoundSystem
{
private:
protected:
	friend class XSoundset;
	friend class XSoundShader;
	friend class XSoundSample;

	List<XSoundRef>		m_FreeSounds;

	struct XSoundScene
	{
		List<XSoundRef>	m_PlayingSounds;
		List<XSoundRef>	m_PlayingAmbientSounds;
		List<XSoundRef>	m_StoppedSounds;
	};

	XSoundScene*			m_pSoundScenes[MAX_SOUND_SCENES];
	XSoundRef				m_Sounds[MAX_SOUNDS];

	HashMap<CStr, XSoundsetDef*, CStrHashFunc> m_SoundsetDefHash;
	Set<XSoundsetDef*>	m_SoundsetDefs;

	HashMap<CStr, XSoundset*, CStrHashFunc> m_SoundsetHash;
	Set<XSoundset*>	m_Soundsets;

	HashMap<CStr, XSoundShader*, CStrHashFunc>	m_ShadersHash;
	Set<XSoundShader*>	m_Shaders;

	HashMap<CStr, XSoundSample*, CStrHashFunc>	m_SampleHash;
	Set<XSoundSample*>	m_Samples;
	Set<XSoundSample*>	m_LoadedSamples;

	HashMap<CStr, uint, CStrHashFunc> m_EnvironmentsHash;
	Array<XSoundEnv*>			m_Environments;

	HashMap<CStr, uint, CStrHashFunc> m_ShaderClassesHash;
	Set<SoundShaderClass*>			m_ShaderClasses;

	struct XSoundSourceRef
	{
		XSoundSourceRef*	ENF_LIST_PREV;
		XSoundSourceRef*	ENF_LIST_NEXT;
		Set<SoundHandle>	m_Handles;
		XSoundSourceRef(): ENF_LIST_PREV(NULL), ENF_LIST_NEXT(NULL) {}
	};

	Set< Pair<XSoundSource*, uint> >	m_SourcesSet;
	List<XSoundSourceRef>	m_ActiveSources;
	List<XSoundSourceRef>	m_FreeSources;
	XSoundSourceRef	m_SourceRefs[MAX_SOUNDS];

	bool AddSourceRef(XSoundSource* source, SoundHandle handle);
	void RemoveSourceRef(XSoundSource* source, SoundHandle handle);

	uint	m_iTimeStamp;
	uint	m_iAllocatedBytes;
	uint	m_iMaxAllocated;

	ENF_INLINE SoundHandle ToHandle(const XSoundRef* ref) const
	{
		return (ref->m_iSpinCount << MAX_SOUNDS_BITS) | (ref - m_Sounds);
	}

	ENF_INLINE const XSoundRef* ToSoundRef(SoundHandle handle) const
	{
		if(handle)
		{
			uint soundIndex = handle & MAX_SOUNDS_MASK;
			if(m_Sounds[soundIndex].m_iSpinCount == (handle >> MAX_SOUNDS_BITS))
				return &m_Sounds[soundIndex];
		}

		return NULL;
	}

	ENF_INLINE XSoundRef* ToSoundRef(SoundHandle handle)
	{
		if(handle)
		{
			uint soundIndex = handle & MAX_SOUNDS_MASK;
			if(m_Sounds[soundIndex].m_iSpinCount == (handle >> MAX_SOUNDS_BITS))
				return &m_Sounds[soundIndex];
		}
		return NULL;
	}
	bool r_LoadXSoundsetDefinition(TiXmlElement* elem, const XSoundsetDef* parent);
	void InitSounds();
	void Cleanup();

public:
	ENF_DECLARE_ITEM(ISoundSystem, XSoundSystem);
	ISoundSystem();
	virtual ~ISoundSystem();

	void LRUInsert(XSoundSample* sample);
	void LRURemove(XSoundSample* sample);

	virtual bool LoadShaderSources(SoundShaderCollection* coll, CStr name);
	uint GetNumShaderClasses() const;
	SoundShaderClass* GetShaderClass(uint index) const;
	void AddShaderClass(SoundShaderClass* sclass);
	SoundShaderClass* FindShaderClass(CStr name) const;

	uint GetNumEnvironment() const;
	CStr GetEnvironment(uint index) const;

	virtual bool Load(XSoundSample* sample, uint shaderFlags)=0;
	virtual bool Load(XSoundShader* shader, uint shaderFlags);

	virtual bool LoadSoundsetDefinitions(CStr name);
	virtual XSoundset* GetSoundset(CStr name);
	virtual const XSoundsetDef* GetSoundsetDef(CStr name) const;
	virtual XSoundShader* GetShader(CStr shaderName, uint flags);
	virtual bool ExistsShader(CStr shaderName) const;
	uint GetNumShaders() const
	{
		return m_Shaders.GetCardinality();
	}
	XSoundShader* GetShader(uint index) const
	{
		return m_Shaders[index];
	}
	virtual bool LoadShaders(SoundShaderCollection* coll);

};

//-----------------------------------------------------------------------------
}
#endif //ENF_ISOUNDSYSTEM_H