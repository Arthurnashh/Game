#ifdef _MSC_VER
#pragma once
#endif

#ifndef __SUMA_CLASS_SORTED_ARRAY_HPP
#define __SUMA_CLASS_SORTED_ARRAY_HPP

#include <Es/Containers/array.hpp>
#include <Es/Algorithms/qsort.hpp>
#include <Es/Algorithms/bSearch.hpp>

/// traits for key comparison and searching
template <class Type>
struct BinFindArrayKeyTraits
{
  typedef const Type &KeyType;
  /// check if two keys are equal
  static bool IsEqual(KeyType a, KeyType b) {return a==b;}
  /// check if one key is lower than the other
  static bool IsLess(KeyType a, KeyType b) {return a<b;}
  /// get a key from an item
  static KeyType GetKey(const Type &a) {return a;}
};


//! array with searching based on key - binary search used
/**each item may be inserted once only */
template <class Type, class Traits=BinFindArrayKeyTraits<Type>, class Allocator=MemAllocD>
class BinFindArrayKey: public AutoArray<Type,Allocator>
{
  typedef typename Traits::KeyType KeyType;
  typedef AutoArray<Type,Allocator> base;

  public:
  /// if it already exists, return existing entry. If not, create a new one
  int FindOrAdd( const Type &src );
  //! add unless there already is an item with the same key
  int AddUnique( const Type &src );
  //! find first item with the same key
  int Find( const Type &src ) const {return FindKey(Traits::GetKey(src));}
  //! find item by key
  int FindKey(KeyType key) const;
  //! find suitable position for a key, report if exact match was found
  int FindKeyPos(KeyType key, bool &found) const;
  //! delete first item with the same key
  bool Delete( const Type &src ){return DeleteKey(Traits::GetKey(src));}
  //! delete first item with given key
  bool DeleteKey(KeyType src);
  void DeleteAt( int index ) {AutoArray<Type,Allocator>::Delete(index,1);}
  void DeleteAt( int index, int count ) {AutoArray<Type,Allocator>::Delete(index,count);}

	//! replace value with another, without damaging ordering
	void ReplaceAt(int index, const Type &src)
	{
		base::Set(index) = src;

    Assert(index==0 || Traits::IsLess(Traits::GetKey(base::Get(index-1)),Traits::GetKey(base::Get(index))));
    Assert(index==Size()-1 || Traits::IsLess(Traits::GetKey(base::Get(index)),Traits::GetKey(base::Get(index+1))));
	}

  #if _MSC_VER>=1300
  /// AddUnique all elements from another array
  template <class ArrayType>
  void MergeUnique(const ArrayType &src)
  {
    for (int i=0; i<src.Size(); i++)
    {
      AddUnique(src[i]);
    }
  }
  #endif
  
  bool CheckIntegrity() const
  {
    for (int i=1; i<base::Size(); i++)
    {
      if (Traits::IsLess(Traits::GetKey(base::Get(i)),Traits::GetKey(base::Get(i-1))))
        return false;
    }
    return true;
  }
  
  ClassIsMovable(BinFindArrayKey);
};

template<class Type,class Traits,class Allocator>
int BinFindArrayKey<Type,Traits,Allocator>::FindKeyPos(KeyType key, bool &found) const
{
  // based on http://www.dcc.uchile.cl/~rbaeza/handbook/algs/3/321.srch.c
  // see also http://www.nist.gov/dads/HTML/binarySearch.html

  // if the new value is outside of the existing range, return Size()
  if(base::Size()==0 || Traits::IsLess(Traits::GetKey(base::_data[base::Size()-1]),key))
  {
    #if _DEBUG
      for (int i=0; i<base::Size(); i++)
      {
        Assert(!Traits::IsEqual(Traits::GetKey(base::Get(i)),key));
      }
    #endif
    found = false;
    return base::Size();
  }

  if(Traits::IsLess(key,Traits::GetKey(base::_data[0])))
  {
    #if _DEBUG
      for (int i=0; i<Size(); i++)
      {
        Assert(!Traits::IsEqual(Traits::GetKey(Get(i)),key));
      }
    #endif
    found = false;
    return 0;
  }

  int high, low; // high / low not inclusive
  for ( low = -1, high = base::Size();  high-low > 1;  )
  {
    int i = (high+low) / 2;
    if (  !Traits::IsLess(Traits::GetKey(base::_data[i]),key) )  high = i;
    else low  = i;
  }
  found = Traits::IsEqual(key,Traits::GetKey(base::_data[high]) );
  #if 0 // _DEBUG
    if (!found)
    {
      for (int i=0; i<Size(); i++)
      {
        Assert(!Traits::IsEqual(Traits::GetKey(Get(i)),key));
      }
    }
  #endif
  return high;
}

template<class Type,class Traits,class Allocator>
int BinFindArrayKey<Type,Traits,Allocator>::FindKey(KeyType key) const
{
  bool found;
  int index = FindKeyPos(key,found);
  if (found) return index;
  return -1;
}

template<class Type,class Traits,class Allocator>
int BinFindArrayKey<Type,Traits,Allocator>::AddUnique( const Type &src )
{
  bool found;
  int pos = FindKeyPos(Traits::GetKey(src),found);
  if (found) return -1;
  base::Insert(pos,src);
  //Assert( CheckIntegrity() );
  return pos;
}

template<class Type,class Traits,class Allocator>
int BinFindArrayKey<Type,Traits,Allocator>::FindOrAdd( const Type &src )
{
  bool found;
  int pos = FindKeyPos(Traits::GetKey(src),found);
  if (found) return pos;
  base::Insert(pos,src);
  //Assert( CheckIntegrity() );
  return pos;
}

template<class Type,class Traits,class Allocator>
bool BinFindArrayKey<Type,Traits,Allocator>::DeleteKey(KeyType key)
{
  int index=FindKey(key);
  if( index<0 ) return false;
  DeleteAt(index);
  return true;
}


/// array in which items are stored sorted
/**
This makes searching for item possible in O(log n) instead of O(n)
*/
template <class Type,class Allocator=MemAllocD,class Traits=BinFindArrayKeyTraits<Type> >
class SortedArray: public BinFindArrayKey<Type,Traits,Allocator>
{

};
#endif
