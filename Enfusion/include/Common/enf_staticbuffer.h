//-----------------------------------------------------------------------------
// File: enf_staticbuffer.h
//
// Desc: Static heap allocator, static string allocator.
//			For cases when there is never deallocation
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_STATICBUFFER_H
#define ENF_STATICBUFFER_H

#include "enf_config.h"
#include "system/enf_memory.h"
#include "El/Math/mathDefs.hpp"

namespace enf
{
/** @addtogroup Containers
@{*/

/*!
Class for static allocations. These are allocations which will
never be freed. It also holds unique strings in effective tree
for fast searching.
*/
class PUBLIC_API StaticBuffer
{
public:
  ENF_DECLARE_SMALLOBJECT(StaticBuffer);

protected:
	char*				BeginData;
	const char*	EndData;
	char*				Buffer;
	size_t			Size;
	bool				m_bMT;

	virtual bool _IsAllocated(const void* ptr) const=0;
	virtual const char* _FindString(const char* string)=0;
	virtual const char* _AddString(const char* string, size_t len)=0;
	virtual const char* _AddString(const char* string)=0;

	ENF_INLINE bool IsAllocatedMTSafe(const void* ptr) const
	{
		return (((char *)ptr) >= Buffer && ((char *)ptr) < BeginData);
	}

public:
	ENF_INLINE size_t GetSize() const
	{
		return Size;
	}

	/*!
	Tells whether the pointer points into allocated data
	\param ptr Pointer to be tested
	\return
	True when pointer points to memory allocated by this StaticBuffer
	*/
	ENF_INLINE bool IsAllocated(const void* ptr) const
	{
		if(m_bMT)
			return _IsAllocated(ptr);
		else
			return IsAllocatedMTSafe(ptr);
	}

	/*!
	Tells how much memory we hve allocated
	\return
	Number of allocated bytes
	*/
	ENF_INLINE size_t Allocated() const
	{
		return (size_t)(BeginData - Buffer);
	}

	const void* GetData() const
	{
		return Buffer;
	}

	/*!
	Adds string to unique collection. It quickly recognize when
	string is already present.
	\param string
	Null terminated string to be inserted
	\return
	Either new allocated unique string, or just string pointer
	when string is already present.
	*/
	ENF_INLINE const char* AddString(const char* string)
	{
		//already from string table
		if(string == NULL || (!m_bMT && IsAllocatedMTSafe(string)))
			return string;
		return _AddString(string);
	}
	/*!
	Adds string to unique collection. It quickly recognize when
	string is already present.
	\param string
	Pointer to string to be inserted. It must not be null terminated
	\param len
	Length of data
	\return
	Either new allocated unique string, or just string pointer
	when string is already present.
	*/
	ENF_INLINE const char* AddString(const char* string, size_t len)
	{
		//already from string table
		if(string == NULL || (!m_bMT && IsAllocatedMTSafe(string)))
			return string;
		return _AddString(string, len);
	}

	/*!
	Finds unique string in collection
	\param string
	Null terminated string to be found
	\return
	Pointer to string if it's found. Null otherwise
	*/
	ENF_INLINE const char* FindString(const char* string)
	{
		//already from string table
		if(string == NULL || (!m_bMT && IsAllocatedMTSafe(string)))
			return string;
		return _FindString(string);
	}

	/*!
	Allocates raw memory and makes copy od supplied data
	\param len
	Length of buffer to be allocated
	\param data
	Pointer to data to be copied into new allocated buffer
	\return
	Pointer to new memory buffer
	*/
	virtual void* AllocData(size_t len, const void *data)=0;

	/*!
	Allocates raw memory
	\param len
	Length of buffer to be allocated
	\return
	Pointer to new memory buffer
	*/
	virtual void* AllocData(size_t len)=0;

	/*!
	Frees allocated data. Can be used just after AllocData and it deallocated
	memory from back of static memory buffer. Must be used carefully!
	\param len
	Number of bytes to be freed
	*/
	virtual void FreeData(size_t len)=0;

	virtual void Free()=0;

	virtual ~StaticBuffer(){}
};

StaticBuffer* CreateStaticBuffer(size_t size, bool threadSafe);

//-----------------------------------------------------------------------------
/*!
StaticMem
*/
//-----------------------------------------------------------------------------
template <size_t size> class StaticMem
{
private:
	char		Buffer[size];
	size_t	Pos;

public:
	ENF_INLINE size_t GetAllocated() const
	{
		return Pos;
	}

	ENF_INLINE void Clear()
	{
		Pos = 0;
	}

	ENF_INLINE void* Alloc(size_t alloc, bool cleared = false)
	{
		Assert((Pos + alloc) < size);

		void* ptr = (void*)(Buffer + Pos);
		if(cleared)
		{
			ZeroMem(ptr, alloc);
		}
		Pos += alloc;
		return ptr;
	}

	ENF_INLINE const char* Alloc(const char* str)
	{
		size_t alloc = strlen(str) + 1;
		char* ptr = (char*)Alloc(alloc);
		strcpy(ptr, str);
		return ptr;
	}

	StaticMem()
	{
		Clear();
	}
};

//@}
}

#endif //ENF_STATICBUFFER_H