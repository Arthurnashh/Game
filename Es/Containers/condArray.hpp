#ifdef _MSC_VER
#pragma once
#endif

#ifndef __ES_COND_ARRAY_HPP
#define __ES_COND_ARRAY_HPP

#include <Es/Containers/array.hpp>


/// array which is not stored when all entries have the same value
/** Most functions of this array are simply redirected to the underlying container
*/
template <class Type,class Allocator=MemAllocD>
class CondensedAutoArray
{
  AutoArray<Type,Allocator> _data;
  Type _virtualValue;
  int _virtualSize; /// size used when compressed
  
  public:
  
  /// provide means to query type for datatype
  typedef Type DataType;
  
  CondensedAutoArray()
  {
    _virtualSize = 0;
  }
  CondensedAutoArray(const CondensedAutoArray &src)
  {
    _data = src._data;
    _virtualSize = src._virtualSize;
    if (_virtualSize > 0) _virtualValue = src._virtualValue;
  }
  void operator = (const CondensedAutoArray &src)
  {
    _data = src._data;
    _virtualSize = src._virtualSize;
    if (_virtualSize > 0) _virtualValue = src._virtualValue;
  }

  /// read access to any element
  const Type &Get(int i) const
  {
    Assert(_virtualSize==0 || _data.Size()==0);
    if (i<_data.Size()) return _data[i];
    Assert(i<_virtualSize);
    return _virtualValue;
  }
  bool IsCondensed() const
  {
    return _virtualSize>0;
  }
  const Type &GetCondensedValue() const
  {
    Assert(_virtualSize>0);
    return _virtualValue;
  }
  template <class Op>
  void ForEach(const Op &op)
  {
    if (_virtualSize>0)
    {
      op(_virtualValue);
    }
    else
    {
      for (int i=0; i<_data.Size(); i++)
      {
        op(_data.Set(i));
      }
    }
  }
  template <class Op>
  void ForEach(Op &op)
  {
    if (_virtualSize>0)
    {
      op(_virtualValue);
    }
    else
    {
      for (int i=0; i<_data.Size(); i++)
      {
        op(_data.Set(i));
      }
    }
  }
  __forceinline const Type &operator[] (int i) const {return Get(i);}

  /// write access to all elements - use only when not condensed
  Type *Data()
  {
    Assert(_virtualSize==0);
    return _data.Data();
  }
  
  /// write access to any element - use only when not condensed
  Type &Set(int i)
  {
    Assert(_virtualSize==0);
    return _data.Set(i);
  }
  void Add(const Type &src)
  {
    Assert(_virtualSize==0);
    _data.Add(src);
  }
  void SetAll(const Type &val)
  {
    int size = Size();
    SetCondensed(val,size);
  }
  void Clear()
  {
    _data.Clear();
    _virtualSize = 0;
  }
  void Init(int n)
  {
    Assert(_virtualSize==0);
    _data.Init(n);
  }
  void Compact()
  {
    _data.Compact();
  }
  double GetMemoryUsed() const
  {
    return _data.GetMemoryUsed();
  }
  void Realloc(int size)
  {
    _data.Realloc(size);
  }
  void Resize(int size)
  {
    Assert(_virtualSize==0 || _data.Size()==0);
    if (_virtualSize>0)
    {
      _virtualSize = size;
    }
    else
    {
      _data.Resize(size);
    }
  }
  void Reserve(int needed, int wanted)
  {
    _data.Reserve(needed,wanted);
  }

  //! use 
  __forceinline int Size() const
  {
    Assert(_virtualSize==0 || _data.Size()==0);
    return _virtualSize+_data.Size();
  }
  
  /// if all values are the same, condense the array
  void Condense()
  {
    // check if all values are the same
    if (_data.Size()<=0) return; // nothing to condense
    const Type &val = _data[0];
    for (int i=1; i<_data.Size(); i++)
    {
      if (val!=_data[i]) return; // cannot condense
    }
    _virtualSize = _data.Size();
    _virtualValue = val;
    _data.Clear();
  }
  /// if condensed, make sure all values are stored
  void Expand()
  {
    if (_virtualSize<=0) return; // nothing to expand
    _data.Realloc(_virtualSize);
    _data.Resize(_virtualSize);
    for (int i=0; i<_virtualSize; i++)
    {
      _data[i] = _virtualValue;
    }
    // _virtualValue is no longer used and could be destroyed, if we knew how
    _virtualSize = 0;
  }
  //! expand to autoarray
  template <class ArrayType>
  void ExpandTo(ArrayType &tgt) const
  {
    Assert(_virtualSize==0 || _data.Size()==0);
    tgt.Realloc(Size());
    tgt.Resize(Size());
    if (_virtualSize>0)
    {
      for (int i=0; i<_virtualSize; i++)
      {
        tgt.Set(i) = _virtualValue;
      }
    }
    else
    {
      tgt.Copy(_data.Data(),_data.Size());
    }
  }

  void CondenseFrom(const Type *src, int size)
  {
    // check if all values are the same
    if (size<=0)
    {
      Clear();
      return; // nothing to condense
    }
    const Type &val = src[0];
    bool condensed = true;
    for (int i=1; i<size; i++)
    {
      if (val!=src[i])
      {
        // cannot condense
        condensed=false;
        break;
      }
    }
    if (condensed)
    {
      _virtualSize = size;
      _virtualValue = val;
      _data.Clear();
    }
    else
    {
      _data.Copy(src,size);
      _virtualSize = 0;
    }
  }

  /// condense from an autoarray
  template <class ArrayType>
  void CondenseFrom(const ArrayType &src)
  {
    CondenseFrom(src.Data(),src.Size());
  }
  /// set same value for all entries
  void SetCondensed(const Type &val, int size)
  {
    _virtualValue = val;
    _virtualSize = size;
    _data.Clear();
  }

  size_t GetMemoryAllocated() const
  {
    return _data.GetMemoryAllocated();
  }
  ClassIsMovableZeroed(CondensedAutoArray)
};

#endif
