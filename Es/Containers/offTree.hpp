#ifdef _MSC_VER
#pragma once
#endif

#ifndef __ES_QUAD_TREE_HPP
#define __ES_QUAD_TREE_HPP

/**
@file
quadtree with overlapping borders
*/

#include <Es/essencepch.hpp>
#include <Es/Memory/memAlloc.hpp>
#include <Es/Types/pointers.hpp>
#include <Es/Containers/array.hpp>
#include <Es/Strings/bString.hpp>


template <class Type>
struct OffTreeNumericTraitsDef
{
  static Type Min(Type a, Type b) {return ( a<b ? a : b);}
  static Type Max(Type a, Type b) {return ( a>b ? a : b);}
  
  static Type Mul(Type a, float f) {return a*f;}
  
};

template <class Type>
struct OffTreeNumericTraits: public OffTreeNumericTraitsDef<Type>
{
}; 

template <>
struct OffTreeNumericTraits<float>: public OffTreeNumericTraitsDef<float>
{
  static float MaxValue() {return FLT_MAX;}
};

template <>
struct OffTreeNumericTraits<int>: public OffTreeNumericTraitsDef<int>
{
  static int MaxValue() {return INT_MAX;}
  static int Mul(int a, float f) {return toLargeInt(a*f);}
};

template <>
struct OffTreeNumericTraits<long long>: public OffTreeNumericTraitsDef<long long>
{
  static long long MaxValue() {return LLONG_MAX;}
  static long long Mul(long long a, float f) {return to64bInt(a*f);}
};

template <class Numeric, class Traits=OffTreeNumericTraits<Numeric> >
struct OffTreeRegion
{
  typedef Traits NumericTraits;
  typedef Numeric NumericType;
  
  Numeric _begX,_endX;
  Numeric _begZ,_endZ;
  
  OffTreeRegion(){}
  OffTreeRegion(Numeric begX, Numeric begZ, Numeric endX, Numeric endZ)
  :_begX(begX),_begZ(begZ),_endX(endX),_endZ(endZ)
  {
  }
  
  bool IsIntersection(const OffTreeRegion &reg) const
  {
    return(
      Traits::Max(reg._begX,_begX)<Traits::Min(reg._endX,_endX) &&
      Traits::Max(reg._begZ,_begZ)<Traits::Min(reg._endZ,_endZ)
    );
  }
  ClassIsSimple(OffTreeRegion)
};

/// interface between OffTree items and OffTreeRegion
template <class Type, class Allocator=MemAllocD>
struct OffTreeRegionTraits
{
  typedef OffTreeRegion<float> Region;
  /// get corresponding region  
  static const Region &GetRegion(const Type &item){return item.GetRegion();}

  static bool IsIntersection(const Type &item, const Region &q) {return item.IsIntersection(q);}
  
  static bool IsEqual(const Type &item1, const Type &item2){return item1==item2;}
};

/// implement query statistics
template <class Type>
struct OffTreeStatTraits: public OffTreeRegionTraits<Type>
{
  typedef typename OffTreeRegionTraits<Type>::Region Region;
  typedef typename Region::NumericType Numeric;
  typedef AutoArray<Type> Container;
  
  static Numeric GetMaxBorder() {return Region::NumericTraits::MaxValue();}
  static float GetRelBorder() {return 0.10f;}

  struct QueryResult
  {
    int regionTests;
    int nodeTests;
    
    QueryResult(){regionTests = 0;nodeTests = 0;}
    /// stats are easy to merge
    void operator += (const QueryResult &b)
    {
      regionTests += b.regionTests;
      nodeTests += b.nodeTests;
    }
  };


  /// perform a check against all regions
  static bool PerformQuery(QueryResult &res, const Region &q, const Container &regions)
  {
    res.regionTests += regions.Size();
    res.nodeTests += 1;
    return false;
  }
};

/// implement real query
template <class Type, class Allocator=MemAllocD>
struct OffTreeDefTraits: public OffTreeRegionTraits<Type, Allocator>
{
  typedef AutoArray<Type, Allocator> Container;
  typedef typename OffTreeRegionTraits<Type>::Region Region;
  typedef typename Region::NumericType Numeric;
  
  static Numeric GetMaxBorder() {return Region::NumericTraits::MaxValue();}
  static float GetRelBorder() {return 0.10f;}

  struct QueryResult
  {
    Container regions;
    
    QueryResult(){}
    /// merging implemented, but it can be quite slow
    void operator += (const QueryResult &b)
    {
      regions.Merge(b.regions);
    }
  };

  /// perform a check against all regions
  static bool PerformQuery(QueryResult &res, const Region &q, const Container &items)
  {
    for (int i=0; i<items.Size(); i++)
    {
      const Type &item = items[i];
      if (OffTreeDefTraits::IsIntersection(item,q))
      {
        res.regions.Add(item);
      }
    }
    // by default search for all elements matching the query
    return false;
  }
  static void SetStorage(const Container &container) {}
  
  template <class Functor>
  static int ForEachWithDelete(Container &container, const Functor &func)
  {
    int size = container.Size();
    container.ForEachWithDelete(func);
    return size-container.Size();
  }
};

/// traits for OffTree property implementation
template <class Type, class Allocator=MemAllocD>
struct OffTreeTraits: public OffTreeDefTraits<Type, Allocator>
{
};


/// one level of offset tree

template <class Type, class Traits=OffTreeTraits<Type>, class Allocator=MemAllocD >
class OffTree;

template <class Type, class Traits, class Allocator>
struct SRefAllocTraits
{
  static void Destruct(Allocator &alloc, Type *type)
  {
    Delete(alloc,type);
  }
};

template <
  class Type, class Traits, class Allocator,
  class AllocTraits=SRefAllocTraits<Type,Traits,Allocator>
>
class SRefAlloc: private NoCopy
{
  Type *_ref;
  
  public:
  SRefAlloc() {_ref = NULL;}
  explicit SRefAlloc(Allocator &alloc)
  :_ref(AllocatorNew<Type>(alloc))
  {}
  void New(Allocator &alloc)
  {
    Assert(!_ref);
    _ref = AllocatorNew<Type>(alloc);
  }
  void Free(Allocator &alloc)
  {
    if (_ref)
    {
      AllocTraits::Destruct(alloc,_ref);
      _ref = NULL;
    }
  }
  ~SRefAlloc()
  {
    Assert(!_ref);
  }
  Type *operator ->() const {return _ref;}
  bool IsNull() const {return _ref==NULL;}
  bool NotNull() const {return _ref!=NULL;}
};

template <class Type, class Traits, class Allocator>
class OffTreeLevel;

//template <class Type, class Traits, class Allocator>
//struct SRefAllocTraits< TypeOffTreeLevel<Type,Traits,Allocator> >

template <class Type, class Traits, class Allocator>
struct SRefAllocTraits< OffTreeLevel<Type,Traits,Allocator>, Traits, Allocator >
{
  static void Destruct(Allocator &alloc, OffTreeLevel<Type,Traits,Allocator> *type)
  {
    type->Free(alloc);
    AllocatorDelete(alloc,type);
  }
};

/// quad-tree with overlapping borders
template <class Type, class Traits, class Allocator>
class OffTreeLevel
{
  //@{ simple access to traits
  typedef typename Traits::Region Region;
  typedef typename Traits::QueryResult QueryResult;
  typedef typename Traits::Container Container;
  typedef typename Region::NumericType Numeric;
  static Numeric GetMaxBorder() {return Traits::GetMaxBorder();}
  static float GetRelBorder() {return Traits::GetRelBorder();}
  //@}
  
  friend class OffTree<Type,Traits,Allocator>;
  /// children
  SRefAlloc<OffTreeLevel,Traits,Allocator> _child[2][2];
  /// list of items contained here
  Container _regions;
  
  #if _DEBUG
  /// count items in this branch
  int _regionsInBranch;
  #endif
  
  /// check if branch is empty - such branch should be removed
  bool IsEmpty() const 
  {
    return (
      _regions.Size()==0 &&
      _child[0][0].IsNull() && _child[0][1].IsNull() &&
      _child[1][0].IsNull() && _child[1][1].IsNull()
    );
  }
  public:
  OffTreeLevel()
  {
//    _child[0][0] = NULL;
//    _child[0][1] = NULL;
//    _child[1][0] = NULL;
//    _child[1][1] = NULL;
    #if _DEBUG
      _regionsInBranch = 0;
    #endif
  }
  /// add a new item into a corresponding location
  void Add(
    const Type &item, Numeric startX, Numeric startZ, Numeric size, OffTree<Type,Traits,Allocator> &alloc
  );
  /// find and remove the item
  int Remove(
    const Type &item, Numeric startX, Numeric startZ, Numeric size, Allocator &alloc
  );
  /// find all items in given region
  bool Query(QueryResult &res, const Region &q, Numeric startX, Numeric startZ, Numeric size) const;

  /// traverse all items in given region
  template <class Functor>
  int ForEach(
    const Region &q, Numeric startX, Numeric startZ, Numeric size,
    const Functor &func, OffTree<Type,Traits,Allocator> &alloc
  );
  
  void Dump(int level, Numeric startX, Numeric startZ, Numeric size) const;
  
  void Free(Allocator &alloc)
  {
    _child[0][0].Free(alloc);
    _child[0][1].Free(alloc);
    _child[1][0].Free(alloc);
    _child[1][1].Free(alloc);
  }
};



template <class Type,class Traits,class Allocator>
void OffTreeLevel<Type,Traits,Allocator>::Add(
  const Type &item, Numeric startX, Numeric startZ, Numeric size,
  OffTree<Type,Traits,Allocator> &alloc
)
{
  #if _DEBUG
  _regionsInBranch++;
  #endif
  Assert(size>0);
  // check if we can subdivide even more
  Numeric halfSize = Region::NumericTraits::Mul(size,0.5f);
  Numeric border = Region::NumericTraits::Min(Region::NumericTraits::Mul(halfSize,GetRelBorder()),GetMaxBorder());
  Numeric begX0 = startX-border;
  Numeric endX0 = startX+halfSize+border;
  Numeric begX1 = startX+halfSize-border;
  Numeric endX1 = startX+size+border;
  Numeric begZ0 = startZ-border;
  Numeric endZ0 = startZ+halfSize+border;
  Numeric begZ1 = startZ+halfSize-border;
  Numeric endZ1 = startZ+size+border;
  // left child: start...mid
  // right child: mid...start+size
  // we can be part of child if: child.beg<=beg && child.end>=end
  // if both contain us, prefer the one containing us "more"
  Numeric edgeDistX0 = -1;
  Numeric edgeDistX1 = -1;
  Numeric edgeDistZ0 = -1;
  Numeric edgeDistZ1 = -1;
  const Region &reg = Traits::GetRegion(item);
  if (reg._begX>=begX0 && reg._endX<=endX0)
  {
    edgeDistX0 = floatMin(reg._begX-begX0,endX0-reg._endX);
  }
  if (reg._begX>=begX1 && reg._endX<=endX1)
  {
    edgeDistX1 = floatMin(reg._begX-begX1,endX1-reg._endX);
  }
  if (reg._begZ>=begZ0 && reg._endZ<=endZ0)
  {
    edgeDistZ0 = floatMin(reg._begZ-begZ0,endZ0-reg._endZ);
  }
  if (reg._begZ>=begZ1 && reg._endZ<=endZ1)
  {
    edgeDistZ1 = floatMin(reg._begZ-begZ1,endZ1-reg._endZ);
  }
  if (edgeDistZ0>edgeDistZ1)
  {
    if (edgeDistX0>edgeDistX1)
    {
      // right child is better
      // if it does not exist, create it
      if (_child[0][0].IsNull()) _child[0][0].New(alloc);
      _child[0][0]->Add(item,startX,startZ,halfSize,alloc);
      return;
    }
    else if (edgeDistX1>=0)
    {
      if (_child[1][0].IsNull()) _child[1][0].New(alloc);
      _child[1][0]->Add(item,startX+halfSize,startZ,halfSize,alloc);
      return;
    }
  }
  else if (edgeDistZ1>=0)
  {
    if (edgeDistX0>edgeDistX1)
    {
      // right child is better
      // if it does not exist, create it
      if (_child[0][1].IsNull()) _child[0][1].New(alloc);
      _child[0][1]->Add(item,startX,startZ+halfSize,halfSize,alloc);
      return;
    }
    else if (edgeDistX1>=0)
    {
      if (_child[1][1].IsNull()) _child[1][1].New(alloc);
      _child[1][1]->Add(item,startX+halfSize,startZ+halfSize,halfSize,alloc);
      return;
    }
  }
  
  // we need to be placed here
  if (_regions.Size()==0) alloc.SetStorage(_regions);
  _regions.Add(item);
}

template <class Type,class Traits,class Allocator>
int OffTreeLevel<Type,Traits,Allocator>::Remove(
  const Type &item, Numeric startX, Numeric startZ, Numeric size,
  Allocator &alloc
)
{
  Assert(size>0);
  // check if we can subdivide even more
  Numeric halfSize = Region::NumericTraits::Mul(size,0.5f);
  Numeric border = Region::NumericTraits::Min(Region::NumericTraits::Mul(halfSize,GetRelBorder()),GetMaxBorder());
  Numeric begX0 = startX-border;
  Numeric endX0 = startX+halfSize+border;
  Numeric begX1 = startX+halfSize-border;
  Numeric endX1 = startX+size+border;
  Numeric begZ0 = startZ-border;
  Numeric endZ0 = startZ+halfSize+border;
  Numeric begZ1 = startZ+halfSize-border;
  Numeric endZ1 = startZ+size+border;
  // left child: start...mid
  // right child: mid...start+size
  // we can be part of child if: child.beg<=beg && child.end>=end
  // if both contain us, prefer the one containing us "more"
  Numeric edgeDistX0 = -1;
  Numeric edgeDistX1 = -1;
  Numeric edgeDistZ0 = -1;
  Numeric edgeDistZ1 = -1;
  const Region &reg = Traits::GetRegion(item);
  if (reg._begX>=begX0 && reg._endX<=endX0)
  {
    edgeDistX0 = floatMin(reg._begX-begX0,endX0-reg._endX);
  }
  if (reg._begX>=begX1 && reg._endX<=endX1)
  {
    edgeDistX1 = floatMin(reg._begX-begX1,endX1-reg._endX);
  }
  if (reg._begZ>=begZ0 && reg._endZ<=endZ0)
  {
    edgeDistZ0 = floatMin(reg._begZ-begZ0,endZ0-reg._endZ);
  }
  if (reg._begZ>=begZ1 && reg._endZ<=endZ1)
  {
    edgeDistZ1 = floatMin(reg._begZ-begZ1,endZ1-reg._endZ);
  }
  if (edgeDistZ0>edgeDistZ1)
  {
    if (edgeDistX0>edgeDistX1)
    {
      // right child is better
      if (_child[0][0].IsNull()) return 0;
      int ret = _child[0][0]->Remove(item,startX,startZ,halfSize,alloc);
      #if _DEBUG
        _regionsInBranch -= ret;
      #endif
      if (_child[0][0]->IsEmpty())
      {
        _child[0][0].Free(alloc);
      }
      return ret;
    }
    else if (edgeDistX1>=0)
    {
      if (_child[1][0].IsNull()) return 0;
      int ret = _child[1][0]->Remove(item,startX+halfSize,startZ,halfSize,alloc);
      #if _DEBUG
        _regionsInBranch -= ret;
      #endif
      if (_child[1][0]->IsEmpty())
      {
        _child[1][0].Free(alloc);
      }
      return ret;
    }
  }
  else if (edgeDistZ1>=0)
  {
    if (edgeDistX0>edgeDistX1)
    {
      // right child is better
      if (_child[0][1].IsNull()) return 0;
      int ret = _child[0][1]->Remove(item,startX,startZ+halfSize,halfSize,alloc);
      #if _DEBUG
        _regionsInBranch -= ret;
      #endif
      if (_child[0][1]->IsEmpty())
      {
        _child[0][1].Free(alloc);
      }
      return ret;
    }
    else if (edgeDistX1>=0)
    {
      if (_child[1][1].IsNull()) return 0;
      int ret = _child[1][1]->Remove(item,startX+halfSize,startZ+halfSize,halfSize,alloc);
      #if _DEBUG
        _regionsInBranch -= ret;
      #endif
      if (_child[1][1]->IsEmpty())
      {
        _child[1][1].Free(alloc);
      }
      return ret;
    }
  }
  
  // we need to be placed here
  int src, dst;
  for (src=0,dst=0; src<_regions.Size(); src++)
  {
    if (!Traits::IsEqual(_regions[src],item))
    {
      _regions[dst++] = _regions[src];
    }
  }
  _regions.Resize(dst);
  #if _DEBUG
    _regionsInBranch -= src-dst;
  #endif
  return src-dst;
}



template <class Type,class Traits, class Allocator>
template <class Functor>
int OffTreeLevel<Type,Traits,Allocator>::ForEach(const Region &q, Numeric startX, Numeric startZ, Numeric size,
  const Functor &func, OffTree<Type,Traits,Allocator> &alloc)
{
  // check if we can subdivide even more
  Numeric halfSize = Region::NumericTraits::Mul(size,0.5f);
  Numeric border = Region::NumericTraits::Min(Region::NumericTraits::Mul(halfSize,GetRelBorder()),GetMaxBorder());
  Numeric begX0 = startX-border;
  Numeric endX0 = startX+halfSize+border;
  Numeric begX1 = startX+halfSize-border;
  Numeric endX1 = startX+size+border;
  Numeric begZ0 = startZ-border;
  Numeric endZ0 = startZ+halfSize+border;
  Numeric begZ1 = startZ+halfSize-border;
  Numeric endZ1 = startZ+size+border;
  // left child: start...mid
  // right child: mid...start+size
  // we can be part of child if: child.beg<=beg && child.end>=end
  // if both contain us, prefer the one containing us "more"

  int deleted = 0;
  if (Region::NumericTraits::Max(q._begZ,begZ0)<Region::NumericTraits::Min(q._endZ,endZ0))
  {
    if (_child[0][0].NotNull() && Region::NumericTraits::Max(q._begX,begX0)<Region::NumericTraits::Min(q._endX,endX0))
    {
      deleted += _child[0][0]->ForEach(q,startX,startZ,halfSize,func,alloc);
      if (_child[0][0]->IsEmpty()) _child[0][0].Free(alloc);
    }
    if (_child[1][0].NotNull() && Region::NumericTraits::Max(q._begX,begX1)<Region::NumericTraits::Min(q._endX,endX1))
    {
      deleted += _child[1][0]->ForEach(q,startX+halfSize,startZ,halfSize,func,alloc);
      if (_child[1][0]->IsEmpty()) _child[1][0].Free(alloc);
    }
  }
  if (Region::NumericTraits::Max(q._begZ,begZ1)<Region::NumericTraits::Min(q._endZ,endZ1))
  {
    if (_child[0][1].NotNull() && Region::NumericTraits::Max(q._begX,begX0)<Region::NumericTraits::Min(q._endX,endX0))
    {
      deleted += _child[0][1]->ForEach(q,startX,startZ+halfSize,halfSize,func,alloc);
      if (_child[0][1]->IsEmpty()) _child[0][1].Free(alloc);
    }
    if (_child[1][1].NotNull() && Region::NumericTraits::Max(q._begX,begX1)<Region::NumericTraits::Min(q._endX,endX1))
    {
      deleted += _child[1][1]->ForEach(q,startX+halfSize,startZ+halfSize,halfSize,func,alloc);
      if (_child[1][1]->IsEmpty()) _child[1][1].Free(alloc);
    }
  }
  // postorder depth-first traversal
  // this way we can safely delete nodes when no longer needed
  deleted += Traits::ForEachWithDelete(_regions,func);
  #if _DEBUG
    _regionsInBranch -= deleted;
  #endif
  return deleted;
}

template <class Type,class Traits, class Allocator>
bool OffTreeLevel<Type,Traits,Allocator>::Query(QueryResult &res, const Region &q, Numeric startX, Numeric startZ, Numeric size) const
{
  //Log("Query %g,%g - %d: %g,%g,%g",begX,begZ,level,startX,startZ,size);
  // check intersection with children
  if (Traits::PerformQuery(res,q,_regions))
  {
    return true;
  }
  // check if we can subdivide even more
  Numeric halfSize = Region::NumericTraits::Mul(size,0.5f);
  Numeric border = Region::NumericTraits::Min(Region::NumericTraits::Mul(halfSize,GetRelBorder()),GetMaxBorder());
  Numeric begX0 = startX-border;
  Numeric endX0 = startX+halfSize+border;
  Numeric begX1 = startX+halfSize-border;
  Numeric endX1 = startX+size+border;
  Numeric begZ0 = startZ-border;
  Numeric endZ0 = startZ+halfSize+border;
  Numeric begZ1 = startZ+halfSize-border;
  Numeric endZ1 = startZ+size+border;
  // left child: start...mid
  // right child: mid...start+size
  // we can be part of child if: child.beg<=beg && child.end>=end
  // if both contain us, prefer the one containing us "more"

  if (Region::NumericTraits::Max(q._begZ,begZ0)<Region::NumericTraits::Min(q._endZ,endZ0))
  {
    if (_child[0][0].NotNull() && Region::NumericTraits::Max(q._begX,begX0)<Region::NumericTraits::Min(q._endX,endX0))
    {
      bool done = _child[0][0]->Query(res,q,startX,startZ,halfSize);
      if (done) return done;
    }
    if (_child[1][0].NotNull() && Region::NumericTraits::Max(q._begX,begX1)<Region::NumericTraits::Min(q._endX,endX1))
    {
      bool done = _child[1][0]->Query(res,q,startX+halfSize,startZ,halfSize);
      if (done) return done;
    }
  }
  if (Region::NumericTraits::Max(q._begZ,begZ1)<Region::NumericTraits::Min(q._endZ,endZ1))
  {
    if (_child[0][1].NotNull() && Region::NumericTraits::Max(q._begX,begX0)<Region::NumericTraits::Min(q._endX,endX0))
    {
      bool done = _child[0][1]->Query(res,q,startX,startZ+halfSize,halfSize);
      if (done) return done;
    }
    if (_child[1][1].NotNull() && Region::NumericTraits::Max(q._begX,begX1)<Region::NumericTraits::Min(q._endX,endX1))
    {
      bool done = _child[1][1]->Query(res,q,startX+halfSize,startZ+halfSize,halfSize);
      if (done) return done;
    }
  }
  return false;
}

template <class Type,class Traits, class Allocator>
void OffTreeLevel<Type,Traits,Allocator>::Dump(int level, Numeric startX, Numeric startZ, Numeric size) const
{
  BString<256> indent;
  for(int i=0; i<level; i++) strcat(indent,"  ");
  #if _DEBUG
  LogF(
    "%sNode %d: %g,%g..%g,%g (%g) - %d regions",
    cc_cast(indent),
    level,startX,startZ,startX+size,startZ+size,size,_regionsInBranch
  );
  #endif
  for (int i=0; i<_regions.Size(); i++)
  {
    const Region &r = _regions[i];
    Log(
      "%s  region %g,%g..%g,%g (%g,%g)",
      cc_cast(indent),
      r._begX,r._begZ,r._endX,r._endZ,r._endX-r._begX,r._endZ-r._begZ
    );
  }
  Numeric halfSize = size*0.5f;
  if (_child[0][0])
  {
    _child[0][0]->Dump(level+1,startX,startZ,halfSize);
  }
  if (_child[1][0])
  {
    _child[1][0]->Dump(level+1,startX+halfSize,startZ,halfSize);
  }
  if (_child[0][1])
  {
    _child[0][1]->Dump(level+1,startX,startZ+halfSize,halfSize);
  }
  if (_child[1][1])
  {
    _child[1][1]->Dump(level+1,startX+halfSize,startZ+halfSize,halfSize);
  }
}

/// offset tree root
template <class Type, class Traits, class Allocator>
class OffTree: public Allocator, public Traits
{
  public:
  //@{ simple access to traits
  typedef typename Traits::QueryResult QueryResult;
  typedef typename Traits::Container Container;
  typedef typename Traits::Region Region;
  typedef typename Region::NumericType Numeric;
  static Numeric GetMaxBorder() {return Traits::GetMaxBorder();}
  static float GetRelBorder() {return Traits::GetRelBorder();}
  //@}
  
  private:
  
  SRefAlloc< OffTreeLevel<Type,Traits,Allocator>, Traits, Allocator > _root;
  //@{ span of the whole tree
  Numeric _startX,_startZ,_size;
  //@}

  public:
  OffTree(Numeric startX=0, Numeric startZ=0, Numeric size=1);
  /// add item to the tree
  void Add(const Type &item);
  /// remove all occurences of the item from the tree
  int Remove(const Type &item);
  /// find items in given area
  bool Query(QueryResult &res, Numeric begX, Numeric begZ, Numeric endX, Numeric endZ) const;
  /// traverse all items in given area
  template <class Functor>
  void ForEach(Numeric begX, Numeric begZ, Numeric endX, Numeric endZ, const Functor &f)
  {
    if (begX<=_startX) begX = _startX;
    if (endX>=_startX+_size) endX = _startX+_size;
    if (begZ<=_startZ) begZ = _startZ;
    if (endZ>=_startZ+_size) endZ = _startZ+_size;
    Region q(begX,begZ,endX,endZ);
    return _root->ForEach(q,_startX,_startZ,_size,f,*this);
  }
  
  template <class Functor>
  void ForEach(const Functor &f)
  {
    Region q(_startX,_startZ,_startX+_size,_startZ+_size);
    _root->ForEach(q,_startX,_startZ,_size,f,*this);
  }
  
  #if _DEBUG
    int RegionCount() const {return _root->_regionsInBranch;}
  #endif
  void Dump() const;
  /// erase data, keep dimensions intact
  void Clear()
  {
    _root.Free(*this);
    _root.New(*this);
  }
  
  bool IsEmpty() const
  {
    if (_root->_regions.Size()>0) return false;
    if (_root->_child[0][0].NotNull()) return false;
    if (_root->_child[0][1].NotNull()) return false;
    if (_root->_child[1][0].NotNull()) return false;
    if (_root->_child[1][1].NotNull()) return false;
    return true;
  }
  /// erase data, change dimensions
  void Dim(Numeric startX, Numeric startZ, Numeric size)
  {
    _startX = startX;
    _startZ = startZ;
    _size = size;
    _root.Free(*this);
    _root.New(*this);
  }
  ~OffTree()
  {
    _root.Free(*this);
  }
};

#pragma warning(disable:4355)
template <class Type, class Traits, class Allocator>
OffTree<Type,Traits,Allocator>::OffTree(Numeric startX, Numeric startZ, Numeric size)
:_startX(startX),_startZ(startZ),_size(size),
_root(*this)
{
}

template <class Type, class Traits, class Allocator>
void OffTree<Type,Traits,Allocator>::Add(const Type &item)
{
  #if _DEBUG
    const Region &reg = Traits::GetRegion(item);
  // find where should we add
    Assert(reg._begX>=_startX);
    Assert(reg._endX<=_startX+_size);
    Assert(reg._begX<=reg._endX);
    Assert(reg._begZ<=reg._endZ);
    Assert(reg._begX<reg._endX || reg._begX<reg._endZ); // zero sized item would require infinite depth - impossible
  #endif
  _root->Add(item,_startX,_startZ,_size,*this);
}

/**
@return how many times item was found
*/
template <class Type, class Traits, class Allocator>
int OffTree<Type,Traits,Allocator>::Remove(const Type &item)
{
  return _root->Remove(item,_startX,_startZ,_size,*this);
}

template <class Type, class Traits, class Allocator>
bool OffTree<Type,Traits,Allocator>::Query(QueryResult &res, Numeric begX, Numeric begZ, Numeric endX, Numeric endZ) const
{
  if (begX<=_startX) begX = _startX;
  if (endX>=_startX+_size) endX = _startX+_size;
  if (begZ<=_startZ) begZ = _startZ;
  if (endZ>=_startZ+_size) endZ = _startZ+_size;
  Region q(begX,begZ,endX,endZ);
  return _root->Query(res,q,_startX,_startZ,_size);
}

template <class Type, class Traits, class Allocator>
void OffTree<Type,Traits,Allocator>::Dump() const
{
  LogF("**** %g,%g,%g",_startX,_startZ,_size);
  _root->Dump(0,_startX,_startZ,_size);
}

#endif
