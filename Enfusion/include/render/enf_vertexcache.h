//-----------------------------------------------------------------------------
// File: render/enf_vertexcache.h
//
// Desc: Dynamic VB/IB
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_VERTEXCACHE_H
#define ENF_VERTEXCACHE_H

#include "enf_config.h"
#include "common/enf_types.h"

namespace enf
{
/** @addtogroup Renderer
@{*/

class VertexBuffer;
class IndexBuffer;

//-----------------------------------------------------------------------
/*!
Allocation remember structure for allocating vertexes
from ::VCache
*/
struct VCacheEntry
{
	uint			m_iVertexOffset;
	uint			m_iID;

	VCacheEntry()
	{
		m_iVertexOffset = 0;
		m_iID = -1;
	}
};

/*!
VCache is collection of VertexBuffer and IndexBuffer objects
of dynamic type. It's used for on-the-fly created geometries
which could be destroyed by time. The creator of some data can
query if his data are still in cache, or not and they has to
be reallocated. For this query the VCacheEntry structure is used.
TODO: This class should be refactored to some Mesh-creator instead
of separate vertex/index allocations. It doesn't work well too.
*/
class PUBLIC_API VCache
{
private:
	uint				m_iVertTopPos[4];
	VertexBuffer*	m_pVB[4];
	IndexBuffer*	m_pIB[4];

	uint				m_iVertSize;
	uint				m_iNum;
	uint				m_iActive;

public:
	VCache();
	~VCache();

	void Advance();

	VertexBuffer* GetVertexBuffer()
	{
		return m_pVB[m_iActive];
	}

	IndexBuffer* GetIndexBuffer()
	{
		return m_pIB[m_iActive];
	}

	bool IsValidEntry(const VCacheEntry& entry) const;

	bool	Init(uint vbuffsize, uint ibuffsize = 0, uint count = 1);
	void	Free();

	void*	Alloc(uint flags, uint verts, uint stride, VCacheEntry* entry = NULL);
	void	EndAlloc();
};
//@}
}
#endif //ENF_VERTEXCACHE_H
