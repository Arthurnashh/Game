#ifdef _MSC_VER
#pragma once
#endif

#ifndef _COMPACT_BUF_HPP
#define _COMPACT_BUF_HPP

#include <Es/Containers/typeOpts.hpp>
#ifdef _MT_ELES
#include <El/MultiThread/Interlocked.h>
#endif

#define CB_LOG_NEW_DELETE 0

#if _DEBUG && !DISABLE_CHECK_RSTRING_MT
# define CHECK_RSTRING_MT 0
#else
# define CHECK_RSTRING_MT 0
#endif


#pragma warning(disable:4200)

/**
MT safety:
AddRef/Release for one object must always be called from the same thread, unless _MT_ELES is used
*/

template <class Type>
class CompactBuffer: public RefCountBaseT<true>, private NoCopy
{
	// include RefCount functionality to avoid virtual functions table and delete

  private:
  
  #if CHECK_RSTRING_MT
    mutable InitVal<int,0> _threadId;
  #endif

  public:
  int Release() const
  {
    int ret=DecRef();
    if (ret==0) Delete();
    return ret;
  }

	// buffer implementation
	private:
	size_t _size;
	Type _data[1];

	public:
	explicit CompactBuffer( size_t count )
	{
		_size=count;
		ConstructTraits<Type>::ConstructArray(_data,_size);
	}
	CompactBuffer( Type const *data, size_t count )
	{
		_size=count;
		ConstructTraits<Type>::CopyConstruct(_data,data,_size);
	}
	~CompactBuffer()
	{
		ConstructTraits<Type>::DestructArray(_data,_size);
	}

	private: // disable copy

	#include <Es/Memory/normalNew.hpp>

  // note: new and delete must never be used on CompactBuffer
	void *operator new( size_t size ) {Fail("new: Should be never used.");}
	void operator delete( void *mem ) {Fail("delete: Should be never used.");}

	inline void *operator new(size_t size, void *placement) {return placement;}
	inline void operator delete(void *mem, void *placement) {}

	#include <Es/Memory/debugNew.hpp>

	public:

	Type *Data() {return _data;}
	const Type *Data() const {return _data;}
	int Size() const {return _size;}

	static CompactBuffer *New( int n )
	{
		Assert( n>0 );
		int size = sizeof(CompactBuffer)-sizeof(Type)+sizeof(Type)*n;

		//#if ALLOC_DEBUGGER && defined _CPPRTTI
		//#else
		CompactBuffer *buffer=(CompactBuffer *)new char[size];
		//#endif

		#include <Es/Memory/normalNew.hpp>

		// placement new required
		new(buffer) CompactBuffer(n);

		#include <Es/Memory/debugNew.hpp>

		#if CB_LOG_NEW_DELETE
		LogF("New %x: Size %d",buffer,size);
		#endif
		return buffer;
	}

	void Delete() const
	{
		#if CB_LOG_NEW_DELETE
		LogF
		(
			"Delete %x: Size %d",
			this,_size*sizeof(Type)+sizeof(CompactBuffer)-sizeof(Type)
		);
		#endif
		this->~CompactBuffer();
		delete[] (char *)this;
	}

	static CompactBuffer *Copy( CompactBuffer *str )
	{
		int len=str->Size();
		Assert(len>0);
		CompactBuffer *buffer=(CompactBuffer *)new char
		[
			sizeof(CompactBuffer)-sizeof(Type)+sizeof(Type)*len
		];
		new(buffer) CompactBuffer(str->Data(),len);
		return buffer;
	}
	static CompactBuffer *Copy( CompactBuffer *str, size_t len )
	{
		if( len>str->Size() ) len=str->Size();
		Assert(len>0);
		size_t size = sizeof(CompactBuffer)-sizeof(Type)+sizeof(Type)*len;
		CompactBuffer *buffer=(CompactBuffer *)new char[size];
		#include <Es/Memory/normalNew.hpp>
		new(buffer) CompactBuffer(str->Data(),len);
		#include <Es/Memory/debugNew.hpp>
		#if CB_LOG_NEW_DELETE
		LogF("Copy %x: Size %d",buffer,size);
		#endif
		return buffer;
	}
	static CompactBuffer *Copy( Type const *data, size_t len )
	{
		Assert(len>0);
		size_t size = sizeof(CompactBuffer)-sizeof(Type)+sizeof(Type)*len;
		CompactBuffer *buffer=(CompactBuffer *)new char[size];
		#include <Es/Memory/normalNew.hpp>
		new(buffer) CompactBuffer(data,len);
		#include <Es/Memory/debugNew.hpp>
		#if CB_LOG_NEW_DELETE
		LogF("Copy %x: Size %d",buffer,size);
		#endif
		return buffer;
	}
};

//#pragma warning(default:4200)

#include <Es/Memory/debugNew.hpp>

typedef CompactBuffer<char> CompactString;


#endif

