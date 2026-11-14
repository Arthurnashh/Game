//-----------------------------------------------------------------------------
// File: enf_set.h
//
// Desc: Sorted array
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#pragma once

#ifndef ENF_SET_H
#define ENF_SET_H

#include "enf_config.h"
#include "system/enf_memory.h"
#include <string.h> //because of memcpy/memmove

namespace enf
{
/** @addtogroup Containers
@{*/

#define ENF_SET_DEFAULT_SIZE 0

//-----------------------------------------------------------------------------
template <class T> uint BinarySearchEQ(const T* data, uint count, const T& a)
{
	int l = 0;
	int r = count - 1;
	int i;
	for(;r >= l;)
	{
		i = (l + r) / 2;
		if(data[i] > a)
		{
			r = i - 1;
		}
		else
		{
			if(data[i] == a)
			{
				return i;
			}
			l = i + 1;
		}
	}
	return (unsigned)-1;
}

template <class T> uint BinarySearchGE(const T* data, uint count, const T &a)
{
	int last = count - 1;
	int l = 0;
	int r = last;
	int i;
	for(;r >= l;)
	{
		i = (l + r) / 2;
		if (data[i] > a)
		{
			r = i - 1;
		}
		else
		{
			//we want to avoid <= operator which is not common
			//if(!(data[i] > a) && ((i == ((int)count - 1)) || data[i + 1] > a))
			if (i == last || data[i + 1] > a)
			{
				return i;
			}
			l = i + 1;
		}
	}
	return (unsigned)-1;
}

//-----------------------------------------------------------------------------
/*!
Set
*/
//-----------------------------------------------------------------------------
template <class T, class TAllocator = DefaultAllocator> class Set
{
public:

	typedef T ElementType;

	#if defined(ENF_PS3) || defined(__GCC__)
		TAllocator Alloc;
	#else
		typename TAllocator Alloc;
	#endif


	//-----------------------------------------------------------------------
	/*!
	Default Set constructor.
	Allocates enough room for SET_DEFAULT_SIZE elements.
	*/
	//-----------------------------------------------------------------------
	Set(): size(0), count(0), data1(NULL)
	{
		if (ENF_SET_DEFAULT_SIZE > 0)
			Grow(ENF_SET_DEFAULT_SIZE);
	}

	Set(const TAllocator& init, uint cnt = 0): count(cnt), Alloc(init)
	{
		data1 = (T*)Alloc.GetBuffer();
		size = Alloc.GetSize();
	}


	//-----------------------------------------------------------------------
	/*!
	Set constructor.
	Allocates enough room for size elements. 

	\param nsize
	Size of set to be defined. 
	*/
	//-----------------------------------------------------------------------
	Set(unsigned int nsize):
		count(0),
		size(0),
		data1(NULL)
	{
		if (nsize > 0)
		{
			Grow(nsize);
		}
	}

	void Reserve(unsigned int newsize)
	{
		if(this->size < newsize)
			Grow(newsize);
	}

	void SetSize(unsigned int newsize)
	{
		if(this->size != newsize)
		{
			if(newsize == 0)
			{
				FreeAll();
				data1 =  NULL;
				size = 0;
			}
			else
				Grow(newsize);
		}

		count = newsize;
	}

	void Flush(uint level)
	{
		if(level == ENF_CLEANUP_FLUSHLEVEL || (count == 0 && data1))
		{
			FreeAll();
			data1 = NULL;
			count = size = 0;
			return;
		}

		if(count < size)
			Grow(count);
	}

	//-----------------------------------------------------------------------
	/*!
	Set Constructor.

	\param array[]
	Specifies an array with sorted data.
	\param size
	Specifies the maximum number of array entries.
	*/
	//-----------------------------------------------------------------------
	Set(const T array[], const unsigned int size)
	{
		data1 = NULL;
		count = size;
		if (size > 0)
		{
			Grow(size);
			for(unsigned int n = 0; n < size; n++)
				data1[n] = array[n];
		}
	}

	//-----------------------------------------------------------------------
	/*!
	Set copy constructor.

	\param s
	The Set that will be copied.
	*/
	//-----------------------------------------------------------------------
	Set(const Set<T> &s):
		count(0),
		size(0),
		data1(NULL)
	{
		if(s.count > 0)
		{
			Grow(s.count);
			count = s.count;
			for(unsigned int n = 0; n < count; n++)
				data1[n] = s[n];
		}
	}
	
   //-----------------------------------------------------------------------
	/*!
	Default destructor.	
	*/
	//-----------------------------------------------------------------------
	~Set()
	{
		FreeAll();
	}

	//-----------------------------------------------------------------------
	ENF_INLINE void Swap(Set<T>& set)
	{
		enf_swap(data1, set.data1);
		enf_swap(size, set.size);
		enf_swap(count, set.count);
	}

	//-----------------------------------------------------------------------
	/*!
	Return an element from this set. Operation is O(1).
	\param i
	The index of the element that will be returned.
	\return
	Element i in the array.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE T& GetElement(unsigned int i) const
	{
		Assert(i < count);
		return data1[i];
	}


	//-----------------------------------------------------------------------
	/*!
	Return an element from this set. Operation is O(1).
	\param i
	The index of the element that will be returned.
	\return
	Element i in the array.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE T& GetElement(unsigned int i)
	{
		Assert(i < count);
		return data1[i];
	}

	//-----------------------------------------------------------------------
	/*!
	Return the last element from this set. Operation is O(1).
	\return
	Element (cardinality-1) in the set.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE T& GetLast()
	{
		Assert(!IsEmpty());
		return data1[GetCardinality() - 1];
	}

	//-----------------------------------------------------------------------
	/*!
	Return the last element from this set. Operation is O(1).
	\return
	Element (cardinality-1) in the set.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE const T& GetLast() const
	{
		Assert(!IsEmpty());
		return data1[GetCardinality() - 1];
	}


	//-----------------------------------------------------------------------
	/*!
	Set indexing operator.
	\param i
	The index of the element that will be returned.
	\return
	The element i in the array.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE T& operator[] (unsigned int i) const
	{
		Assert(i < count);
		return data1[i];
	}

	//-----------------------------------------------------------------------
	/*!
	Set indexing operator.
	\param i
	The index of the element that will be returned.
	\return
	The element i in the array.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE T& operator[] (unsigned int i)
	{
		Assert(i < count);
		return data1[i];
	}

	//-----------------------------------------------------------------------
	/*!
	Returns the sets array representation.
	\return
	The sets representation.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE const T* GetArray() const
	{
		return data1;
	}


	//-----------------------------------------------------------------------
	/*!
	Returns the number of entries in the set. This can, and probably will,
	differ from the array size.
	\return
	The number of of entries in the set.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE unsigned int GetCardinality() const
	{	//number of elements
		return count;
	}

	ENF_INLINE uint GetMemorySize() const { return count * sizeof(T); }

	//-----------------------------------------------------------------------
	/*!
	Returns true if the set is empty.
	\return
	True if the set has no entries, false otherwise.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE bool IsEmpty() const
	{
		return (count == 0);
	}


	//-----------------------------------------------------------------------
	/*!
	Clears the set.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE void Clear()
	{
		count = 0;
	}


	//-----------------------------------------------------------------------
	/*!
	Return index of a specific element.

	\param a  
	The element to search for.
	\return
	The index of this element, or -1 if not found.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE int GetIndexOf(const T &a) const
	{
		return BinarySearchEQ<T>(data1, count, a);
	}

	ENF_INLINE int FindGE(const T &a) const
	{
		return BinarySearchGE<T>(data1, count, a);
	}
	//-----------------------------------------------------------------------
	/*!
	Determine if this set contains a specific element.

	\param a
	The element
	\return
	True if the element is in the set.
	False if the element is not in the set.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE bool Contains(const T &a) const
	{
		return GetIndexOf(a) != -1;
	}

	//-----------------------------------------------------------------------
	/*!
	Insert a element to the set if it does not already exists within it. 
	If this Set is full it will be expanded to hold the new element. 

	\param a
	The element that will be added to this set.
	\param index
	Optional pointer to index where element is inserted.
	\return
	True if the element was inserted.
	False if the element was already in the set.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE bool Insert(const T &a, unsigned int* index = NULL)
	{
		int p = 0;
		int r = count - 1;
		int i;
		for(;r >= p;)
		{
			i = (p + r) / 2;
			if(data1[i] > a)
			{
				r = i - 1;
			}
			else
			{
				if(data1[i] == a)
				{
					if (index) *index = i;
					return false;
				}
				p = i + 1;
			}
		}

		//move the elements [l count-1] to [l+1 count]
		if (size <= count)
		{
			if(&a >= data1 && &a < &data1[count])
			{
				//rare case, when we insert item from the same array
				T temp = a;
				Grow(1 + size * 2);
				if (count - p > 0)
					memmove(&data1[p + 1], &data1[p], (count - p) * sizeof(T));
				
				PLACEMENT_NEW(&data1[p]) T;

				data1[p] = temp;
				count++;
				if (index) *index = p;
				return true;
			}

			Grow(1 + size * 2);
		}
		if (count - p > 0)
			memmove((void*)&data1[p + 1], (void*)&data1[p], (count - p) * sizeof(T));
		
		//initialize this element as new one, as it was duplicated to [p + 1] position
		PLACEMENT_NEW(&data1[p]) T;

		data1[p] = a;
		count++;
		if (index) *index = p;
		return true;
	}


	//-----------------------------------------------------------------------
	/*!
	Subtracts the element on position i from the set.
	Operation is O(n). Fastest removal on high index elements.
	\param i
	The position of the element that will be subtracted from this set.
	\return
	The element that was removed.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE T RemoveElement(unsigned int i)
	{
		Assert(i < count);
		T tmp = data1[i];
		//destroy removed element
		(data1+i)->~T();
		memmove(data1 + i, data1 + i + 1, (count - i - 1) * sizeof(T));
		//reinitialize element at the end, which become duplicated now
		PLACEMENT_NEW(&data1[count - 1]) T;

		count--;
		return tmp;
	}

	//-----------------------------------------------------------------------
	/*!
	Remove one element from the set, if it exists.
	\param a
	The element that will be subtracted from this set.
	\return
	True if the element was removed. False if no 
	element was found.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE bool Remove(const T &a)
	{
		int l = 0;
		int r = count - 1;
		int i;
		for(;r >= l;)
		{
			i = (l + r) / 2;
			if(data1[i] > a)
			{
				r = i - 1;
			}
			else
			{
				if(data1[i] == a)
				{
					RemoveElement(i);
					return true;
				}
				l = i + 1;
			}
		}
		return false;
	}


	//-----------------------------------------------------------------------
	/*!
	Insert a given set to this set, more formaly called a union between the sets.
	\param s
	The set that will be added to this one.
	*/
	//-----------------------------------------------------------------------	
	ENF_INLINE void Insert(const Set& s)
	{
		uint count2 = s.GetCardinality();

		if(count2 == 0)
			return;
		
		uint count1 = GetCardinality();

		uint newSize = 2 * count1 + count2;

		//Make room for this set + this set and other set
		if(this->size < newSize)
			Grow(newSize);

		T* data2 = data1 + GetCardinality();

		unsigned int i = 0, j = 0, k = 0; //index in local, origin and destination arrays

		if(count == 0)
		{
			*this = s;
			return;
		}
		while(true)
		{
			if(data1[i] < s.GetElement(j))
			{
				data2[k++] = data1[i++];
			}
			else
			{
				if(data1[i] > s.GetElement(j))
				{
					data2[k++] = s.GetElement(j++);
				}
				else
				{
					data2[k++] = data1[i++];
					j++;
				}
			}
			if(i == count)
			{
				memcpy(data2 + k, s.data1 + j, (s.GetCardinality() - j) * sizeof(T));
				k += s.GetCardinality() - j;
				break;
			}

			if(j == s.GetCardinality())
			{
				memcpy(&data2[k], &data1[i], (count - i) * sizeof(T));
				k += count - i;
				break;
			}
		}
		memmove(data1, data2, k * sizeof(T));
		count = k;
	}


	//-----------------------------------------------------------------------
	/*!
	Remove a given set from this set, more formaly called a difference between
	this set and the given one.
	\param s
	The given set.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE void Remove(const Set& s)
	{
		if (s.IsEmpty()) return;

		T* data2 = data1 + GetCardinality();

		unsigned int i = 0, j = 0, k = 0; //index in local, foreign and destination array
		if(s.GetCardinality() == 0) return;
		if(count == 0) return;

		while(true)
		{
			if(data1[i] < s.GetElement(j))
			{
				data2[k++] = data1[i++];
			}
			else
			{
				if(data1[i] == s.GetElement(j)) i++;
				j++;
			}
			if(i == count) break;
			if(j == s.GetCardinality())
			{
				memcpy(data2 + k, data1 + i, (count - i) * sizeof(T));
				k += count - i;
				break;
			}
		}
		memmove(data1, data2,k * sizeof(T));
		count = k;
	}

	Set<T>& operator=(const Set<T>& set)
	{
		Clear();
		if(size < set.GetCardinality())
			Grow(set.GetCardinality());

		count = set.GetCardinality();
		for(uint i = 0; i < set.GetCardinality(); i++)
			data1[i] = set.data1[i];

		return *this;
	}

	//-----------------------------------------------------------------------
	/*!
	\return
	Returns the number of elements the current array can hold.
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE unsigned int GetArraySize()
	{
		return size;
	}

private:
	T *data1;
	unsigned int size;
	unsigned int count;

	void FreeAll()
	{
		//destroy all elements
		for(unsigned int i = 0; i < size; i++)
		{
			(data1+i)->~T();
		}
		if (data1) Alloc.Free(data1);
	}

	//-----------------------------------------------------------------------
	/*!
	Resizes the available memory for the array representing the set.

	\param nsize
	The number of entries that the set should be able to hold.
	*/
	//-----------------------------------------------------------------------
	void Grow(unsigned int nsize)
	{
		data1 = (T*)Alloc.ReAlloc(data1, nsize * sizeof(T));

		Assert(data1);

		//call constructors for new objects
		for(unsigned int n = this->size; n < nsize; n++)
		{
			T* tmp=data1 + n;
			PLACEMENT_NEW(tmp) T;
		}

		this->size = nsize;
	}

};

//-----------------------------------------------------------------------------
/*!
Pair is convenient way when you want to store some types into Set
but you want to sort it by other key than the elements itself.
K stands for key element and T stands for type you want to order
*/
//-----------------------------------------------------------------------------
template <class K, class T> struct Pair
{
	K		Key;
	T		Data;

	bool operator==(const Pair<K, T>& other) const
	{
		return (Key == other.Key);
	}

	bool operator<(const Pair<K, T>& other) const
	{
		return (Key < other.Key);
	}

	bool operator>(const Pair<K, T>& other) const
	{
		return (Key > other.Key);
	}

	Pair() {	}
	Pair(const K& key): Key(key) { }
	Pair(const K& key, const T& data): Key(key), Data(data) { }
};

//!Creates Pair based upon used types
template <class K, class D> Pair<K,D> MakePair(const K& k, const D& d) {return Pair<K,D>(k,d);}
//@}
}
#endif //ENF_SET_H