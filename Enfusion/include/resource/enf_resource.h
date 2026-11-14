//-----------------------------------------------------------------------------
// File: enf_resource.h
//
// Desc: Resource manager implementation
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_RESOURCE_H
#define ENF_RESOURCE_H

#include "enf_config.h"
#include "common/enf_types.h"


namespace enf
{
/** @addtogroup Resources
@{*/

class IResourceManager;
class ResourceManager;
class LoadResource;

const int MAX_RESOURCE_LEVEL = 100;

//!Used loading/eviction sort
enum ResourcePriority
{
	RESPRI_LOWEST		= -1000,
	RESPRI_LOWER		= -100,
	RESPRI_LOW			= -50,
	RESPRI_DEFAULT		= 0,
	RESPRI_HIGH			= 50,
	RESPRI_HIGHER		= 100,
	RESPRI_HIGHEST		= 500,
	RESPRI_IMMEDIATE	= INT_MAX
};

class PUBLIC_API ResourceRef
{
protected:
	friend class IResourceManager;
	friend class LoadResource;

	static IL_INT32			m_iCurrentFrame;

	ResourceManager*			m_pManager;
	mutable IL_INT32			m_iLastFrame;
	mutable uint				m_iXLevel;
	volatile LoadResource*	m_pLoadHandle;
	FileHandle					m_FileHandle;
	int							m_iPriority;

	virtual void _Touch(uint level) const;

public:
	void SetFileHandle(FileHandle handle);
	FileHandle GetFileHandle() const
	{
		return m_FileHandle;
	}
	ResourceManager* GetManager() const
	{
		return m_pManager;
	}
	ENF_INLINE void Touch(uint level) const
	{
		if(m_iLastFrame != m_iCurrentFrame)
			_Touch(level);
		else if(level != 0 && level > m_iXLevel)
		{
			m_iXLevel = level;
		}
	}

	bool IsLoading() const
	{
		return (m_pLoadHandle != NULL);
	}

	uint GetAge() const;
	int GetPriority() const
	{
		return m_iPriority;
	}
	void SetPriority(int priority)
	{
		m_iPriority = priority;
	}
	//!Returns approximate size of resource
	virtual size_t GetSize() const
	{
		return 0;
	}

	//!Returns the name of resource. Reimplemented in ancestors
	virtual CStr GetName() const { return NULL; }

	//!we need to free some memory. Returns how much we did
	virtual size_t Flush(uint level)
	{
		return 0;
	}

	void FinishLoading(LoadResource* ldres);
	ResourceRef(): m_pManager(NULL), m_iLastFrame(0), m_iXLevel(MAX_RESOURCE_LEVEL), m_pLoadHandle(NULL), m_FileHandle(BAD_FILE_HANDLE), m_iPriority(0)
	{
	}
	virtual ~ResourceRef();
};

//@}
}

#endif //ENF_RESOURCE_H