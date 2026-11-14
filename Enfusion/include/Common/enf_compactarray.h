//-----------------------------------------------------------------------------
// File: enf_compactarray.h
//
// Desc: Compacted array of unique values, order is kept as values are inserted
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#pragma once

#ifndef ENF_COMPACTARRAY_H
#define ENF_COMPACTARRAY_H

#include "enf_config.h"
#include "system/enf_memory.h"
#include "common/enf_types.h"
#include "common/enf_set.h"

namespace enf
{
/** @addtogroup Containers
@{*/

template<class T> class CompactArray
{
private:
	Array<T>		m_Values;
	typedef Pair<T, uint>	TIndex;
	Set<TIndex>	m_Set;

public:

	typedef T ElementType;

	/*!
	Default CompactArray constructor.
	*/
	CompactArray()
	{
	}

	//-----------------------------------------------------------------------
	/*!
	CompactArray constructor.
	Allocates enough room for size elements. 
	\param nsize
	Size of set to be defined. 
	*/
	//-----------------------------------------------------------------------
	CompactArray(unsigned int nsize): m_Set(nsize), m_Values(nsize)
	{
	}

	void Clear()
	{
		m_Values.Clear();
		m_Set.Clear();
	}

	ENF_INLINE uint GetIndexOf(const T& v) const
	{
		uint index = m_Set.GetIndexOf(MakePair(v, uint(0)));
		if(index != -1)
			return m_Set[index].Data;
		return -1;
	}

	ENF_INLINE bool Contains(const T &a) const
	{
		return GetIndexOf(a) != -1;
	}

	ENF_INLINE uint Insert(const T& v)
	{
		uint index = GetIndexOf(v);
		if(index != -1)
			return index;

		index = m_Values.Insert(v);
		m_Set.Insert(MakePair(v, index));
		return index;
	}

	ENF_INLINE const T& operator[] (uint i) const
	{
		return m_Values[i];
	}

	ENF_INLINE T& operator[] (uint i)
	{
		return m_Values[i];
	}

	ENF_INLINE const T& GetElement(uint i) const
	{
		return m_Values[i];
	}

	ENF_INLINE T& GetElement(uint i)
	{
		return m_Values[i];
	}

	ENF_INLINE T& GetLast()
	{
		return m_Values.GetLast();
	}

	void Flush(uint level)
	{
		m_Values.Flush(level);
		m_Set.Flush(level);
	}

	ENF_INLINE void Swap(CompactArray<T>& carray)
	{
		m_Set.Swap(carray.m_Set);
		m_Values.Swap(carray.m_Values);
	}

	//-----------------------------------------------------------------------
	/*!
	Returns the array representation.
	\return
	The sets representation.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE const T* GetArray() const
	{
		return m_Values.GetArray();
	}

	ENF_INLINE const Array<T>& GetValues() const
	{
		return m_Values;
	}
	//-----------------------------------------------------------------------
	/*!
	Returns the number of entries in the array. This can, and probably will,
	differ from the array size.
	\return
	The number of of entries in the array.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE unsigned int GetCardinality() const
	{
		return m_Values.GetCardinality();
	}


	//-----------------------------------------------------------------------
	/*!
	Returns true if the array is empty.
	\return
	True if the set has no entries, false otherwise.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE bool IsEmpty() const
	{
		return m_Values.IsEmpty();
	}
};
//@}

}
#endif //ENF_COMPACTARRAY_H