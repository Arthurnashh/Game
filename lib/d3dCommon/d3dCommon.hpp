/** Common classes for DirectX engines */

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _D3DCOMMON_HPP
#define _D3DCOMMON_HPP

#include "../engine.hpp"
#include <El/QStream/qbStream.hpp>
#if !_DISABLE_GUI

/// Vertex shader debugging switch
#if _ENABLE_CHEATS
extern bool DebugVS;
#else
const bool DebugVS=false;
#endif

/// enable debugging textures and render targets
//#define DEBUG_SURF

#if _ENABLE_PERFLOG && !defined _XBOX
#define DEBUG_PERFHUD
#endif

/// queues used for 2D rendering
enum
{
  MeshBufferLength=16*1024,
  IndexBufferLength=32*1024
};

#ifdef _XBOX
#define CUSTOM_VB_DISCARD 1
#else
#define CUSTOM_VB_DISCARD 1
#endif

#if CUSTOM_VB_DISCARD
const int NumDynamicVB3D = 4;
const int NumDynamicVB2D = 4;
#else
const int NumDynamicVB3D = 1;
const int NumDynamicVB2D = 1;
#endif

#ifdef _XBOX
//! Flag to control usage of dynamic buffers or their CPU equivalent (with DrawPrimitiveUP)
# define DONT_USE_DYNBUFFERS 1
#else
// dynamic buffer do not work well with predication
# define DONT_USE_DYNBUFFERS 1

#endif

#if defined(_XBOX) || defined(_X360SHADERGEN)
# define XBOX_D3D 1
#else
# define XBOX_D3D 0
#endif

struct VectorCompressed
{
  DWORD v;
};

struct FloatConstant
{
  float ar[4];

  float operator[] (int ix) const { return ar[ix]; }
  float &operator[] (int ix) { return ar[ix]; }
};

TypeIsSimple(FloatConstant);

struct IntConstant
{
  int ar[4];

  int operator[] (int ix) const { return ar[ix]; }
  int &operator[] (int ix) { return ar[ix]; }
};

TypeIsSimple(IntConstant);

struct FloatVSConstant
{
  float ar[4];

  FloatVSConstant() {};
  float operator[] (int ix) const { return ar[ix]; }
  float &operator[] (int ix) { return ar[ix]; }
};
TypeIsSimple(FloatVSConstant);

#if XBOX_D3D
//typedef void *HWND;
#ifdef _XBOX
typedef int RGNDATA;
#endif

typedef const D3DSURFACE_PARAMETERS *CreateRTLastParType;

#else

typedef HANDLE *CreateRTLastParType;
#endif

bool CheckValidIntegerConstant(const IntConstant *data, int registerCount);

/// set of float constants set in one call
template <int maxN=192>
struct FloatConstantsSet
{
  static const int maxNum = maxN;
  int num;
  /* values need to go last - sometimes truncated */
  FloatConstant val[maxNum];

  __forceinline FloatConstantsSet(){}
  __forceinline void operator = (const Array<FloatConstant> &src)
  {
    Assert(src.Size()<=maxNum);
    memcpy(val,src.Data(),src.Size()*sizeof(FloatConstant));
    num = src.Size();
  }

  __forceinline size_t GetUnusedBytes() const {return (maxNum-num)*sizeof(FloatConstant);}
};

/// set of int constants set in one call
struct IntConstantsSet
{
  static const int maxNum = 16;
  int num;
  /* values need to go last - sometimes truncated */
  IntConstant val[maxNum];

  IntConstantsSet(){}
  IntConstantsSet(const Array<IntConstant> &src)
  {
    Assert(src.Size()<=maxNum);
    memcpy(val,src.Data(),src.Size()*sizeof(IntConstant));
    num = src.Size();
  }
  __forceinline size_t GetUnusedBytes() const {return (maxNum-num)*sizeof(IntConstant);}
};


/// set of bool constants set in one call
template <class Type, int store>
struct StoredArray
{
  static const int maxNum = store;
  int num;
  /* values need to go last - sometimes truncated */
  Type val[maxNum];

  StoredArray(){}
  StoredArray(const Array<Type> &src)
  {
    Assert(src.Size()<=maxNum);
    // optimized for a common case of binary/simple types
    ConstructTraits<Type>::DestructArray(val,src.Size());
    ConstructTraits<Type>::CopyConstruct(val,src.Data(),src.Size());
    // for generic types plain loop might be more efficient, but we do not expect to be working with them
    //for (int i=0; i<src.Size(); i++) val[i] = src[i];
    num = src.Size();
  }
  __forceinline size_t GetUnusedBytes() const {return (maxNum-num)*sizeof(Type);}

  // this should be ok, because Array copy constructor copies only pointer
  operator Array<Type>() const {return Array<Type>((Type *)val, num);}

  const Type *Data() const {return val;}
  int Size() const {return num;}
};

/// set of bool constants set in one call
template <class Type, int store>
struct StoredArraySimple
{
  static const int maxNum = store;
  int num;
  /* values need to go last - sometimes truncated */
  Type val[maxNum];

  StoredArraySimple(){}
  StoredArraySimple(const Array<Type> &src)
  {
    Assert(src.Size()<=maxNum);
    for (int i=0; i<src.Size(); i++) val[i] = src[i];
    num = src.Size();
  }
  __forceinline size_t GetUnusedBytes() const {return (maxNum-num)*sizeof(Type);}

  // this should be ok, because Array copy constructor copies only pointer
  operator Array<Type>() const {return Array<Type>((Type *)val, num);}

  const Type *Data() const {return val;}
  int Size() const {return num;}
};


typedef StoredArray<BOOL,16> BoolConstantsSet;

typedef StoredArray<Vertex2DAbs,6> StoredArrayPolyAbs;
typedef StoredArray<Vertex2DPixel,6> StoredArrayPolyPixel;

/*
/// set of bool constants set in one call
struct BoolConstantsSet
{
static const int maxNum = 16;
int num;
/ * values need to go last - sometimes truncated * /
BOOL val[maxNum];

BoolConstantsSet(){}
BoolConstantsSet(const Array<BOOL> &src)
{
Assert(src.Size()<=maxNum);
memcpy(val,src.Data(),src.Size()*sizeof(BOOL));
num = src.Size();
}
__forceinline size_t GetUnusedBytes() const {return (maxNum-num)*sizeof(BOOL);}
};
*/

/// store comref with no reference counting - plain pointer with a ComRef like interface
/** we use this to store references we know that are permanent */
template <class Type>
class ComRefFake: public InitPtr<Type>
{
public:
  /// releasing fake ref does not change ref. count, only sets to NULL

  ComRefFake(){}
  ComRefFake(Type *src):InitPtr<Type>(src){}
  void operator = (Type *src){InitPtr<Type>::operator =(src);}
  void Free(){InitPtr<Type>::operator =(NULL);}
};

#ifndef _XBOX
# define SAFE_RESOURCE_UNSET 1
/*
on PC safe unset not needed - set resources are AddRef-ed by the device.
Still, using Release on the main thread only prevents possible problems with D3D CS contention.
*/
#else
# define SAFE_RESOURCE_UNSET 1 // on Xbox safe unset is always needed
#endif


struct FallbackNew
{
  static void *Alloc(size_t size) {return new char[size];}
  static void Free(void *mem) {return delete[] (char *)mem;}
};

#if _XBOX
struct FallbackPhys
{
  static void *Alloc(size_t size)
  {
    return XPhysicalAlloc(size, MAXULONG_PTR, 0, PAGE_READWRITE|PAGE_WRITECOMBINE);
  }
  static void Free(void *mem) {XPhysicalFree(mem);}
};
#endif

/// simple "growing" allocator
/**
Allocating from a pre-allocated heap, growing only, Free is not returning any memory into the pre-allocated heap
MT safe, lock free and wait free
*/
template <class Fallback=FallbackNew>
class SimpleAllocatorMT
{
private:
  //@{ memory backup
  char *_memory;
  int _size;
  //@}
  /// how much is already allocated
  AtomicInt _used;
  /// buffer can be used as circular, freeing from the beginning
  /** note: freeing is not MT safe */
  int _freeTo;

#if _DEBUG || _PROFILE
#define DEBUG_COUNT_ALLOC 1
#else
#define DEBUG_COUNT_ALLOC 0
#endif
#if DEBUG_COUNT_ALLOC
  /// a simple way do detect existence of leak (missing Free for Allocate)
  AtomicInt _allocationsCount;
  /** it is possible we access this only from main thread, but having it MT safe does little harm */
  AtomicInt _allocationsAmount;
#endif

  /// accumulate all memory allocated and released
  InitVal<int,0> _memoryTraffic;

public:
  SimpleAllocatorMT()
  {
    _used = AtomicInt(0);
    _freeTo = 0; // modulo arithmetics - same as "whole free"
#if DEBUG_COUNT_ALLOC
    _allocationsCount = AtomicInt(0);
    _allocationsAmount = AtomicInt(0);
#endif
  }

  void SetMemory(void *memory, int size)  
  {
    _size = size;
    _freeTo = size;
    _memory = (char *)memory;
    // we are using x&size to implement cyclical buffer - check if size meets requirements
    DoAssert(roundTo2PowerNCeil(_size)==_size || _size==0);
  }
  void *GetMemory() const {return _memory;}


  void *Allocate(int size)
  {
#if DEBUG_COUNT_ALLOC
    _allocationsCount++;
    _allocationsAmount += size;
#endif

    Assert(size>0 && size<_size);
Retry:
    // note: += returns new value, but underlying InterlockedExchangeAdd returns original value
    int newEnd = (_used += size);
    // TODO: check the compiler optimizes +size-size away
    int newStart = newEnd-size;

    // allow circular manipulation
    int newEndCircular = newEnd&(_size-1);
    int newStartCircular = newStart&(_size-1);
    // check if we looped around
    if (newEndCircular<newStartCircular)
    {
      // we can never go back. Once we have crossed the boundary, we need to allocate once more
      // check if we have any chance to fit into the free region
      if (newEnd+size<_freeTo)
      {
        goto Retry;
      }
    }
    else
    {
      // newStart..newEnd
      // free region is newStart.._free, _free is always behind the start
      // verify we fit into the free region
      int free = _freeTo-newEnd; // size of the free region
      // if there is still some memory free after the allocation, we may use it
      if (free>0)
      {
        return _memory+newStartCircular;
      }
    }
    LogF("SimpleAllocatorMT not large enough (%d, alloc %d, needed %d)",_size,size,_size-(_freeTo-newEnd));
    // 
    // once we fail, we always fail (and use dynamic allocator) - this keeps implementation simple
    // we could also track back on failure, some allocation might fail meanwhile, but successive ones might succeed again
    // another alternative could be a lock-free (but not wait-free) loop
    // however, as we do not expect the failures to happen at all, it is not worth implementing
    return Fallback::Alloc(size);
  }

  /** free is still needed because of fallback */
  void Free(void *mem, int size)
  {
#if DEBUG_COUNT_ALLOC
    _allocationsCount--;
    _allocationsAmount-=size;
    // when count is zero, amount needs to be zero as well
    Assert(_allocationsCount>0 || _allocationsAmount==0);
    DoAssert(_allocationsCount>=0);
#endif
    // when freeing the memory allocated by new, we must delete it
    if (mem<_memory || mem>=_memory+_size)
    {
      Fallback::Free(mem);
    }
  }

  void Clear(int amount)
  {
    // move the free boundary
    _freeTo += amount;
    _memoryTraffic += amount;
  }

  /* not MT safe - may be called only when other threads are not accessing the allocator */
  int Compact()
  {
    // the caller is currently calling this only when expecting all allocations are flushed
#if DEBUG_COUNT_ALLOC
    DoAssert(_allocationsCount==0);
    DoAssert(_allocationsAmount==0);
#endif
    // if whole buffer is free, we may reset the buffer
    // ... and we know the buffer is free, otherwise nobody should call the Compact
    //if (_freeTo==_used+_size)
    if (true)
    {
      // verify there is nothing left
      int adjust = -_used;
      _used = AtomicInt(0);
      _freeTo = _size;
      return adjust;
    }
    else
    {
      // both numbers need to move the same amount, so that _freeTo does not chase in front of _used
      // as _freeTo goes behind _used (and is therefore smaller), we always modify based on it
      DoAssert(_freeTo>_used);
      int newUsed = _used&(_size-1);
      int adjust = newUsed-_used;
      _freeTo += adjust;
      _used = AtomicInt(newUsed);
      DoAssert(_freeTo>_used);
      return adjust;;
    }
  }

  /// return memory allocation measurements results and reset them
  /** the stats are maintained by Compact, which is responsible for reclaiming the memory */
  int AllocationTurnover()
  {
    int ret = _memoryTraffic;
    _memoryTraffic = 0;
    return ret;
  }

  int MaxSize() {return _size;}

  int Allocated() const {return _size-(_freeTo-_used);}
  int UsedPtr() const {return _used;}
};

/// Coefficient to scale by the content of an environmental map (to represent bigger range of values)
static const float EnvMapScale = 1.0f;

#if _PROFILE || _DEBUG
#define PIX_NAMED_CB_SCOPES 1
#endif


#include <limits>


template <class Numeric, int age, int cluster>
class MovingHistory
{
  static const int NClusters = (age+cluster-1)/cluster;
  Numeric _clustersMax[NClusters];
  Numeric _clustersMin[NClusters];
  int _clusterAge;

public:
  MovingHistory()
  {
    _clusterAge = 0;
    for (int i=0; i<NClusters; i++) _clustersMax[i] = 0, _clustersMin[i] = 0;
  }
  void AddValue(Numeric value)
  {
    if (++_clusterAge>=cluster)
    {
      for (int i=0; i<NClusters-1; i++)
      {
        _clustersMax[i] = _clustersMax[i+1];
        _clustersMin[i] = _clustersMin[i+1];
      }
      _clustersMax[NClusters-1] = +std::numeric_limits<Numeric>::min();
      _clustersMin[NClusters-1] = +std::numeric_limits<Numeric>::max();
      _clusterAge = 0;
    }
    if (value>_clustersMax[NClusters-1]) _clustersMax[NClusters-1] = value;
    if (value<_clustersMin[NClusters-1]) _clustersMin[NClusters-1] = value;
  }

  Numeric Min() const
  {
    Numeric ret = +std::numeric_limits<Numeric>::max();
    for (int i=0; i<NClusters; i++) ret = min(ret,_clustersMin[i]);
    return ret;
  }
  Numeric Max() const
  {
    Numeric ret = -std::numeric_limits<Numeric>::max();
    for (int i=0; i<NClusters; i++) ret = max(ret,_clustersMax[i]);
    return ret;
  }
};


//! Maximum number of UV sets
#define MAX_UV_SETS 2

#if defined _M_PPC && defined _MATH3DK_HPP
typedef XMVECTOR MatrixFloat34[3];
#else
typedef float MatrixFloat34[3][4];
#endif

//! Informations related to one section
struct VBSectionInfo
{
  int beg,end;
  int begVertex,endVertex; // used vertex range
};
TypeIsSimple(VBSectionInfo);

// Type of Texture coordinate generation
enum ETTG {
  TTG_UNASSIGNED = -1,
  TTG_MATRIX = 0,
  TTG_COPY = 1
};

//! Stencil shadow drawing mode
enum EShadowState
{
  SS_None,
  /// transparent - does not change z buffer, does not change stencil buffer
  SS_Transparent,
  /// ordinary model, can receive stencil shadows
  SS_Receiver,
  /// ordinary model, can not receive stencil shadows
  SS_Disabled,
  /// front facing part of the shadow volume
  SS_Volume_Front,
  /// back facing part of the shadow volume
  SS_Volume_Back,
  //! Shadow buffer rendering
  SS_ShadowBuffer,
};

//! Enum to determine what SB technique we can use on present HW
/*!
\patch 5151 Date 3/29/2007 by Flyman
- Fixed: Crash when trying to use HW without possibility to use shadow buffer technique (observed on Intel on board graphics card)
*/
enum SBTechnique
{
  SBT_Default,  // D3DFMT_R32F format of texture, PS calculation
  SBT_nVidia,   // nVidia solution - rendering to depth buffer D3DFMT_D24S8
  SBT_nVidiaINTZ, // nVidia solution - rendering to depth buffer D3DFMT_D24S8, using INTZ
  SBT_ATIDF16,  // ATI DF16 solution
  SBT_ATIDF24,  // ATI DF24 solution (can be Fetch4, not yet implemented)
  SBT_None,     // No SB shadows can be used
  SBTCount
};

//! Variation of the same shader for different purposes
enum PixelShaderType
{
  PSTBasic,                 // Ordinary PS - no shadows
  PSTBasicSSSM,             // PS with SSSM shadows
  PSTShadowReceiver1,       // PS with shadows - kernel size 1
  PSTShadowCaster,          // PS simplified as a caster
  PSTInvDepthAlphaTest,     // PS to write inverse of depth value including alpha kill for alpha-test
  PSTThermal,               // Thermal vision
  PSTAlphaOnly,             // PS to get accurate alpha - used for z-priming pass
  NPixelShaderType,
  PSTUninitialized = -1
};

//! Rendering modes
enum RenderingMode
{
  RMCommon,
  RMShadowBuffer,
  RMZPrime,
  RMDPrime,
  RMCrater,
  RMCaustics,
  NRenderingMode
};

// 8b stencil layout
#define STENCIL_FULL 0xff
#define STENCIL_COUNTER 0xff // 5 bits counter
#define STENCIL_SHADOW_LEVEL_MASK 0x00
#define STENCIL_SHADOW_LEVEL_STEP 0x00

#define ENABLE_NVG_NOISE 1


static inline int MemCmp32(const void *m1, const void *m2, int dwordCount)
{
  const int *mem1 = (const int *)m1;
  const int *mem2 = (const int *)m2;
  while (--dwordCount>=0)
  {
    if (*mem1++!=*mem2++)
    {
      return 1;
    }
  }
  return 0;
}

static inline int Clip32B(long long x)
{
  if (x>INT_MAX) return INT_MAX;
  return (int)x;
}

static inline int Clip32BUnsigned(long long x)
{
  if (x>UINT_MAX) return UINT_MAX;
  return (unsigned)x;
}

struct LazyLoadFile
{
  virtual const char *Load(int &size) const = 0;
};

//! Stream with ready data - used for shader loading
class QOStrStreamData : public QOStrStream
{
public:
  void CopyFile(const char *fileName)
  {
    QIFStreamB in;
    in.AutoOpen(fileName);
    in.copy(*this);
    in.close();
    GFileServerFunctions->FlushReadHandle(fileName);
  }
  QOStrStreamData(){}
  QOStrStreamData(const char *fileName)
  {
    CopyFile(fileName);
  }
};

#endif //!_DISABLE_GUI
#endif //_D3DCOMMON_HPP