///////////////////////////////////////////////////////////////////////////////
// based on
// FastFixedAllocator.h
// from CGDC'98
//
///////////////////////////////////////////////////////////////////////////////


#ifndef _FAST_ALLOC_HPP
#define _FAST_ALLOC_HPP

#include <stddef.h>
#include "checkMem.hpp"

#include "normalNew.hpp"
#ifdef _CPPRTTI
#include <typeinfo.h>
#endif

#include <Es/Containers/listBidir.hpp>

#include <Es/Framework/debugLog.hpp>

///////////////////////////////////////////////////////////////////////////////
// FastFixedAllocator
//
// This version of the allocator is very similar to the one in Stroustrup's
// "The C++ Programming Language", 3rd edition. We have another version that
// we sometimes use that is based on an array instead of a linked list. It
// is slightly faster, but slightly less flexible, as well.
//
/*
  Example usage:
  //Create a class and define a custom allocator for it.

  //or use predefined macro
  struct A
  {
    USE_FIXED_ALLOCATOR
  };
  DEFINE_FIXED_ALLOCATOR(A);
*/

class FastCAlloc; // used for classes
class FastAlloc;

#ifndef PREPROCESS_DOCUMENTATION

#define USE_FAST_ALLOCATOR_ID_ALLOC(AllocType,x) \
  private: \
  static AllocType _allocator##x; \
  void* operator new[]   (size_t n); \
  void  operator delete[](void* ptr, size_t n); \
  public: \
  void *operator new( size_t n ){return _allocator##x.CAlloc(n);} \
  void operator delete(void* ptr){_allocator##x.CFree(ptr);} \
  void *operator new( size_t n, const char *file, int line ){return _allocator##x.CAlloc(n);} \
  void operator delete(void* ptr, const char *file, int line){_allocator##x.CFree(ptr);}

#define DEFINE_FAST_ALLOCATOR_ID_ALLOC(AllocType,className,x) AllocType className::_allocator##x(sizeof(className),#className);

#if defined _WIN32 && !defined MFC_NEW
# define USE_FAST_ALLOCATORS 0 // 1 means FastCAlloc, 2 means NedMalloc
#else
# define USE_FAST_ALLOCATORS 0
#endif

#if defined _WIN32 && !defined MFC_NEW
  #define USE_FAST_ALLOCATOR_SINGLETHREAD USE_FAST_ALLOCATOR_ID_ALLOC(FastCAlloc,F)
  #define DEFINE_FAST_ALLOCATOR_SINGLETHREAD(className) DEFINE_FAST_ALLOCATOR_ID_ALLOC(FastCAlloc,className,F)
#else
  #define USE_FAST_ALLOCATOR_SINGLETHREAD
  #define DEFINE_FAST_ALLOCATOR_SINGLETHREAD(className)
#endif

#if USE_FAST_ALLOCATORS
// real implementation

  #if USE_FAST_ALLOCATORS==1
    typedef FastCAlloc FastAllocType;
  #else
    #include "../../lib/MemAlloc/NedMalloc/nedmalloc.h"
    
    struct FastDummyAlloc
    {
      FastDummyAlloc(int size, const char *name=""){}
      void SetManualCleanUp(bool val){}
      void CleanUp(){}
      __forceinline __declspec(restrict) void *CAlloc(int size) {return nedalloc::nedmalloc(size);}
      __forceinline void CFree(void *mem) {nedalloc::nedfree(mem);}
    };

    typedef FastDummyAlloc FastAllocType;
  #endif

  #define USE_FAST_ALLOCATOR_ID(x) USE_FAST_ALLOCATOR_ID_ALLOC(FastAllocType,x)
  #define DEFINE_FAST_ALLOCATOR_ID(className,x) DEFINE_FAST_ALLOCATOR_ID_ALLOC(FastAllocType,className,x)

  #define DEFINE_FAST_ALLOCATOR_TEMPLATE(tempString,className,fullName) \
          template <tempString> \
          FastAllocType fullName::_allocatorF = sizeof(typename className);
  #define DEFINE_FAST_ALLOCATOR_TEMPLATE2(decl1, decl2, className, def1, def2) \
          template <decl1, decl2> \
          FastAllocType className<def1, def2>::_allocatorF = sizeof(typename className);

#else
  #define USE_FAST_ALLOCATOR_ID(x)
  #define DEFINE_FAST_ALLOCATOR_ID(className,x)
  #define DEFINE_FAST_ALLOCATOR_TEMPLATE(tempString,className,fullName)
  #define DEFINE_FAST_ALLOCATOR_TEMPLATE2(decl1, decl2, className, def1, def2)
#endif

#define USE_FAST_ALLOCATOR USE_FAST_ALLOCATOR_ID(F)
#define DEFINE_FAST_ALLOCATOR(className) DEFINE_FAST_ALLOCATOR_ID(className,F)

#if defined __GNUC__ && __EXCEPTIONS
  #define NOEXCEPTION  noexcept(true)
#else
  #define NOEXCEPTION
#endif

void * CCALL operator new( size_t size, const char *file, int line );
void * CCALL operator new( size_t size );
void CCALL operator delete( void *ptr ) NOEXCEPTION;

#define USE_NORMAL_ALLOCATOR \
 \
  public: \
  void* operator new[] (size_t n){return ::operator new(n);} \
  void  operator delete[](void* ptr, size_t n){::operator delete(ptr);} \
  void* operator new (size_t n){return ::operator new(n);} \
  void  operator delete(void* ptr ){::operator delete(ptr);} \
  void* operator new (size_t n, const char *file, int line){return ::operator new (n,file,line);} \
  void operator delete(void* ptr, const char *file, int line){::operator delete(ptr);}

#else

// macro definitions for documentation purposes (Doxygen)

//! Use fast fixed size allocation (declare)
#define USE_FAST_ALLOCATOR public: FastCAlloc _allocator;
//! Use fast fixed size allocation (define)
#define DEFINE_FAST_ALLOCATOR(className)

//! Use fast fixed size allocation (declare with unique ID)
#define USE_FAST_ALLOCATOR_ID(x) public: FastCAlloc _allocator;
//! Use fast fixed size allocation (define with unique ID)
#define DEFINE_FAST_ALLOCATOR_ID(className,x)

//! Use global allocation
#define USE_NORMAL_ALLOCATOR public: NormalAlloc _allocator;

//! Normal new / delete operators used
/*!
  User to overload FastCAlloc allocators inherited from base class
*/

class NormalAlloc
{
};

#endif

#pragma warning(disable:4200)

#ifndef PAGE_ALIGNMENT_POSSIBLE
  #error PAGE_ALIGNMENT_POSSIBLE must be defined (should be defined in checkMem.hpp)
#endif

#if PAGE_ALIGNMENT_POSSIBLE && defined _WIN32
// without chunk pointers small allocations are much more effective
// but we need to be able to perform page aligned allocation for this
#define USE_CHUNK_POINTERS 0
#else
#define USE_CHUNK_POINTERS 1
#endif

#if _DEBUG
#define CHECK_HEAP_THREAD 0
# define CHECK_CHEAP_THREAD 1
#else
# define CHECK_HEAP_THREAD 0
# define CHECK_CHEAP_THREAD 0
#endif

// assume page-based allocation will be used, and we can therefore use whole regions
//enum {FastAllocChunkSize=8*1024};

//! Fast fixed block allocation
/*!
  Used as base class for different fixed block allocators
*/
class FastAlloc
{
#if CHECK_HEAP_THREAD
  int _unsafeHeapThread;
#endif

  public:
  //enum {chunkSize=FastAllocChunkSize};
  struct Chunk;

  FastAlloc( size_t n, const char *name="", int alignOffset=0, int pageSize=0 );
  ~FastAlloc();


  // only counted allocation left
  // uncounted is too problematic - leads to fragmentation

  //! free unused chunks
  void CleanUp();

  //! check which allocator - uses stored pointer, is able to detect normal allocated blocks
  static FastAlloc *WhichAllocator( void*pAlloc );
  //! check which allocator - may caused crash when block does no belong to any allocator
  static FastAlloc *WhichAllocatorUnsafe( void*pAlloc, size_t pageSize );

  //! allocate a block with counting
  void *AllocCounted( size_t n );
  //! free a block with counting
  void FreeCounted(void*pAlloc);

  const char *Name() const {return _name;}
  size_t ItemSize() const {return esize;}
  int ChunkSize() {return _chunkSize;}

  int CompareAllocatedSize(const FastAlloc &with) const
  {
    return allocated*esize-with.allocated*with.esize;
  }
  
  bool CheckIntegrity() const;

  protected:
  struct Link //: public CLDLink
  {
    Link *next; // next free in chunk
    Chunk *chunk; // which chunk is it in
  };
  struct ChunkHead
  {
    int null1; // null - to distinguish this block type
    FastAlloc *allocator; // which allocator this chunk serves for
    int allocated; // number of allocated items
    // align mem to 16 B boundary
    // each block will have short 4B description on the beginning
    Link *head;
    //int _align[1];
  };

  public:
  struct Chunk: public ChunkHead,public TLinkBidirD
  {
    //enum {size = chunkSize-sizeof(ChunkHead)-sizeof(TLinkBidirD)};
    char mem[];
    
    bool CheckIntegrity(int chunkSize) const;
  };


  protected:

  // no free items - add new chunk
  bool Grow();

  // remove specific chunks and all its items
  // there shoulb be no item allocated in chunk
  //virtual void ChunkRemove( Chunk *chunk );
  void ChunkRemove( Chunk *chunk );

  //!   
  // parent memory manager
  virtual Chunk *NewChunk();
  virtual void DeleteChunk( Chunk *chunk );

  // !destruction - free all chunks
  virtual void FreeChunks();

  //! if all blocks are deallocated, free all chunks
  virtual void ReleaseAll();

  TListBidir<Chunk,TLinkBidirD, SimpleCounter<Chunk> > chunksPart; // some elements are free - prefer using such link
  TListBidir<Chunk,TLinkBidirD, SimpleCounter<Chunk> > chunksFree; // all elements are free - can be released
  TListBidir<Chunk,TLinkBidirD, SimpleCounter<Chunk> > chunksBusy; // all elements are busy - kept for reference

  const char *_name; // for debugging purposes
  int _alignOffset; // align each chunk
  unsigned int esize;
  //! runtime determined chunk size for each allocation size
  int _chunkSize;
  //! number of elements in one chunk
  int _nElemPerChunk;

  //! how many elements are allocated in the whole allocator
  int allocated;
  
  /// for some allocators we want to perform manual cleanup only
  /** this is handy when allocator is releasing a lot of items and later allocating a lot of items */
  bool _manualCleanUp;

  friend void Damage_FastAlloc(FastAlloc *alloc);

  bool CheckIfFree(Chunk *chunk, void *data) const;
  /// size of _mem area
  int ChunkMemSize() const
  {
    // compensate for the fact before the mem there is a header
    return _chunkSize-offsetof(Chunk,mem);
  }

#if CHECK_HEAP_THREAD
  void AssertThread();
#endif

  public:
  void SetManualCleanUp(bool manual) {_manualCleanUp = manual;}
  
  size_t Requested() const; // how much was requested by this FastAllocs
  size_t Allocated() const; // how much was really allocated

};

//! Fast fixed block allocation with automatic cleanup
/*!
  User to define fast allocators for different classes (see USE_FAST_ALLOCATOR)
*/

class FastCAlloc: public FastAlloc
{
#if CHECK_CHEAP_THREAD
  int _cHeapThread;
#endif
  typedef FastAlloc base;

  public:
  FastCAlloc( size_t n, const char *name="" );
  ~FastCAlloc();

  void *CAlloc( size_t n );
  void CFree( void*pAlloc );

  //void ChunkRemove( Chunk *chunk );

  static void CleanUpAll(); // free unused chunks in all FastCAlloc instances
  static size_t TotalRequested(); ///< how much was requested by various FastAllocs
  static size_t TotalAllocated(); ///< how much was really allocated
  
  #if _ENABLE_REPORT
  static void ReportTotals(); ///< report memory allocations to debug output
  #endif

  private: // hide parent members
  static FastAlloc *WhichAllocator( void*pAlloc );
  void *AllocCounted(size_t n);
  void FreeCounted(void*pAlloc);

};

#include "debugNew.hpp"

#endif //sentry
