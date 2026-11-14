#ifdef _MSC_VER
#pragma once
#endif

#ifndef _STREAM_ARRAY_HPP
#define _STREAM_ARRAY_HPP

// Type must be able to:
// know size of instance: int ItemSize() const
// duplicate any instance: void Duplicate( Type &dst ) const

#include <Es/Containers/array.hpp>
#include <Es/Containers/staticArray.hpp>

#ifdef _DEBUG
	class Offset
	{
		private:
		int _offset;

		public:
		Offset(){_offset=0;}
		explicit Offset( int offset ):_offset(offset){}
		void Advance( int a ) {_offset+=a;}
		int GetOffset() const {return _offset;}

		bool operator < (Offset c) const {return _offset<c._offset;}
		bool operator <= (Offset c) const {return _offset<=c._offset;}
		bool operator == (Offset c) const {return _offset==c._offset;}
		bool operator >= (Offset c) const {return _offset>=c._offset;}
		bool operator > (Offset c) const {return _offset>c._offset;}
		bool operator != (Offset c) const {return _offset!=c._offset;}
	};
	#define O(i) ( (i).GetOffset() )
	#define A(i,n) ( (i).Advance(n) )
	#define OffsetAdvance(i,n) ( (i).Advance(n) )

	inline int OffsetDiff(Offset a, Offset b)
	{
		return a.GetOffset()-b.GetOffset();
	}

	TypeIsSimpleZeroed(Offset);
#else
	typedef int Offset; // better optimized, no safety
	#define O(i) (i)
	#define A(i,n) ( (i)+=(n) )
	#define OffsetAdvance(i,n) ( (i)+=(n) )
	#define OffsetDiff(a,b) ((a)-(b))
#endif

struct StreamSortSegment
{
	Offset beg,end;

	void Clear(){beg=end=Offset(0);}
};

TypeIsSimple(StreamSortSegment)

// transform normal ordering ordering function into a segment based one

template <class Type, class Container, class Function>
class CompareByOffset
{
  const Function &_f;
	public:
	CompareByOffset(const Function &f)
	:_f(f)
	{
	  
	}
	int operator () (
		const StreamSortSegment *s1, const StreamSortSegment *s2, Container *array
	) const
	{
		return _f(&array->Get(s1->beg),&array->Get(s2->beg));
	}
};

template <class Type,class Container>
class StreamArray
{
	// we assume Type is simple - no constructor, destructor, can be moved
	// see TypeIsSimple
	private:
	Container _data;
	int _count;
	
	void DoConstruct( const StreamArray &src );

	public:
	StreamArray():_count(0){}
	~StreamArray(){Clear();}

	void operator = ( const StreamArray &src ) {Clear();DoConstruct(src);}
	StreamArray( const StreamArray &src ) {DoConstruct(src);}
	void Move( StreamArray &src );

	void Copy( const Type *src, int n );
	void Merge( const StreamArray &src );
	void Merge( const char *data, int size, int n );

	// read access functions
	Offset Begin() const {return Offset(0);}
	Offset End() const {return Offset(_data.Size());}
	void Next( Offset &index ) const {A(index,Get(index).ItemSize());}
	const Type &Get( Offset pos ) const
	{
		return reinterpret_cast<const Type &>(_data[O(pos)]);
	}
	Type &Set( Offset pos )
	{
		return reinterpret_cast<Type &>(_data[O(pos)]);
	}

	const Type &operator [] ( Offset pos ) const {return Get(pos);}
	Type &operator [] ( Offset pos ) {return Set(pos);}

	Offset Find( int index ) const;
	int Size() const {return _count;}

	int CalculateCount() const;

	Container &GetData() {return _data;} // for access to underlying data
	const Container &GetData() const {return _data;} // for access to underlying data

	const char *RawData() const {return _data.Data();}
	int RawSize() const {return _data.Size();}
	void RawRealloc( int n ) {_data.Realloc(n);}
	
	// write access functions
	Offset Add( const Type &src ) // add single
	{
		int pos=_data.Size();
		_data.Resize(pos+src.ItemSize());
		Type &dst=reinterpret_cast<Type &>(_data[pos]);
		src.Duplicate(dst);
		_count++;
		return Offset(pos);
	}
	void Clear(){_data.Clear(),_count=0;}
	void Resize0()
	{
		_data.Resize(0);
		_count = 0;
	}
	/// slow resize - recounting needed
	void Resize(Offset size)
	{
	  int count = 0;
	  Assert(size<=End());
	  for (Offset o=Begin(); o<size; Next(o))
	  {
	    count++;
	  }
	  Resize(size,count);
	}
	/// fast resize - count known
	void Resize(Offset size, int count)
	{
	  _data.Resize(O(size));
	  _count = count;
	}
	void Delete( Offset pos )
	{
		int size=Get(pos).ItemSize();
		// generic version
		//while( --size>=0 ) _data.Delete(pos);
		// optimized version
		// note: Container should be char storage
		Assert( sizeof(Container::DataType)==1 );
#ifdef _WIN32
    memcpy(_data.Data()+O(pos),_data.Data()+(O(pos)+size),_data.Size()-(O(pos)+size));
#else
    // overlapping memory segments can cause problems (libgcc is linked as shared library on Linux)
    memmove(_data.Data()+O(pos),_data.Data()+(O(pos)+size),_data.Size()-(O(pos)+size));
#endif
		_data.Resize(_data.Size()-size);
		_count--;
	}

	void Compact() {_data.Realloc(_data.Size());}
	void ReserveRaw( int size ) {_data.Reserve(size,size);}
	void Reserve( int n )
	{
		int size=Type::TypicalItemSize()*n;
		_data.Reserve(size,size);
	}
	void ReserveMore( int n )
	{
		int size=O(End())+Type::TypicalItemSize()*n;
		_data.Reserve(size);
	}
	// use Realloc with caution
	//void Realloc( int n ) {_data.Realloc(Type::TypicalItemSize()*n);}
	void Realloc( int n )
	{
		int size=Type::TypicalItemSize()*n;
		int minSize=_data.Size();
		if( size<minSize ) size=minSize;
		_data.Realloc(size);
	}

  size_t GetMemoryAllocated() const {return _data.GetMemoryAllocated();}
	void SetStorage( const typename Container::AllocatorType &alloc )
	{
		_data.SetStorage(alloc);
	}	
};

template <class Type,class Container>
void StreamArray<Type,Container>::DoConstruct( const StreamArray &src )
{
	_data=src._data;
	_count=src._count;
}

template <class Type,class Container>
void StreamArray<Type,Container>::Copy( const Type *src, int n )
{
	Reserve(n);
	_data.Resize(0);
	for( int i=0; i<n; i++ ) Add(src[i]);
}

template <class Type,class Container>
void StreamArray<Type,Container>::Move( StreamArray &src )
{
	// src will be clear
	_count=src._count;
	src._count=0;
	// transfer actual data
	_data.Move(src._data);
}

template <class Type,class Container>
void StreamArray<Type,Container>::Merge( const StreamArray &src )
{
	Assert( sizeof(Container::DataType)==1 );
	int offset=_data.Size();
	_data.Resize(offset+src._data.Size());
	memcpy(_data.Data()+offset,src._data.Data(),src._data.Size());
	_count+=src._count;
}

template <class Type,class Container>
void StreamArray<Type,Container>::Merge( const char *data, int size, int n )
{
	Assert( sizeof(Container::DataType)==1 );
	int offset=_data.Size();
	_data.Resize(offset+size);
	memcpy(_data.Data()+offset,data,size);
	_count+=n;
}

template <class Type,class Container>
Offset StreamArray<Type,Container>::Find( int index ) const
{
	int i=0;
	for( Offset f=Begin(),e=End(); f<e; Next(f),i++ )
	{
		if( i==index ) return f;
	}
	Fail("No entry");
	return End();
}

template <class Type,class Container>
int StreamArray<Type,Container>::CalculateCount() const
{
	int i=0;
	for( Offset f=Begin(),e=End(); f<e; Next(f),i++ )	{}
	return i;
}


#undef O
#undef A

#endif

