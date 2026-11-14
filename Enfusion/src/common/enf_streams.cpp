//-----------------------------------------------------------------------------
// File: enf_streams.cpp
//
// Desc: IO streams
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
#include "common/enf_streams.h"
#include "system/enf_vfilesystem.h"
#include "common/enf_memorybuffer.h"

namespace enf
{
//-----------------------------------------------------------------------
CFileOutputStream::CFileOutputStream(const char* name):COutputStream(), m_bNoClose(false)
{
	m_pFile = GetFileSystem()->Open(name, FILEMODE_WRITE);
}

//-----------------------------------------------------------------------
CFileOutputStream::CFileOutputStream(VFile* file, bool noClose):COutputStream(), m_bNoClose(noClose)
{
	enf_assert(file != NULL);
	m_pFile = file;
}

//-----------------------------------------------------------------------
bool CFileOutputStream::Open(CStr fileName)
{
	m_pFile = GetFileSystem()->Open(fileName, FILEMODE_WRITE);
	return m_pFile != NULL;
}

//-----------------------------------------------------------------------
CFileOutputStream::~CFileOutputStream()
{
	Close();
}

//-----------------------------------------------------------------------
size_t CFileOutputStream::Write(const void* data, size_t size)
{
	enf_assert(m_pFile != NULL);
	return m_pFile->Write(data, size);
}

//-----------------------------------------------------------------------
size_t CFileOutputStream::Seek(size_t pos)
{
	enf_assert(m_pFile != NULL);
	m_pFile->Seek(pos, SEEKMODE_BEGIN);
	return pos;
}

//-----------------------------------------------------------------------
size_t CFileOutputStream::GetCurrentPosition() const
{
	enf_assert(m_pFile != NULL);
	return m_pFile->GetPos();
}

//-----------------------------------------------------------------------
void CFileOutputStream::Close()
{
	if(m_pFile)
	{
		if(!m_bNoClose)
			m_pFile->Close();
		m_pFile = NULL;
	}
}

//-----------------------------------------------------------------------
CFileInputStream::CFileInputStream(const char* name):CInputStream(), m_bNoClose(false)
{
	m_pFile = GetFileSystem()->Open(name, FILEMODE_READ);
}

//-----------------------------------------------------------------------
CFileInputStream::CFileInputStream(VFile* file, bool noClose):CInputStream(), m_bNoClose(noClose)
{
	enf_assert(file != NULL);
	m_pFile = file;
}

//-----------------------------------------------------------------------
CFileInputStream::CFileInputStream(FileHandle fileHandle, bool noClose):CInputStream(), m_bNoClose(noClose)
{
	enf_assert(fileHandle != 0);
	VFile* file = GetFileSystem()->Open(fileHandle, FILEMODE_READ);
	m_pFile = file;
}

//-----------------------------------------------------------------------
CFileInputStream::~CFileInputStream()
{
	Close();
}

//-----------------------------------------------------------------------
size_t CFileInputStream::GetSize() const
{
	enf_assert(m_pFile != NULL);
	return m_pFile->GetLength() - m_pFile->GetPos();
}
//-----------------------------------------------------------------------
size_t CFileInputStream::Read(void* data, size_t size)
{
	enf_assert(m_pFile != NULL);
	return m_pFile->Read(data, size);
}

//-----------------------------------------------------------------------
size_t CFileInputStream::Seek(size_t pos)
{
	enf_assert(m_pFile != NULL);
	m_pFile->Seek(pos, SEEKMODE_BEGIN);
	return pos;
}

//-----------------------------------------------------------------------
size_t CFileInputStream::GetCurrentPosition() const
{
	enf_assert(m_pFile != NULL);
	return m_pFile->GetPos();
}

//-----------------------------------------------------------------------
void CFileInputStream::Close()
{
	if(m_pFile)
	{
		if(!m_bNoClose)
			m_pFile->Close();
		m_pFile = NULL;
	}
}

//-----------------------------------------------------------------------
size_t CFileInputStream::Skip(size_t size)
{
	size_t fpos = m_pFile->GetPos();
	size_t fsize = m_pFile->GetLength() - fpos;
	enf_assert2(size <= fsize, "Attempt to seek out of bounds");
	size = enf_min(size, fsize);
	m_pFile->Seek(size + fpos, SEEKMODE_BEGIN);
	return size;
}

//-----------------------------------------------------------------------
CMemoryOutputStream::CMemoryOutputStream(MemoryBuffer* buffer):
	COutputStream(),
	m_pBuffer(buffer)
{
}

//-----------------------------------------------------------------------
CMemoryOutputStream::~CMemoryOutputStream()
{
	Close();
}

//-----------------------------------------------------------------------
size_t CMemoryOutputStream::Write(const void* data, size_t size)
{
	enf_assert(m_pBuffer != NULL);
	m_pBuffer->Write(data, size);
	return size;
}

//-----------------------------------------------------------------------
size_t CMemoryOutputStream::Seek(size_t pos)
{
	enf_assert(m_pBuffer != NULL);
	m_pBuffer->Seek(pos);
	return pos;
}

//-----------------------------------------------------------------------
size_t CMemoryOutputStream::GetCurrentPosition() const
{
	enf_assert(m_pBuffer != NULL);
	return m_pBuffer->Tell();
}

//-----------------------------------------------------------------------
void CMemoryOutputStream::Close()
{
	m_pBuffer = NULL;
}

//-----------------------------------------------------------------------
CMemoryInputStream::CMemoryInputStream(MemoryBuffer* buffer):
	CInputStream(),
	m_pBuffer(buffer)
{
}

//-----------------------------------------------------------------------
CMemoryInputStream::~CMemoryInputStream()
{
	Close();
}

//-----------------------------------------------------------------------
size_t CMemoryInputStream::Read(void* data, size_t size)
{
	enf_assert(m_pBuffer != NULL);
	return m_pBuffer->Read(data, size);
}

//-----------------------------------------------------------------------
size_t CMemoryInputStream::Seek(size_t pos)
{
	enf_assert(m_pBuffer != NULL);
	m_pBuffer->Seek(pos);
	return pos;
}

//-----------------------------------------------------------------------
size_t CMemoryInputStream::Skip(size_t size)
{
	enf_assert(m_pBuffer != NULL);
	size = enf_min(size, m_pBuffer->GetSize());
	m_pBuffer->Seek(m_pBuffer->Tell() + size);
	return size;
}

//-----------------------------------------------------------------------
size_t CMemoryInputStream::GetCurrentPosition() const
{
	enf_assert(m_pBuffer != NULL);
	return m_pBuffer->Tell();
}

//-----------------------------------------------------------------------
size_t CMemoryInputStream::GetSize() const
{
	enf_assert(m_pBuffer != NULL);
	return m_pBuffer->GetSize() - m_pBuffer->Tell();
}

//-----------------------------------------------------------------------
void CMemoryInputStream::Close()
{
	m_pBuffer = NULL;
}

//-----------------------------------------------------------------------
size_t CBufferInputStream::Read(void* data, size_t size)
{
	size_t off = m_iOffset + size;
	if(off > m_iSize)
		size -= (off - m_iSize);

	memcpy(data, m_pBuffer + m_iOffset, size);
	m_iOffset += size;
	return size;
}

}