#ifdef _MSC_VER
#pragma once
#endif

#ifndef __ENGDDT_HPP
#define __ENGDDT_HPP

#if !_DISABLE_GUI && _ENABLE_DX10

// implementation of DirectDraw engine
// without actual triangle drawing
// so that different implementations (including Direct3D) are possible

#include "d3dDefsT.hpp"

#include <Es/Containers/array.hpp>
#include <Es/Containers/staticArray.hpp>
#include <Es/Containers/smallArray.hpp>
#include <El/FreeOnDemand/memFreeReq.hpp>
#include "../types.hpp"
#include "../engine.hpp"
#include "../tlVertex.hpp"
#include "../Shape/plane.hpp"
#include "pushBufferT.hpp"



enum TexLoc {TexLocalVidMem,TexNonlocalVidMem,TexSysMem};

enum
{
  MeshBufferLength=32*1024,
  IndexBufferLength=4*1024
};

#define CUSTOM_VB_DISCARD 0

#if CUSTOM_VB_DISCARD
const int NumDynamicVB3D = 2;
const int NumDynamicVB2D = 2;
#else
const int NumDynamicVB3D = 1;
const int NumDynamicVB2D = 1;
#endif

enum PixelShaderSpecular
{
  PSSSpecular,
  PSSNormal,
  NPixelShaderSpecular,
  PSSUninitialized = -1
};

//! Variation of the same shader for different purposes
enum PixelShaderType
{
  PSTBasic,                 // Ordinary PS - no shadows
  PSTBasicNA,               // Ordinary PS - no shadows, no alpha shadows
  PSTShadowReceiver1,       // PS with shadows - kernel size 1
  PSTShadowCaster,          // PS simplified as a caster
  PSTInvDepthAlphaTest,     // PS to write inverse of depth value including alpha kill for alpha-test
  NPixelShaderType,
  PSTUninitialized = -1
};

//! Rendering modes
enum RenderingMode
{
  RMCommon,
  RMShadowBuffer,
  RMDepthMap,
};

//! Shadow buffer size
const int ShadowBufferSize = 2048;
const int ShadowBufferSizeHQ = 4096;

// several queues for different textures
// other states must be same

class TexMaterial;
class TextureD3DT;

struct TriQueueT
{
  //int _queueBase,_queueSize; // _triangleQueue position in VB

  StaticArray<WORD> _triangleQueue; // see BeginMesh,EndMesh

  TextureD3DT *_texture; // which texture
  const TexMaterial *_material; // what material
  int _level; // which level
  int _special; // drawing flags
  int _lastUsed; // used counter
};


enum
{
  //MaxTriQueues=32, // 16 - 32 seems to be reasonable
  // triangle queues have very little effect with HW T&L
  // as they are primarily used to optimize 3D operation across several objects
  // which is not possible with HW T&L
  // they have little effect on 2D operation, but they also make FlushAllQueues much slower
  MaxTriQueues=1, // 16 - 32 seems to be reasonable
  TriQueueSize=2048
};

//! Structure to hold vertices for nonTL drawing
struct QueueT
{
  //! Vertex buffer
  ComRef<IDirect3DVertexBuffer9Old> _meshBuffer[NumDynamicVB2D];
  //! Index buffer
  ComRef<IDirect3DIndexBuffer9Old> _indexBuffer[NumDynamicVB2D];
  //! How many vertices are in vertex buffer
  int _vertexBufferUsed;
  //! How many indices are in index buffer
  int _indexBufferUsed;

  int _meshBase,_meshSize; // _mesh position in VB
  int _actualVBIndex;
  int _actualIBIndex;

  TriQueueT _tri[MaxTriQueues];
  bool _triUsed[MaxTriQueues];
  // queue 0 is used only for alpha rendering
  int _actTri; // queue assigned by PrepareTriangle call

  int _usedCounter; // used for LRU tracking
  bool _firstVertex; // first call in scene should always use DISCARD
  bool _firstIndex;

  //! Constructor
  QueueT();
  int Allocate
  (
    TextureD3DT *tex, int level, const TexMaterial *mat, int spec,
    int minI, int maxI, int tip
  );
  void Free( int i );

  const ComRef<IDirect3DVertexBuffer9Old> &GetVBuffer() const {return _meshBuffer[_actualVBIndex];}
  const ComRef<IDirect3DIndexBuffer9Old> &GetIBuffer() const {return _indexBuffer[_actualIBIndex];}
};

/// Vertex shader debugging switch
#if _ENABLE_CHEATS
  extern bool DebugVS;
#else
  const bool DebugVS=false;
#endif

/// enable debugging textures and render targets
//#define DEBUG_SURF

#ifdef _ENABLE_PERFLOG
#define DEBUG_PERFHUD
#endif

/// simulate Xbox "fence" on PC
/**
Two implementations possible:
advanced and accurate using query
very simple using "safe estimate" -
  assume buffer is locked for given number of frames since used
*/
class ResourceBusyT
{
  /// frame in which the fence will still be busy
  int _busyUntil;

  public:
  ResourceBusyT():_busyUntil(0){};

  /// start fence - resource is used
  void Used();
  /// check if resource is still used
  bool IsBusy() const;
  /// check if resource is still untouched
  bool WasNeverUsed() const {return _busyUntil==0;}
};

struct SVertexT;

class VertexDynamicDataT
{
  friend class EngineDDT;

  ComRef<IDirect3DVertexBuffer9Old> _vBuffer[NumDynamicVB3D];
  int _vBufferSize; // reserved sizes of resources
  int _vBufferUsed; // used sizes
  int _actualVBIndex;
  
  const ComRef<IDirect3DVertexBuffer9Old> &GetVBuffer() const
  {
    return _vBuffer[_actualVBIndex];
  }

public:
  VertexDynamicDataT();

  void Init(int size, ID3D10Device *dev);
  void Dealloc();

  int AddVertices(const VertexTableAnimationContext *animContext, const Shape &src);
  //! Allocates space for n vertices and returns pointer to the vertices array to be filled out
  int Lock(SVertexT *&vertex, int n);
  //! Unlocks the vertex buffer
  void Unlock();
};

#include "../heap.hpp"

typedef unsigned int HeapIndex;

typedef Heap<HeapIndex,int> IndexHeapT;
typedef Heap<HeapIndex,int> VertexHeapT;

typedef IndexHeapT::HeapItem IndexAllocT;
typedef VertexHeapT::HeapItem VertexAllocT;

/// index buffer encapsulation
class IndexStaticDataT: public RefCountWithLinks
{
  friend class EngineDDT;
private:
  //! Managed index buffer
  ComRef<IDirect3DIndexBuffer9Old> _iBuffer;
  //! Pointer to engine
  EngineDDT *_engine;
  //! Data manager
  IndexHeapT _manager;
  //! Number of shapes inside the buffer
  int _nShapes;
  /// busy tracker
  ResourceBusyT _busy;
public:
  //! Constructor
  IndexStaticDataT();
  //! Destructor
  ~IndexStaticDataT();
  //! Initializing
  /*!
    \param engine Pointer to engine
    \param indices Required number of indices in the buffer
    \param dev Pointer to 3D device
  */
  void Init(EngineDDT *engine, int indices, ID3D10Device *dev);
  //! Deallocating of data
  /*!
    Frees the vertex buffer and informs the engine about that
  */
  void Dealloc();

  //! Calculates how many indices the shape will occupy (as a triangle list)
  static int IndicesNeededTotal(const Shape &src, int nInstances);
  //! Calculates how many indices the shape will occupy (as a triangle strip)
  static int IndicesNeededStrippedTotal(const Shape &src, int nInstances);
  //! Add shape data to this buffer
  /*!
    \param src Shape to add
    \param iIndex Information about position and size of the data in the index buffer
    \param dynamic Method is working only if "dynamic == false" (we are working with static data)
    \return True if method succeeded, false elsewhere
  */
  bool AddShape(const Shape &src, IndexAllocT *&iIndex, int indicesTotal, bool dynamic, int nInstances);
  //! Add shape data as a strip to this buffer
  /*!
  \param src Shape to add
  \param iIndex Information about position and size of the data in the index buffer
  \param dynamic Method is working only if "dynamic == false" (we are working with static data)
  \return True if method succeeded, false elsewhere
  */
  bool AddShapeStripped(const Shape &src, IndexAllocT *&iIndex, int indicesTotal, bool dynamic, int nInstances);
  //! Add shape data as sprites to this buffer
  /*!
  \param src Shape to add
  \param iIndex Information about position and size of the data in the index buffer
  \param dynamic Method is working only if "dynamic == false" (we are working with static data)
  \return True if method succeeded, false elsewhere
  */
  bool AddShapeSprite(const Shape &src, IndexAllocT *&iIndex, int indicesTotal, bool dynamic, int nInstances);
  //! Removes specified shape from the heap associated with the index buffer
  /*!
    \param iIndex Item of the heap to remove
  */
  void RemoveShape(IndexAllocT *iIndex);
  //! Check if given buffer is empty and can be deallocated
  bool IsEmpty() const {return _nShapes==0;}

  /// check if buffer is currently used by GPU
  bool IsBusy() const {return _busy.IsBusy();}

  void Used(){_busy.Used();}
};

class VertexBufferD3DT;

/// vertex buffer encapsulation
class VertexStaticDataT: public RefCountWithLinks
{
  friend class EngineDDT;
private:
  //! Data manager
  VertexHeapT _manager;
  //! Pointer to engine
  EngineDDT *_engine;
  //! Size of one vertex in bytes
  int _vertexSize;
  //! Number of shapes inside the buffer
  int _nShapes;
  /// busy tracker
  ResourceBusyT _busy;
  /// track if dynamic usage was used
  bool _usageDynamic;

  //!{ Locking and unlocking vertex data
  void *Lock(const VertexHeapT::HeapItem *item, int nVertex, int vertexSize);
  void Unlock();
  //!}

  //!{ Fills out the data with the shape (specialized version for each data type required)
  template <int VertexDecl>
  void AddShapeData(void *data, const Shape &src, const VertexBufferD3DT *vbuf);
  template <int VertexDecl>
  void AddShapeInstancedData(void *data, const Shape &src, const VertexBufferD3DT *vbuf, int instancesCount);
  //!}

public:
  //! Managed vertex buffer
  ComRef<IDirect3DVertexBuffer9Old> _vBuffer;
  //! Constructor
  VertexStaticDataT();
  //! Destructor
  ~VertexStaticDataT();
  //! Initializing
  void Init(EngineDDT *engine, int vertices, int vertexSize, bool frequent, ID3D10Device *dev);
  //! Deallocating of data
  /*!
    Frees the vertex buffer and informs the engine about that
  */
  void Dealloc();

  //!{ Add shape data to this buffer (and instanced version)
  /*!
    \param src Shape to add
    \param vIndex Information about position and size of the data in the vertex buffer
    \param dynamic Method is working only if "dynamic == false" (we are working with static data)
    \return True if method succeeded, false elsewhere
  */
  template <int VertexDecl>
  bool AddShape(const Shape &src, VertexBufferD3DT *vbuf, bool dynamic);
  template <int VertexDecl>
  bool AddShapeInstanced(const Shape &src, VertexBufferD3DT *vbuf, bool dynamic, int instancesCount);
  //!}

  //! Sprite has an own function to fill out the data
  bool AddShapeSpriteInstanced(const Shape &src, VertexBufferD3DT *vbuf, bool dynamic, int instancesCount);

  //!{ Skinned instanced version
#if _ENABLE_SKINNEDINSTANCING
  bool AddShapeSkinnedInstanced(const Shape &src, VertexHeapT::HeapItem *&vIndex, bool dynamic, int instancesCount);
  bool AddShapeNormalSkinnedInstanced(const Shape &src, VertexHeapT::HeapItem *&vIndex, bool dynamic, int instancesCount);
  bool AddShapeShadowVolumeSkinnedInstanced(const Shape &src, VertexHeapT::HeapItem *&vIndex, bool dynamic, int instancesCount);
#endif
  //!}
  //! Removes specified shape from the heap associated with the vertex buffer
  /*!
    \param vIndex Item of the heap to remove
  */
  void RemoveShape(VertexHeapT::HeapItem *vIndex);
  //! Returns number of allocated vertices
  /*!
    \return Number of allocated vertices
  */
  int Size() {return _manager.Size();};
  //! Returns size of the vertex in bytes
  /*!
    \return Size of the vertex in bytes
  */
  int VertexSize() {return _vertexSize;};
  //! Check if given buffer is empty and can be deallocated
  bool IsEmpty() const {return _nShapes==0;}

  /// check if buffer is currently used by GPU
  bool IsBusy() const {return _busy.IsBusy();}

  void Used(){_busy.Used();}
};

// ------------------------
// Shader constants factory
/*
// The factory
#define CONSTANTS_LIST(XX) \
  XX(proj_matrix) \
  XX(view_matrix) \
                     XX(view_matrix_1)  XX(view_matrix_2)  XX(view_matrix_3)  XX(view_matrix_4)  XX(view_matrix_5)  XX(view_matrix_6)  XX(view_matrix_7)  XX(view_matrix_8)  XX(view_matrix_9) \
  XX(view_matrix_10) XX(view_matrix_11) XX(view_matrix_12) XX(view_matrix_13) XX(view_matrix_14) XX(view_matrix_15) XX(view_matrix_16) XX(view_matrix_17) XX(view_matrix_18) XX(view_matrix_19) \
  XX(view_matrix_20) XX(view_matrix_21) XX(view_matrix_22) XX(view_matrix_23) XX(view_matrix_24) XX(view_matrix_25) XX(view_matrix_26) XX(view_matrix_27) XX(view_matrix_28) XX(view_matrix_29) \
  XX(view_matrix_30) XX(view_matrix_31) XX(view_matrix_32) \
  XX(matrix_offset) \
  XX(A_Dforced_E) \
  XX(LDirection_D) \
  XX(LDirection_S) \
  XX(LDirection_TransformedDir) \
  XX(LPoint_TransformedPos1)  XX(LPoint_TransformedPos2)  XX(LPoint_TransformedPos3)  XX(LPoint_TransformedPos4)  XX(LPoint_TransformedPos5)  XX(LPoint_TransformedPos6)  XX(LPoint_TransformedPos7)  XX(LPoint_TransformedPos8) \
  XX(LPoint_Atten1)           XX(LPoint_Atten2)           XX(LPoint_Atten3)           XX(LPoint_Atten4)           XX(LPoint_Atten5)           XX(LPoint_Atten6)           XX(LPoint_Atten7)           XX(LPoint_Atten8) \
  XX(LPoint_D1)               XX(LPoint_D2)               XX(LPoint_D3)               XX(LPoint_D4)               XX(LPoint_D5)               XX(LPoint_D6)               XX(LPoint_D7)               XX(LPoint_D8) \
  XX(LPoint_A1)               XX(LPoint_A2)               XX(LPoint_A3)               XX(LPoint_A4)               XX(LPoint_A5)               XX(LPoint_A6)               XX(LPoint_A7)               XX(LPoint_A8) \
  XX(LSpot_TransformedPos1) XX(LSpot_TransformedPos2) XX(LSpot_TransformedPos3) XX(LSpot_TransformedPos4) XX(LSpot_TransformedPos5) XX(LSpot_TransformedPos6) XX(LSpot_TransformedPos7) XX(LSpot_TransformedPos8) \
  XX(LSpot_TransformedDir1) XX(LSpot_TransformedDir2) XX(LSpot_TransformedDir3) XX(LSpot_TransformedDir4) XX(LSpot_TransformedDir5) XX(LSpot_TransformedDir6) XX(LSpot_TransformedDir7) XX(LSpot_TransformedDir8) \
  XX(LSpot_CosPhiHalf1)     XX(LSpot_CosPhiHalf2)     XX(LSpot_CosPhiHalf3)     XX(LSpot_CosPhiHalf4)     XX(LSpot_CosPhiHalf5)     XX(LSpot_CosPhiHalf6)     XX(LSpot_CosPhiHalf7)     XX(LSpot_CosPhiHalf8) \
  XX(LSpot_CosThetaHalf1)   XX(LSpot_CosThetaHalf2)   XX(LSpot_CosThetaHalf3)   XX(LSpot_CosThetaHalf4)   XX(LSpot_CosThetaHalf5)   XX(LSpot_CosThetaHalf6)   XX(LSpot_CosThetaHalf7)   XX(LSpot_CosThetaHalf8) \
  XX(LSpot_InvCTHMCPH1)     XX(LSpot_InvCTHMCPH2)     XX(LSpot_InvCTHMCPH3)     XX(LSpot_InvCTHMCPH4)     XX(LSpot_InvCTHMCPH5)     XX(LSpot_InvCTHMCPH6)     XX(LSpot_InvCTHMCPH7)     XX(LSpot_InvCTHMCPH8) \
  XX(LSpot_Atten1)          XX(LSpot_Atten2)          XX(LSpot_Atten3)          XX(LSpot_Atten4)          XX(LSpot_Atten5)          XX(LSpot_Atten6)          XX(LSpot_Atten7)          XX(LSpot_Atten8) \
  XX(LSpot_D1)              XX(LSpot_D2)              XX(LSpot_D3)              XX(LSpot_D4)              XX(LSpot_D5)              XX(LSpot_D6)              XX(LSpot_D7)              XX(LSpot_D8) \
  XX(LSpot_A1)              XX(LSpot_A2)              XX(LSpot_A3)              XX(LSpot_A4)              XX(LSpot_A5)              XX(LSpot_A6)              XX(LSpot_A7)              XX(LSpot_A8) \
  XX(specularPower_alpha_fogEnd_rcpFogEndMinusFogStart) \
  XX(texTransform0) XX(texTransform1)

// The enum
#define CONSTANTS_LIST_ENUM_ITEM(item) item,
enum EConstantsList
{
  CONSTANTS_LIST(CONSTANTS_LIST_ENUM_ITEM)
  NEConstantsList
};
*/

// Type of Texture coordinate generation
enum ETTG {
  TTG_UNASSIGNED = -1,
  TTG_MATRIX = 0,
  TTG_COPY = 1
};

/// track VRAM / system RAM separately at the same time

struct MemSizeVRAMSys
{
  size_t _vram;
  size_t _sys;
  
  MemSizeVRAMSys():_vram(0),_sys(0){}
  MemSizeVRAMSys(int vram, int sys):_vram(vram),_sys(sys){}
  
  void operator += (const MemSizeVRAMSys &add)
  {
    _vram += add._vram;
    _sys += add._sys;
  }
  
  void operator -= (const MemSizeVRAMSys &add)
  {
    _vram -= add._vram;
    _sys -= add._sys;
  }
  
  size_t GetSysSize() const {return _sys;}
  size_t GetVRAMSize() const {return _vram;}
};

/// traits for LinkBidirWithFrameStoreT using MemSizeVRAMSys
struct FrameStoreMemSizeVRAMSys
{
  typedef MemSizeVRAMSys MemSize;
  static bool IsEmpty(MemSize size) {return size.GetSysSize()==0 && size.GetVRAMSize()==0;}
};
/// proxy class used to avoid necessity to declare VertexBufferD3D here
class VertexBufferD3DTLRUItem:
  public LinkBidirWithFrameStoreT<FrameStoreMemSizeVRAMSys>
{
  friend class EngineDDT;
  /// if shape is destroyed, vertex buffer is always destroyed with it
  /**
  owner should call Detach(owner} before destruction
  this is verified in destructor
  */
  const Shape *_owner;

public:
  VertexBufferD3DTLRUItem() {_owner = NULL;}
  ~VertexBufferD3DTLRUItem() {DoAssert(_owner==NULL);}
  virtual MemSizeVRAMSys GetMemoryControlled() const = NULL;

protected:
  void SetOwner(const Shape *owner){_owner = owner;}
  const Shape *GetOwner() const {return _owner;}
};

/// query manager
class QueryIdT
{
  // Direct3D 10 NEEDS UPDATE

// 
//   /// Xbox HW limit is D3DVISIBILITY_TEST_MAX - 16K, but we do not need that much
//   enum {MaxQueries=1024};
//   /// recycle handles
//   ComRef<IDirect3DQuery9> _query[MaxQueries];
//   /// check which queries are currently in use
//   bool _used[MaxQueries];
//   /// last one we have used - helps to find the next free one fast
//   int _curUsed;
// 
public:
  QueryIdT();

  int Open();
  void Close(int id);
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
  /// old-fasioned (projected) shadow
  SS_Ordinary,
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
  SBT_ATIDF16,  // ATI DF16 solution
  SBT_ATIDF24,  // ATI DF24 solution (can be Fetch4, not yet implemented)
  SBT_None,     // No SB shadows can be used
  SBTCount
};

typedef float FloatConstant[4];
TypeIsSimple(FloatConstant);

typedef float MatrixFloat34[3][4];

//! Informations related to one section
struct VBSectionInfo
{
  int beg,end;
  int begVertex,endVertex; // used vertex range
};
TypeIsSimple(VBSectionInfo);

//! Maximum number of UV sets
#define MAX_UV_SETS 2

//! Specialized version of vertex buffer
class VertexBufferD3DT: public VertexBuffer, public VertexBufferD3DTLRUItem
{
  friend class EngineDDT;
  friend class PBIDrawIndexedPrimitiveT;
  friend class VertexStaticDataT;

private:

  bool _separate;
  bool _dynamic;

  AutoArray<VBSectionInfo> _sections;
  IndexAllocT *_iAlloc;
  VertexAllocT *_vAlloc;
  int _iOffset; // where are indices placed in shared static buffer
  int _vOffset; // vertex index offsetting done by D3D

  //! Maximum UV values the vertex buffer contains (suitable for UV compression) for each stage
  UVPair _maxUV[MAX_UV_SETS];
  //! Minimum UV values the vertex buffer contains (suitable for UV compression) for each stage
  UVPair _minUV[MAX_UV_SETS];

  // following variables make sense only for "separate" vertex buffers
  ComRef<IDirect3DVertexBuffer9Old> _vBuffer;
  ComRef<IDirect3DIndexBuffer9Old> _iBuffer;
  int _iBufferSize;
  int _vBufferSize;
  //int _vBufferVertexSize;

  // following are valid only for "shared" (!_separate)
  Link<VertexStaticDataT> _sharedV;
  Link<IndexStaticDataT> _sharedI;

protected:
  void CopyData( const Shape &src );
  void AllocateIndices(EngineDDT *engine, int indices);
  void AllocateVertices(EngineDDT *engine, int vertices);

public:
  VertexBufferD3DT();
  ~VertexBufferD3DT();

  bool Init(EngineDDT *engine, const Shape &src, VBType type, bool frequent);

  //@{ implementation of VertexBuffer
  virtual void Update(const VertexTableAnimationContext *animContext, const Shape &src, const EngineShapeProperties &prop, VSDVersion vsdVersion);
  virtual void Detach(const Shape &src);
  virtual void RemoveIndexBuffer();
  virtual bool IsGeometryLoaded() const;
  virtual void OnSizeChanged();
  virtual size_t GetBufferSize() const;
  //@}

  //! Get number of vertices in the buffer
  int GetVBufferSize() const
  {
    return _vAlloc ? _vAlloc->Size() : _vBufferSize;
  };

  //@{ implementation of VertexBufferD3DTLRUItem
  virtual MemSizeVRAMSys GetMemoryControlled() const;
  //@}
};

class TextBankD3DT;

// 8b stencil layout
#define STENCIL_FULL 0xff
#define STENCIL_COUNTER 0x1f // 5 bits counter
#define STENCIL_PROJ_SHADOW 0x20 // 1 bit - projected shadow
#define STENCIL_SHADOW_LEVEL_MASK 0xc0 // 2 bit - shadow level counter
#define STENCIL_SHADOW_LEVEL_STEP 0x40

//! Water parameters
struct WaterPar
{
  float _shoreTop,_shoreBottom;
  float _peakWaveTop,_peakWaveBottom;
  float _seaLevel;
  float _grid;
  float _waterSegSize;
};

//! Structure to hold pointer to input layout definition and number of IL elements
struct InputElements
{
  //! Input element description
  D3D10_INPUT_ELEMENT_DESC *_ied;
  //! Number of input elements
  int _size;
};

//! Structure to hold objects in a hash table for quick access with possibility to delete the oldest objects
/*!
TypeDef holds the structure that identifies the object. TypeObj holds the object structure.
*/
template <class TypeDef, class TypeObj, int maxItems>
class HashTableWithHistory
{
private:
  //! Key type
  typedef TypeDef KeyType;

public:
  //! Object in the table
  struct TableItem : public RefCount, public TLinkBidirD
  {
    //! Structure that identifies the object (it is the key)
    KeyType _definition;
    //! Object associated with the key
    TypeObj _object;
    //! Constructor
    TableItem() {}
    //! Constructor
    TableItem(KeyType d, TypeObj o) {_definition = d; _object = o;}
    //! Structure character
    ClassIsMovableZeroed(TableItem)
  };

private:
  //! Traits of the object
  struct TableItemTraits : public DefMapClassTraits<Ref<TableItem> >
  {
    //! Typedef to overwrite the local DefMapClassTraits KeyType typedef
    typedef TypeDef KeyType;

    //! Calculate hash value
    static unsigned int CalculateHashValue(KeyType key)
    {
      unsigned int k = 0;
      int s = sizeof(KeyType) / 4;
      unsigned int *d = (unsigned int *)(&key);
      for (int i = 0; i < s; i++)
      {
        //unsigned int toChangeMask = k^d[i];
        k = k^d[i];
      }
      return k;
    }
    
    //! Compare keys, return negative when k1<k2, positive when k1>k2, zero when equal
    static int CmpKey(KeyType k1, KeyType k2)
    {
      return memcmp(&k1, &k2, sizeof(KeyType));
    }

    //! Return the key
    static KeyType GetKey(const Ref<TableItem> &item) {return item->_definition;}
  };
  //! Hash table container
  MapStringToClass<Ref<TableItem>, RefArray<TableItem>, TableItemTraits> _hashTable;
  //! Link list container
  TListBidir<TableItem> _list;

public:
  //! Get item from the hash table, mark it as most recent
  const Ref<TableItem> &Get(KeyType &d)
  {
    // Get item from the hash table
    const Ref<TableItem> &htItem = _hashTable.Get(d);

    // If the item was found, then move it to the beginning
    if (_hashTable.NotNull(htItem))
    {
      //! Remove item from the list
      _list.Delete(htItem);

      //! Put it at the beginning
      _list.Insert(htItem);
    }

    // Return the item
    return htItem;
  }
  //! Add item to the hash table, remove some old items if necessary
  void Add(const Ref<TableItem> &item)
  {
    // If the number of items in the hash table exceeded limit, remove the last
    while (_hashTable.NItems() >= maxItems)
    {
      TableItem *lastItem = _list.Last();
      _hashTable.Remove(lastItem->_definition);
      LogF("Info: hash table was reduced");
    }

    // Add item to the hash table
    _hashTable.Add(item);

    // Add item to the link list at the beginning
    _list.Insert(item);
  }
  //! Determine the item to be NULL
  bool IsNull(const Ref<TableItem> &item) const {return _hashTable.IsNull(item);}
  //! Determine the item not to be NULL
  bool NotNull(const Ref<TableItem> &item) const {return _hashTable.NotNull(item);}
};

typedef HashTableWithHistory<D3D10_DEPTH_STENCIL_DESC, ComRef<ID3D10DepthStencilState>, 100> DepthStencilStates;
typedef HashTableWithHistory<D3D10_BLEND_DESC, ComRef<ID3D10BlendState>, 100> BlendStates;
typedef HashTableWithHistory<D3D10_RASTERIZER_DESC, ComRef<ID3D10RasterizerState>, 100> RasterizerStates;
typedef HashTableWithHistory<D3D10_SAMPLER_DESC, ComRef<ID3D10SamplerState>, 100> SamplerStates; // Note there must be at least MaxSamplerStages here, otherwise items could be deleted from the used list

//! D3D engine implementation
/*!
  both HW and SW T&L version are implemented
*/
class EngineDDT: public Engine, public MemoryFreeOnDemandHelper
{
  friend struct SConstantInfo;
  
  friend class D3DVisibilityQueryT;

  //!{ Pushbuffer friend classes
  friend class PBItemT;
  friend class PBISetTextureT;
  friend class PBISetTexture0T;
  friend class PBISetVertexShaderConstantT;
  friend class PBISetVertexShaderConstant2T;
  friend class PBISetVertexShaderConstantBT;
  friend class PBISetVertexShaderConstantB2T;
  friend class PBISetVertexShaderConstantIT;
  friend class PBISetVertexShaderConstantMaxColorT;
  friend class PBISetVertexShaderConstantMLSunT;
  friend class PBISetVertexShaderConstantMLNoneT;
  friend class PBISetVertexShaderConstantSAFRT;
  friend class PBISetVertexShaderConstantFFART;
  friend class PBISetVertexShaderConstantLWSMT;
  friend class PBISetRenderStateT;
  friend class PBISetSamplerStateT;
  friend class PBISetPixelShaderT;
  friend class PBISetVertexShaderT;
  friend class PBISetVertexDeclarationT;
  friend class PBISetPixelShaderConstantT;
  friend class PBISetPixelShaderConstantDBST;
  friend class PBISetPixelShaderConstantDBSZeroT;
  friend class PBISetPixelShaderConstant_A_DFORCED_E_SunT;
  friend class PBISetPixelShaderConstant_A_DFORCED_E_NoSunT;
  friend class PBISetPixelShaderConstantWaterT;
  friend class PBISetPixelShaderConstantWaterSimpleT;
  friend class PBISetPixelShaderConstantGlassT;
  friend class PBIDrawIndexedPrimitiveT;
  friend class PBISetBiasT;
  //!}

  //!{ Post process classes
  class PostProcess;
  class PPSimple;
  class PPCopy;
  class PPFinal;
  class PPGaussianBlur;
  class PPDOF;
  class PPDistanceDOF;
  class PPGlow;
  class PPStencilShadowsPri;
  class PPStencilShadowsSec;
  class PPFlareIntensity;
  //!}

  /// HLSL include support for custom files
  class HLSLInclude : public ID3D10Include
  {
  public:
    /**
    @param data [out] file data
    @param bytes [out] file size
    @return error code
    */
    STDMETHOD(Open)(D3D10_INCLUDE_TYPE includeType, LPCSTR fileName, LPCVOID parentData, LPCVOID *data, UINT *bytes);
    STDMETHOD(Close)(LPCVOID pData);
  };


  typedef Engine base;

protected:
  // data members
  bool _initialized;

  //@{ back buffer dimensions
  int _w,_h;
  //@}
  //@{ viewport dimensions
  int _viewW,_viewH;
  //@}
  

  int _pixelSize; // 16 or 32 bit mode?
  int _depthBpp; // 16 or 32 bit mode?
  int _refreshRate;

  /// performance level of the current device
  int _deviceLevel;

  /// used to detect a device change
  RString _lastDeviceId;
  ///

  /// we may use _ddraw interface to peform memory status queries
  ComRef<IDirectDraw7> _ddraw;
  
  //! Swap chain description
  DXGI_SWAP_CHAIN_DESC _pp;

  //! Engine's device type
  D3D10_DRIVER_TYPE _devType;

  /// format of the primary render target
  DXGI_FORMAT _rtFormat;

  /// format of the down-sampling render target
  /** we want this to be the same as _rtFormat, but we need filtering available */
  DXGI_FORMAT _downsampleFormat;
  
  /// format of aperture surfaces
  DXGI_FORMAT _apertureFormat;
  
  //@{ multi-sampling settings
  D3DMULTISAMPLE_TYPE_OLD _rtMultiSampleType;
  DWORD               _rtMultiSampleQuality;
  
  /// user FSAA settings
  int _fsAAWanted;
  /// max FSAA supported by the current mode
  int _maxFSAA;
  //@}
  
  //@{ anisotropy filtering settings
  int _afWanted;
  /// max. level we allow to the user (0..4)
  int _maxAF;
  /// max. level allowed by the HW (typicaly 1..16)
  int _maxAFLevel;
  //@}
  
  /// postprocess effects quality
  int _postFxQuality;
  
  /// HDR precision requested by the user (8,16 or 32 may make a sense)
  HDRPrec _hdrPrecWanted;
  /// HDR precision supported by the game - combination of HDR8,HDR16
  int _hdrPrecSupported;

  //!{ Eye accommodation based on image space measurements
  float _lastKnownAvgIntensity;
  //!}
  /// adaptation factor used to render last frame
  float _accomFactorPrev;

  /// refcount for handling nested Begin/EndVisiblityTest
  int _visTestCount;

  HWND _hwndApp;
  HINSTANCE _hInst;
  
  DWORD _shaderHandle;
  
  int _bias; // z-buffer bias
  float _matC, _matD; // w-buffer limits set
  bool _clipANearEnabled,_clipAFarEnabled;
  Plane _clipANear, _clipAFar; // additional clipping set

  int _minGuardX; // guard band clipping properties
  int _maxGuardX;
  int _minGuardY;
  int _maxGuardY;

  bool _useWBuffer;
  bool _userEnabledWBuffer;
  bool _windowed;
  /// device was lost - reset is needed
  /// reset might be needed - reasons may be different
  /** we use value of D3DERR_DEVICENOTRESET to force the reset */
  HRESULT _resetStatus;
  /// reset was done (lost device was recovered)
  bool _resetDone;

  QueryIdT _ids;

protected:
  Ref<TextBankD3DT> _textBank;
  //TextBankD3DT *_textBank;
  TLVertexTable *_mesh; // mesh data used during rendering
  //const VertexTable *_sMesh;

  // this class provides all functionality common for window/full screen versions
protected:
  //! D3D Device
  ComRef<ID3D10Device> _d3DDevice;
  //! D3D IDXGISwapChain
  ComRef<IDXGISwapChain> _d3DSwapChain;
  //! Render target
  ComRef<ID3D10RenderTargetView> _renderTarget;
  //! Depth stencil
  ComRef<ID3D10DepthStencilView> _depthStencil;
  
  
  int _lastSpec;
  TexMaterialLODInfo _lastMat;
  int _prepSpec;

  enum {MaxStages=8,MaxSamplerStages=16,UsableSampleStages=15};
  TextureD3DHandle _lastHandle[MaxSamplerStages];
  TextureD3DHandle _setHandle[MaxSamplerStages];
  EShadowState _currentShadowStateSettings;
  
  /// some rendering used no back face culling
  bool _backCullDisabled;
  bool _currBackCullDisabled;

  //! Crater rendering state
  bool _currCraterRendering;

  /// shadow receiver level - drawing objects
  int _shadowLevel;
  /// shadow receiver level needed for casted shadows
  int _shadowLevelNeeded;
  
  //! Maximal color of texture on stage 0
  Color _texture0MaxColor;
  //! Average color of texture on stage 0
  Color _texture0AvgColor;
  //! Flag to determine the _texture0MaxColor is to be included in the PS parameters (so we save one instruction in PS)
  bool _texture0MaxColorSeparate;

  //!{ UV compression properties
  UVPair _texGenScale[MAX_UV_SETS];
  UVPair _texGenOffset[MAX_UV_SETS];
  bool _texGenScaleOrOffsetHasChanged;
  //!}

  /// current 2D/3D preference
  enum TLMode
  {
    /// 2D rendering
    TLDisabled,
    /// 3D rendering
    TLEnabled,
    /// 3D view space rendering - post-process effects
    TLViewspace
  };
  TLMode _tlActive;

  EMainLight _mainLight;
  EMainLight _currMainLight;

  bool _applyAplhaFog;

  EFogMode _fogMode;
  EFogMode _currFogMode;

  //! New status of enabling the sun
  //bool _sunEnabled;
  //! Current status of enabling the sun
  //bool _currSunEnabled;

  //! Currently active lights
  LinkArray<Light> _lights;

  ComRef<IDirect3DVertexBuffer9Old> _vBufferLast; // last SetStreamSource
  ComRef<IDirect3DIndexBuffer9Old> _iBufferLast; // last SetIndices
  int _vOffsetLast;
  int _vBufferSize;

  int _iOffset; // something must be added to index source in DIP

  // dynamic data for TL
  VertexDynamicDataT _dynVBuffer;
  // shared static data for TL
  RefArray<VertexStaticDataT> _statVBuffer;
  RefArray<IndexStaticDataT> _statIBuffer;

  /// assumed alignment for VB allocations
  int _vbAlignment;

  /// assumed alignment for texture allocations
  int _texAlignment;

  //@{ device capabilities/properties
  /// can we assume static vertex buffers go into non-local memory?
  bool _staticVBNonLocal;
  
  /// can we assume dynamic vertex buffers go into non-local memory?
  bool _dynamicVBNonLocal;

  /// can we assume index buffers go into non-local memory?
  bool _ibNonLocal;
  
  
  // should we use the local VRAM prediction and DX7 number?
  bool _localPrediction;
  //@}

  /// should all vertex buffers be static?
  /** this is set depending on amount of local VRAM and texture detail required */
  bool _vbuffersStatic;

  //! how much memory is allocated in index buffers (in bytes)
  int _iBuffersAllocated;
  //! how much memory is allocated in vertex buffers (in bytes)
  int _vBuffersAllocated;

  /// total memory allocated in all render-targets/back-buffers...
  int _rtAllocated;

  


  /// VRAM amount (checked by DX7)
  int _localVramAmount;
  /// AGP amount (checked by DX7)
  int _nonlocalVramAmount;
  
  //@{ The estimation based on DX7 test
  /// VRAM amount free (estimated)
  int _freeLocalVramAmount;
  /// AGP amount free (estimated)
  int _freeNonlocalVramAmount;
  /// how much did happen since the last DX7 test (how much is a new test needed)
  int _freeEstDirty;

  /// time when last check was performed
  DWORD _freeEstCheckTime;
  //@}

  
  /// LRU list of vertex buffers - used to release buffer that were not used for a long time
  /** list of all shapes which have geometry loaded, but it can be released */
  FramedMemoryControlledList<VertexBufferD3DTLRUItem> _vBufferLRU;
  
  bool _enableReorder; // external hint - reorder enabled

  TexLoc _texLoc;

  int _dxtFormats; // bit-field of DXT formats (1..5)
  //! Maximum size of the texture the HW is capable to use
  int _maxTextureSize;  
  
  bool _canHWMouse;
  bool _canReadSRGB;
  bool _canWriteSRGB;
  
  bool _hasStencilBuffer;
  bool _canTwoSidedStencil;
  bool _canWBuffer;
  bool _canWBuffer32;
  int _maxLights;

  int _vertexShaders; // pixel shader version
  int _pixelShaders; // pixel shader version
  ComRef<ID3D10PixelShader> _pixelShader[NPixelShaderSpecular][NPixelShaderID][NPixelShaderType];
  PixelShaderSpecular _pixelShaderSpecularSel;
  PixelShaderID _pixelShaderSel;
  PixelShaderType _pixelShaderTypeSel;
  ComRef<ID3D10PixelShader> _pixelShaderLast;
  
  // how many bones at one time are supported by given implementation
  int _maxBones;
  int _maxInstances;
  int _maxInstancesSprite;
  int _maxInstancesShadowVolume;

  //!{ Maximum instances in the night (smaller number than in the day)
  int _maxNightInstances;
  int _maxNightInstancesSprite;
  //!}

  /// HLSL include support for custom files
  HLSLInclude _hlslInclude;
  //! Non-TL vertex declaration
  ComRef<ID3D10InputLayout> _vertexDeclNonTL;
  //! Array of all possible vertex declarations
  ComRef<ID3D10InputLayout> _vertexDeclD[VDCount][VSDVCount];
  //! New vertex declaration
  ComRef<ID3D10InputLayout> _vertexDecl;
  //! Current vertex declaration
  ComRef<ID3D10InputLayout> _vertexDeclLast;
  //! New vertex shader ID
  VertexShaderID _vertexShaderID;
  //! Current vertex shader ID
  VertexShaderID _currVertexShaderID;
  //!{ UV sources for particular stages
  UVSource _uvSource[MaxStages];
  UVSource _currUVSource[MaxStages];
  int _uvSourceCount;
  int _currUVSourceCount;
  bool _uvSourceOnlyTex0;
  bool _currUVSourceOnlyTex0;
  //!}

  //!{ Shader pool (no-linking)
  //! Shader pool dimensions
  enum
  {
    SHP_VertexShaderID      = NVertexShaderPoolID,
    SHP_UVInput             = 2,
    SHP_MainLight           = 4,
    SHP_RenderingMode       = 4,
    SHP_SkinningInstancing  = 3,
  };
  //! Non-TL vertex shader (shader for 2D drawing)
  VertexShaderInfo _vertexShaderNonTL;
  //! Shader pool n-dimension array
  VertexShaderInfo _vertexShaderPool[SHP_VertexShaderID][SHP_UVInput][SHP_MainLight][SHP_RenderingMode][SHP_SkinningInstancing];
  //! Shadow volume shaders for normal, skinning and instancing objects
  VertexShaderInfo _vertexShaderShadowVolume[SHP_SkinningInstancing];
  //! List of special vertex shaders (those shaders that exist in one version only)
  VertexShaderInfo _vertexShaderSpecial[NVertexShaderSpecialID];
  //! Terrain shader in 3 variations
  VertexShaderInfo _vertexShaderTerrain[SHP_RenderingMode];
  //!}

  //!{ Current and previous vertex shaders
  VertexShaderInfo _vertexShader;
  VertexShaderInfo _vertexShaderLast;
  //!}

  //!{ P&S lights count and their current values in vertex shader
  int _pointLightsCount;
  int _currPointLightsCount;
  int _spotLightsCount;
  int _currSpotLightsCount;
  //!}

  bool _currShadowReceiver;
  //ETTG _currTexGenType[2];
  Matrix4 _world;
  Matrix4 _invWorld;
  Matrix4 _view;
  /// to avoid numeric precision problem we avoid inverting camera space in model space
  /** we prefer subtracting position and inverting orientation only */
  Matrix4 _invView;
  /// some post-effects may be interested in knowing scene projection
  Matrix4 _proj;
  Color _lightAmbient;
  Color _lightDiffuse;
  Color _lightSpecular;
  Vector3 _direction;
  Vector3 _shadowDirection;
  Color _matAmbient;
  Color _matDiffuse;
  Color _matForcedDiffuse;
  Color _matSpecular;
  Color _matEmmisive;
  float _matPower;

  /// modulation color - used for HDR buffer transformation
  Color _modColor;

  /// is pixel shader work done in linear space, or SRGB?
  bool _linearSpace;
  /// is render target linear, or is SRGB on write performed?
  bool _rtIsSRGB;

  //ETTG _texGenType[MaxStages];
  float _night;
  //@{ fog parameters
  float _fogStart,_fogEnd;
  //@}
  //@{ alpha fog parameters
  float _aFogStart,_aFogEnd;
  //@}
  /// exponential fog density
  /** used in shader as haze = exp(dist*_expFogCoef) */
  float _expFogCoef;
  
  enum
  {
    // our shaders are ready only up to 50
#if _ENABLE_SKINNEDINSTANCING
    LimitBones = 256,
#else
    LimitBones = 60,
#endif
    LimitInstances = 45,
    LimitInstancesSprite = 60,
    LimitInstancesShadowVolume = 60,
  };
protected:

  //! Type of skinning
  ESkinningType _skinningType;
  //! Current type of skinning
  ESkinningType _currSkinningType;
  //! Array of bones
  typedef VerySmallArray<Matrix4, sizeof(Matrix4) * 256 + sizeof(int)> BonesType;
  BonesType _bones;
  //! Flag to determine bones were set during SetSkinningType method call
  bool _bonesWereSetDirectly;

  //! Flag to determine the instancing is being used
  bool _currInstancing;
  int _currInstancesCount;

#if _ENABLE_SKINNEDINSTANCING
  //! Bones with instancing
  //VerySmallArray<BonesType, sizeof(BonesType) * LimitInstances + sizeof(int)> _bonesInstanced;
  AutoArray<BonesType> _bonesInstanced;
#endif

  //! Parameters used for water drawing
  WaterPar _waterPar;

  AutoArray<FloatConstant> _waves;

  //!( Wanted and current values of misc. rendering flags
  bool _alphaShadowEnabled;
  bool _currAlphaShadowEnabled;
  //!}

protected:

  //!{ Vertex declarations creation/destruction
  bool CreateVertexDeclarations();
  void DestroyVertexDeclarations();
  //!}

  //!{ PostProcess resources creation/destruction
  bool CreatePostProcessStuff();
  void DestroyPostProcessStuff();
  //!}


  float _nightEye;

  // members used for both DirectDraw/Direct3D

  //! Actual back buffer
  /*!
    We remember this back buffer at the start of each frame and set it again in the end.
  */
  // ComRef<IDirect3DSurface9> _backBuffer; // Direct3D 10 NEEDS UPDATE 
  //! Depth buffer associated with the current backbuffer
  // ComRef<IDirect3DSurface9> _depthBuffer; // Direct3D 10 NEEDS UPDATE 
  //! Depth buffer associated with the current render target
  // ComRef<IDirect3DSurface9> _depthBufferRT; // Direct3D 10 NEEDS UPDATE 

  //{! Render targets
  /*!
    We set these textures as render target at the start of each frame, use them for
    rendering and in the end we copy the data to the back buffer. We use destination (D)
    as a target and source (S) as a texture to render from. Each PP step switches the
    source and the destination.
  */
  // ComRef<ID3D10Texture2D> _renderTargetD; // Direct3D 10 NEEDS UPDATE 
  // ComRef<IDirect3DSurface9> _renderTargetSurfaceD; // Direct3D 10 NEEDS UPDATE 
  // ComRef<ID3D10Texture2D> _renderTargetS; // Direct3D 10 NEEDS UPDATE 
  // ComRef<IDirect3DSurface9> _renderTargetSurfaceS; // Direct3D 10 NEEDS UPDATE 


  //!{ Currently used render target and depth stencil view
  ComRef<ID3D10RenderTargetView> _currRenderTargetView;
  ComRef<ID3D10DepthStencilView> _currDepthStencilView;
  //!}

  // Direct3D 10 CHANGED 
//   ComRef<ID3D10Texture2D> _currRenderTarget;
//   ComRef<IDirect3DSurface9> _currRenderTargetSurface;
//   ComRef<IDirect3DSurface9> _currRenderTargetDepthBufferSurface;

  /// get current render target for sampling
  ComRef<ID3D10Texture2D> GetCurrRenderTargetAsTexture();
  /// we need render target texture - find any
  void SwitchToAnotherRenderTarget();
  /// we need to render to the backbuffer
  void SwitchToBackBufferRenderTarget();
  
  /// make sure we have a copy of the current render target
  ComRef<ID3D10Texture2D> CreateRenderTargetCopyAsTexture();

  //!{ Setting and restoring of temporary render target
  ComRef<ID3D10RenderTargetView> _savedRenderTargetView;
  ComRef<ID3D10DepthStencilView> _savedDepthStencilView;
  void SetTemporaryRenderTarget(const ComRef<ID3D10RenderTargetView> &rt, const ComRef<ID3D10DepthStencilView> &ds);
  // Direct3D 10 CHANGED 
//   ComRef<ID3D10Texture2D> _renderTargetSaved;
//   ComRef<IDirect3DSurface9> _renderTargetSurfaceSaved;
//   ComRef<IDirect3DSurface9> _renderTargetDepthBufferSurfaceSaved;
//   void SetTemporaryRenderTarget(const ComRef<ID3D10Texture2D> &rt, const ComRef<IDirect3DSurface9> &rts, const ComRef<IDirect3DSurface9> &dts);
  void RestoreOldRenderTarget();
  //!}

  //!}

  //!{ Post processing stuff
  Ref<PostProcess> _ppStencilShadowsPri;
  Ref<PostProcess> _ppStencilShadowsSec;
  Ref<PostProcess> _ppGaussianBlur;
  Ref<PostProcess> _ppDOF;
  Ref<PostProcess> _ppDistanceDOF;
  Ref<PPGlow> _ppGlow;
  Ref<PostProcess> _ppFinal;
  Ref<PPFlareIntensity> _ppFlareIntensity;
  //!}

  QueueT _queueNo;
  //QueueT _queueTL;

  enum RenderMode {RMLines,RMTris,RM2DLines,RM2DTris};
  RenderMode _renderMode;

  float _gamma;
  
  /// corresponds to TextBank value,
  /** needed here because it can be set before TextBank is created */
  int _textureQuality;

  bool _flippable; // front/back buffer can be flipped

  bool _d3dFrameOpen; // FinishFrame is necessary

  Color _textColor;

  //enum VFormatSet {SingleTex,DetailTex,SpecularTex};
  //VFormatSet _formatSet;

private:
  // 256 constants are enough for VS 2 model
  float _shaderConstValues[256 + NBOOLCONST + NINTCONST][4];
  int _shaderConstValuesI[16][4];
  float _pixelShaderConstValues[32][4];
  void ResetShaderConstantValues();

  Vector3 _treeCrown[2];
  
  //! Number of DrawIndexedPrimitive calls from the beginning of the frame
  int _dipCount;

  //!{ Flag to determine what we're rendering (Usual stuff, shadow buffer, depth buffer...)
  RenderingMode _renderingMode;
  RenderingMode _currRenderingMode; // Rendering mode the shaders are built with
  RenderingMode _lastRenderingMode; // Rendering mode the render states are set with
  //!}

  //! Flag to determine the pushbuffer is being created
  bool _pbIsBeingCreated;
  //! Pointer to pushbuffer that is being created
  RefArray<PBItemT> *_pushBuffer;

  //!{ Shadow buffer parameters
  /// did user enable the SB rendering?
  int _sbQuality;
  //! Flag to determine what SB technique can be used
  SBTechnique _sbTechnique;
  /// size of shadow buffer surface
  int _sbSize;
  //!{ Render target
// Direct3D 10 NEEDS UPDATE 
//   ComRef<ID3D10Texture2D> _renderTargetSB;
//   ComRef<IDirect3DSurface9> _renderTargetSurfaceSB;
//   ComRef<ID3D10Texture2D> _renderTargetDepthBufferSB;
//   ComRef<IDirect3DSurface9> _renderTargetDepthBufferSurfaceSB;
  //!}
  //!{ Shadow plane parameters
  Matrix4 _shadowPlaneOrigin;
  Matrix4 _shadowPlaneProjection;
  //float _shadowPlaneRadius;
  //!}
  //! SP rounded camera position SP orientation
  Matrix4 _oldLRSP;
  //!}

  //!{ Depth of field parameters
  //! Flag to determine engine is capable of depth buffer
  bool _dbAvailable;
  //! Did user enable the DB rendering?
  int _dbQuality;
// Direct3D 10 NEEDS UPDATE 
//   ComRef<ID3D10Texture2D> _renderTargetDB;
//   ComRef<IDirect3DSurface9> _renderTargetSurfaceDB;
//   ComRef<IDirect3DSurface9> _renderTargetDepthStencilSurfaceDB;
  //! Focal plane distance
  float _focusDist;
  //! Amount of blurring from interval <0, 1>
  float _blurCoef;
  //! Flag to signalize special DOF effect (blur far only objects) should be used
  bool _farOnly;
  //!}

  //! Flag to determine we use the low-end quality
  bool _lowEndQuality;

  //!{ Single instance parameters
  float _instanceShadowIntensity;
  Color _instanceColor;
  //!}

  /// mouse cursor currently in use
  /** NULL = cursor disabled */
  LLink<TextureD3DT> _cursorTex;
  /// color of current mouse cursor
  PackedColor _cursorColor;
  /// we may need to perform format conversion
  // ComRef<IDirect3DSurface9> _cursorSurface; // Direct3D 10 NEEDS UPDATE 
  /// mouse center x coordinate 
  int _cursorX;
  /// mouse center y coordinate 
  int _cursorY;
  /// was cursor set in this frame?
  bool _cursorSet;
  /// is the cursor currently visible?
  bool _cursorShown;

  //! Uses _world and _invWorld matrices to transform specified position to object-space
  Vector3 TransformPositionToObjectSpace(Vector3Par position);
  //! Uses _world and _invWorld matrices to transform and normalize specified position to object-space
  Vector3 TransformAndNormalizePositionToObjectSpace(Vector3Par position);
  //! Transforms a vector to object space
  Vector3 TransformAndNormalizeVectorToObjectSpace(Vector3Par vector);
  /// interface for tree shader
  const Vector3 *GetTreeCrown() const {return _treeCrown;}
  
  void SetFiltering(int stage, TexFilter filter, int maxAFLevel);
  void SetMultiTexturing(TextureD3DT **tex, int count, int keepUntouchedLimit = TEXID_FIRST);
  void SetTexture( const TextureD3DT *tex, const TexMaterial *mat, int specFlags, int filterFlags );

  void SetTexture0Color(ColorVal maxColor, ColorVal avgColor, bool optimize);
  void SetTexture0(const TextureD3DT *tex);

  //!{ Method and helper structures that setups render and sampler states - DX9 simulation
  DepthStencilStates _depthStencilStates;
  BlendStates _blendStates;
  RasterizerStates _rasterizerStates;
  SamplerStates _samplerStates;
  void SetupDepthStencilStates();
  void SetupBlendStates();
  void SetupRasterizerStates();
  void SetupSamplerStates();
  ComRef<ID3D10Buffer> _vsConstants;
  ComRef<ID3D10Buffer> _psConstants;
  void SetupVSConstantBuffer();
  void SetupPSConstantBuffer();
  void SetupTextures();

  void SetupRenderStates();
  //!}


  //!{ Wrappers of DX9 functions, some of them lazy
public:
  HRESULT Map(const ComRef<IDirect3DVertexBuffer9Old> &buf, D3D10_MAP mapType, UINT mapFlags, void **ppData);
  HRESULT Map(const ComRef<IDirect3DIndexBuffer9Old> &buf, D3D10_MAP mapType, UINT mapFlags, void **ppData);
private:
  void SetTexture(int stage, ID3D10ShaderResourceView *tex, bool optimize = true);
  void SetStreamSource(const ComRef<IDirect3DVertexBuffer9Old> &buf, int stride, int bufSize, bool optimize = true);
  void SetVertexDeclaration(const ComRef<ID3D10InputLayout> &decl, bool optimize = true);
  void SetVertexShader(const VertexShaderInfo &vs, bool optimize = true);
  bool SetVertexShaderConstantF(int startRegister, const float *data, int vector4fCount, bool optimize = true);
  bool SetVertexShaderConstantI(int startRegister, const int *data, int vector4fCount, bool optimize = true);
  bool SetVertexShaderConstantB(int startRegister, const bool *data, int boolCount, bool optimize = true);
  void SetPixelShader(const ComRef<ID3D10PixelShader> &ps, bool optimize = true);
  bool SetPixelShaderConstantF(int startRegister, const float *data, int vector4fCount, bool optimize = true);
  // Direct3D 10 NEEDS UPDATE 
//   void SetRenderTarget(
//     const ComRef<IDirect3DSurface9> &rts, const ComRef<ID3D10Texture2D> &rt,
//     const ComRef<IDirect3DSurface9> &dt,
//     bool optimize = true);
//   void SetRenderTargetAndDepthStencil(
//     const ComRef<IDirect3DSurface9> &rts, const ComRef<ID3D10Texture2D> &rt,
//     const ComRef<IDirect3DSurface9> &dts,  
//     bool optimize = true);
  void DrawPrimitive(D3DPRIMITIVETYPE_OLD primitiveType, UINT startVertex, UINT primitiveCount);
  void DrawIndexedPrimitive(D3DPRIMITIVETYPE_OLD primitiveType, INT baseVertexIndex, UINT minIndex, UINT numVertices, UINT startIndex, UINT primitiveCount);

  ComRef<ID3D10DepthStencilState> _currDSS;
  UINT _currStencilRef;
  void OMSetDepthStencilState(const ComRef<ID3D10DepthStencilState> &dss, UINT stencilRef);

  ComRef<ID3D10BlendState> _currBS;
  UINT _currSampleMask;
  void OMSetBlendState(const ComRef<ID3D10BlendState> &bs, UINT sampleMask);

  ComRef<ID3D10RasterizerState> _currRS;
  void RSSetState(const ComRef<ID3D10RasterizerState> &rs);
  //!}

public:
  // properties
  RString GetDebugName() const;
  int Width() const {return _w;}
  int Height() const {return _h;}
  int PixelSize() const {return _pixelSize;} // 16 or 32 bit mode?
  int RefreshRate() const {return 0; /*Direct3D 10 NEEDS UPDATE return _pp.FullScreen_RefreshRateInHz;*/}
  bool CanBeWindowed() const {return true;}
  bool IsWindowed() const {return _windowed;}

  int Width2D() const {return _w;}
  int Height2D() const {return _h;}
  
  int MinGuardX() const {return _minGuardX;}
  int MaxGuardX() const {return _maxGuardX;}
  int MinGuardY() const {return _minGuardY;}
  int MaxGuardY() const {return _maxGuardY;}

  int MinSatX() const {return _minGuardX;}
  int MaxSatX() const {return _maxGuardX;}
  int MinSatY() const {return _minGuardY;}
  int MaxSatY() const {return _maxGuardY;}

  void InitVRAMPixelFormat( DXGI_FORMAT &pf, PacFormat format, bool enableDXT );
  
  int MaxInstances() const {return _maxInstances;}
  int MaxInstancesSprite() const {return _maxInstancesSprite;}
  int MaxInstancesShadowVolume() const {return _maxInstancesShadowVolume;}
  
protected:
  void WorkToBack();
  //! Presenting result to the output
  void BackToFront();

  void SearchMonitor(int &x, int &y, int &w, int &h);

  public:
  // constructor - init all pointers to NULL

  void CreateSurfaces(bool windowed, ParamEntryPar cfg, ParamEntryPar fcfg);
  void DestroySurfaces(bool destroyTexBank = true);
  
public:
  EngineDDT();
/*
  EngineDDT
  (
    HINSTANCE hInst, HINSTANCE hPrev, int sw,
    int x, int y, int width, int height, bool windowed, int bpp
  );
*/
  ~EngineDDT();
  //! Virtual method
  virtual PushBuffer *CreateNewPushBuffer(int key);

  //! Initialization
  bool Init(
    HINSTANCE hInst, HINSTANCE hPrev,
    int sw, int x, int y, int width, int height,
    bool windowed, int bpp, bool generateShaders);
  //! Deinitialization
  void Done();
  /// virtual initialization done after GEngine is assigned
  virtual void Init();
  //! Destroying of all possible surfaces and buffers
  /*!
    \param hard Release system textures as well
  */
  void PreReset(bool hard);
  void PostReset();
  /// can be used before time consuming engine switch (resolution switch...)
  void DisplayWaitScreen();
  bool Reset(); // use Reset to change setting / reset device
  bool ResetHard(); // destroy and create the device again

  //! Virtual method
  virtual void MaterialHasChanged() {_lastMat = TexMaterialLODInfo((TexMaterial *)-1, 0);}

  //! Method to set bones constants on the GPU
  void SetBones(int minBoneIndex, const Matrix4 *bones, int nBones);

  //! Virtual method
  virtual bool IsSkinningSupported() const {return true;};
  //! Virtual method
  virtual int GetMaxBonesCount() const {return _maxBones;};
  //! Virtual method
  virtual bool BonesWereSetDirectly() const {return _bonesWereSetDirectly;};
  //! Virtual method
  virtual void SetSkinningType(ESkinningType skinningType, const Matrix4 *bones = NULL, int nBones = 0);
#if _ENABLE_SKINNEDINSTANCING
  virtual void SetSkinningTypeInstance(ESkinningType skinningType, int instanceIndex, const Matrix4 *bones, int nBones);
#endif

  virtual bool CanSeaWaves() const {return true;}
  virtual bool CanSeaUVMapping(int waterSegSize, float &mulUV, float &addUV) const;
  
  virtual void SetWaterDepth(
    const WaterDepthQuad *water, int xRange, int zRange, float grid,
    float seaLevel,
    float shoreTop, float shoreBottom, float peakWaveTop, float peakWaveBottom
  );
  //! Setting of shadow information for one segment of terrain
  virtual void SetLandShadow(const float *shadow, int xRange, int zRange, float grid);
  //! Virtual method
  virtual void SetInstanceInfo(ColorVal color, float shadowIntensity, bool optimize = true);
  //! Virtual method
  virtual ColorVal GetInstanceColor() const {return _instanceColor;}
  //! Virtual method
  virtual float GetInstanceShadowIntensity() const {return _instanceShadowIntensity;}
  
  //! Sets the modulate color
  virtual void SetModulateColor(const Color &modColor) {_modColor = modColor;}

  virtual void SetPostFxEnabled(int value);
  virtual void SetPostFxParam(int fx, const float *param);
  
  
  virtual void Postprocess(float deltaT);
  virtual void NoPostprocess();

  //! Virtual method
  virtual void CalculateFlareIntensity(int x, int y, int sizeX, int sizeY);
  //! Virtual method

  virtual AbilityToDraw IsAbleToDraw(bool wantReset);
  bool IsAbleToDrawCheckOnly();
  bool ResetNeeded() const {return _resetStatus!=D3D_OK;}
  
  void InitDraw( bool clear=false, Color color = HBlack ); // Begin scene
  void FinishDraw(int wantedDurationVSync, bool allBackBuffers = false, RString fileName = RString(""));
  bool InitDrawDone();
  void BackBufferToAllBuffers();
  void NextFrame(); // swap frames - get ready for next frame

  virtual float GetSmoothFrameDuration() const;
  virtual float GetSmoothFrameDurationMax() const;

  void Pause();
  void Restore();
  void SetLimitedViewport(float viewSize);


  bool SwitchRes(int w, int h, int bpp);
  bool SwitchRefreshRate(int refresh);
  bool SwitchWindowed(bool windowed);

  void ListResolutions(FindArray<ResolutionInfo> &ret); // result is zero terminated w,h pairs of valid resolutions
  void ListRefreshRates(FindArray<int> &ret); // result is zero terminated w,h pairs of valid resolutions

  HDRPrec ValidateHDRPrecision(HDRPrec prec) const;

  virtual HDRPrec HDRPrecision() const {return _hdrPrecWanted;}
  virtual void SetHDRPrecision(HDRPrec hdrPrec);
  virtual int SupportedHDRPrecision() const {return _hdrPrecSupported;}

  // functions specific to Direct3D

  // functions for both DirectDraw/Direct3D
  void InitSurfaces(ParamEntryPar cfg, ParamEntryPar fcfg);
  void DoneSurfaces(bool destroyTexBank = true);
  
  void AddLocalVRAMAllocated(int size);
  void AddIBuffersAllocated(int n);
  void AddVBuffersAllocated(int n, bool dynamic);

  IDirectDraw7 *GetDirectDraw() const {return _ddraw;}
  ID3D10Device *GetDirect3DDevice() const {return _d3DDevice;}

  bool Can565() const {return false;}
  bool Can88() const {return false;}
  bool Can8888() const {return true;}
  int MaxTextureSize() const {return _maxTextureSize;}

  TexLoc GetTexLoc() const {return _texLoc;}
  
  int DXTSupport() const {return _dxtFormats;}
  bool CanDXT( int i ) const {return (_dxtFormats&(1<<i))!=0;}
  
public:
  
  bool GetHWTL() const {return true;}

  //! Clear rendertarget and depth-stencil
  void Clear(bool clearZ = true, bool clear = true, PackedColor color = PackedColor(0));
  //! Set the viewport
  void SetViewport(int width, int height);
  
  VertexBuffer *CreateVertexBuffer(const Shape &src, VBType type, bool frequent);
  int CompareBuffers(const Shape &s1, const Shape &s2);
  void BufferReleased(VertexBufferD3DT *buf);
  void OnSizeChanged(VertexBufferD3DT *buf);
  virtual void ReleaseAllVertexBuffers();

  // access to _statVBuffer
  template <int vertexDecl>
  VertexStaticDataT *AddShapeVertices(const Shape &src, VertexBufferD3DT &vbuf, bool dynamic, bool separate, bool frequent);
  template <int vertexDecl>
  VertexStaticDataT *AddShapeVerticesInstanced(const Shape &src, VertexBufferD3DT &vbuf, bool dynamic, bool separate, bool frequent, int instancesCount);

  VertexStaticDataT *AddShapeVerticesSpriteInstanced(const Shape &src, VertexBufferD3DT &vbuf, bool dynamic, bool separate, bool frequent, int instancesCount);
#if _ENABLE_SKINNEDINSTANCING
  VertexStaticDataT *AddShapeVerticesSkinnedInstanced(const Shape &src, VertexHeapT::HeapItem *&vIndex, bool dynamic, bool separate, bool frequent, int instancesCount);
  VertexStaticDataT *AddShapeVerticesNormalSkinnedInstanced(const Shape &src, VertexHeapT::HeapItem *&vIndex, bool dynamic, bool separate, bool frequent, int instancesCount);
  VertexStaticDataT *AddShapeVerticesShadowVolumeSkinnedInstanced(const Shape &src, VertexHeapT::HeapItem *&vIndex, bool dynamic, bool separate, bool frequent, int instancesCount);
#endif

  IndexStaticDataT *AddShapeIndices(const Shape &src, IndexAllocT *&iIndex, bool dynamic, int nInstances);
  IndexStaticDataT *AddShapeIndicesSprite(const Shape &src, IndexAllocT *&iIndex, bool dynamic, int nInstances);
  void DeleteVB(VertexStaticDataT *vb);
  void DeleteIB(IndexStaticDataT *ib);

  int FrameTime() const;
  int AFrameTime() const {return FrameTime();}

  void DoSetGamma();
  void SetGamma( float gamma );
  float GetGamma() const {return _gamma;}

  bool SetTextureQualityWanted(int value);
  virtual void SetTextureQuality(int value);
  virtual int GetTextureQuality() const {return _textureQuality;}
  virtual int GetMaxTextureQuality() const;

  int GetLocalVramAmount() const {return _localVramAmount;}

  bool GetDynamicVBNonLocal() const {return _dynamicVBNonLocal;}
  
  int GetVBAlignment() const {return _vbAlignment;}
  int GetTexAlignment() const {return _texAlignment;}

  int AlignVRAMTex(int size) const {return (size+_texAlignment-1)&~(_texAlignment-1);}
  int AlignVRAMVB(int size) const {return (size+_vbAlignment-1)&~(_vbAlignment-1);}
  int AlignVRAMVBSigned(int size) const {return size>=0 ? AlignVRAMVB(size) : -AlignVRAMVB(-size);}
  
  int GetFreeLocalVramAmount() const {return _freeLocalVramAmount;}
  int GetFreeNonlocalVramAmount() const {return _freeNonlocalVramAmount;}
  
  int GetNonLocalVramAmount() const {return _nonlocalVramAmount;}
  void SetVBuffersStatic(bool val){_vbuffersStatic = val;}
  bool GetVBuffersStatic() const {return _vbuffersStatic;}
  
  bool UseLocalPrediction() const {return _localPrediction;}

  virtual void SetFSAA(int quality);
  virtual int GetFSAA() const;
  virtual int GetMaxFSAA() const;

  //@{ only set values, do not apply it yet
  bool SetFSAAWanted(int quality);
  bool SetHDRPrecisionWanted(HDRPrec prec);
  bool SetResolutionWanted(int w, int h, int bpp);
  bool SetWBufferWanted(bool val);
  bool SetPostprocessEffectsWanted(int quality);

  //@}
  /// apply values set by Wanted functions
  void ApplyWanted();

  virtual void SetAnisotropyQuality(int quality);
  virtual int GetAnisotropyQuality() const;
  virtual int GetMaxAnisotropyQuality() const;

  virtual void SetPostprocessEffects(int quality);
  virtual int GetPostprocessEffects() const;

  int AnisotropyLevel(PixelShaderID ps, bool road, TexFilter filter) const;
  
  virtual float GetPreHDRBrightness() const;
  float GetPostHDRBrightness() const;
  
  virtual float GetDefaultGamma() const {return 1.0f;}
  virtual float GetDefaultBrightness() const {return 1.0f;}
  /// we want to use less than 1 bit
  virtual float GetHDRFactor() const;

  virtual int GetDeviceLevel() const;
  virtual int GetCurrentDeviceLevel() const;
  virtual void SetDefaultSettingsLevel(int level);

  void LoadConfig(ParamEntryPar cfg,ParamEntryPar fcfg);
  void SaveConfig(ParamFile &cfg);
  
  //HWND WindowHandle() const {return _hwndApp;}
  //HWND GetWindowHandle() const {return _hwndApp;}

  void FlushQueues();

  virtual void SetShadowLevel( int value );
  virtual void SetShadowLevelNeeded( int value );
  virtual void SetAlphaFog(float start, float end)
  {
    _aFogStart=start,_aFogEnd=end;
    _applyAplhaFog = start<end;
  }

  virtual void GetZCoefs(float &zAdd, float &zMult);
  void SetBias( int bias );
  int GetBias(){return _bias;}

  bool ZBiasExclusion() const {return !_hasStencilBuffer;}

  AbstractTextBank *TextBank();
  TextBankD3DT *TextBankDD() const {return _textBank;}

  void CreateTextBank(ParamEntryPar cfg, ParamEntryPar fcfg);
  void DestroyTextBank();
  void ReportGRAM(const char *name);

  void Screenshot(RString filename);
  
protected: // D3D helpers

  struct StateInfo
  {
    DWORD value;
    StateInfo( DWORD v=-1 ):value(v){}
    ClassIsMovable(StateInfo);
  };
  typedef StateInfo TextureStageStateInfo;
  typedef StateInfo SamplerStateInfo;
  typedef StateInfo RenderStateInfo;
  AutoArray<TextureStageStateInfo> _textureStageState[MaxStages];
  AutoArray<SamplerStateInfo> _samplerState[MaxSamplerStages];
  AutoArray<RenderStateInfo> _renderState;
  

  void ShadowsStatesSettings(EShadowState ss, bool noBackCull)
  {
    if (ss == _currentShadowStateSettings && _backCullDisabled==noBackCull) return;
    DoShadowsStatesSettings(ss,noBackCull);
  }
  void DoShadowsStatesSettings(EShadowState ss, bool noBackCull);

  void ChangeClipPlanes();
  void ChangeWDepth(float matC, float matD);
  void PrepareDetailTex(const TexMaterialLODInfo &mat, const EngineShapeProperties &prop);
  void PrepareSingleTexDiffuseA();

  void EnableCustomTexGen(const D3DXMATRIX **matrix, int nMatrices, bool optimize);
  void EnableCustomTexGenZero(bool optimize);

#if _DEBUG
  #define RS_DIAGS 1
#else
  #define RS_DIAGS 0
#endif
#if RS_DIAGS
  HRESULT D3DSetTextureStageStateName
  (
    DWORD stage, D3DTEXTURESTAGESTATETYPE_OLD state, DWORD value, const char *text,
    bool optimize=true
  );
  HRESULT D3DSetSamplerStateName
  (
    DWORD stage, D3DSAMPLERSTATETYPE_OLD state, DWORD value, const char *text,
    bool optimize=true
  );
  HRESULT D3DSetRenderStateName
  (
    D3DRENDERSTATETYPE_OLD state, DWORD value, const char *text,
    bool optimize=true
  );
  #define D3DSetRenderState(state,value) \
    D3DSetRenderStateName(state,value,#state,true)
  #define D3DSetTextureStageState(stage,state,value) \
    D3DSetTextureStageStateName(stage,state,value,#state,true)
  #define D3DSetSamplerState(stage,state,value) \
    D3DSetSamplerStateName(stage,state,value,#state,true)
  #define D3DSetRenderStateOpt(state,value,optimize) \
    D3DSetRenderStateName(state,value,#state,optimize)
  #define D3DSetTextureStageStateOpt(stage,state,value,optimize) \
    D3DSetTextureStageStateName(stage,state,value,#state,optimize)
  #define D3DSetSamplerStateOpt(stage,state,value,optimize) \
    D3DSetSamplerStateName(stage,state,value,#state,optimize)
#else
  HRESULT D3DSetTextureStageState
  (
    DWORD stage, D3DTEXTURESTAGESTATETYPE_OLD state, DWORD value, bool optimize=true
  );
  HRESULT D3DSetSamplerState
  (
    DWORD stage, D3DSAMPLERSTATETYPE_OLD state, DWORD value, bool optimize=true
  );
  HRESULT D3DSetRenderState
  (
    D3DRENDERSTATETYPE_OLD state, DWORD value, bool optimize=true
  );    
  #define D3DSetRenderStateOpt D3DSetRenderState
  #define D3DSetTextureStageStateOpt D3DSetTextureStageState
  #define D3DSetSamplerStateOpt D3DSetSamplerState
#endif

  //! Virtual method
  virtual bool LightsPresent() const {return (_pointLightsCount > 0) || (_spotLightsCount > 0);}
  //! Virtual method
  virtual int GetDynamicVariablesIdentifier(
    bool instancing, const SectionMaterialLODs &matLOD, int begSec, int endSec
  ) const;
  //! Virtual method
  virtual void SetHWToUndefinedState();
  //! Virtual method
  virtual void PBDrawingCleanup();
  //! Virtual method
  virtual void BeginPBCreation(PushBuffer *pb);
  //! Virtual method
  virtual void EndPBCreation();

  //! Method to compile and create a vertex shader
  void CreateShader(QOStrStream &hlslSource, CONST D3D10_SHADER_MACRO *pDefines,
                    LPCSTR pFunctionName, LPCSTR pProfile, DWORD flags, const InputElements &ie,
                    VertexShaderInfo &vertexShader);

  //!{ Wrappers for various device setting with pushbuffer recording
  void D3DSetSamplerStateR(DWORD stage, D3DSAMPLERSTATETYPE_OLD state, DWORD value);
  void D3DSetRenderStateR(D3DRENDERSTATETYPE_OLD state, DWORD value);
  void SetVertexShaderConstantR(int startRegister, const float *data);
  void SetVertexShaderConstant2R(int startRegister, const float *data);
  void SetVertexShaderConstantBR(int startRegister, const bool *data);
  void SetVertexShaderConstantB2R(int startRegister, const bool *data);
  void SetVertexShaderConstantIR(int startRegister, const int *data);
  void SetVertexShaderConstantMaxColorR();
  void SetVertexShaderConstantMLSunR(Color matAmbient, Color matDiffuse, Color matForcedDiffuse, Color matSpecular, Color matEmmisive, bool vsIsAS);
  void SetVertexShaderConstantMLNoneR(Color matEmmisive, bool vsIsAS);
  void SetVertexShaderConstantSAFRR(float matPower, Color matAmbient);
  void SetVertexShaderConstantFFARR();
  void SetVertexShaderConstantLWSMR();
  void SetTextureR(DWORD stage, const TextureD3DT *texture);
  void SetTexture0R(const TextureD3DT *texture);
  void SetPixelShaderR(const ComRef<ID3D10PixelShader> &ps);
  void SetPixelShaderConstantR(int startRegister, const float *data);
  void SetPixelShaderConstantDBSR(bool texture0MaxColorSeparate, ColorVal matDiffuse, ColorVal matForcedDiffuse, ColorVal matSpecular, float matPower);
  void SetPixelShaderConstantDBSZeroR();
  void SetPixelShaderConstant_A_DFORCED_E_SunR(bool texture0MaxColorSeparate, ColorVal matEmmisive, ColorVal matAmbient, ColorVal matForcedDiffuse);
  void SetPixelShaderConstant_A_DFORCED_E_NoSunR(ColorVal matEmmisive);
  void SetPixelShaderConstantWaterR();
  void SetPixelShaderConstantWaterSimpleR();
  void SetPixelShaderConstantGlassR(ColorVal matSpecular);
  void SetVertexShaderR(const VertexShaderInfo &vs);
  void SetVertexDeclarationR(const ComRef<ID3D10InputLayout> &decl);
  void DrawIndexedPrimitiveR(int beg, int end, const Shape &sMesh, int instancesOffset, int instancesCount);
  void SetBiasR(int bias);
  //!}

  // Method to forget buffers used by the device (function is used prior removing or locking buffers)
  void ForgetDeviceBuffers();

  //! Temporary setting of a render state
  /*!
    The state will be returned to it's previous value as soon as this object is destroyed
  */
  class SetRenderStateTemp
  {
  private:
    //! Engine
    static EngineDDT *_engine;
    //! State we set and then set back
    D3DRENDERSTATETYPE_OLD _state;
    //! Old value to set back
    DWORD _oldValue;
  public:
    //! Constructor - remember the old state and value, set the new one
    SetRenderStateTemp(D3DRENDERSTATETYPE_OLD state, DWORD value)
    {
      _engine->_renderState.Access(state);
      RenderStateInfo &info = _engine->_renderState[state];
//      if (state != D3DRS_FOGCOLOR_OLD)
//      {
//        DoAssert( info.value != -1);
//      }
      _oldValue = info.value;
      _state = state;
      _engine->D3DSetRenderState(state, value);
    }
    //! Destructor - restore the old state
    ~SetRenderStateTemp()
    {
      // If the state was not previously set, keep the current value
      if (_state != D3DRS_FOGCOLOR_OLD)
      {
        if (_oldValue == -1) return;
      }

      _engine->D3DSetRenderState(_state, _oldValue);
    }
    //! Setting of the engine pointer
    static void SetEngine(EngineDDT *engine) {_engine = engine;}
  };

  //! Temporary setting of a sampler state
  /*!
    The state will be returned to it's previous value as soon as this object is destroyed
  */
  class SetSamplerStateTemp
  {
  private:
    //! Engine
    static EngineDDT *_engine;
    //! Stage we set and then set back
    int _stage;
    //! State we set and then set back
    D3DSAMPLERSTATETYPE_OLD _state;
    //! Old value to set back
    DWORD _oldValue;
  public:
    //! Constructor - remember the old state and value, set the new one
    SetSamplerStateTemp(int stage, D3DSAMPLERSTATETYPE_OLD state, DWORD value)
    {
      _engine->_samplerState[stage].Access(state);
      RenderStateInfo &info = _engine->_samplerState[stage][state];
      //DoAssert(info.value != -1);
      _oldValue = info.value;
      _stage = stage;
      _state = state;
      _engine->D3DSetSamplerState(stage, state, value);
    }
    //! Destructor - restore the old state
    ~SetSamplerStateTemp()
    {
      // If the state was not previously set, keep the current value
      if (_oldValue == -1) return;

      _engine->D3DSetSamplerState(_stage, _state, _oldValue);
    }
    //! Setting of the engine pointer
    static void SetEngine(EngineDDT *engine) {_engine = engine;}
  };

  //! Scope "protection" of a vertex shader constant
  /*!
    The value will be returned to it's previous value as soon as this object is destroyed
  */
  class PushVertexShaderConstant: private ::NoCopy
  {
  protected:
    //! Engine
    static EngineDDT *_engine;
    //! State we set and then set back
    int _registerIndex;
    //! Old value to set back
    float _oldValue[4];
  public:
    //! Constructor - remember the old state and value, set the new one
    PushVertexShaderConstant(int registerIndex)
    {
      _oldValue[0] = _engine->_shaderConstValues[registerIndex][0];
      _oldValue[1] = _engine->_shaderConstValues[registerIndex][1];
      _oldValue[2] = _engine->_shaderConstValues[registerIndex][2];
      _oldValue[3] = _engine->_shaderConstValues[registerIndex][3];
      _registerIndex = registerIndex;
    }
    //! Destructor - restore the old state
    ~PushVertexShaderConstant()
    {
      _engine->SetVertexShaderConstantF(_registerIndex, _oldValue, 1);
    }
    //! Setting of the engine pointer
    static void SetEngine(EngineDDT *engine) {_engine = engine;}
  };

  //! Temporary setting of a vertex shader constant
  /*!
    The value will be returned to it's previous value as soon as this object is destroyed
  */
  class SetVertexShaderConstantTemp: public PushVertexShaderConstant
  {
  public:
    //! Constructor - remember the old state and value, set the new one
    SetVertexShaderConstantTemp(int registerIndex, const float *value)
    :PushVertexShaderConstant(registerIndex)
    {
      _engine->SetVertexShaderConstantF(registerIndex, value, 1);
    }
  };

  //! Temporary setting of a pixel shader constant
  /*!
    The value will be returned to it's previous value as soon as this object is destroyed
  */
  class PushPixelShaderConstant: private ::NoCopy
  {
  protected:
    //! Engine
    static EngineDDT *_engine;
    //! State we set and then set back
    int _registerIndex;
    //! Old value to set back
    float _oldValue[4];
  public:
    //! Constructor - remember the old state and value, set the new one
    PushPixelShaderConstant(int registerIndex)
    {
      _oldValue[0] = _engine->_pixelShaderConstValues[registerIndex][0];
      _oldValue[1] = _engine->_pixelShaderConstValues[registerIndex][1];
      _oldValue[2] = _engine->_pixelShaderConstValues[registerIndex][2];
      _oldValue[3] = _engine->_pixelShaderConstValues[registerIndex][3];
      _registerIndex = registerIndex;
    }
    //! Destructor - restore the old state
    ~PushPixelShaderConstant()
    {
      _engine->SetPixelShaderConstantF(_registerIndex, _oldValue, 1);
    }
    //! Setting of the engine pointer
    static void SetEngine(EngineDDT *engine) {_engine = engine;}
  };

  //! Temporary setting of a pixel shader constant
  /*!
    The value will be returned to it's previous value as soon as this object is destroyed
  */
  class SetPixelShaderConstantTemp: public PushPixelShaderConstant
  {
  public:
    //! Constructor - remember the old state and value, set the new one
    SetPixelShaderConstantTemp(int registerIndex, const float *value)
    :PushPixelShaderConstant(registerIndex)
    {
      _engine->SetPixelShaderConstantF(registerIndex, value, 1);
    }
  };

  //! Temporary setting of a texture
  /*!
    The value will be returned to it's previous value as soon as this object is destroyed
  */
  class SetTextureTemp
  {
  private:
    //! Engine
    static EngineDDT *_engine;
    //! Stage we set and then set back
    int _stage;
    //! Old texture to set back
    ID3D10ShaderResourceView *_oldTex;
  public:
    //! Constructor - remember the old state and value, set the new one
    SetTextureTemp(int stage, ID3D10ShaderResourceView *tex)
    {
      _oldTex = _engine->_lastHandle[stage];
      _stage = stage;
      _engine->SetTexture(stage, tex);
    }
    //! Destructor - restore the old state
    ~SetTextureTemp()
    {
      _engine->SetTexture(_stage, _oldTex);
    }
    //! Setting of the engine pointer
    static void SetEngine(EngineDDT *engine) {_engine = engine;}
  };

  WORD *QueueAdd( QueueT &queue, int n );
  void QueueFan( const VertexIndex *ii, int n );

  void Queue2DPoly( const TLVertex *v, int n );

  void FlushQueue(QueueT &queue, int index);
  void FlushAndFreeQueue(QueueT &queue, int index);

  int AllocateQueue(QueueT &queue, TextureD3DT *tex, int level, const TexMaterial *mat, int spec);
  void FreeQueue(QueueT &queue, int index);
  void FreeAllQueues(QueueT &queue);
  void FlushAndFreeAllQueues(QueueT &queue, bool nonEmptyOnly=false);
  void FlushAllQueues(QueueT &queue, int skip=-1);

  void CloseAllQueues(QueueT &queue);

  //void FlushQueue(){FlushQueue(_queue,index;
  //void Flush2DQueue();


  void SwitchRenderMode( RenderMode mode )
  {
    if (_renderMode==mode) return;
    DoSwitchRenderMode(mode);
  }
  void DoSwitchRenderMode( RenderMode mode );

  void D3DPreparePoint();
  void D3DPrepare3DLine();
  void DrawPoints( int beg, int end );
  void DrawLine( int beg, int end, int orFlags=0 );

  void DrawPolyPrepare
  (
    const MipInfo &mip, int specFlags=DefSpecFlags2D
  );
  void DrawPolyDo
  (
    const Vertex2DPixel *vertices, int nVertices, const Rect2DPixel &clip=Rect2DClipPixel
  );
  void DrawPolyDo
  (
    const Vertex2DAbs *vertices, int nVertices, const Rect2DAbs &clip=Rect2DClipAbs
  );

  virtual void DrawPoly3D(Matrix4Par space, const MipInfo &mip, const Vertex2DFloat *vertices, int nVertices, const Rect2DFloat &clip, int specFlags, int matType, const Pars3D *pars3D=NULL);

  void DoPrepareTriangle(const MipInfo &absMip, const TexMaterialLODInfo &mat, int specFlags, const EngineShapeProperties &prop);
  void QueuePrepareTriangle( const MipInfo &absMip, const TexMaterial *mat, int specFlags );
  void DoPrepareTriangle(TextureD3DT *tex, const TexMaterialLODInfo &mat, int level, int spec, const EngineShapeProperties &prop);

  void PrepareTriangle( const MipInfo &absMip, const TexMaterial *mat, int specFlags );
  virtual void PrepareTriangleTL(
    const PolyProperties *section,
    const MipInfo &mip, const TexMaterialLODInfo &mat, int specFlags,
    const TLMaterial &tlMat, int spec, const EngineShapeProperties &prop
  );

  void DrawPolygon(const VertexIndex *ii, int n);
  void DrawSection(const FaceArray &face, Offset beg, Offset end);


  // integrated transform&lighting
  bool GetTL() const {return true;}
  bool HasWBuffer() const {return _useWBuffer;}
  //bool HasWBuffer() const {return false;}

  virtual float GetLastKnownAvgIntensity() const;
  virtual void ForgetAvgIntensity();

  // engine w-buffer interface
  virtual bool IsWBuffer() const;
  virtual bool CanWBuffer() const;
  virtual void SetWBuffer(bool val);

  virtual bool CanTwoSidedStencil() const {return _canTwoSidedStencil;}

  //! Setup all the lights for one model (main light and P&S lights)
  void SetupLights(const LightList &lights, int spec);
  //! Combine material and light colors
  void SetupMaterialLights(const TLMaterial &mat, bool vsIsAS);
  void DoSwitchTL( TLMode active );

  //void AddVertices( QueueT &queue, const void *v, int n, int vSize );

  void DiscardVB();
  bool AddVertices( const TLVertex *v, int n );
  //void AddTLVertices( const SVertexT *v, int n );

public:
  //! Setting of texgen scale and offset (usually called when setting the VB)
  void SetTexGenScaleAndOffset(const UVPair *scale, const UVPair *offset);
  /// helper: used from VertexBufferD3DT::Update
  void SetVSourceStatic(const Shape &src, const EngineShapeProperties &prop, const VertexBufferD3DT &buf, VSDVersion vsdVersion);
  void SetVSourceDynamic
  (
    const VertexTableAnimationContext *animContext, const Shape &src, const EngineShapeProperties &prop, const VertexBufferD3DT &buf
  );


public:
  void SwitchTL( TLMode active )
  {
    if (active==_tlActive) return;
    DoSwitchTL(active);
  }
  //! Set the current material, combine material color with lights
  void DoSetMaterial(const TLMaterial &mat, bool vsIsAS);
  //void SetMaterial(const TLMaterial &mat, const LightList &lights, int spec);

  void DoSetMaterial2D()
  {
    SelectPixelShaderSpecular(PSSNormal);
  }

  void ProjectionChanged();
  void EnableSunLight(EMainLight mainLight);
  void SetFogMode(EFogMode fogMode);

  int ShapePropertiesNeeded(const TexMaterial *mat) const;
  bool BeginMeshTLInstanced(const Shape &sMesh, const EngineShapeProperties &prop, int spec, const LightList &lights, int bias, float minDist2, const DrawParameters &dp);
  virtual void SetupInstances(const ObjectInstanceInfo *instances, int nInstances);
  virtual void SetupInstancesSprite(const ObjectInstanceInfo *instances, int nInstances);
  void SetupInstancesShadowVolume(const ObjectInstanceInfo *instances, int nInstances);
  //! Function to draw all sections in instanced version of drawing
  void DrawMeshTLSectionsInstanced(const Shape &sMesh, const SectionMaterialLODs &matLOD, const EngineShapeProperties &prop, int spec, int bias, int instancesOffset, int instancesCount);
  //! Instanced version of drawing
  virtual void DrawMeshTLInstanced(
    const Shape &sMesh, const SectionMaterialLODs &matLOD, const EngineShapeProperties &prop, int spec, const LightList &lights, int bias,
    const ObjectInstanceInfo *instances, int nInstances, float minDist2, const DrawParameters &dp
  );
  virtual void DrawShadowVolumeInstanced(LODShape *shape, int level, const ObjectInstanceInfo *instances, int instanceCount);

  void ClearLights();

  //void PrepareMeshTL( const LightList &lights, const Matrix4 &modelToWorld, int spec ); // prepare internal variables
  virtual void BeginShadowVolume(float shadowVolumeSize);
  //! Function to calculate closest Z^2 upon specified mindist2 and Engine's aspect settings
  float GetClosestZ2(float minDist2);
  virtual void BeginInstanceTL(const Matrix4 &modelToWorld, float minDist2, int spec, const LightList &lights, const DrawParameters &dp);
  virtual bool BeginMeshTL(const VertexTableAnimationContext *animContext, const Shape &sMesh, const EngineShapeProperties &prop, int spec, const LightList &lights, int bias); // convert all mesh vertices
  virtual void EndMeshTL( const Shape &sMesh ); // forget mesh

  virtual void BeginVisibilityTesting();
  virtual IVisibilityQuery *QueryVisibility(IVisibilityQuery *query, ClipFlags clipFlags, const Vector3 *minmax, const FrameBase &pos);
  virtual void EndVisibilityTesting();

  virtual IVisibilityQuery *BegPixelCounting(IVisibilityQuery *query);
  virtual void EndPixelCounting(IVisibilityQuery *query);
  
  virtual bool IsCursorSupported() const;
  virtual bool IsCursorShown() const;
  virtual void SetCursor(Texture *cursor, int x, int y, float hsX, float hsY, PackedColor color);
  virtual void UpdateCursorPos(int x, int y);

private:
  
  void ShowDeviceCursor(bool showIt);
  
public:
  //!{ Methods to control the ShadowMapRendering
  virtual bool IsSBPossible() const
  {
    Fail("DX10 TODO");
    return false;
    // Direct3D 10 NEEDS UPDATE 
//    return _sbTechnique != SBT_None;
  }
  virtual void EnableSBShadows(int quality);
  virtual bool IsSBEnabled() const;
  virtual void BeginShadowMapRendering(float sunClosestDistanceFrom0);
  virtual void EndShadowMapRendering();
  //!}

  //!{ Methods to control the depth of field effect
  /// some engines may be able to implement depth of field
  virtual void EnableDepthOfField(float focusDist, float blur, bool farOnly);
  virtual void EnableDepthOfField(int quality);
  virtual bool IsDepthOfFieldEnabled() const;
  virtual void BeginDepthMapRendering();
  virtual void EndDepthMapRendering();
  virtual void SetDepthClip(float zClip);
  //!}

  //! Method to fill out the PS constants with craters properties
  virtual void UseCraters(const AutoArray<CraterProperties> &craters, int maxCraters, const Color &craterColor);

  //!{ Methods to control the low-end quality settings (related to low-end PS for trees)
  virtual void SetLowEndQuality();
  virtual bool GetLowEndQuality() const {return _lowEndQuality;}
  virtual bool TextureShouldBeUsed(const Texture *tex) const;
  //!}
  
  //! Lazy setting of the shader
  __forceinline void SetupSectionTL(EMainLight mainLight, EFogMode fogMode,
    VertexShaderID vertexShaderID, const UVSource *uvSource, int uvSourceCount, bool uvSourceOnlyTex0,
    ESkinningType skinningType,
    bool instancing, int instancesCount, int pointLightsCount, int spotLightsCount,
    bool shadowReceiver,
    bool alphaShadowEnabled, RenderingMode renderingMode, bool backCullDisabled, bool craterRendering)
  {
    if ((_currMainLight == mainLight) &&
      (_currFogMode == fogMode) &&
      (_currVertexShaderID == vertexShaderID) &&
      (_currUVSourceOnlyTex0 == uvSourceOnlyTex0) &&
      (_currUVSourceCount == uvSourceCount))
      {
        int i;
        for (i = 0; i < uvSourceCount; i++)
        {
          if (_currUVSource[i] != uvSource[i]) goto DoSetup;
        }
        if ((_vertexShaderLast._shader.NotNull()) &&
          (_currSkinningType == skinningType) &&
          (_currInstancing == instancing) &&
          ((!_currInstancing) || (_currInstancesCount == instancesCount)) &&
          (_currPointLightsCount == pointLightsCount) &&
          (_currSpotLightsCount == spotLightsCount) &&
          (_currShadowReceiver == shadowReceiver) &&
          (_currAlphaShadowEnabled == alphaShadowEnabled) &&
          (_currRenderingMode == renderingMode) &&
          (_currBackCullDisabled == backCullDisabled) &&
          (_currCraterRendering == craterRendering))
          {
            return;
          }
      }
DoSetup:
    DoSetupSectionTL(mainLight, fogMode, vertexShaderID, uvSource, uvSourceCount, uvSourceOnlyTex0, skinningType, instancing, instancesCount, pointLightsCount, spotLightsCount, shadowReceiver, alphaShadowEnabled, renderingMode, backCullDisabled, craterRendering);
  }
  //! Setting of the shader
  void DoSetupSectionTL(EMainLight mainLight, EFogMode fogMode,
    VertexShaderID vertexShaderID, const UVSource *uvSource, int uvSourceCount, bool uvSourceOnlyTex0,
    ESkinningType skinningType,
    bool instancing, int instancesCount, int pointLightsCount, int spotLightsCount,
    bool shadowReceiver,
    bool alphaShadowEnabled, RenderingMode renderingMode, bool backCullDisabled, bool craterRendering);
  //!{ Sets the VS constants
  void SetupVSConstantsPSLights();
  void SetupVSConstantsLODPars();
  void SetupVSConstantsPool(int minBoneIndex, int bonesCount);
  void SetupVSConstantsShadowVolume(int minBoneIndex, int bonesCount);
  void SetupVSConstantsSprite();
  void SetupVSConstantsWater();
  void SetupVSConstantsShore();
  void SetupVSConstantsTerrain();
  void SetupVSConstantsTerrainGrass();
  bool SetupVSConstants(int minBoneIndex, int bonesCount);
  //!}
  //! Sets the PS constants
  void SetupPSConstants();


  virtual void DrawSectionTL(const Shape &sMesh, int beg, int end, int bias, int instancesOffset = 0, int instancesCount = 0, const DrawParameters &dp = DrawParameters::_default);
#if 0
  virtual void RememberAndVerifyHWState();
#endif
  void DrawStencilShadows(bool interior, int quality);

  void DrawDecal(Vector3Par screen, float rhw, float sizeX, float sizeY, PackedColor color, const MipInfo &mip, const TexMaterial *mat, int specFlags);
  bool ClipDraw2D
  (
    float &xBeg, float &yBeg, float &xEnd, float &yEnd,
    float &uBeg, float &vBeg, float &uEnd, float &vEnd,
    const Draw2DParsExt &pars, const Rect2DAbs &rect, const Rect2DAbs &clip
  );
  void Draw2D
  (
    const Draw2DParsExt &pars, const Rect2DAbs &rect, const Rect2DAbs &clip
  );
  virtual void Draw2D
  (
    const Draw2DPars &pars, const Rect2DAbs &rect, const Rect2DAbs &clip=Rect2DClipAbs
  );
  virtual void Draw2D
  (
    const Draw2DParsNoTex &pars, const Rect2DAbs &rect, const Rect2DAbs &clip=Rect2DClipAbs
  );
  void DrawLinePrepare();
  void DrawLineDo
  (
    const Line2DAbs &line,
    PackedColor c0, PackedColor c1,
    const Rect2DAbs &clip=Rect2DClipAbs
  );
  //void D3DTextState();

  void D3DDrawTexts();

  void D3DConstruct();
  void D3DDestruct();
  void TextureDestroyed( Texture *tex );

  void D3DTextureDestroyed(TextureD3DHandle tex);

  void D3DCloseDraw();
  void D3DBeginScene();
  void D3DEndScene();
public:
  void CreateVB();
  void DestroyVB();

  void CreateVBTL();
  void DestroyVBTL();

  void RestoreVB();
  
  void DDError9( const char *text, int err );
  
  bool CanRestore();
  void FogColorChanged( ColorVal fogColor );
  virtual void EnableNightEye(float night);
  virtual void EnableAlphaShadow(bool enable){_alphaShadowEnabled = enable;}

  void PrepareMesh( int spec ); // prepare internal variables
  bool BeginMesh( TLVertexTable &mesh, int spec ); // convert all mesh vertices
  void EndMesh( TLVertexTable &mesh ); // forget mesh

  void SetDebugMarker(int id);
  int ProfileBeginGraphScope(unsigned int color,const char *name) const;
  int ProfileEndGraphScope() const;

  void InitVertexShaders(VertexShaderID vs = NVertexShaderID);
  void DeinitVertexShaders();
  void ReloadVertexShader(VertexShaderID vsID);
  void InitPixelShaders(SBTechnique sbTechnique);
  void DeinitPixelShaders();

  //! Selecting specular/non-specular version of the pixel shader
  void SelectPixelShaderSpecular(PixelShaderSpecular spec) {_pixelShaderSpecularSel = spec;}
  //! Selecting the pixel shader (like NormalMap)
  void SelectPixelShader(PixelShaderID ps)
  {
    _pixelShaderSel = ps;
    switch (ps)
    {
      case PSDetailMacroAS:
      case PSNormalMapMacroAS:
      case PSNormalMapDiffuseMacroAS:
      case PSNormalMapMacroASSpecularMap:
      case PSNormalMapDetailMacroASSpecularMap:
        _texture0MaxColorSeparate = true;
        break;
      default:
        _texture0MaxColorSeparate = false;
        break;
    }
  }
  //! Selecting type of the pixel shader (like shadowReceiver, no shadows)
  void SelectPixelShaderType(PixelShaderType pixelShaderType) {_pixelShaderTypeSel = pixelShaderType;}
  //! Setting the pixel shader on the GPU
  void DoSelectPixelShader(const DrawParameters &dp = DrawParameters::_default);


  void CheckFreeTextureMemory(int &local, int &nonlocal);
  
  struct CheckMemTypeScope
  {
    #if _ENABLE_CHEATS
      #define TRACK_VRAM_ALLOC 0
      //#define TRACK_VRAM_ALLOC 1
    #endif
    #if TRACK_VRAM_ALLOC
      EngineDDT *_engine;
      int _local,_nonlocal,_size;
      const char *_text;
    #endif
    
    CheckMemTypeScope(EngineDDT *engine, const char *text, int size)
    {
      #if TRACK_VRAM_ALLOC
        _engine = engine;
        _text = text;
        _size = size;
        _engine->CheckFreeTextureMemory(_local,_nonlocal);
      #endif
    }
    
    ~CheckMemTypeScope()
    {
      #if TRACK_VRAM_ALLOC
        int local,nonlocal;
        _engine->CheckFreeTextureMemory(local,nonlocal);
        
        LogF("%s - %d: loc %d,nonloc %d",_text,_size,_local-local,_nonlocal-nonlocal);
      #endif
    }
  };
  /// verify current estimations
  void CheckLocalNonlocalTextureMemory();
  /// create resources needed for shadow buffers
  void CreateSBResources();
  /// destroy resources needed for shadow buffers
  void DestroySBResources();

  /// Create resources needed for depth of field
  void CreateDBResources();
  /// Destroy resources needed for depth of field
  void DestroyDBResources();

  /// get approximate adapter performance
  int GetAdapterLevel(int adapter) const;
  
  /// test if fog is SRGB or linear
  void Init3DState();
  void Done3DState();
  void FindMode(int adapter); // prepare presentation parameters based on _w, _h, _pixelSize
  void Init3D();
  void Done3D();
  // make sure adapter is selected and _deviceLevel set
  void InitAdapter();
  //void SetD3DBias( int bias );

  float ZShadowEpsilon() const {return 0.01;}
  float ZRoadEpsilon() const {return 0.005;}

  float ObjMipmapCoef() const {return 1.5;}
  float LandMipmapCoef() const {return 1.0;}

  bool ShadowsFirst() const {return false;}
  bool SortByShape() const {return true;}

  private:
  // helper for various freeing implementations
  size_t FreeOneItem(int minAge);
  size_t Free(size_t size, int minAge);
  
  public:
  //@{ implementation of MemoryFreeOnDemandHelper
  virtual float Priority() const;
  virtual size_t MemoryControlled() const;
  virtual size_t MemoryControlledRecent() const;
  virtual size_t FreeOneItem();
  virtual void MemoryControlledFrame();
  //@}

  //@{ VRAM management - vertex buffers only
  size_t ReleaseVideoMemory(size_t size, int minAge);
  size_t ThrottleVRAMAllocation(size_t limit, int minAge);
  size_t UsedVRAM() const;
  /// VRAM discard metrics for current discard candidate
  float VRAMDiscardMetrics() const;
  int VRAMDiscardCountAge(int minAge, int *totalSize=NULL) const;
  //@}

  #if _ENABLE_CHEATS
  RString GetStat(int statId, RString &statVal, RString &statVal2);
  #endif
};

#define GEngineDD static_cast<EngineDDT *>(GEngine)

// XBOX / Win32 specific texture format definition

#define tex_A1R5G5B5 D3DFMT_A1R5G5B5
#define tex_A4R4G4B4 D3DFMT_A4R4G4B4
#define tex_A8R8G8B8 D3DFMT_A8R8G8B8
#define tex_R5G6B5 D3DFMT_R5G6B5
#define tex_A8L8 D3DFMT_A8L8

#include <Dxerr8.h>
#define DX_CHECK(hr) \
  if (hr!=D3D_OK) \
    { \
    DXTRACE_ERR("Check",hr); \
    Assert(false); \
    }

#define DX_CHECKN(n,hr) \
  if (hr!=D3D_OK) \
    { \
    DXTRACE_ERR(n,hr); \
    Assert(false); \
    }

#endif // !_DISABLE_GUI

#endif

