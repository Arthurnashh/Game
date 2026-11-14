#ifdef _MSC_VER
#pragma once
#endif
#ifndef __MEM_ALLOC_HPP
#define __MEM_ALLOC_HPP

// MemAlloc template sample
// default implementation usign new char

#include <Es/Framework/debugLog.hpp>

#include <Es/Memory/normalNew.hpp>

/// default allocator, using new char[] and delete[]

class MemAllocD //: public CountInstances<MemAllocD>
{
  public:
  #if ALLOC_DEBUGGER
  static void *Alloc( size_t &size, const char *file, int line, const char *postfix )
  {
    return GMemFunctions->New(size,FileLineF(file,line,postfix),0);
  }
  #else
  static void *Alloc( size_t &size )
  {
    // size is not changed
    return GMemFunctions->New(size);
  }
  #endif

  /// if the allocator is able to resize blocks, it may greatly increase efficiency
  /**
  @param oldSize current size of the block
  @param size in/out - requested size / allocated size
  */
  static void *Realloc (void *mem, int oldSize, size_t &size)
  {
    return GMemFunctions->Resize(mem,size);
  }

  /// release the block
  static void Free( void *mem, size_t size )
  {
    GMemFunctions->Delete(mem);
  }
  static void Unlink( void *mem ) {}

  static inline int MinGrow() {return 32;}
  // report no storage
  static size_t StorageSize() {return 0;}
};

/// Allocator similar to MemAllocD, using (thread) safe heap
class MemAllocDSafe //: public CountInstances<MemAllocDSafe>
{
public:
#if ALLOC_DEBUGGER
  static void *Alloc( size_t &size, const char *file, int line, const char *postfix )
  {
    return GMTMemFunctions->New(size,FileLineF(file,line,postfix),0);
  }
#else
  static void *Alloc( size_t &size )
  {
    // size is not changed
    return GMTMemFunctions->New(size);
  }
#endif

  /// if the allocator is able to resize blocks, it may greatly increase efficiency
  /**
  @param oldSize current size of the block
  @param size in/out - requested size / allocated size
  */
  static void *Realloc (void *mem, size_t oldSize, size_t &size)
  {
    return GMTMemFunctions->Resize(mem,size);
  }

  /// release the block
  static void Free( void *mem, size_t size )
  {
    GMTMemFunctions->Delete(mem);
  }
  static void Unlink( void *mem ) {}

  static inline int MinGrow() {return 32;}
  // report no storage
  static int StorageSize() {return 0;}
};

/// allocate and construct
template <class Type, class Allocator>
static Type *AllocatorNew(Allocator &alloc)
{
  size_t size = sizeof(Type);
  #if ALLOC_DEBUGGER
    #ifdef _CPPRTTI 
      void *mem = alloc.Alloc(size,__FILE__,__LINE__,typeid(Type).name());
    #else
      void *mem = alloc.Alloc(size,__FILE__,__LINE__,"");
    #endif
  #else
    void *mem = alloc.Alloc(size);
  #endif
  // if the allocator provided more, it can expect we will tell it the same value
  // once we call Delete, however we are unable to do so
  Assert(size==sizeof(Type));
  return new(mem) Type();
}

/// deallocate and destruct
template <class Type, class Allocator>
static void AllocatorDelete(Allocator &alloc, Type *type)
{
  type->~Type();
  alloc.Free(type,sizeof(Type));
}

class MemAllocS
{
  void *_mem;
  size_t _size;
  bool _busy;
  MemAllocD _fallBack;

  public:
  #if ALLOC_DEBUGGER
  void *Alloc( size_t &size, const char *file, int line, const char *postfix )
  {
    if( size<=_size && !_busy)
    {
      size=_size;
      _busy = true;
      return _mem;
    }
    else return _fallBack.Alloc(size,file,line,postfix);
  }
  #else
  void *Alloc( size_t &size )
  {
    // size is not changed
    if( size<=_size && !_busy)
    {
      size=_size;
      _busy = true;
      return _mem;
    }
    else return _fallBack.Alloc(size);
  }
  #endif
  void Free( void *mem, int size )
  {
    if( mem==_mem )
    {
      Assert( _busy );
      _busy = false;
    }
    else
    {
      _fallBack.Free(mem,size);
    }
  }
  static void *Realloc (void *mem, int oldSize, int &size) {return NULL;}
  
  MemAllocS():_mem(NULL),_size(0){}
  ~MemAllocS(){Clear();}

  MemAllocS *InitStorage( size_t size )
  {
    if( !_mem )
    {
      #if ALLOC_DEBUGGER
      _mem=_fallBack.Alloc(size,__FILE__,__LINE__,"");
      #else
      _mem=_fallBack.Alloc(size);
      #endif
      _size = size;
      _busy = false;
    }
    return this;
  }

  void *GetMemory() const {return _mem;} // for diagnostic purposes only

  MemAllocS *InitStorage( size_t size, const char *name )
  {
    if( !_mem )
    {
      #if ALLOC_DEBUGGER
      _mem=_fallBack.Alloc(size,__FILE__,__LINE__,name);
      #else
      _mem=_fallBack.Alloc(size);
      #endif
      _size = size;
      _busy = false;
    }
    return this;
  }

  void Clear()
  {
    if( _mem )
    {
      DoAssert( !_busy );
      _fallBack.Free(_mem,_size); // _size does not matter for MemAllocD
      _mem=NULL;
      _size=0;
    }
  }

  void Unlink( void *mem )
  {
    if( _mem==mem )
    {
      // memory is now dynamic - leave it to _fallback
      _mem = NULL;
      // allocate new static memory
      InitStorage(_size,"Unlink");
    }
  }

  static inline int MinGrow() {return 32;}
  size_t StorageSize() const {return _size;}

};

/// static shared allocator, memory allocated using MemAllocS

class MemAllocSS // static shared
{
  MemAllocS *_alloc;

  public:
  void Init( MemAllocS *alloc ) {_alloc=alloc;}
  MemAllocSS():_alloc(NULL) {}
  MemAllocSS( MemAllocS *alloc ):_alloc(alloc) {}
  //return _alloc->Alloc(

  #if ALLOC_DEBUGGER
  void *Alloc( size_t &size, const char *file, int line, const char *postfix )
  {
    if( _alloc ) return _alloc->Alloc(size,file,line,postfix);
    else return MemAllocD::Alloc(size,file,line,postfix);
  }
  #else
  void *Alloc( size_t &size )
  {
    if( _alloc ) return _alloc->Alloc(size);
    else return MemAllocD::Alloc(size);
  }
  #endif
  void Free( void *mem, size_t size )
  {
    if( _alloc ) _alloc->Free(mem,size);
    else MemAllocD::Free(mem,size);
  }

  static void *Realloc (void *mem, int oldSize, size_t &size) {return NULL;}

  void Unlink( void *mem )
  {
    if( _alloc )
    {
      _alloc->Unlink(mem);
      _alloc=NULL;
    }
  }

  static inline int MinGrow() {return 32;}
  size_t StorageSize() const {return _alloc ? _alloc->StorageSize() : 0;}

};

/// memory allocated externally, typically on the stack (auto)
class MemAllocSA: private MemAllocD
{
  typedef MemAllocD Fallback;
  /// memory allocated (externally, e.g on stack)
  void *_mem;
  size_t _size;
  bool _busy;

  public:
  #if ALLOC_DEBUGGER
  void *Alloc( size_t &size, const char *file, int line, const char *postfix )
  {
    if( size<=_size && !_busy )
    {
      // we have enough memory
      size=_size;
      _busy = true;
      return _mem;
    }
    else return Fallback::Alloc(size,file,line,postfix);
  }
  #else
  void *Alloc( size_t &size )
  {
    if( size<=_size && !_busy )
    {
      // we have enough memory
      size=_size;
      _busy = true;
      return _mem;
    }
    else return Fallback::Alloc(size);
  }
  #endif
  void Free( void *mem, int size )
  {
    if( mem==_mem )
    {
      Assert( _busy );
      _busy = false;
    }
    else
    {
      Fallback::Free(mem,size);
    }
  }
  static void *Realloc (void *mem, size_t oldSize, size_t &size) {return NULL;}
  
  MemAllocSA():_mem(NULL),_size(0),_busy(false){}
  MemAllocSA(void *mem, int size):_mem(mem),_size(size),_busy(false){}
  void Init( void *mem, int size ){_mem=mem,_size=size,_busy=false;}
  ~MemAllocSA(){Clear();}

  void Clear()
  {
    if( _mem )
    {
      // only non-allocated memory may be released
      DoAssert(!_busy);
      _mem=NULL;
      _size=0;
    }
  }

  void Unlink( void *mem )
  {
    if( _mem==mem )
    {
      Fail("Unable to unlink");
    }
  }

  static inline int MinGrow() {return 32;}
  size_t StorageSize() const {return _size;}
};

template <class Type>
class MStorage: public MemAllocS
{
  public:
  MemAllocS *Init( int size )
  {
    const int MinSize=32; // must be higher or equal MinGrow from AutoArray
    if( size<MinSize ) size=MinSize;
    #if ALLOC_DEBUGGER && defined _CPPRTTI
    return InitStorage(size*sizeof(Type),typeid(Type).name());
    #else
    return InitStorage(size*sizeof(Type));
    #endif
  }
};

///Memory allocated on stack.
/** allocator will allocate memory on stack, if requested size is lower or equal to reserved
space on stack (defined by parameter). If requested size is above, it will use base allocator 
(typically MemAllocD).

@param Item type of items that will be allocated by this allocator (MUST be same as type in AutoArray)
@param count count of items, which must be at least allocated on stack.
@param AlignType you can optionally specify align type, which forces compiler to reserve space aligned
  for specified type. Default value causes default align.
@param Base you can optionally specify base class for this allocator. Specified allocator will be used
  in case, that requested size if above reserved space. Default is MemAllocD.

*/

template<class Item, int count, class AlignType=int, class Base=MemAllocD>
class MemAllocLocal: private Base, private NoCopy
{
#if _MSC_VER<=1200
  
  // For MSVC6.0 define constants as enums.
  
  /// Calculates size of buffer in bytes
  enum {BufferSize=(sizeof(Item)*count)};
  /// Calculates size of buffer in AlignType - it can add some padding for right alignment.
  enum {ArrayItemCount=(BufferSize+sizeof(AlignType)-1)/sizeof(AlignType)};

#else
  
  // For .NET and newer compilers, use standard constants

  /// Calculates size of buffer in bytes
  static const size_t BufferSize=(sizeof(Item)*count);
  /// Calculates size of buffer in AlignType - it can add some padding for right alignment.
  static const size_t ArrayItemCount=(BufferSize+sizeof(AlignType)-1)/sizeof(AlignType);


#endif

  ///Buffer allocation. 
  AlignType _buffer[ArrayItemCount];

  bool _memused;
public:
  MemAllocLocal():_memused(false) {}
  #if ALLOC_DEBUGGER
  ///Allocates spaces in allocator (debug version)
  void *Alloc( size_t &size, const char *file, int line, const char *postfix )
  {
    if (size<=BufferSize && !_memused)
    {
      size = BufferSize;
      _memused=true;
      return _buffer;
    }
    else return Base::Alloc(size,file,line,postfix);
  }
  #else
  ///Allocates spaces in allocator (release version)
  void *Alloc( size_t &size)
  {
    if (size<=BufferSize && !_memused)
    {
      size = BufferSize;
      _memused=true;
      return _buffer;
    }
    else return Base::Alloc(size);
  }
  #endif
  void *Realloc (void *mem, size_t oldSize, size_t &size)
  {
    // our buffer - handle it
    if (mem==_buffer)
    {
      // check if we can fit
      if (size<=BufferSize) return mem;
      return NULL;
    }
    else return Base::Realloc(mem,oldSize,size);
  }
  ///Deallocates space. 
  /**
  @param mem pointer to memory allocated by Alloc.
  @param size size of memory to deallocate - this parameter is ignored by this allocator
  */
  void Free( void *mem, size_t size )
  {
    if (mem!=_buffer) Base::Free(mem,size);
    else _memused=false;
  }

  void Unlink( void *mem ) {}

  ///
  /**
  @return Minimal recommended size in items for optimal usage of this allocator
  */
  static inline int MinGrow() {return 1;}
  const size_t StorageSize() {return BufferSize;}
};

#pragma deprecated(MemAllocStack)
#define MemAllocStack MemAllocLocal


/// Dedicated data stack - allowing simple pop/push style memory allocations
class DataStack: private NoCopy
{
  char *_stack;
  size_t _allocated;
  size_t _dataSize;
  #if _ENABLE_REPORT && !defined(_LINUX)
  size_t _totalAlloc;
  size_t _maxTotalAlloc;
  int _owningThread;
  #endif
  public:
  DataStack(size_t size)
  {
    _allocated = 0;
    _stack = (char *)malloc(size);
    _dataSize = size;
    #if _ENABLE_REPORT && !defined(_LINUX)
      _totalAlloc = 0;
      _maxTotalAlloc = 0;
    #endif
  }
  ~DataStack()
  {
    free(_stack);
  }
  void *Allocate(size_t size, size_t align, size_t &old)
  {
    #if _ENABLE_REPORT && !defined(_LINUX)
    AssertSameThread (_owningThread);
    #endif
    // first of all we need to align
    size_t aligned = (_allocated + align-1)&~(align-1);
    // check if we fit in
    if(aligned+size<=_dataSize)
    {
      // if we do, advance the stack pointer
      void *ret = _stack+aligned;
      old = _allocated;
      _allocated = aligned+size;
      #if _ENABLE_REPORT && !defined(_LINUX)
        // track total usage
        _totalAlloc += _allocated-old;
        if (_totalAlloc>_maxTotalAlloc) _maxTotalAlloc = _totalAlloc;
      #endif
      return ret;
    }
    else
    {
      // if not, mark we did not use the stack and use a fallback method
      old = -1;
      // caution: when alignment was required, are we sure the new will respect it?
      char *ret = (char *)malloc(size);
      DoAssert((intptr_t(ret)&(align-1))==0);
      #if _ENABLE_REPORT && !defined(_LINUX)
        // track total usage
        _totalAlloc += _msize(ret);
        if (_totalAlloc>_maxTotalAlloc) _maxTotalAlloc = _totalAlloc;
      #endif
      return ret;
    }
    
  }
  void Release(void *mem, size_t old)
  {
    #if _ENABLE_REPORT && !defined(_LINUX)
    AssertSameThread (_owningThread);
    #endif
    if (old==(size_t)-1)
    {
      #if _ENABLE_REPORT && !defined(_LINUX)
        // track total usage
        _totalAlloc -= _msize(mem);
      #endif
      // was using the fall-back, use fall-back again
      free(mem);
      Assert(old==-1);
    }
    else
    {
      #if _ENABLE_REPORT && !defined(_LINUX)
        _totalAlloc -= _allocated-old;
      #endif
      Assert(old<=_allocated);
      Assert(old>=0);
      _allocated = old;
    }
  }
};

/**
Note: when somebody is using this, he must make sure GDataStack is defined and initialized.
*/
extern DataStack GDataStack;

///Memory allocated on a dedicated data stack
/** allocator will allocate memory in a dedicated data area, if requested size is lower or equal to reserved
space on stack (defined by parameter). If requested size is above, it will use base allocator 
(typically MemAllocD).

@param Item type of items that will be allocated by this allocator (MUST be same as type in AutoArray)
@param count count of items, which must be at least allocated on stack.
@param AlignType you can optionally specify align type, which forces compiler to reserve space aligned
  for specified type. Default value causes default align.
@param Base you can optionally specify base class for this allocator. Specified allocator will be used
  in case, that requested size if above reserved space. Default is MemAllocD.

*/
template<class Item, int count, class AlignType=int, DataStack *stack=&GDataStack, class Base=MemAllocD>
class MemAllocDataStack: private Base, private NoCopy
{
  /// Calculates size of buffer in bytes
  static const size_t BufferSize=(sizeof(Item)*count);
  /// Calculates size of buffer in AlignType - it can add some padding for right alignment.
  static const size_t ArrayItemCount=(BufferSize+sizeof(AlignType)-1)/sizeof(AlignType);

  ///Buffer allocation. 
  size_t _oldSP;
  void *_buffer;

  bool _memused;
public:
  MemAllocDataStack():_memused(false)
  {
    _buffer = stack->Allocate(BufferSize,sizeof(AlignType),_oldSP);
  }
  ~MemAllocDataStack()
  {
    stack->Release(_buffer,_oldSP);
  }
  #if ALLOC_DEBUGGER
  ///Allocates spaces in allocator (debug version)
  void *Alloc( size_t &size, const char *file, int line, const char *postfix )
  {
    if (size<=BufferSize && !_memused)
    {
      size = BufferSize;
      _memused=true;
      return _buffer;
    }
    else return Base::Alloc(size,file,line,postfix);
  }
  #else
  ///Allocates spaces in allocator (release version)
  void *Alloc( size_t &size)
  {
    if (size<=BufferSize && !_memused)
    {
      size = BufferSize;
      _memused=true;
      return _buffer;
    }
    else return Base::Alloc(size);
  }
  #endif
  void *Realloc (void *mem, size_t oldSize, size_t &size)
  {
    // our buffer - handle it
    if (mem==_buffer)
    {
      // check if we can fit
      if (size<=BufferSize) return mem;
      return NULL;
    }
    else return Base::Realloc(mem,oldSize,size);
  }
  ///Deallocates space. 
  /**
  @param mem pointer to memory allocated by Alloc.
  @param size size of memory to deallocate - this parameter is ignored by this allocator
  */
  void Free( void *mem, size_t size )
  {
    if (mem!=_buffer) Base::Free(mem,size);
    else _memused=false;
  }

  void Unlink( void *mem ) {}

  ///
  /**
  @return Minimal recommended size in items for optimal usage of this allocator
  */
  static inline int MinGrow() {return 1;}
  size_t StorageSize() const {return BufferSize;}
};


// note: MemAllocDataStack is not MT safe
//#define MemAllocDataStack MemAllocLocal

//{@ helper types useful for 4B, 8B and 16 B alignment

typedef int AllocAlign4;
typedef double AllocAlign8;

#if defined _KNI
  typedef __m128 AllocAlign16;
#elif defined _M_PPC
  #include <PPCIntrinsics.h>
  typedef __vector4 AllocAlign16;
#elif defined _MSC_VER
  #include <xmmintrin.h>
  typedef __m128 AllocAlign16;
#else
  // see AUTO_STATIC_ARRAY_16 for explanation
  typedef double AllocAlign16;
#endif

//@}


#include <Es/Memory/debugNew.hpp>

/// storage for MemAllocSimple
template <class AlignType, size_t storageSize>
class MemAllocSimpleStorage
{
  AlignType _mem[storageSize];
  /// used memory - in bytes
  size_t _used;

  public:  
  MemAllocSimpleStorage()
  {
    _used = 0;
  }
  void *Alloc(size_t ammount)
  {
    Assert(ammount%sizeof(AlignType)==0);
    if (ammount+_used>storageSize*sizeof(AlignType)) return NULL;
    void *ret = (char *)_mem+_used;
    _used += ammount;
    return ret;
  }
  void *Realloc(void *mem, size_t oldSize, size_t newSize)
  {
    // check if mem is the last block and that we fit into the block
    if (
      (char *)mem+oldSize==(char *)_mem+_used
      && _used+newSize-oldSize<=storageSize*sizeof(AlignType)
    )
    {
      _used += newSize-oldSize;
      return mem;
    }
    return NULL;
  }
  bool IsFromStorage(void *mem) const
  {
    char *memC = (char *)mem;
    return memC>=(char *)_mem && memC<(char *)_mem+storageSize*sizeof(AlignType);
  }
  bool Free(void *mem, size_t size)
  {
    Assert(size%sizeof(AlignType)==0);
    if (IsFromStorage(mem))
    {
      // if we are deallocating the last part, we can mark it as free
      if ((char *)mem+size==(char *)_mem+_used)
      {
        _used -= size;
      }
      return true;
    }
    return false;
  }
};

///Memory allocated by growing item by item
/**
Memory is often not cleaned up, only after objects is destructed.
*/
template<class Storage, class Base=MemAllocD>
class MemAllocSimple: private Base
{
  ///Buffer allocation. 
  Storage *_storage;

public:
  MemAllocSimple()
  :_storage(NULL)
  {
  }
  MemAllocSimple(Storage &storage)
  :_storage(&storage)
  {
  }
  
  void operator = (const MemAllocSimple &src)
  {
    // make sure no storage was set/used yet - if we use the storage, we cannot change it
    Assert(!_storage);
    _storage = src._storage;
  }
  
  void SetStorage(Storage &storage)
  {
    Assert(!_storage);
    _storage = &storage;
  }
  
  #if ALLOC_DEBUGGER
  ///Allocates spaces in allocator (debug version)
  void *Alloc(size_t size, const char *file, int line, const char *postfix)
  {
    if (_storage)
    {
      void *ret = _storage->Alloc(size);
      if (ret) return ret;
    }
    return Base::Alloc(size,file,line,postfix);
  }
  #else
  ///Allocates spaces in allocator (release version)
  void *Alloc(size_t size)
  {
    if (_storage)
    {
      void *ret = _storage->Alloc(size);
      if (ret) return ret;
    }
    return Base::Alloc(size);
  }
  #endif
  void *Realloc(void *mem, size_t oldSize, size_t &size)
  {
    // if we fit into the same buffer, 
    if (_storage)
    {
      // check if memory is from the storage
      if (_storage->IsFromStorage(mem))
      {
        void *ret = _storage->Realloc(mem,oldSize,size);
        if (ret) return ret;
        return NULL;
      }
    }
    return Base::Realloc(mem,oldSize,size);
  }
  ///Deallocates space. 
  /**
  @param mem pointer to memory allocated by Alloc.
  @param size size of memory to deallocate
  */
  void Free(void *mem, size_t size)
  {
    // note: this allocator really uses size
    // if size is incorrect, we may be unable to reclaim the memory
    if (!_storage || !_storage->Free(mem,size)) Base::Free(mem,size);
  }

  void Unlink( void *mem ) {}

  ///
  /**
  @return Minimal recommended size in items for optimal usage of this allocator
  */
  static inline int MinGrow() {return 16;}
  // report no storage
  int StorageSize() const {return _storage ? _storage->StorageSize() : 0;}
};

///Memory allocated by growing item by item, containing storage
/**
Memory is often not cleaned up, only after objects is destructed.
Compared to MemAllocSimple this is faster, it does not require
any arguments for construction.
Drawback is it cannot be copied, and it therefore cannot be shared.
It is not likely to be used with AutoArray,
but it is quite suitable for OffTree allocator.
*/

template<class AlignType, size_t storageSize, class Base=MemAllocD>
class MemAllocSimpleDirect: private Base, private NoCopy
{
  ///Buffer allocation. 
  MemAllocSimpleStorage<AlignType,storageSize> _storage;

public:
  #if ALLOC_DEBUGGER
  ///Allocates spaces in allocator (debug version)
  void *Alloc(size_t size, const char *file, int line, const char *postfix)
  {
    void *ret = _storage.Alloc(size);
    if (ret) return ret;
    return Base::Alloc(size,file,line,postfix);
  }
  #else
  ///Allocates spaces in allocator (release version)
  void *Alloc(size_t size)
  {
    void *ret = _storage.Alloc(size);
    if (ret) return ret;
    return Base::Alloc(size);
  }
  #endif
  ///Deallocates space. 
  /**
  @param mem pointer to memory allocated by Alloc.
  @param size size of memory to deallocate
  */
  void Free(void *mem, size_t size)
  {
    // note: this allocator really uses size
    // if size is incorrect, we may be unable to reclaim the memory
    if (!_storage.Free(mem,size)) Base::Free(mem,size);
  }
  void *Realloc(void *mem, int oldSize, size_t &size)
  {
    void *ret = _storage.Realloc(mem,oldSize,size);
    if (ret) return ret;
    return Base::Realloc(mem,oldSize,size);
  }

  void Unlink( void *mem ) {}

  ///
  /**
  @return Minimal recommended size in items for optimal usage of this allocator
  */
  static inline int MinGrow() {return 64;}
  // report no storage
  static int StorageSize() {return 0;}
};

#endif
