#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SMALL_ARRAY_HPP
#define _SMALL_ARRAY_HPP

#include <Es/Framework/debugLog.hpp>
#include <Es/Containers/array.hpp>

#define _small ( (Type *)_smallSpace )

template <class Type,int size=64>
class VerySmallArray
{
  typedef ConstructTraits<Type> CTraits;

  enum {MaxSmall=(size-4)/sizeof(Type)};
  int _nSmall;
  unsigned char _smallSpace[MaxSmall*sizeof(Type)]; // note: Type may be misaligned

  public:
  VerySmallArray();
  ~VerySmallArray(){Clear();}

  __forceinline const Type *Data() const {return _small;}
  const Type &Get( int i ) const
  {
    AssertDebug((i>=0) && (i<_nSmall));
    return _small[i];
  }
  Type &Set( int i )
  {
    AssertDebug((i>=0) && (i<_nSmall));
    return _small[i];
  }

  const Type &operator [] ( int i ) const
  {
    AssertDebug((i>=0) && (i<_nSmall));
    return _small[i];
  }
  Type &operator [] ( int i )
  {
    AssertDebug((i>=0) && (i<_nSmall));
    return _small[i];
  }

  void Resize(int n);
  void Access(int n);

  int Add();
  int Add( const Type &object );
  int Insert( int index );

  void Delete( int index );
  void Clear();
  __forceinline int Size() const {return _nSmall;}
  __forceinline int MaxSize() const {return MaxSmall;}
  double GetMemoryUsed() const;

  ClassIsMovableZeroed(VerySmallArray);
};

template <class Type,int size>
VerySmallArray<Type,size>::VerySmallArray()
:_nSmall(0)
{
}

template <class Type,int size>
void VerySmallArray<Type,size>::Resize(int n)
{
  AssertDebug((n>=0) && (n<=MaxSmall));
  // change length of controlled sequence
  if( n>_nSmall ) CTraits::ConstructArray(_small+_nSmall,n-_nSmall);
  else if( _nSmall>n ) CTraits::DestructArray(_small+n,_nSmall-n);
  _nSmall=n;
}

template <class Type,int size>
void VerySmallArray<Type,size>::Access(int n)
{
  if (_nSmall < n + 1) Resize(n + 1);
}

template <class Type,int size>
int VerySmallArray<Type,size>::Add( const Type &object )
{
  if( _nSmall<MaxSmall )
  {
    CTraits::CopyConstruct(_small[_nSmall],object);
    return _nSmall++;
  }
  return -1;
}

template<class Type,int size>
double VerySmallArray<Type,size>::GetMemoryUsed() const
{
  double res = 0;
  // we cannot assume items have GetMemoryUsed member
  for (int i=0; i<Size(); i++)
  {
    res += GetMemoryUsedTraits<Type>::GetMemoryUsed(Get(i));
  }
  return res;
}

template <class Type,int size>
int VerySmallArray<Type,size>::Add()
{
  if( _nSmall<MaxSmall )
  {
    CTraits::Construct(_small[_nSmall]);
    return _nSmall++;
  }
  return -1;
}

template<class Type,int size>
int VerySmallArray<Type,size>::Insert( int index )
{
  if( _nSmall<MaxSmall )
  {
    _nSmall++;
    CTraits::InsertData(_small+index,_nSmall-index);
    CTraits::Construct(_small[index]);
    return index;
  }
  return -1;
}

template <class Type,int size>
void VerySmallArray<Type,size>::Delete( int index )
{
  CTraits::Destruct(_small[index]);
  CTraits::DeleteData(_small+index,_nSmall-index,1);
  _nSmall--;
}

template <class Type,int size>
void VerySmallArray<Type,size>::Clear()
{
  CTraits::DestructArray(_small,_nSmall);
  _nSmall=0;
}

#undef _small

template <class Type,int size=64>
class SmallArray
{
  // sizeof( AutoArray ) is 4 DWORDS
  // try to allign to 16 DWORDS (64 B)
  // we have 44 bytes to contain Type elements
  VerySmallArray<Type,size-16> _small;
  AutoArray<Type> _large;
  
  public:
  SmallArray();
  ~SmallArray(){Clear();}

  int Size() const {return _small.Size()+_large.Size();}
  const Type &Get( int i ) const 
  {
    if( i<_small.Size() ) return _small.Get(i);
    else return _large.Get(i-_small.Size());
  }
  Type &Set( int i )
  {
    if( i<_small.Size() ) return _small.Set(i);
    else return _large.Set(i-_small.Size());
  }

  const Type &operator [] ( int i ) const {return Get(i);}
  Type &operator [] ( int i ) {return Set(i);}

  void Realloc(int n);
  int Add( const Type &object );
  void Delete( int index );
  void Compact();
  void Clear();
  double GetMemoryUsed() const
  {
    return _small.GetMemoryUsed()+_large.GetMemoryUsed();
  }
  /// memory allocated in the dynamic container
  size_t GetMemoryAllocated() const
  {
    return _large.MaxSize()*sizeof(Type);
  }

  ClassIsMovableZeroed(SmallArray);
};

template <class Type,int size>
SmallArray<Type,size>::SmallArray()
{
}

template <class Type,int size>
int SmallArray<Type,size>::Add( const Type &object )
{
  int index=_small.Add(object);
  if( index>=0 ) return index;
  return _large.Add(object)+_small.Size();
}

template <class Type,int size>
void SmallArray<Type,size>::Delete( int index )
{
  if( index<_small.Size() )
  {
    _small.Delete(index);
  }
  else
  {
    _large.Delete(index-_small.Size(),1);
  }
}

template <class Type,int size>
void SmallArray<Type,size>::Realloc(int n)
{
  // TODO: clever compacting of large/small list
  if (n<_small.MaxSize())
  {
    _large.Clear();
  }
  else
  {
    _large.Realloc(n-_small.MaxSize());
  }
}

template <class Type,int size>
void SmallArray<Type,size>::Compact()
{
  // TODO: clever compacting of large/small list
  _large.Compact();
}

template <class Type,int size>
void SmallArray<Type,size>::Clear()
{
  _small.Clear();
  _large.Clear();
}

#undef _small

#endif
