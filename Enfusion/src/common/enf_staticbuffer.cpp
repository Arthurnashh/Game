//-----------------------------------------------------------------------------
// File: enf_staticbuffer.cpp
//
// Desc: Static heap allocator, static string allocator.
//			For cases when there is never deallocation
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "common/enf_types.h"
#include "common/enf_staticbuffer.h"
#include "system/enf_mutex.h"

namespace enf
{
const uint ALIGN = 4;

//-----------------------------------------------------------------------
class PUBLIC_API IStaticBuffer: public StaticBuffer
{
public:
	ENF_DECLARE_SMALLOBJECT(IStaticBuffer);

	struct StrNode
	{
		const char* m_strString;
		int					m_iWeight;
		StrNode*		m_psChildren[2];
	};

private:
	StrNode*	StringRoot;
	bool			m_bMT;
	CMutex		m_Mutex;

public:
	bool _IsAllocated(const void* ptr) const
	{
    CMutexLock lock(m_Mutex, m_bMT);
		return (((char *)ptr) >= Buffer && ((char *)ptr) < BeginData);
	}
	const char* _AddString(const char* string);
	const char* _AddString(const char* string, size_t len);
	const char* _FindString(const char* string);
	void* AllocData(size_t len, const void *data);
	void* AllocData(size_t len);
	void FreeData(size_t len);
	bool Init(size_t size);
	void Free();

	~IStaticBuffer();
	IStaticBuffer();
	IStaticBuffer(size_t size, bool threadSafe);
};

//-----------------------------------------------------------------------
ENF_INLINE int StringWeight(const char *str)
{
	uint weight = 0;

	while(*str)
		weight = (weight * 37) + (unsigned)*str++;

	return (int)weight;
}

//-----------------------------------------------------------------------
ENF_INLINE int StringWeight(const char *str, size_t len)
{
	uint weight = 0;

	while(len--)
		weight = (weight * 37) + (unsigned)*str++;

	return weight;
}

//-----------------------------------------------------------------------
bool IStaticBuffer::Init(size_t size)
{
	BeginData = NULL;
	EndData = NULL;
	Size = 0;
	StringRoot = NULL;
	Buffer = (char*)GMemFunctions->New(size, __FILE__, __LINE__);

	if(Buffer == NULL)
		return false;

	FillMem(Buffer, size, 0);
	BeginData = Buffer;
	Size = size;
	EndData = BeginData + size;
	return true;
}

//-----------------------------------------------------------------------
void IStaticBuffer::Free()
{
	GMemFunctions->Delete(Buffer);
	Size = 0;
}

//-----------------------------------------------------------------------
void *IStaticBuffer::AllocData(size_t len)
{
	CMutexLock lock(m_Mutex, m_bMT);

	intptr_t p;
	if(len)
	{
		p = (intptr_t)BeginData;
		p = (p + (intptr_t(ALIGN) - 1)) & ~(intptr_t(ALIGN) - 1);

		BeginData = (char *)(p + len);

    RlsAssert(BeginData < EndData);
	}
	else
		p = NULL;

	return (void *)p;
}

//-----------------------------------------------------------------------
void *IStaticBuffer::AllocData(size_t len, const void *data)
{
	void *p;

	if((p = AllocData(len)) != NULL)
		CopyMem(p, data, len);

	return (void *)p;
}

//-----------------------------------------------------------------------
void IStaticBuffer::FreeData(size_t len)
{
	CMutexLock lock(m_Mutex, m_bMT);

	Assert((BeginData - len) >= Buffer);

	BeginData -=len;
	ZeroMem(BeginData, len);
}

#define SIGN2INDEX(a) ((unsigned)(a) >> 31)

//-----------------------------------------------------------------------
const char* IStaticBuffer::_AddString(const char* string)
{
	CMutexLock lock(m_Mutex, m_bMT);

	//already from string table
	if(m_bMT && IsAllocatedMTSafe(string))
		return string;

StrNode **node = &StringRoot;
StrNode *nnode;

int w = StringWeight(string);

	while((nnode = *node) != NULL)
	{
		int diff = w - nnode->m_iWeight;

		if(diff == 0 && strcmp(string, nnode->m_strString) == 0)
			goto done;

		node = &nnode->m_psChildren[SIGN2INDEX(diff)];
	}

	{
		int len = strlen(string);

		nnode = (StrNode *)AllocData(sizeof(StrNode) + len + 1);
		*node = nnode;

		CopyMem((char *)(nnode + 1), (void*)string, len);
	}
	nnode->m_strString = (char *)(nnode + 1);
	nnode->m_iWeight = w;
done:;
	return nnode->m_strString;
}

//-----------------------------------------------------------------------
const char* IStaticBuffer::_AddString(const char* string, size_t len)
{
	CMutexLock lock(m_Mutex, m_bMT);

	//already from string table
	if(m_bMT && IsAllocatedMTSafe(string))
		return string;

	StrNode** node = &StringRoot;
	StrNode* nnode;

	int w = StringWeight(string, len);

	while((nnode = *node) != NULL)
	{
		int diff = w - nnode->m_iWeight;

		if(w == nnode->m_iWeight && strncmp(string, nnode->m_strString, len) == 0)
			goto done;

		node = &nnode->m_psChildren[SIGN2INDEX(diff)];
	}

	nnode = (StrNode *)AllocData(sizeof(StrNode) + len + 1);
	*node = nnode;

	CopyMem((char *)(nnode + 1), (void*)string, len);
	nnode->m_strString = (char *)(nnode + 1);
	nnode->m_iWeight = w;

done:;
	return nnode->m_strString;
}

//-----------------------------------------------------------------------
const char* IStaticBuffer::_FindString(const char* string)
{
	CMutexLock lock(m_Mutex, m_bMT);

	//already from string table
	if(m_bMT && IsAllocatedMTSafe(string))
		return string;


	StrNode* node = StringRoot;
	int w = StringWeight(string);

	while(node)
	{
		int diff = w - node->m_iWeight;
		if(w == node->m_iWeight && strcmp(string, node->m_strString) == 0)
		{
			return node->m_strString;
		}
		node = node->m_psChildren[SIGN2INDEX(diff)];
	}

	return NULL;
}

//-----------------------------------------------------------------------
IStaticBuffer::IStaticBuffer()
{
	BeginData = NULL;
	EndData = NULL;
	Buffer = NULL;
	Size = 0;
	StringRoot = NULL;
}

//-----------------------------------------------------------------------
IStaticBuffer::IStaticBuffer(size_t size, bool threadSafe)
{
	BeginData = NULL;
	EndData = NULL;
	Buffer = NULL;
	Size = 0;
	StringRoot = NULL;
	m_bMT = threadSafe;

	Init(size);
}

//-----------------------------------------------------------------------
IStaticBuffer::~IStaticBuffer()
{
	Free();
}

//-----------------------------------------------------------------------
StaticBuffer* CreateStaticBuffer(size_t size, bool threadSafe)
{
	return new IStaticBuffer(size, threadSafe);
}
}