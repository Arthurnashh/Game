#ifdef _MSC_VER
#pragma once
#endif

#ifndef __HASHMAP_HPP
#define __HASHMAP_HPP

#include <Es/Framework/debugLog.hpp>
#include <Es/Containers/typeOpts.hpp>
#include <Es/Common/fltopts.hpp>
#include <Es/Types/pointers.hpp>
#include <ctype.h>

#ifdef _CPPRTTI
  #include "typeinfo.h"
#endif

#include <Es/Containers/array.hpp>

// WIP:HASHING
///#define DEBUG_HASH_STATISTIC

/// How many items are stored in one slot.
/**
Smaller value makes finding items faster, but memory overhead is higher.
*/
const int DefCoefExpand = 16;

static inline unsigned int CalculateStringHashValue(const char *key, int hashValue=0)
{
  while (*key)
  {
    hashValue = hashValue*33 + (unsigned char)(*key++);
  }
  return hashValue;
}
static inline unsigned int CalculateStringHashValueCI(const char *key, int hashValue=0)
{
  while (*key)
  {
    hashValue = hashValue*33 + (unsigned char)myLower((unsigned char)*key++);
  }
  return hashValue;
}

//! helper for creating MapClassTraits

template <class Type>
struct DefMapClassTraits
{
  /// key type
  typedef const char *KeyType;
  /// calculate hash value
  static unsigned int CalculateHashValue(KeyType key)
  {
    return CalculateStringHashValue(key);
  }

  /// compare keys, return negative when k1<k2, positive when k1>k2, zero when equal
  static int CmpKey(KeyType k1, KeyType k2)
  {
    return strcmp(k1,k2);
  }
  
  static int CoefExpand() {return DefCoefExpand;}
  
  /// get a key for given a item
  static KeyType GetKey(const Type &item) {return item.GetKey();}
  
  /// cleanup zombies (by default do nothing)
  template <class Container>
  static int CleanUp(Container &array){return 0;}
};

//! define properties of class stored in MapStringToClass
template <class Type>
struct MapClassTraits: public DefMapClassTraits<Type>
{
};

/// MapClassTraits variant is case sensitivity is not desired
template <class Type>
struct MapClassTraitsNoCase: public DefMapClassTraits<Type>
{
  typedef typename DefMapClassTraits<Type>::KeyType KeyType;
  /// calculate hash value
  static unsigned int CalculateHashValue(KeyType key)
  {
    return CalculateStringHashValueCI(key);
  }

  /// compare keys, return negative when k1<k2, positive when k1>k2, zero when equal
  static int CmpKey(KeyType k1, KeyType k2)
  {
    return strcmpi(k1,k2);
  }
};

//! Hashing table
/*!
  This class implements a hashing method called hashing with chains. If we are inserting new items
  then the hash table is being resized, if we are removing items the size of the hash table remains
  unchanged. Class Container must have the ability to store class Type values. Class Type must
  implement following functions: GetKey(). Class Container must implement following functions: Size(),
  MaxSize(), Realloc(int), Add(Type). A good example is to use RString as a Type and
  AutoArray<RString> as a Container.
*/
template <class Type, class Container, class Traits = MapClassTraits<Type>, class Allocator = MemAllocD>
class MapStringToClass : protected Allocator
{
  typedef typename Traits::KeyType KeyType;
  typedef ConstructTraits<Container> CTraits;

protected:
  //! Hash table like an array of containers.
  Container *_hashTable;
  //! Size of the hash table in containers (or number of containers in the array if you please...).
  int _tableSize;
  //! Number of items in the hash table.
  int _count;
#ifdef DEBUG_HASH_STATISTIC // WIP:HASHING
  // number of added items (in case of removals, that could be bigger than _count)
  int _numAdds;
#endif
  //! Declaration of static value for each type which will be considered as a NULL value.
  static Type _null;
  //! Constant which defines default size of the table.
  enum {DefTableSize = 15};
public:
  class Iterator
  {
  protected:
    int _table;
    int _item;
    const MapStringToClass<Type, Container, Traits, Allocator> *_base;

    void FindNextValid()
    {
      while (_table < _base->NTables() && _item >= _base->GetTable(_table).Size())
      {
        _table++;
        _item = 0;
      }
      
    }
  public:
    Iterator(const MapStringToClass<Type, Container, Traits, Allocator> &base)
    {
      _table = 0; _item = 0; _base = &base;
      FindNextValid();
    }
    operator bool () const {return _table < _base->NTables();}
    void operator ++ ()
    {
      if (_table >= _base->NTables()) return;
      ++_item;
      FindNextValid();
    }
    const Type &operator * ()
    {
      return _base->GetTable(_table)[_item];
    }
  };

  //! Constructor which will initialize hash table to empty table and set the size of the table (optional).
  /*!
    \param tableSize Size of the hash table.
  */
  MapStringToClass(int tableSize = DefTableSize)
  {
    _hashTable = NULL;
    Init(tableSize);
#ifdef DEBUG_HASH_STATISTIC // WIP:HASHING
    // total number of added items (in case of removals that could be bigger than _count)
    _numAdds = 0;
    LogF("Hash:0x%p created with %d buckets", this, tableSize);
#endif
  }
  //! Destructor will free the allocated memory for the hash table.
  ~MapStringToClass()
  {
    Clear();
#ifdef DEBUG_HASH_STATISTIC // WIP:HASHING
    LogF("Hash:0x%p destructed", this);
#endif
  }
  //! Copy constructor
  MapStringToClass(const MapStringToClass &src);
  //! = operator
  void operator = (const MapStringToClass &src);
  //! Initializes hash table.
  /*!
    This method will free previously allocated space.
    \param tableSize New size of the hash table.
  */
  void Init(int tableSize);
  //! Free hash table from memory.
  void Clear(bool compactTables=true);
  //! Rebuilding of the hash table with new specified size.
  /*!
    Previous data will be rehashed.
    \param tableSize Size of the new hash table.
  */
  void Rebuild(int tableSize);

  /// rebuild the hash table to reduce the storage if it excesses what is needed
  void Optimize();
  
  /// clean-up any zombies (useful only when cleanup is provided with traits)
  void CleanUp();
  
  //! Reserve hash space for given number of elements
  /*!
  If you know you will be adding many elements, using Reserve will make this much faster.
  */
  void Reserve(int nElem);
  
  //! Compact all containers when no other changes are expected (to save memory)
  void Compact();

#if !defined _MSC_VER || _MSC_VER>=1300
  //! Calling of a function for each item in the hash table.
  /*!
    Using this method you cannot change neither class Type members nor this class MapStringToClass members.
    \param func functor to call.
  */
  template <class Func>
  bool ForEachF(Func func) const;

  //! Calling of a function for each item in the hash table.
  /*!
    Using this method you can change both class Type members and this class MapStringToClass members.
    \param Func functor to call.
  */
  template <class Func>
  bool ForEachF(Func func);
#endif
  
  //! Calling of callback function for each item in the hash table.
  /*!
    Using this method you can change both class Type members and this class MapStringToClass members.
    \param Func Callback function to call.
    \param context Parameters data to pass to the callback function.
  */
  void ForEach(void (*Func)(Type &, MapStringToClass *, void *), void *context=NULL);

  //! Calling of callback function for each item in the hash table.
  /*!
    Using this method you cannot change neither class Type members nor this class MapStringToClass members.
    \param Func Callback function to call.
    \param context Parameters data to pass to the callback function.
  */
  void ForEach(void (*Func)(const Type &, const MapStringToClass *, void *), void *context=NULL) const;
  //! According to specified key this method will return desired item.
  /*!
    Using this method you cannot change retrieved item.
    \param key The key according to find the item.
    \return Desired item.
  */
  const Type &Get(KeyType key) const;
  //! According to specified key this method will return desired item.
  /*!
    Using this method you can change retrieved item.
    \param key The key according to find the item.
    \return Desired item.
  */
  Type &Set(KeyType key);
  //! Using this operator you can access desired item directly by name.
  Type &operator [] (KeyType key) {return Set(key);}
  //! Using this operator you can access desired item directly by name.
  const Type &operator [] (KeyType key) const {return Get(key);}
  //! Adding of specified item into hash table.
  /*!
    Item with equal key will be replaced. In case there are too items on the corresponding position,
    then it will resize entire table.
    \param value Value to add.
    \return Index of the container the item have been added to or -1 in case some error occurred.
  */
  int Add(const Type &value);
  //! Removing of specified item from the hash table.
  /*!
    In case there is no item with such key in the hash table, nothing will happen. Note that size
    of the hash table (number of containers) will not be changed.
    @param key Key which specifies the item to remove.
    @return false when item was not found
  */
  bool Remove(KeyType key);
  //! Comparing of specified value to a NULL value.
  /*!
    \param value Value to be compared.
    \return True in case value is NULL, false otherwise.
  */
  static bool IsNull(const Type &value) {return &value == &_null;}
  //! Comparing of specified value to a NULL value.
  /*!
    \param value Value to be compared.
    \return False in case value is NULL, true otherwise.
  */
  static bool NotNull(const Type &value) {return &value != &_null;}
  //! Retrieving of null value.
  /*!
    \return Instance of type Type from heap which is considered as a NULL.
  */
  static Type &Null() {return _null;}

public:
// Direct access - used only for serialization
  //! Retrieves number of items in the hash table.
  /*!
    \return Number of items in the hash table.
  */
  int NItems() const {return _count;}
  //! Retrieves number of containers.
  /*!
    \return Number of containers.
  */
  int NTables() const {return _hashTable ? _tableSize : 0;}
  //! Retrieves desired container.
  /*!
    \param i Index of the container to return.
    \return Desired container.
  */
  Container &GetTable(int i) const {return _hashTable[i];}

// Implementation
protected:
  //! This method will hash the key into integer number from range <0 - n>
  /*!
    Note that in case n <= 0 it will return number from range <0 - _tableSize>
    \param key String to extract the key from.
    \param n Upper bound of the range the desired value to be within.
    \return Index of the desired container.
  */
  int HashKey(KeyType key, int n = 0) const;

  /// Allocate the table using allocator, construct it using construct traits
  Container *CreateTable(int n)
  {
    if (n == 0) return NULL;
    
    // allocate
    size_t size = n * sizeof(Container);
#if ALLOC_DEBUGGER && defined _CPPRTTI
    Container *ret = (Container *)Allocator::Alloc(size, __FILE__, __LINE__, typeid(Container).name());
#else
    Container *ret = (Container *)Allocator::Alloc(size);
#endif

    // construct
    CTraits::ConstructArray(ret, n);
    return ret;
  }
  /// Destruct the table using construct traits, free using allocator
  void DestroyTable(Container *mem, int n)
  {
    // destruct
    CTraits::DestructArray(mem, n);
    // free
    int size = n * sizeof(Container);
    Allocator::Free(mem, size);
  }
#ifdef DEBUG_HASH_STATISTIC // WIP:HASHING
  /// Show some statistics
  void DebugShowStatistic(const char* msg="")
  {
    static char buffer[256];
    char* ptr = buffer;
    int buckets = NTables();
    if (_numAdds > 0)
    {
      if (_count == _numAdds)
        ptr += sprintf(ptr, "Hash:0x%p with %d buckets and %d elements:", this, buckets, _count);
      else
        ptr += sprintf(ptr, "Hash:0x%p with %d buckets and %d elements (but %d adds):", this, buckets, _count, _numAdds);
      if (*msg)
        ptr += sprintf(ptr, " [%s]", msg);
      LogF(buffer);
      ptr = buffer;
    }
    else
      LogF("Hash:0x%p destructed after no add operations!", this);
    if (_count > 0)
    {
      int n = _hashTable[0].Size();
      int min = n;
      int max = n;
      double avg = (double)_count / _tableSize;
      double delta = 0.0;
      int count = 0;
      for (int i = 0; i < _tableSize; i++)
      {
        n = _hashTable[i].Size();
        ptr += sprintf(ptr, " %2d", n);
        if ((++count % 32) == 0)
        {
          LogF(buffer);
          ptr = buffer;
        }
        if (n < min)
          min = n;
        else if (n > max)
          max = n;
        double diff = n - avg;
        delta += diff * diff;
      }
      if (ptr != buffer)
        LogF(buffer);
      LogF(" min: %d, max: %d, avg: %.2f, delta %.2f", min, max, avg, delta);
    }
  }
#endif

  ClassIsMovableZeroed(MapStringToClass);
};

template<class Type, class Container, class Traits, class Allocator>
Type MapStringToClass<Type, Container, Traits, Allocator>::_null;

template<class Type, class Container, class Traits, class Allocator>
MapStringToClass<Type, Container, Traits, Allocator>::MapStringToClass(const MapStringToClass &src)
{
  _count = src._count;
  _tableSize = src._tableSize;
  if (src._hashTable)
  {
    _hashTable = CreateTable(_tableSize);
    for (int i=0; i<_tableSize; i++) _hashTable[i] = src._hashTable[i];
  }
  else _hashTable = NULL;
#ifdef DEBUG_HASH_STATISTIC // WIP:HASHING
  _numAdds = src._numAdds;
  LogF("Hash:0x%p created by copy constructor from hash:0x%p with %d buckets", this, &src, _tableSize);
#endif
}

template<class Type, class Container, class Traits, class Allocator>
void MapStringToClass<Type, Container, Traits, Allocator>::operator = (const MapStringToClass &src)
{
  Clear();
  _count = src._count;
#ifdef DEBUG_HASH_STATISTIC // WIP:HASHING
  _numAdds = src._numAdds;
#endif
  _tableSize = src._tableSize;
  if (src._hashTable)
  {
    _hashTable = CreateTable(_tableSize);
    for (int i=0; i<_tableSize; i++) _hashTable[i] = src._hashTable[i];
  }
  else _hashTable = NULL;
}

template<class Type, class Container, class Traits, class Allocator>
int MapStringToClass<Type, Container, Traits, Allocator>::HashKey(KeyType key, int n) const
{
  if (n <= 0) n = _tableSize;
  unsigned int nHash = Traits::CalculateHashValue(key);
  // well designed hash function should be different
  // for strings of any length that start with different letters
  // this forces nMash multiplier not equal to power of 2
  // in this case actual nHash multiplier is 33, which is OK
  return nHash % n;
}

template<class Type, class Container, class Traits, class Allocator>
void MapStringToClass<Type, Container, Traits, Allocator>::Init(int tableSize)
{
  Clear();
  _tableSize = tableSize;
}

/**
@param compactTables when false, the table count is retained, meaning items are removed, but the hash map
 is still dimensioned based on the previous item count
*/
template<class Type, class Container, class Traits, class Allocator>
void MapStringToClass<Type, Container, Traits, Allocator>::Clear(bool compactTables)
{
#ifdef DEBUG_HASH_STATISTIC // WIP:HASHING
  if (_hashTable && (_count > 0))
      DebugShowStatistic("clear");
  _numAdds = 0;
#endif
  if (_hashTable)
  {
    DestroyTable(_hashTable, _tableSize);
    _hashTable = NULL;
  }
  _count = 0;
  if (compactTables) _tableSize = DefTableSize;
}

template<class Type, class Container, class Traits, class Allocator>
void MapStringToClass<Type, Container, Traits, Allocator>::Rebuild(int tableSize)
{
//LogF("Rebuilding hash table: %d items, new size %d", _count, tableSize);
  // In case hash table is empty
  if (!_hashTable)
  {
    _tableSize = tableSize;
    return;
  }
  Container *newTable = CreateTable(tableSize);
  // For all old containers
  for (int i=0; i<_tableSize; i++)
  {
    const Container &container = _hashTable[i];
    // For all items in container
    for (int j=0; j<container.Size(); j++)
    {
      // Get single item from container
      const Type &item = container[j];
      // Get new position of the item
      int nHashKey = HashKey(Traits::GetKey(item), tableSize);
      // Get reference to the destination container
      Container &dest=newTable[nHashKey];
      // In case there is not enough space in the destination, then resize it (double the current size)
      int need=dest.Size()+1;
      int haveSize=dest.MaxSize();
      if( need>haveSize )
      {
        if( haveSize<1 ) haveSize=1;
        while( need>haveSize ) haveSize=haveSize*2;
        dest.Realloc(haveSize);
      }
      // Add the item itself
      dest.Add(item);
    }
  }
  // Delete the previous hash table and assign the new one
  DestroyTable(_hashTable, _tableSize);
  _hashTable = newTable;
  _tableSize = tableSize;
}


template<class Type, class Container, class Traits, class Allocator>
void MapStringToClass<Type, Container, Traits, Allocator>::ForEach
(
  void (*Func)(Type &, MapStringToClass *, void *), void *context
)
{
  if( !_hashTable ) return;
RestartI:
  int n = _tableSize;
  for (int i=0; i<n; i++)
  {
    Container &container = _hashTable[i];
RestartJ:
    int m = container.Size();
    for (int j=0; j<m; j++)
    {
      Type &item = container[j];
      Func(item, this, context);
      if (_tableSize != n) goto RestartI;
      if (container.Size() != m) goto RestartJ;
    }
  }
}

template<class Type, class Container, class Traits, class Allocator>
void MapStringToClass<Type, Container, Traits, Allocator>::ForEach
(
  void (*Func)(const Type &, const MapStringToClass *, void *), void *context
) const
{
  if( !_hashTable ) return;
  int n = _tableSize;
  for (int i=0; i<n; i++)
  {
    const Container &container = _hashTable[i];
    int m = container.Size();
    for (int j=0; j<m; j++)
    {
      const Type &item = container[j];
      Func(item, this, context);
    }
  }
}

#if !defined _MSC_VER || _MSC_VER>=1300
template<class Type, class Container, class Traits, class Allocator>
template <class Func>
bool MapStringToClass<Type, Container, Traits, Allocator>::ForEachF(Func func)
{
RestartI:
  if( !_hashTable ) return false;
  int n = _tableSize;
  for (int i=0; i<n; i++)
  {
RestartJ:
    Container &container = _hashTable[i];
    int m = container.Size();
    for (int j=0; j<m; j++)
    {
      Type &item = container[j];
      if (func(item, this)) return true;
      if( !_hashTable ) return false; // all items may have been deleted - container becoming invalid by this
      if (_tableSize != n) goto RestartI;
      if (container.Size() != m)
      {
        goto RestartJ;
    }
  }
  }
  return false;
}

template<class Type, class Container, class Traits, class Allocator>
template <class Func>
bool MapStringToClass<Type, Container, Traits, Allocator>::ForEachF(Func func) const
{
  if( !_hashTable ) return false;
  int n = _tableSize;
  for (int i=0; i<n; i++)
  {
    const Container &container = _hashTable[i];
    int m = container.Size();
    for (int j=0; j<m; j++)
    {
      const Type &item = container[j];
      if (func(item, this)) return true;
    }
  }
  return false;
}
#endif

template<class Type, class Container, class Traits, class Allocator>
const Type &MapStringToClass<Type, Container, Traits, Allocator>::Get(KeyType key) const
{
  if (_count <= 0) return Null();
  Assert(_hashTable);

  int nHashKey = HashKey(key);
  for (int i=0; i<_hashTable[nHashKey].Size(); i++)
  {
    const Type &item = _hashTable[nHashKey][i];
    if (Traits::CmpKey(Traits::GetKey(item), key) == 0)
      return item;
  }
  return Null();
}

template<class Type, class Container, class Traits, class Allocator>
Type &MapStringToClass<Type, Container, Traits, Allocator>::Set(KeyType key)
{
  if (_count <= 0) return Null();
  Assert(_hashTable);

  int nHashKey = HashKey(key);
  for (int i=0; i<_hashTable[nHashKey].Size(); i++)
  {
    Type &item = _hashTable[nHashKey][i];
    if (Traits::CmpKey(Traits::GetKey(item), key) == 0)
      return item;
  }
  return Null();
}

template<class Type, class Container, class Traits, class Allocator>
void MapStringToClass<Type, Container, Traits, Allocator>::Optimize()
{
  // inverse calculation to what was done in Add
  int tableSize = roundTo2PowerNCeil(toIntCeil(_count/Traits::CoefExpand()+1))-1;
  if (tableSize<DefTableSize) tableSize = DefTableSize;
  
  // we want only to reduce here
  if (_tableSize>tableSize)
  {
    Rebuild(tableSize);
  }
}

template<class Type, class Container, class Traits, class Allocator>
void MapStringToClass<Type, Container, Traits, Allocator>::CleanUp()
{
  if (_hashTable)
  {
    // give opportunity to clean "zombies" (like NULL Links)
    for (int i=0; i<_tableSize; i++)
    {
      _count -= Traits::CleanUp(_hashTable[i]);
    }
  }
}

template<class Type, class Container, class Traits, class Allocator>
int MapStringToClass<Type, Container, Traits, Allocator>::Add(const Type &value)
{
  // Retrieve the key of the item
  KeyType key = Traits::GetKey(value);
  Type &old = Set(key);
  // In case there exists some value already, replace it
  if (!IsNull(old))
  {
    // replace old value
    old = value;
    return HashKey(key);
  }
  // Retrieve maximum of acceptable number of items in the hash table
  int maxCount = Traits::CoefExpand() * _tableSize;
  // In case we will have to resize the hash table
  if (_count + 1 > maxCount)
  {
    if (_hashTable)
    {
      // give opportunity to clean "zombies" (like NULL Links)
      for (int i=0; i<_tableSize; i++)
      {
        _count -= Traits::CleanUp(_hashTable[i]);
      }
    }
    if (_count + 1 > maxCount)
    {
      int tableSize = _tableSize + 1;
      do
      {
        tableSize *= 2;
        maxCount = Traits::CoefExpand() * (tableSize - 1);
      }
      while (_count + 1 > maxCount);
      Rebuild(tableSize - 1);
    }
  }
  // In case the hash table is empty then create array of containers
  if (!_hashTable)
  {
    Assert(_tableSize > 0);
    if (_tableSize <= 0)
      return -1;
    _hashTable = CreateTable(_tableSize);
  }
  // Retrieve number of desired container from the key
  int nHashKey = HashKey(key);
  // Get desired container
  Container &dest=_hashTable[nHashKey];
  // In case there is not enough space in the destination, then resize it (double the current size)
  int need=dest.Size()+1;
  int haveSize=dest.MaxSize();
  if( need>haveSize )
  {
    if( haveSize<1 ) haveSize=1;
    while( need>haveSize ) haveSize=haveSize*2;
    dest.Realloc(haveSize);
  }
  // Add the item Eventually
  dest.Add(value);
  // Increase number of items and return the index of the changed container
  _count++;
#ifdef DEBUG_HASH_STATISTIC // WIP:HASHING
  _numAdds++;
#endif
  return nHashKey;
}

template<class Type, class Container, class Traits, class Allocator>
void MapStringToClass<Type, Container, Traits, Allocator>::Reserve(int nElem)
{
  //_tableSize =0;
  int maxCount = Traits::CoefExpand() * _tableSize;
  // In case we will have to resize the hash table
  if (nElem > maxCount)
  {
    int tableSize = _tableSize + 1;
    while (nElem > maxCount)
    {
      tableSize *= 2;
      maxCount = Traits::CoefExpand() * (tableSize - 1);
    }
    Rebuild(tableSize - 1);
  }
}

template<class Type, class Container, class Traits, class Allocator>
void MapStringToClass<Type, Container, Traits, Allocator>::Compact()
{
  if (!_hashTable) return;
  for (int i=0; i<_tableSize; i++)
    _hashTable[i].Compact();
}

template<class Type, class Container, class Traits, class Allocator>
bool MapStringToClass<Type, Container, Traits, Allocator>::Remove(KeyType key)
{
  if (_count <= 0) return false;
  Assert(_hashTable);

  int nHashKey = HashKey(key);
  for (int i=0; i<_hashTable[nHashKey].Size(); i++)
  {
    Type &item = _hashTable[nHashKey][i];
    if (Traits::CmpKey(Traits::GetKey(item), key) == 0)
    {
#ifdef DEBUG_HASH_STATISTIC // WIP:HASHING
      if (_numAdds == _count)
        DebugShowStatistic("first removal");
#endif
      _hashTable[nHashKey].Delete(i);
      _count--;
      Assert(_count >= 0);
      if (_count == 0)
        Clear();
      return true;
    }
  }
  return false;
}

#endif
