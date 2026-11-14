//-----------------------------------------------------------------------------
// File: enf_textureresloader.h
//
// Desc: Resource manager implementation
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_TEXTURERESLOADER_H
#define ENF_TEXTURERESLOADER_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "resource/enf_iresourcemanager.h"
#include "resource/enf_imageloader.h"


namespace enf
{
/** @addtogroup Resources
@{*/

//-----------------------------------------------------------------------
class PUBLIC_API LoadTextureResource: public LoadResource
{
protected:
	uint						m_iFlags;
	uint						m_iSkipMipMap;
	TextureUsage		m_Usage;
	MemoryImageData	m_ImageData;
	DString					m_strName;

public:
	ENF_DECLARE_ITEM(LoadTextureResource, LoadResource);
	ENF_DECLARE_SMALLOBJECT(LoadTextureResource);

	Texture* GetTexture() const
	{
		return static_cast<Texture*>(m_pResource);
	}

	virtual CStr GetName() const
	{
		return m_strName.cstr();
	}
	void SetName(CStr name)
	{
		m_strName = name;
	}

	size_t GetDataSize() const;
	LoadTextureResource(Texture* tex, uint flags, TextureUsage usage, int loadPriority, uint skipMipMap = 0);
	~LoadTextureResource();
	bool DoLoad();
	void Apply();
};

}

#endif //ENF_TEXTURERESLOADER_H