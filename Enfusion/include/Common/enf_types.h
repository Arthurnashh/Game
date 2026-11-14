//-----------------------------------------------------------------------------
// File: enf_types.h
//
// Desc: Basic types
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#pragma once

#ifndef ENF_TYPES_H
#define ENF_TYPES_H

#include "enf_config.h"
#include "common/enf_inttypes.h"
#include "system/enf_memory.h"
#include "system/enf_interlocked.h"

#ifdef ENF_XENON
	#include <xtl.h>
#else
	#include <string.h>
#endif

#ifdef ENF_SSE
	#include <xmmintrin.h>
#endif

#ifdef ENF_PS3
	#include <ppu_intrinsics.h>
	#define __fself(a, b, c) __fsels(a, b, c)
#endif

#ifdef __GCC__
	#include <stddef.h>
#endif

#define ENF_MAKEFOURCC(ch0, ch1, ch2, ch3) \
	((enf::uint)(enf::ubyte)(ch0) | ((enf::uint)(enf::ubyte)(ch1) << 8) | ((enf::uint)(enf::ubyte)(ch2) << 16) | ((enf::uint)(enf::ubyte)(ch3) << 24 ))

namespace enf
{
/** @defgroup Common Common types, containers and libs
@{*/

typedef uint TJobHandle;
const TJobHandle BAD_JOBHANDLE(0xffffffffUL);


#ifdef __GCC__
	#define STRUCT_MEMBER_OFFSET(t, m) offsetof(t, m)
	
#else
	#define STRUCT_MEMBER_OFFSET(t, m) ((int)&(((t *)0L)->m))
#endif


/*!
Returns true if the provided types are the same
*/
template< typename T, typename U >
struct IsSameType
{
	enum { value = false };
};

template< typename T >
struct IsSameType< T, T >
{
	enum { value = true };
};

//-----------------------------------------------------------------------------
/*!
return -1.0 if f is negative, otherwise 1.0
timing:	signfloat			 4.5236
f>=0?1.0:-1.0		 15.843
\param f	
*/
//-----------------------------------------------------------------------------
ENF_INLINEFUNC float enf_signfloat (float f)
{
#ifdef ENF_POWERPC
	return __fself(f, 1.0f, -1.0f);
#else
	uint intval = *((uint*) &f);
	uint res = 0x3F800000 | (intval & 0x80000000);
	return *(float*) &res;
#endif
}



//-----------------------------------------------------------------------------
/*!
return -1 if i is negative, otherwise zero is positive
\param i		signed integer	
*/
//-----------------------------------------------------------------------------
ENF_INLINEFUNC int enf_signint (int i)
{
	//works for signed int! if is less then zero, arithmetic shifting lets the correct sign
	return 1 | (i >> 31);
}



//-----------------------------------------------------------------------------
/*!
return value bigger than one if int value is negative (0x8000000)
\param i		signed integer	
*/
//-----------------------------------------------------------------------------
ENF_INLINEFUNC int enf_isnegative (int i)
{
	return i & 0x80000000;
}



//-----------------------------------------------------------------------------
/*!
return maximum of two numbers
\param a		one number
\param b		second number
*/
//-----------------------------------------------------------------------------
template<class T> ENF_INLINEFUNC T enf_max(T a, T b) 
{
	return (a > b) ? a : b;
}


//-----------------------------------------------------------------------------
/*!
return minimum of two numbers
\param a		one number
\param b		second number
*/
//-----------------------------------------------------------------------------
template<class T> ENF_INLINEFUNC T enf_min(T a, T b) 
{
	return (a < b) ? a : b;
}

//-----------------------------------------------------------------------------
/*!
return number clamped to interval
\param a		number
\param min	minimum
\param max	maximum
*/
//-----------------------------------------------------------------------------
template<class T> ENF_INLINEFUNC T enf_limit(T a, T min, T max) 
{
	return (a < min) ? min : ((a > max) ? max : a);
}


//-----------------------------------------------------------------------------
/*!
GPU saturate to <0..1>
\param a		number
*/
//-----------------------------------------------------------------------------
ENF_INLINEFUNC float enf_saturate(float a) 
{
	return enf_limit(a, 0.0f, 1.0f);
}



#ifdef ENF_POWERPC
//specialized variant for PowerPC
ENF_INLINEFUNC float enf_min(float a, float b) 
{
	return __fself(a - b, b, a);
}

ENF_INLINEFUNC float enf_max(float a, float b) 
{
	return __fself(a - b, a, b);
}

ENF_INLINEFUNC float enf_limit(float a, float min, float max) 
{
	return enf_max(enf_min(a, max), min);
}

#endif


//-----------------------------------------------------------------------------
/*!
return true when number is in interval, the borders of interval returns true
-> a >= min && a <= max
\param a		number
\param min	minimum
\param max	maximum
\return ...
*/
//-----------------------------------------------------------------------------
template<class T> ENF_INLINEFUNC bool enf_in(T a, T min, T max) 
{
	enf_assert(min <= max);
	return (a >= min && a <= max);
}


//-----------------------------------------------------------------------------
/*!
return true when number is in interval, the borders of interval returns false
-> a > min && a < max
\param a		number
\param min	minimum
\param max	maximum
\return ...
*/
//-----------------------------------------------------------------------------
template<class T> ENF_INLINEFUNC bool enf_in_sharp(T a, T min, T max) 
{
	enf_assert(min <= max);
	return (a > min && a < max);
}


//-----------------------------------------------------------------------------
/*!
return true when number is in interval, left is exclusive -> a > min && a <= max
\param a		number
\param min	minimum
\param max	maximum
\return ...
*/
//-----------------------------------------------------------------------------
template<class T> ENF_INLINEFUNC bool enf_in_sharpmin(T a, T min, T max) 
{
	enf_assert(min <= max);
	return (a > min && a <= max);
}


//-----------------------------------------------------------------------------
/*!
return true when number is in interval, left is exclusive -> a >= min && a < max
\param a		number
\param min	minimum
\param max	maximum
\return ...
*/
//-----------------------------------------------------------------------------
template<class T> ENF_INLINEFUNC bool enf_in_sharpmax(T a, T min, T max) 
{
	enf_assert(min <= max);
	return (a >= min && a < max);
}


//-----------------------------------------------------------------------------
/*!
swap two types
\param a1	first 
\param a2	second 
*/
//-----------------------------------------------------------------------------
template<class T> ENF_INLINEFUNC void enf_swap(T &a1, T &a2) 
{
	T tmp = a1;
	a1 = a2;
	a2 = tmp;
}





template<class T> class RefCounter
{
	T& Ref;

public:
	RefCounter(T& ref): Ref(ref)
	{
		Ref++;
	}

	~RefCounter()
	{
		Ref--;
	}
};

//-----------------------------------------------------------------------------
/*!
smart pointer for COM objects and RefCountObject descendants
*/
//-----------------------------------------------------------------------------
template<class T> class ComPtr
{
private:
	T* Ptr;

public:
	ENF_INLINE ComPtr(): Ptr(NULL)
	{
	}

	ENF_INLINE ComPtr(T* p):
		Ptr(p)
	{
	}

	ENF_INLINE bool IsNull() const
	{
		return (Ptr == NULL);
	}

	ENF_INLINE T* operator->() const
	{
		return Ptr;
	}

	ENF_INLINE T** Init()
	{
		Release();
		return &Ptr;
	}

	ENF_INLINE uint Release()
	{
		if(Ptr == NULL)
			return 0;

		uint refc = Ptr->Release();
		Ptr = NULL;
		return refc;
	}

	ENF_INLINE uint AddRef()
	{
		if(Ptr)
			return Ptr->AddRef();
		return 0;
	}
	ENF_INLINE void operator=(const ComPtr& ref)
	{
		T* src = ref.Ptr;
		if(src != Ptr)
		{
			if(src)
				src->AddRef();

			Release();
			Ptr = src;
		}
	}

	ENF_INLINE void operator=(T* src)
	{
		if(src != Ptr)
		{
			Release();
			Ptr = src;
		}
	}

	ENF_INLINE T* SetRef(T* src)
	{
		if(src != Ptr)
		{
			if(src)
				src->AddRef();

			Release();
			Ptr = src;
		}
		return Ptr;
	}

	ENF_INLINE ComPtr(const ComPtr& src):
		Ptr(src.Ptr)
	{
		if(Ptr) Ptr->AddRef();
	}

	ENF_INLINE operator T*() const
	{
		return Ptr;
	}

	ENF_INLINE ~ComPtr()
	{
		Release();
	}
};

//-----------------------------------------------------------------------------
/*!
RefCountObject
*/
//-----------------------------------------------------------------------------
class PUBLIC_API RefCountObject
{
protected:
	IL_INT32	m_iRefs;
	virtual ~RefCountObject() {}

	RefCountObject(): m_iRefs(1)
	{
	}

public:

	//!Returns number of references on this object
	ENF_INLINE int RefCount() const
	{
		return m_iRefs;
	}

	/*!
	Adds reference on this object
	\return
	Current number of references
	*/
	ENF_INLINE int AddRef()
	{
		return IL_INCREMENT(m_iRefs);
	}

	/*!
	Removes one reference on object. Must be used very
	carefully as it doesn't release object when references==0!
	\return
	Current number of references
	*/
	ENF_INLINE int Unref()
	{
		enf_assert(m_iRefs > 0);
		return IL_DECREMENT(m_iRefs);
	}

	/*!
	This default implementation removes one reference
	and calls destructor when we reach zero.
	\return
	Number of references, or zero, when object is deleted
	*/
	virtual int Release()
	{
		int refCount = Unref();
		if(refCount == 0)
		{
			delete this;
			return 0;
		}
		return refCount;
	}
};

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

/*!
Use it as base for classes that cannot be copied
*/
class PUBLIC_API CDisableCopy
{
public:
  CDisableCopy(){}

private:
  CDisableCopy(const CDisableCopy &src);
  CDisableCopy &operator =( const CDisableCopy &src);
};


/*!
Utility class for locking a object for scope-lifetime
Lock()/Unlock() methods must exist in the object class
*/
template<class T> class ScopeLock: public CDisableCopy
{
private:
	T& object;

public:
	ScopeLock(T& obj): object(obj) { object.Lock(); }
	~ScopeLock() { object.Unlock(); }
};


//-----------------------------------------------------------------------------
/*!
Flag definition
*/
//-----------------------------------------------------------------------------
template <class T = uint> struct Flags
{
	T	m_Flags;

	Flags(): m_Flags(0) {}
	Flags(T flags): m_Flags(flags) {}

	//-----------------------------------------------------------------------------
	/*!
	set flag/s
	\param flags Set flag/s
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE const Flags<T>& Set(T flags)
	{
		m_Flags |= flags;
		return *this;
	}

	ENF_INLINE const Flags<T>& Set(T flags, bool condition)
	{
		m_Flags = condition ? (m_Flags | flags) : (m_Flags & ~flags);
		return *this;
	}

	//-----------------------------------------------------------------------------
	/*!
	Clear flag/s
	\param flags Cleared flag/s
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE void Clear(T flags)
	{
		m_Flags &= ~flags;
	}

	//-----------------------------------------------------------------------------
	/*!
	true/false if any of flag/s is/are set
	\param flags	tested flag/s
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE bool IsAnySet(T flags) const
	{
		return (m_Flags & flags) != 0;
	}

	//-----------------------------------------------------------------------------
	/*!
	true/false if all of flag/s is/are set
	\param flags	tested flag/s
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE bool IsSet(T flags) const
	{
		return (m_Flags & flags) == flags;
	}

	//-----------------------------------------------------------------------------
	/*!
	clear flags
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE void Clear()
	{
		m_Flags = 0;
	}

	//-----------------------------------------------------------------------------
	/*!
	set to some value
	\param value	New value
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE void Init(T value)
	{
		m_Flags = value;
	}

	//-----------------------------------------------------------------------------
	/*!
	==
	\param src src
	*/
	//-----------------------------------------------------------------------------
	ENF_INLINE bool operator==(const Flags<T> src) const
	{
		return m_Flags == src.m_Flags;
	}
};

//!32 bit flags
typedef Flags<uint>	Flags32;

//!16 bit flags
typedef Flags<ushort>	Flags16;

//!8 bit flags
typedef Flags<uchar>	Flags8;
//@}



/** @defgroup Containers Containers
@ingroup Common
@{*/

//-----------------------------------------------------------------------------
/*!
Dynamic array
*/
//-----------------------------------------------------------------------------
template <class T, class TAllocator = DefaultAllocator> class Array
{
public:

	typedef T ElementType;

	#if defined(ENF_PS3) || defined(__GCC__)
		TAllocator Alloc;
	#else
		typename TAllocator Alloc;
	#endif

	//-----------------------------------------------------------------------
	void Reserve(uint newsize)
	{
		if(this->size < newsize)
			Grow(newsize);
	}

	//-----------------------------------------------------------------------
	void SetSize(uint newsize)
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

	//-----------------------------------------------------------------------
	void Flush(uint level)
	{
		if(level == ENF_CLEANUP_FLUSHLEVEL || (count == 0 && data1))
		{
			FreeAll();
			data1 = NULL;
			count = size = 0;
			return;
		}

		//is it worth to free?
		if(level == 0)
		{
			if(((size * sizeof(T)) - (count * sizeof(T))) > 256)
				Grow(count);
		}
		else
		{
			if(count < size)
				Grow(count);
		}
	}

	ENF_INLINE void Swap(Array<T>& array)
	{
		enf_swap(data1, array.data1);
		enf_swap(size, array.size);
		enf_swap(count, array.count);
	}

	//-----------------------------------------------------------------------
	Array(): count(0), size(0), data1(NULL) {}
	Array(const TAllocator& init, uint cnt = 0): count(cnt), Alloc(init)
	{
		data1 = (T*)Alloc.GetBuffer();
		size = Alloc.GetSize();
	}


	//-----------------------------------------------------------------------
	Array(uint nsize): count(0), size(0), data1(NULL)
	{
		Grow(nsize);
	}


	//-----------------------------------------------------------------------
	Array(const Array<T> &a): count(0), size(0), data1(NULL)
	{
		if (a.count > 0)
		{
			Grow(a.count);
			for(uint i = 0; i < a.count; i++)
			{
				data1[i] = a[i];
			}
			count = a.count;
		}
		else
		{
			//destroy all elements
			for(uint i = 0; i < size; i++)
			{
				(data1+i)->~T();
			}
			if (data1)
			{
				Alloc.Free(data1);
				data1 = NULL;
			}
			size = 0;
			count = 0;
		}
	}

	//-----------------------------------------------------------------------
	~Array() { FreeAll(); }

	//-----------------------------------------------------------------------
	ENF_INLINE const T& operator[] (uint i) const
	{
		enf_assert(i < count);
		return data1[i];
	}

	//-----------------------------------------------------------------------
	ENF_INLINE T& operator[] (uint i)
	{
		enf_assert(i < count);
		return data1[i];
	}

	//-----------------------------------------------------------------------
	ENF_INLINE const T& GetElement(uint i) const
	{
		enf_assert(i < count);
		return data1[i];
	}

	//-----------------------------------------------------------------------
	ENF_INLINE T& GetElement(uint i)
	{
		enf_assert(i < count);
		return data1[i];
	}

	//-----------------------------------------------------------------------
	ENF_INLINE T& GetLast()
	{
		enf_assert(count > 0);
		return data1[count - 1];
	}

	//-----------------------------------------------------------------------
	ENF_INLINE const T& GetLast() const
	{
		enf_assert(count > 0);
		return data1[count - 1];
	}

	//-----------------------------------------------------------------------
	ENF_INLINE const T* GetArray() const { return data1; }
	ENF_INLINE T* GetArray() { return data1; }
	ENF_INLINE uint GetCardinality() const { return count; }
	ENF_INLINE uint GetMemorySize() const { return count * sizeof(T); }
	ENF_INLINE bool IsEmpty() const { return (count == 0); }
	ENF_INLINE void Clear() { count = 0; }

	//-----------------------------------------------------------------------
	ENF_INLINE uint GetIndexOf(const T &a) const
	{
		uint i;
		for(i = 0; i < count; i++)
		{
			if(data1[i] == a)
				return i;
		}
		return (unsigned)-1;
	}

	//-----------------------------------------------------------------------
	/*!
	Inserts element at the end of array.
	\param a
	Element to be inserted
	\return
	Position at which element is inserted
	*/
	//-----------------------------------------------------------------------
	ENF_INLINE uint Insert(const T &a)
	{
		if(size <= count)
		{
			if(&a >= data1 && &a < &data1[count])
			{
				T objCopy = a;
				Grow(size * 2 + 1);
				data1[count] = objCopy;

				return count++;
			}
			else
			{
				Grow(size * 2 + 1);
			}
		}
		data1[count] = a;

		return count++;
	}

	ENF_INLINE T& Append()
	{
		if(size <= count)
			Grow(size * 2 + 1);
		return data1[count++];
	}

	//-----------------------------------------------------------------------
	/*!
	Inserts element at certain position and moves all elements behind
	this position by one.
	\param a
	Element to be inserted
	\param index
	Position at which element is inserted. Must be less than Array::GetCardinality()
	\return
	Number of elements after insertion
	*/
	ENF_INLINE uint InsertAt(const T &a, uint index)
	{
		enf_assert(index <= count);
		if(size <= count)
		{
			if(&a >= data1 && &a < &data1[count])
			{
				T objCopy = a;
				Grow(size * 2 + 1);
				data1[count] = objCopy;

				return count++;
			}
			else
			{
				Grow(size * 2 + 1);
			}
		}

		memmove(&data1[index + 1], &data1[index], (count - index) * sizeof(T));
		//reinitialize the element, which become duplicated now
		PLACEMENT_NEW(&data1[index]) T;

		data1[index] = a;

		return ++count;
	}

	//-----------------------------------------------------------------------
	ENF_INLINE uint Push(const T &a)
	{
		return Insert(a);
	}

	//-----------------------------------------------------------------------
	ENF_INLINE T& Pop()
	{
		enf_assert(count > 0);
		count--;
		return data1[count];
	}

	//-----------------------------------------------------------------------
	void Insert(const Array<T>& a)
	{
		if (a.IsEmpty())
			return;

		if(size <= count + a.count)
			Grow((count + a.count + 1) * 2);

		for(uint n = 0; n < a.count; n++)
			data1[count + n] = a.data1[n];

		count += a.count;
	}

	//-----------------------------------------------------------------------
	/*!
	Removes element from array. The empty position is replaced by
	last element, so removal is quite fast but do not retain order.
	\param i
	Index of element to be removed
	\return
	Removed element
	*/
	T RemoveElement(uint i)
	{
		enf_assert(count > 0);
		enf_assert(i < count);
		T tmp = data1[i];
		data1[i] = data1[--count];
		return tmp;
	}

	//-----------------------------------------------------------------------
	/*!
	Removes element from array, but retain all elements ordered. It's
	slower than RemoveElement
	\param i
	Index of element to be removed
	\return
	Removed element
	*/
	T RemoveElementOrdered(uint i)
	{
		enf_assert(count > 0);
		enf_assert(i < count);
		T tmp = data1[i];
		//(data1+i)->~T(); //destroy element
		memmove((void*)&data1[i], (void*)&data1[i + 1], sizeof(T) * (count - i - 1));
		//reinitialize element at the end, which become duplicated now
		PLACEMENT_NEW(&data1[--count]) T;
		return tmp;
	}

	void RemoveMultipleElementOrdered(uint from, uint count)
	{
		enf_assert(this->count > 0);
		enf_assert((from + count) <= this->count);

		for(uint c = from; c < (from + count); c++)
			(data1 + c)->~T(); //destroy

		memmove(&data1[from], &data1[from + count], sizeof(T) * (this->count - (from + count)));
		//reinitialize elements at the end, which become duplicated now
		for(uint c = this->count - count; c < this->count; c++)
			PLACEMENT_NEW(&data1[c]) T;
		this->count -= count;
	}

	//-----------------------------------------------------------------------
	bool Remove(const T &a)
	{
		uint i;
		uint low = 0;
		bool ret = false;
		do
		{
			for(i = low; i < count; i++)
			{
				if(data1[i] == a)
					break;
			}

			if (i < count)
			{
				RemoveElement(i);
				ret = true;
				low = i;
			}
		} while(i < count);
		return ret;
	}

	//-----------------------------------------------------------------------
	void Remove(const Array<T> *a)
	{
		if (a == NULL)
			return;

		for(uint i = 0; i < a->GetCardinality(); i++)
			Remove(a->GetElement(i));
	}

	Array<T>& operator=(const Array<T>& array)
	{
		Clear();
		if(size < array.GetCardinality())
			Grow(array.GetCardinality());
		for(uint i = 0; i < array.GetCardinality(); i++)
			Insert(array.GetElement(i));
		return *this;
	}

private:
	T *data1;
	uint size;
	uint count;

	//-----------------------------------------------------------------------
	ENF_INLINE uint GetArraySize() { return size; }

	//-----------------------------------------------------------------------
	void Grow(uint nsize)
	{
		if(nsize != size)
		{
			data1 = (T*)Alloc.ReAlloc(data1, nsize * sizeof(T));

			enf_assert(data1);

			//call constructors for new objects
			for(uint n = this->size; n < nsize; n++)
			{
				PLACEMENT_NEW(&data1[n]) T;
			}

			this->size = nsize;
		}
	}

	//-----------------------------------------------------------------------
	void FreeAll()
	{
		//destroy all elements
		for(uint i = 0; i < size; i++)
		{
			(data1+i)->~T();
		}
		if (data1) Alloc.Free(data1);
	}

};

//-----------------------------------------------------------------------------
/*!
Constant size array
*/
//-----------------------------------------------------------------------------
template <class T, class TAllocator = DefaultAllocator> class CArray
{
public:

	typedef T ElementType;

	#if defined(ENF_PS3) || defined(__GCC__)
		TAllocator Alloc;
	#else
		typename TAllocator Alloc;
	#endif

	//-----------------------------------------------------------------------
	void SetSize(uint newsize)
	{
		if(this->count != newsize)
			Resize(newsize);

		count = newsize;
	}

	void Flush(uint level)
	{
		if(level == ENF_CLEANUP_FLUSHLEVEL || (count == 0 && data1))
		{
			FreeAll();
			data1 = NULL;
			count = 0;
			return;
		}
	}

	//-----------------------------------------------------------------------
	CArray(): count(0), data1(NULL)
	{
	}


	//-----------------------------------------------------------------------
	CArray(uint nsize): count(0), data1(NULL)
	{
		if (nsize > 0)
		{
			SetSize(nsize);
		}
	}


	//-----------------------------------------------------------------------
	CArray(const CArray<T> &a): count(0), data1(NULL)
	{
		if (a.count > 0)
		{
			Resize(a.count);
			for(uint i = 0; i < a.count; i++)
			{
				data1[i] = a[i];
			}
			count = a.count;
		}
		else
		{
			//destroy all elements
			for(uint i = 0; i < count; i++)
			{
				(data1+i)->~T();
			}
			if (data1)
			{
				MemoryManager::Free(data1);
				data1 = NULL;
			}
			count = 0;
		}
	}

	//-----------------------------------------------------------------------
	~CArray()
	{
		FreeAll();
	}

	void Swap(CArray<T>& array)
	{
		enf_swap(data1, array.data1);
		enf_swap(count, array.count);
	}

	//-----------------------------------------------------------------------
	ENF_INLINE const T& operator[] (uint i) const
	{
		enf_assert(i < count);
		return data1[i];
	}

	//-----------------------------------------------------------------------
	ENF_INLINE T& operator[] (uint i)
	{
		enf_assert(i < count);
		return data1[i];
	}

	//-----------------------------------------------------------------------
	ENF_INLINE const T& GetElement(uint i) const
	{
		enf_assert(i < count);
		return data1[i];
	}

	//-----------------------------------------------------------------------
	ENF_INLINE T& GetElement(uint i)
	{
		enf_assert(i < count);
		return data1[i];
	}

	//-----------------------------------------------------------------------
	ENF_INLINE const T* GetArray() const { return data1; }
	ENF_INLINE T* GetArray() { return data1; }
	ENF_INLINE uint GetCardinality() const { return count; }
	ENF_INLINE uint GetMemorySize() const { return count * sizeof(T); }
	ENF_INLINE bool IsEmpty() const { return (count == 0); }

	//-----------------------------------------------------------------------
	ENF_INLINE void Clear()
	{
		FreeAll();
		count = 0;
		data1 = NULL;
	}

	//-----------------------------------------------------------------------
	ENF_INLINE uint GetIndexOf(const T &a) const
	{
		uint i;
		for(i = 0; i < count; i++)
		{
			if(data1[i] == a)
				return i;
		}
		return -1;
	}

	CArray<T>& operator=(const CArray<T>& array)
	{
		if(count != array.count)
			Resize(array.count);
		
		for(uint i = 0; i < array.GetCardinality(); i++)
			data1[i] = array.GetElement(i);

		return *this;
	}

private:
	T *data1;
	uint count;

	//-----------------------------------------------------------------------
	void Resize(uint nsize)
	{
		//destroy all elements
		for(uint i = 0; i < count; i++)
		{
			(data1+i)->~T();
		}

		data1 = (T*)REALLOC(data1, nsize * sizeof(T));

		enf_assert(data1);

		//call constructors for new objects
		for(uint n = 0; n < nsize; n++)
		{
			PLACEMENT_NEW(&data1[n]) T;
		}

		this->count = nsize;
	}

	//-----------------------------------------------------------------------
	void FreeAll()
	{
		//destroy all elements
		for(uint i = 0; i < count; i++)
		{
			(data1+i)->~T();
		}
		if (data1) MemoryManager::Free(data1);
	}

};

//-----------------------------------------------------------------------------
/*!
Static array with size by template parameter
*/
//-----------------------------------------------------------------------------
template <class T, int S> class SArray
{
protected:
	uint		Num;
	T			m_Array[S];

public:

	typedef T ElementType;

	ENF_INLINE T* GetArray()
	{
		return m_Array;
	}

	ENF_INLINE const T* GetArray() const
	{
		return m_Array;
	}

	ENF_INLINE uint Max() const
	{
		return S;
	}

	ENF_INLINE T& operator[](uint element)
	{
		enf_assert(element < Num);
		return m_Array[element];
	}

	ENF_INLINE const T& operator[](uint element) const
	{
		enf_assert(element < Num);
		return m_Array[element];
	}

	ENF_INLINE T& GetElement(uint element)
	{
		enf_assert(element < Num);
		return m_Array[element];
	}

	ENF_INLINE const T& GetElement(uint element) const
	{
		enf_assert(element < Num);
		return m_Array[element];
	}

	ENF_INLINE T RemoveElement(uint i)
	{
		enf_assert(Num > 0);
		enf_assert(i < Num);
		T tmp = m_Array[i];
		m_Array[i] = m_Array[--Num];
		return tmp;
	}

	ENF_INLINE T RemoveElementOrdered(uint i)
	{
		enf_assert(Num > 0);
		enf_assert(i < Num);
		T tmp = m_Array[i];
		for(uint n = i + 1; n < Num; n++)
			m_Array[n - 1] = m_Array[n];
		Num--;
		return tmp;
	}

	ENF_INLINE T& Append()
	{
		enf_assert(Num < S);
		return m_Array[Num++];
	}
	ENF_INLINE uint Insert(const T& data)
	{
		enf_assert(Num < S);
		m_Array[Num] = data;
		return Num++;
	}

	ENF_INLINE T Pop()
	{
		enf_assert(Num > 0);
		return m_Array[--Num];
	}

	ENF_INLINE void SetSize(uint size)
	{
		enf_assert(size <= S);
		Num = size;
	}

	ENF_INLINE int GetIndexOf(const T &a) const
	{
		for(uint i = 0; i < Num; i++)
		{
			if(m_Array[i] == a)
				return i;
		}
		return -1;
	}

	ENF_INLINE bool Contains(const T &a) const
	{
		return GetIndexOf(a) != -1;
	}

	ENF_INLINE bool Find(const T& a) const
	{
		return Contains(a);
	}

	ENF_INLINE void Clear()
	{
		Num = 0;
	}

	ENF_INLINE uint GetCardinality() const
	{
		return Num;
	}

	ENF_INLINE bool IsEmpty() const
	{
		return (Num == 0);
	}

	ENF_INLINE bool IsFull() const
	{
		return (Num == S);
	}

	ENF_INLINE T& GetLast()
	{
		enf_assert (Num > 0);
		return m_Array[Num - 1];
	}
	
	ENF_INLINE const T& GetLast() const
	{
		enf_assert (Num > 0);
		return m_Array[Num - 1];
	}
	
	SArray()
	{
		Num = 0;
	}
};


#define STRUCT_FROM_CLINK(l, t, m) ((t *)((char *)l - STRUCT_MEMBER_OFFSET(t, m)))

//-----------------------------------------------------------------------------
/*!
CLink, cyclic link
*/
//-----------------------------------------------------------------------------
struct CLink
{
	CLink*	Prev;
	CLink*	Next;

	ENF_INLINE CLink()
	{
		Prev = Next = this;
	}

	ENF_INLINE void Clear()
	{
		Prev = Next = this;
	}

	ENF_INLINE bool IsLinked()
	{
		return (Next != this && Prev != this);
	}

	ENF_INLINE void Insert(CLink *l)
	{
		Next = l;
		Prev = l->Prev;
		Prev->Next = this;
		Next->Prev = this;
	}
	ENF_INLINE void InsertBefore(CLink *l)
	{
		Next = l->Next;
		Prev = l;
		Prev->Next = this;
		Next->Prev = this;
	}

	ENF_INLINE void Remove()
	{
		Next->Prev = Prev;
		Prev->Next = Next;
	}
};

#define ENF_LIST_NEXT m_pNextNode
#define ENF_LIST_PREV m_pPrevNode

//-----------------------------------------------------------------------
/*!
Doubly linked list with no additional memory allocation
as the nodes themselves holds the list representation.
*/
//-----------------------------------------------------------------------
template <class ListNode> class List
{
public:
	List()
	{
		Clear();
	}

	inline int GetIndexOf(const ListNode *node) const
	{
		uint i = 0;
		ListNode *tmp = FirstNode;
		while(tmp != NULL)
		{
			if (tmp == node) return i;
			tmp = tmp->ENF_LIST_NEXT;
			i++;
		}
		return -1;
	}

	inline ListNode *GetElement(uint i) const
	{
		enf_assert(i < Count);
		ListNode *tmp = FirstNode;
		while(i-- > 0)
		{
			tmp = tmp->ENF_LIST_NEXT;
		}
		return tmp;
	}

	inline ListNode *operator[](uint i) const
	{
		return GetElement(i);
	}

	//-----------------------------------------------------------------------
	/*!
	\return
	The number of of entries in the list.
	*/
	//-----------------------------------------------------------------------
	inline uint GetCardinality() const
	{	//number of elements
		return Count;
	}

	//-----------------------------------------------------------------------
	/*!
	Returns true if the list is empty. Operation is O(1).
	\return
	True if the array has no entries, false otherwise.
	*/
	//-----------------------------------------------------------------------
	inline bool IsEmpty() const
	{
		return (Count==0);
	}

	//-----------------------------------------------------------------------
	/*!
	Clear the array. Operation is O(1).
	*/
	//-----------------------------------------------------------------------
	inline void Clear()
	{
		Count = 0;
		FirstNode = NULL;
		LastNode = NULL;
	}

	//-----------------------------------------------------------------------
	/*!
	\return
	True if the list contains element a
	*/
	//-----------------------------------------------------------------------
	inline bool Contains(const ListNode *a) const
	{
		ListNode *tmp = FirstNode;
		while(tmp != NULL){
			if (tmp == a) return true;
			tmp = tmp->ENF_LIST_NEXT;
		}
		return false;
	}

	//-----------------------------------------------------------------------
	/*!
	\return
	Return the list element passed by operator== or 0
	*/
	//-----------------------------------------------------------------------
	inline ListNode* Find( const ListNode* a )
	{
		ListNode* tmp = FirstNode;
		while( tmp != NULL )
		{
			if( *tmp == *a ) return tmp;
			tmp = tmp->ENF_LIST_NEXT;
		}

		return 0;
	}


	//-----------------------------------------------------------------------
	/*!
	Adds one element to the end of the list. Operation is O(1).
	\param a
	The element that will be added to this list.
	*/
	//-----------------------------------------------------------------------
	inline void Insert(ListNode *a)
	{
		enf_assert(!Contains(a));
		if (LastNode) LastNode->ENF_LIST_NEXT = a;
		if (!FirstNode) FirstNode = a;
		a->ENF_LIST_NEXT = NULL;
		a->ENF_LIST_PREV = LastNode;
		LastNode = a;
		Count++;
	}

	inline void InsertAsFirst(ListNode *a)
	{
		enf_assert(!Contains(a));
		a->ENF_LIST_NEXT = FirstNode;
		a->ENF_LIST_PREV = NULL;
		if (FirstNode)
		{
			FirstNode->ENF_LIST_PREV = a;
		}

		if (!LastNode) LastNode = a;
		FirstNode = a;
		Count++;
	}

	//-----------------------------------------------------------------------
	/*!
	Adds element behind given element. Operation is O(1).
	\param a
	The element that will be added to this list.
	\param prev
	Reference element.
	*/
	//-----------------------------------------------------------------------
	inline void InsertAfter( ListNode* a, ListNode* prev )
	{
		enf_assert(!Contains(a));
		a->ENF_LIST_PREV = prev;
		a->ENF_LIST_NEXT = prev->ENF_LIST_NEXT;
		a->ENF_LIST_PREV->ENF_LIST_NEXT = a;
		if( a->ENF_LIST_NEXT )
			a->ENF_LIST_NEXT->ENF_LIST_PREV = a;
		else
			LastNode = a;
		++Count;
	}

	//-----------------------------------------------------------------------
	/*!
	Adds element before given element. Operation is O(1).
	\param a
	The element that will be added to this list.
	\param next
	Reference element.
	*/
	//-----------------------------------------------------------------------
	inline void InsertBefore( ListNode* a, ListNode* next )
	{
		enf_assert(!Contains(a));
		a->ENF_LIST_NEXT = next;
		a->ENF_LIST_PREV = next->ENF_LIST_PREV;
		a->ENF_LIST_NEXT->ENF_LIST_PREV = a;
		if( a->ENF_LIST_PREV )
			a->ENF_LIST_PREV->ENF_LIST_NEXT = a;
		else
			FirstNode = a;
		++Count;
	}


	//-----------------------------------------------------------------------
	/*!
	Remove an element from the list
	Operation is O(1)
	\param a
	The element that will be removed from this array.
	*/
	//-----------------------------------------------------------------------
	inline void Remove(ListNode *a)
	{
		enf_assert(Contains(a));
		enf_assert(Count > 0);
		if (a->ENF_LIST_NEXT)
			a->ENF_LIST_NEXT->ENF_LIST_PREV = a->ENF_LIST_PREV;
		else
			LastNode = a->ENF_LIST_PREV;
		if (a->ENF_LIST_PREV)
			a->ENF_LIST_PREV->ENF_LIST_NEXT = a->ENF_LIST_NEXT;
		else
			FirstNode = a->ENF_LIST_NEXT;
		a->ENF_LIST_NEXT = NULL;
		a->ENF_LIST_PREV = NULL;
		Count--;
	}

	inline ListNode *GetFirstNode() const
	{
		return FirstNode;
	}

	inline ListNode *GetLastNode() const
	{
		return LastNode;
	}

private:
	uint			Count;
	ListNode*	FirstNode;
	ListNode*	LastNode;
};


//-----------------------------------------------------------------------------
/*!
ListIterator USE FOR BACKWARD COMPATIBILITY ONLY!!!
*/
//-----------------------------------------------------------------------------
template <class ListNode> class ListIterator
{
public:
	ListIterator(const List<ListNode> *list)
	{
		Current = list->GetFirstNode();
	}

	ListIterator(const List<ListNode> &list)
	{
		Current = list.GetFirstNode();
	}

	bool HasMoreElements() const
	{
		return Current != NULL;
	}

	ListNode *GetNext()
	{
		ListNode *tmp = Current;
		if (tmp) Current = Current->ENF_LIST_NEXT;
		else Current = NULL;
		return tmp;
	}

private:
	ListNode *Current;
};

//!Self initializing pointer container
template<class T> class Ptr
{
public:
	Ptr(): ptr(NULL) {}
	Ptr(T* p): ptr(p) {}
	bool IsNull() const
	{
		return ptr == NULL;
	}
	void Delete()
	{
		if(ptr != NULL)
		{
			delete ptr;
			ptr = NULL;
		}
	}
	ENF_INLINE void operator=(T* src)
	{
		ptr = src;
	}

	ENF_INLINE operator T*() const
	{
		return ptr;
	}

	ENF_INLINE T* operator->() const
	{
		return ptr;
	}

	ENF_INLINE void operator=(const Ptr& src)
	{
		ptr = src.ptr;
	}
protected:
	T*	ptr;
};

template<class T> class AutoPtr: public Ptr<T>
{
public:
	AutoPtr(T* p): Ptr<T>(p) {}
	~AutoPtr()
	{
		SafeDeletePtr<T>(Ptr<T>::ptr);
	}
};


//@}

//TEMP: tohle si Fido presun kam uznas za vhodne
template< class TFunction, template< class, class > class TContainer, typename T >
void ForEach( TContainer< T, DefaultAllocator >& container )
{
	for( int i = (int)container.GetCardinality() - 1; i >= 0; --i )
	{
		TFunction::Do( container.GetElement( i ) );
	}
}

struct DeleteItem
{
	template< typename T >
	static void Do( T& item );

	template< typename T >
	static void Do( T*& item )
	{
		// if you can get the macro working here, you won't need the stuff above
		//CTCHECK( (ai::TypeTraits< T >::IsPointer), Cannot_Delete_Non_Pointer_Variables );
		delete item;
	}
};

struct SafeDeleteItem
{
	template< typename T >
	static void Do( T& item );

	template< typename T >
	static void Do( T*& item )
	{
		// if you can get the macro working here, you won't need the stuff above
		//CTCHECK( (ai::TypeTraits< T >::IsPointer), Cannot_Delete_Non_Pointer_Variables );
		if( item )
		{
			delete item;
			item = NULL;
		}
	}
};

struct ReleaseItem
{
	template< typename T >
	static void Do( T& item )
	{
		item.Release();
	}

	template< typename T >
	static void Do( T*& item )
	{
		item->Release();
	}
};

struct SafeReleaseItem
{
	template< typename T >
	static void Do( T& item )
	{
		item.Release();
	}

	template< typename T >
	static void Do( T*& item )
	{
		if(item)
		{
			item->Release();
			item = NULL;
		}
	}
};

}

#endif //ENF_TYPES_H

