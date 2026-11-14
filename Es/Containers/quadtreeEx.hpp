#ifdef _MSC_VER
#pragma once
#endif

#ifndef __QUAD_TREE_EX_HPP
#define __QUAD_TREE_EX_HPP

#ifdef _CPPRTTI
#include "typeinfo.h"
#endif

#include <Es/Memory/memAlloc.hpp>
#include <Es/Memory/fastAlloc.hpp>
#include <Es/Memory/normalNew.hpp>

template <unsigned int bits, class Item = unsigned char>
class PackedBoolArrayEx
{
  enum {PerItem = sizeof(Item) * 8};
  enum {Size = (bits + PerItem - 1) / PerItem};
  Item _data[Size];

public:
  PackedBoolArrayEx() {Clear();}
  bool Get(unsigned int pos) const
  {
    if (Size == 1)
    {
      return (_data[0] & (1 << pos)) != 0;
    }
    else
    {
      unsigned int index = pos / PerItem;
      Assert(index>=0 && index<Size);
      pos -= index * PerItem;
      return (_data[index] & (1 << pos)) != 0;
    }
  }
  void Set(unsigned int pos, bool value)
  {
    if (Size==1)
    {
      if (value)
        _data[0] |= (1 << pos);
      else
        _data[0] &= (~(1 << pos));
    }
    else
    {
      unsigned int index = pos / PerItem;
      pos -= index * PerItem;
      Assert(index>=0 && index<Size);
      if (value)
        _data[index] |= (1 << pos);
      else
        _data[index] &= (~(1 << pos));
    }
  }
  __forceinline bool operator [] (unsigned int i) const {return Get(i);}
  void Clear()
  {
    memset(_data, 0, Size * sizeof(Item));
  }
  bool IsEmpty() const
  {
    for (unsigned int i=0; i<Size; i++)
    {
      if (_data[i] != 0) return false;
    }
    return true;
  }
  ClassIsSimpleZeroed(PackedBoolArrayEx);
};

template <class ItemType>
struct QuadTreeExTraits
{
  enum {LogSizeX = 0};
  enum {LogSizeY = 0};
  typedef ItemType Type;
  static bool IsEqual(const ItemType &a, const ItemType &b)
  {
    return a==b;
  }
  static const ItemType &Get(const Type &value, int x, int y)
  {
    // Assert(x >= 0 && x < (1 << LogSizeX));
    // Assert(y >= 0 && y < (1 << LogSizeY));
    Assert(x == 0);
    Assert(y == 0);
    return value;
  }
  static void Set(Type &value, int x, int y, const ItemType &item)
  {
    // Assert(x >= 0 && x < (1 << LogSizeX));
    // Assert(y >= 0 && y < (1 << LogSizeY));
    Assert(x == 0);
    Assert(y == 0);
    value = item;
  }
};

template <class ItemType>
struct QuadTreeExTraitsBYTE
{
  enum {LogSizeX = 1};
  enum {LogSizeY = 1};
  typedef DWORD Type;
  static bool IsEqual(ItemType a, ItemType b)
  {
    return a == b;
  }
  static bool IsEqual(DWORD a, DWORD b)
  {
    return a == b;
  }
  static const ItemType &Get(const DWORD &value, int x, int y)
  {
    Assert(x >= 0 && x < 2);
    Assert(y >= 0 && y < 2);
    return ((ItemType *)&value)[(y << 1) + x];
  }
  static void Set(DWORD &value, int x, int y, const ItemType &item)
  {
    Assert(x >= 0 && x < 2);
    Assert(y >= 0 && y < 2);
    ((ItemType *)&value)[(y << 1) + x] = item;
  }
};

template <class ItemType>
struct QuadTreeExTraitsWORD
{
  enum {LogSizeX = 1};
  enum {LogSizeY = 0};
  typedef DWORD Type;
  static bool IsEqual(ItemType a, ItemType b)
  {
    return a == b;
  }
  static bool IsEqual(DWORD a, DWORD b)
  {
    return a == b;
  }
  static const ItemType &Get(const DWORD &value, int x, int y)
  {
    Assert(x >= 0 && x < 2);
    Assert(y == 0);
    return ((ItemType *)&value)[x];
  }
  static void Set(DWORD &value, int x, int y, const ItemType &item)
  {
    Assert(x >= 0 && x < 2);
    Assert(y == 0);
    ((ItemType *)&value)[x] = item;
  }
};

#if USE_FAST_ALLOCATORS
/// Default allocator, using the fast alloc heap
template <class ItemType, class Traits, int logSizeLX, int logSizeLY>
class QuadTreeExAllocator
{
private:
  static FastAllocType _allocator;

public:
#if ALLOC_DEBUGGER
  /// allocate the block
  static void *Alloc(size_t &size, const char *file, int line, const char *postfix)
  {
    // size is not changed
    return _allocator.CAlloc(size);
  }
#else
  /// allocate the block
  static void *Alloc(size_t &size)
  {
    // size is not changed
    return _allocator.CAlloc(size);
  }
#endif

  static void *Realloc(void *mem, size_t oldSize, size_t &size)
  {
    return NULL;
  }

  /// release the block
  static void Free(void *mem, size_t size)
  {
    _allocator.CFree(mem);
  }
  static void Unlink(void *mem) {}

  static inline int MinGrow() {return 1;}
};
#else
template <class ItemType, class Traits, int logSizeLX, int logSizeLY>
class QuadTreeExAllocator : public MemAllocD
{
};
#endif

#include <type_traits>
#include <stdint.h>

template<size_t i>
struct SelectUintType
{
  typedef typename std::conditional<i <= 8*sizeof(unsigned short),unsigned short,unsigned int>::type type;
};

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
class QuadTreeEx : protected Allocator, private NoCopy
{
public:
  enum {logSizeX = logSizeLX};
  enum {logSizeY = logSizeLY};
  enum {SizeX = 1 << logSizeX};
  enum {SizeY = 1 << logSizeY};
  typedef typename Traits::Type Type;

  typedef QuadTreeEx QuadTreeType;

protected:

  typedef QuadTreeType *PtrType;
  enum {ItemSize = sizeof(Type) > sizeof(PtrType) ? sizeof(Type) : sizeof(PtrType)};
  typedef int Item[(ItemSize + sizeof(int) - 1) / sizeof(int)];
  /// data itself
  Item _data[SizeY * SizeX];
  /// _flag[i] == false <=> _data[i] contains value
  PackedBoolArrayEx<SizeY * SizeX, unsigned short> _flags;
  typedef typename SelectUintType<logSizeX+logSizeY>::type ChangesInt; // must be able to represent SizeY*SizeX-1
  ChangesInt _changes; 
  STATIC_ASSERT(8*(int)sizeof(ChangesInt)>=logSizeX+logSizeY);

  void IncrementChanges()
  {
    _changes++;
    // detect overflow - unsigned would go to zero on overflow
    // _changes<SizeY*SizeX, even SizeY*SizeX different values as only SizeY*SizeX-1 differences in between
    //DoAssert(_changes<SizeY*SizeX);
    //DoAssert(_changes>0);
  }

  bool IsChangesValid() const
  {
    int changes = 0;
    for (int i=1; i<SizeX*SizeY; i++)
    {
      if (_flags[i] || _flags[i - 1] || !Traits::IsEqual(AsType(i), AsType(i - 1)))
      {
        changes++;
      }
    }
    if (changes!=_changes)
    {
      LogF("Bad changes %d!=%d in (%s)%#p",changes,_changes,typeid(this).name(),this);
      return false;
    }
    return true;
  }

protected:
  #define ASSERT_I(i) Assert( (i)>=0 && (i)<SizeY*SizeX)
  
  PtrType &AsPtr(int i) {ASSERT_I(i); return *(PtrType *)_data[i];}
  const QuadTreeType *&AsPtr(int i) const {ASSERT_I(i);return *(const QuadTreeType **)_data[i];}
  Type &AsType(int i) {ASSERT_I(i);return *(Type *)_data[i];}
  const Type &AsType(int i) const {ASSERT_I(i);return *(Type *)_data[i];}
  const ItemType &AsItemType(int i, int x, int y) const;

  void SetPtr(int i, PtrType ptr);
  Type ResetValue(int i);
  void SetNewValue(int i, const Type &value);

public:
  QuadTreeEx(const Type &value);
  ~QuadTreeEx();
  
  /// Return value at [x, y]
  const ItemType &Get(int x, int y, unsigned int shiftedX, unsigned int shiftedY) const;

  /// return value of two successive elements
  bool Get2(
    int x, int y, int logSizeTotalX, int logSizeTotalY, int sizeX, int sizeY,
    ItemType &g1, ItemType &g2, unsigned int shiftedX, unsigned int shiftedY, int sizeTotalX, int sizeTotalY
  ) const;

  /// Set value at [x, y] to value
  /**
  \return true if aggregation can be done
  */
  bool Set(int x, int y, int logSizeTotalX, int logSizeTotalY, const ItemType &value, Type &aggregateValue, unsigned int shiftedX, unsigned int shiftedY);

  static bool Advance(int &x, int &y, int logSizeTotalX, int logSizeTotalY);

  // Serialization interface
  Type &GetItemRaw(int i) {return AsType(i);}
  const PackedBoolArrayEx<SizeY * SizeX, unsigned short> &GetFlagsRaw() const {return _flags;}
  QuadTreeType &GetSubtreeRaw(int i) {return *AsPtr(i);}
  void CreateValueRaw(int i, const Type &value) {ResetValue(i);SetNewValue(i, value);}

  // Allocation / deallocation of the subtree
  /// provide public access to the allocator type
  typedef Allocator AllocatorType;
  QuadTreeType &CreateSubtreeRaw(int i, const Type &value)
  {
    // allocate using Allocator, call in-place constructor
    size_t size = sizeof(QuadTreeType);
#if ALLOC_DEBUGGER
    QuadTreeType *subtree = new(AllocatorType::Alloc(size, __FILE__, __LINE__, typeid(QuadTreeType).name())) QuadTreeType(value);
#else
    QuadTreeType *subtree = new(AllocatorType::Alloc(size)) QuadTreeType(value);
#endif
    SetPtr(i, subtree);
    return *subtree;
  }
  void DestroySubtreeRaw(int i)
  {
    PtrType &ptr = AsPtr(i);
    if (ptr)
    {
      // call destructor, then deallocate using Allocator
      ptr->~QuadTreeType();
      AllocatorType::Free(ptr, sizeof(QuadTreeType));
      ptr = NULL;
    }
  }

  size_t GetMemoryAllocated() const;
  
  /// Enumerator
  template <class Func>
  void ForEachItem(const Func &f);

  /// Enumerator
  template <class Func>
  void ForEachItemConst(const Func &f) const;

  /// selective enumerator
  template <class Detect, class Func>
  void ForSomeItem(
    const Detect &det, const Func &f,
    int x, int y, int logSizeTotalX, int logSizeTotalY
  );

  /// selective enumerator
  template <class Detect, class Func>
  void ForSomeItemConst(
    const Detect &det, const Func &f,
    int x, int y, int logSizeTotalX, int logSizeTotalY
  ) const;

  template <class Detect, class Func>
  static void ForSomePosition(
    const Detect &det, const Func &f,
    int x, int y, int logSizeTotalX, int logSizeTotalY
  );
};

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
QuadTreeEx<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::QuadTreeEx(const Type &value)
{
  for (int i=0; i<SizeY*SizeX; i++)
  {
    // _flags[i] == false
    ConstructTraits<Type>::CopyConstruct(AsType(i), value);
  }
  _changes = 0;
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
QuadTreeEx<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::~QuadTreeEx()
{
  for (int i=0; i<SizeY*SizeX; i++)
  {
    if (_flags[i])
      DestroySubtreeRaw(i);
    else
      ConstructTraits<Type>::Destruct(AsType(i));
  }
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
const ItemType &QuadTreeEx<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::AsItemType(int i, int x, int y) const
{
  int maskX = (1 << Traits::LogSizeX) - 1;
  int maskY = (1 << Traits::LogSizeY) - 1;
  return Traits::Get(AsType(i), x & maskX, y & maskY);
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
void QuadTreeEx<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::SetPtr(int i, QuadTreeEx *ptr)
{
  DoAssert(IsChangesValid());
  if (_flags[i])
  {
    // _changes not changed
    DestroySubtreeRaw(i);
  }
  else
  {
    if (i > 0 && !_flags[i - 1] && Traits::IsEqual(AsType(i), AsType(i - 1)))
      IncrementChanges();
    if (i < SizeY * SizeX - 1 && !_flags[i + 1] && Traits::IsEqual(AsType(i), AsType(i + 1)))
      IncrementChanges();
    ConstructTraits<Type>::Destruct(AsType(i));
  }

  _flags.Set(i, true);
  AsPtr(i) = ptr;
  DoAssert(IsChangesValid());
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
typename QuadTreeEx<ItemType,Traits,logSizeLX,logSizeLY,Allocator>::Type QuadTreeEx<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::ResetValue(int i)
{
  //DoAssert(IsChangesValid());
  if (_flags[i])
  {
    if (i > 0) _changes--;
    if (i < SizeY * SizeX - 1) _changes--;
    DestroySubtreeRaw(i);
    _flags.Set(i, false);
    return Type();
  }
  else
  {
    if (i > 0 && (_flags[i - 1] || !Traits::IsEqual(AsType(i), AsType(i - 1))))
      _changes--;
    if (i < SizeY * SizeX - 1 && (_flags[i + 1] || !Traits::IsEqual(AsType(i), AsType(i + 1))))
      _changes--;
    Type ret = AsType(i);
    ConstructTraits<Type>::Destruct(AsType(i));
    return ret;
  }
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
void QuadTreeEx<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::SetNewValue(int i, const Type &value)
{
  ConstructTraits<Type>::CopyConstruct(AsType(i), value);
  if (i > 0 && (_flags[i - 1] || !Traits::IsEqual(AsType(i), AsType(i - 1))))
    IncrementChanges();
  if (i < SizeY * SizeX - 1 && (_flags[i + 1] || !Traits::IsEqual(AsType(i), AsType(i + 1))))
    IncrementChanges();
  DoAssert(IsChangesValid());
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
const ItemType &QuadTreeEx<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::Get(int x, int y, unsigned int shiftedX, unsigned int shiftedY) const
{
  // use (logSize) highest bits of shiftedX, shiftedX as indices
  int indexX = shiftedX >> (8 * sizeof(int) - logSizeX);
  int indexY = shiftedY >> (8 * sizeof(int) - logSizeY);
  // 2D to 1D array conversion
  int index = (indexY << logSizeX) + indexX;
  if (_flags[index])
  {
    // move shiftedX, shiftedY to make next bits available
    return AsPtr(index)->Get(x, y, shiftedX << logSizeX, shiftedY << logSizeY);
  }
  else
  {
    // mask only lowest bits from the original x, y
    int maskX = (1 << Traits::LogSizeX) - 1;
    int maskY = (1 << Traits::LogSizeY) - 1;
    return Traits::Get(AsType(index), x & maskX, y & maskY);
  }
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
bool QuadTreeEx<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::Get2(
  int x, int y, int logSizeTotalX, int logSizeTotalY, int sizeX, int sizeY,
  ItemType &g1, ItemType &g2, unsigned int shiftedX, unsigned int shiftedY, int sizeTotalX, int sizeTotalY
) const
{
  int indexX = shiftedX >> (8 * sizeof(int) - logSizeX);
  int indexY = shiftedY >> (8 * sizeof(int) - logSizeY);
  int index = (indexY << logSizeX) + indexX;

  int logSizeItemX = logSizeTotalX - logSizeX;
  int logSizeItemY = logSizeTotalY - logSizeY;

  int sizeItemX = sizeTotalX >> logSizeX;
  int sizeItemY = sizeTotalY >> logSizeY;

  if (_flags[index])
  {
    if (AsPtr(index)->Get2(
      // we can now pass the full size as sizeX, sizeY
      // we know limited size can be present only at the root level
      x, y, logSizeItemX, logSizeItemY, sizeItemX, sizeItemY, g1, g2, shiftedX << logSizeX, shiftedY << logSizeY, sizeItemX, sizeItemY
    ))
    {
      return true;
    }
    goto NextIndex;
  }
  else
  {
    int maskX = (1 << Traits::LogSizeX) - 1;
    int maskY = (1 << Traits::LogSizeY) - 1;

    g1 = Traits::Get(AsType(index), x & maskX, y & maskY);

    int xx = x & (sizeItemX - 1), yy = y & (sizeItemY - 1);

    xx++;
    if (xx >= sizeItemX)
    {
      yy++;
      if (yy >= sizeItemY)
      {
        goto NextIndex;
      }
    }
    g2 = Traits::Get(AsType(index), xx & maskX, yy & maskY);
    return true;
  }
  NextIndex:
  int maxX = sizeX >> logSizeItemX, maxY = sizeY >> logSizeItemY;

  for(;;)
  {
    int nIndex = index + 1;
    int yNIndex = nIndex >> logSizeX;
    
    // when y is out of range, we know we are done
    if (yNIndex >= maxY)
    {
      return false;
    }
    int xNIndex = nIndex & ((1 << logSizeX) - 1);
    if (xNIndex >= maxX)
    {
      // we need to skip the rest of the line
      index = nIndex + SizeX - xNIndex - 1;
      continue;
    }
    if (_flags[nIndex])
    {
      g2 = AsPtr(nIndex)->Get(0, 0, 0, 0);
    }
    else
    {
      g2 = Traits::Get(AsType(nIndex), 0, 0);
    }
    return true;
  }
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
bool QuadTreeEx<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::Advance(int &x, int &y, int logSizeTotalX, int logSizeTotalY)
{
  int logSizeItemX = logSizeTotalX - logSizeX;
  int logSizeItemY = logSizeTotalY - logSizeY;

  int maxX = 1 << logSizeTotalX;
  int maxY = 1 << logSizeTotalY;
  
  if (logSizeItemX <= Traits::LogSizeX && logSizeItemY <= Traits::LogSizeY)
  {
    x++;
    if (x >= maxX)
    {
      x = 0;
      y++;
      if (y >= maxY) return false;
    }
    return true;
  }
  else
  {
    int maxItemX = 1 << logSizeItemX;
    int maxItemY = 1 << logSizeItemY;

    int itemX = x & (maxItemX - 1);
    int itemY = y & (maxItemY - 1);

    int restX = x - itemX;
    int restY = y - itemY;

    if (Advance(itemX, itemY, logSizeItemX, logSizeItemX))
    {
      x = restX + itemX;
      y = restY + itemY;
      return true;
    }
    else
    {
      x = restX;
      y = restY;

      x += maxItemX;
      if (x >= maxX)
      {
        x = 0;
        y += maxItemY;
        if (y >= maxY) return false;
      }
      return true;
    }
  }
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
bool QuadTreeEx<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::Set(int x, int y, int logSizeTotalX, int logSizeTotalY, const ItemType &value, Type &aggregateValue, unsigned int shiftedX, unsigned int shiftedY)
{
  int logSizeItemX = logSizeTotalX - logSizeX;
  int logSizeItemY = logSizeTotalY - logSizeY;

  int indexX = shiftedX >> (8 * sizeof(int) - logSizeX);
  int indexY = shiftedY >> (8 * sizeof(int) - logSizeY);
  int index = (indexY << logSizeX) + indexX;
  
  if (_flags[index])
  {
    DoAssert(IsChangesValid());
    bool aggregate = AsPtr(index)->Set(x, y, logSizeItemX, logSizeItemY, value, aggregateValue, shiftedX << logSizeX, shiftedY << logSizeY);
    if (!aggregate) return false;
    ResetValue(index);
    SetNewValue(index, aggregateValue);
    DoAssert(IsChangesValid());
    return _changes == 0;
  }
  else if (Traits::IsEqual(AsItemType(index, x, y), value))
  {
    // no change
    return false;
  }
  else if (logSizeItemX <= Traits::LogSizeX && logSizeItemY <= Traits::LogSizeY)
  {
    #if _DEBUG
      int oldChanges = _changes;auto oldFlags = _flags;
      (void)oldChanges;
      Item oldData[SizeY * SizeX];
      memcpy(oldData,_data,sizeof(oldData));
      STATIC_ASSERT(sizeof(oldData)==sizeof(_data));
    #endif
    DoAssert(IsChangesValid());
    aggregateValue = ResetValue(index);
    int maskX = (1 << Traits::LogSizeX) - 1;
    int maskY = (1 << Traits::LogSizeY) - 1;
    Traits::Set(aggregateValue, x & maskX, y & maskY, value);
    SetNewValue(index, aggregateValue);
    return _changes == 0;
  }
  else
  {
    // split
    DoAssert(IsChangesValid());
    QuadTreeType &newItem = CreateSubtreeRaw(index, AsType(index));
    Type aggregateValue;
    Verify(!newItem.Set(x, y, logSizeItemX, logSizeItemY, value, aggregateValue, shiftedX << logSizeX, shiftedY << logSizeY));
    DoAssert(IsChangesValid());
    return false;
  }
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
size_t QuadTreeEx<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::GetMemoryAllocated() const
{
  size_t ret = sizeof(*this);
  for (int i=0; i<SizeY * SizeX; i++)
  {
    if (_flags[i])
    {
      ret += AsPtr(i)->GetMemoryAllocated();
    }
  }
  return ret;
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
template <class Func>
void QuadTreeEx<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::ForEachItem(const Func &f)
{
  for (int i=0; i<SizeY * SizeX; i++)
  {
    if (_flags[i])
      AsPtr(i)->ForEachItem(f);
    else
    {
      Type value = AsType(i);
      Assert(Traits::IsEqual(value, AsType(i))); // assert for Ptr - AsType is invalidate during operator =
      for (int y=0; y<(1<<Traits::LogSizeY); y++)
        for (int x=0; x<(1<<Traits::LogSizeX); x++)
        {
          Traits::Set(value, x, y, f(Traits::Get(value, x, y)));
        }
      ResetValue(i);
      SetNewValue(i, value);
    }
  }
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
template <class Func>
void QuadTreeEx<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::ForEachItemConst(const Func &f) const
{
  for (int i=0; i<SizeY * SizeX; i++)
  {
    if (_flags[i])
      AsPtr(i)->ForEachItemConst(f);
    else
    {
      const Type &value = AsType(i);
      Assert(Traits::IsEqual(value, AsType(i))); // assert for Ptr - AsType is invalidate during operator =
      for (int y=0; y<(1<<Traits::LogSizeY); y++)
        for (int x=0; x<(1<<Traits::LogSizeX); x++)
        {
          f(Traits::Get(value, x, y));
        }
    }
  }
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
template <class Detect, class Func>
void QuadTreeEx<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::ForSomeItem(
  const Detect &det, const Func &f,
  int x, int y, int logSizeTotalX, int logSizeTotalY
)
{
  int logSizeItemX = logSizeTotalX - logSizeX;
  int logSizeItemY = logSizeTotalY - logSizeY;
  int sizeItemX = 1<<logSizeItemX;
  int sizeItemY = 1<<logSizeItemY;
  
  int index = 0;
  for (int yy=0; yy<SizeY; yy++)
  for (int xx=0; xx<SizeX; xx++,index++)
  {
    int itemX = x+(xx<<logSizeItemX);
    int itemY = y+(yy<<logSizeItemY);
    if (_flags[index])
    {
      if (det(itemX,itemY,sizeItemX,sizeItemY))  
      {
        /// TODO: tristate det, call ForEach if possible
        AsPtr(index)->ForSomeItem(det,f,itemX,itemY,logSizeItemX,logSizeItemY);
      }
    }
    else
    {
      Type value = AsType(index);
      int xValueSize = 1<<(logSizeItemX-Traits::LogSizeX);
      int yValueSize = 1<<(logSizeItemY-Traits::LogSizeY);
      Assert(Traits::IsEqual(value, AsType(index))); // assert for Ptr - AsType is invalidate during operator =
      for (int y=0; y<(1<<Traits::LogSizeY); y++)
        for (int x=0; x<(1<<Traits::LogSizeX); x++)
        {
          int xValue = itemX+(x<<(logSizeItemX-Traits::LogSizeX));
          int yValue = itemY+(y<<(logSizeItemY-Traits::LogSizeY));
          if (det(xValue,yValue,xValueSize,yValueSize))
          {
            Traits::Set(value, x, y, f(Traits::Get(value, x, y),xValue,yValue,xValueSize,yValueSize));
          }
        }
      ResetValue(index);
      SetNewValue(index, value);
    }
  }
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
template <class Detect, class Func>
void QuadTreeEx<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::ForSomeItemConst(
  const Detect &det, const Func &f,
  int x, int y, int logSizeTotalX, int logSizeTotalY
) const
{
  int logSizeItemX = logSizeTotalX - logSizeX;
  int logSizeItemY = logSizeTotalY - logSizeY;
  int sizeItemX = 1<<logSizeItemX;
  int sizeItemY = 1<<logSizeItemY;
  
  int index = 0;
  for (int yy=0; yy<SizeY; yy++)
  for (int xx=0; xx<SizeX; xx++,index++)
  {
    int itemX = x+(xx<<logSizeItemX);
    int itemY = y+(yy<<logSizeItemY);
    if (_flags[index])
    {
      if (det(itemX,itemY,sizeItemX,sizeItemY))  
      {
        /// TODO: tristate det, call ForEach if possible
        AsPtr(index)->ForSomeItemConst(det,f,itemX,itemY,logSizeItemX,logSizeItemY);
      }
    }
    else
    {
      const Type &value = AsType(index);
      int xValueSize = 1<<(logSizeItemX-Traits::LogSizeX);
      int yValueSize = 1<<(logSizeItemY-Traits::LogSizeY);
      Assert(Traits::IsEqual(value, AsType(index))); // assert for Ptr - AsType is invalidate during operator =
      for (int y=0; y<(1<<Traits::LogSizeY); y++)
        for (int x=0; x<(1<<Traits::LogSizeX); x++)
        {
          int xValue = itemX+(x<<(logSizeItemX-Traits::LogSizeX));
          int yValue = itemY+(y<<(logSizeItemY-Traits::LogSizeY));
          if (det(xValue,yValue,xValueSize,yValueSize))
          {
            f(Traits::Get(value, x, y),xValue,yValue,xValueSize,yValueSize);
          }
        }
    }
  }
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
template <class Detect, class Func>
void QuadTreeEx<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::ForSomePosition(
  const Detect &det, const Func &f,
  int x, int y, int logSizeTotalX, int logSizeTotalY
)
{
  int logSizeItemX = logSizeTotalX - logSizeX;
  int logSizeItemY = logSizeTotalY - logSizeY;
  int sizeItemX = 1<<logSizeItemX;
  int sizeItemY = 1<<logSizeItemY;
  
  for (int yy=0; yy<SizeY; yy++)
  for (int xx=0; xx<SizeX; xx++)
  {
    int itemX = x+(xx<<logSizeItemX);
    int itemY = y+(yy<<logSizeItemY);
    if (logSizeItemX>Traits::LogSizeX || logSizeItemY>Traits::LogSizeY)
    {
      if (det(itemX,itemY,sizeItemX,sizeItemY))  
      {
        /// TODO: tristate det, call ForEach if possible
        ForSomePosition(det,f,itemX,itemY,logSizeItemX,logSizeItemY);
      }
    }
    else
    {
      int xValueSize = 1<<(logSizeItemX-Traits::LogSizeX);
      int yValueSize = 1<<(logSizeItemY-Traits::LogSizeY);
      Assert(xValueSize==1);
      Assert(yValueSize==1);
      for (int y=0; y<(1<<Traits::LogSizeY); y++)
        for (int x=0; x<(1<<Traits::LogSizeX); x++)
        {
          int xValue = itemX+(x<<(logSizeItemX-Traits::LogSizeX));
          int yValue = itemY+(y<<(logSizeItemY-Traits::LogSizeY));
          if (det(xValue,yValue,xValueSize,yValueSize))
          {
            f(xValue,yValue);
          }
        }
    }
  }
}

// Default allocator implementation
#if USE_FAST_ALLOCATORS

template <class ItemType, class Traits, int logSizeLX, int logSizeLY>
FastCAlloc QuadTreeExAllocator<ItemType, Traits, logSizeLX, logSizeLY>::_allocator(
  sizeof(QuadTreeEx<ItemType, Traits, logSizeLX, logSizeLY, QuadTreeExAllocator>),
  typeid(QuadTreeEx<ItemType, Traits, logSizeLX, logSizeLY, QuadTreeExAllocator>).name());

#endif

template < class ItemType, class Traits=QuadTreeExTraits<ItemType>, int logSizeLX = 2, int logSizeLY = 2,
  class Allocator=QuadTreeExAllocator<ItemType, Traits, logSizeLX, logSizeLY> >
class QuadTreeExRoot : protected Allocator, private NoCopy
{
//  enum {logSizeX = logSizeLX-Traits::LogSizeX>=0 ? logSizeLX-Traits::LogSizeX : 0};
//  enum {logSizeY = logSizeLY-Traits::LogSizeY>=0 ? logSizeLY-Traits::LogSizeY : 0};
  enum {logSizeX = logSizeLX};
  enum {logSizeY = logSizeLY};
public:
  typedef typename Traits::Type Type;
  typedef QuadTreeEx<ItemType, Traits, logSizeX, logSizeY, Allocator> QuadTreeType;

protected:
  
  typedef QuadTreeType *PtrType;
  enum {ItemSize = sizeof(Type) > sizeof(PtrType) ? sizeof(Type) : sizeof(PtrType)};
  typedef int Item[(ItemSize + sizeof(int) - 1) / sizeof(int)];

  ItemType _outer;
  Item _inner;
  bool _flag;

  //@{ logical dimension - how many items user wants to store
  int _sizeX;
  int _sizeY;
  //@}

  //@{ storage structure description - count of "logical" items structure is able to hold
  int _logSizeTotalX;
  int _logSizeTotalY;
  /// precalculated (1 << _logSizeTotalX) - too expensive on Xbox
  int _sizeTotalX;
  /// precalculated (1 << _logSizeTotalY) - too expensive on Xbox
  int _sizeTotalY;
  //@}

public:
  /// QuadTree iterator - iterates through all members in quadtree order
  /**
  This iterator walks through all members, with no respect to real quadtree structure.
  If some area is merged, it is visited several times by the iterator.
  If you want to visit unique quad-tree nodes only, use ForEach or ForSome members.
  */
  struct Iterator
  {
    int x;
    int y;
    const QuadTreeExRoot &container;

    Iterator(const QuadTreeExRoot &cont)
      : container(cont)
    {
      x = 0; y = 0;
    }
    Iterator(
      const QuadTreeExRoot &cont, int xx, int yy
    )
    : container(cont)
    {
      x = xx; y = yy;
    }
    
    operator bool () const {return x < container.GetXRange() && y < container.GetYRange();}
    void operator ++ ()
    {
      do
      {
        container.Advance(x, y);
        // if (x >= container.GetXRange() && y >= container.GetYRange()) return; // out of 
        // if (x < container.GetXRange() && y < container.GetYRange()) return;
      } while ((x >= container.GetXRange()) != (y >= container.GetYRange()));
    }
    const ItemType &operator * ()
    {
      return container.Get(x, y);
    }
  };

protected:
  PtrType &AsPtr() {return *(PtrType *)_inner;}
  const QuadTreeType *&AsPtr() const {return *(const QuadTreeType **)_inner;}
  Type &AsType(){return *(Type *)_inner;}
  const Type &AsType() const {return *(Type *)_inner;}
  const ItemType &AsItemType(int x, int y) const;

  void SetPtr(QuadTreeType *ptr);
  void SetValue(const Type &value);

  void Advance(int &x, int &y) const;

public:
  QuadTreeExRoot(int sizeX, int sizeY, const ItemType &value);
  /// Copy constructor
  /**
  \param source Reference to the source structure. The source will be invalid then.
  */
  QuadTreeExRoot(QuadTreeExRoot &source);
  //! Assigning
  /**
  \param source Reference to the source structure. The source will be invalid then.
  */
  void operator = (QuadTreeExRoot &source);

  ~QuadTreeExRoot();
  void Dim(int sizeX, int sizeY);
  void Init(const ItemType &value);
  /// Return value at [x, y]
  const ItemType &Get(int x, int y) const;

  /// return value of two successive elements
  bool Get2(
    int x, int y, ItemType &g1, ItemType &g2
  ) const;

  /// Set value at [x, y] to value
  void Set(int x, int y, const ItemType &value);

  //! read only access
  __forceinline const ItemType &operator ()(int x, int y) const
  {
    return Get(x, y);
  }

  int GetXRange() const {return _sizeX;}
  int GetYRange() const {return _sizeY;}

  // Serialization interface
  const ItemType &GetOuterRaw() const {return _outer;}
  bool GetFlagRaw() const {return _flag;}
  QuadTreeType &GetSubtreeRaw() {return *AsPtr();}
  Type &GetItemRaw() {return AsType();}
  void CreateValueRaw(const Type &value) {SetValue(value);}

  // Allocation / deallocation of the subtree
  /// provide public access to the allocator type
  typedef Allocator AllocatorType;
  QuadTreeType &CreateSubtreeRaw(const Type &value)
  {
    // allocate using Allocator, call in-place constructor
    size_t size = sizeof(QuadTreeType);
#if ALLOC_DEBUGGER
    QuadTreeType *subtree = new(AllocatorType::Alloc(size, __FILE__, __LINE__, typeid(QuadTreeType).name())) QuadTreeType(value);
#else
    QuadTreeType *subtree = new(AllocatorType::Alloc(size)) QuadTreeType(value);
#endif
    SetPtr(subtree);
    return *subtree;
  }
  void DestroySubtreeRaw()
  {
    PtrType &ptr = AsPtr();
    if (ptr)
    {
      // call destructor, then deallocate using Allocator
      ptr->~QuadTreeType();
      AllocatorType::Free(ptr, sizeof(QuadTreeType));
      ptr = NULL;
    }
  }

  /// get memory statistics
  size_t GetMemoryAllocated() const;
  
  // Enumerator
  template <class Func>
  void ForEachItem(const Func &f);

  // Enumerator
  template <class Func>
  void ForEachItem(const Func &f) const;

  /// selective enumerator - can modify
  template <class Detect, class Func>
  void ForSomeItem(const Detect &det, const Func &f);

  /// selective enumerator - read only
  template <class Detect, class Func>
  void ForSomeItem(const Detect &det, const Func &f) const;

  // selective enumerator - enumerates all possible positions, not respecting real structure
  template <class Detect, class Func>
  void ForSomePosition(
    const Detect &det, const Func &f
  ) const;
};

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::QuadTreeExRoot(int sizeX, int sizeY, const ItemType &value)
{
  Dim(sizeX, sizeY);

  _flag = false;
  _outer = value;
  Type element;
  for (int y=0; y<(1 << Traits::LogSizeY); y++)
    for (int x=0; x<(1 << Traits::LogSizeX); x++)
      Traits::Set(element, x, y, value);
  ConstructTraits<Type>::CopyConstruct(AsType(), element);
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::QuadTreeExRoot(QuadTreeExRoot &source)
{
  _sizeX = source._sizeX;
  _sizeY = source._sizeY;
  _logSizeTotalX = source._logSizeTotalX;
  _logSizeTotalY = source._logSizeTotalY;
  _sizeTotalX = source._sizeTotalX;
  _sizeTotalY = source._sizeTotalY;
  _outer = source._outer;
  _flag = source._flag;
  if (_flag)
  {
    AsPtr() = source.AsPtr();
    source.Init(_outer);
  }
  else
  {
    ConstructTraits<Type>::CopyConstruct(AsType(), source.AsType());
  }
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
void QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::operator = (QuadTreeExRoot &source)
{
  _sizeX = source._sizeX;
  _sizeY = source._sizeY;
  _logSizeTotalX = source._logSizeTotalX;
  _logSizeTotalY = source._logSizeTotalY;
  _sizeTotalX = source._sizeTotalX;
  _sizeTotalY = source._sizeTotalY;
  _outer = source._outer;
  _flag = source._flag;
  if (_flag)
  {
    AsPtr() = source.AsPtr();
    source.Init(_outer);
  }
  else
  {
    AsType() = source.AsType();
  }
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::~QuadTreeExRoot()
{
  if (_flag)
    DestroySubtreeRaw();
  else
    ConstructTraits<Type>::Destruct(AsType());
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
const ItemType &QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::AsItemType(int x, int y) const
{
  int maskX = (1 << Traits::LogSizeX) - 1;
  int maskY = (1 << Traits::LogSizeY) - 1;
  return Traits::Get(AsType(), x & maskX, y & maskY);
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
void QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::Dim(int sizeX, int sizeY)
{
  _sizeX = sizeX;
  _sizeY = sizeY;

  sizeX--;
  _logSizeTotalX = 0;
  while (sizeX)
  {
    _logSizeTotalX++;
    sizeX >>= 1;
  }

  sizeY--;
  _logSizeTotalY = 0;
  while (sizeY)
  {
    _logSizeTotalY++;
    sizeY >>= 1;
  }

  // optimize _logSizeTotalX, _logSizeTotalY
  int numLevelsX = ( _logSizeTotalX - Traits::LogSizeX + logSizeX -1) / logSizeX;
  int numLevelsY = ( _logSizeTotalY - Traits::LogSizeY + logSizeY -1) / logSizeY;
  
  int numLevels = numLevelsX>numLevelsY ? numLevelsX : numLevelsY;
  
  _logSizeTotalX = numLevels * logSizeX + Traits::LogSizeX;
  _logSizeTotalY = numLevels * logSizeY + Traits::LogSizeY;
  _sizeTotalX = 1 << _logSizeTotalX;
  _sizeTotalY = 1 << _logSizeTotalY;
  
  // check if quad tree is well-formed
  Assert(_sizeTotalX >= _sizeX);
  Assert(_sizeTotalY >= _sizeY);
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
void QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::Init(const ItemType &value)
{
  _outer = value;
  Type element;
  for (int y=0; y<(1 << Traits::LogSizeY); y++)
    for (int x=0; x<(1 << Traits::LogSizeX); x++)
      Traits::Set(element, x, y, value);
  SetValue(element);
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
void QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::SetPtr(QuadTreeType *ptr)
{
  if (_flag)
    DestroySubtreeRaw();
  else
    ConstructTraits<Type>::Destruct(AsType());

  _flag = true;
  AsPtr() = ptr;
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
void QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::SetValue(const Type &value)
{
  if (_flag)
    DestroySubtreeRaw();
  else
    ConstructTraits<Type>::Destruct(AsType());

  _flag = false;
  ConstructTraits<Type>::CopyConstruct(AsType(), value);
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
const ItemType &QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::Get(int x, int y) const
{
  if (x < 0 || x >= _sizeTotalX) return _outer;
  if (y < 0 || y >= _sizeTotalY) return _outer;

  unsigned int shiftedX = x << (8 * sizeof(int) - _logSizeTotalX); // make highest bits accessible on left side
  unsigned int shiftedY = y << (8 * sizeof(int) - _logSizeTotalY); // make highest bits accessible on left side
  if (_flag) return AsPtr()->Get(x, y, shiftedX, shiftedY);
  else return AsItemType(x, y);
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
bool QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::Get2
(
  int x, int y, ItemType &g1, ItemType &g2
) const
{
  // negative coordinates are not supported
  Assert(x >= 0 && y >= 0);
  // get outside of the valid area are not supported
  Assert(x < _sizeX && y < _sizeY);
  
  // bug: non pow-2 size currently not supported in Advance
  //Assert(_sizeX == _sizeTotalX);
  //Assert(_sizeY == _sizeTotalY);
  // once advance is fixed, we can use following implementation
  // handle last value
  if (x >= _sizeX - 1 && y >= _sizeY - 1)
  {
    // this actually means querying last item: x == _sizeX - 1 && y == _sizeY - 1
    g1 = Get(x, y);
    return false;
  }
      
  if (
    _flag &&
    x >= 0 && x < _sizeTotalX &&
    y >= 0 && y < _sizeTotalY
  )
  {
    unsigned int shiftedX = x << (8 * sizeof(int) - _logSizeTotalX); // make highest bits accessible on left side
    unsigned int shiftedY = y << (8 * sizeof(int) - _logSizeTotalY); // make highest bits accessible on left side

    bool ret = AsPtr()->Get2(x, y, _logSizeTotalX, _logSizeTotalY, _sizeX, _sizeY, g1, g2, shiftedX, shiftedY, _sizeTotalX, _sizeTotalY);

    #if 0 // _DEBUG //_ENABLE_REPORT
      // verification using an easy solution
      ItemType vg1,vg2;
      vg1 = Get(x,y);
      DoAssert(vg1==g1);
      Iterator it(*this,x,y);
      ++it;
      bool vRet;
      if (it)
      {
        vg2 = *it;
        vRet = true;
        DoAssert(vg2==g2);
      }
      else
      {
        vRet = false;
      }
      DoAssert(vRet==ret);
    #endif

    return ret;

  }
  else
  {
    // no deep structure - use easy solution
    g1 = Get(x, y);
    Iterator it(*this, x, y);
    ++it;
    if (it)
    {
      g2 = *it;
      return true;
    }
    return false;
  }
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
void QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::Advance(int &x, int &y) const
{
  // bug: non pow-2 size currently not supported
  //Assert(_sizeX == _sizeTotalX);
  //Assert(_sizeY == _sizeTotalY);
  if (!QuadTreeType::Advance(x, y, _logSizeTotalX, _logSizeTotalY))
  {
    x = _sizeTotalX;
    y = _sizeTotalY;
  }
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
void QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::Set(int x, int y, const ItemType &value)
{
  if (x < 0 || x >= _sizeTotalX)
  {
    RptF("x (%d) out of range <0, %d)", x, _sizeTotalX);
    return;
  }
  if (y < 0 || y >= _sizeTotalY)
  {
    RptF("y (%d) out of range <0, %d)", y, _sizeTotalY);
    return;
  }
  if (_flag)
  {
    unsigned int shiftedX = x << (8 * sizeof(int) - _logSizeTotalX); // make highest bits accessible on left side
    unsigned int shiftedY = y << (8 * sizeof(int) - _logSizeTotalY); // make highest bits accessible on left side

    Type aggregateValue;
    bool aggregate = AsPtr()->Set(x, y, _logSizeTotalX, _logSizeTotalY, value, aggregateValue, shiftedX, shiftedY);
    if (aggregate) SetValue(aggregateValue);
  }
  else if (Traits::IsEqual(AsItemType(x, y), value))
  {
    // no change
  }
  else
  {
    unsigned int shiftedX = x << (8 * sizeof(int) - _logSizeTotalX); // make highest bits accessible on left side
    unsigned int shiftedY = y << (8 * sizeof(int) - _logSizeTotalY); // make highest bits accessible on left side

    QuadTreeType &newItem = CreateSubtreeRaw(AsType());
    Type aggregateValue;
    Verify(!newItem.Set(x, y, _logSizeTotalX, _logSizeTotalY, value, aggregateValue, shiftedX, shiftedY));
  }
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
template <class Func>
void QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::ForEachItem(const Func &f)
{
  if (_flag)
    AsPtr()->ForEachItem(f);
  else
  {
    Type value = AsType();
    Assert(Traits::IsEqual(value, AsType())); // assert for Ptr - AsType is invalidate during operator =
    for (int y=0; y<(1<<Traits::LogSizeY); y++)
      for (int x=0; x<(1<<Traits::LogSizeX); x++)
      {
        Traits::Set(value, x, y, f(Traits::Get(value, x, y)));
      }
      SetValue(value);
  }
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
template <class Func>
void QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::ForEachItem(const Func &f) const
{
  if (_flag)
    AsPtr()->ForEachItemConst(f);
  else
  {
    const Type &value = AsType();
    Assert(Traits::IsEqual(value, AsType())); // assert for Ptr - AsType is invalidate during operator =
    for (int y=0; y<(1<<Traits::LogSizeY); y++)
      for (int x=0; x<(1<<Traits::LogSizeX); x++)
      {
        f(Traits::Get(value, x, y));
      }
  }
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
size_t QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::GetMemoryAllocated() const
{
  size_t ret = sizeof(*this);
  if (_flag)
  {
    ret += AsPtr()->GetMemoryAllocated();
  }
  return ret;
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
template <class Detect, class Func>
void QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::ForSomeItem(
  const Detect &det, const Func &f
)
{
  if (!det(0,0,_sizeX,_sizeY)) return;
  if (_flag)
  {
    AsPtr()->ForSomeItem(det,f,0,0,_logSizeTotalX,_logSizeTotalY);
  }
  else
  {
    Type value = AsType();
    Assert(Traits::IsEqual(value, AsType())); // assert for Ptr - AsType is invalidate during operator =
    int xValueSize = 1<<(_logSizeTotalX-Traits::LogSizeX);
    int yValueSize = 1<<(_logSizeTotalY-Traits::LogSizeY);
    for (int y=0; y<(1<<Traits::LogSizeY); y++)
      for (int x=0; x<(1<<Traits::LogSizeX); x++)
      {
        int xValue = x<<(_logSizeTotalX-Traits::LogSizeX);
        int yValue = y<<(_logSizeTotalY-Traits::LogSizeY);
        if (det(xValue,yValue,xValueSize,yValueSize))
        {
          Traits::Set(value, x, y, f(Traits::Get(value, x, y), xValue,yValue,xValueSize,yValueSize));
        }
      }
      SetValue(value);
  }
}

template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
template <class Detect, class Func>
void QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::ForSomeItem(
  const Detect &det, const Func &f
) const
{
  if (!det(0,0,_sizeX,_sizeY)) return;
  if (_flag)
  {
    AsPtr()->ForSomeItemConst(det,f,0,0,_logSizeTotalX,_logSizeTotalY);
  }
  else
  {
    const Type &value = AsType();
    Assert(Traits::IsEqual(value, AsType())); // assert for Ptr - AsType is invalidate during operator =
    int xValueSize = 1<<(_logSizeTotalX-Traits::LogSizeX);
    int yValueSize = 1<<(_logSizeTotalY-Traits::LogSizeY);
    for (int y=0; y<(1<<Traits::LogSizeY); y++)
      for (int x=0; x<(1<<Traits::LogSizeX); x++)
      {
        int xValue = x<<(_logSizeTotalX-Traits::LogSizeX);
        int yValue = y<<(_logSizeTotalY-Traits::LogSizeY);
        if (det(xValue,yValue,xValueSize,yValueSize))
        {
          f(Traits::Get(value, x, y), xValue,yValue,xValueSize,yValueSize);
        }
      }
  }
}


template <class ItemType, class Traits, int logSizeLX, int logSizeLY, class Allocator>
template <class Detect, class Func>
void QuadTreeExRoot<ItemType, Traits, logSizeLX, logSizeLY, Allocator>::ForSomePosition(
  const Detect &det, const Func &f
) const
{
  if (!det(0,0,_sizeX,_sizeY)) return;
  if (_logSizeTotalX>Traits::LogSizeX || _logSizeTotalY>Traits::LogSizeY)
  {
    QuadTreeType::ForSomePosition(det,f,0,0,_logSizeTotalX,_logSizeTotalY);
  }
  else
  {
    int xValueSize = 1<<(_logSizeTotalX-Traits::LogSizeX);
    int yValueSize = 1<<(_logSizeTotalY-Traits::LogSizeY);
    Assert(xValueSize==1);
    Assert(yValueSize==1);
    for (int y=0; y<(1<<Traits::LogSizeY); y++)
      for (int x=0; x<(1<<Traits::LogSizeX); x++)
      {
        int xValue = x<<(_logSizeTotalX-Traits::LogSizeX);
        int yValue = y<<(_logSizeTotalY-Traits::LogSizeY);
        if (det(xValue,yValue,xValueSize,yValueSize))
        {
          f(xValue,yValue);
        }
      }
  }
}

#include <Es/Memory/debugNew.hpp>

#endif

