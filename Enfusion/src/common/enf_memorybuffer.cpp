//-----------------------------------------------------------------------------
// File: enf_memorybuffer.cpp
//
// Desc: Growable memory buffer
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "common/enf_memorybuffer.h"
#include "common/enf_types.h"

namespace enf
{
//-----------------------------------------------------------------------
MemoryBuffer::MemoryBuffer(size_t initialLength)
{
	m_pBuffer = (unsigned char*)MemoryManager::Alloc(initialLength, __FILE__, __LINE__);
	m_iPos = 0;
	m_iLength = initialLength;
	m_iSize = 0;
}

//-----------------------------------------------------------------------
MemoryBuffer::~MemoryBuffer()
{
	MemoryManager::Free(m_pBuffer);
}

//-----------------------------------------------------------------------
void MemoryBuffer::Seek(uint pos)
{
	if (pos > m_iSize)
	{
		m_iPos = m_iSize;
	}
	else
	{
		m_iPos = pos;
	}
}

//-----------------------------------------------------------------------
void MemoryBuffer::Write(const void* buffer, size_t length)
{
	Grow(m_iPos + length);
	memcpy(m_pBuffer + m_iPos, buffer, length);
	m_iPos += length;
	if (m_iPos > m_iSize)
	{
		m_iSize = m_iPos;
	}
}

//-----------------------------------------------------------------------
int MemoryBuffer::Read(void* buffer, size_t length)
{
	uint toRead = length;
	if (m_iPos + length > m_iSize)
	{
		toRead = m_iSize - m_iPos;
	}
	memcpy(buffer, m_pBuffer + m_iPos, toRead);
	m_iPos += toRead;
	return toRead;
}

//-----------------------------------------------------------------------
void MemoryBuffer::Grow(size_t newMinLength)
{
	if (m_iLength < newMinLength)
	{
		uint newSize = enf_max(newMinLength, m_iLength * 2);
		m_pBuffer = (unsigned char*) REALLOC(m_pBuffer, newSize);
		m_iLength = newSize;
	}
}

//-----------------------------------------------------------------------
void MemoryBuffer::Reallocate(size_t newLength=1024)
{
	m_pBuffer = (unsigned char*) REALLOC(m_pBuffer, newLength);
	m_iPos = 0;
	m_iSize = 0;
	m_iLength = newLength;
}

}