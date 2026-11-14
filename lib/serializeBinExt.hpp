#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SERIALIZE_BIN_EXT_HPP
#define _SERIALIZE_BIN_EXT_HPP

#include <El/QStream/serializeBin.hpp>
#include <El/Math/math3d.hpp>
#include <El/Math/mathStore.hpp>
#include <El/Color/colors.hpp>
#include "Shape/vertex.hpp"


//@{ vector, matrix ... swap by component
template <> struct EndianTraits<Vector3P>: public EndianTraitsSwap<Coord> {};
template <> struct EndianTraits<Matrix3P>: public EndianTraitsSwap<Coord> {};
template <> struct EndianTraits<Matrix4P>: public EndianTraitsSwap<Coord> {};
template <> struct EndianTraits<ColorP>: public EndianTraitsSwap<float> {};
template <> struct EndianTraits<UVPair>: public EndianTraitsSwap<float> {};
template <> struct EndianTraits<STPair>: public EndianTraitsSwap<int> {};
template <> struct EndianTraits<Matrix4Quat16b>: public EndianTraitsSwap<short> {};
template <> struct EndianTraits<Vector3Compressed>: public EndianTraitsSwap<int> {};
template <> struct EndianTraits<UVPairCompressed>: public EndianTraitsSwap<short> {};
//@}

void operator << ( SerializeBinStream &s, Color &data );
void operator << ( SerializeBinStream &s, PackedColor &data );
void operator << ( SerializeBinStream &s, Vector3 &data );
void operator << ( SerializeBinStream &s, Matrix3 &data );
void operator << ( SerializeBinStream &s, Matrix4 &data );
inline void operator << ( SerializeBinStream &s, DWORD &data ){s << (int &)data;}
inline void operator << ( SerializeBinStream &s, unsigned int &data ){s << (int &)data;}
void operator << ( SerializeBinStream &s, short &data );
inline void operator << ( SerializeBinStream &s, unsigned short &data ){ s << (short&) data;}

inline void operator << ( SerializeBinStream &s, UVPair &data )
{
  s.TransferBinary(data);
  if (s.IsLoading()) s.ProcessLittleEndian(&data,sizeof(data),EndianTraits<UVPair>::BlockSize);
}

#if defined Vector3KPar // TODO: introduce a new macro to switch in cases like this
//@{ SerializeAs specializations - used to serialize arrays of vectors or matrices
template <>
struct SerializeAs<Vector3K>
{
  typedef Vector3K Type;
  typedef Vector3P As;
  
  /// if Type == As, we may use simplified processing
  static SerializeStyle GetStyle() {return SerializeStyleLoadInPlace;}

  /// in-place conversion, src == dst
  static void ConvertOnLoad(Type *dst, As *src, int nElems, bool endianSwap);
};

template <>
struct SerializeAs<Matrix3K>
{
  typedef SerializeAs<Matrix3K> base;
  
  typedef Matrix3K Type;
  typedef Matrix3P As;
  
  /// if Type == As, we may use simplified processing
  static SerializeStyle GetStyle() {return SerializeStyleLoadInPlace;}

  /// in-place conversion, src == dst
  static void ConvertOnLoad(Type *dst, As *src, int nElems, bool endianSwap);
};

template <>
struct SerializeAs<Matrix4K>
{
  typedef Matrix4K Type;
  typedef Matrix4P As;
  
  /// if Type == As, we may use simplified processing
  static SerializeStyle GetStyle() {return SerializeStyleLoadInPlace;}

  /// in-place conversion, src == dst
  static void ConvertOnLoad(Type *dst, As *src, int nElems, bool endianSwap);
};

template <>
struct SerializeAs<STPair>
{
  typedef STPair Type;
  typedef struct As {Vector3P s,t;};

  /// if Type == As, we may use simplified processing
  static SerializeStyle GetStyle() {return SerializeStyleLoadInPlace;}

  /// in-place conversion, src == dst
  static void ConvertOnLoad(Type *dst, As *src, int nElems, bool endianSwap);
};

//@}
#endif

#include <Es/Containers/quadtreeEx.hpp>

#define QUADTREE_ITEMS (QuadTreeEx<ItemType, Traits, logSizeX, logSizeY, Allocator>::SizeX * QuadTreeEx<ItemType, Traits, logSizeX, logSizeY, Allocator>::SizeY)

template <class ItemType, class Traits, int logSizeX, int logSizeY, class Allocator>
void SaveContentBinary(SerializeBinStream &s, QuadTreeEx<ItemType, Traits, logSizeX, logSizeY, Allocator> &quadTree)
{
  const PackedBoolArrayEx<QUADTREE_ITEMS, unsigned short> &flags = quadTree.GetFlagsRaw();
  s.TransferBinary((void *)&flags, sizeof(flags));
  for (int i=0; i<QUADTREE_ITEMS; i++)
  {
    if (flags[i])
      SaveContentBinary(s, quadTree.GetSubtreeRaw(i));
    else
      s << quadTree.GetItemRaw(i);
  }
}

template <class ItemType, class Traits, int logSizeX, int logSizeY, class Allocator>
void SaveContentBinary(SerializeBinStream &s, QuadTreeExRoot<ItemType, Traits, logSizeX, logSizeY, Allocator> &quadTree)
{
  bool flag = quadTree.GetFlagRaw();
  s << flag;
  if (flag)
    SaveContentBinary(s, quadTree.GetSubtreeRaw());
  else
    s << quadTree.GetItemRaw();
}

template <class ItemType, class Traits, int logSizeX, int logSizeY, class Allocator>
void SaveBinary(SerializeBinStream &s, QuadTreeExRoot<ItemType, Traits, logSizeX, logSizeY, Allocator> &quadTree)
{
  s << quadTree.GetXRange();
  s << quadTree.GetYRange();
  s << quadTree.GetOuterRaw();
  SaveContentBinary(s, quadTree);
}

template <class ItemType, class Traits, int logSizeX, int logSizeY, class Allocator>
void LoadContentBinary(SerializeBinStream &s, QuadTreeEx<ItemType, Traits, logSizeX, logSizeY, Allocator> &quadTree)
{
  PackedBoolArrayEx<QUADTREE_ITEMS,unsigned short> flags;
  //s.TransferBinary(&flags, sizeof(flags));
  s.LoadLittleEndian(&flags, sizeof(flags)/sizeof(unsigned short),sizeof(unsigned short));
  for (int i=0; i<QUADTREE_ITEMS; i++)
  {
    if (flags[i])
    {
      QuadTreeEx<ItemType, Traits, logSizeX, logSizeY, Allocator> &subtree = quadTree.CreateSubtreeRaw
      (
        i, typename QuadTreeEx<ItemType, Traits, logSizeX, logSizeY, Allocator>::Type()
      );
      LoadContentBinary(s, subtree);
    }
    else
    {
      typename QuadTreeEx<ItemType, Traits, logSizeX, logSizeY, Allocator>::Type value;
      // transferring value (using Transfer or <<) with endian conversion is not correct/portable
      // we need to perform endian conversion based on ItemType, not on value type
      s.LoadLittleEndian(&value,sizeof(value)/sizeof(ItemType),sizeof(ItemType));
      
      quadTree.CreateValueRaw(i, value);
    }
  }
}

template <class ItemType, class Traits, int logSizeX, int logSizeY, class Allocator>
void LoadContentBinary(SerializeBinStream &s, QuadTreeExRoot<ItemType, Traits, logSizeX, logSizeY, Allocator> &quadTree)
{
  bool flag;
  s << flag;
  if (flag)
  {
    typename QuadTreeExRoot<ItemType, Traits, logSizeX, logSizeY, Allocator>::QuadTreeType &subtree = quadTree.CreateSubtreeRaw
    (
      typename QuadTreeExRoot<ItemType, Traits, logSizeX, logSizeY, Allocator>::Type()
    );
    LoadContentBinary(s, subtree);
  }
  else
  {
    typename QuadTreeExRoot<ItemType, Traits, logSizeX, logSizeY, Allocator>::Type value;
    // transferring value (using Transfer or <<) with endian conversion is not correct/portable
    // we need to perform endian conversion based on ItemType, not on value type
    s.LoadLittleEndian(&value,sizeof(value)/sizeof(ItemType),sizeof(ItemType));
    quadTree.CreateValueRaw(value);
  }
}

template <class ItemType, class Traits, int logSizeX, int logSizeY, class Allocator>
void LoadBinary(SerializeBinStream &s, QuadTreeExRoot<ItemType, Traits, logSizeX, logSizeY, Allocator> &quadTree)
{
  int xRange, yRange;
  s << xRange;
  s << yRange;
  ItemType defValue;
  s << defValue;
  quadTree.Dim(xRange, yRange);
  quadTree.Init(defValue);
  LoadContentBinary(s, quadTree);
}

#endif
