#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MEM_FREE_REQ_HPP
#define _MEM_FREE_REQ_HPP

#include <Es/Containers/listBidir.hpp>
#include <Es/Strings/rString.hpp>

//! interface for on-demand deallocator (any class that is able to free memory on demand)
/*!
 this is used when memory allocation fails to gain any memory
 that may be used by variaos caches
 each MemHeap maintains list of objects that are able to free some memory in it
*/

class IMemoryFreeOnDemand: public TLinkBidirD
{
  public:
  //! try to free given amount of memory
  /*!
  \param amount amount of memory that needs to be freed
  \return amount of memory actually freed
  */
  virtual size_t Free(size_t amount) = 0;
  //! try to free as much memory as possible
  /*!
  \return amount of memory actually freed
  */
  virtual size_t FreeAll() = 0;
  //! estimate cost associated with recreating objects freed
  /*
  lowest priority objects are freed first.
  Static property: used when on-demand deallocator is registered.
  \return estimated number of CPU cycles required to recreate one byte of controled objects
  */
  virtual float Priority() const = 0;

  //! name for debugging purposes
  virtual RString GetDebugName() const = 0;
  //! how much memory is controlled by this manager
  virtual size_t MemoryControlled() const = 0;

  /// some allocations (MemoryStore) may use physical memory only
  virtual bool UsesVirtualMemory() const {return true;}
  /// how much memory is was recently used by this manager
  /**
  Memory that was recently used should not be released if possible,
  as it is very likely it will be needed soon again.
  */
  virtual size_t MemoryControlledRecent() const {return 0;}
  
  /// React to frame boundary
  /**
  Frames are intended to distinguish recently used memory (see MemoryControlledRecent).
  It does not matter if it is called regularly or not.
  If it is never called, all memory is considered recent.
  */
  virtual void MemoryControlledFrame() {}
  
  //! unregister this class from the list
  void Unregister()
  {
    if (IsInList())
    {
      Delete();
    }
  }
  //! virtual destructor
  virtual ~IMemoryFreeOnDemand(){}
};

typedef long long mem_size_t;

inline size_t MemSizeTToSizeT(mem_size_t size)
{
  if (size<0) return 0;
  if (size>(size_t)-1) return (size_t)-1;
  // note: surprisingly, (size_t)-1 seems to be more portable than (size_t)~0
  // both work on 2 complement architectures, though
  return size;
}

//! structure used to keep information about a single IMemoryFreeOnDemand during balancing

struct OnDemandStats
{
  IMemoryFreeOnDemand *mem;
  size_t controlled;
  bool releasePossible;
  bool system; //!< system or normal heap
};

TypeIsSimple(OnDemandStats)


//! list of objects that are able to free memory
/*!
this list is maintained sorted by priority (lowest first)
*/

class MemoryFreeOnDemandList: public TListBidir<IMemoryFreeOnDemand>
{
  public:
  void Register(IMemoryFreeOnDemand *object);
  size_t Free(size_t amount);
  size_t FreeAll();

  static mem_size_t BalanceList(
    mem_size_t amount, OnDemandStats *stats, int nStats, IMemoryFreeOnDemand **lowLevel, int nLowLevel,
    mem_size_t *system=NULL
  );
};

class MemoryFreeOnDemandHelper: public IMemoryFreeOnDemand
{
  // typical implementation frame
  //! constant returned when some memory is released, but we do not know how much
  enum {FreeAmmountUncertain=~0};

  public:
  //! free one item you (preferably the cheapest one)
  virtual size_t FreeOneItem() = 0;

  //! using FreeOneItem we implement some functions of IMemoryFreeOnDemand
  virtual size_t Free(size_t amount);
  virtual size_t FreeAll();

};

/// multiple inteface support
#define MemHandlerInstance(id) \
  class MHInstance##id: public MemoryFreeOnDemandHelper \
  { \
    public: \
    /* reimplement functions */ \
    virtual size_t FreeOneItem##id() = 0; \
    \
    virtual float Priority##id() const = 0; \
    \
    virtual RString GetDebugName##id() const = 0; \
    virtual size_t MemoryControlled##id() const = 0; \
    virtual size_t MemoryControlledRecent##id() const = 0; \
    virtual void MemoryControlledFrame##id() = 0; \
    /* redirect to the reimplementation */ \
    virtual size_t FreeOneItem() {return FreeOneItem##id();} \
    \
    virtual float Priority() const {return Priority##id();} \
    \
    virtual RString GetDebugName() const {return GetDebugName##id();} \
    virtual size_t MemoryControlled() const {return MemoryControlled##id();} \
    virtual size_t MemoryControlledRecent() const {return MemoryControlledRecent##id();} \
    virtual void MemoryControlledFrame() {MemoryControlledFrame##id();} \
  };


/// various properties used for storing memory requirement
struct FrameStoreTraits
{
  typedef size_t MemSize;

  static bool IsEmpty(MemSize size){return size==0;}
};
/// link with memory size stored
/** It holds how much memory was counted when adding into list,
so that it can be deduced when removing.
*/

template <class Traits=FrameStoreTraits>
class LinkBidirWithFrameStoreT: public TLinkBidir<2>
{
  public:
  typedef Traits StoreTraits;
  typedef typename Traits::MemSize MemSize;
  private:
  mutable MemSize _size;
  mutable int _frameNum;
  
  public:
  LinkBidirWithFrameStoreT()
  {
    _size = MemSize();
    _frameNum = INT_MIN;
  }
  #if _DEBUG
  ~LinkBidirWithFrameStoreT()
  {
    Assert(Traits::IsEmpty(_size));
    Assert(_frameNum==INT_MIN);
  }
  #endif
  MemSize GetStoredMemoryControlled() const {return _size;}
  int GetFrameNum() const {Assert(_frameNum!=INT_MIN);return _frameNum;}
  void SetStoredMemoryControlled(int frameNum, MemSize size) const
  {
    Assert(Traits::IsEmpty(_size));
    Assert(_frameNum==INT_MIN);
    _frameNum = frameNum;
    _size = size;
  }
  void ResetStoredMemoryControlled() const
  {
    _size = MemSize();
    _frameNum = INT_MIN;
  }
};

typedef LinkBidirWithFrameStoreT<> LinkBidirWithFrameStore;

template <class Type>
struct FramedMemoryControlledListTraits
{
  /// claim gaining permanent ownership
  static void AddRef(Type *item) {}
  /// report permanent ownership lost
  static void Release(Type *item){}
};

template <class Type>
struct FramedMemoryControlledListTraitsRef
{
  static void AddRef(Type *item) {item->AddRef();}
  static void Release(Type *item){item->Release();}
};

/// release given amount from any slots containing id in the name (ignore case)
size_t FreeOnDemandReleaseSlot(const char *id, size_t toRelease, size_t *verifyRelease=NULL);

/// let all on-demand handlers know frame is finished
void FreeOnDemandFrame();

extern int FrameId;

/// check frame ID for the frame which is being processed now
__forceinline int FreeOnDemandFrameID() {return FrameId;}


/// check total memory allocation allowed for the application
/** This does not include VRAM unless running on UMA architecture */
mem_size_t GetMemoryUsageLimit(mem_size_t *virtualLimit=NULL);

/// list is able to tell how much memory was used recently
// and how much is controlled in total
/**
@param NLists how many ages to we want to differentiate between
*/
template <class Type, class Traits=FramedMemoryControlledListTraits<Type>,int NLists=3>
class FramedMemoryControlledList
{
  /// Type is usually derived from LinkBidirWithFrameStore
  typedef TLinkBidir<2> BaseLinkType;
  typedef typename Type::StoreTraits StoreTraits;
  typedef typename StoreTraits::MemSize MemSize;
  typedef TListBidir<Type,BaseLinkType> ListType;
  
  /**
  Note: internal list order is reversed compared to the interface.
  This is because of the way Move works - appending to the beginning. 
  */
  ListType _list[NLists];
  /**
  Index 0 is the most recent one
  In previous naive implementation stored _listItemCount contained count for given age.
  As we want to be able to access sizes and counts lock-free, we rather store count for given age or newer.
  This makes reading the counter atomic.
  */
  int _listItemCount[NLists];
  MemSize _listSize[NLists];

  /// local frame ID
  /** this is to prevent race conditions seen when global FrameId can be changed at any time */
  int _frameId;
  
  private:
  /// shadow global function by a variable so that compiler detects when it is accessed
  void *FreeOnDemandFrameID;
  

  public:
  FramedMemoryControlledList()
  {
    for (int i=0; i<NLists; i++) _listSize[i] = MemSize(), _listItemCount[i]=0;
    AssertMainThread();
    _frameId = ::FreeOnDemandFrameID();
  }
  ~FramedMemoryControlledList()
  {
    Assert(First()==NULL);
    Assert(StoreTraits::IsEmpty(MemoryControlled()));
  }

  __forceinline int FrameID() const {return _frameId;}
  /// add as a most recent entry
  /**
  @param permanent indicates whether ownership change should be handled
    template used to enforce runtime resolution
  */
  template <bool permanent>
  void Add(Type *item)
  {
    MemSize size = item->GetMemoryControlled();
    
    // always add to most recent list
    const int age = 0;
    _list[age].Insert(item);
    if (permanent) Traits::AddRef(item);
    for (int i=age; i<NLists; i++)
    {
      _listSize[i] += size;
      _listItemCount[i]++;
    }
    item->SetStoredMemoryControlled(FrameID(),size);
    // following assert is valid, but causes severe performance hit
    //Assert( CheckIntegrity() ); 
  }
  /// add, but make it oldest entry
  template <bool permanent>
  void AddLast(Type *item)
  {
    MemSize size = item->GetMemoryControlled();
    const int age = NLists-1;
    // add to the end of the list
    _list[age].Add(item);
    if (permanent) Traits::AddRef(item);
    for (int i=age; i<NLists; i++)
    {
      _listSize[i] += size;
      _listItemCount[i]++;
    }
    item->SetStoredMemoryControlled(FrameID()-age,size);
    // following assert is valid, but causes severe performance hit
    //Assert( CheckIntegrity() );
  }
  /// first is the oldest one
  Type *First() const
  {
    for (int i=NLists; --i>=0; )
    {
      Type *item = _list[i].Last();
      if (item) return item;
    }
    return NULL;
  }
  Type *Next(Type *item) const
  {
    int frameAge = FrameID()-item->GetFrameNum();
    Assert(frameAge>=0);
    if (frameAge>NLists-1) frameAge = NLists-1;
    
    Type *next = _list[frameAge].Prev(item);
    if (next) return next;
    // return more recent list
    while (--frameAge>=0)
    {
      Type *next = _list[frameAge].Last();
      if (next) return next;
    }
    return NULL;
  }
  /// last is the most recent one
  Type *Last() const
  {
    for (int i=0; i<NLists; i++)
    {
      Type *item = _list[i].First();
      if (item) return item;
    }
    return NULL;
  }
  Type *Prev(Type *item) const
  {
    int frameAge = FrameID()-item->GetFrameNum();
    Assert(frameAge>=0);
    if (frameAge>NLists-1) frameAge = NLists-1;
    
    Type *prev = _list[frameAge].Next(item);
    if (prev) return prev;
    // return more recent list
    while (++frameAge<NLists)
    {
      Type *prev = _list[frameAge].First();
      if (prev) return prev;
    }
    return NULL;
  }
  /// Release all items
  void Clear()
  {
    Type *item;
    while ((item=First())!=NULL)
    {
      Delete(item);
    }
  }
  /// Release given item
  /**
  @return how many bytes was released
  */
  template <bool permanent>
  MemSize Delete(Type *item)
  {
    // delete based on item frame number
    int frameAge = FrameID()-item->GetFrameNum();
    Assert(frameAge>=0);
    if (frameAge>NLists-1) frameAge = NLists-1;
    MemSize size = item->GetStoredMemoryControlled();
    // update corresponding frame and all more recent
    for (int i=frameAge; i<NLists; i++)
    {
      _listSize[i] -= size;
      _listItemCount[i]--;
    }
    item->ResetStoredMemoryControlled();
    item->BaseLinkType::Delete();
    if (permanent) Traits::Release(item);
    // following assert is valid, but causes severe performance hit
    //Assert( CheckIntegrity() );
    return size;
  }
  __forceinline void Add(Type *item) {return Add<true>(item);}
  __forceinline void AddLast(Type *item) {return AddLast<true>(item);}
  __forceinline MemSize Delete(Type *item) {return Delete<true>(item);}

  /// recalculate the item memory stats
  void RefreshMemoryControlled(Type *item)
  {
    int frameNum = item->GetFrameNum();
    int frameAge = FrameID()-frameNum;
    Assert(frameAge>=0);
    if (frameAge>NLists-1) frameAge = NLists-1;
    MemSize newSize = item->GetMemoryControlled();
    MemSize change = newSize;
    // use -=, as we cannot assume - is implemented
    change -= item->GetStoredMemoryControlled();

    // adjust the total based on the change size
    for (int i=frameAge; i<NLists; i++)
    {
      _listSize[i] += change;
    }

    // always add to most recent list
    item->ResetStoredMemoryControlled();
    item->SetStoredMemoryControlled(frameNum,newSize);
  }
  /// mark the item as LRU, move it to the freshest position
  void Refresh(Type *item)
  {
    if (item->GetFrameNum()==FrameID())
    {
      // easy refresh - no need to update stats
      item->BaseLinkType::Delete();
      _list[0].Insert(item);
      //Assert( CheckIntegrity() );
    }
    else
    {
      // Delete/Add normally does Release/AddRef
      // here we want to avoid this, as we keep the ownership
      Delete<false>(item);
      Add<false>(item);
    }
  }
  void Frame()
  {
    // move each list into the older one
    Assert( CheckIntegrity() );
    for (int i=NLists-1; i>=1; i--)
    {
      ListType &src = _list[i-1];
      ListType &dst = _list[i];
      // src.First becomes dst.First() after Move
      dst.Move(src);
      // each slot contains more recent data as well
      // oldest slot does not contain only data NLists-1 old, but rather data of any age
      // this is a number which is not changing at all
      if (i!=NLists-1)
      {
        // each slot becomes a frame older, therefore each stat moves one slot      
        _listSize[i] = _listSize[i-1];
        _listItemCount[i] = _listItemCount[i-1];
      }
    }
    // there are no fresh data now - reset stats
    _listSize[0] = MemSize();
    _listItemCount[0] = 0;
    _frameId++;
    
    Assert(_frameId==::FreeOnDemandFrameID());
    // following assert is valid, but causes a performance hit
    // calling once per frame is enough to be confident there is no problem
    //Assert( CheckIntegrity() );
  }
  private:
  MemSize MemoryControlled(int nLists) const
  {
    return _listSize[nLists-1];
  }
  int ItemCount(int nLists) const
  {
    return _listItemCount[nLists-1];
  }
  
  public:
  MemSize MemoryControlled() const {return MemoryControlled(NLists);}
  MemSize MemoryControlledRecent() const {return MemoryControlled(NLists-1);}
  
  int ItemCount() const {return ItemCount(NLists);}
  int ItemCountRecent() const {return ItemCount(NLists-1);}

  bool CheckIntegrity() const
  {
    #if _DEBUG
    MemSize total = MemSize();
    int count = 0;
    bool ret = true;
    for (int i=0; i<NLists; i++)
    {
      const ListType &list = _list[i];
      for (Type *s = list.Start(); list.NotEnd(s); s = list.Advance(s))
      {
        total += s->GetStoredMemoryControlled();
        count++;
      }
      if (count!=_listItemCount[i])
      {
        LogF("count (%d)!=list->ItemCount (%d), list %d",count,_listItemCount[i],i);
        ret = false;
      }
      if (!StoreTraits::IsEmpty(total -_listSize[i]))
      {
        LogF("total (%d)!=list->Size (%d), list %d, total %d, _listSize[i] %d",total, _listSize[i], i);
        ret = false;
    }
    }
    return ret;
    #else
    return true;
    #endif
  }
};

#include <Es/Containers/bankArray.hpp>

/// bank with caching
template <class Type, class Traits=BankTraits<Type>, class ExtTraits=BankTraitsExt<Traits> >
class BankArrayCached: public BankArray<Type,Traits,ExtTraits>, public MemoryFreeOnDemandHelper
{
  typedef BankArray<Type,Traits,ExtTraits> base;

  protected:  
  FramedMemoryControlledList<Type> _cache;
  
  public:
  BankArrayCached();

  Type *Find(typename Traits::NameType name)
  {
    Type *item = base::Find(name);
    if (item)
    {
      if (item->IsInList())
      {
    		DoAssert(item->RefCounter()==0);
    		_cache.Delete(item);
        Traits::ReuseCachedItem(item);
      }
      return item;
    }
    #if _DEBUG
    /// if not found, search cache
    for (Type *item = _cache.Last(); item; item=_cache.Prev(item))
    {
      if (!Traits::CompareNames(ExtTraits::GetName(item),name))
      {
        Assert(item->RefCounter()==0);
        ExtTraits::AddItem(*this,item);
        _cache.Delete(item);
        Traits::ReuseCachedItem(item);
    		ErrF("Late cached item discovery for %s",typeid(Type).name());
        return item;
      }
    }
    #endif
    return NULL;
  }


  Type *New(typename Traits::NameType name)
  {
    /// search existing items  first
    Type *item = Find(name);
    if (item)
    {
      return item;
    }
    return base::Load(name);
  }
  bool Delete(Type *item)
  {
    Assert(!item->IsInList());
    return base::Delete(item);
  }
  /// flush all cached items
  void ClearCache()
  {
    // it would be cleaner to remove entries from the hashmap here as well,
    // but as this is done during cleanup we do not care
    for(;;)
    {
      Type *last = _cache.First();
      if (!last) break;
      Assert(last->RefCounter()==0);
      _cache.Delete(last);
      Delete(last);
      //Assert(last->CheckNoLinks());
      last->Destroy();
    }
  }

  /// called when item should be cached
  void OnUnused(Type *tex) {_cache.Add(tex);}

  //@{ implementation of MemoryFreeOnDemandHelper
  virtual size_t MemoryControlled() const {return _cache.MemoryControlled();}
  virtual size_t MemoryControlledRecent() const {return _cache.MemoryControlledRecent();}
  virtual void MemoryControlledFrame() {_cache.Frame();}
  virtual size_t FreeOneItem()
  {
    Type *last = _cache.First();
    if (!last) return 0;
    Assert(last->RefCounter()==0);
    //Assert(last->CheckNoLinks());
    size_t ret = _cache.Delete(last);
    // need to remove from the hash map as well
    Delete(last);
    last->Destroy();
    return ret;
  }
  //@} implementation of MemoryFreeOnDemandHelper
};

template <class Type,class Traits, class ExtTraits>
BankArrayCached<Type,Traits,ExtTraits>::BankArrayCached()
{
}

//@{
/*!
\name main heap memory handling
Items in this heap may be deallocated during explicit request.
*/
extern mem_size_t FreeOnDemandMemory(mem_size_t size, bool physical, IMemoryFreeOnDemand **extras=NULL, int nExtras=0);
extern void RegisterFreeOnDemandMemory(IMemoryFreeOnDemand *object);
extern IMemoryFreeOnDemand *GetFirstFreeOnDemandMemory();
extern IMemoryFreeOnDemand *GetNextFreeOnDemandMemory(IMemoryFreeOnDemand *cur);
extern void FreeOnDemandLock();
extern void FreeOnDemandUnlock();

//@}

//@{
/*!
\name system heap memory handling
Items in this heap may be deallocated during explicit request.

MT safety: Enumeration must be enclosed in FreeOnDemandLock / FreeOnDemandUnlock
*/
extern size_t FreeOnDemandSystemMemory(
  size_t size, IMemoryFreeOnDemand **extras=NULL, int nExtras=0
);
extern void RegisterFreeOnDemandSystemMemory(IMemoryFreeOnDemand *object);
extern IMemoryFreeOnDemand *GetFirstFreeOnDemandSystemMemory();
extern IMemoryFreeOnDemand *GetNextFreeOnDemandSystemMemory(IMemoryFreeOnDemand *cur);
//@}

/// garbage collect all heaps (system, main and low-level)
/** typically used once per loop or when the game knows it is a good time to do so */

void FreeOnDemandGarbageCollectMain(size_t freeRequired, size_t freeSysRequired, bool isMain=true);

/// garbage collect all heaps (system, main and low-level)
inline void FreeOnDemandGarbageCollect(size_t freeRequired, size_t freeSysRequired)
{
  // not all platforms require a strict memory management - on PC temporary allocation is quite fine
  // even on X360 we should have quite a large file cache which should be able to handle temporary drop-outs
}


/*!
\name low-level heap memory handling
Items in this heap may be deallocated anytime during new or malloc.

MT safety: Enumeration must be enclosed in FreeOnDemandLock / FreeOnDemandUnlock
*/
/// called when some memory allocation failed
extern size_t FreeOnDemandLowLevelMemory(size_t size);
extern void RegisterFreeOnDemandLowLevelMemory(IMemoryFreeOnDemand *object);
extern IMemoryFreeOnDemand *GetFirstFreeOnDemandLowLevelMemory();
extern IMemoryFreeOnDemand *GetNextFreeOnDemandLowLevelMemory(IMemoryFreeOnDemand *cur);

/// helper to allow easy safe usage of enumeration
struct FreeOnDemandScopeLock: private NoCopy
{
  FreeOnDemandScopeLock(){FreeOnDemandLock();}
  ~FreeOnDemandScopeLock(){FreeOnDemandUnlock();}
  
  static IMemoryFreeOnDemand *GetFirstFreeOnDemandMemory(){return ::GetFirstFreeOnDemandMemory();}
  static IMemoryFreeOnDemand *GetNextFreeOnDemandMemory(IMemoryFreeOnDemand *cur){return ::GetNextFreeOnDemandMemory(cur);}
  static IMemoryFreeOnDemand *GetFirstFreeOnDemandLowLevelMemory(){return ::GetFirstFreeOnDemandLowLevelMemory();}
  static IMemoryFreeOnDemand *GetNextFreeOnDemandLowLevelMemory(IMemoryFreeOnDemand *cur){return ::GetNextFreeOnDemandLowLevelMemory(cur);}
  
};

/// called when some system memory allocation failed
size_t FreeOnDemandSystemMemoryLowLevel(size_t size);

/// called to make sure some system memory is free
void FreeOnDemandGarbageCollectSystemMemoryLowLevel(size_t freeSysRequired);

#endif
