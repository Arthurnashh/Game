//-----------------------------------------------------------------------------
// File: enf_streams.h
//
// Desc: IO streams
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#pragma once

#ifndef ENF_STREAMS_H
#define ENF_STREAMS_H

#include "enf_config.h"
#include "system/enf_memory.h"
#include "common/enf_strings.h"
#include "common/enf_types.h"

namespace enf
{
/** @addtogroup Common
@{*/

class VFile;
class MemoryBuffer;

/*!
CStream
*/
class PUBLIC_API CStream
{
protected:
	CStream()
	{
	}

public:
	ENF_DECLARE_MMOBJECT(CStream);
	virtual ~CStream() {}
	virtual bool IsOpen() const { return false; }
	virtual size_t Seek(size_t pos)=0;
	virtual size_t GetCurrentPosition() const=0;
	virtual void Close()=0;
};

/*!
COutputStream
*/
class PUBLIC_API COutputStream: public CStream
{
private:
public:
	virtual size_t Write(const void* data, size_t size)=0;
	COutputStream& operator <<(char c)
	{
		Write(&c, 1);
		return *this;
	}
	COutputStream& operator <<(const char* str)
	{
		Write(str, strlen(str));
		return *this;
	}
	COutputStream& operator <<(const DString& str)
	{
		Write(str.c_str(), str.Length());
		return *this;
	}
	COutputStream& operator <<(CStr str)
	{
		Write(str.cstr(), str.Length());
		return *this;
	}
	COutputStream(): CStream()
	{
	}
};

/*!
CInputStream
*/
class PUBLIC_API CInputStream: public CStream
{
private:
public:
	virtual size_t Read(void* data, size_t size)=0;
	virtual size_t GetSize() const=0;
	virtual size_t Skip(size_t size)
	{
		size_t seek = enf_min(GetSize(), size);
		Seek(seek + GetCurrentPosition());
		return size;
	}

	CInputStream(): CStream()
	{
	}
};

//-----------------------------------------------------------------------------
template<uint CacheSize> class PUBLIC_API CBufferedOutputStream
{
	COutputStream& m_OutStream;
	uint	m_iCachePos;
	ubyte	m_Buffer[CacheSize];

public:
	CBufferedOutputStream(COutputStream& ostr): m_OutStream(ostr), m_iCachePos(0) {}
	~CBufferedOutputStream()
	{
		Flush();
	}
	ENF_INLINE void Flush()
	{
		if(m_iCachePos)
		{
			m_OutStream.Write(m_Buffer, m_iCachePos);
			m_iCachePos = 0;
		}
	}

	ENF_INLINE size_t Write(const void* data, size_t size)
	{
		
		uint cachePos = m_iCachePos;
		uint left = CacheSize - cachePos;
		if(left < size)
		{
			memcpy(m_Buffer + cachePos, data, left);
			Flush();
			uint towrite = size - left;
			if(towrite <= CacheSize)
			{
				memcpy(m_Buffer, (const ubyte*)data + left, towrite);
				m_iCachePos = towrite;
			}
			else
			{
				m_OutStream.Write((const ubyte*)data + left, towrite);
			}
		}
		else
		{
			memcpy(m_Buffer + cachePos, data, size);
			m_iCachePos = cachePos + size;
		}
		return size;
	}
	size_t Seek(size_t pos)
	{
		Flush();
		return m_OutStream.Seek(pos);
	}
	size_t GetCurrentPosition() const
	{
		return m_OutStream.GetCurrentPosition() + m_iCachePos;
	}
	void Close()
	{
		m_OutStream.Close();
	}
};

//-----------------------------------------------------------------------------
class PUBLIC_API CFileOutputStream: public COutputStream
{
	VFile* m_pFile;
	bool m_bNoClose;

public:
	CFileOutputStream(): m_pFile(NULL), m_bNoClose(false) {}
	CFileOutputStream(const char* name);
	CFileOutputStream(VFile* file, bool noClose = false);
	~CFileOutputStream();
	bool Open(CStr fileName);
	bool IsOpen() const { return m_pFile != NULL; }
	size_t Write(const void* data, size_t size);
	size_t Seek(size_t pos);
	size_t GetCurrentPosition() const;
	void Close();
};

//-----------------------------------------------------------------------------
class PUBLIC_API CFileInputStream: public CInputStream
{
	VFile* m_pFile;
	bool m_bNoClose;

public:
	CFileInputStream(const char* name);
	CFileInputStream(FileHandle file, bool noClose = false);
	CFileInputStream(VFile* file, bool noClose = false);
	~CFileInputStream();
	bool IsOpen() const { return m_pFile != NULL; }
	size_t Read(void* data, size_t size);
	size_t GetSize() const;
	size_t Seek(size_t pos);
	size_t GetCurrentPosition() const;
	size_t Skip(size_t size);

	void Close();
};

//-----------------------------------------------------------------------------
class PUBLIC_API CMemoryOutputStream: public COutputStream
{
	MemoryBuffer* m_pBuffer;

public:
	CMemoryOutputStream(MemoryBuffer* buffer);
	~CMemoryOutputStream();
	bool IsOpen() const { return m_pBuffer != NULL; }
	size_t Write(const void* data, size_t size);
	size_t Seek(size_t pos);
	size_t GetCurrentPosition() const;
	void Close();
};

//-----------------------------------------------------------------------------
class PUBLIC_API CMemoryInputStream: public CInputStream
{
	MemoryBuffer* m_pBuffer;

public:
	CMemoryInputStream(MemoryBuffer* buffer);
	~CMemoryInputStream();
	bool IsOpen() const { return m_pBuffer != NULL; }
	size_t Read(void* data, size_t size);
	size_t Seek(size_t pos);
	size_t GetCurrentPosition() const;
	size_t GetSize() const;
	size_t Skip(size_t size);
	void Close();
};
//-----------------------------------------------------------------------------
class PUBLIC_API CBufferInputStream: public CInputStream
{
	const char* m_pBuffer;
	size_t		m_iOffset;
	size_t		m_iSize;

public:
	CBufferInputStream(const void* buffer, size_t size): m_pBuffer((const char*)buffer), m_iOffset(0), m_iSize(size)
	{
	}
	~CBufferInputStream() {}
	bool IsOpen() const { return m_pBuffer != NULL; }
	size_t Read(void* data, size_t size);
	size_t Seek(size_t pos)
	{
		Assert(pos < m_iSize);
		m_iOffset = pos;
		return pos;
	}
	size_t GetCurrentPosition() const
	{
		return m_iOffset;
	}
	size_t GetSize() const
	{
		return m_iSize - m_iOffset;
	}
	size_t Skip(size_t size)
	{
		size = enf_min(size, m_iSize - m_iOffset);
		m_iOffset = m_iOffset + size;
		return size;
	}

	void Close()
	{
		m_pBuffer = NULL;
		m_iOffset = 0;
		m_iSize = 0;
	}
};

//-----------------------------------------------------------------------------
/*!
wrapper over the CInputStream, allows to use simple GetXXXX functions which are
implemented over the internal cache. 

Currently, due to ZIP implementation, 4 bytes are allowed to be put back into input
stream and when the algorithm allows to use this, they are accessible through 
xxxAllowBack functions.

Internally, the size of buffer is Size + 8 bytes, where first four bytes allow
return of data back to cache and the last four bytes are zero to simplify the 
uint and ushort glowing 
*/
template<uint Size = 8192> class PUBLIC_API CByteInputStream
{
protected:
	CInputStream&	m_Stream;				///< input stream
	int				m_iStreamLeft;			///< how much data is still in input stream 
	int				m_iStreamInCache;		///< how much data is in local cache
	int				m_iCachePointer;		///< pointer to current data in cache
	int				m_iBackData;			///< number of data returned to cache
	
	const uint		CacheSize;

	uchar  m_pLocCache [Size + 8]; 
	uchar* m_pCache;

	/*!
	fill cache with data
	*/
	ENF_INLINE void FillCache()
	{
		Assert(m_iStreamLeft > 0);
		Assert(m_iBackData == 0);

		int size = enf_min(m_iStreamLeft, (int)CacheSize);
		m_iStreamInCache = size;
		m_iStreamLeft -= size;

		uint read = m_Stream.Read(m_pCache, size);
		Assert(read == size);
	}


	/*!
	fill cache with data
	*/
	ENF_INLINE size_t DirectCopy(uchar* dest, uint size)
	{
		Assert(m_iStreamLeft > 0 && (int)size <= m_iStreamLeft);
		Assert(m_iBackData == 0);

		m_iStreamLeft -= size;
		m_iStreamInCache = 0;
		m_iCachePointer  = CacheSize;

		size_t read = m_Stream.Read(dest, size);

		Assert(read == size);
		return read;
	}


	/*!
	copy data into dest address
	\param dest		destination
	\param size		how much data
	*/
	ENF_INLINE size_t CopyData(uchar* dest, size_t size)
	{
		if (m_iCachePointer >= (int)CacheSize)
		{
			//this left no data in cache, which is good when we read large chunks of data, but not for small one
			//return DirectCopy(dest, size);

			//preread into cache
			if (size < 16 && m_iStreamLeft > 16)
			{
				FillCache();
				m_iCachePointer = 0;
			}
			else
			{
				return DirectCopy(dest, size);
			}	
		}

		Assert((int) size <= GetStreamSize());

		//copy the rest from cache
		uint incache = CacheSize - m_iCachePointer + m_iBackData;
		uint count   = enf_min((uint) size, incache);
		memcpy(dest, m_pCache + m_iCachePointer - m_iBackData, count);

		m_iCachePointer += count;
		dest += count;
		size -= count;

		Assert(size >= 0);

		m_iBackData = 0;

		//still something to read
		if (size)
		{
			return DirectCopy(dest, size) + count;
		}
		
		return count;
	}

public:
	/*!
	create
	\param stream	input stream
	\param streamsize	maximum size to read (0xFFFFFFFF when do not use)
	*/
	CByteInputStream(CInputStream &stream, int streamsize = 0x7FFFFFFF): m_Stream(stream), CacheSize(Size)
	{
		Assert2(CacheSize >= 4, "Size of cache must be minimally 8 bytes");

		//leave first four bytes unattached and set last four bytes to zero
		m_pCache = &m_pLocCache[0] + 4;
		*(uint*) (m_pCache + CacheSize) = 0;

		m_iStreamLeft		= streamsize;
		m_iStreamInCache	= 0;
		m_iBackData			= 0;
		m_iCachePointer	= CacheSize;
		
		if (m_iStreamLeft == 0x7FFFFFFF)
		{
			m_iStreamLeft = (int) stream.GetSize();
		}
	}

	
	~CByteInputStream()
	{
		//Assert(m_iStreamLeft == 0);
	}

	/*!
	return number of not read data
	*/
	ENF_INLINE int GetStreamSize()
	{
		Assert((int) m_iCachePointer <= m_iStreamInCache);
		return m_iStreamLeft + (m_iStreamInCache - m_iCachePointer) + m_iBackData;
	}

	/*!
	put char back, only 4 are supported
	*/
	ENF_INLINE void PutBack(uchar ch)
	{
		Assert(m_iBackData < 4);
		m_pCache[m_iCachePointer - ++m_iBackData] = ch;
	}


	/*!
	get char from stream and advance the pointer to data by 1
	*/
	ENF_INLINE uchar GetUChar()
	{
		Assert(m_iBackData == 0);

		if (m_iCachePointer < m_iStreamInCache)
		{	
			return m_pCache[m_iCachePointer++];
		}

		FillCache();

		m_iCachePointer = 1;
		return m_pCache[0];
	}

	/*!
	get char from stream but DO NOT advance the pointer to data by 1
	*/
	ENF_INLINE uchar TestUChar()
	{
		Assert(m_iBackData == 0);

		if (m_iCachePointer < m_iStreamInCache)
		{	
			return m_pCache[m_iCachePointer];
		}

		FillCache();

		m_iCachePointer = 0;
		return m_pCache[0];
	}


	/*!
	get char from stream and advance the pointer to data by 1
	*/
	ENF_INLINE uchar GetUCharAllowBack()
	{
		if (m_iBackData)
		{
			return m_pCache[m_iCachePointer - m_iBackData--];
		}

		if (m_iCachePointer < (int)CacheSize)
		{	
			return m_pCache[m_iCachePointer++];
		}

		FillCache();

		m_iCachePointer = 1;
		return m_pCache[0];
	}


	/*!
	get char from stream and advance the pointer to data by 2, no back data allowed
	*/
	ENF_INLINE ulong GetUShort()
	{
		Assert(m_iBackData == 0);

		//enough room in cache
		if (m_iCachePointer < CacheSize - 1)
		{	
			uint index = m_iCachePointer;
			m_iCachePointer += 2;
			return *(ushort*) (m_pCache + index);
		}

		//one byte still i cache
		if (m_iCachePointer < CacheSize)
		{
			ushort first = m_pCache[CacheSize - 1];
			FillCache();

			m_iCachePointer = 1;
			return first | (m_pCache[0] << 8);
		}

		FillCache();
		m_iCachePointer = 2;
		return *(ushort*) m_pCache;
	}



	/*!
	get char from stream and advance the pointer to data by 2
	*/
	ENF_INLINE ulong GetUShortAllowBack()
	{
		if (m_iBackData)
		{
			int index = (int) m_iCachePointer - (int) m_iBackData;

			//this is possible and the simplest
			if (m_iBackData > 1)
			{
				m_iBackData -= 2;
				return *(ushort*) (m_pCache + index);
			}

			//just one in back cache, enough data in normal cache?
			if (index < CacheSize - 1)
			{
				m_iBackData = 0;
				m_iCachePointer++;
				return *(ushort*) (m_pCache + index);
			}

			ushort first = m_pCache[index];
			m_iBackData  = 0;

			FillCache();

			m_iCachePointer = 1;

			return first | m_pCache[0] << 8;
		}

		//enough room in cache
		if (m_iCachePointer < CacheSize - 1)
		{	
			uint index = m_iCachePointer;
			m_iCachePointer += 2;
			return *(ushort*) (m_pCache + index);
		}

		//one byte still i cache
		if (m_iCachePointer < CacheSize)
		{
			ushort first = m_pCache[CacheSize - 1];
			FillCache();

			m_iCachePointer = 1;
			return first | (m_pCache[0] << 8);
		}

		FillCache();
		m_iCachePointer = 2;
		return *(ushort*) m_pCache;
	}


	/*!
	get uint from stream and advance the pointer to data by 4
	*/
	ENF_INLINE uint GetUInt()
	{
		Assert(m_iBackData == 0);

		uint r = *(uint*) (m_pCache + m_iCachePointer);
		
		if (m_iCachePointer <= (int)CacheSize - 4)
		{	
			m_iCachePointer += 4;		
			return r;
		}

		Assert(m_iStreamLeft > 0);

		uint left = CacheSize - m_iCachePointer;

		FillCache();

		uint first = *(uint*) m_pCache;
		m_iCachePointer = 4 - left;

		//test overwrite of last four bytes
		Assert(*(uint*) (m_pCache + CacheSize) == 0);

		//need to copy XYZW to another location, let's say that X is on the end of stream and YZW on the beginning of new
		//need to glue two uints, in r is one part, in first is second - assuming that the unused bytes of r are zero !
		#if ENF_INTEGER_BIG_ENDIAN
			//BE is simple : in r is e.g. X000, add YZW? from first
			r = r | (first >> (left << 3));
		#else
			//LE is inverse from BE,  in r is 000X and in first is ?WZY
			r = r | (first << (left << 3));
		#endif

		return r;
	}

	/*!
	copy data into dest address
	\param data		destination
	\param size		how much data
	*/
	size_t Read(void *data, size_t size)
	{
		return CopyData((uchar*) data, size);
	}
};



//@}
}
#endif //ENF_STREAMS_H