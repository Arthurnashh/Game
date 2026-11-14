#ifdef _MSC_VER
#  pragma once
#endif

/**
@file   maps.hpp
@brief  General hash-map templates.

Copyright (C) 2001-2003 by BIStudio (www.bistudio.com)
@author PE
@since 21.11.2001
@date  5.10.2003
*/

#ifndef _MAPS_H
#define _MAPS_H

#include "Es/Common/global.hpp"
#include "Es/Threads/pocritical.hpp"
#include "Es/Types/scopeLock.hpp"

#if _DEBUG
//#define MAP_STAT 1
#endif
//------------------------------------------------------------
//  support definitions:

/// State variable type for map iterators.
typedef unsigned IteratorState;

/// Empty value of <code>IteratorState</code> (used to finish a pass).
const IteratorState ITERATOR_NULL = 0xffffffff;

/// Default maximal load-factor (global constant).
const float DEFAULT_LOAD_FACTOR = 0.5f;

/// Minimum number of removed items which is not compressed.
const unsigned REMOVED_LOW_MARK = 6;

/// Super-type for all integer numerical types.
typedef int64 SuperInt;

//------------------------------------------------------------
//  hash-array size:

/**
Determines the new hash-array size.
Selects the first suitable prime number from a pre-defined set.
@param  s Minimal hash-array size.
@return Hash-array size to be used.
*/
extern unsigned hashSize ( unsigned s );

//------------------------------------------------------------
//  trait types:

/**
Traits for implicit maps.
Contains two special TargetType instances: null and zombie.
*/
template < typename TargetType >
struct ImplicitMapTraits {
  static TargetType null;                 ///< Empty value (can be passed as return value).
  static TargetType zombie;               ///< Removed value (internal usage only).
};

/// Empty value, also used as return value.
//template < typename TargetType >
//TargetType ImplicitMapTraits<TargetType>::null;

/// For removed values (internal).
//template < typename TargetType >
//TargetType ImplicitMapTraits<TargetType>::zombie;

//template < typename KeyType >
//KeyType identityKey ( KeyType &id )
//{
//  return id;
//}

/**
Traits for explicit maps.
Contains three special TargetType and KeyType instances: null, keyNull and zombie.
*/
template < typename KeyType, typename TargetType >
struct ExplicitMapTraits {
  static TargetType null;                   ///< Empty target value (can be passed as return value).
  static KeyType keyNull;                   ///< Empty key value (internal usage only).
  static KeyType zombie;                    ///< Removed value (internal usage only).
  /// Hash function KeyType -> unsigned.
  static unsigned getHash ( KeyType &key )
  { return (unsigned)key; }
};

/// Empty target value, also used as return value.
template < typename KeyType, typename TargetType >
TargetType ExplicitMapTraits<KeyType,TargetType>::null;

/// Empty key value (internal).
template < typename KeyType, typename TargetType >
KeyType ExplicitMapTraits<KeyType,TargetType>::keyNull;

/// For removed values (internal).
template < typename KeyType, typename TargetType >
KeyType ExplicitMapTraits<KeyType,TargetType>::zombie;

#if _MSC_VER>=1310
// .NET 2003 (VC 7.1) supports partial class template specialization
//#define USE_PART_SPEC 1
#define USE_PART_SPEC 0
#else
#define USE_PART_SPEC 0
#endif

#if USE_PART_SPEC

template <typename TargetType>
struct ExplicitMapTraits<int,TargetType> {
  static TargetType null;                   ///< Empty target value (can be passed as return value).
  static int keyNull;                       ///< Empty key value (internal usage only).
  static int zombie;                        ///< Removed value (internal usage only).
  /// Hash function KeyType -> unsigned.
  static unsigned getHash ( int &key )
  { return (unsigned)key; }
};

template < typename TargetType >
TargetType ExplicitMapTraits<int,TargetType>::null;

template < typename TargetType >
int ExplicitMapTraits<int,TargetType>::keyNull = -1;

template < typename TargetType >
int ExplicitMapTraits<int,TargetType>::zombie  = -2;

#endif

//------------------------------------------------------------
//  hash-map statistics:

#ifdef MAP_STAT

#  define IncResize     statResize++
#  define IncGet        statGet++
#  define IncPut        statPut++
#  define IncReplace    statReplace++
#  define IncRemove     statRemove++
#  define IncZombie     statZombie++
#  define IncCollision  statCollision++

#else

#  define IncResize
#  define IncGet
#  define IncPut
#  define IncReplace
#  define IncRemove
#  define IncZombie
#  define IncCollision

#endif

//------------------------------------------------------------
//  ImplicitMap: numeric -> object instance:

/**
@class  ImplicitMap
Template for implicit maps.
Maps from any numeric type (<code>KeyType</code>) to the given <code>TargetType</code>.
<p>Key value must be implicitly defined by the given <code>implicitKey()</code> routine.
<p><code>operator new[]</code>, <code>operator delete[]</code>, <code>operator=</code> and
<code>operator==</code> must be implemented correctly for <code>TargetType</code>.
<code>operator==</code> must be implemented on <code>KeyType</code>.
@param  KeyType Type used for hash-keys (indices).
<code>int operator==(const KeyType&,const KeyType&)</code> must be defined.
@param  TargetType Type which is stored internally in one big <b>array</b>. The <code>TargetValue</code>
instance has to <b>implicitly</b> contain the apropriate key (<code>KeyType</code>)
value. Following operators must be defined:
<code>TargetType& TargetType::operator= (const TargetType&)</code>,
<code>void* TargetType::operator new[](size_t)</code>,
<code>void TargetType::operator delete[](void*)</code>,
<code>int operator==(const TargetType&,const TargetType&)</code>.
@param  implicitKey Function that 'diggs-out' a key value from the <code>TargetType</code> instance.
@param  Traits Container for two specail values: <code>null</code> and <code>zombie</code>.
@see    ExplicitMap
*/
template < typename KeyType, typename TargetType, KeyType implicitKey(const TargetType &), bool mtSafe =false, typename Allocator =MemAllocD >
class ImplicitMap {

protected:

  /// Implicit trait type: holding null and zombie only..
  typedef ImplicitMapTraits<TargetType> Traits;

  typedef ConstructTraits<TargetType> CTraits;
  typedef ConstructTraits<KeyType> KCTraits;

  /// Cardinality of the map.
  unsigned used;

  /// Allocated array size (in number of TargetType items).
  unsigned allocated;

  /// Number of removed values.
  unsigned removed;

  /** Actual load factor.
  Reasonable values: <code>0.25</code> to <code>0.8</code>.
  */
  float loadFactor;

  /// Overload boundary.
  unsigned overLoad;

  /// Hash-table itself.
  AutoArray<TargetType,Allocator> table;

  /// Critical section to guard this map.
  mutable PoCriticalSection lock;

  /**
  Resizes/reorganizes the map-table.
  Makes room for at least <code>newSize</code> pairs.
  <p>MT-unsafe (caller must assure that resize() is called inside enter() and leave())!
  @param  newSize Minimal number of pairs the map will hold.
  */
  void resize ( unsigned newSize )
  {
    if ( newSize < used ) return;       // newSize == used => reorganize the table
    IncResize;
#ifdef NET_LOG_MAPS
    NetLog("ImplicitMap::resize: newSize=%u(x %u), used=%u, removed=%u, allocated=%u, overload=%u",
      newSize,sizeof(TargetType),used,removed,allocated,overLoad);
#endif
    unsigned newAllocated = hashSize((unsigned)(newSize/loadFactor+1.0f));
    AutoArray<TargetType,Allocator> newTable(newAllocated);
    unsigned i, j;
    for ( i = 0; i++ < newAllocated; ) newTable.AddFast(Traits::null);
    for ( j = 0; j < allocated; j++ )
      if ( table[j] != Traits::null && table[j] != Traits::zombie ) {
        IncPut;
        i = (unsigned)(implicitKey(table[j]) % newAllocated);
        while ( newTable[i] != Traits::null ) {
          IncCollision;
          if ( ++i >= newAllocated ) i = 0;
        }
        CTraits::Destruct(newTable[i]);
        CTraits::MoveData(&newTable[i],&table[j],1);
      }
      else
        CTraits::Destruct(table[j]);
    if ( table.Size() ) {
#ifdef NET_LOG_ZOMBIE
      int zombies = 0;
      for ( i = 0; i < allocated; )
        if ( table[i++] == Traits::zombie ) zombies++;
      NetLog("ImplicitMap::resize: destroying zombies: pred count=%d, number=%d",
        Traits::zombie->RefCounter(),zombies);
#endif
    }
    table.UnsafeResize(0);              // all instances were moved to newTable
    table.Move(newTable);
    allocated = newAllocated;
    overLoad = (unsigned)(allocated * loadFactor) + 1;
    removed = 0;
  }

public:
  /// Enters critical code section
  inline void Lock() const
  {
    if (mtSafe) lock.enter();
  }

  /// Leaves critical code section
  inline void Unlock() const
  {
    if (mtSafe) lock.leave();
  }

  /**
  External access to special null value.
  */
  static inline const TargetType &null ()
  { return Traits::null; }

#ifdef MAP_STAT
  /// Number of <code>resize()</code> calls.
  unsigned statResize;
  /// Number of <code>get()</code> operations.
  unsigned statGet;
  /// Number of <code>put()</code> operations.
  unsigned statPut;
  /// Number of <code>replace()</code> operations.
  unsigned statReplace;
  /// Number of <code>remove()</code> operations.
  unsigned statRemove;
  /// Number of zobies used in <code>remove()</code> operations.
  unsigned statZombie;
  /// Number of collisions encoutered in all operations.
  unsigned statCollision;
#endif

  /**
  Initializes the map.
  Optionally pre-allocates room for <code>initSize</code> items.
  @param  initSize Minimal number of pairs the map will hold.
  @param  initLoadFactor Load factor (ratio of occupied items in hash-array). Should be between
  <code>0.25</code> (very fast but space-wasting table) and <code>0.8</code>
  (slow but compact table).
  */
  ImplicitMap ( unsigned initSize =0, float initLoadFactor =DEFAULT_LOAD_FACTOR )
    : LockInit(lock,"ImplicitMap",mtSafe)
  {
    Lock();
    used = removed = allocated = overLoad = 0;
    loadFactor = initLoadFactor;
#ifdef MAP_STAT
    statResize = statGet = statPut = statReplace = statRemove = statZombie = statCollision = 0;
#endif
    if ( initSize > 0 ) resize(initSize);
    Unlock();
  }

  /**
  Looks for the given key.
  Returns an associated value (or <code>null</code>).
  @param  key Key value to be looked for.
  @param  result Assigns associated value or <code>null</code> if not found.
  @return <code>true</code> if found.
  */
  bool get ( KeyType key, TargetType &result ) const
  {
    ScopeLock<const ImplicitMap> sl(*this);
    IncGet;
    if ( used > 0 ) {
      unsigned i = (unsigned)(key % allocated);
      while ( table[i] != Traits::null ) {
        if ( table[i] != Traits::zombie && implicitKey(table[i]) == key ) {
          result = table[i];
          return true;
        }
        IncCollision;
        if ( ++i >= allocated ) i = 0;
      }
    }
    result = Traits::null;
    return false;
  }

  /**
  Looks for the given key.
  Returns pointer to the associated value (or <code>NULL</code>).
  @param  key Key value to be looked for.
  @return Pointer to associated value or <code>NULL</code> if not found.
  */
  TargetType *get ( KeyType key )
  {
    ScopeLock<const ImplicitMap> sl(*this);
    IncGet;
    if ( used > 0 ) {
      unsigned i = (unsigned)(key % allocated);
      while ( table[i] != Traits::null ) {
        if ( table[i] != Traits::zombie && implicitKey(table[i]) == key ) {
          return( &table[i] );
        }
        IncCollision;
        if ( ++i >= allocated ) i = 0;
      }
    }
    return NULL;
  }

  /**
  Looks for the given key.
  Returns pointer to the associated value (or <code>NULL</code>).
  @param  key Key value to be looked for.
  @return Pointer to associated value or <code>NULL</code> if not found.
  */
  const TargetType *get ( KeyType key ) const
  {
    ScopeLock<const ImplicitMap> sl(*this);
    IncGet;
    if ( used > 0 ) {
      unsigned i = (unsigned)(key % allocated);
      while ( table[i] != Traits::null ) {
        if ( table[i] != Traits::zombie && implicitKey(table[i]) == key ) {
          return( &table[i] );
        }
        IncCollision;
        if ( ++i >= allocated ) i = 0;
      }
    }
    return NULL;
  }

  /**
  Checks if the given key-value is already present in the map.
  @param  key Key to be looked for.
  @return <code>true</code> if the key is present.
  */
  bool presentKey ( KeyType key )
  {
    ScopeLock<const ImplicitMap> sl(*this);
    IncGet;
    if ( used > 0 ) {
      unsigned i = (unsigned)(key % allocated);
      while ( table[i] != Traits::null ) {
        if ( table[i] != Traits::zombie && implicitKey(table[i]) == key )
          return true;
        IncCollision;
        if ( ++i >= allocated ) i = 0;
      }
    }
    return false;
  }

  /**
  Checks if the given value is already present in the map.
  @param  value Value to be looked for.
  @return <code>true</code> if the value is present.
  */
  bool presentValue ( TargetType value )
  {
    if ( value == Traits::null ||
      value == Traits::zombie ) return false;
    return presentKey(implicitKey(value));
  }

  /// pre-allocated to be able to contain given number of elements with no more resizing
  /**
  @param expected number of elements
  */
  void rebuild(int size)
  {
    ScopeLock<const ImplicitMap> sl(*this);
    resize(size);
  }

  /**
  Puts a new [key,value] pair into the map.
  @param  value The inserted value (implicitly contains the <b>key</b>).
  @param  old To fill the old item associated with the same key (can be <code>NULL</code>).
  @return <code>true</code> if replaced.
  */
  bool put ( TargetType value, TargetType *old =NULL )
  {
    ScopeLock<const ImplicitMap> sl(*this);
    if ( value == Traits::null ||
      value == Traits::zombie ) {
        if ( old ) *old = Traits::null;
        return false;
    }
    IncPut;
    if ( !allocated ) resize(2);                // map would have been empty!
    KeyType key = implicitKey(value);
    unsigned i = (unsigned)(key % allocated);
    unsigned insert = allocated;
    while ( table[i] != Traits::null ) {
      if ( table[i] == Traits::zombie )       // I'll replace this zombie later
        insert = i;
      else
        if ( implicitKey(table[i]) == key ) {   // replace the old item
          IncReplace;
          if ( old ) *old = table[i];
          table[i] = value;
          return true;
        }
        IncCollision;
        if ( ++i >= allocated ) i = 0;
    }
    if ( insert < allocated ) {                 // replace the zombie item
      IncReplace;
      used++; removed--;
#ifdef NET_LOG_ZOMBIE
      NetLog("ImplicitMap::put: - replacing zombie: pred count=%d",
        Traits::zombie->RefCounter());
#endif
      table[insert] = value;
      if ( old ) *old = Traits::null;
      return false;
    }
    // insert as a new item:
    if ( used + removed + 1 >= overLoad ) {     // I must do resize/reorganization
      if ( removed )                          // prediction of future "remove()"s
        if ( removed < REMOVED_LOW_MARK )
          resize(used+REMOVED_LOW_MARK);
        else
          resize(used+(removed>>1)+1);
      else
        resize(used+1);
      i = (unsigned)(key % allocated);
      while ( table[i] != Traits::null ) {    // looking for an empty slot
        IncCollision;
        if ( ++i >= allocated ) i = 0;
      }
    }
    used++;
    table[i] = value;
    if ( old ) *old = Traits::null;
    return false;
  }

  /**
  Puts a new [key,value] pair into the map.
  @param  value The inserted value (implicitly contains the <b>key</b>).
  @param  old To fill the old item associated with the same key (can be <code>NULL</code>).
  @return <code>true</code> pointer to a value
  */
  TargetType *putAndGet ( TargetType value, TargetType *old =NULL )
  {
    ScopeLock<const ImplicitMap> sl(*this);
    if ( value == Traits::null ||
      value == Traits::zombie ) {
        if ( old ) *old = Traits::null;
        return NULL;
    }
    IncPut;
    if ( !allocated ) resize(2);                // map would have been empty!
    KeyType key = implicitKey(value);
    unsigned i = (unsigned)(key % allocated);
    unsigned insert = allocated;
    while ( table[i] != Traits::null ) {
      if ( table[i] == Traits::zombie )       // I'll replace this zombie later
        insert = i;
      else
        if ( implicitKey(table[i]) == key ) {   // replace the old item
          IncReplace;
          if ( old ) *old = table[i];
          table[i] = value;
          return &table[i];
        }
        IncCollision;
        if ( ++i >= allocated ) i = 0;
    }
    if ( insert < allocated ) {                 // replace the zombie item
      IncReplace;
      used++; removed--;
#ifdef NET_LOG_ZOMBIE
      NetLog("ImplicitMap::put: - replacing zombie: pred count=%d",
        Traits::zombie->RefCounter());
#endif
      table[insert] = value;
      if ( old ) *old = Traits::null;
      return &table[insert];
    }
    // insert as a new item:
    if ( used + removed + 1 >= overLoad ) {     // I must do resize/reorganization
      if ( removed )                          // prediction of future "remove()"s
        if ( removed < REMOVED_LOW_MARK )
          resize(used+REMOVED_LOW_MARK);
        else
          resize(used+(removed>>1)+1);
      else
        resize(used+1);
      i = (unsigned)(key % allocated);
      while ( table[i] != Traits::null ) {    // looking for an empty slot
        IncCollision;
        if ( ++i >= allocated ) i = 0;
      }
    }
    used++;
    table[i] = value;
    if ( old ) *old = Traits::null;
    return &table[i];
  }

  /**
  Removes the item associated with the given key.
  @param  key Key value to be removed.
  @param  old To fill the removed item (can be <code>NULL</code>).
  @return <code>true</code> if the item was removed.
  */
  bool removeKey ( KeyType key, TargetType *old =NULL )
  {
    ScopeLock<const ImplicitMap> sl(*this);
    IncRemove;
    if ( used == 0 ) {
      if ( old ) *old = Traits::null;
      return false;
    }
    unsigned i = (unsigned)(key % allocated);
    while ( table[i] != Traits::null ) {
      if ( table[i] != Traits::zombie && implicitKey(table[i]) == key ) {     // found!
        if ( old ) *old = table[i];
#ifdef NET_LOG_DESTRUCTOR
        NetLog("ImplicitMap::removeKey: - assigning zombie: pred count=%d",
          Traits::zombie->RefCounter());
#endif
        table[i] = Traits::zombie;
        used--; removed++;
        IncZombie;
        return true;
      }
      IncCollision;
      if ( ++i >= allocated ) i = 0;
    }
    if ( old ) *old = Traits::null;
    return false;
  }

  /**
  Removes the given item from the map.
  @param  value The item to be removed.
  @return <code>true</code> if the item was removed,
  <code>false</code> if it was't found in the map.
  */
  bool removeValue ( TargetType value )
  {
    if ( value == Traits::null ||
      value == Traits::zombie )
      return false;
    return removeKey(implicitKey(value));
  }

  /**
  Map cardinality.
  Actual number of [key,value] pairs.
  @return Total number of [key,value] pairs contained in the map.
  */
  inline unsigned card () const
  {
    return used;                        // mt-safe (used contains valid value all the time..)
  }

  /**
  Re-initializes the map.
  Removes all [key,value] pairs but desn't relocate the hash-array.
  */
  void reset ()
  {
    Lock();
    if ( used > 0 ) {
      unsigned i;
      used = removed = 0;
      for ( i = 0; i < allocated; ) table[i++] = Traits::null;
    }
    Unlock();
  }

  /**
  Iterator initialization.
  No assumption about associated items' order could be made!
  Can be used both for linear- and for cyclic- passes.
  <p>Cyclic pass: do <code>IteratorState origin = iterator</code> assignment after getFirst() call.
  Alternatively <code>origin = iterator</code> can be done in any time you want..
  @param  iterator Reference to an iterator's state (stored outside this class).
  @param  first Reference to the first associated value (or <code>null</code> if the map is empty).
  @return <code>true</code> if the map is non-empty.
  */
  const TargetType *getFirst(IteratorState &iterator) const
  {
    iterator = 0;
    if ( used ) return getNext(iterator);
    iterator = ITERATOR_NULL;
    return NULL;
  }

  /**
  Single iteration.
  No assumption about associated items' order could be made!
  <p>External iterator's state (<code>IteratorState</code>) had to be pre-defined
  by a <code>getFirst()</code> call.
  @param  iterator Reference to an iterator's state (stored outside this class).
  @param  next Reference to the next associated value (or <code>null</code>) if no more values exist in the map).
  @return <code>true</code> if next is not null.
  */
  const TargetType *getNext(IteratorState &iterator) const
  {
    ScopeLock<const ImplicitMap> sl(*this);
    while ( iterator < allocated ) {
      if ( table[iterator] != Traits::null &&
        table[iterator] != Traits::zombie ) {
          return &table[iterator++];
      }
      iterator++;
    }
    iterator = ITERATOR_NULL;
    return NULL;
  }

  void SetStorage(const Allocator &allocator)
  {
    table.SetStorage(allocator);
  }
  /**
  Iterator initialization.
  No assumption about associated items' order could be made!
  Can be used both for linear- and for cyclic- passes.
  <p>Cyclic pass: do <code>IteratorState origin = iterator</code> assignment after getFirst() call.
  Alternatively <code>origin = iterator</code> can be done in any time you want..
  @param  iterator Reference to an iterator's state (stored outside this class).
  @param  first Reference to the first associated value (or <code>null</code> if the map is empty).
  @return <code>true</code> if the map is non-empty.
  */
  bool getFirst(IteratorState &iterator, TargetType &first) const
  {
    iterator = 0;
    if ( used ) return getNext(iterator,first);
    iterator = ITERATOR_NULL;
    first = Traits::null;
    return false;
  }

  /**
  Single iteration.
  No assumption about associated items' order could be made!
  <p>External iterator's state (<code>IteratorState</code>) had to be pre-defined
  by a <code>getFirst()</code> call.
  @param  iterator Reference to an iterator's state (stored outside this class).
  @param  next Reference to the next associated value (or <code>null</code>) if no more values exist in the map).
  @return <code>true</code> if next is not null.
  */
  bool getNext(IteratorState &iterator, TargetType &next) const
  {
    ScopeLock<const ImplicitMap> sl(*this);
    while ( iterator < allocated ) {
      if ( table[iterator] != Traits::null &&
        table[iterator] != Traits::zombie ) {
          next = table[iterator++];
          return true;
      }
      iterator++;
    }
    iterator = ITERATOR_NULL;
    next = Traits::null;
    return false;
  }

  template <class Function>
  void forEach(const Function &function)
  {
    IteratorState it;
    TargetType item;
    if( getFirst(it,item) ) do
    {
      function(item);
    } while( getNext(it,item) );
  }
  template <class Function>
  void forEach(const Function &function) const
  {
    IteratorState it;
    TargetType item;
    if( getFirst(it,item) ) do
    {
      function(item);
    } while( getNext(it,item) );
  }

  // TODO: implement const iterator variant
  struct Iterator 
  {
    IteratorState it;
    const TargetType *value;
    Iterator():value(NULL){}
    operator bool () const {return value!=NULL;}
    TargetType &operator *() const {return *unconst_cast(value);}
  };

  Iterator begin() const
  {
    Iterator it;
    it.value = getFirst(it.it);
    return it;
  }
  void next(Iterator &it) const
  {
    it.value = getNext(it.it);
  }
  /**
  Cyclic iterator initialization.
  No assumption about associated items' order could be made!
  <p>Pass origin should be kept externally (and passed as <code>origin</code> parameter).
  @param  iterator Reference to an iterator's state (stored outside this class).
  @param  origin Origin of the iteration (after 1st item passed in this pass).
  @param  first Reference to the next associated value (or <code>null</code>) if no more values exist in the map).
  @return <code>true</code> if the map is nonempty.
  */
  bool getFirstCyclic ( IteratorState &iterator, IteratorState &origin, TargetType &first )
  {
    ScopeLock<const ImplicitMap> sl(*this);
    if ( !used ) {
      first = Traits::null;
      return false;
    }
    if ( origin > allocated ) origin = allocated;
    iterator = origin;
    if ( iterator < allocated &&
      table[iterator] != Traits::null &&
      table[iterator] != Traits::zombie ) {
        first = table[iterator++];
        return true;
    }
    iterator++;
    return getNextCyclic(iterator,origin,first);
  }

  /**
  Single iteration for cyclic map pass.
  No assumption about associated items' order could be made!
  <p>External iterator's state (<code>IteratorState</code>) had to be pre-defined
  by a <code>getFirstCyclic()</code> call.
  <p>Pass origin should be kept externally (and passed as <code>origin</code> parameter).
  <p>Almost thread-safe
  @param  iterator Reference to an iterator's state (stored outside this class).
  @param  origin Origin of the iteration (after 1st item passed in this pass).
  @param  next Reference to the next associated value (or <code>null</code>) if no more values exist in the map).
  @return <code>true</code> if next is not null.
  */
  bool getNextCyclic ( IteratorState &iterator, IteratorState &origin, TargetType &next )
  {
    ScopeLock<const ImplicitMap> sl(*this);
    if ( !used || iterator == origin ) {
      next = Traits::null;
      return false;
    }
    if ( origin > allocated ) origin = allocated;
    if ( iterator > origin ) {          // pass to the end of the array
      while ( iterator < allocated ) {
        if ( table[iterator] != Traits::null &&
          table[iterator] != Traits::zombie ) {
            next = table[iterator++];
            return true;
        }
        iterator++;
      }
      iterator = 0;
    }
    // pass from the beginning to the origin
    while ( iterator < origin ) {
      if ( table[iterator] != Traits::null &&
        table[iterator] != Traits::zombie ) {
          next = table[iterator++];
          return true;
      }
      iterator++;
    }
    next = Traits::null;
    return false;
  }

  /**
  Destroys the map including all values associated in it.
  */
  ~ImplicitMap ()
  {
    reset();
    Lock();
    table.Clear();
    allocated = overLoad = 0;               // to be sure
    Unlock();
  }

};


//------------------------------------------------------------
//  ImplicitSet: set of object instances:

extern unsigned intToUnsigned (const int &id);

template < typename TargetType, unsigned implicitKey(const TargetType &), bool mtSafe =false, typename Allocator =MemAllocD >
class ImplicitSet : public ImplicitMap<unsigned,TargetType,implicitKey,mtSafe,Allocator> 
{
  typedef ImplicitMap<unsigned,TargetType,implicitKey,mtSafe,Allocator> base;

public:

  ImplicitSet ( unsigned initSize =0, float initLoadFactor =DEFAULT_LOAD_FACTOR )
    : ImplicitMap<unsigned,TargetType,implicitKey,mtSafe,Allocator>(initSize,initLoadFactor)
  {
  }

  TargetType *get ( TargetType value )
  {
    ScopeLock<const ImplicitMap<unsigned,TargetType,implicitKey,mtSafe,Allocator> > sl(*this);
    IncGet;
    if ( base::used > 0 ) {
      unsigned i = implicitKey(value) % base::allocated;
      while ( base::table[i] != base::Traits::null ) {
        if ( base::table[i] == value ) return( &base::table[i] );
        IncCollision;
        if ( ++i >= base::allocated ) i = 0;
      }
    }
    return NULL;
  }

  bool present ( TargetType value )
  {
    ScopeLock<const ImplicitMap<unsigned,TargetType,implicitKey,mtSafe,Allocator> > sl(*this);
    IncGet;
    if ( base::used > 0 ) {
      unsigned i = implicitKey(value) % base::allocated;
      while ( base::table[i] != base::Traits::null ) {
        if ( base::table[i] == value ) return true;
        IncCollision;
        if ( ++i >= base::allocated ) i = 0;
      }
    }
    return false;
  }

  bool put ( TargetType value, TargetType *old =NULL )
  {
    ScopeLock<const ImplicitMap<unsigned,TargetType,implicitKey,mtSafe,Allocator> > sl(*this);
    if ( value == base::Traits::null ||
      value == base::Traits::zombie ) {
        if ( old ) *old = base::Traits::null;
        return false;
    }
    IncPut;
    if ( !base::allocated ) base::resize(2);            // map would have been empty!
    unsigned key = implicitKey(value);
    unsigned i = key % base::allocated;
    unsigned insert = base::allocated;
    while ( base::table[i] != base::Traits::null ) {
      if ( base::table[i] == base::Traits::zombie )     // I'll replace this zombie later
        insert = i;
      else
        if ( base::table[i] == value ) {          // replace the old item
          IncReplace;
          if ( old ) *old = base::table[i];
          base::table[i] = value;
          return true;
        }
        IncCollision;
        if ( ++i >= base::allocated ) i = 0;
    }
    if ( insert < base::allocated ) {             // replace the zombie item
      IncReplace;
      base::used++; base::removed--;
#ifdef NET_LOG_ZOMBIE
      NetLog("ImplicitSet::put: - replacing zombie: pred count=%d",
        base::Traits::zombie->RefCounter());
#endif
      base::table[insert] = value;
      if ( old ) *old = base::Traits::null;
      return false;
    }
    // insert as a new item:
    if ( base::used + base::removed + 1 >= base::overLoad ) { // I must do resize/reorganization
      if ( base::removed )                        // prediction of future "remove()"s
        if ( base::removed < REMOVED_LOW_MARK )
          this->resize(base::used+REMOVED_LOW_MARK);
        else
          this->resize(base::used+(base::removed>>1)+1);
      else
        this->resize(base::used+1);
      i = key % base::allocated;
      while ( base::table[i] != base::Traits::null ) {  // looking for an empty slot
        IncCollision;
        if ( ++i >= base::allocated ) i = 0;
      }
    }
    base::used++;
    base::table[i] = value;
    if ( old ) *old = base::Traits::null;
    return false;
  }

  bool remove ( TargetType value, TargetType *old =NULL )
  {
    ScopeLock<const ImplicitMap<unsigned,TargetType,implicitKey,mtSafe,Allocator> > sl(*this);
    IncRemove;
    if ( base::used == 0 ) {
      if ( old ) *old = base::Traits::null;
      return false;
    }
    unsigned i = implicitKey(value) % base::allocated;
    while ( base::table[i] != base::Traits::null ) {
      if ( base::table[i] == value ) {            // found!
        if ( old ) *old = base::table[i];
#ifdef NET_LOG_DESTRUCTOR
        NetLog("ImplicitSet::remove: - assigning zombie: pred count=%d",
          base::Traits::zombie->RefCounter());
#endif
        base::table[i] = base::Traits::zombie;
        base::used--; base::removed++;
        IncZombie;
        return true;
      }
      IncCollision;
      if ( ++i >= base::allocated ) i = 0;
    }
    if ( old ) *old = base::Traits::null;
    return false;
  }

};

//------------------------------------------------------------
//  ExplicitMap: key -> object instance:

/**
@class  ExplicitMap
Template for explicit maps.
Maps from any type (<code>KeyType</code>) to the given <code>TargetType</code>.
<p>Explicit key values are stored within the <code>ExplicitMap</code> instance.
<p><code>operator new[]</code>, <code>operator delete[]</code>, <code>operator=</code> and
<code>operator==</code> must be implemented correctly for both <code>TargetType</code> and
<code>KeyType</code> types.
@param  KeyType Type used for hash-keys (indices). Following operators must be defined:
<code>KeyType& KeyType::operator= (const KeyType&)</code>,
<code>void* KeyType::operator new[](size_t)</code>,
<code>void KeyType::operator delete[](void*)</code>,
<code>int operator==(const KeyType&,const KeyType&)</code>.
@param  TargetType Type which is stored internally in one big <b>array</b>.
Following operators must be defined:
<code>TargetType& TargetType::operator= (const TargetType&)</code>,
<code>void* TargetType::operator new[](size_t)</code>,
<code>void TargetType::operator delete[](void*)</code>,
<code>int operator==(const TargetType&,const TargetType&)</code>.
@param  getHash Computes hash-value from the given <code>KeyType</code> value.
@see    ImplicitMap
*/
template < typename KeyType, typename TargetType, bool mtSafe =false, typename Allocator =MemAllocD >
class ExplicitMap {

protected:

  /// Implicit trait type: holding null, keyNull and zombie only..
  typedef ExplicitMapTraits<KeyType,TargetType> Traits;

  typedef ConstructTraits<TargetType> CTraits;
  typedef ConstructTraits<KeyType> KCTraits;

  /// Cardinality of the map.
  unsigned used;

  /// Allocated array size.
  unsigned allocated;

  /// Number of removed values.
  unsigned removed;

  /**
  Actual load factor.
  Reasonable values: <code>0.25</code> to <code>0.8</code>.
  */
  float loadFactor;

  /// Overload boundary.
  unsigned overLoad;

  /// Hash-table itself
  AutoArray<TargetType,Allocator> table;

  /// Key-table array (the same size as <code>table</code>).
  AutoArray<KeyType,Allocator> keys;

  /// Critical section to guard this map.
  PoCriticalSection lock;

  /**
  Resizes/reorganizes the map-table.
  Makes room for at least <code>newSize</code> pairs.
  Must be called within enter() and leave()!
  @param  newSize Minimal number of pairs the map will hold.
  */
  void resize ( unsigned newSize )
  {
    if ( newSize < used ) return;       // newSize == used => reorganize the table
    IncResize;
#ifdef NET_LOG_MAPS
    NetLog("ExplicitMap::resize: newSize=%u(x %u+%u), used=%u, removed=%u, allocated=%u, overload=%u",
      newSize,sizeof(KeyType),sizeof(TargetType),used,removed,allocated,overLoad);
#endif
    unsigned newAllocated = hashSize((unsigned)(newSize/loadFactor+1.0f));
    AutoArray<KeyType,Allocator> newKeys(newAllocated);
    AutoArray<TargetType,Allocator> newTable(newAllocated);
    unsigned i, j;
    for ( i = 0; i++ < newAllocated; ) {
      newKeys.AddFast(Traits::keyNull);
      newTable.AddFast(Traits::null);
    }
    for ( j = 0; j < allocated; j++ )
      if ( keys[j] != Traits::keyNull &&
        keys[j] != Traits::zombie ) {
          IncPut;
          i = Traits::getHash(keys[j]) % newAllocated;
          while ( newKeys[i] != Traits::keyNull ) {
            IncCollision;
            if ( ++i >= newAllocated ) i = 0;
          }
          KCTraits::Destruct(newKeys[i]);
          KCTraits::MoveData(&newKeys[i],&keys[j],1);
          CTraits::Destruct(newTable[i]);
          CTraits::MoveData(&newTable[i],&table[j],1);
      }
      else {
        KCTraits::Destruct(keys[j]);
        CTraits::Destruct(table[j]);
      }
      keys.UnsafeResize(0);               // all instances were moved to newKeys
      keys.Move(newKeys);
      table.UnsafeResize(0);              // all instances were moved to newTable
      table.Move(newTable);
      allocated = newAllocated;
      overLoad = (unsigned)(allocated * loadFactor) + 1;
      removed = 0;
  }

public:
  /**
  Verify hash map is valid and correct.
  */
  bool checkIntegrity()
  {
    IteratorState it;
    TargetType value;
    KeyType key;
    if (getFirst(it,value,&key))
    {
      do
      {
        TargetType check;
        if (!get(key,check))
        {
          Fail("get failed");
          return false;
        }
      } while( getNext(it,value,&key) );
    }
    return true;
  }

  /// Enters critical code section
  inline void Lock() const
  {
    if ( mtSafe ) lock.enter();
  }

  /// Leaves critical code section
  inline void Unlock() const
  {
    if ( mtSafe ) lock.leave();
  }

  /**
  External access to special null value.
  */
  static inline const TargetType &null ()
  { return Traits::null; }

#ifdef MAP_STAT
  /// Number of <code>resize()</code> calls.
  unsigned statResize;
  /// Number of <code>get()</code> operations.
  unsigned statGet;
  /// Number of <code>put()</code> operations.
  unsigned statPut;
  /// Number of <code>replace()</code> operations.
  unsigned statReplace;
  /// Number of <code>remove()</code> operations.
  unsigned statRemove;
  /// Number of zobies used in <code>remove()</code> operations.
  unsigned statZombie;
  /// Number of collisions encoutered in all operations.
  unsigned statCollision;
#endif

  /**
  Initializes the map.
  Optionally pre-allocates room for <code>initSize</code> items.
  @param  initSize Minimal number of pairs the map will hold.
  @param  initLoadFactor Load factor (ratio of occupied items in hash-array). Should be between
  <code>0.25</code> (very fast but space-wasting table) and <code>0.8</code>
  (slow but compact table).
  */
  ExplicitMap ( unsigned initSize =0, float initLoadFactor =DEFAULT_LOAD_FACTOR )
    : LockInit(lock,"ExplicitMap",mtSafe)
  {
    Lock();
    used = removed = allocated = overLoad = 0;
    loadFactor = initLoadFactor;
#ifdef MAP_STAT
    statResize = statGet = statPut = statReplace = statRemove = statZombie = statCollision = 0;
#endif
    if ( initSize > 0 ) resize(initSize);
    Unlock();
  }

  /**
  Looks for the given key.
  Returns an associated value (or <code>null</code> if not found).
  @param  key Key value to be looked for.
  @param  result Associated value or <code>null</code> if not found.
  @return <code>true</code> if found.
  */
  bool get ( KeyType key, TargetType &result ) const
  {
    ScopeLock<const ExplicitMap> sl(*this);
    IncGet;
    if ( used == 0 ||
      key == Traits::keyNull ||
      key == Traits::zombie ) {
        result = Traits::null;
        return false;
    }
    unsigned i = Traits::getHash(key) % allocated;
    while ( keys[i] != Traits::keyNull ) {
      if ( keys[i] == key ) {
        result = table[i];
        return true;
      }
      IncCollision;
      if ( ++i >= allocated ) i = 0;
    }
    result = Traits::null;
    return false;
  }

  /**
  Looks for the given key.
  Returns pointer to an associated value (or <code>NULL</code> if not found).
  Potentially dangerous! The map should be locked while the pointer is used!
  @param  key Key value to be looked for.
  @return Pointer to associated value or <code>NULL</code> if not found.
  */
  TargetType *get ( KeyType key )
  {
    ScopeLock<const ExplicitMap> sl(*this);
    IncGet;
    if ( used == 0 ||
      key == Traits::keyNull ||
      key == Traits::zombie )
      return NULL;
    unsigned i = Traits::getHash(key) % allocated;
    while ( keys[i] != Traits::keyNull ) {
      if ( keys[i] == key )
        return &table[i];
      IncCollision;
      if ( ++i >= allocated ) i = 0;
    }
    return NULL;
  }

  /**
  Checks if the given key-value pair is already present in the map.
  @param  key Key value to be looked for.
  @return <code>true</code> if found.
  */
  bool presentKey ( KeyType key ) const
  {
    ScopeLock<const ExplicitMap> sl(*this);
    IncGet;
    if ( used == 0 ||
      key == Traits::keyNull ||
      key == Traits::zombie )
      return false;
    unsigned i = Traits::getHash(key) % allocated;
    while ( keys[i] != Traits::keyNull ) {
      if ( keys[i] == key )
        return true;
      IncCollision;
      if ( ++i >= allocated ) i = 0;
    }
    return false;
  }

  /// pre-allocated to be able to contain given number of elements with no more resizing
  /**
  @param expected number of elements
  */
  void rebuild(int size)
  {
    ScopeLock<const ExplicitMap> sl(*this);
    resize(size);
  }
  /**
  Puts a new [key,value] pair into the map.
  @param  key Key to be inserted.
  @param  value Value to be inserted.
  @param  old To fill the old item associated with the same key (can be <code>NULL</code>).
  @return <code>true</code> if replaced.
  */
  bool put ( KeyType key, TargetType value, TargetType *old =NULL )
  {
    if ( value == Traits::null ) return removeKey(key,old);
    if ( key == Traits::keyNull ||
      key == Traits::zombie ) {
        if ( old ) *old = Traits::null;
        return false;
    }
    ScopeLock<const ExplicitMap> sl(*this);
    IncPut;
    if ( !allocated ) resize(2);                // map would have been empty!
    unsigned i = Traits::getHash(key) % allocated;
    unsigned insert = allocated;
    while ( keys[i] != Traits::keyNull ) {
      if ( keys[i] == Traits::zombie )        // I'll replace this zombie later
        insert = i;
      else
        if ( keys[i] == key ) {                 // replace the old item
          IncReplace;
          if ( old ) *old = table[i];
          table[i] = value;
          return true;
        }
        IncCollision;
        if ( ++i >= allocated ) i = 0;
    }
    if ( insert < allocated ) {                 // replace the zombie item
      IncReplace;
      used++; removed--;
      keys[insert]  = key;
      table[insert] = value;
      if ( old ) *old = Traits::null;
      return false;
    }
    // insert as a new item:
    if ( used + removed + 1 >= overLoad ) {     // I must do resize/reorganization
      if ( removed )                          // prediction of future "remove()"s
        if ( removed < REMOVED_LOW_MARK )
          resize(used+REMOVED_LOW_MARK);
        else
          resize(used+(removed>>1)+1);
      else
        resize(used+1);
      i = Traits::getHash(key) % allocated;
      while ( keys[i] != Traits::keyNull ) {
        IncCollision;
        if ( ++i >= allocated ) i = 0;
      }
    }
    used++;
    keys[i]  = key;
    table[i] = value;
    if ( old ) *old = Traits::null;
    return false;
  }

  /**
  Removes the item associated with the given key.
  @param  key Key value to be removed.
  @param  old To fill the removed item (can be <code>NULL</code>).
  @return <code>true</code> if an item was removed.
  */
  bool removeKey ( KeyType key, TargetType *old =NULL )
  {
    if ( key == Traits::keyNull ||
      key == Traits::zombie ) {
        if ( old ) *old = Traits::null;
        return false;
    }
    ScopeLock<const ExplicitMap> sl(*this);
    IncRemove;
    if ( used == 0 ) {
      if ( old ) *old = Traits::null;
      return false;
    }
    unsigned i = Traits::getHash(key) % allocated;
    while ( keys[i] != Traits::keyNull ) {
      if ( keys[i] == key ) {     // found!
        if ( old ) *old = table[i];
        keys[i]  = Traits::zombie;
        table[i] = Traits::null;
        used--; removed++;
        IncZombie;
        return true;
      }
      IncCollision;
      if ( ++i >= allocated ) i = 0;
    }
    if ( old ) *old = Traits::null;
    return false;
  }

  /**
  Map cardinality.
  Number of [key,value] pairs.
  @return Total number of [key,value] pairs contained in the map.
  */
  inline unsigned card () const
  {
    return used;                        // MT-safe as "used" is always valid.
  }

  /**
  Re-initializes the map.
  Removes all [key,value] pairs but desn't relocate the hash-arrays.
  */
  void reset ()
  {
    Lock();
    if ( used > 0 ) {
      unsigned i;
      for ( i = 0; i < allocated; ) {
        keys[i]    = Traits::keyNull;
        table[i++] = Traits::null;
      }
      used = removed = 0;
    }
    Unlock();
  }

  /**
  Iterator initialization.
  No assumption about associated items' order could be made!
  @param  iterator Reference to an iterator's state (stored outside this class).
  @param  first The first associated value (or <code>null</code> if the map is empty).
  @param  key Pointer to the buffer which will receive the key (can be <code>NULL</code>).
  @return <code>true</code> if the map is nonempty.
  */
  bool getFirst ( IteratorState &iterator, TargetType &first, KeyType *key =NULL ) const
  {
    iterator = 0;
    return getNext(iterator,first,key);
  }

  /**
  Single iteration.
  No assumption about associated items' order could be made!
  <p>External iterator's state (<code>IteratorState</code>) had to be pre-defined
  by a <code>getFirst()</code> call.
  @param  iterator Reference to an iterator's state (stored outside this class).
  @param  next The next associated value (or <code>null</code> if not found).
  @param  key Pointer to the buffer which will receive the key (can be <code>NULL</code>).
  @return <code>true</code> if found.
  */
  bool getNext ( IteratorState &iterator, TargetType &next, KeyType *key =NULL ) const
  {
    ScopeLock<const ExplicitMap> sl(*this);
    while ( iterator < allocated ) {
      if ( keys[iterator] != Traits::keyNull &&
        keys[iterator] != Traits::zombie ) {
          if ( key ) *key = keys[iterator];
          next = table[iterator++];
          return true;
      }
      iterator++;
    }
    iterator = ITERATOR_NULL;
    if ( key ) *key = Traits::keyNull;
    next = Traits::null;
    return false;
  }

  // TODO: implement const iterator variant
  struct Iterator 
  {
    IteratorState it;
    const KeyType *key;
    const TargetType *value;
    Iterator() : key(NULL), value(NULL) {}
    operator bool () const {return value != NULL;}
    KeyType &GetKey() const {return *unconst_cast(key);}
    TargetType &GetValue() const {return *unconst_cast(value);}
  };

  Iterator begin() const
  {
    Iterator it;
    it.it = 0;
    next(it);
    return it;
/*
    Iterator it;
    if (!getFirst(it.it, *unconst_cast(it.value), unconst_cast(it.key)))
    {
      it.value = NULL; it.key = NULL;
    }
    return it;
*/
  }
  void next(Iterator &it) const
  {
/*
    if (!getNext(it.it, *unconst_cast(it.value), unconst_cast(it.key)))
    {
      it.value = NULL; it.key = NULL;
    }
*/
    ScopeLock<const ExplicitMap> sl(*this);
    while (it.it < allocated)
    {
      if (keys[it.it] != Traits::keyNull && keys[it.it] != Traits::zombie) 
      {
        it.key = &keys[it.it];
        it.value = &table[it.it++];
        return;
      }
      it.it++;
    }
    it.it = ITERATOR_NULL;
    it.key = NULL;
    it.value = NULL;
  }

  /**
  Destroys the map including all values associated in it.
  */
  ~ExplicitMap ()
  {
    reset();
    Lock();
    keys.Clear();
    table.Clear();
    allocated = overLoad = 0;               // to be sure
    Unlock();
  }

};

#endif
