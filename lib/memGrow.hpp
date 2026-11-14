#ifndef _MEMGROW_HPP
#define _MEMGROW_HPP

/// win32/kernel allocation
class MemAllocSystem
{
  public:
  #if ALLOC_DEBUGGER
  static void *Alloc( size_t &size, const char *file, int line, const char *postfix );
  #else
  static void *Alloc( size_t &size );
  #endif
  static void Free( void *mem, size_t size );
  static void *Realloc( void *mem, size_t oldSize, size_t size );
  static void Unlink( void *mem ) {}
  static inline const int MinGrow() {return 32;}

};

/// array able to use low memory when contained values are all identical

template <class Type, class Allocator, int size>
class CompactingArray: private Allocator
{
  Type *_data;
  /// track number of changes
  /** this allows for incremental management */
  int _changes;
  /// if all the values are the same, no need to store them
  Type _constValue;
  
  public:
  CompactingArray()
  :_data(NULL),_constValue(),_changes(0)
  {
  }
  
  /// get one element
  Type Get(int i) const
  {
    Assert(i>=0 && i<size);
    if (!_data) return _constValue;
    return _data[i];
  }

  /// set one element, allocate memory if needed
  void Set(int i, int value)
  {
    if (!_data && value==_constValue) return;
    if (!_data) Alloc();
    DoSet(i,value);
    if (_changes==0)
    {
      // all data are the same - we can compact
      Compact();
    }
  }
  
  /// set all elements, free allocated memory if any
  void SetAll(int value)
  {
    _constValue = value;
    _changes = 0;
    Free();
  }
  /// set range of elements, allocate memory if needed
  void SetRange(int from, int to, Type value)
  {
    Assert(from>=0 && from<=size);
    Assert(to>=0 && to<=size);
    Assert(from<=to);
    if (from>=size || from>=to || !_data && value==_constValue) return;
    if (from==0 && to==size)
    {
      SetAll(value);
      return;
    }
    if (!_data) Alloc();
    // TODO: change count tracking could be optimized, as we know there will be no changes
    // inside of the region being set 
    for (int i=from; i<to; i++)
    {
      DoSet(i,value);
    }
    if (_changes==0)
    {
      Compact();
    }
  }
  
  int FindFirst(int from, int to, Type needed, Type forbidden) const
  {
    Assert(from>=0 && from<=size);
    Assert(to>=0 && to<=size);
    Assert(from<=to);
    // requirements must not contradict
    Assert((needed&forbidden)==0);
    if (!_data)
    {
      // all values are the same - one check for all of them is enough
      if ((_constValue&needed)==needed && (_constValue&forbidden)==0) return from;
      return -1;
    }
    
    for (int i=from; i<to; i++)
    {
      if ((_data[i]&needed)==needed && (_data[i]&forbidden)==0) return i;
    }
    return -1;
  }
  
  private:
  
  /// allocate memory and replicate current virtual value into all values
  void Alloc()
  {
    // no changes can be here so far, there is only one value
    Assert(_changes==0);
    Assert(!_data);
    size_t memSize = size*sizeof(Type);
    #if ALLOC_DEBUGGER && defined _CPPRTTI
    _data = (Type *)Allocator::Alloc(memSize,__FILE__,__LINE__,"");
    #elif ALLOC_DEBUGGER
    _data = (Type *)Allocator::Alloc(memSize,__FILE__,__LINE__,typeid(Type).name());
    #else
    _data = (Type *)Allocator::Alloc(memSize);
    #endif
    ConstructTraits<Type>::ConstructArray(_data,size);
    for (int i=0; i<size; i++) _data[i] = _constValue;
  }
  /// deallocate memory
  void Free()
  {
    if (_data)
    {
      ConstructTraits<Type>::DestructArray(_data,size);
      Allocator::Free(_data,size);
      _data = NULL;
    }
  }

  /// used when we know all data are the same
  void Compact()
  {
    Assert(_changes==0);
    // verify the array can be compacted
    for (int i=1; i<size; i++)
    {
      Assert(_data[i-1]==_data[i]);
    }
    // any value will do
    _constValue = _data[0];
    Free();
  }

  /// helper used for setting including tracking changes
  void DoSet(int i, int value)
  {
    if (i>0)
    {
      // note: using conversion from bool to int here 
      // perfectly legal and conformant, but a bit unusual
      // add newChange-oldChange
      _changes += (value!=_data[i-1])-(_data[i]!=_data[i-1]);
    }
    if (i<size-1)
    {
      _changes += (value!=_data[i+1])-(_data[i]!=_data[i+1]);
    }
    _data[i] = value;
  }
};

/// array containing long strips of identical values
template <class Type, class Allocator, int totalSize, int groupSizeLog>
class SparseArray
{
  
  static const int GroupSize = 1<<groupSizeLog;
  static const int NGroups = (totalSize+GroupSize-1)/GroupSize;
  
  
  CompactingArray<Type,Allocator,GroupSize> _groups[NGroups];
  
  // TODO: memory is allocated only when the group value is not the same for the whole group
  // currently we assume: where there is no group, value is 0
  
  public:
  SparseArray()
  {
    // set all to default value
    SetAll(Type());
  }
  ~SparseArray()
  {
    // set all to default value - this will free any memory
    SetAll(Type());
  }
  
  void SetAll(Type value)
  {
    for (int i=0; i<NGroups; i++) _groups[i].SetAll(value);
  }
  void SetRange(int from, int to, Type value)
  {
    int fromGroup = from>>groupSizeLog;
    int toGroup = to>>groupSizeLog;

    Assert(fromGroup>=0 && fromGroup<=NGroups);
    Assert(toGroup>=0 && toGroup<=NGroups);
    Assert(fromGroup<=toGroup);
    
    int fromOffset = from&(GroupSize-1);
    int toOffset = to&(GroupSize-1);
    // starting / ending group may be the same one
    // process starting (possibly partial) group
    for (int g=fromGroup; g<=toGroup; g++)
    {
      int gStart = g==fromGroup ? fromOffset : 0;
      int gEnd = g==toGroup ? toOffset : GroupSize;
      if (gStart>=gEnd) continue;
      _groups[g].SetRange(gStart,gEnd,value);
    }
  }
  
  Type operator [] (int index) const
  {
    int group = index>>groupSizeLog;
    int offset = index&(GroupSize-1);
    Assert(group>=0 && group<NGroups);
    return _groups[group].Get(offset);
  }
  
  void Set(int index, Type value)
  {
    int group = index>>groupSizeLog;
    int offset = index&(GroupSize-1);
    Assert(group>=0 && group<NGroups);
    return _groups[group].Set(offset,value);
  }
  
  int FindFirst(int from, int to, Type needed, Type forbidden) const
  {
    // find nearest item with needed flags set and forbidden flags not set
    int fromGroup = from>>groupSizeLog;
    int toGroup = to>>groupSizeLog;
    
    int fromOffset = from&(GroupSize-1);
    int toOffset = to&(GroupSize-1);
    // starting / ending group may be the same one
    // process starting (possibly partial) group
    for (int g=fromGroup; g<=toGroup; g++)
    {
      int gStart = g==fromGroup ? fromOffset : 0;
      int gEnd = g==toGroup ? toOffset : GroupSize;
      if (gStart>=gEnd) continue;
      int ret = _groups[g].FindFirst(gStart,gEnd,needed,forbidden);
      // return first match
      if (ret>=0)
      {
        Assert(ret>=gStart && ret<gEnd);
        return (g<<groupSizeLog)+ret;
      }
    }
    return -1;
  }
};

/// array containing long strips of identical values
template <class Type, class Allocator, int groupSizeLog>
class SparseArrayEx
{
  static const int GroupSize = 1<<groupSizeLog;
  
  public:
  typedef CompactingArray<Type,Allocator,GroupSize> Group;

  private:  
  
  Group *_groups;
  int _nGroups;
  
  // TODO: memory is allocated only when the group value is not the same for the whole group
  // currently we assume: where there is no group, value is 0
  
  public:
  SparseArrayEx()
  {
    _groups = NULL;
    _nGroups = 0;
  }
  SparseArrayEx(void *mem, int nGroups)
  {
    _groups = NULL;
    _nGroups = 0;
    Init(mem,nGroups);
  }
  void Init(void *mem, int nGroups)
  {
    DoAssert(_groups==NULL);
    _groups = (Group *)mem;
    _nGroups = nGroups;
    for (int i=0; i<_nGroups; i++) ConstructAt(_groups[i]);
    // set all to default value
    SetAll(Type());
  }
  void Destroy()
  {
    // set all to default value - this will free any memory
    SetAll(Type());
    for (int i=0; i<_nGroups; i++) _groups[i].~Group();
    _groups = NULL;
    _nGroups = 0;
  }
  ~SparseArrayEx()
  {
    Destroy();
  }
  
  void SetAll(Type value)
  {
    for (int i=0; i<_nGroups; i++) _groups[i].SetAll(value);
  }
  void SetRange(int from, int to, Type value)
  {
    int fromGroup = from>>groupSizeLog;
    int toGroup = to>>groupSizeLog;

    Assert(fromGroup>=0 && fromGroup<=_nGroups);
    Assert(toGroup>=0 && toGroup<=_nGroups);
    Assert(fromGroup<=toGroup);
    
    int fromOffset = from&(GroupSize-1);
    int toOffset = to&(GroupSize-1);
    // starting / ending group may be the same one
    // process starting (possibly partial) group
    for (int g=fromGroup; g<=toGroup; g++)
    {
      int gStart = g==fromGroup ? fromOffset : 0;
      int gEnd = g==toGroup ? toOffset : GroupSize;
      if (gStart>=gEnd) continue;
      _groups[g].SetRange(gStart,gEnd,value);
    }
  }
  
  Type operator [] (int index) const
  {
    int group = index>>groupSizeLog;
    int offset = index&(GroupSize-1);
    Assert(group>=0 && group<_nGroups);
    return _groups[group].Get(offset);
  }
  
  void Set(int index, Type value)
  {
    int group = index>>groupSizeLog;
    int offset = index&(GroupSize-1);
    Assert(group>=0 && group<_nGroups);
    return _groups[group].Set(offset,value);
  }
  
  int FindFirst(int from, int to, Type needed, Type forbidden) const
  {
    Assert(from>=0 && from<=_nGroups*GroupSize);
    Assert(to>=0 && to<=_nGroups*GroupSize);
    // find nearest item with needed flags set and forbidden flags not set
    int fromGroup = from>>groupSizeLog;
    int toGroup = to>>groupSizeLog;
    
    int fromOffset = from&(GroupSize-1);
    int toOffset = to&(GroupSize-1);
    // starting / ending group may be the same one
    // process starting (possibly partial) group
    for (int g=fromGroup; g<=toGroup; g++)
    {
      int gStart = g==fromGroup ? fromOffset : 0;
      int gEnd = g==toGroup ? toOffset : GroupSize;
      if (gStart>=gEnd) continue;
      if (g<0 || g>=_nGroups) continue;
      int ret = _groups[g].FindFirst(gStart,gEnd,needed,forbidden);
      // return first match
      if (ret>=0)
      {
        Assert(ret>=gStart && ret<gEnd);
        return (g<<groupSizeLog)+ret;
      }
    }
    return -1;
  }
};

/// information about a block of reserved virtual memory
struct MemReservedInfo
{
  /// groupSizeLog=10 means 1K pages (1<<10)
  static const int GroupSizeLog = 10;
  
  // TODO: use always failing allocator instead of MemAllocD as a base for MemAllocLocal
  typedef SparseArrayEx<unsigned char, MemAllocLocal<unsigned char,1<<GroupSizeLog>, GroupSizeLog> FlagsArray;
  
  void *_data;
  int _reserved;
  
  /// allocation flags for individual pages
  FlagsArray _flags;
};

TypeIsSimple(MemReservedInfo)

/// object tracking state shared between more MemCommit objects
struct MemSharedState
{
  /// ammount of memory heaps are allowed to reserve from now
  int _leftToReserve;
  
  /// recommended size which should be used when reserving more memory
  int _reserveGranularity;
  
  MemSharedState()
  :_leftToReserve(0),_reserveGranularity(0)
  {}
};

//! growing as necessary, pages are released when no longer used
class MemCommit
{
  private:
  enum {MaxResBlocks=32};
  MemReservedInfo _res[MaxResBlocks];
  /// number of items in the _res list
  int _nRes;
  
  
  /// max. ammount of memory which can be committed in this manager
  /**
  Note: sometimes we allow a lot larger ammount of virtual memory (address space) than physical memory
  */
  int _maxCommitted;
  
  /// we want to have some limit common to all MemCommit heaps in the application
  MemSharedState &_sharedState;
  
  int _pageSize;
  int _pageSizeLog;
  /// on Win32: SYSTEM_INFO::dwAllocationGranularity, used for VirtualAlloc
  int _allocationGranularity;
  
  enum
  {
    /// mark page as committed
    PageCommited=1,
    /// mark page as locked (committed and used - cannot be uncommitted)
    PageLocked=2,
    /// when used in conjunction with PageLocked, it means 
    PageLockedTemp=4,
    /// page is available for the manager
    PageAvailable = 0x10,
  };
  
  int _pagesCommited;
  int _pagesLocked;
  bool _error;
  /// some pages should always be committed
  int _pagesMinCommited;

  protected:
  void DoConstruct();
  void DoConstruct( int size );
  void DoDestruct();

  void ReleaseOvercommit();
  
  private: // no copy
  MemCommit( const MemCommit &src );
  void operator = ( const MemCommit &src );

  public:
  explicit MemCommit(MemSharedState &state):_sharedState(state){DoConstruct();}
  explicit MemCommit(MemSharedState &state, int size ):_sharedState(state){DoConstruct(size);}
  ~MemCommit(){DoDestruct();}
  const MemReservedInfo &ReserveWithMult(size_t minPhysicalSize, size_t maxPhysicalSize, size_t vspaceMultiplier)
  {
    const int maxVirtualSize = 256*1024*1024;
    if (maxPhysicalSize>maxVirtualSize)
    {
      return Reserve(maxPhysicalSize,minPhysicalSize,maxPhysicalSize);
    }
    else if(maxPhysicalSize>maxVirtualSize/vspaceMultiplier)
    {
      return Reserve(maxVirtualSize,minPhysicalSize,maxPhysicalSize);
    }
    else
    {
      return Reserve(maxPhysicalSize*vspaceMultiplier,minPhysicalSize,maxPhysicalSize);
    }
  }
  /// is reserving more memory allowed?
  bool CanReserve() const {return _nRes<MaxResBlocks;}
  /// reserve more memory
  const MemReservedInfo &Reserve(size_t virtualSize, size_t minPhysicalSize, size_t maxPhysicalSize);
  /// make sure given amount is always available
  void SetMinCommit(size_t minUsage);


  //! commit given range, free any pages that are not locked
  /*! if commit is not possible return false. This normally means no commit with the same size is possible.
  */
  bool CommitRange(void *startAddr, void *endAddr, bool lock=true);
  void LockRange(void *startAddr, void *endAddr);

  int FindRes(void * startAddr, void * endAddr) const;
  bool CheckLockedRange(void *startAddr, void *endAddr) const;
  void UnlockRange(void *startAddr, void *endAddr);
  bool CheckUnlockedRange(void *startAddr, void *endAddr) const;
  
  bool CommitPages(int rIn, int iStart, int iEnd, bool lock, bool allowDecommit);
  int DecommitPages(int pages);
  bool DecommitOnePage();
  void *FindFreeRange(int size, int align) const;
  
  /// useful for debugging - fragmentation check
  int FindLongestRange() const;
  
  int FindNoncommitedPage(int &rIn) const;

  size_t GetPageSize() const {return _pageSize;}

  char *ExtendEnd(char *end, char *extend);
  char *ExtendBeg(char *beg, char *extend);

  int GetLocked() const {return _pagesLocked*_pageSize;}
  int GetCommitted() const {return _pagesCommited*_pageSize;}
  int GetMaxCommited() const {return _maxCommitted;}

  int GetReserved() const;
  /// check if given pointer is owned by this heap
  bool OwnsPointer(void *p) const;
  void *End() const;
  void Clear(){DoDestruct();}
  
  //void *Data() const {return _data;}
  //int Size() const {return _reserved;}
};

#endif
