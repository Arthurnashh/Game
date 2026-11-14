//-----------------------------------------------------------------------------
// File: enf_memorybuffer.h
//
// Desc: Growable memory buffer
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_MEMORYBUFFER_H
#define ENF_MEMORYBUFFER_H

#include "enf_config.h"
#include "common/enf_inttypes.h"
#include "system/enf_memory.h"

namespace enf
{
/** @addtogroup Common
@{*/

/*!
MemoryBuffer is a dynamic-sized general memorybuffer.
*/
class PUBLIC_API MemoryBuffer
{
public:
	ENF_DECLARE_MMOBJECT(MemoryBuffer);

	/*!
	Construct a new memorybuffer.

	\param length
	Initial length to allocate for the buffer.
	*/
	MemoryBuffer(size_t length = 1024);

	/*!
	Destructor. Deallocates the data associated with this buffer.
	*/
	~MemoryBuffer();

	/*!
	Set current position in the buffer. If the position is past the end of
	buffer, its set to last byte.
	*/
	void Seek(uint pos);

	/*!
	Get current position in the buffer.
	*/
	ENF_INLINE size_t Tell() const
	{
		return m_iPos;
	}

	/*!
	Get the current used size of the buffer in bytes. 
	Use getLength to get the total buffer length (which may be larger).
	*/
	ENF_INLINE size_t GetSize() const
	{
		return m_iSize;
	}

	/*!
	Return true if we reached last byte in buffer.
	*/
	ENF_INLINE bool Eof() const
	{ 
		return (m_iPos == m_iSize);
	}

	/*!
	Write data into the buffer. The buffer will grow if needed.

	\param buffer
	Data to write into the MemoryBuffer
	\param length
	Length of the data in buffer in bytes.
	*/
	void Write(const void* buffer, size_t length);

	/*!
	Read data from the buffer.

	\param buffer
	Data to read from the MemoryBuffer. The buffer must be of sufficient
	size to hold the data.
	\param length
	Length of the data to read in bytes.
	*/
	int Read(void* buffer, size_t length);

	/*!
	Grow the buffer to new size.
	This function cannot be used to shrink the buffer.

	\param newMinLength
	New minimum length that the buffer should be able to hold.
	*/
	void Grow(size_t newMinLength);

	/*!
	Release the old buffer and allocate a new one.

	\param newLength
	New minimum length that the buffer should be able to hold.
	*/
	void Reallocate(size_t newLength);

	/*!
	Get the length of the buffer.
	Use getSize if you want to know how much of the buffer that is used.
	*/
	ENF_INLINE size_t GetLength() const
	{
		return m_iLength;
	}

	/*!
	Get a raw pointer to the data kept in this buffer.
	*/
	ENF_INLINE unsigned char* GetData()
	{
		return m_pBuffer;
	}

	/*!
	Get a raw pointer to the data kept in this buffer.
	*/
	ENF_INLINE const unsigned char* GetData() const
	{
		return m_pBuffer;
	}

	/*!
	Allocates space and returns pointer to the buffer.
	It's valid only until another call, causing reallocation
	\param size
	Space to be allocated
	*/
	ENF_INLINE unsigned char* Append(size_t size)
	{
		Grow(m_iPos + size);
		m_iSize += size;
		return m_pBuffer + m_iPos;
	}

protected:
	unsigned char*	m_pBuffer;
	size_t			m_iLength;
	size_t			m_iPos;
	size_t			m_iSize;
};
//@}
}
#endif

