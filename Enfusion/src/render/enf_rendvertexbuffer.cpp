//-----------------------------------------------------------------------------
// File: enf_rendvertexbuffer.cpp
//
// Desc: Dynamic Vertex Buffer class & VB abstraction
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------
//#include "precomp.h"
#include "enf_main.h"
#include "render/enf_irendmain.h"
#include "render/enf_renderresources.h"
#include "system/enf_profiler.h"

namespace enf
{
//-----------------------------------------------------------------------
VertexBuffer* IRenderer::CreateVertexBuffer(uint usage, uint len, uint stride, CStr name)
{
	enf_rlsassert(CheckThread());
	//bad use
	if(stride == 0)
		return NULL;

	VertexBuffer *vb = ENF_NEW VertexBuffer(this, usage, len, stride);

#ifdef ENF_DEVELOPER
	vb->m_strName = name;
#endif
	AddResource(vb);

	return vb;
}

//-----------------------------------------------------------------------
ERESULT VertexBuffer::Allocate()
{
	size_t memSize = GetMemorySize();
	Free();
#ifdef ENF_DX9

	#ifdef ENF_DISABLE_MANAGED_RESOURCES
		uint pool = D3DPOOL_DEFAULT;
		m_bManaged = false;
	#else
		uint pool = (m_iUsage & RES_USAGE_DYNAMIC) ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;
		m_bManaged = (pool == D3DPOOL_MANAGED);
	#endif

	//if HW don't support VS, change USAGE
	uint swusage = (GetDevice()->GetCaps().VertexShaderVersion < D3DVS_VERSION(3,0)) ? RES_USAGE_SOFTWAREPROCESSING : 0;

	do
	{
		if(GetDevice()->GetDXDevice()->CreateVertexBuffer(memSize,
			m_iUsage | swusage,
			0,
			(D3DPOOL)pool,
			&m_pVB,
			NULL) == D3D_OK)
		{
			MemStats::m_iTotalVertices += memSize;
			return EE_OK;
		}
	} while(MemoryManager::OutOfMemory(__FILE__, __LINE__, MT_VIDEO, memSize));

#elif defined(ENF_OGL)
	GLuint vb;

	GLCALL(glGenBuffers(1, &vb));
	enf_assert(vb != 0);
	GLCALL(glBindBuffer(GL_ARRAY_BUFFER, vb));
	GetDevice()->DirtyMask(Renderer::GLCM_STREAMS);
	m_iBuffer = vb;
	
	GLenum usage = (m_iUsage & RES_USAGE_DYNAMIC) ? GL_STREAM_DRAW : GL_STATIC_DRAW;
	GLCALL(glBufferData(GL_ARRAY_BUFFER, memSize, NULL, usage));
	return EE_OK;
#endif

	return EE_ERROR;
}

//-----------------------------------------------------------------------
void VertexBuffer::Free()
{
	enf_rlsassert(GetDevice()->CheckThread());
#ifdef ENF_DX9
	if(m_pVB)
	{
		uint c = m_pVB->Release();

		enf_assert(c == 0);
		m_pVB = NULL;
		MemStats::m_iTotalVertices -= GetMemorySize();
	}
#elif defined(ENF_OGL)
	if(m_iBuffer)
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		GetDevice()->DirtyMask(Renderer::GLCM_STREAMS);
		GLCALL(glDeleteBuffers(1, &m_iBuffer));
		m_iBuffer = 0;
	}
#endif
}

//-----------------------------------------------------------------------
RenderBuffer::RenderBuffer(IRenderer* dev): DeviceResource(dev), m_iOffset(0), m_iDBOffset(0), m_iUsage(0)
{
	#ifdef ENF_OGL
		m_iBuffer = 0;
	#endif
}

//-----------------------------------------------------------------------
RenderBuffer::~RenderBuffer()
{
}

//-----------------------------------------------------------------------
VertexBuffer::VertexBuffer(IRenderer* dev, uint usage, uint len, uint stride):	RenderBuffer(dev),	m_iStride(stride)
{
	m_iLength = len;
	#ifdef ENF_DX9
		m_pVB = NULL;
	#endif
	m_iUsage = usage;

	Allocate();
}

//-----------------------------------------------------------------------
uint VertexBuffer::EndAlloc()
{
	#ifdef ENF_DX9
		m_pVB->Unlock();
	#elif defined(ENF_OGL)
		GLCALL(glBindBuffer(GL_ARRAY_BUFFER, m_iBuffer));
		GLCALL(glUnmapBuffer(GL_ARRAY_BUFFER));
		GetDevice()->DirtyMask(Renderer::GLCM_STREAMS);
	#endif
	return m_iDBOffset;
}

//-----------------------------------------------------------------------
void *VertexBuffer::BeginAlloc(uint numverts, uint* id)
{
	enf_rlsassert(GetDevice()->CheckThread());
	uint usage;

	//not enough space
	if(numverts >= GetNumVerts())
		return NULL;

	if((m_iOffset + numverts) < GetNumVerts())
	{
		usage = RES_LOCK_NOOVERWRITE;
		m_iDBOffset = m_iOffset;
		m_iOffset += numverts;
	}
	else
	{
		usage = RES_LOCK_DISCARD;
		m_iDBOffset = 0;
		m_iOffset = numverts;
		IncrementID();
	}

	ENF_PROFILE(PROFILE_LOCK_VB);

	void *dv;
	#ifdef ENF_DX9
		if(m_pVB->Lock(m_iDBOffset * m_iStride, numverts * m_iStride,&dv, usage) == D3D_OK)
		{
			if(id) *id = GetID();
			return (void *)dv;
		}

	#elif defined(ENF_OGL)
		GLCALL(glBindBuffer(GL_ARRAY_BUFFER, m_iBuffer));

		#ifdef __OPENGLES__
			GLCALL(dv = glMapBufferOES(GL_ARRAY_BUFFER, GL_WRITE_ONLY_OES));
		#else
			GLCALL(dv = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
		#endif
		GetDevice()->DirtyMask(Renderer::GLCM_STREAMS);
		enf_assert(dv != NULL);
		return ((char*)dv + (m_iDBOffset * m_iStride));
	#endif

	return NULL;
}

//-----------------------------------------------------------------------
void*	VertexBuffer::Lock(uint from, uint num, uint flags)
{
	enf_rlsassert(GetDevice()->CheckThread());
	void* ptr;

	enf_assert((from + num) <= GetNumVerts());

	ENF_PROFILE(PROFILE_LOCK_VB);
	#ifdef ENF_DX9
		if(m_pVB->Lock(from * m_iStride, num * m_iStride, &ptr, flags) == D3D_OK)
			return ptr;

	#elif defined(ENF_OGL)
		GLCALL(glBindBuffer(GL_ARRAY_BUFFER, m_iBuffer));
		GLuint access = 0;
		if(flags & RES_LOCK_READONLY)
			access = GL_READ_ONLY;
		else if(flags & RES_LOCK_WRITEONLY)
			access = GL_WRITE_ONLY;
		else
			access = GL_READ_WRITE;

		GLCALL(ptr = glMapBuffer(GL_ARRAY_BUFFER, access));
		GetDevice()->DirtyMask(Renderer::GLCM_STREAMS);
		enf_assert(ptr != NULL);

		return ((char*)ptr + (from * m_iStride));
	#endif

	return NULL;
}

//-----------------------------------------------------------------------
void VertexBuffer::Unlock()
{
	enf_rlsassert(GetDevice()->CheckThread());

	#ifdef ENF_DX9
		m_pVB->Unlock();

	#elif defined(ENF_OGL)
		GLCALL(glBindBuffer(GL_ARRAY_BUFFER, m_iBuffer));
		GLCALL(glUnmapBuffer(GL_ARRAY_BUFFER));
		GetDevice()->DirtyMask(Renderer::GLCM_STREAMS);
	#endif
}

}