#ifdef _MSC_VER
#pragma once
#endif

#ifndef __ENGDDXB_HPP
#define __ENGDDXB_HPP

#if !_DISABLE_GUI || defined _XBOX

// implementation of DirectDraw engine
// without actual triangle drawing
// so that different implementations (including Direct3D) are possible

#include "d3dXBdefs.hpp"

#include <Es/Containers/array.hpp>
#include <Es/Containers/staticArray.hpp>
#include <El/FreeOnDemand/memFreeReq.hpp>
#include "../types.hpp"
#include "../engine.hpp"
#include "../Shape/plane.hpp"
#include "../tlVertex.hpp"
#include "shaders/FPShadersDecl.h"

void DDErrorXB( const char *text, int err );

enum TexLoc {TexLocalVidMem,TexNonlocalVidMem,TexSysMem};

enum
{
  // size of vertex and index buffers for 2D or SW TL transformed vertex data
#ifdef _XBOX
  MeshBufferLength=2*1024,
  IndexBufferLength=2*1024
#else
  MeshBufferLength=32*1024,
  IndexBufferLength=4*1024
#endif
};

#ifdef _XBOX
#define CUSTOM_VB_DISCARD 1
#else
#define CUSTOM_VB_DISCARD 0
#endif

#if CUSTOM_VB_DISCARD
// cpu may be two frames ahead - it makes no sense to have more than three buffers
const int NumDynamicVB3D = 3;
const int NumDynamicVB2D = 3;
#else
const int NumDynamicVB3D = 1;
const int NumDynamicVB2D = 1;
#endif

#ifndef _XBOX
#define PIXEL_SHADERS 1
#else
#define PIXEL_SHADERS 1
#endif

#if PIXEL_SHADERS

enum PixelShaderSpecular
{
  PSSSpecular,
  PSSNormal,
  NPixelShaderSpecular,
  PSSUninitialized = -1
};

#endif

#include "physHeap.hpp"
  
// several queues for different textures
// other states must be same

class TexMaterial;
class TextureD3DXB;

struct TriQueueXB
{
  //int _queueBase,_queueSize; // _triangleQueue position in VB

  StaticArray<WORD> _triangleQueue; // see BeginMesh,EndMesh

  RefR<TextureD3DXB> _texture; // which texture
  RefR<const TexMaterial,const RefCount> _material; // what material
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
  MaxTriQueues=8, // 16 - 32 seems to be reasonable
  TriQueueSize=2048
};

struct QueueXB
{
  ComRef<IDirect3DVertexBuffer8> _meshBuffer[NumDynamicVB2D];
  ComRef<IDirect3DIndexBuffer8> _indexBuffer[NumDynamicVB2D];
  int _vertexBufferUsed; // how many vertices are in vertex buffer
  int _indexBufferUsed; // how many indices are is index buffer

  int _meshBase,_meshSize; // _mesh position in VB
  int _actualVBIndex;
  int _actualIBIndex;

  TriQueueXB _tri[MaxTriQueues];
  bool _triUsed[MaxTriQueues];
  // queue 0 is used only for alpha rendering
  int _actTri; // queue assigned by PrepareTriangle call

  int _usedCounter; // used for LRU tracking
  bool _firstVertex; // first call in scene should always use DISCARD
  bool _firstIndex;

  QueueXB();
  int Allocate
    (
    TextureD3DXB *tex, int level, const TexMaterial *mat, int spec,
    int minI, int maxI, int tip
    );
  void Free( int i );


  const ComRef<IDirect3DVertexBuffer8> &GetVBuffer() const {return _meshBuffer[_actualVBIndex];}
  const ComRef<IDirect3DIndexBuffer8> &GetIBuffer() const {return _indexBuffer[_actualIBIndex];}
};

#ifdef _XBOX
#define COMPRESSED_NORMALS 1
#else
#define COMPRESSED_NORMALS 0
#endif

#if COMPRESSED_NORMALS
#pragma pack(push,1)
struct UVPairCompressed
{
  short u, v;
};
struct Vector3Compressed
{
  short x, y, z;
};
struct SVertexXB
{
  Vector3Compressed pos;
  DWORD norm; // packed normals use much less memory
  UVPairCompressed t0;
};
struct SVertexNormalXB
{
  Vector3Compressed pos;
  DWORD norm;
  UVPairCompressed t0;
  DWORD s;
  DWORD t;
};
struct SVertexNormalAlphaXB
{
  Vector3Compressed pos;
  DWORD norm;
  UVPairCompressed t0;
  DWORD s;
  DWORD t;
  BYTE alpha;
};

struct SVertexInstancedXB
{
  Vector3Compressed pos;
  DWORD norm; // packed normals use much less memory
  UVPairCompressed t0;
  BYTE index;
};
struct SVertexNormalInstancedXB
{
  Vector3Compressed pos;
  DWORD norm;
  UVPairCompressed t0;
  DWORD s;
  DWORD t;
  BYTE index;
};
struct SVertexSpriteInstancedXB
{
  BYTE t0u;
  BYTE t0v;
  BYTE index;
};
struct SVertexSkinnedXB
{
  Vector3Compressed pos;
  DWORD norm;
  UVPairCompressed t0;
  BYTE weights[4];
  BYTE indices[4];
};
struct SVertexNormalSkinnedXB
{
  Vector3Compressed pos;
  DWORD norm;
  UVPairCompressed t0;
  DWORD s;
  DWORD t;
  BYTE weights[4];
  BYTE indices[4];
};
struct SVertexShadowVolumeXB
{
  Vector3Compressed pos;
  DWORD norm;
};
struct SVertexShadowVolumeInstancedXB
{
  Vector3Compressed pos;
  DWORD norm;
  BYTE index;
};
struct SVertexShadowVolumeSkinnedXB
{
  Vector3Compressed pos;
  BYTE weights[4];
  BYTE indices[4];
  Vector3Compressed posA;
  BYTE weightsA[4];
  BYTE indicesA[4];
  Vector3Compressed posB;
  BYTE weightsB[4];
  BYTE indicesB[4];
};
struct SVertexPointXB
{
  DWORD pos;
  BYTE t0u;
  BYTE t0v;
  DWORD color;
};
#pragma pack(pop)
#else
struct SVertexXB
{
  Vector3P pos;
  Vector3P norm;
  UVPair t0;
};
struct SVertexNormalXB
{
  Vector3P pos;
  Vector3P norm;
  UVPair t0;
  Vector3P s;
  Vector3P t;
};
struct SVertexNormalAlphaXB
{
  Vector3P pos;
  Vector3P norm;
  UVPair t0;
  Vector3P s;
  Vector3P t;
  // what uncompressed representation would work well?
  //float alpha;
};
struct SVertexSkinnedXB
{
  Vector3P pos;
  Vector3P norm;
  UVPair t0;
  float weights[4];
  float indices[4];
};
struct SVertexNormalSkinnedXB
{
  Vector3P pos;
  Vector3P norm;
  UVPair t0;
  Vector3P s;
  Vector3P t;
  float weights[4];
  float indices[4];
};
struct SVertexShadowVolumeXB
{
  Vector3P pos;
  Vector3P norm;
};
struct SVertexShadowVolumeInstancedXB
{
  Vector3P pos;
  Vector3P norm;
  BYTE index;
};
struct SVertexShadowVolumeSkinnedXB
{
  Vector3P pos;
  float weights[4];
  float indices[4];
  Vector3P posA;
  float weightsA[4];
  float indicesA[4];
  Vector3P posB;
  float weightsB[4];
  float indicesB[4];
};
#endif

//! User of the texture
/*!
Descendant of this class can be registered with the texture and important events of the texture
(like recreating) will be let known to the object.
*/
class TextureLoadHandler : public TLinkBidirD
{
public:
  //! In case the content of the texture has changed this function is beeing called
  virtual void TextureHasChanged() {};
};

enum FEType
{
  FET_Texture,
  FET_MaxColor,
  FET_AvgColor,
  FET_Diffuse,
  FET_DiffuseBack,
  FET_Specular,
};

class FixupEntry : public RefCount
{
protected:
  DWORD _pbOffset;
public:
  explicit FixupEntry(DWORD pbOffset)
  {
    _pbOffset = pbOffset;
  }
  //! Constructor used before serialization
  FixupEntry()
  {
    _pbOffset = 0xFFFFFFFF;
  }
  ~FixupEntry()
  {
  }
  virtual void Apply(IDirect3DPushBuffer8 *pb) = NULL;
  virtual bool HasChanged() = NULL;
  virtual void Reset() = NULL;
  virtual void StoreResources(RefArray<Texture> &usedTextures) {}
  virtual FEType FixupEntryType() = NULL;
  virtual bool SerializeBin(SerializeBinStream &f);
};

class PushBufferXB;

class FETextureHandle : public FixupEntry, public TextureLoadHandler
{
protected:
  RefR<TextureD3DXB> _tex;
  //! Pointer to the push buffer - owner of this fixup entry
  /*!
  Note that this can't be the reference, because the owner couldn't be deleted then
  */
  PushBufferXB *_pb;
  //! Flag to determine that the texture has been changed
  bool _textureHasChanged;
public:
  FETextureHandle(DWORD pbOffset, TextureD3DXB *tex, PushBufferXB *pb);
  //! Constructor used before serialization
  FETextureHandle(PushBufferXB *pb);
  virtual void TextureHasChanged();
  virtual bool HasChanged()
  {
    return _textureHasChanged;
  }
  virtual void Reset()
  {
    _textureHasChanged = true;
  }
  virtual bool SerializeBin(SerializeBinStream &f);
};

class FETexture : public FETextureHandle
{
private:
  int _stage;
public:
  FETexture(DWORD pbOffset, int stage, TextureD3DXB *tex, PushBufferXB *pb) : FETextureHandle(pbOffset, tex, pb)
  {
    _stage = stage;
  }
  //! Constructor used before serialization
  FETexture(PushBufferXB *pb) : FETextureHandle(pb)
  {
    _stage = -1;
  }
  virtual void Apply(IDirect3DPushBuffer8 *pb);
  virtual void StoreResources(RefArray<Texture> &usedTextures);
  virtual FEType FixupEntryType() {return FET_Texture;}
  virtual bool SerializeBin(SerializeBinStream &f);
};

class FEMaxColor : public FETextureHandle
{
private:
  D3DRENDERSTATETYPE _rs;
public:
  FEMaxColor(DWORD pbOffset, D3DRENDERSTATETYPE rs, TextureD3DXB *tex, PushBufferXB *pb) : FETextureHandle(pbOffset, tex, pb)
  {
    _rs = rs;
  }
  //! Constructor used before serialization
  FEMaxColor(PushBufferXB *pb) : FETextureHandle(pb)
  {
    _rs = D3DRS_FORCE_DWORD;
  }
  virtual void Apply(IDirect3DPushBuffer8 *pb);
  virtual FEType FixupEntryType() {return FET_MaxColor;}
  virtual bool SerializeBin(SerializeBinStream &f);
};

class FEAvgColor : public FETextureHandle
{
private:
  D3DRENDERSTATETYPE _rs;
public:
  FEAvgColor(DWORD pbOffset, D3DRENDERSTATETYPE rs, TextureD3DXB *tex, PushBufferXB *pb) : FETextureHandle(pbOffset, tex, pb)
  {
    _rs = rs;
  }
  //! Constructor used before serialization
  FEAvgColor(PushBufferXB *pb) : FETextureHandle(pb)
  {
    _rs = D3DRS_FORCE_DWORD;
  }
  virtual void Apply(IDirect3DPushBuffer8 *pb);
  virtual FEType FixupEntryType() {return FET_AvgColor;}
  virtual bool SerializeBin(SerializeBinStream &f);
};

class FEDiffuse : public FixupEntry
{
private:
  D3DRENDERSTATETYPE _rs;
  EMainLight _mainLight;
  Color _matDiffuse;
  Color _currSunDiffuse;
public:
  FEDiffuse(DWORD pbOffset, D3DRENDERSTATETYPE rs, EMainLight mainLight, Color matDiffuse) : FixupEntry(pbOffset)
  {
    _rs = rs;
    _mainLight = mainLight;
    _matDiffuse = matDiffuse;
    Reset();
  }
  //! Constructor used before serialization
  FEDiffuse()
  {
    _rs = D3DRS_FORCE_DWORD;
    _mainLight = ML_None;
    _matDiffuse = Color(-1, -1, -1, -1);
    Reset();
  }
  virtual void Apply(IDirect3DPushBuffer8 *pb);
  virtual bool HasChanged();
  virtual void Reset()
  {
    _currSunDiffuse = Color(-1, -1, -1, -1);
  }
  virtual FEType FixupEntryType() {return FET_Diffuse;}
  virtual bool SerializeBin(SerializeBinStream &f);
};

class FEDiffuseBack : public FixupEntry
{
private:
  D3DRENDERSTATETYPE _rs;
  EMainLight _mainLight;
  Color _matDiffuse;
  /// taken from _matForcedDiffuse.A()
  float _backCoef;
  Color _currSunDiffuse;
public:
  FEDiffuseBack(DWORD pbOffset, D3DRENDERSTATETYPE rs, EMainLight mainLight, Color matDiffuse, float backCoef) : FixupEntry(pbOffset)
  {
    _rs = rs;
    _mainLight = mainLight;
    _matDiffuse = matDiffuse;
    _backCoef = backCoef;
    Reset();
  }
  //! Constructor used before serialization
  FEDiffuseBack()
  {
    _rs = D3DRS_FORCE_DWORD;
    _mainLight = ML_None;
    _matDiffuse = Color(-1, -1, -1, -1);
    _backCoef = -1;
    Reset();
  }
  virtual void Apply(IDirect3DPushBuffer8 *pb);
  virtual bool HasChanged();
  virtual void Reset()
  {
    _currSunDiffuse = Color(-1, -1, -1, -1);
  }
  virtual FEType FixupEntryType() {return FET_DiffuseBack;}
  virtual bool SerializeBin(SerializeBinStream &f);
};

class FESpecular : public FixupEntry
{
private:
  D3DRENDERSTATETYPE _rs;
  EMainLight _mainLight;
  Color _matSpecular;
  Color _currSunDiffuse;
public:
  FESpecular(DWORD pbOffset, D3DRENDERSTATETYPE rs, EMainLight mainLight, Color matSpecular) : FixupEntry(pbOffset)
  {
    _rs = rs;
    _mainLight = mainLight;
    _matSpecular = matSpecular;
    Reset();
  }
  //! Constructor used before serialization
  FESpecular()
  {
    _rs = D3DRS_FORCE_DWORD;
    _mainLight = ML_None;
    _matSpecular = Color(-1, -1, -1, -1);
    Reset();
  }
  virtual void Apply(IDirect3DPushBuffer8 *pb);
  virtual bool HasChanged();
  virtual void Reset()
  {
    _currSunDiffuse = Color(-1, -1, -1, -1);
  }
  virtual FEType FixupEntryType() {return FET_Specular;}
  virtual bool SerializeBin(SerializeBinStream &f);
};

#define CUSTOM_PB_ALLOCATOR 1

#if CUSTOM_PB_ALLOCATOR
  struct PushBufferHandle
  {
    IDirect3DPushBuffer8 *com;
    PhysMemHeapItemXB mem;
    
    PushBufferHandle():com(NULL){}
    
    IDirect3DPushBuffer8 *operator -> () const {return com;}
    IDirect3DPushBuffer8 *GetRef() const {return com;}
    
    bool NotNull() const {return com!=NULL;}
  };
#else
  typedef ComRef<IDirect3DPushBuffer8> PushBufferHandle;
#endif


class PushBufferXB : public PushBuffer
{
  friend class EngineDDXB;
  PushBufferHandle _pb;
#if _ENABLE_PERFLOG
  int _tris;
#endif
  RefArray<FixupEntry> _fixupEntries;
  //! Fixup ready to be used
  ComRef<IDirect3DFixup8> _fuToSet;
  //! Minimal size of the fixup to use
  DWORD _fixupSize;
  //! Offset of the SetStreamSource (SetVertexShaderInputDirect) command in the push buffer
  DWORD _setStreamSourceOffset;
  //! Flag to determine that some texture referenced by some fixupEntry has been changed
  bool _textureHasChanged;
  //! Last used night eye factor
  float _nightEye;
  //! Last used sun diffuse color
  Color _sunDiffuse;

  //! Calculates the size of the fixup and stores it in the _fixupSize variable
  void CalculateFixupSize();
  //! Aplies fixups
  void ApplyFixups();
public:
  PushBufferXB();
  ~PushBufferXB();
  
  virtual bool Init(Shape *shape, const LODShape *lodShape, RefArray<Texture> *usedTextures, int instancesCount);
  virtual void Prepare(const Shape &shape, int spec);
  virtual void Run(const Shape &shape, bool instancing);
  virtual size_t GetMemoryControlled() const;
  virtual bool SerializeBin(SerializeBinStream &f);
  virtual void VertexBufferHasChanged(Ref<VertexBuffer> &buffer, VBType type, EVertexDecl vertexDeclaration, bool instanced);
  void TextureHasChanged()
  {
    _textureHasChanged = true;
  }
};

class VertexDynamicDataXB
{
  friend class EngineDDXB;

  ComRef<IDirect3DVertexBuffer8> _vBuffer[NumDynamicVB3D];
  int _vBufferSize; // reserved sizes of resources
  int _vBufferUsed; // used sizes
  int _actualVBIndex;

  const ComRef<IDirect3DVertexBuffer8> &GetVBuffer() const
  {
    return _vBuffer[_actualVBIndex];
  }

public:
  VertexDynamicDataXB();

  void Init(int size, IDirect3DDevice8 *dev);
  void Dealloc();


  int AddVertices(const Shape &src);
  //! Allocates space for n vertices and returns pointer to the vertices array to be filled out
  int Lock(SVertexXB *&vertex, int n);
  //! Unlocks the vertex buffer
  void Unlock();
};

#include "../heap.hpp"

typedef unsigned int HeapIndex;

typedef Heap<HeapIndex,int> IndexHeapXB;
typedef Heap<HeapIndex,int> VertexHeapXB;

typedef IndexHeapXB::HeapItem IndexAllocXB;
typedef VertexHeapXB::HeapItem VertexAllocXB;

class IndexStaticDataXB: public RefCountWithLinks
{
  friend class EngineDDXB;

private:

  ComRef<IDirect3DIndexBuffer8> _iBuffer;
  EngineDDXB *_engine;

  IndexHeapXB _manager;

  int _nShapes;

public:
  IndexStaticDataXB();
  ~IndexStaticDataXB();

  void Init(EngineDDXB *engine, int indices, IDirect3DDevice8 *dev);
  void Dealloc();

  static int MaxIndicesInOneSection(const Shape &src);

  static int IndicesNeededTotal(const Shape &src, int nInstances);
  static int IndicesNeededStrippedTotal(const Shape &src, int nInstances);
  bool AddShape(const Shape &src, int vOffset, IndexAllocXB *&iIndex, int indicesTotal, bool dynamic, int nInstances);
  bool AddShapeStripped(const Shape &src, IndexAllocXB *&iIndex, int indicesTotal, bool dynamic, int nInstances);
  bool AddShapeSprite(const Shape &src, IndexAllocXB *&iIndex, int indicesTotal, bool dynamic, int nInstances);
  bool AddShapePoint(const Shape &src, IndexAllocXB *&iIndex, int indicesTotal, bool dynamic, int nInstances);
  void RemoveShape(IndexAllocXB *iIndex);

  /// true when buffer is used and locking it would block
  bool IsBusy() const {return false;}
  /// true when buffer is used and locking it would block
  bool IsBusyForAdd() const {return false;}

  /// check if given buffer is empty and can be deallocated
  bool IsEmpty() const {return _nShapes==0;}
};

class VertexBufferD3DXB;

/**
If using custom allocator, vertex buffers can be allocated smaller than 4 KB.
VB coalescing is therefore not used, and no size rounding is needed.
*/
#define CUSTOM_VB_ALLOCATOR 1

class VertexStaticDataXB: public RefCountWithLinks
{
  friend class EngineDDXB;
  friend class PushBufferXB;

private:

  #if CUSTOM_VB_ALLOCATOR
    /// handle to physical memory
    PhysMemHeapItemXB _mem;
    IDirect3DVertexBuffer8 *_vBuffer;
  #else
    ComRef<IDirect3DVertexBuffer8> _vBuffer;
  #endif
  VertexHeapXB _manager;
  EngineDDXB *_engine;
  //! Size of one vertex in bytes
  int _vertexSize;

  int _nShapes;

  /// last fence used for whole buffer
  DWORD _fence;
  
  /// last fence used for parts of the buffer which are already unused
  DWORD _fenceReleased;

public:
  VertexStaticDataXB();
  ~VertexStaticDataXB();

  void Init(EngineDDXB *engine, int vertices, int vertexSize, IDirect3DDevice8 *dev);
  void Dealloc();

  /// true when buffer is used and locking it would block
  bool IsBusy() const;
  /// true when released parts of buffer are still used
  bool IsBusyForAdd() const;

  bool AddShape(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic);
  bool AddShapeNormal(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic);
  bool AddShapeNormalAlpha(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic);
  bool AddShapeShadowVolume(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic);
  bool AddShapePoint(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic);
  bool AddShapeInstanced(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, int instancesCount);
  bool AddShapeNormalInstanced(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, int instancesCount);
  bool AddShapeShadowVolumeInstanced(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, int instancesCount);
  bool AddShapeSpriteInstanced(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, int instancesCount);
  bool AddShapeSkinned(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic);
  bool AddShapeNormalSkinned(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic);
  bool AddShapeShadowVolumeSkinned(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic);
  void RemoveShape(VertexHeapXB::HeapItem *vIndex);
  int Size() {return _manager.Size();};
  int VertexSize() {return _vertexSize;};

  /// check if given buffer is empty and can be deallocated
  bool IsEmpty() const {return _nShapes==0;}
  //! Fence setting
  void SetFence(DWORD fence)
  {
    _fence = fence;
  }
};

/// proxy class used to avoid necessity to declare VertexBufferD3D here
class VertexBufferD3DXBLRUItem: public LinkBidirWithFrameStore
{
  friend class EngineDDXB;
  /// if shape is destroyed, vertex buffer is always destroyed with it
  /**
  owner should call Detach(owner} before destruction
  this is verified in destructor
  */
  const Shape *_owner;

public:
  VertexBufferD3DXBLRUItem()
  {
    _owner = NULL;
  }
  ~VertexBufferD3DXBLRUItem()
  {
    DoAssert(_owner==NULL);
  }
  size_t GetMemoryControlled() const
  {
    return GetBufferSize();
  }

protected:
  void SetOwner(const Shape *owner){_owner = owner;}
  const Shape *GetOwner() const {return _owner;}

  virtual size_t GetBufferSize() const = NULL;
};

/// stencil shadow drawing mode
enum EShadowStateXB
{
  /// no value used and set
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
};

#define TEXTURE_STAGES 4


class TextBankD3DXB;
class PerfCounter;
class FrameBase;

/// query manager
class QueryId
{
  /// Xbox HW limit is D3DVISIBILITY_TEST_MAX - 16K, but we do not need that much
  enum {MaxQueries=512};
  /// check which queries are currently in use
  bool _used[MaxQueries];
  /// last one we have used - helps to find the next free one fast
  int _curUsed;

public:
  QueryId();

  int Open();
  void Close(int id);
};

#define MAX_CONSTANT_STAGES 2
struct PSConstRef
{
  D3DRENDERSTATETYPE _rs[MAX_CONSTANT_STAGES];
  int _count;
  PSConstRef()
  {
    _count = 0;
  }
};

#if _ENABLE_CHEATS
  #define SHADER_INFO_PRINTING 1
#else
  #define SHADER_INFO_PRINTING 0
#endif

#if SHADER_INFO_PRINTING
struct VSInfo
{
  const DWORD *_fragments[16];
  int _fragmentCount;
  int _icBeforeOptimalization;
  int _icAfterOptimalization;
  int _usedCount;
};
TypeIsSimple(VSInfo);
#endif

struct ShaderCacheId
{
  const DWORD *_fragments[16];
  int _fragmentsCount;
  
  ShaderCacheId()
  :_fragmentsCount(0)
  {
  }
  
  struct Creator
  {
    ShaderCacheId &sci;
    int i;
    
    Creator(ShaderCacheId &c):sci(c),i(0){}
    
    void Add(DWORD *shader){sci._fragments[i++] = shader;}
    ~Creator(){sci._fragmentsCount=i;}
  };
};

struct ShaderCacheItem : public LinkBidirWithFrameStore
{
  AutoArray<DWORD> _shader;
  size_t GetMemoryControlled() const
  {
    return /*sizeof(*this) + */_shader.GetMemoryAllocated(); 
  }
};

// ((1 << 4) - 1 + 15 + 1)
#define SC_LIGHTCOMBINATIONS 31

typedef ShaderCacheItem ShaderCacheItemWithLights[SC_LIGHTCOMBINATIONS];

TypeIsMovable(ShaderCacheId)

struct FMCList : public MemoryFreeOnDemandHelper
{
  FramedMemoryControlledList<ShaderCacheItem> _fmcl;
  virtual float Priority() const
  {
    return 0.05f;
  }
  virtual RString GetDebugName() const
  {
    return "VSCache";
  }
  virtual size_t MemoryControlled() const
  {
    return _fmcl.MemoryControlled();
  }
  virtual size_t FreeOneItem()
  {
    ShaderCacheItem *first = _fmcl.First();
    if (!first) return 0;
    size_t estSize = first->GetStoredMemoryControlled();
    first->_shader.Clear();
    _fmcl.Delete(first);
    return estSize;
  }
  virtual size_t MemoryControlledRecent() const
  {
    return _fmcl.MemoryControlledRecent();
  }
  virtual void MemoryControlledFrame()
  {
    _fmcl.Frame();
  }
};


#define MAX_INSTRUCTION_COUNT 136

//! D3D engine implementation
/*!
both HW and SW T&L version are implemented
*/

class EngineDDXB: public Engine, public MemoryFreeOnDemandHelper
{
  friend class XBVisibilityQuery;
  friend class PushBufferXB;

  typedef Engine base;

protected:
  // data members
  int _w,_h; // back buffer dimensions

  int _pixelSize; // 16 or 32 bit mode?
  int _depthBpp; // 16 or 32 bit mode?
  int _refreshRate;
  int _presentationInterval;
  int _defPresentationInterval;

  D3DPRESENT_PARAMETERS _pp;
  QueryId _ids;
  
  enum {NBrightQueries=16};
  Ref<IVisibilityQuery> _brightQuery[NBrightQueries];
  //@{ eye accommodation based on image space measurements
  float _lastKnownAvgIntensity;
  float _lastKnownAvgIntensityAccomFactor; // we need to know what accommodation value was used
  //@}
  
  /// refcount for handling nested Begin/EndVisiblityTest
  int _visTestCount;

  HWND _hwndApp;

  DWORD _shaderHandle;
  DWORD _shaderNonTLHandle;
  DWORD _shaderNonTLTex4Handle;

  int _bias; // z-buffer bias
  float _matC, _matD; // w-buffer limits set
  bool _clipANearEnabled,_clipAFarEnabled;
  Plane _clipANear, _clipAFar; // additional clipping set

  int _minGuardX; // guard band clipping properties
  int _maxGuardX;
  int _minGuardY;
  int _maxGuardY;

  bool _useDXTL;
  bool _useHWTL;
  bool _useWBuffer;
  bool _userEnabledWBuffer;
  bool _windowed;
  bool _resetNeeded;

protected:
  TextBankD3DXB *_textBank;
  TLVertexTable *_mesh; // mesh data used during rendering
  //const VertexTable *_sMesh;

  // source mesh for T&L
  //VertexBuffer *_sBuffer;


  // this class provides all functionality common for window/full screen versions
protected:
  // members used for Direct3D only
  ComRef<IDirect3D8> _direct3D;
  ComRef<IDirect3DDevice8> _d3DDevice;

  //! Flag to determine whether HW is in defined state or not
  bool _hwInDefinedState;
  //! Material defined to be used in SetHWToDefinedState function
  RefR<TexMaterial> _definedMat;
  //! Old material defined to be used in SetHWToDefinedState function
  TLMaterial _definedMatMod;

  int _lastSpec;
  const TexMaterial *_lastMat;
  int _prepSpec;
  /// we need to unbind the texture set by SetTexture once it is destroyed
  TextureD3DHandle _lastHandle[TEXTURE_STAGES];
  /// high-level texture caching
  RefR<TextureD3DXB> _lastTexture[TEXTURE_STAGES];

  //bool _stencilExclusionEnabled;
  EShadowStateXB _currentShadowStateSettings;
  bool _backCullDisabled;
  /// shadow receiver level - drawing objects
  int _shadowLevel;
  /// shadow receiver level needed for casted shadows
  int _shadowLevelNeeded;
  //! Always In Shadow flag
  //int _aisValue;
  /// adjustable params for all postprocess fx.
  float _postFxParam[NPFX][4];
  /// enabled flag for all postprocess fx.
  bool _postFxEnabled[NPFX];
  
  /// we can blur only far away from us
  bool _depthOfFieldFarOnly;
  //@{ texture 0 information
  Color _texture0MaxColor;
  Color _texture0AvgColor;
  //@}

  D3DMATRIX _texGenMatrix[TEXTURE_STAGES];
  UVPair _texGenScale;
  UVPair _texGenOffset;
  UVPair _newTexGenScale;
  UVPair _newTexGenOffset;
  bool _texGenScaleOrOffsetHasChanged;

  bool _tlActive;

  EMainLight _mainLight;
  EMainLight _currMainLight;

  EFogMode _fogMode;
  EFogMode _currFogMode;

  LinkArray<Light> _lights; // currently active lights
  bool _lightsHasChanged;

  TLMaterial _materialSet;
  int _materialSetSpec;

  #if CUSTOM_VB_ALLOCATOR
    IDirect3DVertexBuffer8 *_vBufferLast; // last SetStreamSource
  #else
    ComRef<IDirect3DVertexBuffer8> _vBufferLast; // last SetStreamSource
  #endif
  ComRef<IDirect3DIndexBuffer8> _iBufferLast; // last SetIndices
  int _vOffsetLast;
  VSDVersion _vsdVersionLast;
  int _vBufferSize;

  int _iOffset; // something must be added to index source in DIP

  // dynamic data for TL
  VertexDynamicDataXB _dynVBuffer;
  /// shared static vertex data
  RefArray<VertexStaticDataXB> _statVBuffer;
  /// shared static index data
  RefArray<IndexStaticDataXB> _statIBuffer;
  /// vertex data queued for deletion
  RefArray<VertexStaticDataXB> _statVBufferDelete;

  /// LRU list of vertex buffers - used to release buffer that were not used for a long time
  /** list of all shapes which have geometry loaded, but it can be released */
  FramedMemoryControlledList<VertexBufferD3DXBLRUItem> _vBufferLRU;

  //! how much memory is allocated in index buffers (in bytes)
  int _iBuffersAllocated;
  //! how much memory is allocated in vertex buffers (in bytes)
  int _vBuffersAllocated;

  //! how much memory is requested (needed) in index buffers (in bytes)
  int _iBuffersRequested;
  //! how much memory is requested (needed) in vertex buffers (in bytes)
  int _vBuffersRequested;
  
  /// how much physical memory was really used for _vBuffersAllocated
  int _vBuffersUsed;

  /// clamping for stage 0
  int _lastClamp;
  
  bool _enableReorder; // external hint - reorder enabled

  TexLoc _texLoc;

  bool _can565,_can88,_can8888;
  int _dxtFormats; // bit-field of DXT formats (1..5)

  bool _hasStencilBuffer:1;
  bool _canDetailTex:1;
  bool _canWBuffer:1;
  bool _canWBuffer32:1;


#if PIXEL_SHADERS
  int _vertexShaders; // pixel shader version
  int _pixelShaders; // pixel shader version
  bool _usePixelShaders;
  DWORD _pixelShader[NPixelShaderSpecular][NPixelShaderID];
  PSConstRef _constMaxColor[NPixelShaderSpecular][NPixelShaderID];
  PSConstRef _constAvgColor[NPixelShaderSpecular][NPixelShaderID];
  //PSConstRef _constRgbEyeCoef[NPixelShaderSpecular][NPixelShaderID];
  PSConstRef _constDiffuse[NPixelShaderSpecular][NPixelShaderID];
  PSConstRef _constDiffuseBack[NPixelShaderSpecular][NPixelShaderID];
  PSConstRef _constSpecular[NPixelShaderSpecular][NPixelShaderID];
  PixelShaderID _pixelShaderSel;
  PixelShaderSpecular _pixelShaderSpecularSel;
  PixelShaderID _currPixelShaderSel;
  PixelShaderSpecular _currPixelShaderSpecularSel;
#endif

  //! New vertex shader ID
  VertexShaderID _vertexShaderID;
  //! Current vertex shader ID
  VertexShaderID _currVertexShaderID;

#if VERTEX_SHADERS
  VertexShader _vertexShaderSel;
  DWORD _vertexShader[NVertexShaders];
#endif

  // Vertex shader section
#ifdef _XBOX
#if SHADER_INFO_PRINTING
  AutoArray<VSInfo> _vsInfo;
  int _vsInfoLastCount;
  InitVal<bool,false> _showNotCachedShader;
  InitVal<bool,false> _printNotCachedShader;
#endif

  //FramedMemoryControlledList<ShaderCacheItem> _shaderCacheLRU;
  FMCList _shaderCacheLRU;
  AutoArray<ShaderCacheId> _shaderCacheLightId;
  AutoArray<ShaderCacheId> _shaderCacheId;

  // we cannot use AutoArray, ShaderCacheItem has no copy constructor
  SRefArray<ShaderCacheItemWithLights> _shaderCacheLight;
  SRefArray<ShaderCacheItem> _shaderCache;

  AutoArray<DWORD> _shaderTokenArray;
  DWORD *_shader;
  DWORD *_shaderGetPosNorm;
  DWORD *_shaderGetInst;
  DWORD *_shaderLandShadow;
  DWORD *_shaderModShadow;
  DWORD *_shaderModInstShadow;
  DWORD *_shaderNormal;
  DWORD *_shaderNormalDiffuse;
  DWORD *_shaderNormalThrough;
  DWORD *_shaderShadowVolume;
  DWORD *_shaderPointO;
  DWORD *_shaderInstancing;
  DWORD *_shaderNormalInstancing;
  DWORD *_shaderInstancingModulate;
  DWORD *_shaderInstancingEmissive;
  DWORD *_shaderNormalDiffuseInstancing;
  DWORD *_shaderNormalThroughInstancing;
  DWORD *_shaderShadowVolumeInstancing;
  DWORD *_shaderSpriteInstancing;
  DWORD *_shaderSkinning;
  DWORD *_shaderNormalSkinning;
  DWORD *_shaderNormalDiffuseSkinning;
  DWORD *_shaderNormalThroughSkinning;
  DWORD *_shaderShadowVolumeSkinning;
  DWORD *_shaderWater;
  DWORD *_shaderWaterSimple;
  DWORD *_shaderDirectional;
  DWORD *_shaderDirectionalADForced;
  DWORD *_shaderDirectionalNoSpec;
  DWORD *_shaderDirectionalSky;
  DWORD *_shaderDirectionalStars;
  DWORD *_shaderDirectionalSunObject;
  DWORD *_shaderDirectionalSunHaloObject;
  DWORD *_shaderDirectionalMoonObject;
  DWORD *_shaderDirectionalMoonHaloObject;
  DWORD *_shaderAlphaShadow;
  DWORD *_shaderFog;
  DWORD *_shaderFogAlpha;
  DWORD *_shaderFogNone;
  static const int maxPSLights = 4;
  static const int maxPointLights = 4;
  static const int maxSpotLights = 4;
  DWORD *_shaderPoint[maxPointLights];
  DWORD *_shaderSpot[maxSpotLights];
  DWORD *_shaderWriteLights;
  DWORD *_shaderWriteLightsColorMod;
  DWORD *_shaderWriteLightsAlphaMod;
#endif

  bool _isWorldMatrixChanged;
  Matrix4 _world;
  Matrix4 _invWorld;

  //Vector3 _posScale;
  //Vector3 _posOffset;

  //Color _diffuse;
  //Color _specular;
  //Color _a_dforced_e;

  Color _matAmbient;
  Color _matDiffuse;
  Color _matSpecular;
  Color _matForcedDiffuse;
  //bool _matValuesHasChanged;

  Color _modColor;


  ESkinningType _skinningType;
  ESkinningType _currSkinningType;
  bool _currInstancing;
  //bool _currSunEnabled;

  //! Array of bones
  AutoArray<Matrix4> _bones;
  //! Flag to determine whether bones have changed since the last DrawSectionTL
  bool _bonesHaveChanged;
  //! Determines whether bones were posted directly to constants or are stored in the _bones array
  bool _bonesWereSetDirectly;
  //! Current index of a bone (to prevent setting of constants between sections)
  int _currMinBoneIndex;

  //@( wanted and current values of misc. rendering flags
  bool _alphaShadowEnabled;
  bool _landShadowEnabled;
  bool _alphaInstancing;
  
  bool _currAlphaShadowEnabled;
  bool _currLandShadowEnabled;
  bool _currAlphaInstancing;
  //@}

  DWORD _shaderStencilVisualisationHandle;
  DWORD _psStencilVisualisation;
  //@{ postprocess pixel shaders
  DWORD _psPostNight;
  DWORD _psPostGlow;
  DWORD _psPostBrightQuery;
  DWORD _psPostShadowVisualisation;
  DWORD _psPostShadowVisualisationAIS;
  DWORD _psPostDepthOfField;
  DWORD _psPostDepthOfFieldFarOnly;
  DWORD _psPostDepthToA;
  DWORD _psPostCopy;
  //@}
  ComRef<IDirect3DVertexBuffer8> _vbStencilVisualisation;
  ComRef<IDirect3DVertexBuffer8> _vbMinMaxCube;

  bool _recording;
public:

  PushBufferXB *_activePushBuffer;

  //! Current lock of the push buffer
  DWORD _currPBLock;

protected:


  float _nightEye;

  // members used for both DirectDraw/Direct3D

  ComRef<IDirect3DSurface8> _backBuffer;
  ComRef<IDirect3DSurface8> _frontBuffer;
  //@{ temporary used for post-processing effects
  /** it is a member in order to guarantee lifetime
  It has no real existence - once device is destroyed, it is destroyed as well.
  */
  IDirect3DTexture8 _backBufferTexture;
  IDirect3DTexture8 _depthBufferTexture;
  //@}
  /// post-processing temporary buffer - 4x scaled down from _postFxBuffer
  ComRef<IDirect3DTexture8> _postFxBuffer;
  /// histogram acquisition buffer - 4x scaled down from _postFxBuffer
  ComRef<IDirect3DTexture8> _brightBuffer;
#ifdef _XBOX
  // 64x64 DXT1 surface for save
  ComRef<D3DSurface> _screenshot;
#endif

  QueueXB _queueNo;

  enum RenderMode {RMLines,RMTris,RM2DLines,RM2DTris};
  RenderMode _renderMode;

  float _gamma;

  bool _flippable; // front/back buffer can be flipped

  bool _d3dFrameOpen; // FinishFrame is necessary

  Color _textColor;

  //enum VFormatSet {SingleTex,DetailTex};
  //VFormatSet _formatSet;


private:
  //! Uses _world and _invWorld matrices to transform and normalize specified position to object-space
  Vector3 TransformAndNormalizePositionToObjectSpace(Vector3Par position);
  void EnablePointSampling(int filter, bool optimize);
  void SetMultiTexturingTextureDetail(int stageIndex, TextureD3DXB *tex);
  void SetMultiTexturing(TextureD3DXB **tex, int count);
  void SetTexture(TextureD3DXB *tex, const TexMaterial *mat, int specFlags);
  void SetTextureDetail(int stageIndex, const TexMaterial *mat);

  void SetTexture0Color(ColorVal maxColor, ColorVal avgColor, bool optimize);
  void SetTexture0(TextureD3DXB *tex);

public:
  float GetNightEye() {return _nightEye;}
  void SetHWToDefinedState();
  bool HWInDefinedState() const {return _hwInDefinedState;}
  virtual void LazySetHWToDefinedState()
  {
    DoAssert(_modColor == Color(GetHDRFactor(), GetHDRFactor(), GetHDRFactor()));
    if (!HWInDefinedState())
    {
      SetHWToDefinedState();
    }
  }
  // properties
  RString GetDebugName() const;
  int Width() const {return _w;}
  int Height() const {return _h;}
  int PixelSize() const {return _pixelSize;} // 16 or 32 bit mode?
  int RefreshRate() const {return _pp.FullScreen_RefreshRateInHz;}
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

  void InitVRAMPixelFormat( D3DFORMAT &pf, PacFormat format, bool enableDXT );

protected:
  void WorkToBack();
  void BackToFront(int wantedDurationVsync);
  //! Sets the modulate color
  virtual void SetModulateColor(const Color &modColor) {_modColor = modColor;}
  /// final post process effects - depth information may be lost
  void Postprocess();
  /// postprocess effects which require depth information
  void PostprocessDepth();
  //@{ prepare everything but pixel shader/stencil op
  void PreparePostprocess();
  void CleanUpPostprocess();
  //@}
  //@{ prepare/cleanup depth buffer surface
  bool PreparePostprocessDepth();
  void CleanUpPostprocessDepth();
  //@}

  //@{ prepare/cleanup work buffer surface
  void PreparePostprocessWorkBuffer();
  void CleanUpPostprocessWorkBuffer();
  //@}

  //@{ default (empty} additional textures setup/cleanup
  void PreparePostTexturesDefault();
  void CleanupPostTexturesDefault();
  //@}

  //@{ PostprocessFx helpers for blur
  void PreparePostTexturesBlur();
  void CleanupPostTexturesBlur();
  //@}
  //@{ PostprocessFx helpers for glow
  void PreparePostTexturesGlow();
  void CleanupPostTexturesGlow();
  //@}

  /// HDR glow posteffect
  void PostprocessGlow();
  /// night b/w vision posteffect
  void PostprocessNight();
  /// camera depth of field posteffect
  void PostprocessDepthOfField();

  void DoPostprocessShadow();

  /// do postprocess assuming everything is set up
  void DoPostprocess(int nPixels, float scale, float zValue=0.5f);
  /// do postprocess, use work. buffer UV in stages 3 and 4
  void DoPostprocessWorkBuff();

  /// do postprocess, use texture in 1..3 stages with offset coordinates
  void DoPostprocessGlow(int nPixels, float scale, float zValue);
  /// do postprocess, use texture in 2..3 stages with offset coordinates
  void DoPostprocessBlur(int nPixels, float scale);

  void PostprocessFx(
    int pixelShader, int fxPars, int nPixels, float scale,
    void (EngineDDXB::*doPostprocess)(int nPixels, float scale, float zValue)=EngineDDXB::DoPostprocess,
    void (EngineDDXB::*prepareTextures)()=EngineDDXB::PreparePostTexturesDefault,
    void (EngineDDXB::*cleanupTextures)()=EngineDDXB::CleanupPostTexturesDefault
  );

  //! Post-processing step - shadow visualisation
  void PostprocessShadowVisualization(bool interior);
  void PostprocessShadowVisualizationAIS();

  //! Determine we can use new shadow system (controlled by alpha)
  bool CanUseNewShadows();

  void CreateD3D();
  void DestroyD3D();
  void SearchMonitor(int &x, int &y, int &w, int &h);

public:
  // constructor - init all pointers to NULL

  void CreateSurfaces(bool windowed);

public:
  EngineDDXB
    (
    HINSTANCE hInst, HINSTANCE hPrev, int sw,
    int x, int y, int width, int height, bool windowed, int bpp
    );
  ~EngineDDXB();

  virtual void CreateAllPushBuffers();
  virtual void Init();

  enum
  {
    MaxSkinningBones=(CONSTANTS_OFFSET - BONES_START) / 3, // 3 constants per bones
    MaxInstances=(CONSTANTS_OFFSET - BONES_START) / 4, // 4 constants per instance
    MaxInstancesSprite=(CONSTANTS_OFFSET - BONES_START) / 2, // 2 constants per instance
    MaxInstancesShadowVolume=(CONSTANTS_OFFSET - BONES_START) / 3 // 2 constants per instance
  };

  bool InitDrawDone();
  bool IsSkinningSupported() {return true;};
  int GetMaxBonesCount() {return MaxSkinningBones;};

  virtual bool CanSeaUVMapping() const {return true;}
  virtual bool CanSeaWaves() const {return true;}
  virtual void SetWaterDepth(
    const WaterDepthQuad *water, int xRange, int zRange, float grid,
    float seaLevel,
    float shoreTop, float shoreBottom, float peakWaveTop, float peakWaveBottom
  );
  virtual void SetLandShadow(
    const float *shadow, int xRange, int zRange, float grid
  );
  
  void SetSkinningType(ESkinningType skinningType, const Matrix4 *bones = NULL, int nBones = 0);
  ESkinningType GetSkinningType() const {return _skinningType;}
  void SetPosScaleAndOffset(const Vector3 &scale, const Vector3 &offset);
  void SetTexGenScaleAndOffset(const UVPair &scale, const UVPair &offset);
  void SetTreeCrown(const Vector3 &min, const Vector3 &max);

  virtual int GetMaxPSLightsCount() const {return maxPSLights;}

  AbilityToDraw IsAbleToDraw();
  bool IsAbleToDrawCheckOnly();
  virtual void RecalcAccomodation();
  void InitDraw( bool clear=false, Color color = HBlack ); // Begin scene
  void FinishDraw(int wantedDurationVSync, bool allBackBuffers=false);

  virtual void EnableDepthOfField(float focusDist, float blur, bool farOnly);

  float WaitBeforeFrameDrawing();
  void BackBufferToAllBuffers();
  void NextFrame(); // swap frames - get ready for next frame

  void Pause();
  void ClearStateCaches();
  void Restore();

  void PreReset(bool hard);
  void PostReset();

  bool Reset(); // use Reset to change setting / reset device
  bool ResetHard(); // destroy and create the device again

  bool SwitchRes(int w, int h, int bpp);
  bool SwitchRefreshRate(int refresh);
  bool SwitchWindowed(bool windowed);

  void ListResolutions(FindArray<ResolutionInfo> &ret); // result is zero terminated w,h pairs of valid resolutions
  void ListRefreshRates(FindArray<int> &ret); // result is zero terminated w,h pairs of valid resolutions

  // functions specific to Direct3D

  // functions for both DirectDraw/Direct3D
  void InitSurfaces();
  void DestroySurfaces();

  void AddIBuffersAllocated(int n);
  void AddVBuffersAllocated(int n);

  void ReportVBAlloc(int size);
  void ReportIBAlloc(int size);
  void ReportVBDealloc(int size);
  void ReportIBDealloc(int size);

  //IDirectDraw7 *GetDirectDraw() const {return _directDraw;}
  IDirect3D8 *GetDirect3D() const {return _direct3D;}
  IDirect3DDevice8 *GetDirect3DDevice() const {return _d3DDevice;}

  bool Can565() const {return _can565;}
  bool Can88() const {return _can88;}
  bool Can8888() const {return _can8888;}

  TexLoc GetTexLoc() const {return _texLoc;}

  int DXTSupport() const {return _dxtFormats;}
  bool CanDXT( int i ) const {return (_dxtFormats&(1<<i))!=0;}

public:

  bool GetHWTL() const {return _useHWTL;}

  void Clear( bool clearZ=true, bool clear=true, PackedColor color=PackedColor(0) );

  VertexBuffer *CreateVertexBuffer(const Shape &src, VBType type, bool frequent);
  int CompareBuffers(const Shape &s1, const Shape &s2);

  virtual bool IsPushBufferSupported() {return true;}
  virtual PushBuffer *CreatePushBuffer();
  virtual DrawCommand *CreatePushBuffer(Shape *shape, const LODShape *lodShape);
  virtual void CreateProgramLoaders(LODShape *ls);
  virtual bool PreloadProgramLoaderData(FileRequestPriority prior, const char *name);
  virtual bool PreloadProgramLoaderPermanentLevels(FileRequestPriority prior, const char *name);

  //@{ PB creation/destruction encapsulated
  PushBufferHandle CreateHWPushBuffer(DWORD size);
  void DestroyHWPushBuffer(PushBufferHandle &pb);
  //@}

  /// serialize push-buffer header/version info
  bool SerializeBinPBHeader(SerializeBinStream &f);

  void BeginRecording();
  void EndRecording();

  void BufferReleased(VertexBufferD3DXB *buf);
  void OnSizeChanged(VertexBufferD3DXB *buf);

  // access to _statVBuffer
  VertexStaticDataXB *AddShapeVertices(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate);
  VertexStaticDataXB *AddShapeVerticesNormal(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate);
  VertexStaticDataXB *AddShapeVerticesNormalAlpha(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate);
  VertexStaticDataXB *AddShapeVerticesShadowVolume(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate);
  VertexStaticDataXB *AddShapeVerticesPoint(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate);
  VertexStaticDataXB *AddShapeVerticesInstanced(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate, int instancesCount);
  VertexStaticDataXB *AddShapeVerticesNormalInstanced(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate, int instancesCount);
  VertexStaticDataXB *AddShapeVerticesShadowVolumeInstanced(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate, int instancesCount);
  VertexStaticDataXB *AddShapeVerticesSpriteInstanced(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate, int instancesCount);
  VertexStaticDataXB *AddShapeVerticesSkinned(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate);
  VertexStaticDataXB *AddShapeVerticesNormalSkinned(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate);
  VertexStaticDataXB *AddShapeVerticesShadowVolumeSkinned(const Shape &src, VertexBufferD3DXB &vbuf, bool dynamic, bool separate);
  IndexStaticDataXB *AddShapeIndices(const Shape &src, int vOffset, IndexAllocXB *&iIndex, bool dynamic, int nInstances);
  IndexStaticDataXB *AddShapeIndicesSprite(const Shape &src, IndexAllocXB *&iIndex, bool dynamic, int nInstances);
  IndexStaticDataXB *AddShapeIndicesPoint(const Shape &src, IndexAllocXB *&iIndex, bool dynamic, int nInstances);

  void DeleteVB(VertexStaticDataXB *vb);
  void DeleteIB(IndexStaticDataXB *ib);

  int FrameTime() const;
  int AFrameTime() const {return FrameTime();}

  void DoSetGamma();
  void SetGamma( float gamma );
  float GetGamma() const {return _gamma;}
  float GetDefaultGamma() const {return 1.0f / 0.90f;}
  virtual float GetDefaultBrightness() const {return 1.25f;}

  virtual float GetHDRFactor() const;
  virtual float GetPreHDRBrightness() const;
  virtual float GetPostHDRBrightness() const;

  void LoadConfig(ParamEntryPar cfg,ParamEntryPar fcfg);
  void SaveConfig(ParamFile &cfg);

  void EnableReorderQueues( bool enableReorded );
  void FlushQueues();

  virtual void SetPostFxEnabled(int value);
  virtual void SetPostFxParam(int fx, const float *param);
  virtual void SetShadowLevel( int value );
  virtual void SetShadowLevelNeeded( int value );

  int GetStencilRefValue() const;

  virtual void GetZCoefs(float &zAdd, float &zMult);
  void SetBias( int bias );
  int GetBias(){return _bias;}

  bool CanZBias() const;
  bool ZBiasExclusion() const {return !_hasStencilBuffer;}

  AbstractTextBank *TextBank();
  TextBankD3DXB *TextBankDD() const {return _textBank;}

  void FlushMemory(bool deep);
  void CreateTextBank(ParamEntryPar cfg,ParamEntryPar fcfg);
  void ReportGRAM(const char *name);

  void Screenshot(RString filename);

protected: // D3D helpers

  enum {MaxStages=8};
  struct StateInfo
  {
    DWORD value;
    StateInfo( DWORD v=-1 ):value(v){}
    ClassIsMovable(StateInfo);
  };
  typedef StateInfo TextureStageStateInfo;
  typedef StateInfo RenderStateInfo;
  AutoArray<TextureStageStateInfo> _textureStageState[MaxStages];
  AutoArray<RenderStateInfo> _renderState;
  void ShadowsStatesSettings(EShadowStateXB ss, bool noBackCull)
  {
    if (ss == _currentShadowStateSettings && _backCullDisabled==noBackCull) return;
    DoShadowsStatesSettings(ss, true, noBackCull);
  }
  void DoShadowsStatesSettings(EShadowStateXB ss, bool optimize, bool noBackCull=false);

  void ChangeClipPlanes();
  void ChangeWDepth(float matC, float matD);
  void PrepareDetailTex(int spec, const TexMaterial *mat, const EngineShapeProperties &prop);
  void PrepareSingleTexDiffuseA(int spec);

  void DoEnableDetailTexGen(
    const D3DMATRIX *matrix0, const D3DMATRIX *matrix1, const D3DMATRIX *matrix2,
    bool matrix0UVCompressed, bool matrix1UVCompressed, bool matrix2UVCompressed,
    bool optimize
  );

#if _DEBUG
#define RS_DIAGS 1
#else
#define RS_DIAGS 0
#endif
#if RS_DIAGS
  HRESULT D3DSetTextureStageStateName
    (
    DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD value, const char *text,
    bool optimize=true
    );
  HRESULT D3DSetRenderStateName
    (
    D3DRENDERSTATETYPE state, DWORD value, const char *text,
    bool optimize=true
    );
#define D3DSetRenderState(state,value) \
  D3DSetRenderStateName(state,value,#state,true)
#define D3DSetTextureStageState(stage,state,value) \
  D3DSetTextureStageStateName(stage,state,value,#state,true)
#define D3DSetRenderStateOpt(state,value,optimize) \
  D3DSetRenderStateName(state,value,#state,optimize)
#define D3DSetTextureStageStateOpt(stage,state,value,optimize) \
  D3DSetTextureStageStateName(stage,state,value,#state,optimize)
#else
  HRESULT D3DSetTextureStageState
    (
    DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD value, bool optimize=true
    );
  HRESULT D3DSetRenderState
    (
    D3DRENDERSTATETYPE state, DWORD value, bool optimize=true
    );    
#define D3DSetRenderStateOpt D3DSetRenderState
#define D3DSetTextureStageStateOpt D3DSetTextureStageState
#endif

  WORD *QueueAdd( QueueXB &queue, int n );
  void QueueFan( const VertexIndex *ii, int n );

  void Queue2DPoly( const TLVertex *v, int n );

  void FlushQueue(QueueXB &queue, int index);
  void FlushAndFreeQueue(QueueXB &queue, int index);

  int AllocateQueue(QueueXB &queue, TextureD3DXB *tex, int level, const TexMaterial *mat, int spec);
  void FreeQueue(QueueXB &queue, int index);
  void FreeAllQueues(QueueXB &queue);
  void FlushAndFreeAllQueues(QueueXB &queue, bool nonEmptyOnly=false);
  void FlushAllQueues(QueueXB &queue, int skip=-1);

  void CloseAllQueues(QueueXB &queue);

  //void FlushQueue(){FlushQueue(_queue,index;
  //void Flush2DQueue();

  float GetPushBufferUsage() const;

  void SwitchRenderMode( RenderMode mode )
  {
    if (_renderMode==mode) return;
    DoSwitchRenderMode(mode);
  }
  void DoSwitchRenderMode( RenderMode mode );

  void DrawPoints( int beg, int end );
  void DrawLine( int beg, int end );

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

  virtual void DrawPoly3D(Matrix4Par space, const MipInfo &mip, const Vertex2DFloat *vertices, int nVertices, const Rect2DFloat &clip, int specFlags, int matType);

  void DoPrepareTriangle( const MipInfo &absMip, const TexMaterial *mat, int specFlags, const EngineShapeProperties &prop);
  void QueuePrepareTriangle( const MipInfo &absMip, const TexMaterial *mat, int specFlags );
  void DoPrepareTriangle(
    TextureD3DXB *tex, const TexMaterial *mat, int level, int spec, const EngineShapeProperties &prop
  );

  void PrepareTriangle( const MipInfo &absMip, const TexMaterial *mat, int specFlags );
  void PrepareTriangleTL(
    const MipInfo &mip, const TexMaterialLODInfo &mat, int specFlags,
    const TLMaterial &tlMat, const LightList &lights,
    int spec, const EngineShapeProperties &prop
  );

  void DrawPolygon(const VertexIndex *ii, int n);
  void DrawSection(const FaceArray &face, Offset beg, Offset end);


  // integrated transform&lighting
  bool GetTL() const {return _useDXTL;}
  bool HasWBuffer() const {return _useWBuffer;}

  // engine w-buffer interface
  virtual bool IsWBuffer() const;
  virtual bool CanWBuffer() const;
  virtual void SetWBuffer(bool val);

  void SetupLights(const TLMaterial &mat, const LightList &lights, int spec);
  void SetLights(const LightList &lights, int spec, bool fakeLightRelocation = false);
  void DoSwitchTL( bool active );

  void DiscardVB();
  bool AddVertices( const TLVertex *v, int n );

public:
  /// helper: used from VertexBufferD3D::Update
  void SetVSourceStatic(const Shape &src, const EngineShapeProperties &prop, const VertexBufferD3DXB &buf, VSDVersion vsdVersion);
  void SetVSourceDynamic(const Shape &src, const EngineShapeProperties &prop, const VertexBufferD3DXB &buf);

public:
  void SwitchTL( bool active )
  {
    if (active==_tlActive) return;
    DoSwitchTL(active);
  }
  void DoSetMaterial(
    const TLMaterial &mat, const LightList &lights, int spec,
    bool forceSpecular
  );
  //void SetMaterial(const TLMaterial &mat, const LightList &lights, int spec);
  void DoSetMaterial2D()
  {
    SelectPixelShaderSpecular(PSSNormal);
  }
  void ProjectionChanged();
  void EnableSunLight(EMainLight mainLight);
  void SetFogMode(EFogMode fogMode);
  void BeginMeshTLInstanced(const Shape &sMesh, const EngineShapeProperties &prop, int spec, const LightList &lights, int bias, const DrawParameters &dp);
  virtual void SetupInstances(const ObjectInstanceInfo *instances, int nInstances);
  virtual void SetupInstancesSprite(const ObjectInstanceInfo *instances, int nInstances);
  virtual void DrawMeshTLInstanced(
    const Shape &sMesh, const SectionMaterialLODs &matLOD, const EngineShapeProperties &prop, int spec, const LightList &lights, int bias,
    const ObjectInstanceInfo *instances, int nInstances, float minDist2, bool useColors, const DrawParameters &dp
    );
  void SetupShadowVolumeInstances(const ObjectInstanceInfo *instances, int nInstances);
  virtual void DrawShadowVolumeInstanced(LODShape *shape, int level, const ObjectInstanceInfo *instances, int instanceCount);

  int AddLight(Light *light);
  void ClearLights();

  virtual void BeginShadowVolume(float shadowVolumeSize);
  int ShapePropertiesNeeded(const TexMaterial *mat) const;
  void BeginInstanceTL(const Matrix4 &modelToWorld, const DrawParameters &dp);
  void BeginMeshTL( const Shape &sMesh, const EngineShapeProperties &prop, int spec, const LightList &lights, int bias);
  void EndMeshTL( const Shape &sMesh ); // forget mesh
  void BeginVisibilityTesting();
  IVisibilityQuery *QueryVisibility(IVisibilityQuery *query, ClipFlags clipFlags, const Vector3 *minmax, const FrameBase &pos);
  void EndVisibilityTesting();
  virtual void DrawSectionTL(const Shape &sMesh, int beg, int end, int bias, int instancesOffset = 0, int instancesCount = 0);
  void DrawStencilShadows(bool interior, int quality);

  IVisibilityQuery *BegPixelCounting(IVisibilityQuery *query);
  void EndPixelCounting(IVisibilityQuery *query);

  //! Lazy setting of the shader
  __forceinline void SetupSectionTL(
    EVertexDecl decl, bool instancing, ESkinningType skinningType, VertexShaderID vertexShaderID, EMainLight mainLight, EFogMode fogMode,
    bool alphaShadowEnabled, bool landShadowEnabled, bool alphaInstancing,
    int triangleCount = 0
  )
  {
    // Don't do anything in case nothing has changed
    if (
  #if SHADER_INFO_PRINTING
      _showNotCachedShader || _printNotCachedShader ||
  #endif
      _currVertexShaderID != vertexShaderID ||
      _lightsHasChanged ||
      _currSkinningType != skinningType ||
      _currInstancing != instancing ||
      _currMainLight != mainLight ||
      _currFogMode != fogMode ||
      _currAlphaShadowEnabled != alphaShadowEnabled ||
      _currLandShadowEnabled != landShadowEnabled ||
      _currAlphaInstancing!= alphaInstancing
    )
    {
      DoSetupSectionTL(
        decl, instancing, skinningType, vertexShaderID, mainLight, fogMode,
        alphaShadowEnabled, landShadowEnabled, alphaInstancing, triangleCount
      );
    }
  };
  //! Setting of the shader
  void DoSetupSectionTL(
    EVertexDecl decl, bool instancing, ESkinningType skinningType, VertexShaderID vertexShaderID, EMainLight mainLight, EFogMode fogMode,
    bool alphaShadowEnabled, bool landShadowEnabled, bool alphaInstancing, 
    int triangleCount
  );
  //! Sets the PS constants
  void SetupPSConstants();

  void SetupModelSpaceRendering(const Matrix4 &trans);
  void SetupPointSpotConstants();
  __forceinline void SetupNonFixedConstants()
  {
    /*
    if ((_matValuesHasChanged) && (_lights.Size() > 0))
    {
    SetupPointSpotConstants();
    }
    */
  };

  void DrawDecal
    (
    Vector3Par screen, float rhw, float sizeX, float sizeY, PackedColor color,
    const MipInfo &mip, int specFlags
    );
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

  void D3DDrawTexts();

  void D3DConstruct(ParamEntryPar cfg,ParamEntryPar fcfg);
  void D3DDestruct();
  void TextureDestroyed( Texture *tex );

  void D3DTextureDestroyed( TextureD3DHandle tex );

  void D3DCloseDraw();
  void D3DBeginScene();
  void D3DEndScene();

  void AddFragmentNames(char *s, const DWORD * const *fragments, int fragmentCount);

public:
  void CreateVB();
  void DestroyVB();

  void CreateVBTL();
  void DestroyVBTL();

  void RestoreVB();

#if _ENABLE_CHEATS
  RString GetStat(int statId, RString &statVal, RString &statVal2);
#endif
  bool CanRestore();
  void FogColorChanged( ColorVal fogColor )
  {
    SetD3DFog(fogColor);
  }
  virtual void EnableNightEye(float night);

  virtual void EnableAlphaShadow(bool enable)
  {
    _alphaShadowEnabled = enable;
  }
  bool GetAlphaShadowEnabled() const {return _alphaShadowEnabled;}

  void PrepareMesh( int spec ); // prepare internal variables
  bool BeginMesh( TLVertexTable &mesh, int spec ); // convert all mesh vertices
  void EndMesh( TLVertexTable &mesh ); // forget mesh

  void InitVertexShaderCache();
#if VERTEX_SHADERS
  void InitVertexShaders();
  void DeinitVertexShaders();

  void SelectVertexShader(VertexShader mode)
  {
    if (_vertexShaderSel==mode) return;
    DoSelectVertexShader(mode);
  }

  void DoSelectVertexShader(VertexShader mode);
#endif

#if PIXEL_SHADERS

  void InitPixelShaders();
  void DeinitPixelShaders();
  bool UsingPixelShaders() const {return _usePixelShaders;}

  void SelectPixelShaderSpecular(PixelShaderSpecular spec)
  {
    _pixelShaderSpecularSel = spec;
  }

  void SelectPixelShader(PixelShaderID ps)
  {
    _pixelShaderSel = ps;
  }
  void DoSelectPixelShader();
#else

  bool UsingPixelShaders() const {return false;}

#endif

  void Init3DState();
  void FindMode(int adapter); // prepare presentation parameters based on _w, _h, _pixelSize
  void Init3D();
  void Done3D();
  void SetD3DFog( ColorVal fog );
  void EnablePerfCap(bool perfCap);
  void DoPerfCap(bool perfCap, bool slowFrame);
  void SetDebugMarker(int id);
  int ProfileBeginGraphScope(unsigned int color,const char *name) const;
  int ProfileEndGraphScope() const;

  virtual void SetShadowFactor(ColorVal shadowLA, ColorVal shadowLD);

  virtual void SetShadowIntensity(float intensity);

  float ZShadowEpsilon() const {return 0.01;}
  float ZRoadEpsilon() const {return 0.005;}

  float ObjMipmapCoef() const {return 1.5;}
  float LandMipmapCoef() const {return 1.0;}

  bool ShadowsFirst() const {return false;}
  bool SortByShape() const {return true;}

  //@{ implementation of MemoryFreeOnDemandHelper
  virtual float Priority() const;
  virtual size_t MemoryControlled() const;
  virtual size_t MemoryControlledRecent() const;
  virtual size_t FreeOneItem();
  virtual void MemoryControlledFrame();
  //@}

  //@{ video interface
  virtual VideoPlayback *VideoOpen(const char *path, float volume);
  virtual void VideoClose(VideoPlayback *video);
  virtual void VideoFrame(VideoPlayback *video);
  //@}

  virtual void PrepareScreenshot();
  virtual void CleanUpScreenshot();
  virtual bool WriteScreenshotToXPR(RString path);

  virtual void PersistDisplay();

#ifdef _XBOX
  enum {TrackFrames=16};
  /// duration of the frame as vsync count
  static int _frameDurationVSync[TrackFrames];
  /// sliding index into _frameStart
  static int _curFrame;
  /// last vsync when last know Swap callback was reported
  static int _lastSwapVSync;

  /// # of the last swap requested
  static int _lastSwapId;

  /// last missed vsync difference
  static int _lastKnownError;

  /// last vsync #
  static int _lastVSync;
  /// rdtsc time of last vsync #
  static int _lastVSyncTime;
  /// time in rdtsc between vsyncs
  static int _vSyncInterval;
  /// vsync D3D callback
  static void __cdecl VBlankCallback(D3DVBLANKDATA *pData);
  /// D3D swap done callback
  static void __cdecl SwapCallback(D3DSWAPDATA *pData);


public:
  virtual void ResetTimingHistory(int ms);
  virtual void ResetTimingVSync();
  virtual bool CanVSyncTime() const;

  virtual float GetAvgFrameDurationVsync() const;
  virtual int GetMaxFrameDurationVsync() const;
  
  virtual float GetLastKnownAvgIntensity(float &accomFactor) const;
  virtual void ForgetAvgIntensity();
 

  virtual float GetTimeVSync() const;
  virtual int GetPresentationIntervalVSync() const;
  virtual int GetLastFrameEndVSync() const;
  virtual int GetKnownErrorVsync() const;

  void SetPBLock(DWORD pbLock)
  {
    _currPBLock = pbLock;
  }
  DWORD GetPBLock()
  {
    return _currPBLock;
  }

private:
  /// query is not needed - the result is known, record it
  void VisibilityQueryResultKnown(XBVisibilityQuery *q, int pixels);
  /// query was started - start tracking
  void VisibilityQueryStarted(XBVisibilityQuery *q, int id);
#endif

};

#define GEngineDD static_cast<EngineDDXB *>(GEngine)

// XBOX / Win32 specific texture format definition

#ifdef _XBOX

#define tex_A1R5G5B5 D3DFMT_A1R5G5B5
#define tex_A4R4G4B4 D3DFMT_A4R4G4B4
#define tex_A8R8G8B8 D3DFMT_A8R8G8B8
#define tex_R5G6B5 D3DFMT_R5G6B5
#define tex_A8L8 D3DFMT_A8L8

#else

#define tex_A1R5G5B5 D3DFMT_A1R5G5B5
#define tex_A4R4G4B4 D3DFMT_A4R4G4B4
#define tex_A8R8G8B8 D3DFMT_A8R8G8B8
#define tex_R5G6B5 D3DFMT_R5G6B5
#define tex_A8L8 D3DFMT_A8L8

#endif

#endif // !_DISABLE_GUI

#endif
