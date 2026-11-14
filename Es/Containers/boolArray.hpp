#ifdef _MSC_VER
#pragma once
#endif

#ifndef _BOOL_ARRAY_HPP
#define _BOOL_ARRAY_HPP

#include <Es/Types/enum_decl.hpp>
/// enum stored using any type instead of default unsigned int
template <class Enum, class Type=unsigned char>
class SizedEnum
{
  Type _data;

  public:
#ifdef __GNUC__
  //operator Enum () const {return Enum(_data);}
  operator Enum () const {return (Enum)(int)(_data);}
  //operator Type () const {return _data;}
#else
  operator Enum () const {return (Enum)_data;}
#endif
  SizedEnum( Enum val ):_data(val){}
#if NO_UNDEF_ENUM_REFERENCE && NO_ENUM_FORWARD_DECLARATION 
  SizedEnum( typename Enum::Helper::EnumType val ):_data(val){}
  bool operator==(const SizedEnum en2) const {return en2.GetEnumValue()==_data;}
  bool operator!=(const SizedEnum en2) const {return en2.GetEnumValue()!=_data;}
#endif
  SizedEnum(){}
  Enum GetEnumValue() const {return (Enum)(int)(_data);}
} PACKED;

/// bitfield (array of bool) able to hold as much values as will fit into "int" type
class PackedBoolArray
{
private:
  int _data;

public:
  enum {NBools=sizeof(int)*8};
  PackedBoolArray() {Init();}
  void Init() {_data = 0;}
  /// return true when given bit is set
  bool Get(int pos) const
  {
    Assert(pos >= 0 && pos < NBools);
    // check not neccessary, operator << will do the same
    //if (pos < 0 || pos >= sizeof(int)) return false;
    return (_data & (1 << pos)) != 0;
  }
  /// return non-zero when given bit is set
  int Test(int pos) const
  {
    Assert(pos >= 0 && pos < NBools);
    // check not neccessary, operator << will do the same
    //if (pos < 0 || pos >= sizeof(int)) return false;
    return _data & (1 << pos);
  }
  void Set(int pos, bool value)
  {
    Assert(pos >= 0 && pos < NBools);
    // check not neccessary, operator << will do the same
    //if (pos < 0 || pos >= sizeof(int)) return;
    if (value)
      _data |= (1 << pos);
    else
      _data &= (~(1 << pos));
  }
  bool operator [] ( int i ) const {return Get(i);}

  void Toggle(int pos)
  {
    Assert(pos >= 0 && pos < NBools);
    // check not neccessary, operator << will do the same
    //if (pos < 0 || pos >= sizeof(int)) return;
    _data ^= (1 << pos);
  }
  bool IsEmpty() const {return _data==0;}
  int GetCount() const
  {
    int c=0;
    for( int i=0; i<NBools; i++ ) if( Get(i) ) c++;
    return c;
  }

  bool operator ==(const PackedBoolArray dst) const {return _data == dst._data;}
  bool IsPartOf(const PackedBoolArray dst) const {return (_data & dst._data) == _data;}
  bool Contain(const PackedBoolArray dst) const {return (_data & dst._data) == dst._data;}
  void operator &=(const PackedBoolArray op) {_data &= op._data;}
};

TypeIsSimpleZeroed(PackedBoolArray);

/// bitfield of arbitrary size supported by the container

template < class Container=AutoArray<PackedBoolArray,MemAllocD> >
class PackedBoolAutoArrayTT
{
  enum {PerItem=PackedBoolArray::NBools};
  Container _data;

public:
  bool Get( int pos ) const
  {
    int index=pos/PerItem;
    if( index>=_data.Size() ) return false;
    return _data.Get(index).Get(pos-index*PerItem);
  }
  int Test( int pos ) const
  {
    int index=pos/PerItem;
    if( index>=_data.Size() ) return 0;
    return _data.Get(index).Test(pos-index*PerItem);
  }
  void Set( int pos, bool value )
  {
    int index=pos/PerItem;
    _data.Access(index);
    _data.Set(index).Set(pos-index*PerItem,value);
  }
  bool operator [] ( int i ) const {return Get(i);}
  bool operator == (const PackedBoolAutoArrayTT &with ) const
  {
    if (_data.Size() != with._data.Size()) return false;
    for (int i = 0; i < _data.Size(); i++)
    {
      if (!(_data[i] == with._data[i])) return false;
    }
    return true;
  }
  void Clear() {_data.Clear();}
  //! Returns the maximum number of items (it can be slightly bigger than maximum inserted)
  int Size() const {return _data.Size() * PerItem;}
  bool IsEmpty() const
  {
    for( int i=0; i<_data.Size(); i++ )
    {
      if( !_data[i].IsEmpty() ) return false;
    }
    return true;
  }
  int GetCount() const
  {
    int c=0;
    for( int i=0; i<_data.Size(); i++ )
    {
      c += _data[i].GetCount();
    }
    return c;
  }

  void SetStorage(const typename Container::AllocatorType &alloc) {_data.SetStorage(alloc);}
  void Init(int size)
  {
    int items = (size + PerItem - 1) / PerItem;
    _data.Realloc(items);
    _data.Resize(items);
  }
  void InitRaw(int size)
  {
    _data.Realloc(size);
    _data.Resize(size);
  }
  int RawSize() const {return _data.Size();}
  int *RawData() const {return (int *)_data.Data();}
  
  void operator &= (const PackedBoolAutoArrayTT &op)
  {
    if (op._data.Size()<_data.Size()) _data.Resize(op._data.Size());
    for (int i=0; i<_data.Size(); i++) _data[i] &= op._data[i];
    
  }

  void Resize(int size)
  {
    _data.Resize((size + PerItem - 1) / PerItem); // no need to initialize, new PackedBoolArray items are zero initialized automatically
  }

  ClassIsMovableZeroed(PackedBoolAutoArrayTT);
};


/// bitfield of arbitrary size

template <class Allocator=MemAllocD>
class PackedBoolAutoArrayT: public PackedBoolAutoArrayTT< AutoArray<PackedBoolArray,Allocator> >
{
};

/// bitfield of arbitrary size with a default dynamic allocator

typedef PackedBoolAutoArrayTT<> PackedBoolAutoArray;

/// container which holds a constant number of items, with an interface similiar to AutoArray
template <class Type,int count>
class FixedArray
{
  Type _data[count];
  
  public:
  //@{ size related functions
  void Realloc(int n){}
  void Resize(int n){Assert(n<=count);}
  int Size() const {return count;}
  int MaxSize() const {return count;}
  void Access(int i){Assert(i<count);}
  void Clear(){}
  const Type *Data() const {return _data;}
  //@}
  
  //@{ item access
  Type &Set(int i) {return _data[i];}
  const Type &Get(int i) const {return _data[i];}
  Type &operator [](int i) {return _data[i];}
  const Type &operator [](int i) const {return _data[i];}
  //@}
  
  class AllocatorType {};
  typedef Type DataType;
};

#ifdef _MSC_VER //MSC is unable to recognize template specializations in CPP

// specialization - optimization to avoid 1x loop
template <>
bool PackedBoolAutoArrayTT< FixedArray<PackedBoolArray,1> >::IsEmpty() const
{
  return _data[0].IsEmpty();
}

template <>
bool PackedBoolAutoArrayTT< FixedArray<PackedBoolArray,1> >::Get( int pos ) const
{
  if( pos>=PackedBoolArray::NBools) return false;
  return _data.Get(0).Get(pos);
}

template <>
int PackedBoolAutoArrayTT< FixedArray<PackedBoolArray,1> >::Test( int pos ) const
{
  if( pos>=PackedBoolArray::NBools) return false;
  return _data.Get(0).Test(pos);
}

#else
// specialization - optimization to avoid 1x loop
template <>
bool PackedBoolAutoArrayTT< FixedArray<PackedBoolArray,1> >::IsEmpty() const;

template <>
bool PackedBoolAutoArrayTT< FixedArray<PackedBoolArray,1> >::Get( int pos ) const;
  
template <>
int PackedBoolAutoArrayTT< FixedArray<PackedBoolArray,1> >::Test( int pos ) const;

#endif

#endif
